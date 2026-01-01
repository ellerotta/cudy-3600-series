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

#include <net/if.h>  /* for IFNAM_SIZE */
#include <linux/if_ether.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_wan.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_route.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_dsl.h"
#include "rut_ethintf.h"
#include "rut_system.h"
#include "rut_network.h"
#include "rut_qos.h"
#include "rut_wanlayer2.h"
#if defined(DMP_X_BROADCOM_COM_GPONWAN_1)
#include "rut_gponwan.h"
#endif // DMP_X_BROADCOM_COM_GPONWAN_1

#ifdef SUPPORT_VLANCTL
#ifdef SUPPORT_VLANSUBIF
#include "vlan_subif.h"
#else
#error This file requires SUPPORT_VLANSUBIF
#endif
#endif

#include "rut_eponwan.h"
#include "rut_iptunnel.h"
#include "rut_wifiwan.h"
#include "rut_ebtables.h"
#include "rut_dhcp.h"
#ifdef BUILD_CUSTOMER
#include "rut_customer.h"
#endif

#define RDPA_WAN_TYPE_PSP_KEY     "RdpaWanType"

#if defined(BRCM_PKTCBL_SUPPORT)
/*  EMTA */
#define DHCP_OPTION_REPORT_LIST_EMTA  "43_60_125" /* Pls connect multi codes with "_" */
#define DHCP_OPTION_REQUEST_LIST_EMTA  "7_42_122_125"  /* Pls connect multi codes with "_" */
#endif // BRCM_PKTCBL_SUPPORT

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
/*  EPTA */
#define DHCP_OPTION_REPORT_LIST_EPTA  "43_125" /* Pls connect multi codes with "_" */
#define DHCP_OPTION_REQUEST_LIST_EPTA  "42_125"  /* Pls connect multi codes with "_" */
#endif // DMP_X_BROADCOM_COM_EPONWAN_1

SINT32 rut_wanGetIntfIndex(const char *ifcName)
{
   struct ifreq ifr;
   SINT32 s = 0;

   if (ifcName == NULL)
      return -1;

   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      return -1;
      
   strncpy(ifr.ifr_name, ifcName, sizeof(ifr.ifr_name)-1);
   ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
      
   if (ioctl(s, SIOCGIFINDEX, &ifr) < 0)
   {
      close(s);
      return 0;
   }
   close(s);
   return ifr.ifr_ifindex;
}



UBOOL8 rut_waitIntfExists(const char *ifName)
{
#ifdef DESKTOP_LINUX
   return 1;
#endif

   SINT32 retry = 0;
   SINT32 ret;

   while (retry < INTERFACE_RETRY_COUNT)
   {
      if ((ret = rut_wanGetIntfIndex(ifName)) <= 0)
      {
         usleep(USLEEP_COUNT);
         cmsLog_notice("not exist,retry %d, ret %d\n", retry, ret);
         retry++;
      }
      else
      {
         return 1;
      }
   } /* while */
   return 0;
}



CmsRet rutWan_getL3InterfaceInfo(MdmObjectId wanConnOid,
                                 const InstanceIdStack *iidStack,
                                 SINT32 vlanMuxId __attribute__((unused)),
                                 SINT32 connId,
                                 const char *L3IfName,
                                 char *l2IfName,
                                 char *baseL3IfName,
                                 ConnectionModeType *connMode)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter");

   /* Need to get layer 2 interface name, connMode  */
   if ((ret = rutWl2_getL2IfName(wanConnOid, iidStack, l2IfName)) != CMSRET_SUCCESS)
   {
      return ret;
   }
   *connMode = rutWl2_getConnMode(wanConnOid, iidStack);

   /** For bridge/Ipoe, baseL3IfName is same as layer 3 ifName. For PPPoE, in vlan and msc mode,
    * the layer 3 interface name looks like ppp0.100 (vlan) and base layer 3 interface name looks
    * like ptm0.100 which is based on the base layer 2 vlan interface ptm0.
    */
   if (wanConnOid == MDMOID_WAN_PPP_CONN)
   {
      if (*connMode == CMS_CONNECTION_MODE_VLANMUX)
      {
         sprintf(baseL3IfName, "%s.%d", l2IfName, connId);
      }
      else
      {
         strcpy(baseL3IfName, l2IfName);
      }
   }
   else if (wanConnOid == MDMOID_WAN_IP_CONN)
   {
      /* For IPoE/bridge, baseL3IfName (atm0_1, atm1.100, atm3) is same as layer 3 ifName.
      */
      strcpy(baseL3IfName, L3IfName);
   }

   return ret;

}

#define SLOW_PROTOCOL_ETHER_TYPE 0x8809

