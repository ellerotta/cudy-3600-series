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

#ifdef DMP_X_BROADCOM_COM_PPTPAC_2

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "rut2_pptpac.h"
#define PPTP_MTU 1444
#define PPTP_MRU 1444

CmsRet rutPptpAC_start_dev2(Dev2PppInterfacePptpObject *newObj, const char *server, const char *userid, const char *password)
{
   UINT32 pid = 0;
   CmsRet ret = CMSRET_SUCCESS;
   char pptpcmd[BUFLEN_256];
   char pptpAcLinkName[BUFLEN_16]={0};
   int intfIdx;
      
   if(!is_PppIntf_exist(DEFAULT_PPTPAC_LINKNAME))  // default linkname is ok to use
   {
      strncpy(pptpAcLinkName, DEFAULT_PPTPAC_LINKNAME, sizeof(pptpAcLinkName)-1);
      pptpAcLinkName[sizeof(pptpAcLinkName)-1] = '\0';
      cmsLog_debug("pptpac use default linkname %s", DEFAULT_PPTPAC_LINKNAME);
   } 
   else
   {
      // try to get available ppp linkname
      intfIdx = getUnusedIntfindex(PPP_PREFIX);
      if( intfIdx < 0 )
      {    
         cmsLog_error("failed to get available ppp linkname for pptpac");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         sprintf(pptpAcLinkName, "%s%d", PPP_PREFIX, intfIdx);
         pptpAcLinkName[sizeof(pptpAcLinkName)-1] = '\0';
         cmsLog_debug("pptpac use available linkname %s%d", PPP_PREFIX, intfIdx);
      }
   }
   
   /* start pptpac connection */
   snprintf(pptpcmd, sizeof(pptpcmd), " nodetach plugin %s linkname %s pptp_server %s name %s password %s mtu %d mru %d nodeflate novj novjccomp"
                                    ,PPTP_LIB_PATH ,pptpAcLinkName ,server ,userid ,password, PPTP_MTU, PPTP_MRU);
   
   cmsLog_debug("pptpcmd is %s", pptpcmd);
   /* send message to ssk to launch pptpd */
   if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PPP, pptpcmd, strlen(pptpcmd)+1)) == CMS_INVALID_PID)
   {
      printf("failed to start or restart pptp\n");
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      snprintf(pptpcmd, sizeof(pptpcmd), "echo %d > %s", pid, PPTPAC_PID_FILENAME);
      rut_doSystemAction("Save PptpAc Pid", pptpcmd);
      snprintf(pptpcmd, sizeof(pptpcmd), "echo %s > %s", pptpAcLinkName, PPTPAC_LINKNAME_FILE);
      rut_doSystemAction("PptpAcLinkname", pptpcmd);
   }

   return ret;
}

CmsRet rutPptpAC_stop_dev2(Dev2PppInterfacePptpObject *currObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 specificEid = 0;
   FILE* fs = NULL;
   char cmd[BUFLEN_128];
   char buf[BUFLEN_128];
   int retval = 0;
   
   if ((fs = fopen(PPTPAC_PID_FILENAME, "r")) == NULL)
   {
       cmsLog_debug("Failed to get %s\n", PPTPAC_PID_FILENAME);
   }
   else
   {
      retval = fread(buf, 1, sizeof(buf), fs);
      if (retval > 0)
      {
         printf("stop ppp pid is %d\n", atoi(buf));
         specificEid = MAKE_SPECIFIC_EID(atoi(buf), EID_PPP);
      }
      else
         printf("fread PPTPAC_PID_FILENAME fail\n");
           
      fclose(fs);
        
      if ((rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL,0)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("failed to stop pptp.");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         unlink(PPTPAC_PID_FILENAME);
         unlink(PPTPAC_LINKNAME_FILE);
      }  
   }
    
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_PPTPAC_2 */
