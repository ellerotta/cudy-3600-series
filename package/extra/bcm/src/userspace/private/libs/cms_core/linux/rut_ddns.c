/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_msg.h"
#include "rut_ddns.h"

#define DDNS_CONFIG_FILE  "/var/inadyn.conf"


void rutDDns_restart(void)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   FILE * fp[8] = {NULL};
   FILE * fptr = NULL;
   char ifName[8][16];
   unsigned char i=0, cnt = 0;
   char path[CMS_MAX_FULLPATH_LENGTH] = {0};
   char cmdLine[BUFLEN_256];
   CmsRet ret;
   UBOOL8 ssl_flag = FALSE;

   /*
    * build path to config file in a way that is friendly to desktop linux.
    */
   if ((ret = cmsUtl_getRunTimePath(DDNS_CONFIG_FILE, path, sizeof(path))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to %s", DDNS_CONFIG_FILE);
      return;
   }

   // if CA file exist, SSL=1
   if (access("/etc/ssl/certs/cert.pem", F_OK) ==  0)
       ssl_flag = TRUE; 

   while ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS)
   {
      if (ddnsObj->enable)
      {
         cmsLog_debug("setting info: %s/%s/%s/%s/%s", ddnsObj->fullyQualifiedDomainName, ddnsObj->userName, ddnsObj->password, ddnsObj->ifName, ddnsObj->providerName);

         // if config file for the interface exist, use existing file ptr
         for(i=0, fptr=NULL; i<cnt; i++)
         {
             if(!strcmp(ifName[i], ddnsObj->ifName))
             {
                 cmsLog_debug("config file for %s found:", ddnsObj->ifName);
                 fptr = fp[i]; 
                 break;
             }
         }

         // if config file for the interface does not exist, create file
         if(fptr == NULL)
         {
             char conf[CMS_MAX_FULLPATH_LENGTH+8] = {0};

             sprintf(conf,"%s%hhu", path, i);
             if ((fptr = fopen(conf, "w+")) == NULL )
             {
                 cmsLog_error("could not open %s", conf);
                 cmsObj_free((void **) &ddnsObj);
                 return;
             }
             fprintf( fptr, "period          = 20\n");
             fprintf( fptr, "user-agent      = Mozilla/5.0\n");
             if(ssl_flag == TRUE)
             {
                 /* secure-ssl < true | false >
                    If the HTTPS certificate validation fails for a provider inadyn aborts the DDNS update before sending any credentials. 
                    When this setting is disabled, i.e. false, then inadyn will only issue a warning.
                    Set it to false here to pass CDRouter test. Customer can set it to true */
                 fprintf( fptr, "secure-ssl      = false\n");
                 fprintf( fptr, "ca-trust-file   = /etc/ssl/certs/cert.pem\n");
             }
             // save file ptr for the interface
             fp[cnt] = fptr;
             strncpy(ifName[cnt], ddnsObj->ifName, sizeof(ifName[cnt])-1);            
             cmsLog_debug("config file for %s created:", ddnsObj->ifName);
             cnt++;
         }

         // update per interface config file         
         fprintf( fptr, "\nprovider %s {\n", ddnsObj->providerName);
         if(ssl_flag == TRUE)
             fprintf( fptr, "    ssl      = true\n");
         else
             fprintf( fptr, "    ssl      = false\n");
         fprintf( fptr, "    wildcard = true\n");
         fprintf( fptr, "    username = %s\n", ddnsObj->userName );
         fprintf( fptr, "    password = %s\n", ddnsObj->password );
         fprintf( fptr, "    hostname = {%s}\n", ddnsObj->fullyQualifiedDomainName );
         fprintf( fptr, "    checkip-command = \"ifconfig %s | grep 'inet addr'\"\n}\n", ddnsObj->ifName );
      }

      cmsObj_free((void **) &ddnsObj);
   }

   // run inadyn per interface
   for(i=0; i<cnt; i++)
   {
      fclose( fp[i] );
      snprintf(cmdLine, sizeof(cmdLine), "inadyn -C -l debug -f /var/inadyn.conf%hhu -i %s --pidfile=/var/inadyn.pid%hhu", i, ifName[i], i);
      cmsLog_debug("cmdLine = %s\n", cmdLine);
      rut_doSystemAction("rut", cmdLine);
   }
   
   return;
}