CmsRet rutWan_startL3Interface(SINT32 vlanMuxId __attribute__((unused)),
                              SINT32 vlanMuxPr __attribute((unused)),
                              UINT32 vlanTpid __attribute((unused)),
                              char * l2IfName,
                              char * baseL3IfName,
                              ConnectionModeType connMode,
                              UBOOL8 isBridge  __attribute((unused)))
{
   char cmdStr[BUFLEN_128];
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("rutWan_startL3Interface: vlanId=%d, vlan801p=%d, vlanTpid=0x%X,  L2=%s BaseL3=%s\n",
      vlanMuxId, vlanMuxPr, vlanTpid, l2IfName,baseL3IfName);
   if (rut_wanGetIntfIndex(baseL3IfName) > 0)
   {
      cmsLog_debug("rutWan_startL3Interface: Intf already exist <%s>\n",baseL3IfName);
      return ret;
   }

   switch (connMode)
   {
   case CMS_CONNECTION_MODE_DEFAULT:
      /* do nothing for default connction mode */
      break;

   case CMS_CONNECTION_MODE_VLANMUX:
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANSUBIF)
   {
      vlansubif_options_t vlan_subif_options = {
         .name = baseL3IfName,
         .pbit = vlanMuxPr,
         .tpid = vlanTpid,
         .is_phyif_other_owned = 0
      };

      /* Create untagged virtual interface */
 #ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
      if (!cmsUtl_strstr(l2IfName, WLAN_IFC_STR))
 #endif
         vlan_subif_options.is_routed = !isBridge;

      /* Create VLAN sub-interface */
      vlanSubif_createVlanInterface(l2IfName, vlanMuxId, &vlan_subif_options);

      if (!rut_waitIntfExists(baseL3IfName))
      {
         cmsLog_error("Failed to create %s", baseL3IfName);
         return CMSRET_INTERNAL_ERROR;
      }
   }
#endif /* #if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANSUBIF) */
      break;

   default:
      cmsLog_error("Wrong connMode %d", connMode);
      return CMSRET_INTERNAL_ERROR;
   }

   /* if layer 3 IfName differs from layer 2 ifName, do a "ifconfig L3IfName up" */
   if (cmsUtl_strcmp(l2IfName, baseL3IfName))
   {
      if (rut_waitIntfExists(baseL3IfName))
      {
         if (!isBridge)
         {
            snprintf(cmdStr, sizeof(cmdStr), "echo 1 > /proc/sys/net/ipv4/conf/%s/arp_filter", baseL3IfName);
            rut_doSystemAction("rutWan_startL3Interface: echo 1 > L3IfName arp_filter", (cmdStr));
#ifdef SUPPORT_IPV6
            snprintf(cmdStr, sizeof(cmdStr), "echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", baseL3IfName);
            rut_doSystemAction("rutWan_startL3Interface: echo 2 > L3IfName accept_ra", (cmdStr));
#endif
         }

         /* bring interface up */
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", baseL3IfName);
         rut_doSystemAction("rutWan_startL3Interface: ifconfig L3IfName up", (cmdStr));
      }
      else
      {
         cmsLog_error("L2IfName %s is not up", baseL3IfName);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;

}


#ifdef DMP_BASELINE_1

CmsRet rutWan_stopL3Interface(MdmObjectId wanConnOid,
                              const InstanceIdStack *iidStack,
                              SINT32 vlanMuxId,
                              SINT32 connId,
                              const char *L3IfName)
{
   char cmdStr[BUFLEN_128];
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter.");

   if ((ret = rutWan_getL3InterfaceInfo(wanConnOid,
                                        iidStack,
                                        vlanMuxId,
                                        connId,
                                        L3IfName,
                                        l2IfName,
                                        baseL3IfName,
                                        &connMode)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWan_getL3InterfaceInfo failed. ret=%d", ret);
      return ret;
   }

   cmsLog_debug("l2IfName=%s, baseL3IfName=%s ", l2IfName, baseL3IfName);
   if (rutWl2_isWanLayer2DSL(wanConnOid, iidStack))
   {
      /*
      * For xDSL case when in msc and vlan modes, L2IfName looks like atm0 and L3Ifname atm0.100.
      * If layer 3 IfName differs from layer 2 ifName, do a "ifconfig L3IfName down".
      */
      if (cmsUtl_strcmp(l2IfName, baseL3IfName))
      {
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down", baseL3IfName);
         rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName down", (cmdStr));
      }
   }

   /* Need to remove the network from the routing table by
   * doing  "ifconfig L3IfName 0 0.0.0.0"
   */
   if (rut_wanGetIntfIndex(baseL3IfName) > 0)
   {
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", baseL3IfName);
      rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName 0.0.0.0", (cmdStr));

      switch (connMode)
      {
      case CMS_CONNECTION_MODE_DEFAULT:
         /* action below -- ifconfig L3IfName down */
         break;

      case CMS_CONNECTION_MODE_VLANMUX:
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANSUBIF)
         /* Delete the virtual interface. All rules associated with it will be purged. */
         vlanSubif_deleteVlanInterface(baseL3IfName);
#endif
         break;

      default:
         cmsLog_error("Wrong connMode %d", connMode);
         ret = CMSRET_INTERNAL_ERROR;
      }
   } /* if there is such an interface */

   return ret;

}



CmsRet rutWan_deleteWanIpOrPppConnection(const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;

   /* Delete all the Qos classes associated with this layer 3 interface */
   if ((ret = rutQos_qMgmtClassDelete(ifName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_qMgmtClassDelete returns error. ret=%d", ret);
   }

   /* Deactivate the tunnels associated with this connection */
   rutTunnel_control(ifName, NULL, FALSE);

   rutRt_removeDefaultGatewayIfUsed(ifName);

   rutNtwk_removeIpvxDnsIfNameFromList(CMS_AF_SELECT_IPVX, ifName);

   /* Remove L3 forwarding object -- static routes which use this interface */
   rutRt_removeL3ForwardingEntry(ifName);

   rutQos_doDefaultPolicy();  /* TODO: for jeff, what about removing iptables associated with the wan in tearDown ? */

   return ret;

}

#endif  /* DMP_BASELINE_1 */


CmsRet rutWan_getBcastStrAndSubnetFromIpAndMask(char *InIpAddressStr,
                                                char *InMaskStr,
                                                char *outBCastStr,
                                                char *outSubnetStr)
{
   struct in_addr ip, mask, subnet;

   if (rut_getBCastFromIpSubnetMask(InIpAddressStr, InMaskStr, outBCastStr) != CMSRET_SUCCESS)
   {
      cmsLog_error("bad address %s/%s", InIpAddressStr, InMaskStr);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (!inet_aton(InIpAddressStr, &ip) || !inet_aton(InMaskStr, &mask) || outSubnetStr == NULL)
   {
      cmsLog_error("Invalid strings:  ipAddress %s, mask %s or subnet %s", InIpAddressStr, InMaskStr, outSubnetStr);
      return CMSRET_INVALID_ARGUMENTS;
   }

   subnet.s_addr = ip.s_addr & mask.s_addr;
   strcpy(outSubnetStr, inet_ntoa(subnet));

   return CMSRET_SUCCESS;

}


CmsRet rutWan_activateIpEthInterface(_WanIpConnObject *newObj)
{
   char cmdStr[BUFLEN_128 + CMS_IPADDR_LENGTH];
   char bCastStr[CMS_IPADDR_LENGTH];
   char subnetStr[CMS_IPADDR_LENGTH];

   if ((rutWan_getBcastStrAndSubnetFromIpAndMask(newObj->externalIPAddress,
                                                 newObj->subnetMask,
                                                 bCastStr,
                                                 subnetStr)) != CMSRET_SUCCESS)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }


   /* setup static mer/IPoW : such as
    * "ifconfig nas_0_37 10.6.33.165 netmask 255.255.255.192 broadcast 10.6.33.191 up"
    * "ifconfig eth0 10.6.33.165 netmask 255.255.255.192 broadcast 10.6.33.191 up"
    */
   snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s netmask %s broadcast %s up",
      newObj->X_BROADCOM_COM_IfName, newObj->externalIPAddress, newObj->subnetMask, bCastStr);
   rut_doSystemAction("rcl", cmdStr);

   /* for static MER/IPoW, need to add the next hop -- eg.  route add -net 10.6.33.129 netmask 255.255.255.192 metric 1 gw 10.6.33.129 */
   if (strcmp(newObj->addressingType, MDMVS_STATIC) == 0)
   {
      snprintf(cmdStr, sizeof(cmdStr), "route add -net %s netmask %s metric 1 gw %s",
         subnetStr, newObj->subnetMask, newObj->defaultGateway);
      rut_doSystemAction("rcl", cmdStr);
   }

   snprintf(cmdStr, sizeof(cmdStr), "bcm_sendarp -s %s -d %s", "br0", "br0");
   rut_doSystemAction("rcl", cmdStr);

   return CMSRET_SUCCESS;

}


SINT32 rutWan_startDhcpc
   (const char *ifName,
    const char *vid,
    const char *duid,
    const char *iaid,
    const char *uid,
    UBOOL8 op125,
    const char *ipAddress,
    const char *serverIpAddress,
    const char *leasedTime,
    UBOOL8 op212)
{
   char cmdLine[BUFLEN_1024]={0};
   char ouiBuf[BUFLEN_64+1]={0};
   char serialNumBuf[BUFLEN_16+1]={0};
   char productBuf[BUFLEN_64+1]={0};
   SINT32 pid=CMS_INVALID_PID;
   CmsRet      ret;

   if (!cmsNet_isInterfaceExist(ifName))
      return pid;

   if (op212)
      snprintf(cmdLine, sizeof(cmdLine), "-6 -i %s", ifName);
   else
      snprintf(cmdLine, sizeof(cmdLine), "-i %s", ifName);

   if (vid != NULL && strlen(vid) != 0)
      sprintf(&cmdLine[strlen(cmdLine)], " -d %s", vid);
   if (duid != NULL && strlen(duid) != 0)
      sprintf(&cmdLine[strlen(cmdLine)], " -D %s", duid);
   if (iaid != NULL && strlen(iaid) != 0)
      sprintf(&cmdLine[strlen(cmdLine)], " -I %s", iaid);
   if (uid != NULL && strlen(uid) != 0)
      sprintf(&cmdLine[strlen(cmdLine)], " -u %s", uid);
#ifdef BUILD_CUSTOMER
   rutDhcp_setWanDHCPCustomerOption(ifName, cmdLine);
#endif
   if (op125)
   {
      ret = rutWan_getDhcpDeviceInfo(ouiBuf, serialNumBuf, productBuf);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get device info object!, ret=%d", ret);
      }
      else
      {
         if (!IS_EMPTY_STRING(ouiBuf))
            sprintf(&cmdLine[strlen(cmdLine)], " -O %s", ouiBuf);
         if (!IS_EMPTY_STRING(serialNumBuf))
            sprintf(&cmdLine[strlen(cmdLine)], " -S %s", serialNumBuf);
         if (!IS_EMPTY_STRING(productBuf))
            sprintf(&cmdLine[strlen(cmdLine)], " -P %s", productBuf);}
   }
   if (!IS_EMPTY_STRING(ipAddress))
      sprintf(&cmdLine[strlen(cmdLine)], " -r %s", ipAddress);
   if (!IS_EMPTY_STRING(serverIpAddress))
      sprintf(&cmdLine[strlen(cmdLine)], " -a %s", serverIpAddress);
   if (!IS_EMPTY_STRING(leasedTime) != 0)
   {
      UINT32 lease = 0;
      sscanf(leasedTime, "%d", &lease);
      if (lease != 0)
         sprintf(&cmdLine[strlen(cmdLine)], " -l %s", leasedTime);
   }

    cmsDhcp_mkCfgDir(DHCP_V4, ifName);
#if defined(BRCM_PKTCBL_SUPPORT)
#if defined(SUPPORT_DM_PURE181)
   char voiceIf[BUFLEN_64] = { 0 };
   rutWan_getVoiceBoundIfName(voiceIf, sizeof(voiceIf)); // support tr181 only
   if (strcmp(ifName, voiceIf) == 0)
   {
      rutDhcp_createOption43(WAN_PKTCBL_EMTA, ifName, duid);
      rutDhcp_createOption60(ifName);
      rutDhcp_createOption125(ifName);
      sprintf(&cmdLine[strlen(cmdLine)], " -R %s", DHCP_OPTION_REPORT_LIST_EMTA);
      sprintf(&cmdLine[strlen(cmdLine)], " -Q %s", DHCP_OPTION_REQUEST_LIST_EMTA);
      sprintf(&cmdLine[strlen(cmdLine)], " -K");
   }
#endif // SUPPORT_DM_PURE181
#endif // BRCM_PKTCBL_SUPPORT

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
   if (strcmp(ifName, epon_epta_wan_name()) == 0)
   {
      rutDhcp_createOption43(WAN_EPON_EPTA, ifName, duid);
      rutDhcp_createOption125(ifName);
      sprintf(&cmdLine[strlen(cmdLine)], " -R %s", DHCP_OPTION_REPORT_LIST_EPTA);
      sprintf(&cmdLine[strlen(cmdLine)], " -Q %s", DHCP_OPTION_REQUEST_LIST_EPTA);
      sprintf(&cmdLine[strlen(cmdLine)], " -K");
   }
#endif // DMP_X_BROADCOM_COM_EPONWAN_1

   if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_DHCPC, cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or start dhcpc on %s", ifName);
   }
   else
   {
      cmsLog_debug("starting dhcpc, pid=%d on %s", pid, ifName);
   }

   return pid;
}




#ifdef DMP_BASELINE_1

CmsRet rutWan_getDhcpDeviceInfo_igd(char *oui, char *serialNum, char *productClass)
{
   IGDDeviceInfoObject *deviceInfoObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get device info object!, ret=%d", ret);
   }
   else
   {
      if (oui && deviceInfoObj->manufacturerOUI)
         sprintf(oui, "%s", deviceInfoObj->manufacturerOUI);
      if (serialNum && deviceInfoObj->serialNumber)
         sprintf(serialNum, "%s", deviceInfoObj->serialNumber);
      if (productClass && deviceInfoObj->productClass)
         sprintf(productClass, "%s", deviceInfoObj->productClass);

      cmsObj_free((void**)&deviceInfoObj);
   }

   return ret;
}

