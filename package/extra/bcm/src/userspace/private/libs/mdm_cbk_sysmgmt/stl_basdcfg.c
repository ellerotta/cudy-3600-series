/***********************************************************************
 *
 *
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
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
#ifdef DMP_X_BROADCOM_COM_BASD_1

#include "time.h"
#include "stl.h"
#include "cms_util.h"
#include "rut_util.h"

#include "rut_basdcfg.h"

static CmsRet getXSIDateTimeFromTime_t(time_t *time, char *buf, UINT32 bufLen)
{
   int          c;
   struct tm   *tmp;

   tmp = localtime(time);
   memset(buf, 0, bufLen);
   c = strftime(buf, bufLen, "%Y-%m-%dT%H:%M:%S%z", tmp);
   if ((c == 0) || (c+1 > (int) bufLen))
   {
      /* buf was not long enough */
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* fix missing : in time-zone offset-- change -500 to -5:00 */
   buf[c+1] = '\0';
   buf[c] = buf[c-1];
   buf[c-1] = buf[c-2];
   buf[c-2]=':';

   return CMSRET_SUCCESS;
}

char *getAppStatus(UBOOL8 enable, CmsEntityId eid)
{
   if (!enable)
   {
      return MDMVS_IDLE;
   }
   if (basdcfg_isApplicationRunning((UINT32)eid) == TRUE)
   {
      return MDMVS_ACTIVE;
   }
   else
   {
      if (rut_isApplicationActive(eid) == TRUE)
      {
         /* this means the application is launched, but not running */
         return (MDMVS_ERROR);
      }
      else
      {
         return (MDMVS_IDLE);
      }
   }
}

CmsRet stl_basdCfgObject
   (_BasdCfgObject *obj,
    const InstanceIdStack *iidStack __attribute__((unused)))
{
   char *status;
   char dateTimeBuf[BUFLEN_64];
   FILE * fp = NULL;
   time_t time;
   int byteRead = 0;
    
   status = (getAppStatus(obj->enable,EID_BASD));

   if (cmsUtl_strcmp(obj->status,status) == 0)
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, status, mdmLibCtx.allocFlags);
      if (strcmp(status,MDMVS_ACTIVE) == 0)
      {
         fp = fopen(BASD_VAR_UPTIME_FILENAME, "r");
         if (fp) {
            byteRead = fread(&time, 1, sizeof(time), fp);
            fclose(fp);
            fp = NULL;
         }
         else
            printf("not able to open file %s\n",BASD_VAR_UPTIME_FILENAME);

         if (byteRead > 0)
         {
            getXSIDateTimeFromTime_t(&time,dateTimeBuf,sizeof(dateTimeBuf));
            CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Possibly auto-relaunched", mdmLibCtx.allocFlags);      
         }
         else
         {
            cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
            CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Unknown", mdmLibCtx.allocFlags);      
         }
      }
      else
      {
         cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Unknown", mdmLibCtx.allocFlags);      
      }
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);      
      return (CMSRET_SUCCESS);
   }
}

CmsRet stl_basHistoryObject(_BasHistoryObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_basClientObject
   (_BasClientObject *obj, 
    const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsEntityId eid;
   char *status;   
   char dateTimeBuf[BUFLEN_64];
   
   eid = appNameToEid(obj->name);
   status = getAppStatus(obj->enable,eid);

   if (cmsUtl_strcmp(obj->status,status) == 0)
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      /* normally the application's status and timestamp get updated when admin up/down.
       * But in the case of an app exiting by itself, the status is not updated.
       * The timestamp of when an app is started can be written to a file like basd2 app and 
       * read through here.   Otherwise, just do it here (slightly delay in time)
       */
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, status, mdmLibCtx.allocFlags);      
      cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);

      if ((obj->enable) && (!cmsUtl_strcmp(status,MDMVS_ACTIVE)))
      {
         /* mode is enable, and the status is changed from idle/error to active */
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Possibly auto-relaunched", mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Unknown", mdmLibCtx.allocFlags);
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet stl_basClientHistoryObject(_BasClientHistoryObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_BASD_1 */
