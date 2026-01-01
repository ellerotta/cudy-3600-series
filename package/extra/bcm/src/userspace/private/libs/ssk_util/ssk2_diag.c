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
* :>
*/


#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_obj.h"
#include "cms_lck.h"
#include "prctl.h"
#include "ssk_util.h"

// unfortunately, this is a cut-and-paste from owrut_launchApp
CmsRet sskUtil_launchApp(const char *exe, const char *args, SINT32 *pid)
{
    SpawnProcessInfo spawnInfo;
    SpawnedProcessInfo procInfo;
    CmsRet ret;

    cmsLog_notice("Entered: exe=%s args=%s", exe, args);

    memset(&procInfo, 0, sizeof(procInfo));
    memset(&spawnInfo, 0, sizeof(spawnInfo));
    spawnInfo.exe = exe;
    spawnInfo.args = args;
    spawnInfo.spawnMode = SPAWN_AND_RETURN;
    spawnInfo.stdinFd = 0;
    spawnInfo.stdoutFd = 1;
    spawnInfo.stderrFd = 2;
    spawnInfo.maxFd = 50;
    spawnInfo.inheritSigint = FALSE;

    ret = prctl_spawnProcess(&spawnInfo, &procInfo);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("could not spawn process %s args %s", exe, args);
    }
    else
    {
       if (pid != NULL)
       {
           *pid = procInfo.pid;
       }
    }

    return ret;
}

CmsRet sskUtil_terminateApp(SINT32 pid)
{
    CollectProcessInfo collectInfo;
    SpawnedProcessInfo procInfo;
    CmsRet ret;

    collectInfo.collectMode = COLLECT_PID_TIMEOUT;
    collectInfo.pid = pid;
    collectInfo.timeout = 2 * 1000; /* Unit is in ms. */

    if ((ret = prctl_terminateProcessForcefully(pid)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Terminate process failed, pid=%d", pid);
    }

    if ((ret = prctl_collectProcess(&collectInfo, &procInfo)) == CMSRET_SUCCESS)
    {
        cmsLog_debug("collected child pid=%d after signal", pid);
    }
    else
    {
        cmsLog_error("could not collect child pid=%d", pid);
    }

    return ret;
}

static CmsRet convertCmdLineIntfName(void *appMsgHandle, const char *orig, char *cmdLine, UINT32 len)
{
   UINT32 i=0;
   UINT32 j=0;

   cmsLog_debug("Entered: orig=%s", orig);
   while ((orig[i] != '\0') && (j < len))
   {
      cmdLine[j++] = orig[i++];

      if ((orig[i-1] == ' ') &&
          ((orig[i-2] == 'I' || orig[i-2] == 'i')) &&
          (orig[i-3] == '-'))
      {
         // we just copied over a -I or -i, look at interface and convert
         // fullpath to intf name.  (This only happens in BDK mode).
         if (!strncmp(&(orig[i]), "Device.", 7))
         {
            char fullpath[1024]={0};
            char queryStr[128+sizeof(fullpath)]={0};
            char *respStr = NULL;
            UINT32 k=0;
            CmsRet ret;

            // copy fullpath in orig line to tmp fullpath
            while ((orig[i] != ' ') && (k < sizeof(fullpath)-1))
               fullpath[k++] = orig[i++];

            snprintf(queryStr, sizeof(queryStr), "fullpath=%s", fullpath);
            ret = sskUtil_queryIntfName(appMsgHandle, EID_DIAG_MD, queryStr, &respStr);
            if (ret == CMSRET_SUCCESS)
            {
               UINT32 f = 0;
               // copy the intfName to cmdLine
               cmsLog_debug("queryIntfName fullpath=%s resp=%s", fullpath, respStr);
               while (respStr[f] != '\0')
                  cmdLine[j++] = respStr[f++];

               CMSMEM_FREE_BUF_AND_NULL_PTR(respStr);
            }
            else
            {
               // put in a bad intf name and get error response from ping.
               cmdLine[j++] = 'e';
               cmdLine[j++] = 'r';
               cmdLine[j++] = 'r';
            }
         }
      }
   }

   cmsLog_debug("Exit: final cmdLine=%s", cmdLine);
   return CMSRET_SUCCESS;
}

void processStartPingDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered: cmdLine=%s", (char *)(msg+1));

   convertCmdLineIntfName(appMsgHandle, (char *)(msg+1), cmdLine, sizeof(cmdLine)-1);
   // even if there was an error, launch bcm_ping anyways so bcm_ping will
   // report the error.

   ret = sskUtil_launchApp("bcm_ping", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpPingDiagObject *ipPingObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_PING_DIAG, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &ipPingObj)) == CMSRET_SUCCESS)
      {
         ipPingObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(ipPingObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipPingObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &ipPingObj);
      }
   }

   return;
}

