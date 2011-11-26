/*
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
#include <machine/pic32mx.h>
#include <machine/usb_device.h>
#include <machine/usb_function_hid.h>

#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config UPLLEN   = ON       	// USB PLL Enabled
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSECME        // Clock Switching & Fail Safe Clock Monitor enabled
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = ON          	// Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF           // Background Debugger Enable

#define FLASH_PAGE_SIZE 4096

//
// Beginning of application program memory (not occupied by bootloader).
// **THIS VALUE MUST BE ALIGNED WITH BLOCK BOUNDRY**
// Also, in order to work correctly, make sure the StartPageToErase
// is set to erase this section.
//
#define ProgramMemStart 0x1D005000
#define BootMemStart	0x9D006000

#define MaxPageToEraseNoConfigs     BMXPFMSZ/FLASH_PAGE_SIZE    // Last full page of flash on the PIC24FJ256GB110, which does not contain the flash configuration words.
#define MaxPageToEraseWithConfigs   BMXPFMSZ/FLASH_PAGE_SIZE    // Page 170 contains the flash configurations words on the PIC24FJ256GB110.  Page 170 is also smaller than the rest of the (1536 byte) pages.

#define ProgramMemStopNoConfigs	    0x1D000000 + BMXPFMSZ
#define ProgramMemStopWithConfigs   0x1D000000 + BMXPFMSZ

// Switch State Variable Choices
#define	QUERY_DEVICE	    0x02    // Command that the host uses to learn about the device (what regions can be programmed, and what type of memory is the region)
#define	UNLOCK_CONFIG	    0x03    // Note, this command is used for both locking and unlocking the config bits (see the "//Unlock Configs Command Definitions" below)
#define ERASE_DEVICE	    0x04    // Host sends this command to start an erase operation.  Firmware controls which pages should be erased.
#define PROGRAM_DEVICE	    0x05    // If host is going to send a full RequestDataBlockSize to be programmed, it uses this command.
#define	PROGRAM_COMPLETE    0x06    // If host send less than a RequestDataBlockSize to be programmed, or if it wished to program whatever was left in the buffer, it uses this command.
#define GET_DATA	    0x07    // The host sends this command in order to read out memory from the device.  Used during verify (and read/export hex operations)
#define	RESET_DEVICE	    0x08    // Resets the microcontroller, so it can update the config bits (if they were programmed, and so as to leave the bootloader (and potentially go back into the main application)

// Unlock Configs Command Definitions
#define UNLOCKCONFIG	    0x00    // Sub-command for the ERASE_DEVICE command
#define LOCKCONFIG	    0x01    // Sub-command for the ERASE_DEVICE command

// Query Device Response "Types"
#define	TypeProgramMemory   0x01    // When the host sends a QUERY_DEVICE command, need to respond by populating a list of valid memory regions that exist in the device (and should be programmed)
#define TypeEEPROM	    0x02
#define TypeConfigWords	    0x03
#define	TypeEndOfTypeList   0xFF    // Sort of serves as a "null terminator" like number, which denotes the end of the memory region list has been reached.

// BootState Variable States
#define	IdleState	    0x00
#define NotIdleState	    0x01

//OtherConstants
#define InvalidAddress	    0xFFFFFFFF

//Application and Microcontroller constants
#define DEVICE_FAMILY	    0x03    //0x01 for PIC18, 0x02 for PIC24, 0x03 for PIC23

#define	TotalPacketSize	    0x40
#define RequestDataBlockSize 56     // Number of bytes in the "Data" field of a standard request to/from the PC.  Must be an even number from 2 to 56.
#define BufferSize 	    0x20    // 32 16-bit words of buffer

int Switch2IsPressed(void);
int Switch3IsPressed(void);
void Emulate_Mouse(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();

void EraseFlash(void);
void WriteFlashSubBlock(void);
void BootApplication(void);

typedef union __attribute__ ((packed)) _USB_HID_BOOTLOADER_COMMAND
{
    unsigned char Contents[64];

    struct __attribute__ ((packed)) {
        unsigned char Command;
        unsigned AddressHigh;
        unsigned AddressLow;
        unsigned char Size;
        unsigned char PadBytes[(TotalPacketSize - 6) - (RequestDataBlockSize)];
        unsigned int Data[RequestDataBlockSize/sizeof(unsigned)];
    };

    struct __attribute__ ((packed)) {
        unsigned char Command;
        unsigned Address;
        unsigned char Size;
        unsigned char PadBytes[(TotalPacketSize - 6) - (RequestDataBlockSize)];
        unsigned int Data[RequestDataBlockSize/sizeof(unsigned)];
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
} PacketToFromPC;

PacketToFromPC PacketFromPC;        // 64 byte buffer for receiving packets on EP1 OUT from the PC
PacketToFromPC PacketToPC;          // 64 byte buffer for sending packets on EP1 IN to the PC
PacketToFromPC PacketFromPCBuffer;

USB_HANDLE USBOutHandle = 0;
USB_HANDLE USBInHandle = 0;

//unsigned int __attribute__((section(".boot_software_key_sec,\"aw\",@nobits#"))) SoftwareKey;
unsigned int SoftwareKey;

unsigned char MaxPageToErase;
unsigned long ProgramMemStopAddress;
unsigned char BootState;
unsigned char ErasePageTracker;
unsigned int ProgrammingBuffer[BufferSize];
unsigned char BufferedDataIndex;
unsigned long ProgrammedPointer;
unsigned char ConfigsProtected;

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

static void soft_reset()
{
#if 0
9d0010f0 <SoftReset>:
9d0010f0:	27bdffe8 	addiu	sp,sp,-24
9d0010f4:	afbf0014 	sw	ra,20(sp)

9d0010f8:	0f40045a 	jal	9d001168 <INTDisableInterrupts>     -- di
9d0010fc:	00000000 	nop

9d001100:	3c02bf88 	lui	v0,0xbf88
9d001104:	8c433000 	lw	v1,12288(v0)
9d001108:	7c630300 	ext	v1,v1,0xc,0x1
9d00110c:	14600008 	bnez	v1,9d001130 <SoftReset+0x40>
9d001110:	3c03aa99 	lui	v1,0xaa99

9d001114:	24041000 	li	a0,4096
9d001118:	3c03bf88 	lui	v1,0xbf88
9d00111c:	ac643008 	sw	a0,12296(v1)

9d001120:	8c433000 	lw	v1,12288(v0)
9d001124:	30630800 	andi	v1,v1,0x800
9d001128:	1460fffd 	bnez	v1,9d001120 <SoftReset+0x30>
9d00112c:	3c03aa99 	lui	v1,0xaa99

9d001130:	3c02bf81 	lui	v0,0xbf81
9d001134:	24636655 	addiu	v1,v1,26197
9d001138:	ac40f230 	sw	zero,-3536(v0)
9d00113c:	ac43f230 	sw	v1,-3536(v0)
9d001140:	3c035566 	lui	v1,0x5566
9d001144:	346399aa 	ori	v1,v1,0x99aa
9d001148:	ac43f230 	sw	v1,-3536(v0)
9d00114c:	3c02bf81 	lui	v0,0xbf81
9d001150:	24030001 	li	v1,1
9d001154:	ac43f618 	sw	v1,-2536(v0)
9d001158:	3c02bf81 	lui	v0,0xbf81
9d00115c:	8c42f610 	lw	v0,-2544(v0)

9d001160:	0b400458 	j	9d001160 <SoftReset+0x70>
9d001164:	00000000 	nop
#endif
}

/*
 * BlinkUSBStatus turns on and off LEDs
 * corresponding to the USB device state.
 */
