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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bcm_ulog.h"
#include "bcm_net.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "mdm.h"
#include "prctl.h"
#include "rut_basdcfg.h"

#if defined(SUPPORT_OPS_BAS)
#include "cms_msg_modsw.h"
#endif


#if defined(SUPPORT_OPS_BAS)

CmsRet startBasEu(UBOOL8 *found, UBOOL8 start)
{
   EUObject *euObj = NULL;
   InstanceIdStack iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   *found = FALSE;
   INIT_INSTANCE_ID_STACK(&iidStack);

   /* loop through EUObject to find bas */
   while (*found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &euObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObj->name, BASD_NAME) == 0)
      {
         *found = TRUE;

         if (start)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(euObj->requestedState,
                                            MDMVS_ACTIVE, mdmLibCtx.allocFlags);
         }
         else
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(euObj->requestedState,
                                            MDMVS_IDLE, mdmLibCtx.allocFlags);
         }

         ret = cmsObj_set(euObj, &iidStack);

         if (ret != CMSRET_SUCCESS)
         {
            bcmuLog_error("set requestedState failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **)&euObj);
   }

   if (*found == FALSE)
   {
      bcmuLog_notice("Cannot find BAS EU");
   }

   return ret;
}

void basdcfg_startBasEuAtBootup(const char *url, const char *devId, 
                                UINT32 debugLevel, const char *username,
                                const char *password, UINT32 keepAliveInterval)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader reqMsg = EMPTY_MSG_HEADER;
   char cmd[BUFLEN_1024] = {0};
   char urlStr[BUFLEN_256] = {0};
   char devIdStr[BUFLEN_64] = {0};
   char typeStr[BUFLEN_64] = {0};
   char wanTypeBuf[BUFLEN_16] = {0};   
   FILE *fp;

   if (IS_EMPTY_STRING(url))
   {
      cmsLog_error("no portal URL");
      return;
   }

   reqMsg.src = mdmLibCtx.eid;
   reqMsg.type = (CmsMsgType) CMS_MSG_BOOTUP_START_BAS;
   reqMsg.flags_request = 1;
   reqMsg.dst = EID_MODSWD;

   if((ret = cmsMsg_send(mdmLibCtx.msgHandle, &reqMsg)) != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to send message (ret=%d)", ret);
      return;      
   }

   if (!IS_EMPTY_STRING(devId))
   {
      snprintf(devIdStr, sizeof(devIdStr)-1, "-i %s", devId);
   }

   if (bcmNet_getWanTypeStr(wanTypeBuf,sizeof(wanTypeBuf)) == 0)
   {
      snprintf(typeStr, sizeof(typeStr), "-t %s%s", "rg-",wanTypeBuf);
   }
   else
   {
      snprintf(typeStr, sizeof(typeStr), "-t %s", BAS_DEFAULT_PLATFORM_TYPE);
   }

   if (cmsUtl_strstr(url, "://"))
   {
      snprintf(urlStr, sizeof(urlStr)-1, "%s", url);
   }
   else
   {
      snprintf(urlStr, sizeof(urlStr)-1, "-w %s", url);
   }

   /* username and password are optional */
   if (IS_EMPTY_STRING(username) || IS_EMPTY_STRING(password))
   {
      snprintf(cmd,sizeof(cmd)-1,"run -d %d -b %s %s %s -c %s -p %s -k %d",
               debugLevel,urlStr,devIdStr,typeStr,
               BASD_CERT_FILE_FULLPATH,BASD_CACHE_FILE_FULLPATH,
               keepAliveInterval);
   }
   else
   {
      snprintf(cmd, sizeof(cmd)-1,
               "run -d %d -b %s %s %s -c %s -p %s -n %s -s %s -k %d",
               debugLevel, urlStr, devIdStr, typeStr,
               BASD_CERT_FILE_FULLPATH, BASD_CACHE_FILE_FULLPATH,
               username, password, keepAliveInterval);
   }

   fp = fopen(BASD_ARGUMENT_INFO_FILENAME, "w");
   if (fp)
   {
      fputs(cmd, fp);
      fclose(fp);
   }
   else
   {
      bcmuLog_error("fail to write bas argument file");
      return;
   }
}