CmsRet rutWan_initPPPoE_igd(const InstanceIdStack *iidStack,  void *obj)
{
   SINT32 pppPid;
   char cmdLine[BUFLEN_256];
   char serverFlag[BUFLEN_64];
   char staticIPAddrFlag[BUFLEN_32];
   char passwordFlag[BUFLEN_32];
   passwordFlag[0] = serverFlag[0] = staticIPAddrFlag[0] = '\0';
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;
   char idlelimit[BUFLEN_32] = {0};
   _WanPppConnObject *newObj = (_WanPppConnObject *) obj;

   /* Need baseL3IfName for PPPoE */
   if ((ret = rutWan_getL3InterfaceInfo(MDMOID_WAN_PPP_CONN,
                                        iidStack,
                                        newObj->X_BROADCOM_COM_VlanMuxID,
                                        newObj->X_BROADCOM_COM_ConnectionId,
                                        newObj->X_BROADCOM_COM_IfName,
                                        l2IfName,
                                        baseL3IfName,
                                        &connMode)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWan_getL3InterfaceInfo failed. ret=%d", ret);
      return ret;
   }

   /* password is an optional parameter */
   if (newObj->password && newObj->password[0] != '\0')
   {
      snprintf(passwordFlag, sizeof(passwordFlag), "-p %s", newObj->password);
   }

   /*
    *  server name (on web display it is service name)
    * The -r argument is passed to pppoe.  So it cannot be an empty string,
    * and it cannot be a string containing any white spaces.  The check below
    * only guards against empty string, but it should also guard against white
    * space.  So far, this has not been a problem.
    */
   //rp_pppoe_service
   if ((newObj->PPPoEServiceName != NULL) && (strlen(newObj->PPPoEServiceName) > 0))
   {
      snprintf(serverFlag, sizeof(serverFlag), "rp_pppoe_service %s", newObj->PPPoEServiceName);
   }

   /* static IP address */
   if (newObj->X_BROADCOM_COM_IPv4Enabled && newObj->X_BROADCOM_COM_UseStaticIPAddress)
   {
      snprintf(staticIPAddrFlag, sizeof(staticIPAddrFlag), "%s:", newObj->X_BROADCOM_COM_LocalIPAddress);
   }

   snprintf(cmdLine, sizeof(cmdLine), "nodetach plugin /lib%s/rp-pppoe.so %s %s %s linkname %s user %s password %s nodeflate",
#if INTPTR_MAX == INT64_MAX   /* 64-bit platform */
                  "64",
#else
                  "",
#endif
                  baseL3IfName, serverFlag, staticIPAddrFlag,
                  newObj->X_BROADCOM_COM_IfName,
                  newObj->username, newObj->password);
/*
    pppd command list example:
    pppd debug nodetach plugin /lib/rp-pppoe.so eth4.1 linkname ppp0.1 user abc password abc nodeflate &
*/

/*  PPPAuthenticationProtocol
    0 is AUTO
    1 is PAP => require-pap
    2 is CHAP => require-chap
    3 is MSCHAP => require-mschap
*/

   switch (cmsUtl_pppAuthToNum(newObj->PPPAuthenticationProtocol)) {
   case PPP_AUTH_METHOD_PAP:
       strncat(cmdLine, " require-pap refuse-chap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_CHAP:
       strncat(cmdLine, " require-chap refuse-pap refuse-mschap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_MSCHAP:
       strncat(cmdLine, " require-mschap refuse-pap refuse-chap noauth", sizeof(cmdLine)-1);
       break;
   case PPP_AUTH_METHOD_AUTO:
   default:
       break;
   }

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
      strncat(cmdLine, " debug", sizeof(cmdLine)-1);
   }

   /* if on demand is selected add the parameter.  IdleDisconnectTime is in seconds */
   if (newObj->idleDisconnectTime > 0)
   {
      snprintf(idlelimit, sizeof(idlelimit), " demand idle %d", newObj->idleDisconnectTime);
      strncat(cmdLine, idlelimit, sizeof(cmdLine) -1);
   }

   /* IP extension */
   if (newObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }

#ifdef SUPPORT_IPV6
   /* IPv6 */
   if (newObj->X_BROADCOM_COM_IPv6Enabled)
   {
      strncat(cmdLine, " +ipv6", sizeof(cmdLine)-1); //pppd arg is +ipv6
   }

   /* disable IPCP if it's IPv6 only */
   if (!newObj->X_BROADCOM_COM_IPv4Enabled)
   {
      strncat(cmdLine, " noip", sizeof(cmdLine)-1); //pppd arg is noip
   }
#endif

   ret = rutWan_configPPPoE(cmdLine, l2IfName, baseL3IfName, newObj->X_BROADCOM_COM_AddPppToBridge,
                            newObj->X_BROADCOM_COM_VlanMuxID, newObj->X_BROADCOM_COM_VlanMux8021p,
                            newObj->X_BROADCOM_COM_VlanTpid, &pppPid);

   if (ret == CMSRET_SUCCESS)
   {
      newObj->X_BROADCOM_COM_PppdPid = pppPid;
   }

   return ret;

}

CmsRet rutWan_cleanUpPPPoE_igd(const InstanceIdStack *iidStack, const void *obj)
{
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;
   _WanPppConnObject *currObj = (_WanPppConnObject *) obj;

   /* Need baseL3IfName for PPPoE */
   if ((ret = rutWan_getL3InterfaceInfo(MDMOID_WAN_PPP_CONN,
                                        iidStack,
                                        currObj->X_BROADCOM_COM_VlanMuxID,
                                        currObj->X_BROADCOM_COM_ConnectionId,
                                        currObj->X_BROADCOM_COM_IfName,
                                        l2IfName,
                                        baseL3IfName,
                                        &connMode)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWan_getL3InterfaceInfo failed. ret=%d", ret);
      return ret;
   }

   return ret;
}

#endif  /* DMP_BASELINE_1 */




CmsRet rutWan_IpExtensionRelay(const char *ifcName, const char *defGw, const char *externIp,
      const UBOOL8 isPpp __attribute__((unused)))
{
   _LanIpIntfObject *lanIpIntfObj = NULL;
   InstanceIdStack lanIpIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char cmd[BUFLEN_128];
   char bCastStr[BUFLEN_24];
   CmsRet ret;

   cmsLog_debug("Entered");

   if ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF,
                                          &iidStack,
                                          &lanIpIntfIidStack,
                                          (void **) &lanIpIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get lanIpIntfObj, ret=%d", ret);
      return ret;
   }

   if ((ret = rut_getBCastFromIpSubnetMask
      (lanIpIntfObj->IPInterfaceIPAddress, lanIpIntfObj->IPInterfaceSubnetMask, bCastStr)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   rut_doSystemAction("rut", "ifconfig br0 0.0.0.0");
   snprintf(cmd,  sizeof(cmd), "ifconfig br0 %s netmask %s broadcast %s",
      lanIpIntfObj->IPInterfaceIPAddress, lanIpIntfObj->IPInterfaceSubnetMask, bCastStr);
   rut_doSystemAction("rut", cmd);

   snprintf(cmd,  sizeof(cmd), "ifconfig %s 0.0.0.0", ifcName);
   rut_doSystemAction("rut", cmd);

   snprintf(cmd,  sizeof(cmd), "route add -net %s netmask 255.255.255.255 dev %s metric 1 2>/dev/null", defGw, ifcName);
   rut_doSystemAction("rut", cmd);

   snprintf(cmd,  sizeof(cmd), "route add default gw %s 2>/dev/null", defGw);
   rut_doSystemAction("rut", cmd);


   snprintf(cmd,  sizeof(cmd), "route add -host %s dev br0 2>/dev/null", externIp);
   rut_doSystemAction("rut", cmd);

   /* for ppp ip extension, the module list differs from regular one */
   rutIpt_insertIpModules();

   rutIpt_redirectHttpTelnetPorts(ifcName,  lanIpIntfObj->IPInterfaceIPAddress);

   cmsObj_free((void **) &lanIpIntfObj);

   /* enable proxy arp. */
   rut_doSystemAction("rut", "echo \"1\" > /proc/sys/net/ipv4/conf/br0/proxy_arp\n");

   /* todo:  old code and comments
     *set up IP filtering entry
     *SecCfgMngr *objSecMngr = SecCfgMngr::getInstance();
     * do partial IP Global Policy
     * objSecMngr->doIgmpPolicy();
     * objSecMngr->doQosPolicy();
     */

   return ret;

}


UBOOL8 rutWan_isAllBridgePvcs_igd(void)
{
   UBOOL8 allBridges = TRUE;
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *wanPPPConn = NULL;
   UBOOL8 noPvcFound = TRUE;

   /* if there is a WAN PPP Connection, just return FALSE */
   if (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanPPPConn) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &wanPPPConn);
      return FALSE;
   }

   while ((ret == CMSRET_SUCCESS) && allBridges)
   {
      _WanIpConnObject *wanIpConn = NULL;

      if ((ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanIpConn)) == CMSRET_SUCCESS)
      {
         noPvcFound = FALSE;
         if (strcmp(wanIpConn->connectionType, MDMVS_IP_BRIDGED) != 0)
         {
            allBridges = FALSE;
         }
         cmsObj_free((void **) &wanIpConn);
      }
   }

   /* if no pvc in the configuration, still need to have dhcpd running */
   if (noPvcFound)
   {
      allBridges = FALSE;
   }
   return allBridges;

}

UBOOL8 rut_isWanInterfaceUp(const char *ifName __attribute__((unused)), UBOOL8 isIPv4 __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("ifName=%s isIPv4=%d, DESKTOP_LINUX mode fake return true",
                ifName, isIPv4);
   return TRUE;
#else
   InstanceIdStack iidStack;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 ifcUp = FALSE;
   UBOOL8 found = FALSE;

   cmsLog_debug("Enter: ifName=%s isIPv4=%d", ifName, isIPv4);

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   /* In Hybrid mode, we have to check IPv6 on _dev2 side */
   if (!isIPv4)
   {
      return (qdmIpIntf_isWanInterfaceUpLocked_dev2(ifName, isIPv4));
   }
#endif

   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) )
      {
         found = TRUE;
         if (isIPv4 && !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED))
         {
            ifcUp = TRUE;
         }
#ifdef DMP_X_BROADCOM_COM_IPV6_1
         else if (!isIPv4 && !cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
         {
            ifcUp = TRUE;
         }
#endif
      }
      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, pppConn->X_BROADCOM_COM_IfName) )
      {
         found = TRUE;
         if (isIPv4 && !cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED))
         {
            ifcUp = TRUE;
         }
#ifdef DMP_X_BROADCOM_COM_IPV6_1
         else if (!isIPv4 && !cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
         {
            ifcUp = TRUE;
         }
#endif
      }
      cmsObj_free((void **) &pppConn);
   }

   if (ifcUp)
   {
      cmsLog_debug("Interface %s is up", ifName);
   }
   else
   {
      cmsLog_debug("Interface %s is down", ifName);
   }

   return ifcUp;
