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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_qdm.h"
#include "rut2_ipdiag.h"

#if defined(DMP_DEVICE2_DOWNLOAD_1) || defined(DMP_DEVICE2_UPLOAD_1)
/* this header file is installed by userspace/private/libs/tr143_utils */
#include "tr143_defs.h"


static void clearPerConnectionResults(const MdmObjectId oid)
{
   void* perConnResultObj = NULL;
   InstanceIdStack perConnResultIidStack = EMPTY_INSTANCE_ID_STACK;

   // This is a wrapper function to only delete uploadDiag/downloadDiag perconnection results
   if (oid != MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT &&
       oid != MDMOID_DEV2_UPLOAD_PER_CONN_RESULT)
      return;

   /** delete all previous routehops objects. */
   while (cmsObj_getNextFlags(oid, &perConnResultIidStack, OGF_NO_VALUE_UPDATE, (void **)&perConnResultObj) == CMSRET_SUCCESS)
   {
      cmsObj_deleteInstance(oid, &perConnResultIidStack);
      cmsObj_free((void **) &perConnResultObj);
      INIT_INSTANCE_ID_STACK(&perConnResultIidStack);
   }
}

#endif

/*!\file rcl2_ipdiag.c
 * \brief This file contains device 2 device.ip.diagnostics objects related functions.
 *
 */
#ifdef DMP_DEVICE2_IPPING_1
CmsRet rcl_dev2IpDiagnosticsObject( _Dev2IpDiagnosticsObject *newObj __attribute__((unused)),
                                    const _Dev2IpDiagnosticsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2IpPingDiagObject( _Dev2IpPingDiagObject *newObj,
                                 const _Dev2IpPingDiagObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_1024]={0};
   char ifName[CMS_IFNAME_LENGTH]={0};

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if ((mdmLibCtx.eid != EID_SSK) && (mdmLibCtx.eid != EID_DIAG_SSK))
   {
      // Normal apps such as tr69c and httpd can only write requested or
      // none to the diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      // diagnosticsState is still MDMVS_NONE so return CMSRET_SUCCESS
      // but record other params which may have changed.
      else if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_NONE) == 0)
      {
         return CMSRET_SUCCESS;
      }
   }

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      cmsLog_debug("newObj->diagnosticsState %s, repetitions %d, size %d, timeout (sec) %d, host %s, intf=%s",
                   newObj->diagnosticsState,(int)newObj->numberOfRepetitions,
                   newObj->dataBlockSize,newObj->timeout/1000,newObj->host,
                   newObj->interface);

      // Sanity check params.
      if (newObj->host == NULL)
      {
         *errorParam = cmsMem_strdupFlags("Host",mdmLibCtx.allocFlags);
         *errorCode = CMSRET_INVALID_PARAM_VALUE;
         return CMSRET_INVALID_ARGUMENTS;
      }

      // We can only call qdmIntf_fullPathToIntfName in CMS mode or if we
      // are running in BDK sysmgmt.  But in reality, in BDK mode, this code
      // will only run in the diag component.
      if (cmsMdm_isCmsClassic() || cmsMdm_isBdkSysmgmt())
      {
         if (!IS_EMPTY_STRING(newObj->interface))
         {
            if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfname(%s) returns error ret=%d", newObj->interface,ret);
               *errorParam = cmsMem_strdupFlags("Interface",mdmLibCtx.allocFlags);
               *errorCode = CMSRET_INVALID_PARAM_VALUE;
               return CMSRET_INVALID_ARGUMENTS;
            }
         }
      }

      if (!IS_EMPTY_STRING(ifName))
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-I %s ", ifName);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }
      else if (!IS_EMPTY_STRING(newObj->interface))
      {
         // In BDK mode, we cannot convert fullpath to ifName here, so send
         // it to diag_ssk to convert.
         char tmpBuf[1024] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-I %s ", newObj->interface);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (cmsMdm_isBdkDiag())
      {
         // in BDK mode, this code should only run in the diag component,
         // so send responses to diag_ssk.
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-n -d %d ", EID_DIAG_SSK);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (strcmp(newObj->protocolVersion, MDMVS_IPV4) == 0) // -4 for IPv4
      {
         strncat(cmdLine, "-4 ", sizeof(cmdLine)-1);
      }
      else if (strcmp(newObj->protocolVersion, MDMVS_IPV6) == 0) // -6 for IPv6
      {
         strncat(cmdLine, "-6 ", sizeof(cmdLine)-1);
      }

      {
         char tmpBuf[1024] = {0};
         /* ping expects a timeout in second unit.
          * The data model timeout parameter is in ms unit. */
         snprintf(tmpBuf, sizeof(tmpBuf), "-c %d -s %d -W %d -q -m %s",
                  (int)newObj->numberOfRepetitions,
                  (int)newObj->dataBlockSize,
                  (int)(newObj->timeout/1000),
                  newObj->host);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      cmsLog_debug("cmdLine=%s", cmdLine);

      /*
       * At Plugfest 1/15/08, an ACS vendor suggested that when we start
       * a new ping diagnostic, that we should clear the results from
       * the previous diagnostic.
       */
      newObj->successCount = 0;
      newObj->failureCount = 0;
      newObj->averageResponseTime = 0;
      newObj->minimumResponseTime = 0;
      newObj->maximumResponseTime = 0;

      // In CMS, this is sent to ssk.
      // In BDK, this message will get routed to diag_ssk.
      rut_sendEventMsgToSsk(CMS_MSG_START_PING_DIAG, EID_PING, cmdLine, strlen(cmdLine)+1);
   } /* requested: start test */
   else if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_PING_INPROGRESS))
   {
      // If status is not "Requested" and not "Ping_InProgress", then the
      // test is either over by itself or ssk has requested to cancel it.
      // Kill the ping app and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_PING_DIAG, EID_PING, NULL, 0);
   }

   return (ret);
}
#endif /* #ifdef DMP_DEVICE2_IPPING_1 */

