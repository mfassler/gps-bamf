
/*
 * System Timer Interrupt.
 *
 * Our jiffies are approx. 100 Hz.
 * 16-bit jiffies will roll-over once every 10.9 minutes.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t jiffies;

void system_timer_init(void) {
	// Timer0
	PRR &= ~(1<<PRTIM0);  // Disable power reduction for timer0
	TCCR0A = 0;  // Normal mode
	TCCR0B = (1<<CS02) | (1<<CS00);   // Prescaler = clk/1024


	/* Timeout =  Number_of_steps / (Clk / Prescaler)
	 *  So, on the USBKey (8Mhz) with a 16-bit timer, with a 1024 pre-scalaer,
	 * the timeout is 65536 / ( 8 MHz / 1024) = 8.39 seconds.
	 */
	TCNT0 = 0xb2;  // Should be about 0.01 seconds

	// Enable interrupt:
	TIMSK0 = (1<<TOIE0); // Timer 0 Overflow Interrupt Enable

	// Enable global interrupts:
	SREG |= (1 << SREG_I);
}

ISR(TIMER0_OVF_vect) {
	TCNT0 = 0xb2;  // Should be about 0.01 seconds
	jiffies++;
}



/*
 I can press a key about 10 times per second, max
 so button presses faster than ~10Hz will be ignored

 */


