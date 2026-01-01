/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom
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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_qos.h"
#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "rcl.h"
#include "odl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_atm.h"
#include "rut_dsl.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include <linux/bcm_skb_defines.h>


extern char *defaultClassifications[];    /* defined in rut_qosDefaultClassifications.c */
extern UINT32 sizeOfDefaultClassifications(void);

/** Local functions **/


#ifdef DMP_ADSLWAN_1

/** Check the connection types on all the QoS enabled PVCs.
 *
 * @param bridgePvc (OUT) TRUE if there exists a bridge connection on a
 *                        QoS enabled PVC in the system.
 * @param routePvc  (OUT) TRUE if there exists a route connection on a
 *                        QoS enabled PVC in the system.
 * @return CmsRet         enum.
 *
 */
CmsRet rutQos_getWanQosInfo_igd(UBOOL8 *bridgeQos, UBOOL8 *routeQos)
{
#ifdef DMP_ETHERNETWAN_1
   WanEthIntfObject     *wethIntf = NULL;
#endif
   WanDslIntfCfgObject  *dslIntf = NULL;
   WanIpConnObject      *ipCon   = NULL;
   WanPppConnObject     *pppCon  = NULL;
   InstanceIdStack      iid, iidTmp;
   char                 linkEncap[BUFLEN_64];
   UBOOL8               ipBridged, ipRouted;
   CmsRet               ret;

   *bridgeQos = *routeQos = FALSE;

   /* search through all WanIpConnObjects */
   INIT_INSTANCE_ID_STACK(&iid);
   while ((*bridgeQos == FALSE || *routeQos == FALSE) &&
          (ret = cmsObj_getNext(MDMOID_WAN_IP_CONN, &iid, (void **)&ipCon)) == CMSRET_SUCCESS)
   {
      if (!ipCon->enable)
      {
         cmsObj_free((void **)&ipCon);
         continue;
      }

      ipBridged = ipRouted = FALSE;
      if (*bridgeQos == FALSE &&
          cmsUtl_strcmp(ipCon->connectionType, MDMVS_IP_BRIDGED) == 0)
      {
         ipBridged = TRUE;
      }
      else if (*routeQos == FALSE &&
               cmsUtl_strcmp(ipCon->connectionType, MDMVS_IP_ROUTED) == 0)
      {
         ipRouted = TRUE;
      }
      else
      {
         cmsObj_free((void **)&ipCon);
         continue;
      }
      cmsObj_free((void **)&ipCon);

#ifdef DMP_ETHERNETWAN_1
      /* see if this is wan over Ethernet mode */
      iidTmp = iid;
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_ETH_INTF, MDMOID_WAN_IP_CONN, &iidTmp, OGF_NO_VALUE_UPDATE, (void **)&wethIntf)) == CMSRET_SUCCESS)
      {
         UBOOL8 wethEnabled = wethIntf->enable;

         cmsObj_free((void **)&wethIntf);  /* don't need this object anymore */

         if (wethEnabled)
         {
            if (*bridgeQos == FALSE && ipBridged)
            {
               *bridgeQos = TRUE;
            }
            else if (*routeQos == FALSE && ipRouted)
            {
               *routeQos = TRUE;
            }

            continue;
         }
      }
