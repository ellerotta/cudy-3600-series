/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom 
   All Rights Reserved

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

/*!\file ssk_util.c
 * \brief This lib contains common functions
 *        for all variants of ssk (esp dsl_ssk and intfstack).
 */

#include <string.h>
#include <net/if.h>
#include <linux/if_addr.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "bcmnetlink.h"
#include <errno.h>

#include "cms_log.h"
#include "ssk_util.h"
#include "cms_boardioctl.h"
#include "cms_msg.h"
#include "cms_msg_pubsub.h"
#include "ethswctl_api.h"
#include "bcm_fsutils.h"
#include "bcm_boardutils.h"

#ifdef SUPPORT_DSL
#include "AdslMibDef.h"
#include "devctl_adsl.h"
#endif

#ifdef BUILD_CUSTOMER
/* customer msg handler */
extern void processCustomerMsg(struct nlmsghdr *nl_msgHdr);
#endif

#ifdef BRCM_VOICE_SUPPORT
UBOOL8 isVoiceOnLanSide=FALSE;
UBOOL8 isVoiceOnAnyWan=FALSE;
char * voiceWanIfName=NULL;  /* if this is not null, then voice is bound to this wanIfName */
#endif
/* Voice related callbacks needed at various places in intfstack processing. */
SskVoiceCallbacks sskVoiceCallbacks;

char _myCompName[BDK_COMP_NAME_LEN+1]={0};
char _myAppName[64]={0};
SINT32 _myEid=0;
PppXtmLockEntry pppXtmLockTable[MAX_PPPXTM_LOCK_ENTRIES];

char defaultLanIntfName[SYSUTL_IFNAME_LEN];
char defaultWanIntfName[SYSUTL_IFNAME_LEN];

UBOOL8 isRdpaGBEAEsysLockedIn=FALSE;
CmsTimestamp bootTimestamp;
DLIST_HEAD(sskLinkStatusRecordHead);

UBOOL8 isCmsMode = FALSE;
UBOOL8 isMdmInitDone=FALSE;  // refers to full interface stack init done.
CmsMsgHeader *deferredMsgs=NULL;  // msgs to be sent after MDM_INIT_DONE

// These are internal functions and should be declared static, but then that
// requires complicated #ifdefs.  Use underscore to signify "internal" func.
CmsRet _deferMsg(const CmsMsgHeader *orig);
void _sendDeferredMsgs(void *appMsgHandle);

// variable to help reproduce SWBCACPE-41122.
// Normally, we need to do XTMLOCK.  But we can disable it at runtime to
// reproduce the kernel oops if we don't lock.
int _doXtmLock = 1;


void sskUtil_setMyAppInfo(const char *compName, const char *appName, SINT32 eid)
{
   snprintf(_myCompName, sizeof(_myCompName), "%s", compName);
   snprintf(_myAppName, sizeof(_myAppName), "%s", appName);
   _myEid = eid;

   if (!strcmp(compName, BDK_COMP_CMS_CLASSIC))
      isCmsMode = TRUE;
   
   // Initialize (zeroize) the table.  Only used in BDK case.
   memset(pppXtmLockTable, 0, sizeof(pppXtmLockTable));

   return;
}


/* opens a netlink socket and intiliaze it to recieve messages from
 * kernel for protocol NETLINK_BRCM_MONITOR
 */
SINT32 initBcmNetlinkMonitorFd()
{
#ifdef DESKTOP_LINUX
   return 999;  // TODO: define this somewhere, also in sysutil_net.c
#else
   SINT32 fd = -1;
   CmsRet ret=CMSRET_SUCCESS;
   struct sockaddr_nl addr;

   if ((fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_BRCM_MONITOR)) < 0)
   {
      cmsLog_error("Could not open netlink socket for kernel monitor");
      return -1;
   }
   else
   {
      cmsLog_debug("kernelMonitorFd=%d", fd);
   }

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0;

    if (bind(fd,(struct sockaddr *)&addr,sizeof(addr))<0)
    {
       cmsLog_error("Could not bind netlink socket for kernel monitor");
       close(fd);
       return -1;
    }

   /*send the pid of ssk to kernel,so that it can send events */
   ret = devCtl_boardIoctl(BOARD_IOCTL_SET_MONITOR_FD, 0, "", 0, getpid(), "");
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("registered fd %d with kernel monitor", fd);
   }
   else
   {
      cmsLog_error("failed to register fd %d with kernel monitor, ret=%d", fd, ret);
      close(fd);
      return -1;
   }

   return fd;
#endif  /* DESKTOP_LINUX */
}


/** Receive and process netlink messages from Broadcom proprietary/legacy
 * netlink socket.
 */
void processBcmNetlinkMonitor(int monitorFd, const BcmNetlinkCallbacks *callbacks)
{
   int recvLen;
   char buf[4096]={0};
   struct iovec iov = { buf, sizeof(buf) };
   struct sockaddr_nl nl_srcAddr;
   struct msghdr msg;
   struct nlmsghdr *nl_msgHdr;

   memset(&nl_srcAddr, 0, sizeof(nl_srcAddr));
   memset(&msg, 0, sizeof(msg));
   msg.msg_name = (void*)&nl_srcAddr;
   msg.msg_namelen = sizeof(nl_srcAddr);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;


   cmsLog_notice("Enter: monitorFd=%d callbacks=%p", monitorFd, callbacks);

   recvLen = recvmsg(monitorFd, &msg, 0);
   cmsLog_debug("read %d bytes", recvLen);

   if(recvLen < 0)
   {
      if (errno == EWOULDBLOCK || errno == EAGAIN)
         return ;

      /* Anything else is an error */
      cmsLog_error("read_netlink: Error recvmsg: %d\n", recvLen);
      perror("read_netlink: Error: ");
      return ;
   }

   if(recvLen == 0)
   {
      cmsLog_error("read_netlink: EOF\n");
   }

   /* There can be  more than one message per recvmsg */
   for( nl_msgHdr = (struct nlmsghdr *) buf;
        NLMSG_OK(nl_msgHdr, (unsigned int)recvLen);
        nl_msgHdr = NLMSG_NEXT(nl_msgHdr, recvLen) )
   {
      cmsLog_debug("processing msgType 0x%x (msgHdr=%p buf=%p len=%d)",
                   nl_msgHdr->nlmsg_type, nl_msgHdr, buf, recvLen);
      /* Finish reading */
      if (nl_msgHdr->nlmsg_type == NLMSG_DONE)
         return ;

      /* Message is some kind of error */
      if (nl_msgHdr->nlmsg_type == NLMSG_ERROR)
      {
         cmsLog_error("read_netlink: Message is an error \n");
         return ; // Error
      }

      /*Currently we expect messages only from kernel, make sure
       * the message is from kernel
       */
      if(nl_msgHdr->nlmsg_pid !=0)
      {
         cmsLog_error("netlink message source(%d)is not kernel",nl_msgHdr->nlmsg_pid);
         return;
      }

      /* Call message handler */
      switch (nl_msgHdr->nlmsg_type)
      {
         case MSG_NETLINK_BRCM_WAKEUP_MONITOR_TASK:
         case MSG_NETLINK_BRCM_LINK_STATUS_CHANGED:
            /*process the message */
            cmsLog_debug("received LINK_STATUS_CHANGED message\n");
            if (callbacks->updateLinkStatus != NULL)
               (*callbacks->updateLinkStatus)(NULL);
            break;

#ifdef SUPPORT_DSL_BONDING
         case MSG_NETLINK_BRCM_LINK_TRAFFIC_TYPE_MISMATCH   :
         {
            unsigned int nl_msgData;
            cmsLog_debug("received LINK_TRAFFIC_TYPE_MISMATCH message\n");
            memcpy ((char *) &nl_msgData, NLMSG_DATA(nl_msgHdr), sizeof (nl_msgData)) ;
            processTrafficMismatchMessage(nl_msgData) ;
            break;
         }
#endif

    /* For the 63138 and 63148, implement a workaround to strip bytes and
       allow OAM traffic due to JIRA HW63138-12 */
#if defined (CONFIG_BCM963138) || defined (CONFIG_BCM963148)
          case MSG_NETLINK_BRCM_LINK_OAM_STRIP_BYTE   :
          {
              unsigned int nl_msgData;
              cmsLog_debug("received LINK_OAM_STRIP_BYTE message\n");
              memcpy ((char *) &nl_msgData, NLMSG_DATA(nl_msgHdr), sizeof (nl_msgData)) ;
              processOamStripByte(nl_msgData) ;
              break;
          }
#endif

#ifdef SUPPORT_DSL
         case MSG_NETLINK_BRCM_SAVE_DSL_CFG:
         {
            unsigned int nl_msgData = MSG_ID_BRCM_SAVE_DSL_CFG_ALL;
            cmsLog_debug("received SAVE_DSL_CFG  message\n");
            if(nl_msgHdr->nlmsg_len > NLMSG_HDRLEN)
               memcpy ((char *) &nl_msgData, NLMSG_DATA(nl_msgHdr), sizeof (nl_msgData));
            if (callbacks->processXdslCfgSaveMsg != NULL)
               (*callbacks->processXdslCfgSaveMsg)(nl_msgData);
            break;
         }
         case MSG_NETLINK_BRCM_CALLBACK_DSL_DRV:
            cmsLog_debug("received CALLBACK_DSL_DRV  message\n");
            if (callbacks->xdslDriverCallback != NULL)
               (*callbacks->xdslDriverCallback)(0);
            break;
#endif
#ifdef BUILD_CUSTOMER
         case MSG_NETLINK_BRCM_WIFI_ONOFF_BTN_TASK:
            processCustomerMsg(nl_msgHdr);
            break;
#endif
         default:
            cmsLog_error(" Unknown netlink nlmsg_type %d\n",
                  nl_msgHdr->nlmsg_type);
            break;
      }
   }

   cmsLog_debug("Exit\n");
   return;
}

