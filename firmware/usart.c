
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "usart.h"


// The incoming serial is used by the GPS module which spits out NMEA "sentences"

// Outgoing serial is just the NMEA sentence again, but also our own sentences
// for the various sensors.  

// For now, each sentence is plain ASCII and ends with CRLF (or LFCR??  whatever)

// We need to do line by line buffering so that the GPS sentences and our own
// sentences don't get mixed.

// The ATmega328 has 2K of SRAM
// Apparently, a NMEA sentence is, at most, 82 chars.
// So let's try an input buffer of 90 bytes * 5 lines (450 bytes)
//  (this input buffer will be re-used as the output buffer for GPS/NMEA sentences)
// And we'll need our own output buffer of, let's say... 256 bytes by 2 lines (512 bytes)...

// Apparently, the compiler does weird stuff to our global
// variables (as shared between main() and ISR()), unless
// we declare them "volatile"

// To put data into a FIFO is the "Producer"
// To take data from a FIFO is the "Consumer"


#define PRINT_BUFFER_SIZE 1024
volatile char print_buffer[PRINT_BUFFER_SIZE];
volatile uint16_t print_producer_idx = 0;
volatile uint16_t print_consumer_idx = 0;

#define FIFO_BUFFER_SIZE 100
volatile char fifo_buffer[FIFO_BUFFER_SIZE];
volatile uint8_t fifo_buffer_len = 0;


void _try_to_transmit(void) {
	while (
		(UCSR0A & (1 << UDRE0))  // Transmit buffer can take new data
		&& (print_producer_idx != print_consumer_idx) // and there is new data
	) {
		UDR0 = print_buffer[print_consumer_idx];  // Copy from memory to hardware
		print_consumer_idx++;
		if (print_consumer_idx >= PRINT_BUFFER_SIZE) {
			print_consumer_idx = 0;
		}
	}
}


ISR(USART1_RX_vect) {
	// Receive GPS NMEA sentences into a line-by-line FIFO buffer.
	int i;
	// Disable interrupts:
	UCSR1B = (1<<RXEN1) |  (1<<TXEN1);

	while (UCSR1A & (1 << RXC1)) {  // There is unread data in the receive buffer
		fifo_buffer[fifo_buffer_len] = UDR1;  // Read from HW into memory

		// Lines end in CRLF == '\n\r' == {0x0a, 0x0d}

		if (fifo_buffer[fifo_buffer_len] == 0x0a) {  // EOL

			for (i=0; i <= fifo_buffer_len; i++) {
				print_buffer[print_producer_idx] = fifo_buffer[i];

				print_producer_idx++;
				if (print_producer_idx >= PRINT_BUFFER_SIZE) {
					print_producer_idx = 0;
				}
			}

			fifo_buffer_len = 0;


			_try_to_transmit();

		} else {
			fifo_buffer_len++;
			if (fifo_buffer_len >= FIFO_BUFFER_SIZE) {
				// Ruh-roh.  This is a bug.  Buffer overflow.
				fifo_buffer_len = 0;
			}
		}
	}

	// Enable RX interrupts:
	UCSR1B = (1<<RXEN1) |  (1<<TXEN1) | (1<<RXCIE1);
}


ISR(USART0_TX_vect) {
	_try_to_transmit();
}


void USART0_Init(unsigned int baud_rate) {
	// USART0 is used to send out debug messages.

	unsigned int ubrr = F_CPU/16/baud_rate - 1;

	// Set baud rate
	UBRR0H = (unsigned char) (ubrr>>8);
	UBRR0L = (unsigned char) ubrr;

	// Enable receiver, transmitter, and tx interrupts:
	UCSR0B = (1<<RXEN0) |  (1<<TXEN0) | (1<<TXCIE0);

	// Set frame format: 8N1
	UCSR0C = (3<<UCSZ00);

	// Enable global interrupts:
	SREG |= (1 << SREG_I);
}


void USART1_Init(unsigned int baud_rate) {
	// USART1 is used to receive GPS sentences from the GPS module

	unsigned int ubrr = F_CPU/16/baud_rate - 1;

	// Set baud rate
	UBRR1H = (unsigned char) (ubrr>>8);
	UBRR1L = (unsigned char) ubrr;

	// Enable receiver, transmitter, and rx interrupts:
	UCSR1B = (1<<RXEN1) |  (1<<TXEN1) | (1<<RXCIE1);

	// Set frame format: 8N1
	UCSR1C = (3<<UCSZ10);

	// Enable global interrupts:
	SREG |= (1 << SREG_I);
}


void USART0_printf(const char *fmt, ...) {
	va_list args;
	char buffer[256];
	int bufLen;
	int i;

	// Disable interrupts:
	UCSR0B = (1<<RXEN0) |  (1<<TXEN0);

	va_start(args, fmt);
	bufLen = vsprintf(buffer, fmt, args);
	va_end(args);

	if (bufLen > 254) {
		// Bug.  Buffer overflow.  Don't do this...
		bufLen = 254;
	}

	buffer[bufLen - 1] = 0x0d;  // CR
	buffer[bufLen] = 0x0a;  // LF


	bufLen++;
	for (i=0; i < bufLen; i++) {
		print_buffer[print_producer_idx] = buffer[i];

		print_producer_idx++;
		if (print_producer_idx >= PRINT_BUFFER_SIZE) {
			print_producer_idx = 0;
		}
	}

	_try_to_transmit();

	// Enable interrupts:
	UCSR0B = (1<<RXEN0) |  (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
}