#endif

      /* find out the link encapsulation mode */
      iidTmp = iid;
      if ((ret = cmsObj_getAncestor(MDMOID_WAN_DSL_INTF_CFG, MDMOID_WAN_IP_CONN, &iidTmp, (void **)&dslIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
         return ret;
      }
      strcpy(linkEncap, dslIntf->linkEncapsulationUsed);
      cmsObj_free((void **)&dslIntf);

      if (cmsUtl_strcmp(linkEncap, MDMVS_G_992_3_ANNEX_K_ATM) == 0)
      {
         /* atm mode */
         WanDslLinkCfgObject  *dslLink = NULL;

         iidTmp = iid;
         if ((ret = cmsObj_getAncestor(MDMOID_WAN_DSL_LINK_CFG, MDMOID_WAN_IP_CONN, &iidTmp, (void **)&dslLink)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
            return ret;
         }

         if (dslLink->X_BROADCOM_COM_ATMEnbQos)
         {
            if (*bridgeQos == FALSE && ipBridged)
            {
               *bridgeQos = TRUE;
            }
            else if (*routeQos == FALSE && ipRouted)
            {
               *routeQos = TRUE;
            }
         }
         cmsObj_free((void **)&dslLink);
      }
#ifdef DMP_PTMWAN_1
      else if (cmsUtl_strcmp(linkEncap, MDMVS_G_993_2_ANNEX_K_PTM) == 0)
      {
         /* ptm mode */
         WanPtmLinkCfgObject  *ptmLink = NULL;

         iidTmp = iid;
         if ((ret = cmsObj_getAncestor(MDMOID_WAN_PTM_LINK_CFG, MDMOID_WAN_IP_CONN, &iidTmp, (void **)&ptmLink)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
            return ret;
         }

         if (ptmLink->X_BROADCOM_COM_PTMEnbQos)
         {
            if (*bridgeQos == FALSE && ipBridged)
            {
               *bridgeQos = TRUE;
            }
            else if (*routeQos == FALSE && ipRouted)
            {
               *routeQos = TRUE;
            }
         }
         cmsObj_free((void **)&ptmLink);
      }
#endif
      else
      {
         cmsLog_error("Invalid linkEncapsulation=%d", linkEncap);
         return CMSRET_INTERNAL_ERROR;
      }
   }  /* while() */

   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
      return ret;
   }

   /* search through all WanPppConnObjects */
   INIT_INSTANCE_ID_STACK(&iid);
   while ((*routeQos == FALSE) &&
          (ret = cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iid, (void **)&pppCon)) == CMSRET_SUCCESS)
   {
      if (!pppCon->enable)
      {
         cmsObj_free((void **)&pppCon);
         continue;
      }

      if (cmsUtl_strcmp(pppCon->connectionType, MDMVS_IP_ROUTED) == 0)
      {
         ipRouted = TRUE;
      }
      else
      {
         cmsObj_free((void **)&pppCon);
         continue;
      }
      cmsObj_free((void **)&pppCon);

      /* find out the link encapsulation mode */
      iidTmp = iid;
      if ((ret = cmsObj_getAncestor(MDMOID_WAN_DSL_INTF_CFG, MDMOID_WAN_PPP_CONN, &iidTmp, (void **)&dslIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
         return ret;
      }
      strcpy(linkEncap, dslIntf->linkEncapsulationUsed);
      cmsObj_free((void **)&dslIntf);

      if (cmsUtl_strcmp(linkEncap, MDMVS_G_992_3_ANNEX_K_ATM) == 0)
      {
         /* atm mode */
         WanDslLinkCfgObject  *dslLink = NULL;

         iidTmp = iid;
         if ((ret = cmsObj_getAncestor(MDMOID_WAN_DSL_LINK_CFG, MDMOID_WAN_PPP_CONN, &iidTmp, (void **)&dslLink)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
            return ret;
         }

         if (dslLink->X_BROADCOM_COM_ATMEnbQos)
         {
            if (*routeQos == FALSE && ipRouted)
            {
               *routeQos = TRUE;
            }
         }
         cmsObj_free((void **)&dslLink);
      }
#ifdef DMP_PTMWAN_1
      else if (cmsUtl_strcmp(linkEncap, MDMVS_G_993_2_ANNEX_K_PTM) == 0)
      {
         /* ptm mode */
         WanPtmLinkCfgObject  *ptmLink = NULL;

         iidTmp = iid;
         if ((ret = cmsObj_getAncestor(MDMOID_WAN_PTM_LINK_CFG, MDMOID_WAN_PPP_CONN, &iidTmp, (void **)&ptmLink)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_getAncestor returns error. ret=%d", ret);
            return ret;
         }

         if (ptmLink->X_BROADCOM_COM_PTMEnbQos)
         {
            if (*routeQos == FALSE && ipRouted)
            {
               *routeQos = TRUE;
            }
         }
         cmsObj_free((void **)&ptmLink);
      }
#endif
      else
      {
         cmsLog_error("Invalid linkEncapsulation=%d", linkEncap);
         return CMSRET_INTERNAL_ERROR;
      }
   }  /* while() */

   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rutQos_checkQosEnabledPVC() */

#endif /* DMP_ADSLWAN_1 */

#ifndef SUPPORT_NF_TABLES
/** This function should really be called activateDefaultClassifiers
 * because it inserts classifications rules that put some basic protocol
 * packets (e.g. dhcp, tr69) in the highest priority queue.  This was the
 * way it is done in TR98.
 *
 * For TR181, we should try to put these classifications in actual
 * classification entries to they are visible to the ACS.
 */
void rutQos_doDefaultPolicy(void)
{

#ifdef SUPPORT_DSL

#if defined (SUPPORT_NF_NAT) || defined(SUPPORT_NF_MANGLE)
   UINT32 i;
#endif
   char cmd[BUFLEN_1024];
   char defaultPolicyScript[BUFLEN_64];
   static UBOOL8 isDefaltPolicySet = FALSE;
   FILE *fpDefaultPolicy;   

   /* make sure that all the required modules for qos support are loaded */
   rutIpt_qosLoadModule();

   /* Check if default policy already set or not. */
   if (isDefaltPolicySet == TRUE)
   {
       cmsLog_notice("Skip rutQos_doDefaultPolicy() since it is done once.");
       return;
   }

   /* Prepare the QoS default policy script.*/
   snprintf(defaultPolicyScript, sizeof(defaultPolicyScript), "/var/qos_default_policy.sh");
   fpDefaultPolicy = fopen(defaultPolicyScript, "w");
   if (fpDefaultPolicy == NULL) {
      cmsLog_error("%s: unable to open file\n", defaultPolicyScript);
      return;
   }

#ifdef SUPPORT_NF_NAT
   /* first, remove all the implicit QoS policies regardless they have been launched or not */
   fprintf(fpDefaultPolicy, "ebtables --concurrent -t nat -D POSTROUTING -j mark --mark-or 0x%x -p ARP\n", XTM_QOS_LEVELS-1);
#endif // SUPPORT_NF_NAT

#ifdef SUPPORT_NF_MANGLE
   for (i = 0; i < sizeOfDefaultClassifications()/sizeof(char *); i++)
   {
      fprintf(fpDefaultPolicy, "iptables -w -t mangle -D OUTPUT -j MARK --or-mark 0x%x %s\n", XTM_QOS_LEVELS-1, defaultClassifications[i]);

#ifdef SUPPORT_IPV6
      fprintf(fpDefaultPolicy, "ip6tables -w -t mangle -D OUTPUT -j MARK --or-mark 0x%x %s\n", XTM_QOS_LEVELS-1, defaultClassifications[i]);
#endif
   }

   /* Remove default policy for DHCPv4 */
   fprintf(fpDefaultPolicy, "iptables -w -t mangle -D OUTPUT -j MARK --or-mark 0x%x -p udp --dport 67:68\n", XTM_QOS_LEVELS-1);

#ifdef SUPPORT_IPV6
   /* remove default policy for dhcpv6 */      
   fprintf(fpDefaultPolicy, "ip6tables -w -t mangle -D OUTPUT -j MARK --or-mark 0x%x -p udp --dport 546:547\n", XTM_QOS_LEVELS-1);   
#endif
#endif // SUPPORT_NF_MANGLE

#ifdef SUPPORT_NF_NAT
   /* Set up the implicit rule to give priority to ARP packets */
   fprintf(fpDefaultPolicy, "ebtables --concurrent -t nat -A POSTROUTING -j mark --mark-or 0x%x -p ARP\n", XTM_QOS_LEVELS-1);
#endif // SUPPORT_NF_NAT

   /* Launch implicit QoS policies on the OUTPUT chains to allow the packets generated
    * by the router itself to be able to go out when the upstream link is saturated by
    * certain high-priority bursty packets such as FTP. In other words, we need to
    * set the highest priority for such packets as DNS probe, SNMP, IGMP, RIP, DHCP relay
    * so that the router can still respond to those protocols. 
    * Note: output chain will NOT affect routing performance.
    */
#ifdef SUPPORT_NF_MANGLE
   for (i = 0; i < sizeOfDefaultClassifications()/sizeof(char *); i++)
   {
      fprintf(fpDefaultPolicy, "iptables -w -t mangle -A OUTPUT -j MARK --or-mark 0x%x %s\n", XTM_QOS_LEVELS-1, defaultClassifications[i]);

#ifdef SUPPORT_IPV6
      fprintf(fpDefaultPolicy, "ip6tables -w -t mangle -A OUTPUT -j MARK --or-mark 0x%x %s\n", XTM_QOS_LEVELS-1, defaultClassifications[i]);
#endif
   }

   /* Add default policy for DHCPv6 */
   fprintf(fpDefaultPolicy, "iptables -w -t mangle -A OUTPUT -j MARK --or-mark 0x%x -p udp --dport 67:68\n", XTM_QOS_LEVELS-1);

#ifdef SUPPORT_IPV6
   /* add default policy for DHCPv6 */
   fprintf(fpDefaultPolicy, "ip6tables -w -t mangle -A OUTPUT -j MARK --or-mark 0x%x -p udp --dport 546:547\n", XTM_QOS_LEVELS-1);
#endif
#endif // SUPPORT_NF_MANGLE

   fclose(fpDefaultPolicy);

   /* Execute the QoS default policy script. */
   snprintf(cmd, sizeof(cmd), "sh %s 2>/dev/null", defaultPolicyScript);
   rut_doSystemAction("rutQos_doDefaultPolicy", cmd);
   snprintf(cmd, sizeof(cmd), "rm -f %s 2>/dev/null", defaultPolicyScript);
   rut_doSystemAction("rutQos_doDefaultPolicy", cmd);
   isDefaltPolicySet = TRUE;

   return;
#else
   /* on non-DSL systems do nothing ?!? */
   return;
#endif /* SUPPORT_DSL */
}  /* End of rutQos_doDefaultPolicy() */
#else
void rutQos_doDefaultPolicy(void)
{
#ifdef SUPPORT_DSL
   char cmd[BUFLEN_1024];

   // remove QosDefault rule first
   snprintf(cmd, sizeof(cmd), "nft delete chain ip_filter QosDefault 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   /* Launch implicit QoS policies on the OUTPUT chains to allow the packets generated
    * by the router itself to be able to go out when the upstream link is saturated by
    * certain high-priority bursty packets such as FTP. In other words, we need to
    * set the highest priority for such packets as DNS probe, SNMP, IGMP, RIP, DHCP relay
    * so that the router can still respond to those protocols. 
    * Note: output chain will NOT affect routing performance.
    */

   // add chain ip mangle OUTPUT { type route hook output priority -150; policy accept; }
   snprintf(cmd, sizeof(cmd), "nft 'add chain ip ip_filter QosDefault { type route hook output priority -150; policy accept; }' 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   // udp dport 53(dns) 67-68(dhcp relay)
   snprintf(cmd, sizeof(cmd), "nft 'add rule ip ip_filter QosDefault udp dport {53, 67, 68} counter meta mark set mark or 0x7' 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   // udp sport 161(snmp), 520(rip)
   snprintf(cmd, sizeof(cmd), "nft 'add rule ip ip_filter QosDefault udp sport {161, 520} counter meta mark set mark or 0x7' 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   // tcp sport 30005(tr69)
   snprintf(cmd, sizeof(cmd), "nft 'add rule ip ip_filter QosDefault tcp sport {30005} counter meta mark set mark or 0x7' 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   snprintf(cmd, sizeof(cmd), "nft 'add rule ip ip_filter QosDefault ip protocol igmp counter meta mark set mark or 0x7' 2>/dev/null");
   rut_doSystemAction("QosDefault", cmd);

   return;
#else
   /* on non-DSL systems do nothing ?!? */
   return;
#endif /* SUPPORT_DSL */
}  /* End of rutQos_doDefaultPolicy() */
#endif

void rutQos_doDefaultDSCPMarkPolicy(QosCommandType cmdType, SINT32 defaultDSCPMark)
{
   unsigned char dscp = 0;
   char cmd[BUFLEN_1024]={0};

   cmsLog_debug("enter: cmd=%d mark=0x%x", cmdType, defaultDSCPMark);

   /* make sure that all the required modules for qos support are loaded */
   rutIpt_qosLoadModule();

   if (defaultDSCPMark == QOS_RESULT_AUTO_MARK)
   {
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -A POSTROUTING -j ftos --8021q-ftos");
      }
      else
      {
         snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -D POSTROUTING -j ftos --8021q-ftos 2>/dev/null");
      }
      rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
   }
   else if (defaultDSCPMark != QOS_RESULT_NO_CHANGE)
   {
      dscp |= defaultDSCPMark;
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         /* Insert in the begining of the chain */
         snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -I POSTROUTING -j dscp --set-dscp 0x%x --dscp-target CONTINUE", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
         snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -A POSTROUTING -j DSCP --set-dscp 0x%x", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
#ifdef SUPPORT_IPV6
         snprintf(cmd, sizeof(cmd), "ip6tables -w -t mangle -A POSTROUTING -j DSCP --set-dscp 0x%x", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
#endif
      }
      else
      {
         snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -D POSTROUTING -j dscp --set-dscp 0x%x --dscp-target CONTINUE 2>/dev/null", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
         snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -D POSTROUTING -j DSCP --set-dscp 0x%x 2>/dev/null", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
#ifdef SUPPORT_IPV6
         snprintf(cmd, sizeof(cmd), "ip6tables -w -t mangle -D POSTROUTING -j DSCP --set-dscp 0x%x 2>/dev/null", dscp);
         rut_doSystemAction("rutQos_doDefaultDSCPMarkPolicy", cmd);
#endif
      }
   }

   return;
}


#ifdef BRCM_WLAN
/** This function is called from rutQos_setDefaultWlQueues() whenever
 *  a SSID (wifi interface) comes up or down.
 */
void rutQos_doDefaultWlPolicy(const char *ifname, UBOOL8 enabled)
{
   char cmd[BUFLEN_256];

   cmsLog_debug("Enter: ifname=%s enabled=%d", ifname, enabled);

   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -D POSTROUTING -o %s -p IPV4 -j wmm-mark --wmm-mark-target CONTINUE 2>/dev/null", ifname);
   rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);
#ifdef SUPPORT_IPV6
   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -D POSTROUTING -o %s -p IPV6 -j wmm-mark --wmm-mark-target CONTINUE 2>/dev/null", ifname);
   rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);
#endif   
   snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -D POSTROUTING -o %s -p 802_1Q -j wmm-mark --wmm-marktag vlan --wmm-mark-target CONTINUE 2>/dev/null", ifname);
   rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);

   if (enabled)
   {
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -A POSTROUTING -o %s -p IPV4 -j wmm-mark --wmm-mark-target CONTINUE", ifname);
      rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);
#ifdef SUPPORT_IPV6
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -A POSTROUTING -o %s -p IPV6 -j wmm-mark --wmm-mark-target CONTINUE", ifname);
      rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);
#endif
      snprintf(cmd, sizeof(cmd), "ebtables --concurrent -t nat -A POSTROUTING -o %s -p 802_1Q -j wmm-mark --wmm-marktag vlan --wmm-mark-target CONTINUE", ifname);
      rut_doSystemAction("rutQos_doDefaultWlPolicy", cmd);
   }
}  /* End of rutQos_doDefaultWlPolicy() */
#endif

CmsRet rutQos_referenceCheck(MdmObjectId oid, SINT32 instance, UBOOL8 *isRefered)
{
   InstanceIdStack iidStack;
   _QMgmtClassificationObject *cObj = NULL;
   SINT32 refInstance;
   CmsRet ret;

   cmsLog_debug("enter");

   if (oid != MDMOID_Q_MGMT_QUEUE && oid != MDMOID_Q_MGMT_POLICER)
   {
      cmsLog_error("Invalid oid %d", oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   *isRefered = FALSE;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->classificationEnable)
      {
         if (oid == MDMOID_Q_MGMT_QUEUE)
         {
            refInstance = cObj->classQueue;
         }
         else
         {
            refInstance = cObj->classPolicer;
         }
         cmsObj_free((void **)&cObj);

         if (refInstance == instance)
         {
            *isRefered = TRUE;
            break;
         }
      }
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rutQos_referenceCheck() */

void rutQos_flushFlows(const char *ifname __attribute__((unused)))
{
#ifdef SUPPORT_FCCTL
   char cmd[BUFLEN_256];

   if (ifname == NULL)
   {
      snprintf(cmd, sizeof(cmd), "fc flush --silent");
   }
   else
   {
      snprintf(cmd, sizeof(cmd), "fc flush --if %s --silent", ifname);
   }

   rut_doSystemAction("rutQos_flushFlows", cmd);
#endif
   return;
}