void basdcfg_startBasClientEuAtBootup(const char *name)
{
   CmsRet ret = CMSRET_SUCCESS;
   char clientName[BUFLEN_32] = {0};
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(clientName)] = {0};
   CmsMsgHeader *reqMsg = (CmsMsgHeader *)msgBuf;
   char *str = (char *)(reqMsg+1);

   if (IS_EMPTY_STRING(name))
   {
      cmsLog_error("no client name");
      return;
   }

   cmsUtl_strncpy(clientName, name, sizeof(clientName));
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->type = (CmsMsgType) CMS_MSG_BOOTUP_START_BAS_CLIENT;
   reqMsg->flags_request = 1;
   reqMsg->dst = EID_MODSWD;
   reqMsg->dataLength = sizeof(clientName);

   cmsUtl_strncpy(str, clientName, sizeof(clientName));
   if((ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg)) != CMSRET_SUCCESS)
   {
      bcmuLog_error("Failed to send message (ret=%d)", ret);
      return;      
   }
}

static UBOOL8 isOpenplatPkgClient(UINT32 eid)
{
   UBOOL8 ret = FALSE;

   /* So far, the following is the list of clients supporting openplat pkg
    * rgclient, bas_openplat, bas_rg and bas_tr143
    */
   if (eid == EID_BAS_CLIENT_RG ||
       eid == EID_BAS_CLIENT_OPENPLAT || eid == EID_BAS_CLIENT_TR143 ||
       eid == EID_BAS_CLIENT_RG_BDK)
   {
      ret = TRUE;
   }

   return ret;
}

CmsRet startBasClientEu(UINT32 eid, UBOOL8 start)
{
   EUObject *euObj = NULL;
   InstanceIdStack iidStack;
   UBOOL8 found = FALSE;
   char name[BUFLEN_32] = {0};
   CmsRet ret = CMSRET_SUCCESS;

   if (eid == EID_BAS_CLIENT_RG)
   {
      strcpy(name, BAS_CLIENT_RG_EXE);
   }
   else if (eid == EID_BAS_CLIENT_OPENPLAT)
   {
      strcpy(name, BAS_CLIENT_OPENPLAT_EXE);
   }
   else if (eid == EID_BAS_CLIENT_TR143)
   {
      strcpy(name, BAS_CLIENT_TR143_EXE);
   }
   else if (eid == EID_BAS_CLIENT_RG_BDK)
   {
      strcpy(name, BAS_CLIENT_RG_BDK_EXE);
   }
   else
   {
      bcmuLog_debug("eid<%d> does not support openplat pkg", eid);
      return ret;
   }

   if (start && mdmShmCtx->inMdmInit)
   {
      basdcfg_startBasClientEuAtBootup(name);
      return ret;
   }
   INIT_INSTANCE_ID_STACK(&iidStack);

   /* loop through EUObject to find bas */
   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &euObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObj->name, name) == 0)
      {
         found = TRUE;

         if (start)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(euObj->requestedState,
                                            MDMVS_ACTIVE, mdmLibCtx.allocFlags);
         }
         else
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(euObj->requestedState,
                                            MDMVS_IDLE, mdmLibCtx.allocFlags);
         }

         ret = cmsObj_set(euObj, &iidStack);

         if (ret != CMSRET_SUCCESS)
         {
            bcmuLog_error("set requestedState failed, ret=%d", ret);
         }
      }

      cmsObj_free((void **)&euObj);
   }

   if (found == FALSE)
   {
      bcmuLog_notice("Cannot find BAS client EU with eid<%d>", eid);
   }

   return ret;
}

#endif   /* #if defined(SUPPORT_OPS_BAS) */

