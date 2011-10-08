/*
 * This file contains functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the MCHPFSUSB device
 * stack. This file should be included in projects that use the device stack.
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
#include <machine/pic32mx.h>
#include <machine/usb_ch9.h>
#include <machine/usb.h>
#include <machine/usb_device.h>

#if (USB_PING_PONG_MODE != USB_PING_PONG__FULL_PING_PONG)
    #error "PIC32 only supports full ping pong mode."
#endif

USB_VOLATILE unsigned char USBDeviceState;
USB_VOLATILE unsigned char USBActiveConfiguration;
USB_VOLATILE unsigned char USBAlternateInterface[USB_MAX_NUM_INT];
volatile BDT_ENTRY *pBDTEntryEP0OutCurrent;
volatile BDT_ENTRY *pBDTEntryEP0OutNext;
volatile BDT_ENTRY *pBDTEntryOut[USB_MAX_EP_NUMBER+1];
volatile BDT_ENTRY *pBDTEntryIn[USB_MAX_EP_NUMBER+1];
USB_VOLATILE unsigned char shortPacketStatus;
USB_VOLATILE unsigned char controlTransferState;
USB_VOLATILE IN_PIPE inPipes[1];
USB_VOLATILE OUT_PIPE outPipes[1];
USB_VOLATILE unsigned char *pDst;
USB_VOLATILE int RemoteWakeup;
USB_VOLATILE unsigned char USTATcopy;
USB_VOLATILE unsigned short USBInMaxPacketSize[USB_MAX_EP_NUMBER];
USB_VOLATILE unsigned char *USBInData[USB_MAX_EP_NUMBER];

/*
 * Section A: Buffer Descriptor Table
 * - 0x400 - 0x4FF(max)
 * - USB_MAX_EP_NUMBER is defined in target.cfg
 */
volatile BDT_ENTRY BDT [(USB_MAX_EP_NUMBER + 1) * 4] __attribute__ ((aligned (512)));

/*
 * Section B: EP0 Buffer Space
 */
volatile CTRL_TRF_SETUP SetupPkt;           // 8-byte only
volatile unsigned char CtrlTrfData[USB_EP0_BUFF_SIZE];

/*
 * This function initializes the device stack
 * it in the default state
 *
 * The USB module will be completely reset including
 * all of the internal variables, registers, and
 * interrupt flags.
 */
void USBDeviceInit(void)
{
    unsigned char i;

    // Clear all USB error flags
    USBClearInterruptRegister(U1EIR);

    // Clears all USB interrupts
    USBClearInterruptRegister(U1IR);

    U1EIE = 0x9F;                   // Unmask all USB error interrupts
    U1IE = 0xFB;                    // Enable all interrupts except ACTVIE

    //power up the module
    U1PWRC |= PIC32_U1PWRC_USBPWR;

    //set the address of the BDT (if applicable)
    U1BDTP1 = (unsigned) BDT >> 8;

    // Reset all of the Ping Pong buffers
    U1CON |= PIC32_U1CON_PPBRST;
    U1CON &= ~PIC32_U1CON_PPBRST;

    // Reset to default address
    U1ADDR = 0x00;

    //Clear all of the endpoint control registers
    bzero((void*) &U1EP1, USB_MAX_EP_NUMBER - 1);

    //Clear all of the BDT entries
    for(i=0;i<(sizeof(BDT)/sizeof(BDT_ENTRY));i++)
    {
        BDT[i].Val = 0x00;
    }

    // Initialize EP0 as a Ctrl EP
    U1EP0 = EP_CTRL | USB_HANDSHAKE_ENABLED;

    // Flush any pending transactions
    while (USBTransactionCompleteIF)
    {
        USBClearInterruptFlag (USBTransactionCompleteIFReg, USBTransactionCompleteIFBitNum);
    }

    //clear all of the internal pipe information
    inPipes[0].info.Val = 0;
    outPipes[0].info.Val = 0;
    outPipes[0].wCount = 0;

    // Make sure packet processing is enabled
    U1CON &= ~PIC32_U1CON_PKTDIS;

    //Get ready for the first packet
    pBDTEntryIn[0] = (volatile BDT_ENTRY*) &BDT[EP0_IN_EVEN];

    // Clear active configuration
    USBActiveConfiguration = 0;

    //Indicate that we are now in the detached state
    USBDeviceState = DETACHED_STATE;
}

/*
 * This function is the main state machine of the
 * USB device side stack.  This function should be
 * called periodically to receive and transmit
 * packets through the stack.  This function should
 * be called  preferably once every 100us
 * during the enumeration process.  After the
 * enumeration process this function still needs to
 * be called periodically to respond to various
 * situations on the bus but is more relaxed in its
 * time requirements.  This function should also
 * be called at least as fast as the OUT data
 * expected from the PC.
 */
