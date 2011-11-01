/*
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC® Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PIC Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <machine/pic32mx.h>
#include <machine/usb.h>
#include <machine/usb_function_cdc.h>

struct tty cnttys [1];

void cnstart (struct tty *tp);

/*
 * Initialize USB module SFRs and firmware variables to known state.
 * Enable interrupts.
 */
void cninit()
{
    usb_device_init();
    IECSET(1) = 1 << (PIC32_IRQ_USB - 32);

    // TODO: wait for user connection.
    // Blink all LEDs while waiting.
}

void cnidentify()
{
    printf ("console: USB\n");
}

int cnopen (dev, flag, mode)
    dev_t dev;
{
    register struct tty *tp = &cnttys[0];

    tp->t_oproc = cnstart;
    if ((tp->t_state & TS_ISOPEN) == 0) {
            ttychars(tp);
            tp->t_state = TS_ISOPEN | TS_CARR_ON;
            tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
    }
    if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
            return (EBUSY);

    if (! linesw[tp->t_line].l_open)
            return (ENODEV);
    return (*linesw[tp->t_line].l_open) (dev, tp);
}

int cnclose (dev, flag, mode)
    dev_t dev;
{
    register struct tty *tp = &cnttys[0];

    if (linesw[tp->t_line].l_close)
        (*linesw[tp->t_line].l_close) (tp, flag);
    ttyclose (tp);
    return 0;
}

int cnread (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct tty *tp = &cnttys[0];

    if (! linesw[tp->t_line].l_read)
        return (ENODEV);
    return (*linesw[tp->t_line].l_read) (tp, uio, flag);
}

int cnwrite (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct tty *tp = &cnttys[0];

    if (! linesw[tp->t_line].l_write)
        return (ENODEV);
    return (*linesw[tp->t_line].l_write) (tp, uio, flag);
}

int cnioctl (dev, cmd, addr, flag)
    dev_t dev;
    register u_int cmd;
    caddr_t addr;
{
    register struct tty *tp = &cnttys[0];
    register int error;

    if (linesw[tp->t_line].l_ioctl) {
        error = (*linesw[tp->t_line].l_ioctl) (tp, cmd, addr, flag);
        if (error >= 0)
            return error;
    }
    error = ttioctl (tp, cmd, addr, flag);
    if (error < 0)
            error = ENOTTY;
    return error;
}

int cnselect (dev, rw)
    register dev_t dev;
    int rw;
{
    register struct tty *tp = &cnttys[0];

    return ttyselect (tp, rw);
}

void cnstart (tp)
    register struct tty *tp;
{
    register int s;

    s = spltty();
    if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
out:	/* Disable transmit_interrupt. */
        led_control (LED_TTY, 0);
        splx (s);
        return;
    }
    ttyowake(tp);
    if (tp->t_outq.c_cc == 0)
        goto out;
    // TODO
#if 0
    if (reg->sta & PIC32_USTA_TRMT) {
        int c = getc (&tp->t_outq);
        reg->txreg = c;
        tp->t_state |= TS_BUSY;
    }
#endif
    led_control (LED_TTY, 1);
    splx (s);
}

/*
 * Put a symbol on console terminal.
 */
void cnputc (c)
    char c;
{
    // TODO
}

//
// Check bus status and service USB interrupts.
//
void usb_intr()
{
    // Must call this function from interrupt or periodically.
    // It will process and respond to SETUP transactions
    // (such as during the enumeration process when you first
    // plug in).  USB hosts require that USB devices should accept
    // and process SETUP packets in a timely fashion.  Therefore,
    // when using polling, this function should be called
    // frequently (such as once about every 100 microseconds) at any
    // time that a SETUP packet might reasonably be expected to
    // be sent by the host to your device.  In most cases, the
    // usb_device_tasks() function does not take very long to
    // execute (~50 instruction cycles) before it returns.
    usb_device_tasks();

    // User Application USB tasks
    if (usb_device_state >= CONFIGURED_STATE && ! (U1PWRC & PIC32_U1PWRC_USUSPEND)) {
        unsigned nbytes_read;
        static unsigned char inbuf[64];

        // Pull in some new data if there is new data to pull in
        nbytes_read = getsUSBUSART ((char*) inbuf, 64);
        if (nbytes_read != 0) {
            // TODO
#if 0
            if (linesw[tp->t_line].l_rint)
                    (*linesw[tp->t_line].l_rint) (c, tp);
#endif
            printf ("Received %d bytes: %02x...\r\n", nbytes_read, inbuf[0]);
            putUSBUSART ("Ok\r\n", 4);
        }
        CDCTxService();
    }
}

