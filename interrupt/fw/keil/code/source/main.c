#include "main.h"

/// define to enable byte counter value to be sent as first byte for each USB HID IN
/// report
#define COUNTER_MODE

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
		if (!SendPacketBusy) {
			#ifdef COUNTER_MODE
			count++;
			*(P_IN_PACKET_SEND + 1) = count;
			#endif
			
			SendPacket(0);
		}
  }
}

