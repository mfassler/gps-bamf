#!/usr/bin/env python3

import sys
if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

import time
import calendar
import numpy as np
import matplotlib.pyplot as plt

from enum import IntEnum



# ******* Parse the strings
#f = open('bamf-sdcard-2018-04-14/strings.txt', 'rb')
#f = open('bamf-sdcard-2018-04-14/only_gps_and_10dof_and_cleaned_by_hand.txt', 'rb')
f = open('bamf-sdcard-2018-04-14/only_gps_and_10dof_and_cleaned_by_hand__fixed_misordered_pps.txt', 'rb')
allLines = list(f)
f.close()

# We will try to attach timestamps to each line in the logfile.
# The GPS gives us correct timestamps, but the incoming is always delayed
#  (over a 9600 baud link).
# The PPS (pulse-per-second) signal is our most precise clock, occurring
#  at the xxx.000000 boundary.  So we have to find the GPS timestamp, and
#  backtrack to the PPS signal.
# The log has "jiffie" timestamps, which seemed to be occuring at about
# 95 Hz (but not precisely).  So from all this we can infer the absolute
# timestamp of each line to approx 11 ms accuracy.  
#
# Note that if the gps-bamf was reset, then the jiffie numbers will be reset.  So
# we have to tolerate that.   (... and there might be other missing data...  and
# sometimes we might lose GPS lock...)
#
# The first column is the SECONDs Integer of the GPS timestamp
# The second column is the inferred (interpolated) MICROSECONDS Integer in-between
# the PPS lines



###  hrmm.... pandas DataFrames are slow as fuq for me...
###   (mebbe I'm doing something wrong...)
#import pandas
#
#df = pandas.DataFrame({'rowType':         np.zeros(len(allLines), np.uint8),
#                       'rawJiffieNumber': np.zeros(len(allLines), np.uint32),
#                       'gpsTimestamp':    np.zeros(len(allLines), np.uint32)})



df_rowType = np.zeros(len(allLines), np.uint8)
df_rawJiffieNumber = np.zeros(len(allLines), np.uint32)
df_gpsTimestamp = np.zeros(len(allLines), np.uint32)

df_calcTimestamp = np.zeros(len(allLines), np.float64)

df_pressure = np.zeros(len(allLines), np.float64)
df_temp = np.zeros(len(allLines), np.float32)

df_accel = np.zeros((len(allLines), 3), np.float64)
df_gyro = np.zeros((len(allLines), 3), np.float64)
df_mag = np.zeros((len(allLines), 3), np.float64)

#rowTypes = ['unknown', 'corrupt', 'validPPS', 'validGPRMC']

class RowType(IntEnum):
    UNKNOWN = 0
    CORRUPT = 1
    VALID_PPS = 2
    VALID_GPRMC = 3
    VALID_JAGMT = 4
    VALID_PT = 5
    PERFECT_TIMESTAMP = 6



tss = np.zeros((len(allLines), 2), np.uint32)
validTss = np.zeros(len(allLines), np.bool)

def parse_NMEA_gprmc_sentence(inputString):
    if not inputString.startswith(b'$GPRMC'):
        return
    pieces = inputString.split(b',')
    ## From the NMEA protocol:
    # pieces[0] -> "$GPRMC"
    # pieces[1] -> time-of-day timestamp
    # pieces[2] -> "A" for okay, or "V" for warning
    # pieces[3] -> latitude
    # pieces[4] -> lat hemisphere, "N" or "S"
    # pieces[5] -> longitude
    # pieces[6] -> lon hemisphere, "E" or "W"
    # pieces[7] -> speed
    # pieces[8] -> true course
    # pieces[9] -> date, eg:  "140418" for 14-April-2018
    # pieces[10] -> variation
    # pieces[11] -> east/west
    # pieces[12] -> checksum
    if pieces[2] != b'A':
        return
    pyTime = time.strptime(pieces[1].decode() + ' ' + pieces[9].decode(), '%H%M%S.000 %d%m%y')
    ts = calendar.timegm(pyTime)

    return {"ts": ts}


