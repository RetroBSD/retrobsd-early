/*
 * This file contains all of functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the CDC function
 * driver. This file should be included in projects that use the CDC
 * \function driver.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC® Microcontroller is intended and
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
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/usb_device.h>
#include <machine/usb_function_cdc.h>

unsigned cdc_trf_state;         // States are defined cdc.h
unsigned cdc_tx_len;            // total tx length

LINE_CODING cdc_line_coding;	// Buffer to store line coding information

static const char *cdc_src;	// Dedicated source pointer

static USB_HANDLE data_out;
static USB_HANDLE data_in;

static CONTROL_SIGNAL_BITMAP control_signal_bitmap;

static volatile unsigned char cdc_data_rx [CDC_DATA_OUT_EP_SIZE];
static volatile unsigned char cdc_data_tx [CDC_DATA_IN_EP_SIZE];

/*
 * SEND_ENCAPSULATED_COMMAND and GET_ENCAPSULATED_RESPONSE are required
 * requests according to the CDC specification.
 * However, it is not really being used here, therefore a dummy buffer is
 * used for conformance.
 */
#define DUMMY_LENGTH    0x08

static unsigned char dummy_encapsulated_cmd_response [DUMMY_LENGTH];

/*
 * This routine checks the setup data packet to see if it
 * knows how to handle it.
 */
void cdc_check_request()
{
    /*
     * If request recipient is not an interface then return
     */
    if (usb_setup_pkt.Recipient != RCPT_INTF)
        return;

    /*
     * If request type is not class-specific then return
     */
    if (usb_setup_pkt.RequestType != CLASS)
        return;

    /*
     * Interface ID must match interface numbers associated with
     * CDC class, else return
     */
    if (usb_setup_pkt.bIntfID != CDC_COMM_INTF_ID &&
        usb_setup_pkt.bIntfID != CDC_DATA_INTF_ID)
        return;

    switch (usb_setup_pkt.bRequest)
    {
    //****** These commands are required *//
    case SEND_ENCAPSULATED_COMMAND:
     //send the packet
        usb_in_pipe[0].pSrc.bRam = (unsigned char*) &dummy_encapsulated_cmd_response;
        usb_in_pipe[0].wCount = DUMMY_LENGTH;
        usb_in_pipe[0].info.bits.ctrl_trf_mem = USB_INPIPES_RAM;
        usb_in_pipe[0].info.bits.busy = 1;
        break;
    case GET_ENCAPSULATED_RESPONSE:
        // Populate dummy_encapsulated_cmd_response first.
        usb_in_pipe[0].pSrc.bRam = (unsigned char*) &dummy_encapsulated_cmd_response;
        usb_in_pipe[0].info.bits.busy = 1;
        break;
    //****** End of required commands *//

#ifdef USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1
    case SET_LINE_CODING:
        usb_out_pipe[0].wCount = usb_setup_pkt.wLength;
        usb_out_pipe[0].pDst.bRam = (unsigned char*) &cdc_line_coding._byte[0];
        usb_out_pipe[0].pFunc = 0;
        usb_out_pipe[0].info.bits.busy = 1;
        break;

    case GET_LINE_CODING:
        usb_ep0_send_ram_ptr ((unsigned char*) &cdc_line_coding,
            LINE_CODING_LENGTH, USB_EP0_INCLUDE_ZERO);
        break;

    case SET_CONTROL_LINE_STATE:
        control_signal_bitmap._byte = (unsigned char)usb_setup_pkt.W_Value;
        CONFIGURE_RTS(control_signal_bitmap.CARRIER_CONTROL);
        CONFIGURE_DTR(control_signal_bitmap.DTE_PRESENT);
        usb_in_pipe[0].info.bits.busy = 1;
        break;
#endif

#ifdef USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2
    case SEND_BREAK:            // Optional
        usb_in_pipe[0].info.bits.busy = 1;
        if (usb_setup_pkt.wValue == 0xFFFF) {
            UART_ENABLE = 0;    // turn off USART
            UART_TRISTx = 0;    // Make TX pin an output
            UART_Tx = 0;        // make it low
        }
        else if (usb_setup_pkt.wValue == 0x0000) {
            UART_ENABLE = 1;    // turn on USART
            UART_TRISTx = 1;    // Make TX pin an input
        }
        else {
            UART_SEND_BREAK();
        }
        break;
#endif
    default:
        break;
    }
}

/*
 * This function initializes the CDC function driver. This function sets
 * the default line coding (baud rate, bit parity, number of data bits,
 * and format). This function also enables the endpoints and prepares for
 * the first transfer from the host.
 *
 * This function should be called after the SET_CONFIGURATION command.
 * This is most simply done by calling this function from the
 * usbcb_init_ep() function.
 *
 * Usage:
 *     void usbcb_init_ep()
 *     {
 *         cdc_init_ep();
 *     }
 */
