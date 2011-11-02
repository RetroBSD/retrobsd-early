/*
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC� Microcontroller is intended and
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
#include <machine/usb_device.h>
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
    usb_device_tasks();

    // User application USB tasks
    if (usb_device_state >= CONFIGURED_STATE && ! (U1PWRC & PIC32_U1PWRC_USUSPEND)) {
        unsigned nbytes_read;
        static unsigned char inbuf[64];

        // Pull in some new data if there is new data to pull in
        nbytes_read = cdc_gets ((char*) inbuf, 64);
        if (nbytes_read != 0) {
            // TODO
#if 0
            if (linesw[tp->t_line].l_rint)
                    (*linesw[tp->t_line].l_rint) (c, tp);
#endif
            printf ("Received %d bytes: %02x...\r\n", nbytes_read, inbuf[0]);
            cdc_put ("Ok\r\n", 4);
        }
        cdc_tx_service();
    }
}

/*
 * USB Callback Functions
 */

/*
 * This function is called when the device becomes initialized.
 * It should initialize the endpoints for the device's usage
 * according to the current configuration.
 */
void usbcb_init_ep()
{
    cdc_init_ep();
}

/*
 * Process device-specific SETUP requests.
 */
void usbcb_check_other_req()
{
    cdc_check_request();
}

/*
 * Handle a SETUP SET_DESCRIPTOR request (optional).
 */
void usbcb_std_set_dsc_handler()
{
    /* Empty. */
}

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
 * Wake up a host PC.
 */
void usb_send_resume (void)
{
    /* Start RESUME signaling. */
    U1CON |= PIC32_U1CON_RESUME;

    /* Set RESUME line for 1-13 ms. */
    udelay (5000);

    U1CON &= ~PIC32_U1CON_RESUME;
}

/*
 * Device Descriptor
 */
const USB_DEVICE_DESCRIPTOR usb_device = {
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
static const unsigned char config1_descriptor[] =
{
    /* Configuration Descriptor */
    0x09,				// sizeof(USB_CFG_DSC)
    USB_DESCRIPTOR_CONFIGURATION,	// CONFIGURATION descriptor type
    67, 0,				// Total length of data for this cfg
    2,                                  // Number of interfaces in this cfg
    1,                                  // Index value of this configuration
    0,                                  // Configuration string index
    _DEFAULT | _SELF,                   // Attributes, see usb_device.h
    50,                                 // Max power consumption (2X mA)

    /* Interface Descriptor */
    9,                                  // sizeof(USB_INTF_DSC)
    USB_DESCRIPTOR_INTERFACE,           // INTERFACE descriptor type
    0,                                  // Interface Number
    0,                                  // Alternate Setting Number
    1,                                  // Number of endpoints in this intf
    COMM_INTF,                          // Class code
    ABSTRACT_CONTROL_MODEL,		// Subclass code
    V25TER,				// Protocol code
    0,                                  // Interface string index

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
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP02_IN,			        // EndpointAddress
    _INTERRUPT,			        // Attributes
    0x08, 0x00,			        // size
    0x02,				// Interval

    /* Interface Descriptor */
    9,				        /* sizeof(USB_INTF_DSC) */
    USB_DESCRIPTOR_INTERFACE,	        // INTERFACE descriptor type
    1,				        // Interface Number
    0,				        // Alternate Setting Number
    2,				        // Number of endpoints in this intf
    DATA_INTF,			        // Class code
    0,				        // Subclass code
    NO_PROTOCOL,			// Protocol code
    0,				        // Interface string index

    /* Endpoint Descriptor */
    0x07,				/* sizeof(USB_EP_DSC) */
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP03_OUT,			        // EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			        // size
    0x00,				// Interval

    /* Endpoint Descriptor */
    0x07,				/* sizeof(USB_EP_DSC) */
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP03_IN,			        // EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			        // size
    0x00,				// Interval
};


/*
 * Language code string descriptor.
 */
static const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[1];
} string0_descriptor = {
    sizeof(string0_descriptor),
    USB_DESCRIPTOR_STRING,
    { 0x0409 }                          /* US English */
};

/*
 * Manufacturer string descriptor
 */
static const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[25];
} string1_descriptor = {
    sizeof(string1_descriptor),
    USB_DESCRIPTOR_STRING,
    { 'M','i','c','r','o','c','h','i','p',' ',
      'T','e','c','h','n','o','l','o','g','y',' ','I','n','c','.', },
};

/*
 * Product string descriptor
 */
static const struct {
    unsigned char bLength;
    unsigned char bDscType;
    unsigned short string[25];
} string2_descriptor = {
    sizeof(string2_descriptor),
    USB_DESCRIPTOR_STRING,
    { 'R','e','t','r','o','B','S','D',' ','C','o','n','s','o','l','e', },
};

/*
 * Array of configuration descriptors
 */
const unsigned char *const usb_config[] = {
    (const unsigned char *const) &config1_descriptor,
};

/*
 * Array of string descriptors
 */
const unsigned char *const usb_string[USB_NUM_STRING_DESCRIPTORS] = {
    (const unsigned char *const) &string0_descriptor,
    (const unsigned char *const) &string1_descriptor,
    (const unsigned char *const) &string2_descriptor,
};
