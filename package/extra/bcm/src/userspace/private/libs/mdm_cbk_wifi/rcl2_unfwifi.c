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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIRADIO_1

#include "mdm.h"
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_wifi.h"
#include "rut2_unfwifi.h"
#include "cms_qdm.h"
#include "rut_qos.h"

#include "wlsysutil.h"

CmsRet rcl_dev2WifiObject( _Dev2WifiObject *newObj __attribute__((unused)),
                           const _Dev2WifiObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiBsdCfgObject( _Dev2WifiBsdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiBsdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWpsCfgObject( _Dev2WifiWpsCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiWpsCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiNasCfgObject( _Dev2WifiNasCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiNasCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiDebugMonitorCfgObject( _Dev2WifiDebugMonitorCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiDebugMonitorCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiCeventdCfgObject( _Dev2WifiCeventdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiCeventdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiHapdCfgObject( _Dev2WifiHapdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiHapdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsdCfgObject( _Dev2WifiSsdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiSsdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWbdCfgObject( _Dev2WifiWbdCfgObject *newObj  __attribute__((unused)),
                const _Dev2WifiWbdCfgObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   rut2_sendWifiChange();
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWbdCfgMbssObject( _Dev2WifiWbdCfgMbssObject *newObj  __attribute__((unused)),
                const _Dev2WifiWbdCfgMbssObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   rut2_sendWifiChange();
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioObject( _Dev2WifiRadioObject *newObj,
                                const _Dev2WifiRadioObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
    }

    if (DISABLE_EXISTING(newObj, currObj))
    {
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
    }

    if (newObj && currObj)
    {
        unsigned int band = 0, bw = 0;
        char capBuffer[BUFLEN_1024] = {0};
        char buffer[BUFLEN_1024] = {0};
        char standards[BUFLEN_64] = {0};
        int updateRate = 0;
        int support11ax = 0;
        int support11ac = 0;
        sscanf(newObj->operatingFrequencyBand, "%u", &band);
        sscanf(newObj->operatingChannelBandwidth, "%u", &bw);

        wlgetCapability(newObj->name, capBuffer, BUFLEN_1024);
        support11ax = strstr(capBuffer, "11ax") ? 1 : 0;
        if (band == 5)
           support11ac = 1;

        if (newObj->autoChannelEnable != TRUE) 
        {
            if(newObj->channel == 0)
            {
                if (strcmp(newObj->operatingFrequencyBand, MDMVS_5GHZ) == 0)
                    newObj->channel = 36;
                else //2.4G and 6G
                    newObj->channel = 1;
            }
        }
        else if (newObj->channel != currObj->channel && newObj->channel)
        {
            newObj->autoChannelEnable=0;
        }

        if ( (cmsUtl_strcmp(newObj->operatingFrequencyBand, currObj->operatingFrequencyBand) != 0) ||
             (cmsUtl_strcmp(newObj->supportedChannelBandwidth, currObj->supportedChannelBandwidth) != 0) ||
             IS_EMPTY_STRING(newObj->supportedOperatingChannelBandwidths))
        {
           updateRate = 1;
           if (cmsUtl_strcmp(newObj->supportedChannelBandwidth, MDMVS_20MHZ) == 0) // bw_cap = 1
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz", mdmLibCtx.allocFlags);
           }
           else if (cmsUtl_strcmp(newObj->supportedChannelBandwidth, MDMVS_40MHZ) == 0) // bw_cap = 3
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,Auto", mdmLibCtx.allocFlags);
           }
           else if (cmsUtl_strcmp(newObj->supportedChannelBandwidth, MDMVS_80MHZ) == 0) // bw_cap = 7
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,80MHz,Auto", mdmLibCtx.allocFlags);
           }
           else if (cmsUtl_strcmp(newObj->supportedChannelBandwidth, MDMVS_160MHZ) == 0) // bw_cap = 15
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,80MHz,160MHz,Auto", mdmLibCtx.allocFlags);
           }
           else if (cmsUtl_strcmp(newObj->supportedChannelBandwidth, MDMVS_320MHZ) == 0) // bw_cap = 31
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,80MHz,160MHz,320MHz,Auto", mdmLibCtx.allocFlags);
           }
           else // bw_cap == -1 (auto)
           {
              if ( cmsUtl_strcmp(newObj->operatingFrequencyBand, MDMVS_2_4GHZ) == 0)
              {
                 REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,Auto", mdmLibCtx.allocFlags);
              }
              else // 5GHz or 6GHz
              {
                 REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedOperatingChannelBandwidths, "20MHz,40MHz,80MHz,160MHz,Auto", mdmLibCtx.allocFlags);
              }
           }

           // operatingFrequencyBand change would change SupportedStandards
           // 2.4GHz, only values b, g, n, ax are allowed.
           // 5GHz, only values a, n, ac, ax are allowed.
           // 6GHz, only values ax is allowed. 
           if (band == 2) // switch to 2.4G
               snprintf(standards, sizeof(standards)-1, "b,g,n%s", support11ax?",ax":"");
           else if (band == 5)
               snprintf(standards, sizeof(standards)-1, "a,n,ac%s", support11ax?",ax":"");
           else if (band == 6)
                 snprintf(standards, sizeof(standards)-1, "%s", support11ax?"ax":"");

           if ( IS_EMPTY_STRING(newObj->operatingStandards) || 
                cmsUtl_strcmp(newObj->operatingStandards, "v") == 0 || // For backward compatible 
                cmsUtl_strcmp(newObj->operatingFrequencyBand, currObj->operatingFrequencyBand) != 0)
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->operatingStandards, standards, mdmLibCtx.allocFlags);
           }

           if ( IS_EMPTY_STRING(newObj->supportedStandards) ||
                cmsUtl_strcmp(newObj->operatingFrequencyBand, currObj->operatingFrequencyBand) != 0)
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedStandards, standards, mdmLibCtx.allocFlags);
           }
        }

        if (cmsUtl_strcmp(newObj->operatingStandards, currObj->operatingStandards) != 0)
        {
           updateRate = 1;
           // Support "11ax"
           if (support11ax)
           { 
              // enable "11ax"
              if ((cmsUtl_strstr(newObj->operatingStandards, "ax") != NULL) &&
                  (cmsUtl_strstr(currObj->operatingStandards, "ax") == NULL))
              {
                 if (newObj->X_BROADCOM_COM_WlHeFeatures == 0)
                    newObj->X_BROADCOM_COM_WlHeFeatures = -1;  //enable to auto
              }

              // disable "11ax"
              if ((cmsUtl_strstr(newObj->operatingStandards, "ax") == NULL) &&
                  (cmsUtl_strstr(currObj->operatingStandards, "ax") != NULL))
              {
                 newObj->X_BROADCOM_COM_WlHeFeatures = 0; // disable
              }
           }
           else // not support "11ax"
           {
              if (cmsUtl_strstr(newObj->operatingStandards, "ax") != NULL)
              {
                 cmsLog_error("%s does not support 11ax", newObj->name);
                 return CMSRET_INVALID_PARAM_VALUE;
              }
           }

           // support "11ac"
           if (support11ac)
           {
              // enable "11ac"
              if ((cmsUtl_strstr(newObj->operatingStandards, "ac") != NULL) &&
                  (cmsUtl_strstr(currObj->operatingStandards, "ac") == NULL))
              {
                 if (newObj->X_BROADCOM_COM_WlVhtFeatures == 0)
                    newObj->X_BROADCOM_COM_WlVhtFeatures = -1; // enable to auto

                 if (newObj->X_BROADCOM_COM_WlVhtMode == 0)
                    newObj->X_BROADCOM_COM_WlVhtMode = -1;
              }

              // try to disable "11ac"
              if ((cmsUtl_strstr(newObj->operatingStandards, "ac") == NULL) &&
                  (cmsUtl_strstr(currObj->operatingStandards, "ac") != NULL))
              {
                 cmsLog_error("%s can not disable 11ac", newObj->name);
                 return CMSRET_INVALID_PARAM_VALUE;
              }
           }
           else // not support "11ac"
           {
              if (cmsUtl_strstr(newObj->operatingStandards, "ac") != NULL)
              {
                 cmsLog_error("%s does not support 11ac", newObj->name);
                 return CMSRET_INVALID_PARAM_VALUE;
              }
           }
        }

        if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlBasicRate, currObj->X_BROADCOM_COM_WlBasicRate) != 0)
           updateRate = 1;


        if (updateRate)
        {
           char rateBuffer[BUFLEN_512] = {0};
           if (wlgetBasicRateset(newObj->name, rateBuffer, sizeof(rateBuffer)) != NULL)
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->basicDataTransmitRates, rateBuffer, mdmLibCtx.allocFlags); 
           }
           memset(rateBuffer, 0, sizeof(rateBuffer)); 
           if (wlgetSupportRateset(newObj->name, rateBuffer, sizeof(rateBuffer)) != NULL)
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->supportedDataTransmitRates, rateBuffer, mdmLibCtx.allocFlags);
           }

           memset(rateBuffer, 0, sizeof(rateBuffer)); 
           if (wlgetCurrentRateset(newObj->name, rateBuffer, sizeof(rateBuffer)) != NULL)
           {
              REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->operationalDataTransmitRates, rateBuffer, mdmLibCtx.allocFlags);
           }
        }

        if (newObj->autoChannelEnable == FALSE && newObj->channel != currObj->channel)
        {
           Dev2WifiRadioStatsObject *statsObj = NULL;
           if (cmsObj_get(MDMOID_DEV2_WIFI_RADIO_STATS, iidStack, OGF_NO_VALUE_UPDATE, (void **)&statsObj) == CMSRET_SUCCESS)
           {
              statsObj->manualChannelChangeCount += 1;
              if ((cmsObj_set(statsObj, iidStack)) != CMSRET_SUCCESS)
              {
                 cmsLog_debug("cmsObj error!");
              }
              cmsObj_free((void **)&statsObj);
           }
        }

        // Disable AutoChannel but keep old ExtensionChannel
        if (newObj->channel && !newObj->autoChannelEnable &&
            cmsUtl_strcmp(newObj->extensionChannel, currObj->extensionChannel) == 0)
        {
            // if band, bandwidth, channel is changed... adjust ExtensionChannel
            if (newObj->channel != currObj->channel ||
                (cmsUtl_strcmp(newObj->operatingChannelBandwidth, currObj->operatingChannelBandwidth) != 0) ||
                (cmsUtl_strcmp(newObj->operatingFrequencyBand, currObj->operatingFrequencyBand) != 0))
            {
                char sb_buf[BUFLEN_64] = {0};
                snprintf(sb_buf, sizeof(sb_buf)-1, "%s", newObj->extensionChannel);

                if (rutWifi_channelToSideband(newObj->channel, bw, band, sb_buf) == CMSRET_SUCCESS)
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->extensionChannel, sb_buf, mdmLibCtx.allocFlags);
                }
                else // give default value
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->extensionChannel, MDMVS_BELOWCONTROLCHANNEL, mdmLibCtx.allocFlags);
                }
            }
        }

        wlgetVer(newObj->name, buffer);
        CMSMEM_REPLACE_STRING_FLAGS(newObj->firmwareVersion, buffer, mdmLibCtx.allocFlags);
    }

    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioStatsObject( _Dev2WifiRadioStatsObject *newObj __attribute__((unused)),
                                     const _Dev2WifiRadioStatsObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiNeighboringwifidiagnosticObject( _Dev2WifiNeighboringwifidiagnosticObject *newObj,
        const _Dev2WifiNeighboringwifidiagnosticObject *currObj,
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_16] = {0};
   int pid;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      if (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED))
      {
         void *msgHandle = cmsMdm_getThreadMsgHandle();
         CmsEntityId eid = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));

         cmsLog_notice("Sending message to SMD to start doing wlDiag");
   
         snprintf(cmdLine, sizeof(cmdLine), "%s", eid == EID_TR69C ? "-n" : "");
   
         cmsLog_debug("wlDiag command string=%s", cmdLine);
      
         pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_WLDIAG, cmdLine, strlen(cmdLine)+1);
         
         if (pid == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start wlDiag test.");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Start wlDiag msg sent, pid=%d", pid);
         }   
      }   
   } /* requested */
   else if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_CANCELED) == 0)
   {
      if (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_CANCELED))
      {
         cmsLog_notice("Sending message to SMD to stop wlDiag");
   	  
         ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_WLDIAG, NULL, 0);
         
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to stop wlDiag test.");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Stop wlDiag msg sent");
         }   
      }   
   }
   
   return (ret);
}


