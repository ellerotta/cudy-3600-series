#!/bin/bash


for intf in `(cd /sys/class/net ; echo eth*)`
do
  if ifconfig $intf up >/dev/null 2>/dev/null
  then
    if type tmctl 2> /dev/null > /dev/null
    then
        tmctl porttminit --devtype 0 --if $intf --numqueues 8 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 7 --priority 7 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 6 --priority 6 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 5 --priority 5 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 4 --priority 4 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 3 --priority 3 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 2 --priority 2 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 1 --priority 1 --weight 1 --schedmode 1 2>/dev/null
        tmctl setqcfg --devtype 0 --if $intf --qid 0 --priority 0 --weight 1 --schedmode 1 2>/dev/null
    fi
    #disable switch spanning tree
    ethswctl -c hwstp -i $intf -o disable 2> /dev/null
  fi
done

if [ -e /etc/adsl/adsl_phy.bin ]
then
    #dsldiagd&
    adsl start --up
    xdslctl1 connection --up  2> /dev/null
fi

#force eth0 as wan port
ethswctl -c wan -i eth0 -o enable 2> /dev/null
