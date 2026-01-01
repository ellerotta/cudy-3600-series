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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1


/* This file initializes the TR181 and X_BROADCOM_COM_Dev2_Wifi objects.
 * This file als will contain the Wifi as WAN initialization (if any)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "cms.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "mdm_initwlan.h"
#include "wlsysutil.h"

/* forward declaration */
CmsRet mdm_initWifiAccessPoint_dev2(void);
CmsRet mdm_initWifiAfc_dev2(void);

/* local functions */
static CmsRet addWifiRadioInstance(const char *ifname, int instanceNum, UINT32 *ssidAdded);
static CmsRet addWifiSsidInstance(UINT32 radioIdx, const char *radioFullPath, UINT32 ssidIdx);
static CmsRet addWifiAccessPoint(UINT32 radioIdx, const char *ssidFullPath, UINT32 ssidIdx);
static CmsRet addMobappdPairings(int num);
static CmsRet validateWiFiRadioInstance(int instanceNum);

/** This is the "weak" mdm_adjustForHardware_dev2.
 *  It will only be called in a distributed MDM build in the WIFI component.
 */
#pragma weak mdm_adjustForHardware_dev2
CmsRet mdm_adjustForHardware_dev2()
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("Distributed MDM build: adjustForHardware for WIFI only");

   /* init Wifi on LAN side, meaning Access Point */
   /* or should it do both LAN side and WAN side at the same time ? */
   if ((ret = mdm_initWifiAccessPoint_dev2()) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_initWifi_dev2 failed, ret=%d", ret);
   }
   else
   {
       ret = mdm_initWifiAfc_dev2();
   }

   return ret;
}


CmsRet mdm_initWifiAfc_dev2(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2WifiMobappdObject  *mobObj;
   Dev2WifiMobappdPairingObject *pairingObj;

   PUSH_INSTANCE_ID(&iidStack, 1);
   ret = mdm_getObject(MDMOID_DEV2_WIFI_MOBAPPD_PAIRING, &iidStack, (void **)&pairingObj);
   if (ret == CMSRET_SUCCESS)
   {
       //mobappd pairing objects have been created, do nothing.
       mdm_freeObject((void **)&pairingObj);
       return ret;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   ret = mdm_getObject(MDMOID_DEV2_WIFI_MOBAPPD, &iidStack, (void **)&mobObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get DEV2_WIFI object, ret=%d", ret);
   }
   ret = addMobappdPairings(mobObj->maxNumPairedApp);
   mdm_freeObject((void **)&mobObj);
   return ret;
}

CmsRet mdm_initWifiAccessPoint_dev2(void)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2WifiObject *wifiObj=NULL;
   int i;
   CmsRet ret = CMSRET_SUCCESS;
   int  wl_cnt = wlgetintfNo();
   UINT32 numSsidAdded=0;

#if defined(BUILD_BCM_WLAN_DPDCTL)
    /* Include dummy interfaces due to DPD powered down interfaces */
    wl_cnt += wlgetdummyintfNo();
#endif /* BUILD_BCM_WLAN_DPDCTL */

   ret = mdm_getObject(MDMOID_DEV2_WIFI, &iidStack, (void **) &wifiObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get DEV2_WIFI object, ret=%d", ret);
      return ret;
   }

   cmsLog_notice("actual Wifi radio=%d, radioNum=%d ssidNum=%d APNum=%d",
                 wl_cnt,
                 wifiObj->radioNumberOfEntries,
                 wifiObj->SSIDNumberOfEntries,
                 wifiObj->accessPointNumberOfEntries);

   for ( i = 0 ; ((i < wifiObj->radioNumberOfEntries && i < wl_cnt) && ret == CMSRET_SUCCESS) ; i++)
   {
      ret = validateWiFiRadioInstance(i+1);
   }

   if (wl_cnt <= (SINT32) wifiObj->radioNumberOfEntries || ret != CMSRET_SUCCESS)
   {
      /* MDM already describes actual hardware (or more), so return */
      mdm_freeObject((void **) &wifiObj);
      return ret;
   }

   /*
    * If we get here, actual count of hardware instances is greater than
    * what is in the MDM, so add new instances in the MDM.
    */
   for ( i=(SINT32) wifiObj->radioNumberOfEntries; i<wl_cnt && ret == CMSRET_SUCCESS; i++ )
   {
      char nameBuf[CMS_IFNAME_LENGTH];

      sprintf(nameBuf, "wl%d", i);

      ret = addWifiRadioInstance( nameBuf, i+1, &numSsidAdded);
   }

   if (ret == CMSRET_SUCCESS)
   {
      /* in mdm_init code we have to manually update the xyzNumberOfEntries */
      wifiObj->radioNumberOfEntries = wl_cnt;
      wifiObj->SSIDNumberOfEntries += numSsidAdded;
      wifiObj->accessPointNumberOfEntries += numSsidAdded;

      if ((ret = mdm_setObject((void **) &wifiObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set DEV2_Wifi object, ret=%d", ret);
      }
   }


   mdm_freeObject((void **) &wifiObj);

   return ret;
}


CmsRet addWifiRadioInstance(const char *ifname, int instanceNum, UINT32 *numSsidAdded)
{
   MdmPathDescriptor pathDesc;
   Dev2WifiRadioObject *radioObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 ssidPerRadio, i;
   char *radioFullPath=NULL;

   cmsLog_debug("add %s at instance %d", ifname, instanceNum);

   /* add a radio object at the specified instance number */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), instanceNum);

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("addObjectInstance failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &radioObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("getObject failed,  ret=%d", ret);
      return ret;
   }

   ssidPerRadio = wlgetVirtIntfNo( instanceNum-1 );
   radioObj->enable = TRUE;
   radioObj->autoChannelSupported = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(radioObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(radioObj->guardInterval, MDMVS_AUTO, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(radioObj->transmitPowerSupported, "0,20,40,60,80,100", mdmLibCtx.allocFlags);
   radioObj->maxSupportedSSIDs = ssidPerRadio;


   // leaving radioObj->upstream in its default value of FALSE means LAN side
   ret = mdm_setObject((void **) &radioObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&radioObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set radioObj. ret=%d", ret);
      return ret;
   }

   /*
    * Create the SSID and AccessPoint objects
    */
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &radioFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get radio fullpath, ret=%d", ret);
      return ret;
   }

   for (i = 0; i < ssidPerRadio; i++)
   {
      /* SSID creation will trigger AccessPoint creation */
      ret = addWifiSsidInstance(instanceNum-1, radioFullPath, i);
      if (ret != CMSRET_SUCCESS)
      {
         break;
      }
   }

   (*numSsidAdded) += ssidPerRadio;

   CMSMEM_FREE_BUF_AND_NULL_PTR(radioFullPath);

   return ret;
}

static CmsRet addMobappdPairings(int num)
{
   int i;
   CmsRet ret = CMSRET_SUCCESS;

   if (num <= 0)
       return ret;

   for (i = 0; i < num; i++)
   {
      MdmPathDescriptor pathDesc;
      CmsRet ret = CMSRET_SUCCESS;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_WIFI_MOBAPPD_PAIRING;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("addObjectInstance failed, ret=%d", ret);
         return ret;
      }
   }
   return ret;
}

