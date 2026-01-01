#!/bin/sh
PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/lighttpd
NAME=lighttpd
DESC="web server"
SCRIPTNAME=/etc/init.d/$NAME

DAEMON_OPTS="-f /etc/lighttpd/lighttpd.conf"

test -x $DAEMON || exit 0

set -e

check_syntax()
{
	$DAEMON -t $DAEMON_OPTS > /dev/null || exit $?
}

case "$1" in
	start)
		check_syntax
		echo "Starting $DESC" $NAME
		$DAEMON $DAEMON_OPTS
		;;
	stop)
		echo "Stopping $DESC" $NAME
		killall lighttpd
		;;
	restart)
		check_syntax
		$0 stop
		$0 start
		;;
	*)
		echo "Usage: $SCRIPTNAME {start|stop|restart}" >&2
		exit 1
		;;
esac

exit 0