void processStopPingDiag_dev2()
{
   Dev2IpPingDiagObject *ipPingObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_PING_DIAG, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &ipPingObj)) == CMSRET_SUCCESS)
   {
      if (ipPingObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(ipPingObj->X_BROADCOM_COM_Pid);
         ipPingObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(ipPingObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipPingObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &ipPingObj);
   }

   return;
}

// strictly speaking, we should have this #ifdef DMP_DEVICE2_IPPING_1
// but avoid compilation issues (at the cost of slighly bigger code).
void processPingStateChanged_dev2(CmsMsgHeader *msg)
{
   PingDataMsgBody *pingInfo = (PingDataMsgBody *) (msg + 1);
   Dev2IpPingDiagObject *ipPingObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags = 0;
   CmsRet ret;

   cmsLog_notice("Entered: dataLength=%d state=%s interface=%s host=%s",msg->dataLength, pingInfo->diagnosticsState, pingInfo->interface, pingInfo->host);

   cmsLog_debug("success %d, fail %d, avg/min/max %d/%d/%d requestId %d",pingInfo->successCount,
                pingInfo->failureCount,pingInfo->averageResponseTime,pingInfo->minimumResponseTime,
                pingInfo->maximumResponseTime,(int)pingInfo->requesterId);

   cmsLck_acquireLock();

   if ((ret = cmsObj_get(MDMOID_DEV2_IP_PING_DIAG, &iidStack, 0, (void **) &ipPingObj)) == CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipPingObj->diagnosticsState);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipPingObj->host);
      ipPingObj->diagnosticsState = cmsMem_strdup(pingInfo->diagnosticsState);
      ipPingObj->host = cmsMem_strdup(pingInfo->host);
      ipPingObj->successCount = pingInfo->successCount;
      ipPingObj->failureCount = pingInfo->failureCount;
      ipPingObj->averageResponseTime = pingInfo->averageResponseTime;
      ipPingObj->maximumResponseTime = pingInfo->maximumResponseTime;
      ipPingObj->minimumResponseTime = pingInfo->minimumResponseTime;

      if (!cmsUtl_strcmp(pingInfo->diagnosticsState, MDMVS_PING_INPROGRESS))
      {
         // for progress updates, just set the data into the MDM, but no need
         // to call the RCL handler function.
         flags |= OSF_NO_RCL_CALLBACK;
      }

      if ((ret = cmsObj_setFlags(ipPingObj, &iidStack, flags)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set of ipPingObj failed, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("set ipPingObj OK, successCount %d, failCount %d",
                      ipPingObj->successCount,ipPingObj->failureCount);
      }     
      
      cmsObj_free((void **) &ipPingObj);
   } 
   else
   {
      cmsLog_error("cmsObj_get of MDMOID_DEV2_IP_PING_DIAG failed, ret %d",ret);
   }

   cmsLck_releaseLock();
   return;
}


void processStartTracertDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered: cmdLine=%s", (char *)(msg+1));

   convertCmdLineIntfName(appMsgHandle, (char *)(msg+1), cmdLine, sizeof(cmdLine)-1);
   // even if there was an error, launch bcm_ping anyways so bcm_ping will
   // report the error.

   ret = sskUtil_launchApp("bcm_traceroute", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagTraceRouteObject *ipTracertObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &ipTracertObj)) == CMSRET_SUCCESS)
      {
         ipTracertObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(ipTracertObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipTracertObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &ipTracertObj);
      }
   }

   return;
}

void processStopTracertDiag_dev2()
{
   Dev2IpDiagTraceRouteObject *ipTracertObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &ipTracertObj)) == CMSRET_SUCCESS)
   {
      if (ipTracertObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(ipTracertObj->X_BROADCOM_COM_Pid);
         ipTracertObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(ipTracertObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipTracertObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &ipTracertObj);
   }

   return;
}