/*
 * USB Callback Functions
 *
 * The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
 * events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
 * packets to your device.  In response to this, all USB devices are supposed to decrease their power
 * consumption from the USB Vbus to <2.5mA each.  The USB module detects this condition (which according
 * to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
 * function.  You should modify these callback functions to take appropriate actions for each of these
 * conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
 * consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
 * microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
 * add code that undoes the power saving things done in the USBCBSuspend() function.
 *
 * The USBCBSendResume() function is special, in that the USB stack will not automatically call this
 * function.  This function is meant to be called from the application firmware instead.  See the
 * additional comments near the function.
 */

/*
 * This function is called when the device becomes
 * initialized, which occurs after the host sends a
 * SET_CONFIGURATION (wValue not = 0) request.  This
 * callback function should initialize the endpoints
 * for the device's usage according to the current
 * configuration.
 */
void USBCBInitEP (void)
{
    CDCInitEP();
}

/*
 * When SETUP packets arrive from the host, some
 * firmware must process the request and respond
 * appropriately to fulfill the request.  Some of
 * the SETUP packets will be for standard
 * USB "chapter 9" (as in, fulfilling chapter 9 of
 * the official USB specifications) requests, while
 * others may be specific to the USB device class
 * that is being implemented.  For example, a HID
 * class device needs to be able to respond to
 * "GET REPORT" type of requests.  This
 * is not a standard USB chapter 9 request, and
 * therefore not handled by usb_device.c.  Instead
 * this request should be handled by class specific
 * firmware, such as that contained in usb_function_hid.c.
 */
void USBCBCheckOtherReq (void)
{
    USBCheckCDCRequest();
}

/*
 * The USBCBStdSetDscHandler() callback function is
 * called when a SETUP, bRequest: SET_DESCRIPTOR request
 * arrives.  Typically SET_DESCRIPTOR requests are
 * not used in most applications, and it is
 * optional to support this type of request.
 */
void USBCBStdSetDscHandler(void)
{
    /* Must claim session ownership if supporting this request */
}

/*
 * The host may put USB peripheral devices in low power
 * suspend mode (by "sending" 3+ms of idle).  Once in suspend
 * mode, the host may wake the device back up by sending non-
 * idle state signalling.
 *
 * This call back is invoked when a wakeup from USB suspend is detected.
 */
void USBCBWakeFromSuspend(void)
{
    // If clock switching or other power savings measures were taken when
    // executing the USBCBSuspend() function, now would be a good time to
    // switch back to normal full power run mode conditions.  The host allows
    // a few milliseconds of wakeup time, after which the device must be
    // fully back to normal, and capable of receiving and processing USB
    // packets.  In order to do this, the USB module must receive proper
    // clocking (IE: 48MHz clock must be available to SIE for full speed USB
    // operation).
}

/*
 * Call back that is invoked when a USB suspend is detected
 */
void USBCBSuspend (void)
{
}

/*
 * The USB host sends out a SOF packet to full-speed
 * devices every 1 ms. This interrupt may be useful
 * for isochronous pipes. End designers should
 * implement callback routine as necessary.
 */
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
}

/*
 * The purpose of this callback is mainly for
 * debugging during development. Check UEIR to see
 * which error causes the interrupt.
 */
void USBCBErrorHandler(void)
{
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

    // Typically, user firmware does not need to do anything special
    // if a USB error occurs.  For example, if the host sends an OUT
    // packet to your device, but the packet gets corrupted (ex:
    // because of a bad connection, or the user unplugs the
    // USB cable during the transmission) this will typically set
    // one or more USB error interrupt flags.  Nothing specific
    // needs to be done however, since the SIE will automatically
    // send a "NAK" packet to the host.  In response to this, the
    // host will normally retry to send the packet again, and no
    // data loss occurs.  The system will typically recover
    // automatically, without the need for application firmware
    // intervention.

    // Nevertheless, this callback function is provided, such as
    // for debugging purposes.
}

