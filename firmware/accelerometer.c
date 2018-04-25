/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdio.h>  // for datatypes
#include "bamf_twi.h"

// Address of accelerometer on i2c bus:
#define ACCEL_SLA_W 0x32
#define ACCEL_SLA_R 0x33


int accel_init(void) {
	int retval;

	// -----------------
	// Enable the device
	// -----------------

	// 0x20 is CTRL_REG1_A
	// 0x5. sets update rate to 100 Hz
	// 0x2. sets update rate to 10 Hz
	// 0x.7 turns on the device and enables all three axes
	char writeBuf[] = {0x20, 0x27};
	char readBuf[] = {0x00};

	bamf_twi_write(ACCEL_SLA_W, writeBuf, 2);
	retval = bamf_twi_write_read(ACCEL_SLA_W, writeBuf, 1, ACCEL_SLA_R, readBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (readBuf[0] != 0x27) {
		return -99;  // so many problems...
	}


	// ------------------
	// Set the data range
	// ------------------

	// CTRL_REG4_A:
	writeBuf[0] = 0x23;
	// 0x10 sets the range to be +/- 4G (instead of 2G)
	writeBuf[1] = 0x10;

	readBuf[0] = 0x00;

	bamf_twi_write(ACCEL_SLA_W, writeBuf, 2);
	retval = bamf_twi_write_read(ACCEL_SLA_W, writeBuf, 1, ACCEL_SLA_R, readBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (readBuf[0] != 0x10) {
		return -98;
	}

	return 0;
}


int accel_take_sample(int16_t *xyz) {
	char writeBuf[1];
	char readBuf[6];
	int retval;

	writeBuf[0] = 0x28 | 0x80;  // the 0x80 bit means to continue reading multiple bytes

	retval = bamf_twi_write_read(ACCEL_SLA_W, writeBuf, 1, ACCEL_SLA_R, readBuf, 6);
	if (retval < 0) {
		return retval;
	}

	xyz[0] = (int16_t)(readBuf[0] | (readBuf[1] << 8)) >> 4;
	xyz[1] = (int16_t)(readBuf[2] | (readBuf[3] << 8)) >> 4;
	xyz[2] = (int16_t)(readBuf[4] | (readBuf[5] << 8)) >> 4;

	return 0;
}

