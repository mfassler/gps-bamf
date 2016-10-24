/*
 * Counter for external Tachometer
 *
 * Connect a Hall-Probe to PD5 (the other end is grounded)
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

extern volatile uint32_t jiffies;

volatile uint16_t tachy_count = 21;


void tachy_init(void) {
	// We will actually do the counter in software (so that we can have
	// an interrupt for each, individual event)

	// We want PD2 to be INT0 (external interrupt)

	// PD2 is an input:
	//DDRD &= ~(1 << PD2);
	DDRD = 0b11111011;

	// Enable global interrupts:
	SREG |= (1 << SREG_I);

	// PD2 is INT0 -- trigger an falling
	EICRA = (1<<ISC01); //0x02;
	//EICRA = 0x0f;

	// EIMSK - External Interrupt Mask
	EIMSK = (1<<INT0);
	//sei();

}


ISR(INT0_vect) {
	tachy_count++;

	USART0_printf("\t\t\t\t\t****** DING!  Tach:%ld,%d\n", jiffies, tachy_count);
}



// My wheels are 27" in diameter.
// My top speed ever was 60.2 km/hr
// That comes to: 13970.7 clicks per hour
//                3.88 clicks per second