/*
 * The USB specifications allow some types of USB
 * peripheral devices to wake up a host PC (such
 * as if it is in a low power suspend to RAM state).
 * This can be a very useful feature in some
 * USB applications, such as an Infrared remote
 * control receiver.  If a user presses the "power"
 * button on a remote control, it is nice that the
 * IR receiver can detect this signalling, and then
 * send a USB "command" to the PC to wake up.
 *
 * The USBCBSendResume() "callback" function is used
 * to send this special USB signalling which wakes
 * up the PC.  This function may be called by
 * application firmware to wake up the PC.  This
 * function should only be called when:
 *
 * 1.  The USB driver used on the host PC supports
 *     the remote wakeup capability.
 * 2.  The USB configuration descriptor indicates
 *     the device is remote wakeup capable in the
 *     bmAttributes field.
 * 3.  The USB host PC is currently sleeping,
 *     and has previously sent your device a SET
 *     FEATURE setup packet which "armed" the
 *     remote wakeup capability.
 *
 * This callback should send a RESUME signal that
 * has the period of 1-15ms.
 *
 * Note: Interrupt vs. Polling
 * -Primary clock
 * -Secondary clock ***** MAKE NOTES ABOUT THIS *******
 * > Can switch to primary first by calling USBCBWakeFromSuspend()
 *
 * The modifiable section in this routine should be changed
 * to meet the application needs. Current implementation
 * temporary blocks other functions from executing for a
 * period of 1-13 ms depending on the core frequency.
 *
 * According to USB 2.0 specification section 7.1.7.7,
 * "The remote wakeup device must hold the resume signaling
 * for at lest 1 ms but for no more than 15 ms."
 * The idea here is to use a delay counter loop, using a
 * common value that would work over a wide range of core
 * frequencies.
 * That value selected is 1800. See table below:
 * ==========================================================
 * Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 * ==========================================================
 *     48              12          1.05
 *      4              1           12.6
 * ==========================================================
 *  * These timing could be incorrect when using code
 *    optimization or extended instruction mode,
 *    or when having other interrupts enabled.
 *    Make sure to verify using the MPLAB SIM's Stopwatch
 *    and verify the actual signal on an oscilloscope.
 */
void USBCBSendResume (void)
{
    // Start RESUME signaling
    U1CON |= PIC32_U1CON_RESUME;

    // Set RESUME line for 1-13 ms
    udelay (5000);

    U1CON &= ~PIC32_U1CON_RESUME;
}

/*
 * This function is called whenever a EP0 data
 * packet is received.  This gives the user (and
 * thus the various class examples a way to get
 * data that is received via the control endpoint.
 * This function needs to be used in conjunction
 * with the USBCBCheckOtherReq() function since
 * the USBCBCheckOtherReq() function is the apps
 * method for getting the initial control transfer
 * before the data arrives.
 *
 * PreCondition: ENABLE_EP0_DATA_RECEIVED_CALLBACK must be
 * defined already (in usb_config.h)
 */
#if defined(ENABLE_EP0_DATA_RECEIVED_CALLBACK)
void USBCBEP0DataReceived(void)
{
}
#endif

/*
 * Filling in the descriptor values in the usb_descriptors.c file:
 * -------------------------------------------------------------------
 * [Device Descriptors]
 * The device descriptor is defined as a USB_DEVICE_DESCRIPTOR type.
 * This type is defined in usb_ch9.h  Each entry into this structure
 * needs to be the correct length for the data type of the entry.
 */

/*
 * [Configuration Descriptors]
 * The configuration descriptor was changed in v2.x from a structure
 * to a BYTE array.  Given that the configuration is now a byte array
 * each byte of multi-byte fields must be listed individually.  This
 * means that for fields like the total size of the configuration where
 * the field is a 16-bit value "64,0," is the correct entry for a
 * configuration that is only 64 bytes long and not "64," which is one
 * too few bytes.
 *
 * The configuration attribute must always have the _DEFAULT
 * definition at the minimum. Additional options can be ORed
 * to the _DEFAULT attribute. Available options are _SELF and _RWU.
 * These definitions are defined in the usb_device.h file. The
 * _SELF tells the USB host that this device is self-powered. The
 * _RWU tells the USB host that this device supports Remote Wakeup.
 */

