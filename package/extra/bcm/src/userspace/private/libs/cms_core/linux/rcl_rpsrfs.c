/*
* <:copyright-BRCM:2023:proprietary:standard
* 
*    Copyright (c) 2023 Broadcom 
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


#ifdef DMP_X_BROADCOM_COM_RPSRFS_1

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_rpsrfs.h"

#define MAX_RPS_SOCK_FLOW_ENTRIES 32768
#define MAX_NETDEV_MAX_BACKLOG 10000
#define DEF_RPS_SOCK_FLOW_ENTRIES 0
#define DEF_NETDEV_MAX_BACKLOG 1000

CmsRet rcl_rpsRfsObject( _RpsRfsObject *newObj,
                const _RpsRfsObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    char cmdStr[BUFLEN_128];

    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %d > /proc/sys/net/core/rps_sock_flow_entries", MAX_RPS_SOCK_FLOW_ENTRIES);
        rut_doSystemAction("rcl_rpsRfsObject", cmdStr);
        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %d > /proc/sys/net/core/netdev_max_backlog", MAX_NETDEV_MAX_BACKLOG);
        rut_doSystemAction("rcl_rpsRfsObject", cmdStr);
        rutRpsRfs_globalSet(TRUE);
    }
    else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %d > /proc/sys/net/core/rps_sock_flow_entries", DEF_RPS_SOCK_FLOW_ENTRIES);
        rut_doSystemAction("rcl_rpsRfsObject", cmdStr);
        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %d > /proc/sys/net/core/netdev_max_backlog", DEF_NETDEV_MAX_BACKLOG);
        rut_doSystemAction("rcl_rpsRfsObject", cmdStr);
        rutRpsRfs_globalSet(FALSE);
    }

    return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_RPSRFS_1 */
