#!/bin/sh

. /etc/init.d/startup_ctl.sh

case "$1" in
	start)
		start_up ubus
		(
			echo "Starting UBUS daemon..."
			trap_err
			ubusd &
			start_done
			exit 0
		)&
		;;

	*)
		echo "$0: unrecognized option $1"
		exit 1
		;;

esac

