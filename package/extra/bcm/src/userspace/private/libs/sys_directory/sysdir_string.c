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


/*!\file sysdir_string.c
 * \stringingfy rbtree database and vice versa.
 *
 */

#include <errno.h>
#include <string.h>
#include "number_defs.h"
#include "os_defs.h"
#include "cms_util.h"  // for cmsMem_alloc
#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "genutil_rbtree.h"
#include "json_tokener.h"
#include "sys_directory.h"

static void _rbtree_foreach_Namespace(rbnode_t *rbnode, void *arg)
{
    SysDirNamespaceNode *node = (SysDirNamespaceNode *) rbnode;
    json_object *jNamespaces, *jObj, *jval;

    bcmuLog_debug("[%s]: %s", node->nsData.ownerCompName, node->nsData.namespc);
    jNamespaces = (json_object *) arg;

    jObj = json_object_new_object();
    if (jObj == NULL)
    {
        bcmuLog_error("Failed to create new json object!");
        return;
    }

    jval = json_object_new_string(node->nsData.ownerCompName);
    json_object_object_add(jObj, "CompName", jval);

    jval = json_object_new_string(node->nsData.namespc);
    json_object_object_add(jObj, "Namespace", jval);

    json_object_array_add(jNamespaces, jObj);
}


static void _rbtree_foreach_MdmOwner(rbnode_t *rbnode, void *arg)
{
    SysDirMdmOwnerNode *node = (SysDirMdmOwnerNode *) rbnode;
    json_object *jMdmOwners, *jval;

    bcmuLog_debug("[%s]", node->ownerCompName);
    jMdmOwners = (json_object *) arg;
    jval = json_object_new_string(node->ownerCompName);
    json_object_array_add(jMdmOwners, jval);
}

/* This function return a JSON string which has below format:
 * 1. An array of exisisting namespace-to-owner mappings.
 * 2. An array of distributed MDM owners.
 * Caller is responsible for freeing the list.
 */
char *sysdir_stringifyNamespaces(rbtree_t *rbtNamespaces,
                                 rbtree_t *rbtMdmOwners)
{
    const char *t;
    char *strOut;
    int ret;
    json_object *jOut, *jNamespaces, *jMdmOwners;

    jOut = json_object_new_object();
    if (jOut == NULL)
    {
        bcmuLog_error("Failed to create new json object!");
        return NULL;
    }

    jNamespaces = json_object_new_array();
    if (jNamespaces == NULL)
    {
        json_object_put(jOut);
        bcmuLog_error("Failed to create new json object!");
        return NULL;
    }

    traverse_postorder(rbtNamespaces, _rbtree_foreach_Namespace, (void *)jNamespaces);
    ret = json_object_object_add(jOut, "Namespaces", jNamespaces);
    if (ret != 0)
    {
        bcmuLog_error("Failed to add jNamespaces to jOut");
        json_object_put(jNamespaces);
        json_object_put(jOut);
        return NULL;
    }

    jMdmOwners = json_object_new_array();
    if (jMdmOwners == NULL)
    {
        json_object_put(jOut);
        bcmuLog_error("Failed to create new json object!");
        return NULL;
    }

    traverse_postorder(rbtMdmOwners, _rbtree_foreach_MdmOwner, (void *)jMdmOwners);

    ret = json_object_object_add(jOut, "MdmOwners", jMdmOwners);
    if (ret != 0)
    {
        bcmuLog_error("Failed to add jMdmOwners to jOut");
        json_object_put(jMdmOwners);
        json_object_put(jOut);
        return NULL;
    }

    t = json_object_to_json_string(jOut);
    if (t != NULL)
    {
        strOut = cmsMem_strdup(t);
    }
    else
    {
        bcmuLog_error("Failed to stringify!");
        strOut = NULL;
    }

    json_object_put(jOut);
    bcmuLog_debug("strOut=%s", strOut);
    return strOut;
}

static BcmRet _load_namespaces_from_json(rbtree_t *rbtNamespaces,
                                         const json_object *jNamespaces)
{
    SysDirNamespaceNode *node;
    json_object *jObj, *jVal;
    unsigned int i;
    const char *s;

    for (i = 0; i < json_object_array_length(jNamespaces); i++)
    {
        jObj = json_object_array_get_idx(jNamespaces, i);
        if (jObj != NULL)
        {
            char *namespace, *compName;

            if (json_object_object_get_ex(jObj, "Namespace", &jVal) == TRUE)
            {
                s = json_object_get_string(jVal);
                namespace = cmsMem_strdup(s);
            }
            else
            {
                bcmuLog_error("Failed to get Namespace");
                continue;
            }

            if (json_object_object_get_ex(jObj, "CompName", &jVal) == TRUE)
            {
                s = json_object_get_string(jVal);
                compName = cmsMem_strdup(s);
            }
            else
            {
                bcmuLog_error("Failed to get MdmOwner");
                cmsMem_free(namespace);
                continue;
            }

            node = sysdir_newNamespaceNode(namespace, compName);
            cmsMem_free(namespace);
            cmsMem_free(compName);
            if (node == NULL)
            {
                return BCMRET_RESOURCE_EXCEEDED;
            }
            rbtree_insert(rbtNamespaces, (rbnode_t *)node);
        }
        else
        {
            bcmuLog_error("Unexpected empty jObj at %d!", i);
        }
    }
    return BCMRET_SUCCESS;
}

static BcmRet _load_mdmOwners_from_json(rbtree_t *rbtMdmOwners,
                                        const json_object *jMdmOwners)
{
    SysDirMdmOwnerNode *node;
    unsigned int i;

    for (i = 0; i < json_object_array_length(jMdmOwners); i++)
    {
        json_object *jObj;
        const char *s;

        jObj = json_object_array_get_idx(jMdmOwners, i);
        if (jObj != NULL)
        {
            s = json_object_get_string(jObj);
            if (s == NULL)
            {
                bcmuLog_error("Failed to retrieve compName!");
                return BCMRET_INTERNAL_ERROR;
            }
            bcmuLog_debug("[%d] adding compName %s", i, s);
            node = sysdir_newMdmOwnerNode(s);
            if (node == NULL)
            {
                return BCMRET_RESOURCE_EXCEEDED;
            }
            rbtree_insert(rbtMdmOwners, (rbnode_t *)node);
        }
        else
        {
            bcmuLog_error("Unexpected empty jObj at %d!", i);
        }
    }
    return BCMRET_SUCCESS;
}

BcmRet sysdir_loadNamespacesFromString(const char *strDB,
                                       rbtree_t *rbtNamespaces,
                                       rbtree_t *rbtMdmOwners)
{
    json_object *jContainer, *jNamespaces, *jMdmOwners;

    jContainer = json_tokener_parse(strDB);
    if (NULL == jContainer)
    {
        bcmuLog_error("Failed to parse %s into json object!", strDB);
        return BCMRET_INTERNAL_ERROR;
    }

    if (json_object_object_get_ex(jContainer, "Namespaces", &jNamespaces))
    {
        _load_namespaces_from_json(rbtNamespaces, jNamespaces);
    }

    if (json_object_object_get_ex(jContainer, "MdmOwners", &jMdmOwners))
    {
        _load_mdmOwners_from_json(rbtMdmOwners, jMdmOwners);
    }

    json_object_put(jContainer);
    return BCMRET_SUCCESS;
}
