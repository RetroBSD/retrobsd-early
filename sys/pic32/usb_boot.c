/*
 * USB HIT bootloader for PIC32 microcontroller.
 *
 * Based on Microchip sources.
 * Heavily rewritten by Serge Vakulenko, <serge@vak.ru>.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC(R) Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PIC Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#include <machine/usb_device.h>
#include <machine/usb_function_hid.h>
#include <machine/pic32mx.h>
#include <machine/io.h>

/*
 * Flash memory.
 */
#define FLASH_START         0x1D000000  /* physical */
#define FLASH_PROGRAM       0x1D005000  /* physical - beginning of application */
#define FLASH_BOOT          0x9D006000  /* virtual - start of application */

#define BYTES_PER_PAGE      4096

/*
 * Commands of bootloader protocol.
 */
#define	QUERY_DEVICE	    0x02    // Command that the host uses to learn
                                    // about the device (what regions can be
                                    // programmed, and what type of memory
                                    // is the region)
#define	UNLOCK_CONFIG	    0x03    // Note, this command is used for both
                                    // locking and unlocking the config bits
#define ERASE_DEVICE	    0x04    // Host sends this command to start
                                    // an erase operation.  Firmware controls
                                    // which pages should be erased.
#define PROGRAM_DEVICE	    0x05    // If host is going to send a full
                                    // REQUEST_SIZE to be programmed, it
                                    // uses this command.
#define	PROGRAM_COMPLETE    0x06    // If host send less than a REQUEST_SIZE
                                    // to be programmed, or if it wished
                                    // to program whatever was left in
                                    // the buffer, it uses this command.
#define GET_DATA	    0x07    // The host sends this command in order
                                    // to read out memory from the device.
                                    // Used during verify (and read/export
                                    // hex operations)
#define	RESET_DEVICE	    0x08    // Resets the microcontroller, so it can
                                    // update the config bits (if they were
                                    // programmed, and so as to leave
                                    // the bootloader (and potentially go back
                                    // into the main application)

/*
 * HID packet structure.
 */
#define	PACKET_SIZE	    64      // HID packet size
#define REQUEST_SIZE        56      // Number of bytes of a standard request

typedef union __attribute__ ((packed)) _USB_HID_BOOTLOADER_COMMAND
{
    unsigned char Contents [PACKET_SIZE];

    struct __attribute__ ((packed)) {
        unsigned char Command;
        unsigned AddressHigh;
        unsigned AddressLow;
        unsigned char Size;
        unsigned char PadBytes [PACKET_SIZE - 6 - REQUEST_SIZE];
        unsigned int Data [REQUEST_SIZE / sizeof(unsigned)];
    };

    struct __attribute__ ((packed)) {
        unsigned char Command;
        unsigned Address;
        unsigned char Size;
        unsigned char PadBytes [PACKET_SIZE - 6 - REQUEST_SIZE];
        unsigned int Data [REQUEST_SIZE / sizeof(unsigned)];
    };

    struct __attribute__ ((packed)) {
        unsigned char Command;
        unsigned char PacketDataFieldSize;
        unsigned char DeviceFamily;
        unsigned char Type1;
        unsigned long Address1;
        unsigned long Length1;
        unsigned char Type2;
        unsigned long Address2;
        unsigned long Length2;
        unsigned char Type3;		// End of sections list indicator goes here, when not programming the vectors, in that case fill with 0xFF.
        unsigned long Address3;
        unsigned long Length3;
        unsigned char Type4;		// End of sections list indicator goes here, fill with 0xFF.
        unsigned char ExtraPadBytes[33];
    };

    struct __attribute__ ((packed)) {   // For lock/unlock config command
        unsigned char Command;
        unsigned char LockValue;
    };
} packet_t;

static packet_t send;           // 64-byte send buffer (EP1 IN to the PC)
static packet_t receive;        // 64-byte receive buffer (EP1 OUT from the PC)
static packet_t receive_buffer;

static USB_HANDLE out_handle;
static USB_HANDLE in_handle;

/*
 * Placed at fixed address 0x80000000 by linker.
 * To enter boot mode from user program, set this value
 * to 0x12345678 and perform a software reset.
 */
unsigned int reset_key
     __attribute__((section(".sdata")));

static int packet_received;
static unsigned int buf [32];
static unsigned int buf_index;
static unsigned int base_address;

/*
 * Chip configuration.
 */
