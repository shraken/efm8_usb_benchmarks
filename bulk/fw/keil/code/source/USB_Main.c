//-----------------------------------------------------------------------------
// USB_Main.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This application note covers the implementation of a simple USB application 
// using the interrupt transfer type. This includes support for device
// enumeration, control and interrupt transactions, and definitions of 
// descriptor data. The purpose of this software is to give a simple working 
// example of an interrupt transfer application; it does not include
// support for multiple configurations or other transfer types.
//
// How To Test:		See Readme.txt
//
//
// FID:				32X000024
// Target:			C8051F32x
// Tool chain:		Keil C51 7.50 / Keil EVAL C51
//					Silicon Laboratories IDE version 2.6
// Command Line:	 See Readme.txt
// Project Name:	 F32x_USB_Interrupt
//
//
// Release 1.3
//		-All changes by GP
//		-22 NOV 2005
//		-Changed revision number to match project revision
//		No content changes to this file
//		-Modified file to fit new formatting guidelines
//		-Changed file name from USB_MAIN.c

// Release 1.1
//		-All changes by DM
//		-22 NOV 2002
//		-Added support for switches and sample USB interrupt application.
//
// Release 1.0
//		-Initial Revision (JS)
//		-22 FEB 2002
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "USB_CDC_Type.h"
#include "USB_Register.h"
#include "USB_Main.h"
#include "USB_Descriptor.h"
#include "USB_Standard_Requests.h"
#include "USB_CDC_UART.h"

#include <string.h>

extern Ttxbuffer TXBuffer[ TXBUFSIZE ];	// Ring buffer for TX and RX
extern Trxbuffer RXBuffer[ RXBUFSIZE ];
extern Ttxbuffer * TXWTPtr;
extern Trxbuffer * RXRDPtr;

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F32x
//-----------------------------------------------------------------------------

#if defined KEIL

sfr16 TMR2RL	= 0xca;					// Timer2 reload value
sfr16 TMR2		= 0xcc;					// Timer2 counter

#endif // KEIL

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

#if defined KEIL

sbit Led1 = P2^2;						// LED='1' means ON
sbit Led2 = P2^3;

#endif // KEIL

#if defined SDCC

#define Led1   P2_2						// LED='1' means ON
#define Led2   P2_3

#endif // SDCC

//-----------------------------------------------------------------------------
// Local Prototypes
//-----------------------------------------------------------------------------

// Initialization Routines
void Sysclk_Init(void);					// Initialize the system clock
void Port_Init(void);					// Configure ports
void Usb0_Init(void);					// Configure USB for Full speed
void Timer_Init(void);					// Timer 2 for use by ADC and swtiches
void Adc_Init(void);					// Configure ADC for continuous
										// conversion low-power mode

// Other Routines
void Delay(void);						// About 80 us/1 ms on Full Speed

//-----------------------------------------------------------------------------
// ISR prototypes for SDCC
//-----------------------------------------------------------------------------

#if defined SDCC

extern void Timer2_ISR(void) interrupt 5;					// Checks if switches are pressed
//extern void Adc_ConvComplete_ISR(void) interrupt 10;		// Upon Conversion, switch ADC MUX
extern void Usb_ISR(void) interrupt 8;						// Determines type of USB interrupt

#endif // SDCC

//-----------------------------------------------------------------------------
// startup routine
//-----------------------------------------------------------------------------

#if defined SDCC

unsigned char _sdcc_external_startup ( void )
{
   PCA0MD &= ~0x40;                    // Disable Watchdog timer temporarily
   return 0;
}

#endif // SDCC