// Unfortunately there is another copy of this function in bcm_ethsw_hal.
void processStdNetlinkMonitor(int monitorFd, const BcmNetlinkCallbacks *callbacks)
{
   int recvLen;
   char buf[4096];
   struct iovec iov = { buf, sizeof buf };
   struct sockaddr_nl nl_srcAddr;
   struct msghdr msg ;
   struct nlmsghdr *nl_msgHdr;

   memset(&msg,0,  sizeof(struct msghdr));

   msg.msg_name = (void*)&nl_srcAddr;
   msg.msg_namelen = sizeof(nl_srcAddr);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1 ;


   cmsLog_debug("Enter");

   recvLen = recvmsg(monitorFd, &msg, 0);

   cmsLog_debug("recvLen=%d", recvLen);

   if (recvLen <= 0)
   {
      return;
   }

   /* There can be  more than one message per recvmsg */
   for(nl_msgHdr = (struct nlmsghdr *) buf;
       NLMSG_OK (nl_msgHdr, (unsigned int)recvLen);
       nl_msgHdr = NLMSG_NEXT (nl_msgHdr, recvLen))
   {
      char intfNameBuf[CMS_IFNAME_LENGTH];

      memset(intfNameBuf, 0, sizeof(intfNameBuf));

      /* Finish reading */
      if (nl_msgHdr->nlmsg_type == NLMSG_DONE)
      {
         cmsLog_debug("got DONE msg");
         return ;
      }

      /* Message is some kind of error */
      if (nl_msgHdr->nlmsg_type == NLMSG_ERROR)
      {
         cmsLog_error("read_netlink: Message is an error \n");
         return ; // Error
      }

      switch (nl_msgHdr->nlmsg_type)
      {
         case RTM_NEWLINK:
         {
            struct ifinfomsg *info = (struct ifinfomsg *) NLMSG_DATA(nl_msgHdr);
            // cmsLog_debug("got NEWLINK! (len=%d)", nl_msgHdr->nlmsg_len);
            if (0 > cmsNet_getIfnameByIndex(info->ifi_index, intfNameBuf))
            {
               /* ptm0 interface disappears when link goes down, so cannot
                * complain too loudly here.
                */
               cmsLog_debug("Could not find intfName for index %d", info->ifi_index);
            }
            else
            {
               cmsLog_debug("NEWLINK index=%d => %s (flags=0x%x %s %s)",
                            info->ifi_index, intfNameBuf,
                            info->ifi_flags,
                     ((info->ifi_flags & IFF_UP) ? "IFF_UP" : ""),
                     ((info->ifi_flags & IFF_RUNNING) ? "IFF_RUNNING" : ""));
            }

            break;
         }

         case RTM_DELLINK:
         {
            struct ifinfomsg *info = (struct ifinfomsg *) NLMSG_DATA(nl_msgHdr);
            // cmsLog_debug("got DELLINK! (len=%d)", nl_msgHdr->nlmsg_len);
            if (0 > cmsNet_getIfnameByIndex(info->ifi_index, intfNameBuf))
            {
               /* ptm0 interface disappears when link goes down, so cannot
                * complain too loudly here.
                */
               cmsLog_debug("Could not find intfName for index %d", info->ifi_index);
            }
            else
            {
               cmsLog_debug("DELLINK index=%d => %s (flags=0x%x)",
                            info->ifi_index, intfNameBuf, info->ifi_flags);
            }
            break;
         }

         default:
            cmsLog_debug("got %d (len=%d)", nl_msgHdr->nlmsg_type, nl_msgHdr->nlmsg_len);
            break;
      }

      /*
       * Do a little filtering before we do more with this link event.
       * We can definitely handle eth and wl link events.
       * GPON notifies ssk of linkstatus using a CMS msg,
       * so does not need to be handled here.
       * USB, EPON, not yet, but we should also use this mechanism.
       * All new interfaces should use this mechanism.
       * Do not use this mechanism for everything else (DSL, ppp).
       */
      if (!cmsUtl_strncmp(intfNameBuf, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
          !cmsUtl_strncmp(intfNameBuf, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) ||
          !cmsUtl_strncmp(intfNameBuf, USB_IFC_STR, strlen(USB_IFC_STR)) ||
          !cmsUtl_strncmp(intfNameBuf, EPON_IFC_STR, strlen(EPON_IFC_STR)) ||
          !cmsUtl_strncmp(intfNameBuf, ETHLAG_IFC_STR, strlen(ETHLAG_IFC_STR)))
      {
         if (callbacks->updateLinkStatus != NULL)
            (*callbacks->updateLinkStatus)(intfNameBuf);
      }
   }

   cmsLog_debug("exit");

   return;
}


#ifdef DMP_DEVICE2_BASELINE_1

// Given parent bridge intf name, br0, br1, fill buf with comma separated
// list of children intf names.
static void getChildrenIntfNames(const char *parentBridgeName,
                                 char *buf, UINT32 bufLen)
{
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   CmsRet ret;

   memset(buf, 0, bufLen);

   // First find the right parent bridge
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, parentBridgeName))
      {
         found = TRUE;
         // Loop through all the non-management children
         while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                           &brIidStack, &brPortIidStack, OGF_NO_VALUE_UPDATE,
                           (void **)&brPortObj)) == CMSRET_SUCCESS)
         {
            if (!brPortObj->managementPort)
            {
               UINT32 i = strlen(buf);
               if (i == 0)
               {
                  snprintf(buf, bufLen, "%s", brPortObj->name);
               }
               else
               {
                  if (i + strlen(brPortObj->name) + 2 > bufLen)
                  {
                     cmsLog_error("too many intfNames, truncated! %s %s",
                                  buf, brPortObj->name);
                  }
                  else
                  {
                     snprintf(&buf[i], bufLen-i, ",%s", brPortObj->name);
                  }
               }
            }
            cmsObj_free((void **)&brPortObj);
         }
      }
      cmsObj_free((void **)&brObj);
   }
   return;
}

static SINT32 getBridgeIndex(const char *bridgeName)
{
   Dev2BridgeObject *brObj=NULL;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   SINT32 bridgeIndex = -1;
   CmsRet ret;

   // First find the right parent bridge
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridgeName))
      {
         found = TRUE;
         bridgeIndex = brObj->X_BROADCOM_COM_Index;
      }
      cmsObj_free((void **)&brObj);
   }

   return bridgeIndex;
}

static void getBridgeType(const char *bridgeName, char *buffer, size_t size)
{
   Dev2BridgeObject *brObj=NULL;
   InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   CmsRet ret;

   // First find the right parent bridge
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&brObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridgeName))
      {
         found = TRUE;
         cmsUtl_strncpy(buffer, brObj->X_BROADCOM_COM_Type, size);
      }
      cmsObj_free((void **)&brObj);
   }
   return;
}

void sendIpInterfaceEvenMsgForBridge(void *appMsgHandle, const char *mgmtBridgePortFullpath)
{
   Dev2BridgePortObject *brPortObj=NULL;
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   char *ipIntfFullpath=NULL;
   UBOOL8 isLayer2=FALSE;
   CmsRet ret;

   ret = cmsMdm_fullPathToPathDescriptor(mgmtBridgePortFullpath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Invalid fullpath %s, ret=%d",
                    mgmtBridgePortFullpath, ret);
      return;
   }

   ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                    (void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get object for %s, ret=%d",
                    mgmtBridgePortFullpath, ret);
      return;
   }

   ret = qdmIntf_intfnameToFullPathLocked_dev2(brPortObj->name,
                                               isLayer2, &ipIntfFullpath);
   if (ret == CMSRET_SUCCESS)
   {
      sendIpInterfaceEventMsg(appMsgHandle, ipIntfFullpath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullpath);
   }
   else
   {
      cmsLog_error("Could not find IP intf fullpath for %s", brPortObj->name);
   }
   cmsObj_free((void **) &brPortObj);
   return;
}


void sendIpInterfaceEventMsg(void *appMsgHandle, const char *ipIntfFullpath)
{
   sendIpInterfaceEventMsgEx(appMsgHandle, ipIntfFullpath, TRUE, NULL);
}