CmsRet rcl_dev2WifiNeighboringwifidiagnosticResultObject( _Dev2WifiNeighboringwifidiagnosticResultObject *newObj __attribute__((unused)),
        const _Dev2WifiNeighboringwifidiagnosticResultObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioAcsdCfgObject( _Dev2WifiRadioAcsdCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiRadioAcsdCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

static CmsRet configQueue(char *ssid_name, UBOOL8 enabled)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2QosObject *qMgmtObj = NULL;
    Dev2QosQueueObject *qObj = NULL;
    char *fullPath=NULL;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("configuring QoS Queue for wifi interface %s (enable=%d)",
                                                     ssid_name, enabled);
    /* get the current queue management config */
    if ((ret = cmsObj_get(MDMOID_DEV2_QOS, &iidStack, 0, (void **)&qMgmtObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_get DEV2_QOS returns error. ret=%d", ret);
        return ret;
    }

    /* do nothing if global QoS enable is not true */
    if (!qMgmtObj->X_BROADCOM_COM_Enable)
    {
        cmsObj_free((void **)&qMgmtObj);
        return CMSRET_SUCCESS;
    }

    /* set the flag as TRUE. RCL of qos queue will check this flag and 
     * skip reconfiguring all classifiers for the incoming qos queue enabled/disabled
     */
    qMgmtObj->X_BROADCOM_COM_EnableStateChanged = TRUE;
    if ((ret = cmsObj_set((void *)qMgmtObj, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("set of DEV2_QOS object failed, ret=%d", ret);
        cmsObj_free((void **)&qMgmtObj);
    return ret;
    }

    /* translate ssid_name to fullpath */
    if ((ret = qdmIntf_intfnameToFullPathLocked(ssid_name, TRUE, &fullPath)) != CMSRET_SUCCESS)
    {
        cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d",
                         ssid_name, ret);
        cmsObj_free((void **)&qMgmtObj);
        return ret;
    }
    else
    {
        cmsLog_debug("ssid_name %s ==> %s", ssid_name, fullPath);
    }

    INIT_INSTANCE_ID_STACK(&iidStack);
    /* walk through each queue  */
    while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_QUEUE, &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &qObj)) == CMSRET_SUCCESS)
    {
        if (!cmsUtl_strcmp(qObj->interface, fullPath) &&
                           qObj->enable != enabled)
        {
            CmsRet r2;

            qObj->enable = enabled;
            cmsLog_debug("config the qos queue %s", qObj->alias);
            r2 = cmsObj_set(qObj, &iidStack);
            if (r2 != CMSRET_SUCCESS)
            {
                cmsLog_error("set of qObj->name %s failed, r2=%d",
                             qObj->X_BROADCOM_COM_QueueName, r2);
            }
        }

        cmsObj_free((void **) &qObj);
    }

    /* reconfig all classifiers after setting all the qos queues */
    rutQos_reconfigAllClassifications_dev2(NULL);

    CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

    /* set it back to FALSE */
    qMgmtObj->X_BROADCOM_COM_EnableStateChanged = FALSE;
    if ((ret = cmsObj_set((void *)qMgmtObj, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("set of DEV2_QOS object failed, ret=%d", ret);
        cmsObj_free((void **)&qMgmtObj);
        return ret;
    }
    cmsObj_free((void **)&qMgmtObj);

    if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
    {
       cmsLog_error("error while traversing queues, ret=%d", ret);
    }
    return ret;
}

CmsRet rcl_dev2WifiSsidObject( _Dev2WifiSsidObject *newObj,
                               const _Dev2WifiSsidObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    CmsEntityId eid = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));
    UBOOL8 isEnableChanged = FALSE;
    UBOOL8 enable_queue = FALSE;

    cmsLog_notice("Entered:");

    if (ADD_NEW(newObj, currObj))
    {
        if (eid == EID_SSK || eid == EID_WLSSK)
        {
            rutUtil_modifyNumWifiSsid(1);
            return CMSRET_SUCCESS;
        }
        else
        {
            cmsLog_error("By design, all Wifi SSID objects are added at init time, "
                         "and the Wifi MDM is only initialized by ssk or wlssk, " 
                         "so they are the only apps which can add a SSID obj.  "
                         "Add FAILED, eid=%d (0x%x)", eid, eid);
            return CMSRET_REQUEST_DENIED;
        }
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiSsid(-1);
        return CMSRET_SUCCESS;
    }

    if (newObj != NULL)
    {
        // force BSSID the same as MACAddress
        if (cmsUtl_strcmp(newObj->BSSID, newObj->MACAddress) != 0)
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->BSSID, newObj->MACAddress, mdmLibCtx.allocFlags);
        }
    }

    if (ENABLE_EXISTING(newObj, currObj))
    {
        isEnableChanged = TRUE;
        enable_queue = TRUE;
    }
    if (DISABLE_EXISTING(newObj, currObj))
    {
        isEnableChanged = TRUE;
        enable_queue = FALSE;
    }

    if (isEnableChanged)
    {
        configQueue(newObj->name, enable_queue);
    }

    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiSsidStatsObject( _Dev2WifiSsidStatsObject *newObj __attribute__((unused)),
                                    const _Dev2WifiSsidStatsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidBsdCfgObject( _Dev2WifiSsidBsdCfgObject *newObj __attribute__((unused)),
                                    const _Dev2WifiSsidBsdCfgObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidHspotCfgObject( _Dev2WifiSsidHspotCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiSsidHspotCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidSsdCfgObject( _Dev2WifiSsidSsdCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiSsidSsdCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
CmsRet rcl_dev2WifiAccessPointObject( _Dev2WifiAccessPointObject *newObj __attribute__((unused)),
                                      const _Dev2WifiAccessPointObject *currObj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    CmsEntityId eid = GENERIC_EID(cmsMsg_getHandleEid(msgHandle));

    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        if (eid == EID_SSK || eid == EID_WLSSK)
        {
            rutUtil_modifyNumWifiAccessPoint(1);
            return CMSRET_SUCCESS;
        }
        else
        {
            cmsLog_error("By design, all Wifi AccessPoint objects are added at init time, "
                         "and the Wifi MDM is only initialized by ssk or wlssk, " 
                         "so they are the only apps which can add an AccessPoint obj.  "
                         "Add FAILED, eid=%d (0x%x)", eid, eid);
            return CMSRET_REQUEST_DENIED;
        }
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiAccessPoint(-1);
        return CMSRET_SUCCESS;
    }

    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}


static CmsRet updateNvramWEPKeyChanged( _Dev2WifiAccessPointSecurityObject *newObj,
                                        const _Dev2WifiAccessPointSecurityObject *currObj)
{
    CmsRet ret = CMSRET_SUCCESS;
    int isChanged = FALSE;
    char *targetKey = NULL;

    if (newObj == NULL || currObj == NULL)
        return CMSRET_SUCCESS;

    // check key index change
    if (newObj->X_BROADCOM_COM_WlKeyIndex != currObj->X_BROADCOM_COM_WlKeyIndex)
    {
        isChanged = TRUE;
    }

    // check referenced key change
    switch(newObj->X_BROADCOM_COM_WlKeyIndex)
    {
        case 1:
            targetKey = newObj->X_BROADCOM_COM_WlKey1;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey1) != 0)
                isChanged = TRUE;
            break;
        case 2:
            targetKey = newObj->X_BROADCOM_COM_WlKey2;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey2) != 0)
                isChanged = TRUE;
            break;
        case 3:
            targetKey = newObj->X_BROADCOM_COM_WlKey3;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey3) != 0)
                isChanged = TRUE;
            break;
        case 4:
            targetKey = newObj->X_BROADCOM_COM_WlKey4;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey4) != 0)
                isChanged = TRUE;
            break;
        default:
            newObj->X_BROADCOM_COM_WlKeyIndex = 1;
            targetKey = newObj->X_BROADCOM_COM_WlKey1;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey1) != 0)
                isChanged = TRUE;
            break;
    }

    /** WEPKey only accept hexstring. 
     * translate wlkey[n] here if it is a ascii string.
     */
    if (isChanged && targetKey != NULL)
    {
        int i, keyLength = strlen(targetKey);
        char *hexStr = NULL;

        if (((keyLength % 2) != 0) && (keyLength == 5 || keyLength == 13)) // 5 or 13 ascii
        {
            ret = cmsUtl_binaryBufToHexString((const UINT8*)targetKey, keyLength, &hexStr);
        }
        else if (((keyLength % 2) == 0) && (keyLength == 10 || keyLength == 26)) // 10 or 26 hex string
        {
            for (i = 0 ; i < keyLength ; i++)
                if(!isxdigit(targetKey[i]))
                    ret = CMSRET_INVALID_PARAM_VALUE;

            if (ret == CMSRET_SUCCESS)
                hexStr = cmsMem_strdup(targetKey);
        }
        else if(keyLength == 0)
        { // for unset key
            ret = CMSRET_SUCCESS;
        }
        else
        {
            ret = CMSRET_INVALID_PARAM_VALUE;
        }

        if (ret == CMSRET_SUCCESS && hexStr != NULL)
        {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->WEPKey, hexStr, mdmLibCtx.allocFlags);
            cmsMem_free(hexStr);
        }
    }

    return ret;
}

