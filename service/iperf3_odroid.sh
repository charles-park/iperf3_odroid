#!/bin/bash

#--------------------------
# delay for system stability
#--------------------------
sleep 10 && sync

#--------------------------
# ODROID-M1S Server enable (iperf3 odroid iperf3 demon mode)
#--------------------------
while true
do
    /usr/bin/iperf3_odroid -s -p 8000
    sleep 1
done