void sendIpInterfaceEventMsgEx(void *appMsgHandle,
                               const char *ipIntfFullpath, UBOOL8 pub,
                               const char *additionalStrData)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   CmsMsgHeader *msgHdr=NULL;
   PubSubInterfaceMsgBody *msgBody=NULL;
   UINT32 totalLen=0;
   CmsRet ret;

   cmsLog_notice("Entered: %s (pub=%d)", ipIntfFullpath, pub);

   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullpath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s to pathDesc, ret=%d",
                    ipIntfFullpath, ret);
      return;
   }

   totalLen = sizeof(CmsMsgHeader) + sizeof(PubSubInterfaceMsgBody) + cmsUtl_strlen(additionalStrData) + 1;
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed, len=%d", totalLen);
      return;
   }

   msgHdr->src = _myEid;
   // In BDK, if this code is not running in sysmgmt, it will get forwarded to
   // the local MD.
   // In CMS Classic, smd will intercept this message.
   msgHdr->dst = EID_SYSMGMT_MD;
   msgHdr->type = (pub) ? CMS_MSG_PUBLISH_EVENT : CMS_MSG_UNPUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = totalLen - sizeof(CmsMsgHeader);

   msgBody = (PubSubInterfaceMsgBody *) (msgHdr+1);

   // If PUBLISH, need to fill in the data.
   if (pub)
   {
      // TODO: still does not handle brx being deleted.  (needs UNPUBLISH)
      // TODO: Jane this function now supports UNPUBLISH, but I don't know
      // what to do with these children interfaces when the layer 3 (LAN) bridge is deleted.  can you fill in later?
      ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, &pathDesc.iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&ipIntfObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IP.Interface object, ret=%d", ret);
         cmsMem_free(msgHdr);
         return;
      }

      snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", ipIntfObj->name);

      if (!strncmp(msgBody->intfName, "br", 2))
      {
         getChildrenIntfNames(ipIntfObj->name, msgBody->childrenIntfNames,
                              sizeof(msgBody->childrenIntfNames));
         getBridgeType(ipIntfObj->name, msgBody->bridgeType, sizeof(msgBody->bridgeType));
         msgBody->bridgeIndex = getBridgeIndex(ipIntfObj->name);
      }
      msgBody->isUpstream = ipIntfObj->X_BROADCOM_COM_Upstream;
      msgBody->isLayer3 = 1;
      msgBody->isIpv4Enabled = ipIntfObj->IPv4Enable;
      msgBody->isIpv4Up = strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP) ? 0 : 1;
      if (msgBody->isIpv4Up)
      {
         sskConn_getAnyIpv4AddressLocked(&pathDesc.iidStack, msgBody->ipv4Addr,
                                         msgBody->ipv4Netmask);

         // get gateway/router for this interface
         sskConn_getIpv4GatewayForIpIntfLocked(ipIntfFullpath, msgBody->ipv4Gateway);
      }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      msgBody->isIpv6Enabled = ipIntfObj->IPv6Enable;
      msgBody->isIpv6Up = strcmp(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP) ? 0 : 1;
      if (msgBody->isIpv6Up)
      {
         sskConn_getAnyIpv6AddressLocked(&pathDesc.iidStack, msgBody->ipv6GlobalAddr);

         // get next-hop/router for this interface
         sskConn_getIpv6NextHopForIpIntfLocked(ipIntfFullpath, msgBody->ipv6NextHop);
      }
#endif

      // DNS servers for/from this IP interface(independent of IPv4 or IPv6)
      // Not the same as statically configured DNS servers.
      sskConn_getDnsServersForIpIntfLocked(ipIntfObj->name, msgBody->dnsServers);

      snprintf(msgBody->pubCompName, sizeof(msgBody->pubCompName), "%s", _myCompName);
      snprintf(msgBody->fullpath, sizeof(msgBody->fullpath), "%s", ipIntfFullpath);
      if (additionalStrData != NULL)
      {
         strcpy((char *)(msgBody+1), additionalStrData);
      }

      if (cmsMdm_isBdkSysmgmt())
      {
         // If defaultLanIntfName has not been set yet, then take the first 
         // non-Upstream ip intf that has either groupname of "Default" or
         // intf name of "br0".  This is useful to support the
         // boundIfName=LAN setting in tr69c and maybe other apps.
         // Note this logic is not quite the same as qdmIpIntf_getDefaultLanIntfNameLocked_dev2
         // but probably good enough.  Unify later.  Also does not handle the
         // scenario where the default LAN intf is removed (this algo does not
         // select another intf).
         if (IS_EMPTY_STRING(defaultLanIntfName))
         {
            // defaultLan not selected yet, see if this is a candidate.
            if ((ipIntfObj->X_BROADCOM_COM_Upstream == FALSE) &&
                (!cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default") ||
                 !cmsUtl_strcmp(ipIntfObj->name, "br0")))
            {
               snprintf(defaultLanIntfName, sizeof(defaultLanIntfName), "%s", ipIntfObj->name);
               snprintf(msgBody->tags, sizeof(msgBody->tags), PUBSUB_INTF_TAG_DEFAULT_LAN);
               cmsLog_notice("Set %s on %s", msgBody->tags, defaultLanIntfName);
            }
         }
         else
         {
            // defaultLan has been selected, see if it is this one.
            // Must continue to add the tag to updates of this interface.
            if (!cmsUtl_strcmp(defaultLanIntfName, ipIntfObj->name))
            {
               snprintf(msgBody->tags, sizeof(msgBody->tags), PUBSUB_INTF_TAG_DEFAULT_LAN);
            }
         }

         // Set first routed WAN interface that comes up as "Default_WAN", which
         // is used to support the boundIfName=ANY_WAN setting in tr69c and maybe
         // other apps.  Note this does not handle scenario where this wan intf
         // goes down and some other wan interface comes up.  So this logic is not
         // quite the same as rutWan_findFirstIpvxRoutedAndConnected(), but
         // probably good enough.  Unify later.
         if (IS_EMPTY_STRING(defaultWanIntfName))
         {
            // defaultWan not selected yet, see if this is a candidate.
            if ((ipIntfObj->X_BROADCOM_COM_Upstream == TRUE) &&
                (msgBody->isIpv4Up || msgBody->isIpv6Up))
            {
               snprintf(defaultWanIntfName, sizeof(defaultWanIntfName), "%s", ipIntfObj->name);
               snprintf(msgBody->tags, sizeof(msgBody->tags), PUBSUB_INTF_TAG_DEFAULT_WAN);
               cmsLog_notice("Set %s on %s", msgBody->tags, defaultWanIntfName);
            }
         }
         else
         {
            // defaultWan has been selected, see if it is this one.
            // Must continue to add the tag to updates of this interface.
            if (!cmsUtl_strcmp(defaultWanIntfName, ipIntfObj->name))
            {
               snprintf(msgBody->tags, sizeof(msgBody->tags), PUBSUB_INTF_TAG_DEFAULT_WAN);
            }
         }
      }
   }
   else
   {
      // UNPUBLISH
      // Due to sequence of operations, by the time we get here, the IP
      // interface has already been deleted from MDM.  So must query
      // sys_directory to get the intfName.
      char queryStr[1024]={0};
      char *respStr = NULL;

      snprintf(queryStr, sizeof(queryStr), "fullpath=%s", ipIntfFullpath);
      ret = sskUtil_queryIntfName(appMsgHandle, EID_SYSMGMT_MD, queryStr, &respStr);
      if (ret == CMSRET_SUCCESS)
      {
         snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", respStr);

         //reset defaultWanIntfName
         if (!cmsUtl_strcmp(defaultWanIntfName, respStr))
         {
            memset(defaultWanIntfName, 0, sizeof(defaultWanIntfName));
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(respStr);
      }
      else
      {
         // could not find this fullpath in sys_directory, nothing to delete.
         // This could happen if ppp does not come up so never published in
         // sys_directory.
         cmsLog_debug("Could not find intfName for %s in sys_directory", ipIntfFullpath);
         cmsMem_free(msgHdr);
         return;
      }
   }

   if (isMdmInitDone || isCmsMode)
      ret = cmsMsg_send(appMsgHandle, msgHdr);
   else
      ret = _deferMsg(msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send interface event msg, ret=%d", ret);
   }
   cmsLog_debug("msg sent: ret=%d", ret);

   cmsMem_free(msgHdr);
   cmsObj_free((void **) &ipIntfObj);
   return;
}


void sendLayer2InterfaceEventMsg(void *appMsgHandle, const char *status,
                                 const MdmPathDescriptor *pathDesc)
{
   sendLayer2InterfaceEventMsgEx(appMsgHandle, status, pathDesc, TRUE, NULL);
}

