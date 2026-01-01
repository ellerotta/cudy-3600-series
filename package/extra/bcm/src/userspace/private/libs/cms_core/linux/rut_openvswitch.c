/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom 
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
 * 
 *  This program is the proprietary software of Broadcom  and/or its
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
 *
 ************************************************************************/
#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)

#include <sys/utsname.h>
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "rut_pmap.h"
#include "mdm.h"
#include "rut_wan.h"
#include "rut_openvswitch.h"


#ifdef DMP_DEVICE2_BRIDGE_1
extern CmsRet rutBridge_addLanIntfToBridge_dev2(const char *intfName,
   const char *brIntfName);
extern void rutBridge_deleteIntfNameFromBridge_dev2(const char *intfName);
extern CmsRet rutBridge_updateSSIDBridgeName(const char *intfName, const char *brIntfName);
#endif

UBOOL8 rutOpenVS_isEnabled_igd(void)
{
   UBOOL8 openVSEnable = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _OpenvswitchCfgObject *openVSCfg=NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
   }
   else
   {
      openVSEnable = openVSCfg->enable;
      cmsObj_free((void **) &openVSCfg);
   }

   cmsLog_debug("openvswitch enable cfg = %d", openVSEnable);
   return openVSEnable;
}

UBOOL8 rutOpenVS_isRunning_igd(void)
{
   UBOOL8 isRunning = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _OpenvswitchCfgObject *openVSCfg = NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
   }
   else
   {
      if (0 == cmsUtl_strcmp(openVSCfg->status, MDMVS_ENABLED))
      {
          isRunning = TRUE;
      }
      cmsObj_free((void **) &openVSCfg);
   }
   cmsLog_debug("openvswitch status running = %d", isRunning);
   return isRunning;
}

UBOOL8 rutOpenVS_isOpenVSPorts(const char *ifName)
{
   UBOOL8 isOpenVSPort = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _OpenvswitchCfgObject *openVSCfg=NULL;
   char  *ptr,*currIfName,*tmpIfNameList = NULL, *savePtr=NULL;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d",ret );
   }
   else
   {
      if ( openVSCfg->ifNameList == NULL)
      {
         cmsObj_free((void **) &openVSCfg);
         return FALSE;
      }
      tmpIfNameList = cmsMem_strdup(openVSCfg->ifNameList);
      if (!tmpIfNameList)
      {
         cmsLog_error("Memory allocation failure");
         cmsObj_free((void **) &openVSCfg);
         return FALSE;
      }
      ptr = strtok_r(tmpIfNameList, ",", &savePtr);
      /* check those interface deleted */
      while (ptr )
      {
         currIfName=ptr;
         while ((isspace(*currIfName)) && (*currIfName != 0))
         {
            /* skip white space after comma */
            currIfName++;
         }
         /*interfaces is deleted from ports list*/
         if( strcmp(currIfName, ifName) == 0)
         {
            isOpenVSPort = TRUE;
            break;
         }
         ptr = strtok_r(NULL, ",", &savePtr);
      }
      cmsMem_free(tmpIfNameList);
      cmsObj_free((void **) &openVSCfg);
   }
   return isOpenVSPort;
}

UBOOL8 rutOpenVS_isEnabled_dev2(void)
{
   UBOOL8 openVSEnable = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2OpenvswitchCfgObject *openVSCfg=NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_DEV2_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
   }
   else
   {
      openVSEnable = openVSCfg->enable;
      cmsObj_free((void **) &openVSCfg);
   }

   cmsLog_debug("openvswitch enable cfg = %d", openVSEnable);
   return openVSEnable;
}

UBOOL8 rutOpenVS_isRunning_dev2(void)
{
   UBOOL8 isRunning = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2OpenvswitchCfgObject *openVSCfg = NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_DEV2_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
   }
   else
   {
      if (0 == cmsUtl_strcmp(openVSCfg->status, MDMVS_ENABLED))
      {
          isRunning = TRUE;
      }
      cmsObj_free((void **) &openVSCfg);
   }
   cmsLog_debug("openvswitch status running = %d", isRunning);
   return isRunning;
}

