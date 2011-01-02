/*
 * Hardware register defines for all Microchip PIC32MX microcontrollers.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifndef _IO_PIC32MX_H
#define _IO_PIC32MX_H

/*--------------------------------------
 * Coprocessor 0 registers.
 */
#define C0_HWRENA	7	/* Enable RDHWR in non-privileged mode */
#define C0_BADVADDR	8	/* Virtual address of last exception */
#define C0_COUNT	9	/* Processor cycle count */
#define C0_COMPARE	11	/* Timer interrupt control */
#define C0_STATUS	12	/* Processor status and control */
#define C0_CAUSE	13	/* Cause of last exception */
#define C0_EPC		14	/* Program counter at last exception */
#define C0_PRID_EBASE	15	/* Processor identification; exception base address */
#define C0_CONFIG	16	/* Configuration */
#define C0_DEBUG	23	/* Debug control and status */
#define C0_DEPC		24	/* Program counter at last debug exception */
#define C0_ERROREPC	30	/* Program counter at last error */
#define C0_DESAVE	31	/* Debug handler scratchpad register */

/*
 * Status register.
 */
#define ST_CU0		0x10000000	/* Access to coprocessor 0 allowed (in user mode) */
#define ST_RP		0x08000000	/* Enable reduced power mode */
#define ST_RE		0x02000000	/* Reverse endianness (in user mode) */
#define ST_BEV		0x00400000	/* Exception vectors: bootstrap */
#define ST_SR		0x00100000	/* Soft reset */
#define ST_NMI		0x00080000	/* NMI reset */
#define ST_IPL(x)	((x) << 10)	/* Current interrupt priority level */
#define ST_UM		0x00000010	/* User mode */
#define ST_ERL		0x00000004	/* Error level */
#define ST_EXL		0x00000002	/* Exception level */
#define ST_IE		0x00000001	/* Interrupt enable */

/*
 * Сause register.
 */
#define CA_BD		0x80000000	/* Exception occured in delay slot */
#define CA_TI		0x40000000	/* Timer interrupt is pending */
#define CA_CE		0x30000000	/* Coprocessor exception */
#define CA_DC		0x08000000	/* Disable COUNT register */
#define CA_IV		0x00800000	/* Use special interrupt vector 0x200 */
#define CA_RIPL(r)	((r)>>10 & 63)	/* Requested interrupt priority level */
#define CA_IP1		0x00020000	/* Request software interrupt 1 */
#define CA_IP0		0x00010000	/* Request software interrupt 0 */
#define CA_EXC_CODE	0x0000007c	/* Exception code */

#define CA_Int		0		/* Interrupt */
#define CA_AdEL		(4 << 2)	/* Address error, load or instruction fetch */
#define CA_AdES		(5 << 2)	/* Address error, store */
#define CA_IBE		(6 << 2)	/* Bus error, instruction fetch */
#define CA_DBE		(7 << 2)	/* Bus error, load or store */
#define CA_Sys		(8 << 2)	/* Syscall */
#define CA_Bp		(9 << 2)	/* Breakpoint */
#define CA_RI		(10 << 2)	/* Reserved instruction */
#define CA_CPU		(11 << 2)	/* Coprocessor unusable */
#define CA_Ov		(12 << 2)	/* Arithmetic overflow */
#define CA_Tr		(13 << 2)	/* Trap */

/*--------------------------------------
 * Peripheral registers.
 */
#define PIC32_R(a)		*(volatile unsigned*)(0xBF800000 + (a))

/*--------------------------------------
 * UART registers.
 */
#define U1MODE		PIC32_R (0x6000) /* Mode */
#define U1MODECLR	PIC32_R (0x6004)
#define U1MODESET	PIC32_R (0x6008)
#define U1MODEINV	PIC32_R (0x600C)
#define U1STA		PIC32_R (0x6010) /* Status and control */
#define U1STACLR	PIC32_R (0x6014)
#define U1STASET	PIC32_R (0x6018)
#define U1STAINV	PIC32_R (0x601C)
#define U1TXREG		PIC32_R (0x6020) /* Transmit */
#define U1RXREG		PIC32_R (0x6030) /* Receive */
#define U1BRG		PIC32_R (0x6040) /* Baud rate */
#define U1BRGCLR	PIC32_R (0x6044)
#define U1BRGSET	PIC32_R (0x6048)
#define U1BRGINV	PIC32_R (0x604C)

