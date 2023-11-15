#!/bin/bash
systemctl disable iperf3_odroid.service && sync

cp ./iperf3_odroid.service /etc/systemd/system/ && sync

systemctl enable iperf3_odroid.service && sync

systemctl restart iperf3_odroid.service && sync
