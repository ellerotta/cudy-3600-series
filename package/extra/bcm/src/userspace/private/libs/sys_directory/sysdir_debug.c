/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2019:proprietary:standard

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

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "genutil_rbtree.h"
#include "sys_directory.h"


/*!\file sysdir_debug.c
 * \brief Debug functions for sys_directory
 *
 */

// Allow users to directly access these RBtrees for now, but should force them
// to use a read-only getter.
extern rbtree_t rbtKeyValues;
extern rbtree_t rbtNamespaces;
extern rbtree_t rbtMdmOwners;
extern rbtree_t rbtInterfaces;

void sysdir_dumpSubscribers(const DlistNode *head)
{
   DlistNode *curr=head->next;
   SysDirSubscriber *sub;

   printf("  Subscribers: ");
   while (curr != head)
   {
      sub = (SysDirSubscriber *) curr;
      printf("%s/%d ", sub->compName, sub->eid);
      curr = curr->next;
   }
   printf("\n\n");
}


void _dumpKeyValueData(rbnode_t *rbnode, void *arg __attribute__((unused)))
{
   SysDirKeyValueNode *node = (SysDirKeyValueNode *) rbnode;
   const char *value = (const char *) (node+1);

   printf("[%s] %s=%s\n", node->pubCompName, node->key, value);

   if (!dlist_empty(&(node->subscribers)))
   {
      sysdir_dumpSubscribers(&(node->subscribers));
   }
   printf("\n");
   return;
}

void sysdir_dumpKeyValues()
{
   printf("Dumping Key Value pairs:\n");
   traverse_postorder(&rbtKeyValues, _dumpKeyValueData, NULL);
   printf("\n\n");
}


void _dumpNamespaceData(rbnode_t *rbnode, void *arg __attribute__((unused)))
{
   SysDirNamespaceNode *node = (SysDirNamespaceNode *) rbnode;
   printf("%s [%s]\n", node->nsData.namespc, node->nsData.ownerCompName);
   if ((node->firstInstanceId > 0) && (node->lastInstanceId > 0) &&
       (node->parsedNamespc[0] != '\0'))
   {
      printf("  instanceId range [%u-%u] base=%s\n",
             node->firstInstanceId, node->lastInstanceId, node->parsedNamespc);
   }
   return;
}

void sysdir_dumpNamespaces()
{
   printf("Dumping Namespaces:\n");
   traverse_postorder(&rbtNamespaces, _dumpNamespaceData, NULL);
   // There is only 1 global list of namespace subscribers
   printf("\nDumping global list of namespace subscribers:\n");
   if (!dlist_empty(&(namespaceSubscribers)))
   {
      sysdir_dumpSubscribers(&(namespaceSubscribers));
   }
   printf("\n\n");
   return;
}

void _dumpMdmOwnerData(rbnode_t *rbnode, void *arg __attribute__((unused)))
{
   SysDirMdmOwnerNode *node = (SysDirMdmOwnerNode *) rbnode;
   printf("%s\n", node->ownerCompName);
   return;
}

void sysdir_dumpMdmOwners()
{
   printf("Dumping MDM Owners:\n");
   traverse_postorder(&rbtMdmOwners, _dumpMdmOwnerData, NULL);
   printf("\n\n");
   return;
}

void _dumpInterfaceData(rbnode_t *rbnode, void *arg __attribute__((unused)))
{
   SysDirInterfaceNode *node = (SysDirInterfaceNode *) rbnode;
   // const char *extraData = (const char *) (node+1);

   printf("[%s]:\n", (char *) node->rbnode.key);
   printf("  isUpstream: %d\n", node->intfData.isUpstream);

   // Layer 2 related info
   printf("  isLayer2:   %d\n", node->intfData.isLayer2);
   if (node->intfData.isLayer2)
   {
      printf("  isLinkUp:   %d\n", node->intfData.isLinkUp);
   }

   // Bridge related info
   if (node->intfData.bridgeType[0] != '\0')
   {
      printf("  bridgeType:   %s\n", node->intfData.bridgeType);
      printf("  bridgeIndex:   %d\n", node->intfData.bridgeIndex);
   }

   // Layer 3 related info
   printf("  isLayer3:   %d\n", node->intfData.isLayer3);
   if (node->intfData.isLayer3)
   {
      printf("  isIpv4Enabled: %d\n", node->intfData.isIpv4Enabled);
      if (node->intfData.isIpv4Enabled)
      {
         printf("  isIpv4Up:   %d\n", node->intfData.isIpv4Up);
         if (node->intfData.isIpv4Up)
         {
            printf("  ipv4Addr/mask: %s/%s\n", node->intfData.ipv4Addr,
                                               node->intfData.ipv4Netmask);
            printf("  ipv4Gateway: %s\n", node->intfData.ipv4Gateway);
         }
      }

      printf("  isIpv6Enabled: %d\n", node->intfData.isIpv6Enabled);
      if (node->intfData.isIpv6Enabled)
      {
         printf("  isIpv6Up:   %d\n", node->intfData.isIpv6Up);
         if (node->intfData.isIpv6Up)
         {
            printf("  ipv6GlobalAddr: %s\n", node->intfData.ipv6GlobalAddr);
            printf("  ipv6NextHop: %s\n", node->intfData.ipv6NextHop);
         }
      }

      printf("  dnsServers: %s\n", node->intfData.dnsServers);
      if (node->intfData.childrenIntfNames[0] != '\0')
      {
         printf("  childrenInterfaces: %s\n", node->intfData.childrenIntfNames);
      }
   }

   printf("  wlBrIndex: %d\n", node->intfData.wlBrIndex);
   printf("  published by: %s\n", node->intfData.pubCompName);
   printf("  (optional) fullpath: %s\n", node->intfData.fullpath);
   if (node->intfData.tags[0] != '\0')
      printf("  tags: %s\n", node->intfData.tags);

   printf("  additionaDataLen: %d\n", node->intfData.additionalDataLen);
   if (node->intfData.additionalDataLen > 0)
      printf("  additionalData: %s\n", (const char *) (node+1));

   if (!dlist_empty(&(node->subscribers)))
   {
      sysdir_dumpSubscribers(&(node->subscribers));
   }
   printf("\n");
}

void sysdir_dumpInterfaces()
{
   printf("Dumping Interfaces:\n");
   traverse_postorder(&rbtInterfaces, _dumpInterfaceData, NULL);
   printf("\n\n");
   return;
}

