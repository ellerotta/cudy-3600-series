/*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bcmnet.h"  /* for SIOCGLINKSTATE, in bcmdrivers/opensource/include/bcm963xx */

#include "cms.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_network.h"
#include "rut_wan.h"
#include "rut_iptables.h"
#include "rut_ebtables.h"
#include "rut_multicast.h"
#ifdef DMP_DEVICE2_DHCPV4_1
#include "device2/rut2_dhcpv4.h"
#endif
#ifdef DMP_BRIDGING_1 /* SUPPORT_PORT_MAP  */
#include "rut_pmap.h"
#endif
#include "rut_qos.h"

#ifdef SUPPORT_LANVLAN
#ifdef SUPPORT_VLANSUBIF
#include "vlan_subif.h"
#else
#error This file requires SUPPORT_VLANSUBIF
#endif
#endif

#ifdef BRCM_VOICE_SUPPORT /* Support for VOIP */
#include "rut_voice.h"
#endif
#include "ethswctl_api.h"
#ifdef DMP_X_ITU_ORG_GPON_1
#include "rut_omci.h"
#endif
#if defined(DMP_X_BROADCOM_COM_MCAST_1)
#include "bcm_mcast_api.h"
#endif

#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)
#include "rut_openvswitch.h"
#endif
#ifdef DMP_X_BROADCOM_COM_IPV6_1
#include "rut_dns6.h"
#endif


#ifdef SUPPORT_LANVLAN
void rutLan_RemoveDefaultLanVlanInterface(char *l2IfName)
{
   char vlanIface[BUFLEN_32];

   /* If this is WANOnly interface
    * just return without removing default LANVLAN interface
    */
   if (qdmIntf_isInterfaceWANOnly(l2IfName))
   {
      cmsLog_notice("Skip removing default LANVLAN interface %s", l2IfName);
      return;
   }
   vlanSubif_IntfName(l2IfName, 0, vlanIface, sizeof(vlanIface));
   vlanSubif_deleteVlanInterface(vlanIface);
}

void rutLan_AddDefaultLanVlanInterface(char *l2IfName)
{
   char vlanIface[BUFLEN_32];

   if (rut_checkInterfaceUp(l2IfName) == FALSE)
      rut_setIfState(l2IfName, 1);

   /* If this is WANOnly interface
    * just return without adding default LANVLAN interface
    */
   if (qdmIntf_isInterfaceWANOnly(l2IfName))
   {
      cmsLog_notice("Skip adding default LANVLAN interface %s", l2IfName);
      return;
   }

   /* Create a default vlan interface */
   vlanSubif_createVlanInterface(l2IfName, -1, NULL);
   vlanSubif_IntfName(l2IfName, 0, vlanIface, sizeof(vlanIface));
   rut_setIfState(vlanIface, 1);
}

void rutLan_RemoveLanVlanInterface(char *l2IfName, char *vlanList)
{
   char *p, pVlanList[BUFLEN_512], vlanIface[BUFLEN_32];
   UINT32 vid;

   cmsLog_debug("l2IfName = %s, vlanList = %s", l2IfName, vlanList);

   vlanSubif_init();

   strncpy(pVlanList, vlanList, sizeof(pVlanList)-1);
   pVlanList[sizeof(pVlanList)-1] = '\0';

   for (p = strtok(pVlanList, "/"); p; p = strtok(NULL, "/"))
   {
      /*Get M/N pair */
      vid = atoi(p);
      p = strtok(NULL, ",");
      if (p == NULL)
      {
         cmsLog_error("invalid vlanlist format %d, ignore", vid);
         break;
      }
      vlanSubif_IntfName(l2IfName, vid, vlanIface, sizeof(vlanIface));
      vlanSubif_deleteVlanInterface(vlanIface);

#ifdef DMP_BRIDGING_1
      rutPMap_deleteFilterWithVlan(l2IfName, vid);
#endif
   }
   vlanSubif_cleanup();
}

void rutLan_CreateLanVlanIf(char *l2IfName, char *vlanList,
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     UBOOL8 addnew __attribute__((unused)))
{
   char vlanIface[BUFLEN_32];
   UINT32 vid, pbits;
   vlansubif_options_t vlan_subif_options;

   memset((void *)&vlan_subif_options, 0, sizeof(vlan_subif_options));
   cmsLog_debug("l2IfName = %s, vlanList = %s", l2IfName, vlanList);

   /*Bring lan interface up */
   if (rut_checkInterfaceUp(l2IfName) == FALSE)
   	rut_setIfState(l2IfName, 1);

   if (vlanList)
   {
      int  pos=0;
      char tmpStr[BUFLEN_16];

      vlanSubif_init();

      //format is vid1/pbits1, vid2/pbits2, ...
      while (pos < (int)strlen(vlanList))
      {
         if (sscanf(&vlanList[pos], "%u/%u", &vid, &pbits) != 2)
         {
             cmsLog_error("Wrong format of vlanList, [%s]", vlanList);
             break;
         }
         sprintf(tmpStr, "%u/%u", vid, pbits);
         pos += cmsUtl_strlen(tmpStr);
         while (vlanList[pos] != '\0' && (vlanList[pos] == ' ' || vlanList[pos] == ','))
         {
             pos++;
         }

#ifdef DMP_X_ITU_ORG_GPON_1
         if (rutOmci_isEthPortControlledByOmci(l2IfName) == TRUE)
         {
            vlan_subif_options.is_phyif_other_owned = 1; 
         }
#endif

         /* Create vlan interface or vid */
         vlan_subif_options.pbit = pbits;
         vlanSubif_createVlanInterface(l2IfName, vid, &vlan_subif_options);
         vlanSubif_IntfName(l2IfName, vid, vlanIface, sizeof(vlanIface));
         rut_setIfState(vlanIface, 1);

#ifdef DMP_BRIDGING_1
         if (!addnew)
            rutPMap_addFilter(vlanIface, 0, -1);
         else
         {
            char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};

            if (rutPMap_lanIfNameToAvailableInterfaceReference(l2IfName, availInterfaceReference) == CMSRET_SUCCESS)
            {
               InstanceIdStack IidStack2 = EMPTY_INSTANCE_ID_STACK;
               L2BridgingIntfObject *availIntfObj = NULL;

               /*
                * very anoying, the TR98 spec says the fullname does not end in dot,
                * but our internal conversion routines put in the final dot, so strip
                * it out before putting into the MDM.
                */
               availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';
               if (rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &IidStack2, &availIntfObj) == CMSRET_SUCCESS)
               {
                  L2BridgingFilterObject *filterObj = NULL;
                  char buf[BUFLEN_32];

                  sprintf(buf, "%u", availIntfObj->availableInterfaceKey);
                  cmsObj_free((void **)&availIntfObj);

                  if (rutPMap_getFilterWithVlan(buf, vid, &IidStack2, &filterObj) == CMSRET_SUCCESS)
                  {
                     L2BridgingEntryObject *bridgeObj = NULL;

                     if (filterObj->filterBridgeReference >= 0 &&
                        rutPMap_getBridgeByKey(filterObj->filterBridgeReference, &IidStack2, &bridgeObj) == CMSRET_SUCCESS)
                     {
                        snprintf(buf, sizeof(buf), "br%d", bridgeObj->bridgeKey);
                        rutLan_addInterfaceToBridge(vlanIface, 0, buf);
                        cmsObj_free((void **)&bridgeObj);
                     }
                     cmsObj_free((void **)&filterObj);
                  }
               }
            }
         }
#endif
      }

      vlanSubif_cleanup();
   }
}
#endif

#ifdef DMP_BASELINE_1
/*
 * Most of the functions in this file are only usable in Legacy TR98 mode
 * or Hybrid TR98+TR181 mode.  But some are generic and can be used by
 * the Pure TR181 code.  There will be some functions in thif file
 * that are outside of #ifdef DMP_BASELINE_1
 */


CmsRet rutLan_addBridge(UINT32 *bridgeNumber)
{
   InstanceIdStack lanDevIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack ipIntfIidStack;
   _LanIpIntfObject *ipIntfObj=NULL;
   _LanHostCfgObject *lanHostCfgObj=NULL;
   UINT32 flags=0;

   char ipAddr[BUFLEN_32];
   char ipMask[BUFLEN_32];
   CmsRet ret;

   /*
    * first add another instance of LANDevice.{i}.
    * This will automatically create LANDevice.{i}.LANHostConfigManagement. ,
    * which is the object that controls dhcpd.
    */
   if ((ret = cmsObj_addInstance(MDMOID_LAN_DEV, &lanDevIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create another LANDev instance, ret=%d", ret);
      return ret;
   }

   cmsLog_debug("new LANDevice created at %s", cmsMdm_dumpIidStack(&lanDevIidStack));

   /* now create a single IP Interface instance under this LANDevice */
   ipIntfIidStack = lanDevIidStack;
   if ((ret = cmsObj_addInstance(MDMOID_LAN_IP_INTF, &ipIntfIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create IP interface instance, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_DEV, &lanDevIidStack);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_LAN_IP_INTF, &ipIntfIidStack, flags, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get IP interface, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_IP_INTF, &ipIntfIidStack);
      cmsObj_deleteInstance(MDMOID_LAN_DEV, &lanDevIidStack);
      return ret;
   }

   cmsLog_debug("new IP Intf (for bridge %s) created at %s", ipIntfObj->X_BROADCOM_COM_IfName, cmsMdm_dumpIidStack(&ipIntfIidStack));


   /*
    * Set the IP address, which is based on the bridge number,
    * but don't enable the ipIntfObj yet.
    * The IP intf will be enabled when the portmapping entry is enabled.
    */
   (*bridgeNumber) = atoi(&(ipIntfObj->X_BROADCOM_COM_IfName[2]));

    if(!strcmp(ipIntfObj->IPInterfaceAddressingType, MDMVS_DHCP)) {
       sprintf(ipAddr, "0.0.0.0");
       sprintf(ipMask, "0.0.0.0");
    } else {
        sprintf(ipAddr, "192.168.%d.1", (*bridgeNumber) + 1);
        sprintf(ipMask, "255.255.255.0");
    }

   cmsLog_debug("setting %s ipAddr=%s", ipIntfObj->X_BROADCOM_COM_IfName, ipAddr);

   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceIPAddress, ipAddr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceSubnetMask, ipMask, mdmLibCtx.allocFlags);

   ret = cmsObj_set(ipIntfObj, &ipIntfIidStack);
   cmsObj_free((void **) &ipIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IP interface, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_IP_INTF, &ipIntfIidStack);
      cmsObj_deleteInstance(MDMOID_LAN_DEV, &lanDevIidStack);
      return ret;
   }


   /*
    * Now we also configure the LANDevice.{i}.LANHostConfigManagement. object,
    * which controls dhcpd.
    */
   if ((ret = cmsObj_get(MDMOID_LAN_HOST_CFG, &lanDevIidStack, flags, (void **) &lanHostCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN_HOST_CFG object, ret=%d", ret);
   }

   lanHostCfgObj->DHCPServerEnable = TRUE;
   sprintf(ipAddr, "192.168.%d.2", (*bridgeNumber) + 1);
   CMSMEM_REPLACE_STRING_FLAGS(lanHostCfgObj->minAddress, ipAddr, mdmLibCtx.allocFlags);
   sprintf(ipAddr, "192.168.%d.254", (*bridgeNumber) + 1);
   CMSMEM_REPLACE_STRING_FLAGS(lanHostCfgObj->maxAddress, ipAddr, mdmLibCtx.allocFlags);

   ret = cmsObj_set(lanHostCfgObj, &lanDevIidStack);
   cmsObj_free((void **) &lanHostCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LAN Host Cfg, ret=%d", ret);
   }

   return ret;
}


void rutLan_deleteBridge(const char *bridgeIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = rutLan_getLanDevIidStackOfBridge(bridgeIfName, &iidStack) != CMSRET_SUCCESS))
   {
      cmsLog_error("could not find bridge %s to delete", bridgeIfName);
      return;
   }

   cmsLog_debug("found bridge %s to delete under LANDevice %s", bridgeIfName, cmsMdm_dumpIidStack(&iidStack));

   cmsObj_deleteInstance(MDMOID_LAN_DEV, &iidStack);

   /* update dhcpd */
   rutLan_updateDhcpd();

   /* update DLNA dms */
#ifdef DMP_X_BROADCOM_COM_DLNA_1
   rutLan_updateDlna();
#endif
   return;
}


