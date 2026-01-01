#
# Copyright (C) 2020 OpenWrt.org
#

#ARCH:=aarch64
SUBTARGET:=bcm96856
BOARDNAME:=BCM96856
CPU_TYPE:=cortex-a53
TARGET_PROFILE:=96856GWO

define Target/Description
	Build firmware image for Broadcom 6856 chip boards.
endef
