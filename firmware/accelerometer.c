/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdio.h>  // for datatypes
#include "bamf_twi.h"
#include "accelerometer.h"

// Address of accelerometer on i2c bus:
#define ACCEL_SLA_W 0x32
#define ACCEL_SLA_R 0x33


int accel_init(void) {
	// From adafruit:
	// write to LSM303_REGISTER_ACCEL_CTRL_REG1_A (0x20), value 0x57
	// read from same register, expect same value
	int retval;

	char accelWriteBuf[] = {0x20, 0x57};
	char accelReadBuf[] = {0x00};

	bamf_twi_write(ACCEL_SLA_W, accelWriteBuf, 2);
	retval = bamf_twi_write_read(ACCEL_SLA_W, accelWriteBuf, 1, ACCEL_SLA_R, accelReadBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (accelReadBuf[0] != 0x57) {
		return -99;  // so many problems...
	}

	return 0;
}


// Our goal here is to smooth out physical vibration, but still log accurate data.
// Our goal is to find the grade (slope of hill) and a forward acceleration of
// the bicycle, nothing more.  (Speed of bicycle, and therefore acceleration, will
// come from the tachometer, so we should be able to subtract out the forward
// accel (during data analysis, later) to find the grade.)
#define NUMBER_OF_SAMPLES 4
volatile uint8_t sample_number = 0;

volatile int16_t x_samples[] = {0,0,0,0};
volatile int16_t y_samples[] = {0,0,0,0};
volatile int16_t z_samples[] = {0,0,0,0};

int accel_take_sample(void) {
	char accelWriteBuf[1];
	char accelReadBuf[6];
	int retval;

	accelWriteBuf[0] = 0x28 | 0x80;  // the 0x80 bit means to continue reading multiple bytes

	retval = bamf_twi_write_read(ACCEL_SLA_W, accelWriteBuf, 1, ACCEL_SLA_R, accelReadBuf, 6);
	if (retval < 0) {
		return retval;
	}

	x_samples[sample_number] = (int16_t)(accelReadBuf[0] | (accelReadBuf[1] << 8)) >> 4;
	y_samples[sample_number] = (int16_t)(accelReadBuf[2] | (accelReadBuf[3] << 8)) >> 4;
	z_samples[sample_number] = (int16_t)(accelReadBuf[4] | (accelReadBuf[5] << 8)) >> 4;

	sample_number++;
	if (sample_number >= NUMBER_OF_SAMPLES) {
		sample_number = 0;
	}
	return (int) sample_number;
}


int accel_get(int16_t *xyz) {
	// xyz is an allocated vector of length 3
	int i;

	// Data type is 16 bit, but actual data is 12 bit, so we can add 16 samples without overflow

	xyz[0] = x_samples[0];
	xyz[1] = y_samples[0];
	xyz[2] = z_samples[0];

	for (i = 1; i < NUMBER_OF_SAMPLES; i++) {
		xyz[0] += x_samples[i];
		xyz[1] += y_samples[i];
		xyz[2] += z_samples[i];
	}

	// right-shift by 2 is equivalent to divide by 4
	// (According to the Arduino docs, this trick is valid for signed ints)
	xyz[0] >>= 2;
	xyz[1] >>= 2;
	xyz[2] >>= 2;

	return 0;
}