#define U2MODE		PIC32_R (0x6200) /* Mode */
#define U2MODECLR	PIC32_R (0x6204)
#define U2MODESET	PIC32_R (0x6208)
#define U2MODEINV	PIC32_R (0x620C)
#define U2STA		PIC32_R (0x6210) /* Status and control */
#define U2STACLR	PIC32_R (0x6214)
#define U2STASET	PIC32_R (0x6218)
#define U2STAINV	PIC32_R (0x621C)
#define U2TXREG		PIC32_R (0x6220) /* Transmit */
#define U2RXREG		PIC32_R (0x6230) /* Receive */
#define U2BRG		PIC32_R (0x6240) /* Baud rate */
#define U2BRGCLR	PIC32_R (0x6244)
#define U2BRGSET	PIC32_R (0x6248)
#define U2BRGINV	PIC32_R (0x624C)

/*
 * UART Mode register.
 */
#define PIC32_UMODE_STSEL	0x0001	/* 2 Stop bits */
#define PIC32_UMODE_PDSEL	0x0006	/* Bitmask: */
#define PIC32_UMODE_PDSEL_8NPAR	0x0000	/* 8-bit data, no parity */
#define PIC32_UMODE_PDSEL_8EVEN	0x0002	/* 8-bit data, even parity */
#define PIC32_UMODE_PDSEL_8ODD	0x0004	/* 8-bit data, odd parity */
#define PIC32_UMODE_PDSEL_9NPAR	0x0006	/* 9-bit data, no parity */
#define PIC32_UMODE_BRGH	0x0008	/* High Baud Rate Enable */
#define PIC32_UMODE_RXINV	0x0010	/* Receive Polarity Inversion */
#define PIC32_UMODE_ABAUD	0x0020	/* Auto-Baud Enable */
#define PIC32_UMODE_LPBACK	0x0040	/* UARTx Loopback Mode */
#define PIC32_UMODE_WAKE	0x0080	/* Wake-up on start bit during Sleep Mode */
#define PIC32_UMODE_UEN		0x0300	/* Bitmask: */
#define PIC32_UMODE_UEN_RTS	0x0100	/* Using UxRTS pin */
#define PIC32_UMODE_UEN_RTSCTS	0x0200	/* Using UxCTS and UxRTS pins */
#define PIC32_UMODE_UEN_BCLK	0x0300	/* Using UxBCLK pin */
#define PIC32_UMODE_RTSMD	0x0800	/* UxRTS Pin Simplex mode */
#define PIC32_UMODE_IREN	0x1000	/* IrDA Encoder and Decoder Enable bit */
#define PIC32_UMODE_SIDL	0x2000	/* Stop in Idle Mode */
#define PIC32_UMODE_FRZ		0x4000	/* Freeze in Debug Exception Mode */
#define PIC32_UMODE_ON		0x8000	/* UART Enable */

/*
 * UART Control and status register.
 */
#define PIC32_USTA_URXDA	0x00000001 /* Receive Data Available (read-only) */
#define PIC32_USTA_OERR		0x00000002 /* Receive Buffer Overrun */
#define PIC32_USTA_FERR		0x00000004 /* Framing error detected (read-only) */
#define PIC32_USTA_PERR		0x00000008 /* Parity error detected (read-only) */
#define PIC32_USTA_RIDLE	0x00000010 /* Receiver is idle (read-only) */
#define PIC32_USTA_ADDEN	0x00000020 /* Address Detect mode */
#define PIC32_USTA_URXISEL	0x000000C0 /* Bitmask: receive interrupt is set when... */
#define PIC32_USTA_URXISEL_NEMP	0x00000000 /* ...receive buffer is not empty */
#define PIC32_USTA_URXISEL_HALF	0x00000040 /* ...receive buffer becomes 1/2 full */
#define PIC32_USTA_URXISEL_3_4	0x00000080 /* ...receive buffer becomes 3/4 full */
#define PIC32_USTA_TRMT		0x00000100 /* Transmit shift register is empty (read-only) */
#define PIC32_USTA_UTXBF	0x00000200 /* Transmit buffer is full (read-only) */
#define PIC32_USTA_UTXEN	0x00000400 /* Transmit Enable */
#define PIC32_USTA_UTXBRK	0x00000800 /* Transmit Break */
#define PIC32_USTA_URXEN	0x00001000 /* Receiver Enable */
#define PIC32_USTA_UTXINV	0x00002000 /* Transmit Polarity Inversion */
#define PIC32_USTA_UTXISEL	0x0000C000 /* Bitmask: TX interrupt is generated when... */
#define PIC32_USTA_UTXISEL_1	0x00000000 /* ...the transmit buffer contains at least one empty space */
#define PIC32_USTA_UTXISEL_ALL	0x00004000 /* ...all characters have been transmitted */
#define PIC32_USTA_UTXISEL_EMP	0x00008000 /* ...the transmit buffer becomes empty */
#define PIC32_USTA_ADDR		0x00FF0000 /* Automatic Address Mask */
#define PIC32_USTA_ADM_EN	0x01000000 /* Automatic Address Detect */

