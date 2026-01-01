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

#ifndef __SYS_DIRECTORY_H__
#define __SYS_DIRECTORY_H__

/*!\file sys_directory.h
 * \brief Public header file for libsys_directory.so.
 *
 */

#include <unistd.h>
#include "number_defs.h"
#include "os_defs.h"
#include "bcm_retcodes.h"
#include "genutil_rbtree.h"
#include "cms_msg_pubsub.h"
#include "cms_dlist.h"
#include "bdk.h"



// subscribers to namespaces is global, not on an individual namespace
extern DlistNode namespaceSubscribers;


// Subscriber info struct used by all pubsub event types.
typedef struct
{
   DlistNode node;    // This must be the first field in the struct.
   char compName[BDK_COMP_NAME_LEN+1];
   UINT32 eid;
} SysDirSubscriber;


typedef struct
{
   rbnode_t rbnode;  // This must be the first field in the struct.
   char pubCompName[BDK_COMP_NAME_LEN+1];  // Component name of the publisher, there can be only 1 publisher of a key.
   DlistNode subscribers;   // Doubly linked list of subscribers to this key.
                            // value string follows this structure.
   char key[PUBSUB_KEY_MAX_LEN+1];  // real storage location of key
                                    // value string follows this struct.
} SysDirKeyValueNode;


typedef struct
{
   rbnode_t rbnode;  // This must be the first field in the struct.
                     // Note that subscribing to namespace is global, not on individual namespace.
   PubSubNamespaceMsgBody nsData;  // has namespace (key) and owner.
   // The next 3 fields is related to namespaces that has an instance id range
   // specifier, e.g. Device.QoS.Queue.[800000-899999].
   // If namespc does not have an instance id range specifier, these fields
   // will be 0 or empty.
   UINT32 firstInstanceId;
   UINT32 lastInstanceId;
   char parsedNamespc[PUBSUB_NAMESPACE_MAX_LEN+1];
} SysDirNamespaceNode;

// This node is related to the Namespace node, except component/MDM owner is
// the key.  It does not contain any data (yet).
typedef struct
{
   rbnode_t rbnode;  // This must be the first field in the struct.
   char ownerCompName[BDK_COMP_NAME_LEN+1];  /**< component which owns a Distributed MDM (key) */
} SysDirMdmOwnerNode;


typedef struct
{
   rbnode_t rbnode;  // This must be the first field in the struct.
   char pubCompName[BDK_COMP_NAME_LEN+1];  // Component name of the publisher, there can be only 1 publisher of a key.
   DlistNode subscribers;   // Doubly linked list of subscribers to this key.
                            // value string follows this structure.
   PubSubInterfaceMsgBody intfData; // includes the intfName, which is the key
                                    // additional dhcp info may follow this struct.
} SysDirInterfaceNode;


/** Initialize sys_directory lib.  This must be called before any other API's.
 */
BcmRet sysdir_init();

// Key-Value API's
BcmRet sysdir_publishKeyValue(const char *compName, UINT32 eid,
                              const char *key, const char *value,
                              DlistNode **subDlist);
BcmRet sysdir_unpublishKeyValue(const char *compName, const char *key,
                                DlistNode **subDlist);
const char *sysdir_subscribeKeyValue(const char *compName, UINT32 eid,
                                     const char *key);
void sysdir_unsubscribeKeyValue(const char *compName, UINT32 eid,
                                const char *key);
const char *sysdir_getKeyValue(const char *key);
const SysDirKeyValueNode *_sysdir_getKeyValueNode(const char *key);

// Namespace (and MDM owner) API's
BcmRet sysdir_publishNamespace(const char *compName, UINT32 eid,
                               const char *namespc,
                               DlistNode **subDlist);
BcmRet sysdir_unpublishNamespace(const char *compName, UINT32 eid,
                               const char *namespc,
                               DlistNode **subDlist);
char *sysdir_subscribeNamespace(const char *compName, UINT32 eid);
void sysdir_unsubscribeNamespace(const char *compName, UINT32 eid);
UBOOL8 sysdir_doesMdmOwnerExist(const char *compName);
char *sysdir_getNamespace(const char *namespc);

SysDirNamespaceNode *sysdir_newNamespaceNode(const char *namespc,
                                             const char *compName);

SysDirMdmOwnerNode *sysdir_newMdmOwnerNode(const char *compName);

BcmRet sysdir_loadNamespacesFromString(const char *strDB,
                                       rbtree_t *rbtNamespaces,
                                       rbtree_t *rbtMdmOwners);

char *sysdir_stringifyNamespaces(rbtree_t *rbtNamespaces,
                                 rbtree_t *rbtMdmOwners);


// Interface API's
BcmRet sysdir_publishInterface(const char *compName, UINT32 eid,
                               const PubSubInterfaceMsgBody *intfData,
                               DlistNode **subDlist);
BcmRet sysdir_unpublishInterface(const char *compName, UINT32 eid,
                               const PubSubInterfaceMsgBody *intfData,
                               DlistNode **subDlist);
const PubSubInterfaceMsgBody *sysdir_subscribeInterface(const char *compName,
                                                  UINT32 eid, const char *key);
void sysdir_unsubscribeInterface(const char *compName, UINT32 eid,
                                 const char *key);
const PubSubInterfaceMsgBody *sysdir_getInterface(const char *key);

// example queryStr: isLayer2=1
// returns a string buffer which must be freed by caller using cmsMem_free.
// If query does not match any interface, an empty string will be returned,
// which must still be freed.  If multiple interfaces match query, return
// buffer will contain comma separated list of interface names.
// On error, returns NULL.
char *sysdir_queryInterface(const char *queryStr);

// For testing
const SysDirInterfaceNode *_sysdir_getInterfaceNode(const char *key);


// Debug and CLI functions
void sysdir_dumpKeyValues();
void sysdir_dumpNamespaces();
void sysdir_dumpMdmOwners();
void sysdir_dumpInterfaces();

#endif /* __SYS_DIRECTORY_H__ */
