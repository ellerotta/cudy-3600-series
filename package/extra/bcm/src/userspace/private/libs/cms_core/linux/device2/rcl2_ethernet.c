/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1

#include "cms_core.h"
#include "cms_util.h"
#include "sysutil_net.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_util.h"
#include "rut2_ethernet.h"
#include "rut_ethintf.h"
#include "rut_qos.h"

#ifdef DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1
#include "rut2_ethcablediag.h"
#endif

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
#include "rut_rpsrfs.h"
#endif

/*!\file rcl2_ethernet.c
 * \brief This file contains Device2 ethernet related functions.
 *
 */

CmsRet rcl_dev2EthernetObject( _Dev2EthernetObject *newObj __attribute__((unused)),
                const _Dev2EthernetObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

// This should be moved to rut2_ethernet.c.  For now, this file is the only caller.
static void rutEth_activateEthernet_dev2(_Dev2EthernetInterfaceObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Entered:ifName=%s upstream=%d",
                 newObj->name, newObj->upstream);

   // The second arg to rutEth_setSwitchWanPort() is "enable", but we are
   // passing in "upStream".  It is a bit surprising, but I guess it works.
   // Basically, we are telling the switch to disable wan port status if
   // upStream == FALSE.
   ret = rutEth_setSwitchWanPort(newObj->name, newObj->upstream);

   if (ret != CMSRET_SUCCESS && 
       !rutOptical_getIntfByIfName(newObj->name, NULL, NULL))
   {
      UBOOL8 isPresent = FALSE;
      if (sysUtl_getIfindexByIfname(newObj->name) >= 0)
      {
         isPresent = TRUE;
      }
      /* 
       * Port does not exists, it might be port that will be created by
       * by wanconf, then this function will be called once more.  But we
       * should not have called this function if eth port does not exist yet
       */
      cmsLog_error("setSwitchWanPort failed, ifName=%s upstream=%d isPresent=%d ret=%d",
                   newObj->name, newObj->upstream, isPresent, ret);

      if(isIntf_USB(newObj->name))
          rutLan_enableInterface(newObj->name);

      return;
   }

   /* Prepare all TX queues and port shaper before interface comes up. */
   rutQos_tmPortInit(newObj->name, newObj->upstream);
#ifdef DMP_DEVICE2_QOS_1
   rutQos_reconfigAllQueuesOnLayer2Intf_dev2(newObj->name);
   rutQos_reconfigShaperOnLayer2Intf_dev2(newObj->name);
#endif

   rutLan_enableInterface(newObj->name);

#ifdef SUPPORT_LANVLAN
   if (newObj->upstream == FALSE)
   {
      rutLan_AddDefaultLanVlanInterface(newObj->name);
   }
#endif


#ifdef DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1
   if(!isIntf_USB(newObj->name))
   {
      cmsLog_debug("Set [%s] cable diagonostics auto enabled : %d", newObj->name, newObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled);
      rutEthCableDiag_SetupAutoEnable(newObj->name, newObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled);
   }   
#endif

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
   if (newObj->lowerLayers == NULL)
   {
      rutRpsRfs_checkSet(newObj->name);
   }
#endif

   return;
}

CmsRet rcl_dev2EthernetInterfaceObject( _Dev2EthernetInterfaceObject *newObj,
                const _Dev2EthernetInterfaceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj)
   {
      cmsLog_debug("Entered: newObj->name=%s enable=%d currObj=%p iidStack=%s",
                    newObj->name, newObj->enable, currObj,
                    cmsMdm_dumpIidStack(iidStack));
   }
   else if (currObj)
   {
      cmsLog_debug("Entered: delete currObj->name=%s iidStack=%s",
                    currObj->name, cmsMdm_dumpIidStack(iidStack));
   }

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumEthInterface(iidStack, 1);
   }


   /* enable eth device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if (rutEth_isDynamicActiveEthernet(newObj->name))
      {
         // This is a dynamic active ethernet port controlled by wanConf
         if (sysUtl_getIfindexByIfname(newObj->name) >= 0)
         {
            // This happens on the first time through this code after a restoredefault.
            // When wanconf notifies ssk that the interface exists, ssk will create
            // this object and then enable it.
            cmsLog_debug("First time enable/activate of %s", newObj->name);
            rutEth_activateEthernet_dev2(newObj);
         }
         else
         {
            // On bootup, this object exists and is enabled but the dynamic
            // active ethernet port does not exist yet, so don't activate it yet.
            // wanconf will send a CMS msg to ssk when the interface exists.
            cmsLog_debug("do not activate %s on bootup, wait for wanconf", newObj->name);
         }
      }
      else
      {
          // This is a regular ethernet interface (non-AE, non-dynamic).
          rutEth_activateEthernet_dev2(newObj);
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if (rutEth_isDynamicActiveEthernet(newObj->name))
      {
         if (sysUtl_getIfindexByIfname(newObj->name) >= 0)
         {
            // This happens on the second, third, fourth boots.  The ethernet obj
            // already exists and is enabled, but it was not activated during
            // bootup because the interface does not exist yet.  Later, wanconf
            // tells ssk interface exists, and ssk calls this function, and we
            // get here.
            cmsLog_debug("Activating %s", newObj->name);
            rutEth_activateEthernet_dev2(newObj);
         }
         else
         {
            // Strange, the interface should exist now.
            cmsLog_error("%s does not exist yet!", newObj->name);
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      // For WANONLY ports, need to clear the WAN port setting on it when it's disabled.
      // For non WANONLY ports, the WAN port setting will be re-configured when it's enabled next time
      if (currObj)
      {
         if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY) &&
              currObj->upstream)
         {
            rutEth_setSwitchWanPort(currObj->name, FALSE);
         }       
         rutLan_disableInterface(currObj->name);
      
#ifdef DMP_DEVICE2_QOS_1
         /* For not WANONLY port, need to delete QoS queues when disabling this interface. */
         if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY))
         {
            rutQos_deleteQueues_dev2(currObj->name);
         }
#endif

         // TR98 code also does rutLan_removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName, bridgeIfName);
         // But there is no reason to delete an interface from bridge just because it is disabled, right?
#ifdef SUPPORT_LANVLAN
         if (currObj->name && currObj->upstream == FALSE)
         {
            rutLan_RemoveDefaultLanVlanInterface(currObj->name);
         }
#endif
      }
      
      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumEthInterface(iidStack, -1);
         // Do we have to delete the QoS queues associated with this interface?
      }
   }

#ifdef DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1
   if (newObj->enable && currObj!= NULL)
   {
      if (newObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled != currObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled)
      {
         cmsLog_debug("Set [%s] cable diagonostics auto enabled : %d", newObj->name, newObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled);
         rutEthCableDiag_SetupAutoEnable(newObj->name, newObj->X_BROADCOM_COM_CableDiagnostics_AutoEnabled);
      }
   }
#endif /* DMP_X_BROADCOM_COM_CABLEDIAGNOSTICS_1 */

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2EthernetInterfaceStatsObject( _Dev2EthernetInterfaceStatsObject *newObj __attribute__((unused)),
                const _Dev2EthernetInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_ETHERNETINTERFACE_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
#error "Device2 ethernet interface objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif

#endif  /* DMP_DEVICE2_BASELINE_1 */
