#
# Copyright (C) 2020 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm96858
BOARDNAME:=BCM96858
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=96858GO

define Target/Description
	Build firmware image for Broadcom 6858 chip boards.
endef
