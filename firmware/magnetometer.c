/*
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdint.h>  // for datatypes
#include "bamf_twi.h"

// Address of magnetometer on i2c bus:
#define MAGNET_SLA_W 0x3c
#define MAGNET_SLA_R 0x3d


int magnet_init(void) {
	// inspired from the Adafruit code:
	//     https://github.com/adafruit/Adafruit_LSM303DLHC
	int retval;
	char writeBuf[2];
	char readBuf[1];

	// write 0x00 to MR_REG_M to enable device
	writeBuf[0] = 0x02;  // MR_REG_M
	writeBuf[1] = 0x00;  //   start device
	bamf_twi_write(MAGNET_SLA_W, writeBuf, 2);

	// Check for a known value at a register to see if it's our device
	writeBuf[0] = 0x00;  // CRA_REG_M
	retval = bamf_twi_write_read(MAGNET_SLA_W, writeBuf, 1, MAGNET_SLA_R, readBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (readBuf[0] != 0x10) {
		return -99;  // so many problems...
	}

	// Set gain:
	writeBuf[0] = 0x01;  // CRB_REG_M

	// Earth's magnetic field is ~0.25 to ~0.65 Gauss (according to wikipedia)
	// But as I code this, the building that I'm in has a stronger magnetic field
	//  than 1.3...   If a measurement on this chip overflows anywhere, then
	//  all values go to -4096.  (But somehow, the XY and the Z circuits are separate...)
	//twiWriteBuf[1] = 0x20;  // gain is +/- 1.3
	//twiWriteBuf[1] = 0x40;  // gain is +/- 1.9
	// ## This does not overflow in this room:
	writeBuf[1] = 0x60;  // gain is +/- 2.5   XY factor: 670, Z factor: 600
	//twiWriteBuf[1] = 0x80;  // gain is +/- 4.0
	//twiWriteBuf[1] = 0xc0;  // gain is +/- 5.6
	bamf_twi_write(MAGNET_SLA_W, writeBuf, 2);

	return 0;
}


int magnet_take_sample(int16_t *xyz) {
	char writeBuf[1];
	char readBuf[6];
	int retval;

	writeBuf[0] = 0x03; // MAG_OUT_X_H_M

	retval = bamf_twi_write_read(MAGNET_SLA_W, writeBuf, 1, MAGNET_SLA_R, readBuf, 6);
	if (retval < 0) {
		return retval;
	}

	// yes, the order is X, Z, Y
	xyz[0] = (int16_t)(readBuf[0] << 8) | (int16_t)readBuf[1];
	xyz[2] = (int16_t)(readBuf[2] << 8) | (int16_t)readBuf[3];
	xyz[1] = (int16_t)(readBuf[4] << 8) | (int16_t)readBuf[5];

	return 0;
}

