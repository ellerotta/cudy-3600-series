/***********************************************************************
 *
 * Copyright (c) 2021  Broadcom Corporation
 * All Rights Reserved
 *
 * <:label-BRCM:2021:DUAL/GPL:standard
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 *
 ************************************************************************/

#ifndef __CMS_MSG_USP_H__
#define __CMS_MSG_USP_H__

#include "cms.h"
#include "cms_msg.h"
#include "mdm_params.h"


/*!\file cms_msg_usp.h
 * \brief defines for USP message Functionality.
 *
 * USP messages reserved range is   0x10003200-0x100034FF (see cms_msg.h)
 *
 */


/*!\enum CmsUspMsgType
 * \brief  Enumeration of possible message types
 *
 */
typedef enum 
{
   CMS_MSG_AGENT_MTP_CHANGE                   = 0x10003200,
   CMS_MSG_AGENT_STOMP_CHANGE                 = 0x10003201,
   CMS_MSG_CONTROLLER_CHANGE                  = 0x10003202,
   CMS_MSG_SUBSCRIPTION_CHANGE                = 0x10003203,
   CMS_MSG_STOMP_CONN_CONFIG_CHANGE           = 0x10003204,
   CMS_MSG_ROLE_PERM_CONFIG_CHANGE            = 0x10003205,
   CMS_MSG_CERTIFICATE_CHANGE                 = 0x10003206,
   CMS_MSG_AGENT_COAP_CHANGE                  = 0x10003207,
   CMS_MSG_CONTROLLER_MTP_CHANGE              = 0x10003208,
   CMS_MSG_AGENT_MQTT_CHANGE                  = 0x10003209,
   CMS_MSG_CNTRL_MQTT_CHANGE                  = 0x1000320A,
   CMS_MSG_MQTT_CLIENT_CONFIG_CHANGE          = 0x1000320B,
   CMS_MSG_MQTT_SUBS_CONFIG_CHANGE            = 0x1000320C,
   CMS_MSG_USP_CANCEL_COMMAND                 = 0x1000320D,
   CMS_MSG_CHALLENGE_CONFIG_CHANGE            = 0x1000320E,
   CMS_MSG_AGENT_WEBSOCKET_CHANGE             = 0x1000320F,
   CMS_MSG_CNTRL_WEBSOCKET_CHANGE             = 0x10003210,
   CMS_MSG_E2E_SESSION_CHANGE                 = 0x10003211,
   CMS_MSG_DUMP_DM_SCHEMA                     = 0x10003220,
   CMS_MSG_DUMP_DM_INSTANCES                  = 0x10003221,
   CMS_MSG_DUMP_DM_NODE_MAP                   = 0x10003222,
   CMS_MSG_SET_PROTOCOL_TRACE                 = 0x10003295,
   CMS_MSG_USP_TEST_DOWNLOAD                  = 0x10003300,
   CMS_MSG_USP_TEST_ACTIVATE                  = 0x10003301,
   CMS_MSG_USP_TEST_RESTORE                   = 0x10003302,
   CMS_MSG_USP_TEST_BACKUP                    = 0x10003303,
   CMS_MSG_BULKDATA_CHANGE                    = 0x10003350,
   CMS_MSG_BULKDATA_PROFILE_CHANGE            = 0x10003351,
   CMS_MSG_BULKDATA_PROFILE_HTTP_CHANGE       = 0x10003352,
} CmsUspMsgType;



/** Data body for CMS_MSG_AGENT_MTP_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   UBOOL8 enable;
   UBOOL8 enableMDNS;
   char protocol[BUFLEN_64];
} UspAgentMtpChangedMsgBody;



/** Data body for CMS_MSG_AGENT_STOMP_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   char fullPath[BUFLEN_256];
   char reference[BUFLEN_64];
   char destination[BUFLEN_256];
   char destinationFromServer[BUFLEN_256];
} UspAgentStompChangedMsgBody;


/** Data body for CMS_MSG_AGENT_COAP_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   char fullPath[BUFLEN_256];
   char interfaces[BUFLEN_256];
} UspAgentCoapChangedMsgBody;


/** Data body for CMS_MSG_AGENT_WEBSOCKET_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   char fullPath[BUFLEN_256];
} UspAgentWebSocketChangedMsgBody;


/** Data body for CMS_MSG_CNTRL_WEBSOCKET_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   UINT32 port;
   UINT32 encrypt;
   UINT32 interval;
   UINT32 retryInterval;
   UINT32 retryMulti;
   char fullPath[BUFLEN_256];
   char host[BUFLEN_256];
   char path[BUFLEN_256];
} UspCntrlWebsocketChangedMsgBody;


/** Data body for CMS_MSG_CONTROLLER_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   UBOOL8 enable;
   UBOOL8 isDelete;
   UINT32 interval;
   char notifTime[BUFLEN_64];
} UspControllerChangedMsgBody;


/** Data body for CMS_MSG_CONTROLLER_MTP_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   UBOOL8 isDelete;
   UBOOL8 enable;
   char protocol[BUFLEN_64];
} UspControllerMtpChangedMsgBody;


/** Data body for CMS_MSG_SUBSCRIPTION_CHANGE message type.
 *
 */
