/***********************************************************************
 *
 * <:copyright-BRCM:2015:DUAL/GPL:standard
 *
 *    Copyright (c) 2015 Broadcom
 *    All Rights Reserved
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 *
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 *
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 *
 * :>
 *
 ************************************************************************/

#if !defined(CONFIG_BCM963158)

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "rdpa_types.h"
#include "opticaldet.h"
#include "wanconf.h"
#if !defined(PON_WAN_TYPE_AUTO_DETECT)
#include "board.h"
#include "cms_psp.h"
#endif
#include "pmd.h"
#include "laser.h"
#include "trxbus.h"

extern int set_parameters_to_scratchpad(char *wan_type, char *wan_speed);

#if defined(PON_WAN_TYPE_AUTO_DETECT)
#if defined(PMD_JSON_LIB)
extern int load_pmd_calibration_data(pmd_calibration_parameters *calibration_binary);
#endif

static inline int board_populated_with_pmd()
{
    uint32_t is_pmd = 0;
    int LaserDevFd;

    if (!(LaserDevFd = open(LASER_DEV, O_RDWR)))
    {
        printf("failed to open %s\n", LASER_DEV);
        return 0;
    }

    ioctl(LaserDevFd, LASER_IOCTL_GET_DRV_INFO, &is_pmd);
    close(LaserDevFd);
    return is_pmd == BCM_I2C_PON_OPTICS_TYPE_PMD;
}

static int detect_wan_type(wan_type_auto_detect_result *result,
    int is_signal_detect_required)
{
    int fd, ret;
    wan_type_auto_detect_info wan_auto_detect;

    memset(&wan_auto_detect, 0, sizeof(wan_type_auto_detect_info));
    fd = open("/dev/wantypedetect", O_RDWR);
    if (fd < 0)
    {
        wc_log_err("%s: %s\n", "/dev/wantypedetect", strerror(errno));
        return DETECTION_ERROR;
    }
 
#if defined(PMD_JSON_LIB)
    if (board_populated_with_pmd())
    {
        ret = load_pmd_calibration_data(&(wan_auto_detect.pmd_settings.calibration_parameters_from_json));
        if (!ret)
        {
            wan_auto_detect.pmd_settings.is_calibration_file_valid = 1;
        }
    }
#endif
    wan_auto_detect.signal_detect_required = is_signal_detect_required;

    ret = ioctl(fd, OPTICALDET_IOCTL_DETECT, &wan_auto_detect);
    close(fd);

    if (ret)
    {
        wc_log_err("ioctl failed, Errno[%s] ret=%d\n", strerror(errno), ret);
        return DETECTION_ERROR;
    }

    memcpy(result, &(wan_auto_detect.result), sizeof(wan_type_auto_detect_result));
    wc_log("wan type detect result %s:%s\n", result->wan_type, result->wan_rate);
    return DETECTION_OK;
}

#endif

#if defined(PON_WAN_TYPE_AUTO_DETECT)
static int detect_and_set_scratchpad(int signal_detect_required,
    int scratchpad_set_required)
{
    int ret = DETECTION_OK;
    wan_type_auto_detect_result result;

    memset(&result, 0, sizeof(wan_type_auto_detect_result));

    ret = detect_wan_type(&result, signal_detect_required);
    if ((ret == DETECTION_OK) && (scratchpad_set_required))
    {
        set_parameters_to_scratchpad(result.wan_type,
            result.wan_rate);
    }
    return ret;
}
#endif


int try_wan_type_detect_and_set(int signal_detect_required,
    int scratchpad_set_required)
{
    int ret = DETECTION_OK;

#if defined(PON_WAN_TYPE_AUTO_DETECT)
    wc_log("wan type detect signal_detect_required:scratchpad_set_required %d:%d\n",
        signal_detect_required, scratchpad_set_required);

    ret = system("insmod /lib/modules/"KERNELVER"/extra/wantypedet.ko");
    if (ret)
    {
        wc_log_err("Failed to load wantypedet.ko (rc=%d)\n", ret);
        return DETECTION_ERROR;
    }

    ret = detect_and_set_scratchpad(signal_detect_required, scratchpad_set_required);
    if (ret)
        wc_log_err("Failed to detect (ret=%d)\n", ret);

    if(system("rmmod wantypedet.ko") != 0)
        wc_log_err("Failed to unload wantypedet.ko \n");
#endif

    return ret;
}

#endif
