/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2023:proprietary:standard

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

#include "tables.h"

/* After testing, we don't get any improvement even if caching the search result.
 * We still leave the code here, but it's not enabled by default.
 */
#undef ENABLE_CACHE_ENTRY

static Wde_writeback_entry wde_writeback_table[]= 
{
//
     //{ {"Device.WiFi.DataElements.", 0x0, NULL},
     //  {"wfa-dataelements", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} },
     { {"Device.WiFi.DataElements.Network.", 0x0, NULL},
       {"wfa-dataelements:Network", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.ControllerID", 0x0, NULL},
       {"ControllerID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.ID", 0x0, NULL},
       {"ID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.DeviceNumberOfEntries", 0x0, NULL},
       {"NumberOfDevices", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "DeviceList" }} }

    ,{ {"Device.WiFi.DataElements.Network.MSCSDisallowedStaList", 0x0, NULL},
       {"MSCSDisallowedStaList", WDE_TYPE_ARRAY, NULL, { HINT_MAC_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.SCSDisallowedStaList", 0x0, NULL},
       {"SCSDisallowedStaList", WDE_TYPE_ARRAY, NULL, { HINT_MAC_ARRAY }} }

//
    ,{ {"Device.WiFi.DataElements.Network.SSID.{i}.", 0x0, NULL},
       {"NetworkSSIDList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.SSID.{i}.Band", 0x0, NULL},
       {"Band", WDE_TYPE_ARRAY, NULL, { HINT_STR_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.SSID.{i}.SSID", 0x0, NULL},
       {"SSID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.", 0x0, NULL},
       {"DeviceList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.BTMSteeringDisallowedSTAList", 0x0, NULL},
       {"BTMSteeringDisallowedSTAList", WDE_TYPE_ARRAY, NULL, { HINT_MAC_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.APMetricsReportingInterval", 0x0, NULL},
       {"APMetricsReportingInterval", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CollectionInterval", 0x0, NULL},
       {"CollectionInterval", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CoordinatedCACAllowed", 0x0, NULL},
       {"CoordinatedCACAllowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CountryCode", 0x0, NULL},
       {"CountryCode", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.DFSEnable", 0x0, NULL},
       {"DFSEnable", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.DSCPMap", 0x0, NULL},
       {"DSCPMap", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ReportIndependentScans", 0x0, NULL},
       {"ReportIndependentScans", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ExecutionEnv", 0x0, NULL},
       {"ExecutionEnv", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ID", 0x0, NULL},
       {"ID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.LocalSteeringDisallowedSTAList", 0x0, NULL},
       {"LocalSteeringDisallowedSTAList", WDE_TYPE_ARRAY, NULL, { HINT_MAC_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Manufacturer", 0x0, NULL},
       {"Manufacturer", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ManufacturerModel", 0x0, NULL},
       {"ManufacturerModel", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.MaxPrioritizationRules", 0x0, NULL},
       {"MaxPrioritizationRules", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.MaxReportingRate", 0x0, NULL},
       {"MaxReportingRate", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.MaxUnsuccessfulAssociationReportingRate", 0x0, NULL},
       {"MaxUnsuccessfulAssociationReportingRate", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.MaxVIDs", 0x0, NULL},
       {"MaxVIDs", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.MultiAPCapabilities", 0x0, NULL},
       {"MultiAPCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.RadioNumberOfEntries", 0x0, NULL},
       {"NumberOfRadios", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "RadioList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.PrioritizationSupport", 0x0, NULL},
       {"PrioritizationSupport", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ReportUnsuccessfulAssociations", 0x0, NULL},
       {"ReportUnsuccessfulAssociations", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SerialNumber", 0x0, NULL},
       {"SerialNumber", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.ServicePrioritizationAllowed", 0x0, NULL},
       {"ServicePrioritizationAllowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SoftwareVersion", 0x0, NULL},
       {"SoftwareVersion", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.STASteeringState", 0x0, NULL},
       {"STASteeringState", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.TrafficSeparationAllowed", 0x0, NULL},
       {"TrafficSeparationAllowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Default8021Q.{i}.", 0x0, NULL},
       {"Default8021Q", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Default8021Q.{i}.DefaultPCP", 0x0, NULL},
       {"DefaultPCP", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Default8021Q.{i}.PrimaryVID", 0x0, NULL},
       {"PrimaryVID", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SSIDtoVIDMapping.{i}.", 0x0, NULL},
       {"TrafficSeparationPolicy", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SSIDtoVIDMapping.{i}.SSID", 0x0, NULL},
       {"SSID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SSIDtoVIDMapping.{i}.VID", 0x0, NULL},
       {"VID", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.", 0x0, NULL},
       {"CACStatus", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACAvailableChannelNumberOfEntries", 0x0, NULL},
       {NULL, WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "AvailableChannelList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACNonOccupancyChannelNumberOfEntries", 0x0, NULL},
       {NULL, WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "NonOccupancyChannelList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACActiveChannelNumberOfEntries", 0x0, NULL},
       {NULL, WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "ActiveChannelList" }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACAvailableChannel.{i}.", 0x0, NULL},
       {"AvailableChannelList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACAvailableChannel.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACAvailableChannel.{i}.Minutes", 0x0, NULL},
       {"Minutes", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACAvailableChannel.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACNonOccupancyChannel.{i}.", 0x0, NULL},
       {"NonOccupancyChannelList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACNonOccupancyChannel.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACNonOccupancyChannel.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACNonOccupancyChannel.{i}.Seconds", 0x0, NULL},
       {"Seconds", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACActiveChannel.{i}.", 0x0, NULL},
       {"ActiveChannelList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACActiveChannel.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACActiveChannel.{i}.Countdown", 0x0, NULL},
       {"Countdown", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.CACStatus.{i}.CACActiveChannel.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SPRule.{i}.", 0x0, NULL},
       {"Prioritization", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SPRule.{i}.AlwaysMatch", 0x0, NULL},
       {"AlwaysMatch", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SPRule.{i}.ID", 0x0, NULL},
       {"ID", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SPRule.{i}.Output", 0x0, NULL},
       {"Output", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.SPRule.{i}.Precedence", 0x0, NULL},
       {"Precedence", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.IEEE1905Security.{i}.", 0x0, NULL},
       {"IEEE1905Security", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.IEEE1905Security.{i}.EncryptionAlgorithm", 0x0, NULL},
       {"EncryptionAlgorithm", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.IEEE1905Security.{i}.IntegrityAlgorithm", 0x0, NULL},
       {"IntegrityAlgorithm", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.IEEE1905Security.{i}.OnboardingProtocol", 0x0, NULL},
       {"OnboardingProtocol", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannels.{i}.", 0x0, NULL},
       {"AnticipatedChannels", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannels.{i}.ChannelList", 0x0, NULL},
       {"ChannelList", WDE_TYPE_ARRAY, NULL, { HINT_INT_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannels.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.", 0x0, NULL},
       {"AnticipatedChannelUsage", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.ReferenceBSSID", 0x0, NULL},
       {"ReferenceBSSID", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.Entry.{i}.", 0x0, NULL},
       {"Entry", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.BurstInterval", 0x0, NULL},
       {"BurstInterval", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.BurstLength", 0x0, NULL},
       {"BurstLength", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.BurstStartTime", 0x0, NULL},
       {"BurstStartTime", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.ChannelUsageReason", 0x0, NULL},
       {"ChannelUsageReason", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.PowerLevel", 0x0, NULL},
       {"PowerLevel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.Repetitions", 0x0, NULL},
       {"Repetitions", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.RUBitmask", 0x0, NULL},
       {"RUBitmask", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.AnticipatedChannelUsage.{i}.TransmitterIdentifier", 0x0, NULL},
       {"TransmitterIdentifier", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.", 0x0, NULL},
       {"RadioList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Enabled", 0x0, NULL},
       {"Enabled", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ID", 0x0, NULL},
       {"ID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Noise", 0x0, NULL},
       {"Noise", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSSNumberOfEntries", 0x0, NULL},
       {"NumberOfBSS", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "BSSList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfileNumberOfEntries", 0x0, NULL},
       {"NumberOfCurrOpClass", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "CurrentOperatingClasses" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.UnassociatedSTANumberOfEntries", 0x0, NULL},
       {"NumberOfUnassocSta", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.RCPISteeringThreshold", 0x0, NULL},
       {"RCPISteeringThreshold", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.APMetricsWiFi6", 0x0, NULL},
       {"APMetricsWiFi6", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.AssociatedSTALinkMetricsInclusionPolicy", 0x0, NULL},
       {"AssociatedSTALinkMetricsInclusionPolicy", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.AssociatedSTATrafficStatsInclusionPolicy", 0x0, NULL},
       {"AssociatedSTATrafficStatsInclusionPolicy", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.STAReportingRCPIHysteresisMarginOverride", 0x0, NULL},
       {"STAReportingRCPIHysteresisMarginOverride", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.STAReportingRCPIThreshold", 0x0, NULL},
       {"STAReportingRCPIThreshold", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SteeringPolicy", 0x0, NULL},
       {"SteeringPolicy", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Utilization", 0x0, NULL},
       {"Utilization", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ChannelUtilizationReportingThreshold", 0x0, NULL},
       {"ChannelUtilizationReportingThreshold", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ChannelUtilizationThreshold", 0x0, NULL},
       {"ChannelUtilizationThreshold", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ChipsetVendor", 0x0, NULL},
       {"ChipsetVendor", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Transmit", 0x0, NULL},
       {"Transmit", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ReceiveSelf", 0x0, NULL},
       {"ReceiveSelf", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ReceiveOther", 0x0, NULL},
       {"ReceiveOther", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.TrafficSeparationCombinedBackhaul", 0x0, NULL},
       {"TrafficSeparationCombinedBackhaul", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.TrafficSeparationCombinedFronthaul", 0x0, NULL},
       {"TrafficSeparationCombinedFronthaul", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.", 0x0, NULL},
       {"ScanResultList", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScanNumberOfEntries", 0x0, NULL},
       {"NumberOfOpClassScans", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "OpClassScanList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.", 0x0, NULL},
       {"OpClassScanList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScanNumberOfEntries", 0x0, NULL},
       {"NumberOfChannelScans", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "ChannelScanList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.OperatingClass", 0x0, NULL},
       {"OperatingClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.", 0x0, NULL},
       {"ChannelScanList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.Noise", 0x0, NULL},
       {"Noise", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSSNumberOfEntries", 0x0, NULL},
       {"NumberOfNeighbors", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "NeighborList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.Utilization", 0x0, NULL},
       {"Utilization", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.", 0x0, NULL},
       {"NeighborList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.BSSID", 0x0, NULL},
       {"BSSID", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.ChannelBandwidth", 0x0, NULL},
       {"ChannelBandwidth", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.ChannelUtilization", 0x0, NULL},
       {"ChannelUtilization", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.SignalStrength", 0x0, NULL},
       {"SignalStrength", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.SSID", 0x0, NULL},
       {"SSID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanResult.{i}.OpClassScan.{i}.ChannelScan.{i}.NeighborBSS.{i}.StationCount", 0x0, NULL},
       {"StationCount", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BackhaulSta.", 0x0, NULL},
       {"BackhaulSta", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BackhaulSta.MACAddress", 0x0, NULL},
       {"MACAddress", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.", 0x0, NULL},
       {"ScanCapability", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.Impact", 0x0, NULL},
       {"Impact", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.MinimumInterval", 0x0, NULL},
       {"MinimumInterval", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.OnBootOnly", 0x0, NULL},
       {"OnBootOnly", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.OpClassChannelsNumberOfEntries", 0x0, NULL},
       {NULL, WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "OpClassList" }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.OpClassChannels.{i}.", 0x0, NULL},
       {"OpClassList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.OpClassChannels.{i}.ChannelList", 0x0, NULL},
       {"ChannelList", WDE_TYPE_ARRAY, NULL, { HINT_INT_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.ScanCapability.OpClassChannels.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.", 0x0, NULL},
       {"CACCapability", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.Method", 0x0, NULL},
       {"Method", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.NumberOfSeconds", 0x0, NULL},
       {"NumberOfSeconds", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.OpClassChannels.{i}.", 0x0, NULL},
       {"OpClassList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.OpClassChannels.{i}.ChannelList", 0x0, NULL},
       {"ChannelList", WDE_TYPE_ARRAY, NULL, { HINT_INT_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CACCapability.CACMethod.{i}.OpClassChannels.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.", 0x0, NULL},
       {"Capabilities", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.HECapabilities", 0x0, NULL},
       {"HECapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.HTCapabilities", 0x0, NULL},
       {"HTCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfileNumberOfEntries", 0x0, NULL},
       {"NumberOfOpClass", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "OperatingClasses" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.VHTCapabilities", 0x0, NULL},
       {"VHTCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.", 0x0, NULL},
       {"WiFi6APRole", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.AnticipatedChannelUsage", 0x0, NULL},
       {"AnticipatedChannelUsage", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.Beamformee80orLess", 0x0, NULL},
       {"Beamformee80orLess", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.BeamformeeAbove80", 0x0, NULL},
       {"BeamformeeAbove80", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.HE160", 0x0, NULL},
       {"HE160", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.HE8080", 0x0, NULL},
       {"HE8080", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MaxDLMUMIMO", 0x0, NULL},
       {"MaxDLMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MaxDLOFDMA", 0x0, NULL},
       {"MaxDLOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MaxULMUMIMO", 0x0, NULL},
       {"MaxULMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MaxULOFDMA", 0x0, NULL},
       {"MaxULOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MCSNSS", 0x0, NULL},
       {"MCSNSS", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MUBeamformer", 0x0, NULL},
       {"MUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MUEDCA", 0x0, NULL},
       {"MUEDCA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MultiBSSID", 0x0, NULL},
       {"MultiBSSID", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.MURTS", 0x0, NULL},
       {"MURTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.RTS", 0x0, NULL},
       {"RTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.SpatialReuse", 0x0, NULL},
       {"SpatialReuse", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.SUBeamformee", 0x0, NULL},
       {"SUBeamformee", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.SUBeamformer", 0x0, NULL},
       {"SUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.TWTRequestor", 0x0, NULL},
       {"TWTRequestor", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.TWTResponder", 0x0, NULL},
       {"TWTResponder", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.ULMUMIMO", 0x0, NULL},
       {"ULMUMIMO", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6APRole.ULOFDMA", 0x0, NULL},
       {"ULOFDMA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.", 0x0, NULL},
       {"WiFi6bSTARole", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.AnticipatedChannelUsage", 0x0, NULL},
       {"AnticipatedChannelUsage", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.Beamformee80orLess", 0x0, NULL},
       {"Beamformee80orLess", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.BeamformeeAbove80", 0x0, NULL},
       {"BeamformeeAbove80", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.HE160", 0x0, NULL},
       {"HE160", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.HE8080", 0x0, NULL},
       {"HE8080", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MaxDLMUMIMO", 0x0, NULL},
       {"MaxDLMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MaxDLOFDMA", 0x0, NULL},
       {"MaxDLOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MaxULMUMIMO", 0x0, NULL},
       {"MaxULMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MaxULOFDMA", 0x0, NULL},
       {"MaxULOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MCSNSS", 0x0, NULL},
       {"MCSNSS", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MUBeamformer", 0x0, NULL},
       {"MUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MUEDCA", 0x0, NULL},
       {"MUEDCA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MultiBSSID", 0x0, NULL},
       {"MultiBSSID", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.MURTS", 0x0, NULL},
       {"MURTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.RTS", 0x0, NULL},
       {"RTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.SpatialReuse", 0x0, NULL},
       {"SpatialReuse", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.SUBeamformee", 0x0, NULL},
       {"SUBeamformee", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.SUBeamformer", 0x0, NULL},
       {"SUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.TWTRequestor", 0x0, NULL},
       {"TWTRequestor", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.TWTResponder", 0x0, NULL},
       {"TWTResponder", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.ULMUMIMO", 0x0, NULL},
       {"ULMUMIMO", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.WiFi6bSTARole.ULOFDMA", 0x0, NULL},
       {"ULOFDMA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMFrontHaul.{i}.", 0x0, NULL},
       {"AKMFrontHaul", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMFrontHaul.{i}.OUI", 0x0, NULL},
       {"OUI", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMFrontHaul.{i}.Type", 0x0, NULL},
       {"Type", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMBackhaul.{i}.", 0x0, NULL},
       {"AKMBackhaul", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMBackhaul.{i}.OUI", 0x0, NULL},
       {"OUI", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.AKMBackhaul.{i}.Type", 0x0, NULL},
       {"Type", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfile.{i}.", 0x0, NULL},
       {"OperatingClasses", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfile.{i}.Class", 0x0, NULL},
       {"Class", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfile.{i}.MaxTxPower", 0x0, NULL},
       {"MaxTxPower", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfile.{i}.NonOperable", 0x0, NULL},
       {"NonOperable", WDE_TYPE_ARRAY, NULL, { HINT_INT_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.Capabilities.CapableOperatingClassProfile.{i}.NumberOfNonOperChan", 0x0, NULL},
       {"NumberOfNonOperChan", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "NonOperable" }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfile.{i}.", 0x0, NULL},
       {"CurrentOperatingClasses", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfile.{i}.Channel", 0x0, NULL},
       {"Channel", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfile.{i}.Class", 0x0, NULL},
       {"Class", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfile.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.CurrentOperatingClassProfile.{i}.TxPower", 0x0, NULL},
       {"TxPower", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.DisAllowedOpClassChannels.{i}.", 0x0, NULL},
       {"DisAllowedOpClassChannels", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.DisAllowedOpClassChannels.{i}.ChannelList", 0x0, NULL},
       {"ChannelList", WDE_TYPE_ARRAY, NULL, { HINT_INT_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.DisAllowedOpClassChannels.{i}.OpClass", 0x0, NULL},
       {"OpClass", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.DisAllowedOpClassChannels.{i}.Enable", 0x0, NULL},
       {"Enable", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.", 0x0, NULL},
       {"SpatialReuse", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.PartialBSSColor", 0x0, NULL},
       {"PartialBSSColor", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.BSSColor", 0x0, NULL},
       {"BSSColor", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.HESIGASpatialReuseValue15Allowed", 0x0, NULL},
       {"HESIGASpatialReuseValue15Allowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.SRGInformationValid", 0x0, NULL},
       {"SRGInformationValid", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.NonSRGOffsetValid", 0x0, NULL},
       {"NonSRGOffsetValid", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.PSRDisallowed", 0x0, NULL},
       {"PSRDisallowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.NonSRGOBSSPDMaxOffset", 0x0, NULL},
       {"NonSRGOBSSPDMaxOffset", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.SRGOBSSPDMinOffset", 0x0, NULL},
       {"SRGOBSSPDMinOffset", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.SRGOBSSPDMaxOffset", 0x0, NULL},
       {"SRGOBSSPDMaxOffset", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.SRGBSSColorBitmap", 0x0, NULL},
       {"SRGBSSColorBitmap", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.SRGPartialBSSIDBitmap", 0x0, NULL},
       {"SRGPartialBSSIDBitmap", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.SpatialReuse.NeighborBSSColorInUseBitmap", 0x0, NULL},
       {"NeighborBSSColorInUseBitmap", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.", 0x0, NULL},
       {"BSSList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.AssociationAllowanceStatus", 0x0, NULL},
       {"AssociationAllowanceStatus", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.BackhaulAKMsAllowed", 0x0, NULL},
       {"BackhaulAKMsAllowed", WDE_TYPE_ARRAY, NULL, { HINT_STR_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.BackhaulUse", 0x0, NULL},
       {"BackhaulUse", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.BroadcastBytesReceived", 0x0, NULL},
       {"BroadcastBytesReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.BroadcastBytesSent", 0x0, NULL},
       {"BroadcastBytesSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.BSSID", 0x0, NULL},
       {"BSSID", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.ByteCounterUnits", 0x0, NULL},
       {"ByteCounterUnits", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.Enabled", 0x0, NULL},
       {"Enabled", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.EstServiceParametersBE", 0x0, NULL},
       {"EstServiceParametersBE", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.EstServiceParametersBK", 0x0, NULL},
       {"EstServiceParametersBK", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.EstServiceParametersVI", 0x0, NULL},
       {"EstServiceParametersVI", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.EstServiceParametersVO", 0x0, NULL},
       {"EstServiceParametersVO", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.FronthaulAKMsAllowed", 0x0, NULL},
       {"FronthaulAKMsAllowed", WDE_TYPE_ARRAY, NULL, { HINT_STR_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.FronthaulUse", 0x0, NULL},
       {"FronthaulUse", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.LastChange", 0x0, NULL},
       {"LastChange", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.MultiBSSID", 0x0, NULL},
       {"MultiBSSID", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.MulticastBytesReceived", 0x0, NULL},
       {"MulticastBytesReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.MulticastBytesSent", 0x0, NULL},
       {"MulticastBytesSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STANumberOfEntries", 0x0, NULL},
       {"NumberOfSTA", WDE_TYPE_INTEGER, NULL, { .type = HINT_NUM_OF, .element = "STAList" }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.Profile1bSTAsDisallowed", 0x0, NULL},
       {"Profile1bSTAsDisallowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.Profile2bSTAsDisallowed", 0x0, NULL},
       {"Profile2bSTAsDisallowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.R1disallowed", 0x0, NULL},
       {"R1disallowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.R2disallowed", 0x0, NULL},
       {"R2disallowed", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.SSID", 0x0, NULL},
       {"SSID", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.QMDescriptor.{i}.", 0x0, NULL},
       {"QMDescriptor", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.QMDescriptor.{i}.ClientMAC", 0x0, NULL},
       {"ClientMac", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.QMDescriptor.{i}.DescriptorElement", 0x0, NULL},
       {"DescriptorElement", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.", 0x0, NULL},
       {"STAList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.BytesReceived", 0x0, NULL},
       {"BytesReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.BytesSent", 0x0, NULL},
       {"BytesSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.CellularDataPreference", 0x0, NULL},
       {"CellularDataPreference", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.ClientCapabilities", 0x0, NULL},
       {"ClientCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.ErrorsReceived", 0x0, NULL},
       {"ErrorsReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.ErrorsSent", 0x0, NULL},
       {"ErrorsSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.EstMACDataRateDownlink", 0x0, NULL},
       {"EstMACDataRateDownlink", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.EstMACDataRateUplink", 0x0, NULL},
       {"EstMACDataRateUplink", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.HECapabilities", 0x0, NULL},
       {"HECapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.Hostname", 0x0, NULL},
       {"Hostname", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.HTCapabilities", 0x0, NULL},
       {"HTCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.IPV4Address", 0x0, NULL},
       {"IPV4Address", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.IPV6Address", 0x0, NULL},
       {"IPV6Address", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.LastConnectTime", 0x0, NULL},
       {"LastConnectTime", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.LastDataDownlinkRate", 0x0, NULL},
       {"LastDataDownlinkRate", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.LastDataUplinkRate", 0x0, NULL},
       {"LastDataUplinkRate", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.MACAddress", 0x0, NULL},
       {"MACAddress", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.MeasurementReport", 0x0, NULL},
       {"MeasurementReport", WDE_TYPE_ARRAY, NULL, { HINT_STR_ARRAY }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.NumberOfMeasureReports", 0x0, NULL},
       {"NumberOfMeasureReports", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.PacketsReceived", 0x0, NULL},
       {"PacketsReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.PacketsSent", 0x0, NULL},
       {"PacketsSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.ReAssociationDelay", 0x0, NULL},
       {"ReAssociationDelay", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.RetransCount", 0x0, NULL},
       {"RetransCount", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.SignalStrength", 0x0, NULL},
       {"SignalStrength", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.UtilizationReceive", 0x0, NULL},
       {"UtilizationReceive", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.UtilizationTransmit", 0x0, NULL},
       {"UtilizationTransmit", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.VHTCapabilities", 0x0, NULL},
       {"VHTCapabilities", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.TimeStamp", 0x0, NULL},
       {"TimeStamp", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.TransmittedBSSID", 0x0, NULL},
       {"TransmittedBSSID", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.UnicastBytesReceived", 0x0, NULL},
       {"UnicastBytesReceived", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.UnicastBytesSent", 0x0, NULL},
       {"UnicastBytesSent", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.", 0x0, NULL},
       {"WiFi6Capabilities", WDE_TYPE_OBJECT, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.AnticipatedChannelUsage", 0x0, NULL},
       {"AnticipatedChannelUsage", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.Beamformee80orLess", 0x0, NULL},
       {"Beamformee80orLess", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.BeamformeeAbove80", 0x0, NULL},
       {"BeamformeeAbove80", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.HE160", 0x0, NULL},
       {"HE160", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.HE8080", 0x0, NULL},
       {"HE8080", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MaxDLMUMIMO", 0x0, NULL},
       {"MaxDLMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MaxDLOFDMA", 0x0, NULL},
       {"MaxDLOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MaxULMUMIMO", 0x0, NULL},
       {"MaxULMUMIMO", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MaxULOFDMA", 0x0, NULL},
       {"MaxULOFDMA", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MCSNSS", 0x0, NULL},
       {"MCSNSS", WDE_TYPE_STRING, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MUBeamformer", 0x0, NULL},
       {"MUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MUEDCA", 0x0, NULL},
       {"MUEDCA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MultiBSSID", 0x0, NULL},
       {"MultiBSSID", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.MURTS", 0x0, NULL},
       {"MURTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.RTS", 0x0, NULL},
       {"RTS", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.SpatialReuse", 0x0, NULL},
       {"SpatialReuse", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.SUBeamformee", 0x0, NULL},
       {"SUBeamformee", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.SUBeamformer", 0x0, NULL},
       {"SUBeamformer", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.TWTRequestor", 0x0, NULL},
       {"TWTRequestor", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.TWTResponder", 0x0, NULL},
       {"TWTResponder", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.ULMUMIMO", 0x0, NULL},
       {"ULMUMIMO", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.WiFi6Capabilities.ULOFDMA", 0x0, NULL},
       {"ULOFDMA", WDE_TYPE_BOOLEAN, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.TIDQueueSizes.{i}.", 0x0, NULL},
       {"TIDQueueSizes", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.TIDQueueSizes.{i}.Size", 0x0, NULL},
       {"Size", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.BSS.{i}.STA.{i}.TIDQueueSizes.{i}.TID", 0x0, NULL},
       {"TID", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

//
    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.UnassociatedSTA.{i}.", 0x0, NULL},
       {"UnassociatedStaList", WDE_TYPE_ARRAY, NULL, { HINT_NONE }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.UnassociatedSTA.{i}.MACAddress", 0x0, NULL},
       {"MACAddress", WDE_TYPE_STRING, NULL, { HINT_MAC_STR }} }

    ,{ {"Device.WiFi.DataElements.Network.Device.{i}.Radio.{i}.UnassociatedSTA.{i}.SignalStrength", 0x0, NULL},
       {"SignalStrength", WDE_TYPE_INTEGER, NULL, { HINT_NONE }} }

};
static const unsigned int wde_writeback_table_size = sizeof(wde_writeback_table) / sizeof(Wde_writeback_entry);

static int comparator(const void *a1, const void *a2)
{
    const Wde_writeback_entry *b1, *b2;
    char *s1, *s2;

    b1 = (Wde_writeback_entry *)a1;
    b2 = (Wde_writeback_entry *)a2;
    s1 = b1->mdm_node.fullpath;
    s2 = b2->mdm_node.fullpath;

    assert(s1);
    assert(s2);
    return strcmp(s1, s2);
}

int sort_table(void)
{
    //int i;
    qsort(wde_writeback_table, wde_writeback_table_size, sizeof(struct wde_writeback_entry), &comparator);
//    for (i = 0; i < writeback_table_size; i++)
//    {
//        printf("#### [%s] ####\n", writeback_table[i].mdm_node.fullpath);
//    }
    return 0;
}

Wde_writeback_entry* get_mapping_entry(char *fullpath)
{
    
    Wde_writeback_entry key;
    Wde_writeback_entry *result = NULL;
#ifdef ENABLE_CACHE_ENTRY
    static wde_wb_entry_cache cache_entry = {0};
#endif

    if (!fullpath)
    {
        printf("#### invalid input parameters! ####\n");
        return NULL;
    }

#ifdef ENABLE_CACHE_ENTRY
    if (0 == strcmp(fullpath, cache_entry.last_fullpath))
    {
        //printf("#### Use mapping cache: [%s]  ####\n", (cache_entry.entry)?"MAPPED":"UNMAPPED");
        return cache_entry.entry;
    }
#endif

    key.mdm_node.fullpath = strdup(fullpath);
    
    result = bsearch(&key, wde_writeback_table, wde_writeback_table_size,
                     sizeof(struct wde_writeback_entry), &comparator);
    free(key.mdm_node.fullpath);

#ifdef ENABLE_CACHE_ENTRY
    snprintf(cache_entry.last_fullpath, FULLPATH_LEN, "%s", fullpath);
    cache_entry.entry = result;
#endif

    return result;
}