void cdc_init_ep()
{
    // Abstract line coding information
    cdc_line_coding.dwDTERate = 19200;	// baud rate
    cdc_line_coding.bCharFormat = 0;	// 1 stop bit
    cdc_line_coding.bParityType = 0;	// None
    cdc_line_coding.bDataBits = 8;	// 5,6,7,8, or 16

    cdc_trf_state = CDC_TX_READY;

    /*
     * Do not have to init Cnt of IN pipes here.
     * Reason:  Number of BYTEs to send to the host
     *          varies from one transaction to
     *          another. Cnt should equal the exact
     *          number of BYTEs to transmit for
     *          a given IN transaction.
     *          This number of BYTEs will only
     *          be known right before the data is
     *          sent.
     */
    usb_enable_endpoint (CDC_COMM_EP, USB_IN_ENABLED |
        USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
    usb_enable_endpoint (CDC_DATA_EP, USB_IN_ENABLED | USB_OUT_ENABLED |
        USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);

    data_out = usb_rx_one_packet (CDC_DATA_EP,
        (unsigned char*) &cdc_data_rx, sizeof(cdc_data_rx));
    data_in = 0;
}

#if 0
/*
 * cdc_gets copies a string of BYTEs received through USB CDC Bulk OUT
 * endpoint to a user's specified location. It is a non-blocking function.
 * It does not wait for data if there is no data available. Instead it
 * returns '0' to notify the caller that there is no data available.
 *
 * Usage:
 *       unsigned char numBytes;
 *       unsigned char buffer[64]
 *
 *       numBytes = cdc_gets(buffer,sizeof(buffer)); //until the buffer is free.
 *       if (numBytes > 0)
 *       {
 *           // we received numBytes bytes of data and they are copied into
 *           // the "buffer" variable.  We can do something with the data here.
 *       }
 *
 * Conditions:
 *   Value of input argument 'len' should be smaller than the maximum
 *   endpoint size responsible for receiving bulk data from USB host for CDC
 *   class. Input argument 'buffer' should point to a buffer area that is
 *   bigger or equal to the size specified by 'len'.
 * Input:
 *   buffer -  Pointer to where received BYTEs are to be stored
 *   len -     The number of BYTEs expected.
 */
unsigned cdc_gets(char *buffer, unsigned len)
{
    unsigned n;

    if (usb_handle_busy (data_out))
        return 0;
    /*
     * Adjust the expected number of BYTEs to equal
     * the actual number of BYTEs received.
     */
    n = usb_handle_get_length (data_out);
    if (len > n)
        len = n;

    /*
     * Copy data from dual-ram buffer to user's buffer
     */
    for (n=0; n<len; n++)
        buffer[n] = cdc_data_rx[n];

    /*
     * Prepare dual-ram buffer for next OUT transaction
     */
    data_out = usb_rx_one_packet (CDC_DATA_EP,
            (unsigned char*) &cdc_data_rx, sizeof(cdc_data_rx));
    return n;
}
#endif

unsigned cdc_consume (void (*func) (int))
{
    unsigned n, len;

    if (usb_handle_busy (data_out))
        return 0;

    /*
     * Pass received data to user function.
     */
    len = usb_handle_get_length (data_out);
    for (n=0; n<len; n++)
        func (cdc_data_rx[n]);

    /*
     * Prepare dual-ram buffer for next OUT transaction
     */
    data_out = usb_rx_one_packet (CDC_DATA_EP,
            (unsigned char*) &cdc_data_rx, sizeof(cdc_data_rx));
    return n;
}

/*
 * cdc_put writes an array of data to the USB. Use this version, is
 * capable of transfering 0x00 (what is typically a NULL character in any of
 * the string transfer functions).
 *
 * Usage:
 *       if (cdc_is_tx_ready())
 *       {
 *           char data[] = {0x00, 0x01, 0x02, 0x03, 0x04};
 *           cdc_put (data, 5);
 *       }
 *
 * The transfer mechanism for device-to-host(put) is more flexible than
 * host-to-device(get). It can handle a string of data larger than the
 * maximum size of bulk IN endpoint. A state machine is used to transfer a
 * long string of data over multiple USB transactions. cdc_tx_service()
 * must be called periodically to keep sending blocks of data to the host.
 *
 * Conditions:
 *   cdc_is_tx_ready() must return TRUE. This indicates that the last
 *   transfer is complete and is ready to receive a new block of data. The
 *   string of characters pointed to by 'data' must equal to or smaller than
 *   255 BYTEs.
 *
 * Input:
 *   char *data - pointer to a RAM array of data to be transfered to the host
 *   unsigned char length - the number of bytes to be transfered (must be less than 255).
 */
void cdc_put (char *data, unsigned length)
{
    /*
     * User should have checked that cdc_trf_state is in CDC_TX_READY state
     * before calling this function.
     * As a safety precaution, this fuction checks the state one more time
     * to make sure it does not override any pending transactions.
     *
     * Currently it just quits the routine without reporting any errors back
     * to the user.
     *
     * Bottomline: User MUST make sure that cdc_is_tx_ready()==1
     *             before calling this function!
     * Example:
     * if (cdc_is_tx_ready())
     *     cdc_put(pData, Length);
     *
     * IMPORTANT: Never use the following blocking while loop to wait:
     * while (!cdc_is_tx_ready())
     *     cdc_put(pData, Length);
     *
     * The whole firmware framework is written based on cooperative
     * multi-tasking and a blocking code is not acceptable.
     * Use a state machine instead.
     */
    if (cdc_trf_state == CDC_TX_READY)
    {
        cdc_src = data;
        cdc_tx_len = length;
        cdc_trf_state = CDC_TX_BUSY;
    }
}

/*
 * cdc_puts writes a string of data to the USB including the null
 * character. Use this version, 'puts', to transfer data from a RAM buffer.
 *
 * Usage:
 *       if (cdc_is_tx_ready())
 *       {
 *           char data[] = "Hello World";
 *           cdc_puts(data);
 *       }
 *
 * The transfer mechanism for device-to-host(put) is more flexible than
 * host-to-device(get). It can handle a string of data larger than the
 * maximum size of bulk IN endpoint. A state machine is used to transfer a
 * \long string of data over multiple USB transactions. cdc_tx_service()
 * must be called periodically to keep sending blocks of data to the host.
 *
 * Conditions:
 *   cdc_is_tx_ready() must return TRUE. This indicates that the last
 *   transfer is complete and is ready to receive a new block of data. The
 *   string of characters pointed to by 'data' must equal to or smaller than
 *   255 BYTEs.
 *
 * Input:
 *   char *data -  null\-terminated string of constant data. If a
 *                           null character is not found, 255 BYTEs of data
 *                           will be transferred to the host.
 */
void cdc_puts(char *data)
{
    unsigned char len;
    char *pData;

    /*
     * User should have checked that cdc_trf_state is in CDC_TX_READY state
     * before calling this function.
     * As a safety precaution, this fuction checks the state one more time
     * to make sure it does not override any pending transactions.
     *
     * Currently it just quits the routine without reporting any errors back
     * to the user.
     *
     * Bottomline: User MUST make sure that cdc_is_tx_ready()==1
     *             before calling this function!
     * Example:
     * if (cdc_is_tx_ready())
     *     cdc_puts(pData, Length);
     *
     * IMPORTANT: Never use the following blocking while loop to wait:
     * while (!cdc_is_tx_ready())
     *     cdc_puts(pData);
     *
     * The whole firmware framework is written based on cooperative
     * multi-tasking and a blocking code is not acceptable.
     * Use a state machine instead.
     */
    if (cdc_trf_state != CDC_TX_READY)
        return;

    /*
     * While loop counts the number of BYTEs to send including the
     * null character.
     */
    len = 0;
    pData = data;
    do {
        len++;
        if (len == 255)
            break;       // Break loop once max len is reached.
    } while (*pData++);

    /*
     * Second piece of information (length of data to send) is ready.
     * Call tx_ram to setup the transfer.
     * The actual transfer process will be handled by cc_tx_service(),
     * which should be called once per Main Program loop.
     */
    cdc_src = data;
    cdc_tx_len = len;
    cdc_trf_state = CDC_TX_BUSY;
}

/*
 * cdc_putrs writes a string of data to the USB including the null
 * character. Use this version, 'putrs', to transfer data literals and
 * data located in program memory.
 *
 * Usage:
 *       if (cdc_is_tx_ready())
 *       {
 *           cdc_putrs("Hello World");
 *       }
 *
 * The transfer mechanism for device-to-host(put) is more flexible than
 * host-to-device(get). It can handle a string of data larger than the
 * maximum size of bulk IN endpoint. A state machine is used to transfer a
 * long string of data over multiple USB transactions. cdc_tx_service()
 * must be called periodically to keep sending blocks of data to the host.
 *
 * Conditions:
 *   cdc_is_tx_ready() must return TRUE. This indicates that the last
 *   transfer is complete and is ready to receive a new block of data. The
 *   string of characters pointed to by 'data' must equal to or smaller than
 *   255 BYTEs.
 *
 * Input:
 *   const char *data -  null\-terminated string of constant data. If a
 *                       null character is not found, 255 BYTEs of data
 *                       will be transferred to the host.
 */
void cdc_putrs(const char *data)
{
    unsigned char len;
    const char *pData;

    /*
     * User should have checked that cdc_trf_state is in CDC_TX_READY state
     * before calling this function.
     * As a safety precaution, this fuction checks the state one more time
     * to make sure it does not override any pending transactions.
     *
     * Currently it just quits the routine without reporting any errors back
     * to the user.
     *
     * Bottomline: User MUST make sure that cdc_is_tx_ready()
     *             before calling this function!
     * Example:
     * if (cdc_is_tx_ready())
     *     cdc_puts(pData);
     *
     * IMPORTANT: Never use the following blocking while loop to wait:
     * while (cdc_trf_state != CDC_TX_READY)
     *     cdc_puts(pData);
     *
     * The whole firmware framework is written based on cooperative
     * multi-tasking and a blocking code is not acceptable.
     * Use a state machine instead.
     */
    if (cdc_trf_state != CDC_TX_READY)
        return;

    /*
     * While loop counts the number of BYTEs to send including the
     * null character.
     */
    len = 0;
    pData = data;
    do {
        len++;
        if (len == 255)
            break;       // Break loop once max len is reached.
    } while (*pData++);

    /*
     * Second piece of information (length of data to send) is ready.
     * The actual transfer process will be handled by cdc_tx_service(),
     * which should be called once per Main Program loop.
     */
    cdc_src = data;
    cdc_tx_len = len;
    cdc_trf_state = CDC_TX_BUSY;
}

/*
 * cdc_tx_service handles device-to-host transaction(s). This function
 * should be called once per Main Program loop after the device reaches
 * the configured state.
 *
 * Usage:
 *   void main()
 *   {
 *       usb_device_init();
 *       while (1)
 *       {
 *           usb_device_tasks();
 *           if (USBGetDeviceState() < CONFIGURED_STATE || USBIsDeviceSuspended())
 *           {
 *               // Either the device is not configured or we are suspended
 *               // so we don't want to do execute any application code
 *               continue;   // go back to the top of the while loop
 *           } else {
 *               // Keep trying to send data to the PC as required
 *               cdc_tx_service();
 *
 *               // Run application code.
 *               UserApplication();
 *           }
 *       }
 *   }
 */
void cdc_tx_service()
{
    unsigned char nbytes_to_send;

    // Check that USB connection is established.
    if (usb_device_state < CONFIGURED_STATE ||
        (U1PWRC & PIC32_U1PWRC_USUSPEND))
        return;

    if (usb_handle_busy(data_in))
        return;
    /*
     * Completing stage is necessary while [ mCDCUSartTxIsBusy()==1 ].
     * By having this stage, user can always check cdc_trf_state,
     * and not having to call mCDCUsartTxIsBusy() directly.
     */
    if (cdc_trf_state == CDC_TX_COMPLETING)
        cdc_trf_state = CDC_TX_READY;

    /*
     * If CDC_TX_READY state, nothing to do, just return.
     */
    if (cdc_trf_state == CDC_TX_READY)
        return;

    /*
     * If CDC_TX_BUSY_ZLP state, send zero length packet
     */
    if (cdc_trf_state == CDC_TX_BUSY_ZLP)
    {
        data_in = usb_tx_one_packet (CDC_DATA_EP, 0, 0);
        cdc_trf_state = CDC_TX_COMPLETING;
    }
    else if (cdc_trf_state == CDC_TX_BUSY) {
        /*
         * First, have to figure out how many byte of data to send.
         */
    	if (cdc_tx_len > sizeof(cdc_data_tx))
    	    nbytes_to_send = sizeof(cdc_data_tx);
    	else
    	    nbytes_to_send = cdc_tx_len;

        /*
         * Subtract the number of bytes just about to be sent from the total.
         */
    	cdc_tx_len -= nbytes_to_send;

        bcopy (cdc_src, &cdc_data_tx, nbytes_to_send);
        cdc_src += nbytes_to_send;

        /*
         * Lastly, determine if a zero length packet state is necessary.
         * See explanation in USB Specification 2.0: Section 5.8.3
         */
        if (cdc_tx_len == 0)
        {
            if (nbytes_to_send == CDC_DATA_IN_EP_SIZE)
                cdc_trf_state = CDC_TX_BUSY_ZLP;
            else
                cdc_trf_state = CDC_TX_COMPLETING;
        }
        data_in = usb_tx_one_packet (CDC_DATA_EP, (unsigned char*)&cdc_data_tx, nbytes_to_send);
    }
}
