#!/usr/bin/python3

from bluepy.btle import Scanner
import sys

scanner = Scanner()
devices = scanner.scan(10.0)

rssiIn = int(sys.argv[1])

for device in devices:
    if (device.rssi > rssiIn):
        print("DEV = {} RSSI = {}.".format(device.addr, device.rssi))


