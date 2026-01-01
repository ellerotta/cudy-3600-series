#!/bin/sh

export PATH=/bin/docker:$PATH
if [ -n "$1" ]; then
	DATA_ROOT="$1"
else
	DATA_ROOT=_set_by_docker_makefile_root_
fi

dockerd --storage-driver=_set_by_docker_makefile_drv_ --bip=_set_by_docker_makefile_ip_ --data-root=$DATA_ROOT &

exit 0