/*
 * [Endpoint Descriptors]
 * Like the configuration descriptor, the endpoint descriptors were
 * changed in v2.x of the stack from a structure to a BYTE array.  As
 * endpoint descriptors also has a field that are multi-byte entities,
 * please be sure to specify both bytes of the field.  For example, for
 * the endpoint size an endpoint that is 64 bytes needs to have the size
 * defined as "64,0," instead of "64,"
 *
 * Take the following example:
 *     // Endpoint Descriptor //
 *     0x07,                       //the size of this descriptor //
 *     USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
 *     _EP02_IN,                   //EndpointAddress
 *     _INT,                       //Attributes
 *     0x08,0x00,                  //size (note: 2 bytes)
 *     0x02,                       //Interval
 *
 * The first two parameters are self-explanatory. They specify the
 * length of this endpoint descriptor (7) and the descriptor type.
 * The next parameter identifies the endpoint, the definitions are
 * defined in usb_device.h and has the following naming
 * convention:
 * _EP<##>_<dir>
 * where ## is the endpoint number and dir is the direction of
 * transfer. The dir has the value of either 'OUT' or 'IN'.
 * The next parameter identifies the type of the endpoint. Available
 * options are _BULK, _INT, _ISO, and _CTRL. The _CTRL is not
 * typically used because the default control transfer endpoint is
 * not defined in the USB descriptors. When _ISO option is used,
 * addition options can be ORed to _ISO. Example:
 * _ISO|_AD|_FE
 * This describes the endpoint as an isochronous pipe with adaptive
 * and feedback attributes. See usb_device.h and the USB
 * specification for details. The next parameter defines the size of
 * the endpoint. The last parameter in the polling interval.
 */

/*
 * Adding a USB String
 * -------------------------------------------------------------------
 * A string descriptor array should have the following format:
 *
 * rom struct{byte bLength;byte bDscType;word string[size];}sdxxx={
 * sizeof(sdxxx),DSC_STR,<text>};
 *
 * The above structure provides a means for the C compiler to
 * calculate the length of string descriptor sdxxx, where xxx is the
 * index number. The first two bytes of the descriptor are descriptor
 * length and type. The rest <text> are string texts which must be
 * in the unicode format. The unicode format is achieved by declaring
 * each character as a word type. The whole text string is declared
 * as a word array with the number of characters equals to <size>.
 * <size> has to be manually counted and entered into the array
 * declaration. Let's study this through an example:
 * if the string is "USB" , then the string descriptor should be:
 * (Using index 02)
 * rom struct{byte bLength;byte bDscType;word string[3];}sd002={
 * sizeof(sd002),DSC_STR,'U','S','B'};
 *
 * A USB project may have multiple strings and the firmware supports
 * the management of multiple strings through a look-up table.
 * The look-up table is defined as:
 * rom const unsigned char *rom USB_SD_Ptr[]={&sd000,&sd001,&sd002};
 *
 * The above declaration has 3 strings, sd000, sd001, and sd002.
 * Strings can be removed or added. sd000 is a specialized string
 * descriptor. It defines the language code, usually this is
 * US English (0x0409). The index of the string must match the index
 * position of the USB_SD_Ptr array, &sd000 must be in position
 * USB_SD_Ptr[0], &sd001 must be in position USB_SD_Ptr[1] and so on.
 * The look-up table USB_SD_Ptr is used by the get string handler
 * function.
 */

/*
 * The look-up table scheme also applies to the configuration
 * descriptor. A USB device may have multiple configuration
 * descriptors, i.e. CFG01, CFG02, etc. To add a configuration
 * descriptor, user must implement a structure similar to CFG01.
 * The next step is to add the configuration descriptor name, i.e.
 * cfg01, cfg02,.., to the look-up table USB_CD_Ptr. USB_CD_Ptr[0]
 * is a dummy place holder since configuration 0 is the un-configured
 * state according to the definition in the USB specification.
 */

/*
 * Device Descriptor
 */
