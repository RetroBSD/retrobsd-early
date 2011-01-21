/*
 * Hardware register defines for Microchip PIC32MX microcontrollers.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 */
#ifndef _IO_PIC32MX_H
#define _IO_PIC32MX_H

/*--------------------------------------
 * Interrupt controller registers.
 */
#define PIC32_INTCON	PIC32_R (0x81000)           /* Interrupt Control */
#define PIC32_INTCONCLR	PIC32_R (0x81004)
#define PIC32_INTCONSET	PIC32_R (0x81008)
#define PIC32_INTCONINV	PIC32_R (0x8100C)
#define PIC32_INTSTAT	PIC32_R (0x81010)           /* Interrupt Status */
#define PIC32_IPTMR		PIC32_R (0x81020)           /* Temporal Proximity Timer */
#define PIC32_IPTMRCLR	PIC32_R (0x81024)
#define PIC32_IPTMRSET	PIC32_R (0x81028)
#define PIC32_IPTMRINV	PIC32_R (0x8102C)
#define PIC32_IFS(n)	PIC32_R (0x81030+((n)<<4))  /* IFS(0..2) - Interrupt Flag Status */
#define PIC32_IFSCLR(n)	PIC32_R (0x81034+((n)<<4))
#define PIC32_IFSSET(n)	PIC32_R (0x81038+((n)<<4))
#define PIC32_IFSINV(n)	PIC32_R (0x8103C+((n)<<4))
#define PIC32_IEC(n)	PIC32_R (0x81060+((n)<<4))  /* IEC(0..2) - Interrupt Enable Control */
#define PIC32_IECCLR(n)	PIC32_R (0x81064+((n)<<4))
#define PIC32_IECSET(n)	PIC32_R (0x81068+((n)<<4))
#define PIC32_IECINV(n)	PIC32_R (0x8106C+((n)<<4))
#define PIC32_IPC(n)	PIC32_R (0x81090+((n)<<4))  /* IPC(0..11) - Interrupt Priority Control */
#define PIC32_IPCCLR(n)	PIC32_R (0x81094+((n)<<4))
#define PIC32_IPCSET(n)	PIC32_R (0x81098+((n)<<4))
#define PIC32_IPCINV(n)	PIC32_R (0x8109C+((n)<<4))

/*
 * Interrupt Control register.
 */
#define PIC32_INTCON_INT0EP	0x0001      /* External interrupt 0 polarity rising edge */
#define PIC32_INTCON_INT1EP	0x0002      /* External interrupt 1 polarity rising edge */
#define PIC32_INTCON_INT2EP	0x0004      /* External interrupt 2 polarity rising edge */
#define PIC32_INTCON_INT3EP	0x0008      /* External interrupt 3 polarity rising edge */
#define PIC32_INTCON_INT4EP	0x0010      /* External interrupt 4 polarity rising edge */
#define PIC32_INTCON_TPC(x)	((x)<<8)    /* Temporal proximity group priority */
#define PIC32_INTCON_MVEC	0x1000      /* Multi-vectored mode */
#define PIC32_INTCON_FRZ	0x4000      /* Freeze in debug mode */
#define PIC32_INTCON_SS0	0x8000      /* Single vector has a shadow register set */

/*
 * Interrupt Status register.
 */
#define PIC32_INTSTAT_VEC(s)	((s) & 0xFF)	/* Interrupt vector */
#define PIC32_INTSTAT_SRIPL(s)	((s) >> 8 & 7)	/* Requested priority level */

/*
 * Interrupt Prority Control register.
 */
#define PIC32_IPC_IS0(x)	(x)         /* Interrupt 0 subpriority */
#define PIC32_IPC_IP0(x)	((x)<<2)	/* Interrupt 0 priority */
#define PIC32_IPC_IS1(x)	((x)<<8)	/* Interrupt 1 subpriority */
#define PIC32_IPC_IP1(x)	((x)<<10)	/* Interrupt 1 priority */
#define PIC32_IPC_IS2(x)	((x)<<16)	/* Interrupt 2 subpriority */
#define PIC32_IPC_IP2(x)	((x)<<18)	/* Interrupt 2 priority */
#define PIC32_IPC_IS3(x)	((x)<<24)	/* Interrupt 3 subpriority */
#define PIC32_IPC_IP3(x)	((x)<<26)	/* Interrupt 3 priority */

/*
 * IRQ numbers for PIC32MX3xx/4xx.
 */
