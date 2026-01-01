/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_OPTICAL_1


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_lan.h"
#include "rut_qos.h"

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#include "rut_eponwan.h"
#endif

#include "rdpa_types.h"
#include "rdpactl_api.h"

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
#include "rut_rpsrfs.h"
#endif

#if defined(EPON_HGU) || defined(GPON_HGU)
static CmsRet reconfigure_queues(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;

    if (enabled)
    {
        cmsLog_debug("Enabling TM for interface %s", ifname);

        if ((ret = rutQos_tmPortUninit(ifname, TRUE)) != CMSRET_SUCCESS)
        {
            cmsLog_error("rutQos_tmPortUninit failed, ret=%d", ret);
            goto Exit;
        }

        if ((ret = rutQos_tmPortInit(ifname, TRUE)) != CMSRET_SUCCESS)
        {
            cmsLog_error("rutQos_tmPortInit failed, ret=%d", ret);
            goto Exit;
        }

#ifdef DMP_DEVICE2_QOS_1
        rutQos_reconfigAllQueuesOnLayer2Intf_dev2(ifname);
#endif
    }
    else
    {
        cmsLog_debug("Disabling TM for interface %s", ifname);

        if ((ret = rutQos_tmPortUninit(ifname, TRUE)) != CMSRET_SUCCESS)
            goto Exit;
    }

Exit:
    return ret;
}
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
static CmsRet handle_epon_change(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;  
    
#ifdef EPON_HGU
#define veipStr "veip"
    if (cmsUtl_strncmp(ifname, veipStr, cmsUtl_strlen(veipStr)) == 0) 
    {
        ret = reconfigure_queues(ifname, enabled);        
    }
    else        
#endif
    { 
        if (enabled)
        {
            cmsLog_debug("Enabling EPON interface %s", ifname);
            rutLan_enableInterface(ifname);
        }
        else
        {
            cmsLog_debug("Disabling EPON interface %s", ifname);
            rutLan_disableInterface(ifname);
        }
    } 

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
    if (enabled)
    {
        char epon_port_name[CMS_IFNAME_LENGTH];
        if (rdpactl_has_rdpa_port_type_epon(epon_port_name))
        {
            rutRpsRfs_checkSet(epon_port_name);
        }
        else
        {
            cmsLog_error("Could not find epon port");
        }
    }
#endif

    return ret;
} 
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
static CmsRet handle_gpon_change(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef GPON_HGU
    ret = reconfigure_queues(ifname, enabled);
#endif

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
    if (enabled)
    {
        char gpon_port_name[CMS_IFNAME_LENGTH];
        if (rdpactl_has_rdpa_port_type_gpon(gpon_port_name))
        {
            rutRpsRfs_checkSet(gpon_port_name);
        }
        else
        {
            cmsLog_error("Could not find gpon port");
        }
    }
#endif
    return ret;
}
#endif


CmsRet rcl_deviceOpticalObject( _DeviceOpticalObject *newObj __attribute__((unused)),
    const _DeviceOpticalObject *currObj __attribute__((unused)),
    const InstanceIdStack *iidStack __attribute__((unused)),
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_opticalInterfaceObject( _OpticalInterfaceObject *newObj,
    const _OpticalInterfaceObject *currObj,
    const InstanceIdStack *iidStack,
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 newUp, currUp, isChanged;
    UBOOL8 isPhyDetected;
    UBOOL8 isBackhaul = FALSE;
    rdpa_port_type wan_type = rdpa_port_type_none;
    const char *ifname;
    int rc = 0;

#define IS_UP(n) ((((n) != NULL) && ((n->enable) != NULL) && (!cmsUtl_strcmp(n->status, MDMVS_UP))))

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumOpticalIntf_dev2(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumOpticalIntf_dev2(iidStack, -1);
    }

    newUp = IS_UP(newObj);
    currUp = IS_UP(currObj);
    isChanged = (newUp != currUp);
    isPhyDetected = (newObj && (newObj->enable)
        && (!cmsUtl_strcmp(newObj->status, MDMVS_DORMANT)));

    if (isPhyDetected)
    {
        isBackhaul = (rut_tmctl_getQueueOwner() == TMCTL_OWNER_BH);
        rdpaCtl_set_sys_car_mode(!isBackhaul);
        cmsLog_debug("PHY detetced, reconfigure car_mode with isBackhaul %s", isBackhaul?"TRUE":"FALSE");
    }

    if (!isChanged)
    {
        cmsLog_debug("WAN Optical interafce status did not change");
        goto Exit;
    }

    ifname = newUp ? newObj->name : currObj->name;
    if (ifname)
        wan_type = rdpactl_get_port_type(ifname);
    switch (wan_type)
    {
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
    case rdpa_port_epon:
    case rdpa_port_xepon:
        ret = handle_epon_change(ifname, newUp);
        break;
#endif
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
    case rdpa_port_gpon:
    case rdpa_port_xgpon:
        ret = handle_gpon_change(ifname, newUp);
        break;
#endif
    default:
        break;
    }

Exit:
    return ret;
}

CmsRet rcl_opticalInterfaceStatsObject( _OpticalInterfaceStatsObject *newObj __attribute__((unused)),
    const _OpticalInterfaceStatsObject *currObj __attribute__((unused)),
    const InstanceIdStack *iidStack __attribute__((unused)),
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

#endif    // DMP_DEVICE2_OPTICAL_1

#endif  /* DMP_DEVICE2_BASELINE_1 */