PIC32_DEVCFG (
    DEVCFG0_DEBUG_ENABLED,      /* ICE debugger disabled */

    DEVCFG1_FNOSC_PRIPLL |      /* Primary oscillator with PLL */
    DEVCFG1_POSCMOD_HS |        /* HS oscillator */
    DEVCFG1_FPBDIV_1 |          /* Peripheral bus clock = SYSCLK/1 */
    DEVCFG1_WDTPS_1,            /* Watchdog postscale = 1/1024 */

    DEVCFG2_FPLLIDIV_2 |        /* PLL divider = 1/2 */
    DEVCFG2_FPLLMUL_20 |        /* PLL multiplier = 20x */
    DEVCFG2_UPLLIDIV_2 |        /* USB PLL divider = 1/2 */
    DEVCFG2_FPLLODIV_1,         /* PLL postscaler = 1/1 */

    DEVCFG3_USERID(0xffff) |    /* User-defined ID */
    DEVCFG3_FSRSSEL_7 |         /* Assign irq priority 7 to shadow set */
    DEVCFG3_FETHIO);            /* Default Ethernet i/o pins */

/*
 * Boot code.
 */
asm ("          .section .startup,\"ax\",@progbits");
asm ("          .globl _start");
asm ("          .type _start, function");
asm ("_start:   la      $sp, _estack");
asm ("          la      $ra, main");
asm ("          la      $gp, _gp");
asm ("          jr      $ra");
asm ("          .text");

static void button_init()
{
#if defined (UBW32)
    TRISESET = 1 << 7;

#elif defined (MAXIMITE)
    TRISCSET = 1 << 13;

#elif defined (DIP)
    TRISGSET = 1 << 6;
#else
#error "Unknown board"
#endif
}

static int button_pressed()
{
#if defined (UBW32)
    return ! (PORTE & (1 << 7));

#elif defined (MAXIMITE)
    return ! (PORTC & (1 << 13));

#elif defined (DIP)
    return ! (PORTG & (1 << 6));
#else
#error "Unknown board"
#endif
}

static void led_init()
{
#if defined (UBW32)
    LATECLR = 0xF;
    TRISECLR = 0xF;

#elif defined (MAXIMITE)
    LATECLR = 2;
    TRISECLR = 2;
    LATFCLR = 1;
    TRISFCLR = 1;

#elif defined (DIP)
    LATECLR = 0xF0;
    TRISECLR = 0xF0;
#else
#error "Unknown board"
#endif
}

static void led_toggle()
{
#if defined (UBW32)
    PORTEINV = 1 << 3;

#elif defined (MAXIMITE)
    PORTEINV = 1 << 1;

#elif defined (DIP)
    PORTEINV = 1 << 7;
#else
#error "Unknown board"
#endif
}

/*
 * Reset the microrontroller.
 */
static void soft_reset()
{
    mips_intr_disable();

    if (! (DMACON & 0x1000)) {
        DMACONSET = 0x1000;
        while (DMACON & 0x800)
            continue;
    }
    SYSKEY = 0;
    SYSKEY = 0xaa996655;
    SYSKEY = 0x556699aa;

    RSWRSTSET = 1;
    (void) RSWRST;

    for (;;)
        continue;
}

/*
 * Microsecond delay routine for MIPS processor.
 */
static void udelay (unsigned usec)
{
    unsigned now = mips_read_c0_register (C0_COUNT, 0);
    unsigned final = now + usec * (CPU_KHZ / 1000);

    for (;;) {
        now = mips_read_c0_register (C0_COUNT, 0);

        /* This comparison is valid only when using a signed type. */
        if ((int) (now - final) >= 0)
            break;
    }
}

/*
 * BlinkUSBStatus turns on and off LEDs
 * corresponding to the USB device state.
 */
static void led_blink()
{
    static unsigned led_count = 0;

    if (led_count == 0)
        led_count = 10000U;
    led_count--;

    if (! usb_is_device_suspended() &&
        usb_device_state == CONFIGURED_STATE &&
        led_count == 0) {
            led_toggle();
    }
}

/*
 * Perform non-volatile memory operation.
 */
