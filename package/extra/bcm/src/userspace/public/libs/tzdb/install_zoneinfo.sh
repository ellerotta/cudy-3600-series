#!/bin/sh
#
##################################################################################
#                                                                                 #
# Copyright (C) 2018, Broadcom. All Rights Reserved.                              #
#                                                                                 #
# Permission to use, copy, modify, and/or distribute this software for any        #
# purpose with or without fee is hereby granted, provided that the above          #
# copyright notice and this permission notice appear in all copies.               #
#                                                                                 #
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES        #
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF                #
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY     #
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES              #
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION    #
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN          #
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.                        #
#                                                                                 #
###################################################################################

SRC=$1
DEST=$2

#echo "source=$SRC destination=$DEST"

mkdir -p $DEST

# copy timezone list into destination folder

cd $SRC
cp --parents "Etc/GMT-12"            "$DEST"
cp --parents "Pacific/Midway"        "$DEST"
cp --parents "US/Hawaii"             "$DEST"
cp --parents "US/Alaska"             "$DEST"
cp --parents "America/Tijuana"       "$DEST"
cp --parents "US/Arizona"            "$DEST"
cp --parents "America/Chihuahua"     "$DEST"
cp --parents "US/Mountain"           "$DEST"
cp --parents "US/Central"            "$DEST"
cp --parents "America/Monterrey"     "$DEST"
cp --parents "Canada/Saskatchewan"   "$DEST"
cp --parents "America/Bogota"        "$DEST"
cp --parents "US/Eastern"            "$DEST"
cp --parents "America/Indianapolis"  "$DEST"
cp --parents "Canada/Atlantic"       "$DEST"
cp --parents "America/Caracas"       "$DEST"
cp --parents "America/Santiago"      "$DEST"
cp --parents "Canada/Newfoundland"   "$DEST"
cp --parents "Brazil/East"           "$DEST"
cp --parents "America/Buenos_Aires"  "$DEST"
cp --parents "America/Godthab"       "$DEST"
cp --parents "America/Noronha"       "$DEST"
cp --parents "Atlantic/Azores"       "$DEST"
cp --parents "Atlantic/Cape_Verde"   "$DEST"
cp --parents "Africa/Casablanca"     "$DEST"
cp --parents "GMT"                   "$DEST"
cp --parents "Europe/Amsterdam"      "$DEST"
cp --parents "Europe/Belgrade"       "$DEST"
cp --parents "Europe/Brussels"       "$DEST"
cp --parents "Europe/Sarajevo"       "$DEST"
cp --parents "Africa/Kinshasa"       "$DEST"
cp --parents "Europe/Athens"         "$DEST"
cp --parents "Europe/Bucharest"      "$DEST"
cp --parents "Africa/Cairo"          "$DEST"
cp --parents "Africa/Harare"         "$DEST"
cp --parents "Europe/Helsinki"       "$DEST"
cp --parents "Asia/Jerusalem"        "$DEST"
cp --parents "Asia/Baghdad"          "$DEST"
cp --parents "Asia/Kuwait"           "$DEST"
cp --parents "Europe/Moscow"         "$DEST"
cp --parents "Africa/Nairobi"        "$DEST"
cp --parents "Asia/Tehran"           "$DEST"
cp --parents "Asia/Muscat"           "$DEST"
cp --parents "Asia/Baku"             "$DEST"
cp --parents "Asia/Kabul"            "$DEST"
cp --parents "Asia/Karachi"          "$DEST"
cp --parents "Asia/Kolkata"          "$DEST"
cp --parents "Asia/Kathmandu"        "$DEST"
cp --parents "Asia/Almaty"           "$DEST"
cp --parents "Asia/Dhaka"            "$DEST"
cp --parents "Asia/Rangoon"          "$DEST"
cp --parents "Asia/Bangkok"          "$DEST"
cp --parents "Asia/Krasnoyarsk"      "$DEST"
cp --parents "Asia/Shanghai"         "$DEST"
cp --parents "Asia/Irkutsk"          "$DEST"
cp --parents "Asia/Kuala_Lumpur"     "$DEST"
cp --parents "Australia/Perth"       "$DEST"
cp --parents "Asia/Taipei"           "$DEST"
cp --parents "Asia/Tokyo"            "$DEST"
cp --parents "Asia/Seoul"            "$DEST"
cp --parents "Asia/Yakutsk"          "$DEST"
cp --parents "Australia/Adelaide"    "$DEST"
cp --parents "Australia/Darwin"      "$DEST"
cp --parents "Australia/Brisbane"    "$DEST"
cp --parents "Australia/Canberra"    "$DEST"
cp --parents "Pacific/Guam"          "$DEST"
cp --parents "Australia/Hobart"      "$DEST"
cp --parents "Asia/Vladivostok"      "$DEST"
cp --parents "Asia/Magadan"          "$DEST"
cp --parents "Pacific/Auckland"      "$DEST"
cp --parents "Pacific/Fiji"          "$DEST"
