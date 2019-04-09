#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "init.h"
#include "F3xx_USB0_InterruptServiceRoutine.h"

#define COUNTER_MODE

extern unsigned char *P_IN_PACKET_SEND;

// required to disable the watchdog timer before the sdcc mcs51 init routines
// run to init global variables otherwise the watchdog will reset processor
// and the main function will never be reached.
//
// http://community.silabs.com/t5/8-bit-MCU-Knowledge-Base/Code-not-executing-to-main/ta-p/110667
void _sdcc_external_startup (void)
{
    // Disable Watchdog timer
    PCA0MD &= ~0x40;
}

//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main(void)
{
	uint8_t count = 0;

	system_init();	
  usb_init();

	PCA0MD = 0x00;                      // Disable watchdog timer
	EA = 1;                             // Globally enable interrupts
		
	while (1) {
		if (!F) {
			#ifdef COUNTER_MODE
			count++;
			*(P_IN_PACKET_SEND + 1) = count;
			#endif
			
			SendPacket(0);
		}
  }
}

