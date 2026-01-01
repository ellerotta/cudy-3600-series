#ifdef DMP_X_BROADCOM_COM_SPDSVC_1
/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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

#include "stl.h"
#include "cms_util.h"
#include "rut_spdsvc.h"

CmsRet stl_speedServiceObject(_SpeedServiceObject *obj,
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   char status[BUFLEN_32] = {0};
   char runTime[BUFLEN_64] = {0};
   char dir[BUFLEN_32] = {0};
   UINT32 goodPut = 0;
   UINT32 packetLoss = 0;
   UINT32 avgLatency = 0;
   UINT32 payloadRate = 0;
   UINT32 receivedRate = 0;
   UINT32 receivedTime = 0;
   UINT32 overhead = 0;
   FILE *fp = NULL;
   int rc;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   // This code was originally in processSpeedserviceComplete in ssk.c

   // If file is not there, then there is no data to update.
   fp = fopen(SPDSVC_RESULT_FILE,"r");
   if (fp == NULL)
   {
      return ret;
   }

   rc = fscanf(fp, "%s %s %u %u %u %u %u %u %u %s",
               status, runTime, &goodPut, &payloadRate, &packetLoss, &avgLatency,
               &receivedRate, &receivedTime, &overhead, dir);
   fclose(fp);

   if (rc < 1)
   {
      cmsLog_error("fscanf returned %d, expected 1", rc);
      return ret;
   }

   // Put data into MDM
   CMSMEM_REPLACE_STRING_FLAGS(obj->diagnosticsState, status, mdmLibCtx.allocFlags);
   if (strcmp(status, MDMVS_COMPLETED) == 0)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastRunTime, runTime, mdmLibCtx.allocFlags);
      obj->goodPut = goodPut;
      obj->payloadRate = payloadRate;
      obj->packetLoss = packetLoss;
      obj->avgLatency = avgLatency;
      obj->adjustReceivedRate = receivedRate;
      obj->receivedTime = receivedTime;
      obj->overhead = overhead;
   }

   // Now that we have transfered data from file to MDM, delete it.
   unlink(SPDSVC_RESULT_FILE);

   return CMSRET_SUCCESS;
}

CmsRet stl_resultHistoryObject(_ResultHistoryObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_SPDSVC_1 */
