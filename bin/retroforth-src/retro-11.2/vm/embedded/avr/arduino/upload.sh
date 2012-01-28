#!/bin/sh

avrdude -F -V -c stk500v2 -p m2560 -b 115200 -P /dev/ttyU0 -U flash:w:retro.hex