void sendLayer2InterfaceEventMsgEx(void *appMsgHandle, const char *status,
                                   const MdmPathDescriptor *pathDesc,
                                   UBOOL8 pub,
                    const char *additionalStrData __attribute__((unused)))
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubInterfaceMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubInterfaceMsgBody *msgBody = (PubSubInterfaceMsgBody *) (msgHdr+1);
   char l2FullpathBuf[sizeof(msgBody->fullpath)] = {0};
   CmsRet ret;

   if (bcmUtl_isShutdownInProgress())
   {
      cmsLog_notice("system shutdown in progress, do nothing");
      return;
   }

   // TODO: in order to support additionalStrData, the msg buf must be
   // dynamically allocated.  Also the dsl_hal_thread and dsl_md path
   // needs enhancement.

   cmsLog_notice("Entered: oid=%d status=%s (pub=%d)",
                 pathDesc->oid, status, pub);

   {
      char *l2Fullpath=NULL;
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(pathDesc, &l2Fullpath);
      if (ret == CMSRET_SUCCESS)
      {
         // copy to local buffer so we don't have to worry about freeing
         strncpy(l2FullpathBuf, l2Fullpath, sizeof(l2FullpathBuf)-1);
         CMSMEM_FREE_BUF_AND_NULL_PTR(l2Fullpath);
      }
      else
      {
         cmsLog_error("Could not get fullpath for oid=%d iidStack=%s ret=%d",
                   pathDesc->oid, cmsMdm_dumpIidStack(&pathDesc->iidStack), ret);
         return;
      }
   }

   if (pathDesc->oid == MDMOID_DEV2_BRIDGE_PORT)
   {
      Dev2BridgePortObject *bridgePortObj=NULL;
      ret = cmsObj_get(pathDesc->oid, &pathDesc->iidStack, OGF_NO_VALUE_UPDATE,
                       (void **)&bridgePortObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get Bridge Port object, ret=%d", ret);
         return;
      }

      // Don't send for bridge mgmt port, with is also called br0.  Nobody
      // cares about the state of the mgmt port anyways.
      // Wifi SSID are also under bridge port.  Wifi interfaces are handled
      // separately, so don't send any updates out for those.
      // Bridge port is mainly/only? for ethernet.
      // TODO: fix intfStack code to only call this for ethernet?
      if ((bridgePortObj->managementPort) ||
          (!strncmp(bridgePortObj->name, "wl", 2)))
      {
         cmsObj_free((void **) &bridgePortObj);
         return;
      }

      // Fill in data specific to this interface
      cmsLog_debug("name=%s", bridgePortObj->name);
      snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", bridgePortObj->name);
      msgBody->isUpstream = 0;  // TODO: ethWan would be upstream
      cmsObj_free((void **) &bridgePortObj);
   }
   else if (pathDesc->oid == MDMOID_DEV2_PTM_LINK)
   {
      if (pub)
      {
         Dev2PtmLinkObject *ptmObj=NULL;
         ret = cmsObj_get(pathDesc->oid, &pathDesc->iidStack, OGF_NO_VALUE_UPDATE,
                          (void **)&ptmObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get PTM_LINK object, ret=%d", ret);
            return;
         }

         // Fill in data specific to this interface
         cmsLog_debug("PTM link name=%s", ptmObj->name);
         snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", ptmObj->name);
         msgBody->isUpstream = 1;  // DSL layer 2 is always upstream
         cmsObj_free((void **) &ptmObj);
      }
      else
      {
         // UNPUBLISH
         // Due to sequence of operations, by the time we get here, the xtm
         // link has already been deleted from MDM.  So must query
         // sys_directory to get the intfName.
         char queryStr[128+sizeof(l2FullpathBuf)]={0};
         char *respStr = NULL;

         snprintf(queryStr, sizeof(queryStr), "fullpath=%s", l2FullpathBuf);
         ret = sskUtil_queryIntfName(appMsgHandle, EID_DSL_HAL_THREAD, queryStr, &respStr);
         if (ret == CMSRET_SUCCESS)
         {
            snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", respStr);
            CMSMEM_FREE_BUF_AND_NULL_PTR(respStr);
         }
         else
         {
            // Could not find this l2Fullpath in sys_directory, so no need
            // to UNPUBLISH.  In BDK, this would be a bit odd.  But in RDK,
            // this is normal.  So only log it at debug.
            cmsLog_debug("Could not find intfName for %s in sys_directory", queryStr);
            return;
         }
      }
   }
   else if (pathDesc->oid == MDMOID_DEV2_ATM_LINK)
   {
      if (pub)
      {
         Dev2AtmLinkObject *atmObj=NULL;
         ret = cmsObj_get(pathDesc->oid, &pathDesc->iidStack, OGF_NO_VALUE_UPDATE,
                          (void **)&atmObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get ATM_LINK object, ret=%d", ret);
            return;
         }

         // Fill in data specific to this interface
         cmsLog_debug("ATM link name=%s", atmObj->name);
         snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", atmObj->name);
         msgBody->isUpstream = 1;  // DSL layer 2 is always upstream
         cmsObj_free((void **) &atmObj);
      }
      else
      {
         // UNPUBLISH (see comments in the PTM case above).
         char queryStr[128+sizeof(l2FullpathBuf)]={0};
         char *respStr = NULL;

         snprintf(queryStr, sizeof(queryStr), "fullpath=%s", l2FullpathBuf);
         ret = sskUtil_queryIntfName(appMsgHandle, EID_DSL_HAL_THREAD, queryStr, &respStr);
         if (ret == CMSRET_SUCCESS && respStr)
         {
            snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", respStr);
            CMSMEM_FREE_BUF_AND_NULL_PTR(respStr);
         }
         else
         {
            cmsLog_debug("Could not find intfName for %s in sys_directory", queryStr);
            return;
         }
      }
   }
   else if (pathDesc->oid == MDMOID_DEV2_WIFI_SSID)
   {
      Dev2WifiSsidObject *ssidObj=NULL;

      ret = cmsObj_get(pathDesc->oid, &pathDesc->iidStack, OGF_NO_VALUE_UPDATE,
                       (void **)&ssidObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get SSID object, ret=%d", ret);
         return;
      }

      // Fill in data specific to this interface
      cmsLog_debug("wireless interface name=%s wlbrIndex=%d",
                   ssidObj->name, ssidObj->X_BROADCOM_COM_WlBrIndex);
      snprintf(msgBody->intfName, sizeof(msgBody->intfName), "%s", ssidObj->name);
      msgBody->wlBrIndex = ssidObj->X_BROADCOM_COM_WlBrIndex;
      msgBody->isUpstream = 0;  // wifi could be used as upstream?
      cmsObj_free((void **) &ssidObj);
   }
   else
   {
      cmsLog_error("Unsupported oid %d", pathDesc->oid);
      return;
   }

   // Fill in commomn parts of the msg
   msgHdr->src = _myEid;
   // In BDK, if this code is not running in sysmgmt, it will get forwarded to
   // the local bus manager.
   // In CMS Classic, smd will intercept this message.
   msgHdr->dst = EID_SYSMGMT_MD;
   msgHdr->type = (pub) ? CMS_MSG_PUBLISH_EVENT : CMS_MSG_UNPUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = sizeof(PubSubInterfaceMsgBody);

   if (pub)
   {
      // This info is only needed for PUBLISH
      msgBody->isLayer2 = 1;
      msgBody->isLinkUp = strcmp(status, MDMVS_UP) ? 0 : 1;
      snprintf(msgBody->pubCompName, sizeof(msgBody->pubCompName), "%s", _myCompName);

      // The fullpath is needed to connect a layer 2 intf in a remote component
      // to the interface stack in the sysmgmt component.
      snprintf(msgBody->fullpath, sizeof(msgBody->fullpath), "%s", l2FullpathBuf);
   }

   cmsLog_debug("ready to send, isMdmInitDone=%d isCmsMode=%d", isMdmInitDone, isCmsMode);
   if (isMdmInitDone || isCmsMode)
      ret = cmsMsg_send(appMsgHandle, msgHdr);
   else
      ret = _deferMsg(msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send L2 interface event msg, ret=%d", ret);
   }
   cmsLog_debug("msg sent!");
   return;
}

void sendPppXtmLockMsg(void *appMsgHandle, const char *xtmLinkFullpath, const char *lockStr)
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)+BDK_PPPXTM_VALUE_LEN]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubKeyValueMsgBody *keyBody = (PubSubKeyValueMsgBody *) (msgHdr+1);
   char *value = (char *) (keyBody+1);
   CmsRet ret;

   if ((xtmLinkFullpath == NULL) || (lockStr == NULL))
   {
      cmsLog_error("NULL input args (xtmLinkFullpath:%p/lockStr:%p)", xtmLinkFullpath, lockStr);
      return;
   }

   cmsLog_notice("Entered: xtmLinkFullpath=%s lockStr=%s", xtmLinkFullpath, lockStr);

   msgHdr->src = _myEid;
   msgHdr->dst = EID_SYSMGMT_MD;
   msgHdr->type = CMS_MSG_PUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody) + strlen(lockStr) + 1;

   snprintf(keyBody->key, sizeof(keyBody->key), "%s%s",
            BDK_PPPXTM_LOCK_PREFIX, xtmLinkFullpath);
   keyBody->valueLen = strlen(lockStr) + 1;

   strncpy(value, lockStr, BDK_PPPXTM_VALUE_LEN-1);

   if (_doXtmLock)
   {
      cmsLog_debug("Sending PUBLISH key %s value %s", keyBody->key, value);
      if ((ret = cmsMsg_send(appMsgHandle, msgHdr)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to PPPXTM lock msg, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("PPPXTM lock msg sent! (%s=%s)", keyBody->key, lockStr);
      }
   }
   else
   {
      cmsLog_error("DEBUG mode activated: skip XTM lock for %s (key=%s)",
                   xtmLinkFullpath, keyBody->key);
   }

   // Since we are unlocking this, also unpublish it so the key can be
   // deleted.
   if (IS_EMPTY_STRING(lockStr))
   {
      msgHdr->type = CMS_MSG_UNPUBLISH_EVENT;
      cmsLog_debug("Sending UNPUBLISH key %s", keyBody->key);
      if ((ret = cmsMsg_send(appMsgHandle, msgHdr)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to PPPXTM unpublish msg, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("PPPXTM unpublish msg sent! (%s)", keyBody->key);
      }
   }
   return;
}

void sendSubscribePppXtmLockMsg(void *appMsgHandle, const char *xtmLinkFullpath, UBOOL8 sub)
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubKeyValueMsgBody *keyBody = (PubSubKeyValueMsgBody *) (msgHdr+1);
   CmsRet ret;

   if (xtmLinkFullpath == NULL)
   {
      cmsLog_error("xtmLinkFullpath is NULL!");
      return;
   }

   cmsLog_notice("Entered: xtmLinkFullpath=%s subscribe=%d",
                 xtmLinkFullpath, sub);

   msgHdr->src = _myEid;
   msgHdr->dst = EID_DSL_HAL_THREAD;
   msgHdr->type = (sub ? CMS_MSG_SUBSCRIBE_EVENT : CMS_MSG_UNSUBSCRIBE_EVENT);
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->flags_request = (sub ? 1 : 0);  // subscribe is a request
   msgHdr->flags_event = (sub ? 0 : 1);    // unsubscribe is an event
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody);

   snprintf(keyBody->key, sizeof(keyBody->key), "%s%s",
            BDK_PPPXTM_LOCK_PREFIX, xtmLinkFullpath);

   cmsLog_debug("Sending msgType 0x%x key=%s (src=%d dst=%d dataLength=%d)",
                msgHdr->type, keyBody->key,
                msgHdr->src, msgHdr->dst, msgHdr->dataLength);

   // This function is called by dsl_ssk during interface stack processing,
   // lock is held.  Send message but do not wait for reply so that dsl_ssk
   // will release the lock.  dsl_md might need to process some other DBus
   // request first (which requires the lock) before dsl_md can process this
   // request.  The response will be handled in the main dsl_ssk loop.
   if (isMdmInitDone)
      ret = cmsMsg_send(appMsgHandle, msgHdr);
   else
      ret = _deferMsg(msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send subscribe/unsubscribe of %s, ret=%d",
                   keyBody->key, ret);
   }

   return;
}