UBOOL8 basdcfg_isApplicationRunning(UINT32 eid)
{
   UBOOL8 running = FALSE;

#if defined(SUPPORT_OPS_BAS)
   EUObject *euObj = NULL;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack;
   char name[BUFLEN_32] = {0};

   INIT_INSTANCE_ID_STACK(&iidStack);

   if (eid == EID_BAS_CLIENT_RG)
   {
      strcpy(name, BAS_CLIENT_RG_EXE);
   }
   else if (eid == EID_BAS_CLIENT_OPENPLAT)
   {
      strcpy(name, BAS_CLIENT_OPENPLAT_EXE);
   }
   else if (eid == EID_BAS_CLIENT_TR143)
   {
      strcpy(name, BAS_CLIENT_TR143_EXE);
   }
   else if (eid == EID_BAS_CLIENT_RG_BDK)
   {
      strcpy(name, BAS_CLIENT_RG_BDK_EXE);
   }
   else if (eid == EID_BASD)
   {
      strcpy(name, BASD_NAME);
   }
   else
   {
      bcmuLog_debug("eid<%d> does not support openplat pkg", eid);
      running = rut_isApplicationRunning((CmsEntityId)eid);
      goto out;
   }

   while (found == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &euObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObj->name, name) == 0)
      {
         found = TRUE;

         if (cmsUtl_strcmp(euObj->status, MDMVS_ACTIVE) == 0)
         {
            running = TRUE;
         }
      }

      cmsObj_free((void **)&euObj);
   }

out:
#else
   running = rut_isApplicationRunning((CmsEntityId)eid);
#endif   /* #if defined(SUPPORT_OPS_BAS) */

   return running;
}

UBOOL8 basdcfg_isBasExist(void)
{
   UBOOL8 exist = FALSE;

#if defined(SUPPORT_OPS_BAS)

   EUObject *euObj = NULL;
   InstanceIdStack iidStack;

   INIT_INSTANCE_ID_STACK(&iidStack);

   /* loop through EUObject to find bas */
   while (exist == FALSE &&
          cmsObj_getNextFlags(MDMOID_EU, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &euObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euObj->name, BASD_NAME) == 0)
      {
         exist = TRUE;

      }

      cmsObj_free((void **)&euObj);
   }

#else

   if (access(BASD_LOCATION, F_OK) != -1)
   {
      exist = TRUE;
   }

#endif   /* #if defined(SUPPORT_OPS_BAS) */

   return exist;
}

/* marking obj unused, but I will use it when I am finished with the implementation
 * of status and reason change items
 */
void basdcfg_stopBas(BasdCfgObject *obj)
{
   
#if defined(SUPPORT_OPS_BAS)

   UBOOL8 found=FALSE;
   UBOOL8 start=FALSE;
   CmsRet ret;

   ret = startBasEu(&found, start);

   if (ret != CMSRET_SUCCESS || found == FALSE)
   {
      bcmuLog_notice("Cannot stop BAS EU");
   }

#else
   char dateTimeBuf[BUFLEN_64];
   
   /* when it fails here, we can log the reason here */
   if ((rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_BASD, NULL, 0)) != CMSRET_SUCCESS)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively stopped (internal error)", mdmLibCtx.allocFlags);
      cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);            
      cmsLog_error("failed to stop basd");
   }

#endif   /* #if defined(SUPPORT_OPS_BAS) */
}

