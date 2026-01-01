#
# Copyright (C) 2020 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm94912
BOARDNAME:=BCM94912
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=964912GW

define Target/Description
	Build firmware image for Broadcom 4912 or 4915 based boards.
endef
