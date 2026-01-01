/***********************************************************************
 *
 * Copyright (c) 2015  Broadcom Corporation
 * All Rights Reserved
 *
 * <:label-BRCM:2015:DUAL/GPL:standard
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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "os_defs.h"

#include "spdsvc_api.h"

/*
 * Macros
 */

//#define CC_SPDSVC_API_DEBUG

#if defined(CC_SPDSVC_API_DEBUG)
#define spdSvcApi_debug(fmt, arg...) printf("%s.%u: " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#else
#define spdSvcApi_debug(fmt, arg...)
#endif

#define spdSvcApi_error(fmt, arg...) printf("ERROR[%s.%u]: " fmt, __FUNCTION__, __LINE__, ##arg)

/*
 * Local functions
 */

static int __sendSpdSvcIoctl(spdsvc_ioctl_t ioctlCmd, spdsvc_ioctl_arg_t *arg_p)
{
    int ret;
    int fd;

    fd = open(SPDSVC_DRV_DEVICE_NAME, O_RDWR);
    if(fd < 0)
    {
        spdSvcApi_error("%s: %s\n", SPDSVC_DRV_DEVICE_NAME, strerror(errno));

        return -EINVAL;
    }

    ret = ioctl(fd, ioctlCmd, (uint32_t)arg_p);
    if(ret)
    {
        spdSvcApi_error("%s\n", strerror(errno));
    }

    close(fd);

    return ret;
}


/*
 * Public functions
 */

int spdsvc_enable(spdsvc_config_t *config_p)
{
    spdsvc_ioctl_arg_t spdsvc;

    spdsvc.config = *config_p;

    spdSvcApi_debug("\n\tSPDSVC API: spdsvc_enable connindex %d \n", spdsvc.config.tr471.connindex);

    return __sendSpdSvcIoctl(SPDSVC_IOCTL_ENABLE, &spdsvc);
}

int spdsvc_getOverhead(spdsvc_config_t *config_p, uint32_t *overhead_p)
{
    spdsvc_ioctl_arg_t spdsvc;
    int ret;

    spdsvc.config = *config_p;
    ret = __sendSpdSvcIoctl(SPDSVC_IOCTL_GET_OVERHEAD, &spdsvc);
    if(ret)
    {
        return ret;
    }

    *overhead_p = spdsvc.overhead;

    return 0;
}

int spdsvc_getResult(spdsvc_config_t *config_p, spdsvc_result_t *result_p)
{
    spdsvc_ioctl_arg_t spdsvc;
    int ret;

    spdsvc.config = *config_p;

    ret = __sendSpdSvcIoctl(SPDSVC_IOCTL_GET_RESULT, &spdsvc);
    if(ret)
    {
        return ret;
    }

    *result_p = spdsvc.result;

    spdSvcApi_debug("\n\tSPDSVC API: running %u, rx_packets %u, rx_bytes %u, tx_packets %u, tx_discards %u\n",
                    spdsvc.result.running, spdsvc.result.rx_packets, spdsvc.result.rx_bytes,
                    spdsvc.result.tx_packets, spdsvc.result.tx_discards);
    return 0;
}

int spdsvc_disable(spdsvc_config_t *config_p)
{
    spdsvc_ioctl_arg_t spdsvc;

    spdsvc.config = *config_p;

    return __sendSpdSvcIoctl(SPDSVC_IOCTL_DISABLE, &spdsvc);
}
