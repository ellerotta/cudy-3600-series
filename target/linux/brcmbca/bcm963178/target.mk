#
# Copyright (C) 2019 OpenWrt.org
#

SUBTARGET:=bcm963178
BOARDNAME:=BCM963178
#FEATURES+=fpu
CPU_TYPE:=cortex-a7
TARGET_PROFILE:=963178GW

define Target/Description
	Build firmware image for Broadcom BCM963178 based boards.
endef
