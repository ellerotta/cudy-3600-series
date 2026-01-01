/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom 
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

/*!\file comp_devinfo_md.c
 * \brief High level functions used by devinfo_md.
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include "bcm_ulog.h"
#include "cms_mdm.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "bcm_comp_md.h"
#include "comp_md_private.h"


BcmRet compMd_initDevinfo(const char **myNamespaces,
                          void **msgHandle, SINT32 *shmId)
{
   CmsEntityId eid=EID_DEVINFO_MD;
   const char *busName=DEVINFO_MSG_BUS;
   UBOOL8 isBusManager=TRUE;
   const char *args="-c devinfo";
   MdmInitConfig mdmConfig;
   BcmRet ret;

   myCompName = BDK_COMP_DEVINFO;

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

   // Initialize the MDM.
   memset(&mdmConfig, 0, sizeof(MdmInitConfig));
   mdmConfig.mdmLibName = "libmdm2_devinfo.so";
   mdmConfig.shmSize = MAX_MDM_SHM_SIZE_DEVINFO;  // probably don't need this much.
   mdmConfig.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR_DEVINFO;
   mdmConfig.lockKeyOffset = DEVINFO_KEY_OFFSET;
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

   bcmuLog_notice("init done!");
   return BCMRET_SUCCESS;
}