#endif

}


UINT16 rutWan_getNumberOfActiveConnections(const InstanceIdStack *parentIidStack)
{
   InstanceIdStack iidStack;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject  *ipConn = NULL;
   UINT16 num = 0;


   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, parentIidStack, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&ipConn) == CMSRET_SUCCESS)
   {
      if (!strcmp(ipConn->connectionStatus, MDMVS_CONNECTED))
      {
         num++;
      }
      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_WAN_PPP_CONN, parentIidStack, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn) == CMSRET_SUCCESS)
   {
      if (!strcmp(pppConn->connectionStatus, MDMVS_CONNECTED))
      {
         num++;
      }
      cmsObj_free((void **) &pppConn);
   }

   return num;
}


#ifdef DMP_X_BROADCOM_COM_GPONWAN_1

static CmsRet sendGponRgWanServiceStatusChange(const OmciServiceMsgBody *pService)
{
   UINT32  msgDataLen = sizeof(OmciServiceMsgBody);
   char buf[sizeof(CmsMsgHeader) + sizeof(OmciServiceMsgBody)]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   OmciServiceMsgBody *info = (OmciServiceMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsRet ret = CMSRET_SUCCESS;

   msg->type = CMS_MSG_OMCI_RG_WAN_SERVICE_STAUTS_CHANGE;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_OMCID;
   msg->flags_response = 0;
   msg->flags_event = 0;
   msg->flags_request = 1;
   msg->dataLength = msgDataLen;
   msg->flags_bounceIfNotRunning = 1;

   cmsLog_debug("ifName=%s, pbits=%d, vlanId=%d",
      pService->l2Ifname, pService->serviceParams.pbits, pService->serviceParams.vlanId);

   /* copy service info to message body */
   memcpy(info, pService, msgDataLen);

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not send out CMS_MSG_OMCI_GPON_WAN_SERVICE_GET, ret=%d", ret);
   }

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

static void sendConnectionUpdateMsg(const MdmObjectId oid,
                                    const InstanceIdStack *iidStack,
                                    const UBOOL8 isAddPvc,
                                    const UBOOL8 isStatic,
                                    const UBOOL8 isDeleted,
                                    const UBOOL8 isAutoDetectChange)
{
   UINT32  msgDataLen = sizeof(WatchedWanConnection);
   char buf[sizeof(CmsMsgHeader) + sizeof(WatchedWanConnection)]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   WatchedWanConnection *info = (WatchedWanConnection *) &(buf[sizeof(CmsMsgHeader)]);
   void *msgHandle = cmsMdm_getThreadMsgHandle();
   CmsRet ret = CMSRET_SUCCESS;

   if (iidStack == NULL)
   {
      cmsLog_error("iidStack must be provided");
      return;
   }
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
   {
       void *L2LinkCfgObj=NULL;
       MdmObjectId L2LinkCfgOid;
       CmsRet ret=CMSRET_SUCCESS;

       if ((ret = rutWl2_getL2LinkObj(oid,
                                     iidStack,
                                     &L2LinkCfgObj)) != CMSRET_SUCCESS)
       {
          return;
       }

       L2LinkCfgOid = GET_MDM_OBJECT_ID(L2LinkCfgObj);
       cmsObj_free((void**)&L2LinkCfgObj);
       /* This message is NOT sent for RG-Full. */
       if (L2LinkCfgOid == MDMOID_WAN_GPON_LINK_CFG)
       {
           return;
       }
   }
#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */

   msg->type = CMS_MSG_WATCH_WAN_CONNECTION;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_SSK;
   msg->flags_request = 1;
   msg->dataLength = msgDataLen;

   info->oid = oid;
   info->iidStack = *iidStack;
   info->isAdd = isAddPvc;
   info->isStatic = isStatic;
   info->isDeleted = isDeleted;
   info->isAutoDetectChange = isAutoDetectChange;

   cmsLog_debug("dataLen=%d, oid=%d, info->iidStack=%s, isAdd=%d, isStatic=%d",
      msgDataLen, info->oid, cmsMdm_dumpIidStack(&(info->iidStack)), info->isAdd, info->isStatic);

  /*
    * Be careful when sending a message to ssk.  During bootup, ssk is the app that
    * initializes the mdm.  So if any RCL/RUT sends a message to ssk, ssk will
    * still be inside the initialization code, so it won't be able to respond.
    * So inside the MDM, rcl/rut/stl functions can only send a message to ssk.
    * But it cannot expect a reply.
    */
   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not send out CMS_MSG_WATCH_WAN_CONNECTION, ret=%d", ret);
   }

   return;
}


void rutWan_sendConnectionUpdateMsg(const MdmObjectId oid,
                                    const InstanceIdStack *iidStack,
                                    const UBOOL8 isAddPvc,
                                    const UBOOL8 isStatic,
                                    const UBOOL8 isDeleted,
                                    const UBOOL8 isAutoDetectChange)
{
  /*
    * IF gpon wan connection then send
    * CMS_MSG_OMCI_GPON_WAN_SERVICE_GET to omcid
    * for checking layer 2 link status then omcid send
    * CMS_MSG_OMCI_GPON_WAN_SERVICE_SEND to ssk
    * for updating wan connection status.
    *
    * ELSE send CMS_MSG_WATCH_WAN_CONNECTION to ssk
    * for checking layer 2 link status and
    * update wan connection status
    */
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
   OmciServiceMsgBody service;

   memset(&service, 0, sizeof(OmciServiceMsgBody));
   service.serviceParams.serviceStatus = (isDeleted ? FALSE : TRUE);

   if (rutGpon_getWanServiceL2IfName(oid,iidStack,service.l2Ifname) == CMSRET_SUCCESS)
   {
      if (rutGpon_getWanServiceParams(oid, iidStack, &service.serviceParams) == CMSRET_SUCCESS)
      {
         sendGponRgWanServiceStatusChange(&service);
      }
   }
#endif   // DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
   sendConnectionUpdateMsg(oid, iidStack, isAddPvc, isStatic, isDeleted, isAutoDetectChange);
   return;
}

#ifdef DMP_BASELINE_1

#ifdef DMP_ETHERNETWAN_1

CmsRet  rutWan_moveEthLanToWan_igd(const char *ifName)
{
   _LanEthIntfObject *lanEthObj = NULL;
   _WanEthIntfObject *wanEthObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rutLan_getEthInterface(ifName, &iidStack, &lanEthObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot find the interface (%s) in br0", ifName);
      return ret;
   }


   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = rutWl2_getWanEthObject(&iidStack, &wanEthObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanEthIntfObject, ret=%d", ret);
      cmsObj_free((void **) &lanEthObj);
      return ret;
   }

   /*
    * Transfer settings of the LANEthernetInterface object to the
    * WANEthernetInterface object.  Don't enable it yet, let upper layer
    * do that when it is ready.
    */
   CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->MACAddress, lanEthObj->MACAddress, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->maxBitRate, lanEthObj->maxBitRate, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->duplexMode, lanEthObj->duplexMode, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_IfName, lanEthObj->X_BROADCOM_COM_IfName, mdmLibCtx.allocFlags);

   /* Need gmac flag and WanLan Atrribute preserved in WANEthObj */
   wanEthObj->X_BROADCOM_COM_GMAC_Enabled = lanEthObj->X_BROADCOM_COM_GMAC_Enabled;
   CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_WanLan_Attribute,  lanEthObj->X_BROADCOM_COM_WanLan_Attribute, mdmLibCtx.allocFlags);

   ret = cmsObj_set(wanEthObj, &iidStack);

   cmsObj_free((void **) &wanEthObj);
   cmsObj_free((void **) &lanEthObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set WanEthIntfObject, ret = %d", ret);
      return ret;
   }

   /* tmPortInit will be called in stl_wanEthIntfObject() when the link is up */
