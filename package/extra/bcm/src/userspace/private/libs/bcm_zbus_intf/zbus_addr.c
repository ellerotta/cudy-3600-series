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


/*!\file zbus_addr.c
 * \brief Utility functions for ZBus Addresses.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "bdk_dbus.h"
#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "bcm_zbus_intf.h"


ZbusAddr compNameToZbusAddrTable[] =
{
   {BDK_COMP_DEVINFO,
    DEVINFO_MD_BUS_NAME, DEVINFO_MD_INTERFACE_NAME, DEVINFO_MD_OBJECT_PATH},
   {BDK_COMP_DIAG,
    DIAG_MD_BUS_NAME, DIAG_MD_INTERFACE_NAME, DIAG_MD_OBJECT_PATH},
   {BDK_COMP_DSL,
    DSL_MD_BUS_NAME, DSL_MD_INTERFACE_NAME, DSL_MD_OBJECT_PATH},
   {BDK_COMP_GPON,
    GPON_MD_BUS_NAME, GPON_MD_INTERFACE_NAME, GPON_MD_OBJECT_PATH},
   {BDK_COMP_EPON,
    EPON_MD_BUS_NAME, EPON_MD_INTERFACE_NAME, EPON_MD_OBJECT_PATH},
   {BDK_COMP_WIFI,
    WIFI_MD_BUS_NAME, WIFI_MD_INTERFACE_NAME, WIFI_MD_OBJECT_PATH},
   {BDK_COMP_VOICE,
    VOICE_MD_BUS_NAME, VOICE_MD_INTERFACE_NAME, VOICE_MD_OBJECT_PATH},
   {BDK_COMP_STORAGE,
    STORAGE_MD_BUS_NAME, STORAGE_MD_INTERFACE_NAME, STORAGE_MD_OBJECT_PATH},
   {BDK_COMP_TR69,
    TR69_MD_BUS_NAME, TR69_MD_INTERFACE_NAME, TR69_MD_OBJECT_PATH},
   {BDK_COMP_USP,
    USP_MD_BUS_NAME, USP_MD_INTERFACE_NAME, USP_MD_OBJECT_PATH},
   {BDK_COMP_SYSMGMT,
    SYSMGMT_MD_BUS_NAME, SYSMGMT_MD_INTERFACE_NAME, SYSMGMT_MD_OBJECT_PATH},
   {BDK_APP_SYSMGMT_NB,
    SYSMGMT_NB_BUS_SVR_BUS_NAME, SYSMGMT_NB_BUS_SVR_INTERFACE_NAME, SYSMGMT_NB_BUS_SVR_OBJECT_PATH},
   {BDK_COMP_SYS_DIRECTORY,
    SYS_DIRECTORY_BUS_NAME, SYS_DIRECTORY_INTERFACE_NAME, SYS_DIRECTORY_OBJECT_PATH},
   {BDK_COMP_OPENPLAT,
    OPENPLAT_MD_BUS_NAME, OPENPLAT_MD_STRPROTO_INTERFACE, OPENPLAT_MD_OBJECT_PATH},
   {BDK_APP_FIREWALLD,
    FIREWALLD_BUS_NAME, FIREWALLD_INTERFACE, FIREWALLD_OBJECT_PATH},
   {BDK_COMP_SYSDIRCTL,
    SYSDIRCTL_BUS_NAME, SYSDIRCTL_INTERFACE_NAME, SYSDIRCTL_OBJECT_PATH},
   {BDK_COMP_WRTSYSMGMT,
    WRTSYSMGMT_BUS_NAME, WRTSYSMGMT_INTERFACE_NAME, WRTSYSMGMT_OBJECT_PATH},
   {BDK_COMP_WRTGPONAGT,
    WRT_GPONAGT_BUS_NAME, WRT_GPONAGT_INTERFACE_NAME, WRT_GPONAGT_OBJECT_PATH},
   {BDK_APP_APIBDK_SD,
    APIBDK_SD_BUS_NAME, APIBDK_SD_INTERFACE, APIBDK_SD_OBJECT_PATH},
   {BDK_APP_WRTVOIPAGT,
    WRT_VOIPAGT_BUS_NAME, WRT_VOIPAGT_INTERFACE_NAME, WRT_VOIPAGT_OBJECT_PATH},
   {BDK_COMP_AUDIO,
    AUDIO_MD_BUS_NAME, AUDIO_MD_INTERFACE_NAME, AUDIO_MD_OBJECT_PATH},
};

const ZbusAddr *zbusIntf_componentNameToZbusAddr(const char *compName)
{
   UINT32 i;

   if (compName == NULL || compName[0] == '\0')
      return NULL;

   for (i=0; i < sizeof(compNameToZbusAddrTable)/sizeof(ZbusAddr); i++)
   {
      if (!strcasecmp(compNameToZbusAddrTable[i].compName, compName))
      {
         return &(compNameToZbusAddrTable[i]);
      }
   }

   return NULL;
}