#define PIC32_IRQ_CT        0   /* Core Timer Interrupt */
#define PIC32_IRQ_CS0       1   /* Core Software Interrupt 0 */
#define PIC32_IRQ_CS1       2   /* Core Software Interrupt 1 */
#define PIC32_IRQ_INT0      3   /* External Interrupt 0 */
#define PIC32_IRQ_T1        4   /* Timer1 */
#define PIC32_IRQ_IC1       5   /* Input Capture 1 */
#define PIC32_IRQ_OC1       6   /* Output Compare 1 */
#define PIC32_IRQ_INT1      7   /* External Interrupt 1 */
#define PIC32_IRQ_T2        8   /* Timer2 */
#define PIC32_IRQ_IC2       9   /* Input Capture 2 */
#define PIC32_IRQ_OC2       10  /* Output Compare 2 */
#define PIC32_IRQ_INT2      11  /* External Interrupt 2 */
#define PIC32_IRQ_T3        12  /* Timer3 */
#define PIC32_IRQ_IC3       13  /* Input Capture 3 */
#define PIC32_IRQ_OC3       14  /* Output Compare 3 */
#define PIC32_IRQ_INT3      15  /* External Interrupt 3 */
#define PIC32_IRQ_T4        16  /* Timer4 */
#define PIC32_IRQ_IC4       17  /* Input Capture 4 */
#define PIC32_IRQ_OC4       18  /* Output Compare 4 */
#define PIC32_IRQ_INT4      19  /* External Interrupt 4 */
#define PIC32_IRQ_T5        20  /* Timer5 */
#define PIC32_IRQ_IC5       21  /* Input Capture 5 */
#define PIC32_IRQ_OC5       22  /* Output Compare 5 */
#define PIC32_IRQ_SPI1E     23  /* SPI1 Fault */
#define PIC32_IRQ_SPI1TX    24  /* SPI1 Transfer Done */
#define PIC32_IRQ_SPI1RX    25  /* SPI1 Receive Done */
#define PIC32_IRQ_U1E       26  /* UART1 Error */
#define PIC32_IRQ_U1RX      27  /* UART1 Receiver */
#define PIC32_IRQ_U1TX      28  /* UART1 Transmitter */
#define PIC32_IRQ_I2C1B     29  /* I2C1 Bus Collision Event */
#define PIC32_IRQ_I2C1S     30  /* I2C1 Slave Event */
#define PIC32_IRQ_I2C1M     31  /* I2C1 Master Event */
#define PIC32_IRQ_CN        32  /* Input Change Interrupt */
#define PIC32_IRQ_AD1       33  /* ADC1 Convert Done */
#define PIC32_IRQ_PMP       34  /* Parallel Master Port */
#define PIC32_IRQ_CMP1      35  /* Comparator Interrupt */
#define PIC32_IRQ_CMP2      36  /* Comparator Interrupt */
#define PIC32_IRQ_SPI2E     37  /* SPI2 Fault */
#define PIC32_IRQ_SPI2TX    38  /* SPI2 Transfer Done */
#define PIC32_IRQ_SPI2RX    39  /* SPI2 Receive Done */
#define PIC32_IRQ_U2E       40  /* UART2 Error */
#define PIC32_IRQ_U2RX      41  /* UART2 Receiver */
#define PIC32_IRQ_U2TX      42  /* UART2 Transmitter */
#define PIC32_IRQ_I2C2B     43  /* I2C2 Bus Collision Event */
#define PIC32_IRQ_I2C2S     44  /* I2C2 Slave Event */
#define PIC32_IRQ_I2C2M     45  /* I2C2 Master Event */
#define PIC32_IRQ_FSCM      46  /* Fail-Safe Clock Monitor */
#define PIC32_IRQ_RTCC      47  /* Real-Time Clock and Calendar */
#define PIC32_IRQ_DMA0      48  /* DMA Channel 0 */
#define PIC32_IRQ_DMA1      49  /* DMA Channel 1 */
#define PIC32_IRQ_DMA2      50  /* DMA Channel 2 */
#define PIC32_IRQ_DMA3      51  /* DMA Channel 3 */
#define PIC32_IRQ_FCE       56  /* Flash Control Event */
#define PIC32_IRQ_USB       57  /* USB */

/*
 * Interrupt vector numbers for PIC32MX3xx/4xx.
 */
