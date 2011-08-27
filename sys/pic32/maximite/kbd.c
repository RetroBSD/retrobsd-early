/*
 *
 * RetroBSD - PS2 keyboard driver for the Maximite PIC32 board
 *
 * Copyright (C) 2011 Rob Judd <judd@ob-wan.com>
 * All rights reserved.  The three clause ("New" or "Modified")
 * Berkeley software License Agreement specifies the terms and
 * conditions for redistribution.
 *
 */
 
/*
../kbd.c: In function 'initKBD':
../kbd.c:155:8: error: request for member 'ON' in something not a structure or union
../kbd.c:156:8: error: request for member 'CNPUE15' in something not a structure or union
../kbd.c:157:8: error: request for member 'CNPUE16' in something not a structure or union
../kbd.c: In function 'readKBD':
../kbd.c:173:9: error: request for member 'RD7' in something not a structure or union
../kbd.c:174:9: error: request for member 'RD6' in something not a structure or union
 *
 * This driver uses a 20uS timer, probably too fast
 */

#include "sys/types.h"
#include "machine/io.h"
#include "kbd.h"

//#define TRACE   printf
#ifndef TRACE
#define TRACE(...)
#endif

// I2C registers
struct kbdreg {
    //
};

#define USASCII         1
//#define RUSSIAN         1

#define true  1
#define false 0
#define PS2CLK          1//PORTD.RD6
#define PS2DAT          2//PORTD.RD7
#define QUEUE_SIZE      256

extern volatile char in_queue[QUEUE_SIZE];
extern volatile int in_queue_head, in_queue_tail;
volatile int abort;

#define POLL            20*(CPU_KHZ/1000)   // # clock cycles for 20uS between keyboard reads
#define TIMEOUT         500*(CPU_KHZ/1000)  // # clock cycles for 500uS timeout

enum {PS2START, PS2BIT, PS2PARITY, PS2STOP};

// Keyboard state machine and buffer
int PS2State;
unsigned char key_buff;
int key_state, key_count, key_parity, key_timer;

// IBM keyboard scancode set 2 - Special Keys
#define F1      0x0e
#define F2      0x0f
#define F3      0x10
#define F4      0x11
#define F5      0x12
#define F6      0x13
#define F7      0x14                        // maps to F5
#define F8      0x15
#define F9      0x16
#define F10     0x17
#define F11     0x18
#define F12     0x19

#define NUM     0x00
#define BKSP    0x08
#define TAB     0x09
#define L_ALT   0x11
#define L_SHF   0x12 
#define L_CTL   0x14
#define CAPS    0x58
#define R_SHF   0x59
#define ENTER   0x0d
#define ESC     0x1b
#define SCRL    0x7e

#ifdef USASCII
#include "usascii.inc"
#elif defined RUSSIAN
#include "russian.inc"
#endif

/*
    Standard PC init sequence:
    
    Keyboard: AA  Self-test passed                ;Keyboard controller init
    Host:     ED  Set/Reset Status Indicators 
    Keyboard: FA  Acknowledge
    Host:     00  Turn off all LEDs
    Keyboard: FA  Acknowledge
    Host:     F2  Read ID
    Keyboard: FA  Acknowledge
    Keyboard: AB  First byte of ID
    Host:     ED  Set/Reset Status Indicators     ;BIOS init
    Keyboard: FA  Acknowledge
    Host:     02  Turn on Num Lock LED
    Keyboard: FA  Acknowledge
    Host:     F3  Set Typematic Rate/Delay        ;Windows init
    Keyboard: FA  Acknowledge
    Host:     20  500 ms / 30.0 reports/sec
    Keyboard: FA  Acknowledge
    Host:     F4  Enable
    Keyboard: FA  Acknowledge
    Host:     F3  Set Typematic Rate/delay
    Keyboard: FA  Acknowledge
    Host:     00  250 ms / 30.0 reports/sec
    Keyboard: FA  Acknowledge
*/

char init_kbd(void)
{
	// enable pullups on the clock and data lines.  
	// This stops them from floating and generating random chars when no keyboard is attached
// 	CNCON.ON = 1;                           // turn on Change Notice for interrupt
// 	CNPUE.CNPUE15 = 1;                      // turn on the pullup for pin D6 also called CN15
// 	CNPUE.CNPUE16 = 1;                      // turn on the pullup for pin D7 also called CN16
 	
 	return false;
}

