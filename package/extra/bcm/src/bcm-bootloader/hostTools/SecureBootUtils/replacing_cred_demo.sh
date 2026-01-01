#!/bin/sh

HOSTTOOLS=${HOSTTOOLS:=../hostTools}
proceed()
{
	echo "Proceed ? (type Y/N then press Enter)"
	read OK
	[[ "$OK" =~ [Y|y] ]] && return 1 || return 0
}

SRC=$1
DST=$2
CHIP=$3
prompt (){
echo "ERROR: $1 "
echo	"Replacing Credentials openssl Demo:
	- Generates KRSMFG 
	- Generates MFG credentials required for SBI MFG header 
	- Generates FLD credentials required for SBI FLD header; MFG credentials generation can be skipped 
		$0  <your directory> <replacement directory>  - will generate MFG and FLD keys ;
							MFG credetentials from BROADCOM must be placed to <your directory>
	or
		$0 <your directory> KRSAMFG - will generate Krsa-mfg.pem, Krsa-mfg-pub.bin then exit  
"
}
if [  $# -eq 2 ]
then 
	if [[ "$DST" != "KRSAMFG" ]] 
	then
		prompt "Invalid arguments" 
		exit 0
	fi 
	if [ $DST == "KRSAMFG" ]
	then 
		echo "Generating Krsa-mfg and Kaes-mfg "
		proceed
		if [ $? -eq 1 ]
		then
			echo "openssl genrsa -out $SRC/my.Krsa-mfg.pem -F4 2048"
			mkdir -p $SRC
			openssl genrsa -out $SRC/my.Krsa-mfg.pem -F4 2048
			openssl rsa -in $SRC/my.Krsa-mfg.pem -pubout -modulus > $SRC/my.Krsa-mfg-pub.pem
			echo "--- $SRC $HOSTTOOLS"
			cat $SRC/my.Krsa-mfg-pub.pem|$HOSTTOOLS/SecureBootUtils/mod2bin > $SRC/my.Krsa-mfg-pub.bin 
		fi
		exit 0
	fi
elif [ $# -lt 2 ]
then
	prompt "Invalid arguments"
	exit 1
elif [[ ! "$CHIP" =~ 47622|4908|63158|63178|6836|6846|6856|6858|6878 ]]
then
	prompt "Platform is unsupported"
	exit 1
fi 


echo "Running as: $0 $SRC $DST $CHIP"


echo "Step 1:
 Generating MFG Secure credentils"
echo " DID YOU request MFG credentials from BROADCOM?
    If not run this prograrm as $0 <your dir> KRSAMFG
    then sent to Broadcom. When you receive MFG credentials place it here $SRC or
    other convenient to you directory"
proceed
if [ $? -eq 1 ]
then
	echo " Proceeding"
else
	exit 1
fi

echo "

****** Begining to generate MFG additional credentials and ****** 
	replacing demonstration credentials"
echo "... Generating mid+Kaes-mfg.enc and Kaes-mfg-ek/iv pair "
proceed
if [ $? -eq 1 ]
then
	echo " cat $SRC/mid.bin $SRC/my.Kaes-mfg-ek.bin $SRC/my.Kaes-mfg-iv.bin | openssl rsautl -encrypt -pubin -inkey $SRC/Kroe2-mfg-pub.base64 –out $SRC/my.mid+Kaes-mfg.enc"
			#openssl rsautl -encrypt -pubin -inkey $SRC/Kroe2-mfg-pub.base64 –out $SRC/my.mid+Kaes-mfg.enc
	dd if=/dev/random of=$SRC/my.Kaes-mfg-ek.bin bs=1 count=16
	#echo “000000: <16 bytes separated by spaces>” | xxd -r > $SRC/my.Kaes-mfg-ek.bin
	dd if=/dev/random of=$SRC/my.Kaes-mfg-iv.bin bs=1 count=16 
	#echo “000000: <16 bytes separated by spaces>” | xxd -r > $SRC/my.Kaes-mfg-iv.bin
	cat $SRC/mid.bin $SRC/my.Kaes-mfg-ek.bin $SRC/my.Kaes-mfg-iv.bin | openssl rsautl -encrypt -out $SRC/my.mid+Kaes-mfg.enc -pubin -inkey $SRC/Kroe2-mfg-pub.base64
fi


echo "... Begin to copy MFG credentials to your replacement directory : $DST "
proceed

if [ $? -eq 1 ]
then
	cp -vf $SRC/my.Krsa-mfg.pem $DST/Krsa-mfg.pem
	cp -vf $SRC/my.Krsa-mfg-pub.bin $DST/Krsa-mfg-pub.bin
	cp -vf $SRC/my.mid+Kaes-mfg.enc $DST/mid+Kaes-mfg.enc 
	cp -vf $SRC/my.Kaes-mfg-ek.bin $DST/Kaes-mfg-ek.bin
	cp -vf $SRC/my.Kaes-mfg-iv.bin $DST/Kaes-mfg-iv.bin
	cp -vf $SRC/mid.bin $DST/mid.bin
	cp -vf $SRC/Kroe2-mfg-priv.enc $DST/$CHIP/ 
	cp -vf $SRC/Kroe2-mfg-pub.bin  $DST/ 
	cp -vf $SRC/mfgRoeData.sig     $DST/$CHIP/
if [ "$CHIP" == "4908" ]
then 
	cp -vf $SRC/mfgOemData.sig     $DST/
else
	cp -vf $SRC/mfgOemData2.sig    $DST/
fi

fi


echo "

****** Begining to generate FLD credentials  ****** 
	replacing demonstration credentials"



proceed
if [ $? -eq 1 ]
then
	echo "... Generating Krot-fld.pem"
	echo "openssl genrsa -out $SRC/my.Krsa-mfg.pem -F4 2048"
	mkdir -p $SRC
	openssl genrsa -out $SRC/my.Krot-fld.pem -F4 2048
	openssl rsa -in $SRC/my.Krot-fld.pem -pubout -modulus > $SRC/my.Krot-fld-pub.pem
	$HOSTTOOLS/SecureBootUtils/./mod2bin --in $SRC/my.Krot-fld-pub.pem --out $SRC/my.Krot-fld-pub.bin 
	dd if=/dev/random of=$SRC/my.Kroe-fld-ek.bin bs=1 count=16
	dd if=/dev/random of=$SRC/my.Kroe-fld-iv.bin bs=1 count=16
	echo "cat $SRC/mid.bin $SRC/my.Krot-fld-pub.bin|sha256sum|sed s/[^0-9a-fA-F]//|xxd -r -p > $SRC/my.Hmid-rot-fld-pub.bin"
	cat $SRC/mid.bin $SRC/my.Krot-fld-pub.bin|sha256sum
	cat $SRC/mid.bin $SRC/my.Krot-fld-pub.bin|sha256sum|sed s/[^0-9a-fA-F]//|xxd -r -p > $SRC/my.Hmid-rot-fld-pub.bin
	cat $SRC/my.Hmid-rot-fld-pub.bin|hexdump -v -e '/1 "%02x"' 
	echo "
             ... Begin to replace FLD credentials to the replacement directory : $DST "
	proceed
	if [ $? -eq 1 ]
	then
		cp -vf $SRC/mid.bin $DST/mid-fld.bin
		cp -vf $SRC/my.Krot-fld.pem $DST/Krot-fld.pem
		cp -vf $SRC/my.Krot-fld-pub.bin $DST/Krot-fld-pub.bin
		cp -vf $SRC/my.Hmid-rot-fld-pub.bin $DST/Hmid-rot-fld-pub.bin 
		cp -vf $SRC/my.Kroe-fld-ek.bin $DST/Kroe-fld-ek.bin
		cp -vf $SRC/my.Kroe-fld-iv.bin $DST/Kroe-fld-iv.bin
	fi
fi
