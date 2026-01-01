#
# Copyright (C) 2019 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm96813
BOARDNAME:=BCM96813
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=96813GW

define Target/Description
	Build firmware image for Broadcom 6813 chip board.
endef
