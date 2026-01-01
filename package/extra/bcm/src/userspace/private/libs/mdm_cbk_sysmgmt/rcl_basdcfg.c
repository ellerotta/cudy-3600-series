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

#include "cms.h"
#include "bcm_ulog.h"
#include "cms_util.h"
#include "rut_util.h"
#include "cms_core.h"
#include "cms_mem.h"
#include "cms_msg.h"
#include "cms_boardioctl.h"
#include "bcm_boardutils.h"
#include "mdm.h"
#include "mdm_private.h"
#include "mdm_validstrings.h"
#include "prctl.h"
#include "rut_system.h"
#include "rut_basdcfg.h"


static void setupBasDirFile(void)
{
   char line[BUFLEN_512] = {0};

   /* create /var/bas */
   snprintf(line, sizeof(line)-1, "mkdir -p %s", BASD_VAR_DIR);
   prctl_runCommandInShellBlocking(line);

   /* setup soft link for certificate */
   snprintf(line, sizeof(line)-1, "mkdir -p %s;ln -sf %s %s", BASD_DATA_DIR,
            BASD_REAL_CERTIFICATE, BASD_DATA_CERT_FILENAME);
   prctl_runCommandInShellBlocking(line);
}


CmsRet rcl_basdCfgObject( _BasdCfgObject *newObj,
                const _BasdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj != NULL && currObj == NULL)
   {
      char buf[BUFLEN_64] = {0};

      if (IS_EMPTY_STRING(newObj->deviceId) == TRUE)
      {
         bcmUtl_getSerialNumber(buf,BUFLEN_64);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS
            (newObj->deviceId, buf, mdmLibCtx.allocFlags);
      }
   }

   if (mdmShmCtx->inMdmInit)
   {
      /* 
       * - create /var/bas
       * - setup soft link for certificate
       */
      setupBasDirFile();

      /* start bas if enable is true at bootup */
      if (newObj != NULL && newObj->enable == TRUE)
      {
         /* start bas only */
#if defined(SUPPORT_OPS_BAS)
         basdcfg_startBasEuAtBootup(newObj->URL, newObj->deviceId,
                                    newObj->debugLevel, newObj->username,
                                    newObj->password, newObj->keepAliveInterval);
#else
         basdcfg_startBas(newObj);
#endif
      }
   }

   if (newObj != NULL && currObj != NULL)
   {
      if (currObj->enable == FALSE && newObj->enable == TRUE)
      {
         /* start bas only */
         basdcfg_startBas(newObj);         
      }
      else if (currObj->enable == TRUE && newObj->enable == FALSE)
      {
         /* stop bas only */
         basdcfg_stopBas(newObj);
      }
      else if (currObj->enable == TRUE && newObj->enable == TRUE)
      {
         if (newObj->restarting)
         {
            basdcfg_restartBas(newObj);
         }
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_basClientObject( _BasClientObject *newObj,
                            const _BasClientObject *currObj,
                            const InstanceIdStack *iidStack __attribute((unused)),
                            char **errorParam __attribute((unused)),
                            CmsRet *errorCode __attribute((unused)))
{
   UBOOL8 needOp = FALSE;
   UBOOL8 start= FALSE;
   UBOOL8 restart= FALSE;
   char arg[256] = {0};
   char exe[256] = {0};
   UINT32 eid = EID_INVALID;

   memset(exe, 0, sizeof(exe));
   memset(arg, 0, sizeof(arg));

   if (ADD_NEW(newObj, currObj))
   {
      /* at startup when activateObjects, or a dynamic client is added */
      if (newObj->enable)
      {
         needOp = TRUE;
         start = TRUE;
      }
   }
   else if ( DELETE_EXISTING(newObj, currObj) )
   {
      needOp = TRUE;
      start = FALSE;
      /* stop the client */
   }
   else
   {
      if (newObj != NULL)
      {
         /* config change */
         if ((newObj->enable != currObj->enable))
         {
            needOp = TRUE;
            if (newObj->enable)
            {
               start = TRUE;
            }
         }
         else
         {
            if ((currObj->enable) && (newObj->restarting))
            {
               needOp = TRUE;
               restart = TRUE;
            }
         }
      }
   }

   if (needOp)
   {
      /* Assumption: Objects with isStatic==TRUE can be launched by smd */
      if ((!start) && (!restart))
      {
         /* this is the delete */
         if (currObj->isStatic)
         {
            eid = appNameToEid(currObj->name);
         }
         else
         {
            strncpy(exe,currObj->exeName,sizeof(exe)-1);
         }
      }
      else
      {
         if (newObj->isStatic)
         {
            eid = appNameToEid(newObj->name);
         }
         else
         {
            strncpy(exe,newObj->exeName,sizeof(exe)-1);
         }
         if (!IS_EMPTY_STRING(newObj->exeArgs))
         {
            strncpy(arg,newObj->exeArgs,sizeof(arg)-1);
         }
      }
      if (IS_EMPTY_STRING(exe))
      {
         doClientOpWithEid(newObj, eid, arg, start, restart);
      }
      else
      {
         doClientOpWithExe(newObj, exe, arg, start, restart);
      }
   } /* needOp */
   return CMSRET_SUCCESS;
}

CmsRet rcl_basHistoryObject( _BasHistoryObject *newObj,
                             const _BasHistoryObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute((unused)),
                             CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      if (mdmShmCtx->inMdmInit)
      {
         /*
          * During system startup, we might have loaded from a config file.
          * The config file already contains the correct count of these objects.
          * So don't check or update the count in the parent object.
         */
         cmsLog_debug("don't check or update bas client's history count");
         return CMSRET_SUCCESS;
      }

      /* check max entry limit */
      rutBas_checkBasdNumHistory();
      rutBas_modifyNumBasdHistory(1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutBas_modifyNumBasdHistory(-1);
   }
   
   return CMSRET_SUCCESS;
}

CmsRet rcl_basClientHistoryObject( _BasClientHistoryObject *newObj,
                                   const _BasClientHistoryObject *currObj,
                                   const InstanceIdStack *iidStack,
                                   char **errorParam __attribute((unused)),
                                   CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      if (mdmShmCtx->inMdmInit)
      {
         /*
          * During system startup, we might have loaded from a config file.
          * The config file already contains the correct count of these objects.
          * So don't check or update the count in the parent object.
         */
         cmsLog_debug("don't check or update bas client's history count");
         return CMSRET_SUCCESS;
      }

      rutBas_checkBasClientNumHistory(iidStack);
      rutBas_modifyNumBasClientHistory(1,iidStack);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutBas_modifyNumBasClientHistory(-1,iidStack);
   }

   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_BASD_1 */
