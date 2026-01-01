/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifdef DMP_X_BROADCOM_COM_L2TPAC_2

#include <sys/stat.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "../rut_l2tpac.h"



/* Additional info 
 * xl2tp dial system command : echo "c newObj->tunnelName" > /tmp/l2tp-control 
 * xl2tp shutown system command : echo "d newObj->tunnelName" > /tmp/l2tp-control 
 * /tmp/l2tp-control is defined in l2tp.h
 */
UINT32 getL2tpAvailPort(void)
{
   struct stat st;
   UINT32 availPort = L2tpDefaultPort;
   FILE *fp_lacPort;
   
   /* Reference file for available port not exist, create it and return L2tpDefaultPort */
   if (stat(xl2tpdAssignedPort, &st) == -1)
   {     
      fp_lacPort = fopen(xl2tpdAssignedPort, "w");
      if (fp_lacPort != NULL)
      {
         fprintf(fp_lacPort, "%u\n", availPort);
         fclose(fp_lacPort);
         cmsLog_notice("create file %s and use port %u", xl2tpdAssignedPort, availPort);
      }
      else
      {
         cmsLog_error("unable to create file: %s, still use l2tp default port %u", xl2tpdAssignedPort, availPort);
      }
      return availPort;
   }
   
   /* Get the available port number for caller, then increment it and write back */
   fp_lacPort = fopen(xl2tpdAssignedPort, "r");
   if (fp_lacPort != NULL)
   {
      if( fscanf(fp_lacPort, "%u", &availPort) != 1 )
      {
         cmsLog_error("fscanf xl2tpdAssignedPort (%s) error!!", xl2tpdAssignedPort);
         fclose(fp_lacPort); 
      }
      else
      {
         fclose(fp_lacPort);
         fp_lacPort = fopen(xl2tpdAssignedPort, "w");
         if(fp_lacPort != NULL)
         {
            availPort++;
            fprintf(fp_lacPort, "%u\n", availPort);
            fclose(fp_lacPort);
         }                   
      }
   }
   
   return availPort;
}
CmsRet rut_getWanL2tpAcObject_dev2(InstanceIdStack *iidStack,
                               Dev2PppInterfaceL2tpObject **L2tpAcIntf)
{
   Dev2PppInterfaceObject *pppIntfObj = NULL;
   Dev2PppInterfaceL2tpObject *l2tpObj = NULL;      
   CmsRet ret = CMSRET_SUCCESS;     
   UBOOL8 found = FALSE;
      
   /* Look thru  all l interfaces and check for the lowerlayer matches with
   * the layer 2 interface (vlantermation here).  If found, get the pppoe object for
   * X_BROADCOM_COM_AddPppToBridge and other params.
   */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "PPPoL2tpAc"))
      {
         InstanceIdStack l2tpIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP, 
                                                iidStack, 
                                                &l2tpIidStack,
                                                (void **) &l2tpObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get l2tpObj, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }
            
         found = TRUE;
         
         if (L2tpAcIntf)
         {
             /* return obj to caller, don't free */
             *L2tpAcIntf = l2tpObj;
         }
      }
      else
      {
         cmsObj_free((void **)&pppIntfObj);
      }
   }

   if (!found)
   {
      cmsLog_debug("Failed to find the matching pppIntfObj");
      return CMSRET_INTERNAL_ERROR;
   }
  
   cmsObj_free((void **)&pppIntfObj); 
      
   return ret;
}