/*TODO insert modules vi rut_iptables.c instead of doing it directly here
 * this may break some dependecies 
*/
CmsRet rutOpenVS_startOpenvswitch(void)
{
   char cmdStr[CMS_MAX_FULLPATH_LENGTH];
   struct utsname kernel;
	
   if (uname(&kernel) == -1) 
   {
      cmsLog_error("Failed to get kernel version");
      return CMSRET_INTERNAL_ERROR;
   }

   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko > /dev/null 2>&1", kernel.release);	 
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv6/netfilter/nf_defrag_ipv6.ko > /dev/null 2>&1", kernel.release);	 
   rut_doSystemAction("rut", cmdStr);

   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/nf_conntrack.ko nf_conntrack_helper=1 > /dev/null 2>&1", kernel.release);	
   rut_doSystemAction("rut", cmdStr);

   rut_doSystemAction("rut", "echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal");

   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/xt_conntrack.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
  
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/nf_nat.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
 
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_nat_ipv4.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv6/netfilter/nf_nat_ipv6.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/gre.ko > /dev/null 2>&1", kernel.release);
   rut_doSystemAction("rut", cmdStr);
   rut_doSystemAction("rut", OVS_START_SCRIPT);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_stopOpenvswitch(void)
{
   rut_doSystemAction("rut", OVS_STOP_SCRIPT);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_updateOpenvswitchOFController(const char *ofControllerAddr, UINT32 ofControllerPort)
{
   char cmdLine[BUFLEN_128];

   snprintf(cmdLine, sizeof(cmdLine), "%s set-controller %s tcp:%s:%d", OVS_VSCTL,OVS_BRIDGE,ofControllerAddr,ofControllerPort);
   rut_doSystemAction("rut", cmdLine);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_updateOpenvswitchPorts(const char *oldOpenVSports, const char *newOpenVSports)
{
   char *tmpOldPortsList=NULL, *tmpNewPortsList=NULL;
   char *currIfName, *ptr, *saveOldPortsPtr=NULL, *saveNewPortsPtr=NULL;

   if (oldOpenVSports == NULL || newOpenVSports == NULL)
   {
      cmsLog_error("Invalid parameters! old=%p, new=%p", oldOpenVSports, newOpenVSports);
      return CMSRET_INVALID_ARGUMENTS;
   }

   tmpOldPortsList = cmsMem_strdup(oldOpenVSports);
   tmpNewPortsList = cmsMem_strdup(newOpenVSports);

   if (!tmpOldPortsList || !tmpNewPortsList)
   {
      cmsLog_error("Memory allocation failure");
      cmsMem_free(tmpOldPortsList);
      cmsMem_free(tmpNewPortsList);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   ptr = strtok_r(tmpOldPortsList, ",", &saveOldPortsPtr);
   /* check those interface deleted */
   while (ptr )
   {
      currIfName=ptr;
      while ((isspace(*currIfName)) && (*currIfName != 0))
      {
         /* skip white space after comma */
         currIfName++;
      }
      /*interfaces is deleted from ports list*/
      if (cmsUtl_findInList(newOpenVSports, currIfName) == NULL)
      {
         rutOpenVS_deleteOpenVSport(currIfName);
      }
      ptr = strtok_r(NULL, ",", &saveOldPortsPtr);
   }
   cmsMem_free(tmpOldPortsList);
   ptr = strtok_r(tmpNewPortsList, ",", &saveNewPortsPtr);
   /* check those interface new added */
   while (ptr )
   {
      currIfName=ptr;
      while ((isspace(*currIfName)) && (*currIfName != 0))
      {
         /* skip white space after comma */
         currIfName++;
      }

      /*interfaces is newly added into openvswitch ports list*/
      if (cmsUtl_findInList(oldOpenVSports, currIfName) == NULL)
      {
         rutOpenVS_addOpenVSport(currIfName);
      }
      ptr = strtok_r(NULL, ",", &saveNewPortsPtr);
   }
   cmsMem_free(tmpNewPortsList);
   return CMSRET_SUCCESS;
}

UBOOL8 rutOpenVS_isValidBridgeWanInterface(const char *ifName)
{
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 found = FALSE;
   cmsLog_debug("Enter: ifName=%s", ifName);
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         found = TRUE;            
      }
      cmsObj_free((void **) &ipConn);
   }

   cmsLog_debug("Exit: ifName=%s found=%d", ifName, found);
   return found;
}


#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)

#ifdef BRCM_WLAN
CmsRet rutOpenVS_UpdateWlanInterface(const char *ifName, const char *toBridgeIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   _WlVirtIntfCfgObject *wlVirtIntfCfgObj=NULL;
   CmsRet ret;
   char cmdLine[BUFLEN_128];

   cmsLog_debug("Update %s to Bridge %s", ifName, toBridgeIfName);

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_WL_VIRT_INTF_CFG,  &iidStack, (void **) &wlVirtIntfCfgObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wlVirtIntfCfgObj->wlIfcname, ifName))
      {
         found = TRUE;
         break;
      }

      cmsObj_free((void **) &wlVirtIntfCfgObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find %s (from bridge %s to %s)", ifName, toBridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }


   if ( toBridgeIfName != NULL )
      CMSMEM_REPLACE_STRING_FLAGS( wlVirtIntfCfgObj->wlBrName, toBridgeIfName, mdmLibCtx.allocFlags );
   
   snprintf(cmdLine, sizeof(cmdLine), "ifconfig %s up", OVS_BRIDGE);  
   rut_doSystemAction("rut", cmdLine);
   
   ret = cmsObj_set(wlVirtIntfCfgObj, &iidStack);
   cmsObj_free((void **) &wlVirtIntfCfgObj);
   return ret;
}
#endif

CmsRet rutOpenVS_deleteOpenVSport_igd(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 isWanIntf = FALSE;
#ifdef DMP_BRIDGING_1
   UINT32 defaultBridgeRef = 0;
#endif
   cmsLog_debug("Enter: ifName=%s", ifName);
   /* for any delete interfaces 
    1. delete it from openvswitch
    2. add the port back to linux default bridge
   */
   /* this function should be called when openswitch is running*/
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
 
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!isWanIntf &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         isWanIntf = TRUE; 		   
         break;
      }
      cmsObj_free((void **) &ipConn);
   }

   /* check if it is wan, if no, assume it is lan*/