static void led_blink(void)
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
 * Main program entry point.
 */
int main(void)
{
    // The PRG switch is tied high, so when it is not pressed it will read 1.
    // To call the normal application (i.e. don't go into the bootloader)
    // we need to make sure that the PRG button is not pressed, and that we
    // don't have a software reset
    if (! button_pressed() &&
        (! (RCON & PIC32_RCON_SWR) || SoftwareKey != 0x12345678))
    {
	// Must clear out software key
	SoftwareKey = 0;

        // If there is NO real program to execute, then fall through to the bootloader
	if (*(int*) BootMemStart != 0xFFFFFFFF)	{
            void (*fptr)(void) = (void (*)(void)) BootMemStart;

            fptr();
            for (;;)
                continue;
	}
    }
    // Must clear out software key
    SoftwareKey = 0;
    RCONCLR = PIC32_RCON_SWR;

    // Initialize USB module SFRs and firmware variables to known states.
    usb_device_init();

    // Initialize all of the LED pins
    led_init();

    // Initialize all of the push buttons
    button_init();

    // Initialize the variable holding the handle for the last
    // transmission
    USBOutHandle = 0;
    USBInHandle = 0;

    // Initialize bootloader state variables
    MaxPageToErase = MaxPageToEraseNoConfigs;		// Assume we will not allow erase/programming of config words (unless host sends override command)
    ProgramMemStopAddress = ProgramMemStopNoConfigs;
    ConfigsProtected = LOCKCONFIG;			// Assume we will not erase or program the vector table at first.  Must receive unlock config bits/vectors command first.
    BootState = IdleState;
    ProgrammedPointer = InvalidAddress;
    BufferedDataIndex = 0;

    while(1) {
	// Check bus status and service USB interrupts.
        usb_device_tasks();

        // Blink the LEDs according to the USB device status
        led_blink();

        // User Application USB tasks
        if (usb_device_state < CONFIGURED_STATE || usb_is_device_suspended())
            continue;

        BootApplication();
    }
}

