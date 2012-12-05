#ifndef RD_SDRAMP_CONFIG_H_

#include "pic32mx.h"

/* TODO: better support for different sized sdram chips, 16 bit support */

#ifdef SDRAM_FPGA_DIR_SUPPORT
#define SDR_DATA_DIR_PORT TRISA
#define	SDR_DATA_DIR_BIT 15
#endif


/*
 * Bank Selection
 * BA0 is connected to SDR_BANK_0_BIT
 * BA1 is connected to SDR_BANK_0_BIT + 1
 *
 * So, if SDR_BANK_0_BIT is 4, then bit 5
 * must be connected to BA1.
 */

#define SDR_BANK_PORT 	TRISD
#define SDR_BANK_0_BIT 	4

/*
 * Clock Enable
 *
 * Connect to CKE on sdram
 */
#define SDR_CKE_PORT 	TRISA
#define SDR_CKE_BIT 	10

/*
 * Control Lines
 *
 * Connect to /WE, /CAS, /CS and /RAS pins on sdram
 */
#define SDR_CONTROL_PORT 	TRISF
#define SDR_CONTROL_WE_BIT 	0
#define SDR_CONTROL_CAS_BIT 	1
#define SDR_CONTROL_CS_BIT 	12
#define SDR_CONTROL_RAS_BIT 	13

/*
 * Address Lines
 *
 * At present, the port can be changed, but
 * changing the address line bits is unsupported.
 */

#define SDR_ADDRESS_PORT 	TRISB

/***** WARNING - DO NOT CHANGE WITHOUT ALSO CHANGING CODE TO MATCH *****/
#define SDR_ADDRESS_A0_BIT 	11
#define SDR_ADDRESS_A1_BIT 	12
#define SDR_ADDRESS_A2_BIT 	13
#define SDR_ADDRESS_A3_BIT 	14
#define SDR_ADDRESS_A4_BIT 	5
#define SDR_ADDRESS_A5_BIT 	4
#define SDR_ADDRESS_A6_BIT 	3
#define SDR_ADDRESS_A7_BIT 	2
#define SDR_ADDRESS_A8_BIT 	6
#define SDR_ADDRESS_A9_BIT 	7
#define SDR_ADDRESS_A10_BIT 	15
#define SDR_ADDRESS_A11_BIT 	9
/***** END WARNING *****/

/*
 * Data Lines
 *
 * The low 8 bits (bits 0-7) must be used
 * and connected to the data lines on the sdram.
 * The specific order in which the 8 pins are
 * connected to the data pins of the sdram is
 * not significant, unless you wish for a neat
 * and tidy design that is easy connect to a
 * logic analyzer for debugging purposes.
 */

#define SDR_DATA_PORT 	TRISA

/*
 * Output Compare
 *
 * Currently supporting OC1CON or OC4CON
 * Timer2 is used in all cases.
 * The appropriate pin should be connected to CLK on the sdram.
 * OC1CON - RD0
 * OC4CON - RD3
 */

#define SDR_OCR		OC1CON

/* 
 * Additional sdram connections
 *
 * Power and ground as appropriate.
 * LDQM or DQM0 should be tied low.
 * UDQM or DQM1 can be tied low for future expansion to 16 bits, 
 * or high in order to isolate DQ8-DQ15.
 * DQ8-DQ15 should be pulled up or down, but in the test bed they
 * were simply left floating.
 * If the ram as an A12 address line it should be tied low.
 */


/***************************************************************/

/*
 * Anthing following should not normally need to be modified.
 * There are here in order to share definitions between C and ASM.
 */

#define ADDRESS_MASK 									\
	((1<<SDR_ADDRESS_A0_BIT)|(1<<SDR_ADDRESS_A1_BIT)|(1<<SDR_ADDRESS_A2_BIT)|	\
	 (1<<SDR_ADDRESS_A3_BIT)|(1<<SDR_ADDRESS_A4_BIT)|(1<<SDR_ADDRESS_A5_BIT)|	\
	 (1<<SDR_ADDRESS_A6_BIT)|(1<<SDR_ADDRESS_A7_BIT)|(1<<SDR_ADDRESS_A8_BIT)|	\
	 (1<<SDR_ADDRESS_A9_BIT)|(1<<SDR_ADDRESS_A10_BIT)|(1<<SDR_ADDRESS_A11_BIT))

#define CONTROL_ALL_MASK 					\
	((1<<SDR_CONTROL_CS_BIT)|(1<<SDR_CONTROL_RAS_BIT)| 	\
         (1<<SDR_CONTROL_CAS_BIT)|(1<<SDR_CONTROL_WE_BIT))

#define BANK_BITMASK 3
#define BANK_ALL_MASK (BANK_BITMASK << SDR_BANK_0_BIT)


#endif
