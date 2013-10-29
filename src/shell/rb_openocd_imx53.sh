#!/bin/bash

OPENOCD_DIR=''

StartOpenOCDServer() {
    $OPENOCD_DIR/openocd -f ./tcl/interface/olimex-arm-usb-ocd.cfg -f ./tcl/target/imx53.cfg > openocd.server.log 2> openocd.error.log &
}

StartOpenOCDClient() {
     telnet localhost 4444
     # < imx53.cpu arp_halt
}

PrintHelp() {
    echo 'server - start openOCD server'
    echo 'client - connect to started server'
}

case $1 in
    'server') StartOpenOCDServer;;
    'client') StartOpenOCDClient;;
    *       ) PrintHelp;;
esac