/*
 * Compute the 16-bit baud rate divisor, given
 * the oscillator frequency and baud rate.
 * Round to the nearest integer.
 */
#define PIC32_BRG_BAUD(fr,bd)	((((fr)/8 + (bd)) / (bd) / 2) - 1)

/*--------------------------------------
 * Port A-G registers.
 */
#define TRISA		PIC32_R (0x86000) /* Port A: mask of inputs */
#define TRISACLR	PIC32_R (0x86004)
#define TRISASET	PIC32_R (0x86008)
#define TRISAINV	PIC32_R (0x8600C)
#define PORTA		PIC32_R (0x86010) /* Port A: read inputs, write outputs */
#define PORTACLR	PIC32_R (0x86014)
#define PORTASET	PIC32_R (0x86018)
#define PORTAINV	PIC32_R (0x8601C)
#define LATA		PIC32_R (0x86020) /* Port A: read/write outputs */
#define LATACLR		PIC32_R (0x86024)
#define LATASET		PIC32_R (0x86028)
#define LATAINV		PIC32_R (0x8602C)
#define ODCA		PIC32_R (0x86030) /* Port A: open drain configuration */
#define ODCACLR		PIC32_R (0x86034)
#define ODCASET		PIC32_R (0x86038)
#define ODCAINV		PIC32_R (0x8603C)

#define TRISB		PIC32_R (0x86040) /* Port B: mask of inputs */
#define TRISBCLR	PIC32_R (0x86044)
#define TRISBSET	PIC32_R (0x86048)
#define TRISBINV	PIC32_R (0x8604C)
#define PORTB		PIC32_R (0x86050) /* Port B: read inputs, write outputs */
#define PORTBCLR	PIC32_R (0x86054)
#define PORTBSET	PIC32_R (0x86058)
#define PORTBINV	PIC32_R (0x8605C)
#define LATB		PIC32_R (0x86060) /* Port B: read/write outputs */
#define LATBCLR		PIC32_R (0x86064)
#define LATBSET		PIC32_R (0x86068)
#define LATBINV		PIC32_R (0x8606C)
#define ODCB		PIC32_R (0x86070) /* Port B: open drain configuration */
#define ODCBCLR		PIC32_R (0x86074)
#define ODCBSET		PIC32_R (0x86078)
#define ODCBINV		PIC32_R (0x8607C)

#define TRISC		PIC32_R (0x86080) /* Port C: mask of inputs */
#define TRISCCLR	PIC32_R (0x86084)
#define TRISCSET	PIC32_R (0x86088)
#define TRISCINV	PIC32_R (0x8608C)
#define PORTC		PIC32_R (0x86090) /* Port C: read inputs, write outputs */
#define PORTCCLR	PIC32_R (0x86094)
#define PORTCSET	PIC32_R (0x86098)
#define PORTCINV	PIC32_R (0x8609C)
#define LATC		PIC32_R (0x860A0) /* Port C: read/write outputs */
#define LATCCLR		PIC32_R (0x860A4)
#define LATCSET		PIC32_R (0x860A8)
#define LATCINV		PIC32_R (0x860AC)
#define ODCC		PIC32_R (0x860B0) /* Port C: open drain configuration */
#define ODCCCLR		PIC32_R (0x860B4)
#define ODCCSET		PIC32_R (0x860B8)
#define ODCCINV		PIC32_R (0x860BC)

#define TRISD		PIC32_R (0x860C0) /* Port D: mask of inputs */
#define TRISDCLR	PIC32_R (0x860C4)
#define TRISDSET	PIC32_R (0x860C8)
#define TRISDINV	PIC32_R (0x860CC)
#define PORTD		PIC32_R (0x860D0) /* Port D: read inputs, write outputs */
#define PORTDCLR	PIC32_R (0x860D4)
#define PORTDSET	PIC32_R (0x860D8)
#define PORTDINV	PIC32_R (0x860DC)
#define LATD		PIC32_R (0x860E0) /* Port D: read/write outputs */
#define LATDCLR		PIC32_R (0x860E4)
#define LATDSET		PIC32_R (0x860E8)
#define LATDINV		PIC32_R (0x860EC)
#define ODCD		PIC32_R (0x860F0) /* Port D: open drain configuration */
#define ODCDCLR		PIC32_R (0x860F4)
#define ODCDSET		PIC32_R (0x860F8)
#define ODCDINV		PIC32_R (0x860FC)