void USBDeviceTasks(void)
{
    unsigned char i;

#ifdef USB_SUPPORT_OTG
    //SRP Time Out Check
    if (USBOTGSRPIsReady())
    {
        if (USBT1MSECIF && USBT1MSECIE)
        {
            if (USBOTGGetSRPTimeOutFlag())
            {
                if (USBOTGIsSRPTimeOutExpired())
                {
                    USB_OTGEventHandler(0,OTG_EVENT_SRP_FAILED,0,0);
                }
            }
            //Clear Interrupt Flag
            USBClearInterruptFlag(USBT1MSECIFReg,USBT1MSECIFBitNum);
        }
    }
    //If Session Is Started Then
    else {
        //If SRP Is Ready
        if (USBOTGSRPIsReady())
        {
            //Clear SRPReady
            USBOTGClearSRPReady();

            //Clear SRP Timeout Flag
            USBOTGClearSRPTimeOutFlag();

            //Indicate Session Started
            UART2PrintString( "\r\n***** USB OTG B Event - Session Started  *****\r\n" );
        }
    }
#endif

    //if we are in the detached state
    if(USBDeviceState == DETACHED_STATE)
    {
	// Disable module & detach from bus
        U1CON = 0;

        // Mask all USB interrupts
        U1IE = 0;

        // Enable module & attach to bus
        while (! (U1CON & PIC32_U1CON_USBEN)) {
		U1CON |= PIC32_U1CON_USBEN;
	}

        //moved to the attached state
        USBDeviceState = ATTACHED_STATE;

        //Enable/set things like: pull ups, full/low-speed mode,
        //set the ping pong mode, and set internal transceiver
        SetConfigurationOptions();

#ifdef  USB_SUPPORT_OTG
	U1OTGCON = USB_OTG_DPLUS_ENABLE | USB_OTG_ENABLE;
#endif
    }

    if(USBDeviceState == ATTACHED_STATE)
    {
        /*
         * After enabling the USB module, it takes some time for the
         * voltage on the D+ or D- line to rise high enough to get out
         * of the SE0 condition. The USB Reset interrupt should not be
         * unmasked until the SE0 condition is cleared. This helps
         * prevent the firmware from misinterpreting this unique event
         * as a USB bus reset from the USB host.
         */
        U1IR = 0;			// Clear all USB interrupts
        U1IE = 0;			// Mask all USB interrupts
	U1IE |= PIC32_U1I_URST |	// Unmask RESET interrupt
		PIC32_U1I_IDLE;		// Unmask IDLE interrupt
	USBDeviceState = POWERED_STATE;
    }

    #ifdef  USB_SUPPORT_OTG
        //If ID Pin Changed State
        if (USBIDIF && USBIDIE)
        {
            //Re-detect & Initialize
            USBOTGInitialize();

            USBClearInterruptFlag(USBIDIFReg,USBIDIFBitNum);
        }
    #endif

    /*
     * Task A: Service USB Activity Interrupt
     */
    if(USBActivityIF && USBActivityIE)
    {
        #if defined(USB_SUPPORT_OTG)
        USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
            U1OTGIR = 0x10;
        #else
            USBWakeFromSuspend();
        #endif
    }

    /*
     * Pointless to continue servicing if the device is in suspend mode.
     */
    if (U1PWRC & PIC32_U1PWRC_USUSPEND)
    {
        return;
    }

    /*
     * Task B: Service USB Bus Reset Interrupt.
     * When bus reset is received during suspend, ACTVIF will be set first,
     * once the UCON_SUSPND is clear, then the URSTIF bit will be asserted.
     * This is why URSTIF is checked after ACTVIF.
     *
     * The USB reset flag is masked when the USB state is in
     * DETACHED_STATE or ATTACHED_STATE, and therefore cannot
     * cause a USB reset event during these two states.
     */
    if(USBResetIF && USBResetIE)
    {
        USBDeviceInit();
        USBDeviceState = DEFAULT_STATE;

        /*
        Bug Fix: Feb 26, 2007 v2.1 (#F1)
        *********************************************************************
        In the original firmware, if an OUT token is sent by the host
        before a SETUP token is sent, the firmware would respond with an ACK.
        This is not a correct response, the firmware should have sent a STALL.
        This is a minor non-compliance since a compliant host should not
        send an OUT before sending a SETUP token. The fix allows a SETUP
        transaction to be accepted while stalling OUT transactions.
        */
        BDT[EP0_OUT_EVEN].ADR = ConvertToPhysicalAddress (&SetupPkt);
        BDT[EP0_OUT_EVEN].CNT = USB_EP0_BUFF_SIZE;
        BDT[EP0_OUT_EVEN].STAT.Val &= ~_STAT_MASK;
        BDT[EP0_OUT_EVEN].STAT.Val |= _USIE|_DAT0|_DTSEN|_BSTALL;

        #ifdef USB_SUPPORT_OTG
             //Disable HNP
             USBOTGDisableHnp();

             //Deactivate HNP
             USBOTGDeactivateHnp();
        #endif
    }

    /*
     * Task C: Service other USB interrupts
     */
    if(USBIdleIF && USBIdleIE)
    {
        #ifdef  USB_SUPPORT_OTG
            //If Suspended, Try to switch to Host
            USBOTGSelectRole(ROLE_HOST);
        #else
            USBSuspend();
        #endif

        USBClearInterruptFlag(USBIdleIFReg,USBIdleIFBitNum);
    }

    if(USBSOFIF && USBSOFIE)
    {
        USBCB_SOF_Handler();    // Required callback, see usbcallbacks.c
        USBClearInterruptFlag(USBSOFIFReg,USBSOFIFBitNum);
    }

    if(USBStallIF && USBStallIE)
    {
        USBStallHandler();
    }

    if(USBErrorIF && USBErrorIE)
    {
        USBCBErrorHandler();    // Required callback, see usbcallbacks.c
        USBClearInterruptRegister(U1EIR);               // This clears UERRIF
    }

    /*
     * Pointless to continue servicing if the host has not sent a bus reset.
     * Once bus reset is received, the device transitions into the DEFAULT
     * state and is ready for communication.
     */
    if(USBDeviceState < DEFAULT_STATE) return;

    /*
     * Task D: Servicing USB Transaction Complete Interrupt
     */
    if(USBTransactionCompleteIE)
    {
	    for(i = 0; i < 4; i++)	//Drain or deplete the USAT FIFO entries.  If the USB FIFO ever gets full, USB bandwidth
		{						//utilization can be compromised, and the device won't be able to receive SETUP packets.
		    if(USBTransactionCompleteIF)
		    {
		        USTATcopy = U1STAT;

		        USBClearInterruptFlag(USBTransactionCompleteIFReg,USBTransactionCompleteIFBitNum);

		        /*
		         * USBCtrlEPService only services transactions over EP0.
		         * It ignores all other EP transactions.
		         */
		        USBCtrlEPService();
		    }//end if(USBTransactionCompleteIF)
		    else
		    	break;	//USTAT FIFO must be empty.
		}
	}
}

/*
 * This function handles the event of a STALL occuring on the bus
 */
void USBStallHandler(void)
{
    /*
     * Does not really have to do anything here,
     * even for the control endpoint.
     * All BDs of Endpoint 0 are owned by SIE right now,
     * but once a Setup Transaction is received, the ownership
     * for EP0_OUT will be returned to CPU.
     * When the Setup Transaction is serviced, the ownership
     * for EP0_IN will then be forced back to CPU by firmware.
     */

    /* v2b fix */
    if (U1EP0 & PIC32_U1EP_EPSTALL)
    {
        // UOWN - if 0, owned by CPU, if 1, owned by SIE
        if((pBDTEntryEP0OutCurrent->STAT.Val == _USIE) && (pBDTEntryIn[0]->STAT.Val == (_USIE|_BSTALL)))
        {
            // Set ep0Bo to stall also
            pBDTEntryEP0OutCurrent->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
        }
	// Clear stall status
	U1EP0 &= ~PIC32_U1EP_EPSTALL;
    }//end if

    USBClearInterruptFlag(USBStallIFReg,USBStallIFBitNum);
}

