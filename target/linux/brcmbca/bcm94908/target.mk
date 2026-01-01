#
# Copyright (C) 2019 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm94908
BOARDNAME:=BCM94908
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=962118GW

define Target/Description
	Build firmware image for Broadcom 4908 or 62118 chip boards.
endef
