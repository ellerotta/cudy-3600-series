/*
* <:copyright-BRCM:2019:proprietary:standard
*
*    Copyright (c) 2019 Broadcom
*    All Rights Reserved
*
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
*
*  Except as expressly set forth in the Authorized License,
*
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
*
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
*
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#if defined(BCM_PON) || defined(CONFIG_BCM963158) || defined(CONFIG_BCM96813) 
#include <rdpa_user.h>
#include <rdpa_system.h>
#include <rdpactl_api.h>
#include <rdpa_system_user_ag.h>
#include <rdpactl_api.h>
#include "bcm_ulog.h"
#include "ponsys_api.h"

int get_rdpa_user_system_resources(rdpa_user_system_resources *info)
{
    bdmf_object_handle system = 0;
    rdpa_system_resources_t system_resources;
    int rc, ret = 0;
    static int cached =
#ifndef DESKTOP_LINUX
                        0;
#else
                        RDPA_DEFAULT_MAX_TCONTS;
#endif

    memset(&system_resources, 0x00, sizeof(rdpa_system_resources_t));

    if (cached)
    {
        info->num_tconts = cached;
    }
    else if ((rc = rdpa_system_get(&system)) || (system == 0))
    {
        bcmuLog_error("rdpa_system_get() failed, rc=%d\n", rc);
        ret = -ENOENT;
    }
    else
    {
        if ((rc = rdpa_system_system_resources_get(system, &system_resources)))
        {
            printf("rdpa_system_system_resources_get failed, rc=%d\n", rc);
            ret = -ENOENT;
        }
        else
        {
            cached = info->num_tconts = system_resources.num_tcont;
        }
        bdmf_put(system);
    }

    return ret;
}

UINT32 get_rdpa_user_num_tconts(void)
{
    static int ret, num_tconts = 0;
    rdpa_user_system_resources system_resources;

    if (!num_tconts)
    {
        if ((ret = get_rdpa_user_system_resources(&system_resources)))
        {
            bcmuLog_error("get_rdpa_user_system_resources() failed, ret=%d, using NumDataTconts=%d\n", ret, num_tconts = RDPA_DEFAULT_MAX_TCONTS);
        }
        else
        {
            num_tconts = system_resources.num_tconts - 1;
        }
    }
    return num_tconts;
}


UINT32 get_rdpa_user_num_ds_queues(void)
{
    bdmf_object_handle system = 0;
    rdpa_qm_cfg_t qm_cfg = {0};
    UINT32 rc = 0;
    UINT32 total_ds_queues_number = 0;
 
    if ((rc = rdpa_system_get(&system)) == 0)
    {
        if ((rc = rdpa_system_qm_cfg_get(system, &qm_cfg)) == 0)
        {
            total_ds_queues_number = qm_cfg.number_of_ds_queues;
        }
        bdmf_put(system);
    }
    return total_ds_queues_number;
}
#endif