CmsRet basdCfg_composeCmdStr(char *cmd, int len, BasdCfgObject *obj)
{
   char urlStr[BUFLEN_256] = {0};
   char devIdStr[BUFLEN_64] = {0};
   char typeStr[BUFLEN_64] = {0};
   char wanTypeBuf[BUFLEN_16] = {0};
  
   if (IS_EMPTY_STRING(obj->URL))
   {
      cmsLog_error("no portal URL");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (!IS_EMPTY_STRING(obj->deviceId))
   {
      snprintf(devIdStr, sizeof(devIdStr)-1, "-i %s", obj->deviceId);
   }

   if (bcmNet_getWanTypeStr(wanTypeBuf,sizeof(wanTypeBuf)) == 0)
   {
      snprintf(typeStr, sizeof(typeStr), "-t %s%s", "rg-",wanTypeBuf);
   }
   else
   {
      snprintf(typeStr, sizeof(typeStr), "-t %s", BAS_DEFAULT_PLATFORM_TYPE);
   }
   
   /*
    * The default protocol BAS configuration is wss. So if the portal is using
    * wss, just enter the "BAS Portal URL" with URL or IP address.
    * If the portal is using tcp or ssl, you have to enter in the following
    * format:
    *   tcp://url_or_ip_address:port, e.g. tcp://1.2.3.4:1883
    *   ssl://url_or_ip_address:port, e.g. ssl://1.2.3.4:8883 
    */
   if (cmsUtl_strstr(obj->URL, "://"))
   {
      snprintf(urlStr, sizeof(urlStr)-1, "%s", obj->URL);
   }
   else
   {
      snprintf(urlStr, sizeof(urlStr)-1, "-w %s", obj->URL);
   }

   /* username and password are optional */
   if (IS_EMPTY_STRING(obj->username) || IS_EMPTY_STRING(obj->password))
   {
      snprintf(cmd,len-1,"run -d %d -b %s %s %s -c %s -p %s -k %d",
               obj->debugLevel,urlStr,devIdStr,typeStr,
               BASD_CERT_FILE_FULLPATH,BASD_CACHE_FILE_FULLPATH,
               obj->keepAliveInterval);
   }
   else
   {
      snprintf(cmd,len-1,"run -d %d -b %s %s %s -c %s -p %s -n %s -s %s -k %d",
               obj->debugLevel,urlStr,devIdStr,typeStr,
               BASD_CERT_FILE_FULLPATH,BASD_CACHE_FILE_FULLPATH,
               obj->username,obj->password,obj->keepAliveInterval);
   }

   return CMSRET_SUCCESS;
}
void basdcfg_startBas(BasdCfgObject *obj)
{
   char cmd[BUFLEN_1024] = {0};

#if defined(SUPPORT_OPS_BAS)

   UBOOL8 found=FALSE;
   UBOOL8 start=TRUE;
   CmsRet ret;
   FILE *fp;

   basdCfg_composeCmdStr(cmd,sizeof(cmd),obj);

   fp = fopen(BASD_ARGUMENT_INFO_FILENAME, "w");
   if (fp)
   {
      fputs(cmd, fp);
      fclose(fp);
   }
   else
   {
      bcmuLog_error("fail to write bas argument file");
      return;
   }

   ret = startBasEu(&found, start);

   if (ret != CMSRET_SUCCESS || found == FALSE)
   {
      bcmuLog_notice("Cannot start BAS EU");
   }

#else
   char dateTimeBuf[BUFLEN_64]; 
   UINT32 pid = 0;   

   snprintf(cmd, sizeof(cmd)-1, "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lib/services:/lib/plugins");
   prctl_runCommandInShellBlocking(cmd);
   memset(cmd,0,sizeof(cmd));
   basdCfg_composeCmdStr(cmd,sizeof(cmd),obj);
   
   bcmuLog_notice("Start basd with command <%s>", cmd);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_BASD, cmd, strlen(cmd))) == CMS_INVALID_PID)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively started (internal error)", mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ACTIVE, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively started", mdmLibCtx.allocFlags);
   }
   cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
   CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);            

#endif   /* #if defined(SUPPORT_OPS_BAS) */
}