SINT32 rutLan_getNextAvailableBridgeNumber_igd()
{
   UBOOL8 inUseArray[MAX_LAYER2_BRIDGES] = {FALSE};
   UINT32 i;
   _LanIpIntfObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   /*
    * Enumerate through all of our bridges and for each bridge, mark that
    * bridge's slot in the inUseArray as TRUE;
    */
   while ((ret = cmsObj_getNext(MDMOID_LAN_IP_INTF, &iidStack, (void **) &ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_IfName != NULL)
      {
         UINT32 bridgeNumber;
         bridgeNumber = atoi(&(ipIntfObj->X_BROADCOM_COM_IfName[2]));
         cmsAst_assert(bridgeNumber < MAX_LAYER2_BRIDGES);
         inUseArray[bridgeNumber] = TRUE;
      }
      cmsObj_free((void **) &ipIntfObj);
   }

   /*
    * Now go through the array and return the first available.
    */
   for (i=0; i < MAX_LAYER2_BRIDGES; i++)
   {
      if (inUseArray[i] == FALSE)
      {
         cmsLog_debug("returing %d", (SINT32) i);
         return ((SINT32) i);
      }
   }


   cmsLog_error("All %d bridges in use!", MAX_LAYER2_BRIDGES);
   return (-1);
}


#endif  /* DMP_BASELINE_1 */


static void rutLan_createBridge(const char *brIfName)
{
   char cmdStr[BUFLEN_128];
   snprintf(cmdStr, sizeof(cmdStr), "brctl addbr %s", brIfName);
   rut_doSystemAction("rutLan", cmdStr);
}

CmsRet rutLan_enableBridge(const char *bridgeIfName, UBOOL8 isOVS,
                         UBOOL8 isNewBridge, const char *ipAddr, const char *netmask,
                         const char *broadcast)
{
   char cmdStr[BUFLEN_128];

#ifdef SUPPORT_SAMBA
   UBOOL8 isBridgeIp = FALSE;
#endif


   /* configure br0: ie.
    * brctl addbr br0;
    * brctl stp br0 off;
    * brctl setfd br0 0;
    * bcm_sendarp -s br0 -d br0;
    */

   /*skip brX:0 interface*/
   if (isNewBridge && cmsUtl_strstr(bridgeIfName, ":") == NULL)
   {
      if (isOVS)
      {
#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)
          if (rutOpenVS_isRunning())
          {
              rutOpenVS_createBridge(bridgeIfName);
          }
          else
          {
              return CMSRET_INVALID_ARGUMENTS;
          }
#else
          return CMSRET_INVALID_ARGUMENTS;
#endif
      }
      else
      {
          rutLan_createBridge(bridgeIfName);
      }

#ifdef DMP_DEVICE2_BRIDGE_1
      /* Always configure the br0's mac address to the base mac address */
      if(cmsUtl_strcmp(bridgeIfName, "br0")==0)
      {
        UINT8 macNum[MAC_ADDR_LEN];
        char macStr[MAC_STR_LEN+1];

        devCtl_getBaseMacAddress(macNum);
        cmsUtl_macNumToStr(macNum, macStr);
        snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s hw ether %s", bridgeIfName, macStr);
        rut_doSystemAction("rutLan", cmdStr);

#if defined(HAS_SF2)
        // write br mac address to switch for pkt to CPU
        bcm_multiport_set(0, (unsigned char *) macNum);
        bcm_multiport_set(1, (unsigned char *) macNum);
#endif

#ifdef SUPPORT_IPV6
        char linkIpv6AddrStr[BUFLEN_48];

        cmsUtl_prefixMacToAddress("fe80::", macNum, linkIpv6AddrStr);
        snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s/64", bridgeIfName, linkIpv6AddrStr);
        rut_doSystemAction("rutLan", cmdStr);
#endif
      }
#endif

#ifdef DMP_X_ITU_ORG_GPON_1
      {
         UINT32 brgFwdMask;

         if (rutOmci_getBrgFwdMask(&brgFwdMask) == CMSRET_SUCCESS)
         {
            snprintf(cmdStr, sizeof(cmdStr),
              " echo %d > /sys/class/net/%s/bridge/group_fwd_mask",
              brgFwdMask, bridgeIfName);
            rut_doSystemAction("rutLan", cmdStr);
         }
      }
#endif

#ifdef SUPPORT_IPV6
      snprintf(cmdStr, sizeof(cmdStr), "echo 1 > /proc/sys/net/ipv6/conf/%s/keep_addr_on_down", bridgeIfName);
      rut_doSystemAction("rutLan", cmdStr);
#endif

      if (!isOVS)
      {
          snprintf(cmdStr, sizeof(cmdStr), "brctl stp %s off", bridgeIfName);
          rut_doSystemAction("rutLan", cmdStr);

          snprintf(cmdStr, sizeof(cmdStr), "brctl setfd %s 0", bridgeIfName);
          rut_doSystemAction("rutLan", cmdStr);
      }
      snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s br0 -d %s", bridgeIfName);
      rut_doSystemAction("rutLan", cmdStr);
   }


   /* change the lan info. eg.
    * ifconfig br0 10.6.34.41 netmask 255.255.255.0 broadcast 10.6.34.255 up
    */

   if (!cmsUtl_strcmp(ipAddr, "0.0.0.0") ||
       !cmsUtl_strcmp(netmask, "0.0.0.0") ||
       !cmsUtl_strcmp(broadcast, "0.0.0.0"))
   {
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up",
               bridgeIfName);
   }
   else
   {
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s netmask %s broadcast %s up",
               bridgeIfName, ipAddr, netmask, broadcast);
#ifdef SUPPORT_SAMBA
      if(cmsUtl_strcmp(bridgeIfName, "br0") == 0)
         isBridgeIp = TRUE;
#endif

   }

#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   /* RFC7084 requirement */
   rutIpt_defaultLANSetup6(bridgeIfName);
   rutEbt_defaultLANSetup6();
#endif

   rut_doSystemAction("rcl_lan", cmdStr);
   /*
    * Due to the hierarchy of the TR98 data model, we call the RCL handler
    * function for IGMPsnooping before the bridge (br0) is created, so
    * the snooping config will not work because the bridge does not
    * exist yet.
    * So here after we create the bridge, call the IGMP snooping RCL again.
    */
   rutMulti_updateIgmpMldSnooping(bridgeIfName);

#ifdef SUPPORT_SAMBA
   if(isBridgeIp)
   {
      SINT32 pid;
      UBOOL8 isActive = FALSE;
      CmsRet ret;
      StorageServiceObject *storageServiceObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if(( ret =cmsObj_getNext(MDMOID_STORAGE_SERVICE, &iidStack, (void **) &storageServiceObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get StorageService object, ret=%d", ret);
         return ret;
      }

      isActive = rut_isApplicationActive(EID_SAMBA);
      if(isActive)
      {	
         if(storageServiceObj->userAccountNumberOfEntries) /* restart smbd */
         {
            if((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_SAMBA, NULL,0)) == CMS_INVALID_PID)
               cmsLog_error("failed to restart SAMBA server.");
         }		
         else /* no user account existed, delete smbd */
         {
            if((rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SAMBA, NULL,0)) != CMSRET_SUCCESS)
               cmsLog_error("failed to stop SAMBA server.");					
         }
      }
      else
      {	
         if(storageServiceObj->userAccountNumberOfEntries) /* start smbd */
         {
            if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_SAMBA, NULL,0)) == CMS_INVALID_PID)
               cmsLog_error("failed to start SAMBA server.");
         }
      }

      cmsObj_free((void **) &storageServiceObj);
   }
#endif


#ifdef BRCM_VOICE_SUPPORT
   /* If voice is enabled and bound to LAN, send an update message */
   rutVoice_updateIfAddr ( MDMVS_LAN, ipAddr );
#endif /* BRCM_VOICE_SUPPORT */

   return CMSRET_SUCCESS;
}


void rutLan_disableBridge(const char *bridgeIfName, UBOOL8 isOVS)
{
   char cmd[BUFLEN_128];

   snprintf (cmd, sizeof(cmd), "ifconfig %s down", bridgeIfName);
   rut_doSystemAction("rutLan", cmd);

   /*skip brX:0 interface*/
   if (cmsUtl_strstr(bridgeIfName, ":") ==NULL)
   {
      if (isOVS)
      {
#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)
          if (rutOpenVS_isRunning() == TRUE)
          {
              rutOpenVS_deleteBridge(bridgeIfName);
          }
#endif
      }
      else
      {
          snprintf (cmd, sizeof(cmd), "brctl delbr %s", bridgeIfName);
          rut_doSystemAction("rutLan", cmd);
      }
   }

   rutMulti_updateIgmpMldSnooping(bridgeIfName);
}



#ifdef DMP_BASELINE_1

