# retrive u-boot environments via fw_printenv 

get_board_id() {
	local board_id="$(fw_printenv -n boardid)"
	echo "$board_id"
}

get_base_macaddr() {
	local basemac="$(fw_printenv -n ethaddr | tr ' ' ':' | sed 's/:$//' | tr 'a-z' 'A-Z')"
	echo "$basemac"
}

get_macaddr_maxsize() {
	local maxsize="$(fw_printenv -n nummacaddrs)"
	echo "$maxsize"
}

get_product_name() {
	local prodname="$(fw_printenv -n ProdName)"
	echo "$prodname"
}

get_hardware_version() {
	local hardware_version="$(fw_printenv -n HV)"
	case $hardware_version in
		.[0-9]*) hardware_version="1$hardware_version" ;;
		[0-9]*) ;;
		*) hardware_version="0" ;;
	esac
	echo "$hardware_version"
}

get_serial_number() {
	local serial_number="$(fw_printenv -n SerialNumber)"
	[ -z "$serial_number" ] && serial_number="$(fw_printenv -n serial_number)"
	case $serial_number in
		*[a-z]*|*[A-Z]*|*[0-9]*) ;;
		*) serial_number="0000000000" ;;
	esac
	echo "$serial_number"
}

get_variant() {
	local variant="$(fw_printenv -n Variant)"
	case $variant in
		*[0-9]*) ;;
		*) variant="0" ;;
	esac
	echo "$variant"
}

get_wpa_key() {
	local wpa_key="$(fw_printenv -n WpaKey)"
	[ -z "$wpa_key" ] && wpa_key="$(fw_printenv -n wpa_key)"
	case $wpa_key in
		*[a-z]*|*[A-Z]*|*[0-9]*) wpa_key=$(echo $wpa_key | sed 's/[ \t]*$//') ;;
		*) wpa_key="00000000" ;;
	esac
	echo "$wpa_key"
}

get_production_mode() {
	local production="$(fw_printenv -n Production)"
	echo "$production"
}

get_board_specific_encryption_key() {
	[ -f /proc/device-tree/key_dev_specific_512 ] && cat /proc/device-tree/key_dev_specific_512 2> /dev/null
}
