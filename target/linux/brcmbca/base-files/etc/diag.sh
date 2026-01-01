#!/bin/sh

. /lib/functions/leds.sh

get_status_led() {
	status_led="oem:green:status"
}

set_state() {
	get_status_led

	[ -z "$status_led" ] && return

	case "$1" in
	preinit)
		status_led_blink_preinit
		;;
	failsafe)
		status_led_blink_failsafe
		;;
	preinit_regular)
		status_led_blink_preinit_regular
		;;
	upgrade)
		led_blink_upgrade
		;;
	done)
		status_led_on
		;;
	esac
}