#define TRISE		PIC32_R (0x86100) /* Port E: mask of inputs */
#define TRISECLR	PIC32_R (0x86104)
#define TRISESET	PIC32_R (0x86108)
#define TRISEINV	PIC32_R (0x8610C)
#define PORTE		PIC32_R (0x86110) /* Port E: read inputs, write outputs */
#define PORTECLR	PIC32_R (0x86114)
#define PORTESET	PIC32_R (0x86118)
#define PORTEINV	PIC32_R (0x8611C)
#define LATE		PIC32_R (0x86120) /* Port E: read/write outputs */
#define LATECLR		PIC32_R (0x86124)
#define LATESET		PIC32_R (0x86128)
#define LATEINV		PIC32_R (0x8612C)
#define ODCE		PIC32_R (0x86130) /* Port E: open drain configuration */
#define ODCECLR		PIC32_R (0x86134)
#define ODCESET		PIC32_R (0x86138)
#define ODCEINV		PIC32_R (0x8613C)

#define TRISF		PIC32_R (0x86140) /* Port F: mask of inputs */
#define TRISFCLR	PIC32_R (0x86144)
#define TRISFSET	PIC32_R (0x86148)
#define TRISFINV	PIC32_R (0x8614C)
#define PORTF		PIC32_R (0x86150) /* Port F: read inputs, write outputs */
#define PORTFCLR	PIC32_R (0x86154)
#define PORTFSET	PIC32_R (0x86158)
#define PORTFINV	PIC32_R (0x8615C)
#define LATF		PIC32_R (0x86160) /* Port F: read/write outputs */
#define LATFCLR		PIC32_R (0x86164)
#define LATFSET		PIC32_R (0x86168)
#define LATFINV		PIC32_R (0x8616C)
#define ODCF		PIC32_R (0x86170) /* Port F: open drain configuration */
#define ODCFCLR		PIC32_R (0x86174)
#define ODCFSET		PIC32_R (0x86178)
#define ODCFINV		PIC32_R (0x8617C)

#define TRISG		PIC32_R (0x86180) /* Port G: mask of inputs */
#define TRISGCLR	PIC32_R (0x86184)
#define TRISGSET	PIC32_R (0x86188)
#define TRISGINV	PIC32_R (0x8618C)
#define PORTG		PIC32_R (0x86190) /* Port G: read inputs, write outputs */
#define PORTGCLR	PIC32_R (0x86194)
#define PORTGSET	PIC32_R (0x86198)
#define PORTGINV	PIC32_R (0x8619C)
#define LATG		PIC32_R (0x861A0) /* Port G: read/write outputs */
#define LATGCLR		PIC32_R (0x861A4)
#define LATGSET		PIC32_R (0x861A8)
#define LATGINV		PIC32_R (0x861AC)
#define ODCG		PIC32_R (0x861B0) /* Port G: open drain configuration */
#define ODCGCLR		PIC32_R (0x861B4)
#define ODCGSET		PIC32_R (0x861B8)
#define ODCGINV		PIC32_R (0x861BC)

#define CNCON		PIC32_R (0x861C0) /* Interrupt-on-change control */
#define CNCONCLR	PIC32_R (0x861C4)
#define CNCONSET	PIC32_R (0x861C8)
#define CNCONINV	PIC32_R (0x861CC)
#define CNEN		PIC32_R (0x861D0) /* Input change interrupt enable */
#define CNENCLR		PIC32_R (0x861D4)
#define CNENSET		PIC32_R (0x861D8)
#define CNENINV		PIC32_R (0x861DC)
#define CNPUE		PIC32_R (0x861E0) /* Input pin pull-up enable */
#define CNPUECLR	PIC32_R (0x861E4)
#define CNPUESET	PIC32_R (0x861E8)
#define CNPUEINV	PIC32_R (0x861EC)

/*--------------------------------------
 * A/D Converter registers.
 */