#define PIC32_VECT_CT       0   /* Core Timer Interrupt */
#define PIC32_VECT_CS0      1   /* Core Software Interrupt 0 */
#define PIC32_VECT_CS1      2   /* Core Software Interrupt 1 */
#define PIC32_VECT_INT0     3   /* External Interrupt 0 */
#define PIC32_VECT_T1       4   /* Timer1 */
#define PIC32_VECT_IC1      5   /* Input Capture 1 */
#define PIC32_VECT_OC1      6   /* Output Compare 1 */
#define PIC32_VECT_INT1     7   /* External Interrupt 1 */
#define PIC32_VECT_T2       8   /* Timer2 */
#define PIC32_VECT_IC2      9   /* Input Capture 2 */
#define PIC32_VECT_OC2      10  /* Output Compare 2 */
#define PIC32_VECT_INT2     11  /* External Interrupt 2 */
#define PIC32_VECT_T3       12  /* Timer3 */
#define PIC32_VECT_IC3      13  /* Input Capture 3 */
#define PIC32_VECT_OC3      14  /* Output Compare 3 */
#define PIC32_VECT_INT3     15  /* External Interrupt 3 */
#define PIC32_VECT_T4       16  /* Timer4 */
#define PIC32_VECT_IC4      17  /* Input Capture 4 */
#define PIC32_VECT_OC4      18  /* Output Compare 4 */
#define PIC32_VECT_INT4     19  /* External Interrupt 4 */
#define PIC32_VECT_T5       20  /* Timer5 */
#define PIC32_VECT_IC5      21  /* Input Capture 5 */
#define PIC32_VECT_OC5      22  /* Output Compare 5 */
#define PIC32_VECT_SPI1     23  /* SPI1 */
#define PIC32_VECT_U1       24  /* UART1 */
#define PIC32_VECT_I2C1     25  /* I2C1 */
#define PIC32_VECT_CN       26  /* Input Change Interrupt */
#define PIC32_VECT_AD1      27  /* ADC1 Convert Done */
#define PIC32_VECT_PMP      28  /* Parallel Master Port */
#define PIC32_VECT_CMP1     29  /* Comparator Interrupt */
#define PIC32_VECT_CMP2     30  /* Comparator Interrupt */
#define PIC32_VECT_SPI2     31  /* SPI2 */
#define PIC32_VECT_U2       32  /* UART2 */
#define PIC32_VECT_I2C2     33  /* I2C2 */
#define PIC32_VECT_FSCM     34  /* Fail-Safe Clock Monitor */
#define PIC32_VECT_RTCC     35  /* Real-Time Clock and Calendar */
#define PIC32_VECT_DMA0     36  /* DMA Channel 0 */
#define PIC32_VECT_DMA1     37  /* DMA Channel 1 */
#define PIC32_VECT_DMA2     38  /* DMA Channel 2 */
#define PIC32_VECT_DMA3     39  /* DMA Channel 3 */
#define PIC32_VECT_FCE      44  /* Flash Control Event */
#define PIC32_VECT_USB      45  /* USB */

/*--------------------------------------
 * Peripheral registers.
 */
#define PIC32_R(a)              (0x1F800000 + (a))

/*--------------------------------------
 * UART registers.
 */
#define PIC32_U1MODE            PIC32_R (0x6000)        /* Mode */
#define PIC32_U1MODECLR         PIC32_R (0x6004)
#define PIC32_U1MODESET         PIC32_R (0x6008)
#define PIC32_U1MODEINV         PIC32_R (0x600C)
#define PIC32_U1STA             PIC32_R (0x6010)        /* Status and control */
#define PIC32_U1STACLR          PIC32_R (0x6014)
#define PIC32_U1STASET          PIC32_R (0x6018)
#define PIC32_U1STAINV          PIC32_R (0x601C)
#define PIC32_U1TXREG           PIC32_R (0x6020)        /* Transmit */
#define PIC32_U1RXREG           PIC32_R (0x6030)        /* Receive */
#define PIC32_U1BRG             PIC32_R (0x6040)        /* Baud rate */
#define PIC32_U1BRGCLR          PIC32_R (0x6044)
#define PIC32_U1BRGSET          PIC32_R (0x6048)
#define PIC32_U1BRGINV          PIC32_R (0x604C)

#define PIC32_U2MODE            PIC32_R (0x6200)        /* Mode */
#define PIC32_U2MODECLR         PIC32_R (0x6204)
#define PIC32_U2MODESET         PIC32_R (0x6208)
#define PIC32_U2MODEINV         PIC32_R (0x620C)
#define PIC32_U2STA             PIC32_R (0x6210)        /* Status and control */
#define PIC32_U2STACLR          PIC32_R (0x6214)
#define PIC32_U2STASET          PIC32_R (0x6218)
#define PIC32_U2STAINV          PIC32_R (0x621C)
#define PIC32_U2TXREG           PIC32_R (0x6220)        /* Transmit */
#define PIC32_U2RXREG           PIC32_R (0x6230)        /* Receive */
#define PIC32_U2BRG             PIC32_R (0x6240)        /* Baud rate */
#define PIC32_U2BRGCLR          PIC32_R (0x6244)
#define PIC32_U2BRGSET          PIC32_R (0x6248)
#define PIC32_U2BRGINV          PIC32_R (0x624C)

/*
 * UART Mode register.
 */
