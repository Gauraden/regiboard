#!/bin/sh

SLAVE_ID=${1:=2}

echo "slave id: $SLAVE_ID"

for id in $(seq 0 11); do ./modbus_test $id $SLAVE_ID; done
