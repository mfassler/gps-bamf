#ifndef __BAMF_TWI_H
#define __BAMF_TWI_H

extern int bamf_twi_write(char, char*, int);
extern int bamf_twi_write_read(char, char*, int, char, char*, int);


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


#endif // __TWI_H