void basdcfg_restartBas(BasdCfgObject *obj)
{
#if defined(SUPPORT_OPS_BAS)
   if (basdcfg_isApplicationRunning(EID_BASD))
   {
      basdcfg_stopBas(obj);
   }

   basdcfg_startBas(obj);
#else
   char cmd[BUFLEN_1024] = {0};
   UINT32 pid = 0;
   char dateTimeBuf[BUFLEN_64];   

   basdCfg_composeCmdStr(cmd,sizeof(cmd),obj);
   bcmuLog_notice("Restart basd with command <%s>", cmd);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_BASD, cmd, strlen(cmd))) == CMS_INVALID_PID)
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively restarted (internal error)", mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively restarted", mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ACTIVE, mdmLibCtx.allocFlags);
   }
   cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
   CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);

#endif
}

UINT32 appNameToEid(char *name)
{
   UINT32 eid = EID_INVALID;
   
   if (cmsUtl_strcmp(name, BAS_CLIENT_RG) == 0)
   {
      eid = EID_BAS_CLIENT_RG;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_RG_BDK) == 0)
   {
      eid = EID_BAS_CLIENT_RG_BDK;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_OPENPLAT) == 0)
   {
      eid = EID_BAS_CLIENT_OPENPLAT;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_XDSL) == 0)
   {
      eid = EID_BAS_CLIENT_XDSL;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_GPON) == 0)
   {
      eid = EID_BAS_CLIENT_GPON;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_VOICE) == 0)
   {
      eid = EID_BAS_CLIENT_VOICE;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_SGS) == 0)
   {
      eid = EID_BAS_CLIENT_SGS;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_RDS) == 0)
   {
      eid = EID_BAS_CLIENT_RDS;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_EYE) == 0)
   {
      eid = EID_BAS_CLIENT_EYE;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_TR143) == 0)
   {
      eid = EID_BAS_CLIENT_TR143;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_RDPA) == 0)
   {
      eid = EID_BAS_CLIENT_RDPA;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_TR471) == 0)
   {
      eid = EID_BAS_CLIENT_TR471;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_WIFI) == 0)
   {
      eid = EID_BAS_CLIENT_WIFI;
   }
   else if (cmsUtl_strcmp(name, BAS_CLIENT_WLDATAELM) == 0)
   {
      eid = EID_BAS_CLIENT_WLDATAELM;
   }
   return (eid);
}
char *eidToAppName(UINT32 eid)
{
   switch (eid)
   {
   case EID_BAS_CLIENT_RG:
      return BAS_CLIENT_RG;
   case EID_BAS_CLIENT_RG_BDK:
      return BAS_CLIENT_RG_BDK;
   case EID_BAS_CLIENT_OPENPLAT:
      return BAS_CLIENT_OPENPLAT;
   case EID_BAS_CLIENT_XDSL:
      return BAS_CLIENT_XDSL;
   case EID_BAS_CLIENT_GPON:
      return BAS_CLIENT_GPON;
   case EID_BAS_CLIENT_VOICE:
      return BAS_CLIENT_VOICE;
   case EID_BAS_CLIENT_SGS:
      return BAS_CLIENT_SGS;
   case EID_BAS_CLIENT_RDS:
      return BAS_CLIENT_RDS;
   case EID_BAS_CLIENT_EYE:
      return BAS_CLIENT_EYE;
   case EID_BAS_CLIENT_TR143:
      return BAS_CLIENT_TR143;
   case EID_BAS_CLIENT_RDPA:
      return BAS_CLIENT_RDPA;
   case EID_BAS_CLIENT_TR471:
      return BAS_CLIENT_TR471;
   case EID_BAS_CLIENT_WIFI:
      return BAS_CLIENT_WIFI;
   case EID_BAS_CLIENT_WLDATAELM:
      return BAS_CLIENT_WLDATAELM;
   }
      
   return BAS_CLIENT_INVALID;
}

