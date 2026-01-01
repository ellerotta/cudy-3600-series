/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cms_util.h"
#include "cms_phl.h"
#include "cms_lck.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_mgm.h"
#include "os_defs.h"
#include "cms_helper.h"
#include "wlmdm.h"

int get_num_instances(MdmObjectId oid)
{
    InstanceIdStack iidStack;
    CmsRet cret = CMSRET_SUCCESS;
    void *cms_obj;
    int num_instances = 0;
    UBOOL8 cached = FALSE;
    static int num_of_wlan_adapters = -1;

    if ((oid == MDMOID_WLAN_ADAPTER) && (num_of_wlan_adapters != -1))
    {
        cached = TRUE;
    }

    if (!cached)
    {
        if ((cret = cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
        {
            return -1;
        }

        INIT_INSTANCE_ID_STACK(&iidStack);
        while (cret == CMSRET_SUCCESS)
        {
            cret = cmsObj_getNextFlags(oid, &iidStack, OGF_NO_VALUE_UPDATE, &cms_obj);
            if (cret == CMSRET_SUCCESS)
            {
                cmsObj_free((void **)&cms_obj);
                num_instances++;
            }
        }

        cmsLck_releaseLock();
    }

    if (oid == MDMOID_WLAN_ADAPTER)
    {
        if (!cached)
        {
            // cache it
            num_of_wlan_adapters = num_instances;
        }
        else
        {
            // read from cache
            num_instances = num_of_wlan_adapters;
        }
    }

    cmsLog_debug("Got %d instances for oid[%d]", num_instances, oid);
    return num_instances;
}

int get_bssid_num_for_radio(unsigned int radio_index)
{
    int num_bssid = 0;
    InstanceIdStack iidStackParent, iidStack;
    CmsRet cret;
    void *virt_intf_obj = NULL;
    static int local_num_bssid = (-1);
    static int local_radio_index = (-1);

    if((local_radio_index != (-1)) && ((local_radio_index) == (radio_index)))
    {
        return local_num_bssid;
    }

    if ((cret = cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
        return -1;
    }

    INIT_INSTANCE_ID_STACK(&iidStackParent);
    /* LANDevice instance id is fixed at 1 for WLANConfiguration. */
    PUSH_INSTANCE_ID(&iidStackParent, 1);
    PUSH_INSTANCE_ID(&iidStackParent, radio_index + 1);

    INIT_INSTANCE_ID_STACK(&iidStack);

    cret = cmsObj_getNextInSubTreeFlags(MDMOID_WL_VIRT_INTF_CFG, &iidStackParent,
                                   &iidStack, OGF_NO_VALUE_UPDATE, (void **)&virt_intf_obj);
    while (cret == CMSRET_SUCCESS)
    {
        cmsObj_free((void **)&virt_intf_obj);
        num_bssid++;
        cret = cmsObj_getNextInSubTreeFlags(MDMOID_WL_VIRT_INTF_CFG, &iidStackParent,
                                       &iidStack, OGF_NO_VALUE_UPDATE, (void **)&virt_intf_obj);
    }

    /* Backup last one */
    local_radio_index = (int)radio_index;
    local_num_bssid   = num_bssid;

    cmsLck_releaseLock();

    return num_bssid;
}

WlmdmRet get_nvram_index(MdmObjectId oid, const InstanceIdStack *iidStack,
                         int *first_index, int *second_index)
{
    InstanceIdStack local_iidStack;
    unsigned int radio_id, bssid_id;

    assert(first_index);
    assert(second_index);

    *first_index = -1;
    *second_index = -1;

    local_iidStack = *iidStack;
    switch(oid)
    {
        case MDMOID_WLAN_ADAPTER:
        case MDMOID_WL_BASE_CFG:
        case MDMOID_WL_MIMO_CFG:
        case MDMOID_WL_SES_CFG:
            radio_id = POP_INSTANCE_ID(&local_iidStack);
            *first_index = radio_id - 1;
            break;

        default:
            bssid_id = POP_INSTANCE_ID(&local_iidStack);
            radio_id = POP_INSTANCE_ID(&local_iidStack);
            *first_index = radio_id - 1;
            *second_index = bssid_id -1;
            break;
    }
    return WLMDM_OK;
}

static UBOOL8 verify_nvram_index(MdmObjectId oid, int first_index, int second_index)
{
    int max_radio, max_bssid;

    max_radio = get_num_instances(MDMOID_WLAN_ADAPTER);

    switch (oid)
    {
        case MDMOID_WLAN_ADAPTER:
        case MDMOID_WL_BASE_CFG:
        case MDMOID_WL_MIMO_CFG:
        case MDMOID_WL_SES_CFG:
            if ((first_index >= 0) && (first_index < max_radio))
            {
                if (second_index < 0)
                {
                    return TRUE;
                }
            }
            else
            {
                cmsLog_notice("radio index %d not in range [0, %d)!",
                              first_index, max_radio);
            }
            break;

        default:
            if ((first_index >= 0) && (first_index < max_radio))
            {
                max_bssid = get_bssid_num_for_radio(first_index);
                if (max_bssid < 0)
                {
                    cmsLog_error("Failed to get bssid num for radio %d!\n", first_index);
                }
                else
                {
                    if (second_index < max_bssid)
                    {
                        return TRUE;
                    }
                    else
                    {
                        cmsLog_notice("bssid index %d exceeds maximum allowed %d for radio %d!",
                                       second_index, max_bssid, first_index);
                    }
                }
            }
            break;
    }
    return FALSE;
}

WlmdmRet get_iidStack(MdmObjectId oid, int radio_index, int bssid_index, InstanceIdStack *iidStack)
{
    WlmdmRet ret = WLMDM_OK;

    assert(iidStack);

    if (FALSE == verify_nvram_index(oid, radio_index, bssid_index))
    {
        return WLMDM_NOT_FOUND;
    }

    INIT_INSTANCE_ID_STACK(iidStack);

    /* LANDevice instance id is fixed at 1 for WLANConfiguration. */
    PUSH_INSTANCE_ID(iidStack, 1);
    switch (oid)
    {
        case MDMOID_WLAN_ADAPTER:
        case MDMOID_WL_BASE_CFG:
        case MDMOID_WL_MIMO_CFG:
        case MDMOID_WL_SES_CFG:
            PUSH_INSTANCE_ID(iidStack, radio_index + 1);
            break;

        default:
            bssid_index = (bssid_index < 0) ? 0 : bssid_index;
            PUSH_INSTANCE_ID(iidStack, radio_index + 1);
            PUSH_INSTANCE_ID(iidStack, bssid_index + 1);
            break;
    }
    return ret;
}

void get_wlnvram_pathDesc(MdmPathDescriptor *pathDesc)
{
    assert(pathDesc);
    pathDesc->oid = MDMOID_WLAN_NVRAM;
    INIT_INSTANCE_ID_STACK(&(pathDesc->iidStack));
    strncpy((char *)&pathDesc->paramName, "WlanNvram", sizeof(pathDesc->paramName));
}

void get_wlUnsetNvram_pathDesc(MdmPathDescriptor *pathDesc)
{
    assert(pathDesc);
    pathDesc->oid = MDMOID_WLAN_NVRAM;
    INIT_INSTANCE_ID_STACK(&(pathDesc->iidStack));
    strncpy((char *)&pathDesc->paramName, "WlanUnsetNvram", sizeof(pathDesc->paramName));
}
