/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#ifndef __RUT2_WIFI_H__
#define __RUT2_WIFI_H__


/*!\file rut2_wifi.h
 * \brief Header file for common TR181 Wifi helper functions.
 *
 * The functions in this file should only be called by
 * RCL2, STL2, and other RUT2 functions.
 */


#include "cms.h"

/* One channel is 5MHz wide. These defines return the number of channels in a given bandwidth: */
#define CH_320MHZ_APART      64u
#define CH_160MHZ_APART      32u
#define CH_80MHZ_APART       16u
#define CH_40MHZ_APART        8u
#define CH_20MHZ_APART        4u
#define CH_10MHZ_APART        2u

#define CH_MIN_2G_CHANNEL     1u  /* Min channel in 2G band */
#define CH_MAX_2G_CHANNEL    14u  /* Max channel in 2G band */

#define CH_MIN_5G_CHANNEL    36u	 /* Min channel in 5G band */
#define CH_MAX_5G_CHANNEL   181u	 /* Max channel in 5G band */

#define CH_MIN_6G_CHANNEL     1u  /* Min channel in 6G band */
#define CH_MAX_6G_CHANNEL   233u  /* Max channel in 6G band */


CmsRet rutWifi_channelToSideband(unsigned int channel, unsigned int bw, unsigned int band, char* sb_buf);


void rutUtil_modifyNumWifiSsid(SINT32 delta);
void rutUtil_modifyNumWifiAccessPoint(SINT32 delta);
void rutUtil_modifyNumWifiAssociatedDevice(const InstanceIdStack *iidStack, SINT32 delta);

/** tmp buf used to form a config line, delcared in rut2_wifi.h */
extern char wifi_configBuf[BUFLEN_128];


/** Write a config string to "nvram"
 *
 * @param configStr  (IN) config string
 *
 */
void rutWifi_writeNvram(const char *configStr);


struct RadioCounters {
    UINT32 PLCPErrorCount;
    UINT32 FCSErrorCount;
    UINT32 invalidMACCount;
    UINT32 packetsOtherReceived;
    SINT32 noise;
};

struct SSIDCounters {
    UINT32 retransCount;
    UINT32 failedRetransCount;
    UINT32 retryCount;
    UINT32 multipleRetryCount;
    UINT32 ACKFailureCount;
    UINT32 aggregatedPacketCount;
};

struct AssocDevCounters {
    UINT32 lastDataDownlinkRate;
    UINT32 lastDataUplinkRate;
    SINT32 signalStrength;
    SINT32 noise;
};


CmsRet rutWifi_getRadioCounters(const char *devName, struct RadioCounters *rCounters);
CmsRet rutWifi_getSSIDCounters(const char *devName, struct SSIDCounters *sCounters);
CmsRet rutWifi_getAssocDevCounters(const char *devName, const char *staMACAddr, struct AssocDevCounters *aCounters);

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
void rutWifi_find_AP_ByIndex_locked(int radioIndex, int ssidIndex, InstanceIdStack *iidStack, void **obj);
Dev2WifiSsidObject*  rutWifi_get_AP_SSID_dev2(_Dev2WifiAccessPointObject  *obj);
CmsRet rutWifi_get_AP_Radio_dev2(const _Dev2WifiAccessPointObject  *apObj,void **radioObj,InstanceIdStack *iidStack);
void rutWifi_Clear_AssocicatedDevices(Dev2WifiAccessPointObject *apObj,const InstanceIdStack *iidStack);

CmsRet rutWifi_sendAPClientAssocChanged_dev2(const char* intfName, const char* MACAddress, unsigned int staInstance, unsigned int op);
CmsRet rutWifi_updateSTAHostEntry(const char* intfName, const char* MACAddress, unsigned int staInstance, unsigned int op);

#define REMOVE_ENTRY   (0)
#define ACTIVE_ENTRY   (1)
#define INACTIVE_ENTRY (2)


#endif
#endif  /* __RUT2_WIFI_H__ */