CmsRet rutL2tpAC_start_dev2(const char *tunnelName, const char *server, const char *userid, const char *password, const int ppp_debug)
{
   CmsRet ret = CMSRET_SUCCESS;
   char xl2tpdPidFile[BUFLEN_64]={0};
   char xl2tpdConfFile[BUFLEN_64]={0};
   char xl2tpdPppOptionFile[BUFLEN_64]={0};   
   char l2tpcmd[BUFLEN_128]={0};
   FILE *fp_conf, *fp_ppp;
   struct stat st;
   int intfIdx;

   cmsLog_notice("Entered: tunnelName=%s server=%s userid=%s",
                 tunnelName, server, userid);

   /* create /var/run/xl2tpd folder for holding xl2tpd related config files */
   // TODO: look at similar code rut_l2tpac.c, which seems to support multiple
   // tunnels, while the code here does not.
   /* coverity[toctou] */ 
   if (stat(xl2tpdFolder, &st) == -1) {
      if (mkdir(xl2tpdFolder, 0700)) {
         cmsLog_error("unable to create %s", xl2tpdFolder);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   
   //Generate the corresponding files via tunnelName
   snprintf(xl2tpdPidFile, sizeof(xl2tpdPidFile), "%s/xl2tpd_%s.pid", xl2tpdFolder, tunnelName);
   snprintf(xl2tpdConfFile, sizeof(xl2tpdConfFile), "%s/xl2tpd_%s.conf", xl2tpdFolder, tunnelName);
   snprintf(xl2tpdPppOptionFile, sizeof(xl2tpdPppOptionFile), "%s/ppp-options_%s.xl2tpd", xl2tpdFolder, tunnelName);
   fp_ppp = fopen(xl2tpdPppOptionFile, "w");
   if (fp_ppp == NULL) {
      cmsLog_error("unable to open %s", xl2tpdPppOptionFile);
      return CMSRET_INTERNAL_ERROR;
   }
   fprintf(fp_ppp, "user %s\n", userid);
   fprintf(fp_ppp, "password %s\n", password);
   fprintf(fp_ppp, "mtu %d\n", L2TP_MTU);
   fprintf(fp_ppp, "mru %d\n", L2TP_MRU);

   if(!is_PppIntf_exist(DEFAULT_LAC_LINKNAME))  // default linkname is ok to use
   {
      fprintf(fp_ppp, "linkname %s\n", DEFAULT_LAC_LINKNAME);
      cmsLog_debug("l2tpac use default linkname %s", DEFAULT_LAC_LINKNAME);
   } 
   else
   {
      // try to get available ppp linkname
      intfIdx = getUnusedIntfindex(PPP_PREFIX);
      if( intfIdx < 0 )
      {
         fclose(fp_ppp);
         cmsLog_error("failed to get available ppp linkname for l2tpac");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         fprintf(fp_ppp, "linkname %s%d\n", PPP_PREFIX, intfIdx);
         cmsLog_debug("l2tpac use available linkname %s%d", PPP_PREFIX, intfIdx);
      }
   }
      
   /* write out xl2tpd.conf file */
   // TODO: use the link config writing code in rut_l2tpac.c, that one
   // seems to support multiple tunnels, but this code seems to support
   // only a single tunnel.
   fp_conf = fopen(xl2tpdConfFile, "w");
   if (fp_conf == NULL) {
      cmsLog_error("unable to open %s", xl2tpdConfFile);
      fclose(fp_ppp);
      return CMSRET_INTERNAL_ERROR;
   }
   
   /* global context */
   fprintf(fp_conf, "[global]\n");
   fprintf(fp_conf, "port = %d\n", getL2tpAvailPort());
   
   /* lac context */
   fprintf(fp_conf, "[lac %s]\n", tunnelName);
   fprintf(fp_conf, "lns = %s\n", server);
   if(ppp_debug)
      fprintf(fp_conf, "ppp debug = yes\n");
   else
      fprintf(fp_conf, "ppp debug = no\n");
   fprintf(fp_conf, "autodial = yes\n");
   fprintf(fp_conf, "redial = yes\n");
   fprintf(fp_conf, "redial timeout = 10\n");
   fprintf(fp_conf, "max redials = 5\n"); 
   fprintf(fp_conf, "pppoptfile = %s\n", xl2tpdPppOptionFile);
   
   fclose(fp_conf);
   fclose(fp_ppp);

   snprintf(l2tpcmd, sizeof(l2tpcmd), " -D -c %s -p %s", xl2tpdConfFile, xl2tpdPidFile);
   cmsLog_notice("### start run %s", xl2tpdConfFile);
   ret = rutL2tp_startApp(l2tpcmd);
   sleep(1);
   return ret;
}

CmsRet rutL2tpAC_stop_dev2(const char *tunnelName)
{
   CmsRet ret = CMSRET_SUCCESS;
   struct stat st;
   FILE *fp;
   UINT32 xl2tpdPid = 0;
   char xl2tpdPidFile[BUFLEN_64] = {0};
   char xl2tpdConfFile[BUFLEN_64] = {0};
   char xl2tpdPppOptionFile[BUFLEN_64] = {0};      

   cmsLog_notice("Entered: tunnelName=%s", tunnelName);
    
   /* Findout the corresponding files via tunnelName */
   snprintf(xl2tpdPidFile, sizeof(xl2tpdPidFile), "%s/xl2tpd_%s.pid", xl2tpdFolder, tunnelName);
   snprintf(xl2tpdConfFile, sizeof(xl2tpdConfFile), "%s/xl2tpd_%s.conf", xl2tpdFolder, tunnelName);
   snprintf(xl2tpdPppOptionFile, sizeof(xl2tpdPppOptionFile), "%s/ppp-options_%s.xl2tpd", xl2tpdFolder, tunnelName);
    
   if (stat(xl2tpdPidFile, &st) == -1) 
   {
      return ret;
   }

   fp = fopen(xl2tpdPidFile, "r");
   if (fp != NULL)
   {
      if( fscanf(fp, "%u", &xl2tpdPid) != 1 )
      {
          cmsLog_error("fscanf pid from xl2tpdPidFile (%s) error!!", xl2tpdPidFile);
      }
      fclose(fp);
   }
   else
   {
      cmsLog_error("Cannot open pidfile %s", xl2tpdPidFile);
   }

   if (xl2tpdPid)
   {
      rutL2tp_stopApp(xl2tpdPid);
   }

   // Typically, in linux we do not report errors if we cannot delete
   // a file.  It usually means the file has already been deleted.
   remove(xl2tpdPidFile);
   remove(xl2tpdConfFile);
   remove(xl2tpdPppOptionFile);

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_2 */
