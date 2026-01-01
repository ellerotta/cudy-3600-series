/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2006:proprietary:standard
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
 * :>
 *
 ************************************************************************/


#include "cms.h"
#include "cms_util.h"
#include "cms_mgm.h"
#include "cms_obj.h"
#include "cms_lck.h"
#include "bcm_flashutil.h"
#include "prctl.h"
#include "nanoxml.h"
#include "mdm.h"
#include "mdm_private.h"
#include "oal.h"


/** This file deals only with writing a config file out.
 *
 */


static UBOOL8 isAllWhiteSpace(const char *p, UINT32 len);
static CmsRet parseVersionNumber(const char *buf, UINT32 *major, UINT32 *minor);
static UINT32 myMajor=0;
static UINT32 myMinor=0;

static CmsRet extractMyConfig(char *buf, UINT32 *len);

/* config write related functions */
static void mdm_tagBeginCallbackFunc(nxml_t handle, const char *tagName, UINT32 len);
static void mdm_attrBeginCallbackFunc(nxml_t handle, const char *attrName, UINT32 len);
static void mdm_attrValueCallbackFunc(nxml_t handle, const char *attrValue, UINT32 len, SINT32 more);
static void mdm_dataCallbackFunc(nxml_t handle, const char *data, UINT32 len, SINT32 more);
static void mdm_tagEndCallbackFunc(nxml_t handle, const char *tagName, UINT32 len);

static void mdm_fixIntfStackNumberOfEntries();
static void mdm_fixQueueNumberOfEntries();
static void mdm_fixClassificationNumberOfEntries();
static void mdm_overwriteDefaultPasswords();
static void filterMonolithicObjsForComponent();


#ifdef CMS_CONFIG_COMPAT

/* older config file conversion functions */
static void convert_v1_password(void);
static void convert_v1_v2_defaultGateway(void);
static void convert_v3_tunnelObjects(MdmObjectId oid, char *buf);

#endif