static void nvm_operation (unsigned op, unsigned address, unsigned data)
{
    int x;

    // Convert virtual address to physical
    NVMADDR = address & 0x1fffffff;
    NVMDATA = data;

    // Disable interrupts
    x = mips_intr_disable();

    // Enable Flash Write/Erase Operations
    NVMCON = PIC32_NVMCON_WREN | op;

    // Data sheet prescribes 6us delay for LVD to become stable.
    // To be on the safer side, we shall set 7us delay.
    udelay (7);

    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMCONSET = PIC32_NVMCON_WR;

    // Wait for WR bit to clear
    while (NVMCON & PIC32_NVMCON_WR)
        continue;

    // Disable Flash Write/Erase operations
    NVMCONCLR = PIC32_NVMCON_WREN;

    // Enable interrupts
    mips_intr_restore (x);
#if 0
    // Check error status
    if (NVMCON & (PIC32_NVMCON_WRERR | PIC32_NVMCON_LVDERR)) {
        TODO;
    }
#endif
}

/*
 * Use word writes to write code chunks less than a full 64 byte block size.
 */
static void write_flash_block()
{
    unsigned int i;

    for (i = 0; buf_index > 0; buf_index--) {
        // Write word to flash memory.
        nvm_operation (PIC32_NVMCON_WORD_PGM,
            base_address - buf_index * sizeof(unsigned),
            buf [i++]);
    }
}

/*
 * A command packet was received from PC.
 * Process it and send a reply.
 */
static void handle_packet()
{
    int i;

    switch (receive.Command) {
    case QUERY_DEVICE:
        // Prepare a response packet, which lets the PC software know
        // about the memory ranges of this device.

        send.Command = QUERY_DEVICE;
        send.PacketDataFieldSize = REQUEST_SIZE;
        send.DeviceFamily = 3;            /* PIC23 */

        send.Type1 = 1;                   /* program memory type */
        send.Address1 = FLASH_PROGRAM;
        send.Length1 = FLASH_START + BMXPFMSZ - FLASH_PROGRAM;

        send.Type2 = 0xFF;                /* end of list */

        if (! usb_handle_busy (in_handle)) {
            in_handle = usb_transfer_one_packet (HID_EP, IN_TO_HOST,
                (unsigned char*) &send, PACKET_SIZE);
            packet_received = 0;
        }
        break;

    case UNLOCK_CONFIG:
        packet_received = 0;
        break;

    case ERASE_DEVICE:
        for (i = 0; i < BMXPFMSZ/BYTES_PER_PAGE; i++) {
            /* Erase flash page */
            nvm_operation (PIC32_NVMCON_PAGE_ERASE,
                FLASH_PROGRAM + i * BYTES_PER_PAGE, 0);

            // Call this function periodically to prevent falling
            // off the bus if any SETUP packets should happen to arrive.
            usb_device_tasks();
        }
        // Good practice to clear WREN bit anytime we are
        // not expecting to do erase/write operations,
        // further reducing probability of accidental activation.
        NVMCONCLR = PIC32_NVMCON_WREN;
        packet_received = 0;
        break;

    case PROGRAM_DEVICE:
        if (base_address == 0)
            base_address = receive.Address;

        if (base_address == receive.Address) {
            for(i = 0; i < receive.Size/sizeof(unsigned); i++) {
                unsigned int index;

                index = (REQUEST_SIZE - receive.Size) / sizeof(unsigned) + i;

                // Data field is right justified.
                // Need to put it in the buffer left justified.
                buf [buf_index++] =
                    receive.Data [(REQUEST_SIZE - receive.Size) / sizeof(unsigned) + i];
                base_address += sizeof(unsigned);
                if (buf_index == REQUEST_SIZE / sizeof(unsigned)) {
                    write_flash_block();
                }
            }
        }
        // else host sent us a non-contiguous packet address...
        // to make this firmware simpler, host should not do this without
        // sending a PROGRAM_COMPLETE command in between program sections.
        packet_received = 0;
        break;

    case PROGRAM_COMPLETE:
        write_flash_block();

        // Reinitialize pointer to an invalid range, so we know the next
        // PROGRAM_DEVICE will be the start address of a contiguous section.
        base_address = 0;
        packet_received = 0;
        break;

    case GET_DATA:
        if (! usb_handle_busy (in_handle)) {
            send.Command = GET_DATA;
            send.Address = receive.Address;
            send.Size = receive.Size;

            for (i = 0; i < (receive.Size/sizeof(unsigned)); i++) {
                unsigned *p;
                unsigned data;

                p = ((unsigned*) ((receive.Address + (i * sizeof(unsigned))) | 0x80000000));
                data = *p;

                send.Data [REQUEST_SIZE/sizeof(unsigned) + i -
                    receive.Size/sizeof(unsigned)] =
                    *((unsigned*)((receive.Address + (i * sizeof(unsigned))) | 0x80000000));
            }

            in_handle = usb_transfer_one_packet (HID_EP,  IN_TO_HOST,
                (unsigned char*) &send.Contents[0], PACKET_SIZE);
            packet_received = 0;
        }
        break;

    case RESET_DEVICE:
        // Disable USB module
        U1CON = 0x0000;

        // And wait awhile for the USB cable capacitance to discharge
        // down to disconnected (SE0) state.   Otherwise host might not
        // realize we disconnected/reconnected when we do the reset.
        udelay (1000);

        soft_reset();
        break;
    }
}