/*
 * This function is a place holder for other user routines.
 * It is a mixture of both USB and non-USB tasks.
 */
void BootApplication(void)
{
    unsigned char i;
    unsigned int j;
    void *pFlash;
    int temp;

    if (BootState == IdleState) {
        // Are we done sending the last response.  We need to be before we
        // receive the next command because we clear the PacketToPC buffer
        // once we receive a command
        if (! usb_handle_busy (USBInHandle)) {
            if (! usb_handle_busy (USBOutHandle)) { // Did we receive a command?
                for(i = 0; i < TotalPacketSize; i++)
                {
                    PacketFromPC.Contents[i] = PacketFromPCBuffer.Contents[i];
                }

                USBOutHandle = usb_transfer_one_packet (HID_EP, OUT_FROM_HOST,
                    (char*) &PacketFromPCBuffer, 64);
                BootState = NotIdleState;

                //Prepare the next packet we will send to the host, by initializing the entire packet to 0x00.
                for(i = 0; i < TotalPacketSize; i++)
                {
                    //This saves code space, since we don't have to do it independently in the QUERY_DEVICE and GET_DATA cases.
                    PacketToPC.Contents[i] = 0;
                }
            }
        }
        return;
    }

    switch (PacketFromPC.Command) {
    case QUERY_DEVICE:
        // Prepare a response packet, which lets the PC software know
        // about the memory ranges of this device.

        PacketToPC.Command = (unsigned char) QUERY_DEVICE;
        PacketToPC.PacketDataFieldSize = (unsigned char)RequestDataBlockSize;
        PacketToPC.DeviceFamily = (unsigned char) DEVICE_FAMILY;

        PacketToPC.Type1 = (unsigned char) TypeProgramMemory;
        PacketToPC.Address1 = (unsigned long) ProgramMemStart;
        PacketToPC.Length1 = (unsigned long) (ProgramMemStopAddress - ProgramMemStart);	//Size of program memory area
        PacketToPC.Type2 = (unsigned char) TypeEndOfTypeList;

        // Init pad bytes to 0x00...  Already done after we received
        // the QUERY_DEVICE command (just after calling usb_rx_one_packet()).

        if (! usb_handle_busy (USBInHandle)) {
            USBInHandle = usb_transfer_one_packet (HID_EP, IN_TO_HOST,
                (char*) &PacketToPC, 64);
            BootState = IdleState;
        }
        break;

    case UNLOCK_CONFIG:
        if (PacketFromPC.LockValue == UNLOCKCONFIG) {
            // Assume we will not allow erase/programming of config
            // words (unless host sends override command)
            MaxPageToErase = MaxPageToEraseWithConfigs;
            ProgramMemStopAddress = ProgramMemStopWithConfigs;
            ConfigsProtected = UNLOCKCONFIG;
        } else {
            // LockValue must be == LOCKCONFIG
            MaxPageToErase = MaxPageToEraseNoConfigs;
            ProgramMemStopAddress = ProgramMemStopNoConfigs;
            ConfigsProtected = LOCKCONFIG;
        }
        BootState = IdleState;
        break;

    case ERASE_DEVICE:
        pFlash = (void*) ProgramMemStart;
        for (temp = 0; temp < MaxPageToErase; temp++) {
            NVMErasePage (pFlash + temp * FLASH_PAGE_SIZE);

            // Call USBDriverService() periodically to prevent falling
            // off the bus if any SETUP packets should happen to arrive.
            usb_device_tasks();
        }
        // Good practice to clear WREN bit anytime we are
        // not expecting to do erase/write operations,
        // further reducing probability of accidental activation.
        NVMCONCLR = PIC32_NVMCON_WREN;
        BootState = IdleState;
        break;

    case PROGRAM_DEVICE:
        if (ProgrammedPointer == (unsigned long)InvalidAddress)
            ProgrammedPointer = PacketFromPC.Address;

        if (ProgrammedPointer == (unsigned long) PacketFromPC.Address) {
            for(i = 0; i < (PacketFromPC.Size/sizeof(unsigned)); i++) {
                unsigned int index;

                index = (RequestDataBlockSize - PacketFromPC.Size) / sizeof(unsigned) + i;

                // Data field is right justified.
                // Need to put it in the buffer left justified.
                ProgrammingBuffer[BufferedDataIndex] =
                    PacketFromPC.Data[(RequestDataBlockSize-PacketFromPC.Size) / sizeof(unsigned) + i];
                BufferedDataIndex++;
                ProgrammedPointer += sizeof(unsigned);
                if (BufferedDataIndex == RequestDataBlockSize / sizeof(unsigned)) {
                    // Need to make sure it doesn't call WriteFlashSubBlock()
                    // unless BufferedDataIndex/2 is an integer
                    WriteFlashSubBlock();
                }
            }
        }
        // else host sent us a non-contiguous packet address...
        // to make this firmware simpler, host should not do this without
        // sending a PROGRAM_COMPLETE command in between program sections.
        BootState = IdleState;
        break;

    case PROGRAM_COMPLETE:
        WriteFlashSubBlock();
        ProgrammedPointer = InvalidAddress;		//Reinitialize pointer to an invalid range, so we know the next PROGRAM_DEVICE will be the start address of a contiguous section.
        BootState = IdleState;
        break;

    case GET_DATA:
        if (! usb_handle_busy (USBInHandle)) {
            // Init pad bytes to 0x00...  Already done after we received
            // the QUERY_DEVICE command (just after calling HIDRxReport()).
            PacketToPC.Command = GET_DATA;
            PacketToPC.Address = PacketFromPC.Address;
            PacketToPC.Size = PacketFromPC.Size;

            for (i = 0; i < (PacketFromPC.Size/sizeof(unsigned)); i++) {
                unsigned *p;
                unsigned data;

                p = ((unsigned*) ((PacketFromPC.Address + (i * sizeof(unsigned))) | 0x80000000));
                data = *p;

                PacketToPC.Data [RequestDataBlockSize/sizeof(unsigned) + i -
                    PacketFromPC.Size/sizeof(unsigned)] =
                    *((unsigned*)((PacketFromPC.Address + (i * sizeof(unsigned))) | 0x80000000));
            }

            USBInHandle = usb_transfer_one_packet (HID_EP,  IN_TO_HOST,
                (char*) &PacketToPC.Contents[0], 64);
            BootState = IdleState;
        }
        break;

    case RESET_DEVICE:
        // Disable USB module
        U1CON = 0x0000;

        // And wait awhile for the USB cable capacitance to discharge down to disconnected (SE0) state.
        // Otherwise host might not realize we disconnected/reconnected when we do the reset.
        // A basic for() loop decrementing a 16 bit number would be simpler, but seems to take more code space for
        // a given delay.  So do this instead:
        for (j = 0; j < 0xFFFF; j++) {
            continue;
        }
        soft_reset();
        break;
    }
}

