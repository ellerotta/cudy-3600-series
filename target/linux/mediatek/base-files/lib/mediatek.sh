#!/bin/sh
#
# Copyright (C) 2010-2013 OpenWrt.org
#

MEDIATEK_BOARD_NAME=
MEDIATEK_MODEL=

mediatek_board_detect() {
	[ -z "$MEDIATEK_BOARD_NAME" ] && MEDIATEK_BOARD_NAME="$(cat /proc/device-tree/model | awk -F/ '{print $1}')"
	[ -z "$MEDIATEK_MODEL" ] && MEDIATEK_MODEL="$(cat /proc/device-tree/model)"
	
	local oem_board_name="$(cat /proc/device-tree/model | awk -F/ '{print $2}')"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	oem_boot_version_dump
	
	echo "$MEDIATEK_BOARD_NAME" > /tmp/sysinfo/sdk_board_name
	echo "$MEDIATEK_MODEL" > /tmp/sysinfo/model
	echo "$oem_board_name" > /tmp/sysinfo/board_name
}

mediatek_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