static unsigned int calculateExistSSIDs(int radioIdx)
{
   int num = 0;
   Dev2WifiSsidObject *ssidObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   while ((ret = mdm_getNextObject(MDMOID_DEV2_WIFI_SSID, &iidStack, (void **) &ssidObj)) == CMSRET_SUCCESS)
   {
      if (ssidObj->X_BROADCOM_COM_Adapter == radioIdx)
         num ++;

      mdm_freeObject((void **)&ssidObj);
   }
   return num;
}

CmsRet validateWiFiRadioInstance(int instanceNum)
{
   MdmPathDescriptor pathDesc;
   Dev2WifiRadioObject *radioObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 ssidPerRadio = 0;

   /* get radio object at the specified instance number */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), instanceNum);

   if ((ret = mdm_getObject(MDMOID_DEV2_WIFI_RADIO, &(pathDesc.iidStack), (void **) &radioObj)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("Device.WiFi.Radio.%d. does not exist! create it later", instanceNum);
      return CMSRET_SUCCESS;
   }

   // correct maxSupportedSSIDs if the loaded config has not assigned value
   if (radioObj->maxSupportedSSIDs == 0)
   {
      ssidPerRadio = wlgetVirtIntfNo(instanceNum-1);
      if (ssidPerRadio > 0)
      {
         radioObj->maxSupportedSSIDs = ssidPerRadio;
      }
      // could not retrive from wifi driver. maybe DPD is on
      else
      {
         radioObj->maxSupportedSSIDs = calculateExistSSIDs(instanceNum - 1);
      }
      cmsLog_notice("Restore Device.WiFi.Radio.%d.MaxSupportedSSIDs to %d", instanceNum, radioObj->maxSupportedSSIDs);
      ret = mdm_setObject((void **) &radioObj, &pathDesc.iidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
         cmsLog_error("Failed to set radioObj. ret=%d", ret);
   }

   mdm_freeObject((void **)&radioObj);

   return ret;
}


CmsRet addWifiSsidInstance(UINT32 radioIdx, const char *radioFullPath, UINT32 ssidIdx)
{
   MdmPathDescriptor pathDesc;
   Dev2WifiSsidObject *ssidObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   char ifname[CMS_IFNAME_LENGTH];
   char ssid[BUFLEN_64];
   char *ssidFullPath=NULL;
   UBOOL8 defaultEnable=FALSE;

   /*
    * add a SSID object as child of the radio object.  Since this SSID obj is
    * a child of a new radio object, I do not need to specify the instance
    * number.  They will always be 1-4 (fixed instances are needed by
    * wlconfig)
    */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_WIFI_SSID;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("addObjectInstance failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &ssidObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("getObject failed,  ret=%d", ret);
      return ret;
   }

   if (ssidIdx == 0)
   {
      snprintf(ifname, sizeof(ifname), "wl%d", radioIdx);
      CMSMEM_REPLACE_STRING_FLAGS(ssidObj->name, ifname, mdmLibCtx.allocFlags);

      snprintf(ssid, sizeof(ssid), "BrcmAP%d", radioIdx);
      CMSMEM_REPLACE_STRING_FLAGS(ssidObj->SSID, ssid, mdmLibCtx.allocFlags);

      defaultEnable = TRUE;
      ssidObj->enable = TRUE;
   }
   else
   {
      snprintf(ifname, sizeof(ifname), "wl%d.%d", radioIdx, ssidIdx);
      CMSMEM_REPLACE_STRING_FLAGS(ssidObj->name, ifname, mdmLibCtx.allocFlags);

      snprintf(ssid, sizeof(ssid), "wl%d_Guest%d", radioIdx, ssidIdx);
      CMSMEM_REPLACE_STRING_FLAGS(ssidObj->SSID, ssid, mdmLibCtx.allocFlags);

      /* second through fourth instances are not automatically enabled */
   }
   CMSMEM_REPLACE_STRING_FLAGS(ssidObj->lowerLayers, radioFullPath, mdmLibCtx.allocFlags);
   ssidObj->X_BROADCOM_COM_Adapter = radioIdx;
   ssidObj->X_BROADCOM_COM_Index = ssidIdx;

   cmsLog_debug("add SSID obj %s %s index=%d", ssid, ifname, ssidIdx);

   ret = mdm_setObject((void **) &ssidObj, &(pathDesc.iidStack), FALSE);
   mdm_freeObject((void **)&ssidObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ssidObj. ret=%d", ret);
   }

   /*
    * Add AccessPoint object
    */
   ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &ssidFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get ssid fullpath, ret=%d", ret);
      return ret;
   }

   addWifiAccessPoint(radioIdx, ssidFullPath, ssidIdx);

   // Do not add bridge port here.  For both CMS TR181 and BDK, ssk will
   // add the bridge port and put this SSID under the correct bridge.
   // See processWifiBridgeRequest.


#ifdef SUPPORT_QOS
   /* Add the QoS queues associated with this SSID */
   /* XXX check original code for policy objects */
   if (ret == CMSRET_SUCCESS)
   {
      ret = wladdDefaultWlanQueueObject(ssidFullPath, defaultEnable);
   }
#endif

   CMSMEM_FREE_BUF_AND_NULL_PTR(ssidFullPath);

   return ret;
}


CmsRet addWifiAccessPoint(UINT32 radioIdx, const char *ssidFullPath, UINT32 ssidIdx)
{
   MdmPathDescriptor pathDesc;
   Dev2WifiAccessPointObject *apObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_WIFI_ACCESS_POINT;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("addObjectInstance failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &apObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("getObject failed,  ret=%d", ret);
      return ret;
   }

   if (ssidIdx == 0)
   {
      apObj->enable = TRUE;
   }
   else
   {
      /* second through fourth instances are not automatically enabled */
   }
   CMSMEM_REPLACE_STRING_FLAGS(apObj->SSIDReference, ssidFullPath, mdmLibCtx.allocFlags);
   apObj->X_BROADCOM_COM_Adapter = radioIdx;
   apObj->X_BROADCOM_COM_Index = ssidIdx;
   cmsLog_debug("add AP obj radio=%d index=%d", radioIdx, ssidIdx);

   ret = mdm_setObject((void **) &apObj, &(pathDesc.iidStack), FALSE);
   mdm_freeObject((void **)&apObj);

   return ret;
}

#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */


