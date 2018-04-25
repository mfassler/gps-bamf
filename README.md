GPS Data-Logger for use with Bicycle and Strava.

Hardware:
--------
 - Arduino
 - GPS
 - 10dof (3d accel, 3d gyro, 3d magnet, air-pressure, temperature)
 - tachometer (magnetic pulse from spinning wheel)
 - sdcard attached to the SPI port



How To Write To SD Card:
-----------------------

The card that I am using is a 16 GB SDHC card, version 2.0.  It does NOT work
with older, version 1.0 cards.  I have no idea about newer SDXC cards...


The very first block of the SD card must start with this string:
```
GPS-Bamf!  https://github.com/mfassler/gps-bamf
```
... if so, then GPS-Bamf will take control of the entire card as a ring buffer.

 - Block #0 is the magic string. 
 - Block #1 records the stopping point (only updated once per 100 writes, to save wear-and-tear).
 - Blocks #2 through max is the ring buffer.

There is no file-format, really.  All of the serial data just gets written out to the card.

