


AC1 = 7592
AC2 = -1133
AC3 = -14903
AC4 = 34089
AC5 = 24936
AC6 = 14672
B1 = 6515
B2 = 43
MB = -32768
MC = -11786
MD = 2859

#UT = 23655  # room temp  (27 C)
#UT = 24159  # sortof body temp (30C)
UT = 23591
UP = 41876


X1 = (UT-AC6) * AC5 >> 15
X2 = (MC << 11) / (X1 + MD)
B5 = X1 + X2
T = (B5 + 8) >> 4


B6 = B5 - 4000
X1 = (B2 * ((B6 * B6) >> 12)) >> 11
X2 = (AC2 * B6) >> 11
X3 = X1 + X2
ossMode = 0
B3 = (((AC1 * 4 + X3) << ossMode ) + 2) >> 2
X1 = (AC3 * B6) >> 13
X2 = (B1 * ((B6 * B6) >> 12)) >> 16
X3 = ((X1 + X2) + 2) >> 2;
B4 = (AC4 * (X3 + 32768)) >> 15
B7 = (UP - B3) * (50000 >> ossMode)

if B7 < 0x80000000:
    p = (B7 << 1) / B4
else:
    p = (B7 / B4) << 1

X1 = (p >> 8) * (p >> 8)
X1 = (X1 * 3038) >> 16
X2 = (-7357 * p) >> 16
compp = p + ((X1 + X2 + 3791) >> 4)


###
### Also note:
# from the repo:  https://github.com/adafruit/Adafruit_BMP085_Unified
#  - the file:  Adafruit_BMP085_U.cpp
# already contains a C implementation of this Algorithm, no FPU required.


def calcAltitude(pressure):
    p_0 = 101325.0
    K = 44330.0
    alt = K * (1.0 - (float(pressure) / p_0) ** (1/5.255))
    return alt
