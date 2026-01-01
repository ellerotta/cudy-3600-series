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
#ifdef DMP_X_BROADCOM_COM_BASD_1


#include <stdio.h>


/* these defines below must match with what are defined in
 * /userspace/private/apps/bas/include/Bas.h */
#define BASD_NAME                   "bas"
#define BASD_LOCATION               "/bin/bas"

#define BASD_VAR_DIR                "/var/bas"
#define BASD_VAR_STATUS_FILENAME    BASD_VAR_DIR"/bas.status"
#define BASD_VAR_UPTIME_FILENAME    BASD_VAR_DIR"/bas.uptime"

#define BASD_DATA_DIR               "/data/bas"
#define BASD_ETC_DIR                "/etc/bas"
#define BASD_DATA_CONFIG_FILENAME    BASD_DATA_DIR"/bas.config"
#define BASD_DATA_LIC_FILENAME       BASD_DATA_DIR"/bas.lic"

#if defined(SUPPORT_BAS2)
/* in basd2 core code, the file is expected to be ca.crt; instead of patching the bas core code (ease of basd update), we just make the change here */
/* in rcl_basdcfg.c when the bas directory is set up, we setup a softlink for the certicate file */
#define BASD_DATA_CERT_FILENAME      BASD_DATA_DIR"/ca.crt"
#define BASD_CACHE_FILE_FULLPATH     BASD_VAR_DIR"/bas.cache"
#define BASD_CERT_FILE_FULLPATH      BASD_ETC_DIR"/ca.crt"
#else
/* in basd 1.4, we patched the core code */
#define BASD_DATA_CERT_FILENAME      BASD_DATA_DIR"/bas.crt"
#endif

#define BASD_REAL_CERTIFICATE       "/var/cert/bas.crt.cacert"
#define BASD_ARGUMENT_INFO_FILENAME "/var/bas_arguments"


#define BASD_ETC_DEFAULT_DIR        "/etc/bas_default"
#define BASD_DEFAULT_NOTIF_CONFIG   "systemNotification.cfg"
#define BASD_DEFAULT_DATA_CONFIG    "sysDataCollection.cfg"

/* these are the static bas clients, SMD starts them. */
#define BAS_CLIENT_RG               "rgclient"
#define BAS_CLIENT_RG_BDK           "rg"
#define BAS_CLIENT_OPENPLAT         "openplat"
#define BAS_CLIENT_XDSL             "xdsl"
#define BAS_CLIENT_GPON             "gpon"
#define BAS_CLIENT_VOICE            "voice"
#define BAS_CLIENT_SGS              "sgs"
#define BAS_CLIENT_RDS              "rds"
#define BAS_CLIENT_EYE              "eye"
#define BAS_CLIENT_TR143            "tr143"
#define BAS_CLIENT_RDPA             "rdpa"
#define BAS_CLIENT_TR471            "tr471"
#define BAS_CLIENT_WIFI             "wifi"
#define BAS_CLIENT_WLDATAELM        "wldataelm"
#define BAS_CLIENT_INVALID          "invalid"

#define BAS_CLIENT_RG_EXE           "rgclient"
#define BAS_CLIENT_RG_BDK_EXE       "bas_rg"
#define BAS_CLIENT_OPENPLAT_EXE     "bas_openplat"
#define BAS_CLIENT_XDSL_EXE         "bas_xdsl"
#define BAS_CLIENT_GPON_EXE         "bas_gpon"
#define BAS_CLIENT_VOICE_EXE        "bas_client_voice"
#define BAS_CLIENT_SGS_EXE          "sgs"
#define BAS_CLIENT_RDS_EXE          "bas_rds"
#define BAS_CLIENT_EYE_EXE          "bas_eye"
#define BAS_CLIENT_TR143_EXE        "bas_tr143"
#define BAS_CLIENT_RDPA_EXE         "bas_rdpa"
#define BAS_CLIENT_TR471_EXE        "bas_tr471"
#define BAS_CLIENT_WIFI_EXE         "bas_wifi"
#define BAS_CLIENT_WLDATAELM_EXE    "bas_wldataelm"


#define BAS_DEFAULT_PLATFORM_TYPE   "rg-dsl"

UINT32 appNameToEid(char *name);
char *eidToAppName(UINT32 eid);

UBOOL8 basdcfg_isBasExist(void);

void basdcfg_stopBas(BasdCfgObject *obj);
UBOOL8 basdcfg_isApplicationRunning(UINT32 eid);

void basdcfg_startBas(BasdCfgObject *obj);

void basdcfg_startBasEuAtBootup(const char *url, const char *devId,
                                UINT32 debugLevel,
                                const char *username, const char *password,
                                UINT32 keepAliveInterval);

void basdcfg_restartBas(BasdCfgObject *obj);

CmsRet doClientOpWithEid(BasClientObject *obj, UINT32 eid, char *arg, UBOOL8 start, UBOOL8 restart);
CmsRet doClientOpWithExe(BasClientObject *obj, char *exe, char *arg, UBOOL8 start, UBOOL8 restart);

void rutBas_modifyNumBasClientHistory(SINT32 delta, const InstanceIdStack *iidStack);
void rutBas_modifyNumBasdHistory(SINT32 delta);
void rutBas_checkBasdNumHistory(void);
void rutBas_checkBasClientNumHistory(const InstanceIdStack *iidStack);

#endif /* DMP_X_BROADCOM_COM_BASD_1 */