// strictly speaking, we should have this #ifdef DMP_DEVICE2_TRACEROUTE_1
// but avoid compilation issues (at the cost of slighly bigger code).
void processTracertStateChanged_dev2(CmsMsgHeader *msg)
{
   TracertDataMsgBody *tracertInfo = (TracertDataMsgBody *)(msg + 1);
   Dev2IpDiagTraceRouteObject *ipTracertObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("Entered: dataLength=%d state=%s",
                 msg->dataLength, tracertInfo->diagnosticsState);

   cmsLck_acquireLock();

   if ((cmsUtl_strcmp(tracertInfo->diagnosticsState, MDMVS_REQUESTED) == 0) ||
       (cmsUtl_strcmp(tracertInfo->diagnosticsState, MDMVS_COMPLETE) == 0))
   {
      // Intermediate hop result, or test complete which also contains the
      // final hop result.  In either case, must add a new hop object.
      if ((ret = cmsObj_addInstance(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &iidStack)) == CMSRET_SUCCESS)
      {
         Dev2IpDiagTraceRouteRouteHopsObject *routeHopsObj = NULL;
         if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &iidStack, 0, (void **) &routeHopsObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("cmsObj_get, MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS success");
            routeHopsObj->hostAddress = cmsMem_strdup(tracertInfo->hostAddrOfRouteHop);
            routeHopsObj->host = cmsMem_strdup(tracertInfo->hostOfRouteHop);
            routeHopsObj->RTTimes = cmsMem_strdup(tracertInfo->rtTimes);
            routeHopsObj->errorCode = tracertInfo->errorCode;

            if ((ret = cmsObj_set(routeHopsObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("set of routeHopsObj failed, ret=%d", ret);
            }
            else
            {
               cmsLog_debug("set routeHopsObj OK");
            }
            cmsObj_free((void **) &routeHopsObj);
         }
         else
         {
            cmsLog_debug("cmsObj_get, MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS failed, ret=%d",ret);
         }
      }
   }

   if (cmsUtl_strcmp(tracertInfo->diagnosticsState, MDMVS_REQUESTED) != 0)
   {
      // Test is done.
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE, &iidStack, 0, (void **) &ipTracertObj)) == CMSRET_SUCCESS)
      {
         ipTracertObj->diagnosticsState = cmsMem_strdup(tracertInfo->diagnosticsState);
         ipTracertObj->responseTime = tracertInfo->averageResponseTime;

         if ((ret = cmsObj_set(ipTracertObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipTracertObj failed, ret=%d", ret);
         }
         else
         {
            cmsLog_debug("set ipTracertObj OK");
         }
         cmsObj_free((void **) &ipTracertObj);
      } 
      else
      {
         cmsLog_debug("cmsObj_get, MDMOID_DEV2_IP_DIAG_TRACE_ROUTE failed, ret=%d",ret);
      }
   }

   cmsLck_releaseLock();

   return;
}

void processStartDownloadDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered: cmdLine=%s", (char *)(msg+1));

   convertCmdLineIntfName(appMsgHandle, (char *)(msg+1), cmdLine, sizeof(cmdLine)-1);
   // even if there was an error, launch the app anyways so it will
   // report the error.

   ret = sskUtil_launchApp("tr143DownloadDiag", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagDownloadObject *downloadObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &downloadObj)) == CMSRET_SUCCESS)
      {
         downloadObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(downloadObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of downloadObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &downloadObj);
      }
   }

   return;
}

void processStopDownloadDiag_dev2()
{
   Dev2IpDiagDownloadObject *downloadObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &downloadObj)) == CMSRET_SUCCESS)
   {
      if (downloadObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(downloadObj->X_BROADCOM_COM_Pid);
         downloadObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(downloadObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of downloadObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &downloadObj);
   }

   return;
}

void processDownloadDiagComplete_dev2()
{
   Dev2IpDiagDownloadObject *downloadObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &iidStack,
                         0,
                         (void **) &downloadObj)) == CMSRET_SUCCESS)
   {
      cmsLog_notice("status=%s", downloadObj->diagnosticsState);
      cmsObj_free((void **) &downloadObj);
   }

   return;
}

void processStartUploadDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered: cmdLine=%s", (char *)(msg+1));

   convertCmdLineIntfName(appMsgHandle, (char *)(msg+1), cmdLine, sizeof(cmdLine)-1);
   // even if there was an error, launch the app anyways so it will
   // report the error.

   ret = sskUtil_launchApp("tr143UploadDiag", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagUploadObject *uploadObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &uploadObj)) == CMSRET_SUCCESS)
      {
         uploadObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(uploadObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of uploadObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &uploadObj);
      }
   }

   return;
}

void processStopUploadDiag_dev2()
{
   Dev2IpDiagUploadObject *uploadObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &uploadObj)) == CMSRET_SUCCESS)
   {
      if (uploadObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(uploadObj->X_BROADCOM_COM_Pid);
         uploadObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(uploadObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of uploadObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &uploadObj);
   }

   return;
}

void processUploadDiagComplete_dev2()
{
   Dev2IpDiagUploadObject *uploadObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &iidStack,
                         0,
                         (void **) &uploadObj)) == CMSRET_SUCCESS)
   {
      cmsLog_notice("status=%s", uploadObj->diagnosticsState);
      cmsObj_free((void **) &uploadObj);
   }

   return;
}

void processStartUdpechoDiag_dev2(void *appMsgHandle, const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered: cmdLine=%s", (char *)(msg+1));

   convertCmdLineIntfName(appMsgHandle, (char *)(msg+1), cmdLine, sizeof(cmdLine)-1);
   // even if there was an error, launch the app anyways so it will
   // report the error.

   ret = sskUtil_launchApp("tr143EchoCfgClient", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2UDPEchoDiagObject *udpechoObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_UDP_ECHO_DIAG, &iidStack,OGF_NO_VALUE_UPDATE,
                            (void **) &udpechoObj)) == CMSRET_SUCCESS)
      {
         udpechoObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(udpechoObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of udpechoObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &udpechoObj);
      }
   }

   return;
}

void processStopUdpechoDiag_dev2()
{
   Dev2UDPEchoDiagObject *udpechoObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_UDP_ECHO_DIAG, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &udpechoObj)) == CMSRET_SUCCESS)
   {
      if (udpechoObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(udpechoObj->X_BROADCOM_COM_Pid);
         udpechoObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(udpechoObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of udpechoObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &udpechoObj);
   }

   return;
}

void processUdpechoDiagComplete_dev2()
{
   Dev2UDPEchoDiagObject *udpechoObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_UDP_ECHO_DIAG, &iidStack, 0,
                         (void **) &udpechoObj)) == CMSRET_SUCCESS)
   {
      cmsLog_notice("status=%s", udpechoObj->diagnosticsState);
      cmsObj_free((void **) &udpechoObj);
   }

   return;
}

void processStartUdpecho_dev2(const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("Entered:");
   if (msg->dataLength > 0)
   {
      strncpy(cmdLine, (char *)(msg+1), sizeof(cmdLine)-1);
   }
   cmsLog_notice("Launching echoserver cmdLine=%s", cmdLine);

   ret = sskUtil_launchApp("tr143EchoCfgServer", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagUDPEchoConfigObject *echoObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_UDP_ECHO_CONFIG, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &echoObj)) == CMSRET_SUCCESS)
      {
         echoObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(echoObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of echoObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &echoObj);
      }
   }

   return;
}

void processStopUdpecho_dev2()
{
   Dev2IpDiagUDPEchoConfigObject *echoObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_UDP_ECHO_CONFIG, &iidStack,
                         OGF_NO_VALUE_UPDATE,
                         (void **) &echoObj)) == CMSRET_SUCCESS)
   {
      if (echoObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(echoObj->X_BROADCOM_COM_Pid);
         echoObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(echoObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of echoObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &echoObj);
   }

   return;
}

void processStartUdpst_dev2(const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_notice("CMS_MSG_START_OBUDPST");
   
   if (msg->dataLength > 0)
   {
      strncpy(cmdLine, (char *)(msg+1), sizeof(cmdLine)-1);
   }
   
   cmsLog_notice("Launching udpst cmdLine=%s", cmdLine);

   ret = sskUtil_launchApp("udpst", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagIPCapObject *ipDiagObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_IP_CAP, &iidStack, OGF_NO_VALUE_UPDATE,
                            (void **) &ipDiagObj)) == CMSRET_SUCCESS)
      {
         ipDiagObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(ipDiagObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipDiagObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &ipDiagObj);
      }
   }

   return;
}

