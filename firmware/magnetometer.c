/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdio.h>  // for datatypes
#include "bamf_twi.h"
#include "magnetometer.h"

// Address of magnetometer on i2c bus:
#define MAGNET_SLA_W 0x3c
#define MAGNET_SLA_R 0x3d


int magnet_init(void) {
	// inspired from the Adafruit code:
	//     https://github.com/adafruit/Adafruit_LSM303DLHC
	int retval;
	char twiWriteBuf[2];
	char twiReadBuf[1];

	// write 0x00 to MR_REG_M to enable device
	twiWriteBuf[0] = 0x02;  // MR_REG_M
	twiWriteBuf[1] = 0x00;  //   start device
	bamf_twi_write(MAGNET_SLA_W, twiWriteBuf, 2);

	// Check for a known value at a register to see if it's our device
	twiWriteBuf[0] = 0x00;  // CRA_REG_M
	retval = bamf_twi_write_read(MAGNET_SLA_W, twiWriteBuf, 1, MAGNET_SLA_R, twiReadBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (twiReadBuf[0] != 0x10) {
		return -99;  // so many problems...
	}

	// Set gain:
	twiWriteBuf[0] = 0x01;  // CRB_REG_M

	// Earth's magnetic field is ~0.25 to ~0.65 Gauss (according to wikipedia)
	// But as I code this, the building that I'm in has a stronger magnetic field
	//  than 1.3...   If a measurement on this chip overflows anywhere, then
	//  all values go to -4096.  (But somehow, the XY and the Z circuits are separate...)
	//twiWriteBuf[1] = 0x20;  // gain is +/- 1.3
	//twiWriteBuf[1] = 0x40;  // gain is +/- 1.9
	// ## This does not overflow in this room:
	twiWriteBuf[1] = 0x60;  // gain is +/- 2.5   XY factor: 670, Z factor: 600
	//twiWriteBuf[1] = 0x80;  // gain is +/- 4.0
	//twiWriteBuf[1] = 0xc0;  // gain is +/- 5.6
	bamf_twi_write(MAGNET_SLA_W, twiWriteBuf, 2);

	return 0;
}


// We probably don't need to do these averaging samples... I'm guessing that
// the compass direction changes pretty slowly...
#define MAG_NUMBER_OF_SAMPLES 4
volatile uint8_t mag_sample_number = 0;

volatile int16_t mag_x_samples[] = {0,0,0,0};
volatile int16_t mag_y_samples[] = {0,0,0,0};
volatile int16_t mag_z_samples[] = {0,0,0,0};

int magnet_take_sample(void) {
	char twiWriteBuf[1];
	char twiReadBuf[6];
	int retval;

	twiWriteBuf[0] = 0x03; // MAG_OUT_X_H_M

	retval = bamf_twi_write_read(MAGNET_SLA_W, twiWriteBuf, 1, MAGNET_SLA_R, twiReadBuf, 6);
	if (retval < 0) {
		return retval;
	}

	// yes, the order is X, Z, Y
	mag_x_samples[mag_sample_number] = (int16_t)(twiReadBuf[0] << 8) | (int16_t)twiReadBuf[1];
	mag_z_samples[mag_sample_number] = (int16_t)(twiReadBuf[2] << 8) | (int16_t)twiReadBuf[3];
	mag_y_samples[mag_sample_number] = (int16_t)(twiReadBuf[4] << 8) | (int16_t)twiReadBuf[5];

	mag_sample_number++;
	if (mag_sample_number >= MAG_NUMBER_OF_SAMPLES) {
		mag_sample_number = 0;
	}
	return (int) mag_sample_number;
}


int magnet_get(int16_t *xyz) {
	// xyz is an allocated vector of length 3
	int i;

	// Data type is 16 bit, but actual data is 12 bit, so we can add 16 samples without overflow

	xyz[0] = mag_x_samples[0] >> 2;
	xyz[1] = mag_y_samples[0] >> 2;
	xyz[2] = mag_z_samples[0] >> 2;

	for (i = 1; i < MAG_NUMBER_OF_SAMPLES; i++) {
		xyz[0] += (mag_x_samples[i] >> 2);
		xyz[1] += (mag_y_samples[i] >> 2);
		xyz[2] += (mag_z_samples[i] >> 2);
	}

	return 0;
}