#define AD1CON1		PIC32_R (0x9000) /* Control register 1 */
#define AD1CON1CLR	PIC32_R (0x9004)
#define AD1CON1SET	PIC32_R (0x9008)
#define AD1CON1INV	PIC32_R (0x900C)
#define AD1CON2		PIC32_R (0x9010) /* Control register 2 */
#define AD1CON2CLR	PIC32_R (0x9014)
#define AD1CON2SET	PIC32_R (0x9018)
#define AD1CON2INV	PIC32_R (0x901C)
#define AD1CON3		PIC32_R (0x9020) /* Control register 3 */
#define AD1CON3CLR	PIC32_R (0x9024)
#define AD1CON3SET	PIC32_R (0x9028)
#define AD1CON3INV	PIC32_R (0x902C)
#define AD1CHS		PIC32_R (0x9040) /* Channel select */
#define AD1CHSCLR	PIC32_R (0x9044)
#define AD1CHSSET	PIC32_R (0x9048)
#define AD1CHSINV	PIC32_R (0x904C)
#define AD1CSSL		PIC32_R (0x9050) /* Input scan selection */
#define AD1CSSLCLR	PIC32_R (0x9054)
#define AD1CSSLSET	PIC32_R (0x9058)
#define AD1CSSLINV	PIC32_R (0x905C)
#define AD1PCFG		PIC32_R (0x9060) /* Port configuration */
#define AD1PCFGCLR	PIC32_R (0x9064)
#define AD1PCFGSET	PIC32_R (0x9068)
#define AD1PCFGINV	PIC32_R (0x906C)
#define ADC1BUF0	PIC32_R (0x9070) /* Result words */
#define ADC1BUF1	PIC32_R (0x9080)
#define ADC1BUF2	PIC32_R (0x9090)
#define ADC1BUF3	PIC32_R (0x90A0)
#define ADC1BUF4	PIC32_R (0x90B0)
#define ADC1BUF5	PIC32_R (0x90C0)
#define ADC1BUF6	PIC32_R (0x90D0)
#define ADC1BUF7	PIC32_R (0x90E0)
#define ADC1BUF8	PIC32_R (0x90F0)
#define ADC1BUF9	PIC32_R (0x9100)
#define ADC1BUFA	PIC32_R (0x9110)
#define ADC1BUFB	PIC32_R (0x9120)
#define ADC1BUFC	PIC32_R (0x9130)
#define ADC1BUFD	PIC32_R (0x9140)
#define ADC1BUFE	PIC32_R (0x9150)
#define ADC1BUFF	PIC32_R (0x9160)

/*--------------------------------------
 * Parallel master port registers.
 */
#define PMCON		PIC32_R (0x7000) /* Control */
#define PMCONCLR	PIC32_R (0x7004)
#define PMCONSET	PIC32_R (0x7008)
#define PMCONINV	PIC32_R (0x700C)
#define PMMODE		PIC32_R (0x7010) /* Mode */
#define PMMODECLR	PIC32_R (0x7014)
#define PMMODESET	PIC32_R (0x7018)
#define PMMODEINV	PIC32_R (0x701C)
#define PMADDR		PIC32_R (0x7020) /* Address */
#define PMADDRCLR	PIC32_R (0x7024)
#define PMADDRSET	PIC32_R (0x7028)
#define PMADDRINV	PIC32_R (0x702C)
#define PMDOUT		PIC32_R (0x7030) /* Data output */
#define PMDOUTCLR	PIC32_R (0x7034)
#define PMDOUTSET	PIC32_R (0x7038)
#define PMDOUTINV	PIC32_R (0x703C)
#define PMDIN		PIC32_R (0x7040) /* Data input */
#define PMDINCLR	PIC32_R (0x7044)
#define PMDINSET	PIC32_R (0x7048)
#define PMDININV	PIC32_R (0x704C)
#define PMAEN		PIC32_R (0x7050) /* Pin enable */
#define PMAENCLR	PIC32_R (0x7054)
#define PMAENSET	PIC32_R (0x7058)
#define PMAENINV	PIC32_R (0x705C)
#define PMSTAT		PIC32_R (0x7060) /* Status (slave only) */
#define PMSTATCLR	PIC32_R (0x7064)
#define PMSTATSET	PIC32_R (0x7068)
#define PMSTATINV	PIC32_R (0x706C)

/*
 * PMP Control register.
 */
#define PIC32_PMCON_RDSP	0x0001 /* Read strobe polarity active-high */
#define PIC32_PMCON_WRSP	0x0002 /* Write strobe polarity active-high */
#define PIC32_PMCON_CS1P	0x0008 /* Chip select 0 polarity active-high */
#define PIC32_PMCON_CS2P	0x0010 /* Chip select 1 polarity active-high */
#define PIC32_PMCON_ALP		0x0020 /* Address latch polarity active-high */
#define PIC32_PMCON_CSF		0x00C0 /* Chip select function bitmask: */
#define PIC32_PMCON_CSF_NONE	0x0000 /* PMCS2 and PMCS1 as A[15:14] */
#define PIC32_PMCON_CSF_CS2	0x0040 /* PMCS2 as chip select */
#define PIC32_PMCON_CSF_CS21	0x0080 /* PMCS2 and PMCS1 as chip select */
#define PIC32_PMCON_PTRDEN	0x0100 /* Read/write strobe port enable */
#define PIC32_PMCON_PTWREN	0x0200 /* Write enable strobe port enable */
#define PIC32_PMCON_PMPTTL	0x0400 /* TTL input buffer select */
#define PIC32_PMCON_ADRMUX	0x1800 /* Address/data mux selection bitmask: */
#define PIC32_PMCON_ADRMUX_NONE	0x0000 /* Address and data separate */
#define PIC32_PMCON_ADRMUX_AD	0x0800 /* Lower address on PMD[7:0], high on PMA[15:8] */
#define PIC32_PMCON_ADRMUX_D8	0x1000 /* All address on PMD[7:0] */
#define PIC32_PMCON_ADRMUX_D16	0x1800 /* All address on PMD[15:0] */
#define PIC32_PMCON_SIDL	0x2000 /* Stop in idle */
#define PIC32_PMCON_FRZ		0x4000 /* Freeze in debug exception */
#define PIC32_PMCON_ON		0x8000 /* Parallel master port enable */