/*
 * This function handles if the host tries to suspend the device
 */
void USBSuspend(void)
{
    /*
     * NOTE: Do not clear UIR_ACTVIF here!
     * Reason:
     * ACTVIF is only generated once an IDLEIF has been generated.
     * This is a 1:1 ratio interrupt generation.
     * For every IDLEIF, there will be only one ACTVIF regardless of
     * the number of subsequent bus transitions.
     *
     * If the ACTIF is cleared here, a problem could occur when:
     * [       IDLE       ][bus activity ->
     * <--- 3 ms ----->     ^
     *                ^     ACTVIF=1
     *                IDLEIF=1
     *  #           #           #           #   (#=Program polling flags)
     *                          ^
     *                          This polling loop will see both
     *                          IDLEIF=1 and ACTVIF=1.
     *                          However, the program services IDLEIF first
     *                          because ACTIVIE=0.
     *                          If this routine clears the only ACTIVIF,
     *                          then it can never get out of the suspend
     *                          mode.
     */

    U1OTGIE |= PIC32_U1OTGI_ACTV;	// Enable bus activity interrupt
    USBClearInterruptFlag(USBIdleIFReg,USBIdleIFBitNum);

    /*
     * At this point the PIC can go into sleep,idle, or
     * switch to a slower clock, etc.  This should be done in the
     * USBCBSuspend() if necessary.
     */
    USBCBSuspend();             // Required callback, see usbcallbacks.c
}

/*
 *
 */
void USBWakeFromSuspend(void)
{
    /*
     * If using clock switching, the place to restore the original
     * microcontroller core clock frequency is in the USBCBWakeFromSuspend() callback
     */
    USBCBWakeFromSuspend(); // Required callback, see usbcallbacks.c

    U1OTGIE &= ~PIC32_U1OTGI_ACTV;

    /*
    Bug Fix: Feb 26, 2007 v2.1
    *********************************************************************
    The ACTVIF bit cannot be cleared immediately after the USB module wakes
    up from Suspend or while the USB module is suspended. A few clock cycles
    are required to synchronize the internal hardware state machine before
    the ACTIVIF bit can be cleared by firmware. Clearing the ACTVIF bit
    before the internal hardware is synchronized may not have an effect on
    the value of ACTVIF. Additonally, if the USB module uses the clock from
    the 96 MHz PLL source, then after clearing the SUSPND bit, the USB
    module may not be immediately operational while waiting for the 96 MHz
    PLL to lock.
    */

    // UIRbits.ACTVIF = 0;                      // Removed
    #if defined(__18CXX)
    while(USBActivityIF)
    #endif
    {
        USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
    }  // Added
}

/*
 * USBCtrlEPService checks for three transaction
 * types that it knows how to service and services them:
 * 1. EP0 SETUP
 * 2. EP0 OUT
 * 3. EP0 IN
 * It ignores all other types (i.e. EP1, EP2, etc.)
 *
 * PreCondition: USTAT is loaded with a valid endpoint address.
 */
void USBCtrlEPService(void)
{
	//If the last packet was a EP0 OUT packet
    if((USTATcopy & USTAT_EP0_PP_MASK) == USTAT_EP0_OUT_EVEN)
    {
		//Point to the EP0 OUT buffer of the buffer that arrived
        pBDTEntryEP0OutCurrent = (volatile BDT_ENTRY*)&BDT[(USTATcopy & USTAT_EP_MASK)>>2];

		//Set the next out to the current out packet
        pBDTEntryEP0OutNext = pBDTEntryEP0OutCurrent;
		//Toggle it to the next ping pong buffer (if applicable)
        *(unsigned char*)&pBDTEntryEP0OutNext ^= USB_NEXT_EP0_OUT_PING_PONG;

		//If the current EP0 OUT buffer has a SETUP token
        if(pBDTEntryEP0OutCurrent->STAT.PID == SETUP_TOKEN)
        {
			//Handle the control transfer
            USBCtrlTrfSetupHandler();
        }
        else
        {
			//Handle the DATA transfer
            USBCtrlTrfOutHandler();
        }
    }
    else if((USTATcopy & USTAT_EP0_PP_MASK) == USTAT_EP0_IN)
    {
		//Otherwise the transmission was and EP0 IN
		//  so take care of the IN transfer
        USBCtrlTrfInHandler();
    }
}

/*
 * This routine is a task dispatcher and has 3 stages.
 * 1. It initializes the control transfer state machine.
 * 2. It calls on each of the module that may know how to
 *    service the Setup Request from the host.
 *    Module Example: USBD, HID, CDC, MSD, ...
 *    A callback function, USBCBCheckOtherReq(),
 *    is required to call other module handlers.
 * 3. Once each of the modules has had a chance to check if
 *    it is responsible for servicing the request, stage 3
 *    then checks direction of the transfer to determine how
 *    to prepare EP0 for the control transfer.
 *    Refer to USBCtrlEPServiceComplete() for more details.
 *
 * PreCondition: SetupPkt buffer is loaded with valid USB Setup Data
 *
 * Microchip USB Firmware has three different states for
 * the control transfer state machine:
 * 1. WAIT_SETUP
 * 2. CTRL_TRF_TX
 * 3. CTRL_TRF_RX
 * Refer to firmware manual to find out how one state
 * is transitioned to another.
 *
 * A Control Transfer is composed of many USB transactions.
 * When transferring data over multiple transactions,
 * it is important to keep track of data source, data
 * destination, and data count. These three parameters are
 * stored in pSrc,pDst, and wCount. A flag is used to
 * note if the data source is from ROM or RAM.
 */
void USBCtrlTrfSetupHandler(void)
{
	//if the SIE currently owns the buffer
    if(pBDTEntryIn[0]->STAT.UOWN != 0)
    {
		//give control back to the CPU
		//  Compensate for after a STALL
        pBDTEntryIn[0]->STAT.Val = _UCPU;
    }

	//Keep track of if a short packet has been sent yet or not
    shortPacketStatus = SHORT_PKT_NOT_USED;

    /* Stage 1 */
    controlTransferState = WAIT_SETUP;

    inPipes[0].wCount = 0;
    inPipes[0].info.Val = 0;

    /* Stage 2 */
    USBCheckStdRequest();
    USBCBCheckOtherReq();   // Required callback, see usbcallbacks.c

    /* Stage 3 */
    USBCtrlEPServiceComplete();
}

/*
 * This routine handles an OUT transaction according to
 * which control transfer state is currently active.
 *
 * Note that if the the control transfer was from
 * host to device, the session owner should be notified
 * at the end of each OUT transaction to service the
 * received data.
 */
void USBCtrlTrfOutHandler(void)
{
    if(controlTransferState == CTRL_TRF_RX)
    {
        USBCtrlTrfRxService();
    }
    else    // CTRL_TRF_TX
    {
        USBPrepareForNextSetupTrf();
    }
}

/*
 * This routine handles an IN transaction according to
 * which control transfer state is currently active.
 *
 * A Set Address Request must not change the acutal address
 * of the device until the completion of the control
 * transfer. The end of the control transfer for Set Address
 * Request is an IN transaction. Therefore it is necessary
 * to service this unique situation when the condition is
 * right. Macro mUSBCheckAdrPendingState is defined in
 * usb9.h and its function is to specifically service this event.
 */
void USBCtrlTrfInHandler(void)
{
    unsigned char lastDTS;

    lastDTS = pBDTEntryIn[0]->STAT.DTS;

    //switch to the next ping pong buffer
    *(unsigned char*)&pBDTEntryIn[0] ^= USB_NEXT_EP0_IN_PING_PONG;

    //mUSBCheckAdrPendingState();       // Must check if in ADR_PENDING_STATE
    if(USBDeviceState == ADR_PENDING_STATE)
    {
        U1ADDR = SetupPkt.bDevADR;
        if(U1ADDR > 0)
        {
            USBDeviceState=ADDRESS_STATE;
        }
        else
        {
            USBDeviceState=DEFAULT_STATE;
        }
    }//end if


    if(controlTransferState == CTRL_TRF_TX)
    {
        pBDTEntryIn[0]->ADR = ConvertToPhysicalAddress(CtrlTrfData);
        USBCtrlTrfTxService();

        /* v2b fix */
        if(shortPacketStatus == SHORT_PKT_SENT)
        {
            // If a short packet has been sent, don't want to send any more,
            // stall next time if host is still trying to read.
            pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
        }
        else
        {
            if(lastDTS == 0)
            {
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;
            }
            else
            {
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT0|_DTSEN;
            }
        }//end if(...)else
    }
    else // CTRL_TRF_RX
    {
        USBPrepareForNextSetupTrf();
    }
}

/*
 * The routine forces EP0 OUT to be ready for a new
 * Setup transaction, and forces EP0 IN to be owned by CPU.
 */
void USBPrepareForNextSetupTrf(void)
{
    /*
    Bug Fix: Feb 26, 2007 v2.1
    *********************************************************************
    Facts:
    A Setup Packet should never be stalled. (USB 2.0 Section 8.5.3)
    If a Setup PID is detected by the SIE, the DTSEN setting is ignored.
    This causes a problem at the end of a control write transaction.
    In USBCtrlEPServiceComplete(), during a control write (Host to Device),
    the EP0_OUT is setup to write any data to the CtrlTrfData buffer.
    If <SETUP[0]><IN[1]> is completed and USBCtrlTrfInHandler() is not
    called before the next <SETUP[0]> is received, then the latest Setup
    data will be written to the CtrlTrfData buffer instead of the SetupPkt
    buffer.

    If USBCtrlTrfInHandler() was called before the latest <SETUP[0]> is
    received, then there would be no problem,
    because USBPrepareForNextSetupTrf() would have been called and updated
    ep0Bo.ADR to point to the SetupPkt buffer.

    Work around:
    Check for the problem as described above and copy the Setup data from
    CtrlTrfData to SetupPkt.
    */
    if((controlTransferState == CTRL_TRF_RX) &&
       (U1CON & PIC32_U1CON_PKTDIS) &&
       (pBDTEntryEP0OutCurrent->CNT == sizeof(CTRL_TRF_SETUP)) &&
       (pBDTEntryEP0OutCurrent->STAT.PID == SETUP_TOKEN) &&
       (pBDTEntryEP0OutNext->STAT.UOWN == 0))
    {
        unsigned char setup_cnt;

        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);

        // The Setup data was written to the CtrlTrfData buffer, must copy
        // it back to the SetupPkt buffer so that it can be processed correctly
        // by USBCtrlTrfSetupHandler().
        for(setup_cnt = 0; setup_cnt < sizeof(CTRL_TRF_SETUP); setup_cnt++)
        {
            *(((unsigned char*)&SetupPkt)+setup_cnt) = *(((unsigned char*)&CtrlTrfData)+setup_cnt);
        }//end for
    }
    /* End v3b fix */
    else
    {
        controlTransferState = WAIT_SETUP;
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;      // Defined in target.cfg
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);

        /*
        Bug Fix: Feb 26, 2007 v2.1 (#F1)
        *********************************************************************
        In the original firmware, if an OUT token is sent by the host
        before a SETUP token is sent, the firmware would respond with an ACK.
        This is not a correct response, the firmware should have sent a STALL.
        This is a minor non-compliance since a compliant host should not
        send an OUT before sending a SETUP token. The fix allows a SETUP
        transaction to be accepted while stalling OUT transactions.
        */
        //ep0Bo.Stat.Val = _USIE|_DAT0|_DTSEN;        // Removed
        pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;  //Added #F1

        /*
        Bug Fix: Feb 26, 2007 v2.1 (#F3)
        *********************************************************************
        In the original firmware, if an IN token is sent by the host
        before a SETUP token is sent, the firmware would respond with an ACK.
        This is not a correct response, the firmware should have sent a STALL.
        This is a minor non-compliance since a compliant host should not
        send an IN before sending a SETUP token.

        Comment why this fix (#F3) is interfering with fix (#AF1).
        */
        pBDTEntryIn[0]->STAT.Val = _UCPU;             // Should be removed

        {
            BDT_ENTRY* p;

            p = (BDT_ENTRY*)(((unsigned int)pBDTEntryIn[0])^USB_NEXT_EP0_IN_PING_PONG);
            p->STAT.Val = _UCPU;
        }

        //ep0Bi.Stat.Val = _USIE|_BSTALL;   // Should be added #F3
    }

    //if someone is still expecting data from the control transfer
    //  then make sure to terminate that request and let them know that
    //  they are done
    if(outPipes[0].info.bits.busy == 1) {
        if (outPipes[0].pFunc != 0) {
            outPipes[0].pFunc();
        }
        outPipes[0].info.bits.busy = 0;
    }

}//end USBPrepareForNextSetupTrf

