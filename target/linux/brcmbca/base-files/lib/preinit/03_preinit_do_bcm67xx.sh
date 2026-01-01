#!/bin/sh
#
# Copyright (c) 2014 The Linux Foundation. All rights reserved.
#
. /lib/bcm67xx.sh

do_bcm67xx() {

	bcm67xx_board_detect
}

boot_hook_add preinit_main do_bcm67xx
