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

/*!\file robj.c
 * \brief This file implements the utilities to assemble mdm object data from
 * an array of the object's parameter values. We will need remote object's
 * MdmObjectNode information to do it properly.
 */

#include <stdio.h>
#include <errno.h>

#include "bcm_retcodes.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "mdm_types.h"
#include "mdm.h"
#include "cms_util.h"
#include "bcm_generic_hal_defs.h"

void* robj_assemble(MdmObjectId oid, const BcmGenericParamInfo *parray, SINT32 numParams)
{
    void *mdmObj;
    SINT32 i, j, nextIdx=0;
    UINT32 objSize;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    const char *paramName = NULL;
    const BcmGenericParamInfo *param;
    UBOOL8 found;
    CmsRet ret = CMSRET_SUCCESS;

    objNode = mdm_getObjectNodeFlags(oid, GET_OBJNODE_REMOTE, NULL);
    if (objNode == NULL)
    {
        cmsLog_error("Failed to get objectNode for oid %d", oid);
        return NULL;
    }
    else
    {
        cmsLog_debug("Got MdmObjectNode for oid %d", oid);
    }

    objSize = mdm_getObjectSize(objNode);
    mdmObj = cmsMem_alloc(objSize, mdmLibCtx.allocFlags);
    if (mdmObj == NULL)
    {
        cmsLog_error("malloc of %d bytes for oid %d failed", objSize, objNode->oid);
        return NULL;
    }

   /*
    * Set the oid in the object itself so we will know which object
    * it is when it comes time to free (or dup) it.
    */
    *((MdmObjectId *) mdmObj) = objNode->oid;

   /*
    * Traverse through the parray and fill in the values to the mdmObj.
    * We do not assume the params in the parray is in the same order as mdmObj,
    * and we ignore any params in parray that does not have a field in mdmObj.
    * This can happen with third party remote components (openwrt libbbfdm)
    * which has a slightly different data model than us.
    */
    for (j=0; j < numParams && ret == CMSRET_SUCCESS; j++)
    {
        param = &parray[j];
        paramName = strrchr(param->fullpath, '.');  // find paramName in fullpath
        if (paramName != NULL)
        {
           paramName++;  // move past the dot, now paramName points to just the name
        }
        else
        {
           cmsLog_error("Could not find . in %s", param->fullpath);
           ret = CMSRET_INTERNAL_ERROR;
           break;
        }
        cmsLog_debug("[%d] matching %s (%s)", j, param->fullpath, paramName);

        // In most cases, the order of the paramNames in parray matches the
        // order of the params in the mdmObj, so try to find a match that way.
        found = FALSE;
        paramNode = &(objNode->params[nextIdx]);
        if (!cmsUtl_strcmp(paramNode->name, paramName))
        {
            found = TRUE;
            nextIdx++;
        }
        if (!found)
        {
            // Our heuristic above did not produce a match, so search all the
            // params in the MDM object.
            for (i = 0; i < objNode->numParamNodes; i++)
            {
                paramNode = &(objNode->params[i]);
                if (!cmsUtl_strcmp(paramNode->name, paramName))
                {
                    found = TRUE;
                    nextIdx = i + 1;
                    break;
                }
            }
        }

        if (found)
        {
            cmsLog_debug("[%d] setting paramName=%s value=%s (at obj idx=%d)",
                         j, param->fullpath, param->value, nextIdx-1);
            ret = mdm_setParamNodeString(paramNode, param->value,
                                         mdmLibCtx.allocFlags, mdmObj);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to set parameter %s value %s",
                             param->fullpath, param->value);
            }
        }
        else
        {
            // This is ok, we will not stick the value into our mdmObj
            // because there is no field in our mdmObj for this param.
            // If code really wants to get the value, they need to use the
            // bcm_generic_ API.
            cmsLog_debug("Ignoring unknown param name %s", param->fullpath);
        }

        // if nextIdx goes past the last param of the obj, just set it back
        // to 0.  It will probably be wrong, but we will search through the
        // whole object again for the next match point.
        if (nextIdx >= objNode->numParamNodes)
        {
            nextIdx = 0;
        }
    }

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to assemble object! ret=%d", ret);
        mdm_freeObject(&mdmObj);
        return NULL;
    }

    return (void *) mdmObj;
}

