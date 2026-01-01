/***********************************************************************
 *
 *  Copyright (c) 2020  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2020:proprietary:standard

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

#ifdef DMP_X_BROADCOM_COM_PPTPNS_2

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "rut2_pptpns.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const char *pptpdServerFolder  = "/var/run/pptpd";
const char *pptpdServerConfFile  = "/var/run/pptpd/pptpd-server.conf";
const char *pptpdServerPppOptionFile = "/var/run/pptpd/server-ppp-options.pptpd";
const char *pptpdServerPidFile  = "/var/run/pptpd/pptpd-server.pid";
const char *pptpdServerSecrets  = "/var/run/pptpd/pptp-secrets";


#define PPTP_MTU 1444
#define PPTP_MRU 1444
#define PPTP_LCP_ECHO_FAIL 4
#define PPTP_LCP_ECHO_INTERVAL 30
#define DEFAULT_PPTPNS_LINKNAME "ppp3"
#define PPPD_EXE_PATH "/bin/pppd"

CmsRet rutPptpNS_start_dev2(Dev2PppInterfacePptpNsObject *newObj)
{
   //UINT32 pid = 0;
   UINT32 remoteIp = 0;
   unsigned char byte3;
   char pptpNsCmd[BUFLEN_128];
   FILE *fp_sec, *fp_conf, *fp_ppp;
   struct stat st;      
   CmsRet ret = CMSRET_SUCCESS;
   int intfIdx;
   
   /* create /var/run/pptpd folder for holding pptpd related config files */
   /* coverity[toctou] */ 
   if (stat(pptpdServerFolder, &st) == -1) {
      if (mkdir(pptpdServerFolder, 0700)) {
         fprintf(stderr, "/var/run/pptpd: unable to create folder\n");
         return CMSRET_INTERNAL_ERROR;
      }
   }   
   
   /* write out ppp-options file */
   fp_ppp = fopen(pptpdServerPppOptionFile, "w");
   if (fp_ppp == NULL) {
      fprintf(stderr, "%s: unable to open file\n", pptpdServerPppOptionFile);
      return CMSRET_INTERNAL_ERROR;
   }
   fprintf(fp_ppp, "nodeflate\n");
   fprintf(fp_ppp, "nobsdcomp\n");
   fprintf(fp_ppp, "novj\n");
   fprintf(fp_ppp, "novjccomp\n");   
   fprintf(fp_ppp, "mtu %d\n", PPTP_MTU);
   fprintf(fp_ppp, "mru %d\n", PPTP_MRU);
   fprintf(fp_ppp, "lcp-echo-failure %d\n", PPTP_LCP_ECHO_FAIL);
   fprintf(fp_ppp, "lcp-echo-interval %d\n", PPTP_LCP_ECHO_INTERVAL);
   if(newObj->enableDebugPptpns)
      fprintf(fp_ppp, "debug\n");
   
   if(!is_PppIntf_exist(DEFAULT_PPTPNS_LINKNAME))  // default linkname is ok to use
   {
      fprintf(fp_ppp, "linkname %s\n", DEFAULT_PPTPNS_LINKNAME);
      cmsLog_debug("pptpns use default linkname %s", DEFAULT_PPTPNS_LINKNAME);
   } 
   else
   {
      // try to get available ppp linkname
      intfIdx = getUnusedIntfindex(PPP_PREFIX);
      if( intfIdx < 0 )
      {
         fclose(fp_ppp);
         cmsLog_error("failed to get available ppp linkname for pptpns");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         fprintf(fp_ppp, "linkname %s%d\n", PPP_PREFIX, intfIdx);
         cmsLog_debug("pptpns use available linkname %s%d", PPP_PREFIX, intfIdx);
      }
   }
     
   if(newObj->enablePAP)
      fprintf(fp_ppp, "login\n");
   fprintf(fp_ppp, "%s-pap\n", newObj->enablePAP?"require":"refuse");
   fprintf(fp_ppp, "%s-chap\n", newObj->enableCHAP?"require":"refuse"); 
   fprintf(fp_ppp, "%s-mschap\n", newObj->enableMSCHAP?"require":"refuse"); 
   fprintf(fp_ppp, "%s-mschap-v2\n", newObj->enableMSCHAPv2?"require":"refuse");    

   /* write out pptpns conf file */
   fp_conf = fopen(pptpdServerConfFile, "w");  
   if (fp_conf == NULL) {
      fprintf(stderr, "%s: unable to open file\n", pptpdServerConfFile);
      fclose(fp_ppp);
      return CMSRET_INTERNAL_ERROR;
   }
   fprintf(fp_conf, "ppp %s\n", PPPD_EXE_PATH); 
   fprintf(fp_conf, "option %s\n", pptpdServerPppOptionFile);
   fprintf(fp_conf, "logwtmp\n");
   fprintf(fp_conf, "localip %s\n", newObj->pptpnsLocalIp);
   if( (remoteIp = inet_addr(newObj->peerEndIp)) == INADDR_NONE )
   {
      fprintf(stderr, "illegal remote ip address\n");
      fclose(fp_conf);
      fclose(fp_ppp);
      return CMSRET_INTERNAL_ERROR;   
   }
   else
   {    
      byte3 = (remoteIp >> 24) & 0xFF;
      fprintf(fp_conf, "remoteip %s-%d\n", newObj->peerSatrtIp, byte3);
   }
   if(newObj->enableDebugPptpns)
      fprintf(fp_conf, "debug\n");
   
   /* write out pptp-secrets file */
   fp_sec = fopen(pptpdServerSecrets, "w");
   if (fp_sec == NULL) {
      fprintf(stderr, "%s: unable to open file\n", pptpdServerSecrets);
      fclose(fp_conf);
      fclose(fp_ppp);
      return CMSRET_INTERNAL_ERROR;
   }  
   fprintf(fp_sec, "# Secrets for authentication\n");
   fprintf(fp_sec, "# client    server    secret    IP addresses\n");
   fprintf(fp_sec, "%s * %s *\n", newObj->username, newObj->password);
     
   /* close all fd */
   fclose(fp_sec);
   fclose(fp_conf);
   fclose(fp_ppp);

   /* start pptpns connection */ 
   snprintf(pptpNsCmd, sizeof(pptpNsCmd), "pptpd -c %s -p %s 2> /dev/null",  pptpdServerConfFile, pptpdServerPidFile);
   rut_doSystemAction( __FUNCTION__, pptpNsCmd);

   return ret;
}