#define PIC32_UMODE_STSEL       0x0001  /* 2 Stop bits */
#define PIC32_UMODE_PDSEL       0x0006  /* Bitmask: */
#define PIC32_UMODE_PDSEL_8NPAR 0x0000  /* 8-bit data, no parity */
#define PIC32_UMODE_PDSEL_8EVEN 0x0002  /* 8-bit data, even parity */
#define PIC32_UMODE_PDSEL_8ODD  0x0004  /* 8-bit data, odd parity */
#define PIC32_UMODE_PDSEL_9NPAR 0x0006  /* 9-bit data, no parity */
#define PIC32_UMODE_BRGH        0x0008  /* High Baud Rate Enable */
#define PIC32_UMODE_RXINV       0x0010  /* Receive Polarity Inversion */
#define PIC32_UMODE_ABAUD       0x0020  /* Auto-Baud Enable */
#define PIC32_UMODE_LPBACK      0x0040  /* UARTx Loopback Mode */
#define PIC32_UMODE_WAKE        0x0080  /* Wake-up on start bit during Sleep Mode */
#define PIC32_UMODE_UEN         0x0300  /* Bitmask: */
#define PIC32_UMODE_UEN_RTS     0x0100  /* Using UxRTS pin */
#define PIC32_UMODE_UEN_RTSCTS  0x0200  /* Using UxCTS and UxRTS pins */
#define PIC32_UMODE_UEN_BCLK    0x0300  /* Using UxBCLK pin */
#define PIC32_UMODE_RTSMD       0x0800  /* UxRTS Pin Simplex mode */
#define PIC32_UMODE_IREN        0x1000  /* IrDA Encoder and Decoder Enable bit */
#define PIC32_UMODE_SIDL        0x2000  /* Stop in Idle Mode */
#define PIC32_UMODE_FRZ         0x4000  /* Freeze in Debug Exception Mode */
#define PIC32_UMODE_ON          0x8000  /* UART Enable */

/*
 * UART Control and status register.
 */
#define PIC32_USTA_URXDA        0x00000001      /* Receive Data Available (read-only) */
#define PIC32_USTA_OERR         0x00000002      /* Receive Buffer Overrun */
#define PIC32_USTA_FERR         0x00000004      /* Framing error detected (read-only) */
#define PIC32_USTA_PERR         0x00000008      /* Parity error detected (read-only) */
#define PIC32_USTA_RIDLE        0x00000010      /* Receiver is idle (read-only) */
#define PIC32_USTA_ADDEN        0x00000020      /* Address Detect mode */
#define PIC32_USTA_URXISEL      0x000000C0      /* Bitmask: receive interrupt is set when... */
#define PIC32_USTA_URXISEL_NEMP 0x00000000      /* ...receive buffer is not empty */
#define PIC32_USTA_URXISEL_HALF 0x00000040      /* ...receive buffer becomes 1/2 full */
#define PIC32_USTA_URXISEL_3_4  0x00000080      /* ...receive buffer becomes 3/4 full */
#define PIC32_USTA_TRMT         0x00000100      /* Transmit shift register is empty (read-only) */
#define PIC32_USTA_UTXBF        0x00000200      /* Transmit buffer is full (read-only) */
#define PIC32_USTA_UTXEN        0x00000400      /* Transmit Enable */
#define PIC32_USTA_UTXBRK       0x00000800      /* Transmit Break */
#define PIC32_USTA_URXEN        0x00001000      /* Receiver Enable */
#define PIC32_USTA_UTXINV       0x00002000      /* Transmit Polarity Inversion */
#define PIC32_USTA_UTXISEL      0x0000C000      /* Bitmask: TX interrupt is generated when... */
#define PIC32_USTA_UTXISEL_1    0x00000000      /* ...the transmit buffer contains at least one empty space */
#define PIC32_USTA_UTXISEL_ALL  0x00004000      /* ...all characters have been transmitted */
#define PIC32_USTA_UTXISEL_EMP  0x00008000      /* ...the transmit buffer becomes empty */
#define PIC32_USTA_ADDR         0x00FF0000      /* Automatic Address Mask */
#define PIC32_USTA_ADM_EN       0x01000000      /* Automatic Address Detect */

/*
 * Compute the 16-bit baud rate divisor, given
 * the oscillator frequency and baud rate.
 * Round to the nearest integer.
 */
#define PIC32_BRG_BAUD(fr,bd)   ((((fr)/8 + (bd)) / (bd) / 2) - 1)

/*--------------------------------------
 * Port A-G registers.
 */
#define PIC32_TRISA             PIC32_R (0x86000)       /* Port A: mask of inputs */
#define PIC32_TRISACLR          PIC32_R (0x86004)
#define PIC32_TRISASET          PIC32_R (0x86008)
#define PIC32_TRISAINV          PIC32_R (0x8600C)
#define PIC32_PORTA             PIC32_R (0x86010)       /* Port A: read inputs, write outputs */
#define PIC32_PORTACLR          PIC32_R (0x86014)
#define PIC32_PORTASET          PIC32_R (0x86018)
#define PIC32_PORTAINV          PIC32_R (0x8601C)
#define PIC32_LATA              PIC32_R (0x86020)       /* Port A: read/write outputs */
#define PIC32_LATACLR           PIC32_R (0x86024)
#define PIC32_LATASET           PIC32_R (0x86028)
#define PIC32_LATAINV           PIC32_R (0x8602C)
#define PIC32_ODCA              PIC32_R (0x86030)       /* Port A: open drain configuration */
#define PIC32_ODCACLR           PIC32_R (0x86034)
#define PIC32_ODCASET           PIC32_R (0x86038)
#define PIC32_ODCAINV           PIC32_R (0x8603C)