/*
 * PMP Mode register.
 */
#define PIC32_PMMODE_WAITE(x)	((x)<<0) /* Wait states: data hold after RW strobe */
#define PIC32_PMMODE_WAITM(x)	((x)<<2) /* Wait states: data RW strobe */
#define PIC32_PMMODE_WAITB(x)	((x)<<6) /* Wait states: data setup to RW strobe */
#define PIC32_PMMODE_MODE	0x0300	/* Mode select bitmask: */
#define PIC32_PMMODE_MODE_SLAVE	0x0000	/* Legacy slave */
#define PIC32_PMMODE_MODE_SLENH	0x0100	/* Enhanced slave */
#define PIC32_PMMODE_MODE_MAST2	0x0200	/* Master mode 2 */
#define PIC32_PMMODE_MODE_MAST1	0x0300	/* Master mode 1 */
#define PIC32_PMMODE_MODE16	0x0400	/* 16-bit mode */
#define PIC32_PMMODE_INCM	0x1800	/* Address increment mode bitmask: */
#define PIC32_PMMODE_INCM_NONE	0x0000	/* No increment/decrement */
#define PIC32_PMMODE_INCM_INC	0x0800	/* Increment address */
#define PIC32_PMMODE_INCM_DEC	0x1000	/* Decrement address */
#define PIC32_PMMODE_INCM_SLAVE	0x1800	/* Slave auto-increment */
#define PIC32_PMMODE_IRQM	0x6000	/* Interrupt request bitmask: */
#define PIC32_PMMODE_IRQM_DIS	0x0000	/* No interrupt generated */
#define PIC32_PMMODE_IRQM_END	0x2000	/* Interrupt at end of read/write cycle */
#define PIC32_PMMODE_IRQM_A3	0x4000	/* Interrupt on address 3 */
#define PIC32_PMMODE_BUSY	0x8000	/* Port is busy */

/*
 * PMP Address register.
 */
#define PIC32_PMADDR_PADDR	0x3FFF /* Destination address */
#define PIC32_PMADDR_CS1	0x4000 /* Chip select 1 is active */
#define PIC32_PMADDR_CS2	0x8000 /* Chip select 2 is active */

/*
 * PMP status register (slave only).
 */
#define PIC32_PMSTAT_OB0E	0x0001 /* Output buffer 0 empty */
#define PIC32_PMSTAT_OB1E	0x0002 /* Output buffer 1 empty */
#define PIC32_PMSTAT_OB2E	0x0004 /* Output buffer 2 empty */
#define PIC32_PMSTAT_OB3E	0x0008 /* Output buffer 3 empty */
#define PIC32_PMSTAT_OBUF	0x0040 /* Output buffer underflow */
#define PIC32_PMSTAT_OBE	0x0080 /* Output buffer empty */
#define PIC32_PMSTAT_IB0F	0x0100 /* Input buffer 0 full */
#define PIC32_PMSTAT_IB1F	0x0200 /* Input buffer 1 full */
#define PIC32_PMSTAT_IB2F	0x0400 /* Input buffer 2 full */
#define PIC32_PMSTAT_IB3F	0x0800 /* Input buffer 3 full */
#define PIC32_PMSTAT_IBOV	0x4000 /* Input buffer overflow */
#define PIC32_PMSTAT_IBF	0x8000 /* Input buffer full */

/*--------------------------------------
 * USB registers.
 */