//   if ((ret = rutQos_tmPortInit(ifName, TRUE)) != CMSRET_SUCCESS)
//   {
//      cmsLog_error("rutQos_tmPortInit() returns error. ret=%d", ret);
//   }

   /* unconfig QoS queues associated with this eth interface */
   if ((ret = rutQos_qMgmtQueueOperation(ifName, TRUE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_qMgmtQueueOperation(%s) returns error. ret=%d", ifName, ret);
   }

   rutLan_deleteEthInterface(ifName);

   return ret ;
}


CmsRet rutWan_moveEthWanToLan_igd(const char *ifName)
{
   _LanEthIntfObject *lanEthObj = NULL;
   _WanEthIntfObject *wanEthObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 isWanIntf = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = rutWl2_getWanEthObject(&iidStack, &wanEthObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanEthIntfObject, ret=%d", ret);
      return ret;
   }

   /* unconfig QoS queues associated with this eth interface */
   if ((ret = rutQos_qMgmtQueueOperation(ifName, TRUE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_qMgmtQueueOperation(%s) returns error. ret=%d", ifName, ret);
   }

   if ((ret = rutQos_tmPortUninit(ifName, TRUE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_tmPortUninit() returns error. ret=%d", ret);
   }

   if ((ret = rutLan_addEthInterface(ifName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add new LAN eth interface, ret=%d", ret);
      cmsObj_free((void **) &wanEthObj);
      return ret;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(wanEthObj->X_BROADCOM_COM_IfName);
   wanEthObj->enable = FALSE;
   if ((ret = cmsObj_set(wanEthObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not disable WanEth object");
      cmsObj_free((void **) &wanEthObj);
      return ret;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   rutLan_getEthInterface(ifName, &iidStack, &lanEthObj);


   /*
    * Transfer settings of the WANEthernetInterface object to the
    * LANEthernetInterface object.  Don't enable it yet, let upper layer
    * do that when it is ready.
    */
   CMSMEM_REPLACE_STRING_FLAGS(lanEthObj->MACAddress, wanEthObj->MACAddress, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(lanEthObj->maxBitRate, wanEthObj->maxBitRate, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(lanEthObj->duplexMode, wanEthObj->duplexMode, mdmLibCtx.allocFlags);

   lanEthObj->X_BROADCOM_COM_GMAC_Enabled = wanEthObj->X_BROADCOM_COM_GMAC_Enabled;
   CMSMEM_REPLACE_STRING_FLAGS(lanEthObj->X_BROADCOM_COM_WanLan_Attribute,  wanEthObj->X_BROADCOM_COM_WanLan_Attribute, mdmLibCtx.allocFlags);
   if (!cmsUtl_strcmp(lanEthObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY))
      isWanIntf = TRUE;

   ret = cmsObj_set(lanEthObj, &iidStack);

   cmsObj_free((void **) &wanEthObj);
   cmsObj_free((void **) &lanEthObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set LanEthIntfObject, ret = %d", ret);
      return ret;
   }


#ifdef DMP_BRIDGING_1
   /* only for non-WAN port */
   if ( !isWanIntf )
   {
      UINT32 bridgeRef = 0;

      /*
       * Now that the interface is back on the LAN side, add it as an
       * available interface.  Also add corresponding filter entry.
       */
      rutPMap_addAvailableInterface(ifName, isWanIntf);
      rutPMap_addFilter(ifName, isWanIntf, bridgeRef);
   }
#endif /* DMP_BRIDGING_1 */

   return ret;
}

#endif /* DMP_ETHERNETWAN_1 */

#endif  /* DMP_BASELINE_1 */


CmsRet rutWan_getIpOrPppObjectByIfname(const char *ifName, InstanceIdStack *iidStack, void **obj, UBOOL8 *isPpp)
{
   WanIpConnObject *ipConnObj = NULL;
   WanPppConnObject *pppConnObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("looking for %s", ifName);

   *isPpp = FALSE;

   INIT_INSTANCE_ID_STACK(iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipConnObj->X_BROADCOM_COM_IfName, ifName))
      {
         cmsLog_debug("found WanIPConnection object");
         *obj = ipConnObj;
         found = TRUE;
         /* return immediately to the caller.  don't free the object */
         return CMSRET_SUCCESS;
      }

      cmsObj_free((void **) &ipConnObj);
   }


   INIT_INSTANCE_ID_STACK(iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(pppConnObj->X_BROADCOM_COM_IfName, ifName))
      {
         cmsLog_debug("found WanPPPConnection object");
         found = TRUE;
         *isPpp = TRUE;
         *obj = pppConnObj;
         /* return immediately to the caller.  don't free the object */
         return CMSRET_SUCCESS;
      }

      cmsObj_free((void **) &pppConnObj);
   }

   if (!found)
   {
      cmsLog_debug("could not find object with ifname %s", ifName);
   }

   return CMSRET_OBJECT_NOT_FOUND;
}


UBOOL8 rutWan_findFirstRoutedAndConnected_igd(char *ifName)
{
   return (rutWan_findFirstIpvxRoutedAndConnected_igd(CMS_AF_SELECT_IPV4, ifName));
}


UBOOL8 rutWan_findFirstIpvxRoutedAndConnected_igd(UINT32 ipvx, char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanIpConnObject *ipConn = NULL;
   _WanPppConnObject *pppConn = NULL;
   LanIpIntfObject *lanIpObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;


   cmsLog_debug("Enter: ipvx=%d", ipvx);
   ifName[0] = '\0';


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (ipvx & CMS_AF_SELECT_IPV6)
   {
      /* for Hybrid IPv6, check on _dev2 side first */
      found = rutWan_findFirstIpvxRoutedAndConnected_dev2(CMS_AF_SELECT_IPV6, ifName);
      if (found)
      {
         return found;
      }
   }
#endif

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_IPV6_1
      if ((ipvx & CMS_AF_SELECT_IPV6) &&
          !cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
      {
         found = TRUE;
         strcpy(ifName, ipConn->X_BROADCOM_COM_IfName);
      }
#endif

      if (!found &&
          (ipvx & CMS_AF_SELECT_IPV4) &&
          cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED) &&
          !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED))
      {
         found = TRUE;
         strcpy(ifName, ipConn->X_BROADCOM_COM_IfName);
      }

      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn)) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_IPV6_1
      if ((ipvx & CMS_AF_SELECT_IPV6) &&
          !cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
      {
         found = TRUE;
         strcpy(ifName, pppConn->X_BROADCOM_COM_IfName);
      }
#endif

      if (!found &&
          (ipvx & CMS_AF_SELECT_IPV4) &&
          cmsUtl_strcmp(pppConn->connectionType, MDMVS_IP_BRIDGED) &&
          cmsUtl_strcmp(pppConn->connectionType, MDMVS_PPPOE_BRIDGED) && 
          !cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED))
      {
         found = TRUE;
         strcpy(ifName, pppConn->X_BROADCOM_COM_IfName);
      }

      cmsObj_free((void **) &pppConn);
   }

   /*
    * XXX This is temporary location for this logic: this function is
    * asking for ROUTED interfaces up, but this dhcpc LAN interface is
    * more of a BRIDGED interface with a dynamically assigned IP address.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lanIpObj)) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_IPV6_1
      if (ipvx & CMS_AF_SELECT_IPV6)
      {
         cmsLog_debug("dhcpc on LAN side not supported for old Broadcom IPv6");
      }
#endif

      if (!found &&
          (ipvx & CMS_AF_SELECT_IPV4)
#ifdef DMP_X_BROADCOM_COM_BASELINE_1
          &&
          !cmsUtl_strcmp(lanIpObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_CONNECTED)
#endif
          )
      {
         found = TRUE;
         strcpy(ifName, lanIpObj->X_BROADCOM_COM_IfName);
      }

      cmsObj_free((void **) &lanIpObj);
   }

   cmsLog_debug("Found=%d, ifName=%s", found, ifName);

   return found;
}


UBOOL8 rutWan_findFirstNattedAndConnected_igd(char *ifcName, const char *excludeThisIfc)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *pppConn = NULL;
   WanIpConnObject *ipConn = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("Entered: excludeThisIfc=%s", excludeThisIfc);

   if (ifcName == NULL)
   {
      cmsLog_error("Null ifcName.");
      return FALSE;
   }
   ifcName[0] = '\0';


   while (!found && cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &ipConn) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED) && ipConn->NATEnabled)      {
         /* cmsUtl_strcmp takes care of excludeThis being NULL */
         if  (cmsUtl_strcmp(excludeThisIfc, ipConn->X_BROADCOM_COM_IfName))
         {
            strcpy(ifcName, ipConn->X_BROADCOM_COM_IfName);
            found = TRUE;
         }
      }
      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &pppConn) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED) && pppConn->NATEnabled)
      {
         /* cmsUtl_strcmp takes care of excludeThis being NULL */
         if  (cmsUtl_strcmp(excludeThisIfc, pppConn->X_BROADCOM_COM_IfName))
         {
            strcpy(ifcName, pppConn->X_BROADCOM_COM_IfName);
            found = TRUE;
         }
      }
      cmsObj_free((void **) &pppConn);
   }

   cmsLog_debug("Exit: found=%d ifcName=%s", found, ifcName);

   return found;
}


CmsRet rutWan_getAvailableConIdForMSC(const char *wanL2IfName, SINT32 *outConId)
{
   _WanConnDeviceObject *wanCon;
   _WanIpConnObject *ipConn=NULL;
   _WanPppConnObject *pppConn=NULL;
   MdmObjectId linkObjId = 0xFFFF;  // Initialize to bugus value to get rid of warning.
   InstanceIdStack linkIidStack;
   InstanceIdStack savedWanConIid;
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 conId;
   SINT32 conIdArray[IFC_WAN_MAX+1] = {0};
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   if (wanL2IfName == NULL)
   {
      cmsLog_error("wanL2IfName is NULL");
      return CMSRET_INTERNAL_ERROR;
   }

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
   if (cmsUtl_strstr(wanL2IfName, ATM_IFC_STR))
   {
      if (rutDsl_getDslLinkByIfName((char*) wanL2IfName, &iidStack, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }
      linkObjId = MDMOID_WAN_DSL_LINK_CFG;
      found = TRUE;
   }
#endif

#ifdef DMP_PTMWAN_1
   if (cmsUtl_strstr(wanL2IfName, PTM_IFC_STR))
   {
      if (rutDsl_getPtmLinkByIfName((char*) wanL2IfName, &iidStack, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }
      linkObjId = MDMOID_WAN_PTM_LINK_CFG;
      found = TRUE;
   }
#endif

#ifdef DMP_BASELINE_1
      /* Ethwan implementation will probably be very different in TR181,
       * so for now, make this TR98 only.
       */

#ifdef DMP_ETHERNETWAN_1  /* aka SUPPORT_ETHWAN */
   if (cmsUtl_strstr(wanL2IfName, ETH_IFC_STR))
   {
      InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
      void *obj;
      if (rutEth_getEthIntfByIfName((char*) wanL2IfName, &iidStack1, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }
      /* get iidstack of WANConnectionDevice */
      if ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_ETH_LINK_CFG, &iidStack1, &iidStack, (void **)&obj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get WanEthLinkCfgObject, ret=%d", ret);
         return ret;
      }
      cmsObj_free(&obj);   /* no longer needed */
      linkObjId = MDMOID_WAN_ETH_LINK_CFG;
      found = TRUE;
   }
#endif /* DMP_ETHERNETWAN_1 */
#endif /* DMP_BASELINE_1 */

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
   if (cmsUtl_strstr(wanL2IfName, GPON_IFC_STR) && !rut_isWanTypeEpon())
   {
      if (rutGpon_getGponLinkByIfName((char*) wanL2IfName, &iidStack, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }
      linkObjId = MDMOID_WAN_GPON_LINK_CFG;
      found = TRUE;
   }
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
   if (cmsUtl_strstr(wanL2IfName, EPON_IFC_STR) && rut_isWanTypeEpon())
   {
      if (rutEpon_getEponLinkByIfName((char*) wanL2IfName, &iidStack, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }

      linkObjId = MDMOID_WAN_EPON_LINK_CFG;
      found = TRUE;
   }
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
   if (cmsUtl_strstr(wanL2IfName, WLAN_IFC_STR))
   {
      InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
      void *obj;
      if (rutWanWifi_getWlIntfByIfName((char*) wanL2IfName, &iidStack1, NULL) == FALSE)
      {
         cmsLog_error("Fail to get wanL2IfName=%s", wanL2IfName);
         return CMSRET_INTERNAL_ERROR;
      }
      /* get iidstack of WANConnectionDevice */
      if ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_WIFI_LINK_CFG, &iidStack1, &iidStack, (void **)&obj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get WanWlLinkCfgObject, ret=%d", ret);
         return ret;
      }
      cmsObj_free(&obj);   /* no longer needed */
      linkObjId = MDMOID_WAN_WIFI_LINK_CFG;
      found = TRUE;
   }
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

   if (!found)
   {
      cmsLog_error("Wrong format: wanL2IfName=%s", wanL2IfName);
      return CMSRET_INTERNAL_ERROR;
   }

   /* need to get WAN connection object iidStack from dslLinkCfg or ptmLinkCfg iidStack */
   if ((ret = cmsObj_getAncestor(MDMOID_WAN_CONN_DEVICE, linkObjId,
				    &iidStack, (void **)&wanCon)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get cmsObj_getAncestor(MDMOID_WAN_CONN_DEVICE). ret=%d", ret);
      return ret;
   }
   cmsObj_free((void **) &wanCon);

   savedWanConIid = iidStack;

   /* iidStack is the WanConn Dev now */

   INIT_INSTANCE_ID_STACK(&linkIidStack);
   /* need go thru to find out all the connection Id used in ip and ppp connection objects */
   while (cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, &iidStack, &linkIidStack, OGF_NO_VALUE_UPDATE, (void **)&ipConn) == CMSRET_SUCCESS)
   {
      /* default X_BROADCOM_COM_ConnectionId is 0, just mark the array */
      conIdArray[ipConn->X_BROADCOM_COM_ConnectionId] = 1;
      cmsObj_free((void **) &ipConn);
   }

   iidStack = savedWanConIid;
   INIT_INSTANCE_ID_STACK(&linkIidStack);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_WAN_PPP_CONN, &iidStack, &linkIidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn) == CMSRET_SUCCESS)
   {
      /* default X_BROADCOM_COM_ConnectionId is 0, just mark the array */
      conIdArray[pppConn->X_BROADCOM_COM_ConnectionId] = 1;
      cmsObj_free((void **) &pppConn);
   }

   for (conId = 0; conId <= IFC_WAN_MAX; conId++)
   {
      cmsLog_debug("conIdArray[%d]=%d", conId, conIdArray[conId]);
   }

   /* connection id starts at 1 */
   for (conId = 1; conId <= IFC_WAN_MAX; conId++)
   {
      if (conIdArray[conId] == 0)
      {
         cmsLog_debug("found available conId=%d", conId);
         ret = CMSRET_SUCCESS;
         break;
      }
   }

   if (conId > IFC_WAN_MAX)
   {
      cmsLog_error("Failed to get conId");
      return CMSRET_INTERNAL_ERROR;
   }

   *outConId = conId;

   return ret;

}


UBOOL8 rut_checkInterfaceUp(const char *devname)
{

   int  skfd;
   int  ret = FALSE;
   struct ifreq intf;

   if (devname == NULL || (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      return ret;
   }

   strncpy(intf.ifr_name, devname, sizeof(intf.ifr_name)-1);
   intf.ifr_name[sizeof(intf.ifr_name)-1] = '\0';

   /*    if interface is br0:0 and
    * there is no binding IP address then return down
    */

   if (strchr(devname, ':') != NULL)
   {
      if (ioctl(skfd, SIOCGIFADDR, &intf) < 0)
      {
         close(skfd);
         return ret;
      }
   }

   // if interface flag is down then return down
   if (ioctl(skfd, SIOCGIFFLAGS, &intf) != -1)
   {
      if ((intf.ifr_flags & IFF_UP) != 0)
         ret = TRUE;
   }

   close(skfd);

   return ret;
}


SINT32 rutWan_getInterfaceMTU(SINT32 domain, const char *ifname)
{
   struct ifreq ifr;
   int sockfd, err;

   if( (sockfd = socket(domain, SOCK_DGRAM, 0)) < 0 )
   {
      return -1;
   }

   strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
   ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
   err = ioctl(sockfd, SIOCGIFMTU, (void*)&ifr);
   close(sockfd);

   if (err == -1)
   {
      return -1;
   }
   return ifr.ifr_mtu;
}

CmsRet rutWan_fillInPPPIndexArray_igd(SINT32 *intfArray)
{
   SINT32 index = 0;

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *pppConn;

   while (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NORMAL_UPDATE, (void **) &pppConn) == CMSRET_SUCCESS)
   {
      if (pppConn->X_BROADCOM_COM_IfName == NULL)
      {
         cmsLog_debug("This is one in creating so ifName is NULL, continue...");
         /* this is one we just created and is NULL, so just skip it */
      }
      else
      {
         if (cmsUtl_strstr(pppConn->X_BROADCOM_COM_IfName, PPPOA_IFC_STR))
         {
            index = atoi(&(pppConn)->X_BROADCOM_COM_IfName[cmsUtl_strlen(PPPOA_IFC_STR)]);
         }
         else
         {
            index = atoi(&(pppConn)->X_BROADCOM_COM_IfName[cmsUtl_strlen(PPP_IFC_STR)]);
         }

         cmsLog_debug("pppConn->ifname=%s, index=%d", pppConn->X_BROADCOM_COM_IfName, index);

         if (index > IFC_WAN_MAX)
         {
            cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
            cmsObj_free((void **) &pppConn);
            return CMSRET_INTERNAL_ERROR;
         }
         *(intfArray+index) = 1;            /* mark the interface used */
      }
      cmsObj_free((void **) &pppConn);

   }

   return CMSRET_SUCCESS;

}



CmsRet rutWan_fillPppIfName(UBOOL8 isPPPoE, char *pppName)
{
   SINT32 intfArray[IFC_WAN_MAX];
   SINT32 i;

   if (pppName == NULL)
   {
      cmsLog_error("pppName is Null");
      return CMSRET_INTERNAL_ERROR;
   }
   memset((UINT8 *) &intfArray, 0, sizeof(intfArray));

   if (rutWan_fillInPPPIndexArray(intfArray) != CMSRET_SUCCESS)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   for (i = 0; i < IFC_WAN_MAX; i++)
   {
      cmsLog_debug("intfArray[%d]=%d", i, intfArray[i]);
      if (intfArray[i] == 0 )
      {
         if (isPPPoE)
         {
            sprintf(pppName, "%s%d", PPP_IFC_STR, i);
         }
         else
         {
            sprintf(pppName, "%s%d", PPPOA_IFC_STR, i);
         }
         break;
      }
   }

   cmsLog_debug("ppp device name is %s", pppName);

   return CMSRET_SUCCESS;

}

CmsRet rutWan_getNumUsedQueues(WanLinkType wanType, UINT32 *usedQueues)
{
   CmsRet   ret;

   *usedQueues = 0;

#ifdef SUPPORT_DSL
   XTM_CONN_CFG  connCfg;
   XTM_ADDR connAddrs[MAX_TRANSMIT_QUEUES];
   UINT32   numAddrs = MAX_TRANSMIT_QUEUES;
   UINT32   trafficType;
   UINT32   i;

   if ((ret = devCtl_xtmGetConnAddrs(connAddrs, &numAddrs)) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmGetConnAddrs returns error. ret=%d", ret);
      return ret;
   }

   trafficType = (wanType == ATM)? TRAFFIC_TYPE_ATM : TRAFFIC_TYPE_PTM;

   for (i = 0; i < numAddrs; i++)
   {
      memset(&connCfg, 0, sizeof(XTM_CONN_CFG));
      if ((ret = devCtl_xtmGetConnCfg(&connAddrs[i], &connCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("devCtl_xtmGetConnCfg returns error. ret=%d", ret);
         return ret;
      }

      if (connAddrs[i].ulTrafficType == trafficType)
      {
         *usedQueues += connCfg.ulTransmitQParmsSize;
      }
   }
#else
   cmsLog_error("Unsupported wanType=%d, only ATM or PTM supported", wanType);
   ret = CMSRET_INVALID_ARGUMENTS;
#endif


   return ret;
}


CmsRet rutWan_getNumUnusedQueues(WanLinkType wanType __attribute((unused)),
                                 UINT32 *unusedQueues)
{
   CmsRet   ret;

   *unusedQueues = 0;

#ifdef SUPPORT_DSL
   UINT32   trafficType;
   UINT32   maxQueues;
   UINT32   usedQueues = 0;

   if ((ret = rutWan_getNumUsedQueues(wanType, &usedQueues))  != CMSRET_SUCCESS)
   {
      return ret;
   }

   trafficType = (wanType == ATM)? TRAFFIC_TYPE_ATM : TRAFFIC_TYPE_PTM;

   if (trafficType == TRAFFIC_TYPE_ATM)
   {
      maxQueues = MAX_ATM_TRANSMIT_QUEUES;
   }
   else
   {
      maxQueues = MAX_PTM_TRANSMIT_QUEUES;
   }

   if (usedQueues < maxQueues)
   {
      *unusedQueues = maxQueues - usedQueues;
   }
   else
   {
      *unusedQueues = 0;
   }
#else
   cmsLog_error("Unsupported wanType=%d, only ATM or PTM supported", wanType);
   ret = CMSRET_INVALID_ARGUMENTS;
#endif


   return ret;
}


#ifdef DMP_BASELINE_1
CmsRet rutWan_doDefaultSystemGatewayAndDNS(UBOOL8 isIPv4)
{
   CmsRet ret = CMSRET_SUCCESS;

   /* this is a TR98 only function.  PURE_181 code does not call it. */
   if (rutWan_isAllBridgePvcs())
   {
      /* if all bridges in the system, skip setting default gateway and dns */
      return ret;
   }

   if ((ret = rutRt_doSystemDefaultGateway(isIPv4)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set up default system gateway.  ret=%d",  ret);
   }

   ret = rutNtwk_doSystemDns(isIPv4);

   return ret;
}
#endif /* DMP_BASELINE_1 */


CmsRet rutWan_getIpExIpAddress(char *ipAddress)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *pppConnObj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;


   /* todo:  If support advanced dmz on ipoe, need to add getNext ipConnObj while loop */
    while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(pppConnObj->connectionStatus, MDMVS_CONNECTED) == 0)
      {
         strncpy(ipAddress, pppConnObj->externalIPAddress, BUFLEN_16-1);
         found = TRUE;
      }
      cmsObj_free((void **) &pppConnObj);
   }

   if (found == TRUE)
   {
      cmsLog_debug("found ipEx wan extern ipAddress=%s",  ipAddress);
   }
   else
   {
      ret = CMSRET_OBJECT_NOT_FOUND;
      cmsLog_debug("External IP address cannot be found. (maybe linke is not up yet)");
   }

   return ret;
}



CmsRet rutWan_fillWanL3IfNameAndServiceName(void *wanConnObj,
                                            const InstanceIdStack *iidStack)
{
   MdmObjectId wanConnOid;
   UBOOL8 isDSL=FALSE;
   UBOOL8 isPPPoE=TRUE;
   XTM_ADDR xtmAddr;
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char wanL3IfName[CMS_IFNAME_LENGTH]={0};
   char dfltServiceName[BUFLEN_64]={0};
   char sufix[CMS_IFNAME_LENGTH]={0};
   _WanIpConnObject *ipConnObj = (_WanIpConnObject *) wanConnObj;
   _WanPppConnObject *pppConnObj = (_WanPppConnObject *) wanConnObj;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter");

   memset(&xtmAddr, 0, sizeof(xtmAddr));

   wanConnOid = GET_MDM_OBJECT_ID(wanConnObj);

   /* Need to get layer 2 interface name, connMode and xtmAddr to form the layer 3 name
   * and service name
   */
   ret = rutWl2_getL2IfName(wanConnOid, iidStack, l2IfName);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   connMode = rutWl2_getConnMode(wanConnOid, iidStack);

#ifdef DMP_ADSLWAN_1
   /* Get dsl related info only if the WanConnectionObject's underline layer 2 is dsl */
   isDSL = rutWl2_isWanLayer2DSL(wanConnOid, iidStack);
   if (isDSL == TRUE)
   {
      UBOOL8 isPPPoAorIPoA;
      UBOOL8 isVCMux;

      /* if DSL (ATM/PTM), need to get XTM info */
      if ((ret = rutWl2_getXtmInfo(wanConnOid,
                                    iidStack,
                                    &isPPPoAorIPoA,
                                    &isVCMux,
                                    &xtmAddr)) != CMSRET_SUCCESS)
      {
         return ret;
      }
   }
#endif

   cmsLog_debug("isDSL=%d, connMode=%d, l2IfName=%s", isDSL, connMode, l2IfName);

   if (isDSL)
   {
#ifdef DMP_ADSLWAN_1

      char sufix[BUFLEN_32];

      if (xtmAddr.ulTrafficType == TRAFFIC_TYPE_ATM)
      {
         /* form the sufix for serviceName */
         snprintf(sufix, sizeof(sufix), "_%d_%d_%d",
            PORTMASK_TO_PORTID(xtmAddr.u.Vcc.ulPortMask), xtmAddr.u.Vcc.usVpi, xtmAddr.u.Vcc.usVci);
      }
      else if (xtmAddr.ulTrafficType == TRAFFIC_TYPE_PTM)
      {

         /* form the sufix for serviceName */
         snprintf(sufix, sizeof(sufix), "_%d_%d_%d",
            PORTMASK_TO_PORTID(xtmAddr.u.Flow.ulPortMask), xtmAddr.u.Flow.ulPtmPriority, xtmAddr.u.Flow.ulPtmPriority ? 0:1);
      }

      /* for bridge/ipoe/ipoa Layer2 and layer3 are same and pppoe/ppoa is different */
      strcpy(wanL3IfName, l2IfName);

      if (wanConnOid == MDMOID_WAN_IP_CONN && rutWl2_isIPoA(iidStack))
      {
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s%s", IPOA_IFC_STR, sufix);
      }
      else if (wanConnOid == MDMOID_WAN_PPP_CONN && rutWl2_isPPPoA(iidStack))
      {
         isPPPoE = FALSE;

         /* Need to form the L3 interface name for PPPoA  */
         if ((ret = rutWan_fillPppIfName(isPPPoE, wanL3IfName)) != CMSRET_SUCCESS)
         {
            return ret;
         }
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s%s", PPPOA_IFC_STR, sufix);
      }
      else if (wanConnOid == MDMOID_WAN_IP_CONN &&
         !cmsUtl_strcmp(ipConnObj->connectionType, MDMVS_IP_BRIDGED))
      {
         /* it's bridge */
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s%s", BRIDGE_IFC_STR, sufix);
      }
      else if (wanConnOid == MDMOID_WAN_IP_CONN &&
         !cmsUtl_strcmp(ipConnObj->connectionType, MDMVS_IP_ROUTED))
      {
         /* its IPoE */
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s%s", IPOE_IFC_STR, sufix);
      }
      else if (wanConnOid == MDMOID_WAN_PPP_CONN &&
         !cmsUtl_strcmp(pppConnObj->connectionType, MDMVS_IP_ROUTED))
      {
         /* its PPPoE. Need to form the L3 interface name for PPPoE  */
         if ((ret = rutWan_fillPppIfName(isPPPoE, wanL3IfName)) != CMSRET_SUCCESS)
         {
            return ret;
         }
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s%s", PPPOE_IFC_STR, sufix);
      }
      else
      {
         cmsLog_error("Invalid configuration.");
         return CMSRET_INVALID_ARGUMENTS;
      }

#endif /* DMP_ADSLWAN_1 */

   }
   else /* no DSL (ethWan, L2tpAcWan etc.) */
   {

      /* for bridge/ipoe Layer2 and layer3 are same and pppoe is different */
      strcpy(sufix, l2IfName);
      if (wanConnOid == MDMOID_WAN_IP_CONN)
      {
         strcpy(wanL3IfName, l2IfName);
         if (!cmsUtl_strcmp(ipConnObj->connectionType, MDMVS_IP_ROUTED))
         {
            /* its IPoE */
            snprintf(dfltServiceName, sizeof(dfltServiceName), "%s_", IPOE_IFC_STR);
         }
         else
         {
            /* its a bridge */
            snprintf(dfltServiceName, sizeof(dfltServiceName), "%s_", BRIDGE_IFC_STR);
         }
      }
      else if (wanConnOid == MDMOID_WAN_PPP_CONN &&
         !cmsUtl_strcmp(pppConnObj->connectionType, MDMVS_IP_ROUTED))
      {
         /* its PPPoE. Need to form the L3 interface name for PPPoE  */
         if ((ret = rutWan_fillPppIfName(isPPPoE, wanL3IfName)) != CMSRET_SUCCESS)
         {
            return ret;
         }
         snprintf(dfltServiceName, sizeof(dfltServiceName), "%s_", PPPOE_IFC_STR);
      }
      else
      {
         cmsLog_error("Invalid configuration.");
         return CMSRET_INVALID_ARGUMENTS;
      }
   }


   switch (connMode)
   {
      case CMS_CONNECTION_MODE_DEFAULT:
         /* no sufix for default mode */
         sufix[0] = '\0';
         break;

      case CMS_CONNECTION_MODE_VLANMUX:
         if (wanConnOid == MDMOID_WAN_IP_CONN)
         {
            snprintf(sufix, sizeof(sufix), ".%d", ipConnObj->X_BROADCOM_COM_ConnectionId);
         }
         else
         {
            snprintf(sufix, sizeof(sufix), ".%d", pppConnObj->X_BROADCOM_COM_ConnectionId);
         }
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
         if (cmsUtl_strstr(wanL3IfName, WLAN_IFC_STR))
         {
            sprintf(wanL3IfName, "wifi%d", atoi(&wanL3IfName[2]));
         } else
#endif
         strncat(wanL3IfName, sufix, sizeof(wanL3IfName)-1);

         sufix[0] = '\0';
         if (wanConnOid == MDMOID_WAN_IP_CONN)
         {
            if (ipConnObj->X_BROADCOM_COM_VlanMuxID >= 0)
            {
            snprintf(sufix, sizeof(sufix), ".%d", ipConnObj->X_BROADCOM_COM_VlanMuxID);
         }
         }
         else
         {
            if (pppConnObj->X_BROADCOM_COM_VlanMuxID >= 0)
            {
            snprintf(sufix, sizeof(sufix), ".%d", pppConnObj->X_BROADCOM_COM_VlanMuxID);
         }
         }
         strncat(dfltServiceName, sufix, sizeof(dfltServiceName)-1);
         break;

      default:
         cmsLog_error("Wrong connMode %d", connMode);
         return CMSRET_INTERNAL_ERROR;
   }


   if (wanConnOid == MDMOID_WAN_IP_CONN)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipConnObj->X_BROADCOM_COM_IfName, wanL3IfName, mdmLibCtx.allocFlags);
      /* in case it was input by user */
      if (ipConnObj->name == NULL)
      {
         CMSMEM_REPLACE_STRING_FLAGS(ipConnObj->name, dfltServiceName, mdmLibCtx.allocFlags);
      }
      cmsLog_debug("wan L3 ifName=%s, service name=%s", ipConnObj->X_BROADCOM_COM_IfName, ipConnObj->name);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(pppConnObj->X_BROADCOM_COM_IfName, wanL3IfName, mdmLibCtx.allocFlags);
      /* in case it was input by user */
      if (pppConnObj->name == NULL)
      {
         CMSMEM_REPLACE_STRING_FLAGS(pppConnObj->name, dfltServiceName, mdmLibCtx.allocFlags);
      }
      cmsLog_debug("wan L3 ifName=%s, service name=%s", pppConnObj->X_BROADCOM_COM_IfName, pppConnObj->name);
   }

   cmsLog_debug("Exit. ret=%d", ret);

   return ret;

}