/*
 * Documentation of the PPP XTM lock mechanism.
 * Even though it is specific to DSL, the code may be called from outside
 * #ifdef DSL, so these functions are also outside of #ifdef DSL.
 * It is only used in TR181, so it is inside #ifdef DEVICE2_BASELINE
 * Background: if we have a wan connection that is
 * DSL -> {ATM_LINK or PTM_LINK} -> bcm_vlan -> ppp -> IP
 * and DSL line goes down, as we propagate status up the interface stack,
 * we will bring ATM or PTM link down first.  But this causes an unnatural
 * ordering of events in the kernel because the underlying interface (ptm0)
 * is deleted before the upper vlan interface (ptm0.1) is deleted, so we defer
 * bringing XTM link down until after the bcm_vlan and PPP
 * interfaces are brought down first.  In CMS Classic intf stack, this is done
 * by setStatusOnPppFirst.
 *
 * But in BDK, DSL and bcm_vlan+PPP+IP are in different components, so we
 * need the same idea but different mechanism.  So in BDK, we use key-value
 * in sys_directory and pppXtmLockTable to keep track of state.  When
 * a XTM link comes up, the DSL component registers this link in both the
 * sys_directory and pppXtmLockTable.  When sysmgmt brings up a PPP+IP intf
 * on this XTM link, it "locks" it by publishing the value "LOCKED" to
 * sys_directory.  Sys_directory notifies dsl_ssk, which sets the isLocked
 * field in the pppXtmLockTable entry to TRUE.  When the XTM link goes down,
 * intfStack code checks if the XTM link is locked, if so, it defers bringing
 * down the XTM link. (link status is sent to sys_directory as usual).
 * When sysmgmt finally brings down the PPP+IP intf, it writes an empty 
 * string to the key in sys_directory.  Sys_directory notifies dsl_ssk,
 * when then brings the XTM link down.
 */

void pppXtm_dumpTable()
{
   PppXtmLockEntry *entry;
   UINT32 i;

   for (i=0; i < MAX_PPPXTM_LOCK_ENTRIES; i++)
   {
      entry = &(pppXtmLockTable[i]);
      if (entry->fullpath[0] != '\0')
         printf("[%d] %s (isLocked=%d isLinkUp=%d)\n",
                i, entry->fullpath, entry->isLocked, entry->isLinkUp);
      else
         printf("[%d] avail\n", i);
   }
}

PppXtmLockEntry *pppXtm_getAvailableEntry()
{
   UINT32 i;
   for (i=0; i < MAX_PPPXTM_LOCK_ENTRIES; i++)
   {
      if (pppXtmLockTable[i].fullpath[0] == '\0')
      {
         return (&(pppXtmLockTable[i]));
      }
   }
   cmsLog_error("Could not find empty entry (%d)!", MAX_PPPXTM_LOCK_ENTRIES);
   pppXtm_dumpTable();
   return NULL;
}

PppXtmLockEntry *pppXtm_getEntry(const char *xtmLinkFullpath)
{
   UINT32 i;
   for (i=0; i < MAX_PPPXTM_LOCK_ENTRIES; i++)
   {
      if (!strcmp(xtmLinkFullpath, pppXtmLockTable[i].fullpath))
      {
         return (&(pppXtmLockTable[i]));
      }
   }
   return NULL;
}

// Called by dsl_ssk intfStack when the XTM link is deleted.
void pppXtm_delEntry(const char *xtmLinkFullpath)
{
   PppXtmLockEntry *entry=NULL;

   cmsLog_debug("Entered: %s", xtmLinkFullpath);

   entry = pppXtm_getEntry(xtmLinkFullpath);
   if (entry == NULL)
   {
      // If the link never came up, there would be no entry in the table.
      cmsLog_debug("Could not find %s (ignored)", xtmLinkFullpath);
      return;
   }
   memset(entry, 0, sizeof(PppXtmLockEntry));
   return;
}

// Called by dsl_ssk intfStack when XTM link goes up.  Will also add
// xtmlinkFullpath to the table.  It is ok if entry already exists.
void pppXtm_linkUpAddEntry(const char *xtmLinkFullpath)
{
   PppXtmLockEntry *entry=NULL;

   cmsLog_debug("Entered: %s", xtmLinkFullpath);

   entry = pppXtm_getEntry(xtmLinkFullpath);  // get existing entry, if any.
   if (entry == NULL)
   {
      entry = pppXtm_getAvailableEntry();
      if (entry == NULL)
         return;

      cmsUtl_strncpy(entry->fullpath, xtmLinkFullpath, sizeof(entry->fullpath));
   }

   entry->isLinkUp = TRUE;
   return;
}

// Called by dsl_ssk intfStack when XTM link goes down. Returns TRUE if the
// xtmlink is currently locked, which means intfStack should not bring the xtm
// intf down.  On the other hand, if not locked, intfStack can bring the xtm
// intf down. 
UBOOL8 pppXtm_linkDownIsLocked(const char *xtmLinkFullpath)
{
   PppXtmLockEntry *entry=NULL;

   cmsLog_debug("Entered: %s", xtmLinkFullpath);

   entry = pppXtm_getEntry(xtmLinkFullpath);
   if (entry == NULL)
   {
      // This happens on system bootup when xtm link is initially down.
      return FALSE;
   }

   entry->isLinkUp = FALSE;
   cmsLog_debug("%s isLocked=%d", xtmLinkFullpath, entry->isLocked);
   return (entry->isLocked);
}

// Called by dsl_ssk when it receives a LOCK event notification from sysmgmt.
// If lock==TRUE, just set isLocked in the entry to TRUE.
// If lock==FALSE, then isLinkUp should also be FALSE, so this means sysmgmt
// is telling dsl_ssk it is now safe to delete the XTM intf.
void pppXtm_updateLockState(void *appMsgHandle, const char *xtmLinkFullpath, UBOOL8 lock)
{
   PppXtmLockEntry *entry=NULL;

   cmsLog_debug("Entered: %s lock=%d", xtmLinkFullpath, lock);

   entry = pppXtm_getEntry(xtmLinkFullpath);
   if (entry == NULL)
   {
      cmsLog_error("Could not find entry for %s", xtmLinkFullpath);
      return;
   }

   cmsLog_debug("Current state isLinkUp=%d isLocked=%d",
                entry->isLinkUp, entry->isLocked);

   if (lock)
   {
      /* lock case is easy, just mark the entry as locked */
      if (entry->isLinkUp == FALSE)
      {
         // This could happen if dsl link goes up and then down very quickly,
         // and sysmgmt takes a long time to set up the IP connection.
         cmsLog_debug("during lock: unexpected state on %s (link is down! isLocked=%d)",
                      xtmLinkFullpath, entry->isLocked);
      }
      entry->isLocked = TRUE;
   }
   else
   {
      /* unlock case */
      if (entry->isLinkUp == TRUE)
      {
         // The IP interface could have gone down even though xtm link is still
         // up.  Do nothing in this block.
      }
      else
      {
         // Real xtm link state is down, and going from LOCKED to unlocked.
         // Do the delayed down action on xtm intf.
         if (entry->isLocked)
         {
            if (intfStack_optionalCheckForEnabledByFullpath(xtmLinkFullpath))
            {
               cmsLog_notice("Doing delayed down action on enabled %s, new status=LOWERLAYERDOWN", xtmLinkFullpath);
               intfStack_setStatusByFullPathLocked(xtmLinkFullpath, MDMVS_LOWERLAYERDOWN);
            }
            else
            {
               cmsLog_notice("Doing delayed down action on disabled %s, new status=DOWN", xtmLinkFullpath);
               intfStack_setStatusByFullPathLocked(xtmLinkFullpath, MDMVS_DOWN);
            }
            sendSubscribePppXtmLockMsg(appMsgHandle, xtmLinkFullpath, FALSE);
         }
      }
      entry->isLocked = FALSE;
   }
   return;
}

#endif  /* DMP_DEVICE2_BASELINE_1 */


void sendSubscribeKeyMsg(void *appMsgHandle, CmsEntityId dst, const char *key)
{
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(PubSubKeyValueMsgBody)]={0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   PubSubKeyValueMsgBody *keyBody = (PubSubKeyValueMsgBody *) (msgHdr+1);
   CmsRet ret;

   cmsLog_notice("Entered: dst=%d key=%s", dst, key);

   msgHdr->src = _myEid;
   msgHdr->dst = dst;
   msgHdr->type = CMS_MSG_SUBSCRIBE_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->flags_request = 1;
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody);

   snprintf(keyBody->key, sizeof(keyBody->key), "%s", key);

   // For consistency with sendSubscribePppXtmLockMsg, just send here
   // and get the reply message in the main app (processSubscribeEventMsg).
   ret = cmsMsg_send(appMsgHandle, msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send subscribe of %s, ret=%d",
                   keyBody->key, ret);
   }

   return;
}

