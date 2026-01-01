/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>

#include "bcmnet.h"
#include "rut_util.h"
#include "rut_ethintf.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "ethswctl_api.h"
#include <errno.h>




CmsRet rutEth_setSwitchWanPort(char *ifname __attribute__((unused)), int enabled __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: ifName=%s enabled=%d", ifname, enabled);
   
#ifdef DESKTOP_LINUX
   // fake success for desktop
   return ret;
#else

   UBOOL8 ifUp = FALSE;

   ifUp = rut_checkInterfaceUp(ifname);
   if (ifUp)
   {
      /* bcm_enet_driver_wan_interface_set() causes the enet driver to
       * end up calling enet_set_mac_addr() which will issue warning message
       * "Setting MAC address of <ifname> while it is ifconfig UP."
       * Therefore, we want to set interface down before calling
       * bcm_enet_driver_wan_interface_set().
       */
      rut_setIfState(ifname, FALSE);
   }

   if(!isIntf_USB(ifname))
   {
      if (bcm_enet_driver_wan_interface_set(ifname, enabled) < 0)
      {
         cmsLog_error("bcm_enet_driver_wan_interface_set(%s, %d) returns error! (%d)",
                   ifname, enabled, errno);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   if (ifUp)
   {
      /* interface was up, so set it back up. */
      rut_setIfState(ifname, TRUE);
   }

   return ret;
#endif  /* DESKTOP_LINUX */
}


#ifdef DMP_BASELINE_1
#ifdef DMP_ETHERNETWAN_1
UBOOL8 rutEth_isEthWanMode()
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanCommonIntfCfgObject *wanCommon = NULL;
   UBOOL8 found = FALSE;

   while((!found) &&
         (CMSRET_SUCCESS == cmsObj_getNext(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **)&wanCommon)))
   {
      if (cmsUtl_strcmp(wanCommon->WANAccessType, MDMVS_ETHERNET) == 0)
      {
          found = TRUE;
      }         
      cmsObj_free((void **) &wanCommon);
   } /* while loop over WANCommon. */   
   return found;
}


UBOOL8 rutEth_getEthIntfByIfName(char *ifName, InstanceIdStack *iidStack, WanEthIntfObject **ethIntfCfg)
{
   WanEthIntfObject *wanEth=NULL;
   CmsRet ret;
   UBOOL8 found=FALSE;

   cmsLog_debug("ifName=%s", ifName);

   if ((ret = rutWl2_getWanEthObject(iidStack, &wanEth)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get WanEthIntfObject, ret = %d", ret);
   }
   else
   {
      if(cmsUtl_strcmp(wanEth->X_BROADCOM_COM_IfName, ifName))
      {
         /* ifname is not same, skip this object. */
         cmsLog_debug("found WANEth, but ifNames do not match, looking for %s got %s",
                      ifName, wanEth->X_BROADCOM_COM_IfName);

         cmsObj_free((void **)&wanEth);
      }
      else
      {
         found = TRUE;
         if (ethIntfCfg != NULL)
         {
            /* give object back to caller, so don't free */
            *ethIntfCfg = wanEth;
         }
         else
         {
            cmsObj_free((void **)&wanEth);
         }
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}

CmsRet rutEth_addWanInterfaceObject_igd(const char *ifName)
{
    InstanceIdStack iidStack;
    LanEthIntfObject *lanEthObj  = NULL;
    WanEthIntfObject *wanEthObj = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    int found = FALSE;
    
    /* get LAN Ethernet interfaces */
    INIT_INSTANCE_ID_STACK(&iidStack);
    while (!found && cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, (void**)&lanEthObj)
        == CMSRET_SUCCESS)
    {
        if (!cmsUtl_strcmp(ifName, lanEthObj->X_BROADCOM_COM_IfName))
        {
            found = TRUE;
        }
        cmsObj_free((void**)&lanEthObj);
    }

    if (found)
    {
        cmsLog_debug("found LAN ifName=%s", ifName);
        if ((ret = rutWan_moveEthLanToWan_igd(ifName)) != CMSRET_SUCCESS)
            return ret;
    }

    INIT_INSTANCE_ID_STACK(&iidStack);
    if ((ret = rutWl2_getWanEthObject(&iidStack, &wanEthObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get WanEthIntfObject, ret=%d", ret);
        goto exit;
    }
    /* Check if the object has been enabled, enable it once */
    if (!wanEthObj->enable)
    {
        wanEthObj->enable = TRUE;

        wanEthObj->X_BROADCOM_COM_PersistentDevice = TRUE;
        CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->duplexMode, MDMVS_AUTO, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->maxBitRate, MDMVS_AUTO, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);
        cmsObj_set(wanEthObj, &iidStack);

        /*
        * Also create a single WANConnectionDevice in this WANDevice.
        */
        if ((ret = cmsObj_addInstance(MDMOID_WAN_CONN_DEVICE, &iidStack)) != CMSRET_SUCCESS)
        {
            cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
        }
    }

exit:
    cmsObj_free((void **) &wanEthObj);    
    return ret;
}

#endif  /* DMP_ETHERNETWAN_1 */
#endif  /* DMP_BASELINE_1 */
