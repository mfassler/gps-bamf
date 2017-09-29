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
	// From adafruit:
	// write to LSM303_REGISTER_ACCEL_CTRL_REG1_A (0x20), value 0x57
	// read from same register, expect same value
	int retval;

	char writeBuf[] = {0x20, 0x57};
	char readBuf[] = {0x00};

	bamf_twi_write(ACCEL_SLA_W, writeBuf, 2);
	retval = bamf_twi_write_read(ACCEL_SLA_W, writeBuf, 1, ACCEL_SLA_R, readBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (readBuf[0] != 0x57) {
		return -99;  // so many problems...
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

