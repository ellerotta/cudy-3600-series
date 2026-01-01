/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/
#ifndef DESKTOP_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <errno.h>
#include "maclimit_api.h"

static int maclimit_cmd(bcmnet_info_t *info)
{
    int ret = 0;
    int fd = open(BCMNET_DRV_DEVICE_NAME, O_RDWR);
    
    if (fd < 0)
    {
        fprintf(stderr, "mac limit open failed.\n");
        return fd;
    }

    ret = ioctl(fd, BCMNET_IOCTL_MAC_LIMIT, info);
    if (ret)
    {
        ret = errno;
        fprintf(stderr, "ioctl command return error %d!\n", ret);
    }

    close(fd);
    return ret;
}

static int mac_limit_dev_op(uint32_t cmd_id, uint32_t val, const char *ifname, struct mac_limit *mac_limit)
{
    bcmnet_info_t info;

    if (ifname != NULL)
    {
        strncpy(info.if_name, ifname, sizeof(info.if_name)-1);
        info.if_name[sizeof(info.if_name)-1] = '\0';
    }
    else
        memset(info.if_name, 0, sizeof(info.if_name));

    info.st_mac_limit.cmd = cmd_id;
    info.st_mac_limit.val = val;
    info.st_mac_limit.mac_limit = (void*)mac_limit;

    return maclimit_cmd(&info);
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_get
 * Description  : Get dev's mac limit configuration and counters, mac limit
 * can be configured in any linux net device regardless of its net role
 * Parameters:
 *    ifname (IN) net dev's name
 *    mac_limit (OUT) mac limit structure
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_get(const char *ifname, struct mac_limit *mac_limit)
{
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_GET, 0, ifname, mac_limit);
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_set_max
 * Description  : Set dev's supported max mac learnig entries
 * Parameters:
 *    ifname (IN) net dev's name
 *    max (IN) max entries allowed
 *    zero_drop (IN) max zero value drop or not
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_set_max(const char *ifname, int max, int zero_drop)
{
   struct mac_limit mac_limit;
    
    mac_limit.max = max;
    mac_limit.max_zero_drop = zero_drop;
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_SET, MAC_LIMIT_SET_MAX, ifname, &mac_limit);    
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_set_min
 * Description  : Set dev's commited min mac learnig entries
 * Parameters:
 *    ifname (IN) net dev's name
 *    max (IN) min entries committed
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_set_min(const char *ifname, int min)
{
    struct mac_limit mac_limit;
    
    mac_limit.min = min;
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_SET, MAC_LIMIT_SET_MIN, ifname, &mac_limit);   
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_clr
 * Description  : Clear dev's mac limit configuration&counters
 * Parameters:
 *    ifname (IN) net dev's name
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_clr(const char *ifname)
{
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_CLR, 0, ifname, NULL);
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_disable
 * Description  : mac limit process disabled on dev
 * Parameters:
 *    ifname (IN) net dev's name
 *    if set NULL disable mac limit process globally
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_disable(const char *ifname)
{
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_EN, 0, ifname, NULL); 
}

/* ----------------------------------------------------------------------------
 * Function Name: mac_limit_enable
 * Description  : mac limit process enabled on dev
 * Parameters:
 *    ifname (IN) net dev's name
 *    if set NULL enable mac limit process globally
 *
 * Return: 0 - success, non-0 - error
 * ----------------------------------------------------------------------------
 */
int mac_limit_enable(const char *ifname)
{
    return mac_limit_dev_op(MAC_LIMIT_IOCTL_EN, 1, ifname, NULL);  
}
#endif
