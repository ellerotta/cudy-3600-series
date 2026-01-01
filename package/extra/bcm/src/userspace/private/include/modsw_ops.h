/***********************************************************************
 *
 *  Copyright (c) 2012  Broadcom Corporation
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

#ifndef _MODSW_OPS_H_
#define _MODSW_OPS_H_


/*!\file modsw_ops.h
 * \brief This file contains declarations and definitions associated with
 * libmodsw.so in userspace/private/lib/modsw
 *
 */

#include "modsw.h"
#include "cms_params_modsw.h"

typedef struct
{
    UBOOL8 enable;
    char status[BUFLEN_32];
    UBOOL8 reset;
    char alias[BUFLEN_64];
    char name[BUFLEN_32];
    char type[BUFLEN_256]; //description
    UINT32 initialRunLevel;
    SINT32 requestedRunLevel;
    SINT32 currentRunLevel;
    SINT32 initialExecutionUnitRunLevel;
    char vendor[BUFLEN_128];
    char version[BUFLEN_32];
    char parentExecEnv[BUFLEN_1024];
    SINT32 allocatedDiskSpace;
    SINT32 availableDiskSpace;
    SINT32 allocatedMemory;
    SINT32 availableMemory;
    UINT32 mngrEid;
    char containerName[BEEP_CONTNAME_LEN_MAX];
    char mngrAppName[BUFLEN_32];
    UBOOL8 isPreinstall;
    char username[BUFLEN_64];
    SINT32 uidBase;
    char ipv4Address[BEEP_ADDRESS_LEN_MAX];
    char ipv6Address[BEEP_IPV6_ADDRESS_LEN_MAX];
} OpsEeInfo_t;

typedef struct
{
    char uuid[BUFLEN_40];
    char duid[BUFLEN_64];
    char alias[BUFLEN_64];
    char name[BUFLEN_64];
    char status[BUFLEN_32];
    UBOOL8 resolved;
    char url[BUFLEN_1024];
    char username[BUFLEN_256];
    char password[BUFLEN_256];
    char description[BUFLEN_256];
    char vendor[BUFLEN_128];
    char version[BUFLEN_32];
    char executionUnitList[BUFLEN_1024];
    char executionEnvRef[BUFLEN_1024];
    char ExecutionEnvList[BUFLEN_1024];
    char startTime[BUFLEN_64];
    char completeTime[BUFLEN_64];
    char packageId[BUFLEN_48];
    SINT32 bundleId;
    UBOOL8 isPreinstall;
} OpsDuInfo_t;

typedef struct
{
    char euid[BUFLEN_64];
    char alias[BUFLEN_64];
    char name[BUFLEN_64];
    char execEnvLabel[BUFLEN_64]; //associated EE name
    char status[BUFLEN_32];
    char requestedState[BUFLEN_32];
    char executionFaultCode[BUFLEN_32];
    char executionFaultMessage[BUFLEN_128];
    UBOOL8 autoStart;
    UINT32 runLevel;
    char vendor[BUFLEN_128];
    char version[BUFLEN_32];
    char description[BUFLEN_256];
    char associatedProcessList[BUFLEN_1024];
    char executionEnvRef[BUFLEN_1024];
    char appDigest[BEEP_KEY_LEN_MAX];
    char manifestDigest[BEEP_KEY_LEN_MAX];
    char mediaType[BUFLEN_16];
    char username[BUFLEN_64];
    char mngrAppName[BUFLEN_64];
    char iPv4Address[BEEP_ADDRESS_LEN_MAX];
    char iPv6Address[BEEP_IPV6_ADDRESS_LEN_MAX];
    char containerName[BEEP_CONTNAME_LEN_MAX];
    UINT32 autoStartOrder;
    UBOOL8 autoRelaunch;
    UINT32 maxRestarts;
    UINT32 restartInterval;
    UINT32 successfulStartPeriod;
    UINT32 restartCount;
} OpsEuInfo_t;


#endif /* _MODSW_OPS_H_ */
