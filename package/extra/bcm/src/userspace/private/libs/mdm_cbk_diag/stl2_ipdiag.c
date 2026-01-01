/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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

#include "cms_log.h"
#include "cms_mem.h"
#include "stl.h"
#include "rut_util.h"
#include "rut2_ipdiag.h"

/*!\file stl2_ipdiag.c
 * \brief This file contains device 2 device.ip.diagnostics. objects related functions.
 *
 */
#ifdef DMP_DEVICE2_IPPING_1
CmsRet stl_dev2IpDiagnosticsObject(_Dev2IpDiagnosticsObject *obj __attribute__((unused)),
                                       const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2IpPingDiagObject(_Dev2IpPingDiagObject *obj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* no change is needed since SSK is updating MDM with proper stats from PING */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_IPPING_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */

#if defined(DMP_DEVICE2_DOWNLOAD_1) || defined(DMP_DEVICE2_UPLOAD_1) || defined(DMP_DEVICE2_UDPECHODIAG_1)
/* this header file is installed by userspace/private/libs/tr143_utils */
#include "tr143_defs.h"
#endif

#ifdef DMP_DEVICE2_DOWNLOAD_1
CmsRet stl_dev2IpDiagDownloadObject(_Dev2IpDiagDownloadObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   FILE *fp;
   struct tr143diagstats_t diagstats;
   size_t rc;

   cmsLog_debug("=====> Enter");

   // If result file is not present, that means we have already read the
   // results into the MDM and deleted the results file.  (or the results are
   // not available yet).
   fp=fopen(TR143_DOWNLOAD_RESULT_FILE,"r");
   if (fp == NULL)
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;

   memset(&diagstats, 0, sizeof(diagstats));
   rc = fread(&diagstats,1,sizeof(diagstats),fp);
   fclose(fp);

   if (rc < 1)
   {
      /* fread returns number of elements read (1) */
      cmsLog_error("fread failed");  // printing rc is problematic, maybe use %zu?
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   diagstats.DiagnosticsState[sizeof(diagstats.DiagnosticsState)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, diagstats.DiagnosticsState, mdmLibCtx.allocFlags);
   diagstats.ROMTime[sizeof(diagstats.ROMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->ROMTime, diagstats.ROMTime, mdmLibCtx.allocFlags);
   diagstats.BOMTime[sizeof(diagstats.BOMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->BOMTime, diagstats.BOMTime, mdmLibCtx.allocFlags);
   diagstats.EOMTime[sizeof(diagstats.EOMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->EOMTime, diagstats.EOMTime, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, diagstats.ipAddr, mdmLibCtx.allocFlags);
   obj->testBytesReceived = diagstats.TestBytes;
   obj->totalBytesReceived = diagstats.TotalBytesReceived;
   obj->totalBytesSent = diagstats.TotalBytesSent;
   obj->X_BROADCOM_COM_UserStop = diagstats.forceStop;
#ifdef DMP_DEVICE2_DOWNLOADTCP_1
   diagstats.TCPOpenRequestTime[sizeof(diagstats.TCPOpenRequestTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->TCPOpenRequestTime, diagstats.TCPOpenRequestTime, mdmLibCtx.allocFlags);
   diagstats.TCPOpenResponseTime[sizeof(diagstats.TCPOpenResponseTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->TCPOpenResponseTime, diagstats.TCPOpenResponseTime, mdmLibCtx.allocFlags);
#endif

   if (obj->enablePerConnectionResults == TRUE)
   {
      UINT32 i;
      for (i = 0 ; i < obj->numberOfConnections ; i++)
      {
         CmsRet ret;
         connectionResult_t connResult;
         char connFileName[CMS_IFNAME_LENGTH] = {0};
         _Dev2DownloadPerConnResultObject *perConnResObj = NULL;
         InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
         char timeBuf[BUFLEN_64];

         sprintf(connFileName, "%s%s%d", TR143_DOWNLOAD_RESULT_FILE, TR143_CONNFILE_MIDLE_NAME, i);
         fp=fopen(connFileName, "r");
         if (fp==NULL) continue;
         memset(&connResult, 0, sizeof(connectionResult_t));
         rc = fread(&connResult, 1, sizeof(connectionResult_t), fp);
         fclose(fp);
         if (rc < 1)
         {
            cmsLog_error("fread failed");
            continue;
         }

         if ((ret = cmsObj_addInstance(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not add MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, ret=%d", ret);
            continue;
         }

         if ((ret = cmsObj_get(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &iidStack2, 0, (void **) &perConnResObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get _Dev2DownloadPerConnResultObject, ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &iidStack2);
            continue;
         }

         // fill up DownloadDiagnostics
         cmsTms_getXSIDateTimeMicroseconds(connResult.TCPOpenRequestTime.tv_sec,
               connResult.TCPOpenRequestTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->TCPOpenRequestTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.TCPOpenResponseTime.tv_sec,
               connResult.TCPOpenResponseTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->TCPOpenResponseTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.ROMTime.tv_sec,
               connResult.ROMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->ROMTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.BOMTime.tv_sec,
               connResult.BOMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->BOMTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.EOMTime.tv_sec,
               connResult.EOMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->EOMTime, timeBuf, mdmLibCtx.allocFlags);

         perConnResObj->testBytesReceived = connResult.TestBytes;
         perConnResObj->totalBytesReceived = connResult.TotalBytesReceivedEnd - connResult.TotalBytesReceivedBegin;
         perConnResObj->totalBytesSent = connResult.TotalBytesSentEnd - connResult.TotalBytesSentBegin;


         if ((ret = cmsObj_set(perConnResObj, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT> returns error. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &iidStack2);
            cmsObj_free((void **) &perConnResObj);
            return ret;
         }

         cmsObj_free((void **) &perConnResObj);
      }
   }

   // Delete the results file after we have read it into the MDM.
   unlink(TR143_DOWNLOAD_RESULT_FILE);
   rut_sendEventMsgToSsk(CMS_MSG_STOP_DOWNLOAD_DIAG, EID_DOWNLOAD_DIAG, NULL, 0);

   return CMSRET_SUCCESS;
}

CmsRet stl_dev2DownloadPerConnResultObject(_Dev2DownloadPerConnResultObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   cmsLog_debug("=====> Enter");
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_DOWNLOAD_1 */

#ifdef DMP_DEVICE2_UPLOAD_1
CmsRet stl_dev2IpDiagUploadObject(_Dev2IpDiagUploadObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   FILE *fp;
   struct tr143diagstats_t diagstats;
   size_t rc;

   cmsLog_debug("=====> Enter");

   // If result file is not present, that means we have already read the
   // results into the MDM and deleted the results file.  (or the results are
   // not available yet).
   fp=fopen(TR143_UPLOAD_RESULT_FILE,"r");
   if (fp == NULL)
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;

   memset(&diagstats, 0, sizeof(diagstats));
   rc = fread(&diagstats,1,sizeof(diagstats),fp);
   fclose(fp);

   if (rc < 1)
   {
      /* fread returns number of elements read (1) */
      cmsLog_error("fread failed");  // printing rc is problematic, maybe use %zu?
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   diagstats.DiagnosticsState[sizeof(diagstats.DiagnosticsState)-1] = '\0';
   obj->X_BROADCOM_COM_UserStop = diagstats.forceStop;
   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, diagstats.DiagnosticsState, mdmLibCtx.allocFlags);
   diagstats.ROMTime[sizeof(diagstats.ROMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->ROMTime, diagstats.ROMTime, mdmLibCtx.allocFlags);
   diagstats.BOMTime[sizeof(diagstats.BOMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->BOMTime, diagstats.BOMTime, mdmLibCtx.allocFlags);
   diagstats.EOMTime[sizeof(diagstats.EOMTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->EOMTime, diagstats.EOMTime, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, diagstats.ipAddr, mdmLibCtx.allocFlags);
   obj->testBytesSent = diagstats.TestBytes;
   obj->totalBytesReceived = diagstats.TotalBytesReceived;
   obj->totalBytesSent = diagstats.TotalBytesSent;
#ifdef DMP_DEVICE2_UPLOADTCP_1
   diagstats.TCPOpenRequestTime[sizeof(diagstats.TCPOpenRequestTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->TCPOpenRequestTime, diagstats.TCPOpenRequestTime, mdmLibCtx.allocFlags);
   diagstats.TCPOpenResponseTime[sizeof(diagstats.TCPOpenResponseTime)-1] = '\0';
   CMSMEM_REPLACE_STRING_FLAGS(obj->TCPOpenResponseTime, diagstats.TCPOpenResponseTime, mdmLibCtx.allocFlags);
#endif

   if (obj->enablePerConnectionResults == TRUE)
   {
      UINT32 i;
      for (i = 0 ; i < obj->numberOfConnections ; i++)
      {
         CmsRet ret;
         connectionResult_t connResult;
         char connFileName[CMS_IFNAME_LENGTH] = {0};
         _Dev2UploadPerConnResultObject *perConnResObj = NULL;
         InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
         char timeBuf[BUFLEN_64];

         sprintf(connFileName, "%s%s%d", TR143_UPLOAD_RESULT_FILE, TR143_CONNFILE_MIDLE_NAME, i);
         fp=fopen(connFileName, "r");
         if (fp==NULL) continue;
         memset(&connResult, 0, sizeof(connectionResult_t));
         rc = fread(&connResult, 1, sizeof(connectionResult_t), fp);
         fclose(fp);
         if (rc < 1)
         {
            cmsLog_error("fread failed");
            continue;
         }

         if ((ret = cmsObj_addInstance(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not add MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, ret=%d", ret);
            continue;
         }

         if ((ret = cmsObj_get(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &iidStack2, 0, (void **) &perConnResObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get _Dev2UploadPerConnResultObject, ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &iidStack2);
            continue;
         }

         // fill up UploadDiagnostics
         cmsTms_getXSIDateTimeMicroseconds(connResult.TCPOpenRequestTime.tv_sec,
               connResult.TCPOpenRequestTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->TCPOpenRequestTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.TCPOpenResponseTime.tv_sec,
               connResult.TCPOpenResponseTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->TCPOpenResponseTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.ROMTime.tv_sec,
               connResult.ROMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->ROMTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.BOMTime.tv_sec,
               connResult.BOMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->BOMTime, timeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTimeMicroseconds(connResult.EOMTime.tv_sec,
               connResult.EOMTime.tv_usec, timeBuf, 64);
         CMSMEM_REPLACE_STRING_FLAGS(perConnResObj->EOMTime, timeBuf, mdmLibCtx.allocFlags);

         perConnResObj->testBytesSent = connResult.TestBytes;
         perConnResObj->totalBytesReceived = connResult.TotalBytesReceivedEnd - connResult.TotalBytesReceivedBegin;
         perConnResObj->totalBytesSent = connResult.TotalBytesSentEnd - connResult.TotalBytesSentBegin;


         if ((ret = cmsObj_set(perConnResObj, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_DEV2_UPLOAD_PER_CONN_RESULT> returns error. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &iidStack2);
            cmsObj_free((void **) &perConnResObj);
            return ret;
         }

         cmsObj_free((void **) &perConnResObj);
      }
   }

   // Delete the results file after we have read it into the MDM.
   unlink(TR143_UPLOAD_RESULT_FILE);
   rut_sendEventMsgToSsk(CMS_MSG_STOP_UPLOAD_DIAG, EID_UPLOAD_DIAG, NULL, 0);

   return CMSRET_SUCCESS;
}

CmsRet stl_dev2UploadPerConnResultObject(_Dev2UploadPerConnResultObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   cmsLog_debug("=====> Enter");
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_UPLOAD_1 */

#ifdef DMP_DEVICE2_UDPECHO_1
CmsRet stl_dev2IpDiagUDPEchoConfigObject(_Dev2IpDiagUDPEchoConfigObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_UDPECHO_1 */

#ifdef DMP_DEVICE2_UDPECHODIAG_1
typedef struct tr143UdpechoHistory
{
   char state[64];
   int successCnt;
   int failCnt;
   int minimumTime;
   int maximumTime;
   int averageTime;
   int userStop;
} tr143UdpechoHistory_t;

CmsRet stl_dev2UDPEchoDiagObject(_Dev2UDPEchoDiagObject *obj __attribute__((unused)),
                 const InstanceIdStack *iidStack __attribute__((unused)))
{
   FILE *fp;
   tr143UdpechoHistory_t diagstats;
   char * line = NULL;
   size_t len = 0;
   ssize_t read = 0;
   char ipAddr[64]={0};

   cmsLog_debug("=====> Enter");

   // If result file is not present, that means we have already read the
   // results into the MDM and deleted the results file.  (or the results are
   // not available yet).
   fp=fopen(TR143_UDPECHO_RESULT_FILE,"r");
   if (fp == NULL)
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;

   memset(&diagstats, 0, sizeof(diagstats));

   while ((read = getline(&line, &len, fp)) != -1)
   {
      int length=0;
      char buf[1024]={0};

      strncpy(buf, line, sizeof(buf)-1);
      length = strlen(buf);
      if (buf[length-1] == '\n')
          buf[length-1] = '\0';

      sscanf(buf, "%s %s %d %d %d %d %d %d", diagstats.state, ipAddr,
             &diagstats.successCnt, &diagstats.failCnt,
             &diagstats.minimumTime, &diagstats.maximumTime,
             &diagstats.averageTime, &diagstats.userStop);
   }

   fclose(fp);

   if (diagstats.state[0] == '\0')
   {
      cmsLog_error("invalid file");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, diagstats.state, mdmLibCtx.allocFlags);
   if (strcmp(ipAddr, "NA") != 0)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, ipAddr,mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, "",mdmLibCtx.allocFlags);
   }

   obj->successCount = diagstats.successCnt;
   obj->failureCount = diagstats.failCnt;
   obj->averageResponseTime = diagstats.averageTime;
   obj->minimumResponseTime = diagstats.minimumTime;
   obj->maximumResponseTime = diagstats.maximumTime;
   obj->X_BROADCOM_COM_UserStop = diagstats.userStop;

   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_DEVICE2_TRACEROUTE_1
CmsRet stl_dev2IpDiagTraceRouteObject(_Dev2IpDiagTraceRouteObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
};

CmsRet stl_dev2IpDiagTraceRouteRouteHopsObject(_Dev2IpDiagTraceRouteRouteHopsObject *obj __attribute__((unused)),
           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
};

#endif /* DMP_DEVICE2_TRACEROUTE_1 */

#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
CmsRet stl_dev2IpDiagIPCapObject(_Dev2IpDiagIPCapObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   char buf[10240] = {0}, dbBuf[128];
   FILE *fp = NULL;
   int rc, i, incResObjectLen, modResObjectLen;
   json_object *jobj = NULL, *outputObj = NULL, *atMaxObject = NULL, *summaryObject = NULL;
   json_object *incResObject = NULL, *incResIObject = NULL, *modResObject = NULL, *modResIObject = NULL;
   CmsRet ret;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   Dev2IpDiagIPCapIncResObject *incResObj = NULL;
   Dev2IpDiagIPCapModResObject *modResObj = NULL;

   // If file is not there, then there is no data to update.
   if ((fp = fopen(UDPST_DIAG_RESULT_FILE,"r")) == NULL)
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   rc = fread(buf, 1, sizeof(buf), fp);
   fclose(fp);

   if (rc < 1 || (jobj = json_tokener_parse(buf)) == NULL)
   {
      cmsLog_error("invalid file content, rc = %d", rc);
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   rc = json_object_object_get_int(jobj, "ErrorStatus");
   if (rc)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, MDMVS_ERROR_INTERNAL, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }

   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, MDMVS_COMPLETE, mdmLibCtx.allocFlags);
   
   if (!json_object_object_get_ex(jobj, "Output", &outputObj))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, MDMVS_ERROR_INTERNAL, mdmLibCtx.allocFlags);
      json_object_put(jobj);
      return CMSRET_SUCCESS;
   }
   
   CMSMEM_REPLACE_STRING_FLAGS(obj->BOMTime, json_object_object_get_string(outputObj, "BOMTime"), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->EOMTime, json_object_object_get_string(outputObj, "EOMTime"), mdmLibCtx.allocFlags);
   obj->tmaxUsed = json_object_object_get_int(outputObj, "TmaxUsed");
   obj->testInterval = json_object_object_get_int(outputObj, "TestInterval");
   obj->tmaxRTTUsed = json_object_object_get_int(outputObj, "TmaxRTTUsed");
   obj->timestampResolutionUsed = json_object_object_get_int(outputObj, "TimestampResolutionUsed");
   
   if (!json_object_object_get_ex(outputObj, "AtMax", &atMaxObject))
   {
      json_object_put(jobj);
      return CMSRET_SUCCESS;
   }

   CMSMEM_REPLACE_STRING_FLAGS(obj->maxIPLayerCapacity, json_object_object_get_doubleString(atMaxObject, "MaxIPLayerCapacity", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->timeOfMax, json_object_object_get_string(atMaxObject, "TimeOfMax"), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->maxETHCapacityNoFCS, json_object_object_get_doubleString(atMaxObject, "MaxETHCapacityNoFCS", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->maxETHCapacityWithFCS, json_object_object_get_doubleString(atMaxObject, "MaxETHCapacityWithFCS", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->maxETHCapacityWithFCSVLAN, json_object_object_get_doubleString(atMaxObject, "MaxETHCapacityWithFCSVLAN", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->lossRatioAtMax, json_object_object_get_doubleString(atMaxObject, "LossRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->RTTRangeAtMax, json_object_object_get_doubleString(atMaxObject, "RTTRangeAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->PDVRangeAtMax, json_object_object_get_doubleString(atMaxObject, "PDVRangeAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->minOnewayDelayAtMax, json_object_object_get_doubleString(atMaxObject, "MinOnewayDelayAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->reorderedRatioAtMax, json_object_object_get_doubleString(atMaxObject, "ReorderedRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->replicatedRatioAtMax, json_object_object_get_doubleString(atMaxObject, "ReplicatedRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->interfaceEthMbpsAtMax, json_object_object_get_doubleString(atMaxObject, "InterfaceEthMbps", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);

   if (!json_object_object_get_ex(outputObj, "Summary", &summaryObject))
   {
      json_object_put(jobj);
      return CMSRET_SUCCESS;
   }

   CMSMEM_REPLACE_STRING_FLAGS(obj->IPLayerCapacitySummary, json_object_object_get_doubleString(summaryObject, "IPLayerCapacitySummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->lossRatioSummary, json_object_object_get_string(summaryObject, "LossRatioSummary"), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->RTTRangeSummary, json_object_object_get_doubleString(summaryObject, "RTTRangeSummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->PDVRangeSummary, json_object_object_get_doubleString(summaryObject, "PDVRangeSummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->minOnewayDelaySummary, json_object_object_get_doubleString(summaryObject, "MinOnewayDelaySummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->minRTTSummary, json_object_object_get_doubleString(summaryObject, "RTTMin", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->reorderedRatioSummary, json_object_object_get_doubleString(summaryObject, "ReorderedRatioSummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->replicatedRatioSummary, json_object_object_get_doubleString(summaryObject, "ReplicatedRatioSummary", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(obj->interfaceEthMbpsSummary, json_object_object_get_doubleString(summaryObject, "InterfaceEthMbps", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);

    
   if (!json_object_object_get_ex(outputObj, "IncrementalResult", &incResObject))
   {
      json_object_put(jobj);
      return CMSRET_SUCCESS;
   }
   
   incResObjectLen = json_object_array_length(incResObject);
   cmsLog_debug("incResObjectLen = %d", incResObjectLen);
   
   for (i = 0; i < incResObjectLen; i++)
   { 
      if ((ret = cmsObj_addInstance(MDMOID_IP_CAP_INC_RES, &iidStack2)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not add MDMOID_IP_CAP_INC_RES, ret=%d", ret);
         break;
      }
  
      if ((ret = cmsObj_get(MDMOID_IP_CAP_INC_RES, &iidStack2, 0, (void **) &incResObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get MDMOID_IP_CAP_INC_RES, ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_IP_CAP_INC_RES, &iidStack2);
         break;
      }
      
      incResIObject = json_object_array_get_idx(incResObject, i);

      CMSMEM_REPLACE_STRING_FLAGS(incResObj->IPLayerCapacity, json_object_object_get_doubleString(incResIObject, "IPLayerCapacity", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->timeOfSubInterval, json_object_object_get_string(incResIObject, "TimeOfSubInterval"), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->lossRatio, json_object_object_get_doubleString(incResIObject, "LossRatio", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->RTTRange, json_object_object_get_doubleString(incResIObject, "RTTRange", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->PDVRange, json_object_object_get_doubleString(incResIObject, "PDVAvg", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->minOnewayDelay, json_object_object_get_doubleString(incResIObject, "MinOnewayDelay", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->reorderedRatio, json_object_object_get_doubleString(incResIObject, "ReorderedRatio", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->replicatedRatio, json_object_object_get_doubleString(incResIObject, "ReplicatedRatio", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(incResObj->interfaceEthMbps, json_object_object_get_doubleString(incResIObject, "InterfaceEthMbps", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
      if ((ret = cmsObj_set(incResObj, &iidStack2)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set MDMOID_IP_CAP_INC_RES returns error. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_IP_CAP_INC_RES, &iidStack2);
         cmsObj_free((void **) &incResObj);   
         break;
      }
      
      INIT_INSTANCE_ID_STACK(&iidStack2);
      cmsObj_free((void **) &incResObj);   
   }
      
   if (obj->numberFirstModeTestSubIntervals && 
       json_object_object_get_ex(outputObj, "ModalResult", &modResObject))
   {
      modResObjectLen = json_object_array_length(modResObject);
      cmsLog_debug("modResObjectLen = %d", modResObjectLen);
      for (i = 0; i < modResObjectLen; i++)
      { 
         INIT_INSTANCE_ID_STACK(&iidStack2);
         if ((ret = cmsObj_addInstance(MDMOID_IP_CAP_MOD_RES, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not add MDMOID_IP_CAP_MOD_RES, ret=%d", ret);
            break;
         }
     
         if ((ret = cmsObj_get(MDMOID_IP_CAP_MOD_RES, &iidStack2, 0, (void **) &modResObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get MDMOID_IP_CAP_MOD_RES, ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_IP_CAP_MOD_RES, &iidStack2);
            break;
         }
         
         modResIObject = json_object_array_get_idx(modResObject, i);   
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->maxIPLayerCapacity, json_object_object_get_doubleString(modResIObject, "MaxIPLayerCapacity", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->timeOfMax, json_object_object_get_string(modResIObject, "TimeOfMax"), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->maxETHCapacityNoFCS, json_object_object_get_doubleString(modResIObject, "MaxETHCapacityNoFCS", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->maxETHCapacityWithFCS, json_object_object_get_doubleString(modResIObject, "MaxETHCapacityWithFCS", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->maxETHCapacityWithFCSVLAN, json_object_object_get_doubleString(modResIObject, "MaxETHCapacityWithFCSVLAN", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->lossRatioAtMax, json_object_object_get_doubleString(modResIObject, "LossRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->RTTRangeAtMax, json_object_object_get_doubleString(modResIObject, "RTTRangeAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->PDVRangeAtMax, json_object_object_get_doubleString(modResIObject, "PDVRangeAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->minOnewayDelayAtMax, json_object_object_get_doubleString(modResIObject, "MinOnewayDelayAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->reorderedRatioAtMax, json_object_object_get_doubleString(modResIObject, "ReorderedRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->replicatedRatioAtMax, json_object_object_get_doubleString(modResIObject, "ReplicatedRatioAtMax", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(modResObj->interfaceEthMbpsAtMax, json_object_object_get_doubleString(modResIObject, "InterfaceEthMbps", dbBuf, sizeof(dbBuf)), mdmLibCtx.allocFlags);
         if ((ret = cmsObj_set(modResObj, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set MDMOID_IP_CAP_MOD_RES returns error. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_IP_CAP_MOD_RES, &iidStack2);
            cmsObj_free((void **) &modResObj);   
            break;
         }
         
         cmsObj_free((void **) &modResObj);   
      }
   }
      
   json_object_put(jobj);
   unlink(UDPST_DIAG_RESULT_FILE);
   rut_sendEventMsgToSsk(CMS_MSG_STOP_OBUDPST, EID_OBUDPST, NULL, 0);

   return CMSRET_SUCCESS;
};

CmsRet stl_dev2IpDiagIPCapModResObject(_Dev2IpDiagIPCapModResObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2IpDiagIPCapIncResObject(_Dev2IpDiagIPCapIncResObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DEVICE2_IPLAYERCAPACITYTEST_1 */

#ifdef DMP_DEVICE2_SERVERSELECTIONDIAG_1
typedef struct tr143ServerSelectResult_t
{
   char state[64];
   char fastestHost[1024]; 
   unsigned int minimumTime;
   unsigned int maximumTime;
   unsigned int averageTime;
   char ipaddrUsed[64];
   int userStop;
} tr143ServerSelectResult_t;

CmsRet stl_dev2IpDiagServerSelectionObject(_Dev2IpDiagServerSelectionObject *obj,
                           const InstanceIdStack *iidStack  __attribute__((unused)))
{
   FILE *fp;
   tr143ServerSelectResult_t diagstats;
   char * line = NULL;
   size_t len = 0;
   ssize_t read = 0;
   char ipAddr[64]={0};

   cmsLog_debug("=====> Enter");

   // If result file is not present, that means we have already read the
   // results into the MDM and deleted the results file.  (or the results are
   // not available yet).
   fp=fopen(TR143_SERVERSELECT_RESULT_FILE,"r");
   if (fp == NULL)
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;

   memset(&diagstats, 0, sizeof(diagstats));

   while ((read = getline(&line, &len, fp)) != -1)
   {
      int length=0;
      char buf[1024]={0};

      strncpy(buf, line, sizeof(buf)-1);
      length = strlen(buf);
      if (buf[length-1] == '\n')
          buf[length-1] = '\0';

      sscanf(buf, "%s %s %u %u %u %s %d",
             diagstats.state, diagstats.fastestHost,
             &diagstats.minimumTime, &diagstats.maximumTime,
             &diagstats.averageTime, ipAddr, &diagstats.userStop);
   }

   fclose(fp);

   unlink(TR143_SERVERSELECT_RESULT_FILE);
   rut_sendEventMsgToSsk(CMS_MSG_STOP_SERVERSELECTION_DIAG, EID_SERVERSELECTION_DIAG, NULL, 0);

   if (diagstats.state[0] == '\0')
   {
      cmsLog_error("invalid file");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, diagstats.state, mdmLibCtx.allocFlags);

   if (strcmp(diagstats.fastestHost, "NA") != 0)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->fastestHost, diagstats.fastestHost, mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->fastestHost, "", mdmLibCtx.allocFlags);
   }

   if (strcmp(ipAddr, "NA") != 0)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, ipAddr,mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->IPAddressUsed, "", mdmLibCtx.allocFlags);
   }

   obj->averageResponseTime = diagstats.averageTime;
   obj->minimumResponseTime = diagstats.minimumTime;
   obj->maximumResponseTime = diagstats.maximumTime;

   return CMSRET_SUCCESS;
}
#endif /* DMP_DEVICE2_SERVERSELECTIONDIAG_1 */

