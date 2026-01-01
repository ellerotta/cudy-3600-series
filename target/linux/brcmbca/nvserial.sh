#!/bin/bash
input=$1
output="$(echo -n $input | cut -d '.' -f 1).nvm"
echo ./nvserial -a -s 4423 -o $output $input
./nvserial -a -s 4423 -o $output $input
