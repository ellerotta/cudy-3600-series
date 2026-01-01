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

#ifdef DMP_DEVICE2_LOCALAGENT_1

/*!\file mdm2_init_usp.c
 * \brief MDM initialization for PURE181 Device based tree,
 *        LocalAgent objects.
 *
 */

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "bcm_boardutils.h"


#ifdef DMP_DEVICE2_CONTROLLERTRUST_1
/*
 * FIXME: due to obuspa's limited role implementation, always keep
 * first two roles as TrustedRole and UntrustedRole by default
 */
static CmsRet mdm_addDefaultControllerTrustRoleObjects(void)
{
   CmsRet ret=CMSRET_SUCCESS;
   Dev2LocalagentControllertrustRoleObject *roleObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   UBOOL8 add=TRUE;

   if (mdm_getNextObject(MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE, &iidStack,
                         (void **)&roleObj) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &roleObj);
      add = FALSE;
   }

   if (add)
   {
      MdmPathDescriptor pathDesc;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE;
      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL))!=CMSRET_SUCCESS)
      {
         cmsLog_error("could not add ControllerTrust role, ret<%d>", ret);
         goto out;
      }

      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack,
                               (void **)&roleObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ControllerTrust role Obj, ret<%d>", ret);
         goto out;
      }

      CMSMEM_REPLACE_STRING_FLAGS(roleObj->name, "TrustedRole",
                                  mdmLibCtx.allocFlags);
      if ((ret = mdm_setObject((void **)&roleObj, &pathDesc.iidStack,
                               FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ControllerTrust role Obj, ret<%d>", ret);
         cmsObj_free((void **)&roleObj);
         goto out;
      }

      cmsObj_free((void **)&roleObj);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_LOCALAGENT_CONTROLLERTRUST_ROLE;
      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL))!=CMSRET_SUCCESS)
      {
         cmsLog_error("could not add ControllerTrust role, ret<%d>", ret);
         goto out;
      }

      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack,
                               (void **)&roleObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ControllerTrust role Obj, ret<%d>", ret);
         goto out;
      }

      CMSMEM_REPLACE_STRING_FLAGS(roleObj->name, "UntrustedRole",
                                  mdmLibCtx.allocFlags);
      if ((ret = mdm_setObject((void **)&roleObj, &pathDesc.iidStack,
                               FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ControllerTrust role Obj, ret<%d>", ret);
      }

      cmsObj_free((void **)&roleObj);
   }

out:
   return ret;
}
#endif  /* DMP_DEVICE2_CONTROLLERTRUST_1 */

static CmsRet mdm_addDefaultUspAgentObjects(void)
{
   CmsRet ret=CMSRET_SUCCESS;
   Dev2LocalagentObject *agentObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char buf[BUFLEN_16]={0};
   UINT8 macAddr[6]={0};
   char endpointID[BUFLEN_48]={0};
   char supportedProtocols[BUFLEN_1024]={0};

   ret = mdm_getObject(MDMOID_DEV2_LOCALAGENT, &iidStack, (void **)&agentObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Get LOCALAGENT object failed. ret=%d", ret);
      return ret;
   }

   /* endpointID: "os::<oui>-<serialnumber>"
    * serialnumber is the base mac addr of the board.
    * oui is the first 3 bytes of the mac addr.
    */
   devCtl_getBaseMacAddress(macAddr);
   sprintf(buf, "%02x%02x%02x%02x%02x%02x", 
           (unsigned char) macAddr[0], (unsigned char) macAddr[1],
           (unsigned char) macAddr[2], (unsigned char) macAddr[3],
           (unsigned char) macAddr[4], (unsigned char) macAddr[5]);

   snprintf(endpointID, sizeof(endpointID), "os::%.6s-%s", buf, buf);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(agentObj->endpointID, endpointID,
                                     mdmLibCtx.allocFlags);

#ifdef DMP_DEVICE2_WEBSOCKETAGENT_1
   strcat(supportedProtocols, MDMVS_WEBSOCKET",");
#endif
#ifdef DMP_DEVICE2_STOMPAGENT_1
   strcat(supportedProtocols, MDMVS_STOMP",");
#endif
#ifdef DMP_DEVICE2_MQTTCLIENTBASE_1
   strcat(supportedProtocols, MDMVS_MQTT",");
#endif

   /* must support at least one protocol */
   if (strlen(supportedProtocols) == 0)
   {
      cmsLog_error("Local agent must support at least one protocol");
      ret = CMSRET_INTERNAL_ERROR;
      goto out;
   }
   /* get rid of the last ',' */
   supportedProtocols[strlen(supportedProtocols)-1] = '\0';

   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(agentObj->supportedProtocols, supportedProtocols,
                                     mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **)&agentObj, &iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set LOCALAGENT object failed, ret=%d", ret);
   }

#ifdef DMP_DEVICE2_CONTROLLERTRUST_1
   ret = mdm_addDefaultControllerTrustRoleObjects();
#endif

out:
   mdm_freeObject((void **)&agentObj);
   return ret;

}  /* End of mdm_addDefaultUspAgentObjects() */


/** This is the "weak" mdm_adjustForHardware_dev2.
 *  It will only be called in a distributed MDM build in the devinfo component.
 */
#pragma weak mdm_adjustForHardware_dev2
CmsRet mdm_adjustForHardware_dev2()
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Distributed MDM build: adjustForHardware for USP only");

   mdm_addDefaultUspAgentObjects();

   return ret;
}


#endif  /* DMP_DEVICE2_LOCALAGENT_1 */
