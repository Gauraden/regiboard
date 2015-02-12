#!/bin/bash

function Parse {
  local res=$(echo "$1" | grep -P "$2")
  test "$res" != "" && echo "$res"
}

while read line; do
  Parse "$line" '(.+): ok'
  Parse "$line" 'Loop (.+)'
  Parse "$line" 'FAILURE: (.+)'
done
