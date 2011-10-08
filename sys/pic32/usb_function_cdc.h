/*
 * USB CDC Function Driver File
 *
 * This file contains all of functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the CDC function
 * driver. This file should be included in projects that use the CDC
 * \function driver.  This file should also be included into the
 * usb_descriptors.c file and any other user file that requires access to the
 * CDC interface.
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
#ifndef CDC_H
#define CDC_H

/*
 * Default CDC configuration.
 */
#ifndef CDC_COMM_INTF_ID
#   define CDC_COMM_INTF_ID	0x0
#endif
#ifndef CDC_COMM_EP
#   define CDC_COMM_EP		2
#endif
#ifndef CDC_COMM_IN_EP_SIZE
#   define CDC_COMM_IN_EP_SIZE	8
#endif
#ifndef CDC_DATA_INTF_ID
#   define CDC_DATA_INTF_ID	0x01
#endif
#ifndef CDC_DATA_EP
#   define CDC_DATA_EP		3
#endif
#ifndef CDC_DATA_OUT_EP_SIZE
#   define CDC_DATA_OUT_EP_SIZE	64
#endif
#ifndef CDC_DATA_IN_EP_SIZE
#   define CDC_DATA_IN_EP_SIZE	64
#endif
#if !defined (USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1) && \
    !defined (USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2)
	/* Set_Line_Coding, Set_Control_Line_State,
	 * Get_Line_Coding, and Serial_State commands*/
	#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1
	/* Send_Break command*/
	#undef USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2
#endif

/* Class-Specific Requests */
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

/* Notifications *
 * Note: Notifications are polled over
 * Communication Interface (Interrupt Endpoint)
 */
#define NETWORK_CONNECTION          0x00
#define RESPONSE_AVAILABLE          0x01
#define SERIAL_STATE                0x20


/* Device Class Code */
#define CDC_DEVICE                  0x02

/* Communication Interface Class Code */
#define COMM_INTF                   0x02

/* Communication Interface Class SubClass Codes */
#define ABSTRACT_CONTROL_MODEL      0x02

/* Communication Interface Class Control Protocol Codes */
#define V25TER                      0x01    // Common AT commands ("Hayes(TM)")


/* Data Interface Class Codes */
#define DATA_INTF                   0x0A

/* Data Interface Class Protocol Codes */
#define NO_PROTOCOL                 0x00    // No class specific protocol required


/* Communication Feature Selector Codes */
#define ABSTRACT_STATE              0x01
#define COUNTRY_SETTING             0x02

/* Functional Descriptors */
/* Type Values for the bDscType Field */
#define CS_INTERFACE                0x24
#define CS_ENDPOINT                 0x25

/* bDscSubType in Functional Descriptors */
#define DSC_FN_HEADER               0x00
#define DSC_FN_CALL_MGT             0x01
#define DSC_FN_ACM                  0x02    // ACM - Abstract Control Management
#define DSC_FN_DLM                  0x03    // DLM - Direct Line Managment
#define DSC_FN_TELEPHONE_RINGER     0x04
#define DSC_FN_RPT_CAPABILITIES     0x05
#define DSC_FN_UNION                0x06
#define DSC_FN_COUNTRY_SELECTION    0x07
#define DSC_FN_TEL_OP_MODES         0x08
#define DSC_FN_USB_TERMINAL         0x09
/* more.... see Table 25 in USB CDC Specification 1.1 */

/* CDC Bulk IN transfer states */
#define CDC_TX_READY                0
#define CDC_TX_BUSY                 1
#define CDC_TX_BUSY_ZLP             2       // ZLP: Zero Length Packet
#define CDC_TX_COMPLETING           3

#if defined(USB_CDC_SET_LINE_CODING_HANDLER)
    #define LINE_CODING_TARGET &cdc_notice.SetLineCoding._byte[0]
    #define LINE_CODING_PFUNC &USB_CDC_SET_LINE_CODING_HANDLER
#else
    #define LINE_CODING_TARGET &line_coding._byte[0]
    #define LINE_CODING_PFUNC 0
#endif

#if defined(USB_CDC_SUPPORT_HARDWARE_FLOW_CONTROL)
    #define CONFIGURE_RTS(a) UART_RTS = a;
    #define CONFIGURE_DTR(a) UART_DTR = a;
#else
    #define CONFIGURE_RTS(a)
    #define CONFIGURE_DTR(a)
#endif

#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D0_VAL 0x00
#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D3_VAL 0x00

#if defined(USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2)
    #define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2_VAL 0x04
#else
    #define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2_VAL 0x00
#endif

#if defined(USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1)
    #define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1_VAL 0x02
#else
    #define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1_VAL 0x00
#endif


#define USB_CDC_ACM_FN_DSC_VAL  \
    USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D3_VAL |\
    USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2_VAL |\
    USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1_VAL |\
    USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D0_VAL

/******************************************************************************
    Function:
        void CDCSetBaudRate(unsigned int baudRate)

    Summary:
        This macro is used set the baud rate reported back to the host during
        a get line coding request. (optional)

    Description:
        This macro is used set the baud rate reported back to the host during
        a get line coding request.

        Typical Usage:
        <code>
            CDCSetBaudRate(19200);
        </code>

        This function is optional for CDC devices that do not actually convert
        the USB traffic to a hardware UART.

    PreCondition:
        None

    Parameters:
        unsigned int baudRate - The desired baudrate

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define CDCSetBaudRate(baudRate) {line_coding.dwDTERate.Val=baudRate;}

/******************************************************************************
    Function:
        void CDCSetCharacterFormat (unsigned char charFormat)

    Summary:
        This macro is used manually set the character format reported back to
        the host during a get line coding request. (optional)

    Description:
        This macro is used manually set the character format reported back to
        the host during a get line coding request.

        Typical Usage:
        <code>
            CDCSetCharacterFormat(19200);
        </code>

        This function is optional for CDC devices that do not actually convert
        the USB traffic to a hardware UART.

    PreCondition:
        None

    Parameters:
        unsigned char charFormat - number of stop bits.  Available options are:
         * NUM_STOP_BITS_1 - 1 Stop bit
         * NUM_STOP_BITS_1_5 - 1.5 Stop bits
         * NUM_STOP_BITS_2 - 2 Stop bits

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define CDCSetCharacterFormat(charFormat) {line_coding.bCharFormat=charFormat;}
#define NUM_STOP_BITS_1     0   //1 stop bit - used by CDCSetLineCoding() and CDCSetCharacterFormat()
#define NUM_STOP_BITS_1_5   1   //1.5 stop bit - used by CDCSetLineCoding() and CDCSetCharacterFormat()
#define NUM_STOP_BITS_2     2   //2 stop bit - used by CDCSetLineCoding() and CDCSetCharacterFormat()

/******************************************************************************
    Function:
        void CDCSetParity (unsigned char parityType)

    Summary:
        This function is used manually set the parity format reported back to
        the host during a get line coding request. (optional)

    Description:
        This macro is used manually set the parity format reported back to
        the host during a get line coding request.

        Typical Usage:
        <code>
            CDCSetParity(PARITY_NONE);
        </code>

        This function is optional for CDC devices that do not actually convert
        the USB traffic to a hardware UART.

    PreCondition:
        None

    Parameters:
        unsigned char parityType - Type of parity.  The options are the following:
            * PARITY_NONE
            * PARITY_ODD
            * PARITY_EVEN
            * PARITY_MARK
            * PARITY_SPACE

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define CDCSetParity(parityType) {line_coding.bParityType=parityType;}
#define PARITY_NONE     0 //no parity - used by CDCSetLineCoding() and CDCSetParity()
#define PARITY_ODD      1 //odd parity - used by CDCSetLineCoding() and CDCSetParity()
#define PARITY_EVEN     2 //even parity - used by CDCSetLineCoding() and CDCSetParity()
#define PARITY_MARK     3 //mark parity - used by CDCSetLineCoding() and CDCSetParity()
#define PARITY_SPACE    4 //space parity - used by CDCSetLineCoding() and CDCSetParity()

/******************************************************************************
    Function:
        void CDCSetDataSize (unsigned char dataBits)

    Summary:
        This function is used manually set the number of data bits reported back
        to the host during a get line coding request. (optional)

    Description:
        This function is used manually set the number of data bits reported back
        to the host during a get line coding request.

        Typical Usage:
        <code>
            CDCSetDataSize(8);
        </code>

        This function is optional for CDC devices that do not actually convert
        the USB traffic to a hardware UART.

    PreCondition:
        None

    Parameters:
        unsigned char dataBits - number of data bits.  The options are 5, 6, 7, 8, or 16.

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define CDCSetDataSize(dataBits) {line_coding.bDataBits=dataBits;}

/******************************************************************************
    Function:
        void CDCSetLineCoding(unsigned int baud, unsigned char format, unsigned char parity, unsigned char dataSize)

    Summary:
        This function is used to manually set the data reported back
        to the host during a get line coding request. (optional)

    Description:
        This function is used to manually set the data reported back
        to the host during a get line coding request.

        Typical Usage:
        <code>
            CDCSetLineCoding(19200, NUM_STOP_BITS_1, PARITY_NONE, 8);
        </code>

        This function is optional for CDC devices that do not actually convert
        the USB traffic to a hardware UART.

    PreCondition:
        None

    Parameters:
        unsigned int baud - The desired baudrate
        unsigned char format - number of stop bits.  Available options are:
         * NUM_STOP_BITS_1 - 1 Stop bit
         * NUM_STOP_BITS_1_5 - 1.5 Stop bits
         * NUM_STOP_BITS_2 - 2 Stop bits
        unsigned char parity - Type of parity.  The options are the following:
            * PARITY_NONE
            * PARITY_ODD
            * PARITY_EVEN
            * PARITY_MARK
            * PARITY_SPACE
        unsigned char dataSize - number of data bits.  The options are 5, 6, 7, 8, or 16.

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define CDCSetLineCoding(baud,format,parity,dataSize) {\
            CDCSetBaudRate(baud);\
            CDCSetCharacterFormat(format);\
            CDCSetParity(parity);\
            CDCSetDataSize(dataSize);\
        }

/******************************************************************************
    Function:
        bool_t USBUSARTIsTxTrfReady(void)

    Summary:
        This macro is used to check if the CDC class is ready
        to send more data.

    Description:
        This macro is used to check if the CDC class is ready
        to send more data.

        Typical Usage:
        <code>
            if(USBUSARTIsTxTrfReady())
            {
                putrsUSBUSART("Hello World");
            }
        </code>

    PreCondition:
        None

    Parameters:
        None

    Return Values:
        None

    Remarks:
        None

 *****************************************************************************/
#define USBUSARTIsTxTrfReady()      (cdc_trf_state == CDC_TX_READY)

/******************************************************************************
    Function:
        void mUSBUSARTTxRam (unsigned char *pData, unsigned char len)

    Description:
        Depricated in MCHPFSUSB v2.3.  This macro has been replaced by
        USBUSARTIsTxTrfReady().
 *****************************************************************************/
#define mUSBUSARTIsTxTrfReady()     USBUSARTIsTxTrfReady()

/******************************************************************************
    Function:
        void mUSBUSARTTxRam (unsigned char *pData, unsigned char len)

    Description:
        Use this macro to transfer data located in data memory.
        Use this macro when:
            1. Data stream is not null-terminated
            2. Transfer length is known
        Remember: cdc_trf_state must == CDC_TX_READY
        Unlike putsUSBUSART, there is not code double checking
        the transfer state. Unexpected behavior will occur if
        this function is called when cdc_trf_state != CDC_TX_READY

    PreCondition:
        cdc_trf_state must be in the CDC_TX_READY state.
        Value of 'len' must be equal to or smaller than 255 bytes.

    Paramters:
        pDdata  : Pointer to the starting location of data bytes
        len     : Number of bytes to be transferred

    Return Values:
        None

    Remarks:
        This macro only handles the setup of the transfer. The
        actual transfer is handled by CDCTxService().

 *****************************************************************************/
#define mUSBUSARTTxRam(pData,len)   \
{                                   \
    pCDCSrc.bRam = pData;           \
    cdc_tx_len = len;               \
    cdc_mem_type = _RAM;            \
    cdc_trf_state = CDC_TX_BUSY;    \
}

/******************************************************************************
    Function:
        void mUSBUSARTTxRom (const unsigned char *pData, unsigned char len)

    Description:
        Use this macro to transfer data located in program memory.
        Use this macro when:
            1. Data stream is not null-terminated
            2. Transfer length is known

        Remember: cdc_trf_state must == CDC_TX_READY
        Unlike putrsUSBUSART, there is not code double checking
        the transfer state. Unexpected behavior will occur if
        this function is called when cdc_trf_state != CDC_TX_READY

    PreCondition:
        cdc_trf_state must be in the CDC_TX_READY state.
        Value of 'len' must be equal to or smaller than 255 bytes.

    Parameters:
        pDdata  : Pointer to the starting location of data bytes
        len     : Number of bytes to be transferred

    Return Values:
        None

    Remarks:
        This macro only handles the setup of the transfer. The
        actual transfer is handled by CDCTxService().

 *****************************************************************************/
#define mUSBUSARTTxRom(pData,len)   \
{                                   \
    pCDCSrc.bRom = pData;           \
    cdc_tx_len = len;               \
    cdc_mem_type = _ROM;            \
    cdc_trf_state = CDC_TX_BUSY;    \
}

/** S T R U C T U R E S ******************************************************/

/* Line Coding Structure */
#define LINE_CODING_LENGTH          0x07

typedef union _LINE_CODING
{
    struct
    {
        unsigned char _byte[LINE_CODING_LENGTH];
    };
    struct
    {
        unsigned long dwDTERate;          // Complex data structure
        unsigned char bCharFormat;
        unsigned char bParityType;
        unsigned char bDataBits;
    };
} LINE_CODING;

typedef union _CONTROL_SIGNAL_BITMAP
{
    unsigned char _byte;
    struct
    {
        unsigned DTE_PRESENT:1;       // [0] Not Present  [1] Present
        unsigned CARRIER_CONTROL:1;   // [0] Deactivate   [1] Activate
    };
} CONTROL_SIGNAL_BITMAP;


/* Functional Descriptor Structure - See CDC Specification 1.1 for details */

/* Header Functional Descriptor */
typedef struct __attribute__((packed)) _USB_CDC_HEADER_FN_DSC
{
    unsigned char bFNLength;
    unsigned char bDscType;
    unsigned char bDscSubType;
    unsigned short bcdCDC;
} USB_CDC_HEADER_FN_DSC;

/* Abstract Control Management Functional Descriptor */
typedef struct __attribute__((packed)) _USB_CDC_ACM_FN_DSC
{
    unsigned char bFNLength;
    unsigned char bDscType;
    unsigned char bDscSubType;
    unsigned char bmCapabilities;
} USB_CDC_ACM_FN_DSC;

/* Union Functional Descriptor */
typedef struct __attribute__((packed)) _USB_CDC_UNION_FN_DSC
{
    unsigned char bFNLength;
    unsigned char bDscType;
    unsigned char bDscSubType;
    unsigned char bMasterIntf;
    unsigned char bSaveIntf0;
} USB_CDC_UNION_FN_DSC;

/* Call Management Functional Descriptor */
typedef struct __attribute__((packed)) _USB_CDC_CALL_MGT_FN_DSC
{
    unsigned char bFNLength;
    unsigned char bDscType;
    unsigned char bDscSubType;
    unsigned char bmCapabilities;
    unsigned char bDataInterface;
} USB_CDC_CALL_MGT_FN_DSC;

typedef union __attribute__((packed)) _CDC_NOTICE
{
    LINE_CODING GetLineCoding;
    LINE_CODING SetLineCoding;
    unsigned char packet[CDC_COMM_IN_EP_SIZE];
} CDC_NOTICE, *PCDC_NOTICE;

/** E X T E R N S *********************************************************** */
extern unsigned char cdc_rx_len;
extern USB_HANDLE lastTransmission;

extern unsigned char cdc_trf_state;
extern POINTER pCDCSrc;
extern unsigned char cdc_tx_len;
extern unsigned char cdc_mem_type;

extern volatile CDC_NOTICE cdc_notice;
extern LINE_CODING line_coding;

/** Public Prototypes ************************************************ */
void USBCheckCDCRequest(void);
void CDCInitEP(void);
unsigned char getsUSBUSART(char *buffer, unsigned char len);
void putrsUSBUSART(const char *data);
void putUSBUSART(char *data, unsigned char Length);
void putsUSBUSART(char *data);
void CDCTxService(void);

#endif //CDC_H
