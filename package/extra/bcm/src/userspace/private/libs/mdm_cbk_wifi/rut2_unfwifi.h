/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#ifndef __RUT2_UNFWIFI_H__
#define __RUT2_UNFWIFI_H__


void rut2_sendWifiChange(void);

void rutUtil_modifyNumMultiAPAPDevice(SINT32 delta);
void rutUtil_modifyNumMultiAPRadio(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumMultiAPAP(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumMultiAPAssocDev(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumMultiAPSteerHistory(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEDevice(SINT32 delta);
void rutUtil_modifyNumDESsid(SINT32 delta);
void rutUtil_modifyNumDERadio(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEDefault8021Q(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDESSIDtoVIDMapping(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACStatus(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACAvailableChannel(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACNonOccupancyChannel(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACActiveChannel(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEIEEE1905Security(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDESPRule(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAnticipatedChannels(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAnticipatedChannelUsage(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAnticipatedChannelUsageEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEMultiAPDevBackhaulCurrOPClass(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEBSS(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDESta(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEQMDescriptor(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDETIDQueueSizes(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEMultiAPSTASteeringHistory(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEUnassocSTA(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAssocEvtData(SINT32 delta);
void rutUtil_modifyNumDEDisassocEvtData(SINT32 delta);
void rutUtil_modifyNumDEFailedConnectionEvtData(SINT32 delta);
void rutUtil_modifyNumDEScanResult(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEOpClassScan(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEChannelScan(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDENeighborBSS(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECurrOpClassProfile(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEDisAllowedOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEScanCapabilityOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACCapabilityCACMethod(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDECACCapabilityCACMethodOpClassChannels(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEOpClassProfile(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAKMFrontHaul(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDEAKMBackhaul(const InstanceIdStack *iidStack, SINT32 delta);


#ifdef NOT_SUPPORT
void rutUtil_modifyNumDEScanResult(const InstanceIdStack *iidStack, SINT32 delta);
#endif

#endif //__RUT2_UNFWIFI_H__

