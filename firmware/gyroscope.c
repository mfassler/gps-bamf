/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdint.h>  // for datatypes
#include "bamf_twi.h"

// Address of gyroscope on i2c bus:
#define GYRO_SLA_W 0xd6
#define GYRO_SLA_R 0xd7


int gyro_init(void) {

	int retval;

	char writeBuf[] = {0x0f, 0x00};
	char readBuf[] = {0x00, 0x00};

	retval = bamf_twi_write_read(GYRO_SLA_W, writeBuf, 1, GYRO_SLA_R, readBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if ((readBuf[0] == 0xd4) || (readBuf[0] == 0xd7)) {
		// Yay!
	} else {
		return -99;  // so many problems...
	}

	// Turn the device on:
	writeBuf[0] = 0x20;  // CTRL_REG1
	writeBuf[1] = 0x00;  // turn off
	retval = bamf_twi_write(GYRO_SLA_W, writeBuf, 2);

	writeBuf[1] = 0x0f;  // Wake up
	retval = bamf_twi_write(GYRO_SLA_W, writeBuf, 2);
	if (retval < 0) {
		return retval;
	}
	
	return 0;
}



int gyro_take_sample(int16_t *xyz) {
	char writeBuf[1];
	char readBuf[8];
	int retval;

	writeBuf[0] = 0x26 | 0x80;

	retval = bamf_twi_write_read(GYRO_SLA_W, writeBuf, 1, GYRO_SLA_R, readBuf, 8);
	if (retval < 0) {
		return retval;
	}

	xyz[0] = (int16_t)(readBuf[2] | (readBuf[3] << 8));
	xyz[1] = (int16_t)(readBuf[4] | (readBuf[5] << 8));
	xyz[2] = (int16_t)(readBuf[6] | (readBuf[7] << 8));

	return 0;
}