#define U1OTGIR		PIC32_R (0x85040) /* OTG interrupt flags */
#define U1OTGIE		PIC32_R (0x85050) /* OTG interrupt enable */
#define U1OTGSTAT	PIC32_R (0x85060) /* Comparator and pin status */
#define U1OTGCON	PIC32_R (0x85070) /* Resistor and pin control */
#define U1PWRC		PIC32_R (0x85080) /* Power control */
#define U1IR		PIC32_R (0x85200) /* Pending interrupt */
#define U1IE		PIC32_R (0x85210) /* Interrupt enable */
#define U1EIR		PIC32_R (0x85220) /* Pending error interrupt */
#define U1EIE		PIC32_R (0x85230) /* Error interrupt enable */
#define U1STAT		PIC32_R (0x85240) /* Status FIFO */
#define U1CON		PIC32_R (0x85250) /* Control */
#define U1ADDR		PIC32_R (0x85260) /* Address */
#define U1BDTP1		PIC32_R	(0x85270) /* Buffer descriptor table pointer 1 */
#define U1FRML		PIC32_R (0x85280) /* Frame counter low */
#define U1FRMH		PIC32_R (0x85290) /* Frame counter high */
#define U1TOK		PIC32_R (0x852A0) /* Host control */
#define U1SOF		PIC32_R (0x852B0) /* SOF counter */
#define U1BDTP2		PIC32_R (0x852C0) /* Buffer descriptor table pointer 2 */
#define U1BDTP3		PIC32_R (0x852D0) /* Buffer descriptor table pointer 3 */
#define U1CNFG1		PIC32_R (0x852E0) /* Debug and idle */
#define U1EP0		PIC32_R (0x85300) /* Endpoint control */
#define U1EP1		PIC32_R (0x85310)
#define U1EP2		PIC32_R (0x85320)
#define U1EP3		PIC32_R (0x85330)
#define U1EP4		PIC32_R (0x85340)
#define U1EP5		PIC32_R (0x85350)
#define U1EP6		PIC32_R (0x85360)
#define U1EP7		PIC32_R (0x85370)
#define U1EP8		PIC32_R (0x85380)
#define U1EP9		PIC32_R (0x85390)
#define U1EP10		PIC32_R (0x853A0)
#define U1EP11		PIC32_R (0x853B0)
#define U1EP12		PIC32_R (0x853C0)
#define U1EP13		PIC32_R (0x853D0)
#define U1EP14		PIC32_R (0x853E0)
#define U1EP15		PIC32_R (0x853F0)

/*
 * USB Control register.
 */
#define PIC32_U1CON_USBEN	0x0001 /* USB module enable (device mode) */
#define PIC32_U1CON_SOFEN	0x0001 /* SOF sent every 1 ms (host mode) */
#define PIC32_U1CON_PPBRST	0x0002 /* Ping-pong buffers reset */
#define PIC32_U1CON_RESUME	0x0004 /* Resume signaling enable */
#define PIC32_U1CON_HOSTEN	0x0008 /* Host mode enable */
#define PIC32_U1CON_USBRST	0x0010 /* USB reset */
#define PIC32_U1CON_PKTDIS	0x0020 /* Packet transfer disable */
#define PIC32_U1CON_TOKBUSY	0x0020 /* Token busy indicator */
#define PIC32_U1CON_SE0		0x0040 /* Single ended zero detected */
#define PIC32_U1CON_JSTATE	0x0080 /* Live differential receiver JSTATE flag */

/*
 * USB Power control register.
 */
#define PIC32_U1PWRC_USBPWR	0x0001 /* USB operation enable */
#define PIC32_U1PWRC_USUSPEND	0x0002 /* USB suspend mode */
#define PIC32_U1PWRC_USLPGRD	0x0010 /* USB sleep entry guard */
#define PIC32_U1PWRC_UACTPND	0x0080 /* UAB activity pending */

/*
 * USB Pending interrupt register.
 * USB Interrupt enable register.
 */
#define PIC32_U1I_DETACH	0x0001 /* Detach (host mode) */
#define PIC32_U1I_URST		0x0001 /* USB reset (device mode) */
#define PIC32_U1I_UERR		0x0002 /* USB error condition */
#define PIC32_U1I_SOF		0x0004 /* SOF token  */
#define PIC32_U1I_TRN		0x0008 /* Token processing complete */
#define PIC32_U1I_IDLE		0x0010 /* Idle detect */
#define PIC32_U1I_RESUME	0x0020 /* Resume */
#define PIC32_U1I_ATTACH	0x0040 /* Peripheral attach */
#define PIC32_U1I_STALL		0x0080 /* STALL handshake */

/*
 * USB OTG interrupt flags register.
 * USB OTG interrupt enable register.
 */
#define PIC32_U1OTGI_VBUSVD	0x0001 /* A-device Vbus change */
#define PIC32_U1OTGI_SESEND	0x0004 /* B-device Vbus change */
#define PIC32_U1OTGI_SESVD	0x0008 /* Session valid change */
#define PIC32_U1OTGI_ACTV	0x0010 /* Bus activity indicator */
#define PIC32_U1OTGI_LSTATE	0x0020 /* Line state stable */
#define PIC32_U1OTGI_T1MSEC	0x0040 /* 1 millisecond timer expired */
#define PIC32_U1OTGI_ID		0x0080 /* ID state change */

#define PIC32_U1OTGSTAT_VBUSVD	0x0001 /*  */
#define PIC32_U1OTGSTAT_SESEND	0x0004 /*  */
#define PIC32_U1OTGSTAT_SESVD	0x0008 /*  */
#define PIC32_U1OTGSTAT_LSTATE	0x0020 /*  */
#define PIC32_U1OTGSTAT_ID	0x0080 /*  */

