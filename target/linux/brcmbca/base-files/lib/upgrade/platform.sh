#
# Copyright (C) 2019 OpenWrt.org
#
# broadcom arm/arm64 kernel image partition name
. /lib/functions.sh
. /lib/upgrade/common.sh
. /lib/bcm67xx.sh

PART_NAME=Kernel

get_full_section_name() {
	local img=$1
	local sec=$2

	dumpimage -l ${img} | grep "^ Image.*(${sec})" | \
		sed 's,^ Image.*(\(.*\)),\1,'
}

image_contains() {
	local img=$1
	local sec=$2
	dumpimage -l ${img} | grep -q "^ Image.*(${sec}.*)" || return 1
}

print_sections() {
	local img=$1

	dumpimage -l ${img} | awk '/^ Image.*(.*)/ { print gensub(/Image .* \((.*)\)/,"\\1", $0) }'
}

image_demux() {
	local img=$1

	for sec in $(print_sections ${img}); do
		local fullname=$(get_full_section_name ${img} ${sec})

		local position=$(dumpimage -l ${img} | grep "(${fullname})" | awk '{print $2}')
		dumpimage -T "flat_dt" -p "${position}" -o /tmp/${fullname}.bin  ${img} > /dev/null || { \
			echo "Error while extracting \"${sec}\" from ${img}"
			return 1
		}
	done
	return 0
}

image_is_FIT() {
	if ! dumpimage -l $1 > /dev/null 2>&1; then
		echo "$1 is not a valid FIT image"
		return 1
	fi
	return 0
}

switch_layout() {
	# Layout switching was required only in ipq806x and is not used in other
	# platforms. Currently making it to return 0 by default.
	# This function and all its references need to be removed during clean
	# up.
	return 0
}

get_inactive_partition() {
	rootfs_mtd=$(sh /etc/get_rootfs_dev.sh | awk -F_ '{print $2}')

	if [ x$rootfs_mtd = x"4" ]; then
		echo "2";
	else
		echo "1";
	fi
}

do_flash_mtd() {
    local bin=$1
    local mtdname=$2
    local append=""

    local mtdpart=$(grep "\"${mtdname}\"" /proc/mtd | awk -F: '{print $1}')
    if [ ! -n "$mtdpart" ]; then
            echo "$mtdname is not available" && return
    fi

    local pgsz=$(cat /sys/class/mtd/${mtdpart}/writesize)

    dd if=/tmp/${bin}.bin bs=${pgsz} conv=sync | mtd $append -e "/dev/${mtdpart}" write - "/dev/${mtdpart}"
}

do_flash_failsafe_ubi_volume() {
	local bin=$1
	local vol_name=$2
	local tmpfile="${bin}.bin"
	local mtdnum;
	local mtdparts;
	local filesize;
	local type;

	filesize=$(stat -c "%s" "/tmp/${tmpfile}")
	mtdnum=$(get_inactive_partition)
	mtdparts=$vol_name$mtdnum;

	volumes=$(ls /sys/class/ubi/ubi0/ | grep ubi._.*)

	for vol in ${volumes}
	do
		[ -f /sys/class/ubi/${vol}/name ] && name=$(cat /sys/class/ubi/${vol}/name);type=$(cat /sys/class/ubi/${vol}/type)
		if [ ${name} == ${mtdparts} ]; then
			ubirmvol /dev/ubi0 -N ${name}
			ubimkvol /dev/ubi0 -N ${name} -n ${vol:5} -s $filesize -t $type
			ubiupdatevol /dev/${vol} /tmp/${tmpfile}
			break;
		fi
	done
}

to_lower ()
{
	echo $1 | awk '{print tolower($0)}'
}

to_upper ()
{
	echo $1 | awk '{print toupper($0)}'
}

image_is_nand()
{
	local nand_part=$(ls /sys/class/ubi/ubi0)
	[ -n "$nand_part" ] && return 1
	return 0;
}

flash_section() {
	local sec=$1
	local board=$(board_name)
	local img_boot_ver="$(get_oem_boot_version)"
	local local_boot_ver="$(oem_boot_version)"

	case "${sec}" in

		bootfs*) switch_layout linux; do_flash_failsafe_ubi_volume ${sec} "bootfs" ;;
		nand_squashfs*) switch_layout linux; do_flash_failsafe_ubi_volume ${sec} "rootfs" ;;
		*) echo "Section ${sec} ignored"; return 1;;
	esac

	echo "Flashed ${sec}"
}


platform_do_upgrade() {
	local board="$(board_name)"

#	for sec in $(print_sections $1); do
#		if [ ! -e /tmp/${sec}.bin ]; then
#			echo "Error: Cant' find ${sec} after switching to ramfs, aborting upgrade!"
#			reboot
#		fi
#	done

	##$ write image file to Kernel parttition directly ###
	case "$board" in
	brcm,bcm963178|\
	brcm,bcm947622|\
	brcm,brcm94908ref|\
	brcm,bcm96756|\
	brcm,bcm96765|\
	brcm,bcm96764|\
	brcm,bcm96766|\
	brcm,bcm96846|\
	brcm,bcm96855|\
	brcm,bcm96878|\
	brcm,brcm-v8A)
		echo "broadcom image upgrade here"
		echo $IMAGE
		for sec in $(print_sections $1); do
			flash_section ${sec}
		done

		sleep 3
		;;
	*)
		echo "platform_do_upgrade here"
		default_do_upgrade "$ARGV"
		;;
	esac

	return 0
}

platform_check_image() {
	local img_file="$1"
	local board=$(cat /tmp/sysinfo/sdk_board_name)
	local oem_board=$(bcm67xx_oem_board_name)

	case "$board" in
	brcm,bcm963178|\
	brcm,bcm947622|\
	brcm,brcm94908ref|\
	brcm,bcm96756|\
	brcm,bcm96765|\
	brcm,bcm96764|\
	brcm,bcm96766|\
	brcm,bcm96846|\
	brcm,bcm96855|\
	brcm,bcm96878|\
	brcm,brcm-v8A)

		;;
	*)
		echo "platform_check_image here."
		;;
	esac

	image_demux $1 || {\
		echo "Error: \"$1\" couldn't be extracted. Abort..."
		return 1
	}

	return 0
}

platform_copy_config() {
	jffs2reset -y
	cp /tmp/sysupgrade.tgz /overlay/
	sync
}

platform_pre_upgrade() {
	echo "platform_pre_upgrade here."
}