for i, oneLine in enumerate(allLines):
    if oneLine.startswith(b'$GPRMC'):
        try:
            gprmc = parse_NMEA_gprmc_sentence(oneLine)
        except:
            print("Failed to parse line: ", i)
            df_rowType[i] = RowType.CORRUPT
            #df_rawJiffieNumber[i] is already 0
            #df_gpsTimestamp[i] is already 0
            # print("  --- ", oneLine)
        else:
            if gprmc is None:
                print("dafuq?")
                df_rowType[i] = RowType.CORRUPT
                #df_rawJiffieNumber[i] is already 0
                #df_gpsTimestamp[i] is already 0
                # print("  --- ", oneLine)
            else:
                df_rowType[i] = RowType.VALID_GPRMC
                #df_rawJiffieNumber[i] is already 0
                df_gpsTimestamp[i] = gprmc['ts']

    elif b'PPS' in oneLine:
        try:
            jif = int(oneLine.strip().split(b' ')[-1], 10)
        except:
            print('warning: PPS, bad jiffieStamp: ', i)
            jif = None
        df_rowType[i] = RowType.VALID_PPS
        df_rawJiffieNumber[i] = jif
        #df_gpsTimestamp[i] = is already 0

    elif oneLine.startswith(b'JAGMT:'):
        try:
            pieces = oneLine.split(b',')
            assert len(pieces) == 11
            ppieces = pieces[0].split(b':')
            jif = int(ppieces[1], 10)
            acc_x = int(pieces[1], 10)
            acc_y = int(pieces[2], 10)
            acc_z = int(pieces[3], 10)
            gyr_x = int(pieces[4], 10)
            gyr_y = int(pieces[5], 10)
            gyr_z = int(pieces[6], 10)
            mag_x = int(pieces[7], 10)
            mag_y = int(pieces[8], 10)
            mag_z = int(pieces[9], 10)
            tach = int(pieces[10], 10)
        except:
            print('failed to parse JAGMT line: ', i)
            df_rowType[i] = RowType.CORRUPT
            #df_rawJiffieNumber[i] is already 0
            #df_gpsTimestamp[i] is already 0
        else:
            df_rowType[i] = RowType.VALID_JAGMT
            df_rawJiffieNumber[i] = jif
            #df_gpsTimestamp[i] = is already 0
            df_accel[i] = float(acc_x), float(acc_y), float(acc_z)
            df_gyro[i] = float(gyr_x), float(gyr_y), float(gyr_z)
            df_mag[i] = float(mag_x), float(mag_y), float(mag_z)
            # not doing tach yet...

    elif oneLine.startswith(b'PT:'):
        try:
            pieces = oneLine.strip().split(b':')
            ppieces = pieces[1].split(b',')
            pressure = float(ppieces[0])
            temperature = float(ppieces[1]) / 10.0
        except:
            print("  ** failed to parse PT on line: ", i)
            df_rowType[i] = RowType.CORRUPT
        else:
            df_rowType[i] = RowType.VALID_PT
            df_pressure[i] = pressure
            df_temp[i] = temperature



#failed to JAGMT parse line:  58016
#failed to JAGMT parse line:  253948
#failed to JAGMT parse line:  273156

# ... there is an obvious break in the data around line 58016.
#  Other than that, the jiffies will give us a decent baseline
#  of timestamps, ... we just need the PPS lines and the GPS
#  timestamps to synchronize to GPS time.


###  NOTE NOTE NOTE:   np.where() returns a tuple!!!

validGpsIdxs, = np.where(df_rowType == RowType.VALID_GPRMC)  # returns a tuple
validJifIdxs, = np.where(np.logical_or(df_rowType == RowType.VALID_JAGMT,
                                       df_rowType == RowType.VALID_PPS))

validPtIdxs, = np.where(df_rowType == RowType.VALID_PT)


#goodCount = 0
#missingPps = 0
#missingGpsTs = 0

