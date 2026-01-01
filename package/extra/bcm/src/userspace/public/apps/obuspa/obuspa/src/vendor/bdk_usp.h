/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom
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
/**
 * \file bdk_usp.h
 *
 */
#ifndef BDK_USP_H
#define BDK_USP_H


#include "stomp.h"
#include "usp_api.h"

//------------------------------------------------------------------------------
// API functions
void *BDK_EXEC_Main(void *args);

int vendorGetGroupCb(int group_id, kv_vector_t *params);
int vendorSetGroupCb(int group_id, kv_vector_t *params, unsigned *types, int *failure_index); 
int vendorAddGroupCb(int group_id, char *path, int *instance);
int vendorDelGroupCb(int group_id, char *path);
int vendorRefreshInstancesCb(int group_id, char *path, int *expiry_period);
int vendorStartTrans(void);
int vendorCommitTrans(void);
int vendorAbortTrans(void);

/* -1 is ungrouped */
/* We can treat all the BDK MDM as 1 group.  Or each BDK component is 1 group.
 * obuspa code sort all the parameters by groupId.  But BDK zbus will actually sort them all out
 * by component anyway.
 */
#define GROUP_ID_USP     0
#define GROUP_ID_MAX     1   /* must be less than MAX_VENDOR_PARAM_GROUPS defined in vendor_defs.h */

/* this will be replaced when all the USP data model is implemented; the script will generate more than STOMP connection for USP */
#define DEVICE_STOMP_CONN_ROOT "Device.STOMP.Connection"

int DEVICE_MTP_ConfigChange(const char *fullpath, int enable, int enableMDNS,
                            const char *protocol);
int DEVICE_MTP_Stomp_ConfigChange(const char *fullpath, const char *ref,
                                  const char *dest, const char *serverDest,
                                  int isDelete, int activeStompMtp);
int DEVICE_MTP_Websocket_ConfigChange(const char *fullpath,
                                      int isDelete, int activeMtp);
int DEVICE_Controller_ConfigChange(const char *fullpath, int enable,
                                   int interval, const char *notifTime,
                                   int isDelete);
int DEVICE_Controller_MTP_ConfigChange(const char *fullpath, int enable,
                                       const char *protocol, int isDelete);
int DEVICE_CTRUST_Perm_ConfigChange(const char *fullpath, int enable,
                                    unsigned int order, const char *targets,
                                    const char *param, const char *obj,
                                    const char *instObj, const char *event,
                                    int isDelete);
int DEVICE_CTRUST_Challenge_ConfigChange(int instanceId, int isDelete);
int DEVICE_STOMP_ConfigChange(const char *fullpath, int enable, int isDelete);
int DEVICE_SUBSCRIPTION_ConfigChange(int instance, int isDelete);
int DEVICE_SECURITY_AddCertFromFile(const char *fileName,
                                    const char *alias,
                                    int usage, int role);
int DEVICE_SECURITY_AddCertFromCommand(const char *alias, const char *certificate);
int DEVICE_SECURITY_DeleteCertFromFile(const char *fileName);
int DEVICE_SECURITY_DeleteCertFromCommand(int instance,
                                          const char *issuer,
                                          const char *serialNumber);
int DEVICE_SECURITY_GetCertRoleFromEndpointID(const char *endpointID,
                                              char *rolePath,
                                              int rolePathLen);
int DEVICE_LOCAL_AGENT_SetRebootCause(const char *cause);
int DEVICE_AGENT_MTP_MQTT_ConfigChange
   (const char *fullpath, const char *reference,
    const char *responseTopicConfigured, const char *responseTopicDiscovered,
    unsigned int publishQoS, int isDelete, int activeMtp);
int DEVICE_CNTRL_MTP_MQTT_ConfigChange
   (const char *fullpath, const char *reference, const char *topic,
    int publishRetainResponse, int publishRetainNotify,
    int isDelete, int activeMtp);
int DEVICE_MQTT_CLIENT_ConfigChange(const char *fullpath, int enable, int isDelete);
int DEVICE_MQTT_SUBSCRIPTION_ConfigChange(const char *fullpath, int enable, int isDelete);

int DEVICE_BULKDATA_ConfigChange(unsigned int type, int instance, int isDelete);

int DEVICE_STOMP_SetDestinationFromServer(int instance, const char *subscribe_dest);

int DEVICE_CNTRL_MTP_Websocket_ConfigChange(const char *fullpath,
                          const char *host, const char *path, unsigned int port,
                          unsigned int encrypt, unsigned int interval,
                          unsigned int retryInterval, unsigned int retryMulti,
                          int isDelete, int activeMtp);
#endif   /* BDK_USP.H */
