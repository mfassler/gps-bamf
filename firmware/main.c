/**
 *
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "system_timer.h"
#include "usart.h"
#include "bamf_twi.h"
#include "accelerometer.h"
#include "magnetometer.h"
#include "presstemp.h"
#include "tach.h"

extern volatile uint32_t jiffies;
extern volatile uint16_t tachy_count;

#define PRINTER_DELAY 50

int main(void) {
	int step = 0;
	int retval;

	int16_t accel_xyz[3];
	int16_t magnet_xyz[3];

	int32_t temperature;
	int32_t pressure;


	DDRB |= 1<<PD5; // On-board green LED

	// Initializations...
	USART0_Init(9600);
	_delay_ms(PRINTER_DELAY);
	USART0_printf("\n9600 8N1\n");
	_delay_ms(PRINTER_DELAY);

	USART0_printf("  Hello there!  (^o^)/  This is GPS_Bamf.\n\n");
	_delay_ms(PRINTER_DELAY);
	USART0_printf("Initializing hardware...\n");
	_delay_ms(PRINTER_DELAY);

	system_timer_init();

	retval = accel_init();
	if (retval < 0) {
		USART0_printf("accel_init() failed: %d\n", retval);
	}

	retval = magnet_init();
	if (retval < 0) {
		USART0_printf("magnet_init() failed: %d\n", retval);
	}

	retval = presstemp_init();
	if (retval < 0) {
		USART0_printf("presstemp_init() failed: %d\n", retval);
	}

	tachy_init();

	_delay_ms(PRINTER_DELAY);
	USART0_printf("JAGMT means: Jiffies, Accelerometer (x,y,z), Gyro (x,y,z), Mag (x,y,z), Tach%d\n", retval);
	_delay_ms(PRINTER_DELAY);
	USART0_printf("Jiffies are ~100 Hz.  Precise timing is not gauranteed.\n");
	_delay_ms(PRINTER_DELAY);
	USART0_printf("PT means: pressure, temp\n");
	_delay_ms(PRINTER_DELAY);
	USART0_printf("Pressure is hPa.  Temp is 0.1 degrees C.\n");
	_delay_ms(PRINTER_DELAY);
	USART0_printf("$GP... are NMEA sentences from the GPS module.\n");
	_delay_ms(PRINTER_DELAY);

	USART0_printf("\n\n *** Let us begin...\n\n");
	_delay_ms(PRINTER_DELAY);

	step = 0;
	while(1) {

		retval = magnet_take_sample();
		if (retval < 0) {
			USART0_printf("magnet_take_sample() failed: %d \n", retval);
		}

		retval = accel_take_sample();
		if (retval < 0) {
			USART0_printf("accel_take_sample() failed: %d \n", retval);
		} else if (retval == 0) {
			accel_get(accel_xyz);
			magnet_get(magnet_xyz);

			USART0_printf("JAGMT:%ld,%d,%d,%d,0,0,0,%d,%d,%d,%d\n", jiffies,
				accel_xyz[0], accel_xyz[1], accel_xyz[2],
				magnet_xyz[0], magnet_xyz[1], magnet_xyz[2],
				tachy_count
			);
		}

		// Each step is ~60 ms.  Precise timing is not guaranteed.
		switch (step) {
			case 0:
				PORTB = 0x20;  // on-board green LED ON
				break;
			case 1:
				PORTB = 0x00;  // on-board green LED OFF
				break;
			case 2:
				retval = presstemp_get_UT();
				if (retval < 0) {
					USART0_printf("presstemp_get_UT() failed: %d\n", retval);
				}
				break;
			case 3:
				retval = presstemp_get_UP();
				if (retval < 0) {
					USART0_printf("presstemp_get_UP() failed: %d\n", retval);
				}
				break;
			case 4:
				presstemp_calcPressureAndTemp(&temperature, &pressure);
				USART0_printf("PT:%ld,%ld\n", pressure, temperature);
				break;
			case 5:
				break;
			case 6:
				break;
			case 7:
				break;
			case 8:
				break;
			case 9:
				break;
			default:
				break;
		}

		step++;
		if (step > 9) {
			step = 0;
		}
		_delay_ms(66);
	}

	return 0;
}