#ifdef DMP_BRIDGING_1
   rutPMap_addAvailableInterface(ifName, isWanIntf);
   rutPMap_addFilter(ifName, isWanIntf, defaultBridgeRef);
#endif
   if((isWanIntf == FALSE) || ( isWanIntf &&   !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED)))
   {
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
      char bridgeIfName[CMS_IFNAME_LENGTH]; 
      /* the filter object is already in mdm, so just the action
       * ie. add the wan interface to the interface group with correct bridge ifName
      */			
      if ((rutPMap_getBridgeIfNameFromIfName(ifName, bridgeIfName, isWanIntf)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Just the action to add wan intf to the interface group (%).", bridgeIfName);
         rutLan_addInterfaceToBridge(ifName, isWanIntf, bridgeIfName);
      }
#ifdef BRCM_WLAN
      if (cmsUtl_strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) == 0)
      {
         if (rutOpenVS_UpdateWlanInterface(ifName, bridgeIfName) != CMSRET_SUCCESS)
      	    cmsLog_error("Update wl interface %s information to Bridge %s failed!!", ifName, bridgeIfName);
      }
#endif
#else
      rutLan_addInterfaceToBridge(ifName, isWanIntf, "br0");
#ifdef BRCM_WLAN
      if (cmsUtl_strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) == 0)
      {
         if (rutOpenVS_UpdateWlanInterface(ifName, "br0") != CMSRET_SUCCESS)
            cmsLog_error("Update wl interface %s information to Bridge %s failed!!", ifName, "br0");
      }
#endif
#endif
   }

   if( isWanIntf )
      cmsObj_free((void **) &ipConn);
   cmsLog_debug("ifName=%s ", ifName);
   return CMSRET_SUCCESS;
}

/* for any new interfaces added into openvswitch 
 1. delete the port back to linux default bridge
 2. add it as new openvswitch port
 */
/* this function should be called when openswitch is running*/

CmsRet rutOpenVS_addOpenVSport_igd(const char *ifName)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 isWanIntf = FALSE,bAddToVS =FALSE;
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   char fullName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char availInterfaceReference[BUFLEN_256];
   _L2BridgingIntfObject *availIntfObj=NULL;