for i, idx in enumerate(validGpsIdxs[:-1]):
    nextIdx = validGpsIdxs[i+1]
    if df_gpsTimestamp[nextIdx] - df_gpsTimestamp[idx] == 1:
        if RowType.VALID_PPS in df_rowType[idx:nextIdx]:
            tmtbjc, = np.where(df_rowType[idx:nextIdx] == RowType.VALID_PPS)
            ppsIdx = tmtbjc[0] + idx
            print('perfect match at: ', idx, ppsIdx)
            df_rowType[ppsIdx] = RowType.PERFECT_TIMESTAMP
            df_gpsTimestamp[ppsIdx] = df_gpsTimestamp[nextIdx]


    #        goodCount += 1
    #    else:
    #        missingPps += 1
    #else:
    #    missingGpsTs += 1


# Now we take the "perfect" timestamps, and use simple, linear interpolation
# with the "jiffies" to infer the timestamps of all lines with known jiffies

perfectIdxs, = np.where(df_rowType == RowType.PERFECT_TIMESTAMP)

for i, idx in enumerate(perfectIdxs[:-1]):
    nextIdx = perfectIdxs[i+1]
    print("i: %d, idx: %d, nextIdx: %d" % (i, idx, nextIdx))
    ts0 = df_gpsTimestamp[idx]
    ts1 = df_gpsTimestamp[nextIdx]
    j0 = df_rawJiffieNumber[idx]
    j1 = df_rawJiffieNumber[nextIdx]
    # y = mx+b
    # y = m(x-x0) + y0
    m = float(ts1 - ts0) / float(j1 - j0)
    b = float(ts0)
    for ii in range(idx, nextIdx):
        if ii in validJifIdxs:
            print(ii, m, b)
            jjj = df_rawJiffieNumber[ii] - j0
            df_calcTimestamp[ii] = m * float(jjj) + b


validCalcs, = np.where(df_calcTimestamp != 0.0)

assert False, 'stopping here'

plt.plot(validCalcs, df_calcTimestamp[validCalcs], 'r.')
plt.plot(validGpsIdxs, df_gpsTimestamp[validGpsIdxs], 'b,')


imuIdxs, = np.where(df_rowType == RowType.VALID_JAGMT)


####  IMU, mofo, do you speak it?


xx = df_accel[imuIdxs, 0].mean()
yy = df_accel[imuIdxs, 1].mean()
zz = df_accel[imuIdxs, 2].mean()

gravity_in_lsb = np.sqrt(xx**2 + yy**2 + zz**2)
# gravity should be exactly 1000, but the datasheet says that the 
# absolute calibration could be off by +/- 60, so this fits.

# This will allow us to convert from lsbits to m/s**2:
g = 9.8 / gravity

z_velocity = np.zeros(len(allLines))

## The first valid timestamp doesn't occur until imuIdx[37]
## The last valid timestamp is at imuIdx[-20] or so

imuIdxs = imuIdxs[38:-21]

# this just leaves the ~30 second jump where there was data loss
#np.where(np.diff(df_calcTimestamp[imuIdxs]) > 1)
#np.where(np.diff(df_calcTimestamp[imuIdxs]) < 0)

for i, idx in enumerate(imuIdxs[:-1]):
    nextIdx = imuIdxs[i+1]

    delta_t = df_calcTimestamp[nextIdx] - df_calcTimestamp[idx]
    z_accel = (df_accel[nextIdx, 2] - zz) * g

    change_in_velocity = z_accel * delta_t

    z_velocity[nextIdx] = z_velocity[idx] + change_in_velocity




z_position = np.zeros(len(allLines))

for i, idx in enumerate(imuIdxs[:-1]):
    nextIdx = imuIdxs[i+1]

    delta_t = df_calcTimestamp[nextIdx] - df_calcTimestamp[idx]
    change_in_position = z_velocity[idx] * delta_t
    z_position[nextIdx] = z_position[idx] + change_in_position





# Altitude by air pressure
alt = np.zeros(len(allLines))
K = 44330.0
p_0 = 101695.0

for i, idx in enumerate(validPtIdxs):
    pressure = df_pressure[idx]
    alt[idx] = K * (1.0 - (float(pressure) / p_0) ** (1/5.255))


gScale = alt.max() / z_position.max()
plt.plot(validPtIdxs, alt[validPtIdxs], 'r.')
plt.plot(imuIdxs, z_position[imuIdxs] * gScale, 'b.')
plt.show()



