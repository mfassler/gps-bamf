/*
 * Counter for external Tachometer
 *
 * Connect a Hall-Probe to PB2
 *   (prolly want to have a band-pass filter and Schmitt trigger...)
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

extern volatile uint32_t jiffies;

volatile uint16_t tachy_count = 21;


void tachy_init(void) {
	// We will do the counter in software so that we can have
	// an interrupt for each, individual event

	// We want PB2 to be INT2 (external interrupt)

	// PB2 is an input:
	DDRB &= ~(1 << PB2);

	// Enable global interrupts:
	SREG |= (1 << SREG_I);

	// PB2 is INT2 -- trigger on rising and falling
	EICRA = (1<<ISC20);

	// EIMSK - External Interrupt Mask
	EIMSK = (1<<INT2);
	//sei();

}


ISR(INT2_vect) {
	uint32_t local_jiffies;

	// Copy the global, volatile jiffies to a local variable as quickly as possible:
	local_jiffies = jiffies;

	if (PINB & (1<<PB2)) {
		tachy_count++;
		USART0_printf("### TACH ON:%ld, %d\n", local_jiffies, tachy_count);
	} else {
		USART0_printf("### TACH OFF:%ld, %d\n", local_jiffies, tachy_count);
	}
}


// My wheels are 27" in diameter.
// My top speed ever was 60.2 km/hr
// That comes to: 13970.7 clicks per hour
//                3.88 clicks per second
