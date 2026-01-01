/***********************************************************************
<:copyright-BRCM:2012:proprietary:standard

   Copyright (c) 2012 Broadcom
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

/** command driven CLI code goes into this file */

#ifdef SUPPORT_CLI_CMD
#ifdef SUPPORT_DEBUG_TOOLS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// should be in string.h, but including it did not fix compiler warning
// so I just declare the function here.
extern char *strcasestr(const char *haystack, const char *needle);
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_data_model_selector.h"
#include "prctl.h"

#include "cli.h"
#include "mdm_cli.h"
#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */
#include "mdm_cli_gpon.h"
#endif /* DMP_X_ITU_ORG_GPON_1 */

void processLogLevelCmd(char *cmdLine)
{
   char *getfailed="get failed, ret=";
   char *currlog="current log level is";
   char *setfailed="set failed, ret=";
   char *setsuccess="new log level set.";

   char *ptr;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 getMode;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("cmdLine is ->%s<-", cmdLine);

   if (!strncasecmp(cmdLine, "help", 4) || !strncasecmp(cmdLine, "--help", 6))
   {
      printf("usage: loglevel get appname\n");
      printf("       loglevel set appname loglevel\n");


      printf("where appname is one of: httpd, tr69c, smd, ssk, telnetd, sshd, consoled, upnp");

#ifdef BRCM_VOICE_SUPPORT
#  ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1
      printf(", dectd");
#  endif
#endif

#ifdef DMP_X_BROADCOM_COM_SNMP_1
      printf(", snmpd");
#endif

#ifdef BRCM_WLAN
#ifdef BUILD_BRCM_UNFWLCFG
      printf(", wlssk");
#endif /* BUILD_BRCM_UNFWLCFG */
#endif /* BRCM_WLAN */

#ifdef SUPPORT_OSGI_FELIX
      printf(", osgid");
#endif

#ifdef DMP_DEVICE2_SM_BASELINE_1
      printf(", bbcd");
      printf(", firewalld");
#endif

#ifdef DMP_X_BROADCOM_COM_EPON_1
      printf(", eponapp");
#endif

#ifdef DMP_X_BROADCOM_COM_GPON_1
      printf(", omcid");
#endif

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
      printf(", tr69c_2");
#endif

#ifdef DMP_X_BROADCOM_COM_ECMS_1
      printf(", ecms");
#endif

#ifdef DMP_DEVICE2_LOCALAGENT_1
      printf(", uspmd");
      printf(", obuspa");
#endif

      printf("\n");
      printf("loglevel is one of \"Error\", \"Notice\", or \"Debug\" (use these exact strings).\n");
      return;
   }

   getMode = (strncasecmp(cmdLine, "get", 3) == 0);

   ptr = &(cmdLine[4]);
   if (!strncasecmp(ptr, "httpd", 5))
   {
      HttpdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_HTTPD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   /* Note: please ensure this case is in front of next case (!strncasecmp(ptr, "tr69c", 5))!! otherwise, it will hit next case */
   else if (!strncasecmp(ptr, "tr69c_2", 7))
   {
      E2E_Tr69cCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_E2_TR69C_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[8]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif   // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   else if (!strncasecmp(ptr, "tr69c", 5))
   {
      Tr69cCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_TR69C_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#ifdef SUPPORT_VECTORINGD
   else if (!strncasecmp(ptr, "vectoringd", 10))
   {
      VectoringCfgObject *obj;

      if ((ret = cmsObj_get(MDMOID_VECTORING_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
        printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[11]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif
   else if (!strncasecmp(ptr, "smd", 3))
   {
      SmdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SMD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[4]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "ssk", 3))
   {
      SskCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SSK_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[4]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "sshd", 4))
   {
      SshdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SSHD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "telnetd", 7))
   {
      TelnetdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_TELNETD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[8]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "consoled", 8))
   {
      ConsoledCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_CONSOLED_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[9]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "upnp", 4))
   {
      UpnpCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_UPNP_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#ifdef BRCM_VOICE_SUPPORT
   else if (!strncasecmp(ptr, "voice", 5))
   {
      printf("\"loglevel set voice\" has been deprecated. Please use \"voice set loglevel\" instead.\n");
   }
#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1  /* aka dectd */
   else if (!strncasecmp(ptr, "dectd", 5))
   {
      DectdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_DECTD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */
#endif /* BRCM_VOICE_SUPPORT */

#ifdef DMP_X_BROADCOM_COM_SNMP_1
   else if (!strncasecmp(ptr, "snmpd", 5))
   {
      SnmpdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SNMPD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif  /* DMP_X_BROADCOM_COM_SNMP_1 */
#ifdef BRCM_WLAN
#ifndef BUILD_BRCM_UNFWLCFG
   else if (!strncasecmp(ptr, "wlssk", 5))
   {
       if (getMode)
       {
           printf("loglevel get not supported for wlssk yet\n");
       }
       else
       {
           CmsMsgHeader msg = EMPTY_MSG_HEADER;
           CmsRet r2;
           msg.type = CMS_MSG_SET_LOG_LEVEL;
           msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
           msg.dst = EID_WLSSK;
           msg.flags_request = 1;
           msg.flags_bounceIfNotRunning = 1;
           msg.wordData = cmsUtl_logLevelStringToEnum(&(ptr[6]));
           r2 = cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, &msg, CMSLCK_MAX_HOLDTIME);
           if (r2 != CMSRET_SUCCESS && r2 != CMSRET_MSG_BOUNCED)
           {
               cmsLog_error("update log level failed, ret=%d", r2);
           }
           else
           {
               printf("%s\n", setsuccess);
           }
       }
   }
#endif /* BUILD_BRCM_UNFWLCFG */

#endif /* BRCM_WLAN */
#ifdef DMP_X_ITU_ORG_GPON_1
   else if (!strncasecmp(ptr, "omcid", 5))
   {
      OmcidCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_OMCID_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_X_ITU_ORG_GPON_1 */
#ifdef SUPPORT_OSGI_FELIX
   else if (!strncasecmp(ptr, "osgid", 5))
   {
      OsgidCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_OSGID_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* SUPPORT_OSGI_FELIX */

#ifdef DMP_DEVICE2_SM_BASELINE_1
   else if (!strncasecmp(ptr, "bbcd", 4))
   {
      BbcdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_BBCD_CFG, &iidStack, 0, (void **) &obj)) !=
           CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_DEVICE2_SM_BASELINE_1 */

#ifdef DMP_DEVICE2_SM_BASELINE_1
   else if (!strncasecmp(ptr, "firewalld", 9))
   {
      FirewalldCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_FIREWALLD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[10]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_DEVICE2_SM_BASELINE_1 */

#ifdef DMP_X_BROADCOM_COM_EPON_1
   else if (!strncasecmp(ptr, "eponapp", 7))
   {
      EponappCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_EPONAPP_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[8]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif
#ifdef SUPPORT_XMPP
   else if (!strncasecmp(ptr, "xmppc", 5))
   {
      XmppcCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_XMPPC_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif  /* SUPPORT_XMPP */
#ifdef DMP_X_BROADCOM_COM_ECMS_1
   else if (!strncasecmp(ptr, "ecms", 4))
   {
      EcmsCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_ECMS_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif
#ifdef DMP_DEVICE2_LOCALAGENT_1
   else if (!strncasecmp(ptr, "uspmd", 5))
   {
      UspMdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_USP_MD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "obuspa", 6))
   {
      ObuspaCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_OBUSPA_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingLevel);
         }
         else
         {
            cmsMem_free(obj->loggingLevel);
            obj->loggingLevel = cmsMem_strdup(&(ptr[7]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_DEVICE2_LOCALAGENT_1 */
   else
   {
      printf("invalid or unsupported app name %s\r\n", ptr);
   }


   return;
}


void processLogDestCmd(char *cmdLine)
{
   char *getfailed="get failed, ret=";
   char *currlog="current log dest is";
   char *setfailed="set failed, ret=";
   char *setsuccess="new log dest set.";

   char *ptr;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 getMode;
   CmsRet ret = CMSRET_SUCCESS;

   if (!strncasecmp(cmdLine, "help", 4) || !strncasecmp(cmdLine, "--help", 6))
   {
      printf("usage: logdest get appname\r\n");
      printf("       logdest set appname logdest\r\n");
      printf("where appname is one of: httpd, tr69c, smd, ssk, telnetd, sshd, consoled, upnp, dnsproxy");
#ifdef SUPPORT_OSGI_FELIX
      printf(", osgid");
#endif

#ifdef DMP_X_BROADCOM_COM_EPON_1
      printf(", eponapp");
#endif

#ifdef DMP_X_BROADCOM_COM_GPON_1
      printf(", omcid");
#endif

#ifdef DMP_X_BROADCOM_COM_ECMS_1
      printf(", ecms");
#endif

#ifdef BRCM_WLAN
#ifdef BUILD_BRCM_UNFWLCFG
      printf(", wlssk");
#endif /* BUILD_BRCM_UNFWLCFG */
#endif /* BRCM_WLAN */
#ifdef SUPPORT_XMPP
      printf(", xmppc");
#endif
#ifdef BRCM_VOICE_SUPPORT  /* SUPPORT_VOICE */
      printf(", voice");
#endif
      printf("\r\n");
      printf("loglevel is \"Standard Error\", \"Syslog\" or \"Telnet\".\r\n");
      return;
   }

   getMode = (strncasecmp(cmdLine, "get", 3) == 0);

   ptr = &(cmdLine[4]);
   if (!strncasecmp(ptr, "httpd", 5))
   {
      HttpdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_HTTPD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "tr69c", 5))
   {
      Tr69cCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_TR69C_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
  else if (!strncasecmp(ptr, "smd", 3))
   {
      SmdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SMD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[4]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "ssk", 3))
   {
      SskCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SSK_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[4]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "sshd", 4))
   {
      SshdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_SSHD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "telnetd", 7))
   {
      TelnetdCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_TELNETD_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[8]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "consoled", 8))
   {
      ConsoledCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_CONSOLED_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[9]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "upnp", 4))
   {
      UpnpCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_UPNP_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[5]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
   else if (!strncasecmp(ptr, "dnsproxy", 8))
   {
      DnsProxyCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[9]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }

#ifdef BRCM_WLAN
#ifdef BUILD_BRCM_UNFWLCFG
   else if (!strncasecmp(ptr, "wlssk", 5))
   {
       if (getMode)
       {
           printf("logdest get not supported for wlssk yet\n");
       }
       else
       {
           CmsMsgHeader msg = EMPTY_MSG_HEADER;
           CmsRet r2;
           msg.type = CMS_MSG_SET_LOG_DESTINATION;
           msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
           msg.dst = EID_WLSSK;
           msg.flags_request = 1;
           msg.flags_bounceIfNotRunning = 1;
           msg.wordData = cmsUtl_logDestinationStringToEnum(&(ptr[6]));
           r2 = cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, &msg, CMSLCK_MAX_HOLDTIME);
           if (r2 != CMSRET_SUCCESS && r2 != CMSRET_MSG_BOUNCED)
           {
               cmsLog_error("update log destination failed, ret=%d", r2);
           }
           else
           {
               printf("%s\n", setsuccess);
           }
       }
   }
#endif /* BUILD_BRCM_UNFWLCFG */
#endif /* BRCM_WLAN */

#ifdef SUPPORT_OSGI_FELIX
   if (!strncasecmp(ptr, "osgid", 5))
   {
      OsgidCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_OSGID_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* SUPPORT_OSGI_FELIX */

#ifdef DMP_X_BROADCOM_COM_EPON_1
   else if (!strncasecmp(ptr, "eponapp", 7))
   {
      EponappCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_EPONAPP_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[8]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif
#ifdef DMP_X_BROADCOM_COM_GPON_1
   else if (!strncasecmp(ptr, "omcid", 5))
   {
      EponappCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_OMCID_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* DMP_X_BROADCOM_COM_GPON_1 */
#ifdef DMP_X_BROADCOM_COM_ECMS_1
else if (!strncasecmp(ptr, "ecms", 4))
{
   EcmsCfgObject *obj=NULL;

   if ((ret = cmsObj_get(MDMOID_ECMS_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
   {
      printf("%s %d\n", getfailed, ret);
   }
   else
   {
      if (getMode)
      {
         printf("%s %s\n", currlog, obj->loggingDestination);
      }
      else
      {
         cmsMem_free(obj->loggingDestination);
         obj->loggingDestination = cmsMem_strdup(&(ptr[5]));
         if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
         {
            printf("%s %d\n", setfailed, ret);
         }
         else
         {
            printf("%s\n", setsuccess);
         }
      }

      cmsObj_free((void **) &obj);
   }
}
#endif /* DMP_X_BROADCOM_COM_ECMS_1 */
#ifdef SUPPORT_XMPP
   else if (!strncasecmp(ptr, "xmppc", 5))
   {
      XmppcCfgObject *obj=NULL;

      if ((ret = cmsObj_get(MDMOID_XMPPC_CFG, &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->loggingDestination);
         }
         else
         {
            cmsMem_free(obj->loggingDestination);
            obj->loggingDestination = cmsMem_strdup(&(ptr[6]));
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* SUPPORT_XMPP */
#ifdef BRCM_VOICE_SUPPORT
   else if (!strncasecmp(ptr, "voice", 5))
   {
      VoiceObject *obj=NULL;

      /* Only single voiceservice is supported so return first one found */
      if ((ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void **) &obj)) != CMSRET_SUCCESS)
      {
         printf("%s %d\n", getfailed, ret);
      }
      else
      {
         if (getMode)
         {
            printf("%s %s\n", currlog, obj->X_BROADCOM_COM_LoggingDestination);
         }
         else
         {
            cmsMem_free(obj->X_BROADCOM_COM_LoggingDestination);
 
            obj->X_BROADCOM_COM_LoggingDestination = cmsMem_strdup( &(ptr[6]) );
            if ((ret = cmsObj_set(obj, &iidStack)) != CMSRET_SUCCESS)
            {
               printf("%s %d\n", setfailed, ret);
            }
            else
            {
               printf("%s\n", setsuccess);
            }
         }

         cmsObj_free((void **) &obj);
      }
   }
#endif /* BRCM_VOICE_SUPPORT */
   else
   {
      printf("invalid or unsupported app name %s\r\n", ptr);
   }

   return;
}


#ifdef SUPPORT_DM_DETECT
void processDataModelCmd(char *cmdLine)
{
   if (!strcasecmp(cmdLine, "get"))
   {
      if (cmsMdm_isDataModelDevice2())
      {
         printf("current data model is Pure181\n");
      }
      else
      {
         printf("current data model is Hybrid\n");
      }
   }
   else if (!strcasecmp(cmdLine, "set"))
   {
      cmsUtil_setDataModelDevice2();
      printf("Data Model will be Pure181 on next reboot.\n");
   }
   else if (!strcasecmp(cmdLine, "clear"))
   {
      cmsUtil_clearDataModelDevice2();
      printf("Data Model will be Hybrid on next reboot.\n");
   }
   else if (!strcasecmp(cmdLine, "toggle"))
   {
      cmsUtil_toggleDataModel();

      {
         SINT32 rv;
         UINT8 dmc[CMS_DATA_MODEL_PSP_VALUE_LEN]={0};
         rv = cmsPsp_get(CMS_DATA_MODEL_PSP_KEY, dmc, sizeof(dmc));
         if (rv != CMS_DATA_MODEL_PSP_VALUE_LEN)
         {
            printf("error while trying to read data model mode from PSP, rv=%d\n", rv);
         }
         else
         {
            printf("Data Model will be %s on next reboot.\n",
                  (dmc[0] == 1) ? "Pure181" : "Hybrid");
         }
      }
   }
   else
   {
      printf("usage: datamodel get\n");
      printf("       datamodel set   (set mode to 1, meaning Pure181)\n");
      printf("       datamodel clear (set mode to 0, meaning Hybrid)\n");
      printf("       datamodel toggle\n\n");
   }

   return;
}
#endif  /* SUPPORT_DM_DETECT */


void processDumpEidInfoCmd(char *cmdLine)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsRet ret;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpeid [eid] \n");
      printf("request smd to dump its Entity Info database.  If an eid is\n");
      printf("given, then only that eid will be dumped.\n");
      return;
   }

   msg.dst = EID_SMD;
   msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg.type = CMS_MSG_DUMP_EID_INFO;
   msg.flags_request = 1;

   if (cmsUtl_strlen(cmdLine))
   {
      if (CMSRET_SUCCESS != cmsUtl_strtoul(cmdLine, NULL, 0, &msg.wordData))
      {
         printf("argument must be a (eid) number.\n");
         return;
      }
   }

   if ((ret = cmsMsg_sendAndGetReply(cliPrvtMsgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("msg send failed, ret=%d", ret);
   }
}


void processDumpSmdMsgConnCmd(char *cmdLine)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpMsgConn\n");
      printf("       dump all msg connections in smd \n");
      return;
   }

   msg.type = CMS_MSG_DUMP_MSG_CONNECTIONS;
   msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg.dst = EID_SMD;
   msg.flags_request = 1;

   cmsMsg_sendAndGetReply(cliPrvtMsgHandle, &msg);
   return;
}


// Dump application-specific info
void processDumpAppInfoCmd(char *cmdLine __attribute__((unused)))
{
   /* Dump OMCI MIB. */
#if defined(DMP_X_ITU_ORG_GPON_1) && defined(BRCM_CMS_BUILD)
   if (prctl_getPidByName("omcid") != CMS_INVALID_PID)
   {
       processDumpOmciInfoCmd(cmdLine);
   }
#endif
}


void processDumpSysInfoCmd(char *cmdLine __attribute__((unused)))
{
   int rc;

   // Note that upon entry into this function, the CMS lock has not been
   // acquired yet.  The stuff at the beginning of this function is low
   // level linux stuff which does not require a CMS lock.  The stuff which
   // dumps CMS info will use the new MDM auto-lock feature which will
   // automatically lock and unlock as needed.

   printf("###DumpSysInfo: First dump system information\n");
   rc = system("/opt/scripts/dumpsysinfo.sh");
   if (rc != 0)
   {
      cmsLog_debug("dumpsysinfo shell script returned %d", rc);
   }

   printf("\n\n###DumpSysInfo: now dump CMS information\n");

   printf("\n#####Versions\n");
   processSwVersionCmd("");
   processSwVersionCmd("-b");
   processSwVersionCmd("-m");
   processSwVersionCmd("-c");
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
   processSwVersionCmd("-d");
#endif
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
   processSwVersionCmd("-v");
#endif

   printf("\n\n#####Memory Info\n");
   processMeminfoCmd("");

   printf("\n\n#####Dumping contents of saved config file\n");
   processDumpCfgCmd("");

   printf("\n\n#####Dumping MDM\n");
   processDumpMdmCmd("");

   printf("\n\n#####Dumping application specific info\n");
   processDumpAppInfoCmd("");

   printf("\n\n#####Dumping syslog");
   processSyslogCmd("dump");
}


void processExitOnIdleCmd(char *cmdLine)
{
   if (!strncasecmp(cmdLine, "get", 3))
   {
      printf("current timout is %d seconds\n", mdmCli_getExitOnIdleTimeout());
   }
   else if (!strncasecmp(cmdLine, "set", 3))
   {
      int timeout = atoi(&(cmdLine[4]));
      mdmCli_setExitOnIdleTimeout(timeout);
      printf("timeout is set to %d seconds (for this session only, not saved to config)\n", timeout);
   }
   else
   {
      printf("usage: exitOnIdle get\n");
      printf("       exitOnIdle set <seconds>\n\n");
   }

   return;
}

void processDoXtmLockCmd(char *cmdLine)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   int val = atoi(cmdLine);

   printf("setting doXtmLock to %d\n", val);

   msg.type = CMS_MSG_DOXTMLOCK;
   msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg.dst = EID_SSK;
   msg.flags_request = 1;
   msg.wordData = (UINT32) val;
   cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, &msg, CMSLCK_MAX_HOLDTIME);

   return;
}


#endif /* SUPPORT_DEBUG_TOOLS */

#endif /* SUPPORT_CLI_CMD */
