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


/*!\file sysdir.c
 * \brief core sys_directory functions in a shared library.
 *
 */

#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include "number_defs.h"
#include "os_defs.h"
#include "cms_util.h"  // for cmsMem_alloc
#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "genutil_rbtree.h"
#include "sys_directory.h"


rbtree_t rbtKeyValues;
rbtree_t rbtNamespaces;
rbtree_t rbtMdmOwners;
rbtree_t rbtInterfaces;
DLIST_HEAD(namespaceSubscribers);

BcmRet sysdir_init()
{
   rbtree_initFlags(&rbtKeyValues, RBT_FLAGS_STRCASECMP, NULL, NULL);
   rbtree_initFlags(&rbtNamespaces, RBT_FLAGS_STRCASECMP, NULL, NULL);
   rbtree_initFlags(&rbtMdmOwners, RBT_FLAGS_STRCASECMP, NULL, NULL);
   rbtree_initFlags(&rbtInterfaces, RBT_FLAGS_STRCASECMP, NULL, NULL);

   return BCMRET_SUCCESS;
}

/***************************************************************************
 *  Subscription related functions.
 *  Used by keyValue and interface event types.
 *
 */
static UBOOL8 alreadySubscribed(const DlistNode *head,
                                const char *compName, UINT32 eid)
{
   SysDirSubscriber *sub=NULL;
   DlistNode *curr=NULL;

   if (head == NULL)
   {
      bcmuLog_error("NULL list head!");
      return FALSE;
   }

   curr = head->next;
   while (curr != head)
   {
      sub = (SysDirSubscriber *) curr;
      if ((sub->eid == eid) && (!strcmp(sub->compName, compName)))
         return TRUE;

      curr = curr->next;
   }

   return FALSE;
}

static void insertSubscriber(DlistNode *head, const char *compName, UINT32 eid)
{
   SysDirSubscriber *sub=NULL;

   sub = (SysDirSubscriber *) cmsMem_alloc(sizeof(SysDirSubscriber),
                                           ALLOC_ZEROIZE);
   if (sub == NULL)
   {
      bcmuLog_error("Alloc of SysDirSubscriber failed");
      return;
   }
   snprintf(sub->compName, sizeof(sub->compName), "%s", compName);
   sub->eid = eid;
   dlist_append((DlistNode *) sub, head);
   return;
}

static void deleteSubscriber(DlistNode *head, const char *compName, UINT32 eid)
{
   SysDirSubscriber *sub;
   DlistNode *curr;

   if (head == NULL)
      return;

   curr = head->next;
   while (curr != head)
   {
      sub = (SysDirSubscriber *) curr;
      if ((sub->eid == eid) &&
          !cmsUtl_strcmp(sub->compName, compName))
      {
         dlist_unlink(curr);
         cmsMem_free(curr);
         break;
      }
      curr = curr->next;
   }
   return;
}

void deleteAllSubscribers(DlistNode *head)
{
   DlistNode *curr;

   if (head == NULL)
      return;

   curr = head->next;
   while (curr != head)
   {
      dlist_unlink(curr);
      cmsMem_free(curr);
      curr = head->next;
   }
   return;
}

static void transferSubscribers(DlistNode *dest, DlistNode *src)
{
   DlistNode *curr=src->next;

   // Unlink entries from src and link it into dest, one-by-one
   while (curr != src)
   {
      dlist_unlink(curr);
      dlist_append(curr, dest);
      curr = src->next;
   }
   return;
}


/***************************************************************************
 *  Publish, subscribe, and get Key Value events.
 *
 */