/*
 * Main program entry point.
 */
int main()
{
    /*
     * Setup wait states.
     */
    CHECON = 2;
    BMXCONCLR = 0x40;
    CHECONSET = 0x30;

    /* Config register: enable kseg0 caching. */
    mips_write_c0_register (C0_CONFIG, 0,
        mips_read_c0_register (C0_CONFIG, 0) | 3);

    /*
     * To call the normal application (i.e. don't go into the bootloader)
     * we need to make sure that the PRG button is not pressed, and that we
     * don't have a software reset.
     */
    if (! button_pressed() &&
        (! (RCON & PIC32_RCON_SWR) || reset_key != 0x12345678))
    {
        // Must clear out software key
	reset_key = 0;

        // If there is NO real program to execute, then fall through to the bootloader
	if (*(int*) FLASH_BOOT != 0xFFFFFFFF)	{
            void (*fptr)(void) = (void (*)(void)) FLASH_BOOT;

            fptr();
            for (;;)
                continue;
	}
    }
    // Must clear out software key.
    reset_key = 0;
    RCONCLR = PIC32_RCON_SWR;

    /* Initialize all .bss variables by zeros. */
    extern unsigned __bss_start, __bss_end;
    unsigned *dest = &__bss_start;
    while (dest < &__bss_end)
            *dest++ = 0;

    // Initialize USB module SFRs and firmware variables to known states.
    usb_device_init();

    // Initialize all of the LED pins
    led_init();

    // Initialize all of the push buttons
    button_init();

    for (;;) {
	// Check bus status and service USB interrupts.
        usb_device_tasks();

        // Blink the LEDs according to the USB device status
        led_blink();

        // User Application USB tasks
        if (usb_device_state < CONFIGURED_STATE || usb_is_device_suspended())
            continue;

        if (packet_received) {
            handle_packet();
            continue;
        }

        // Are we done sending the last response.  We need to be before we
        // receive the next command because we clear the send buffer
        // once we receive a command
        if (usb_handle_busy (in_handle))
            continue;

        // Did we receive a command?
        if (usb_handle_busy (out_handle))
            continue;

        // Make a copy of received data.
        int i;
        for (i = 0; i < PACKET_SIZE; i++) {
            receive.Contents[i] = receive_buffer.Contents[i];
        }

        // Restart receiver, to be ready for a next packet.
        out_handle = usb_transfer_one_packet (HID_EP, OUT_FROM_HOST,
            (unsigned char*) &receive_buffer, PACKET_SIZE);

        // Prepare the next packet we will send to the host,
        // by initializing the entire packet to 0x00.
        for (i = 0; i < PACKET_SIZE; i++) {
            send.Contents[i] = 0;
        }
        packet_received = 1;
    }
}

/*
 * USB Callback Functions
 */
/*
 * Call back that is invoked when a USB suspend is detected.
 */
void usbcb_suspend()
{
    /* Empty. */
}

/*
 * This call back is invoked when a wakeup from USB suspend is detected.
 */
void usbcb_wake_from_suspend()
{
    /* Empty. */
}

/*
 * Called when start-of-frame packet arrives, every 1 ms.
 */
void usbcb_sof_handler()
{
    /* Empty. */
}

/*
 * Called on any USB error interrupt, for debugging purposes.
 */
void usbcb_error_handler()
{
    /* Empty. */
}

/*
 * Process device-specific SETUP requests.
 */
void usbcb_check_other_req()
{
    hid_check_request();
}

/*
 * Handle a SETUP SET_DESCRIPTOR request (optional).
 */
void usbcb_std_set_dsc_handler()
{
    /* Empty. */
}

/*
 * This function is called when the device becomes initialized.
 * It should initialize the endpoints for the device's usage
 * according to the current configuration.
 */
void usbcb_init_ep()
{
    // Enable the HID endpoint
    usb_enable_endpoint (HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED |
        USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);

    // Arm the OUT endpoint for the first packet
    out_handle = usb_rx_one_packet (HID_EP,
        &receive_buffer.Contents[0], PACKET_SIZE);
}

