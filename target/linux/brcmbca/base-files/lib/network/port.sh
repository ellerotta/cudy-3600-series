# arg1: port ifname, e.g: eth0
get_max_port_speed() {
	if [ -z "$1" ]; then
		echo 0
		return
	fi

	local ifname="$1"
	local phycap="$(ethctl $ifname media-type 2>/dev/null | grep 'PHY Speed Capabilities\|Serdes Speed Capabilities' | awk '{print$NF}' | cut -d':' -f1)"
	local speed=1000

	case "$phycap" in
		10GFD) speed=10000 ;;
		5GFD) speed=5000 ;;
		2.5GFD) speed=2500 ;;
		1GFD|1GHD) speed=1000 ;;
		100MFD|100MHD) speed=100 ;;
		10MFD|10MHD) speed=10 ;;
	esac

	echo $speed
}

# arg1: port name, e.g: eth0
get_port_number() {
	[ -z "$1" ] && return
	local ports="0 1 2 3 4 5 6 7"
	local units="0 1"
	local port="$1"
	local ifname

	for unit in $units; do
		for prt in $ports; do
			ifname="$(ethswctl getifname $unit $prt 2>/dev/null | awk '{print$NF}')"
			if [ "$ifname" == "$port" ]; then
				echo "$unit $prt"
				return
			fi
		done
	done
}

# arg1: port name, e.g: eth0
get_lport_number() {
	[ -z "$1" ] && return
	local ports="0 1 2 3 4 5 6 7"
	local units="0 1"
	local port="$1"
	local ifname
	local lport

	for unit in $units; do
		for prt in $ports; do
			ifname="$(ethswctl getifname $unit $prt 2>/dev/null | awk '{print$NF}')"
			if [ "$ifname" == "$port" ]; then
				lport=$(expr $unit \* 8 + $prt)
				echo "$lport"
				return
			fi
		done
	done
}

# arg1: port number, e.g: 0
get_port_ifname() {
	[ -z "$1" ] && return
	local ports=$(expr $1 % 8)
	local units="0 1"
	local ifname

	for unit in $units; do
		for prt in $ports; do
			ifname="$(ethswctl getifname $unit $prt 2>/dev/null | awk '{print$NF}')"
			if [ "${ifname:0:3}" == "eth" ]; then
				echo "$ifname"
				return
			fi
		done
	done
}

# arg1: port ifname, e.g: eth0
# arg2: port enabled, e.g: 1
power_updown() {
	local ifname="$1"
	local enabled=$2

	local updown="up"
	[ $enabled -eq 0 ] && updown="down"
	ethctl $ifname phy-power $updown >/dev/null
}

# arg1: port ifname, e.g: eth0
# arg2: port enabled, e.g: 1
# arg3: port speed, e.g: 1000
# arg4: port duplex, e.g: full
# arg5: port autoneg, e.g: on
# arg6: port eee, e.g: 0
# arg7: port pause, e.g: 0
set_port_settings() {
	local ifname="$1"
	local enabled=$2
	local speed="$3"
	local duplex=$4
	local autoneg=$5
	local eee=$6
	local pause=$7

	[ -d /sys/class/net/$ifname ] || return

	local unitport="$(get_port_number $ifname)"
	local unit=$(echo $unitport | cut -d ' ' -f 1)
	local port=$(echo $unitport | cut -d ' ' -f 2)

	[ $autoneg -eq 1 ] && autoneg="on" || autoneg="off"
	[ "$duplex" == "half" ] && duplex=0 || duplex=1
	[ "$duplex" == 0 ] && dplx="HD" || dplx="FD"
	[ "$autoneg" == "on" ] && media_type="auto" || media_type="$speed$dplx"

	phycrossbar="$(ethctl $ifname phy-crossbar | head -1)"
	crossbartype="$(echo $phycrossbar | awk '{print$2$3}')"
	# Take only the last PHY Endpoint (non-Serdes) into account as Serdes port number precedes
	[ "$crossbartype" == "oncrossbar" ] && pyhendpoint="$(echo $phycrossbar | awk '{print$NF}')"

	phycaps="$(ethctl $ifname media-type ${pyhendpoint:+ port $pyhendpoint} | awk -F'PHY Capabilities: ' '{print$2}')"
	numofcaps="$(echo $phycaps | tr '|' ' ' | wc -w)"

	if [ "$numofcaps" == "1" ]; then
		logger -t "port-management" "$ifname is capable of $phycaps only; not setting speed/duplex"
	else
		logger -t "port-management" "$ifname is capable of $phycaps; setting speed/duplex to $media_type"
		ethctl $ifname media-type $media_type ${pyhendpoint:+ port $pyhendpoint} &>/dev/null
	fi

	[ $eee -eq 1 ] && eee="on" || eee="off"
	ethtool --set-eee $ifname eee $eee 2>/dev/null

	case $pause in
		off|0)
			pause=0x0
			auto=off
			rx=off
			tx=off
		;;
		on|1)
			pause=0x2
			auto=off
			rx=on
			tx=on
		;;
		auto)
			pause=0x1
			auto=on
			rx=on
			tx=on
		;;
		tx)
			pause=0x3
			auto=off
			rx=off
			tx=on
		;;
		rx)
			pause=0x4
			auto=off
			rx=on
			tx=off
		;;
	esac
	if [ "$auto" == "on" ]; then
		# use ethswctl utility to set pause autoneg
		# as ethtool is not setting it properly
		ethswctl -c pause -n $unit -p $port -v $pause 2>&1 >/dev/null
	else
		ethtool --pause $ifname autoneg $auto rx $rx tx $tx 2>/dev/null
	fi

	power_updown $ifname $enabled
}