#define PIC32_TRISB             PIC32_R (0x86040)       /* Port B: mask of inputs */
#define PIC32_TRISBCLR          PIC32_R (0x86044)
#define PIC32_TRISBSET          PIC32_R (0x86048)
#define PIC32_TRISBINV          PIC32_R (0x8604C)
#define PIC32_PORTB             PIC32_R (0x86050)       /* Port B: read inputs, write outputs */
#define PIC32_PORTBCLR          PIC32_R (0x86054)
#define PIC32_PORTBSET          PIC32_R (0x86058)
#define PIC32_PORTBINV          PIC32_R (0x8605C)
#define PIC32_LATB              PIC32_R (0x86060)       /* Port B: read/write outputs */
#define PIC32_LATBCLR           PIC32_R (0x86064)
#define PIC32_LATBSET           PIC32_R (0x86068)
#define PIC32_LATBINV           PIC32_R (0x8606C)
#define PIC32_ODCB              PIC32_R (0x86070)       /* Port B: open drain configuration */
#define PIC32_ODCBCLR           PIC32_R (0x86074)
#define PIC32_ODCBSET           PIC32_R (0x86078)
#define PIC32_ODCBINV           PIC32_R (0x8607C)

#define PIC32_TRISC             PIC32_R (0x86080)       /* Port C: mask of inputs */
#define PIC32_TRISCCLR          PIC32_R (0x86084)
#define PIC32_TRISCSET          PIC32_R (0x86088)
#define PIC32_TRISCINV          PIC32_R (0x8608C)
#define PIC32_PORTC             PIC32_R (0x86090)       /* Port C: read inputs, write outputs */
#define PIC32_PORTCCLR          PIC32_R (0x86094)
#define PIC32_PORTCSET          PIC32_R (0x86098)
#define PIC32_PORTCINV          PIC32_R (0x8609C)
#define PIC32_LATC              PIC32_R (0x860A0)       /* Port C: read/write outputs */
#define PIC32_LATCCLR           PIC32_R (0x860A4)
#define PIC32_LATCSET           PIC32_R (0x860A8)
#define PIC32_LATCINV           PIC32_R (0x860AC)
#define PIC32_ODCC              PIC32_R (0x860B0)       /* Port C: open drain configuration */
#define PIC32_ODCCCLR           PIC32_R (0x860B4)
#define PIC32_ODCCSET           PIC32_R (0x860B8)
#define PIC32_ODCCINV           PIC32_R (0x860BC)

#define PIC32_TRISD             PIC32_R (0x860C0)       /* Port D: mask of inputs */
#define PIC32_TRISDCLR          PIC32_R (0x860C4)
#define PIC32_TRISDSET          PIC32_R (0x860C8)
#define PIC32_TRISDINV          PIC32_R (0x860CC)
#define PIC32_PORTD             PIC32_R (0x860D0)       /* Port D: read inputs, write outputs */
#define PIC32_PORTDCLR          PIC32_R (0x860D4)
#define PIC32_PORTDSET          PIC32_R (0x860D8)
#define PIC32_PORTDINV          PIC32_R (0x860DC)
#define PIC32_LATD              PIC32_R (0x860E0)       /* Port D: read/write outputs */
#define PIC32_LATDCLR           PIC32_R (0x860E4)
#define PIC32_LATDSET           PIC32_R (0x860E8)
#define PIC32_LATDINV           PIC32_R (0x860EC)
#define PIC32_ODCD              PIC32_R (0x860F0)       /* Port D: open drain configuration */
#define PIC32_ODCDCLR           PIC32_R (0x860F4)
#define PIC32_ODCDSET           PIC32_R (0x860F8)
#define PIC32_ODCDINV           PIC32_R (0x860FC)

#define PIC32_TRISE             PIC32_R (0x86100)       /* Port E: mask of inputs */
#define PIC32_TRISECLR          PIC32_R (0x86104)
#define PIC32_TRISESET          PIC32_R (0x86108)
#define PIC32_TRISEINV          PIC32_R (0x8610C)
#define PIC32_PORTE             PIC32_R (0x86110)       /* Port E: read inputs, write outputs */
#define PIC32_PORTECLR          PIC32_R (0x86114)
#define PIC32_PORTESET          PIC32_R (0x86118)
#define PIC32_PORTEINV          PIC32_R (0x8611C)
#define PIC32_LATE              PIC32_R (0x86120)       /* Port E: read/write outputs */
#define PIC32_LATECLR           PIC32_R (0x86124)
#define PIC32_LATESET           PIC32_R (0x86128)
#define PIC32_LATEINV           PIC32_R (0x8612C)
#define PIC32_ODCE              PIC32_R (0x86130)       /* Port E: open drain configuration */
#define PIC32_ODCECLR           PIC32_R (0x86134)
#define PIC32_ODCESET           PIC32_R (0x86138)
#define PIC32_ODCEINV           PIC32_R (0x8613C)

