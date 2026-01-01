#!/bin/sh

. /etc/init.d/startup_ctl.sh

# Check/Wait wlan driver loading done
is_wlDriver_ready()
{
   j=0
   while true; do
      if [ -f /tmp/.brcm_bcacpe_wifi -o -f /tmp/.brcm_skip_bcacpe_wifi ]; then
         break
      elif [ $j -gt 3000 ]; then
         echo "ERR: Waiting for wlan driver ready, timeout!"
         exit 1
      else
         usleep 100000
      fi
      let j+=1
   done
}

# Check if wlan adapter is present
# return: 0: succ(present), 1: (fail)not present
is_wlAdapter_present()
{
   grep -q -e '^\s*wl[0-9]' /proc/net/dev \
      && { echo "found at least one wlan adapter"; return 0; } \
      || { echo "no wlan adapter is found, wifi_md not started."; return 1; }
}

case "$1" in
	start)
		start_up wifi_md
		(
		  trap_err
		  is_wlDriver_ready
		  is_wlAdapter_present || { start_done; exit 0; }
		  echo "Starting wifi_md [stage 2] ..."
		  /bin/wifi_md
		  start_done
		  exit 0
		)&
		;;

	stop)
		echo "Stopping wifi_md..."
		is_wlAdapter_present || exit 0
		/bin/send_cms_msg -r -t 1000 -c wifi 0x1000080D 1005
		exit 0
		;;

	*)
		echo "$0: unrecognized option $1"
		exit 1
		;;

esac

