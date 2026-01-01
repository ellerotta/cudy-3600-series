/*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
*    All Rights Reserved
*
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
*
*  Except as expressly set forth in the Authorized License,
*
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
*
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
*
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/


#include "cms.h"
#include "cms_params.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"
#include "bcm_flashutil.h"
#include "linux/rut_pmap.h"
#include "linux/rut_lan.h"
#include "prctl.h"
#include "genutil_rbtree.h"
#include "sysutil.h"


#if defined(DMP_X_BROADCOM_COM_RDPA_1)
#include "ethswctl_api.h"
#endif

#undef OMCID_CREATE_ON_SET

CmsRet mdm_addDefaultLanObjects(void);

// in mdm_binaryHelper.c
void mdm_initFullWriteAccessEidArray();

#ifdef DMP_WIFILAN_1
CmsRet mdm_adjustWlanAdapter(void);
#endif

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
CmsRet addDefaultWanWifiObject(void);
#endif

#ifdef DMP_ETHERNETWAN_1
static CmsRet addDefaultWanEthObject(void);
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
CmsRet mdm_addDefaultWanGponObjects(void);
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
CmsRet mdm_addDefaultWanEponObject(void);
#endif

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
CmsRet mdm_addDefaultL2tpVpnObjects(void);
#endif

#ifdef DMP_X_BROADCOM_COM_PPTPAC_1
CmsRet mdm_addDefaultPptpVpnObjects(void);
#endif

#ifdef DMP_BASELINE_1
CmsRet mdm_initRoot_igd(void);
#endif

#ifdef DMP_BRIDGING_1
static UBOOL8 undoDynamicMappings(void);
#endif



#ifdef DMP_DEVICE2_SM_BASELINE_1
CmsRet mdm_addDefaultModSwObjects(void);
#else
void mdm_removeBeepDatabase(void);
#endif

#if defined(DMP_X_BROADCOM_COM_BMU_1)
CmsRet mdm_addDefaultBmuObject(void);
#endif

#ifdef BUILD_CUSTOMER
CmsRet mdm_initCtObject(void);
#endif

UBOOL8 cmsMdm_isInitialized(void)
{

   /* this function only needs to run once */
   if (mdmLibCtx.initDone)
      return TRUE;
   else
      return FALSE;

}


