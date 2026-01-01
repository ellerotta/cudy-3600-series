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

/*!\file comp_tr69_md.c
 * \brief High level functions used by tr69_md.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

#include "bcm_ulog.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "sysutil_proc.h"
#include "prctl.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"


// Forward declarations
BcmRet compMd_launchTr69c(SINT32 shmId, SINT32 bootLaunch, SINT32 *pid);
BcmRet compMd_launchTr69Proxy(SINT32 shmId);
#ifdef DMP_DEVICE2_XMPPBASIC_1
BcmRet compMd_launchXmppc(SINT32 shmId);
#endif
void compMd_collectChild(SINT32 pid, UINT32 timeoutMs);


// Private vars to track the child processes launched by tr69_md
static SINT32 tr69cPid = 0;
static SINT32 tr69ProxyPid = 0;
#ifdef DMP_DEVICE2_XMPPBASIC_1
static SINT32 xmppcPid = 0;
#endif


static UBOOL8 tr69_proxy_exists(void)
{
   int r;
   r = access("/bin/tr69_proxy", X_OK);
   return (r == 0 ? TRUE : FALSE);
}


BcmRet compMd_initTr69(const char **myNamespaces,
                       void **msgHandle, SINT32 *shmId, SINT32 *tr69cPid)
{
   CmsEntityId eid=EID_TR69_MD;
   const char *busName=TR69_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   const char *args="-c tr69";
   MdmInitConfig mdmConfig;
   BcmRet ret;

   myCompName = BDK_COMP_TR69;

   // Create the message bus for this component.
   ret = compMd_startMsgBus(args);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("could not launch bcm_msgd! ret=%d", ret);
      return ret;
   }

   // Connect to message bus and set myself as bus manager.
   ret = compMd_connectToMsgBus(eid, busName, isBusManager, msgHandle);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Could not init connection to %s, ret=%d", busName, ret);
      return ret;
   }

   // Initialize the MDM (+remote_objd support).
   // Only ssk in BDK sysmgmt, tr69_md, and usp_md are allowed to initialize
   // MDM with remote_objd support.
   memset(&mdmConfig, 0, sizeof(MdmInitConfig));
   mdmConfig.mdmLibName = "libmdm2_tr69.so";
   mdmConfig.remoteMdmLibName = "libmdm2.so";
   mdmConfig.shmSize = MAX_MDM_SHM_SIZE_TR69;
   mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_TR69;
   mdmConfig.lockKeyOffset = TR69_KEY_OFFSET;
   mdmConfig.eid = eid;
   mdmConfig.accessBit = NDA_ACCESS_BDK_MD;

   ret = (BcmRet) cmsMdm_initWithConfig(&mdmConfig, *msgHandle, shmId);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("MdmInit failed, ret=%d", ret);
      return ret;
   }

   // Register any namespaces
   if (myNamespaces != NULL)
   {
      UINT32 i=0;
      while (myNamespaces[i][0] != '\0')
      {
         bcmuLog_debug("registering namespace %s", myNamespaces[i]);
         ret = compMd_registerNamespace(*msgHandle, eid, myNamespaces[i]);
         if (ret != BCMRET_SUCCESS)
         {
            bcmuLog_error("Namespace registration %s failed, ret=%d",
                          myNamespaces[i], ret);
            return ret;
         }
         i++;
      }
   }

   // In CMS, these apps are launched on boot by smd.  But in BDK, we have
   // to launch them here.

   ret = compMd_launchTr69c(*shmId, 1, tr69cPid);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Launch of tr69c failed!, ret=%d", ret);
      return ret;
   }

#ifdef DMP_DEVICE2_XMPPBASIC_1
   ret = compMd_launchXmppc(*shmId);
   if (ret != BCMRET_SUCCESS)
   {
      bcmuLog_error("Launch of xmppc failed!, ret=%d", ret);
      return ret;
   }
#endif

   if (tr69_proxy_exists())
   {
      ret = compMd_launchTr69Proxy(*shmId);
      if (ret != BCMRET_SUCCESS)
      {
         bcmuLog_error("Launch of tr69_proxy failed!, ret=%d", ret);
         return ret;
      }
   }

   bcmuLog_notice("init done!");
   return BCMRET_SUCCESS;
}


void compMd_cleanupTr69()
{

   if (tr69ProxyPid > 0)
   {
      // terminate tr69_proxy in the same way as in CMS.
      prctl_terminateProcessForcefully(tr69ProxyPid);
      compMd_collectChild(tr69ProxyPid, 50);
   }

#ifdef DMP_DEVICE2_XMPPBASIC_1
   if (xmppcPid > 0)
   {
      // terminate xmppc in the same way as in CMS.
      prctl_terminateProcessForcefully(xmppcPid);
      compMd_collectChild(xmppcPid, 50);
   }
#endif

   if (tr69cPid > 0)
   {
      // terminate tr69c in the same way as in CMS.
      prctl_terminateProcessForcefully(tr69cPid);
      // give tr69c a little more time to clean up and exit (200ms)
      compMd_collectChild(tr69cPid, 200);
   }

   // At this point, tr69_md should be the last reference to the MDM, so after
   // this final cmsMdm_cleanup(), the MDM should be destroyed.
   cmsMdm_cleanup();

   // Even though we started bcm_msgd in compMd_initTr69, do not kill it here.
   // bcm_msgd will be killed in compMd_processMsgWithTimeout() after the last
   // reply message has been sent out.
   return;
}


BcmRet compMd_launchTr69c(SINT32 shmId, SINT32 bootLaunch, SINT32 *pid)
{
   const char *prefix = "/bin";
   char args[512]={0};
   BcmRet ret;

   if (shmId == UNINITIALIZED_SHM_ID)
      return BCMRET_INVALID_PARAM_VALUE;

   // The -d option tells tr69c that we are in Distributed MDM (BDK) mode.
   // The -x option tells tr69c that sysmgmt is ready, no need to wait 
   snprintf(args, sizeof(args), "%s-m %d -d -I 1", bootLaunch?"":"-x ", shmId);
   bcmuLog_notice("launch tr69c with args=%s", args);
   ret = compMd_launchApp(prefix, "tr69c", args, &tr69cPid);
   *pid = tr69cPid;
   return ret;
}

#ifdef DMP_DEVICE2_XMPPBASIC_1
BcmRet compMd_launchXmppc(SINT32 shmId)
{
   const char *prefix = "/bin";
   char args[512]={0};
   BcmRet ret;

   // The -d option tells xmppc that we are in Distributed MDM (BDK) mode.
   snprintf(args, sizeof(args), "-m %d -d", shmId);
   bcmuLog_notice("launch xmppc with args=%s", args);
   ret = compMd_launchApp(prefix, "xmppc", args, &xmppcPid);
   return ret;
}
#endif

BcmRet compMd_launchTr69Proxy(SINT32 shmId)
{
   const char *prefix = "/bin";
   char args[512]={0};
   BcmRet ret;

   snprintf(args, sizeof(args), "-m %d", shmId);
   bcmuLog_notice("launch tr69_proxy with args=%s", args);
   ret = compMd_launchApp(prefix, "tr69_proxy", args, &tr69ProxyPid);
   return ret;
}