//
// Use word writes to write code chunks less than a full 64 byte block size.
//
void WriteFlashSubBlock(void)
{
    unsigned int i = 0, address;

    while (BufferedDataIndex > 0) {
        address = (ProgrammedPointer - (BufferedDataIndex * sizeof(unsigned)));
        NVMWriteWord((unsigned*) address, (unsigned int) ProgrammingBuffer[i++]);
        BufferedDataIndex = BufferedDataIndex - 1;
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
    USBOutHandle = usb_rx_one_packet (HID_EP,
        &PacketFromPCBuffer.Contents[0], 64);
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
    64, 0,                  // Size
    1,                      // Interval

    /*
     * Endpoint descriptor
     */
    sizeof (USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT,
    HID_EP | _EP_OUT,       // EndpointAddress
    _INTERRUPT,             // Attributes
    64, 0,                  // Size
    1,                      // Interval
};

/*
 * USB Strings
 */
static const USB_STRING_INIT(1) sd000 = {   /* Language code */
    sizeof(sd000), USB_DESCRIPTOR_STRING,
    { 0x0409 },
};

static const USB_STRING_INIT(25) sd001 = {  /* Manufacturer */
    sizeof(sd001), USB_DESCRIPTOR_STRING,
    { 'M','i','c','r','o','c','h','i','p',' ',
      'T','e','c','h','n','o','l','o','g','y',
      ' ','I','n','c','.' },
};

static const USB_STRING_INIT(18) sd002 = {  /* Product */
    sizeof(sd002), USB_DESCRIPTOR_STRING,
    { 'U','S','B',' ','H','I','D',' ',
      'B','o','o','t','l','o','a','d','e','r' },
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

// Array of configuration descriptors
const unsigned char *const usb_config[] = {
    &usb_config1_descriptor,
};

//Array of string descriptors
const unsigned char *const usb_string[] = {
    (const unsigned char *const) &sd000,
    (const unsigned char *const) &sd001,
    (const unsigned char *const) &sd002,
};