/*
 * This routine checks the setup data packet to see
 * if it knows how to handle it
 */
void USBCheckStdRequest(void)
{
    if(SetupPkt.RequestType != STANDARD) return;

    switch(SetupPkt.bRequest)
    {
        case SET_ADR:
            inPipes[0].info.bits.busy = 1;            // This will generate a zero length packet
            USBDeviceState = ADR_PENDING_STATE;       // Update state only
            /* See USBCtrlTrfInHandler() for the next step */
            break;
        case GET_DSC:
            USBStdGetDscHandler();
            break;
        case SET_CFG:
            USBStdSetCfgHandler();
            break;
        case GET_CFG:
            inPipes[0].pSrc.bRam = (unsigned char*)&USBActiveConfiguration;	// Set Source
            inPipes[0].info.bits.ctrl_trf_mem = _RAM;			// Set memory type
            inPipes[0].wCount |= 0xff;					// Set data count
            inPipes[0].info.bits.busy = 1;
            break;
        case GET_STATUS:
            USBStdGetStatusHandler();
            break;
        case CLR_FEATURE:
        case SET_FEATURE:
            USBStdFeatureReqHandler();
            break;
        case GET_INTF:
            inPipes[0].pSrc.bRam = (unsigned char*)&USBAlternateInterface +
		SetupPkt.bIntfID;				// Set source
            inPipes[0].info.bits.ctrl_trf_mem = _RAM;		// Set memory type
            inPipes[0].wCount |= 0xff;				// Set data count
            inPipes[0].info.bits.busy = 1;
            break;
        case SET_INTF:
            inPipes[0].info.bits.busy = 1;
            USBAlternateInterface[SetupPkt.bIntfID] = SetupPkt.bAltID;
            break;
        case SET_DSC:
            USBCBStdSetDscHandler();
            break;
        case SYNCH_FRAME:
        default:
            break;
    }
}

/*
 * This routine handles the standard SET & CLEAR
 * FEATURES requests
 */
void USBStdFeatureReqHandler(void)
{
    BDT_ENTRY *p;
    unsigned int *pUEP;

#ifdef	USB_SUPPORT_OTG
    if ((SetupPkt.bFeature == OTG_FEATURE_B_HNP_ENABLE)&&
        (SetupPkt.Recipient == RCPT_DEV))
    {
        inPipes[0].info.bits.busy = 1;
        if(SetupPkt.bRequest == SET_FEATURE)
            USBOTGEnableHnp();
        else
            USBOTGDisableHnp();
    }

    if ((SetupPkt.bFeature == OTG_FEATURE_A_HNP_SUPPORT)&&
        (SetupPkt.Recipient == RCPT_DEV))
    {
        inPipes[0].info.bits.busy = 1;
        if(SetupPkt.bRequest == SET_FEATURE)
            USBOTGEnableSupportHnp();
        else
            USBOTGDisableSupportHnp();
    }


    if ((SetupPkt.bFeature == OTG_FEATURE_A_ALT_HNP_SUPPORT)&&
        (SetupPkt.Recipient == RCPT_DEV))
    {
        inPipes[0].info.bits.busy = 1;
        if(SetupPkt.bRequest == SET_FEATURE)
            USBOTGEnableAltHnp();
        else
            USBOTGDisableAltHnp();
    }
#endif
    if((SetupPkt.bFeature == DEVICE_REMOTE_WAKEUP)&&
       (SetupPkt.Recipient == RCPT_DEV))
    {
        inPipes[0].info.bits.busy = 1;
        if(SetupPkt.bRequest == SET_FEATURE)
            RemoteWakeup = 1;
        else
            RemoteWakeup = 0;
    }//end if

    if((SetupPkt.bFeature == ENDPOINT_HALT)&&
       (SetupPkt.Recipient == RCPT_EP)&&
       (SetupPkt.EPNum != 0))
    {
        inPipes[0].info.bits.busy = 1;
        /* Must do address calculation here */

        if(SetupPkt.EPDir == 0)
        {
            p = (BDT_ENTRY*)pBDTEntryOut[SetupPkt.EPNum];
        }
        else
        {
            p = (BDT_ENTRY*)pBDTEntryIn[SetupPkt.EPNum];
        }

		//if it was a SET_FEATURE request
        if(SetupPkt.bRequest == SET_FEATURE)
        {
			//Then STALL the endpoint
            p->STAT.Val = _USIE|_BSTALL;
        }
        else
        {
			//If it was not a SET_FEATURE
			//point to the appropriate UEP register
            pUEP = (unsigned int*) &U1EP0;
            pUEP += SetupPkt.EPNum * 4;

			//Clear the STALL bit in the UEP register
            *pUEP &= ~UEP_STALL;

            if(SetupPkt.EPDir == 1) // IN
            {
				//If the endpoint is an IN endpoint then we
				//  need to return it to the CPU and reset the
				//  DTS bit so that the next transfer is correct
                #if (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0) || (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
                    p->STAT.Val = _UCPU|_DAT0;
                    //toggle over the to the next buffer
                    *(unsigned char*)&p ^= USB_NEXT_PING_PONG;
                    p->STAT.Val = _UCPU|_DAT1;
                #else
                    p->STAT.Val = _UCPU|_DAT1;
                #endif
            }
            else
            {
				//If the endpoint was an OUT endpoint then we
				//  need to give control of the endpoint back to
				//  the SIE so that the function driver can
				//  receive the data as they expected.  Also need
				//  to set the DTS bit so the next packet will be
				//  correct
                #if (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0) || (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
                    p->STAT.Val = _USIE|_DAT0|_DTSEN;
                    //toggle over the to the next buffer
                    *(unsigned char*)&p ^= USB_NEXT_PING_PONG;
                    p->STAT.Val = _USIE|_DAT1|_DTSEN;
                #else
                    p->STAT.Val = _USIE|_DAT1|_DTSEN;
                #endif

            }
        }
    }
}

/*
 * This routine handles the standard GET_DESCRIPTOR request.
 */