CmsRet rutLan_addEthInterface(const char *ethIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _LanEthIntfObject *ethIntfObj=NULL;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   if ((ret = rutLan_getLanDevIidStackOfBridge("br0", &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find br0 to add eth interface to!!");
      return ret;
   }

   if ((ret = cmsObj_addInstance(MDMOID_LAN_ETH_INTF, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create lan eth intf, ret=%d", ret);
   }

   cmsLog_debug("lan eth intf %s created at %s", ethIfName, cmsMdm_dumpIidStack(&iidStack));

   /*
    * Now we need to set some parameter on the eth intf object.
    */
   if ((ret = cmsObj_get(MDMOID_LAN_ETH_INTF, &iidStack, flags, (void **) &ethIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lan eth intf object, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &iidStack);
      return ret;
   }

   ethIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ethIntfObj->X_BROADCOM_COM_IfName, ethIfName, mdmLibCtx.allocFlags);

   if ((ret = cmsObj_set(ethIntfObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LAN eth intf object, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &iidStack);
   }

   cmsObj_free((void **) &ethIntfObj);

   return ret;
}


void rutLan_deleteEthInterface(const char *ethIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _LanEthIntfObject *ethIntfObj=NULL;
   CmsRet ret;

   if ((ret = rutLan_getEthInterface(ethIfName, &iidStack, &ethIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ethIfName %s", ethIfName);
      return;
   }

   cmsObj_free((void **) &ethIntfObj);

   cmsLog_debug("deleting %s at %s", ethIfName, cmsMdm_dumpIidStack(&iidStack));

   cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &iidStack);

   return;
}


CmsRet rutLan_getEthInterface(const char *ifName, InstanceIdStack *iidStack, LanEthIntfObject **lanEthObj)
{
   CmsRet ret;

   while ((ret = cmsObj_getNext(MDMOID_LAN_ETH_INTF, iidStack, (void **) lanEthObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp((*lanEthObj)->X_BROADCOM_COM_IfName, ifName))
      {
         /* need to return the object to the caller */
         break;
      }
      cmsObj_free((void **) lanEthObj);
   }

   return ret;
}


CmsRet rutLan_moveEthInterface(const char *ethIfName, const char *fromBridgeIfName, const char *toBridgeIfName)
{
   InstanceIdStack fromIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack toIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *fromIntfFullPath=NULL;
   char *toIntfFullPath=NULL;
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   UBOOL8 found=FALSE;
   _LanEthIntfObject *origEthObj=NULL;
   CmsRet ret;

   if ((ret = rutLan_getLanDevIidStackOfBridge(fromBridgeIfName, &fromIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find fromBridgeIfName %s", fromBridgeIfName);
      return ret;
   }

   if ((ret = rutLan_getLanDevIidStackOfBridge(toBridgeIfName, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find toBridgeIfName %s", toBridgeIfName);
      return ret;
   }

   cmsLog_debug("moving %s from %s to %s", ethIfName, fromBridgeIfName, toBridgeIfName);

   /*
    * Get the ethLanIntf object we are about to delete, so we preserve
    * the object.
    */
   while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_ETH_INTF, &fromIidStack, &iidStack, (void **) &origEthObj)) == CMSRET_SUCCESS)
   {
#ifdef SUPPORT_LANVLAN
      /* For VLAN LAN, only compare the part without vlan extension; ie. only the "eth1" part since ethIfName
      * is "eth1.0" here
      */
      if (cmsUtl_strncmp(origEthObj->X_BROADCOM_COM_IfName, ethIfName, cmsUtl_strlen(origEthObj->X_BROADCOM_COM_IfName)) == 0)
#else
      if (cmsUtl_strcmp(origEthObj->X_BROADCOM_COM_IfName, ethIfName) == 0)
#endif
      {
         /* do not free origEthObj */
         found = TRUE;

         /* get fullpath of the original ethIntf */
         pathDesc.oid = MDMOID_LAN_ETH_INTF;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fromIntfFullPath);
         break;
      }

      cmsObj_free((void **) &origEthObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find %s in sub-tree %s (bridge %s)", ethIfName, cmsMdm_dumpIidStack(&fromIidStack), fromBridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   /* iidStack now points to the instance of LAN eth intf that has the specified ethIfName */
   cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &iidStack);


   /*
    * Now add a new instance of the LAN eth intf under the "to" bridge.
    */
   if ((ret = cmsObj_addInstance(MDMOID_LAN_ETH_INTF, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not add LAN eth intf instance under toBridgeIfName %s, ret=%d", toBridgeIfName, ret);
      cmsMem_free((void **) &origEthObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fromIntfFullPath);
      return ret;
   }

   /* get fullpath of the new ethIntf */
   pathDesc.iidStack = toIidStack;
   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &toIntfFullPath);

   /*
    * Before doing the set, which will trigger queue reconfig, update the
    * interface fullpaths in the QoS Queue and Classification objs, this is
    * only needed by TR98 (TR181 does not have this issue).
    */
#ifdef DMP_QOS_1
   rutQos_updateInterfaceFullPaths(fromIntfFullPath, toIntfFullPath);
#endif

   CMSMEM_FREE_BUF_AND_NULL_PTR(fromIntfFullPath);
   CMSMEM_FREE_BUF_AND_NULL_PTR(toIntfFullPath);

   /* Set the original EthObj into the new EthObj */
   if ((ret = cmsObj_set(origEthObj, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set origEthObj into place, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &toIidStack);
   }

   cmsObj_free((void **) &origEthObj);

   return ret;
}


#ifdef DMP_USBLAN_1
CmsRet rutLan_moveUsbInterface(const char *usbIfName, const char *fromBridgeIfName, const char *toBridgeIfName)
{
   InstanceIdStack fromIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack toIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *fromIntfFullPath=NULL;
   char *toIntfFullPath=NULL;
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   UBOOL8 found=FALSE;
   _LanUsbIntfObject *origUsbObj=NULL;
   CmsRet ret;

   if ((ret = rutLan_getLanDevIidStackOfBridge(fromBridgeIfName, &fromIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find fromBridgeIfName %s", fromBridgeIfName);
      return ret;
   }

   if ((ret = rutLan_getLanDevIidStackOfBridge(toBridgeIfName, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find toBridgeIfName %s", toBridgeIfName);
      return ret;
   }

   cmsLog_debug("moving %s from %s to %s", usbIfName, fromBridgeIfName, toBridgeIfName);

   /*
    * Get the usbLanIntf object we are about to delete, so we preserve
    * the object.
    */
   while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_USB_INTF, &fromIidStack, &iidStack, (void **) &origUsbObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(origUsbObj->X_BROADCOM_COM_IfName, usbIfName) == 0)
      {
         /* do not free origUsbObj */
         found = TRUE;

         /* get fullpath of the original usbIntf */
         pathDesc.oid = MDMOID_LAN_USB_INTF;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fromIntfFullPath);
         break;
      }

      cmsObj_free((void **) &origUsbObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find %s in sub-tree %s (bridge %s)", usbIfName, cmsMdm_dumpIidStack(&fromIidStack), fromBridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   /* iidStack now points to the instance of LAN usb intf that has the specified usbIfName */
   cmsObj_deleteInstance(MDMOID_LAN_USB_INTF, &iidStack);


   /*
    * Now add a new instance of the LAN usb intf under the "to" bridge.
    */
   if ((ret = cmsObj_addInstance(MDMOID_LAN_USB_INTF, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not add LAN usb intf instance under toBridgeIfName %s, ret=%d", toBridgeIfName, ret);
      cmsMem_free((void **) &origUsbObj);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fromIntfFullPath);
      return ret;
   }

   /* get fullpath of the new usbIntf */
   pathDesc.iidStack = toIidStack;
   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &toIntfFullPath);

   /*
    * Before doing the set, which will trigger queue reconfig, update the
    * interface fullpaths in the QoS Queue and Classification objs, this is
    * only needed by TR98 (TR181 does not have this issue).
    */
#ifdef DMP_QOS_1
   rutQos_updateInterfaceFullPaths(fromIntfFullPath, toIntfFullPath);
#endif

   CMSMEM_FREE_BUF_AND_NULL_PTR(fromIntfFullPath);
   CMSMEM_FREE_BUF_AND_NULL_PTR(toIntfFullPath);

   /* Set the original UsbObj into the new UsbObj */
   if ((ret = cmsObj_set(origUsbObj, &toIidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set origUsbObj into place, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_LAN_USB_INTF, &toIidStack);
   }

   cmsObj_free((void **) &origUsbObj);

   return ret;
}

#endif /* DMP_USBLAN_1 */


#ifdef BRCM_WLAN
CmsRet rutLan_moveWlanInterface(const char *ifName, const char *fromBridgeIfName, const char *toBridgeIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   _WlVirtIntfCfgObject *wlVirtIntfCfgObj=NULL;
   CmsRet ret;


   cmsLog_debug("moving %s from Bridge %s to %s", ifName, fromBridgeIfName, toBridgeIfName);

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
      cmsLog_error("Could not find %s (from bridge %s to %s)", ifName, fromBridgeIfName, toBridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   /*
    * This function is called from rutPMap_moveLanInterface, which is called by
    * rcl_L2BridgingFilterObject when a filterBridgeReference changes from one
    * bridge to another.  Unlike the LANEthIntf objects, the wireless code does not
    * move the wlVirtIntfCfg objects from one LANDevice to another.  So we have to
    * move the interfaces in right here.  The filters have already been modified.
    * And the wlBrName is for the wl internal bookkeeping.
    */

   /* TODO: get the type of target bridge and pass in to the below function calls.*/
   rutLan_removeInterfaceFromBridge(ifName, fromBridgeIfName);
   rutLan_addInterfaceToBridge(ifName, FALSE, toBridgeIfName);

   if ( toBridgeIfName != NULL )
	   CMSMEM_REPLACE_STRING_FLAGS( wlVirtIntfCfgObj->wlBrName, toBridgeIfName, mdmLibCtx.allocFlags );

   ret = cmsObj_set(wlVirtIntfCfgObj, &iidStack);
   cmsObj_free((void **) &wlVirtIntfCfgObj);

   cmsLog_debug("done, ret=%d", ret);
   return ret;
}

CmsRet rutLan_getWlanInterface(const char *ifName, InstanceIdStack *iidStack,  WlVirtIntfCfgObject **wlVirtIntfCfgObj )
{
   UINT32 flags = OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   while ((ret = cmsObj_getNextFlags(MDMOID_WL_VIRT_INTF_CFG, iidStack, flags, (void **) wlVirtIntfCfgObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp((*wlVirtIntfCfgObj)->wlIfcname, ifName))
      {
         /* need to return the object to the caller */
         break;
      }
      cmsObj_free((void **) wlVirtIntfCfgObj);
   }

   return ret;
}

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
CmsRet rutLan_addWlanInterface(const char *wlIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WlVirtIntfCfgObject *wlIntfObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entering...\n");

   if ((ret = rutLan_getWlanInterface(wlIfName, &iidStack, &wlIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find wlIfName %s", wlIfName);
      return ret;
   }

   /*TODO: get the type of target bridge and pass in to the below function call.*/
   rutLan_addInterfaceToBridge(wlIntfObj->wlIfcname, FALSE, wlIntfObj->wlBrName);
   cmsObj_free((void **) &wlIntfObj);

   return ret;
}


void rutLan_deleteWlanInterface(const char * wlIfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WlVirtIntfCfgObject *wlIntfObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entering...\n");

   if ((ret = rutLan_getWlanInterface(wlIfName, &iidStack, &wlIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find wlIfName %s", wlIfName);
      return;
   }

   /* remove wlVirtCfg from current bridge */
   if ( wlIntfObj->wlEnblSsid )
   {
      /*
       * If bridging is not defined, then we just remove this virtintf
       * from the bridge.  Also here interface grouping is not
       * implemented for now
       */
      rutLan_removeInterfaceFromBridge(wlIntfObj->wlIfcname, wlIntfObj->wlBrName);
   }
   cmsObj_free((void **) &wlIntfObj);

   return;
}
#endif /*DMP_X_BROADCOM_COM_WIFIWAN_1 */
#endif


CmsRet rutLan_getLanDevIidStackOfBridge(const char *bridgeIfName, InstanceIdStack *iidStack)
{
   UBOOL8 found=FALSE;
   _LanDevObject *lanDevObj=NULL;
   _LanIpIntfObject *ipIntfObj=NULL;
   CmsRet ret;

   while ((!found) &&
          ((ret = cmsObj_getNext(MDMOID_LAN_IP_INTF, iidStack, (void **) &ipIntfObj)) == CMSRET_SUCCESS))
   {
      found = (0 == cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IfName, bridgeIfName));
      cmsObj_free((void **) &ipIntfObj);
   }

   if (!found)
   {
      cmsLog_error("could not find bridge %s", bridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   /*
    * The LAN_IP_INTF is one level below the LANDevice, so we have to get the
    * iidStack of the LANDevice that we want to delete.
    */
   if ((ret = cmsObj_getAncestor(MDMOID_LAN_DEV, MDMOID_LAN_IP_INTF, iidStack, (void **) &lanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get ancestor object, ret=%d", ret);
      return ret;
   }

   cmsObj_free((void **) &lanDevObj);

   return ret;
}

#endif  /* DMP_BASELINE_1 */



void rutLan_enableInterface(const char *lanIfName)
{
   char cmdStr[BUFLEN_128]={0};
   cmsLog_notice("Entered: ifconfig %s up", lanIfName);
   snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", lanIfName);
   rut_doSystemAction("rutLan_enableInterface", cmdStr);
}


void rutLan_restartDhcpc(const char *bridgeIfName)
{
   InstanceIdStack ipIntfIidStack=EMPTY_INSTANCE_ID_STACK;
   LanIpIntfObject *ipIntfObj=NULL;
   CmsRet ret;
   UBOOL8 found = FALSE;

    while (!found &&
         cmsObj_getNext(MDMOID_LAN_IP_INTF, &ipIntfIidStack, (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IfName, bridgeIfName))
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **) &ipIntfObj);
      }
   }

   if (!found)
   {
      cmsLog_error("could not find IpIntf object of %s", bridgeIfName);
      return;
   }
   CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_UNCONFIGURED);
   ret = cmsObj_set(ipIntfObj, &ipIntfIidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of lanIpIntfObj failed, ret=%d", ret);
   }
   cmsObj_free((void **) &ipIntfObj);
   return;
}

void rutLan_addInterfaceToBridge(const char *origIfName, UBOOL8 isWanIntf,
                                 const char *bridgeIfName)
{
    rutLan_addInterfaceToBridgeEx(origIfName, isWanIntf, bridgeIfName, FALSE);
}

void rutLan_addInterfaceToBridgeEx(const char *origIfName, UBOOL8 isWanIntf,
                                   const char *bridgeIfName, UBOOL8 isOVS)
{
   char cmdStr[BUFLEN_128];
#ifdef SUPPORT_LANVLAN
   char *p = NULL;
#endif
   char ifNameBuf[BUFLEN_32];
   memset(ifNameBuf, 0, sizeof(ifNameBuf));
   const char *ifName = ifNameBuf;
   UINT8 br0macAddrBefore[MAC_ADDR_LEN]={0};
   UINT8 br0macAddrAfter[MAC_ADDR_LEN]={0};

   if (cmsUtl_strlen(origIfName) == 0 ||
       cmsUtl_strlen(bridgeIfName) == 0)
   {
      cmsLog_error("got NULL or empty ifName (%p %d) or bridgeIfName (%p %d)",
                   origIfName, cmsUtl_strlen(origIfName),
                   bridgeIfName, cmsUtl_strlen(bridgeIfName));
      return;
   }

   cmsLog_notice("Entered: bridgeIfName=%s port ifName=%s isWanIntf=%d isOVS=%d",
                 bridgeIfName, origIfName, isWanIntf, isOVS);

#ifdef SUPPORT_LANVLAN
   if (strstr(origIfName, "eth") && ((p = strchr(origIfName, '.')) == NULL))
      snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", origIfName);
   else
#endif
      ifName = origIfName;

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
   if(rutOpenVS_isEnabled() == TRUE)
   {
      if (rutOpenVS_isOpenVSPorts(ifName) == TRUE)
      {
         rutOpenVS_startupOpenVSport(ifName, OVS_BRIDGE);
         return;
      }
   }
#endif

   if (isOVS == TRUE)
   {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_2
      if (rutOpenVS_isRunning() == TRUE)
      {
         rutOpenVS_startupOpenVSport(ifName, bridgeIfName);
      }
#endif
      return;
   }

   /* If this is WANOnly interface
    * just return without adding to the bridge
    */
   if (qdmIntf_isInterfaceWANOnly(origIfName))
   {
      rutLan_enableInterface(origIfName);
      cmsLog_notice("Skip adding WANOnly interface %s to bridge %s", origIfName, bridgeIfName);
      return;
   }

   /* save br0's macaddr before adding interface into br0 */
   if(!cmsUtl_strcmp(bridgeIfName, "br0"))
   {
      cmsNet_getMacAddrByIfname(bridgeIfName, br0macAddrBefore);
   }

   /* Add LAN interface to bridge:
    * brctl addif br0 eth0;
    * ifconfig eth0 0.0.0.0;
    * bcm_sendarp -s br0 -d br0;
    * bcm_sendarp -s br0 -d eth0;
    */

   /* delete it first since it might already in the bridge
    * and the later addif will not cause any error on the console
    */

#if CMS_CONFIG_NFS
   /* Do not delete eth0 if you are NFS. otherwise the system may crash*/
   if (strcmp(ifName, "eth0") != 0)
#endif
   {
      snprintf(cmdStr, sizeof(cmdStr), "brctl delif %s %s 2>/dev/null", bridgeIfName, ifName);
      rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);
   }

   snprintf(cmdStr, sizeof(cmdStr), "brctl addif %s %s", bridgeIfName, ifName);
   rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);


#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   rut_restartRadvdForBridge(bridgeIfName);
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   rutRa_restartRadvdForBridge(bridgeIfName);
#endif

   rutLan_enableInterface(ifName);

   if (!isWanIntf)
   {
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      /* delete the link local address of the interface */
      UINT32 addrIdx = 0;
      UINT32 ifIndex;
      UINT32 prefixLen = 0;
      UINT32 scope     = 0;
      UINT32 ifaFlags  = 0;
      char   ipAddr[BUFLEN_40];

      while (cmsNet_getIfAddr6(ifName, addrIdx,
                               ipAddr, &ifIndex, &prefixLen, &scope, &ifaFlags) == CMSRET_SUCCESS)
      {
         if (strncmp(ipAddr, "fe80", 4) == 0)
         {
            /* delete the link local address */
            snprintf(cmdStr, sizeof(cmdStr), "ip -6 addr del %s/%d dev %s", ipAddr, prefixLen, ifName);
            rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);
            break;
         }
         else
         {
            /* get the next ip address of the interface */
            addrIdx++;
         }
      }
#endif

      {
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", ifName);
         rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);
      }
   }

   snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s br0 -d %s", bridgeIfName);
   rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);

   snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s br0 -d %s", ifName);
   rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
   if ((isWanIntf == TRUE) && cmsUtl_strstr(ifName, GPON_IFC_STR) != NULL)
   {
      UINT32 igmpRate = 0;
      CmsRet ret;

      ret = rutMulti_getIgmpRateLimitOnRgBridge(&igmpRate);
      if (ret == CMSRET_SUCCESS)
      {
#if defined(DMP_X_BROADCOM_COM_MCAST_1)
         int ifi = cmsNet_getIfindexByIfname(bridgeIfName);
         bcm_mcast_api_set_proto_rate_limit(-1, ifi, BCM_MCAST_PROTO_IPV4, igmpRate);
#endif
      }
   }
#endif

   /*
    * By default dhcpc was started on br0 if DHCP_CLIENT_DEFAULT is enabled.
    * We need to restart dhcpc here because br0's mac address might be changed after adding interface into br0.
   */
   if(!cmsUtl_strcmp(bridgeIfName, "br0") && qdmIpIntf_isLanDhcpcEnabled(bridgeIfName))
   {
      char br0macStrBefore[MAC_STR_LEN+1]={0};
      char br0macStrAfter[MAC_STR_LEN+1]={0};

      cmsNet_getMacAddrByIfname(bridgeIfName, br0macAddrAfter);
      cmsUtl_macNumToStr(br0macAddrAfter, br0macStrAfter);
      cmsUtl_macNumToStr(br0macAddrBefore, br0macStrBefore);
      if(cmsUtl_strcmp(br0macStrBefore, br0macStrAfter))
      {

#if defined(DMP_DEVICE2_DHCPV4_1)
         /* br0's macaddr has been changed */
         rutDhcpv4_restartDhcpv4Client_dev2(bridgeIfName);
#else
         rutLan_restartDhcpc(bridgeIfName);
#endif
      }
   }

   return;
}


void rutLan_addDynInterfaceToBridge(const char *lanIfName, const char *bridgeIfName, UBOOL8 isOVS)
{
   if (cmsUtl_strlen(lanIfName) == 0 ||
       cmsUtl_strlen(bridgeIfName) == 0)
   {
      cmsLog_error("got NULL or empty ifName (%p %d) or bridgeIfName (%p %d)",
                   lanIfName, cmsUtl_strlen(lanIfName),
                   bridgeIfName, cmsUtl_strlen(bridgeIfName));
      return;
   }

   if (isOVS == TRUE)
   {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_2
      if (rutOpenVS_isRunning() == TRUE)
      {
         rutOpenVS_startupOpenVSport(lanIfName, bridgeIfName);
      }
#endif
   }
   else
   {
      char cmdStr[BUFLEN_128];
      snprintf(cmdStr, sizeof(cmdStr), "brctl addif %s %s", bridgeIfName, lanIfName);
      rut_doSystemAction("rutLan_addInterfaceToBridge", cmdStr);
   }

   return;
}

void rutLan_disableInterface(const char *lanIfName)
{
   char cmdStr[BUFLEN_128];

   /*
    * bring LAN interface down,
    * ifconfig eth0 down.
    */
   snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down", lanIfName);
   rut_doSystemAction("rclLan_disableInterface", cmdStr);
}

void rutLan_removeInterfaceFromBridge(const char *origLanIfName, const char *bridgeIfName)
{
    rutLan_removeInterfaceFromBridgeEx(origLanIfName, bridgeIfName, FALSE);
}

void rutLan_removeInterfaceFromBridgeEx(const char *origLanIfName, const char *bridgeIfName,
                                        UBOOL8 isOVS)
{
   char cmdStr[BUFLEN_128];
#ifdef SUPPORT_LANVLAN
   char *p = NULL;
#endif
   char lanIfNameBuf[CMS_IFNAME_LENGTH]={0};
   const char *lanIfName = lanIfNameBuf;


   if (cmsUtl_strlen(origLanIfName) == 0 ||
       cmsUtl_strlen(bridgeIfName) == 0)
   {
      cmsLog_error("got NULL or empty ifName (%p %d) or bridgeIfName (%p %d)",
                   origLanIfName, cmsUtl_strlen(origLanIfName),
                   bridgeIfName, cmsUtl_strlen(bridgeIfName));
      return;
   }

#ifdef SUPPORT_LANVLAN
   if (strstr(origLanIfName, "eth") && ((p = strchr(origLanIfName, '.')) == NULL))
      snprintf(lanIfNameBuf, sizeof(lanIfNameBuf), "%s.0", origLanIfName);
   else
#endif
      lanIfName = origLanIfName;

   cmsLog_debug("lanIfName=%s bridgeIfName=%s isOVS=%s", lanIfName, bridgeIfName,
                isOVS? "TRUE" : "FALSE");

#ifdef not_needed
   /*
    * Is this code really needed?  When we move eth between LAN
    * and WAN, the rutWan_move{Eth}{LanToWan, WanToLan} functions
    * also call rutQos_qMgmtQueueOperation(ifName, TRUE) so there is no
    * need to do it here.
    * As for GPON, it is always on WAN, so only possibility is moving
    * between WAN side bridge interfaces.  Is that ever done?  And even if
    * it is done, do we really want to delete all queues?
    */
   if (strstr(lanIfName, ETH_IFC_STR) ||
      strstr(lanIfName, GPON_IFC_STR))
   {
      /* unconfig QoS queues associated with this eth or gpon interface */
      if (rutQos_qMgmtQueueOperation(lanIfName, TRUE) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueOperation(%s) returns error.", lanIfName);
      }
   }
#endif  /* not_needed */

   /* delete lan device from bridge:
    * brctl delif br0 eth0;
    * bcm_sendarp -s br0 -d br0;
    */
   if (isOVS)
   {
#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)
      if (rutOpenVS_isEnabled())
      {
          rutOpenVS_shutdownOpenVSport(lanIfName, bridgeIfName);
      }
#endif
   }
   else
   {
      snprintf(cmdStr, sizeof(cmdStr), "brctl delif %s %s", bridgeIfName, lanIfName);
      rut_doSystemAction("rclLan_removeInterfaceFromBridge", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s br0 -d %s", bridgeIfName);
      rut_doSystemAction("rutLan_removeInterfaceFromBridge", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s br0 -d %s", lanIfName);
      rut_doSystemAction("rutLan_removeInterfaceFromBridge", cmdStr);
   }

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
#if defined(DMP_X_BROADCOM_COM_MCAST_1)
   if (cmsUtl_strstr(origLanIfName, GPON_IFC_STR) != NULL)
   {
      int ifi = cmsNet_getIfindexByIfname(bridgeIfName);
      bcm_mcast_api_set_proto_rate_limit(-1, ifi, BCM_MCAST_PROTO_IPV4, 0);
   }
#endif
#endif

   return;
}


void rutLan_removeDynInterfaceFromBridge(const char *lanIfName, const char *bridgeIfName, UBOOL8 isOVS)
{
   if (cmsUtl_strlen(lanIfName) == 0 ||
       cmsUtl_strlen(bridgeIfName) == 0)
   {
      cmsLog_error("got NULL or empty ifName (%p %d) or bridgeIfName (%p %d)",
                   lanIfName, cmsUtl_strlen(lanIfName),
                   bridgeIfName, cmsUtl_strlen(bridgeIfName));
      return;
   }

   if (isOVS)
   {
#if defined(DMP_X_BROADCOM_COM_OPENVSWITCH_1) || defined(DMP_X_BROADCOM_COM_OPENVSWITCH_2)
      if (rutOpenVS_isEnabled())
      {
          rutOpenVS_shutdownOpenVSport(lanIfName, bridgeIfName);
      }
#endif
   }
   else
   {
      char cmdStr[BUFLEN_128];
      snprintf(cmdStr, sizeof(cmdStr), "brctl delif %s %s", bridgeIfName, lanIfName);
      rut_doSystemAction("rclLan_removeInterfaceFromBridge", cmdStr);
   }

   return;
}

#ifdef DMP_BASELINE_1

CmsRet rutLan_getParentBridgeIfName(MdmObjectId lanOid, const InstanceIdStack *iidStack, char *bridgeIfName)
{
   _LanHostCfgObject *lanHostCfgObj=NULL;
   _LanIpIntfObject *lanIpIntfObj=NULL;
   InstanceIdStack lanHostCfgIidStack = *iidStack;
   InstanceIdStack lanIpIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = cmsObj_getAncestor(MDMOID_LAN_HOST_CFG, lanOid, &lanHostCfgIidStack, (void **) &lanHostCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get ancestor object, ret=%d", ret);
      return ret;
   }

   /* We don't need the object, just the iidStack */
   cmsObj_free((void **) &lanHostCfgObj);

   /*
    * For now, we only support a single IP interface under the LANHostConfig,
    * but in theory, there could be more than 1 IP object.
    */
    ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF,  &lanHostCfgIidStack, &lanIpIntfIidStack, (void **) &lanIpIntfObj);
    if (ret != CMSRET_SUCCESS)
    {
      cmsLog_error("could not get IP interface/bridge object, ret=%d", ret);
    }

    sprintf(bridgeIfName, "%s", lanIpIntfObj->X_BROADCOM_COM_IfName);

    cmsObj_free((void **) &lanIpIntfObj);

    return ret;
}


CmsRet rutLan_getParentBridgeIfNameOfEth(const char *ethIfName, char *bridgeIfName)
{
   UBOOL8 found=FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _LanEthIntfObject *ethIntfObj=NULL;
   CmsRet ret;

   while ((!found) &&
          ((ret = cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, (void **) &ethIntfObj)) == CMSRET_SUCCESS))
   {
      found = (0 == cmsUtl_strcmp(ethIntfObj->X_BROADCOM_COM_IfName, ethIfName));
      cmsObj_free((void **) &ethIntfObj);
   }

   if (!found)
   {
      cmsLog_error("could not find ethIfName %s", ethIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   return (rutLan_getParentBridgeIfName(MDMOID_LAN_ETH_INTF, &iidStack, bridgeIfName));
}

#endif  /* DMP_BASELINE_1 */




void rutLan_createResolvCfg(void)
{
   char cmdStr[BUFLEN_128];
   char tmp_file[BUFLEN_32];
   int  tmp_fd;
   int rc;
   FILE *tmp_fp = NULL;

   cmsLog_debug("entered");

   mode_t cur_umask = umask(S_IRWXO|S_IRWXG);
#if defined(DESKTOP_LINUX)
   /*
    * On desktop, just write to current directory.
    */
   strncpy(tmp_file, "./dnsXXXXXX", sizeof(tmp_file));
#else
   /*
    * Do this only on the real modem.  See comments in rut_lan.h
    * On modem botup, /var/fyi/sys does not exist, so we have to
    * make the directory.  Architecturally, this mkdir should be
    * done by ssk in its system init function, but it is easier to
    * keep all this code together for now.
    */
   if (!cmsFil_isFilePresent(DNS_FYI_FILENAME))
   {
      rc = mkdir("/var/fyi", 0777);
      if ((rc < 0) && (errno != EEXIST))
      {
         cmsLog_error("mkdir() failed, %s (%d)", strerror(errno), errno);
         return;
      }

      rc = mkdir("/var/fyi/sys", 0777);
      if ((rc < 0) && (errno != EEXIST))
      {
         cmsLog_error("mkdir() failed, %s (%d)", strerror(errno), errno);
         return;
      }
   }

   /*
    * Being ultra paranoid here.  Some system app could be trying to read this
    * file while we are updating it, so the standard practice is to write to
    * a tmp file first, then do a rename.
    */
   strncpy(tmp_file, "/var/fyi/sys/dnsXXXXXX", sizeof(tmp_file));
#endif

   tmp_fd = mkstemp(tmp_file);
   umask(cur_umask);
   if (tmp_fd == -1)
   {
      cmsLog_error("could not open tmp file %s", tmp_file);
      return;
   }

   tmp_fp = fdopen(tmp_fd, "w+");
   if (tmp_fp == NULL)
   {
      cmsLog_error("could not open file stream for %s", tmp_file);
      close(tmp_fd);
      return;
   }

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   _DnsProxyCfgObject *dproxyCfg = NULL;
   InstanceIdStack dnsProxyIidStack = EMPTY_INSTANCE_ID_STACK;
   if (cmsObj_get(MDMOID_DNS_PROXY_CFG, &dnsProxyIidStack, 0, (void **) &dproxyCfg) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get DnsProxyCfgObject");
      fclose(tmp_fp);
      return;
   }
   if(dproxyCfg->enable)
   {
      char *domainName=NULL;

#ifdef SUPPORT_IPV6
      /* For dnsproxy, use loopback to redirect all dns query to it */
      snprintf(cmdStr, sizeof(cmdStr), "nameserver ::1\n");
      fputs(cmdStr, tmp_fp);
#endif

      snprintf(cmdStr, sizeof(cmdStr), "nameserver 127.0.0.1\n");
      fputs(cmdStr, tmp_fp);

      rutNtwk_getDomainName(&domainName);
      snprintf(cmdStr, sizeof(cmdStr), "domain %s\n", domainName);
      fputs(cmdStr, tmp_fp);

      CMSMEM_FREE_BUF_AND_NULL_PTR(domainName);
   }
   else
#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */
   {
      /* If dnsproxy is NOT used, just put the active default dns in
       * resolv.conf.  Also, there is no (local) domain name.
       */
      char dns1[CMS_IPADDR_LENGTH]={0};
      char dns2[CMS_IPADDR_LENGTH]={0};

      qdmDns_getActiveIpvxDnsIpLocked(CMS_AF_SELECT_IPVX, dns1, dns2);

      if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, dns1))
      {
         snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", dns1);
         fputs(cmdStr, tmp_fp);
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, dns2))
         {
            snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", dns2);
            fputs(cmdStr, tmp_fp);
         }
      }
#ifdef DMP_X_BROADCOM_COM_IPV6_1
      else
      {
         char activeDNSServers6[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
         fetchActiveDnsServers6(activeDNSServers6);
         /* got actiDNSServer6 and calling parseDNS for IPv6 addresses */
         cmsUtl_parseDNS(activeDNSServers6, dns1, dns2, FALSE);
         snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", dns1);
         fputs(cmdStr, tmp_fp);
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, dns2))
         {
            snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", dns2);
            fputs(cmdStr, tmp_fp);
         }
      }
#endif

   }

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   cmsObj_free((void **) &dproxyCfg);
#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

   fclose(tmp_fp);
   rc = rename(tmp_file, DNS_FYI_FILENAME);
   if (rc < 0)
   {
       cmsLog_error("Unable to rename file %s: %s (%d)",
       tmp_file, strerror(errno), errno);
   }
}



#ifdef DMP_BASELINE_1

void rutLan_createDhcpdCfg_igd(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 isAnyNatEnable = FALSE;
   UBOOL8 isIpExt = FALSE;
   char cmdStr[BUFLEN_128];
   char domainName[BUFLEN_32], serverIPAddr[CMS_IPADDR_LENGTH], ifName[CMS_IFNAME_LENGTH];
   char dns1[CMS_IPADDR_LENGTH]={0};
   char dns2[CMS_IPADDR_LENGTH]={0};
   LanHostCfgObject *lanHostCfg = NULL;
   LanIpIntfObject *lanIpIntfObj = NULL;
   DHCPConditionalServingObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lanIpIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack staticIpIidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   DnsProxyCfgObject *dproxyCfg = NULL;
   InstanceIdStack DnsProxyIidStack = EMPTY_INSTANCE_ID_STACK;
#endif
#ifdef DMP_BRIDGING_1 /* SUPPORT_PORT_MAP  */
   L2BridgingFilterObject *l2bridgefilter = NULL;
   InstanceIdStack FilterIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 ifIndex = 0;
   char * vendorIdString;
#endif
   char externIpAddr[BUFLEN_16];

   FILE* fs;

   cmsLog_debug("Enter.");
   cmdStr[0] = domainName[0] = serverIPAddr[0] = ifName[0] = '\0';

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &DnsProxyIidStack, 0, (void **) &dproxyCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get DnsProxyCfgObject, ret=%d", ret);
      return;
   }
   if(dproxyCfg->enable == TRUE)
   {
      strncpy(domainName, dproxyCfg->deviceDomainName, sizeof(domainName)-1);
   }
   cmsObj_free((void **) &dproxyCfg);
#endif

   if ((fs = fopen(UDHCPD_CONFIG_FILENAME, "w")) == NULL)
   {
      cmsLog_error("Failed to open %s", UDHCPD_CONFIG_FILENAME);
      return;
   }

   /* Global information */
   isAnyNatEnable = rut_isAnyNatEnabled();
   if (rutWan_IsPPPIpExtension() == TRUE)
   {
      if (rutWan_getIpExIpAddress(externIpAddr) == CMSRET_SUCCESS)
      {
         isIpExt = TRUE;
      }
   }

   /* If you change the name of this file, make sure you change it
    * everywhere by searching for UDHCPD_DECLINE macro
    */
   snprintf(cmdStr, sizeof(cmdStr), "decline_file %s\n", UDHCPD_DECLINE);
   fputs(cmdStr, fs);

   /* Per-subnet information */
   while(cmsObj_getNext(MDMOID_LAN_HOST_CFG, &iidStack, (void **) &lanHostCfg) == CMSRET_SUCCESS)
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;

      if ( lanHostCfg->DHCPServerEnable != TRUE )
      {
         cmsObj_free((void **) &lanHostCfg);
         continue;
      }

      mdmLibCtx.hideObjectsPendingDelete = TRUE;

      while (cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF,  &iidStack, &lanIpIntfIidStack, (void **) &lanIpIntfObj) == CMSRET_SUCCESS)
      {
         if (strstr(lanIpIntfObj->X_BROADCOM_COM_IfName, ":") == NULL)
         {
            strncpy(serverIPAddr, lanIpIntfObj->IPInterfaceIPAddress, sizeof(serverIPAddr)-1);
            strncpy(ifName, lanIpIntfObj->X_BROADCOM_COM_IfName, sizeof(ifName)-1);

#ifdef DMP_BRIDGING_1 /* SUPPORT_PORT_MAP  */
             /* always write out dhcp info for br0 */
             ifIndex = atoi(ifName + 2);
             if( ifIndex > 0 && rutPMap_isBridgedInterfaceGroup(ifIndex) )
             {
                 cmsObj_free((void **) &lanIpIntfObj);
                 break;
             }
#endif
            snprintf(cmdStr, sizeof(cmdStr), "interface %s\n", ifName);
            fputs(cmdStr, fs);

#ifdef DMP_BRIDGING_1 /* SUPPORT_PORT_MAP  */
            /* check interface group vendor id */
            /* Only apply the vendor id setting to br0 interface. */
            if(!cmsUtl_strcmp(lanIpIntfObj->X_BROADCOM_COM_IfName, "br0"))
            {
	            while(cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &FilterIidStack, (void **) &l2bridgefilter) == CMSRET_SUCCESS)
	            {
	               if (l2bridgefilter->sourceMACFromVendorClassIDFilter != NULL )
	               {
	                  if ((vendorIdString = cmsUtl_getDhcpVendorIdsFromAggregateString(l2bridgefilter->sourceMACFromVendorClassIDFilter)) != NULL)
	                  {
	                     char *vptr;
	                     UINT32 j;

	                     for (j=0; j < MAX_PORTMAPPING_DHCP_VENDOR_IDS; j++)
	                     {
	                        vptr = &(vendorIdString[j * (DHCP_VENDOR_ID_LEN+1)]);
	                        if (*vptr != '\0')
	                        {
	                           snprintf(cmdStr, sizeof(cmdStr), "vendor_id \"%s\"\n", vptr);
	                           fputs(cmdStr, fs);
	                        }
	                     }

	                     CMSMEM_FREE_BUF_AND_NULL_PTR(vendorIdString);
	                  }
	               }
	               cmsObj_free((void **) &l2bridgefilter);
	            }
            }
#endif
            cmsObj_free((void **) &lanIpIntfObj);
            break;
         }
         cmsObj_free((void **) &lanIpIntfObj);
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      /* there are three types of dhcpd: normal, relay, and forward
       * normal: assign private IP addresses. So the configuration information should be given, ex: start/end IP address, lease time....
       * relay: if the interface group is associated with a special routed WAN interface, we need a dhcpd relay to assign IP addresses.
       *          The configuration information should be: relay WanIpAddress.
       * forward: if the interface group is associated with a bridged WAN interface, we need dhcpd to forward the dhcp client request.
       *              The configuration information is: decline. THIS ONE IS NOT IMPLEMENTED YET!!!
       */

#ifdef DMP_BRIDGING_1 /* SUPPORT_PORT_MAP  */
        /* always write out dhcp info for br0 */
        if( ifIndex > 0 && rutPMap_isBridgedInterfaceGroup(ifIndex))
        {
           cmsObj_free((void **) &lanHostCfg);
           continue;
        }
#endif

#ifdef DHCP_RELAY
      if( lanHostCfg->X_BROADCOM_COM_DhcpRelayServer )
      {
         snprintf(cmdStr, sizeof(cmdStr), "relay %s\n", lanHostCfg->X_BROADCOM_COM_DhcpRelayServer);
         fputs(cmdStr, fs);

         cmsObj_free((void **) &lanHostCfg);
         continue;
      }
#endif

      if (isIpExt && !cmsUtl_strcmp("br0", ifName))
      {
         snprintf(cmdStr, sizeof(cmdStr), "start %s\n", externIpAddr);
         fputs(cmdStr, fs);
         snprintf(cmdStr, sizeof(cmdStr), "end %s\n", externIpAddr);
         fputs(cmdStr, fs);
      }
      else
      {
         snprintf(cmdStr, sizeof(cmdStr), "start %s\n", lanHostCfg->minAddress);
         fputs(cmdStr, fs);
         snprintf(cmdStr, sizeof(cmdStr), "end %s\n", lanHostCfg->maxAddress);
         fputs(cmdStr, fs);
      }
      /* lease is in second and need to convert to hours */
      snprintf(cmdStr, sizeof(cmdStr), "option lease %d\n", lanHostCfg->DHCPLeaseTime);
      fputs(cmdStr, fs);

      snprintf(cmdStr, sizeof(cmdStr), "remaining yes\n");
      fputs(cmdStr, fs);

      snprintf(cmdStr, sizeof(cmdStr), "min_lease 30\n");
      fputs(cmdStr, fs);

      /* subnet mask */
      snprintf(cmdStr, sizeof(cmdStr), "option subnet %s\n", lanHostCfg->subnetMask);
      fputs(cmdStr, fs);

      /* update externalIPAddress for IpExtension */
      if (isIpExt && !cmsUtl_strcmp("br0", ifName))
      {
         snprintf(cmdStr, sizeof(cmdStr), "option router %s\n", externIpAddr);
      }
      else
      {
         snprintf(cmdStr, sizeof(cmdStr), "option router %s\n", serverIPAddr);
      }
      fputs(cmdStr, fs);

      /* dns */
      strcpy(dns1, "0.0.0.0");
      strcpy(dns2, "0.0.0.0");

      /* For IpExtension, the Dns servers on br0 are set to the public Dns server addresses */
      if (isIpExt == TRUE && !cmsUtl_strcmp("br0", ifName))
      {
         qdmDns_getActiveIpvxDnsIpLocked(CMS_AF_SELECT_IPV4, dns1, dns2);

         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, dns1))
         {
            snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns1);
            fputs(cmdStr, fs);
         }
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, dns2))
         {
            snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns2);
            fputs(cmdStr, fs);
         }
      }
      else
      {
         /* use DNS relay only when NAT is enabled */
         if (isAnyNatEnable)
         {
           /* always use DSL router IP address as DNS
             * for DHCP server since we want local PCs
             * use DHCP server relay. The real DNS is
             * stored in /etc/resolv.conf
             */
            snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", serverIPAddr);
            fputs(cmdStr, fs);
         }
         else
         {
            InstanceIdStack ntwkCfgIidStack = EMPTY_INSTANCE_ID_STACK;
            NetworkConfigObject *ntwkCfgObj = NULL;

            if ((ret = cmsObj_get(MDMOID_NETWORK_CONFIG, &ntwkCfgIidStack, 0, (void **) &ntwkCfgObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get NetworkConfigObject, ret=%d", ret);
               cmsObj_free((void **) &lanHostCfg);
               return;
            }
            cmsUtl_parseDNS((char *) ntwkCfgObj->DNSServers, dns1, dns2, TRUE);
            cmsObj_free((void **) &ntwkCfgObj);

            /* use real DNS when NAT is disabled */
            /* first DNS */
            if (cmsUtl_strcmp(dns1, "0.0.0.0"))
            {
               snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns1);
            }
            else
            {
               snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", serverIPAddr);
               strncpy(dns1, serverIPAddr, sizeof(dns1)-1);
            }
            fputs(cmdStr, fs);

            /* 2nd DNS */
            if (cmsUtl_strcmp(dns2, "0.0.0.0"))
            {
               snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns2);
            }

            fputs(cmdStr, fs);

         }
      }

   #ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
      if (domainName[0] != '\0')
      {
         snprintf(cmdStr, sizeof(cmdStr), "option domain %s\n", domainName);
         fputs(cmdStr, fs);
      }
   #endif

      /* static IP lease info */
      while (cmsObj_getNextInSubTree(MDMOID_DHCP_CONDITIONAL_SERVING,  &iidStack, &staticIpIidStack, (void **) &obj) == CMSRET_SUCCESS)
      {
         if( obj->enable == TRUE )
         {
            snprintf(cmdStr, sizeof(cmdStr), "static_lease %s %s\n", obj->chaddr, obj->reservedAddresses);
            fputs(cmdStr, fs);
         }
         cmsObj_free((void **) &obj);
      }

      cmsObj_free((void **) &lanHostCfg);
   }


   fclose(fs);

   return;
}



UBOOL8 rutLan_isHostCfgChanged(const _LanHostCfgObject *newObj, const _LanHostCfgObject *currObj)
{
   UBOOL8 changed = FALSE;

   if (newObj == NULL || currObj == NULL || !newObj->DHCPServerEnable || !currObj->DHCPServerEnable)
   {
      /* this function is not applicable if the objects do not already exist and
       * are enabled. */
      return FALSE;
   }

   if (cmsUtl_strcmp(newObj->minAddress, currObj->minAddress) ||
       cmsUtl_strcmp(newObj->maxAddress, currObj->maxAddress) ||
       cmsUtl_strcmp(newObj->subnetMask,currObj->subnetMask) ||
       cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers) ||
#ifdef DHCP_RELAY
       newObj->DHCPRelay != currObj->DHCPRelay ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpRelayServer, currObj->X_BROADCOM_COM_DhcpRelayServer) ||
#endif
       newObj->DHCPLeaseTime != currObj->DHCPLeaseTime)
   {
      changed = TRUE;
   }

   return changed;

}

UBOOL8 rutLan_isIpv4AddressChanged(const _LanIpIntfObject *newObj, const _LanIpIntfObject *currObj)
{
   UBOOL8 changed = FALSE;

   if (!(POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj)))
   {
      return FALSE;
   }

   if ( cmsUtl_strcmp(newObj->IPInterfaceIPAddress, currObj->IPInterfaceIPAddress)
       || cmsUtl_strcmp(newObj->IPInterfaceSubnetMask, currObj->IPInterfaceSubnetMask)
       || cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpDefaultGateway, currObj->X_BROADCOM_COM_DhcpDefaultGateway))
   {

      changed = TRUE;
   }

   return changed;
}

UBOOL8 rutLan_isIpv4IntfChanged(const _LanIpIntfObject *newObj, const _LanIpIntfObject *currObj)
{
   UBOOL8 changed = FALSE;

   if (!(POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj)))
   {
      return FALSE;
   }

   if ( cmsUtl_strcmp(newObj->IPInterfaceIPAddress, currObj->IPInterfaceIPAddress)
       || cmsUtl_strcmp(newObj->IPInterfaceSubnetMask, currObj->IPInterfaceSubnetMask)
       || newObj->X_BROADCOM_COM_FirewallEnabled != currObj->X_BROADCOM_COM_FirewallEnabled
       || cmsUtl_strcmp(newObj->IPInterfaceAddressingType, currObj->IPInterfaceAddressingType)
       || cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpConnectionStatus, currObj->X_BROADCOM_COM_DhcpConnectionStatus)
       || cmsUtl_strcmp(newObj->X_BROADCOM_COM_DNSServers, currObj->X_BROADCOM_COM_DNSServers)
       || cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpDefaultGateway, currObj->X_BROADCOM_COM_DhcpDefaultGateway))
   {
      changed = TRUE;
   }

   return changed;
}