#define PIC32_TRISF             PIC32_R (0x86140)       /* Port F: mask of inputs */
#define PIC32_TRISFCLR          PIC32_R (0x86144)
#define PIC32_TRISFSET          PIC32_R (0x86148)
#define PIC32_TRISFINV          PIC32_R (0x8614C)
#define PIC32_PORTF             PIC32_R (0x86150)       /* Port F: read inputs, write outputs */
#define PIC32_PORTFCLR          PIC32_R (0x86154)
#define PIC32_PORTFSET          PIC32_R (0x86158)
#define PIC32_PORTFINV          PIC32_R (0x8615C)
#define PIC32_LATF              PIC32_R (0x86160)       /* Port F: read/write outputs */
#define PIC32_LATFCLR           PIC32_R (0x86164)
#define PIC32_LATFSET           PIC32_R (0x86168)
#define PIC32_LATFINV           PIC32_R (0x8616C)
#define PIC32_ODCF              PIC32_R (0x86170)       /* Port F: open drain configuration */
#define PIC32_ODCFCLR           PIC32_R (0x86174)
#define PIC32_ODCFSET           PIC32_R (0x86178)
#define PIC32_ODCFINV           PIC32_R (0x8617C)

#define PIC32_TRISG             PIC32_R (0x86180)       /* Port G: mask of inputs */
#define PIC32_TRISGCLR          PIC32_R (0x86184)
#define PIC32_TRISGSET          PIC32_R (0x86188)
#define PIC32_TRISGINV          PIC32_R (0x8618C)
#define PIC32_PORTG             PIC32_R (0x86190)       /* Port G: read inputs, write outputs */
#define PIC32_PORTGCLR          PIC32_R (0x86194)
#define PIC32_PORTGSET          PIC32_R (0x86198)
#define PIC32_PORTGINV          PIC32_R (0x8619C)
#define PIC32_LATG              PIC32_R (0x861A0)       /* Port G: read/write outputs */
#define PIC32_LATGCLR           PIC32_R (0x861A4)
#define PIC32_LATGSET           PIC32_R (0x861A8)
#define PIC32_LATGINV           PIC32_R (0x861AC)
#define PIC32_ODCG              PIC32_R (0x861B0)       /* Port G: open drain configuration */
#define PIC32_ODCGCLR           PIC32_R (0x861B4)
#define PIC32_ODCGSET           PIC32_R (0x861B8)
#define PIC32_ODCGINV           PIC32_R (0x861BC)

#define PIC32_CNCON             PIC32_R (0x861C0)       /* Interrupt-on-change control */
#define PIC32_CNCONCLR          PIC32_R (0x861C4)
#define PIC32_CNCONSET          PIC32_R (0x861C8)
#define PIC32_CNCONINV          PIC32_R (0x861CC)
#define PIC32_CNEN              PIC32_R (0x861D0)       /* Input change interrupt enable */
#define PIC32_CNENCLR           PIC32_R (0x861D4)
#define PIC32_CNENSET           PIC32_R (0x861D8)
#define PIC32_CNENINV           PIC32_R (0x861DC)
#define PIC32_CNPUE             PIC32_R (0x861E0)       /* Input pin pull-up enable */
#define PIC32_CNPUECLR          PIC32_R (0x861E4)
#define PIC32_CNPUESET          PIC32_R (0x861E8)
#define PIC32_CNPUEINV          PIC32_R (0x861EC)

/*--------------------------------------
 * A/D Converter registers.
 */
