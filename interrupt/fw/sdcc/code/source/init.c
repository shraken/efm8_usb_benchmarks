/**
 * @file init.c
 * @brief Init/bringup routines
 */

#include "init.h"

void system_init(void)
{
    PCA0MD &= ~0x40;                    // Disable Watchdog timer

    OSCICN |= 0x03;                     // Configure internal oscillator for
                                        // its maximum frequency
    RSTSRC  = 0x04;                     // Enable missing clock detector
    
    sysclk_init();                      // Initialize oscillator
    usb_init();                         // Initialize USB0
}

void sysclk_init(void)
{
    OSCICN |= 0x03;                     // Configure internal oscillator for
                                        // its maximum frequency and enable
                                        // missing clock detector

    CLKMUL  = 0x00;                     // Select internal oscillator as
                                        // input to clock multiplier

    CLKMUL |= 0x80;                     // Enable clock multiplier
    Delay();                            // Delay for clock multiplier to begin
    CLKMUL |= 0xC0;                     // Initialize the clock multiplier
    Delay();                            // Delay for clock multiplier to begin

    while(!(CLKMUL & 0x20));            // Wait for multiplier to lock
    CLKSEL = SYS_4x;                    // Select system clock
    CLKSEL |= USB_4X_CLOCK;             // Select USB clock
}

void usb_init(void)
{
   POLL_WRITE_BYTE (POWER, 0x08);      // Force Asynchronous USB Reset
   POLL_WRITE_BYTE (IN1IE, 0x07);      // Enable Endpoint 0-1 in interrupts
   POLL_WRITE_BYTE (OUT1IE,0x07);      // Enable Endpoint 0-1 out interrupts
   POLL_WRITE_BYTE (CMIE, 0x07);       // Enable Reset, Resume, and Suspend
                                       // interrupts
   USB0XCN = 0xE0;                     // Enable transceiver; select full speed
   POLL_WRITE_BYTE (CLKREC,0x8F);      // Enable clock recovery, single-step
                                       // mode disabled

   EIE1 |= 0x02;                       // Enable USB0 Interrupts

                                       // Enable USB0 by clearing the USB
                                       // Inhibit bit
   POLL_WRITE_BYTE(POWER,  0x01);      // and enable suspend detection
}

void Delay(void)
{
   int x;
	
   for(x = 0;x < 500;x)
      x++;
}