void USBStdGetDscHandler(void)
{
    if(SetupPkt.bmRequestType == 0x80)
    {
        inPipes[0].info.Val = USB_INPIPES_ROM | USB_INPIPES_BUSY | USB_INPIPES_INCLUDE_ZERO;

        switch(SetupPkt.bDescriptorType)
        {
            case USB_DESCRIPTOR_DEVICE:
                #if !defined(USB_USER_DEVICE_DESCRIPTOR)
                    inPipes[0].pSrc.bRom = (const unsigned char*)&device_dsc;
                #else
                    inPipes[0].pSrc.bRom = (const unsigned char*)USB_USER_DEVICE_DESCRIPTOR;
                #endif
                inPipes[0].wCount = sizeof(device_dsc);
                break;
            case USB_DESCRIPTOR_CONFIGURATION:
                #if !defined(USB_USER_CONFIG_DESCRIPTOR)
                    inPipes[0].pSrc.bRom = *(USB_CD_Ptr + SetupPkt.bDscIndex);
                #else
                    inPipes[0].pSrc.bRom = *(USB_USER_CONFIG_DESCRIPTOR + SetupPkt.bDscIndex);
                #endif
                inPipes[0].wCount = *(inPipes[0].pSrc.wRom+1);                // Set data count
                break;
            case USB_DESCRIPTOR_STRING:
#if defined(USB_NUM_STRING_DESCRIPTORS)
		if (SetupPkt.bDscIndex < USB_NUM_STRING_DESCRIPTORS)
#else
                if (1)
#endif
                {
                    //Get a pointer to the String descriptor requested
                    inPipes[0].pSrc.bRom = *(USB_SD_Ptr+SetupPkt.bDscIndex);
                    // Set data count
                    inPipes[0].wCount = *inPipes[0].pSrc.bRom;
                }
                else
                {
                    inPipes[0].info.Val = 0;
                }
                break;
            default:
                inPipes[0].info.Val = 0;
                break;
        }//end switch
    }//end if
}//end USBStdGetDscHandler

/*
 * This routine handles the standard GET_STATUS request
 */
void USBStdGetStatusHandler(void)
{
    CtrlTrfData[0] = 0;                 // Initialize content
    CtrlTrfData[1] = 0;

    switch(SetupPkt.Recipient)
    {
        case RCPT_DEV:
            inPipes[0].info.bits.busy = 1;
            /*
             * [0]: bit0: Self-Powered Status [0] Bus-Powered [1] Self-Powered
             *      bit1: RemoteWakeup        [0] Disabled    [1] Enabled
             */
	    CtrlTrfData[0] |= 0x01;		// self powered

            if (RemoteWakeup == 1) {
                CtrlTrfData[0] |= 0x02;
            }
            break;
        case RCPT_INTF:
            inPipes[0].info.bits.busy = 1;     // No data to update
            break;
        case RCPT_EP:
            inPipes[0].info.bits.busy = 1;
            /*
             * [0]: bit0: Halt Status [0] Not Halted [1] Halted
             */
            {
                BDT_ENTRY *p;

                if(SetupPkt.EPDir == 0)
                {
                    p = (BDT_ENTRY*)pBDTEntryOut[SetupPkt.EPNum];
                }
                else
                {
                    p = (BDT_ENTRY*)pBDTEntryIn[SetupPkt.EPNum];
                }

                if(p->STAT.Val & _BSTALL)    // Use _BSTALL as a bit mask
                    CtrlTrfData[0]=0x01;    // Set bit0
                break;
            }
    }//end switch

    if(inPipes[0].info.bits.busy == 1)
    {
        inPipes[0].pSrc.bRam = (unsigned char*)&CtrlTrfData;	// Set Source
        inPipes[0].info.bits.ctrl_trf_mem = _RAM;	// Set memory type
        inPipes[0].wCount &= ~0xff;
        inPipes[0].wCount |= 2;				// Set data count
    }//end if(...)
}

/*
 * This routine wrap up the ramaining tasks in servicing
 * a Setup Request. Its main task is to set the endpoint
 * controls appropriately for a given situation. See code
 * below.
 * There are three main scenarios:
 * a) There was no handler for the Request, in this case
 *    a STALL should be sent out.
 * b) The host has requested a read control transfer,
 *    endpoints are required to be setup in a specific way.
 * c) The host has requested a write control transfer, or
 *    a control data stage is not required, endpoints are
 *    required to be setup in a specific way.
 *
 * Packet processing is resumed by clearing PKTDIS bit.
 */
void USBCtrlEPServiceComplete(void)
{
    /*
     * PKTDIS bit is set when a Setup Transaction is received.
     * Clear to resume packet processing.
     */
    U1CON &= ~PIC32_U1CON_PKTDIS;

    if(inPipes[0].info.bits.busy == 0)
    {
        if(outPipes[0].info.bits.busy == 1)
        {
            controlTransferState = CTRL_TRF_RX;
            /*
             * Control Write:
             * <SETUP[0]><OUT[1]><OUT[0]>...<IN[1]> | <SETUP[0]>
             *
             * 1. Prepare IN EP to respond to early termination
             *
             *    This is the same as a Zero Length Packet Response
             *    for control transfer without a data stage
             */
            pBDTEntryIn[0]->CNT = 0;
            pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;

            /*
             * 2. Prepare OUT EP to receive data.
             */
            pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
            pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&CtrlTrfData);
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
        }
        else
        {
            /*
             * If no one knows how to service this request then stall.
             * Must also prepare EP0 to receive the next SETUP transaction.
             */
            pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
            pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);

            /* v2b fix */
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
            pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
        }
    }
    else    // A module has claimed ownership of the control transfer session.
    {
        if(outPipes[0].info.bits.busy == 0)
        {
			if(SetupPkt.DataDir == DEV_TO_HOST)
			{
				if(SetupPkt.wLength < inPipes[0].wCount)
				{
					inPipes[0].wCount = SetupPkt.wLength;
				}
				USBCtrlTrfTxService();
				controlTransferState = CTRL_TRF_TX;
				/*
				 * Control Read:
				 * <SETUP[0]><IN[1]><IN[0]>...<OUT[1]> | <SETUP[0]>
				 * 1. Prepare OUT EP to respond to early termination
				 *
				 * NOTE:
				 * If something went wrong during the control transfer,
				 * the last status stage may not be sent by the host.
				 * When this happens, two different things could happen
				 * depending on the host.
				 * a) The host could send out a RESET.
				 * b) The host could send out a new SETUP transaction
				 *    without sending a RESET first.
				 * To properly handle case (b), the OUT EP must be setup
				 * to receive either a zero length OUT transaction, or a
				 * new SETUP transaction.
				 *
				 * Furthermore, the Cnt byte should be set to prepare for
				 * the SETUP data (8-byte or more), and the buffer address
				 * should be pointed to SetupPkt.
				 */
				pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
				pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);
				pBDTEntryEP0OutNext->STAT.Val = _USIE;           // Note: DTSEN is 0!

				pBDTEntryEP0OutCurrent->CNT = USB_EP0_BUFF_SIZE;
				pBDTEntryEP0OutCurrent->ADR = (unsigned char*)&SetupPkt;
				pBDTEntryEP0OutCurrent->STAT.Val = _USIE;           // Note: DTSEN is 0!

				/*
				 * 2. Prepare IN EP to transfer data, Cnt should have
				 *    been initialized by responsible request owner.
				 */
				pBDTEntryIn[0]->ADR = ConvertToPhysicalAddress(&CtrlTrfData);
				pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;
			}
			else   // (SetupPkt.DataDir == HOST_TO_DEVICE)
			{
				controlTransferState = CTRL_TRF_RX;
				/*
				 * Control Write:
				 * <SETUP[0]><OUT[1]><OUT[0]>...<IN[1]> | <SETUP[0]>
				 *
				 * 1. Prepare IN EP to respond to early termination
				 *
				 *    This is the same as a Zero Length Packet Response
				 *    for control transfer without a data stage
				 */
				pBDTEntryIn[0]->CNT = 0;
				pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;

				/*
				 * 2. Prepare OUT EP to receive data.
				 */
				pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
				pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&CtrlTrfData);
				pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
			}
        }
    }
}