#define PIC32_U1OTGCON_VBUSDIS	0x0001 /*  */
#define PIC32_U1OTGCON_VBUSCHG	0x0002 /*  */
#define PIC32_U1OTGCON_OTGEN	0x0004 /*  */
#define PIC32_U1OTGCON_VBUSON	0x0008 /*  */
#define PIC32_U1OTGCON_DMPULDWN	0x0010 /*  */
#define PIC32_U1OTGCON_DPPULDWN	0x0020 /*  */
#define PIC32_U1OTGCON_DMPULUP	0x0040 /*  */
#define PIC32_U1OTGCON_DPPULUP	0x0080 /*  */

#define PIC32_U1EI_PID		0x0001 /*  */
#define PIC32_U1EI_CRC5		0x0002 /*  */
#define PIC32_U1EI_EOF		0x0002 /*  */
#define PIC32_U1EI_CRC16	0x0004 /*  */
#define PIC32_U1EI_DFN8		0x0008 /*  */
#define PIC32_U1EI_BTO		0x0010 /*  */
#define PIC32_U1EI_DMA		0x0020 /*  */
#define PIC32_U1EI_BMX		0x0040 /*  */
#define PIC32_U1EI_BTS		0x0080 /*  */

#define PIC32_U1STAT_PPBI	0x0004 /*  */
#define PIC32_U1STAT_DIR	0x0008 /*  */
#define PIC32_U1STAT_ENDPT0	0x0010 /*  */
#define PIC32_U1STAT_ENDPT	0x00F0 /*  */
#define PIC32_U1STAT_ENDPT1	0x0020 /*  */
#define PIC32_U1STAT_ENDPT2	0x0040 /*  */
#define PIC32_U1STAT_ENDPT3	0x0080 /*  */

#define PIC32_U1ADDR_DEVADDR	0x007F /*  */
#define PIC32_U1ADDR_USBADDR0	0x0001 /*  */
#define PIC32_U1ADDR_DEVADDR1	0x0002 /*  */
#define PIC32_U1ADDR_DEVADDR2	0x0004 /*  */
#define PIC32_U1ADDR_DEVADDR3	0x0008 /*  */
#define PIC32_U1ADDR_DEVADDR4	0x0010 /*  */
#define PIC32_U1ADDR_DEVADDR5	0x0020 /*  */
#define PIC32_U1ADDR_DEVADDR6	0x0040 /*  */
#define PIC32_U1ADDR_LSPDEN	0x0080 /*  */

#define PIC32_U1FRML_FRM0	0x0001 /*  */
#define PIC32_U1FRML_FRM1	0x0002 /*  */
#define PIC32_U1FRML_FRM2	0x0004 /*  */
#define PIC32_U1FRML_FRM3	0x0008 /*  */
#define PIC32_U1FRML_FRM4	0x0010 /*  */
#define PIC32_U1FRML_FRM5	0x0020 /*  */
#define PIC32_U1FRML_FRM6	0x0040 /*  */
#define PIC32_U1FRML_FRM7	0x0080 /*  */

#define PIC32_U1FRMH_FRM8	0x0001 /*  */
#define PIC32_U1FRMH_FRM9	0x0002 /*  */
#define PIC32_U1FRMH_FRM10	0x0004 /*  */

#define PIC32_U1TOK_EP0		0x0001 /*  */
#define PIC32_U1TOK_EP		0x000F /*  */
#define PIC32_U1TOK_EP1		0x0002 /*  */
#define PIC32_U1TOK_EP2		0x0004 /*  */
#define PIC32_U1TOK_EP3		0x0008 /*  */
#define PIC32_U1TOK_PID0	0x0010 /*  */
#define PIC32_U1TOK_PID		0x00F0 /*  */
#define PIC32_U1TOK_PID1	0x0020 /*  */
#define PIC32_U1TOK_PID2	0x0040 /*  */
#define PIC32_U1TOK_PID3	0x0080 /*  */

#define PIC32_U1CNFG1_USBSIDL	0x0010 /*  */
#define PIC32_U1CNFG1_USBFRZ	0x0020 /*  */
#define PIC32_U1CNFG1_UOEMON	0x0040 /*  */
#define PIC32_U1CNFG1_UTEYE	0x0080 /*  */

#define PIC32_U1EP_EPHSHK	0x0001 /*  */
#define PIC32_U1EP_EPSTALL	0x0002 /*  */
#define PIC32_U1EP_EPTXEN	0x0004 /*  */
#define PIC32_U1EP_EPRXEN	0x0008 /*  */
#define PIC32_U1EP_EPCONDIS	0x0010 /*  */
#define PIC32_U1EP_RETRYDIS	0x0040 /*  */
#define PIC32_U1EP_LSPD		0x0080 /*  */

#endif /* _IO_PIC32MX_H */
