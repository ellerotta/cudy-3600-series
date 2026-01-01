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
#include "rcl.h"
#include "cms.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut2_unfwifi.h"


CmsRet rcl_dev2DataElementsObject( _Dev2DataElementsObject *newObj __attribute__((unused)),
                                   const _Dev2DataElementsObject *currObj __attribute__((unused)),
                                       const InstanceIdStack *iidStack __attribute__((unused)),
                                       char **errorParam __attribute__((unused)),
                                       CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmNetworkObject( _Dev2DataElmNetworkObject *newObj __attribute__((unused)),
                                     const _Dev2DataElmNetworkObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmNetworkSsidObject( _Dev2DataElmNetworkSsidObject *newObj __attribute__((unused)),
                                         const _Dev2DataElmNetworkSsidObject *currObj __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)),
                                         char **errorParam __attribute__((unused)),
                                         CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDESsid(1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDESsid(-1);
    }

    return CMSRET_SUCCESS;
}



CmsRet rcl_dev2DataElmMultiapsteeringsummaryObject( _Dev2DataElmMultiapsteeringsummaryObject *newObj __attribute__((unused)),
                                                    const _Dev2DataElmMultiapsteeringsummaryObject *currObj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                                    char **errorParam __attribute__((unused)),
                                                    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmDeviceObject( _Dev2DataElmDeviceObject *newObj __attribute__((unused)),
                                    const _Dev2DataElmDeviceObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEDevice(1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEDevice(-1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDeviceDefault8021qObject( _Dev2DataElmDeviceDefault8021qObject *newObj __attribute__((unused)),
                                                const _Dev2DataElmDeviceDefault8021qObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEDefault8021Q(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEDefault8021Q(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDeviceSsidtovidmappingObject( _Dev2DataElmDeviceSsidtovidmappingObject *newObj __attribute__((unused)),
                                                    const _Dev2DataElmDeviceSsidtovidmappingObject *currObj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                                    char **errorParam __attribute__((unused)),
                                                    CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDESSIDtoVIDMapping(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDESSIDtoVIDMapping(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCacstatusObject( _Dev2DataElmCacstatusObject *newObj __attribute__((unused)),
                                       const _Dev2DataElmCacstatusObject *currObj __attribute__((unused)),
                                       const InstanceIdStack *iidStack __attribute__((unused)),
                                       char **errorParam __attribute__((unused)),
                                       CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACStatus(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACStatus(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCacstatusAvailablechannelObject( _Dev2DataElmCacstatusAvailablechannelObject *newObj __attribute__((unused)),
                                                       const _Dev2DataElmCacstatusAvailablechannelObject *currObj __attribute__((unused)),
                                                       const InstanceIdStack *iidStack __attribute__((unused)),
                                                       char **errorParam __attribute__((unused)),
                                                       CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACAvailableChannel(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACAvailableChannel(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCacstatusNonoccupancychannelObject( _Dev2DataElmCacstatusNonoccupancychannelObject *newObj __attribute__((unused)),
                                                          const _Dev2DataElmCacstatusNonoccupancychannelObject *currObj __attribute__((unused)),
                                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                                          char **errorParam __attribute__((unused)),
                                                          CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACNonOccupancyChannel(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACNonOccupancyChannel(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCacstatusActivechannelObject( _Dev2DataElmCacstatusActivechannelObject *newObj __attribute__((unused)),
                                                    const _Dev2DataElmCacstatusActivechannelObject *currObj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                                    char **errorParam __attribute__((unused)),
                                                    CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACActiveChannel(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACActiveChannel(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDeviceSpruleObject( _Dev2DataElmDeviceSpruleObject *newObj __attribute__((unused)),
                                          const _Dev2DataElmDeviceSpruleObject *currObj __attribute__((unused)),
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDESPRule(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDESPRule(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDeviceIeee1905securityObject( _Dev2DataElmDeviceIeee1905securityObject *newObj __attribute__((unused)),
                                                    const _Dev2DataElmDeviceIeee1905securityObject *currObj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                                    char **errorParam __attribute__((unused)),
                                                    CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEIEEE1905Security(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEIEEE1905Security(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAnticipatedchannelsObject( _Dev2DataElmAnticipatedchannelsObject *newObj __attribute__((unused)),
                                                 const _Dev2DataElmAnticipatedchannelsObject *currObj __attribute__((unused)),
                                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                                 char **errorParam __attribute__((unused)),
                                                 CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannels(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannels(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAnticipatedchannelusageObject( _Dev2DataElmAnticipatedchannelusageObject *newObj __attribute__((unused)),
                                                     const _Dev2DataElmAnticipatedchannelusageObject *currObj __attribute__((unused)),
                                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                                     char **errorParam __attribute__((unused)),
                                                     CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannelUsage(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannelUsage(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAnticipatedchannelusageEntryObject( _Dev2DataElmAnticipatedchannelusageEntryObject *newObj __attribute__((unused)),
                                                          const _Dev2DataElmAnticipatedchannelusageEntryObject *currObj __attribute__((unused)),
                                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                                          char **errorParam __attribute__((unused)),
                                                          CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannelUsageEntry(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAnticipatedChannelUsageEntry(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmMultiapdeviceObject( _Dev2DataElmMultiapdeviceObject *newObj __attribute__((unused)),
                                           const _Dev2DataElmMultiapdeviceObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmMultiapdeviceBackhaulObject( _Dev2DataElmMultiapdeviceBackhaulObject *newObj __attribute__((unused)),
                                                   const _Dev2DataElmMultiapdeviceBackhaulObject *currObj __attribute__((unused)),
                                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                                   char **errorParam __attribute__((unused)),
                                                   CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmMultiapdeviceBackhaulCurrOpertclassprofileObject( _Dev2DataElmMultiapdeviceBackhaulCurrOpertclassprofileObject *newObj __attribute__((unused)),
                                                                        const _Dev2DataElmMultiapdeviceBackhaulCurrOpertclassprofileObject *currObj __attribute__((unused)),
                                                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                                                        char **errorParam __attribute__((unused)),
                                                                        CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEMultiAPDevBackhaulCurrOPClass(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEMultiAPDevBackhaulCurrOPClass(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmMultiapdeviceBackhaulStatsObject( _Dev2DataElmMultiapdeviceBackhaulStatsObject *newObj __attribute__((unused)),
                                                        const _Dev2DataElmMultiapdeviceBackhaulStatsObject *currObj __attribute__((unused)),
                                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                                        char **errorParam __attribute__((unused)),
                                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}



CmsRet rcl_dev2DataElmRadioObject( _Dev2DataElmRadioObject *newObj __attribute__((unused)),
                                   const _Dev2DataElmRadioObject *currObj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDERadio(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDERadio(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmBackhaulStaObject( _Dev2DataElmBackhaulStaObject *newObj __attribute__((unused)),
                                         const _Dev2DataElmBackhaulStaObject *currObj __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)),
                                         char **errorParam __attribute__((unused)),
                                         CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioScanCapableObject( _Dev2DataElmRadioScanCapableObject *newObj __attribute__((unused)),
                                         const _Dev2DataElmRadioScanCapableObject *currObj __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)),
                                         char **errorParam __attribute__((unused)),
                                         CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmRadioScanCapableOpclasschannelsObject( _Dev2DataElmRadioScanCapableOpclasschannelsObject *newObj __attribute__((unused)),
                                                             const _Dev2DataElmRadioScanCapableOpclasschannelsObject *currObj __attribute__((unused)),
                                                             const InstanceIdStack *iidStack __attribute__((unused)),
                                                             char **errorParam __attribute__((unused)),
                                                             CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEScanCapabilityOpClassChannels(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEScanCapabilityOpClassChannels(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioCacCapableObject( _Dev2DataElmRadioCacCapableObject *newObj __attribute__((unused)),
                                             const _Dev2DataElmRadioCacCapableObject *currObj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)),
                                             char **errorParam __attribute__((unused)),
                                             CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioCacCapableCacmethodObject( _Dev2DataElmRadioCacCapableCacmethodObject *newObj __attribute__((unused)),
                                                      const _Dev2DataElmRadioCacCapableCacmethodObject *currObj __attribute__((unused)),
                                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                                      char **errorParam __attribute__((unused)),
                                                      CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACCapabilityCACMethod(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACCapabilityCACMethod(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioCacCapableCacmethodOpclasschannelsObject( _Dev2DataElmRadioCacCapableCacmethodOpclasschannelsObject *newObj __attribute__((unused)),
                                                                     const _Dev2DataElmRadioCacCapableCacmethodOpclasschannelsObject *currObj __attribute__((unused)),
                                                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                                                     char **errorParam __attribute__((unused)),
                                                                     CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECACCapabilityCACMethodOpClassChannels(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECACCapabilityCACMethodOpClassChannels(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCapabilitiesObject( _Dev2DataElmCapabilitiesObject *newObj __attribute__((unused)),
                                          const _Dev2DataElmCapabilitiesObject *currObj __attribute__((unused)),
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCapableWifi6aproleObject( _Dev2DataElmCapableWifi6aproleObject *newObj __attribute__((unused)),
                                                const _Dev2DataElmCapableWifi6aproleObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCapableWifi6bstaroleObject( _Dev2DataElmCapableWifi6bstaroleObject *newObj __attribute__((unused)),
                                                  const _Dev2DataElmCapableWifi6bstaroleObject *currObj __attribute__((unused)),
                                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                                  char **errorParam __attribute__((unused)),
                                                  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCapableAkmfronthaulObject( _Dev2DataElmCapableAkmfronthaulObject *newObj __attribute__((unused)),
                                                 const _Dev2DataElmCapableAkmfronthaulObject *currObj __attribute__((unused)),
                                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                                 char **errorParam __attribute__((unused)),
                                                 CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAKMFrontHaul(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAKMFrontHaul(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmCapableAkmbackhaulObject( _Dev2DataElmCapableAkmbackhaulObject *newObj __attribute__((unused)),
                                                const _Dev2DataElmCapableAkmbackhaulObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAKMBackhaul(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAKMBackhaul(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmCurOpClassProfileObject( _Dev2DataElmCurOpClassProfileObject *newObj __attribute__((unused)),
                                          const _Dev2DataElmCurOpClassProfileObject *currObj __attribute__((unused)),
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDECurrOpClassProfile(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDECurrOpClassProfile(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioDisallowedopclasschannelsObject( _Dev2DataElmRadioDisallowedopclasschannelsObject *newObj __attribute__((unused)),
                                                            const _Dev2DataElmRadioDisallowedopclasschannelsObject *currObj __attribute__((unused)),
                                                            const InstanceIdStack *iidStack __attribute__((unused)),
                                                            char **errorParam __attribute__((unused)),
                                                            CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEDisAllowedOpClassChannels(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEDisAllowedOpClassChannels(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmRadioSpatialreuseObject( _Dev2DataElmRadioSpatialreuseObject *newObj __attribute__((unused)),
                                               const _Dev2DataElmRadioSpatialreuseObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmCapableOpClassProfileObject( _Dev2DataElmCapableOpClassProfileObject *newObj __attribute__((unused)),
                                          const _Dev2DataElmCapableOpClassProfileObject *currObj __attribute__((unused)),
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEOpClassProfile(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEOpClassProfile(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmBssObject( _Dev2DataElmBssObject *newObj __attribute__((unused)),
                                 const _Dev2DataElmBssObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEBSS(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEBSS(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmBssQmdescriptorObject( _Dev2DataElmBssQmdescriptorObject *newObj __attribute__((unused)),
                                             const _Dev2DataElmBssQmdescriptorObject *currObj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)),
                                             char **errorParam __attribute__((unused)),
                                             CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEQMDescriptor(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEQMDescriptor(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmBssMultiapsteeringObject( _Dev2DataElmBssMultiapsteeringObject *newObj __attribute__((unused)),
                                                const _Dev2DataElmBssMultiapsteeringObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmStaObject( _Dev2DataElmStaObject *newObj __attribute__((unused)),
                                 const _Dev2DataElmStaObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDESta(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDESta(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmMultiAPSTAObject( _Dev2DataElmMultiAPSTAObject *newObj __attribute__((unused)),
                                        const _Dev2DataElmMultiAPSTAObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmMultiAPSTASteeringSumStatsObject( _Dev2DataElmMultiAPSTASteeringSumStatsObject *newObj __attribute__((unused)),
                                                        const _Dev2DataElmMultiAPSTASteeringSumStatsObject *currObj __attribute__((unused)),
                                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                                        char **errorParam __attribute__((unused)),
                                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmMultiAPSTASteerHistoryObject( _Dev2DataElmMultiAPSTASteerHistoryObject *newObj __attribute__((unused)),
                                                    const _Dev2DataElmMultiAPSTASteerHistoryObject *currObj __attribute__((unused)),
                                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                                    char **errorParam __attribute__((unused)),
                                                    CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEMultiAPSTASteeringHistory(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEMultiAPSTASteeringHistory(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmStaWifi6capabilitiesObject( _Dev2DataElmStaWifi6capabilitiesObject *newObj __attribute__((unused)),
                                                  const _Dev2DataElmStaWifi6capabilitiesObject *currObj __attribute__((unused)),
                                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                                  char **errorParam __attribute__((unused)),
                                                  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmStaTidqueuesizesObject( _Dev2DataElmStaTidqueuesizesObject *newObj __attribute__((unused)),
                                              const _Dev2DataElmStaTidqueuesizesObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDETIDQueueSizes(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDETIDQueueSizes(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmScanResultObject( _Dev2DataElmScanResultObject *newObj __attribute__((unused)),
                                           const _Dev2DataElmScanResultObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEScanResult(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEScanResult(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmOpClassScanObject( _Dev2DataElmOpClassScanObject *newObj __attribute__((unused)),
                const _Dev2DataElmOpClassScanObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEOpClassScan(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEOpClassScan(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmChannelScanObject( _Dev2DataElmChannelScanObject *newObj __attribute__((unused)),
                const _Dev2DataElmChannelScanObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam,
                CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEChannelScan(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEChannelScan(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmNeighborBSSObject( _Dev2DataElmNeighborBSSObject *newObj __attribute__((unused)),
                const _Dev2DataElmNeighborBSSObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDENeighborBSS(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDENeighborBSS(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DataElmUnassociatedStaObject( _Dev2DataElmUnassociatedStaObject *newObj __attribute__((unused)),
                                             const _Dev2DataElmUnassociatedStaObject *currObj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)),
                                             char **errorParam __attribute__((unused)),
                                             CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEUnassocSTA(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEUnassocSTA(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmMultiapradioObject( _Dev2DataElmMultiapradioObject *newObj __attribute__((unused)),
                                        const _Dev2DataElmMultiapradioObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAssocEventObject( _Dev2DataElmAssocEventObject *newObj __attribute__((unused)),
                                        const _Dev2DataElmAssocEventObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAssocEventDataObject( _Dev2DataElmAssocEventDataObject *newObj __attribute__((unused)),
                                            const _Dev2DataElmAssocEventDataObject *currObj __attribute__((unused)),
                                            const InstanceIdStack *iidStack __attribute__((unused)),
                                            char **errorParam __attribute__((unused)),
                                            CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEAssocEvtData(1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEAssocEvtData(-1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmAssocEvtdataWifi6capabilitiesObject( _Dev2DataElmAssocEvtdataWifi6capabilitiesObject *newObj __attribute__((unused)),
                                        const _Dev2DataElmAssocEvtdataWifi6capabilitiesObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDisassocEventObject( _Dev2DataElmDisassocEventObject *newObj __attribute__((unused)),
                                           const _Dev2DataElmDisassocEventObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmDisassocEventDataObject( _Dev2DataElmDisassocEventDataObject *newObj __attribute__((unused)),
                                               const _Dev2DataElmDisassocEventDataObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEDisassocEvtData(1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEDisassocEvtData(-1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmFailedconnEvtObject( _Dev2DataElmFailedconnEvtObject *newObj __attribute__((unused)),
                                           const _Dev2DataElmFailedconnEvtObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2DataElmFailedconnEvtdataObject( _Dev2DataElmFailedconnEvtdataObject *newObj __attribute__((unused)),
                                           const _Dev2DataElmFailedconnEvtdataObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumDEFailedConnectionEvtData(1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumDEFailedConnectionEvtData(-1);
    }

    return CMSRET_SUCCESS;
}

void rutUtil_modifyNumDEDevice(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DataElmNetworkObject *networkObj = NULL;

   if ((cmsObj_get(MDMOID_DEV2_DATA_ELM_NETWORK, &iidStack, 0, (void *) &networkObj)) == CMSRET_SUCCESS)
   {
      networkObj->deviceNumberOfEntries += delta;
      cmsObj_setFlags(networkObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &networkObj);
   }
}

void rutUtil_modifyNumDESsid(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DataElmNetworkObject *networkObj = NULL;

   if ((cmsObj_get(MDMOID_DEV2_DATA_ELM_NETWORK, &iidStack, 0, (void *) &networkObj)) == CMSRET_SUCCESS)
   {
      networkObj->SSIDNumberOfEntries += delta;
      cmsObj_setFlags(networkObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &networkObj);
   }
}

void rutUtil_modifyNumDERadio(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_RADIO, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_RADIO,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->radioNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDEDefault8021Q(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_DEFAULT8021Q, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_DEFAULT8021Q,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->default8021QNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDESSIDtoVIDMapping(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_SSIDTOVIDMAPPING, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_SSIDTOVIDMAPPING,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->SSIDtoVIDMappingNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDECACStatus(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_CACSTATUS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_CACSTATUS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->CACStatusNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDECACAvailableChannel(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCacstatusObject *cacStatusObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_AVAILABLECHANNEL, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &cacStatusObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_AVAILABLECHANNEL,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   cacStatusObj->CACAvailableChannelNumberOfEntries += delta;
   cmsObj_setFlags(cacStatusObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&cacStatusObj);
}

void rutUtil_modifyNumDECACNonOccupancyChannel(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCacstatusObject *cacStatusObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_NONOCCUPANCYCHANNEL, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &cacStatusObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_NONOCCUPANCYCHANNEL,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   cacStatusObj->CACNonOccupancyChannelNumberOfEntries += delta;
   cmsObj_setFlags(cacStatusObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&cacStatusObj);
}

void rutUtil_modifyNumDECACActiveChannel(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCacstatusObject *cacStatusObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_ACTIVECHANNEL, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &cacStatusObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CACSTATUS, MDMOID_DEV2_DATA_ELM_CACSTATUS_ACTIVECHANNEL,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   cacStatusObj->CACActiveChannelNumberOfEntries += delta;
   cmsObj_setFlags(cacStatusObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&cacStatusObj);
}

void rutUtil_modifyNumDEIEEE1905Security(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_IEEE1905SECURITY, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_IEEE1905SECURITY,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->IEEE1905SecurityNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDESPRule(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_SPRULE, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_DEVICE_SPRULE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->SPRuleNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDEAnticipatedChannels(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->anticipatedChannelsNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDEAnticipatedChannelUsage(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *devObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELUSAGE, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &devObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELUSAGE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   devObj->anticipatedChannelUsageNumberOfEntries += delta;
   cmsObj_setFlags(devObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&devObj);
}

void rutUtil_modifyNumDEAnticipatedChannelUsageEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmAnticipatedchannelusageObject *channelUsageObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELUSAGE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELUSAGE_ENTRY, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &channelUsageObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_DEVICE, MDMOID_DEV2_DATA_ELM_ANTICIPATEDCHANNELUSAGE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   channelUsageObj->entryNumberOfEntries += delta;
   cmsObj_setFlags(channelUsageObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&channelUsageObj);
}

void rutUtil_modifyNumDEMultiAPDevBackhaulCurrOPClass(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmMultiapdeviceBackhaulObject *backhulObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_MULTIAPDEVICE_BACKHAUL, MDMOID_DEV2_DATA_ELM_MULTIAPDEVICE_BACKHAUL_CURR_OPERTCLASSPROFILE, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &backhulObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_MULTIAPDEVICE_BACKHAUL, MDMOID_DEV2_DATA_ELM_MULTIAPDEVICE_BACKHAUL_CURR_OPERTCLASSPROFILE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   backhulObj->currentOperatingClassProfileNumberOfEntries += delta;
   cmsObj_setFlags(backhulObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&backhulObj);
}

void rutUtil_modifyNumDEBSS(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_BSS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_BSS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->BSSNumberOfEntries += delta;
   cmsObj_setFlags(radioObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&radioObj);
}

void rutUtil_modifyNumDESta(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmBssObject *bssObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_BSS, MDMOID_DEV2_DATA_ELM_STA, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &bssObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_BSS, MDMOID_DEV2_DATA_ELM_STA,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   bssObj->STANumberOfEntries += delta;
   cmsObj_setFlags(bssObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&bssObj);
}

void rutUtil_modifyNumDEQMDescriptor(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmBssObject *bssObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_BSS, MDMOID_DEV2_DATA_ELM_BSS_QMDESCRIPTOR, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &bssObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_BSS, MDMOID_DEV2_DATA_ELM_BSS_QMDESCRIPTOR,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   bssObj->QMDescriptorNumberOfEntries += delta;
   cmsObj_setFlags(bssObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&bssObj);
}

void rutUtil_modifyNumDETIDQueueSizes(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmStaObject *staObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_STA, MDMOID_DEV2_DATA_ELM_STA_TIDQUEUESIZES, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &staObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_STA, MDMOID_DEV2_DATA_ELM_STA_TIDQUEUESIZES,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   staObj->TIDQueueSizesNumberOfEntries += delta;
   cmsObj_setFlags(staObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&staObj);
}

void rutUtil_modifyNumDEMultiAPSTASteeringHistory(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmMultiAPSTAObject *multiApStaObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_APST_A, MDMOID_APSTA_STEER_HISTORY, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &multiApStaObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_APST_A, MDMOID_APSTA_STEER_HISTORY,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   multiApStaObj->steeringHistoryNumberOfEntries += delta;
   cmsObj_setFlags(multiApStaObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&multiApStaObj);
}

void rutUtil_modifyNumDEScanResult(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_SCAN_RESULT, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_SCAN_RESULT,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->scanResultNumberOfEntries += delta;
   cmsObj_setFlags(radioObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&radioObj);
}

void rutUtil_modifyNumDEOpClassScan(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmScanResultObject *scanResultObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_SCAN_RESULT, MDMOID_DEV2_DATA_ELM_OP_CLASS_SCAN, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &scanResultObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_SCAN_RESULT, MDMOID_DEV2_DATA_ELM_OP_CLASS_SCAN,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   scanResultObj->opClassScanNumberOfEntries += delta;
   cmsObj_setFlags(scanResultObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&scanResultObj);
}

void rutUtil_modifyNumDEChannelScan(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmOpClassScanObject *opClassScanObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_OP_CLASS_SCAN, MDMOID_DEV2_DATA_ELM_CHANNEL_SCAN, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &opClassScanObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_OP_CLASS_SCAN, MDMOID_DEV2_DATA_ELM_CHANNEL_SCAN,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   opClassScanObj->channelScanNumberOfEntries += delta;
   cmsObj_setFlags(opClassScanObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&opClassScanObj);
}

void rutUtil_modifyNumDENeighborBSS(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmChannelScanObject *channelScanObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CHANNEL_SCAN, MDMOID_BS_S, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &channelScanObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CHANNEL_SCAN, MDMOID_BS_S,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   channelScanObj->neighborBSSNumberOfEntries += delta;
   cmsObj_setFlags(channelScanObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&channelScanObj);
}

void rutUtil_modifyNumDEOpClassProfile(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCapabilitiesObject *capObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_OP_CLASS_PROFILE, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &capObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_OP_CLASS_PROFILE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   capObj->capableOperatingClassProfileNumberOfEntries += delta;
   cmsObj_setFlags(capObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&capObj);
}

void rutUtil_modifyNumDEAKMFrontHaul(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCapabilitiesObject *capObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_AKMFRONTHAUL, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &capObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_AKMFRONTHAUL,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   capObj->AKMFrontHaulNumberOfEntries += delta;
   cmsObj_setFlags(capObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&capObj);
}

void rutUtil_modifyNumDEAKMBackhaul(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmCapabilitiesObject *capObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_AKMBACKHAUL, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &capObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_CAPABILITIES, MDMOID_DEV2_DATA_ELM_CAPABLE_AKMBACKHAUL,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   capObj->AKMBackhaulNumberOfEntries += delta;
   cmsObj_setFlags(capObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&capObj);
}

void rutUtil_modifyNumDECurrOpClassProfile(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_CUR_OP_CLASS_PROFILE, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_CUR_OP_CLASS_PROFILE,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->currentOperatingClassProfileNumberOfEntries += delta;
   cmsObj_setFlags(radioObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&radioObj);
}

void rutUtil_modifyNumDEDisAllowedOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_RADIO_DISALLOWEDOPCLASSCHANNELS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_RADIO_DISALLOWEDOPCLASSCHANNELS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->disAllowedOpClassChannelsNumberOfEntries += delta;
   cmsObj_setFlags(radioObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&radioObj);
}

void rutUtil_modifyNumDEScanCapabilityOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioScanCapableObject *scanCapObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO_SCAN_CAPABLE, MDMOID_DEV2_DATA_ELM_RADIO_SCAN_CAPABLE_OPCLASSCHANNELS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &scanCapObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO_SCAN_CAPABLE, MDMOID_DEV2_DATA_ELM_RADIO_SCAN_CAPABLE_OPCLASSCHANNELS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   scanCapObj->opClassChannelsNumberOfEntries += delta;
   cmsObj_setFlags(scanCapObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&scanCapObj);
}

void rutUtil_modifyNumDECACCapabilityCACMethod(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioCacCapableObject *cacCapObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE, MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &cacCapObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE, MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   cacCapObj->CACMethodNumberOfEntries += delta;
   cmsObj_setFlags(cacCapObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&cacCapObj);
}

void rutUtil_modifyNumDECACCapabilityCACMethodOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioCacCapableCacmethodObject *cacCapMethodObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD, MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD_OPCLASSCHANNELS, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &cacCapMethodObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD, MDMOID_DEV2_DATA_ELM_RADIO_CAC_CAPABLE_CACMETHOD_OPCLASSCHANNELS,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   cacCapMethodObj->opClassChannelsNumberOfEntries += delta;
   cmsObj_setFlags(cacCapMethodObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&cacCapMethodObj);
}

void rutUtil_modifyNumDEUnassocSTA(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmRadioObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_UNASSOCIATED_STA, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_UNASSOCIATED_STA,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->unassociatedSTANumberOfEntries += delta;
   cmsObj_setFlags(radioObj, &ancestorIidStack, OSF_NO_RCL_CALLBACK);
   cmsObj_free((void **)&radioObj);
}

void rutUtil_modifyNumDEAssocEvtData(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DataElmAssocEventObject *assocEvtObj = NULL;

   if ((cmsObj_get(MDMOID_DEV2_DATA_ELM_ASSOC_EVENT, &iidStack, 0, (void *) &assocEvtObj)) == CMSRET_SUCCESS)
   {
      assocEvtObj->associationEventDataNumberOfEntries += delta;
      cmsObj_setFlags(assocEvtObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &assocEvtObj);
   }
}

void rutUtil_modifyNumDEDisassocEvtData(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DataElmDisassocEventObject *disassocEvtObj = NULL;
      
   if ((cmsObj_get(MDMOID_DEV2_DATA_ELM_DISASSOC_EVENT, &iidStack, 0, (void *) &disassocEvtObj)) == CMSRET_SUCCESS)
   {
      disassocEvtObj->disassociationEventDataNumberOfEntries += delta;
      cmsObj_setFlags(disassocEvtObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &disassocEvtObj);
   }
}

void rutUtil_modifyNumDEFailedConnectionEvtData(SINT32 delta)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DataElmFailedconnEvtObject *failedConnEvtObj = NULL;
      
   if ((cmsObj_get(MDMOID_DEV2_DATA_ELM_FAILEDCONN_EVT, &iidStack, 0, (void *) &failedConnEvtObj)) == CMSRET_SUCCESS)
   {
      failedConnEvtObj->failedConnectionEventDataNumberOfEntries += delta;
      cmsObj_setFlags(failedConnEvtObj, &iidStack, OSF_NO_RCL_CALLBACK);
      cmsObj_free((void **) &failedConnEvtObj);
   }
}

#ifdef NOT_SUPPORT
void rutUtil_modifyNumDEScanResult(const InstanceIdStack *iidStack, SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2DataElmDeviceObject *radioObj = NULL;
   CmsRet ret;

   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_SCAN_RESULT, 
                                 &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &radioObj); 
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_DATA_ELM_RADIO, MDMOID_DEV2_DATA_ELM_SCAN_RESULT,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   radioObj->scanResultNumberOfEntries += delta;
   cmsObj_set(radioObj, &ancestorIidStack);
   cmsObj_free((void **)&radioObj);
}
#endif