CmsRet cmsMdm_init(CmsEntityId eid, void *msgHandle, SINT32 *shmId)
{
   const CmsEntityInfo *eInfo;

   if ((eInfo = cmsEid_getEntityInfo(eid)) == NULL)
   {
      cmsLog_error("invalid eid %d", eid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return cmsMdm_initWithAcc(eid, eInfo->accessBit, msgHandle, shmId);
}

CmsRet cmsMdm_initWithAcc(CmsEntityId eid, UINT32 acc, void *msgHandle, SINT32 *shmId)
{
   MdmInitConfig config;

   memset(&config, 0, sizeof(config));
   config.eid = eid;
   config.mgmtEid = EID_SMD;
   config.intfStackEid = EID_SSK;
   config.accessBit = acc;
   config.shmAttachAddr = (void *) MDM_SHM_ATTACH_ADDR;
   config.shmSize = 0;  // let cmsMdm_initWithConfig calculate dynamically
   config.useClassicConfigFile = TRUE;

   return (cmsMdm_initWithConfig(&config, msgHandle, shmId));

}

CmsRet cmsMdm_initWithConfig(const MdmInitConfig *config, void *msgHandle, SINT32 *shmId)
{
   CmsRet ret;
   InstanceIdStack iidStack;
   UINT32 shmSize;

#ifdef MDM_SHARED_MEM
   UBOOL8 attachExisting = (*shmId != UNINITIALIZED_SHM_ID);
   mdmLibCtx.allocFlags |= ALLOC_SHARED_MEM;
#endif

   if (config->eid != GENERIC_EID(config->eid))
   {
      cmsLog_error("EID must be a generic EID (expected 0x%x, got 0x%x)",
                   GENERIC_EID(config->eid), config->eid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (config->eid == EID_SMD && msgHandle != NULL)
   {
      cmsLog_error("smd must not pass in msgHandle to MDM, may result in deadlock");
      return CMSRET_INVALID_ARGUMENTS;
   }
   else if (config->eid != EID_SMD && msgHandle == NULL)
   {
      cmsLog_error("WARNING: msgHandle is NULL (eid=%d), dangerous but allow for now.", config->eid);
   }

   if (MAX_ACTUAL_MDM_INSTANCE_DEPTH > MAX_MDM_INSTANCE_DEPTH)
   {
      cmsLog_error("actual MDM instance depth=%d, but binary-only objects were compiled with depth=%d; "
                   "binary-only objects need to be recompiled.",
                   MAX_ACTUAL_MDM_INSTANCE_DEPTH, MAX_MDM_INSTANCE_DEPTH );
      return CMSRET_RESOURCE_EXCEEDED;
   }


   if (MAX_ACTUAL_MDM_PARAM_NAME_LENGTH > MAX_MDM_PARAM_NAME_LENGTH)
   {
      /* actually, mdm.o is not affected by this, but tr69c is */
      cmsLog_error("actual MDM max param name length=%d, but binary-only objects were compiled with length=%d; "
                   "binary-only objects need to be recompiled.",
                   MAX_ACTUAL_MDM_PARAM_NAME_LENGTH, MAX_MDM_PARAM_NAME_LENGTH );
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if (MDM_ALIAS_BUFLEN > MAX_MDM_ALIAS_BUFLEN)
   {
      cmsLog_error("actual MDM alias buflen=%d, but software was compiled with length=%d; "
                   "fix and recompile.",
                   MDM_ALIAS_BUFLEN, MAX_MDM_ALIAS_BUFLEN);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if (MDM_SINGLE_FULLPATH_BUFLEN > MAX_MDM_SINGLE_FULLPATH_BUFLEN)
   {
      cmsLog_error("actual MDM single fullpath buflen=%d, but software was compiled with length=%d; "
                   "fix and recompile.",
                   MDM_SINGLE_FULLPATH_BUFLEN, MAX_MDM_SINGLE_FULLPATH_BUFLEN);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if (MDM_MULTI_FULLPATH_BUFLEN > MAX_MDM_MULTI_FULLPATH_BUFLEN)
   {
      cmsLog_error("actual MDM multi fullpath buflen=%d, but software was compiled with length=%d; "
                   "fix and recompile.",
                   MDM_MULTI_FULLPATH_BUFLEN, MAX_MDM_MULTI_FULLPATH_BUFLEN);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* this function only needs to run once */
   if (mdmLibCtx.initDone)
   {
      return CMSRET_SUCCESS;
   }

   /*
    * Start of one time only initialization.
    */
   mdmLibCtx.initDone = TRUE;

   cmsLog_notice("entered, eid=%d acc=0x%x shmid=%d",
                 config->eid, config->accessBit, *shmId);

   mdm_initFullWriteAccessEidArray();

   if (config->shmSize == 0)
   {
   // CMS Classic should use this approach to calculate the Monolithic MDM shmSize.
   shmSize = MDM_SHM_BASE_SIZE;
#ifdef BRCM_WLAN
   shmSize += MDM_SHM_WLAN_EXTRA;

#ifdef BUILD_WLDATAELD
   shmSize += MDM_SHM_WLAN_DATAELD_EXTRA;
#endif

   /**
    * EAP platform will require more shared memory to support 512 clients.
    */
#ifdef BCA_HND_EAP
   shmSize += MDM_SHM_WLAN_EXTRA;
#endif /* BCA_HND_EAP */
#endif /* BRCM_WLAN */
#ifdef BRCM_VOICE_SUPPORT
   shmSize += MDM_SHM_VOIP_EXTRA;
#endif
#ifdef DMP_X_ITU_ORG_GPON_1
   shmSize += MDM_SHM_GPON_EXTRA;
#endif
#if defined(DMP_DSLDIAGNOSTICS_1) || defined(DMP_VDSL2WAN_1)
   shmSize += MDM_SHM_DSL_BIG_DATA_EXTRA;
#endif
#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   shmSize += MDM_SHM_DUAL_TR069C_EXTRA;
#endif
   }
   else
   {
      // BDK code should pass in the Distributed MDM shmSize.
      shmSize = config->shmSize;
   }

   /*
    * oal_sharedMemInit creates, attaches, copies and does
    * everything needed to make the shared memory ready for use by the MDM.
    * If CMS_SHARED_MEM is not defined, this function will do nothing.
    */
   if ((ret = oalShm_init(config, shmSize, shmId, &(mdmLibCtx.shmAddr))) != CMSRET_SUCCESS)
   {
      return ret;
   }

   mdmLibCtx.eid = config->eid;
   mdmLibCtx.accessBit = config->accessBit;
   mdmLibCtx.pid = cmsEid_getPid();
   mdmLibCtx.msgHandle = msgHandle;
   mdmLibCtx.shmId = *shmId;

   // Automatically register the current thread and msgHandle.  Multi-threaded
   // apps which have multiple threads accessing the MDM with multiple
   // msgHandles can modify the bindings with cmsMdm_registerThreadMsgHandle().
   if (msgHandle != NULL)
   {
      cmsMdm_registerThreadMsgHandle(TRUE, msgHandle);
   }

   /* mwang_todo: if error is detected after this point, we should
    * properly clean up shared mem state before returning.
    */

   if ((ret = odl_init()) != CMSRET_SUCCESS)
   {
      return ret;
   }


#ifdef MDM_SHARED_MEM
   if (attachExisting)
   {
      /* don't need to do the rest if we are not the initial creator of shm */
      cmsLog_notice("attach existing done, ret=%d", ret);
      return ret;
   }
#endif

   /*
    * Everything below this point is done only by the first caller to
    * cmsMdm_init.
    */
   rbtree_initFlags(&mdmShmCtx->typesRBT, RBT_FLAGS_STRCMP, NULL, &mdmShmCtx->nullNode);
   rbtree_initFlags(&mdmShmCtx->profilesRBT, RBT_FLAGS_STRCMP, NULL, &mdmShmCtx->nullNode);

   // Set component name in mdmShmCtx if we are in Distributed MDM mode
   if (config->useClassicConfigFile == FALSE)
   {
      const char *busName, *compName;

      busName = cmsMsg_getBusName(msgHandle);
      cmsAst_assert(busName != NULL);

      compName = cmsMsg_busNameToComponentName(busName);
      cmsAst_assert(compName != NULL);

      snprintf(mdmShmCtx->compName, sizeof(mdmShmCtx->compName), "%s", compName);
      cmsLog_notice("compName=%s", mdmShmCtx->compName);
   }
   else
   {
      // CMS Classic, no component name.
      mdmShmCtx->compName[0] = '\0';
   }

   if (config->remoteMdmLibName)
   {
      cmsLog_notice("calling startRemoteObjd compName=%s", mdmShmCtx->compName);
      if ((ret = odl_startRemoteObjd(mdmShmCtx->compName)) != CMSRET_SUCCESS)
      {
         return ret;
      }
      mdmShmCtx->isRemoteCapable = TRUE;
   }

   // The caller who initializes the MDM sets the intfStackEid.  Subsequent
   // callers do not need to set it and their setting is ignored.
   mdmShmCtx->intfStackEid = config->intfStackEid;

   // Copy over the values if set.
   if (config->multiCompFirstInstanceId > 0)
      mdmShmCtx->multiCompFirstInstanceId = config->multiCompFirstInstanceId;
   if (config->multiCompLastInstanceId > 0)
      mdmShmCtx->multiCompLastInstanceId = config->multiCompLastInstanceId;


   mdm_initParentPointers(mdmShmCtx->rootObjNode);

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_createSubTree(mdmShmCtx->rootObjNode, 0, &iidStack, NULL, NULL)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   // mdm_loadConfig can handle CMS classic monolithic config file or
   // Distributed MDM config files.
   if ((ret = mdm_loadConfig()) != CMSRET_SUCCESS)
   {
      cmsLog_error("Config file loading failed, ret=%d", ret);
      return ret;
   }


   // Nobody is accessing MDM yet, but lock all zones anyways to be safe.
   // Seems like filesystem formatting can be triggered in this step, so
   // increase lock hold warning threshold to 45 seconds.
   // Longer timeout also needed for DEBUG_KASAN builds on slower SOC's.
   cmsLck_acquireAllZoneLocksWithBackoff(0, CMSLCK_MAX_HOLDTIME);
   cmsLck_setHoldTimeWarnThresh(45 * MSECS_IN_SEC);
   ret = mdm_adjustForHardware();
   cmsLck_releaseAllZoneLocks();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Adjust for Hardware failed!!  ret=%d", ret);
      return ret;
   }


   /*
    * The next step, activating objects, may cause apps to get launched.
    * Those apps may want to attach to the MDM.  So we better send the shmid
    * to smd now.
    */
   cmsLog_debug("ready to send out shmId %d (to %d msgHandle %p)",
                *shmId, config->mgmtEid, mdmLibCtx.msgHandle);
   if (mdmLibCtx.msgHandle != NULL && config->mgmtEid != EID_INVALID)
   {
      CmsMsgHeader msg=EMPTY_MSG_HEADER;
      msg.src = config->eid;
      msg.dst = config->mgmtEid;
      msg.type = CMS_MSG_SHMID;
      msg.flags_event = 1;
      msg.wordData = *shmId;
      cmsMsg_send(mdmLibCtx.msgHandle, &msg);
   }


   /*
    * now call the RCL handler funcs to activate the config.
    * Activating an object will cause the RCL handler function to get called,
    * which may launch an app which will try to access the MDM.  So lock all
    * zones during object activation and allow ourselves to hold the locks a
    * bit longer during initialization.
    * Increase lock hold warning threshold to 45 seconds for DEBUG_KASAN builds.
    */
   cmsLog_notice("Activating objects.  Lock all zones...");
   cmsLck_acquireAllZoneLocksWithBackoff(0, CMSLCK_MAX_HOLDTIME);
   cmsLck_setHoldTimeWarnThresh(45 * MSECS_IN_SEC);

   mdmShmCtx->inMdmInit = TRUE;

   INIT_INSTANCE_ID_STACK(&iidStack);
   mdm_activateObjects(mdmShmCtx->rootObjNode, &iidStack);

   mdmShmCtx->inMdmInit = FALSE;


#ifdef DMP_BRIDGING_1
   /*
    * If we had previously moved an interface from one bridge to another
    * because of the DHCP vendor id mechanism, we need to move the interface
    * back to its original bridge on bootup.  Do this after activate objects
    * because unDoDynamicMappings will call RCL handler functions, which should
    * only be done after activate objects.
    */
   undoDynamicMappings();
#endif

   // Just go ahead and save the config, in case we changed anything.
   // Note we are using saveLocalConfigToFlash.  In CMS mode, this saves the
   // entire MDM to config file.  In BDK mode, this only saves the local
   // Distributed MDM to config file.
   cmsMgm_saveLocalConfigToFlash();

   cmsLck_releaseAllZoneLocks();
   cmsLog_notice("Done activating objects, unlocked all zones, "
                 "first attach init ret=%d", ret);

   if (mdmLibCtx.msgHandle != NULL)
   {
      CmsMsgHeader msg=EMPTY_MSG_HEADER;
      // Let ssk know all intfStack messages have been sent out.
      // The src is usually ssk, which means both src and dest are ssk.
      // This is a bit odd, but is correct and it works.
      msg.src = config->eid;
      msg.dst = mdmShmCtx->intfStackEid;
      msg.type = CMS_MSG_MDM_INIT_DONE;
      msg.flags_event = 1;
      cmsMsg_send(mdmLibCtx.msgHandle, &msg);
   }

   cmsLog_notice("full init done, ret=%d", ret);
   return ret;
}


void cmsMdm_cleanup()
{
   cmsLog_notice("entered");

   odl_cleanup();

   oalShm_cleanup(mdmLibCtx.shmId, mdmLibCtx.shmAddr);

   mdmLibCtx.initDone = FALSE;

   cmsLog_notice("done");
}


CmsRet cmsMdm_registerThreadMsgHandle(UBOOL8 add, void *msgHandle)
{
   SINT32 threadId = sysUtl_getThreadId();
   UINT32 i;

   // Look for existing entry by threadId.
   for (i=0; i < MDM_LIB_CTX_NUM_THREAD_SLOTS; i++)
   {
      if (mdmLibCtx.threadCtx[i].threadId == threadId)
      {
         if (add)
         {
            cmsLog_debug("replace existing [%d] threadId %d msgHandle %p -> %p",
                         i, threadId,
                         mdmLibCtx.threadCtx[i].msgHandle, msgHandle);
            mdmLibCtx.threadCtx[i].msgHandle = msgHandle;
         }
         else
         {
            // delete the registration.
            mdmLibCtx.threadCtx[i].threadId = 0;
            mdmLibCtx.threadCtx[i].msgHandle = NULL;
         }
         return CMSRET_SUCCESS;
      }
   }

   // Look for existing entry by msgHandle.
   for (i=0; i < MDM_LIB_CTX_NUM_THREAD_SLOTS; i++)
   {
      if (mdmLibCtx.threadCtx[i].msgHandle == msgHandle)
      {
         if (add)
         {
            cmsLog_debug("replace existing [%d] msgHandle %p owner threadId %d -> %d",
                         i, msgHandle,
                         mdmLibCtx.threadCtx[i].threadId, threadId);
            mdmLibCtx.threadCtx[i].threadId = threadId;
         }
         else
         {
            // delete the registration.
            mdmLibCtx.threadCtx[i].threadId = 0;
            mdmLibCtx.threadCtx[i].msgHandle = NULL;
         }
         return CMSRET_SUCCESS;
      }
   }

   if (!add)
   {
      // If this was a delete, and we get here, it means we did not find
      // an entry.
      return CMSRET_OBJECT_NOT_FOUND;
   }

   // This must be an add operation, look for empty slot.
   // TODO: add locking?  In theory, two threads could both find the same
   // empty slot and try to fill it.  But in practice, this is an extremely
   // rare operation, usually done at startup time by just one other thread.
   for (i=0; i < MDM_LIB_CTX_NUM_THREAD_SLOTS; i++)
   {
      if (mdmLibCtx.threadCtx[i].threadId == 0)
      {
         mdmLibCtx.threadCtx[i].threadId = threadId;
         mdmLibCtx.threadCtx[i].msgHandle = msgHandle;
         cmsLog_debug("add new [%d] threadId %d msgHandle %p",
                      i, threadId, msgHandle);
         return CMSRET_SUCCESS;
      }
   }

   cmsLog_error("MdmLibCtx.threadCtx full, max slots=%d",
                MDM_LIB_CTX_NUM_THREAD_SLOTS);
   return CMSRET_RESOURCE_EXCEEDED;
}


void *cmsMdm_getThreadMsgHandle()
{
   SINT32 threadId = sysUtl_getThreadId();
   UINT32 i;

   if (mdmLibCtx.eid == EID_SMD)
   {
      // smd does not register a msgHandle.  So just return mdmLibCtx.msgHandle,
      // which should be a NULL.
      return mdmLibCtx.msgHandle;
   }

   for (i=0; i < MDM_LIB_CTX_NUM_THREAD_SLOTS; i++)
   {
      if (mdmLibCtx.threadCtx[i].threadId == threadId)
      {
         return (mdmLibCtx.threadCtx[i].msgHandle);
      }
   }

   // No thread specific registration found, just return the app wide msgHandle.
   // This is still ok as long as the second thread is only sending out
   // notification messages, which does not expect a response.
   cmsLog_notice("WARNING: multiple threads accessing MDM but using same CMS msgHandle, "
                 "this could cause unexpected CMS msg behavior.  "
                 "Current threadId=%d (app threadId=%d)",
                 threadId, mdmLibCtx.threadCtx[0].threadId);
   return mdmLibCtx.msgHandle;
}


#ifndef SUPPORT_DM_PURE181
CmsRet mdm_adjustForHardware_igd(void)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Entered===>");

#ifdef DMP_BASELINE_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_initRoot_igd();  // equivalent to mdm_initFirmwareImageObjects_dev2
   }
#endif

#ifdef DMP_ADSLWAN_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultWanDslObjects();
   }
#endif


#ifdef DMP_ETHERNETWAN_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = addDefaultWanEthObject();
   }
#endif


#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   if (ret == CMSRET_SUCCESS)
   {
      mdm_addDefaultL2tpVpnObjects();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_PPTPAC_1
   if (ret == CMSRET_SUCCESS)
   {
      mdm_addDefaultPptpVpnObjects();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
   if (ret == CMSRET_SUCCESS)
   {
      mdm_addDefaultWanGponObjects();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
   if (ret == CMSRET_SUCCESS)
   {
      mdm_addDefaultWanEponObject();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = addDefaultWanWifiObject();
   }
#endif

   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultLanObjects();
   }

#ifdef DMP_WIFILAN_1
   if ( ret == CMSRET_SUCCESS )
		ret = mdm_adjustWlanAdapter();
#endif


#ifdef BRCM_VOICE_SUPPORT
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_adjustForVoiceHardware();
   }
#endif

#ifdef DMP_STORAGESERVICE_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultStorageServiceObject();
   }
#endif

  /*  This section contains only TR_181 data model member (aka DEVICE_2) under the InternetGatewayDevice.
   * a hybrid mode with InternetGatewayDevice.Device.xxxx
   */

#ifdef DMP_DEVICE2_SM_BASELINE_1
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwObjects();
   }
#else
   mdm_removeBeepDatabase();
#endif

#ifdef BUILD_CUSTOMER
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_initCtObject();
   }
#endif

#if defined(DMP_DEVICE2_BASELINE_1) && defined(DMP_DEVICE2_ROUTING_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1) //for hybrid IPv6
   ret = mdm_initRouterObject_dev2();
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
#endif

#if defined(DMP_X_BROADCOM_COM_BMU_1)
   if (ret == CMSRET_SUCCESS)
   {
      ret =  mdm_addDefaultBmuObject();
   }
#endif

   cmsLog_notice("Done, ret=%d", ret);

   return ret;
}
#endif  /* SUPPORT_DM_PURE181 */


#ifdef DMP_ETHERNETWAN_1
CmsRet addDefaultWanEthObject(void)
{
   void *mdmObj=NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanCommonIntfCfgObject *wanCommonObj = NULL;
   UINT32 added = 0;
   CmsRet ret;

   /*
    * User has selected Ethernet as a WAN interface.  See if there is aleady an Ethernet
    * WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_ETHERNET);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding Ethernet WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_ETHERNET);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
         return ret;
      }

      if ((ret = mdm_getObject(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **) &wanCommonObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_ETHERNET, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
      }

   }
   else
   {
      /* Ethernet WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}
#endif /* DMP_ETHERNETWAN_1 */


void mdm_increaseWanDeviceCount(UINT32 added)
{
   _IGDObject *igdObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = mdm_getObject(MDMOID_IGD, &iidStack, (void **) &igdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get IGD object, ret=%d", ret);
      return;
   }

   igdObj->WANDeviceNumberOfEntries += added;

   if ((ret = mdm_setObject((void **) &igdObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IGD object, ret=%d", ret);
   }

   return;
}


void mdm_activateObjects(const MdmObjectNode *objNode,
                         const InstanceIdStack *iidStack)
{
   InstanceHeadNode *instHead=NULL;
   InstanceDescNode *instDesc=NULL;
   void *mdmObj=NULL;
   UBOOL8 callRcl=FALSE;
   UINT32 i;
   CmsRet ret;

   if (IS_INDIRECT0(objNode))
   {
      mdmObj = objNode->objData;
      callRcl = TRUE;
   }
   else if (IS_INDIRECT1(objNode))
   {
      instHead = mdm_getInstanceHead(objNode, iidStack);
      cmsAst_assert(instHead != NULL);

      mdmObj = instHead->objData;
      callRcl = TRUE;
   }
   else if ((IS_INDIRECT2(objNode)) &&
            (DEPTH_OF_IIDSTACK(iidStack) == objNode->instanceDepth))
   {
      instDesc = mdm_getInstanceDescFromObjNode(objNode, iidStack);
      cmsAst_assert(instDesc != NULL);

      mdmObj = instDesc->objData;
      callRcl = TRUE;
   }
   else if ((IS_INDIRECT2(objNode)) &&
            (DEPTH_OF_IIDSTACK(iidStack) + 1 == objNode->instanceDepth))
   {
      instHead = mdm_getInstanceHead(objNode, iidStack);
      cmsAst_assert(instHead != NULL);

      instDesc = (InstanceDescNode *) instHead->objData;
      while (instDesc != NULL)
      {
         InstanceIdStack subTreeIidStack = *iidStack;

         PUSH_INSTANCE_ID(&subTreeIidStack, instDesc->instanceId);

         mdm_activateObjects(objNode, &subTreeIidStack);

         instDesc = instDesc->next;
      }

      return;
   }


   /*
    * Call the Rcl handler function to let it know this object is present
    * in the MDM tree.
    */
   if (callRcl)
   {
      void *newMdmObj=NULL;
      const MdmOidInfoEntry *oidInfo;

      if (mdmObj == NULL)
      {
         cmsLog_error("No mdmObj for %s oid=%d iidStack=%s",
                      objNode->name, objNode->oid,
                      cmsMdm_dumpIidStack(iidStack));
      }

      if ((oidInfo = mdm_getOidInfo(objNode->oid)) == NULL)
      {
         cmsLog_error("Could not find OID info for oid %d", objNode->oid);
      }

      if ((mdmObj != NULL) &&
          (oidInfo != NULL) &&
          (oidInfo->rclHandlerFunc != NULL))
      {
         cmsLog_debug("calling rcl handler for %s oid=%d iidStack=%s",
                       objNode->name, objNode->oid,
                       cmsMdm_dumpIidStack(iidStack));

         /*
          * Protect the internal MDM object by duping it and passing the duped
          * object to odl_setObjectInternal.  This is inefficient since
          * odl_setObjectInternal will dup it again, but this is the safest
          * way to do it.
          */
         if ((newMdmObj = mdm_dupObject(mdmObj, mdmLibCtx.allocFlags)) == NULL)
         {
            cmsLog_error("dup failed! %s oid=%d not activated!",
                         objNode->name, objNode->oid);
         }
         else
         {
            // autoLock this zone before calling odl_setObjectInternal to make
            // the entryCount bookkeeping come out right.
            ret = lck_autoLockZone(objNode->oid, __FUNCTION__);
            if (ret != CMSRET_SUCCESS) 
            {
               cmsLog_error("lck_autoLockZone failed! oid=%d ", objNode->oid);
            }
            
            if ((ret = odl_setObjectInternal(newMdmObj, NULL, iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rcl handler reports error=%d on %s oid=%d iidStack=%s",
                            ret, objNode->name, objNode->oid,
                            cmsMdm_dumpIidStack(iidStack));
            }

            lck_autoUnlockZone(objNode->oid, __FUNCTION__);

            /* odl_setObjectInternal does not steal the newMdmObj, so we have to free it */
            mdm_freeObject(&newMdmObj);
         }
      }
   }



   /* now recurse to children objects */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      mdm_activateObjects(&(objNode->childObjNodes[i]), iidStack);
   }


   return;
}


#ifdef DMP_BASELINE_1

// The only purpose of this function is to check if we are using a new MDM (as
// a result of no config file) and COMMIT the current image.  This is needed
// in the scenario where we are running a BDK or CMS PURE181 image, and then
// we load this new (CMS Hybrid98 or Legacy98) image and marked this new image
// as BOOT_ONCE, but when we boot to this new image, we lost the config file
// (not compatible), but we still need to COMMIT this image.
CmsRet mdm_initRoot_igd(void)
{
   IGDObject *igdObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_notice("Entered:");

   // Load root InternetGatewayDevice object.
   ret = mdm_getObject(MDMOID_IGD, &iidStack, (void **) &igdObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("InternetGatewayDevice object is missing!");
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("LANDeviceNumberOfEntries=%d WANDeviceNumberOfEntries=%d",
                igdObj->LANDeviceNumberOfEntries,
                igdObj->WANDeviceNumberOfEntries);
   if (igdObj->LANDeviceNumberOfEntries == 0 && 
       igdObj->WANDeviceNumberOfEntries == 0)
   {
      UINT32 activePart = getBootPartition();

      printf("FirmwareSelect: COMMIT partition %d "
             "(due to new MDM/no config file)\n", activePart);
      setBootImageState((activePart == 1) ?
                        BOOT_SET_PART1_IMAGE : BOOT_SET_PART2_IMAGE);
   }

   mdm_freeObject((void **) &igdObj);
   return ret;
}


/* This stuff will have to be revisited for TR181, for now, keep it in TR98 only */

#ifdef DMP_BRIDGING_1

UBOOL8 undoDynamicMappings(void)
{
   InstanceIdStack filterIidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack availIntfIidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lanIidStack=EMPTY_INSTANCE_ID_STACK;
   L2BridgingFilterObject *filterObj=NULL;
   L2BridgingIntfObject *availIntfObj=NULL;
   void *lanObj=NULL;
   LanEthIntfObject *ethIntfObj;
   char standardBridgeIfName[CMS_IFNAME_LENGTH];
   char currBridgeIfName[CMS_IFNAME_LENGTH];
   char pathName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 changed=FALSE;
   UINT32 availIntfKey;
   CmsRet ret;

   while (CMSRET_SUCCESS == cmsObj_getNextFlags(MDMOID_L2_BRIDGING_FILTER, &filterIidStack, OGF_NO_VALUE_UPDATE, (void **) &filterObj))
   {
      if ((filterObj->sourceMACFromVendorClassIDFilter == NULL) &&
          (cmsUtl_strcmp(filterObj->filterInterface, MDMVS_LANINTERFACES)) &&
          (CMSRET_SUCCESS == cmsUtl_strtoul(filterObj->filterInterface, NULL, 10, &availIntfKey)))
      {
         ret = rutPMap_getAvailableInterfaceByKey(availIntfKey, &availIntfIidStack, &availIntfObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not find availIntf for key=%d", availIntfKey);
         }
         else
         {
            rutPMap_availableInterfaceReferenceToMdmObject(availIntfObj->interfaceReference, &lanIidStack, &lanObj);

            if (lanObj && (GET_MDM_OBJECT_ID(lanObj) == MDMOID_LAN_ETH_INTF))
            {
               /*
                * we only support undoing the mapping of ethernet objects for now.
                */
               ethIntfObj = (LanEthIntfObject *) lanObj;
               cmsLog_debug("do I need to undo available eth interface %s?", ethIntfObj->X_BROADCOM_COM_IfName);

               /* find out which bridge this eth interface is currently under */
               memset(currBridgeIfName, 0, sizeof(currBridgeIfName));
               rutLan_getParentBridgeIfNameOfEth(ethIntfObj->X_BROADCOM_COM_IfName, currBridgeIfName);

               /* find out which bridge this eth interface really belongs to (according to the filter object) */
               sprintf(standardBridgeIfName, "br%d", filterObj->filterBridgeReference);

               /* compare the two bridge names.  If different, have to move it. */
               if (cmsUtl_strcmp(currBridgeIfName, standardBridgeIfName)
#ifdef SUPPORT_LANVLAN
                  && filterObj->filterBridgeReference >=0 && (filterObj->X_BROADCOM_COM_VLANIDFilter == 0)
#endif
                  )
               {
                  rutLan_moveEthInterface(ethIntfObj->X_BROADCOM_COM_IfName, currBridgeIfName, standardBridgeIfName);

                  /* update the available interface reference path again */
                  rutPMap_lanIfNameToAvailableInterfaceReference(ethIntfObj->X_BROADCOM_COM_IfName, pathName);
                  pathName[strlen(pathName)-1] = '\0';

                  CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceReference, pathName, mdmLibCtx.allocFlags);

                  cmsLog_debug("changing availIntf->interfaceReference to %s", availIntfObj->interfaceReference);

                  if ((ret = cmsObj_set(availIntfObj, &availIntfIidStack)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("update of availintf object failed, ret=%d", ret);
                  }

                  changed = TRUE;
               }
            }

            cmsObj_free((void **) &lanObj);

            cmsObj_free((void **)&availIntfObj);
         }
      }

      cmsObj_free((void **) &filterObj);
   }

   return changed;
}

#endif /* DMP_BRIDGING_1 */

#endif /* DMP_BASELINE_1 */


#ifdef CMS_CONFIG_COMPAT

#ifdef DMP_QOS_1
CmsRet mdmInit_addQosQueue_igd(UINT32 precedence, UINT32 qid,
                               const char *fullPath, const char *qname,
                               UBOOL8 enable)
{
   MdmPathDescriptor pathDesc;
   _QMgmtObject      *mdmObj = NULL;
   _QMgmtQueueObject *qObj   = NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_Q_MGMT_QUEUE;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance returns error. ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&qObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }

   qObj->queueEnable            = enable;
   qObj->queuePrecedence        = precedence;
   qObj->X_BROADCOM_COM_QueueId = qid;
   CMSMEM_REPLACE_STRING_FLAGS(qObj->queueInterface, fullPath, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(qObj->X_BROADCOM_COM_QueueName, qname, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **)&qObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
      mdm_freeObject((void **)&qObj);
      return ret;
   }

   mdm_freeObject((void **)&qObj);

   /* increment the queue count */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_Q_MGMT;

   /* get the queue management object */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&mdmObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }

   mdmObj->queueNumberOfEntries++;
   if ((ret = mdm_setObject((void **)&mdmObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
   }

   mdm_freeObject((void **)&mdmObj);
   return ret;
}
#endif /* DMP_QOS_1 */

#endif  /* CMS_CONFIG_COMPAT */

