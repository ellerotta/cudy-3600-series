/*
* <:copyright-BRCM:2013:proprietary:standard
*
*    Copyright (c) 2013 Broadcom
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
* :>
*/

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_dal.h"
#ifdef BRCM_VOICE_SUPPORT
#include "dal_voice.h"
#endif
#include "cms_msg.h"
#include "ssk_util.h"


/*!\file conn_util.c
 * \brief Some connection related function needed by intf stack (even in
 *        hybrid mode).
 */

void updateWanConnStatusInSubtreeLocked(void *appMsgHandle, const InstanceIdStack *parentIidStack, UBOOL8 isLinkUp)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void *wanConnObj=NULL;
   CmsRet ret;

   cmsLog_debug("isLinkUp=%d iidStack=%s", isLinkUp, cmsMdm_dumpIidStack(parentIidStack));


#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
   isAutoDetectionEnabled = dalAutoDetect_isAutoDetectEnabled();
   if (isAutoDetectionEnabled )
   {
      UBOOL8  startWanConnection = TRUE;
      
      updateWanConnStatusInSubtreeLocked_n(appMsgHandle, parentIidStack,
                                           isLinkUp, startWanConnection);
      return;
   }
#endif

   while ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, parentIidStack, &iidStack, &wanConnObj)) == CMSRET_SUCCESS)
   {
      updateSingleWanConnStatusLocked(appMsgHandle, &iidStack, wanConnObj, isLinkUp);
      cmsObj_free(&wanConnObj);
   }


   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, parentIidStack, &iidStack, &wanConnObj)) == CMSRET_SUCCESS)
   {
      updateSingleWanConnStatusLocked(appMsgHandle, &iidStack, wanConnObj, isLinkUp);
      cmsObj_free(&wanConnObj);
   }

   return;
}


void updateSingleWanConnStatusLocked(void *appMsgHandle, const InstanceIdStack *iidStack, void *wanConnObj, UBOOL8 wanLinkUp)
{
   char ifName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 change = FALSE;
   CmsMsgType connMsg=0;
   CmsRet ret = CMSRET_SUCCESS;   
   MdmObjectId oid = GET_MDM_OBJECT_ID(wanConnObj);

#ifdef BRCM_VOICE_SUPPORT
   char ipAddrBuf[CMS_IPADDR_LENGTH]={0};
#if VOICE_IPV6_SUPPORT
   int isIpv6 = 0;
   char ipAddrFamily[BUFLEN_16];
   unsigned int length = 16;

   ret = dalVoice_GetIpFamily( NULL, ipAddrFamily, length );
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the IP address family for voice, ret=%d", ret);
      return;
   }
   else
   {
      isIpv6 = !(cmsUtl_strcmp( ipAddrFamily, MDMVS_IPV6 ));
   }
#endif   
#endif

   cmsLog_debug("oid=%d, iidstack: %s, wanLinkUp=%d", 
                 oid, cmsMdm_dumpIidStack(iidStack), wanLinkUp);

   if (oid == MDMOID_WAN_IP_CONN)
   {
      WanIpConnObject *wanIpConnObj = (WanIpConnObject *) wanConnObj;

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      cmsLog_debug("wanLinkUp=%d, connectionType=%s connectionStatus=%s IPv6ConnStatus=%s",
                   wanLinkUp, wanIpConnObj->connectionType, wanIpConnObj->connectionStatus,
                   wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus);
#else
      cmsLog_debug("wanLinkUp=%d, connectionType=%s connectionStatus=%s",
                   wanLinkUp, wanIpConnObj->connectionType, wanIpConnObj->connectionStatus);
#endif

      if (wanLinkUp == FALSE) 
      {
         /* Set the transient layer 2 link status for the CmsObj_set operation which may take some time and
         * the real layer 2 link status could be changed during this time.
         */      
         CMSMEM_REPLACE_STRING(wanIpConnObj->X_BROADCOM_COM_TransientLayer2LinkStatus, MDMVS_DOWN);
      
         if (!cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_CONNECTED) ||
             !cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_CONNECTING))
         {
            cmsLog_debug("layer 2 link is down, so set ifName=%s from %s to Disconnected.", 
                         wanIpConnObj->X_BROADCOM_COM_IfName, wanIpConnObj->connectionStatus);
               
            CMSMEM_REPLACE_STRING(wanIpConnObj->connectionStatus, MDMVS_DISCONNECTED);
            snprintf(ifName, sizeof(ifName), "%s", wanIpConnObj->X_BROADCOM_COM_IfName);
            connMsg = CMS_MSG_WAN_CONNECTION_DOWN;
            change = TRUE;
         }
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         if ( (wanIpConnObj->X_BROADCOM_COM_IPv6Enabled) &&
              ( !cmsUtl_strcmp(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) ||
                !cmsUtl_strcmp(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING)) )
         {
            cmsLog_debug("layer 2 link is down, so set ifName=%s from %s to Disconnected.", 
                         wanIpConnObj->X_BROADCOM_COM_IfName,
                         wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus);
               
            CMSMEM_REPLACE_STRING(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED);
            snprintf(ifName, sizeof(ifName), "%s", wanIpConnObj->X_BROADCOM_COM_IfName);
            connMsg = CMS_MSG_WAN_CONNECTION_DOWN;
            change = TRUE;
         }