#endif	
	
   cmsLog_debug("Enter: ifName=%s", ifName);
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!isWanIntf &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         isWanIntf = TRUE; 		   
         break;
      }
      cmsObj_free((void **) &ipConn);
   }

   /* check if it is wan, if no, assume it is lan*/
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
   if (isWanIntf)
      ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, fullName);
   else
      ret= rutPMap_lanIfNameToAvailableInterfaceReference(ifName, fullName);
#endif
  	
   if((isWanIntf == FALSE) || ( isWanIntf &&	!cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED)))
   {
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
      char bridgeIfName[CMS_IFNAME_LENGTH]; 
      if( ret == CMSRET_SUCCESS)
      {
         if ((rutPMap_getBridgeIfNameFromIfName(ifName,bridgeIfName, isWanIntf)) == CMSRET_SUCCESS)
         {
            rutLan_removeInterfaceFromBridge(ifName, bridgeIfName);
         }
      }
#else
      rutLan_removeInterfaceFromBridge(ifName, "br0");
#endif
      bAddToVS = TRUE;
   }

#ifdef DMP_BRIDGING_1
   if( ret == CMSRET_SUCCESS)
   {
      strncpy(availInterfaceReference, fullName, sizeof(availInterfaceReference));
      availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';	 
      if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) == CMSRET_SUCCESS)
      {
         rutPMap_deleteFilter(fullName, isWanIntf);
         rutPMap_deleteAvailableInterface(fullName, isWanIntf);
      }
#ifdef BRCM_WLAN
      if (cmsUtl_strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) == 0)
      {
         if (rutOpenVS_UpdateWlanInterface(ifName, OVS_BRIDGE) != CMSRET_SUCCESS)
      	    cmsLog_error("Update wl interface %s information at %s failed!!", ifName, fullName);
      }
#endif
	  
   }
#endif
   if( isWanIntf )
      cmsObj_free((void **) &ipConn);
   else
      rutLan_enableInterface(ifName);

   if (bAddToVS)
   {
      snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
      rut_doSystemAction("rut", cmdLine);
   }
   cmsLog_debug("ifName=%s ", ifName);
   return ret;
}
#endif

#if defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
static UBOOL8 rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(const char *ifName,
                                                           Dev2IpInterfaceObject **ipIntfObj,
                                                           InstanceIdStack *iidStack)
{
   UBOOL8 found=FALSE;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) ipIntfObj) == CMSRET_SUCCESS)
   {
      if ( ( (*ipIntfObj)->X_BROADCOM_COM_Upstream || (*ipIntfObj)->X_BROADCOM_COM_BridgeService )&&
           !cmsUtl_strcmp((*ipIntfObj)->name, ifName) )  
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **) ipIntfObj);
      }
   }

   return found;
}

CmsRet rutOpenVS_deleteOpenVSport_dev2(const char *ifName)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;

   /* this function should be called when openswitch is running*/
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
   ret = rutBridge_addLanIntfToBridge_dev2(ifName,"br0");
   //for wan bridge interface
   if (rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(ifName, &ipIntfObj, &ipIntfIidStack))
   {
      if (ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
         CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_BridgeName, "br0", 
           mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_BridgeName);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_GroupName);

      ret = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of IP.Interface bridgeName failed,ret=%d", ret);
      }
      cmsObj_free((void **) &ipIntfObj);
   }
#ifdef BRCM_WLAN
   snprintf(cmdLine, sizeof(cmdLine), "ifconfig %s up", OVS_BRIDGE);  
   rut_doSystemAction("rut", cmdLine);

   if( (ret == CMSRET_SUCCESS) && (!strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR))) )
   {
      if ((ret = rutBridge_updateSSIDBridgeName(ifName, "br0")) != CMSRET_SUCCESS)
         cmsLog_error("Setting new bridgeName failed, ret=%d", ret);
   }
#endif
   
   return ret;
}

