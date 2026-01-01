#
# Copyright (C) 2019 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm96765
BOARDNAME:=BCM96765
#FEATURES+=fpu
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=96765GW

define Target/Description
	Build firmware image for Broadcom 6765 WiFi7 chip board.
endef
