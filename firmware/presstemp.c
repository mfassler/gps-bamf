/**
 *
 * GPS_Bamf!  (Datalogger for Bicycle + Strava)
 *
 * Copyright 2016, Mark Fassler
 * Licensed under the GPLv3
 *
 */

#include <stdint.h> // for datatypes
#include "bamf_twi.h"
#include "presstemp.h"
#include "usart.h"

#include <util/delay.h>

#define PRESSTEMP_SLA_W 0xee
#define PRESSTEMP_SLA_R 0xef


// I guess each individual chip is calibrated at the factory?  We have
// to read the calibration parameters from the chip:
static int16_t presstemp_param_AC1;
static int16_t presstemp_param_AC2;
static int16_t presstemp_param_AC3;
static uint16_t presstemp_param_AC4;
static uint16_t presstemp_param_AC5;
static uint16_t presstemp_param_AC6;
static int16_t presstemp_param_B1;
static int16_t presstemp_param_B2;
static int16_t presstemp_param_MB;
static int16_t presstemp_param_MC;
static int16_t presstemp_param_MD;


int16_t _presstemp_readS16_reg(char reg) {
	int retval;

	char twiWriteBuf[1];
	char twiReadBuf[2];

	twiWriteBuf[0] = reg;
	retval = bamf_twi_write_read(PRESSTEMP_SLA_W, twiWriteBuf, 1, PRESSTEMP_SLA_R, twiReadBuf, 2);
	if (retval < 0) {
		USART0_printf("bamf_twi_write_read() failed: %d\n", retval);
		return -1;
	}

	return (int16_t)twiReadBuf[0] << 8 | twiReadBuf[1];
}


uint16_t _presstemp_readU16_reg(char reg) {
	int retval;

	char twiWriteBuf[1];
	char twiReadBuf[2];

	twiWriteBuf[0] = reg;
	retval = bamf_twi_write_read(PRESSTEMP_SLA_W, twiWriteBuf, 1, PRESSTEMP_SLA_R, twiReadBuf, 2);
	if (retval < 0) {
		USART0_printf("bamf_twi_write_read() failed: %d\n", retval);
		return -1;
	}

	return (uint16_t)twiReadBuf[0] << 8 | twiReadBuf[1];
}


int presstemp_readCoefficients(void) {
	//char twiWriteBuf[1];
	//char twiReadBuf[2];

	presstemp_param_AC1 = _presstemp_readS16_reg(0xaa);
	//USART0_printf("param AC1: %d\n", presstemp_param_AC1);

	presstemp_param_AC2 = _presstemp_readS16_reg(0xac);
	//USART0_printf("param AC2: %d\n", presstemp_param_AC2);

	presstemp_param_AC3 = _presstemp_readS16_reg(0xae);
	//USART0_printf("param AC3: %d\n", presstemp_param_AC3);

	presstemp_param_AC4 = _presstemp_readU16_reg(0xb0);
	//USART0_printf("param AC4: %u\n", presstemp_param_AC4);

	presstemp_param_AC5 = _presstemp_readU16_reg(0xb2);
	//USART0_printf("param AC5: %u\n", presstemp_param_AC5);

	presstemp_param_AC6 = _presstemp_readU16_reg(0xb4);
	//USART0_printf("param AC6: %u\n", presstemp_param_AC6);

	presstemp_param_B1 = _presstemp_readS16_reg(0xb6);
	//USART0_printf("param B1: %d\n", presstemp_param_B1);

	presstemp_param_B2 = _presstemp_readS16_reg(0xb8);
	//USART0_printf("param B2: %d\n", presstemp_param_B2);

	presstemp_param_MB = _presstemp_readS16_reg(0xba);
	//USART0_printf("param MB: %d\n", presstemp_param_MB);

	presstemp_param_MC = _presstemp_readS16_reg(0xbc);
	//USART0_printf("param MC: %d\n", presstemp_param_MC);

	presstemp_param_MD = _presstemp_readS16_reg(0xbe);
	//USART0_printf("param MD: %d\n", presstemp_param_MD);

	return 0;
}


int presstemp_init(void) {
	int retval;

	char twiWriteBuf[2];
	char twiReadBuf[2];

	// First, check the ChipID...
	twiWriteBuf[0] = 0xd0;  // ChipID register
	twiReadBuf[0] = 0x00;

	retval = bamf_twi_write_read(PRESSTEMP_SLA_W, twiWriteBuf, 1, PRESSTEMP_SLA_R, twiReadBuf, 1);
	if (retval < 0) {
		return retval;
	}
	if (twiReadBuf[0] != 0x55) {
		return -99; // so many problems...
	}

	retval = presstemp_readCoefficients();
	if (retval < 0) {
		return retval;
	}

	return 0;
	//return (int) twiReadBuf[0];
}


// The data is 16-bit, but we do calculations in 32-bit space
static int32_t presstemp_ucomp_T;  // uncompensated temperature
static int32_t presstemp_ucomp_P;  // uncompensated pressure