CmsRet rutOpenVS_restoreLowerLayersForWan_dev2(const char *ifName, Dev2IpInterfaceObject *ipIntfObj,
   InstanceIdStack *ipIntfIidStack)
{
   CmsRet ret=CMSRET_SUCCESS;
   char *fullPath=NULL;
   char tmpIfName[BUFLEN_128]={0};
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack optIntfIid = EMPTY_INSTANCE_ID_STACK;
   OpticalInterfaceObject *optIntfObj = NULL;

   if (cmsUtl_strstr(ifName, GPON_IFC_STR))
   {
      strncpy (tmpIfName, ifName, sizeof(tmpIfName)-1);
      strtok(tmpIfName, ".");

      rutOptical_getIntfByIfName (tmpIfName, &optIntfIid, &optIntfObj);
      if (optIntfObj == NULL)
      {
         cmsLog_error("Could not find optical intf obj %s(%s)",
                      tmpIfName, ifName);
         return CMSRET_INTERNAL_ERROR;
      }

      pathDesc.oid = MDMOID_OPTICAL_INTERFACE;  
      pathDesc.iidStack = optIntfIid;

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         cmsObj_free((void **) &optIntfObj);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->lowerLayers, fullPath, mdmLibCtx.allocFlags);
      ret = cmsObj_set(ipIntfObj, ipIntfIidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ipIntfObj. ret=%d", ret);
      }

      cmsObj_free((void **) &optIntfObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   }

   return ret;
}

CmsRet rutOpenVS_addOpenVSport_dev2(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 foundWan=FALSE;
   UBOOL8 isBridgeService=FALSE;
   CmsRet ret=CMSRET_SUCCESS;
   UBOOL8 bAddToVS =TRUE;
   char brIntfNameBuf[CMS_IFNAME_LENGTH]="br0";

   /* See if this intfName is a WAN interface, we will need to update the
    * X_BROADCOM_COM_BridgeName as part of this move
    */
   foundWan = rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(ifName, &ipIntfObj, &ipIntfIidStack);
   if (foundWan)
   {
      bAddToVS = FALSE;	
      isBridgeService = ipIntfObj->X_BROADCOM_COM_BridgeService;
      if ( isBridgeService)
      {
         if( !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus,MDMVS_SERVICEUP) )
           bAddToVS =TRUE;

         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_BridgeName);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_GroupName);
         ret = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Setting new bridgeName failed, ret=%d", ret);
         }
      }
      cmsObj_free((void **) &ipIntfObj);
   }

   if (!foundWan || isBridgeService)	
   {
      /* remove this interface from its current bridge */
      rutLan_removeInterfaceFromBridge(ifName, brIntfNameBuf);

      if (bAddToVS)
      {

         snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
         rut_doSystemAction("rut", cmdLine);
      }
   }

#ifdef BRCM_WLAN
   snprintf(cmdLine, sizeof(cmdLine), "ifconfig %s up", OVS_BRIDGE);  
   rut_doSystemAction("rut", cmdLine);

   if( (ret == CMSRET_SUCCESS) && (!strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR))) )
   {
      if ((ret = rutBridge_updateSSIDBridgeName(ifName, OVS_BRIDGE)) != CMSRET_SUCCESS)
         cmsLog_error("Setting new bridgeName failed, ret=%d", ret);
   }
#endif

   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}
#endif

void rutOpenVS_startupOpenVSport(const char *ifName, const char *brName)
{
   char cmdLine[BUFLEN_128];

   if (ifName == NULL || brName == NULL)
   {
       cmsLog_error("ifName and brName can't be empty!");
       return;
   }

   snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s",
            OVS_VSCTL, brName, ifName);
   rut_doSystemAction("rut", cmdLine);
   return;
}

void rutOpenVS_createBridge(const char *brName)
{
   char  cmdLine[BUFLEN_128];
   
   snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-br %s", OVS_VSCTL, brName);
   rut_doSystemAction("rut", cmdLine);

   return;
}

void rutOpenVS_deleteBridge(const char *brName)
{
   char cmdLine[BUFLEN_128];
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-br %s", OVS_VSCTL, brName);
   rut_doSystemAction("rut", cmdLine);
   return;
}

void rutOpenVS_shutdownOpenVSport(const char *ifName, const char *brName)
{
   char cmdLine[BUFLEN_128];

   if (ifName == NULL || brName == NULL)
   {
       cmsLog_error("ifName and brName can't be empty!");
       return;
   }

   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s",
            OVS_VSCTL, brName, ifName);
   rut_doSystemAction("rut", cmdLine);
   return;
}

#endif