void rutDDns_stop(void)
{
   char path_conf[CMS_MAX_FULLPATH_LENGTH]={0};
   char path_pid[CMS_MAX_FULLPATH_LENGTH]={0};
   FILE* fs = NULL;
   char cmd[BUFLEN_128] = {0};
   char buf[BUFLEN_128] = {0};
   unsigned char i;

   /*
    * build path to config file in a way that is friendly to desktop linux.
    */
   if (cmsUtl_getRunTimePath(DDNS_CONFIG_FILE, path_conf, sizeof(path_conf)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to %s", DDNS_CONFIG_FILE);
      return;
   }

   if (cmsUtl_getRunTimePath("/var/inadyn.pid", path_pid, sizeof(path_pid)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to %s", "/var/inadyn.pid");
      return;
   }
   
   for(i=0; i<8; i++)
   {
       char conf[CMS_MAX_FULLPATH_LENGTH+8]={0};
       char pid[CMS_MAX_FULLPATH_LENGTH+8]={0};

       sprintf(conf,"%s%hhu", path_conf, i);
       sprintf(pid,"%s%hhu", path_pid, i);

       // if conf file does exist      
       if (access(conf,  F_OK) ==  0) 
       {
           // remove config file
           /* coverity[toctou] */  
           unlink(conf);

           // if pid file exist, kill pid 
           if (access(pid,  F_OK) ==  0) 
           {
             cmsLog_debug("pid file exist %s", pid);
             /* coverity[toctou] */ 
             if ((fs = fopen(pid, "r")) == NULL)
             {
               cmsLog_error("Failed to get pid file %s", pid);
             }
             else
             {
               if (fread(buf, 1, sizeof(buf), fs) > 0)
               {
                   snprintf(cmd, sizeof(cmd), "kill %d", atoi(buf));
               }
               fclose(fs);

               cmsLog_debug("kill pid  %s", cmd);
               rut_doSystemAction("rut", cmd);
             }
           }
       }
       else
           break;
   }
   return;        
}


UBOOL8 rutDDns_isAllRequiredValuesPresent(const _DDnsCfgObject *ddnsObj)
{
   if (ddnsObj->fullyQualifiedDomainName == NULL ||
       ddnsObj->userName == NULL ||
       ddnsObj->providerName == NULL)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


UBOOL8 rutDDns_isDuplicateFQDN(const char *fqdn, const InstanceIdStack *skipIidStack)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 isDuplicate=FALSE;
   CmsRet ret;


   while ((!isDuplicate) &&
          ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS))
   {
      if (skipIidStack == NULL || cmsMdm_compareIidStacks(&iidStack, skipIidStack))
      {
         /*
          * check for duplicate fully qualified domain name only when
          * the iidStack we got back from getNext is different from
          * the skipIidStack.
          */

         /* isDuplicate is TRUE if the strcmp == 0 */
          isDuplicate = (0 == cmsUtl_strcmp(fqdn, ddnsObj->fullyQualifiedDomainName));
      }

      cmsObj_free((void **) &ddnsObj);
   }

   return isDuplicate;
}


UBOOL8 rutDDns_isValuesChanged(const _DDnsCfgObject *newObj, const _DDnsCfgObject *currObj)
{

   if (cmsUtl_strcmp(newObj->fullyQualifiedDomainName, currObj->fullyQualifiedDomainName) ||
       cmsUtl_strcmp(newObj->userName, currObj->userName) ||
       cmsUtl_strcmp(newObj->password, currObj->password) ||
       cmsUtl_strcmp(newObj->ifName, currObj->ifName) ||
       cmsUtl_strcmp(newObj->providerName, currObj->providerName))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


UINT32 rutDDns_getNumberOfEnabledEntries(void)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 count=0;
   CmsRet ret;


   while ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS)
   {
      if (ddnsObj->enable)
      {
         count++;
      }
      cmsObj_free((void **) &ddnsObj);
   }

   cmsLog_debug("count=%d", count);
   return count;
}

