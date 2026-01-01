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

#ifdef DMP_X_BROADCOM_COM_L2TPNS_2

#include <sys/stat.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "../rut_l2tpac.h"
#include "rut2_l2tpns.h"


const char *xl2tpdServerFolder  = "/var/run/xl2tpd";
const char *xl2tpdServerConfFile  = "/var/run/xl2tpd/xl2tpd-server.conf";
const char *xl2tpdServerPppOptionFile = "/var/run/xl2tpd/server-ppp-options.xl2tpd";
const char *xl2tpdServerPidFile  = "/var/run/xl2tpd-server.pid";
const char *xl2tpdServerSecrets  = "/var/run/xl2tpd/l2tp-secrets";


#define L2TP_MTU 1460
#define L2TP_MRU 1460
#define L2TP_LCP_ECHO_FAIL 4
#define L2TP_LCP_ECHO_INTERVAL 30
#define L2TP_CONNECT_DELAY 5000
#define DEFAULT_LNS_LINKNAME "ppp1"

CmsRet rutL2tpNS_start_dev2(Dev2PppInterfaceL2tpNsObject *newObj)
{
   char l2tpNsCmd[BUFLEN_128]={0};
   FILE *fp_sec, *fp_conf, *fp_ppp;
   struct stat st;      
   CmsRet ret = CMSRET_SUCCESS;
   int intfIdx;

   cmsLog_notice("Entered");

   /* create /var/run/xl2tpd folder for holding xl2tpd related config files */
   /* coverity[toctou] */ 
   if (stat(xl2tpdServerFolder, &st) == -1) {
      if (mkdir(xl2tpdServerFolder, 0700)) {
         cmsLog_error("unable to create %s", xl2tpdServerFolder);
         return CMSRET_INTERNAL_ERROR;
      }
   }   
   
   /* write out ppp-options file */
   fp_ppp = fopen(xl2tpdServerPppOptionFile, "w");
   if (fp_ppp == NULL) {
      cmsLog_error("unable to open %s", xl2tpdServerPppOptionFile);
      return CMSRET_INTERNAL_ERROR;
   }
   fprintf(fp_ppp, "ipcp-accept-local\n");
   fprintf(fp_ppp, "ipcp-accept-remote\n");
   fprintf(fp_ppp, "noccp\n");
   fprintf(fp_ppp, "auth\n");
   fprintf(fp_ppp, "mtu %d\n", L2TP_MTU);
   fprintf(fp_ppp, "mru %d\n", L2TP_MRU);
   fprintf(fp_ppp, "lcp-echo-failure %d\n", L2TP_LCP_ECHO_FAIL);
   fprintf(fp_ppp, "lcp-echo-interval %d\n", L2TP_LCP_ECHO_INTERVAL);
   fprintf(fp_ppp, "connect-delay %d\n", L2TP_CONNECT_DELAY);
   fprintf(fp_ppp, "proxyarp\n");
   fprintf(fp_ppp, "debug\n");
   
   if(!is_PppIntf_exist(DEFAULT_LNS_LINKNAME))  // default linkname is ok to use
   {
      fprintf(fp_ppp, "linkname %s\n", DEFAULT_LNS_LINKNAME);
      cmsLog_debug("l2tpns use default linkname %s", DEFAULT_LNS_LINKNAME);
   } 
   else
   {
      // try to get available ppp linkname
      intfIdx = getUnusedIntfindex(PPP_PREFIX);
      if( intfIdx < 0 )
      {
         fclose(fp_ppp);
         cmsLog_error("failed to get available ppp linkname for l2tpns");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         fprintf(fp_ppp, "linkname %s%d\n", PPP_PREFIX, intfIdx);
         cmsLog_debug("l2tpns use available linkname %s%d", PPP_PREFIX, intfIdx);
      }
   }
     
   if(newObj->enablePAP)   
      fprintf(fp_ppp, "login\n");
   fprintf(fp_ppp, "%s-pap\n", newObj->enablePAP?"require":"refuse");
   fprintf(fp_ppp, "%s-chap\n", newObj->enableCHAP?"require":"refuse"); 
   fprintf(fp_ppp, "%s-mschap\n", newObj->enableMSCHAP?"require":"refuse"); 
   fprintf(fp_ppp, "%s-mschap-v2\n", newObj->enableMSCHAPv2?"require":"refuse");    

   /* write out l2tpns conf file */
   fp_conf = fopen(xl2tpdServerConfFile, "w");
   if (fp_conf == NULL) {
      cmsLog_error("unable to open %s", xl2tpdServerConfFile);
      fclose(fp_ppp);
      return CMSRET_INTERNAL_ERROR;
   }

   fprintf(fp_conf, "[global]\n");                                /* global context */
   fprintf(fp_conf, "auth file = %s\n", xl2tpdServerSecrets);     /* auth file path */
   fprintf(fp_conf, "port = %d\n", DEFAULT_L2TP_PORT);            /* l2tp default port */
  
   fprintf(fp_conf, "[lns %s]\n", newObj->lnsName);               /* lns context */
   fprintf(fp_conf, "ip range = %s-%s\n", newObj->peerSatrtIp, newObj->peerEndIp);
   fprintf(fp_conf, "local ip = %s\n", newObj->lnsLocalIp);
   fprintf(fp_conf, "require authentication = yes\n");
   fprintf(fp_conf, "pppoptfile = %s\n", xl2tpdServerPppOptionFile);
   fprintf(fp_conf, "length bit = %s\n", newObj->enableLengthBit?"yes":"no");
   if(!newObj->enableCHAP && !newObj->enableMSCHAP && !newObj->enableMSCHAPv2)
      fprintf(fp_conf, "refuse chap = yes\n");
   if(!newObj->enablePAP)
      fprintf(fp_conf, "refuse pap = yes\n");
   
   /* write out l2tp-secrets file */
   fp_sec = fopen(xl2tpdServerSecrets, "w");
   if (fp_sec == NULL) {
      cmsLog_error("unable to open %s", xl2tpdServerSecrets);
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

    /* start l2tpns connection */
   snprintf(l2tpNsCmd, sizeof(l2tpNsCmd), " -D -c %s -p %s", xl2tpdServerConfFile, xl2tpdServerPidFile);  
   ret = rutL2tp_startApp(l2tpNsCmd);

   return ret;
}


CmsRet rutL2tpNS_stop_dev2(Dev2PppInterfaceL2tpNsObject *currObj __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   struct stat st;
   FILE *fp;
   UINT32 xl2tpdServerPid   = 0; 

    cmsLog_notice("Entered");

    if (stat(xl2tpdServerPidFile, &st) == -1) 
    {
        return ret;
    }
    
    fp = fopen(xl2tpdServerPidFile, "r");
    if (fp != NULL)
    {
        if( fscanf(fp, "%u", &xl2tpdServerPid) != 1 )
        {
            cmsLog_error("fscanf pid from xl2tpdServerPidFile (%s) error", xl2tpdServerPidFile);
        }
      
        fclose(fp);
    }
    else
    {
        cmsLog_error("unable to read pid from xl2tpdServerPidFile");
        ret = CMSRET_INTERNAL_ERROR;
        return ret;
    }

    if (xl2tpdServerPid)
    {
        rutL2tp_stopApp(xl2tpdServerPid);
    }

    // Typically, in linux we do not report errors if we cannot delete
    // a file.  It usually means the file has already been deleted.
    remove(xl2tpdServerPidFile);
    remove(xl2tpdServerConfFile);
    remove(xl2tpdServerPppOptionFile);
    remove(xl2tpdServerSecrets);

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPNS_2 */
