#!/bin/sh

status() {
	xdslctl info --show > /tmp/xdslinfo
	xdslStatus=`cat /tmp/xdslinfo | grep ^Status`
	if [ "$xdslStatus" = "" ]
	then
		echo "dsl.line_state='DOWN'"
		echo "dsl.line_state_detail='Connection not up'"
	else
		line_state=`echo $xdslStatus | awk -F ': ' '{print $2}'`
		echo "dsl.line_state_detail='$line_state'"
		if [ "$line_state" = "Showtime" ]
		then
			lineMode=`cat /tmp/xdslinfo | grep 'Mode:' | awk '{print $2}'`
			profile=`cat /tmp/xdslinfo | grep 'Profile:' | awk '{print $4}'`
			annex=`cat /tmp/xdslinfo | grep 'Annex' | awk '{print $4}'`
			xdslRateDown=`cat /tmp/xdslinfo | grep '0, Upstream rate' | awk '{print $11}'`
			xdslRateUp=`cat /tmp/xdslinfo | grep '0, Upstream rate' | awk '{print $6}'`
			maxRateDown=`cat /tmp/xdslinfo | grep 'Max:' | awk '{print $10}'`
			maxRateUp=`cat /tmp/xdslinfo | grep 'Max:' | awk '{print $5}'`
			delayDown=`cat /tmp/xdslinfo | grep 'delay:' | head -n 1 | awk '{print $2}'`
			delayUp=`cat /tmp/xdslinfo | grep 'delay:' | head -n 1 | awk '{print $3}'`
			attnDown=`cat /tmp/xdslinfo | grep 'Attn' | awk '{print $2}'`
			attnUp=`cat /tmp/xdslinfo | grep 'Attn' | awk '{print $3}'`
			snrDown=`cat /tmp/xdslinfo | grep 'SNR' | awk '{print $3}'`
			snrUp=`cat /tmp/xdslinfo | grep 'SNR' | awk '{print $4}'`
			pwrDown=`cat /tmp/xdslinfo | grep 'Pwr' | awk '{print $2}'`
			pwrUp=`cat /tmp/xdslinfo | grep 'Pwr' | awk '{print $3}'`
			esNear=`cat /tmp/xdslinfo | grep ^ES | awk '{print $2}'`
			esFar=`cat /tmp/xdslinfo | grep ^ES | awk '{print $3}'`
			sesNear=`cat /tmp/xdslinfo  |grep ^SES | awk '{print $2}'`
			sesFar=`cat /tmp/xdslinfo | grep ^SES | awk '{print $3}'`
			uasNear=`cat /tmp/xdslinfo | grep ^UAS | awk '{print $2}'`
			uasFar=`cat /tmp/xdslinfo | grep ^UAS | awk '{print $3}'`
			hecNear=`cat /tmp/xdslinfo | grep ^HEC | head -n 1 | awk '{print $2}'`
			hecFar=`cat /tmp/xdslinfo | grep ^HEC | head -n 1 | awk '{print $3}'`
			uptime=`xdslctl info --stats | grep 'Since Link time' | awk -F '=' '{print $2}'`

			echo "dsl.line_state='UP'"
			echo "dsl.line_uptime_s='$uptime'"
			echo "dsl.line_mode_s='$lineMode'"
			echo "dsl.profile_s='$profile'"
			echo "dsl.annex_s='$annex'"
			echo "dsl.data_rate_down_s='$xdslRateDown Kbps'"
			echo "dsl.data_rate_up_s='$xdslRateUp Kbps'"
			echo "dsl.max_data_rate_down_s='$maxRateDown Kbps'"
			echo "dsl.max_data_rate_up_s='$maxRateUp Kbps'"
			echo "dsl.latency_num_down='$delayDown ms'"
			echo "dsl.latency_num_up='$delayUp ms'"
			echo "dsl.line_attenuation_down='$attnDown'"
			echo "dsl.line_attenuation_up='$attnUp'"
			echo "dsl.noise_margin_down='$snrDown'"
			echo "dsl.noise_margin_up='$snrUp'"
			echo "dsl.actatp_down='$pwrDown'"
			echo "dsl.actatp_up='$pwrUp'"
			echo "dsl.errors_es_near='$esNear'"
			echo "dsl.errors_es_far='$esFar'"
			echo "dsl.errors_ses_near='$sesNear'"
			echo "dsl.errors_ses_far='$sesFar'"
			echo "dsl.errors_uas_near='$uasNear'"
			echo "dsl.errors_uas_far='$uasFar'"
			echo "dsl.errors_hec_near='$hecNear'"
			echo "dsl.errors_hec_far='$hecFar'"
		else
			echo "dsl.line_state='DOWN'"
		fi
	fi
}

lucistat() {
	echo "local dsl={}"
	status
	echo "return dsl"
}
