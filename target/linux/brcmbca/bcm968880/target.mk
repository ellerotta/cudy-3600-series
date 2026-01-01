#
# Copyright (C) 2019 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm968880
BOARDNAME:=BCM968880
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=968880GWO

define Target/Description
	Build firmware image for Broadcom 68880 chip board.
endef
