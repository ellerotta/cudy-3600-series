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

#ifndef __SYSDIR_NS_H__
#define __SYSDIR_NS_H__

/*!\file sysdir_ns.h
 * \brief Public header file for sysdir namespace resolve functions.
 *
 */
#include "bcm_retcodes.h"
#include "bdk.h"
#include "cms_msg_pubsub.h"

// This structure is returned by sysdir_ns_getAllRemoteComponents().
// Free using sysdir_ns_freeCompNodes().
typedef struct sysdir_comp_node {
    char compName[BDK_COMP_NAME_LEN + 1];
    struct sysdir_comp_node *next;
} SysdirCompNode;


#define NS_MATCH_PREFIX           0x0001
#define NS_MATCH_LONGEST_PREFIX   0x0002
#define NS_MATCH_INSTANCE_ID      0x0004
#define NS_MATCH_ALIAS            0x0008

// This structure is returned by sysdir_ns_lookup().
// Free using sysdir_ns_freeNsNodes().
typedef struct sysdir_ns_node {
    PubSubNamespaceMsgBody info;  // a copy of the matching namespace entry,
                                  // note info.namespc is the namespace claimed by info.ownerCompName.
    char reqFullpath[PUBSUB_NAMESPACE_MAX_LEN*2];  // the fullpath for lookup requested by the caller.
    UINT32 flags;   // See NS_MATCH_XXX above
    struct sysdir_ns_node *next;
} SysdirNsNode;

/** Initialize the namespace and mdmOwners rbtree cache in the caller's memory.
 * @param (IN) strDB: JSON formatted string of namespace and mdm owners
 *  returned by sys directory.
 * @param (IN) componentName: the component name of the calling app.
 *  Results containing this component name will be deleted from all returned results.
 *  May be NULL.
 **/
BcmRet sysdir_ns_init(const char *strDB, const char *componentName);

/** Free all resources allocated by sysdir_ns.**/
BcmRet sysdir_ns_close();

/** Returns a list of SysdirNsNode(s) that are the owner(s) of the fullpath.
 *
 * @param   fullpath (IN) The fullpath to be looked up.
 * @return
 * 1. NULL if no appropriate remote components are found.
 * 2. A pointer to a single SysdirNsNode which is the remote component which
 *    owns the fullpath.
 * 3. In cases where multiple remote components could be the owner, e.g.
 *    Device.Qos.Queue, a pointer to a singly linked list of SysdirNsNodes.
 * (Please note that in case 2 and 3, the value of fullpath is copied to the
 *  namespc field of the matching SysdirNsNode)
 * The list must be freed by calling sysdir_ns_freeNsNode().
 **/
SysdirNsNode* sysdir_ns_lookup(const char *fullpath);

/** Returns a component name that are the possible owner of the next object for the fullpath.
 *  This function is only used for MDM objects which has ranges, for example:
 *  Device.Qos.Queue.[800001-899999].
 * @param   fullpath (IN) The fullpath to be looked up.
 * @param   firstInstanceId (OUT) The first instanceId of the namespace found.
 * @param   lastInstanceId (OUT) The last instanceId of the namespace found.
 * @return
 * 1. NULL if no appropriate remote components are found.
 * 2. A pointer to a component name which owns the fullpath.
 **/
const char *sysdir_ns_lookupNextRange(const char *fullpath, UINT32 *firstInstaneId, UINT32 *lastInstanceId);

/** Free a list of SysdirNsNode. Set the head of the list to be NULL afterwards.**/
void sysdir_ns_freeNsNodes(SysdirNsNode **head);

/** Returns the number of nodes in a SysdirNsNode list. **/
UINT32 sysdir_ns_numNsNode(const SysdirNsNode *head);

/** Returns a list of SysdirCompNode of all remote mdmOwners.**/
SysdirCompNode* sysdir_ns_getAllRemoteComponents();

/** Free a list of SysCompNode. Set the head of the list to be NULL afterwards.**/
void sysdir_ns_freeCompNodes(SysdirCompNode **head);

/** Returns if a remote component exists or not. **/
UBOOL8 sysdir_ns_compExist(const char *compName);


/*********************************************************************
 *
 * The following utility functions do not use the internal
 * sys directory namespace database.  So unlike the above functions,
 * these can be called without calling sysdir_ns_init() first.
 *
 */
UBOOL8 sysdir_ns_isMultiComponentPath(const char *fullpath);

#endif /* __SYSDIR_NS_H__ */