void read_kbd(void)
{
    int data = PS2DAT;
    int clk = PS2CLK;
	static char key_up = false;
	static unsigned char code = 0;
	static unsigned then = 0;
	unsigned now = mips_read_c0_register (C0_COUNT, 0);

    // Is it time to poll the keyboard yet?
    if ((int) (now - then) < POLL)
        return;
     
    // Keyboard state machine
    if (key_state) {                        // previous time clock was high key_state 1
        if (!clk) {                         // PS2CLK == 0, falling edge detected
            key_state = 0;                  // transition to State0
            key_timer = TIMEOUT;            // restart the counter
        
            switch(PS2State){
            default:
            case PS2START:   
                if(!data) {                 // PS2DAT == 0
                    key_count = 8;          // init bit counter
                    key_parity = 0;         // init parity check
                    code = 0;
                    PS2State = PS2BIT;
                }
                break;

            case PS2BIT:      
                code >>= 1;                                     // shift in data bit
                if(data) code |= 0x80;                          // PS2DAT == 1
                key_parity ^= code;                             // calculate parity
                if (--key_count == 0) PS2State = PS2PARITY;     // all bit read
                break;

            case PS2PARITY:         
                if(data) key_parity ^= 0x80;                    // PS2DAT == 1
                if (key_parity & 0x80)                          // parity odd, continue
                    PS2State = PS2STOP;
                else 
                    PS2State = PS2START;   
                break;

            case PS2STOP:
                if(data) {                                      // PS2DAT == 1
	                if(code  == 0xf0)
	                	 key_up = true;
	                else {

						char c;
						
						static char LShift = 0;
						static char RShift = 0;
						static char LCtrl = 0;
						static char CapsLock = 0;
					
						if(key_up) {
						    if(code == L_SHF) LShift = 0;   // left shift button is released
						    if(code == R_SHF) RShift = 0;   // right shift button is released
						    if(code == L_CTL) LCtrl = 0;    // control button is released
						    goto SkipOut;
						}
						
					    if(code == L_SHF) { LShift = 1; goto SkipOut; }    // left shift button is pressed
					    if(code == R_SHF) { RShift = 1; goto SkipOut; }    // right shift button is pressed
					    if(code == L_CTL) { LCtrl = 1; goto SkipOut; }     // control button is pressed
						if(code == CAPS)  { CapsLock = !CapsLock; goto SkipOut; }    // caps lock pressed
					
					    if(LShift || RShift)
					        c = lowerKey[code%128];                         // translate into an ASCII code
					    else
					        c = upperKey[code%128];
						
						if(!c) goto SkipOut;
						
						if(CapsLock && c >= 'a' && c <= 'z') c -= 32;       // adj for caps lock
						if(LCtrl) c &= 0x1F;                                // adj for control
						
						in_queue[in_queue_head] = c;                        // place into the queue
						in_queue_head = (in_queue_head + 1) % QUEUE_SIZE;   // increment the head index
						if(c == 3) {                                        // check for CTRL-C
							abort = true;                                   // bail out here
							in_queue_head = in_queue_tail = 0;              // flush the input buffer
						}

//						PrintSignonToUSB = false;     // show that the user is using the keyboard
						
SkipOut:

	                	key_up = false;
	                } // if key_up	
	            code = 0;
                } // if(data)   
                PS2State = PS2START;
                break;

            } // switch(PS2State)
        } // if(!clk)
        else {                                          // clock still high, remain in State1
            if (--key_timer == 0) PS2State = PS2START;  // timeout, reset data SM
        }
    } // if(key_state)
    else {                                              // Kstate 0
        if(clk) {                                       // PS2CLK == 1, rising edge, transition to State1
            key_state = 1;
        }
        else {                                          // clock still low, remain in State0
            if (--key_timer == 0) PS2State = PS2START;  // timeout, reset data SM
        }
    }

/*
//	GS I2C Start
		if (I2C_Timer) {
			if (--I2C_Timer == 0) {
				I2C_Status |= I2C_Status_Timeout;
				mI2C1MSetIntFlag();
			}
		}
		if (I2C_Status & I2C_Status_MasterCmd) {
			if (!(I2C1STAT & _I2C1STAT_S_MASK)) {
				I2C_Status &= ~I2C_Status_MasterCmd;
				I2C_State = I2C_State_Start;
				I2C1CONSET =_I2C1CON_SEN_MASK;
			}
		}
//	GS I2C End
*/
	
	// Grab the current CPU count on the way out
	then = mips_read_c0_register (C0_COUNT, 0);

    return;
}

char write_kbd(u_char data)
{

    // do something here

    return false;
}