//dissemble data in mdm object into an array of parameters.
BcmGenericParamInfo* robj_dissemble(const void *mdmObj, const InstanceIdStack *iidStack,
                                    UBOOL8 filterReadonly, SINT32 *numParams)
{
    SINT32 i, j;
    MdmObjectId oid;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    BcmGenericParamInfo *result;
    const char *paramType;
    char *paramValue=NULL;
    size_t data_len;
    CmsRet cret;

    oid = *((MdmObjectId *) mdmObj);
    objNode = mdm_getObjectNodeFlags(oid, GET_OBJNODE_REMOTE, NULL);
    if (objNode == NULL)
    {
        cmsLog_error("Failed to get objectNode for oid %d", oid);
        return NULL;
    }
    else
    {
        cmsLog_debug("Got MdmObjectNode for oid %d", oid);
    }

    data_len = objNode->numParamNodes * sizeof(BcmGenericParamInfo);
    result = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
    if (result == NULL)
    {
        cmsLog_error("Failed to create result buf!");
        return NULL;
    }

    j = 0;
    for (i = 0; i < objNode->numParamNodes; i++)
    {
        paramNode = &(objNode->params[i]);
        UINT32 mdmFlags = ALLOC_ZEROIZE | OGF_OMIT_NULL_VALUES;

        if (filterReadonly == TRUE)
        {
            /* If we have a READ-ONLY parameter in the object to be set, omit
             * it from the result param list.  Otherwise, the remote side
             * will think we want to set this param and reject it since this
             * is a READ-ONLY param.  (However, this means we will silently
             * ignore an illegal attempt to set a read-only param.)
             */
            if (!IS_PARAM_WRITABLE(paramNode))
            {
                continue;
            }
        }

        paramType = cmsMdm_paramTypeToString(paramNode->type);
        cret = mdm_getParamNodeString(paramNode, mdmObj, mdmFlags, &paramValue);
        if (cret == CMSRET_SUCCESS)
        {
            MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
            char *fullPath = NULL;

            if (paramValue == NULL)
            {
               /* if the param value is NULL, omit it from the result
                * param list.  Otherwise, we will set the param as an empty
                * string on the remote side, which is not correct.  (However,
                * this means we cannot set a parameter back to NULL in this
                * path.)
                */
               continue;
            }

            pathDesc.oid = oid;
            memcpy(&pathDesc.iidStack, iidStack, sizeof(InstanceIdStack));
            cmsUtl_strncpy(pathDesc.paramName, paramNode->name, sizeof(pathDesc.paramName));
            cret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
            if (cret == CMSRET_SUCCESS)
            {
                UBOOL8 writable;
                UBOOL8 isPassword;
                BcmGenericParamInfo *p = &result[j++];

                memset(p, 0, sizeof(BcmGenericParamInfo));
                writable =((paramNode->flags & PRN_WRITABLE) == PRN_WRITABLE);
                isPassword = ((paramNode->flags & PRN_TR69_PASSWORD) == PRN_TR69_PASSWORD);
                p->fullpath = fullPath;
                p->type = cmsMem_strdup(paramType);
                p->value = paramValue;
                p->profile = cmsMem_strdup(paramNode->profile);
                p->writable = writable;
                p->isPassword = isPassword;
                cmsLog_debug("[%d]: fullPath=%s, paramType=%s, paramValue=%s profile=%s writable=%d isPassword=%d",
                              i, fullPath, paramType, paramValue, paramNode->profile, writable, isPassword);
            }
            else
            {
                cmsLog_error("Failed to convert path descriptor to fullPath!");
                CMSMEM_FREE_BUF_AND_NULL_PTR(paramValue);
            }
        }
        else
        {
            cmsLog_error("Failed to get param node value in string form!");
        }
    }

    *numParams = j;
    return result;
}