CmsRet mdm_loadConfig(void)
{
   char *buf=NULL;
   UINT32 origLen, len;
   CmsRet ret;
   UBOOL8 tryEtcDefaultCfg=TRUE;
   char *selector = CMS_CONFIG_PRIMARY;


   if ((origLen = cmsImg_getConfigFlashSize()) == 0)
   {
      cmsLog_error("cmsImg_getConfigFlashSize returned 0!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* malloc a buffer for holding the config. */
   cmsLog_notice("allocating %d bytes for config file", origLen);
   if ((buf = cmsMem_alloc(origLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", origLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }


   /*
    * First try to load primary config file from flash.
    */
   len = origLen;
   if (((ret = oal_readConfigFlashToBuf(selector, buf, &len)) == CMSRET_SUCCESS) &&
       (len > 0))
   {
      cmsLog_notice("Got primary config file from flash (len=%d), validating....", len);
      // TODO: BDK can process a CMS PURE181 monolithic config file, but it
      // should detect and reject Hybrid TR98+181 monolithic config file.
      if ((ret = mdm_validateConfigBuf(buf, len)) != CMSRET_SUCCESS)
      {
         /*
          * Validation of the config buffer failed, so do not load it 
          * into the MDM.
          */
         cmsLog_error("primary config file from flash is unrecognized or invalid.");
         // No need to invalidate or delete this config file.  If we manage
         // to boot, we will have somehow found or generaed a valid config file
         // which will be written out and overwrite this invalid one.
      }
      else
      {
         cmsLog_notice("primary config file is valid!");
      }
   }

#ifdef SUPPORT_BACKUP_PSI
   if (((ret != CMSRET_SUCCESS) || len == 0) &&
       (cmsImg_isBackupConfigFlashAvailable()))
   {
      cmsLog_notice("trying to read backup config flash");

      len = origLen;
      selector = CMS_CONFIG_BACKUP;
      if (((ret = oal_readConfigFlashToBuf(selector, buf, &len)) == CMSRET_SUCCESS) &&
          (len > 0))
      {
         cmsLog_debug("Got backup config file from flash, len=%d, validating...", len);
         // TODO: BDK can process a CMS PURE181 monolithic config file, but it
         // should detect and reject Hybrid TR98+181 monolithic config file.
         ret = mdm_validateConfigBuf(buf, len);
      }
   }
#endif /* SUPPORT_BACKUP_PSI */


   if ((ret == CMSRET_SUCCESS) && (len > 0))
   {
      cmsLog_notice("config file for %s from flash (%s) is valid, len=%d, loading into MDM....",
                    mdmShmCtx->compName, selector, len);
      mdm_loadValidatedConfigBufIntoMdm(buf, len);
      tryEtcDefaultCfg = FALSE;
   }
   else
   {
      cmsLog_notice("no valid saved config file, try /etc/default.cfg");
   }


   /*
    * If we could not get a config file from flash, see if there
    * is a default config file at /etc/default.cfg
    */
   if (tryEtcDefaultCfg)
   {
      char filename[BUFLEN_1024]={0};
      UINT32 sectionCount;

      snprintf(filename, sizeof(filename), "/etc/default.cfg");

      len = origLen;
      if (((ret = oal_readConfigFileToBuf(filename, buf, &len)) == CMSRET_SUCCESS) &&
          (len > 0))
      {
         // There are 4 possible combinations:
         // 1. we are a BDK image and this is a BDK config file -> extract my component config
         // 2. We are a BDK image and this is a CMS config file -> regular validate
         // 3. We are a CMS image and this is a CMS config file -> regular validate
         // 4. We are a CMS image and this is a BDK config file -> not supported

         sectionCount = cmsImg_countConfigSections(buf);
         cmsLog_notice("Read in %s len=%d sectionCount=%d",
                        filename, len, sectionCount);

         if (cmsMdm_isCmsClassic())
         {
            if (sectionCount > 1)
            {
               // case 4.
               cmsLog_error("Unsupported BDK config file detected in %s (sectionCount=%d)",
                            filename, sectionCount);
               ret = CMSRET_INVALID_ARGUMENTS;
            }
         }
         else
         {
            // We are a BDK image
            if (sectionCount > 1)
            {
               // This is case 1: extract my component's config from this BDK config file.
               ret = extractMyConfig(buf, &len);
            }
         }

         if (ret == CMSRET_SUCCESS)
         {
            cmsLog_notice("Got config file from %s (len=%d), validating....", filename, len);
            if ((ret = mdm_validateConfigBuf(buf, len)) != CMSRET_SUCCESS)
            {
               cmsLog_error("config file from %s (len=%d) is invalid", filename, len);
            }
            else
            {
               cmsLog_notice("config file from %s (len=%d)is valid, loading into MDM....", filename, len);
               mdm_loadValidatedConfigBufIntoMdm(buf, len);

               /*
                * Some customers like to see the config file in the flash
                * immediately, so flush the config into the flash now.
                */
               // Use auto-locking
               // In BDK, we only want to save the config of our own component.
               // In CMS, the call below just turns into a save of the monolithic MDM anyways.
               cmsMgm_saveConfigToFlashEx(OGF_LOCAL_MDM_ONLY);
            }
         }
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);

   // Regardless of whether we really read in a config file or if we are
   // using a new default MDM, check for default password overwrite.
   if (cmsMdm_isCmsClassic() || cmsMdm_isBdkSysmgmt())
   {
      mdm_overwriteDefaultPasswords();
   }

   /* by the time we get down here, always return success */
   return CMSRET_SUCCESS;
}


// Given a BDK config file with multiple sections, find the section for
// the currently running component, and "pull" that section to the beginning
// of the buffer so that it seems like the buffer contains just the config file
// for the current component.  If no matching config file section is found, 
// zero the buffer and return error code.
CmsRet extractMyConfig(char *buf, UINT32 *len)
{
   const char *delim = CMS_CONFIG_XML_TAG;
   const char *componentName = NULL;
   char *componentCfg=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   char *pBegin, *pNext;
   int cfgLen;

   cmsLog_notice("Entered:");

   if (buf == NULL || len == NULL || *len == 0)
   {
      return CMSRET_NO_MORE_INSTANCES;
   }

   cmsLog_notice("len=%d", *len);

   // There is similar code in remote_zbus.c, unify?
   pBegin = strstr(buf, delim);
   while (!found && (pBegin != NULL))
   {
      pNext= strstr((pBegin+strlen(delim)), delim);
      if (pNext != NULL)
      {
         cfgLen = (pNext-1)-pBegin;  // -1 to omit the newline after each section(?)
      }
      else
      {
         cfgLen = strlen(pBegin);
      }
      // +1 for null terminator
      componentCfg = cmsMem_alloc(cfgLen+1, ALLOC_ZEROIZE);
      if (componentCfg == NULL)
      {
         cmsLog_error("Failed to allocate %d bytes", cfgLen+1);
         ret = CMSRET_RESOURCE_EXCEEDED;
         break;
      }
      memcpy(componentCfg, pBegin, cfgLen);
      componentName = cmsImg_configTagToComponentName(componentCfg);
      cmsLog_debug("current compName=%s cfgLen=%d", componentName, cfgLen);

      if (!cmsUtl_strcmp(componentName, mdmShmCtx->compName))
      {
         cmsLog_notice("found my compName %s, cfgLen=%d", componentName, cfgLen);

         // Copy my section to the beginning of the buf, and make the buf
         // seem to contain only my config.
         memset(buf, 0, *len);
         memcpy(buf, componentCfg, cfgLen);
         *len = cfgLen+1;
         found = TRUE;
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(componentCfg);

      // Go to the next section
      pBegin= strstr((pBegin+strlen(delim)), delim);
   }

   if (!found)
   {
      memset(buf, 0, *len);
      *len = 0;
      ret = CMSRET_NO_MORE_INSTANCES;
   }

   cmsLog_notice("Exit: ret=%d len=%d", ret, *len);
   return ret;
}


CmsRet mdm_validateConfigBuf(const char *buf, UINT32 len)
{
   nxml_t nxmlHandle;
   nxml_settings nxmlSettings;
   char *endp = NULL;
   SINT32 rc;
   UINT32 maxLen;
   CmsRet ret=CMSRET_SUCCESS;

   /*
    * When reading from configflash that has been zeroized,
    * we get length of 1.  Way too short to hold valid config file.
    */
   if (buf == NULL || len <= 1)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   

   maxLen = cmsImg_getConfigFlashSize();
   if (len > maxLen)
   {
      cmsLog_error("config buf length %d is greater than max %d", len, maxLen);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* initialize our callback context state */
   memset(&nxmlCtx, 0, sizeof(nxmlCtx));
   nxmlCtx.loadMdm = FALSE;
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;
   nxmlCtx.ret = CMSRET_SUCCESS;   

   /* set callback function handlers */
   nxmlSettings.tag_begin = mdm_tagBeginCallbackFunc;
   nxmlSettings.attribute_begin = mdm_attrBeginCallbackFunc;
   nxmlSettings.attribute_value = mdm_attrValueCallbackFunc;
   nxmlSettings.data = mdm_dataCallbackFunc;
   nxmlSettings.tag_end = mdm_tagEndCallbackFunc;

   if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
   {
      cmsLog_error("xmlOpen failed");
      return CMSRET_INTERNAL_ERROR;
   }

   /* push our data into the nanoxml parser, it will then call our callback funcs. */
   rc = xmlWrite(nxmlHandle, (char *)buf, len, &endp);


   /* check for error conditions. */
   if (rc == 0)
   {
      cmsLog_error("nanoxml parser returned error.");
      ret = CMSRET_INVALID_ARGUMENTS;
   }
   else if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("nxmlCtx.ret = %d", nxmlCtx.ret);
      ret = nxmlCtx.ret;
   }   
   else if (!isAllWhiteSpace(endp, buf+len-endp))
   {
      /* nanoxml endp points at one past the last processed char */
      cmsLog_error("not all data processed, buf %p (len 0x%x) "
                   "endp at %p, buf+len-endp=%d",
                   buf, len, endp, buf+len-endp);
      ret = CMSRET_INVALID_ARGUMENTS;
   }
   else if (!nxmlCtx.topNodeFound || !nxmlCtx.versionFound)
   {
      cmsLog_error("did not find top node and/or version");
      ret = nxmlCtx.ret;
   }

   xmlClose(nxmlHandle);

   return ret;
}


void mdm_loadValidatedConfigBufIntoMdm(const char *buf, UINT32 len)
{
   nxml_t nxmlHandle;
   nxml_settings nxmlSettings;
   char *endp = NULL;
   SINT32 rc;


   /* initialize our callback context state */
   memset(&nxmlCtx, 0, sizeof(nxmlCtx));
   nxmlCtx.loadMdm = TRUE;
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;
   nxmlCtx.ret = CMSRET_SUCCESS;

   /* set callback function handlers */
   nxmlSettings.tag_begin = mdm_tagBeginCallbackFunc;
   nxmlSettings.attribute_begin = mdm_attrBeginCallbackFunc;
   nxmlSettings.attribute_value = mdm_attrValueCallbackFunc;
   nxmlSettings.data = mdm_dataCallbackFunc;
   nxmlSettings.tag_end = mdm_tagEndCallbackFunc;

   if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
   {
      cmsLog_error("xmlOpen failed");
      return;
   }

   /* push our data into the nanoxml parser, it will then call our callback funcs. */
   rc = xmlWrite(nxmlHandle, (char *)buf, len, &endp);


   /* check for error conditions. */
   if (rc == 0)
   {
      cmsLog_error("nanoxml parser returned error.");
   }
   else if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      cmsLog_error("parsing returned error %d", nxmlCtx.ret);
   }
   else if (!isAllWhiteSpace(endp, buf+len-endp))
   {
      /* nanoxml endp seems to point at the last processed char */
      cmsLog_error("not all data processed, buf %p (len 0x%x) "
                   "endp at %p, expected endp %p",
                   buf, len, endp, buf+len-2);
   }

   else if (!nxmlCtx.topNodeFound || !nxmlCtx.versionFound)
   {
      cmsLog_error("did not find top node and/or version");
   }

   xmlClose(nxmlHandle);

   // If we did CMS_PURE181 to DMDM config file conversion, we need to fix the
   // interfaceStackNumberOfEntries, queueNumberOfEntries, etc.
   // To simplify code, we always do this in BDK builds, even though it is
   // only needed after a CMS_PURE181 to DMDM config conversion.
   if (!strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT) ||
       !strcmp(mdmShmCtx->compName, BDK_COMP_DSL) ||
       !strcmp(mdmShmCtx->compName, BDK_COMP_WIFI))
   {
      mdm_fixIntfStackNumberOfEntries();
      mdm_fixQueueNumberOfEntries();
      mdm_fixClassificationNumberOfEntries();
   }

   return;
}

static UINT32 countObjs(MdmObjectId oid)
{
   void *mdmObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   UINT32 count=0;
   CmsRet ret;

   while ((ret = mdm_getNextObject(oid, &iidStack, &mdmObj)) == CMSRET_SUCCESS)
   {
      count++;
      mdm_freeObject(&mdmObj);
   }
   return count;
}

void mdm_fixIntfStackNumberOfEntries()
{
   Dev2DeviceObject *devObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = mdm_getObject(MDMOID_DEV2_DEVICE, &iidStack, (void **)&devObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get Device2 object, ret=%d", ret);
      return;
   }

   devObj->interfaceStackNumberOfEntries = countObjs(MDMOID_DEV2_INTERFACE_STACK);
   ret = mdm_setObject((void **)&devObj, &iidStack, FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2DeviceObject, ret = %d", ret);
   }
   
   return;
}

void mdm_fixQueueNumberOfEntries()
{
   Dev2QosObject *qosObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = mdm_getObject(MDMOID_DEV2_QOS, &iidStack, (void **)&qosObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get QoS object, ret=%d", ret);
      return;
   }

   qosObj->queueNumberOfEntries = countObjs(MDMOID_DEV2_QOS_QUEUE);
   ret = mdm_setObject((void **)&qosObj, &iidStack, FALSE);  
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2QosObject, ret = %d", ret);
   }
   
   return;
}

void mdm_fixClassificationNumberOfEntries()
{
   Dev2QosObject *qosObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = mdm_getObject(MDMOID_DEV2_QOS, &iidStack, (void **)&qosObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get QoS object, ret=%d", ret);
      return;
   }

   qosObj->classificationNumberOfEntries = countObjs(MDMOID_DEV2_QOS_CLASSIFICATION);
   ret = mdm_setObject((void **)&qosObj, &iidStack, FALSE);  
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2QosObject, ret = %d", ret);
   }
   
   return;
}

// Starting in 504L02P1, customers can set the default passwords for the
// admin, support, or user accounts in the UBoot environment variables:
// default_admin_password
// default_support_password
// default_user_password
// If the passwords for these accounts have not been changed from their default
// values of "admin", "support", and "user", respectively, then the passwords
// specified by the environment variables will be used.
void mdm_overwriteDefaultPasswords()
{
   LoginCfgObject *loginObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char buf[CMS_MAX_PASSWORD_LENGTH]={0};
   UBOOL8 doSet = FALSE;
   CmsRet ret;

   cmsLog_notice("Entered:");

   ret = mdm_getObject(MDMOID_LOGIN_CFG, &iidStack, (void **)&loginObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get LOGIN_CFG obj, ret=%d", ret);
      return;
   }

   // if MDM has the default password, and default_admin_password is found
   // in UBoot environment var, use it.
   if (!cmsUtl_strcmp(loginObj->adminPassword, "admin") &&
       (CMSRET_SUCCESS == cmsUtil_getDefaultAdminPassword(buf, sizeof(buf)-1)))
   {
      CMSMEM_REPLACE_STRING_FLAGS(loginObj->adminPassword, buf, mdmLibCtx.allocFlags);
      doSet = TRUE;
   }

   memset(buf, 0, sizeof(buf));
   if (!cmsUtl_strcmp(loginObj->supportPassword, "support") &&
       (CMSRET_SUCCESS == cmsUtil_getDefaultSupportPassword(buf, sizeof(buf)-1)))
   {
      CMSMEM_REPLACE_STRING_FLAGS(loginObj->supportPassword, buf, mdmLibCtx.allocFlags);
      doSet = TRUE;
   }

   memset(buf, 0, sizeof(buf));
   if (!cmsUtl_strcmp(loginObj->userPassword, "user") &&
       (CMSRET_SUCCESS == cmsUtil_getDefaultUserPassword(buf, sizeof(buf)-1)))
   {
      CMSMEM_REPLACE_STRING_FLAGS(loginObj->userPassword, buf, mdmLibCtx.allocFlags);
      doSet = TRUE;
   }

   if (doSet)
   {
      ret = mdm_setObject((void **)&loginObj, &iidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set LoginCfgObject, ret = %d", ret);
      }
   }

   return;
}


void mdm_tagBeginCallbackFunc(nxml_t handle __attribute__((unused)),
                              const char *tagName,
                              UINT32 len)
{
   char buf[NXML_MAX_NAME_SIZE+1];
   MdmObjectNode *objNode;
   MdmParamNode *paramNode;
   CmsRet ret;
   MdmObjectNode *compatChildObjNode=NULL;

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   strncpy(buf, tagName, len);
   buf[len]='\0';

   if (nxmlCtx.ignoreTag)
   {
      cmsLog_debug("Ignoring tag %s inside top level ignoreTag %s\n",
                   buf, nxmlCtx.ignoreTag);
      return;
   }

#ifdef CMS_CONFIG_COMPAT
   /************** Begin convert v1,v2 config file ****************/

   /*
    * In v1, v2, the Time object had X_BROADCOM_COM_NTPEnable.
    * In v3, we use the BBF defined Enable parameter.
    */
   if ((nxmlCtx.versionMajor < 3) &&
       (nxmlCtx.objNode != NULL) && (MDMOID_TIME_SERVER_CFG == nxmlCtx.objNode->oid) &&
       (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_NTPEnable")))
   {
      snprintf(buf, len+1, "Enable");
      cmsLog_debug("converted X_BROADCOM_COM_NTPEnable to %s", buf);
   }

   /* 
    * In v2 and v2, there was a WanPPPConnection.X_BROADCOM_COM_BcastAddr.
    * In v3, we deleted that parameter.  So just pretend the current param
    * is X_BROADCOM_COM_IfName so that we can have a paramNode.  We won't do
    * anything to the X_BROADCOM_COM_IfName param though, we just need to
    * point to a similar string type param node so that later processing will
    * not get confused.
    */
   if ((nxmlCtx.versionMajor < 3) &&
       (nxmlCtx.objNode != NULL) && (MDMOID_WAN_PPP_CONN == nxmlCtx.objNode->oid) &&
       (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_BcastAddr")))
   {
      cmsLog_debug("fake X_BROADCOM_COM_BcastAddr to X_BROADCOM_COM_IfName");
      snprintf(buf, len+1, "X_BROADCOM_COM_IfName");
   }

   /*
    * In December 2011, 4in6Tunnel and 6in4Tunnel were
    * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
    */
   if ((nxmlCtx.versionMajor < 4) && (nxmlCtx.objNode != NULL))
   {
      convert_v3_tunnelObjects(nxmlCtx.objNode->oid, buf);
   }

   /*
    * In 4.14, X_BROADCOM_COM_LineSetting was renamed to
    * X_BROADCOM_COM_DectLineSetting
    */
   if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_LineSetting")))
   {
      snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_DectLineSetting");
      cmsLog_debug("converted X_BROADCOM_COM_LineSetting to %s", buf);
   }

   /*
    * In 4.14L.04 (Oct 2013), X_BROADCOM_COM_MLDSnoopingConfig was moved
    * up two levels.  It is now a child of LANDevice.{i}.
    */
   if (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_MldSnoopingConfig"))
   {
      cmsLog_debug("detected MldSnoopingObject, currObj->name %s",
                   nxmlCtx.objNode->name);

      if (nxmlCtx.objNode->oid == MDMOID_LAN_DEV ||
          nxmlCtx.objNode->oid == MDMOID_DEV2_BRIDGE)
      {
         cmsLog_debug("MldSnoopingConfig already in correct place, do nothing");
      }
      else
      {
         MdmObjectNode *parentObjNode = nxmlCtx.objNode->parent;
         UINT32 count=0;
         UBOOL8 found=FALSE;

         while (!found && parentObjNode && count < 3)
         {
            cmsLog_debug("compat: searching for LAN_DEV: curr %d, count=%d",
                          parentObjNode->oid, count);
            if (parentObjNode->oid == MDMOID_LAN_DEV)
            {
               found = TRUE;
               compatChildObjNode = mdm_getChildObjectNode(parentObjNode, buf);
            }
            else
            {
               parentObjNode = parentObjNode->parent;
               count++;
            }
         }
      }
   }
   /*
    * In 5.04L02, X_BROADCOM_COM_VDSL_BrcmPriv1 was renamed to
    * X_BROADCOM_COM_VDSL_35b
    */
   if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_VDSL_BrcmPriv1")))
   {
      snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_VDSL_35b");
      cmsLog_debug("converted X_BROADCOM_COM_VDSL_BrcmPriv1 to %s", buf);
   }


   /**************** end config file conversion ****************/
#endif


   if (nxmlCtx.objNode == NULL)
   {
      /*
       * This is very early in the config file.  It must start with the
       * CpeConfigFile node followed by the InternetGatewayDevice node.
       */
      if (!strcmp(buf, CONFIG_FILE_TOP_NODE))
      {
         if (nxmlCtx.topNodeFound)
         {
            cmsLog_error("multiple %s nodes detected", CONFIG_FILE_TOP_NODE);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            cmsLog_debug("%s node detected", CONFIG_FILE_TOP_NODE);
            nxmlCtx.topNodeFound = TRUE;
         }
      }
      else if (!strcmp(buf, mdmShmCtx->rootObjNode->name))
      {
         cmsLog_debug("%s node detected", mdmShmCtx->rootObjNode->name);
         nxmlCtx.objNode = mdmShmCtx->rootObjNode;

         ret = mdm_getDefaultObject(nxmlCtx.objNode->oid, &(nxmlCtx.mdmObj));
         if (ret != CMSRET_SUCCESS)
         {
            nxmlCtx.ret = ret;
         }
      }
      else if (!strcmp(buf, CONFIG_FILE_PSI_TOP_NODE))
      {
         // Old 3.x PSI format.
         cmsLog_notice("PSI top node detected");
         nxmlCtx.ret = CMSRET_CONFIG_PSI;
         cmsLog_error("unsupported PSI config file detected");
      }
      else
      {
         cmsLog_error("Invalid start node %s", buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }

      return;
   }


   /*
    * a tag can start a child object node, a peer object node,
    * or a parameter node of the current object node.
    */
   if (((objNode = mdm_getChildObjectNode(nxmlCtx.objNode, buf)) != NULL) ||
       ((nxmlCtx.gotCurrObjEndTag == TRUE) &&
        (nxmlCtx.objNode->parent != NULL) &&
        ((objNode = mdm_getChildObjectNode(nxmlCtx.objNode->parent, buf)) != NULL)) ||
       (compatChildObjNode != NULL))
   {

      if(objNode != NULL)
      {
         if (nxmlCtx.objNode == objNode->parent)
         {
            cmsLog_debug("%s --> child obj %s", nxmlCtx.objNode->name, objNode->name);
         }
         else
         {
            cmsLog_debug("%s --> peer obj %s", nxmlCtx.objNode->name, objNode->name);
         }
      }

      nxmlCtx.gotCurrObjEndTag = FALSE;

      if (nxmlCtx.loadMdm)
      {
         /*
          * Since we are transitioning away from the current object, 
          * set any attributes of the current objNode sub-tree that we detected.
          */
         if (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange
                     || nxmlCtx.attr.setAltNotif)
         {
            cmsLog_debug("set sub-tree attr notif=(%d)%d altNotif=(%d)%d access=(%d)0x%x",
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.setAltNotif,
                         nxmlCtx.attr.altNotif,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask);

            ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                                &(nxmlCtx.iidStack),
                                                &(nxmlCtx.attr),
                                                FALSE);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d altChangeNotification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                            nxmlCtx.objNode->name,
                            cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                            nxmlCtx.attr.notificationChange,
                            nxmlCtx.attr.notification,
                            nxmlCtx.attr.setAltNotif,
                            nxmlCtx.attr.altNotif,
                            nxmlCtx.attr.accessBitMaskChange,
                            nxmlCtx.attr.accessBitMask,
                            ret);
               nxmlCtx.ret = ret;
            }

            nxmlCtx.attr.accessBitMaskChange = 0;
            nxmlCtx.attr.notificationChange = 0;
            nxmlCtx.attr.setAltNotif = 0;
         }


         /*
          * We are in the middle of objectA, and now transitioning into 
          * objectB, like this:
          * <ObjectA>
          *   <param1>blah</param1>
          *   <param2>blah</param2>
          *   <ObjectB>
          *
          * This means the current object is complete, so push it into the MDM.
          *
          * Every new value we've put into the mdmObj has been validated
          * in the dataCallbackFunc, so there is no need to validate the
          * entire mdmObj here.
          * (Highly unlikely we will have a nextInstanceNode == TRUE here, but
          * just leave the code as is, does not harm anything.)
          */
         if (nxmlCtx.mdmObj != NULL && nxmlCtx.nextInstanceNode == FALSE)
         {
            cmsLog_debug("setting obj %s %s",
                         mdm_oidToGenericPath(*((MdmObjectId *) nxmlCtx.mdmObj)),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            ret = mdm_setObject(&(nxmlCtx.mdmObj), &(nxmlCtx.iidStack), FALSE);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("setObject failed, %d", ret);
               mdm_freeObject(&(nxmlCtx.mdmObj));
               nxmlCtx.ret = ret;
            }
         }
      }
      else
      {
         /* not loading mdm, just free the mdmObj */
         if (nxmlCtx.mdmObj)
         {
            mdm_freeObject(&(nxmlCtx.mdmObj));
         }
      }


      nxmlCtx.nextInstanceNode = FALSE;

#ifdef CMS_CONFIG_COMPAT
      if (compatChildObjNode != NULL)
      {
         objNode = compatChildObjNode;
      }
#endif

      /* record the new object node that we are working on. */
      nxmlCtx.objNode = objNode;


      /*
       * node's attributes are inheritied from the parent (I don't think I
       * need this line below.  Any changes to attributes should have
       * been written out in the config file.)
       */
      nxmlCtx.attr = objNode->parent->nodeAttr;


      /* start a new MdmObject */
      if (nxmlCtx.mdmObj)
      {
         cmsLog_error("mem leak averted, free %p oid=%d",
                      nxmlCtx.mdmObj, GET_MDM_OBJECT_ID(nxmlCtx.mdmObj));
         mdm_freeObject(&(nxmlCtx.mdmObj));
      }
      ret = mdm_getDefaultObject(nxmlCtx.objNode->oid, &(nxmlCtx.mdmObj));
      if (ret != CMSRET_SUCCESS)
      {
         nxmlCtx.ret = ret;
      }
   }
   else if ((paramNode = mdm_getParamNode(nxmlCtx.objNode->oid, buf)) != NULL)
   {
      cmsLog_debug("%s :: param %s", nxmlCtx.objNode->name, paramNode->name);

      if (nxmlCtx.nextInstanceNode)
      {
         cmsLog_error("param node %s detected under a next instance object", paramNode->name);
         nxmlCtx.paramNode = paramNode;
         return;
      }

      /*
       * We could be transitioning from obj to param, if any attribute
       * has changed, set them for the current objNode sub-tree.
       * The fourth parameter of setSubTreeParamAttributes is testOnly,
       * so we pass in !loadMdm, which which means when loadMdm=FALSE, testOnly=TRUE.
       */
      if ((nxmlCtx.loadMdm) &&
          (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange || nxmlCtx.attr.setAltNotif))
      {
         cmsLog_debug("set sub-tree attr notif=(%d)%d altNotif=(%d)%d access=(%d)0x%x",
                      nxmlCtx.attr.notificationChange,
                      nxmlCtx.attr.notification,
                      nxmlCtx.attr.setAltNotif,
                      nxmlCtx.attr.altNotif,
                      nxmlCtx.attr.accessBitMaskChange,
                      nxmlCtx.attr.accessBitMask);

         ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                             &(nxmlCtx.iidStack),
                                             &(nxmlCtx.attr),
                                             FALSE);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d altNotification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                         nxmlCtx.objNode->name,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.setAltNotif,
                         nxmlCtx.attr.altNotif,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask,
                         ret);
            nxmlCtx.ret = ret;
         }

         nxmlCtx.attr.accessBitMaskChange = 0;
         nxmlCtx.attr.notificationChange = 0;
         nxmlCtx.attr.setAltNotif = 0;
      }


      if (nxmlCtx.paramNode != NULL)
      {
         cmsLog_error("embedded param node detected %s %s",
                      nxmlCtx.paramNode->name, paramNode->name);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         nxmlCtx.paramNode = paramNode;
      }
   }
   else
   {
#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
     UINT32 oid=0;
      if (nxmlCtx.mdmObj)
         oid = GET_MDM_OBJECT_ID(nxmlCtx.mdmObj);

      nxmlCtx.ignoreTag = cmsMem_strdup(buf);
      cmsLog_debug("recording unrecognized param/obj tag %s (curr mdmObj=%p oid=%d)",
                   nxmlCtx.ignoreTag, nxmlCtx.mdmObj, oid);
#else
      // Workaround for X_BROADCOM_COM_GPON tag issue.
      // Starting in 5.04L02P1, the X_BROADCOM_COM_GPON tag was changed to
      // X_RDK_ONT.  The X_BROADCOM_COM_GPON tag should never get written to
      // config file, but in some configs it did, which causes compatibility
      // issues with images after the name change.  So ignore the old tag even
      // if CMS_CONFIG_IGNORE_UNRECOGNIZED is not enabled.
      if (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_GPON"))
      {
         cmsLog_debug("ignore obsolete X_BROADCOM_COM_GPON tag");
         nxmlCtx.ignoreTag = cmsMem_strdup(buf);
      }
      else
      {
         cmsLog_error("Unrecognized tag %s", buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
#endif
   }

   return;
}

static void mdm_attrBeginCallbackFunc(nxml_t handle __attribute__((unused)),
                                      const char *attrName,
                                      UINT32 len)
{
   char buf[NXML_MAX_NAME_SIZE+1];

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error has been detected. */
      return;
   }
   
   strncpy(buf, attrName, len);
   buf[len]='\0';

   cmsLog_debug("%s", buf);

   if (nxmlCtx.ignoreTag)
   {
      cmsLog_notice("Ignoring attrTag %s (ignoreTag=%s)",
                   buf, nxmlCtx.ignoreTag);
      return;
   }

   if (nxmlCtx.currXmlAttr != MDM_CONFIG_XMLATTR_NONE)
   {
      cmsLog_error("overlapping attrs, currXmlAttr=%d", nxmlCtx.currXmlAttr);
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      return;
   }

   if (!strcmp(CONFIG_FILE_ATTR_VERSION, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_VERSION;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_BDK_CONFIG_IDENT, buf))
   {
      // Starting with 504L02, config files will begin with 
      // <DslCpeConfig version="3.0" bdkConfigIdent="dsl" >
      // where the value of the bdkConfigIdent attr is a component name string
      // defined in bdk.h
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_BDK_CONFIG_IDENT;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_INSTANCE, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_INSTANCE;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_NEXT_INSTANCE, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NEXT_INSTANCE;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_ACCESS_LIST, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_ACCESS_LIST;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_NOTIFICATION, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NOTIFICATION;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_ALTNOTIFICATION, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_ALTNOTIFICATION;
   }
   else
   {
      cmsLog_error("Unrecognized attribute %s", buf);
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
   }

   return;
}


static void mdm_attrValueCallbackFunc(nxml_t handle __attribute__((unused)),
                                      const char *attrValue,
                                      UINT32 len,
                                      SINT32 more)
{
   char *buf;

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error has been detected. */
      return;
   }

   /*
    * our attribute values are always written out in a single line,
    * so more should always be 0.
    */
   if (more != 0)
   {
      cmsLog_error("multi-line attribute value detected");
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      return;
   }

   if (nxmlCtx.ignoreTag)
   {
      cmsLog_notice("Ignoring attrValue (ignoreTag=%s)", nxmlCtx.ignoreTag);
      return;
   }

   if ((buf = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len+1);
      nxmlCtx.ret = CMSRET_RESOURCE_EXCEEDED;
      return;
   }

   strncpy(buf, attrValue, len);
   buf[len]='\0';

   cmsLog_debug("(more=%d)%s", more, buf);

   switch(nxmlCtx.currXmlAttr)
   {
   case MDM_CONFIG_XMLATTR_VERSION:
      {
         /*
          * Config file version may help us with conversion later on.
          * For now, just print it out.
          */
         if (nxmlCtx.objNode != NULL)
         {
            cmsLog_error("version attr can only appear in the top node");
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            cmsLog_debug("config file version=%s", buf);
            nxmlCtx.ret = parseVersionNumber(buf, &nxmlCtx.versionMajor, &nxmlCtx.versionMinor);

            if (nxmlCtx.ret == CMSRET_SUCCESS)
            {
               nxmlCtx.versionFound = TRUE;

               parseVersionNumber(CMS_CONFIG_FILE_VERSION, &myMajor, &myMinor);

#ifdef CMS_CONFIG_COMPAT
               /*
                * If CMS_CONFIG_COMPAT is defined, then we can accept any version
                * of the config file that is less than or equal to my current version.
                */
               if (nxmlCtx.versionMajor > myMajor)
               {
                  cmsLog_error("config file version number is %d, this software only supports up to %d",
                               nxmlCtx.versionMajor, myMajor);
                  nxmlCtx.ret = CMSRET_INVALID_CONFIG_FILE;
               }
#else
               /*
                * If CMS_CONFIG_COMPAT is not defined, then we can only accept
                * config files that have exactly the same version as my current version.
                */
               if (nxmlCtx.versionMajor != myMajor)
               {
                  cmsLog_error("config file version number is %d, this software only supports %d",
                               nxmlCtx.versionMajor, myMajor);
                  cmsLog_error("You may need to enable backward compatibility of CMS Config files in make menuconfig");
                  nxmlCtx.ret = CMSRET_INVALID_CONFIG_FILE;
               }
#endif  /* CMS_CONFIG_COMPAT */
            }
         }
         break;
      }

   case MDM_CONFIG_XMLATTR_BDK_CONFIG_IDENT:
      // This is used by remote_objd to determine which component a chunk of
      // config file belongs to.  No processing needed here.
      break;

   case MDM_CONFIG_XMLATTR_INSTANCE:
      {
         UINT32 instanceId;
         CmsRet ret;

         if ((ret = cmsUtl_strtoul(buf, NULL, 0, &instanceId)) != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid instance number %s", buf);
            nxmlCtx.ret = ret;
         }
         else if (!(IS_INDIRECT2(nxmlCtx.objNode)))
         {
            cmsLog_error("got instance number on non-indirect2 node, %s",
                         nxmlCtx.objNode->name);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else if (DEPTH_OF_IIDSTACK(&(nxmlCtx.iidStack)) + 1 != nxmlCtx.objNode->instanceDepth)
         {
            cmsLog_error("instance depth mismatch on %s, instance depth=%d iidStack %s",
                         nxmlCtx.objNode->name,
                         nxmlCtx.objNode->instanceDepth,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            MdmNodeAttributes parentAttr;
            /* initialize parentAttr */
            parentAttr.notificationChange = 0;
            parentAttr.notification = 0;
            parentAttr.setAltNotif = 0;
            parentAttr.altNotif = 0;
            parentAttr.accessBitMaskChange = 0;
            parentAttr.accessBitMask = 0;
            
            /*
             * Get the parent node's attributes.  The newly created sub-tree
             * will inherit these attributes.  But be careful, the location
             * of the parent's attributes depend on the type of node it is.
             */
            if (nxmlCtx.loadMdm)
            {
               if (IS_INDIRECT0(nxmlCtx.objNode->parent))
               {
                  parentAttr = nxmlCtx.objNode->parent->nodeAttr;
               }
               else if (IS_INDIRECT1(nxmlCtx.objNode->parent))
               {
                  InstanceHeadNode *instHead;

                  instHead = mdm_getInstanceHead(nxmlCtx.objNode->parent, &(nxmlCtx.iidStack));
                  if (instHead == NULL)
                  {
                     cmsLog_error("could not find instHead for %s %s",
                                  nxmlCtx.objNode->parent->name,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
                     nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
                  }
                  else
                  {
                     parentAttr = instHead->nodeAttr;
                  }
               }
               else
               {
                  /* must be indirect 2 */
                  InstanceDescNode *instDesc;

                  instDesc = mdm_getInstanceDescFromObjNode(nxmlCtx.objNode->parent,
                                                            &(nxmlCtx.iidStack));
                  if (instDesc == NULL)
                  {
                     cmsLog_error("could not find instDesc for %s %s",
                                  nxmlCtx.objNode->parent->name,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
                     nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
                  }
                  else
                  {
                     parentAttr = instDesc->nodeAttr;
                  }
               }
            }
                  

            /* create the instance in the MDM */
            PUSH_INSTANCE_ID(&(nxmlCtx.iidStack), instanceId);
            cmsLog_debug("Got instanceId for indirect 2 node %s %s",
                         mdm_oidToGenericPath(nxmlCtx.objNode->oid),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));

            if (nxmlCtx.loadMdm)
            {
               ret = mdm_createSubTree(nxmlCtx.objNode,
                                       nxmlCtx.objNode->instanceDepth,
                                       &(nxmlCtx.iidStack),
                                       NULL, NULL);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("create subtree for obj %s %s failed, ret=%d",
                               nxmlCtx.objNode->name,
                               cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                               ret);
                  nxmlCtx.ret = ret;
               }
               else
               {
                  parentAttr.notificationChange = 1;
                  parentAttr.setAltNotif= 1;
                  parentAttr.accessBitMaskChange = 1;

                  cmsLog_debug("set sub-tree attr notif=(%d)%d altNotif=(%d)%d access=(%d)0x%x",
                               parentAttr.notificationChange,
                               parentAttr.notification,
                               parentAttr.setAltNotif,
                               parentAttr.altNotif,
                               parentAttr.accessBitMaskChange,
                               parentAttr.accessBitMask);


                  ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                                      &(nxmlCtx.iidStack),
                                                      &parentAttr,
                                                      FALSE);
                  if (ret != CMSRET_SUCCESS)
                  {
                     cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d altNotification=(%d)%d accessBitMask=(%d)0x%x, ret=%d",
                                  nxmlCtx.objNode->name,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                                  parentAttr.notificationChange,
                                  parentAttr.notification,
                                  parentAttr.setAltNotif,
                                  parentAttr.altNotif,
                                  parentAttr.accessBitMaskChange,
                                  parentAttr.accessBitMask,
                                  ret);
                     nxmlCtx.ret = ret;
                  }
               }

            }
         }

         break;
      }

   case MDM_CONFIG_XMLATTR_NEXT_INSTANCE:
      {
         UINT32 nextInstanceIdToAssign;
         CmsRet ret;

         if ((ret = cmsUtl_strtoul(buf, NULL, 0, &nextInstanceIdToAssign)) != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid next instance number %s", buf);
            nxmlCtx.ret = ret;
         }
         else if (!(IS_INDIRECT2(nxmlCtx.objNode)))
         {
            cmsLog_error("got next instance number on non-indirect2 node, %s",
                         nxmlCtx.objNode->name);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else if (DEPTH_OF_IIDSTACK(&(nxmlCtx.iidStack)) + 1 != nxmlCtx.objNode->instanceDepth)
         {
            cmsLog_error("instance depth mismatch on %s, instance depth=%d iidStack %s",
                         nxmlCtx.objNode->name,
                         nxmlCtx.objNode->instanceDepth,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {

            nxmlCtx.nextInstanceNode = TRUE;

            /*
             * Just set the nextInstanceId here.  We know there are no other 
             * attributes or parameters that we need to collect for this object.
             */
            if (nxmlCtx.loadMdm)
            {
               InstanceHeadNode *instHead;

               cmsLog_debug("setting nextInstanceId to %u for objNode %s iidStack=%s",
                            nextInstanceIdToAssign, nxmlCtx.objNode->name, cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));

               if ((instHead = mdm_getInstanceHead(nxmlCtx.objNode, &(nxmlCtx.iidStack))) == NULL)
               {
                  cmsLog_error("could not find instHead for %s %s", nxmlCtx.objNode->name, cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
               }
               else
               {
                  if (nxmlCtx.objNode->flags & OBN_MULTI_COMP_OBJ)
                  {
                     if (nextInstanceIdToAssign < mdmShmCtx->multiCompFirstInstanceId)
                     {
                        nextInstanceIdToAssign += mdmShmCtx->multiCompFirstInstanceId - 1;
                        cmsLog_debug("oid %d: nextInstanceIdToAssign=%d",
                                     nxmlCtx.objNode->oid,
                                     nextInstanceIdToAssign);
                     }
                  }
                  instHead->nextInstanceIdToAssign = nextInstanceIdToAssign;  
               }
            }

            /* 
             * Even though we don't need to do anything with the iidStack here,
             * push an instance id on the iidStack now because when we hit the
             * end tag, we will pop it off.
             */
            PUSH_INSTANCE_ID(&(nxmlCtx.iidStack), 1);
            cmsLog_debug("pushed fake iidStack, now is %s", cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
         }

         break;
      }


   case MDM_CONFIG_XMLATTR_ACCESS_LIST:
      {
         UINT16 accessBitMask=0;
         CmsRet ret;

         ret = cmsEid_getBitMaskFromStringNames(buf, &accessBitMask);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("conversion of %s to accessBitMask failed, ret=%d",
                         buf, ret);
            nxmlCtx.ret = ret;
         }
         else
         {
            nxmlCtx.attr.accessBitMaskChange = 1;
            nxmlCtx.attr.accessBitMask = accessBitMask;
         }

         /* we need to do more when we are acutally pushing data into MDM */
         break;
      }

   case MDM_CONFIG_XMLATTR_NOTIFICATION:
      {
         UINT32 notification;
         CmsRet ret;

         ret = cmsUtl_strtoul(buf, NULL, 0, &notification);
         if ((ret != CMSRET_SUCCESS) ||
             ((notification != NDA_TR69_NO_NOTIFICATION) && 
              (notification != NDA_TR69_PASSIVE_NOTIFICATION) && 
              (notification != NDA_TR69_ACTIVE_NOTIFICATION)))
         {
            cmsLog_error("invalid notification number %s", buf);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            nxmlCtx.attr.notificationChange = 1;
            nxmlCtx.attr.notification = notification;
         }

         break;
      }

   case MDM_CONFIG_XMLATTR_ALTNOTIFICATION:
      {
         UINT32 altNotification;
         CmsRet ret;

         ret = cmsUtl_strtoul(buf, NULL, 0, &altNotification);
         if ((ret != CMSRET_SUCCESS) ||
             ((altNotification != NDA_MDM_NO_ALTNOTIFICATION) && 
              (altNotification != NDA_MDM_ALTNOTIFICATION)))
         {
            cmsLog_error("invalid altnotification number %s", buf);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            nxmlCtx.attr.setAltNotif= 1;
            nxmlCtx.attr.altNotif= altNotification;
         }

         break;
      }

   case MDM_CONFIG_XMLATTR_NONE:
      {
         cmsLog_error("got attr value with no attr tag in progress.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         break;
      }

   default:
      {
         cmsLog_error("Unrecognized attribute %d", nxmlCtx.currXmlAttr);
         nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
         break;
      }

   } /* end of switch(nxmlCtx.currXmlAttr) */


   /* we are done with attr processing */
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
}




static void mdm_dataCallbackFunc(nxml_t handle __attribute__((unused)),
                                 const char *data,
                                 UINT32 len,
                                 SINT32 more)
{

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   /* 
    * I don't know if we will ever have data that spans multiple
    * lines.  Don't implement this feature for now.  So this code
    * will only use the last line that was detected and drop
    * all previous lines.
    */
   if (more != 0)
   {
      cmsLog_error("support for multiple data lines not implemented yet.");
      nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
      return;
   }

   if (nxmlCtx.paramValue != NULL)
   {
      cmsLog_error("multiple data callbacks detected, paramValue=%s", nxmlCtx.paramValue);
      nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
      return;
   }


   if (nxmlCtx.ignoreTag)
   {
      /* Since we are ignoring the tag, ignore the data also. */
      cmsLog_notice("ignoreTag=%s, ignoring data", nxmlCtx.ignoreTag);
      return;
   }

   /*
    * we are allocating data in heap memory.  If this data is for
    * a string param, mdm_setParamNodeString will make a copy in shared
    * memory and set that in the mdmObj.  If we have very large string
    * objects, we could optimize this by allocating from shared memory
    * here and let mdm_setParamNodeString steal our buffer.  For small string
    * value buffers, its no big deal.
    */
   if ((nxmlCtx.paramValue = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len+1);
   }
   else
   {
      strncpy(nxmlCtx.paramValue, data, len);
      nxmlCtx.paramValue[len]='\0';

      cmsLog_debug("(more=%d)%s", more, nxmlCtx.paramValue);

      /*
       * For string params, we need to unescape any XML character escape
       * sequences before sending to MDM.
       */
      if (nxmlCtx.paramNode->type == MPT_STRING &&
          cmsXml_isUnescapeNeeded(nxmlCtx.paramValue))
      {
         char *unescapedString=NULL;

         cmsXml_unescapeString(nxmlCtx.paramValue, &unescapedString);

         cmsMem_free(nxmlCtx.paramValue);
         nxmlCtx.paramValue = unescapedString;
      }
   }

   return;
}


void mdm_tagEndCallbackFunc(nxml_t handle __attribute__((unused)),
                            const char *tagName,
                            UINT32 len)
{
   UBOOL8 compatNormalPop = TRUE;
   char buf[NXML_MAX_NAME_SIZE+1];

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   strncpy(buf, tagName, len);
   buf[len]='\0';

   if (nxmlCtx.ignoreTag)
   {
      if (!cmsUtl_strcmp(nxmlCtx.ignoreTag, buf))
      {
         /* found matching end tag.  Go back to normal parsing state. */
         cmsLog_debug("Hit matching endTag %s", buf);
         if (nxmlCtx.mdmObj != NULL)
         {
            cmsLog_debug("saved mdmObj=%p oid=%d",
                         nxmlCtx.mdmObj,
                         (int)GET_MDM_OBJECT_ID(nxmlCtx.mdmObj));
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.ignoreTag);
      }
      else
      {
         /* found matching end tag of the current tag, but we are still
          * inside an ignored object. */
         cmsLog_debug("Ignoring End Tag %s (top level ignoreTag=%s)",
                       buf, nxmlCtx.ignoreTag);
      }
      return;
   }

   if (nxmlCtx.paramNode != NULL)
   {
      /*
       * This is the end tag of a param.  The value of the param is in the
       * paramValue field.  Update my mdmObj.
       */

#ifdef CMS_CONFIG_COMPAT
      /************** Begin convert v1,v2 config file ****************/

      /*
       * In v1, v2, the Time object had X_BROADCOM_COM_NTPEnable.
       * In v3, we use the BBF defined Enable parameter.
       */
      if ((nxmlCtx.versionMajor < 3) &&
          (nxmlCtx.objNode != NULL) && (MDMOID_TIME_SERVER_CFG == nxmlCtx.objNode->oid) &&
          (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_NTPEnable")))
      {
         snprintf(buf, len+1, "Enable");
         cmsLog_debug("converted X_BROADCOM_COM_NTPEnable to %s", buf);
      }

      /* 
       * In v2 and v2, there was a WanPPPConnection.X_BROADCOM_COM_BcastAddr.
       * In v3, we deleted that parameter, so just ignore it and return here.
       */
      if ((nxmlCtx.versionMajor < 3) &&
          (nxmlCtx.objNode != NULL) && (MDMOID_WAN_PPP_CONN == nxmlCtx.objNode->oid) &&
          (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_BcastAddr")))
      {
         cmsLog_debug("ignore X_BROADCOM_COM_BcastAddr in WanPppConn");
         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
         nxmlCtx.paramNode = NULL;
         return;
      }

      /* In 5.04L02, X_BROADCOM_COM_VDSL_BrcmPriv1 was renamed to
       * X_BROADCOM_COM_VDSL_35b
       */
      if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_VDSL_BrcmPriv1")))
      {
         snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_VDSL_35b");
         cmsLog_debug("converted X_BROADCOM_COM_VDSL_BrcmPriv1 to %s", buf);
      }
      
   /**************** end config file conversion ****************/
#endif

      if (strcmp(nxmlCtx.paramNode->name, buf))
      {
         cmsLog_error("unexpected end tag, expected param name %s got %s",
                      nxmlCtx.paramNode->name, buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else if (nxmlCtx.mdmObj == NULL)
      {
         cmsLog_error("no mdmObj but end param tag.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else if ((nxmlCtx.paramValue == NULL) &&
               ((nxmlCtx.paramNode->type != MPT_STRING)  &&
                (nxmlCtx.paramNode->type != MPT_DATE_TIME) &&
                (nxmlCtx.paramNode->type != MPT_BASE64) &&
                (nxmlCtx.paramNode->type != MPT_UUID) &&
                (nxmlCtx.paramNode->type != MPT_IP_ADDR) &&
                (nxmlCtx.paramNode->type != MPT_MAC_ADDR) &&
                (nxmlCtx.paramNode->type != MPT_HEX_BINARY) &&
                (nxmlCtx.paramNode->type != MPT_DECIMAL)))
      {
         /* non-string types must have a value */
         cmsLog_error("no param value but end param tag.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         CmsRet r1=CMSRET_SUCCESS;
         CmsRet r2=CMSRET_SUCCESS;

#ifdef CMS_CONFIG_COMPAT
         /************* Begin convert v1 config file ****************/
         if ((nxmlCtx.versionMajor == 1) &&
             (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "AdminPassword") ||
              !cmsUtl_strcmp(nxmlCtx.paramNode->name, "SupportPassword") ||
              !cmsUtl_strcmp(nxmlCtx.paramNode->name, "UserPassword") ||
              (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "Password") && !cmsUtl_strcmp(nxmlCtx.objNode->name, "WANPPPConnection"))))
         {
            cmsLog_notice("v1 password detected, paramNode->name=%s", nxmlCtx.paramNode->name);
            convert_v1_password();
         }
         
         /************* End convert v1 config file ****************/
         /*
          * Prior to 4.16L.01, the LinkEncapsulationUsed param was incorrectly
          * set to MDMVS_G_992_3_ANNEX_K_PTM (PTM over ADSL).  Starting with
          * 4.16L.01, all CMS code has been changed to use
          * MDMVS_G_993_2_ANNEX_K_PTM (PTM over VDSL).  If we detect a config
          * file with the old incorrect setting, just convert it.
          * Since the old/incorrect string and the new/correct string are
          * the same length, we can just replace inside the same buffer.
          */
         if (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "LinkEncapsulationUsed")  &&
             !cmsUtl_strcmp(nxmlCtx.paramValue, MDMVS_G_992_3_ANNEX_K_PTM))
         {
            snprintf(nxmlCtx.paramValue, len+1, "%s", MDMVS_G_993_2_ANNEX_K_PTM);
            cmsLog_debug("converted %s to %s",
                         MDMVS_G_992_3_ANNEX_K_PTM, nxmlCtx.paramValue);
         }


         if ((nxmlCtx.versionMajor < 3) &&
             (nxmlCtx.objNode->oid == MDMOID_L3_FORWARDING) &&
             (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "DefaultConnectionService")) &&
             (nxmlCtx.paramValue != NULL) &&
             (nxmlCtx.loadMdm))
         {
            cmsLog_debug("converting v1/v2 defaultConnectionService");
            convert_v1_v2_defaultGateway();
         }

         /************* End convert v1,v2 config file ****************/
#endif

         if (nxmlCtx.paramNode->flags & PRN_CONFIG_PASSWORD)
         {
            char *plaintext=NULL;
            UINT32 plaintextLen;

            /* in some very weird corner cases (e.g. unit tests), password may be blank */
            if (nxmlCtx.paramValue)
            {
               r1 = cmsB64_decode(nxmlCtx.paramValue, (unsigned char **) &plaintext, &plaintextLen);
               if (r1 == CMSRET_SUCCESS)
               {
                  cmsLog_debug("decoded string=%s", plaintext);
                  cmsMem_free(nxmlCtx.paramValue);
                  nxmlCtx.paramValue = plaintext;
               }
            }
         }

         if ((r1 != CMSRET_SUCCESS) ||
             ((r2 = mdm_validateParamNodeString(nxmlCtx.paramNode, nxmlCtx.paramValue)) != CMSRET_SUCCESS))
         {
            cmsLog_error("invalid string %s", nxmlCtx.paramValue);
            nxmlCtx.ret = (r1 != CMSRET_SUCCESS) ? r1 : r2;
         }
         else
         {
            r2 = mdm_setParamNodeString(nxmlCtx.paramNode, nxmlCtx.paramValue, mdmLibCtx.allocFlags, nxmlCtx.mdmObj);
            if (r2 != CMSRET_SUCCESS)
            {
               nxmlCtx.ret = r2;
            }
            else
            {
               cmsLog_debug("set obj %s :: param %s -> value %s",
                            nxmlCtx.objNode->name,
                            nxmlCtx.paramNode->name,
                            nxmlCtx.paramValue);
            }

            if ((nxmlCtx.ret == CMSRET_SUCCESS) &&
                (nxmlCtx.loadMdm) &&
                (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange || nxmlCtx.attr.setAltNotif))
            {

               cmsLog_debug("set single param attr notif=(%d)%d altNotif=(%d)%d access=(%d)0x%x",
                            nxmlCtx.attr.notificationChange,
                            nxmlCtx.attr.notification,
                            nxmlCtx.attr.setAltNotif,
                            nxmlCtx.attr.altNotif,
                            nxmlCtx.attr.accessBitMaskChange,
                            nxmlCtx.attr.accessBitMask);

               r2 = mdm_setSingleParamNodeAttributes(nxmlCtx.paramNode,
                                                     &(nxmlCtx.iidStack),
                                                     &(nxmlCtx.attr),
                                                     FALSE);
               if (r2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("setSingle attr failed for %s %s notification=(%d)%d altNotification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                               nxmlCtx.paramNode->name,
                               cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                               nxmlCtx.attr.notificationChange,
                               nxmlCtx.attr.notification,
                               nxmlCtx.attr.setAltNotif,
                               nxmlCtx.attr.altNotif,
                               nxmlCtx.attr.accessBitMaskChange,
                               nxmlCtx.attr.accessBitMask, 
                               r2);
                  nxmlCtx.ret = r2;
               }
            }

            nxmlCtx.attr.accessBitMaskChange = 0;
            nxmlCtx.attr.notificationChange = 0;
            nxmlCtx.attr.setAltNotif = 0;
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
         nxmlCtx.paramNode = NULL;
      }
   }
   else
   {
      /*
       * This must be the end tag of an object.
       */
#ifdef CMS_CONFIG_COMPAT
      /*
       * In December 2011, 4in6Tunnel and 6in4Tunnel were
       * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
       */
      if ((nxmlCtx.versionMajor < 4) && (nxmlCtx.objNode != NULL))
      {
         convert_v3_tunnelObjects(nxmlCtx.objNode->oid, buf);
      }

      /*
       * In 4.14, X_BROADCOM_COM_LineSetting was renamed to
       * X_BROADCOM_COM_DectLineSetting
       */
      if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_LineSetting")))
      {
         snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_DectLineSetting");
         cmsLog_debug("converted X_BROADCOM_COM_LineSetting to %s", buf);
      }

      /* In 5.04L02, X_BROADCOM_COM_VDSL_BrcmPriv1 was renamed to
       * X_BROADCOM_COM_VDSL_35b
       */
      if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_VDSL_BrcmPriv1")))
      {
         snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_VDSL_35b");
         cmsLog_debug("converted X_BROADCOM_COM_VDSL_BrcmPriv1 to %s", buf);
      }

      /*
       * In 4.14L.04 (Oct 2013), X_BROADCOM_COM_MLDSnoopingConfig was moved
       * out from under X_BROADCOM_COM_IPv6LANHostConfigManagement.  But
       * old config files will still have this end tag, so replace it
       * with the expected endTag of LANDevice and also don't pop up
       * to the next level because we already popped up to the right
       * level at the end of X_BROADCOM_COM_MLDSnoopingConfig.
       */
      if (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_IPv6LANHostConfigManagement") &&
          (nxmlCtx.objNode->oid == MDMOID_LAN_DEV))
      {
         sprintf(buf, "LANDevice");
         compatNormalPop = FALSE;
      }
      
#endif /* CMS_CONFIG_COMPAT */


      /* verify the end tag matches the start tag */
      if ((!strcmp(nxmlCtx.objNode->name, buf)) ||
          ((nxmlCtx.objNode == mdmShmCtx->rootObjNode) && (!strcmp(buf, CONFIG_FILE_TOP_NODE))))
      {
         /* this is the correct tag matching case, do nothing. */
      }
      else
      {
         cmsLog_error("unexpected end tag, expected obj name %s got %s",
                      nxmlCtx.objNode->name, buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }


      /*
       * As a special case check, there could be a snipet that looks
       * like this which is at the end of the config file:
       *       <someobject notification=2 accessList=tr69c,telnetd>
       *       </someobject>
       *     </someParentObject>
       *   </InternetGatewayDevice>
       * </DslCpeConfig>
       * (There is no transition to another object or a param.)
       * In which case, the next few lines of code will set the attribute for
       * that object.
       */
      if ((nxmlCtx.ret == CMSRET_SUCCESS) &&
          (nxmlCtx.loadMdm) &&
          (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange || nxmlCtx.attr.setAltNotif))
      {
         CmsRet r2;

         cmsLog_debug("set sub-tree attr notif=(%d)%d altNotif=(%d)%d access=(%d)0x%x",
                      nxmlCtx.attr.notificationChange,
                      nxmlCtx.attr.notification,
                      nxmlCtx.attr.setAltNotif,
                      nxmlCtx.attr.altNotif,
                      nxmlCtx.attr.accessBitMaskChange,
                      nxmlCtx.attr.accessBitMask);

         r2 = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                            &(nxmlCtx.iidStack),
                                            &(nxmlCtx.attr),
                                            FALSE);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d altNotification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                         nxmlCtx.objNode->name,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.setAltNotif,
                         nxmlCtx.attr.altNotif,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask,
                         r2);
            nxmlCtx.ret = r2;
         }

         nxmlCtx.attr.accessBitMaskChange = 0;
         nxmlCtx.attr.notificationChange = 0;
         nxmlCtx.attr.setAltNotif = 0;
      }

      /*
       * We hit the end tag of a leaf object or parent object.  If parent
       * object, no action here because the parent object was already pushed
       * into the MDM when we transitioned into the child object.
       *
       * In BDK, for compat mode with monolithic TR181 config files, we need
       * to do some additional fixups before pushing certain objects into the
       * MDM.
       */
      if (nxmlCtx.ret == CMSRET_SUCCESS)
      {
         if (nxmlCtx.nextInstanceNode)
         {
            // This was a line such as:
            // <FirmwareImage nextInstance="3" ></FirmwareImage>
            // No need to set the mdmObj, just free it.
            mdm_freeObject(&(nxmlCtx.mdmObj));
            nxmlCtx.nextInstanceNode = FALSE;
         }
         else if (nxmlCtx.mdmObj != NULL)
         {
            if (nxmlCtx.loadMdm)
            {
               if (!cmsMdm_isCmsClassic())
               {
                  // We are in BDK Distributed MDM mode.  Do additional
                  // filtering for this BDK component.
                  filterMonolithicObjsForComponent();
               }

              if (nxmlCtx.mdmObj)
              {
                  CmsRet r2;
                  cmsLog_debug("setting obj %s %s",
                         mdm_oidToGenericPath(*((MdmObjectId *)nxmlCtx.mdmObj)),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));

                  r2 = mdm_setObject(&(nxmlCtx.mdmObj), &(nxmlCtx.iidStack), FALSE);
                  if (r2 != CMSRET_SUCCESS)
                  {
                     cmsLog_error("setObject failed, %d", r2);
                     mdm_freeObject(&(nxmlCtx.mdmObj));
                     nxmlCtx.ret = r2;
                  }
              }
            }
            else
            {
               // loadMdm == FALSE, just validating, so free the mdmObj without
               // pushing into MDM.
               mdm_freeObject(&(nxmlCtx.mdmObj));
            }
         }
      }


      if ((nxmlCtx.objNode->parent != NULL) && compatNormalPop)
      {
         /* normal end of object processing. */

         if (IS_INDIRECT2(nxmlCtx.objNode))
         {
            POP_INSTANCE_ID(&(nxmlCtx.iidStack));
         }

         nxmlCtx.objNode = nxmlCtx.objNode->parent;
         cmsLog_debug("pop up to %s %s",
                      mdm_oidToGenericPath(nxmlCtx.objNode->oid),
                      cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
      }

      /*
       * Remember the fact we have seen the end tag for this object.
       * This is needed for the special case where we have an object
       * called WEPKey and a parameter called WEPKey and we need to
       * know whether we are about to load the WEPKey object or the
       * WEPKey parameter.  WEPKey object can only be loaded when
       * the end tag of the previous object has been seen.
       */
      nxmlCtx.gotCurrObjEndTag = TRUE;
   }
}


static const char *intfFullpathToCompName(const char *intfFullpath)
{
   if (strstr(intfFullpath, "Device.ATM.") ||
       strstr(intfFullpath, "Device.PTM.") ||
       strstr(intfFullpath, "Device.DSL."))
   {
      return BDK_COMP_DSL;
   }
   else if (strstr(intfFullpath, "WiFi.SSID."))
   {
      return BDK_COMP_WIFI;
   }
   return BDK_COMP_SYSMGMT;
}

void filterMonolithicObjsForComponent()
{
   const char *compName=NULL;
   char *intfFullpath="";
   MdmPathDescriptor delPathDesc=EMPTY_PATH_DESCRIPTOR;
   MdmObjectId oid;

   if (nxmlCtx.mdmObj == NULL)
      return;

   oid=GET_MDM_OBJECT_ID(nxmlCtx.mdmObj);

   // In theory, we should also handle QOS_QUEUESTATS and QOS_SHAPER, but
   // I don't think we ever created or used those in PURE_TR181
   if ((oid == MDMOID_DEV2_INTERFACE_STACK) ||
       (oid == MDMOID_DEV2_QOS_QUEUE))
   {
#ifdef DMP_DEVICE2_BASELINE_1
      if (oid == MDMOID_DEV2_INTERFACE_STACK)
      {
         Dev2InterfaceStackObject *intfStackObj = (Dev2InterfaceStackObject *)nxmlCtx.mdmObj;
         intfFullpath = intfStackObj->higherLayer;
      }
      else if (oid == MDMOID_DEV2_QOS_QUEUE)
      {
         Dev2QosQueueObject *qObj = (Dev2QosQueueObject *)nxmlCtx.mdmObj;
         intfFullpath = qObj->interface;
      }
#endif

      compName = intfFullpathToCompName(intfFullpath);
      if (strcmp(compName, mdmShmCtx->compName))
      {
         // This mdmObj does not belong to this component
         cmsLog_debug("%s: ignore oid=%d intfFullpath %s",
                      mdmShmCtx->compName, oid, intfFullpath);
         delPathDesc.oid = oid;
         delPathDesc.iidStack = nxmlCtx.iidStack;
         mdm_deleteObjectInstance(&delPathDesc, NULL, NULL);
         mdm_freeObject(&(nxmlCtx.mdmObj));
      }
      else
      {
         // This mdmObj belongs to this component, but if this component
         // is not sysmgmt, we need to move it to a higher instance number.
         // (delete current instance and create new instance the higher
         // instance number).
         if (strcmp(mdmShmCtx->compName, BDK_COMP_SYSMGMT))
         {
            UINT32 iid = PEEK_INSTANCE_ID(&nxmlCtx.iidStack);
            if (iid < mdmShmCtx->multiCompFirstInstanceId)
            {
               delPathDesc.oid = oid;
               delPathDesc.iidStack = nxmlCtx.iidStack;
               mdm_deleteObjectInstance(&delPathDesc, NULL, NULL);
               iid = POP_INSTANCE_ID(&nxmlCtx.iidStack);
               iid += mdmShmCtx->multiCompFirstInstanceId - 1;
               PUSH_INSTANCE_ID(&nxmlCtx.iidStack, iid);
               cmsLog_debug("%s: offset oid %d to new instance %d",
                            mdmShmCtx->compName, oid, iid);
               // This just creates an empty object at the new instance id,
               // the caller will push the nxmlCtx.mdmObj into the MDM.
               mdm_createSubTree(nxmlCtx.objNode,
                                 nxmlCtx.objNode->instanceDepth,
                                 &(nxmlCtx.iidStack), NULL, NULL);
            }
         }
      }
   }
#ifdef DMP_DEVICE2_BASELINE_1
   else if ((oid == MDMOID_DEV2_QOS_CLASSIFICATION))
   {
      if(cmsMdm_isBdkSysmgmt())
      {
         // When converting a CMS config file to BDK config file, we move the
         // DSL and Wifi queues to a higher instance number in the code above.
         // If a Classification obj references one of the moved queues, the
         // queue number must also be updated.
         Dev2QosClassificationObject *classObj = (Dev2QosClassificationObject *) nxmlCtx.mdmObj;
         if ((!cmsUtl_strncmp(classObj->X_BROADCOM_COM_egressInterface, "atm", 3) ||
              !cmsUtl_strncmp(classObj->X_BROADCOM_COM_egressInterface, "ptm", 3)) &&
             (classObj->X_BROADCOM_COM_ClassQueue < COMP_DSL_FIRST_INSTANCE_ID))
         {
            cmsLog_debug("fix DSL classification obj, egress=%s queueNum=%d => %d",
                         classObj->X_BROADCOM_COM_egressInterface,
                         classObj->X_BROADCOM_COM_ClassQueue,
                         classObj->X_BROADCOM_COM_ClassQueue + COMP_DSL_FIRST_INSTANCE_ID - 1);
            classObj->X_BROADCOM_COM_ClassQueue += COMP_DSL_FIRST_INSTANCE_ID - 1;
         }
         else if (!cmsUtl_strncmp(classObj->X_BROADCOM_COM_egressInterface, "wl", 2) &&
                  (classObj->X_BROADCOM_COM_ClassQueue < COMP_WIFI_FIRST_INSTANCE_ID))
         {
            cmsLog_debug("fix Wifi classification obj, egress=%s queueNum=%d => %d",
                         classObj->X_BROADCOM_COM_egressInterface,
                         classObj->X_BROADCOM_COM_ClassQueue,
                         classObj->X_BROADCOM_COM_ClassQueue + COMP_WIFI_FIRST_INSTANCE_ID - 1);
            classObj->X_BROADCOM_COM_ClassQueue += COMP_WIFI_FIRST_INSTANCE_ID - 1;
         }
      }
      else
      {
         // The Classification object should only exist in the sysmgmt component.
         // If it exists elsewhere, delete it.
         delPathDesc.oid = oid;
         delPathDesc.iidStack = nxmlCtx.iidStack;
         mdm_deleteObjectInstance(&delPathDesc, NULL, NULL);
         mdm_freeObject(&(nxmlCtx.mdmObj));
      }
   }
#endif  /* DMP_DEVICE2_BASELINE_1 */

   return;
}


UBOOL8 isAllWhiteSpace(const char *p, UINT32 len)
{
   UINT32 i=0;

   cmsLog_debug("checking %d bytes starting at 0x%x", len, *p);
   for (i=0; i<len; i++)
   {
      cmsLog_debug("p is 0x%x", *p);
      if (*p != 0 &&    /* terminator byte */
          *p != 0xa &&  /* new line */
          *p != 0xd &&  /* carriage return */
          *p != 0x20)   /* space */
      {
         return FALSE;
      }
      p++;
   }

   return TRUE;
}


CmsRet parseVersionNumber(const char *buf, UINT32 *major, UINT32 *minor)
{
   char *copy, *midpoint;
   CmsRet ret;

   copy = cmsMem_strdup(buf);

   midpoint = strstr(copy, ".");

   if (midpoint == NULL)
   {
      /*
       * There is no .  That is OK, treat the whole buffer as a major number.
       */

      ret = cmsUtl_strtoul(copy, NULL, 0, major);
      *minor = 0;
   }
   else
   {
      /* break the string */
      *midpoint = (char) 0;
      midpoint++;

      ret = cmsUtl_strtoul(copy, NULL, 0, major);
      if (ret == CMSRET_SUCCESS)
      {
         ret = cmsUtl_strtoul(midpoint, NULL, 0, minor);
      }
   }


   cmsMem_free(copy);

   return ret;
}


#ifdef CMS_CONFIG_COMPAT

/*
 * Convert version 1 config file password field to version 2+ config file password.
 * In version 1, the passwords were stored in cleartext.  In version 2 and beyond,
 * we expect these fields to be base64 encoded.  So encode the password.
 */
void convert_v1_password(void)
{
   char *encryptedStr=NULL;

   if (nxmlCtx.paramValue)
   {
      cmsB64_encode((unsigned char *) nxmlCtx.paramValue, strlen(nxmlCtx.paramValue)+1, &encryptedStr);

      cmsMem_free(nxmlCtx.paramValue);

      nxmlCtx.paramValue = encryptedStr;
   }
}


void convert_v1_v2_defaultGateway(void)
{
   char fullPath[BUFLEN_1024];
   char ifName[CMS_IFNAME_LENGTH];
   MdmPathDescriptor pathDesc;
   MdmParamNode *paramNode=NULL;
   void *obj=NULL;
   CmsRet ret;

   /* need to put a . at the end of the pathname for conversion */
   snprintf(fullPath, sizeof(fullPath), "%s.", nxmlCtx.paramValue);

   cmsLog_debug("looking up %s", fullPath);
   if ((ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", fullPath, ret);
      return;
   }

   /* get the ppp or ip object, this is a direct pointer to the object, so don't free it */
   ret = mdm_getObjectPointer(pathDesc.oid, &(pathDesc.iidStack), &obj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the MDM object for oid %d iidStack %s",
                   pathDesc.oid, cmsMdm_dumpIidStack(&(pathDesc.iidStack)));
      return;
   }

   /* get the ifName */
   if (pathDesc.oid == MDMOID_WAN_PPP_CONN)
   {
      strncpy(ifName, ((WanPppConnObject *) obj)->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
      ifName[sizeof(ifName)-1] = '\0';      
   }
   else if (pathDesc.oid == MDMOID_WAN_IP_CONN)
   {
      strncpy(ifName, ((WanIpConnObject *) obj)->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
      ifName[sizeof(ifName)-1] = '\0';          
   }
   else 
   {
      cmsLog_error("unexpected oid %d", pathDesc.oid);
      return;
   }

   cmsLog_debug("ifname is %s", ifName);

   /* get the paramNode ptr to the X_BROADCOM_COM_DefaultConnectionServices parameter in the same object */
   if ((paramNode = mdm_getParamNode(nxmlCtx.objNode->oid, "X_BROADCOM_COM_DefaultConnectionServices")) == NULL)
   {
      cmsLog_error("could not get paramNode for X_BROADCOM_COM_DefaultConnectionServices");
      return;
   }

   /* directly set the parameter */
   ret = mdm_setParamNodeString(paramNode, ifName, mdmLibCtx.allocFlags, nxmlCtx.mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set X_BROADCOM_COM_DefaultConnectionServices parameter to %s, ret=%d",
                   ifName, ret);
      return;
   }
                   
   /* free the current param value and set to NULL */
   CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
   cmsLog_debug("conversion successful, set old DefaultConnectionService to %s", nxmlCtx.paramValue);

   return;
}


/*
 * In December 2011, 4in6Tunnel and 6in4Tunnel were
 * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
 */
static void convert_v3_tunnelObjects(MdmObjectId oid, char *buf)
{
   if (MDMOID_IP_TUNNEL == oid ||
       MDMOID_IPV4IN_IPV6_TUNNEL == oid ||
       MDMOID_IPV6IN_IPV4_TUNNEL == oid)
   {
      if (!cmsUtl_strcmp(buf, "4in6Tunnel"))
      {
         cmsLog_debug("convert 4in6Tunnel to V4inV6Tunnel");
         sprintf(buf, "V4inV6Tunnel");
      }
      else if (!cmsUtl_strcmp(buf, "6in4Tunnel"))
      {
         cmsLog_debug("convert 6in4Tunnel to V6inV4Tunnel");
         sprintf(buf, "V6inV4Tunnel");
      }
   }
}

#endif  /* CMS_CONFIG_COMPAT */