void sendStatusMsgToSmd(void *appMsgHandle, CmsMsgType msgType, const char *ifName)
{
   CmsMsgHeader *msgHdr;
   char *dataPtr;
   UINT32 dataLen=0;
   CmsRet ret;

   cmsLog_debug("sending status msg 0x%x", msgType);

   if (ifName != NULL)
   {
      dataLen = strlen(ifName)+1;
      cmsLog_debug("ifName=%s", ifName);
   }

   if (bcmUtl_isShutdownInProgress())
   {
      cmsLog_notice("system down");
      return;
   }


   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) + dataLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed, len of ifName=%d", dataLen);
      return;
   }

   msgHdr->src = EID_SSK;
   msgHdr->dst = EID_SMD;
   msgHdr->type = msgType;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = dataLen;

   if (ifName != NULL)
   {
      dataPtr = (char *) (msgHdr+1);
      strcpy(dataPtr, ifName);
   }

   if ((ret = cmsMsg_send(appMsgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send event msg 0x%x to smd, ret=%d", msgType, ret);
   }

   cmsMem_free(msgHdr);

   return;
}


// In very early startup, especially for DSL, dsl_md is not ready to process
// messages yet.  Keep them in a queue until MDM_INIT_DONE.
CmsRet _deferMsg(const CmsMsgHeader *orig)
{
   UINT32 count=0;
   UINT32 totalLen;
   CmsMsgHeader *msg;

   cmsLog_debug("deferring msg 0x%x dataLength=%d",
                orig->type, orig->dataLength);

   // Caller's message could be declared on stack, so we have to allocate
   // a buffer to store the message.  Buffer will be freed after it is sent.
   totalLen = sizeof(CmsMsgHeader) + orig->dataLength;
   msg = (CmsMsgHeader *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (msg == NULL)
   {
      cmsLog_error("failed to allocate %d bytes", totalLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   memcpy(msg, orig, totalLen);
   msg->next = 0L;  // make sure this is marked as end of list.

   if (deferredMsgs == NULL)
   {
      deferredMsgs = msg;
   }
   else
   {
      // Append new message at the end of the singly linked list.
      CmsMsgHeader *tmp = deferredMsgs;
      while (tmp->next)
      {
         tmp = (CmsMsgHeader *)(unsigned long) tmp->next;
         if (count++ > 1000)
         {
            // Something is wrong!  Save ourselves from infinite loop.
            // We will lose a message, but better than infinite loop.
            cmsLog_error("deferred msg list is over 1000, break out of loop!");
            break;
         }
      }
      // tmp->next is NULL, so now add msg to the end.
      tmp->next = (uint64_t)msg;
   }

   return CMSRET_SUCCESS;
}

void _sendDeferredMsgs(void *appMsgHandle)
{
   CmsMsgHeader *msg = deferredMsgs;
   CmsMsgHeader *tmp;
   CmsRet ret;

   while (msg)
   {
      cmsLog_debug("sending deferred msg 0x%x to %d dataLength=%d",
                   msg->type, msg->dst, msg->dataLength);
      ret = cmsMsg_send(appMsgHandle, msg);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to send msg 0x%x, ret=%d", msg->type, ret);
      }

      // Save the next pointer, free the current buffer, and restore to next.
      tmp = (CmsMsgHeader *) msg->next;
      cmsMem_free(msg);
      msg = tmp;
   }
   
   deferredMsgs = NULL;
   return;
}

/** Send MDM_INITIALIZED msg to smd.  This will trigger stage 2 of
 *  LAUNCH_ON_BOOT.
 */
void sendMdmInitializedMsg(void *appMsgHandle, CmsEntityId srcEid, CmsEntityId dstEid, UINT32 wordData)
{
   char initMsgBuf[sizeof(CmsMsgHeader)+sizeof(UINT32)]={0};
   CmsMsgHeader *initMsg = (CmsMsgHeader *) initMsgBuf;
   UINT32 *dmPtr = (UINT32 *) (initMsg+1);
   CmsRet ret;

   cmsLog_notice("Sending MDM_INITIALIZED from %d to %d", srcEid, dstEid);

   initMsg->type = CMS_MSG_MDM_INITIALIZED;
   initMsg->src = srcEid;
   initMsg->dst = dstEid;
   initMsg->flags_event = 1;
   initMsg->wordData = wordData;
   initMsg->dataLength = sizeof(UINT32);
   *dmPtr = (UINT32) cmsMdm_isDataModelDevice2();

   if ((ret = cmsMsg_send(appMsgHandle, initMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("send of MDM_INITIALIZED event msg to %d failed. ret=%d",
                   dstEid, ret);
   }

   return;
}

void processMdmInitDone(void *appMsgHandle, CmsEntityId srcEid, CmsEntityId dstEid, UINT32 wordData)
{
   isMdmInitDone = TRUE;

   // TODO: change this check to which ssk variants should call updateLinkStatus.
   // Also use function pointer so ssk variants which do not have updateLinkStatus do not need
   // to define a stub.
   if ((srcEid != EID_WLSSK) && (srcEid != EID_DIAG_SSK))
   {
      // Do an initial scan of all link statuses because we might have missed
      // a netlink status msg during bootup (especially with ethernet).
      // This also has a side effect of starting voice if the BoundIfName
      // interface is up.
      updateLinkStatus(NULL);
   }

   sendMdmInitializedMsg(appMsgHandle, srcEid, dstEid, wordData);

   _sendDeferredMsgs(appMsgHandle);
   return;
}


void processKeyValueEvent(void *appMsgHandle __attribute__((unused)),
                          const CmsMsgHeader *msg)
{
   PubSubKeyValueMsgBody *keyBody = (PubSubKeyValueMsgBody *) (msg+1);
   char *value = (char *)(keyBody+1);
#ifdef DMP_DEVICE2_BASELINE_1
   UINT32 prefixLen = strlen(BDK_PPPXTM_LOCK_PREFIX);
#endif

   cmsLog_notice("Entered: key=%s value=%s (len=%d)",
                 keyBody->key, value, keyBody->valueLen);


   if (!strcmp(keyBody->key, BDK_KEY_DEVICE_QOS_PARAMS))
   {
      // Ignore empty value, which is just a null terminating char.
      // We will get an empty value on system startup, when we subscribe to
      // this key but sysmgmt has not published a value to it yet.
      if (keyBody->valueLen > 1)
      {
         processQosParamsUpdate(value);
      }
   }
   else if (!strcmp(keyBody->key, BDK_KEY_X_BROADCOM_COM_TM))
   {
      // See comments above.
      if (keyBody->valueLen > 1)
      {
         processTrafficManagementUpdate(value);
      }
   }
#ifdef DMP_DEVICE2_BASELINE_1
   else if (!strncmp(keyBody->key, BDK_PPPXTM_LOCK_PREFIX, prefixLen))
   {
      char *xtmLinkFullpath=NULL;
      xtmLinkFullpath = &(keyBody->key[prefixLen]);

      // The value is either LOCKED or "", just check for L in first char.
      if (keyBody->valueLen > 1 && value[0] == 'L')
      {
         pppXtm_updateLockState(appMsgHandle, xtmLinkFullpath, TRUE);
      }
      else
      {
         pppXtm_updateLockState(appMsgHandle, xtmLinkFullpath, FALSE);
      }
   }
#endif  /* DMP_DEVICE2_BASELINE_1 */
   else
   {
      cmsLog_error("Unrecognized key %s (value=%s)", keyBody->key, value);
   }

   return;
}


// Convert fullpath to intfName by querying sys_directory instead of reading
// from MDM.  Useful if the comp cannot access Device.IP.Interace data model,
// e.g. diag component.
// On success, the interface name will be in respStr.  Caller is responsible
// for freeing it.
CmsRet sskUtil_queryIntfName(void *appMsgHandle, CmsEntityId dstEid, const char *queryStr, char **respStr)
{
   CmsMsgHeader *msgHdr;
   CmsMsgHeader *respMsg = NULL;
   UINT32 len;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if ((queryStr == NULL) || (respStr == NULL))
   {
      cmsLog_error("one or more NULL input args %p/%p", queryStr, respStr);
      return CMSRET_INVALID_ARGUMENTS;
   }
   *respStr = NULL;

   len = sizeof(CmsMsgHeader)+ strlen(queryStr) + 1;
   msgHdr = cmsMem_alloc(len, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("Alloc of %d bytes failed", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   msgHdr->src = _myEid;
   msgHdr->dst = dstEid;
   msgHdr->flags_request = 1;
   msgHdr->type = CMS_MSG_QUERY_EVENT_STATUS;
   msgHdr->wordData = PUBSUB_EVENT_INTERFACE;
   msgHdr->dataLength = strlen(queryStr) + 1;
   strcpy((char *)(msgHdr+1), queryStr);

   ret = cmsMsg_sendAndGetReplyBufWithTimeout(appMsgHandle, msgHdr, &respMsg, 3000);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get response from sys_directory, ret=%d", ret);
   }
   else
   {
      if ((respMsg->wordData != PUBSUB_EVENT_INTERFACE) ||
          (respMsg->dataLength <= 1))
      {
         // We received a response, but something is wrong.
         cmsLog_debug("Got bad response: wordData=%d dataLength=%d",
                      respMsg->wordData, respMsg->dataLength);
         if (respMsg->wordData != PUBSUB_EVENT_INTERFACE)
         {
            // error code is in here.
            ret = respMsg->wordData;
         }
         else
         {
            // wordData is PUBSUB_EVENT_INTERFACE, but data length is 0.
            // Probably means the fullpath was not found in sys_directory.
            ret = CMSRET_INVALID_PARAM_NAME;
         }
      }
      else
      {
         *respStr = cmsMem_strdup((char *)(respMsg+1));
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msgHdr);
   cmsLog_notice("Exit: ret=%d queryStr=%s respStr=%s", ret, queryStr, *respStr);
   return ret;
}

CmsRet publishDiagCompleteMsg(void *appMsgHandle, CmsMsgType msgType)
{
   CmsMsgHeader *msgHdr = NULL;
   PubSubKeyValueMsgBody *body = NULL;
   UINT32 msgLen = 0;
   UINT32 valueLen = 0;
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   char timeStr[BUFLEN_128] = {0};

   cmsLog_notice("Entered: msgType=0x%x", msgType);

   cmsTms_getXSIDateTime(0,timeStr,sizeof(timeStr));
   valueLen = strlen(timeStr) + 1;
   msgLen = sizeof(CmsMsgHeader)+ sizeof(PubSubKeyValueMsgBody) + valueLen;
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(msgLen, ALLOC_ZEROIZE);

   if (msgHdr == NULL)
   {
      cmsLog_error("Alloc of %d bytes failed", msgLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // Fill in body
   body = (PubSubKeyValueMsgBody *) (msgHdr+1);
   switch (msgType)
   {
   case CMS_MSG_DSL_LOOP_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_DSL_LOOP_DIAG_COMPLETE);
      break;

   case CMS_MSG_DSL_SELT_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_DSL_SELT_DIAG_COMPLETE);
      break;

   case CMS_MSG_PING_STATE_CHANGED:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_PING_DIAG_COMPLETE);
      break;

   case CMS_MSG_TRACERT_STATE_CHANGED:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_TRACERT_DIAG_COMPLETE);
      break;

   case CMS_MSG_DOWNLOAD_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_DOWNLOAD_DIAG_COMPLETE);
      break;

   case CMS_MSG_UPLOAD_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_UPLOAD_DIAG_COMPLETE);
      break;

   case CMS_MSG_UDPECHO_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_UDPECHO_DIAG_COMPLETE);
      break;

   case CMS_MSG_SPDSVC_DIAG_EVENT:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_SPDSVC_DIAG_EVENT);
      break;

   case CMS_MSG_SPDSVC_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_SPDSVC_DIAG_COMPLETE);
      break;

   case CMS_MSG_OBUDPST_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_OBUDPST_DIAG_COMPLETE);
      break;

   case CMS_MSG_ETHCABLE_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_ETHCABLE_DIAG_COMPLETE);
      break;

   case CMS_MSG_SERVERSELECTION_DIAG_COMPLETE:
      snprintf(body->key, sizeof(body->key), "%s", PUBSUB_KEY_SERVERSELECTION_DIAG_COMPLETE);
      break;

   default:
      cmsLog_error("publishDiagCompleteMsg, unknown msgType 0x%x", msgType);
      CMSMEM_FREE_BUF_AND_NULL_PTR(msgHdr);
      return CMSRET_INTERNAL_ERROR;
   }

   body->valueLen = valueLen;
   memcpy((char *)(body+1), timeStr, valueLen);

   // Fill in header
   msgHdr->src = _myEid;
   if (_myEid == EID_DIAG_SSK)
   {
      // special case for diag component, this code runs in diag_ssk, and
      // diag_ssk is the bus manager.  So need to send to diag_md to publish
      // to sys_directory.
      msgHdr->dst = EID_DIAG_MD;
   }
   else
   {
      // In other components, this msg will go to the bus manager, which will
      // publish the msg to sys_directory.
      msgHdr->dst = EID_SYSMGMT_MD;
   }
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_PUBLISH_EVENT;
   msgHdr->wordData = PUBSUB_EVENT_KEY_VALUE;
   msgHdr->dataLength = sizeof(PubSubKeyValueMsgBody) + valueLen;

   ret = cmsMsg_send(appMsgHandle, msgHdr);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send msg, ret=%d", ret);
   }

   cmsLog_debug("[%s/%s] sent, ret=%d", body->key,timeStr,ret);
   CMSMEM_FREE_BUF_AND_NULL_PTR(msgHdr);
   return ret;
}