CmsRet rcl_dev2WifiAccessPointSecurityObject( _Dev2WifiAccessPointSecurityObject *newObj,
                                              const _Dev2WifiAccessPointSecurityObject *currObj,
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    Dev2WifiAccessPointObject *wifiAccessPointObj;

    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }

    if (newObj && currObj)
    {
        cmsLog_debug("....newObj->reset:%d\n",newObj->reset);
        InstanceIdStack ancestorIidStack = *iidStack;

        if (cmsObj_getAncestor(MDMOID_DEV2_WIFI_ACCESS_POINT,
                    MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY,
                    &ancestorIidStack,(void **)&wifiAccessPointObj) != CMSRET_SUCCESS)
        {
            return CMSRET_INVALID_PARAM_VALUE;
        }

        // implement reset 
        if (newObj->reset) 
        {
            _Dev2WifiAccessPointSecurityObject *defaultObj = NULL;
            ret = cmsObj_get(MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY, iidStack, OGF_DEFAULT_VALUES, (void **)&defaultObj);
            if (ret != CMSRET_SUCCESS) {
                cmsObj_free((void **)&wifiAccessPointObj);
                return ret;
            }
            newObj->reset = FALSE;
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, defaultObj->modeEnabled, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, defaultObj->wlAuthAkm, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode,defaultObj->wlAuthMode, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->WEPKey, defaultObj->WEPKey, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->preSharedKey, defaultObj->preSharedKey, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->keyPassphrase, defaultObj->keyPassphrase, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->SAEPassphrase, defaultObj->SAEPassphrase, mdmLibCtx.allocFlags);

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, defaultObj->X_BROADCOM_COM_WlKey1, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey2, defaultObj->X_BROADCOM_COM_WlKey2, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey3, defaultObj->X_BROADCOM_COM_WlKey3, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey4, defaultObj->X_BROADCOM_COM_WlKey4, mdmLibCtx.allocFlags);
            newObj->X_BROADCOM_COM_WlKeyIndex = defaultObj->X_BROADCOM_COM_WlKeyIndex;

            cmsObj_free((void **)&defaultObj);
        } 
        else 
        {
            UBOOL8 wep64_128_mode_changed = FALSE;
            UBOOL8 need_wep_checking = FALSE;

            /* Check mode changed.
             *  --This change is coming from CWMP or CLI
             */ 
            if (cmsUtl_strcmp(newObj->modeEnabled, currObj->modeEnabled) != 0)
            {
                _Dev2WifiRadioObject *radioObj = NULL;
                InstanceIdStack radio_iidStack = EMPTY_INSTANCE_ID_STACK;

                if((ret=rutWifi_get_AP_Radio_dev2(wifiAccessPointObj,(void **)&radioObj,&radio_iidStack))!=CMSRET_SUCCESS)
                {
                    cmsLog_debug("....failed to get radio:%d...\n",ret);
                    cmsObj_free((void **)&wifiAccessPointObj);
                    return CMSRET_INVALID_PARAM_VALUE;
                }

                // WEP64 or WEP128
                if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_64) || !cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_128))
                {
                    wep64_128_mode_changed = TRUE;

                    // turn off "n-mode"
                    if(cmsUtl_strcmp(radioObj->X_BROADCOM_COM_WlNmode,"off"))
                    {
                        cmsLog_debug("set wlNmod set off");
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "off", mdmLibCtx.allocFlags);
                        cmsObj_setFlags(radioObj,&radio_iidStack, OSF_NO_RCL_CALLBACK);
                    }

                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "open", mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "enabled", mdmLibCtx.allocFlags);
                } 
                else
                {
                    // turn on "n-mode"
                    cmsLog_debug("set wlNmode to auto\n");
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "auto", mdmLibCtx.allocFlags);
                    cmsObj_setFlags(radioObj,&radio_iidStack, OSF_NO_RCL_CALLBACK);

                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);

                    if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_NONE))
                    {
                        /* if modeEnable is NONE */
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "open", mdmLibCtx.allocFlags);
                    }
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "psk", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "psk2", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "psk psk2", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA3_PERSONAL_TRANSITION))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "psk2 sae", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA3_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "sae", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "open", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_ENTERPRISE))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "wpa2", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_ENTERPRISE))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, "wpa wpa2", mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    }
                }

                cmsObj_free((void **)&radioObj);
            }
            /* Check nvram akm change and update modeEnabled.
             *  -- This change is coming from nvram commit 
             */
            else if (cmsUtl_strcmp(newObj->wlAuthAkm, currObj->wlAuthAkm) != 0)
            {
                /* Note: the checking sequence does matter. */
                char strAkm[BUFLEN_128] = {0};
                snprintf(strAkm, BUFLEN_128, "%s ", newObj->wlAuthAkm); 
                //if (cmsUtl_strncmp(newObj->wlAuthAkm, MDMVS_PSK_PSK2, strlen(MDMVS_PSK_PSK2)) == 0)
                if (strstr(strAkm, "psk2 ") && strstr(strAkm, "psk "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA_WPA2_PERSONAL, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "psk2 ") && strstr(strAkm, "sae "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA3_PERSONAL_TRANSITION, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "psk2 "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA2_PERSONAL, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "psk "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA_PERSONAL, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "sae "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA3_PERSONAL, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "wpa2 ") && strstr(strAkm, "wpa "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA_WPA2_ENTERPRISE, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if (strstr(strAkm, "wpa2 "))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WPA2_ENTERPRISE, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);
                }
                else if ((cmsUtl_strcmp(newObj->wlAuthAkm, "open") == 0) ||
                         (cmsUtl_strcmp(newObj->wlAuthAkm, "") == 0  ))
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_NONE, mdmLibCtx.allocFlags);
		    need_wep_checking = TRUE;
                }
            }

            // check WEPKey change
            if (cmsUtl_strcmp(newObj->WEPKey, currObj->WEPKey))
            {
                cmsLog_debug("newObj->WEPKey:%s and currObj->WEPKey:%s",newObj->WEPKey,currObj->WEPKey);

                switch(newObj->X_BROADCOM_COM_WlKeyIndex)
                {
                    case 1:
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, newObj->WEPKey, mdmLibCtx.allocFlags);
                        break;
                    case 2:
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey2, newObj->WEPKey, mdmLibCtx.allocFlags);
                        break;
                    case 3:
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey3, newObj->WEPKey, mdmLibCtx.allocFlags);
                        break;
                    case 4:
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey4, newObj->WEPKey, mdmLibCtx.allocFlags);
                        break;
                    default:
                        newObj->X_BROADCOM_COM_WlKeyIndex = 1;
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, newObj->WEPKey, mdmLibCtx.allocFlags);
                        break;
                } /* switch(wlkeyindex) */
            } /* WEPKey */

            // check nvram wep enable and update modeEnabled
            if (need_wep_checking || cmsUtl_strcmp(newObj->wlWep, currObj->wlWep) != 0)
            {
                if ((cmsUtl_strcmp(newObj->wlWep, "enabled") == 0) && !wep64_128_mode_changed)
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_WEP_64, mdmLibCtx.allocFlags);
                }
                else if (cmsUtl_strcmp(newObj->wlWep, "disabled") == 0)
                {
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, MDMVS_NONE, mdmLibCtx.allocFlags);
                }
            }

            // check nvram wep key or index change and update WEPKey
            ret = updateNvramWEPKeyChanged(newObj, currObj);

        } /* no reset */

        cmsObj_free((void **)&wifiAccessPointObj);
    }

    if (ret == CMSRET_SUCCESS)
        rut2_sendWifiChange();

    return ret;
}

