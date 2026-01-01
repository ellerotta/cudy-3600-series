#!/bin/sh
#
# Copyright (c) 2014 The Linux Foundation. All rights reserved.
# Copyright (C) 2011 OpenWrt.org
#

BCM67xx_BOARD_NAME=
BCM67xx_MODEL=

get_active_partition() {
	rootfs_mtd=$(sh /etc/get_rootfs_dev.sh | awk -F_ '{print $2}')

	if [ x$rootfs_mtd = x"4" ]; then
		echo "1";
	else
		echo "2";
	fi
}



bcm67xx_board_detect() {
	[ -z "$BCM67xx_BOARD_NAME" ] && BCM67xx_BOARD_NAME="$(strings /proc/device-tree/compatible | head -1)"
	[ -z "$BCM67xx_MODEL" ] && BCM67xx_MODEL="$(cat /proc/device-tree/model | awk -F/ 'BEGIN{OFS="/"}{$2=""}1' | sed 's/\/$//')"
	
	local oem_board_name="$(cat /proc/device-tree/model | awk -F/ '{print $2}')"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"


	
	echo "$BCM67xx_BOARD_NAME" > /tmp/sysinfo/sdk_board_name
	echo "$BCM67xx_MODEL" > /tmp/sysinfo/model

}

bcm67xx_board_name() {
	local name

	[ -f /tmp/sysinfo/sdk_board_name ] && name=$(cat /tmp/sysinfo/sdk_board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}

bcm67xx_oem_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