typedef struct
{
   int instance;     // Subscription instance number
   UBOOL8 isDelete;  // True if instance was deleted
} UspSubscriptionChangedMsgBody;


/** Data body for CMS_MSG_STOMP_CONN_CONFIG_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   int enable;
   int isDelete;
} StompConnectionChangedMsgBody;


/** Data body for CMS_MSG_ROLE_PERM_CONFIG_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   UBOOL8 enable;
   UINT32 order;
   char targets[BUFLEN_1024];
   char param[BUFLEN_8];
   char obj[BUFLEN_8];
   char instObj[BUFLEN_8];
   char event[BUFLEN_8];
   UBOOL8 isDelete;
} UspRolePermChangedMsgBody;


/** Data body for CMS_MSG_BULKDATA_XXX_CHANGE message type.
 *
 */
typedef struct
{
   int instanceId;   // challenge instance number
   UBOOL8 isDelete;  // True if instance was deleted
} UspChallengeChangedMsgBody;


/** Data body for CMS_MSG_BULKDATA_XXX_CHANGE message type.
 *
 */
typedef struct
{
   int instance;     // profile instance number
   UBOOL8 isDelete;  // True if instance was deleted
} BulkdataChangedMsgBody;



/** Data body for CMS_MSG_CERTIFICATE_CHANGE message type.
 *
 */
typedef struct
{
   char fileName[BUFLEN_256];
   UBOOL8 isDelete;
} UspCertificateChangedMsgBody;


/** Data body for CMS_MSG_USP_TEST_DOWNLOAD (for FirmwareImage) and
 *  CMS_MSG_USP_TEST_RESTORE (for VendorConfigFile).  Both of these commands
 *  use a similar set of arguments.  The args here also match the TR181 spec.
 */
typedef struct
{
   char url[257];
   UBOOL8 autoActivate;  // used only by FirmwareImage.Download()
   UINT32 fileSize;
   char username[257];
   char password[257];
   char checkSumAlgorithm[33];  // "SHA-1", "SHA-256", "SHA-512", see checkSumAlgorithmValues in data model
   char expectedCheckSum[257];  // checksum as reported by controller, in hexbinary
   char actualCheckSum[257];    // checksum calculated by gateway after download, in hexbinary
   char targetFileName[257];    // used only by VendorConfigFile.Restore()
} UspTestDownloadMsgBody;


/** Data body for CMS_MSG_USP_TEST_ACTIVATE message type.
 *  Note these are the args of the USP TR181 Activate command.
 */
typedef struct
{
   UINT32 start;  // offset in seconds after receiving command
   UINT32 end;    // offset in seconds after receiving command
   SINT32 maxRetries;  // [-1:10]
   char   mode[32];  // AnyTime, Immediately, WhenIdle, ConfirmationNeeded, see ActivateModeValues in data model
   char   userMessage[1024];  // Not implemented, but put here as placeholder
} UspTestActivateMsgBody;


/** Data body for CMS_MSG_USP_TEST_BACKUP for VendorConfigFile.
 *  The args here also match the TR181 spec.
 */
typedef struct
{
   char url[257];
   char username[257];
   char password[257];
} UspTestUploadMsgBody;


/** Data body for CMS_MSG_AGENT_MQTT_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   UINT32 publishQoS;
   char fullPath[BUFLEN_256];
   char reference[BUFLEN_256];
   char responseTopicConfigured[BUFLEN_256];
   char responseTopicDiscovered[BUFLEN_256];
} UspAgentMqttChangedMsgBody;


/** Data body for CMS_MSG_CNTRL_MQTT_CHANGE message type.
 *
 */
typedef struct
{
   UBOOL8 isDelete;
   UBOOL8 activeMtp;
   UBOOL8 publishRetainResponse;
   UBOOL8 publishRetainNotify;
   char fullPath[BUFLEN_256];
   char reference[BUFLEN_256];
   char topic[BUFLEN_256];
} UspCntrlMqttChangedMsgBody;


/** Data body for CMS_MSG_MQTT_CLIENT_CONFIG_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   int enable;
   int isDelete;
} MqttClientChangedMsgBody;


/** Data body for CMS_MSG_MQTT_SUBS_CONFIG_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
   int enable;
   int isDelete;
} MqttSubsChangedMsgBody;


/** Data body for CMS_MSG_E2E_SESSION_CHANGE message type.
 *
 */
typedef struct
{
   char fullPath[BUFLEN_256];
} E2eSessionChangedMsgBody;

#endif /* __CMS_MSG_USP_H__ */