int presstemp_get_UT(void) {
	// Get the raw, uncompensated temparature
	int retval;

	char twiWriteBuf[2];

	twiWriteBuf[0] = 0xf4; // register Control
	twiWriteBuf[1] = 0x2e; // command: "read temp"

	retval = bamf_twi_write(PRESSTEMP_SLA_W, twiWriteBuf, 2);
	if (retval < 0) {
		USART0_printf("in get_UT, bamf_twi_write failed: %d\n", retval);
		return -1;
	}

	// spec sheet says we have to wait 4.5 ms for a conversion... :-/
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);

	presstemp_ucomp_T = (int32_t) _presstemp_readU16_reg(0xf6);
	//USART0_printf("uncomp_t: %ld\n", presstemp_ucomp_T);

	return 0;
}


int presstemp_get_UP(void) {
	// Get the raw, uncompensated pressure
	int retval;

	char twiWriteBuf[2];

	twiWriteBuf[0] = 0xf4; // register Control
	twiWriteBuf[1] = 0x34; // command: "read pressure"

	retval = bamf_twi_write(PRESSTEMP_SLA_W, twiWriteBuf, 2);
	if (retval < 0) {
		USART0_printf("in get_UP, bamf_twi_write failed: %d\n", retval);
		return -1;
	}

	// spec sheet says we have to wait 4.5 ms for a conversion... :-/
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);
	_delay_ms(1);

	presstemp_ucomp_P = (int32_t) _presstemp_readU16_reg(0xf6);
	//USART0_printf("uncomp_p: %ld\n", presstemp_ucomp_P);

	return 0;
}


/**
 *  From Adafruit: https://github.com/adafruit/Adafruit_BMP085_Unified
 *                      ->  Adafruit_BMP085_U.cpp
 */
/*
int32_t _presstemp_computeB5(int32_t ut) {
	int32_t X1 = (ut - (int32_t)_bmp085_coeffs.ac6) * ((int32_t)_bmp085_coeffs.ac5) >> 15;
	int32_t X2 = ((int32_t)_bmp085_coeffs.mc << 11) / (X1+(int32_t)_bmp085_coeffs.md);
	return X1 + X2;
}


void Adafruit_BMP085_Unified::getTemperature(float *temp)
  B5 = computeB5(UT);
  t = (B5+8) >> 4;
  t /= 10;

  *temp = t;
}
*/


void presstemp_calcPressureAndTemp(int32_t *temperature, int32_t *pressure) {

	// temperature is in 0.1 degrees C
	// pressure is in hPa

	// TODO: cleanup.  This be ugly...

	int32_t  X1, X2, B5, B6, X3, B3, p;
	uint32_t B4, B7;

	int16_t AC1 = presstemp_param_AC1;
	int16_t AC2 = presstemp_param_AC2;
	int16_t AC3 = presstemp_param_AC3;
	uint16_t AC4 = presstemp_param_AC4;
	//uint16_t AC5 = presstemp_param_AC5;
	//uint16_t AC6 = presstemp_param_AC6;
	int16_t B1 = presstemp_param_B1;
	int16_t B2 = presstemp_param_B2;
	//int16_t MB = presstemp_param_MB;
	//int16_t MC = presstemp_param_MC;
	//int16_t MD = presstemp_param_MD;

	//int32_t UT = presstemp_ucomp_T;  // uncompensated temperature
	int32_t UP = presstemp_ucomp_P;  // uncompensated pressure

	//int32_t temperature;  // in .1 degrees C
	//int32_t pressure; // in hPa
	int ossMode = 0;

	// Temperature compensation
	X1 = ((presstemp_ucomp_T - presstemp_param_AC6) * presstemp_param_AC5) >> 15;
	X2 = ((int32_t)presstemp_param_MC << 11) / (X1+(int32_t)presstemp_param_MD);
	B5 = X1 + X2;

	*temperature = (B5 + 8) >> 4;

	// Pressure compensation
	B6 = B5 - 4000;
	X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
	X2 = (AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((((int32_t)AC1) * 4 + X3) << ossMode) + 2) >> 2;
	X1 = (AC3 * B6) >> 13;
	X2 = (B1 * ((B6 * B6) >> 12)) >> 16;
	X3 = ((X1 + X2) + 2) >> 2;
	B4 = (AC4 * (uint32_t) (X3 + 32768)) >> 15;
	B7 = ((uint32_t) (UP - B3) * (50000 >> ossMode));

	if (B7 < 0x80000000) {
		p = (B7 << 1) / B4;
	} else {
		p = (B7 / B4) << 1;
	}

	X1 = (p >> 8) * (p >> 8);
	X1 = (X1 * 3038) >> 16;
	X2 = (-7357 * p) >> 16;
	*pressure = p + ((X1 + X2 + 3791) >> 4);
}

