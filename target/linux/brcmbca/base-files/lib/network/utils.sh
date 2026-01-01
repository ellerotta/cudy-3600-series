. /lib/functions.sh

# arg1: ifname, e.g:eth0
get_network_of() {

	find_network() {
		local config="$1"
		local iface="$2"
		local net devsec type ports
		local device="$(uci get network.$config.device)"
		if [ "${device:0:3}" == "br-" ]; then
			devsec="$(uci show network | grep name=.*$device | cut -d'.' -f2)"
			sectype="$(uci -q get network.$devsec)"
			devtype="$(uci -q get network.$devsec.type)"
			if [ "$sectype" == "device" -a "$devtype" == "bridge" ]; then
				ports="$(uci get network.$devsec.ports)"
				for prt in $ports; do
					if [ "$prt" == "$iface" ]; then
						net=$config
						break
					fi
				done
			fi
		else
			for dev in $device; do
				if [ "$dev" == "$iface" ]; then
					net=$config
					break
				fi
			done
		fi
		[ -n "$net" ] && echo $net
	}

	config_load network
	config_foreach find_network interface $1
}

# arg1: ifname, e.g:eth0
get_port_name() {
	local port_order=$(db get hw.board.ethernetPortOrder)
	local port_names=$(db get hw.board.ethernetPortNames)
	local cnt=1
	local idx=1

	# get index of interface name
	for i in $port_order; do
		if [ "$i" == "$1" ]; then
			break;
		fi
		idx=$((idx+1))
	done

	# get port name from index
	for i in $port_names; do
		if [ "$cnt" == "$idx" ]; then
			echo $i
			break;
		fi
		cnt=$((cnt+1))
	done

	# for wifi usage default
	if [ -f /etc/config/wireless ]; then 
		uci show wireless | grep ifname | grep -wq "$1" && echo "WLAN"
	fi
}

# config the ports's wan attribution to switch or runner
config_ethports_wan_attr() {
	scan_interface() {
		local sec_intf="$1"
		local device
		# ignore lo and lan interface
		# if [ "$sec_intf" != "loopback" ] && [ "$sec_intf" != "lan" ]; then
		if [ "${sec_intf:0:3}" == "wan" ] || [ "${sec_intf:0:4}" == "iptv" ] || [ "${sec_intf:0:4}" == "voip" ]; then
				if [ "${sec_intf:0:3}" == "wan" ]; then
					device="$(uci get network.$sec_intf.def_ifname)"
				else
					device="$(uci get network.$sec_intf.ifname)"
				fi
				for p in $device; do
						if [ "${p:0:3}" == "eth" ]; then
								ethswctl -c wan -i ${p:0:4} -o enable 2>/dev/null
						fi
				done
		fi

		if [ "${sec_intf:0:3}" == "lan" ]; then
			local device="$(uci get network.$sec_intf.ifname)"
			# skip WAN:ptm, veip, eth0.100 virtual and alias , LAN:br-lan
			for p in $device; do
				if [ "${p:0:3}" == "eth"  ]; then
					ethswctl -c wan -i $p -o disable 2>/dev/null
				fi
			done
		fi
	}

	config_load network
	config_foreach scan_interface interface
}

# checks whether any of the uplink ports are physically connected
uplink_phy_connected() {
	local wanports="$(db -q get hw.board.ethernetWanPort) $(db -q get hw.board.gponWanPort) $(db -q get hw.board.vdslWanPort)"
	local ret=1

	for port in $wanports; do
		if [ "$(cat /sys/class/net/$port/operstate 2>/dev/null)" == "up" ]; then
			ret=0
			break
		fi
	done

	return $ret
}