#define PIC32_AD1CON1           PIC32_R (0x9000)        /* Control register 1 */
#define PIC32_AD1CON1CLR        PIC32_R (0x9004)
#define PIC32_AD1CON1SET        PIC32_R (0x9008)
#define PIC32_AD1CON1INV        PIC32_R (0x900C)
#define PIC32_AD1CON2           PIC32_R (0x9010)        /* Control register 2 */
#define PIC32_AD1CON2CLR        PIC32_R (0x9014)
#define PIC32_AD1CON2SET        PIC32_R (0x9018)
#define PIC32_AD1CON2INV        PIC32_R (0x901C)
#define PIC32_AD1CON3           PIC32_R (0x9020)        /* Control register 3 */
#define PIC32_AD1CON3CLR        PIC32_R (0x9024)
#define PIC32_AD1CON3SET        PIC32_R (0x9028)
#define PIC32_AD1CON3INV        PIC32_R (0x902C)
#define PIC32_AD1CHS            PIC32_R (0x9040)        /* Channel select */
#define PIC32_AD1CHSCLR         PIC32_R (0x9044)
#define PIC32_AD1CHSSET         PIC32_R (0x9048)
#define PIC32_AD1CHSINV         PIC32_R (0x904C)
#define PIC32_AD1CSSL           PIC32_R (0x9050)        /* Input scan selection */
#define PIC32_AD1CSSLCLR        PIC32_R (0x9054)
#define PIC32_AD1CSSLSET        PIC32_R (0x9058)
#define PIC32_AD1CSSLINV        PIC32_R (0x905C)
#define PIC32_AD1PCFG           PIC32_R (0x9060)        /* Port configuration */
#define PIC32_AD1PCFGCLR        PIC32_R (0x9064)
#define PIC32_AD1PCFGSET        PIC32_R (0x9068)
#define PIC32_AD1PCFGINV        PIC32_R (0x906C)
#define PIC32_ADC1BUF0          PIC32_R (0x9070)        /* Result words */
#define PIC32_ADC1BUF1          PIC32_R (0x9080)
#define PIC32_ADC1BUF2          PIC32_R (0x9090)
#define PIC32_ADC1BUF3          PIC32_R (0x90A0)
#define PIC32_ADC1BUF4          PIC32_R (0x90B0)
#define PIC32_ADC1BUF5          PIC32_R (0x90C0)
#define PIC32_ADC1BUF6          PIC32_R (0x90D0)
#define PIC32_ADC1BUF7          PIC32_R (0x90E0)
#define PIC32_ADC1BUF8          PIC32_R (0x90F0)
#define PIC32_ADC1BUF9          PIC32_R (0x9100)
#define PIC32_ADC1BUFA          PIC32_R (0x9110)
#define PIC32_ADC1BUFB          PIC32_R (0x9120)
#define PIC32_ADC1BUFC          PIC32_R (0x9130)
#define PIC32_ADC1BUFD          PIC32_R (0x9140)
#define PIC32_ADC1BUFE          PIC32_R (0x9150)
#define PIC32_ADC1BUFF          PIC32_R (0x9160)

/*--------------------------------------
 * Parallel master port registers.
 */
#define PIC32_PMCON             PIC32_R (0x7000)        /* Control */
#define PIC32_PMCONCLR          PIC32_R (0x7004)
#define PIC32_PMCONSET          PIC32_R (0x7008)
#define PIC32_PMCONINV          PIC32_R (0x700C)
#define PIC32_PMMODE            PIC32_R (0x7010)        /* Mode */
#define PIC32_PMMODECLR         PIC32_R (0x7014)
#define PIC32_PMMODESET         PIC32_R (0x7018)
#define PIC32_PMMODEINV         PIC32_R (0x701C)
#define PIC32_PMADDR            PIC32_R (0x7020)        /* Address */
#define PIC32_PMADDRCLR         PIC32_R (0x7024)
#define PIC32_PMADDRSET         PIC32_R (0x7028)
#define PIC32_PMADDRINV         PIC32_R (0x702C)
#define PIC32_PMDOUT            PIC32_R (0x7030)        /* Data output */
#define PIC32_PMDOUTCLR         PIC32_R (0x7034)
#define PIC32_PMDOUTSET         PIC32_R (0x7038)
#define PIC32_PMDOUTINV         PIC32_R (0x703C)
#define PIC32_PMDIN             PIC32_R (0x7040)        /* Data input */
#define PIC32_PMDINCLR          PIC32_R (0x7044)
#define PIC32_PMDINSET          PIC32_R (0x7048)
#define PIC32_PMDININV          PIC32_R (0x704C)
#define PIC32_PMAEN             PIC32_R (0x7050)        /* Pin enable */
#define PIC32_PMAENCLR          PIC32_R (0x7054)
#define PIC32_PMAENSET          PIC32_R (0x7058)
#define PIC32_PMAENINV          PIC32_R (0x705C)
#define PIC32_PMSTAT            PIC32_R (0x7060)        /* Status (slave only) */
#define PIC32_PMSTATCLR         PIC32_R (0x7064)
#define PIC32_PMSTATSET         PIC32_R (0x7068)
#define PIC32_PMSTATINV         PIC32_R (0x706C)

/*
 * PMP Control register.
 */