UBOOL8 rutLan_isIpv6IntfChanged(const _LanIpIntfObject *newObj __attribute__((unused)),
                                const _LanIpIntfObject *currObj __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   UBOOL8 changed = FALSE;

   cmsLog_debug("enter");

   if (!(POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj)))
   {
      return FALSE;
   }

   if ( newObj->X_BROADCOM_COM_FirewallEnabled != currObj->X_BROADCOM_COM_FirewallEnabled )
   {
      changed = TRUE;
   }

   return changed;
#else
   return FALSE;
#endif
}

#endif  /* DMP_BASELINE_1 */



void rutLan_getIntfStatus(const char *ifName, char *statusStr, char *hwAddr)
{
   int  socketFd;
   struct ifreq intf;
   int status = 0;

   sprintf(hwAddr,"00:00:00:00:00:00");
   sprintf(statusStr,MDMVS_DISABLED);

   if (ifName == NULL)
   {
      return;
   }

#ifdef DESKTOP_LINUX
   /* just return bogus value for Desktop Linux */
   sprintf(hwAddr,"11:22:33:44:55:66");
   sprintf(statusStr,MDMVS_UP);
   return;
#endif


   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      cmsLog_error("could not open socket");
      return;
   }

   {
      strncpy(intf.ifr_name, ifName, sizeof(intf.ifr_name)-1);
      intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';
      intf.ifr_data = (char*)&status;
      if (ioctl(socketFd, SIOCGLINKSTATE, &intf) != -1)
      {
         status = *(int*)(intf.ifr_data);
      }
      if (ioctl(socketFd, SIOCGIFHWADDR, &intf) != -1)
      {
         cmsUtl_macNumToStr((UINT8 *) intf.ifr_hwaddr.sa_data, hwAddr);
      }
      close(socketFd);
   }

   if (status)
   {
      sprintf(statusStr,MDMVS_UP);
   }

   return;
}