SysDirKeyValueNode *sysdir_newKeyValueNode(const char *compName,
                                           const char *key, const char *value)
{
   SysDirKeyValueNode *node;
   UINT32 valueLen, totalLen;

   if (value == NULL)  // should be at least a null termination byte
   {
      bcmuLog_error("value is NULL");
      return NULL;
   }

   bcmuLog_debug("[%s] %s=%s", compName, key, value);

   // Allocate a new node to hold new values.
   valueLen = strlen(value);
   totalLen = sizeof(SysDirKeyValueNode) + valueLen + 1;
   node = (SysDirKeyValueNode *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (node == NULL)
   {
      bcmuLog_error("Alloc of SysDirKeyValueNode failed (len=%d)", totalLen);
      return NULL;
   }

   snprintf(node->pubCompName, sizeof(node->pubCompName), "%s", compName);
   snprintf(node->key, sizeof(node->key), "%s", key);  // real storage for key
   node->rbnode.key = node->key;  // rbnode's key points to real storage
   sprintf((char *)(node+1), "%s", value);
   DLIST_HEAD_IN_STRUCT_INIT(node->subscribers);
   return node;
}

static void clearKeyValueNode(SysDirKeyValueNode *node)
{
   char *valuePtr = (char *)(node+1);

   memset(node->pubCompName, 0, sizeof(node->pubCompName));
   *valuePtr = '\0';
}

static UBOOL8 isTr69AutoSubscribedKey(const char *key __attribute__((unused)))
{
#ifdef SUPPORT_TR69C
   if ((strstr(key, PUBSUB_KEY_MDM_NOTIFICATION_PREFIX)) ||
       (strstr(key, PUBSUB_KEY_ATM_OAM_DIAG_COMPLETE)) ||
       (strstr(key, PUBSUB_KEY_DSL_LOOP_DIAG_COMPLETE)) ||
       (strstr(key, PUBSUB_KEY_DSL_SELT_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_PING_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_TRACERT_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_UDPECHO_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_DOWNLOAD_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_UPLOAD_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_OBUDPST_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_SERVERSELECTION_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_ETHCABLE_DIAG_COMPLETE)))
   {
      return TRUE;
   }
#endif
   return FALSE;
}

static UBOOL8 isUspAutoSubscribedKey(const char *key __attribute__((unused)))
{
#ifdef SUPPORT_OBUSPA
   // obuspa does not depend on MDM active notification to do USP
   // active notification, so no need to subscribe to PUBSUB_KEY_MDM_NOTIFICATION_PREFIX here.
   // All other diag related keys work the same in USP and tr69.
   if ((strstr(key, PUBSUB_KEY_ATM_OAM_DIAG_COMPLETE)) ||
       (strstr(key, PUBSUB_KEY_DSL_LOOP_DIAG_COMPLETE)) ||
       (strstr(key, PUBSUB_KEY_DSL_SELT_DIAG_COMPLETE)) ||
       (strstr(key, PUBSUB_KEY_CERTIFICATE_CHANGE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_PING_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_TRACERT_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_UDPECHO_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_DOWNLOAD_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_UPLOAD_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_OBUDPST_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_SERVERSELECTION_DIAG_COMPLETE)) ||
       (!cmsUtl_strcmp(key, PUBSUB_KEY_ETHCABLE_DIAG_COMPLETE)))
   {
      return TRUE;
   }
#endif
   return FALSE;
}

static UBOOL8 isApibdkAutoSubscribedKey(const char *key __attribute__((unused)))
{
#ifdef SUPPORT_BRCM_OPENWRT
   if (strstr(key, PUBSUB_KEY_MDM_ALT_NOTIFICATION_PREFIX) ||
         strstr(key, PUBSUB_KEY_MDM_NOTIFICATION_PREFIX) ||
         strstr(key, BDK_KEY_AP_CLIENT_ASSOC_CHANGED_PREFIX))
   {
      return TRUE;
   }
#endif
   return FALSE;
}


BcmRet sysdir_publishKeyValue(const char *compName, UINT32 eid,
                              const char *key, const char *value,
                              DlistNode **subDlist)
{
   SysDirKeyValueNode *node=NULL;
   SysDirKeyValueNode *currNode=NULL;

   bcmuLog_notice("compName %s/%d %s=%s", compName, eid, key, value);

   node = sysdir_newKeyValueNode(compName, key, value);
   if (node == NULL)
      return BCMRET_RESOURCE_EXCEEDED;

   currNode = (SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key);
   if (currNode)
   {
      bcmuLog_debug("Updating existing key %s", key);
      if (currNode->pubCompName[0] != '\0' &&
          strcmp(currNode->pubCompName, node->pubCompName))
      {
         // This is unexpected, but allow new publisher to overwrite existing one.
         bcmuLog_error("new compName detected for key %s (%s=>%s)", key,
                       currNode->pubCompName, node->pubCompName);
      }
      transferSubscribers(&(node->subscribers), &(currNode->subscribers));
      rbtree_delete(&rbtKeyValues, key);
      cmsMem_free(currNode);
      rbtree_insert(&rbtKeyValues, (rbnode_t *) node);
   }
   else
   {
      bcmuLog_debug("inserting new node [%s] %s=%s", compName, key, value);
      rbtree_insert(&rbtKeyValues, (rbnode_t *) node);      
   }

   // auto subscribe tr69 to special keys.  tr69_md will receive the
   // notification msgs, convert and forward to tr69c.
   if (isTr69AutoSubscribedKey(key))
   {
      if (!alreadySubscribed(&(node->subscribers), BDK_COMP_TR69, EID_TR69_MD))
      {
         bcmuLog_debug("auto subscribe key %s [%s/%d]", key, BDK_COMP_TR69, EID_TR69_MD);
         insertSubscriber(&(node->subscribers), BDK_COMP_TR69, EID_TR69_MD);
      }
   }

   // auto subscribe USP to special keys.  usp_md will receive the
   // notification msgs, convert and forward to obuspa.
   if (isUspAutoSubscribedKey(key))
   {
      if (!alreadySubscribed(&(node->subscribers), BDK_COMP_USP, EID_USP_MD))
      {
         bcmuLog_debug("auto subscribe key %s [%s/%d]", key, BDK_COMP_USP, EID_USP_MD);
         insertSubscriber(&(node->subscribers), BDK_COMP_USP, EID_USP_MD);
      }
   }

   // auto subscribe apibdk_sd to special keys.  apibdk_sd will receive the
   // notification msgs.
   if (isApibdkAutoSubscribedKey(key))
   {
      if (!alreadySubscribed(&(node->subscribers), BDK_APP_APIBDK_SD, EID_APIBDK_SD))
      {
         bcmuLog_debug("auto subscribe key %s [%s/%d]", key, BDK_APP_APIBDK_SD, EID_APIBDK_SD);
         insertSubscriber(&(node->subscribers), BDK_APP_APIBDK_SD, EID_APIBDK_SD);
      }
   }

   // Give caller the Dlist of subscribers to notify
   if (subDlist != NULL)
      *subDlist = &(node->subscribers);

   return BCMRET_SUCCESS;
}

BcmRet sysdir_unpublishKeyValue(const char *compName, const char *key,
                                DlistNode **subDlist)
{
   SysDirKeyValueNode *currNode=NULL;

   bcmuLog_notice("unpublish key=%s by %s", key, compName);

   currNode = (SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key);
   if (currNode)
   {
      int count = dlist_count(&(currNode->subscribers));
      int autoSubscribeCount = 0;

      if (isTr69AutoSubscribedKey(key))
         autoSubscribeCount++;
      if (isUspAutoSubscribedKey(key))
         autoSubscribeCount++;
      if (isApibdkAutoSubscribedKey(key))
         autoSubscribeCount++;

      bcmuLog_debug("key %s current subscribers %d (auto-subscribers=%d)",
                    key, count, autoSubscribeCount);

      if (currNode->pubCompName[0] != '\0' &&
          strcmp(currNode->pubCompName, compName))
      {
         // This is odd, but still allow it.
         bcmuLog_error("%s is deleting key %s published by %s",
                       compName, key, currNode->pubCompName);
      }

      // zero out pubCompName and value in case we don't delete this node.
      clearKeyValueNode(currNode);

      if ((count == 0) || (count == autoSubscribeCount))
      {
         // No subscribers, or only auto-subscribers.
         // Delete this node.  No notifications.
         rbtree_delete(&rbtKeyValues, key);
         deleteAllSubscribers(&(currNode->subscribers));
         cmsMem_free(currNode);
      }
      else
      {
         // Give caller the Dlist of subscribers to notify
         if (subDlist != NULL)
            *subDlist = &(currNode->subscribers);
      }
   }
   else
   {
      // Strange, but no big deal, the node does not exist.
      bcmuLog_debug("could not find key %s (compName=%s)", key, compName);
   }

   return BCMRET_SUCCESS;
}

const char *sysdir_subscribeKeyValue(const char *compName, UINT32 eid,
                                     const char *key)
{
   SysDirKeyValueNode *node=NULL;

   bcmuLog_notice("compName/eid %s/%d subscribing to key=%s", compName, eid, key);

   node = (SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key);
   if (node)
   {
      // Subscribing to an existing key
      // If not already subscribed, record this compName/eid as subscriber.
      bcmuLog_debug("found existing key %s", key);
      if (!alreadySubscribed(&(node->subscribers), compName, eid))
      {
         bcmuLog_debug("insert new subscriber %s/%d", compName, eid);
         insertSubscriber(&(node->subscribers), compName, eid);
      }
   }
   else
   {
      // Key does not exist yet.  Create new node.  Publisher and value are set
      // to empty string.
      bcmuLog_debug("subscribe to new key %s", key);
      node = sysdir_newKeyValueNode("", key, "");
      if (node == NULL)
         return "";

      insertSubscriber(&(node->subscribers), compName, eid);
      rbtree_insert(&rbtKeyValues, (rbnode_t *) node);
   }

   // Return the value string, which is right after the node (could be empty string).
   bcmuLog_debug("returning %s=%s", key, (const char *)(node+1));
   return ((const char *) (node+1));
}

void sysdir_unsubscribeKeyValue(const char *compName, UINT32 eid,
                                const char *key)
{
   SysDirKeyValueNode *node=NULL;

   bcmuLog_notice("[%s/%d] key=%s", compName, eid, key);

   node = (SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key);
   if (node)
   {
      deleteSubscriber(&(node->subscribers), compName, eid);
      // If no more subscribers and this is an unpublished key, delete.
      if (dlist_empty(&(node->subscribers)) &&
          IS_EMPTY_STRING(node->pubCompName))
      {
         bcmuLog_notice("delete unsubscribed and unpublished key %s", key);
         rbtree_delete(&rbtKeyValues, key);
         cmsMem_free(node);
      }
   }
   else
   {
      bcmuLog_debug("key %s does not exist [%s/%d]", key, compName, eid);
   }

   return;
}

const char *sysdir_getKeyValue(const char *key)
{
   SysDirKeyValueNode *node=NULL;

   bcmuLog_notice("get key=%s", key);

   node = (SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key);
   if (node)
   {
      bcmuLog_debug("returning %s=%s", key, (const char *)(node+1));
      return ((const char *) (node+1));
   }
   else
   {
      bcmuLog_debug("could not find key %s, returning empty string", key);
      return "";
   }
}

// For testing only.
const SysDirKeyValueNode *_sysdir_getKeyValueNode(const char *key)
{
   return ((const SysDirKeyValueNode *) rbtree_search(&rbtKeyValues, key));
}


/***************************************************************************
 *  Publish, subscribe, and get Namespace events.
 *
 */
SysDirNamespaceNode *sysdir_newNamespaceNode(const char *namespc,
                                             const char *compName)
{
   SysDirNamespaceNode *node=NULL;
   UINT32 totalLen=0;

   bcmuLog_debug("Entered: namespc=%s compName=%s", namespc, compName);

   // Allocate a new node to hold new values.
   totalLen = sizeof(SysDirNamespaceNode);
   node = (SysDirNamespaceNode *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (node == NULL)
   {
      bcmuLog_error("Alloc of SysDirNamespaceNode failed (len=%d)", totalLen);
      return NULL;
   }

   snprintf(node->nsData.ownerCompName, sizeof(node->nsData.ownerCompName),
            "%s", compName);
   snprintf(node->nsData.namespc, sizeof(node->nsData.namespc), "%s", namespc);  // real storage for key
   node->rbnode.key = node->nsData.namespc;  // rbnode's key points to real storage

   // Do additional proccessing if namespace has an instance id range
   if (cmsUtl_hasNamespaceRange(namespc))
   {
      cmsUtl_parseNamespaceRange(namespc,
                      &(node->firstInstanceId), &(node->lastInstanceId),
                      node->parsedNamespc);
   }

   return node;
}

SysDirMdmOwnerNode *sysdir_newMdmOwnerNode(const char *compName)
{
   SysDirMdmOwnerNode *node=NULL;
   UINT32 totalLen=0;

   // Allocate a new node to hold new values.
   totalLen = sizeof(SysDirMdmOwnerNode);
   node = (SysDirMdmOwnerNode *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (node == NULL)
   {
      bcmuLog_error("Alloc of SysDirMdmOwnerNode failed (len=%d)", totalLen);
      return NULL;
   }

   snprintf(node->ownerCompName, sizeof(node->ownerCompName), "%s", compName);
   node->rbnode.key = &(node->ownerCompName);  // rbnode's key points to real storage
   return node;
}

BcmRet sysdir_publishNamespace(const char *compName, UINT32 eid,
                               const char *namespc,
                               DlistNode **subDlist)
{
   SysDirNamespaceNode *node=NULL;
   SysDirNamespaceNode *currNode=NULL;

   bcmuLog_notice("[%s/%d] %s", compName, eid, namespc);

   node = sysdir_newNamespaceNode(namespc, compName);
   if (node == NULL)
      return BCMRET_RESOURCE_EXCEEDED;

   currNode = (SysDirNamespaceNode *) rbtree_search(&rbtNamespaces, namespc);
   if (currNode)
   {
      bcmuLog_debug("Updating existing namespace %s", namespc);
      if (currNode->nsData.ownerCompName[0] != '\0' &&
          strcmp(currNode->nsData.ownerCompName, node->nsData.ownerCompName))
      {
         // This is unexpected, but allow new owner to overwrite existing one.
         bcmuLog_error("new compName detected for namespace %s (%s=>%s)",
                       namespc,
                       currNode->nsData.ownerCompName,
                       node->nsData.ownerCompName);
      }
      rbtree_delete(&rbtNamespaces, namespc);
      cmsMem_free(currNode);
      rbtree_insert(&rbtNamespaces, (rbnode_t *) node);
   }
   else
   {
      bcmuLog_debug("inserting new node [%s] %s", compName, namespc);
      rbtree_insert(&rbtNamespaces, (rbnode_t *) node);
   }

   // As a side effect, also update our RBtree of known Distributed MDM owners.
   {
      SysDirMdmOwnerNode *curr;
      curr = (SysDirMdmOwnerNode *) rbtree_search(&rbtMdmOwners, compName);
      if (curr)
      {
         bcmuLog_debug("found existing MDM owner %s, do nothing", compName);
      }
      else
      {
         SysDirMdmOwnerNode *owner=sysdir_newMdmOwnerNode(compName);
         if (owner)
         {
            bcmuLog_debug("inserting new MDM owner %s", compName);
            rbtree_insert(&rbtMdmOwners, (rbnode_t *) owner);
         }
      }
   }

   // Give caller the Dlist of subscribers to notify.
   // For namespaces, the subscriber list is a global list.
   if (subDlist != NULL)
      *subDlist = &namespaceSubscribers;

   return BCMRET_SUCCESS;
}


// Global variable and helper func used by unpublishNamespace
static UBOOL8 _mdmOwnerFound;
void _findMdmOwner(rbnode_t *rbnode, void *arg)
{
   SysDirNamespaceNode *node = (SysDirNamespaceNode *) rbnode;
   char *compName = (char *)arg;
   if (!cmsUtl_strcmp(node->nsData.ownerCompName, compName))
      _mdmOwnerFound = TRUE;
}

BcmRet sysdir_unpublishNamespace(const char *compName, UINT32 eid,
                                 const char *namespc,
                                 DlistNode **subDlist)
{
   SysDirNamespaceNode *currNode=NULL;

   bcmuLog_notice("[%s/%d] %s", compName, eid, namespc);

   currNode = (SysDirNamespaceNode *) rbtree_search(&rbtNamespaces, namespc);
   if (currNode)
   {
      if (currNode->nsData.ownerCompName[0] != '\0' &&
          strcmp(currNode->nsData.ownerCompName, compName))
      {
         // Very strange, log it but still allow it.
         bcmuLog_error("%s/%d is deleting namespace %s originally from %s",
                       compName, eid, namespc,
                       currNode->nsData.ownerCompName);
      }
      rbtree_delete(&rbtNamespaces, namespc);
      cmsMem_free(currNode);

      // Give caller the Dlist of subscribers to notify.
      // For namespaces, the subscriber list is a global list.
      if (subDlist != NULL)
         *subDlist = &namespaceSubscribers;
   }
   else
   {
      bcmuLog_debug("namespace %s not found [%s/%d]", namespc, compName, eid);
   }

   // As a side effect, search through the entire rtbNamespaces tree looking
   // for this owner.  If no instances found, then delete this owner (compName),
   // from rtbMdmOwners. 
   {
      _mdmOwnerFound=FALSE;
      traverse_postorder(&rbtNamespaces, _findMdmOwner, (void *)compName);
      if (!_mdmOwnerFound)
      {
         SysDirMdmOwnerNode *curr;
         curr = (SysDirMdmOwnerNode *) rbtree_delete(&rbtMdmOwners, compName);
         cmsMem_free(curr);
      }
   }
   return BCMRET_SUCCESS;
}


char *sysdir_subscribeNamespace(const char *compName, UINT32 eid)
{
   bcmuLog_notice("compName/eid %s/%d", compName, eid);

   if (alreadySubscribed(&namespaceSubscribers, compName, eid))
   {
      bcmuLog_debug("ignore duplicate subscription from %s/%d", compName, eid);
   }
   else
   {
      bcmuLog_debug("insert new subscriber %s/%d", compName, eid);
      insertSubscriber(&namespaceSubscribers, compName, eid);
   }

   // Consistent with other event types, when you subscribe, you get the
   // current value.  Caller is responsible for freeing list.
   return sysdir_stringifyNamespaces(&rbtNamespaces, &rbtMdmOwners);
}

void sysdir_unsubscribeNamespace(const char *compName, UINT32 eid)
{
   bcmuLog_notice("compName/eid %s/%d", compName, eid);

   deleteSubscriber(&namespaceSubscribers, compName, eid);
   return;
}

UBOOL8 sysdir_doesMdmOwnerExist(const char *compName)
{
   rbnode_t *node;
   UBOOL8 exist=FALSE;

   if (IS_EMPTY_STRING(compName))
   {
      bcmuLog_error("compName is NULL or empty string %p", compName);
      return FALSE;
   }

   node = rbtree_search(&rbtMdmOwners, compName);
   exist = (node != NULL);
   return exist;
}

// Caller is responsible for freeing the returned string.
char *sysdir_getNamespace(const char *namespc)
{
   bcmuLog_notice("Entered: namespc=%s", namespc);

   return sysdir_stringifyNamespaces(&rbtNamespaces, &rbtMdmOwners);
}


/***************************************************************************
 *  Publish, subscribe, and get Interface events.
 *
 */
SysDirInterfaceNode *sysdir_newInterfaceNode(const char *compName,
                                             const PubSubInterfaceMsgBody *intfData)
{
   SysDirInterfaceNode *node=NULL;
   UINT32 totalLen;

   bcmuLog_debug("[%s] intfName%s", compName, intfData->intfName);

   // Allocate a new node.
   totalLen = sizeof(SysDirInterfaceNode) + intfData->additionalDataLen;
   node = (SysDirInterfaceNode *) cmsMem_alloc(totalLen, ALLOC_ZEROIZE);
   if (node == NULL)
   {
      bcmuLog_error("Alloc of SysDirInterfaceNode failed (len=%d)", totalLen);
      return NULL;
   }

   DLIST_HEAD_IN_STRUCT_INIT(node->subscribers);
   snprintf(node->pubCompName, sizeof(node->pubCompName), "%s", compName);
   
   // Copy all data from caller's msgBody to my localMsgBody
   // A simple assignment will work because all storge is local in the msg body.
   node->intfData = *intfData;
   node->rbnode.key = node->intfData.intfName;  // rbnode's key points to real storage
   // Copy any additional interface data
   if (intfData->additionalDataLen > 0)
   {
      memcpy((void *)(node+1), (void *)(intfData+1), intfData->additionalDataLen);
   }

   return node;
}

void clearInterfaceNode(SysDirInterfaceNode *node)
{
   PubSubInterfaceMsgBody emptyIntf;

   // Create an almost empty IntfMsgBody struct.  Only fill in intfName.
   // Copy this almost empty IntfMsgBody to the rbtree node.
   memset(&emptyIntf, 0, sizeof(emptyIntf));
   strcpy(emptyIntf.intfName, node->intfData.intfName);
   memcpy(&(node->intfData), &emptyIntf, sizeof(emptyIntf));

   // Also clear the publisher compName (since this node is being unpublished)
   memset(node->pubCompName, 0, sizeof(node->pubCompName));
   return;
}


BcmRet sysdir_publishInterface(const char *compName, UINT32 eid,
                               const PubSubInterfaceMsgBody *intfData,
                               DlistNode **subDlist)
{
   SysDirInterfaceNode *node=NULL;
   SysDirInterfaceNode *currNode=NULL;
   const char *key = intfData->intfName;

   bcmuLog_notice("compName %s/%d intfName=%s Ipv4Up=%d Ipv6Up=%d LinkUp=%d",
                  compName, eid, intfData->intfName,
                  intfData->isIpv4Up, intfData->isIpv6Up, intfData->isLinkUp);

   node = sysdir_newInterfaceNode(compName, intfData);
   if (node == NULL)
      return BCMRET_RESOURCE_EXCEEDED;

   currNode = (SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key);
   if (currNode)
   {
      bcmuLog_debug("Updating existing key %s node", key);
      if (currNode->pubCompName[0] != '\0' &&
          strcmp(currNode->pubCompName, node->pubCompName))
      {
         // This is unexpected, but allow new publisher to overwrite existing one.
         bcmuLog_error("new compName detected for key %s (%s=>%s)", key,
                       currNode->pubCompName, node->pubCompName);
      }
      transferSubscribers(&(node->subscribers), &(currNode->subscribers));
      rbtree_delete(&rbtInterfaces, key);
      cmsMem_free(currNode);
      rbtree_insert(&rbtInterfaces, (rbnode_t *) node);
   }
   else
   {
      bcmuLog_debug("inserting new node [%s] %s", compName, key);
      rbtree_insert(&rbtInterfaces, (rbnode_t *) node);
   }

#ifdef SUPPORT_BDK_SYSTEM_MANAGEMENT
   // Small hack: sysmgmt/EID_SSK automatically subscribes to interfaces not
   // published by sysmgmt.
   if (strcmp(compName, BDK_COMP_SYSMGMT))
   {
      if (!alreadySubscribed(&(node->subscribers), BDK_COMP_SYSMGMT, EID_SSK))
      {
         bcmuLog_debug("auto subscribe %s/%d", BDK_COMP_SYSMGMT, EID_SSK);
         insertSubscriber(&(node->subscribers), BDK_COMP_SYSMGMT, EID_SSK);
      }
   }
#endif

#ifdef SUPPORT_TR69C
   // Same small hack for tr69c: automatically subscribe to all
   // interfaces.  In TR69, the notification message goes to tr69_md, where
   // it is converted to the old CMS_MSG_WAN_CONNECTION_UP.
   if (!alreadySubscribed(&(node->subscribers), BDK_COMP_TR69, EID_TR69_MD))
   {
      bcmuLog_debug("auto subscribe %s/%d", BDK_COMP_TR69, EID_TR69_MD);
      insertSubscriber(&(node->subscribers), BDK_COMP_TR69, EID_TR69_MD);
   }
#endif

#ifdef SUPPORT_BRCM_OPENWRT
   /**
   * apibdk_sd takes pre-subscribe strategy to monitor all interfaces.
   */
   if (strcmp(compName, BDK_APP_APIBDK_SD))
   {
      /**
      * apibdk itself publishes interface event from OpenWrt side to BDK side, filter out of 
      * this to de-bounce
      */
      if (!alreadySubscribed(&(node->subscribers), BDK_APP_APIBDK_SD, EID_APIBDK_SD))
      {
         bcmuLog_debug("auto subscribe %s/%d", BDK_APP_APIBDK_SD, EID_APIBDK_SD);
         insertSubscriber(&(node->subscribers), BDK_APP_APIBDK_SD, EID_APIBDK_SD);
      }
   }
#endif

   // Give caller the Dlist of subscribers to notify
   if (subDlist != NULL)
      *subDlist = &(node->subscribers);

   return BCMRET_SUCCESS;
}

BcmRet sysdir_unpublishInterface(const char *compName, UINT32 eid,
                                 const PubSubInterfaceMsgBody *intfData,
                                 DlistNode **subDlist)
{
   SysDirInterfaceNode *currNode=NULL;
   const char *key = intfData->intfName;

   bcmuLog_notice("compName %s/%d intfName=%s",
                  compName, eid, intfData->intfName);

   currNode = (SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key);
   if (currNode)
   {
      int subscriberCount = dlist_count(&(currNode->subscribers));
      UBOOL8 autoSubscriberCount = 0;

      // TODO: add some flag to identify auto-subscribers so it is easy to
      // get a count of auto-subscribers
      if (alreadySubscribed(&(currNode->subscribers),
                            BDK_COMP_SYSMGMT, EID_SSK))
      {
         autoSubscriberCount++;
      }
      if (alreadySubscribed(&(currNode->subscribers),
                            BDK_COMP_TR69, EID_TR69_MD))
      {
         autoSubscriberCount++;
      }
      if (alreadySubscribed(&(currNode->subscribers),
                            BDK_COMP_USP, EID_OBUSPA))
      {
         autoSubscriberCount++;
      }
      bcmuLog_notice("key=%s current subscribers=%d autoSubscribers=%d",
                    key, subscriberCount, autoSubscriberCount);

      if (currNode->pubCompName[0] != '\0' &&
          strcmp(currNode->pubCompName, compName))
      {
         // Strange, log it but still allow it.
         bcmuLog_error("%s/%d is deleting %s originally published by %s",
                       compName, eid, key, currNode->pubCompName);
      }

      // Zero out intf structure in case we don't delete it.
      clearInterfaceNode(currNode);

      if ((subscriberCount == 0) ||
          (subscriberCount == autoSubscriberCount))
      {
         // No subscribers, or all subscribers are auto-subscribers.
         // Delete this node. No notifications.
         rbtree_delete(&rbtInterfaces, key);
         deleteAllSubscribers(&(currNode->subscribers));
         cmsMem_free(currNode);
      }
      else
      {
         // Don't delete.  There are still subscribers to this intf
         // Give caller the Dlist of subscribers to notify
         if (subDlist != NULL)
            *subDlist = &(currNode->subscribers);
      }
   }
   else
   {
      bcmuLog_debug("could not find intf %s to unpublish, requested by %s",
                    key, compName);
   }

   return BCMRET_SUCCESS;
}

const PubSubInterfaceMsgBody *sysdir_subscribeInterface(const char *compName,
                                                  UINT32 eid, const char *key)
{
   SysDirInterfaceNode *node=NULL;

   bcmuLog_notice("compName/eid %s/%d subscribing to key=%s", compName, eid, key);

   node = (SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key);
   if (node)
   {
      // Subscribing to an existing key
      // If not already subscribed, record this compName/eid as subscriber.
      bcmuLog_debug("found existing key %s", key);
      if (!alreadySubscribed(&(node->subscribers), compName, eid))
      {
         bcmuLog_debug("insert new subscriber %s/%d", compName, eid);
         insertSubscriber(&(node->subscribers), compName, eid);
      }
   }
   else
   {
      PubSubInterfaceMsgBody intfData;

      // Key does not exist yet.  Create new node.  Publisher is set to empty
      // string.  IntfData is empty struct with just the intfName (key) filled in.
      bcmuLog_debug("subscribe to new key %s", key);
      memset(&intfData, 0, sizeof(intfData));
      snprintf(intfData.intfName, sizeof(intfData.intfName), "%s", key);
      node = sysdir_newInterfaceNode("", &intfData);
      if (node == NULL)
         return NULL;

      insertSubscriber(&(node->subscribers), compName, eid);
      rbtree_insert(&rbtInterfaces, (rbnode_t *) node);
   }

   return (&(node->intfData));
}

void sysdir_unsubscribeInterface(const char *compName, UINT32 eid,
                                 const char *key)
{
   SysDirInterfaceNode *node=NULL;

   bcmuLog_notice("[%s/%d] intfName(key)=%s", compName, eid, key);

   node = (SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key);
   if (node)
   {
      deleteSubscriber(&(node->subscribers), compName, eid);
      // If no more subscribers and this is an unpublished interface, delete.
      if (dlist_empty(&(node->subscribers)) &&
          IS_EMPTY_STRING(node->pubCompName))
      {
         bcmuLog_notice("delete unsubscribed and unpublished intf %s", key);
         rbtree_delete(&rbtInterfaces, key);
         cmsMem_free(node);
      }
   }
   else
   {
      bcmuLog_debug("could not find intf %s to unsubscribe from [%s/%d]",
                    key, compName, eid);
   }

   return;
}

const PubSubInterfaceMsgBody *sysdir_getInterface(const char *key)
{
   SysDirInterfaceNode *node=NULL;

   bcmuLog_notice("key=%s", key);

   node = (SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key);
   if (node)
   {
      return (&(node->intfData));
   }
   else
   {
      bcmuLog_debug("could not find key %s", key);
      return NULL;
   }
}

typedef enum
{
    QueryInvalid = 0,
    QueryAll,
    QueryLayer2,
    QueryLayer3,
    QueryBridgeType,
    QueryBridgeIndex,
    QueryFullpath,
} QueryIntfType;

typedef enum
{
    VAL_NULL,
    VAL_INT,
    VAL_STRING
} QueryValueType;

typedef struct {
   QueryIntfType type; 
   int intValue;     // value to match
   const char *strValue;  // string value to match
   char *resultBuf;  // buffer to hold result
} QueryIntfCtx;

typedef struct {
    char *queryString;
    QueryIntfType type;
    QueryValueType valType;
} QueryMapping;

static QueryMapping queryMappingTable[] = 
{
    {"*", QueryAll, VAL_NULL},
    {"isLayer2=", QueryLayer2, VAL_INT},
    {"isLayer3=", QueryLayer3, VAL_INT},
    {"bridgeType=", QueryBridgeType, VAL_STRING},
    {"bridgeIndex=", QueryBridgeIndex, VAL_INT},
    {"fullpath=", QueryFullpath, VAL_STRING},
};

// Max length of buffer to hold results
#define QUERY_INTF_LEN   2048

static char *alloc_query_result_buf()
{
   char *buf = cmsMem_alloc(QUERY_INTF_LEN, ALLOC_ZEROIZE);
   return buf;
}

static void append_intfName_to_buf(char *buf, const char *intfName)
{
   if (strlen(buf) == 0)
   {
      strcpy(buf, intfName);
   }
   else if (strlen(buf) + strlen(intfName) + 2 < QUERY_INTF_LEN)
   {
      strcat(buf, ",");
      strcat(buf, intfName);
   }
   else
   {
      bcmuLog_error("Too much data (max=%d) %s", QUERY_INTF_LEN, buf);
   }
   return;
}

static void query_interface_func(rbnode_t *node, void *data)
{
   SysDirInterfaceNode *intfNode = (SysDirInterfaceNode *) node;
   QueryIntfCtx        *ctx      = (QueryIntfCtx *) data;

   if (ctx->type == QueryAll)
   {
      // match all interfaces
      append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
   }
   else if (ctx->type == QueryLayer2)
   {
      if (intfNode->intfData.isLayer2 == (UBOOL8) ctx->intValue)
      {
         append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
      }
   }
   else if (ctx->type == QueryLayer3)
   {
      if (intfNode->intfData.isLayer3 == (UBOOL8) ctx->intValue)
      {
         append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
      }
   }
   else if (ctx->type == QueryBridgeType)
   {
      if (0 == strcmp(intfNode->intfData.bridgeType, ctx->strValue))
      {
         append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
      }
   }
   else if (ctx->type == QueryBridgeIndex)
   {
      if ((intfNode->intfData.bridgeType[0] != '\0') &&
          (intfNode->intfData.pubCompName[0] != '\0') &&
          (intfNode->intfData.bridgeIndex == ctx->intValue))
      {
         append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
      }
   }
   else if (ctx->type == QueryFullpath)
   {
      if (0 == strcmp(intfNode->intfData.fullpath, ctx->strValue))
      {
         append_intfName_to_buf(ctx->resultBuf, intfNode->intfData.intfName);
      }
   }
   else
   {
      bcmuLog_error("unsupported query ctx type %d", ctx->type);
   }

   return;
}

static int createQueryIntfCtx(const char *queryStr, QueryIntfCtx *ctx)
{
   int i, typeLen;
   const char *ptr;
   int found = 0;

   for (i = 0; i < (int)(sizeof(queryMappingTable) / sizeof(QueryMapping)); i++)
   {
      typeLen = strlen(queryMappingTable[i].queryString);

      if(0 == strncasecmp(queryMappingTable[i].queryString, queryStr, typeLen))
      {
          ctx->type = queryMappingTable[i].type;
          ptr = queryStr + typeLen;
          switch (queryMappingTable[i].valType)
          {
             case VAL_INT:
                 ctx->intValue = atoi(ptr);
                 break;
             
             case VAL_STRING:
                 ctx->strValue = ptr;
                 break; 
             
             case VAL_NULL:
                 break;

             default:
                 bcmuLog_error("Unknown value type %d!", queryMappingTable[i].valType);
                 return -1;
          }
          found = 1;
          break;
      }
   }
   return (found == 1 ? 0 : -1);
}

char *sysdir_queryInterface(const char *queryStr)
{
   QueryIntfCtx ctx;
   char *resultBuf = NULL;

   bcmuLog_notice("Entered: queryStr=%s", queryStr);

   if (0 != createQueryIntfCtx(queryStr, &ctx))
   {
       bcmuLog_error("Failed to find query for %s", queryStr);
       return NULL;
   }

   resultBuf = alloc_query_result_buf();
   if (resultBuf == NULL)
   {
       bcmuLog_error("Failed to allocate memory!");
   }
   else
   {
       ctx.resultBuf = resultBuf;
       traverse_postorder(&rbtInterfaces, query_interface_func, &ctx);
   }
   return resultBuf;
}

// For testing only.
const SysDirInterfaceNode *_sysdir_getInterfaceNode(const char *key)
{
   return((const SysDirInterfaceNode *) rbtree_search(&rbtInterfaces, key));
}

