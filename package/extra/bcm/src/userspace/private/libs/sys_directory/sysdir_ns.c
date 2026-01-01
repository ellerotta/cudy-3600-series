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

#include <stdio.h>
#include <errno.h>

#include "cms_util.h"
#include "bcm_ulog.h"
#include "sys_directory_ns.h"
#include "sys_directory.h"
#include "genutil_rbtree.h"
#include "mdm_params.h"  // for MDM_ALIAS_BUFLEN

// The next function is "stateless", meaning it does not require the namespace
// database to be initialized with sysdir_ns_init().
// These types of functions are either here in sysdir_ns.c or in 
// mdm_binaryhelper.c (mdm_isMultiComponentOid, mdm_isLocalObject,
// mdm_possibleRemoteSubTree, etc)

// This is for detecting when a getParamValues, getParamNames, getParamAttributes
// from the CMS PHL could potentially go over multiple components.
UBOOL8 sysdir_ns_isMultiComponentPath(const char *fullpath)
{
   if (!strcmp(fullpath, "Device.") ||
       !strcmp(fullpath, "Device.Services.") ||
       !strcmp(fullpath, "Device.X_BROADCOM_COM_AppCfg.") ||
       !strcmp(fullpath, "Device.IP.") ||
       !strcmp(fullpath, "Device.InterfaceStack.") ||
       !strcmp(fullpath, "Device.QoS.") ||
       !strcmp(fullpath, "Device.QoS.Queue.") ||
       !strcmp(fullpath, "Device.QoS.QueueStats.") ||
       !strcmp(fullpath, "Device.QoS.Shaper."))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

static rbtree_t rbtNamespaces;
static rbtree_t rbtMdmOwners;
static char myCompName[BDK_COMP_NAME_LEN + 1];

typedef void (*mdmOwners_foreach_func)(const char *componentName, void *data);
typedef void (*nameSpaces_foreach_func)(const SysDirNamespaceNode *namespaceNode,
                                        void *data);

typedef struct {
    const char *reqFullpath;  // requested fullpath to match
    UINT32      instanceId;   // if non-zero, consider this instanceId for match
    char        aliasBuf[MDM_ALIAS_BUFLEN]; // if not empty, consider for match
    SysdirNsNode *matchList;  // singly linked list of matched results
} MatchData;

typedef struct {
    const char *reqFullpath;  // requested fullpath to match
    UINT32      instanceId;   // if non-zero, consider this instanceId for match
    UINT32      firstInstanceId;   //the first instanceId in the range of the returned component
    UINT32      lastInstanceId; //the lastInstanceId in the range of the returned component
    const char *compName;     //the matched component name. There is only one result in this case.
} MatchRangeData;

typedef struct sysdir_ns_inner_operation {
    void *inner_func;
    void *args;
} SysDirNsInnerOperation;

static void sysdir_ns_foreachNamespace(SysDirNsInnerOperation *iop);
static void sysdir_ns_foreachMdmOwner(SysDirNsInnerOperation *iop);

static void _rbtree_free_node(rbnode_t *rbnode, void *arg __attribute__ ((unused)))
{
    cmsMem_free(rbnode);
}


// Given the internal SysDirNamespaceNode and requested Fullpath, return a
// newly allocated and filled in SysdirNsNode (result NS node).
static SysdirNsNode *getNewNsNode(const SysDirNamespaceNode *namespaceNode,
                                  const char *reqFullpath, UINT32 matchFlags)
{
    SysdirNsNode *nsNode;
    nsNode = cmsMem_alloc(sizeof(SysdirNsNode), ALLOC_ZEROIZE);
    if (nsNode == NULL)
    {
        bcmuLog_error("Failed to alloc SysdirNsNode!");
        return NULL;
    }

    // these structures are both PubSubNamespaceMsgBody, so just do memcpy
    memcpy((void *)&(nsNode->info), (void *)&(namespaceNode->nsData),
           sizeof(nsNode->info));
    strncpy(nsNode->reqFullpath, reqFullpath, sizeof(nsNode->reqFullpath)-1);
    nsNode->flags = matchFlags;
    return nsNode;
}


// This was called "prefix match".
static void inner_partial_match_namespace(const SysDirNamespaceNode *namespaceNode, void *data)
{
    MatchData *md = (MatchData *) data;
    size_t reqFullpathLen = strlen(md->reqFullpath);
    size_t baseNamespcLen;

    if (!IS_EMPTY_STRING(namespaceNode->parsedNamespc))
    {
        // If a namespace is Device.QoS.Queue.[800001-899999],
        // the parsedNamespc is Device.QoS.Queue.
        baseNamespcLen = strlen(namespaceNode->parsedNamespc);
    }
    else
    {
        baseNamespcLen = strlen(namespaceNode->nsData.namespc);
    }

    // Sometimes, the namespace is shorter than the reqFullpath, e.g.
    // [sysmgmt]Device. and reqFullpath is Device.IP.
    // Sometimes, the reqFullpath is shorter than the namespace, e.g.
    // [sysmgmt]Device.QoS.Queue.[800001-899999] and reqFullpath is Device.QoS.
    // Both of the above cases are a match.  So we need to pick the shorter
    // of the 2 strings to do the strncmp.
    size_t cmpLen = (baseNamespcLen < reqFullpathLen) ? baseNamespcLen : reqFullpathLen;

    bcmuLog_debug("matching [%s]%s against %s (cmpLen=%zu)",
                  namespaceNode->nsData.namespc,
                  namespaceNode->nsData.ownerCompName,
                  md->reqFullpath, cmpLen);
    if (strncmp(namespaceNode->nsData.namespc, md->reqFullpath, cmpLen) == 0)
    {
        SysdirNsNode *nsNode;
        
        bcmuLog_debug("MATCHED: %s ==> [%s]%s", md->reqFullpath,
                      namespaceNode->nsData.namespc,
                      namespaceNode->nsData.ownerCompName);
        nsNode = getNewNsNode(namespaceNode, md->reqFullpath, NS_MATCH_PREFIX);
        if (nsNode != NULL)
        {
            // link this new nsNode into our results list
            nsNode->next = md->matchList;
            md->matchList = nsNode;
        }
    }
    return;
}

static void inner_lookup_next_namespace_with_range(const SysDirNamespaceNode *namespaceNode,
                                                   void *data)
{
    MatchRangeData *mr = (MatchRangeData *) data;
    size_t baseNamespcLen;

    //Already found the next component with ranges higher than given mr->instanceId
    if (mr->compName != NULL) return;

    if (IS_EMPTY_STRING(namespaceNode->parsedNamespc))
    {
        //This function only lookup namespace with ranges.
        return;
    }

    // If a namespace is Device.QoS.Queue.[800001-899999],
    // the parsedNamespc is Device.QoS.Queue.
    baseNamespcLen = strlen(namespaceNode->parsedNamespc);

    bcmuLog_debug("matching %s:[%s] against %s (cmpLen=%zu)",
                  namespaceNode->nsData.ownerCompName,
                  namespaceNode->nsData.namespc,
                  mr->reqFullpath, baseNamespcLen);

    if (strncmp(namespaceNode->parsedNamespc, mr->reqFullpath, baseNamespcLen) == 0)
    {
        if (namespaceNode->firstInstanceId > mr->instanceId)
        {
            mr->firstInstanceId = namespaceNode->firstInstanceId;
            mr->lastInstanceId = namespaceNode->lastInstanceId;
            mr->compName = namespaceNode->nsData.ownerCompName;
            bcmuLog_notice("update record: %s ==> [%s] %s",
                      mr->reqFullpath,
                      namespaceNode->nsData.namespc,
                      namespaceNode->nsData.ownerCompName);
        }
    }
    return;
}

static SysdirNsNode *getMultiComponentsByPrefix(const char *fullpath)
{
    MatchData md;
    SysDirNsInnerOperation iop;
    SysdirNsNode *currNsNode, *q, *result = NULL;

    // Set up the args for the walk over all namespaces
    memset((void *) &md, 0, sizeof(md));
    memset((void *) &iop, 0, sizeof(iop));
    md.reqFullpath = fullpath;
    iop.inner_func = inner_partial_match_namespace;
    iop.args = (void *)&md;

    // do the match
    sysdir_ns_foreachNamespace(&iop);

    if (md.matchList == NULL)
    {
        // In theory this is possible, but in real world, not sure when it could
        // happen.  It is possible for MDM to ask the remote_objd
        // to lookup a fullpath, but remote_objd could not find any owner
        // for that fullpath.  The caller must be able to handle this scenario.
        bcmuLog_error("Couldn't find any components managing namespace with prefix[%s]", fullpath);
        return NULL;
    }

    // Eliminate duplicate mdmOwners in matchList.
    // Unique entries are transfered to result.  Non-unique entries are freed.
    while (md.matchList != NULL)
    {
        currNsNode = md.matchList;

        //Check if we already included the component in the final result list.
        q = result;
        while (q != NULL)
        {
            if (0 == strcmp(currNsNode->info.ownerCompName, q->info.ownerCompName))
            {
                break;
            }
            q = q->next;
        }

        if (q == NULL)
        {
            // currNsNode points to a unique component.  Remove it from
            // md.matchLisst
            md.matchList = currNsNode->next;

            // put currNsNode in the final result list.
            currNsNode->next = result;
            result = currNsNode;
        }
        else
        {
            // the component is already on the final result list, no need
            // to add this one. freee it.
            md.matchList = currNsNode->next;
            currNsNode->next = NULL;
            sysdir_ns_freeNsNodes(&currNsNode);
        }
    }

    return result;
}


// convert user provided alias to lowercase since our BDK comp names are
// all lowercase.  We don't seem to have strcasestr in our system.
static void local_tolower(char *str)
{
    int i=0;
    while (str[i] != '\0')
    {
       str[i] = (char) tolower(str[i]);
       i++;
    }
}

static void inner_exact_match_namespace(const SysDirNamespaceNode *namespaceNode, void *data)
{
    MatchData *md = (MatchData *) data;
    UINT32 matchFlags = 0;

    bcmuLog_debug("matching %s (%d/%s) against [%s]%s",
                  md->reqFullpath, md->instanceId, md->aliasBuf,
                  namespaceNode->nsData.ownerCompName,
                  namespaceNode->nsData.namespc);

    if (md->instanceId != 0)
    {
        // there was an instance id in the fullpath which may tell us
        // which component owns this fullpath.
        if ((namespaceNode->firstInstanceId <= md->instanceId) &&
            (md->instanceId <= namespaceNode->lastInstanceId) &&
            (0 == strcmp(namespaceNode->parsedNamespc, md->reqFullpath)))
        {
            matchFlags = NS_MATCH_INSTANCE_ID;
        }
    }
    else if (!IS_EMPTY_STRING(md->aliasBuf))  // instanceId and aliasBuf are mutually exclusive
    {
        // there was an alias in the fullpath which may tell us which
        // component this fullpath is supposed to go to.
        local_tolower(md->aliasBuf);
        if (strstr(md->aliasBuf, namespaceNode->nsData.ownerCompName) &&
            (0 == strcmp(namespaceNode->parsedNamespc, md->reqFullpath)))
        {
            matchFlags = NS_MATCH_ALIAS;
        }
    }

    // Do normal fullpath matching without instanceId or alias hints
    if ((0 == matchFlags) &&
        (0 == strcmp(namespaceNode->nsData.namespc, md->reqFullpath)))
    {
        matchFlags = NS_MATCH_LONGEST_PREFIX;
    }

    if (matchFlags)
    {
        SysdirNsNode *nsNode;
        bcmuLog_debug("MATCHED(0x%x): %s (%d/%s) ==> [%s]%s",
                      matchFlags, md->reqFullpath, md->instanceId, md->aliasBuf,
                      namespaceNode->nsData.ownerCompName,
                      namespaceNode->nsData.namespc);
        if (md->matchList == NULL)
        {
            // This is our first match, create a new nsNode and link into list.
            nsNode = getNewNsNode(namespaceNode, md->reqFullpath, matchFlags);
            if (nsNode != NULL)
            {
                md->matchList = nsNode;
            }
        }
        else
        {
            // Highly suspicious.  Why did the path match another
            // namespace entry?  Is there overlapping namespace claims?
            // Return all nodes to the caller to figure out.
            bcmuLog_error("Already a match in list for %s (%d/%s) : [%s]%s",
                           md->reqFullpath, md->instanceId, md->aliasBuf,
                           md->matchList->info.ownerCompName,
                           md->matchList->info.namespc);
            nsNode = getNewNsNode(namespaceNode, md->reqFullpath, matchFlags);
            if (nsNode != NULL)
            {
                nsNode->next = md->matchList;
                md->matchList = nsNode;
            }
        }
    }
    return;
}

static void extractInstanceId(char *dup, UINT32 *instanceId)
{
    UBOOL8 allDigits = TRUE;
    int i=0;
    
    while ((dup[i] != '.') && (dup[i] != '\0'))
    {
       if (!isdigit(dup[i]))
           allDigits = FALSE;
       i++;
    }

    // terminate by zapping the end dot.
    dup[i] = '\0';

    if (!allDigits)
    {
        bcmuLog_error("Bad or suspicious instanceId %s", dup);
    }
    else
    {
        *instanceId = (UINT32) atoi(dup);
        bcmuLog_debug("Extracted instanceId %u", *instanceId);
    }
    return;
}

static void extractAlias(char *dup, char *aliasBuf, UINT32 aliasBufLen)
{
    int i=0;

    bcmuLog_debug("Entered: dup=%s", dup);

    while ((dup[i] != ']') && (dup[i] != '\0'))
       i++;

    if (dup[i] != ']')
    {
        bcmuLog_error("Bad or suspicious alias [%s", dup);
    }
    else
    {
        // terminate by zapping the close bracket
        dup[i] = '\0';
        strncpy(aliasBuf, dup, aliasBufLen-1);
        bcmuLog_debug("Extracted alias %s", aliasBuf);
    }
    return;
}

// If there is a single remote component that owns this fullpath, then return
// the name of the remote component.  Otherwise, return NULL.
static SysdirNsNode *getSingleCompByLongestPrefix(const char *fullpath)
{
    MatchData md;
    SysDirNsInnerOperation iop;
    char *dup = NULL;
    int i;

    // Set up the common args for the walk over all namespaces
    memset((void *) &md, 0, sizeof(md));
    memset((void *) &iop, 0, sizeof(iop));
    iop.inner_func = inner_exact_match_namespace;
    iop.args = (void *)&md;

    bcmuLog_debug("Entered: fullpath=%s", fullpath);

    dup = cmsMem_strdup(fullpath);
    for (i = strlen(fullpath); i > 0; i--)
    {
        /* Search the namespace DB using decreasingly shorter generic fullpath.
         * As soon as we get a match in the namespace DB, that is the single
         * result.  If no match found, return NULL.
         */
        if (dup[i] == '.')
        {
            // check for instanceId or alias after the dot
            if (isdigit(dup[i+1]))
            {
                extractInstanceId(&(dup[i+1]), &(md.instanceId));
            }
            else if (dup[i+1] == '[')
            {
                extractAlias(&(dup[i+2]), md.aliasBuf, sizeof(md.aliasBuf));
            }

            dup[i+1] = '\0';  // terminate the string after the dot.

            // search all namespace entries for a match
            md.reqFullpath = dup;
            sysdir_ns_foreachNamespace(&iop);
            if (md.matchList != NULL)
            {
                // we need to copy the full requested fullpath into the
                // nsNode, not the subpath that matched.  There should be
                // exactly 1 element in the list.
                strncpy(md.matchList->reqFullpath, fullpath,
                        sizeof(md.matchList->reqFullpath)-1);
                break;
            }
            else
            {
                // clear matchData for the next shorter segment of the fullpath
                memset((void *) &md, 0, sizeof(md));
            }
        }
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(dup);
    return md.matchList;
}

BcmRet sysdir_ns_init(const char *strDB, const char *compName)
{
    BcmRet ret;

    bcmuLog_notice("strDB=%s", strDB);

    rbtree_initFlags(&rbtNamespaces, RBT_FLAGS_STRCASECMP, NULL, NULL);
    rbtree_initFlags(&rbtMdmOwners, RBT_FLAGS_STRCASECMP, NULL, NULL);

    // Note that rbtNamespaces and rbtMdmOwners will include myself.
    // Each function or algorithm will filter out myCompName as needed.
    // I guess this makes sense if we are doing a longest prefix match on a
    // fullpath.  We would want to know if the longest prefix match is
    // actually ourself (which indicates an error since that fullpath should
    // not have been sent here in the first place.)
    ret = sysdir_loadNamespacesFromString(strDB, &rbtNamespaces, &rbtMdmOwners);

    // Record my own component name.
    if (compName != NULL)
    {
        strncpy(myCompName, compName, sizeof(myCompName) - 1);
    }

    return ret;
}

BcmRet sysdir_ns_close()
{
    traverse_postorder(&rbtNamespaces, _rbtree_free_node, NULL);
    traverse_postorder(&rbtMdmOwners, _rbtree_free_node, NULL);
    return BCMRET_SUCCESS;
}

SysdirNsNode *sysdir_ns_lookup(const char *fullpath)
{
    SysdirNsNode *result = NULL;

    if (fullpath == NULL)
    {
        bcmuLog_error("fullpath is NULL!");
        return NULL;
    }
    else if (fullpath[0] == '\0')
    {
        bcmuLog_error("fullpath is empty string!");
        return NULL;
    }

    bcmuLog_notice("Entered: fullpath=%s", fullpath);

    if (sysdir_ns_isMultiComponentPath(fullpath))
    {
        // This is a relatively rare case, happens when caller wants to do a
        // walk over a top level obj such as Device. or Device.QoS.
        result = getMultiComponentsByPrefix(fullpath);
    }
    else
    {
        // General longest prefix match, includes instanceId range and alias
        // hint checking.
        result = getSingleCompByLongestPrefix(fullpath);
    }

    return result;
}

static void inner_copy_componentName(const char *compName, void *data)
{
    SysdirCompNode **cList = (SysdirCompNode **) data;  // the result list
    SysdirCompNode *p;

    p = cmsMem_alloc(sizeof(SysdirCompNode), ALLOC_ZEROIZE);
    if (p == NULL)
    {
        bcmuLog_error("Could not allocate a SysdirCompNode");
        return;
    }
    strncpy(p->compName, compName, sizeof(p->compName)-1);

    // put new node at head of the list.
    p->next = *cList;
    *cList = p;

    return;
}

SysdirCompNode *sysdir_ns_getAllRemoteComponents()
{
    SysdirCompNode *cList = NULL;
    SysDirNsInnerOperation iop;

    iop.inner_func = inner_copy_componentName;
    iop.args = &cList;

    sysdir_ns_foreachMdmOwner(&iop);
    return cList;
}

void sysdir_ns_freeCompNodes(SysdirCompNode **head)
{
    SysdirCompNode *node;
    SysdirCompNode *tmp;

    if (head == NULL)
       return;

    node = *head;
    while (node != NULL)
    {
        tmp = node->next;
        cmsMem_free(node);
        node = tmp;
    }

    *head = NULL;
    return;
}

void sysdir_ns_freeNsNodes(SysdirNsNode **head)
{
    SysdirNsNode *node;
    SysdirNsNode *tmp;

    if (head == NULL)
       return;

    node = *head;
    while (node != NULL)
    {
        tmp = node->next;
        cmsMem_free(node);
        node = tmp;
    }

    *head = NULL;
    return;
}

UINT32 sysdir_ns_numNsNode(const SysdirNsNode *head)
{
    const SysdirNsNode *p = head;
    UINT32 count = 0;
    while (p != NULL)
    {
        count++;
        p = p->next;
    }
    return count;
}

static void _rbtree_foreach_Namespace(rbnode_t *rbnode, void *arg)
{
    SysDirNamespaceNode *namespaceNode = (SysDirNamespaceNode *) rbnode;
    SysDirNsInnerOperation *iop = (SysDirNsInnerOperation *) arg;
    nameSpaces_foreach_func inner_func;

    inner_func = (nameSpaces_foreach_func) iop->inner_func;

    // filter out myCompName
    if (0 != strcmp(namespaceNode->nsData.ownerCompName, myCompName))
    {
       bcmuLog_debug("[%s]:%s",
                     namespaceNode->nsData.ownerCompName,
                     namespaceNode->nsData.namespc);
       inner_func(namespaceNode, iop->args);
    }
}

static void sysdir_ns_foreachNamespace(SysDirNsInnerOperation *iop)
{
    traverse_inorder(&rbtNamespaces, _rbtree_foreach_Namespace, (void *) iop);
}

static void _rbtree_foreach_MdmOwner(rbnode_t *rbnode, void *arg)
{
    SysDirMdmOwnerNode *node = (SysDirMdmOwnerNode *) rbnode;
    SysDirNsInnerOperation *iop = (SysDirNsInnerOperation *) arg;
    mdmOwners_foreach_func inner_func;

    inner_func = (mdmOwners_foreach_func) iop->inner_func;

    // filter out myCompName
    if (0 != strcmp(node->ownerCompName, myCompName))
    {
        bcmuLog_debug("[%s]", node->ownerCompName);
        inner_func(node->ownerCompName, iop->args);
    }
}

static void sysdir_ns_foreachMdmOwner(SysDirNsInnerOperation *iop)
{
    traverse_inorder(&rbtMdmOwners, _rbtree_foreach_MdmOwner, (void *) iop);
}


UBOOL8 sysdir_ns_compExist(const char *compName)
{
    SysDirMdmOwnerNode *ownerNode;

    if (IS_EMPTY_STRING(compName))
       return FALSE;

    ownerNode = (SysDirMdmOwnerNode *) rbtree_search(&rbtMdmOwners, compName);
    // filter out myCompName for consistency with other APIs
    if ((ownerNode != NULL) &&
        (0 != strcmp(ownerNode->ownerCompName, myCompName)))
    {
       return TRUE;
    }
    else
    {
       return FALSE;
    }
}

const char *sysdir_ns_lookupNextRange(const char *fullpath,
                                      UINT32 *firstInstanceId,
                                      UINT32 *lastInstanceId)
{
    MatchRangeData mr;
    SysDirNsInnerOperation iop;
    InstanceIdStack iidStack;
    char *genericPath = NULL;
    CmsRet ret;

    ret = cmsUtl_parseFullpathToGeneric(fullpath, &genericPath, &iidStack);
    if (ret != CMSRET_SUCCESS)
    {
        bcmuLog_error("Failed to parse %s", fullpath);
        return NULL;
    }

    cmsMem_free(genericPath);

    // Set up the args for the walk over all namespaces
    memset((void *) &mr, 0, sizeof(mr));
    memset((void *) &iop, 0, sizeof(iop));
    mr.reqFullpath = fullpath;
    if (iidStack.currentDepth > 0)
    {
        // For now always use the last instanceId in the stack.
        mr.instanceId = iidStack.instance[iidStack.currentDepth - 1];
    }
    mr.firstInstanceId = mr.instanceId;
    iop.inner_func = inner_lookup_next_namespace_with_range;
    iop.args = (void *)&mr;

    // do the match
    sysdir_ns_foreachNamespace(&iop);
    *firstInstanceId = mr.firstInstanceId;
    *lastInstanceId = mr.lastInstanceId;

    return mr.compName;
}