CmsRet rutWan_startPPPoA(const InstanceIdStack *iidStack __attribute((unused)),
                         void *newObj __attribute((unused)))
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;

#if defined(DMP_DEVICE2_ATMLINK_1) || defined(DMP_X_BROADCOM_COM_ATMWAN_1)
   ret = rutDsl_initPPPoA(iidStack, newObj);
#endif

   return ret;
}

CmsRet rutWan_startIPoA(const InstanceIdStack *iidStack __attribute((unused)),
                        _WanIpConnObject *newObj __attribute((unused)))
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;


#if defined(DMP_X_BROADCOM_COM_ATMWAN_1)
   /* The equivalent functionality of initIPoA for TR181 is done in
    * rutIpv4Service_runStateMachine(), so this code is only needed for TR98.
    */
   ret = rutDsl_initIPoA(iidStack, newObj);
#endif

   return ret;
}

UBOOL8 rutWan_IsPPPIpExtension(void)
{
#ifdef DMP_BASELINE_1
   UBOOL8 isPPPIpExtension = FALSE;
   _WanPppConnObject *pppConn = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsObj_getNext(MDMOID_WAN_PPP_CONN,
                                &iidStack,
                                (void **) &pppConn)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("no pppConn in the modem, so return FALSE");
      return isPPPIpExtension;
   }


   isPPPIpExtension = pppConn->X_BROADCOM_COM_IPExtension;

   cmsObj_free((void **) &pppConn);

   cmsLog_debug("ipExt=%d", isPPPIpExtension);

   return isPPPIpExtension;