const USB_DEVICE_DESCRIPTOR device_dsc = {
    0x12,			// Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    CDC_DEVICE,             // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID
    0x000A,                 // Product ID: CDC RS-232 Emulation Demo
    0x0100,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/*
 * Configuration 1 Descriptor
 */
const unsigned char configDescriptor1[] =
{
    /* Configuration Descriptor */
    0x09,				// sizeof(USB_CFG_DSC)
    USB_DESCRIPTOR_CONFIGURATION,	// CONFIGURATION descriptor type
    67, 0,				// Total length of data for this cfg
    2,				// Number of interfaces in this cfg
    1,				// Index value of this configuration
    0,				// Configuration string index
    _DEFAULT | _SELF,		// Attributes, see usb_device.h
    50,				// Max power consumption (2X mA)

    /* Interface Descriptor */
    9,				// sizeof(USB_INTF_DSC)
    USB_DESCRIPTOR_INTERFACE,	// INTERFACE descriptor type
    0,				// Interface Number
    0,				// Alternate Setting Number
    1,				// Number of endpoints in this intf
    COMM_INTF,			// Class code
    ABSTRACT_CONTROL_MODEL,		// Subclass code
    V25TER,				// Protocol code
    0,				// Interface string index

    /* CDC Class-Specific Descriptors */
    sizeof(USB_CDC_HEADER_FN_DSC),
    CS_INTERFACE,
    DSC_FN_HEADER,
    0x10,0x01,

    sizeof(USB_CDC_ACM_FN_DSC),
    CS_INTERFACE,
    DSC_FN_ACM,
    USB_CDC_ACM_FN_DSC_VAL,

    sizeof(USB_CDC_UNION_FN_DSC),
    CS_INTERFACE,
    DSC_FN_UNION,
    CDC_COMM_INTF_ID,
    CDC_DATA_INTF_ID,

    sizeof(USB_CDC_CALL_MGT_FN_DSC),
    CS_INTERFACE,
    DSC_FN_CALL_MGT,
    0x00,
    CDC_DATA_INTF_ID,

    /* Endpoint Descriptor */
    0x07,				/* sizeof(USB_EP_DSC) */
    USB_DESCRIPTOR_ENDPOINT,	// Endpoint Descriptor
    _EP02_IN,			// EndpointAddress
    _INTERRUPT,			// Attributes
    0x08, 0x00,			// size
    0x02,				// Interval

    /* Interface Descriptor */
    9,				/* sizeof(USB_INTF_DSC) */
    USB_DESCRIPTOR_INTERFACE,	// INTERFACE descriptor type
    1,				// Interface Number
    0,				// Alternate Setting Number
    2,				// Number of endpoints in this intf
    DATA_INTF,			// Class code
    0,				// Subclass code
    NO_PROTOCOL,			// Protocol code
    0,				// Interface string index

    /* Endpoint Descriptor */
    0x07,				/* sizeof(USB_EP_DSC) */
    USB_DESCRIPTOR_ENDPOINT,	// Endpoint Descriptor
    _EP03_OUT,			// EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			// size
    0x00,				// Interval

    /* Endpoint Descriptor */
    0x07,				/* sizeof(USB_EP_DSC) */
    USB_DESCRIPTOR_ENDPOINT,	// Endpoint Descriptor
    _EP03_IN,			// EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			// size
    0x00,				// Interval
};


/*
 * Language code string descriptor
 */
const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[1];
} sd000 = {
    sizeof(sd000),
    USB_DESCRIPTOR_STRING,
    { 0x0409 }
};

/*
 * Manufacturer string descriptor
 */
const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[25];
} sd001 = {
    sizeof(sd001),
    USB_DESCRIPTOR_STRING,
    { 'M','i','c','r','o','c','h','i','p',' ',
      'T','e','c','h','n','o','l','o','g','y',' ','I','n','c','.', },
};

/*
 * Product string descriptor
 */
const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[25];
} sd002 = {
    sizeof(sd002),
    USB_DESCRIPTOR_STRING,
    { 'R','e','t','r','o','B','S','D',' ','C','o','n','s','o','l','e', },
};

/*
 * Array of configuration descriptors
 */
const unsigned char *const USB_CD_Ptr[] = {
    (const unsigned char *const) &configDescriptor1
};

/*
 * Array of string descriptors
 */
const unsigned char *const USB_SD_Ptr[USB_NUM_STRING_DESCRIPTORS] = {
    (const unsigned char *const) &sd000,
    (const unsigned char *const) &sd001,
    (const unsigned char *const) &sd002
};
