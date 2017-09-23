/*
 * The GPS module gives us a PPS (Pulse Per Second) signal.  This signal is sync'd to
 * the atomic clocks aboard the GPS satellites.  The rising edge means that the
 * atomic clock has clicked to a new second.  
 *
 * If we sync our seconds to the PPS signal, and if our own internal clock is halfway
 * decent, then we should be able to achieve ~1us clock accuracy.
 *  (I imagine most of our inaccuracy will come from the time it takes for the
 *   processor/ram to actually execute instructions...)
 *
 */


// PPS is connected to pin 19 of the ATmega644, which is configured as PCINT29 input

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

extern volatile uint32_t jiffies;


void gps_pps_init(void) {

	// PD5 is an input:
	DDRD &= ~(1 << PD5);

	// Enable global interrupts:
	SREG |= (1 << SREG_I);

	// PD5 is PCINT29

	// PCINT29 lives on the PCI3 interrupts.  Enable PCI3 interrupts:
	PCICR |= (1<<PCIE3);

	// Only listen to PCINT29:
	PCMSK3 = (1<<PCINT29);
}


ISR(PCINT3_vect) {
	uint32_t local_jiffies;

	// Copy the global, volatile jiffies to a local variable as quickly as possible:
	local_jiffies = jiffies;
	// This is when the interrupt actually occured, regardless of how slow this function is.

	//USART0_printf("PIND: %x\n", PIND);

	if (PIND & (1<<PD5)) {  // We only care about the rising edge
		USART0_printf(" ######### PPS: %ld\n", local_jiffies);
	//} else {
	//	USART0_printf(" ######### PPS ######### --\n");
	}
}