#else
   cmsLog_debug("No PPP IP Extension feature in TR181, return false");
   return FALSE;
#endif  /* DMP_BASELINE_1 */

}

#ifdef SUPPORT_DM_PURE181
 UBOOL8 rutWan_isPppOnDemand(const char *ifName)
 {
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2PppInterfaceObject *pppObj=NULL;
   UBOOL8 isPppOnDemand = FALSE;
   UBOOL8 found = FALSE;
   CmsRet ret;

   while(!found &&
         (ret = cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&pppObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, pppObj->name))
      {
         found = TRUE;
         if (pppObj->idleDisconnectTime > 0)
         {
            isPppOnDemand = TRUE;
         }
      }
      cmsObj_free((void **) &pppObj);
   }

   cmsLog_debug(" Interface %s,  isPppOnDemand = %d", ifName, isPppOnDemand);

   return isPppOnDemand;

 }
#else
 UBOOL8 rutWan_isPppOnDemand(const char *ifName)
 {
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *pppCon = NULL;
   UBOOL8 isPppOnDemand = FALSE;
   UBOOL8 found = FALSE;

   while (!found &&
         (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppCon)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, pppCon->X_BROADCOM_COM_IfName))
      {
         found = TRUE;
         if (pppCon->idleDisconnectTime > 0)
         {
            isPppOnDemand = TRUE;
         }
      }
      cmsObj_free((void **) &pppCon);
   }

   cmsLog_debug(" Interface %s,  isPppOnDemand = %d", ifName, isPppOnDemand);

   return isPppOnDemand;

 }