void rutLan_getIntfMacAddr(const char *ifName, UINT8 *hwAddr)
{
   int  socketFd;
   struct ifreq intf;

   memset(hwAddr, 0, MAC_ADDR_LEN);

   if (ifName == NULL)
   {
      return;
   }

   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      cmsLog_error("could not open socket");
      return;
   }
   else
   {
      strncpy(intf.ifr_name, ifName, sizeof(intf.ifr_name)-1);
      intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';

      if (ioctl(socketFd, SIOCGIFHWADDR, &intf) != -1)
      {
         memcpy(hwAddr, (UINT8 *) intf.ifr_hwaddr.sa_data, MAC_ADDR_LEN);
      }
      close(socketFd);
   }
}


#ifdef DMP_BASELINE_1

UBOOL8 rutLan_findFirstDhcpcAndConnected_igd(char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   LanIpIntfObject *lanIpIntfObj = NULL;
   UBOOL8 found = FALSE;

   while (!found &&
          (CMSRET_SUCCESS == cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lanIpIntfObj)))
   {
      if (!cmsUtl_strcmp(lanIpIntfObj->IPInterfaceAddressingType, MDMVS_DHCP) &&
          !cmsUtl_strcmp(lanIpIntfObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_CONNECTED))
      {
         found = TRUE;
         strcpy(ifName, lanIpIntfObj->X_BROADCOM_COM_IfName);
      }
      cmsObj_free((void **)&lanIpIntfObj);
   }

   return found;
}