#endif
      }
      else if (wanLinkUp == TRUE) 
      {
         /* Set the transient layer 2 link status for the CmsObj_set operation which may take some time and
         * the real layer 2 link status could be changed during this time.
         */      
         CMSMEM_REPLACE_STRING(wanIpConnObj->X_BROADCOM_COM_TransientLayer2LinkStatus, MDMVS_UP);
      
         if ((cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_UNCONFIGURED) == 0) ||
             (cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_DISCONNECTED) == 0))
         {
            cmsLog_debug("wanIpConnObj->connectionStatus=%s", wanIpConnObj->connectionStatus);

            /* If it is disabled, do nothing on the wanIpConnObj */
            if (!wanIpConnObj->enable)
            {
               cmsLog_debug("wan conn object is disabled.");
            }
            else
            {
               /*
                * For static or bridged connections, when layer 2 link goes up,
                * move the WAN connection status directly to "Connected" state.
                */
               if (!cmsUtl_strcmp(wanIpConnObj->addressingType, MDMVS_STATIC) ||
                   cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_ROUTED))     
               {
                  cmsLog_debug("Layer 2 link up in static connection (bridge/IPoA/static IPoE): Wan connectionStatus=%s; Set ifName=%s to Connected.",
                     wanIpConnObj->connectionStatus, wanIpConnObj->X_BROADCOM_COM_IfName);
                     

                  /* record the Connection Establish time */
                  wanIpConnObj->X_BROADCOM_COM_ConnectionEstablishedTime = cmsTms_getSeconds();
                  CMSMEM_REPLACE_STRING(wanIpConnObj->connectionStatus, MDMVS_CONNECTED);
                  snprintf(ifName, sizeof(ifName), "%s", wanIpConnObj->X_BROADCOM_COM_IfName);
#ifdef BRCM_VOICE_SUPPORT
#if VOICE_IPV6_SUPPORT
                  if ( !isIpv6 )
#endif
                  {
                     snprintf(ipAddrBuf, sizeof(ipAddrBuf), "%s", wanIpConnObj->externalIPAddress);
                  }
#endif
                  connMsg = CMS_MSG_WAN_CONNECTION_UP;
                  change = TRUE;
               }
               else
               {
                  /* dynamic IPoE, need to start dhcpc first: set the connectionStatus="Connecting"
                  * and processDhcpcStateChanged will change that to "Connected" if dhcpc gets the 
                  * external IP, etc.
                  */

                  cmsLog_debug("Layer 2 link up in dynamic IPoE: %s is %s, so set to CONNECTING", 
                     wanIpConnObj->X_BROADCOM_COM_IfName,  wanIpConnObj->connectionStatus);
                     
                  CMSMEM_REPLACE_STRING(wanIpConnObj->connectionStatus, MDMVS_CONNECTING);
                  change = TRUE;
               }
            }
         }
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         if ((wanIpConnObj->X_BROADCOM_COM_IPv6Enabled) &&
             ((cmsUtl_strcmp(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_UNCONFIGURED) == 0) ||
              (cmsUtl_strcmp(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED) == 0)) )
         {
            cmsLog_debug("wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus=%s",
                          wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus);

            /*
             * For static connections, when layer 2 link goes up,
             * move the WAN connection status directly to "Connected" state.
            */
            if (!cmsUtl_strcmp(wanIpConnObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC))
            {
               cmsLog_debug("Layer 2 link up in static mode: Wan IPv6ConnStatus=%s; Set ifName=%s to Connected.",
                            wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus,
                            wanIpConnObj->X_BROADCOM_COM_IfName);
                  
               CMSMEM_REPLACE_STRING(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED);
               snprintf(ifName, sizeof(ifName), "%s", wanIpConnObj->X_BROADCOM_COM_IfName);
               connMsg = CMS_MSG_WAN_CONNECTION_UP;
#if defined(BRCM_VOICE_SUPPORT) && VOICE_IPV6_SUPPORT
               if ( isIpv6 )
               {
                  snprintf(ipAddrBuf, sizeof(ipAddrBuf), "%s", wanIpConnObj->X_BROADCOM_COM_ExternalIPv6Address);
               }
#endif
            }
            else
            {
               /*
                * dynamic IPoE, need to start dhcp6c first: set the IPv6ConnStatus="Connecting"
                * and processDhcpcStateChanged will change that to "Connected" if dhcp6c gets the 
                * external IP, etc.
               */

               cmsLog_debug("Layer 2 link up in dynamic IPoE: %s is %s, so set to CONNECTING", 
                            wanIpConnObj->X_BROADCOM_COM_IfName, 
                            wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus);
                  
               CMSMEM_REPLACE_STRING(wanIpConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING);
            }

            wanIpConnObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled = FALSE;
            change = TRUE;
         }