/*
 * The device descriptor.
 */
const USB_DEVICE_DESCRIPTOR usb_device = {
    sizeof (usb_device),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0
    0x04D8,                 // Vendor ID
    0x003C,                 // Product ID: USB HID Bootloader
    0x0002,                 // Device release number in BCD format
    1,                      // Manufacturer string index
    2,                      // Product string index
    0,                      // Device serial number string index
    1,                      // Number of possible configurations
};

/*
 * Configuration 1 descriptor
 */
const unsigned char usb_config1_descriptor[] = {
    /*
     * Configuration descriptor
     */
    sizeof (USB_CONFIGURATION_DESCRIPTOR),
    USB_DESCRIPTOR_CONFIGURATION,
    0x29, 0x00,             // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT | _SELF,       // Attributes
    50,                     // Max power consumption (2X mA)

    /*
     * Interface descriptor
     */
    sizeof (USB_INTERFACE_DESCRIPTOR),
    USB_DESCRIPTOR_INTERFACE,
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    HID_INTF,               // Class code
    0,                      // Subclass code
    0,                      // Protocol code
    0,                      // Interface string index

    /*
     * HID Class-Specific descriptor
     */
    sizeof (USB_HID_DSC) + 3,
    DSC_HID,
    0x11, 0x01,             // HID Spec Release Number in BCD format (1.11)
    0x00,                   // Country Code (0x00 for Not supported)
    HID_NUM_OF_DSC,         // Number of class descriptors
    DSC_RPT,                // Report descriptor type
    HID_RPT01_SIZE, 0x00,   // Size of the report descriptor

    /*
     * Endpoint descriptor
     */
    sizeof (USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT,
    HID_EP | _EP_IN,        // EndpointAddress
    _INTERRUPT,             // Attributes
    PACKET_SIZE, 0,         // Size
    1,                      // Interval

    /*
     * Endpoint descriptor
     */
    sizeof (USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT,
    HID_EP | _EP_OUT,       // EndpointAddress
    _INTERRUPT,             // Attributes
    PACKET_SIZE, 0,         // Size
    1,                      // Interval
};

/*
 * Class specific descriptor - HID
 */
const unsigned char hid_rpt01 [HID_RPT01_SIZE] = {
    0x06, 0x00, 0xFF,       // Usage Page = 0xFF00 (Vendor Defined Page 1)
    0x09, 0x01,             // Usage (Vendor Usage 1)
    0xA1, 0x01,             // Collection (Application)

    0x19, 0x01,             // Usage Minimum
    0x29, 0x40,             // Usage Maximum 64 input usages total (0x01 to 0x40)
    0x15, 0x00,             // Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x26, 0xFF, 0x00,       // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x75, 0x08,             // Report Size: 8-bit field size
    0x95, 0x40,             // Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item)
    0x81, 0x00,             // Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.

    0x19, 0x01,             // Usage Minimum
    0x29, 0x40,             // Usage Maximum 64 output usages total (0x01 to 0x40)
    0x91, 0x00,             // Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.

    0xC0,                   // End Collection
};

/*
 * USB Strings
 */
static const USB_STRING_INIT(1) string0_descriptor = {
    sizeof(string0_descriptor),
    USB_DESCRIPTOR_STRING,              /* Language code */
    { 0x0409 },
};

static const USB_STRING_INIT(25) string1_descriptor = {
    sizeof(string1_descriptor),
    USB_DESCRIPTOR_STRING,              /* Manufacturer */
    { 'M','i','c','r','o','c','h','i','p',' ',
      'T','e','c','h','n','o','l','o','g','y',
      ' ','I','n','c','.' },
};

static const USB_STRING_INIT(18) string2_descriptor = {
    sizeof(string2_descriptor),
    USB_DESCRIPTOR_STRING,              /* Product */
    { 'U','S','B',' ','H','I','D',' ','B','o',
      'o','t','l','o','a','d','e','r' },
};

/*
 * Array of configuration descriptors
 */
const unsigned char *const usb_config[] = {
    (const unsigned char *const) &usb_config1_descriptor,
};

/*
 * Array of string descriptors
 */
const unsigned char *const usb_string[USB_NUM_STRING_DESCRIPTORS] = {
    (const unsigned char *const) &string0_descriptor,
    (const unsigned char *const) &string1_descriptor,
    (const unsigned char *const) &string2_descriptor,
};