//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main(void)
{
	bebop_message_t msg[15]; // 6bytes each
	BYTE temp_out_buffer[RXBUFSIZE];
	BYTE hold_buffer[16];
//	BYTE Count;
	BYTE dt;
	BYTE line_state;
	
	int i;
	UINT main_count = 0;

#if defined KEIL

	PCA0MD &= ~0x40;					// Disable Watchdog timer temporarily

#endif // KEIL

	Sysclk_Init();						// Initialize oscillator
	Port_Init();						// Initialize crossbar and GPIO
	Usb0_Init();						// Initialize USB0

	Flush_COMbuffers();					// Initialize COM ring buffer

//	EIE1	|= 0x0A;					// Enable conversion complete interrupt and US0
	EIE1	|= 0x02;					// Enable USB0 interrupt
	IE		|= 0xA0;					// Enable Timer2 and Global Interrupt enable

	// fill temporary buffer
	for (i = 0; i < RXBUFSIZE; i++) {
		temp_out_buffer[i] = (BYTE) i;
	}
	
	memcpy( RXBuffer, temp_out_buffer, RXBUFSIZE );
	//RXWTPtr = RXBuffer;
	
	for (i = 0; i < sizeof(hold_buffer); i++) {
			hold_buffer[i] = 0x00 + i;
	}
	
	while (1)
	{
		Handle_In2();					// Poll IN/OUT EP2 and handle transaction
		Handle_Out2();

		/*
		// shraken TODO: 5/15/17 - this was causing issues with ReadFile on Windows -- might be
		// something underlying in that OS but worthwhile to look at as ideally we want to check
		// if DTR is active before spamming USB bus but it seems like USB stack already prevents
		// messages or the USB host doesn't IN poll our device unless a virtual COM port is opened
		// so I turned it off
		//
		// see if DTR signal is HIGH indicating a remote terminal has been opened
		if (uart_DTR) {
				// see if the storage buffer feeding endpoint IN2 has emptied and if
				// so then just set the buffer count size to full so that endpoint will
				// empty once again
				if (RXcount == 0) {
						RXcount = RXBUFSIZE - 1;
				}
		}
		*/

		/*
		// send static buffer by reseting the RXCount to always be the full
		// size of the buffer triggering the IN endpoint interrupt to constantly
		// push EP2_PACKET_SIZE or left over bytes into the endpoint buffer to
		// be sent to host
		if (RXcount == 0) {
		    RXcount = RXBUFSIZE - 1;
		}
		*/
		
		// send a simple data structure representing a single sensor value.  The sensor
		// 
		if (RXReady) {				
				// best-case latency: do not try to fill entire endpoint buffer
				COMPutMultiByte( (BYTE *) &msg, sizeof(bebop_message_t) );
		
				#if 0
				// worst-case latency: try to fill entire endpoint buffer being sending
				msg[0].sense_index = 0x00;
				msg[0].drive_index = 0x01;
				msg[0].value = 0x1234;
				
				COMPutMultiByte( (BYTE *) &msg, sizeof(bebop_message_t) * 15 );
				#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sysclk_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters	: None
//
// Initialize the system clock and USB clock
//
//-----------------------------------------------------------------------------
void Sysclk_Init(void)
{

#if defined C8051F320_H
	OSCICN |= 0x03;						// Configure internal oscillator for
										// its maximum frequency and enable
										// missing clock detector
	CLKMUL	= 0x00;						// Select internal oscillator as
										// input to clock multiplier
	CLKMUL |= 0x80;						// Enable clock multiplier
	Delay();							// Delay for clock multiplier to begin
	CLKMUL |= 0xC0;						// Initialize the clock multiplier
	Delay();							// Delay for clock multiplier to begin
	while(!(CLKMUL & 0x20));			// Wait for multiplier to lock
	CLKSEL	= SYS_4X_DIV_2 | USB_4X_CLOCK;	// Select system clock and USB clock
#endif // C8051F320_H

#if defined C8051F326_H
	OSCICN |= 0x82;
	CLKMUL  = 0x00;
	CLKMUL |= 0x80;						// Enable clock multiplier
	Delay();
	CLKMUL |= 0xC0;						// Initialize the clock multiplier
	Delay();							// Delay for clock multiplier to begin
	while(!(CLKMUL & 0x20));			// Wait for multiplier to lock
	CLKSEL = 0x02;						// Use Clock Multiplier/2 as system clock
#endif // C8051F326_H

#if defined C8051F340_H
	OSCICN |= 0x03;						// Configure internal oscillator for
										// its maximum frequency and enable
										// missing clock detector
	CLKMUL  = 0x00;						// Select internal oscillator as
										// input to clock multiplier
	CLKMUL |= 0x80;						// Enable clock multiplier
	Delay();							// Delay for clock multiplier to begin
	CLKMUL |= 0xC0;						// Initialize the clock multiplier
	Delay();							// Delay for clock multiplier to begin
	while(!(CLKMUL & 0x20));			// Wait for multiplier to lock
	CLKSEL	= SYS_4X | USB_4X_CLOCK;	// Select system clock and USB clock
#endif // C8051F340_H

#if defined C8051F380_H
	OSCICN |= 0x03;						// Configure internal oscillator for
										// its maximum frequency and enable
										// missing clock detector
	CLKMUL  = 0x00;						// Select internal oscillator as
										// input to clock multiplier
	CLKMUL |= 0x80;						// Enable clock multiplier
	Delay();							// Delay for clock multiplier to begin
	CLKMUL |= 0xC0;						// Initialize the clock multiplier
	Delay();							// Delay for clock multiplier to begin
	while(!(CLKMUL & 0x20));			// Wait for multiplier to lock
	//CLKSEL  = SYS_INT_OSC;              // Select system clock
  CLKSEL = SYS_4X;
	CLKSEL |= USB_4X_CLOCK;             // Select USB clock
#endif // C8051F380_H

}

//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters	: None
//
// This function configures the crossbar and GPIO ports.
//
// F320DB, F326DB
//  P1.7		analog					Potentiometer
//  P2.2		digital	push-pull		LED
//  P2.3		digital	push-pull		LED
//
// F340DB
//  P2.2		digital push-pull		LED
//  P2.3		digital push-pull		LED
//  P2.5		analog					Potentiometer
//-----------------------------------------------------------------------------
void Port_Init(void)
{

#if defined C8051F320_H
	P1MDIN	 = 0x7F;					// Port 1 pin 7 set as analog input
	P0MDOUT |= 0x0F;					// Port 0 pins 0-3 set push-pull
	P1MDOUT |= 0x0F;					// Port 1 pins 0-3 set push-pull
	P2MDOUT |= 0x0C;					// P2.2 and P2.3 set to push-pull
	P1SKIP	 = 0x80;					// Port 1 pin 7 skipped by crossbar
	XBR0	 = 0x00;
	XBR1	 = 0x40;					// Enable Crossbar
#endif // C8051F320_H

#if defined C8051F326_H
	P2MDOUT |= 0x0C;					// enable LEDs as a push-pull outputs
#endif // C8051F326_H

#if defined C8051F340_H
	P2MDIN   = ~(0x20);					// Port 2 pin 5 set as analog input
	P0MDOUT |= 0x0F;					// Port 0 pins 0-3 set high impedence
	P1MDOUT |= 0x0F;					// Port 1 pins 0-3 set high impedence
	P2MDOUT |= 0x0C;					// Port 2 pins 0,1 set high empedence
	P2SKIP   = 0x20;					// Port 2 pin 5 skipped by crossbar
	XBR0     = 0x00;
	XBR1     = 0x40;					// Enable Crossbar
#endif // C8051F340_H

}

//-----------------------------------------------------------------------------
// Usb0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters	: None
// 
// - Initialize USB0
// - Enable USB0 interrupts
// - Enable USB0 transceiver
// - Enable USB0 with suspend detection
//-----------------------------------------------------------------------------
void Usb0_Init(void)
{
	POLL_WRITE_BYTE(POWER,	0x08);		// Force Asynchronous USB Reset
										// USB Interrupt enable flags are reset by USB bus reset
										// 
	POLL_WRITE_BYTE(IN1IE,	0x01);		// Enable EP 0 interrupt, disable EP1-3 IN interrupt
	POLL_WRITE_BYTE(OUT1IE,	0x00);		// Disable EP 1-3 OUT interrupts
	POLL_WRITE_BYTE(CMIE,	0x05);		// Enable Reset and Suspend interrupts

	USB0XCN = 0xE0;						// Enable transceiver; select full speed
	POLL_WRITE_BYTE(CLKREC, 0x80);		// Enable clock recovery, single-step mode
										// disabled
										// Enable USB0 by clearing the USB 
										// Inhibit bit
	POLL_WRITE_BYTE(POWER,	0x01);		// and enable suspend detection
}

//-----------------------------------------------------------------------------
// Delay
//-----------------------------------------------------------------------------
//
// Used for a small pause, approximately 80 us in Full Speed,
// and 1 ms when clock is configured for Low Speed
//
//-----------------------------------------------------------------------------

void Delay(void)
{
	int x;
	for(x = 0;x < 500;x)
		x++;
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