/*
 * This routine should be called from only two places.
 * One from USBCtrlEPServiceComplete() and one from
 * USBCtrlTrfInHandler(). It takes care of managing a
 * transfer over multiple USB transactions.
 *
 * This routine works with isochronous endpoint larger than
 * 256 bytes and is shown here as an example of how to deal
 * with BC9 and BC8. In reality, a control endpoint can never
 * be larger than 64 bytes.
 *
 * PreCondition: pSrc, wCount, and usb_stat.ctrl_trf_mem are setup properly.
 */
void USBCtrlTrfTxService(void)
{
    unsigned byteToSend;

    /*
     * First, have to figure out how many byte of data to send.
     */
    if(inPipes[0].wCount < USB_EP0_BUFF_SIZE)
    {
        byteToSend = inPipes[0].wCount;

        /* v2b fix */
        if(shortPacketStatus == SHORT_PKT_NOT_USED)
        {
            shortPacketStatus = SHORT_PKT_PENDING;
        }
        else if(shortPacketStatus == SHORT_PKT_PENDING)
        {
            shortPacketStatus = SHORT_PKT_SENT;
        }//end if
        /* end v2b fix for this section */
    }
    else
    {
        byteToSend = USB_EP0_BUFF_SIZE;
    }

    /*
     * Next, load the number of bytes to send to BC9..0 in buffer descriptor
     */
    pBDTEntryIn[0]->CNT = byteToSend;

    /*
     * Subtract the number of bytes just about to be sent from the total.
     */
    inPipes[0].wCount = inPipes[0].wCount - byteToSend;

    pDst = (unsigned char*)CtrlTrfData;        // Set destination pointer

    if(inPipes[0].info.bits.ctrl_trf_mem == USB_INPIPES_ROM)       // Determine type of memory source
    {
        while(byteToSend)
        {
            *pDst++ = *inPipes[0].pSrc.bRom++;
            byteToSend--;
        }//end while(byte_to_send)
    }
    else  // RAM
    {
        while(byteToSend)
        {
            *pDst++ = *inPipes[0].pSrc.bRam++;
            byteToSend--;
        }//end while(byte_to_send)
    }
}

/*
 * *** This routine is only partially complete. Check for
 * new version of the firmware.
 *
 * PreCondition: pDst and wCount are setup properly.
 *               pSrc is always &CtrlTrfData
 *               usb_stat.ctrl_trf_mem is always _RAM.
 *               wCount should be set to 0 at the start of each control transfer.
 */
void USBCtrlTrfRxService(void)
{
    unsigned char byteToRead;
    unsigned char i;

    byteToRead = pBDTEntryEP0OutCurrent->CNT;

    /*
     * Accumulate total number of bytes read
     */
    if(byteToRead > outPipes[0].wCount)
    {
        byteToRead = outPipes[0].wCount;
    }
    else
    {
        outPipes[0].wCount = outPipes[0].wCount - byteToRead;
    }

    for(i=0;i<byteToRead;i++)
    {
        *outPipes[0].pDst.bRam++ = CtrlTrfData[i];
    }//end while(byteToRead)

    //If there is more data to read
    if(outPipes[0].wCount > 0)
    {
        /*
         * Don't have to worry about overwriting _KEEP bit
         * because if _KEEP was set, TRNIF would not have been
         * generated in the first place.
         */
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&CtrlTrfData);
        if(pBDTEntryEP0OutCurrent->STAT.DTS == 0)
        {
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
        }
        else
        {
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN;
        }
    }
    else
    {
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);
        if (outPipes[0].pFunc != 0) {
            outPipes[0].pFunc();
        }
        outPipes[0].info.bits.busy = 0;
    }


    // reset ep0Bo.Cnt to USB_EP0_BUFF_SIZE

}//end USBCtrlTrfRxService

/*
 * This routine first disables all endpoints by
 * clearing UEP registers. It then configures
 * (initializes) endpoints by calling the callback
 * function USBCBInitEP().
 */
void USBStdSetCfgHandler(void)
{
    // This will generate a zero length packet
    inPipes[0].info.bits.busy = 1;

    //disable all endpoints except endpoint 0
    bzero((void*) &U1EP1, USB_MAX_EP_NUMBER - 1);

    //clear the alternate interface settings
    bzero((void*) &USBAlternateInterface, USB_MAX_NUM_INT);

    //set the current configuration
    USBActiveConfiguration = SetupPkt.bConfigurationValue;

    //if the configuration value == 0
    if(SetupPkt.bConfigurationValue == 0)
    {
        //Go back to the addressed state
        USBDeviceState = ADDRESS_STATE;
    }
    else
    {
        //Otherwise go to the configured state
        USBDeviceState = CONFIGURED_STATE;
        //initialize the required endpoints
        USBInitEP((const unsigned char*)(USB_CD_Ptr[USBActiveConfiguration-1]));
        USBCBInitEP();
    }
}

