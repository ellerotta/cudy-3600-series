#
# Copyright (C) 2020 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm963158
BOARDNAME:=BCM963158
#FEATURES+=fpu
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=963158GW

define Target/Description
	Build firmware image for Broadcom BCM963158 based boards.
endef