void processStopUdpst_dev2()
{
   Dev2IpDiagIPCapObject *ipDiagObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("CMS_MSG_STOP_OBUDPST");

   // use auto-locking
   // Just want to get the pid, no need to update any params.
   if ((ret = cmsObj_get(MDMOID_IP_CAP, &iidStack, OGF_NO_VALUE_UPDATE,
                         (void **) &ipDiagObj)) == CMSRET_SUCCESS)
   {
      if (ipDiagObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(ipDiagObj->X_BROADCOM_COM_Pid);
         ipDiagObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(ipDiagObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipDiagObj failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **) &ipDiagObj);
   }

   return;
}

void processUdpstDiagComplete_dev2()
{
   Dev2IpDiagIPCapObject *ipDiagObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("CMS_MSG_OBUDPST_DIAG_COMPLETE");
   
   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_IP_CAP, &iidStack, 0,
                         (void **) &ipDiagObj)) == CMSRET_SUCCESS)
   {
      cmsLog_notice("status=%s", ipDiagObj->diagnosticsState);
      cmsObj_free((void **) &ipDiagObj);
   }

   return;
}

void processEthCableDiagComplete_dev2()
{
   cmsLog_notice("CMS_MSG_ETHCABLE_DIAG_COMPLETE");
   // do nothing since the result has been updated.
   ;
}


void processStartServerSelection_dev2(const CmsMsgHeader *msg)
{
   char cmdLine[1024] = {0};
   SINT32 pid = -1;
   CmsRet ret;

   cmsLog_debug("Entered:");
   if (msg->dataLength > 0)
   {
      strncpy(cmdLine, (char *)(msg+1), sizeof(cmdLine)-1);
   }
   cmsLog_notice("Launching serverselection cmdLine=%s", cmdLine);

   ret = sskUtil_launchApp("tr143ServerSelection", cmdLine, &pid);
   if (ret == CMSRET_SUCCESS)
   {
      Dev2IpDiagServerSelectionObject *serverSelectDiagObj= NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      // save pid to object
      // use auto-locking
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_SERVER_SELECTION, &iidStack,
                            OGF_NO_VALUE_UPDATE,
                            (void **) &serverSelectDiagObj)) == CMSRET_SUCCESS)
      {
         serverSelectDiagObj->X_BROADCOM_COM_Pid = pid;

         // We just want to set the pid into the MDM, but do not want to trigger
         // any actions in the RCL handler function.
         ret = cmsObj_setFlags(serverSelectDiagObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of echoObj failed, ret=%d", ret);
         }

         cmsObj_free((void **) &serverSelectDiagObj);
      }
   }

   return;
}

void processStopServerSelection_dev2(void)
{
   Dev2IpDiagServerSelectionObject *serverSelectDiagObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("CMS_MSG_STOP_SERVERSELECTION_DIAG");
   
   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_SERVER_SELECTION, &iidStack, 0,
                         (void **) &serverSelectDiagObj)) == CMSRET_SUCCESS)
   {
      if (serverSelectDiagObj->X_BROADCOM_COM_Pid > -1)
      {
         sskUtil_terminateApp(serverSelectDiagObj->X_BROADCOM_COM_Pid);
         serverSelectDiagObj->X_BROADCOM_COM_Pid = -1;
         ret = cmsObj_setFlags(serverSelectDiagObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("set of ipDiagObj failed, ret=%d", ret);
         }
      }
      cmsObj_free((void **) &serverSelectDiagObj);
   }

   return;
}

void processServerSelectionDiagComplete_dev2(void)
{
   Dev2IpDiagServerSelectionObject *serverSelectDiagObj= NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("CMS_MSG_SERVERSELECTION_DIAG_COMPLETE");
   
   // use auto-locking
   // Doing a get will trigger the STL handler function to read the results
   // file and update the object.
   if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_SERVER_SELECTION, &iidStack, 0,
                         (void **) &serverSelectDiagObj)) == CMSRET_SUCCESS)
   {
      cmsLog_notice("status=%s", serverSelectDiagObj->diagnosticsState);
      cmsObj_free((void **) &serverSelectDiagObj);
   }

   return;
}

#endif   /* DMP_DEVICE2_BASELINE_1 */

