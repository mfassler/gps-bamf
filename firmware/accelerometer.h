#ifndef __ACCELEROMETER_H
#define __ACCELEROMETER_H

extern int accel_init(void);
extern int accel_take_sample(int16_t*);


/*
 * Device addresses for the AdaFruit 10-DOF:
 *
 * 0x32 - Accelerometer WRITE
 * 0x33 - Accelerometer READ
 * 
 * 0x3c - Mag WRITE
 * 0x3d - Mag READ
 *
 * 0xd4 to 0x47 -- gyroscope
 *
 * 0xee - Temp/Pressure WRITE
 * 0xef - Temp/Pressure READ
 */


#endif // __ACCELEROMETER_H