/* start/stop or restart a static bas client with CMS EID */
CmsRet doClientOpWithEid(BasClientObject *obj, UINT32 eid, char *arg, UBOOL8 start, UBOOL8 restart)
{
   UINT32 pid = CMS_INVALID_PID;
   CmsRet ret = CMSRET_SUCCESS;
   char dateTimeBuf[BUFLEN_64];
   
#if defined(SUPPORT_OPS_BAS)
   UBOOL8 isPkg;

   /* check whether the bas client is openplat package first */
   isPkg = isOpenplatPkgClient(eid);
#endif

   int argLen = strlen(arg);
   
   if (restart)
   {
#if defined(SUPPORT_OPS_BAS)
      if (isPkg)
      {
         ret = startBasClientEu(eid, FALSE);
         ret = startBasClientEu(eid, TRUE);
         goto done_start;
      }
#endif

      if (argLen == 0)
      {
         pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, eid, NULL, 0);
      }
      else
      {
         pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, eid, arg, argLen+1);
      }
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to restart bas client %s",eidToAppName(eid));
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively restarted (internal error)", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         /* we have ways to detect an application going down, but not starting */
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively restarted", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ACTIVE, mdmLibCtx.allocFlags);
      }
      cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);
   } /* restart */
   else if (start)
   {
#if defined(SUPPORT_OPS_BAS)
      if (isPkg)
      {
         ret = startBasClientEu(eid, TRUE);
         goto done_start;
      }
#endif

      /* start the app */
      if (argLen == 0)
      {
         pid = rut_sendMsgToSmd(CMS_MSG_START_APP, eid, NULL, 0);
      }
      else
      {
         pid = rut_sendMsgToSmd(CMS_MSG_START_APP,eid, arg, argLen+1);
      }
      
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start bas client %s.",eidToAppName(eid));
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively started (internal error)", mdmLibCtx.allocFlags);
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         /* we have ways to detect an application going down, but not starting */
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ACTIVE, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively started", mdmLibCtx.allocFlags);
      }
      cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);            
   } /* start */
   else
   {
#if defined(SUPPORT_OPS_BAS)
      if (isPkg)
      {
         ret = startBasClientEu(eid, FALSE);
         goto done_start;
      }
#endif

      /* stop */
      ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, eid, NULL, 0);
      if (ret != CMSRET_SUCCESS)      
      {
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(obj->statusChangeReason, "Adminstratively stopped (internal error)", mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
         CMSMEM_REPLACE_STRING_FLAGS(obj->lastChange, dateTimeBuf, mdmLibCtx.allocFlags);            
         cmsLog_error("failed to stop bas client %s.",eidToAppName(eid));
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         /* smd detects a client is stopped and status is updated there */
         cmsLog_debug("Stop bas client %s msg sent", eidToAppName(eid));
      }
   }

#if defined(SUPPORT_OPS_BAS)
done_start:
#endif
   return(ret);
}

/* start/stop or restart a dynamic bas client with a specified full path executable */
CmsRet doClientOpWithExe(BasClientObject *obj  __attribute__((unused)), char *exe __attribute__((unused)), 
                         char *arg __attribute__((unused)), UBOOL8 start __attribute__((unused)), 
                         UBOOL8 restart __attribute__((unused)))
{
   /* TODO */
   return (CMSRET_SUCCESS);
}

void rutBas_modifyNumBasdHistory(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   BasdCfgObject *basdObj = NULL;

   if ((cmsObj_get(MDMOID_BASD_CFG, &iidStack, 0, (void *) &basdObj)) == CMSRET_SUCCESS)
   {
      basdObj->anomalyHistoryNumberOfEntries += delta;
      cmsObj_set(basdObj, &iidStack);
      cmsObj_free((void **) &basdObj);
   }
}

void rutBas_modifyNumBasClientHistory(SINT32 delta, const InstanceIdStack *iidStack)
{
   BasClientObject *basClientObj = NULL;
   InstanceIdStack ancestorIidStack = *iidStack;
   CmsRet ret;

   ret = cmsObj_getAncestor(MDMOID_BAS_CLIENT, MDMOID_BAS_CLIENT_HISTORY, &ancestorIidStack,
                            (void **)&basClientObj);
   if ( ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s, ret %d",
                   MDMOID_BAS_CLIENT, MDMOID_BAS_CLIENT_HISTORY,
                   cmsMdm_dumpIidStack(iidStack),ret);
   }
   basClientObj->anomalyHistoryNumberOfEntries += delta;
   cmsObj_set(basClientObj, &ancestorIidStack);
   cmsObj_free((void **) &basClientObj);
}