#ifdef DMP_DEVICE2_TRACEROUTE_1
CmsRet rcl_dev2IpDiagTraceRouteObject( _Dev2IpDiagTraceRouteObject *newObj,
                                     const _Dev2IpDiagTraceRouteObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_1024] = {0};
   char ifName[CMS_IFNAME_LENGTH]={0};

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if ((mdmLibCtx.eid != EID_SSK) && (mdmLibCtx.eid != EID_DIAG_SSK))
   {
      // Normal apps such as tr69c and httpd can only write requested or
      // none to the diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      // diagnosticsState is still MDMVS_NONE so return CMSRET_SUCCESS
      // but record other params which may have changed.
      else if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_NONE) == 0)
      {
         return CMSRET_SUCCESS;
      }
   }

   /* When traceroute diag is running, it will set intermediate results to
    * this object with diagnosticsState == MDMVS_REQUESTED.  So only start a
    * new traceroute app running when the diagnosticsState goes from
    * not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      cmsLog_debug("newObj->diagnosticsState %s, interface %s maxhops %d, dscp %d, host %s",
                   newObj->diagnosticsState, newObj->interface, (int)newObj->numberOfTries,
                   newObj->DSCP,newObj->host);

      // Sanity check params.
      if (IS_EMPTY_STRING(newObj->host))
      {
         *errorParam = cmsMem_strdupFlags("Host",mdmLibCtx.allocFlags);
         *errorCode = CMSRET_INVALID_PARAM_VALUE;
         return CMSRET_INVALID_ARGUMENTS;
      }

      // We can only call qdmIntf_fullPathToIntfName in CMS mode or if we
      // are running in BDK sysmgmt.  But in reality, in BDK mode, this code
      // will only run in the diag component.
      if (cmsMdm_isCmsClassic() || cmsMdm_isBdkSysmgmt())
      {
         if (!IS_EMPTY_STRING(newObj->interface))
         {
            if (qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfname(%s) returns error ret=%d", newObj->interface,ret);
               *errorParam = cmsMem_strdupFlags("Interface",mdmLibCtx.allocFlags);
               *errorCode = CMSRET_INVALID_PARAM_VALUE;
               return CMSRET_INVALID_ARGUMENTS;
            }
         }
      }

      if (!IS_EMPTY_STRING(ifName))
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", ifName);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }
      else if (!IS_EMPTY_STRING(newObj->interface))
      {
         // In BDK mode, we cannot convert fullpath to ifName here, so send
         // it to diag_ssk to convert.
         char tmpBuf[1024] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", newObj->interface);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (cmsMdm_isBdkDiag())
      {
         // in BDK mode, this code should only run in the diag component,
         // so send responses to diag_ssk.
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-N -R %d ", EID_DIAG_SSK);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      {
         char tmpBuf[1024] = {0};
         unsigned int timeout;
         // convert timeout milliseconds to seconds.
         timeout = (newObj->timeout > 1000)? newObj->timeout/1000 : 1;

         snprintf(tmpBuf, sizeof(tmpBuf), "-M -w %u -q %u -t %u -m %d %s %u",
                  timeout,                // -w waittime/timeout
                  newObj->numberOfTries,  // -q (number of probes)
                  (newObj->DSCP * 2),     // -t (tos)
                  newObj->maxHopCount,    // -m (max hop count, ttl)
                  newObj->host, newObj->dataBlockSize);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      cmsLog_debug("cmdLine=%s", cmdLine);

      /*
       * Before we start a new traceroute diagnostic,
       * that we should clear the results from
       * the previous diagnostic (including all routehops)
       */
      {
         Dev2IpDiagTraceRouteRouteHopsObject *routeHopsObj = NULL;
         InstanceIdStack routeHopsIidStack = EMPTY_INSTANCE_ID_STACK;

         while (cmsObj_getNextFlags(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &routeHopsIidStack, 0, (void **)&routeHopsObj) == CMSRET_SUCCESS)
         {
            cmsObj_deleteInstance(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &routeHopsIidStack);
            cmsObj_free((void **) &routeHopsObj);
            INIT_INSTANCE_ID_STACK(&routeHopsIidStack);
         }
      }

      newObj->routeHopsNumberOfEntries = 0;
      newObj->responseTime = 0;

      // In CMS, this is sent to ssk.
      // In BDK, this message will get routed to diag_ssk.
      rut_sendEventMsgToSsk(CMS_MSG_START_TRACERT_DIAG, EID_TRACERT, cmdLine, strlen(cmdLine)+1);
   } /* requested: start test*/
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the traceroute app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_TRACERT_DIAG, EID_TRACERT, NULL, 0);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2IpDiagTraceRouteRouteHopsObject( _Dev2IpDiagTraceRouteRouteHopsObject *newObj,
                                     const _Dev2IpDiagTraceRouteRouteHopsObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagTraceRouteObject *tracertObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   if (ADD_NEW(newObj, currObj))
   {
      // Update the routeHopsNumberOfEntries in the parent object.
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE, &accentIidStack, flags, (void **) &tracertObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagTraceRoute object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      tracertObj->routeHopsNumberOfEntries++;

      // We are only updating the numberOfEntries, so don't go into RCL
      // handler function.
      if ((ret = cmsObj_setFlags(tracertObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagTraceRoute object, ret=%d", ret);
         cmsObj_free((void **) &tracertObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &tracertObj);
   }
   return CMSRET_SUCCESS;
};

#endif /* DMP_DEVICE2_TRACEROUTE_1 */

#ifdef DMP_DEVICE2_UPLOAD_1
CmsRet rcl_dev2IpDiagUploadObject( _Dev2IpDiagUploadObject *newObj,
                                   const _Dev2IpDiagUploadObject *currObj,
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
   char cmdLine[BUFLEN_1024]={0};
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_notice("Entered:");

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if ((mdmLibCtx.eid != EID_SSK) &&
       (mdmLibCtx.eid != EID_DIAG_SSK) &&
       (mdmLibCtx.eid != EID_UPLOAD_DIAG))
   {
      // Normal apps such as tr69c and httpd can only write requested to
      // diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
   }

   /** Whenever the testing is in progress or not,
    *  modifying any of the writable parameters except diagnosticsState,
    *  MUST result in the value of diagnosticsState being set to None.
    */
   if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) == 0)
   {
      // If any diagnostics parameter has been changed, Reset diagnosticsState and ressults
      if (cmsUtl_strcmp(currObj->interface, newObj->interface) ||
          cmsUtl_strcmp(currObj->uploadURL, newObj->uploadURL) ||
          (currObj->DSCP != newObj->DSCP) ||
          (currObj->numberOfConnections != newObj->numberOfConnections) ||
          (currObj->testFileLength != newObj->testFileLength) ||
          (currObj->ethernetPriority != newObj->ethernetPriority) ||
          (currObj->enablePerConnectionResults != newObj->enablePerConnectionResults))
      {
         //reset all statistics
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->ROMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->BOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->EOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         newObj->totalBytesSent = 0;
         newObj->totalBytesReceived = 0;
         newObj->testBytesSent = 0;
#ifdef DMP_DEVICE2_UPLOADTCP_1
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->TCPOpenRequestTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->TCPOpenResponseTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
#endif /* DMP_DEVICE2_UPLOADTCP_1 */

         clearPerConnectionResults(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT);
         newObj->perConnectionResultNumberOfEntries = 0;

         // The test is in progress, stop it.
         if (!strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
         {
            cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
            rut_sendEventMsgToSsk(CMS_MSG_STOP_UPLOAD_DIAG, EID_UPLOAD_DIAG, NULL, 0);
         }

         // reset diagnosticsState to None
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
   }

   /* When upload diag is running, it will set intermediate results to
    * this object with diagnosticsState == MDMVS_REQUESTED.  So only start a
    * new upload diag app running when the diagnosticsState goes from
    * not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      // We can only call qdmIntf_fullPathToIntfName in CMS mode or if we
      // are running in BDK sysmgmt.  But in reality, in BDK mode, this code
      // will only run in the diag component.
      if (cmsMdm_isCmsClassic() || cmsMdm_isBdkSysmgmt())
      {
         if (!IS_EMPTY_STRING(newObj->interface))
         {
            CmsRet r2;
            if ((r2 = qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfname(%s) error ret=%d",
                            newObj->interface, r2);
               // Set errorParam?  See ping code above.
               return CMSRET_INVALID_PARAM_VALUE;
            }
         }
      }

      if (IS_EMPTY_STRING(newObj->uploadURL))
      {
         cmsLog_error("uploadURL is empty");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if (newObj->testFileLength == 0)
      {
         cmsLog_error("testFileLength is zero");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if ((newObj->enablePerConnectionResults == TRUE) &&
          (newObj->numberOfConnections > newObj->uploadDiagnosticsMaxConnections))
      {
         cmsLog_error("numberOfConnections(%u) must not set greater than MaxConnections(%u)",
                      newObj->numberOfConnections, newObj->uploadDiagnosticsMaxConnections);
         return CMSRET_INVALID_ARGUMENTS;
      }

      // delete previous test results
      unlink(TR143_UPLOAD_RESULT_FILE);

      /** delete all previous routehops objects. */
      clearPerConnectionResults(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT);
      newObj->perConnectionResultNumberOfEntries= 0;

      // Put together the command line
      if (!IS_EMPTY_STRING(ifName))
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", ifName);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }
      else if (!IS_EMPTY_STRING(newObj->interface))
      {
         // In BDK mode, we cannot convert fullpath to ifName here, so send
         // it to diag_ssk to convert.
         char tmpBuf[1024] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", newObj->interface);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (cmsMdm_isBdkDiag())
      {
         // in BDK mode, this code should only run in the diag component,
         // so send responses to diag_ssk.
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-N ");
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (newObj->enablePerConnectionResults == TRUE) 
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-c %d ", newObj->numberOfConnections);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      {
         char tmpBuf[1024] = {0};
         int loglevel = newObj->X_BROADCOM_COM_LogLevel;
         if (loglevel != LOG_LEVEL_ERR && loglevel != LOG_LEVEL_NOTICE && loglevel != LOG_LEVEL_DEBUG)
            loglevel = DEFAULT_LOG_LEVEL;

         snprintf(tmpBuf, sizeof(tmpBuf), "-m -d %d -l %u -u %s -D %d",
                  newObj->DSCP, newObj->testFileLength,
                  newObj->uploadURL, loglevel);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
#ifdef BRCM_SPDTEST
         strncat(cmdLine, " -H", sizeof(cmdLine)-1);
#endif
      }

      cmsLog_debug("cmdLine=%s", cmdLine);

      rut_sendEventMsgToSsk(CMS_MSG_START_UPLOAD_DIAG, EID_UPLOAD_DIAG, cmdLine, strlen(cmdLine)+1);
   }
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the upload diag app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_UPLOAD_DIAG, EID_UPLOAD_DIAG, NULL, 0);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2UploadPerConnResultObject( _Dev2UploadPerConnResultObject *newObj,
                                          const _Dev2UploadPerConnResultObject *currObj,
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagUploadObject *uploadObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &accentIidStack, flags, (void **) &uploadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      uploadObj->perConnectionResultNumberOfEntries++;

      if ((ret = cmsObj_setFlags(uploadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagUpload object, ret=%d", ret);
         cmsObj_free((void **) &uploadObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &uploadObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &accentIidStack, flags, (void **) &uploadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      uploadObj->perConnectionResultNumberOfEntries--;

      if ((ret = cmsObj_setFlags(uploadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagUpload object, ret=%d", ret);
         cmsObj_free((void **) &uploadObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &uploadObj);
   }

   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_UPLOAD_1 */

#ifdef DMP_DEVICE2_DOWNLOAD_1
CmsRet rcl_dev2IpDiagDownloadObject( _Dev2IpDiagDownloadObject *newObj,
                                     const _Dev2IpDiagDownloadObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   char cmdLine[BUFLEN_1024]={0};
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_notice("Entered:");

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if ((mdmLibCtx.eid != EID_SSK) &&
       (mdmLibCtx.eid != EID_DIAG_SSK) &&
       (mdmLibCtx.eid != EID_DOWNLOAD_DIAG))
   {
      // Normal apps such as tr69c and httpd can only write requested to
      // diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
   }

   /** Whenever the testing is in progress or not,
    *  modifying any of the writable parameters except diagnosticsState,
    *  MUST result in the value of diagnosticsState being set to None.
    */
   if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) == 0)
   {
      // If any diagnostics parameter has been changed, Reset diagnosticsState and ressults
      if (cmsUtl_strcmp(currObj->interface, newObj->interface) ||
          cmsUtl_strcmp(currObj->downloadURL, newObj->downloadURL) ||
          (currObj->DSCP != newObj->DSCP) ||
          (currObj->numberOfConnections != newObj->numberOfConnections) ||
          (currObj->ethernetPriority != newObj->ethernetPriority) ||
          (currObj->enablePerConnectionResults != newObj->enablePerConnectionResults))
      {
         cmsLog_notice("reset all statistics and perConnectionResult(s)");
         //reset all statistics
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->ROMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->BOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->EOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         newObj->totalBytesSent = 0;
         newObj->totalBytesReceived = 0;
         newObj->testBytesReceived = 0;
#ifdef DMP_DEVICE2_DOWNLOADTCP_1
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->TCPOpenRequestTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->TCPOpenResponseTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
#endif /* DMP_DEVICE2_UPLOADTCP_1 */

         clearPerConnectionResults(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT);
         newObj->perConnectionResultNumberOfEntries = 0;

         if (!cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED))
         {
            cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
            rut_sendEventMsgToSsk(CMS_MSG_STOP_DOWNLOAD_DIAG, EID_DOWNLOAD_DIAG, NULL, 0);
         }

         // reset diagnosticsState to None
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
   }

   /* When download diag is running, it will set intermediate results to
    * this object with diagnosticsState == MDMVS_REQUESTED.  So only start a
    * new download diag app running when the diagnosticsState goes from
    * not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      // We can only call qdmIntf_fullPathToIntfName in CMS mode or if we
      // are running in BDK sysmgmt.  But in reality, in BDK mode, this code
      // will only run in the diag component.
      if (cmsMdm_isCmsClassic() || cmsMdm_isBdkSysmgmt())
      {
         if (!IS_EMPTY_STRING(newObj->interface))
         {
            CmsRet r2;
            if ((r2 = qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfname(%s) error ret=%d",
                            newObj->interface, r2);
               // Set errorParam?  See ping code above.
               return CMSRET_INVALID_PARAM_VALUE;
            }
         }
      }

      if (IS_EMPTY_STRING(newObj->downloadURL))
      {
         cmsLog_error("downloadURL is empty");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if ((newObj->enablePerConnectionResults == TRUE) &&
          (newObj->numberOfConnections > newObj->downloadDiagnosticMaxConnections))
      {
         cmsLog_error("numberOfConnections(%u) must not set greater than MaxConnections(%u)",
                      newObj->numberOfConnections, newObj->downloadDiagnosticMaxConnections);
         return CMSRET_INVALID_ARGUMENTS;
      }

      // delete previous test results
      unlink(TR143_DOWNLOAD_RESULT_FILE);

      /** delete all previous routehops objects. */
      clearPerConnectionResults(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT);
      newObj->perConnectionResultNumberOfEntries= 0;

      // Put together the command line
      if (!IS_EMPTY_STRING(ifName))
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", ifName);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }
      else if (!IS_EMPTY_STRING(newObj->interface))
      {
         // In BDK mode, we cannot convert fullpath to ifName here, so send
         // it to diag_ssk to convert.
         char tmpBuf[1024] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-i %s ", newObj->interface);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (cmsMdm_isBdkDiag())
      {
         // in BDK mode, this code should only run in the diag component,
         // so send responses to diag_ssk.
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-N ");
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      if (newObj->enablePerConnectionResults == TRUE) 
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-c %d ", newObj->numberOfConnections);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      {
         char tmpBuf[1024] = {0};
         int loglevel = newObj->X_BROADCOM_COM_LogLevel;
         if (loglevel != LOG_LEVEL_ERR && loglevel != LOG_LEVEL_NOTICE && loglevel != LOG_LEVEL_DEBUG)
            loglevel = DEFAULT_LOG_LEVEL;

         // Add '-Z' option for zero-copy / TCP_DISCARD feature
         snprintf(tmpBuf, sizeof(tmpBuf), "-m -d %d -u %s -D %d -Z",
                  newObj->DSCP, newObj->downloadURL, loglevel);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
#ifdef BRCM_SPDTEST
         strncat(cmdLine, " -H", sizeof(cmdLine)-1);
#endif
      }

      cmsLog_debug("cmdLine=%s", cmdLine);

      rut_sendEventMsgToSsk(CMS_MSG_START_DOWNLOAD_DIAG, EID_DOWNLOAD_DIAG, cmdLine, strlen(cmdLine)+1);
   }
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the downaloddiag app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_DOWNLOAD_DIAG, EID_DOWNLOAD_DIAG, NULL, 0);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DownloadPerConnResultObject( _Dev2DownloadPerConnResultObject *newObj,
                                            const _Dev2DownloadPerConnResultObject *currObj,
                                            const InstanceIdStack *iidStack __attribute__((unused)),
                                            char **errorParam __attribute__((unused)),
                                            CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagDownloadObject *downloadObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &accentIidStack, flags, (void **) &downloadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      downloadObj->perConnectionResultNumberOfEntries++;

      if ((ret = cmsObj_setFlags(downloadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagDownload object, ret=%d", ret);
         cmsObj_free((void **) &downloadObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &downloadObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &accentIidStack, flags, (void **) &downloadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      downloadObj->perConnectionResultNumberOfEntries--;

      if ((ret = cmsObj_setFlags(downloadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagDownload object, ret=%d", ret);
         cmsObj_free((void **) &downloadObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &downloadObj);
   }

   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_DOWNLOAD_1 */


#ifdef DMP_DEVICE2_UDPECHO_1
CmsRet rcl_dev2IpDiagUDPEchoConfigObject( _Dev2IpDiagUDPEchoConfigObject *newObj,
                                          const _Dev2IpDiagUDPEchoConfigObject *currObj,
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
   char buf[BUFLEN_512]={0};

   // Udpecho server writes stats to this object, but no action is needed.
   if (mdmLibCtx.eid == EID_UDPECHO)
      return CMSRET_SUCCESS;

   // DiagnosticsState could have been saved as enabled to config file, so
   // we need to start udpecho server at startup.  Or, it was enabled during
   // runtime.
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      //reset all statistics
      CMSMEM_REPLACE_STRING_FLAGS(newObj->timeFirstPacketReceived, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(newObj->timeLastPacketReceived, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
      newObj->bytesReceived = 0;
      newObj->bytesResponded = 0;
      newObj->packetsReceived = 0;
      newObj->packetsResponded = 0;
      newObj->X_BROADCOM_COM_PacketsRespondedFail = 0;

      // Because it is so inconvenient to translate fullpath to intf name
      // in BDK, just open the firewall hole on all interfaces.
      sprintf(buf, "iptables -w -A INPUT -p udp --dport %d -j ACCEPT", newObj->UDPPort);
      rut_doSystemAction("UDPEchoCfg", buf);

      cmsLog_notice("open firewall hole and starting UDP echo service");
      if (cmsMdm_isBdkDiag())
      {
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-N");
         rut_sendEventMsgToSsk(CMS_MSG_START_UDPECHO, EID_UDPECHO, tmpBuf, strlen(tmpBuf)+1);
      }
      else
      {
         rut_sendEventMsgToSsk(CMS_MSG_START_UDPECHO, EID_UDPECHO, NULL, 0);
      }
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_notice("delete firewall rule and stopping UDP echo service");

      sprintf(buf, "iptables -w -D INPUT -p udp --dport %d -j ACCEPT", currObj->UDPPort);
      rut_doSystemAction("UDPEchoCfg", buf);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_UDPECHO, EID_UDPECHO, NULL, 0);
   }

   return CMSRET_SUCCESS;
}
#endif /* DMP_DEVICE2_UDPECHO_1 */

#ifdef DMP_DEVICE2_UDPECHODIAG_1
static void updateErrorResult(void)
{
   FILE *fp=NULL;
   char line[1026]={0};

   fp = fopen(TR143_UDPECHO_RESULT_FILE, "w");

   if (fp)
   {
      snprintf(line, 1026, "%s NA 0 0 0 0 0", MDMVS_ERROR_OTHER);
      fputs(line, fp);
      fclose(fp);
   }
   else
   {
      cmsLog_error("cannot open file<%s>", TR143_UDPECHO_RESULT_FILE);
   }
}

CmsRet rcl_dev2UDPEchoDiagObject( _Dev2UDPEchoDiagObject *newObj,
                                  const _Dev2UDPEchoDiagObject *currObj,
                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
   char cmdLine[BUFLEN_1024]={0};
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Entered:");

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if ((mdmLibCtx.eid != EID_SSK) &&
       (mdmLibCtx.eid != EID_DIAG_SSK) &&
       (mdmLibCtx.eid != EID_UDPECHO_DIAG))
   {
      // Normal apps such as tr69c and httpd can only write requested to
      // diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            ret = CMSRET_INVALID_PARAM_VALUE;
            goto out;
         }
      }
   }

   if (strcmp(newObj->protocolVersion, MDMVS_IPV6) == 0)
   {
      cmsLog_error("IPv6 is not supported now");
      ret = CMSRET_INVALID_PARAM_VALUE;
      goto out;
   }

   /** Whenever the testing is in progress or not,
    *  modifying any of the writable parameters except diagnosticsState,
    *  MUST result in the value of diagnosticsState being set to None.
    */
   if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) == 0)
   {
      // If any diagnostics parameter has been changed, Reset diagnosticsState and results
      if (cmsUtl_strcmp(currObj->host, newObj->host) ||
          (currObj->port != newObj->port) ||
          (currObj->numberOfRepetitions != newObj->numberOfRepetitions) ||
          (currObj->timeout != newObj->timeout) ||
          (currObj->interTransmissionTime != newObj->interTransmissionTime) ||
          (currObj->enableIndividualPacketResults !=
                                     newObj->enableIndividualPacketResults) ||
          (currObj->dataBlockSize != newObj->dataBlockSize))
      {
         cmsLog_notice("reset all statistics and perConnectionResult(s)");
         //reset all statistics
         newObj->successCount = 0;
         newObj->failureCount = 0;
         newObj->averageResponseTime = 0;
         newObj->minimumResponseTime = 0;
         newObj->maximumResponseTime = 0;

         if (!cmsUtl_strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
         {
            cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
            rut_sendEventMsgToSsk(CMS_MSG_STOP_UDPECHO_DIAG, EID_UDPECHO_DIAG,
                                  NULL, 0);
         }

         // reset diagnosticsState to None
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_NONE,
                                           mdmLibCtx.allocFlags);
      }
   }

   /* When udpecho diag is running, it will set intermediate results to
    * this object with diagnosticsState == MDMVS_REQUESTED.  So only start a
    * new download diag app running when the diagnosticsState goes from
    * not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      if (IS_EMPTY_STRING(newObj->host))
      {
         cmsLog_error("host is empty");
         ret = CMSRET_INVALID_PARAM_VALUE;
         goto out;
      }

      if (newObj->port < 1)
      {
         cmsLog_error("invalid port number");
         ret = CMSRET_INVALID_PARAM_VALUE;
         goto out;
      }

      if (newObj->timeout < 1)
      {
         cmsLog_error("invalid timeout value");
         ret = CMSRET_INVALID_PARAM_VALUE;
         goto out;
      }

      // delete previous test results
      unlink(TR143_UDPECHO_RESULT_FILE);

      if (cmsMdm_isBdkDiag())
      {
         char triesStr[32] = {0};
         char waitTimeStr[32] = {0};

         if (newObj->numberOfRepetitions > 0)
         {
            snprintf(triesStr, sizeof(triesStr), "-n %d",
                     newObj->numberOfRepetitions);
         }

         if (newObj->interTransmissionTime > 0)
         {
            snprintf(waitTimeStr, sizeof(waitTimeStr), "-i %d",
                     newObj->interTransmissionTime);
         }

         snprintf(cmdLine, sizeof(cmdLine), "-N -p %d -t %d %s %s -s %s ",
                  newObj->port, newObj->timeout, triesStr, waitTimeStr,
                  newObj->host);
      }

      cmsLog_debug("cmdLine=%s", cmdLine);

      rut_sendEventMsgToSsk(CMS_MSG_START_UDPECHO_DIAG, EID_UDPECHO_DIAG,
                            cmdLine, strlen(cmdLine)+1);
   }
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the udpechodiag app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_UDPECHO_DIAG, EID_UDPECHO_DIAG,
                            NULL, 0);
   }

out:
   if (ret != CMSRET_SUCCESS)
   {
      updateErrorResult();
   }

   return ret;
}
#endif

#ifdef DMP_DEVICE2_IPLAYERCAPACITYTEST_1
CmsRet rcl_dev2IpDiagIPCapObject( _Dev2IpDiagIPCapObject *newObj,
                const _Dev2IpDiagIPCapObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagIPCapIncResObject *incResObj = NULL;
   InstanceIdStack incResIidStack = EMPTY_INSTANCE_ID_STACK;
   char cmdLine[BUFLEN_1024]={0};
   char bimodalBuf[16]={0};
   int startSendingRate = 0;
   
   cmsLog_notice("Entered:");

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if ((mdmLibCtx.eid != EID_SSK) &&
       (mdmLibCtx.eid != EID_DIAG_SSK))
   {
      // Normal apps such as tr69c and httpd can only write requested to
      // diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
   }

   /** Whenever the testing is in progress or not,
    *  modifying any of the writable parameters except diagnosticsState,
    *  MUST result in the value of diagnosticsState being set to None.
    */
   if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) == 0)
   {
      // If any diagnostics parameter has been changed, Reset diagnosticsState and ressults
      if (cmsUtl_strcmp(currObj->role, newObj->role) ||
          cmsUtl_strcmp(currObj->host, newObj->host) ||
          (currObj->port != newObj->port) ||
          (currObj->jumboFramesPermitted != newObj->jumboFramesPermitted) ||
          (currObj->DSCP != newObj->DSCP) ||
          cmsUtl_strcmp(currObj->testType, newObj->testType) ||
          (currObj->IPDVEnable != newObj->IPDVEnable) ||
          (currObj->testSubInterval != newObj->testSubInterval) ||
          (currObj->statusFeedbackInterval != newObj->statusFeedbackInterval) ||
          (currObj->lowerThresh != newObj->lowerThresh) ||
          (currObj->upperThresh != newObj->upperThresh) ||
          (currObj->highSpeedDelta != newObj->highSpeedDelta) ||
          (currObj->slowAdjThresh != newObj->slowAdjThresh))
      {
         //reset all statistics
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->BOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->EOMTime, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->timeOfMax, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->maxIPLayerCapacity, "0", mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->lossRatioAtMax, "0", mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->RTTRangeAtMax, "0", mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->PDVRangeAtMax, "0", mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->minOnewayDelayAtMax, "0", mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->reorderedRatioAtMax, "0", mdmLibCtx.allocFlags);
         newObj->tmaxUsed = newObj->testInterval = newObj->incrementalResultNumberOfEntries = newObj->tmaxRTTUsed  = newObj->timestampResolutionUsed = 0;

         // The test is in progress, stop it.
         if (!strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
         {
            cmsLog_notice("state=%s (stop udpst)", newObj->diagnosticsState);
            rut_sendEventMsgToSsk(CMS_MSG_STOP_OBUDPST, EID_OBUDPST, NULL, 0);
         }

         // reset diagnosticsState to None
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
   }

   /* only start a new upload diag app running when the diagnosticsState goes from not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      if (IS_EMPTY_STRING(newObj->host))
      {
         cmsLog_error("host is empty");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      // delete previous test results
      unlink(UDPST_DIAG_RESULT_FILE);

      /** delete all previous result objects. */
      while (cmsObj_getNextFlags(MDMOID_IP_CAP_INC_RES, &incResIidStack, OGF_NO_VALUE_UPDATE, (void **)&incResObj) == CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_IP_CAP_INC_RES, &incResIidStack);
         cmsObj_free((void **) &incResObj);
         INIT_INSTANCE_ID_STACK(&incResIidStack);
      }
      newObj->incrementalResultNumberOfEntries = 0;
         
      if (newObj->numberFirstModeTestSubIntervals)
      {
         snprintf(bimodalBuf, sizeof(bimodalBuf), "-i %d", newObj->numberFirstModeTestSubIntervals);
      }
      startSendingRate = newObj->startSendingRate > 1000 ? (1000 + ((newObj->startSendingRate - 1000) / 100)) : (newObj->startSendingRate / 1000);
      snprintf(cmdLine, sizeof(cmdLine), "%s %s -m %d -p %d %s %s -P %d -t %d -F %d -L %d -U %d -h %d -c %d -q %d -f %s -I %s%d %s %s %s %s %s",
               cmsUtl_strcmp(newObj->role,MDMVS_RECEIVER)?"-u":"-d", 
               newObj->jumboFramesPermitted?"":"-j",
               newObj->DSCP,  //m
               newObj->port,  //p
               newObj->IPDVEnable?"-o":"",
               newObj->IPRREnable?"":"-R",
               newObj->testSubInterval/1000,  //P
               newObj->testSubInterval/1000*newObj->numberTestSubIntervals, //t
               newObj->statusFeedbackInterval,  //F
               newObj->lowerThresh, //L
               newObj->upperThresh, //U
               newObj->highSpeedDelta,  //h
               newObj->slowAdjThresh,  //c
               newObj->seqErrThresh,   //q
               UDPST_DIAG_RESULT_FILE, //f
               cmsUtl_strcmp(newObj->testType, MDMVS_SEARCH)?"":"@", startSendingRate,  //I
               cmsUtl_strcmp(newObj->protocolVersion, MDMVS_ANY)?(cmsUtl_strcmp(newObj->protocolVersion, MDMVS_IPV4)?"-6":"-4"):"", 
               cmsUtl_strcmp(newObj->UDPPayloadContent, "random")?"":"-X", 
               bimodalBuf,
               newObj->host,
               cmsMdm_isBdkDiag()?"fromBDK":"fromCMS");

      cmsLog_debug("cmdLine=%s", cmdLine);

      rut_sendEventMsgToSsk(CMS_MSG_START_OBUDPST, EID_OBUDPST, cmdLine, strlen(cmdLine)+1);
   }
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the udpst app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_OBUDPST, EID_OBUDPST, NULL, 0);
   }

   return CMSRET_SUCCESS;
}                


CmsRet rcl_dev2IpDiagIPCapModResObject( _Dev2IpDiagIPCapModResObject *newObj,
                const _Dev2IpDiagIPCapModResObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagIPCapObject *ipCapObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_IP_CAP, &accentIidStack, flags, (void **) &ipCapObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get ipCapObj object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      ipCapObj->modalResultNumberOfEntries++;

      if ((ret = cmsObj_setFlags(ipCapObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set ipCapObj object, ret=%d", ret);
         cmsObj_free((void **) &ipCapObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &ipCapObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_IP_CAP, &accentIidStack, flags, (void **) &ipCapObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get ipCapObj object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      ipCapObj->modalResultNumberOfEntries--;

      if ((ret = cmsObj_setFlags(ipCapObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set ipCapObj object, ret=%d", ret);
         cmsObj_free((void **) &ipCapObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &ipCapObj);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2IpDiagIPCapIncResObject( _Dev2IpDiagIPCapIncResObject *newObj,
                const _Dev2IpDiagIPCapIncResObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagIPCapObject *ipCapObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_IP_CAP, &accentIidStack, flags, (void **) &ipCapObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get ipCapObj object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      ipCapObj->incrementalResultNumberOfEntries++;

      if ((ret = cmsObj_setFlags(ipCapObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set ipCapObj object, ret=%d", ret);
         cmsObj_free((void **) &ipCapObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &ipCapObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_IP_CAP, &accentIidStack, flags, (void **) &ipCapObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get ipCapObj object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      ipCapObj->incrementalResultNumberOfEntries--;

      if ((ret = cmsObj_setFlags(ipCapObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set ipCapObj object, ret=%d", ret);
         cmsObj_free((void **) &ipCapObj);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &ipCapObj);
   }

   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_DEVICE2_SERVERSELECTIONDIAG_1
CmsRet rcl_dev2IpDiagServerSelectionObject( _Dev2IpDiagServerSelectionObject *newObj,
                                            const _Dev2IpDiagServerSelectionObject *currObj,
                                            const InstanceIdStack *iidStack __attribute__((unused)),
                                            char **errorParam __attribute__((unused)),
                                            CmsRet *errorCode __attribute__((unused)))
{
   char cmdLine[BUFLEN_1024]={0};
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Entered:");

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if ((mdmLibCtx.eid != EID_SSK) &&
       (mdmLibCtx.eid != EID_DIAG_SSK) &&
       (mdmLibCtx.eid != EID_SERVERSELECTION_DIAG))
   {
      // Normal apps such as tr69c and httpd can only write requested to
      // diagnosticsState.
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_error("Mgmt apps may only write requested to this object");
            ret = CMSRET_INVALID_PARAM_VALUE;
            goto out;
         }
      }
   }

   /** Whenever the testing is in progress or not,
    *  modifying any of the writable parameters except diagnosticsState,
    *  MUST result in the value of diagnosticsState being set to None.
    */
   if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) == 0)
   {
      // If any diagnostics parameter has been changed, Reset diagnosticsState and results
      if (cmsUtl_strcmp(currObj->hostList, newObj->hostList) ||
          cmsUtl_strcmp(currObj->interface, newObj->interface) ||
          cmsUtl_strcmp(currObj->protocolVersion, newObj->protocolVersion) ||
          cmsUtl_strcmp(currObj->protocol, newObj->protocol) ||
          (currObj->numberOfRepetitions != newObj->numberOfRepetitions) ||
          (currObj->timeout != newObj->timeout))
      {
         cmsLog_notice("reset all statistics and perConnectionResult(s)");
         //reset all statistics
         newObj->averageResponseTime = 0;
         newObj->minimumResponseTime = 0;
         newObj->maximumResponseTime = 0;

         if (!cmsUtl_strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
         {
            cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
            rut_sendEventMsgToSsk(CMS_MSG_STOP_SERVERSELECTION_DIAG, EID_SERVERSELECTION_DIAG,
                                  NULL, 0);
         }

         // reset diagnosticsState to None
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_NONE,
                                           mdmLibCtx.allocFlags);
      }
   }

   /* When serverselect diag is running, it will set intermediate results to
    * this object with diagnosticsState == MDMVS_REQUESTED.  So only start a
    * new download diag app running when the diagnosticsState goes from
    * not-Requested to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      if (IS_EMPTY_STRING(newObj->hostList))
      {
         cmsLog_error("hostList is empty");
         ret = CMSRET_INVALID_PARAM_VALUE;
         goto out;
      }

      if (newObj->timeout < 1)
      {
         cmsLog_error("invalid timeout value");
         ret = CMSRET_INVALID_PARAM_VALUE;
         goto out;
      }

      // delete previous test results
      unlink(TR143_SERVERSELECT_RESULT_FILE);

      if (cmsMdm_isBdkDiag())
      {
         // in BDK mode, this code should only run in the diag component,
         // so send responses to diag_ssk.
         char tmpBuf[128] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-N ");
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }

      {
         char tmpBuf[1024] = {0};
         snprintf(tmpBuf, sizeof(tmpBuf), "-c %d -t %d -P %s -S %s",
                  (int)newObj->numberOfRepetitions,
                  (int)newObj->timeout,
                  newObj->protocol,
                  newObj->hostList);
         strncat(cmdLine, tmpBuf, sizeof(cmdLine)-1);
      }
      cmsLog_debug("cmdLine=%s", cmdLine);

      rut_sendEventMsgToSsk(CMS_MSG_START_SERVERSELECTION_DIAG, EID_SERVERSELECTION_DIAG,
                            cmdLine, strlen(cmdLine)+1);
   }
   else if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0) &&
            (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) == 0))
   {
      // Went from Requested to not Requested, means test is complete.
      // Kill the serverselect app (just in case), and collect the child.
      cmsLog_notice("state=%s (stop app)", newObj->diagnosticsState);
      rut_sendEventMsgToSsk(CMS_MSG_STOP_SERVERSELECTION_DIAG, EID_SERVERSELECTION_DIAG,
                            NULL, 0);
   }

out:
   if (ret != CMSRET_SUCCESS)
   {
      FILE *fp=NULL;
      char line[1026]={0};

      fp = fopen(TR143_SERVERSELECT_RESULT_FILE, "w");

      if (fp)
      {
         snprintf(line, 1026, "%s NA 0 0 0 NA 0", MDMVS_ERROR_OTHER);
         fputs(line, fp);
         fclose(fp);
      }
      else
      {
         cmsLog_error("cannot open file<%s>", TR143_SERVERSELECT_RESULT_FILE);
      }
   }

   return ret;
}
#endif /* DMP_DEVICE2_SERVERSELECTIONDIAG_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