UBOOL8 rutLan_isDhcpdEnabled_igd()
{
   UBOOL8 dhcpdEnabled = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   LanHostCfgObject *lanHostCfg = NULL;

   while (!dhcpdEnabled && cmsObj_getNext(MDMOID_LAN_HOST_CFG, &iidStack, (void **) &lanHostCfg) == CMSRET_SUCCESS)
   {
      if (lanHostCfg->DHCPServerEnable == TRUE)
      {
         dhcpdEnabled = TRUE;
      }
      cmsObj_free((void **) &lanHostCfg);
   }

   cmsLog_debug("dhcpd eanbled: %s", dhcpdEnabled ? "yes": "no");

   return dhcpdEnabled;

}

#endif  /* DMP_BASELINE_1 */


CmsRet rutLan_updateDhcpd(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 pid;

   if (rutLan_isDhcpdEnabled() == FALSE ||
       qdmIpIntf_isAllBridgeWanServiceLocked() == TRUE)
   {
      /* no dhcpd needed in the system */
      cmsLog_debug("No dhcpd needed: not enabled or all bridge pvc's");

      if (rut_isApplicationActive(EID_DHCPD) == TRUE)
      {
         cmsLog_notice("send stop dhcpd msg to smd");
         if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_DHCPD, NULL, 0)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to stop dhcpd");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Dhcpd stopped");
         }
      }
   }
   else
   {
      /* need dhcpd runing */
      cmsLog_debug("Need dhcpd");

      rutLan_createDhcpdCfg();

      if (rut_isApplicationActive(EID_DHCPD) == FALSE)
      {
         cmsLog_notice("send restart dhcpd msg to smd");
         if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DHCPD,  NULL, 0)) == CMS_INVALID_PID)
         {
            cmsLog_error("failed to restart dhcpd");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Dhcpd started with pid=%d", pid);
         }
      }
      else
      {
         rut_sendReloadMsgToDhcpd();
      }
   }

   cmsLog_debug("Exit with ret=%d", ret);

   return ret;
}


#ifdef DMP_BASELINE_1