#endif

void rutWan_clearWanConnIpAdress(const char *ifName)
{
   char cmdStr[BUFLEN_128];

   snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", ifName);
   rut_doSystemAction("rutWan_clearWanConnIpAdress: ifconfig wanIfName 0.0.0.0", (cmdStr));
}

UBOOL8 rutWan_isTransientLayer2LinkUp(const char *transientL2LinkStatus)
{
   return (!cmsUtl_strcmp(transientL2LinkStatus, MDMVS_UP));
}

CmsRet rutWan_configPPPoE(const char *cmdLine, const char *l2IfName, const char *baseL3IfName,
                          UBOOL8 enablePppoeRelay, SINT32 vlanMuxId,SINT32 vlanMuxPr, UINT32 vlanTpid , SINT32 *pppPid)
{
   SINT32 pid=CMS_INVALID_PID;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Entered: cmdLine=%s L2IfName =%s baseL3IfName=%s enablePppoeRelay=%d",
                 cmdLine, l2IfName, baseL3IfName, enablePppoeRelay);

#ifdef SUPPORT_IPV6
   {
      char procCmd[BUFLEN_128]={0};
      snprintf(procCmd, sizeof(procCmd),
               "echo 1 > /proc/sys/net/ipv6/conf/%s/accept_ra", baseL3IfName);
      rut_doSystemAction("rutWan_configPPPoE", procCmd);
   }
#endif

   pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PPP, cmdLine, strlen(cmdLine)+1);

   if (pid == CMS_INVALID_PID)
   {
      cmsLog_error("Failed to start pppoe.");
      ret = CMSRET_INTERNAL_ERROR;
   }
    else
   {
      cmsLog_debug("Restart pppoe msg sent, new pppoe pid=%d", pid);
      *pppPid =pid;
   }
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
   char l3IfName_pppoe_pt[CMS_IFNAME_LENGTH]={0};
   char cmdStr[BUFLEN_128];

   sprintf(l3IfName_pppoe_pt, "%s.pt", baseL3IfName);
   printf("rutWan_configPPPoE : l3IfName_pppoe_pt = %s\n", l3IfName_pppoe_pt);
   if(enablePppoeRelay)
   {
      vlansubif_options_t vlan_subif_options = {
         .name = l3IfName_pppoe_pt,
         .pbit = vlanMuxPr,
         .tpid = vlanTpid,
         .subif_type = VLANSUBIF_TYPE_PPP_BRIDGE,
         .is_phyif_other_owned = 0
      };
      int rc;
      if(vlanMuxId < 0)
      {
         vlan_subif_options.num_etype_filters = 2;
         vlan_subif_options.etype_filter[0] = 0x8863;
         vlan_subif_options.etype_filter[1] = 0x8864;
      }
      /* Create VLAN sub-interface */
      rc = vlanSubif_createVlanInterface(l2IfName, vlanMuxId, &vlan_subif_options);
      if (rc)
      {
         cmsLog_error("Failed to create PPPoE interface %s. Error %d", l3IfName_pppoe_pt, rc);
         ret = CMSRET_INTERNAL_ERROR;
      }

      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", l3IfName_pppoe_pt);
      rut_doSystemAction("rutWan_configPPPoE: ifconfig l3IfName_pppoe_pt up", (cmdStr));

      FILE* fsRoute = fopen("/proc/net/route", "r");
      char col[11][32] = {{0},{0}};
      char line[512];
      SINT32 count = 0;

      if ( fsRoute != NULL )
      {
          while ( fgets(line, sizeof(line), fsRoute) )
          {
              // read pass header line
              if ( count++ < 1 ) continue;
              sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
                  col[0], col[1], col[2], col[3], col[4], col[5],
                  col[6], col[7], col[8], col[9], col[10]);
          }
          fclose(fsRoute);

          snprintf(cmdStr, sizeof(cmdStr), "brctl addif %s %s", col[0], l3IfName_pppoe_pt);
          rut_doSystemAction("rutWan_configPPPoE: Add l3IfName_pppoe_pt to bridge", cmdStr);
      }
      else
          cmsLog_error("bridge not found!");

   }
   else
   {
      if (rut_wanGetIntfIndex(l3IfName_pppoe_pt) > 0)
      {
          /* Delete the virtual interface. All rules associated with it will be purged. */
          cmsLog_debug("rutWan_configPPPoE: delete l3IfName_pppoe_pt = %s", l3IfName_pppoe_pt);
          vlanSubif_deleteVlanInterface(l3IfName_pppoe_pt);
      }
   }
#endif
   cmsLog_debug("ret %d, pppPid %d", ret, *pppPid);
   return ret;

}




CmsRet rutWan_getOpticalWanType(char * optIfWanType)
{
   char wanTypeBuf[8]={0};
   CmsRet ret=CMSRET_SUCCESS;

#ifdef DESKTOP_LINUX
   /* For now, pretend always gpon mode */
   strcpy(wanTypeBuf, "GPON");
#else

   if(cmsPsp_get(RDPA_WAN_TYPE_PSP_KEY, wanTypeBuf, sizeof(wanTypeBuf)) < 0 )
      cmsLog_debug("cmsPsp_get: unable to read RDPA_WAN_TYPE_PSP_KEY"); 

#endif

   if ((strcasecmp(wanTypeBuf,"GPON") == 0) ||
       (strcasecmp(wanTypeBuf,"XGPON1") == 0) ||
       (strcasecmp(wanTypeBuf,"NGPON2") == 0) ||
       (strcasecmp(wanTypeBuf,"XGS") == 0))
   {
      strcpy(optIfWanType, MDMVS_GPON);
   }
   else if (strcasecmp(wanTypeBuf,"EPON") == 0)
   {
      strcpy(optIfWanType, MDMVS_EPON);
   }
   else
   {
      strcpy(optIfWanType, MDMVS_NONE);
   }

   return ret;
}

void rutWan_startPppoeRelay(const char *baseL3IfName)
{
   char cmdLine[BUFLEN_128];
   snprintf(cmdLine, sizeof(cmdLine), "pppoe-relay -C br0 -B %s", baseL3IfName);
   rut_doSystemAction("rutWan_startPppoeRelay", cmdLine);
}

void rutWan_stopPppoeRelay(void)
{
   char cmdLine[BUFLEN_128];
   snprintf(cmdLine, sizeof(cmdLine), "/usr/bin/killall pppoe-relay ");
   rut_doSystemAction("rutWan_stopPppoeRelay", cmdLine);
}