#endif
      }
   }

   else /* must be a pppConn Object */
   {
      WanPppConnObject *wanPppConnObj = (WanPppConnObject *) wanConnObj;

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      cmsLog_debug("wanLinkUp=%d, connectionStatus=%s IPv6ConnStatus=%s", 
                    wanLinkUp, wanPppConnObj->connectionStatus,
                    wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus);
#else
      cmsLog_debug("wanLinkUp=%d, connectionStatus=%s", wanLinkUp, wanPppConnObj->connectionStatus);
#endif
      
      if (wanLinkUp == FALSE)
      {
         /* Set the transient layer 2 link status for the CmsObj_set operation which may take some time and
         * the real layer 2 link status could be changed during this time.
         */      
         CMSMEM_REPLACE_STRING(wanPppConnObj->X_BROADCOM_COM_TransientLayer2LinkStatus, MDMVS_DOWN);
         if (!cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTED) ||
             !cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTING))
         {
            cmsLog_debug("layer 2 link is down so Set ifName=%s from %s to Disconnected.", 
                         wanPppConnObj->X_BROADCOM_COM_IfName, wanPppConnObj->connectionStatus);
               
            CMSMEM_REPLACE_STRING(wanPppConnObj->connectionStatus, MDMVS_DISCONNECTED);

#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
            {
               UBOOL8 realWanLinkUp = dalWan_isWanLayer2LinkUp(MDMOID_WAN_PPP_CONN, iidStack);
               
               cmsLog_debug("realWanLinkUp=%d, fakelinkUp=%d", realWanLinkUp, wanLinkUp);
               if (!realWanLinkUp)
               {
                  /*  X_BROADCOM_COM_StopPppD was set to TRUE for stopping pppd in the manual
                  * selection (change on ppp setting).  Only reset to FALSE if phyiscal link and
                  * fakelinkUp both are down. 
                  */
                   wanPppConnObj->X_BROADCOM_COM_StopPppD = FALSE;
               }
               else
               {
                   wanPppConnObj->X_BROADCOM_COM_StopPppD = TRUE;
               }
            }               
#endif      
            snprintf(ifName, sizeof(ifName), "%s", wanPppConnObj->X_BROADCOM_COM_IfName);
            connMsg = CMS_MSG_WAN_CONNECTION_DOWN;
            change = TRUE;
         }
         else if (!cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_DISCONNECTED) ||
             !cmsUtl_strcmp(wanPppConnObj->lastConnectionError, MDMVS_ERROR_UNKNOWN))
         {
            /* This is needed because if layer 2 link is up but ppp server is down (connectionStatus is "Disconnected")
            *  and then layer 2 link goes down, the pppd in the memory need to be stopped in the rcl_wan.c
            */
            CMSMEM_REPLACE_STRING(wanPppConnObj->lastConnectionError, MDMVS_ERROR_FORCED_DISCONNECT);
            snprintf(ifName, sizeof(ifName), "%s", wanPppConnObj->X_BROADCOM_COM_IfName);
            connMsg = CMS_MSG_WAN_CONNECTION_DOWN;
            change = TRUE;
         }
  
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         if ((wanPppConnObj->X_BROADCOM_COM_IPv6Enabled) &&
             (!cmsUtl_strcmp(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) ||
              !cmsUtl_strcmp(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING)) )
         {
            cmsLog_debug("layer 2 link is down so Set ifName=%s from %s to Disconnected.", 
                         wanPppConnObj->X_BROADCOM_COM_IfName,
                         wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus);
               
            CMSMEM_REPLACE_STRING(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED);
            wanPppConnObj->X_BROADCOM_COM_IPv6PppUp = FALSE;
            snprintf(ifName, sizeof(ifName), "%s", wanPppConnObj->X_BROADCOM_COM_IfName);
            connMsg = CMS_MSG_WAN_CONNECTION_DOWN;
            change = TRUE;
         }