void rutLan_reconfigNatForAddressChange(const char *bridgeIfName,
                                        const char *oldIpAddr, const char *oldMask,
                                        const char *newIpAddr, const char *newMask)
{
   char oldSubnet[CMS_IPADDR_LENGTH];
   char newSubnet[CMS_IPADDR_LENGTH];
   WanIpConnObject *ipConnObj = NULL;
   WanPppConnObject *pppConnObj = NULL;
   struct in_addr ip, mask, subnet;
   InstanceIdStack iidStack;
   CmsRet ret = CMSRET_SUCCESS;
#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
   char groupName[BUFLEN_32]={0};
   char groupBridgeName[BUFLEN_16]={0};
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */

   cmsLog_debug("bridgeIfName=%s oldIpAddr/mask=%s/%s newIpAddr/mask=%s/%s",
     bridgeIfName, oldIpAddr, oldMask, newIpAddr, newMask);

   /* got this from rut_getIfSubnet */
   inet_aton(oldIpAddr, &ip);
   inet_aton(oldMask, &mask);
   subnet.s_addr = ip.s_addr & mask.s_addr;
   sprintf(oldSubnet, "%s", inet_ntoa(subnet));

   inet_aton(newIpAddr, &ip);
   inet_aton(newMask, &mask);
   subnet.s_addr = ip.s_addr & mask.s_addr;
   sprintf(newSubnet, "%s", inet_ntoa(subnet));


   /*
    * mwang_todo: This algorithm needs to check if the WAN device is part of an
    * interface group.  We don't support that feature right now, so this is OK.
    * When we do add support for routed WAN on interface group, then this needs to
    * be fixed also.
    */

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipConnObj->connectionStatus, MDMVS_CONNECTED) &&
          cmsUtl_strcmp(ipConnObj->connectionType, MDMVS_IP_BRIDGED))
      {
         if (ipConnObj->NATEnabled)
         {
             cmsLog_debug("reconfig %s", ipConnObj->X_BROADCOM_COM_IfName);
             rutIpt_deleteNatMasquerade(ipConnObj->X_BROADCOM_COM_IfName, oldSubnet, oldMask);
             rutIpt_insertNatMasquerade(ipConnObj->X_BROADCOM_COM_IfName, newSubnet, newMask);
         }
#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
         /* the WanIf associated with the Bridge or this is br0 */
         if (((rutPMap_isWanUsedForIntfGroup(ipConnObj->X_BROADCOM_COM_IfName) == TRUE) &&
              (rutPMap_getGroupAndBridgeNameFromWanIfName(ipConnObj->X_BROADCOM_COM_IfName,
               groupName, groupBridgeName) == CMSRET_SUCCESS) &&
              (!cmsUtl_strcmp(groupBridgeName, bridgeIfName))) ||
              (!cmsUtl_strcmp(bridgeIfName, "br0")))

         {
             cmsLog_debug("update group: %s", groupName);
             rutIpt_disassociateOtherBridgesFromWanIntf('D', PF_INET, ipConnObj->X_BROADCOM_COM_IfName, oldSubnet, oldMask);
             rutIpt_disassociateOtherBridgesFromWanIntf('A', PF_INET, ipConnObj->X_BROADCOM_COM_IfName, newSubnet, newMask);
         }
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */
      }

      cmsObj_free((void **) &ipConnObj);
   }


   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(pppConnObj->connectionStatus, MDMVS_CONNECTED))
      {
          if (pppConnObj->NATEnabled)
          {
              cmsLog_debug("reconfig %s", pppConnObj->X_BROADCOM_COM_IfName);
              rutIpt_deleteNatMasquerade(pppConnObj->X_BROADCOM_COM_IfName, oldSubnet, oldMask);
              rutIpt_insertNatMasquerade(pppConnObj->X_BROADCOM_COM_IfName, newSubnet, newMask);
          }
#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
         if (((rutPMap_isWanUsedForIntfGroup(pppConnObj->X_BROADCOM_COM_IfName) == TRUE) &&
              (rutPMap_getGroupAndBridgeNameFromWanIfName(pppConnObj->X_BROADCOM_COM_IfName,
               groupName, groupBridgeName) == CMSRET_SUCCESS) &&
              (!cmsUtl_strcmp(groupBridgeName, bridgeIfName))) ||
              (!cmsUtl_strcmp(bridgeIfName, "br0")))
         {
             cmsLog_debug("update group: %s", groupName);
             rutIpt_disassociateOtherBridgesFromWanIntf('D', PF_INET, pppConnObj->X_BROADCOM_COM_IfName, oldSubnet, oldMask);
             rutIpt_disassociateOtherBridgesFromWanIntf('A', PF_INET, pppConnObj->X_BROADCOM_COM_IfName, newSubnet, newMask);
         }
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */
      }

      cmsObj_free((void **) &pppConnObj);
   }

   return;
}

CmsRet rutLan_getLanDevByBridgeIfName(const char *brIfName, InstanceIdStack *iidStack, LanDevObject **lanDev)
{
    LanIpIntfObject *ipIntfObj=NULL;
    UBOOL8 found=FALSE;
    CmsRet ret;

    INIT_INSTANCE_ID_STACK(iidStack);

    while (!found &&
            (ret = cmsObj_getNext(MDMOID_LAN_IP_INTF, iidStack, (void **) &ipIntfObj)) == CMSRET_SUCCESS)
    {
        cmsLog_debug("brIfName:%s, ifname:%s\n",brIfName,ipIntfObj->X_BROADCOM_COM_IfName);
        if (!strcmp(ipIntfObj->X_BROADCOM_COM_IfName, brIfName))
        {
            cmsLog_debug("found \n");
            found = TRUE;

        }

        cmsObj_free((void **) &ipIntfObj);
    }

    if (found)
    {
        cmsLog_debug("\n");
        ret = cmsObj_getAncestor(MDMOID_LAN_DEV, MDMOID_LAN_IP_INTF, iidStack, (void **) lanDev);
        cmsLog_debug("get lanDev:ret:%d\n",ret);
    }
    else
    {
        cmsLog_error("could not find brIfName %s", brIfName);
    }

    return ret;
}


#endif /* DMP_BASELINE_1 */


#ifdef DMP_BASELINE_1
CmsRet rutLan_getHostEntryByMacAddr(const InstanceIdStack *parentIidStack __attribute__((unused)), const char *macAddr __attribute__((unused)), InstanceIdStack *iidStack __attribute__((unused)), LanHostEntryObject **hostEntry __attribute__((unused)))
{
    UBOOL8 found=FALSE;
    CmsRet ret=CMSRET_SUCCESS;

    INIT_INSTANCE_ID_STACK(iidStack);

    while (!found &&
            ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_LAN_HOST_ENTRY, parentIidStack, iidStack, OGF_NO_VALUE_UPDATE, (void **) hostEntry)) == CMSRET_SUCCESS))
    {
        cmsLog_debug("hostEntry mac:%s and amcadd:%s\n",(*hostEntry)->MACAddress,macAddr);
        if (!cmsUtl_strcasecmp((*hostEntry)->MACAddress, macAddr))
        {
            cmsLog_debug("found host entry with mac=%s", (*hostEntry)->MACAddress);
            cmsLog_debug(" found host\n");
            found = TRUE;
            cmsLog_debug("ret:%d\n",ret);
        }
        else
        {
            cmsObj_free((void **) hostEntry);
        }
    }
    if(found==FALSE) *hostEntry=NULL;
    cmsLog_debug("ret:%d\n",ret);
    return ret;
}
#else
CmsRet rutLan_getHostEntryByMacAddr(const InstanceIdStack *parentIidStack __attribute__((unused)), const char *macAddr __attribute__((unused)), InstanceIdStack *iidStack __attribute__((unused)), LanHostEntryObject **hostEntry __attribute__((unused)))
{
    return CMSRET_OBJECT_NOT_FOUND;
}
#endif


#ifdef DMP_X_BROADCOM_COM_DLNA_1
CmsRet rutLan_updateDlna_igd(void)
{
   InstanceIdStack dmsIidStack;
   DmsCfgObject *dmsObj = NULL;
#ifdef DMP_BRIDGING_1
   InstanceIdStack brIidStack;
   _L2BridgingEntryObject *bridgeObj = NULL;
#endif
   UBOOL8 found=TRUE;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(&dmsIidStack);
   if ((ret = cmsObj_get(MDMOID_DMS_CFG, &dmsIidStack, 0, (void **)&dmsObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_DMS_CFG> returns error. ret=%d", ret);
      return ret;
   }

#ifdef DMP_BRIDGING_1
   INIT_INSTANCE_ID_STACK(&brIidStack);
   if ((ret = rutPMap_getBridgeByKey((UINT32) dmsObj->brKey, &brIidStack, &bridgeObj)) != CMSRET_SUCCESS)
   {
       found=FALSE;
   }
   cmsObj_free((void **)&bridgeObj);
#endif

   if (!found)
   {
       /* disable the DLNA due to the bridge interface isn't existed. */
       dmsObj->brKey = 0 ;
       dmsObj->enable = 0 ;
       ret = cmsObj_set(dmsObj, &dmsIidStack);
       cmsObj_free((void **) &dmsObj);
       if (ret != CMSRET_SUCCESS)
       {
          cmsLog_error("could not set DLNA cfg, ret=%d", ret);
          return ret;
       }
   }
   else
   {
       cmsObj_free((void **)&dmsObj);
   }

   cmsLog_debug("Exit with ret=%d", ret);

   return ret;
}
#endif

#if defined (SUPPORT_DM_PURE181) || (SUPPORT_DM_DETECT)
#include "device2/rut2_bridging.h"
#endif

void rutLan_addDynIntfToBridge(const char *intfName,
                               const char *brIntfName)
{
#if defined(SUPPORT_DM_LEGACY98)
   rutLan_addInterfaceToBridge(intfName, FALSE, brIntfName);
#elif defined(SUPPORT_DM_HYBRID)
   rutLan_addInterfaceToBridge(intfName, FALSE, brIntfName);
#elif defined(SUPPORT_DM_PURE181)
   rutBridge_addDynIntfToBridge_dev2(intfName, brIntfName);
#elif defined(SUPPORT_DM_DETECT)
   if (cmsMdm_isDataModelDevice2())
      rutBridge_addDynIntfToBridge_dev2(intfName, brIntfName);
   else
      rutLan_addInterfaceToBridge(intfName, FALSE, brIntfName);
#endif
}


void rutLan_deleteDynIntfFromBridge(const char *intfName,
                                       const char *brIntfName)
{
#if defined(SUPPORT_DM_LEGACY98)
   rutLan_removeInterfaceFromBridge(intfName, brIntfName);
#elif defined(SUPPORT_DM_HYBRID)
   rutLan_removeInterfaceFromBridge(intfName, brIntfName);
#elif defined(SUPPORT_DM_PURE181)
   rutBridge_deleteDynIntfFromBridge_dev2(intfName, brIntfName);
#elif defined(SUPPORT_DM_DETECT)
   if (cmsMdm_isDataModelDevice2())
      rutBridge_deleteDynIntfFromBridge_dev2(intfName, brIntfName);
   else
      rutLan_removeInterfaceFromBridge(intfName, brIntfName);
#endif
}

