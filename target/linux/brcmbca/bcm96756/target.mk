#
# Copyright (C) 2021 OpenWrt.org
#

SUBTARGET:=bcm96756
BOARDNAME:=BCM96756
#FEATURES+=fpu
CPU_TYPE:=cortex-a7
TARGET_PROFILE:=96756GW

define Target/Description
	Build firmware image for Broadcom BCM96756 based boards.
endef