#endif
      }
      else if (wanLinkUp == TRUE)
      {
         /* Set the transient layer 2 link status for the CmsObj_set operation which may take some time and
         * the real layer 2 link status could be changed during this time.
         */
         CMSMEM_REPLACE_STRING(wanPppConnObj->X_BROADCOM_COM_TransientLayer2LinkStatus, MDMVS_UP);
#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
         if (!cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTED) && 
            !wanPppConnObj->enable)
         {
            /* For auto detection profile, if ppp is connected and is disabled in the manual selection,
            *  need to set X_BROADCOM_COM_StopPppD to TRUE for rcl handler to stop pppd
            */
            wanPppConnObj->X_BROADCOM_COM_StopPppD = TRUE;
         }
         else 
#endif
         if (!cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_UNCONFIGURED) ||
             !cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            /* Need to start pppd: set the connectionStatus="Connecting"
            * and processPppStateChanged will change that to "Connected" if pppd gets the 
            * external IP, etc. If it is disabled, do nothing on the wanPppConnObj
            */

            if (!wanPppConnObj->enable)
            {
               cmsLog_debug("wan conn object is disabled.");
            }
            else
            {
               cmsLog_debug("dsl link went from down to up, set %s to Connecting", wanPppConnObj->X_BROADCOM_COM_IfName);
               CMSMEM_REPLACE_STRING(wanPppConnObj->connectionStatus, MDMVS_CONNECTING);
#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
               if (isAutoDetectionEnabled)
               {
                  /* if auto detection is enabled and this ppp is enabled, but it can not connected 
                  * in time (AUTO_DETECT_TASK_INTERVAL), this pppd needs to be stop in rcl
                  */
                  wanPppConnObj->X_BROADCOM_COM_StopPppD = TRUE;
               }
               else
               {
                  wanPppConnObj->X_BROADCOM_COM_StopPppD = FALSE;
               }
#endif            

               change = TRUE;
            }               
         }         
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         if ((wanPppConnObj->X_BROADCOM_COM_IPv6Enabled) &&
             (!cmsUtl_strcmp(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_UNCONFIGURED) ||
              !cmsUtl_strcmp(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED)) )
         {
           /* 
            * Set the IPv6ConnStatus="Connecting"
            * and processPppStateChanged will change PppUp to true if pppd gets the 
            * IPv6CP up information.
            */
          
            cmsLog_debug("dsl link went from down to up, set %s to Connecting",
                         wanPppConnObj->X_BROADCOM_COM_IfName);
            CMSMEM_REPLACE_STRING(wanPppConnObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING);
            wanPppConnObj->X_BROADCOM_COM_IPv6PppUp = FALSE;
            change = TRUE;
         }
#endif
      }
   }
     
   if ( change )
   {
      if ((ret = cmsObj_set(wanConnObj, iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to set wanConnObj. ret=%d", ret);
      }
      else
      {
         cmsLog_debug("Done setting the connectionStatus to wanConnObj.");
      }
      
      /* Only send WAN connection event message if connection is up or down */
      if (connMsg == CMS_MSG_WAN_CONNECTION_DOWN ||
          connMsg == CMS_MSG_WAN_CONNECTION_UP)
      {
         sendStatusMsgToSmd(appMsgHandle, connMsg, ifName);
      }

#ifdef BRCM_VOICE_SUPPORT
      /*
       * The only time we should call this function is when we have a static IPoE
       * connection and the link goes up.  For all the other link up cases, we have
       * not acquired the IP address yet. So we should not call this function yet.
       * For the link down case, I don't see any code that handles switching from
       * one ip address to another, and it does not seem like the code wants 
       * voice to go down if the link goes down.  (The boundIfName implemenation for
       * voice needs to be revisited and cleaned up a bit more... mwang).
       */
      if (wanLinkUp && ipAddrBuf[0] != '\0' && (isVoiceOnAnyWan || voiceWanIfName))
      {
         if (sskVoiceCallbacks.initVoiceOnWanIntf != NULL)
         {
            (*sskVoiceCallbacks.initVoiceOnWanIntf)(ifName, ipAddrBuf);
         }
      }
#endif

   }
   else
   {
      /* do nothing */
      cmsLog_debug("layer 2 link changed but no action taken."); 
   }

#if defined(DMP_DEVICE2_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   /* In Hybrid IPv6 case, we need to inject WAN Layer 2 link status into
    * the TR181 interface stack.
    */
   cmsLog_debug("injecting status for oid %d %s, wanLinkUp=%d",
                oid, cmsMdm_dumpIidStack(iidStack), wanLinkUp);

   intfStack_propagateStatusByIidLocked(appMsgHandle, oid, iidStack,
                                        (wanLinkUp ? MDMVS_UP : MDMVS_DOWN));
#endif

   return;
}


