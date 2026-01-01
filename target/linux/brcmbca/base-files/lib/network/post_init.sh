#!/bin/sh

# calling "rtpolicy" to apply RT settings for the kthreads and apps that have already started
[ -e /bin/rtpolicy ] && rtpolicy auto ALL > /var/rtpolicy.log 2>&1