void rutBas_checkBasdNumHistory(void)
{
   InstanceIdStack idStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack historyIdStack = EMPTY_INSTANCE_ID_STACK;
   BasdCfgObject *basdObj=NULL;
   BasHistoryObject *basdHistoryObj=NULL;

   /* first check if there are are max entries already, if yes, delete the first entry.
    * And so a new entry is added, the count remains the same.
    */
   if (cmsObj_get(MDMOID_BASD_CFG, &idStack, 0,(void **) &basdObj) == CMSRET_SUCCESS)
   {
      if (basdObj->anomalyHistoryNumberOfEntries == basdObj->maxAnomalyHistoryNumberOfEntries)
      {
         /* delete the first (oldest) result history entry instance */
         if (cmsObj_getNext(MDMOID_BAS_HISTORY, &historyIdStack, (void **) &basdHistoryObj) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get the oldest result history entry object");
            cmsObj_free((void **) &basdObj);       
            return;
         }
         cmsObj_free((void **) &basdHistoryObj); 
         if (cmsObj_deleteInstance(MDMOID_BAS_HISTORY, &historyIdStack) != CMSRET_SUCCESS)
         {
            cmsObj_free((void **) &basdObj);
            cmsLog_error("Could not delete the oldest result history entry object");
            return;
         }
         basdObj->anomalyHistoryNumberOfEntries -= 1;
         cmsObj_set(basdObj, &idStack);
      }
      cmsObj_free((void **) &basdObj);
   }
}

void rutBas_checkBasClientNumHistory(const InstanceIdStack *iidStack)
{
   BasClientObject *basClientObj = NULL;
   /* iidStack is the iidStack of the history of client */
   InstanceIdStack ancestorIidStack = *iidStack;
   BasClientHistoryObject *basClientHistoryObj = NULL;
   InstanceIdStack historyIdStack = EMPTY_INSTANCE_ID_STACK;;
   CmsRet ret;
   

   ret = cmsObj_getAncestor(MDMOID_BAS_CLIENT, MDMOID_BAS_CLIENT_HISTORY, &ancestorIidStack,
                            (void **)&basClientObj);
   if ( ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s ret %d",
                   MDMOID_BAS_CLIENT, MDMOID_BAS_CLIENT_HISTORY,
                   cmsMdm_dumpIidStack(iidStack),ret);
      return;
   }
   if (basClientObj->anomalyHistoryNumberOfEntries == basClientObj->maxAnomalyHistoryNumberOfEntries)
   {
      /* delete the first (oldest) result history entry instance */
      if (cmsObj_getNext(MDMOID_BAS_CLIENT_HISTORY, &historyIdStack, (void **) &basClientHistoryObj) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get the oldest result history entry object");
         cmsObj_free((void **) &basClientObj);       
         return;
      }
      cmsObj_free((void **) &basClientHistoryObj);
      if (cmsObj_deleteInstance(MDMOID_BAS_CLIENT_HISTORY, &historyIdStack) != CMSRET_SUCCESS)
      {
         cmsObj_free((void **) &basClientObj);
         cmsLog_error("Could not delete the oldest result history entry object, historyIdStack %s",
                      cmsMdm_dumpIidStack(&historyIdStack));
         return;
      }
      basClientObj->anomalyHistoryNumberOfEntries -= 1;
      cmsObj_set(basClientObj, &ancestorIidStack);
   }
   cmsObj_free((void **) &basClientObj);
}

#endif /* DMP_X_BROADCOM_COM_BASD_1 */