#define PIC32_PMCON_RDSP        0x0001  /* Read strobe polarity active-high */
#define PIC32_PMCON_WRSP        0x0002  /* Write strobe polarity active-high */
#define PIC32_PMCON_CS1P        0x0008  /* Chip select 0 polarity active-high */
#define PIC32_PMCON_CS2P        0x0010  /* Chip select 1 polarity active-high */
#define PIC32_PMCON_ALP         0x0020  /* Address latch polarity active-high */
#define PIC32_PMCON_CSF         0x00C0  /* Chip select function bitmask: */
#define PIC32_PMCON_CSF_NONE    0x0000  /* PMCS2 and PMCS1 as A[15:14] */
#define PIC32_PMCON_CSF_CS2     0x0040  /* PMCS2 as chip select */
#define PIC32_PMCON_CSF_CS21    0x0080  /* PMCS2 and PMCS1 as chip select */
#define PIC32_PMCON_PTRDEN      0x0100  /* Read/write strobe port enable */
#define PIC32_PMCON_PTWREN      0x0200  /* Write enable strobe port enable */
#define PIC32_PMCON_PMPTTL      0x0400  /* TTL input buffer select */
#define PIC32_PMCON_ADRMUX      0x1800  /* Address/data mux selection bitmask: */
#define PIC32_PMCON_ADRMUX_NONE 0x0000  /* Address and data separate */
#define PIC32_PMCON_ADRMUX_AD   0x0800  /* Lower address on PMD[7:0], high on PMA[15:8] */
#define PIC32_PMCON_ADRMUX_D8   0x1000  /* All address on PMD[7:0] */
#define PIC32_PMCON_ADRMUX_D16  0x1800  /* All address on PMD[15:0] */
#define PIC32_PMCON_SIDL        0x2000  /* Stop in idle */
#define PIC32_PMCON_FRZ         0x4000  /* Freeze in debug exception */
#define PIC32_PMCON_ON          0x8000  /* Parallel master port enable */

/*
 * PMP Mode register.
 */
#define PIC32_PMMODE_WAITE(x)   ((x)<<0)        /* Wait states: data hold after RW strobe */
#define PIC32_PMMODE_WAITM(x)   ((x)<<2)        /* Wait states: data RW strobe */
#define PIC32_PMMODE_WAITB(x)   ((x)<<6)        /* Wait states: data setup to RW strobe */
#define PIC32_PMMODE_MODE       0x0300  /* Mode select bitmask: */
#define PIC32_PMMODE_MODE_SLAVE 0x0000  /* Legacy slave */
#define PIC32_PMMODE_MODE_SLENH 0x0100  /* Enhanced slave */
#define PIC32_PMMODE_MODE_MAST2 0x0200  /* Master mode 2 */
#define PIC32_PMMODE_MODE_MAST1 0x0300  /* Master mode 1 */
#define PIC32_PMMODE_MODE16     0x0400  /* 16-bit mode */
#define PIC32_PMMODE_INCM       0x1800  /* Address increment mode bitmask: */
#define PIC32_PMMODE_INCM_NONE  0x0000  /* No increment/decrement */
#define PIC32_PMMODE_INCM_INC   0x0800  /* Increment address */
#define PIC32_PMMODE_INCM_DEC   0x1000  /* Decrement address */
#define PIC32_PMMODE_INCM_SLAVE 0x1800  /* Slave auto-increment */
#define PIC32_PMMODE_IRQM       0x6000  /* Interrupt request bitmask: */
#define PIC32_PMMODE_IRQM_DIS   0x0000  /* No interrupt generated */
#define PIC32_PMMODE_IRQM_END   0x2000  /* Interrupt at end of read/write cycle */
#define PIC32_PMMODE_IRQM_A3    0x4000  /* Interrupt on address 3 */
#define PIC32_PMMODE_BUSY       0x8000  /* Port is busy */

/*
 * PMP Address register.
 */
#define PIC32_PMADDR_PADDR      0x3FFF  /* Destination address */
#define PIC32_PMADDR_CS1        0x4000  /* Chip select 1 is active */
#define PIC32_PMADDR_CS2        0x8000  /* Chip select 2 is active */

/*
 * PMP status register (slave only).
 */
#define PIC32_PMSTAT_OB0E       0x0001  /* Output buffer 0 empty */
#define PIC32_PMSTAT_OB1E       0x0002  /* Output buffer 1 empty */
#define PIC32_PMSTAT_OB2E       0x0004  /* Output buffer 2 empty */
#define PIC32_PMSTAT_OB3E       0x0008  /* Output buffer 3 empty */
#define PIC32_PMSTAT_OBUF       0x0040  /* Output buffer underflow */
#define PIC32_PMSTAT_OBE        0x0080  /* Output buffer empty */
#define PIC32_PMSTAT_IB0F       0x0100  /* Input buffer 0 full */
#define PIC32_PMSTAT_IB1F       0x0200  /* Input buffer 1 full */
#define PIC32_PMSTAT_IB2F       0x0400  /* Input buffer 2 full */
#define PIC32_PMSTAT_IB3F       0x0800  /* Input buffer 3 full */
#define PIC32_PMSTAT_IBOV       0x4000  /* Input buffer overflow */
#define PIC32_PMSTAT_IBF        0x8000  /* Input buffer full */

#endif /* _IO_PIC32MX_H */