CmsRet rutPptpNS_stop_dev2(Dev2PppInterfacePptpNsObject *currObj)
{
   CmsRet ret = CMSRET_SUCCESS;  
   struct stat st;
   FILE *fp;
   UINT32 pptpdServerPid   = 0;
   int retVal = 0;
   char cmdline[BUFLEN_128] = {0};   

    if (stat(pptpdServerPidFile, &st) == -1) 
    {
        ret = CMSRET_INTERNAL_ERROR;
        return ret;
    }
    
    fp = fopen(pptpdServerPidFile, "r");
    if (fp != NULL)
    {
        if( fscanf(fp, "%u", &pptpdServerPid) != 1 )
            printf("fscanf pptpdServerPid from pptpdServerPidFile error!!\n");        
      
        fclose(fp);
    }
    else
    {
        cmsLog_error("unable to read pid from pptpdServerPidFile");
        ret = CMSRET_INTERNAL_ERROR;
        return ret;
    }		
    
    if (pptpdServerPid)
    {
        /* stop pptpd */
        snprintf(cmdline, sizeof(cmdline), "kill -TERM %d 2> /dev/null", pptpdServerPid);
        rut_doSystemAction( __FUNCTION__, cmdline);
    } 

    if (stat(pptpdServerPidFile, &st) == 0) 
    {
        retVal = remove(pptpdServerPidFile);
        if (retVal != 0)
        {
            cmsLog_error("unable to remove pptpdServerPidFile");
        }  
    }
  
    retVal = remove(pptpdServerConfFile);
    if (retVal != 0)
    {
        cmsLog_error("unable to remove pptpdServerConfFile");
    }
    
    retVal = remove(pptpdServerPppOptionFile);
    if (retVal != 0)
    {
        cmsLog_error("unable to remove pptpdServerPppOptionFile");
    }
    
    retVal = remove(pptpdServerSecrets);
    if (retVal != 0)
    {
        cmsLog_error("unable to remove pptpdServerSecrets");
    }

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_PPTPNS_2 */