CmsRet rcl_dev2WifiAccessPointAccountingObject( _Dev2WifiAccessPointAccountingObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAccountingObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiAccessPointWpsObject( _Dev2WifiAccessPointWpsObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointWpsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        return CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        return CMSRET_SUCCESS;
    }

    rut2_sendWifiChange();
    return ret;
}

CmsRet rcl_dev2WifiAssociatedDeviceObject( _Dev2WifiAssociatedDeviceObject *newObj __attribute__((unused)),
        const _Dev2WifiAssociatedDeviceObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumWifiAssociatedDevice(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiAssociatedDevice(iidStack, -1);
        rutWifi_sendAPClientAssocChanged_dev2(currObj->X_BROADCOM_COM_BssIfname,
                                   currObj->MACAddress,
                                   PEEK_INSTANCE_ID(iidStack),
                                   INACTIVE_ENTRY);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAssociateddeviceStatsObject( _Dev2WifiAssociateddeviceStatsObject *newObj __attribute__((unused)),
        const _Dev2WifiAssociateddeviceStatsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAccessPointAcObject ( _Dev2WifiAccessPointAcObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAcObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAccessPointAcStatsObject ( _Dev2WifiAccessPointAcStatsObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAcStatsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#ifdef DMP_DEVICE2_WIFIENDPOINT_1

CmsRet rcl_dev2WifiEndPointObject( _Dev2WifiEndPointObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   /* example for getting radio index
   UINT32 radioIndex;
   radioIndex = qdmWifi_getRadioIndexBySsidFullPathLocked_dev2(newObj->SSIDReference);
   */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointStatsObject( _Dev2WifiEndPointStatsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointStatsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointSecurityObject( _Dev2WifiEndPointSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileObject( _Dev2WifiEndPointProfileObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileSecurityObject( _Dev2WifiEndPointProfileSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointWpsObject( _Dev2WifiEndPointWpsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointWpsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcObject( _Dev2WifiEndPointAcObject *newObj,
                const _Dev2WifiEndPointAcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcStatsObject( _Dev2WifiEndPointAcStatsObject *newObj,
                const _Dev2WifiEndPointAcStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIENDPOINT_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
