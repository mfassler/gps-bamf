#!/usr/bin/env python
# -*- coding: UTF-8 -*-
'''
Tinkering that I did on Fedora 23 using this library:

http://www.intra2net.com/en/developer/libftdi/download/libftdi1-1.2.tar.bz2
'''

import sys
import time
import ftdi1 as ftdi


# My DS_UM232H:
usb_vendor = 0x0403
usb_model = 0x6014

context = ftdi.new()

dev = ftdi.usb_open(context, usb_vendor, usb_model)

if dev < 0:
    print 'Failed to open.  :-('


while True:
    aa, bb = ftdi.read_data(context, 1000)
    if aa != 0:
        # print bb[:aa],
        sys.stdout.write(bb[:aa])
    else:
        time.sleep(0.01)