/*
 * This function will configure the specified endpoint.
 *
 * Input: unsigned char EPNum - the endpoint to be configured
 *        unsigned char direction - the direction to be configured
 */
void USBConfigureEndpoint (unsigned char EPNum, unsigned char direction)
{
    volatile BDT_ENTRY* handle;

    handle = (volatile BDT_ENTRY*)&BDT[EP0_OUT_EVEN];
    handle += BD(EPNum,direction,0)/sizeof(BDT_ENTRY);

    handle->STAT.UOWN = 0;

    if(direction == 0)
    {
        pBDTEntryOut[EPNum] = handle;
    }
    else
    {
        pBDTEntryIn[EPNum] = handle;
    }

    #if (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
        handle->STAT.DTS = 0;
        (handle+1)->STAT.DTS = 1;
    #elif (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
        //Set DTS to one because the first thing we will do
        //when transmitting is toggle the bit
        handle->STAT.DTS = 1;
    #elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
        if(EPNum != 0)
        {
            handle->STAT.DTS = 1;
        }
    #elif (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)
        if(EPNum != 0)
        {
            handle->STAT.DTS = 0;
            (handle+1)->STAT.DTS = 1;
        }
    #endif
}

/*
 * This function will enable the specified endpoint with the specified
 * options.
 *
 * Typical Usage:
 * <code>
 * void USBCBInitEP(void)
 * {
 *     USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
 *     USBMSDInit();
 * }
 * </code>
 *
 * In the above example endpoint number MSD_DATA_IN_EP is being configured
 * for both IN and OUT traffic with handshaking enabled. Also since
 * MSD_DATA_IN_EP is not endpoint 0 (MSD does not allow this), then we can
 * explicitly disable SETUP packets on this endpoint.
 *
 * Input:
 *   unsigned char ep -       the endpoint to be configured
 *   unsigned char options -  optional settings for the endpoint. The options should
 *                   be ORed together to form a single options string. The
 *                   available optional settings for the endpoint. The
 *                   options should be ORed together to form a single options
 *                   string. The available options are the following\:
 *                   * USB_HANDSHAKE_ENABLED enables USB handshaking (ACK,
 *                     NAK)
 *                   * USB_HANDSHAKE_DISABLED disables USB handshaking (ACK,
 *                     NAK)
 *                   * USB_OUT_ENABLED enables the out direction
 *                   * USB_OUT_DISABLED disables the out direction
 *                   * USB_IN_ENABLED enables the in direction
 *                   * USB_IN_DISABLED disables the in direction
 *                   * USB_ALLOW_SETUP enables control transfers
 *                   * USB_DISALLOW_SETUP disables control transfers
 *                   * USB_STALL_ENDPOINT STALLs this endpoint
 */
void USBEnableEndpoint (unsigned char ep, unsigned char options)
{
    //Set the options to the appropriate endpoint control register
    //*((unsigned char*)(&U1EP0+ep)) = options;
    {
        unsigned int* p;

        p = (unsigned int*)(&U1EP0+(4*ep));
        *p = options;
    }

    if(options & USB_OUT_ENABLED)
    {
        USBConfigureEndpoint(ep,0);
    }
    if(options & USB_IN_ENABLED)
    {
        USBConfigureEndpoint(ep,1);
    }
}

/*
 * STALLs the specified endpoint
 *
 * Input:
 *   unsigned char ep - the endpoint the data will be transmitted on
 *   unsigned char dir - the direction of the transfer
 */
void USBStallEndpoint (unsigned char ep, unsigned char dir)
{
    BDT_ENTRY *p;

    if(ep == 0)
    {
        /*
         * If no one knows how to service this request then stall.
         * Must also prepare EP0 to receive the next SETUP transaction.
         */
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&SetupPkt);

        /* v2b fix */
        pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
        pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
    }
    else
    {
        p = (BDT_ENTRY*)(&BDT[EP(ep,dir,0)]);
        p->STAT.Val |= _BSTALL | _USIE;

        //If the device is in FULL or ALL_BUT_EP0 ping pong modes
        //then stall that entry as well
        #if (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG) || \
            (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)

        p = (BDT_ENTRY*)(&BDT[EP(ep,dir,1)]);
        p->STAT.Val |= _BSTALL | _USIE;
        #endif
    }
}

/*
 * Transfers one packet over the USB.
 *
 * Input:
 *   unsigned char ep - the endpoint the data will be transmitted on
 *   unsigned char dir - the direction of the transfer
 *                       This value is either OUT_FROM_HOST or IN_TO_HOST
 *   unsigned char* data - pointer to the data to be sent
 *   unsigned char len - length of the data needing to be sent
 */
USB_HANDLE USBTransferOnePacket (unsigned char ep, unsigned char dir, unsigned char* data, unsigned char len)
{
    USB_HANDLE handle;

    //If the direction is IN
    if(dir != 0)
    {
        //point to the IN BDT of the specified endpoint
        handle = pBDTEntryIn[ep];
    }
    else
    {
        //else point to the OUT BDT of the specified endpoint
        handle = pBDTEntryOut[ep];
    }

    //Toggle the DTS bit if required
    #if (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
        handle->STAT.Val ^= _DTSMASK;
    #elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
        if(ep != 0)
        {
            handle->STAT.Val ^= _DTSMASK;
        }
    #endif

    //Set the data pointer, data length, and enable the endpoint
    handle->ADR = ConvertToPhysicalAddress(data);
    handle->CNT = len;
    handle->STAT.Val &= _DTSMASK;
    handle->STAT.Val |= _USIE | _DTSEN;

    //Point to the next buffer for ping pong purposes.
    if(dir != 0)
    {
        //toggle over the to the next buffer for an IN endpoint
        *(unsigned char*)&pBDTEntryIn[ep] ^= USB_NEXT_PING_PONG;
    }
    else
    {
        //toggle over the to the next buffer for an OUT endpoint
        *(unsigned char*)&pBDTEntryOut[ep] ^= USB_NEXT_PING_PONG;
    }
    return handle;
}

/*
 * Clears the specified interrupt flag.
 *
 * Input:
 *   unsigned char* reg - the register address holding the interrupt flag
 *   unsigned char flag - the bit number needing to be cleared
 */
void USBClearInterruptFlag(unsigned char* reg, unsigned char flag)
{
    *reg = (0x01<<flag);
}