// dsl_ssk and wlssk holds local, read-only copies of Device.Qos and
// X_BROADCOM_COM_TM objects.  When relevant fields in these objects are 
// updated in sysmgmt, the new values are sent to sys_directory.  dsl_ssk
// and wlssk subscribe to those keys, and updates end up here where they are
// pushed into the local objects.

void processQosParamsUpdate(char *value)
{
   int enable=0;
   int stateChanged=0;
   Dev2QosObject *qosObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("Entered: value=%s", value);

   ret = cmsObj_get(MDMOID_DEV2_QOS, &iidStack, OGF_NO_VALUE_UPDATE,
                    (void **)&qosObj);
   if (ret == CMSRET_SUCCESS)
   {
      if (!IS_EMPTY_STRING(value))
      {
         // we assume format is n,n, so zap the comma and extract the 2 values
         value[1] = '\0';
         enable = atoi(value);
         stateChanged = atoi(&(value[2]));
         cmsLog_debug("got value of %d,%d", enable, stateChanged);
      }

      qosObj->X_BROADCOM_COM_Enable = (UBOOL8) enable;
      qosObj->X_BROADCOM_COM_EnableStateChanged = (UBOOL8) stateChanged;
      if ((ret = cmsObj_set(qosObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set of Dev2QosObject failed, ret=%d", ret);
      }
      cmsObj_free((void **)&qosObj);
   }
   else
   {
      cmsLog_error("Could not get QoS object, ret=%d", ret);
   }
   return;
}

void processTrafficManagementUpdate(char *value)
{
   BCMTrafficManagementObject *tmObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = cmsObj_get(MDMOID_BCM_TRAFFIC_MANAGEMENT, &iidStack, OGF_NO_VALUE_UPDATE,
                    (void **)&tmObj);
   if (ret == CMSRET_SUCCESS)
   {
      UINT32 owner=0;
      UINT32 qidPrioMap=0;
      char *mid=NULL;
      // Format of value string is owner,qidPrioMap.
      // separate the string at the ,
      mid = strstr(value, ",");
      if (mid != NULL)
      {
         *mid = '\0';
         mid++;
         owner = (UINT32) atoi(value);
         qidPrioMap = (UINT32) atoi(mid);
         if ((tmObj->owner != owner) || (tmObj->qidPrioMap != qidPrioMap))
         {
            tmObj->owner = owner;
            tmObj->qidPrioMap = qidPrioMap;
            if ((ret = cmsObj_set(tmObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("set of BCMTrafficManagementObject failed, ret=%d", ret);
            }
         }
      }
      else
      {
         cmsLog_error("Could not find , in value string %s", value);
      }
      cmsObj_free((void **)&tmObj);
   }
   else
   {
         cmsLog_error("Could not get X_BROADCOM_COM_TM object, ret=%d", ret);
   }
   return;
}


void matchRdpaWanType(const char *wanTypeUp __attribute__((unused)))
{
}


/** Compare new ifName link status state with previous state.  
 * Create a link state record for the specified ifName if it does not
 * exist.  The new link state record is initially in the "link down" state.
 *
 *@return TRUE if link status has changed
 */
UBOOL8 comparePreviousLinkStatus(const char *ifName, UBOOL8 isWan, const char *status)
{
   return (comparePreviousLinkInfo(ifName, isWan, status, 0));
}

UBOOL8 comparePreviousLinkInfo(const char *ifName, UBOOL8 isWan,
                               const char *status, SINT32 parentBrIndex)
{
   SskLinkStatusRecord *linkStatusRec;
   UBOOL8 changed=FALSE;

   if ((linkStatusRec = getLinkStatusRecord(ifName)) == NULL)
   {
      if (cmsUtl_strlen(ifName) >= (SINT32) sizeof(linkStatusRec->ifName))
      {
         cmsLog_error("ifName %s too long, max=%d", ifName, CMS_IFNAME_LENGTH);
         return FALSE;
      }

      /* need to create a new record for this lan ifName */
      linkStatusRec = (SskLinkStatusRecord *) cmsMem_alloc(sizeof(SskLinkStatusRecord), ALLOC_ZEROIZE);
      if (linkStatusRec == NULL)
      {
         cmsLog_error("Could not allocate LinkStatusRecord");
         return FALSE;
      }
      else
      {
         cmsLog_debug("added new LinkStatusRecord for %s", ifName);
         snprintf(linkStatusRec->ifName, sizeof(linkStatusRec->ifName), "%s",
                  ifName);
         linkStatusRec->isWan = isWan;
         linkStatusRec->parentBrIndex = parentBrIndex;
      }
      dlist_append((DlistNode *) linkStatusRec, &sskLinkStatusRecordHead);
   }


   /* at this point, we have a linkStatusRec */
   if (linkStatusRec->isWan != isWan)
   {
      /*
       * Moving an interface and its associated linkStatusRec to/from LAN
       * and WAN is allowed as long as the current linkstate is not up;
       * meaning you have properly shut down (disabled) the interface
       * before moving it.
       */
      cmsLog_debug("moving linkStatusRecord for %s from %s to %s", ifName,
                   (linkStatusRec->isWan ? "WAN" : "LAN"),
                   (isWan ? "WAN" : "LAN"));

      if (linkStatusRec->isLinkUp)
      {
         cmsLog_error("You must disable %s and allow ssk to get updated before moving", ifName);
         /* oh, well, fix it up anyways */
         linkStatusRec->isLinkUp = FALSE;
      }

      linkStatusRec->isWan = isWan;
   }

   if (linkStatusRec->isLinkUp && cmsUtl_strcmp(status, MDMVS_UP))
   {
      cmsLog_debug("%s went from link up to link down", ifName);
      linkStatusRec->isLinkUp = FALSE;
      changed = TRUE;
   }
   else if (!linkStatusRec->isLinkUp && !cmsUtl_strcmp(status, MDMVS_UP))
   {
      cmsLog_debug("%s went from link down to link up", ifName);
      linkStatusRec->isLinkUp = TRUE;
      changed = TRUE;
   }
   else if (linkStatusRec->parentBrIndex != parentBrIndex)
   {
      cmsLog_debug("%s parentBrIndex went from %d => %d", ifName,
                   linkStatusRec->parentBrIndex, parentBrIndex);
      linkStatusRec->parentBrIndex = parentBrIndex;
      changed = TRUE;
   }
   else
   {
      cmsLog_debug("No status change on %s (still at %s)", ifName, status);
   }

   return changed;
}


SskLinkStatusRecord *getLinkStatusRecord(const char *ifName)
{
   SskLinkStatusRecord *linkStatusRec=NULL;
   UBOOL8 found=FALSE;


   dlist_for_each_entry(linkStatusRec, &sskLinkStatusRecordHead, dlist)
   {
#ifdef SUPPORT_LANVLAN
      /* For VLAN LAN, only compare the part without vlan extension; ie. only the "eth1" part since ifName 
      * is "eth1.0" here
      */
#ifdef BRCM_WLAN
    /* For WIFI interface, need to compare the entire string  */
      if(cmsUtl_strstr(ifName, "wl"))
      {
        if (cmsUtl_strcmp(ifName, linkStatusRec->ifName) == 0)
        {
            found=TRUE;
            break;
        }
      }
      else
#endif
      if (cmsUtl_strncmp(ifName, linkStatusRec->ifName, cmsUtl_strlen(linkStatusRec->ifName)) == 0)
#else   
      if (cmsUtl_strcmp(ifName, linkStatusRec->ifName) == 0)
#endif
      
      {
         found=TRUE;
         break;
      }
   }

   return (found) ? linkStatusRec : NULL;
}


void cleanupLinkStatusRecords(void)
{
   SskLinkStatusRecord *rec;

   cmsLog_debug("free all sskLinkStatusRecords");

   while (!dlist_empty(&sskLinkStatusRecordHead))
   {
      rec = (SskLinkStatusRecord *) sskLinkStatusRecordHead.next;
      dlist_unlink((DlistNode *) rec);
      cmsMem_free(rec);
   }

   return;
}


#if defined(DMP_DEVICE2_ETHERNETINTERFACE_1) || defined(DMP_DEVICE2_WIFIRADIO_1) || \
    defined(DMP_DEVICE2_USBINTERFACE_1) || defined(DMP_DEVICE2_ETHLAG_1)
void updateLanHostEntryActiveStatus_dev2(const char *ifName, UBOOL8 linkUp)
{
   char *fullPath=NULL;
   Dev2HostObject *hostObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: ifName=%s, linkUp=%d", ifName, linkUp);

   ret = qdmIntf_intfnameToFullPathLocked(ifName, TRUE, &fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get fullpath for %s, ret=%d", ifName, ret);
      return;
   }
   else
   {
      cmsLog_debug("ifName %s ==> %s", ifName, fullPath);
   }

   /*
    * Go through all LAN_HOST_ENTRYs and update the status for the specified
    * ifName.
    */
   while (cmsObj_getNextFlags(MDMOID_DEV2_HOST, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&hostObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(hostObj->layer1Interface, fullPath) == 0)
      {
         cmsLog_debug("found Dev2HostObject with layer1Interface=%s, ifName=%s",
                      hostObj->layer1Interface, ifName);

         hostObj->active = linkUp;

         if ((ret = cmsObj_set(hostObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of Dev2HostObject failed, ret=%d (active=%d)",
                         ret, hostObj->active);
         }
      }

      cmsObj_free((void **)&hostObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
 
   return;
}
#endif  
/* DMP_DEVICE2_ETHERNETINTERFACE_1 || DMP_DEVICE2_WIFIRADIO_1 ||
 * DMP_DEVICE2_USBINTERFACE_1 ||
 * DEVICE2_ETHLAG
 */

#ifdef DMP_BASELINE_1

void initWanLinkInfo_igd(void)
{
   WanCommonIntfCfgObject *wanCommonIntf;
   WanDslIntfCfgObject *dslIntfCfg=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   WanLinkInfo *wanLinkInfo;
   UBOOL8 addToList;
   CmsRet ret;


   cmsLog_notice("initializing list of WANDevices (links)");

   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, flags, (void **)&wanCommonIntf)) == CMSRET_SUCCESS)
   {
      addToList = FALSE;

      /* allocate the struct to be added to the list */
      if ((wanLinkInfo = cmsMem_alloc(sizeof(WanLinkInfo), ALLOC_ZEROIZE)) == NULL)
      {
         cmsLog_error("wanLinkInfo allocation failed");
         cmsObj_free((void **) &wanCommonIntf);
         return;
      }


      if (!cmsUtl_strcmp(wanCommonIntf->WANAccessType, MDMVS_DSL))
      {
         ret = cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, &iidStack, flags, (void **) &dslIntfCfg);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get DSL intf cfg, ret=%d", ret);
            cmsObj_free((void **) &wanCommonIntf);
            CMSMEM_FREE_BUF_AND_NULL_PTR(wanLinkInfo);
            continue;
         }

         if (!cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
         {
            /*
             * only add the primary line to our list.  In the monitor loop, I will
             * actually check both lines when I check the primary line.
             */
            if (dslIntfCfg->X_BROADCOM_COM_BondingLineNumber == 0)
            {
               wanLinkInfo->isATM = TRUE;
               addToList = TRUE;
            }
         }
         else if (!cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
         {
            /*
             * only add the primary line to our list.  In the monitor loop, I will
             * actually check both lines when I check the primary line.
             */
            if (dslIntfCfg->X_BROADCOM_COM_BondingLineNumber == 0)
            {
               wanLinkInfo->isPTM = TRUE;
               addToList = TRUE;
            }
         }
         else
         {
            cmsLog_error("unrecognized linkEncapsulation type %s", dslIntfCfg->linkEncapsulationUsed);
            cmsObj_free((void **) &wanCommonIntf);
            cmsObj_free((void **) &dslIntfCfg);
            CMSMEM_FREE_BUF_AND_NULL_PTR(wanLinkInfo);
            continue;
         }
         
         cmsObj_free((void **) &dslIntfCfg);
      }
      else if (!cmsUtl_strcmp(wanCommonIntf->WANAccessType, MDMVS_ETHERNET))
      {
         wanLinkInfo->isEth = TRUE;
         addToList = TRUE;
      }
      else if (!cmsUtl_strcmp(wanCommonIntf->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
      {
         WanPonIntfObject *ponObj = NULL;

         if ((ret = cmsObj_get(MDMOID_WAN_PON_INTF, &iidStack, flags, (void **) &ponObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get Pon intf cfg, ret=%d", ret);
            cmsObj_free((void **) &wanCommonIntf);
            CMSMEM_FREE_BUF_AND_NULL_PTR(wanLinkInfo);
            continue;
         }

         if (!cmsUtl_strcmp(ponObj->ponType, MDMVS_GPON))
         {
            wanLinkInfo->isGpon = TRUE;
            addToList = TRUE;
         }
         else if (!cmsUtl_strcmp(ponObj->ponType, MDMVS_EPON))
         {
            wanLinkInfo->isEpon = TRUE;
            addToList = TRUE;
         }
                  
         cmsObj_free((void **) &ponObj);

      }
      else if (!cmsUtl_strcmp(wanCommonIntf->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI))
      {
         wanLinkInfo->isWifi= TRUE;
         addToList = TRUE;
      }
      
      cmsObj_free((void **) &wanCommonIntf);
      
      if (addToList)
      {
         cmsLog_debug("adding wanLinkInfo at %s, atm=%d ptm=%d eth=%d gpon=%d epon=%d",
                      cmsMdm_dumpIidStack(&iidStack),
                      wanLinkInfo->isATM, wanLinkInfo->isPTM, wanLinkInfo->isEth, wanLinkInfo->isGpon, wanLinkInfo->isEpon);

         wanLinkInfo->iidStack = iidStack;

         /* add this to the end of the list */
         dlist_prepend((DlistNode *) wanLinkInfo, &wanLinkInfoHead);
      }
      else
      {
         cmsLog_debug("ignoring WanDevice %s", cmsMdm_dumpIidStack(&iidStack));
         CMSMEM_FREE_BUF_AND_NULL_PTR(wanLinkInfo);
      }
   }

   return;
}


void freeWanLinkInfoList_igd(void)
{
   WanLinkInfo *tmp = NULL;

   while (dlist_empty(&wanLinkInfoHead) == 0)
   {
      tmp = (WanLinkInfo *) wanLinkInfoHead.next;
      
      cmsLog_debug("Free wanLinkInfo iidstack: %s", 
         cmsMdm_dumpIidStack(&(tmp->iidStack)));

      dlist_unlink((DlistNode *) tmp);
      cmsMem_free(tmp);
   }

    cmsLog_debug("Done free wanLinkInfo list.");
}

#endif  /* DMP_BASELINE_1 */

