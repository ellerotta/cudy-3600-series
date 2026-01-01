/***********************************************************************
 *
 *  Copyright (c) 2019  Broadcom Corporation
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

#include "cms.h"
#include "cms_msg.h"
#include "cms_obj.h"
#include "cms_msg_remoteobj.h"
#include "cms_util.h"
#include "mdm.h"
#include "remote.h"
#include "robj.h"
#include "bcm_strutils.h"
#include "bcm_generic_hal_utils.h"

static void deserializeParamInfoArray(CmsVbuf *vbuf,
                                      BcmGenericParamInfo *parray,
                                      UINT32 numParams)
{
    UINT32 i;

    if (vbuf == NULL) return;

    for (i = 0; i < numParams; i++)
    {
        cmsVbuf_getString(vbuf, &parray[i].fullpath);
        cmsVbuf_getString(vbuf, &parray[i].type);
        cmsVbuf_getString(vbuf, &parray[i].value);
        cmsVbuf_getString(vbuf, &parray[i].profile);
        cmsVbuf_getUBOOL8(vbuf, &parray[i].writable);
        cmsVbuf_getUBOOL8(vbuf, &parray[i].isPassword);
    }
    return;
}

static void deserializeParamAttrArray(CmsVbuf *vbuf,
                                      BcmGenericParamAttr *parray,
                                      UINT32 numParams)
{
    UINT32 i;

    if (vbuf == NULL) return;

    for (i = 0; i < numParams; i++)
    {
        cmsVbuf_getString(vbuf, &(parray[i].fullpath));
        // These fields are relevant during a getParamAttr operation, but
        // the alt notification ones are not implemented yet.
        cmsVbuf_getUINT16(vbuf, &(parray[i].access));
        cmsVbuf_getUINT16(vbuf, &(parray[i].notif));
        cmsVbuf_getUINT16(vbuf, &(parray[i].valueChanged));
        cmsVbuf_getUINT16(vbuf, &(parray[i].altNotif));
        cmsVbuf_getUINT16(vbuf, &(parray[i].altNotifValue));

        cmsLog_notice("[%d]%s: access: 0x%x, notif: 0x%x valueChanged: 0x%x",
                      i, parray[i].fullpath,
                      parray[i].access, parray[i].notif, parray[i].valueChanged);
    }
    return;
}

/** Utility function used by all other functions to send a message to remote_objd
 *  and get the response.
 */
static CmsRet remote_sendAndGetReply(CmsMsgHeader *request, CmsMsgHeader **response)
{
    static UINT16 sequenceNumber = 0;  // ensure msgs between MDM and remote_objd are in expected order.
    void *msgHandle = cmsMdm_getThreadMsgHandle();
    UINT32 timeoutMs = 35000;   // UBus timeout is 20 seconds, so make this longer than that.  (But DBus timeout is infinity)
    SINT32 tries;
    UBOOL8 doGetReply = TRUE;
    CmsRet ret = CMSRET_INTERNAL_ERROR;

    if ((request == NULL) || (response == NULL))
    {
        cmsLog_error("NULL input params %p/%p", request, response);
        return CMSRET_INTERNAL_ERROR;
    }
    if (*response != NULL)
    {
        cmsLog_error("response is pointing to an existing msg buffer (%p)!", *response);
        return CMSRET_INTERNAL_ERROR;
    }

    request->src = cmsMsg_getHandleEid(msgHandle);
    request->dst = EID_REMOTE_OBJD;
    request->flags_request = 1;
    // causes CMS_MSG_BOUNCED to be returned in wordData if remote_objd is not running.
    // TODO: actually, only sysmgmt (where smd is the message router) supports the bounceIfNotRunning flag.
    // In tr69 and usp, the only other components that have remote_objd, bcm_msgd will
    // forward this msg to the bus manager (tr69_md and usp_md), which will print an error message and
    // drop the message.  This is unlikely to happen in tr69 and usp components though.
    // But still, should make the behavior uniform.
    request->flags_bounceIfNotRunning = 1;


    // The goal of this block is to make sure the msg is actually sent to remote_objd.
    // Unfortunately, in order to ensure that, we have to call cmsMsg_getReplyBufWithTimeout.
    tries = 4;
    while (tries-- > 0)
    {
        request->sequenceNumber = sequenceNumber++;
        ret = cmsMsg_send(msgHandle, request);
        if (ret != CMSRET_SUCCESS)
        {
            // the send should always succeed.  If it fails, something is very
            // wrong (or system is shutting down).  Return immediately.
            return ret;
        }

        // On slow systems, in sysmgmt, remote_objd may not be started yet,
        // so we might get a msg bounced response from smd.
       ret = cmsMsg_getReplyBufWithTimeout(msgHandle,
                                           request, response, timeoutMs);
       if (ret == CMSRET_SUCCESS)
       {
            if ((*response)->wordData == CMSRET_MSG_BOUNCED)
            {
                // We should not get a sequence number mismatch so early on,
                // just check and log error if it happens.
                if (request->sequenceNumber != (*response)->sequenceNumber)
                {
                    cmsLog_error("seq number mismatch! got=0x%x:%d expected=0x%x:%d",
                                 (*response)->type, (*response)->sequenceNumber,
                                 request->type, request->sequenceNumber);
                    // fall through.
                }
                CMSMEM_FREE_BUF_AND_NULL_PTR(*response);
                if (tries <= 0)
                {
                    cmsLog_error("remote_objd still not running.  Give up!");
                    // Consistent with error path below, we return error but make
                    // the caller free the response msg.
                    return CMSRET_MSG_BOUNCED;
                }
                else
                {
                    cmsLog_notice("remote_objd not running yet.  wait and try again (tries=%d)",
                                 tries);
                    sleep(1);
                    continue;
                }
            }
            else
            {
                // remote_objd is running, and gave us a real response,
                // process it below.  This is the most common case.
                doGetReply = FALSE;
                break;
            }
        }
        else
        {
            // got some error, process it below.
            doGetReply = FALSE;
            break;
        }
    }

    if (doGetReply)
    {
         cmsLog_error("ASSERT failure: doGetReply should be FALSE here!");
         // fall through.
    }

    tries = 2;
    while (tries-- > 0)
    {
       if (doGetReply)
       {
           ret = cmsMsg_getReplyBufWithTimeout(msgHandle,
                                               request, response, timeoutMs);
       }
       else
       {
           // The first time through this block, doGetReply should be FALSE.
           // But if we come back to the top of the block again, we want to
           // call cmsMsg_getReplyBufWithTimeout.
           doGetReply = TRUE;
       }

       if (ret != CMSRET_SUCCESS)
       {
           // For some strange reason, getting a CMS_MSG_TERMINATE response msg
           // from remote_objd sometimes fails.  Ignore it since we are
           // shutting down anyways.
          if (request->type == CMS_MSG_TERMINATE)
          {
              cmsLog_notice("get TERMINATE reply from remote_objd failed, ret=%d, ignore it", ret);
              ret = CMSRET_SUCCESS;
              break;
          }
          else
          {
              cmsLog_error("getReplyBuf from remote_objd failed, ret=%d msg=0x%x seq=%d tries=%d",
                           ret, request->type, request->sequenceNumber, tries);
          }
          continue;
       }
       else
       {
           if (request->sequenceNumber == (*response)->sequenceNumber)
           {
               // Sequence number matched, so this is the correct response.
               // Note: this function might return an error code, but
               // response buffer still needs to be freed by the caller.
               ret = (*response)->wordData;
               break;
           }
           else
           {
               cmsLog_notice("DROP reply with wrong sequenceNumber (got=0x%x:%d expected=0x%x:%d, tries=%d)",
                             (*response)->type, (*response)->sequenceNumber,
                             request->type, request->sequenceNumber,
                             tries);
               // msgs between MDM and remote_objd are out of order, extend
               // number of tries to hopefully get the right one.
               tries = 2;

               CMSMEM_FREE_BUF_AND_NULL_PTR(*response);
               continue;
           }
       }
    }

    cmsLog_debug("Exit: ret=%d tries=%d", ret, tries);
    return ret;
}

/* Get remote object from remote_objd. */
CmsRet remote_getObject(MdmObjectId oid,
                        const InstanceIdStack *iidStack,
                        UINT32 getFlags, void** mdmObj)
{
    CmsRet ret = CMSRET_SUCCESS;
    char *fullpath = NULL;
    MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
    char *remoteFullpathArray[1];
    BcmGenericParamInfo *pRemoteList = NULL;
    SINT32 numRemoteEntries;

    // Convert oid+iidStack to fullpath
    pathDesc.oid = oid;
    memcpy(&pathDesc.iidStack, iidStack, sizeof(InstanceIdStack));
    ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullpath);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsMdm_pathDescToFullPath failed, ret=%d", ret);
    }
    cmsLog_debug("calculated fullpath: %s", fullpath);
    remoteFullpathArray[0] = fullpath;
    getFlags = getFlags | OGF_OMIT_NULL_VALUES;
    ret = remote_getParamValues((const char **)&remoteFullpathArray, 1,
                                TRUE, getFlags,
                                &pRemoteList, &numRemoteEntries);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get remote parameters! oid=%d numRemote=%d, ret=%d",
                     oid, numRemoteEntries, ret);
    }
    else
    {
        *mdmObj = robj_assemble(oid, pRemoteList, numRemoteEntries); 
        ret = (*mdmObj == NULL) ? CMSRET_INTERNAL_ERROR : CMSRET_SUCCESS;
    }
    cmsMem_free(fullpath);
    cmsUtl_freeParamInfoArray(&pRemoteList, numRemoteEntries);
    return ret;
}

/* Get the next remote object from remote_objd. */
CmsRet remote_getNextObject(MdmObjectId oid,
                            const InstanceIdStack *parentIidStack,
                            InstanceIdStack *iidStack,
                            UINT32 getFlags, void** mdmObj)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjGetNextRequest)] = {0};
    const char *genericPath;
    const MdmObjectNode *objNode;
    CmsMsgHeader *request, *response = NULL;
    RemoteObjGetNextRequest *req_data;

    objNode = mdm_getObjectNodeFlags(oid, GET_OBJNODE_REMOTE, NULL);
    if (objNode == NULL)
    {
        cmsLog_error("Failed to get objectNode for oid %d", oid);
        return CMSRET_INTERNAL_ERROR;
    }

    genericPath = mdm_oidToGenericPath(oid);
    if (genericPath == NULL)
    {
        cmsLog_error("Failed to get generic path! oid=%d", oid);
        return CMSRET_INVALID_ARGUMENTS;
    }

    if ((iidStack->currentDepth > 0) && (iidStack->currentDepth != objNode->instanceDepth))
    {
        cmsLog_error("iidStack has incorrect depth! currentDepth=%d, expected %d", 
                      iidStack->currentDepth, objNode->instanceDepth);
        return CMSRET_INVALID_ARGUMENTS;
    }

    request = (CmsMsgHeader *)&requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_GETNEXT;
    request->dataLength = sizeof(RemoteObjGetNextRequest);
    req_data = (RemoteObjGetNextRequest *)(request + 1);
    strncpy(req_data->genericPath, genericPath, sizeof(req_data->genericPath)-1);
    req_data->genericPath[sizeof(req_data->genericPath)-1] = '\0'; 
    req_data->isMultiCompGetNext = mdm_isMultiCompGetNextOid(oid);
    req_data->parentIidStack = *parentIidStack;
    req_data->iidStack = *iidStack;
    req_data->flags = getFlags | OGF_OMIT_NULL_VALUES;
    ret = remote_sendAndGetReply(request, &response);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("remote_objd returned %d", ret);
    }
    else
    {
        RemoteObjGetNextResponse *res_data;
        BcmGenericParamInfo *pParamInfoArray = NULL;
        MdmPathDescriptor pathDesc = {0};
        size_t data_len;
        CmsVbuf *vbuf;

        /* The message body contains a pointer value for the mdm object allocated by remote_objd.
         * As this object is allocated in the same shared memory, which is attached to the
         * same virtual address in the calling process, we can just reuse the pointer in
         * the calling process.
         */
        cmsLog_notice("remote_objd returned the next object, deserializing and assembling...");
        res_data = (RemoteObjGetNextResponse *)(response + 1);
        ret = cmsMdm_fullPathToPathDescriptor(res_data->nextFullpath, &pathDesc);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", res_data->nextFullpath, ret);
        }
        memcpy(iidStack, &pathDesc.iidStack, sizeof(InstanceIdStack));
        vbuf = cmsVbuf_new();
        if (vbuf == NULL)
        {
            cmsLog_error("Failed to create vbuf!");
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }
        // copy the data from the response msg to our local vbuf.
        ret = cmsVbuf_put(vbuf, (void *)(res_data + 1), res_data->size);
        if (ret != CMSRET_SUCCESS)
        {
            cmsVbuf_destroy(vbuf);
            goto exit;
        }

        // allocate buffer for the result, returned to caller.
        data_len = res_data->numEntries * sizeof(BcmGenericParamInfo);
        pParamInfoArray = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (pParamInfoArray == NULL)
        {
            cmsVbuf_destroy(vbuf);
            ret = CMSRET_RESOURCE_EXCEEDED;
            goto exit;
        }

        deserializeParamInfoArray(vbuf, pParamInfoArray, res_data->numEntries);
        *mdmObj = robj_assemble(oid, pParamInfoArray, res_data->numEntries);
        cmsUtl_freeParamInfoArray(&pParamInfoArray, res_data->numEntries);
        cmsVbuf_destroy(vbuf);
    }

exit:
    CMSMEM_FREE_BUF_AND_NULL_PTR(response);
    return ret;
}

CmsRet remote_getAncestorObject(MdmObjectId ancestorOid,
                                MdmObjectId decendentOid,
                                InstanceIdStack *iidStack,
                                UINT32 getFlags,
                                void **mdmObj)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjGetAncestorRequest)] = {0};
    CmsMsgHeader *request, *response = NULL;
    RemoteObjGetAncestorRequest *req_data;
    const char *decendentGenericPath = NULL;
    const char *ancestorGenericPath = NULL;

    decendentGenericPath = mdm_oidToGenericPath(decendentOid);
    if (decendentGenericPath == NULL)
    {
        cmsLog_error("failed to get generic path for oid %d", decendentOid);
        return CMSRET_INTERNAL_ERROR;
    }

    ancestorGenericPath = mdm_oidToGenericPath(ancestorOid);
    if (ancestorGenericPath == NULL)
    {
        cmsLog_error("failed to get generic path for oid %d", ancestorOid);
        return CMSRET_INTERNAL_ERROR;
    }

    request = (CmsMsgHeader *)&requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_GETANCESTOR;
    request->dataLength = sizeof(RemoteObjGetAncestorRequest);
    req_data = (RemoteObjGetAncestorRequest *)(request + 1);
    cmsUtl_strncpy(req_data->ancestorGenericPath,
                   ancestorGenericPath,
                   sizeof(req_data->ancestorGenericPath));
    cmsUtl_strncpy(req_data->decendentGenericPath,
                   decendentGenericPath,
                   sizeof(req_data->decendentGenericPath));
    memcpy(&req_data->iidStack, iidStack, sizeof(InstanceIdStack));
    req_data->flags = getFlags;
    ret = remote_sendAndGetReply(request, &response);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("remote_objd returned %d", ret);
    }
    else
    {
        RemoteObjGetAncestorResponse *res_data;
        CmsVbuf *vbuf;
        size_t data_len;
        BcmGenericParamInfo *pParamInfoArray;
        /* The message body contains a pointer value for the mdm object allocated by remote_objd.
         * As this object is allocated in the same shared memory, which is attached to the
         * same virtual address in the calling process, we can just reuse the pointer in
         * the calling process.
         */
        cmsLog_notice("remote_objd returned the ancestor object, deserializing and assembling...");
        res_data = (RemoteObjGetAncestorResponse *)(response + 1);
        memcpy(iidStack, &res_data->iidStack, sizeof(InstanceIdStack));
        
        vbuf = cmsVbuf_new();
        if (vbuf == NULL)
        {
            cmsLog_error("Failed to create vbuf!");
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }

        res_data = (RemoteObjGetAncestorResponse *)(response + 1);

        // copy the data from the response msg to our local vbuf.
        ret = cmsVbuf_put(vbuf, (void *)(res_data + 1), res_data->size);
        if (ret != CMSRET_SUCCESS)
        {
            cmsVbuf_destroy(vbuf);
            goto exit;
        }

        // allocate buffer for the result, returned to caller.
        data_len = res_data->numEntries * sizeof(BcmGenericParamInfo);
        pParamInfoArray = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (pParamInfoArray == NULL)
        {
            cmsVbuf_destroy(vbuf);
            ret = CMSRET_RESOURCE_EXCEEDED;
            goto exit;
        }

        deserializeParamInfoArray(vbuf, pParamInfoArray, res_data->numEntries);
        *mdmObj = robj_assemble(ancestorOid, pParamInfoArray, res_data->numEntries);
        cmsUtl_freeParamInfoArray(&pParamInfoArray, res_data->numEntries);
        cmsVbuf_destroy(vbuf);
    }

exit:
    CMSMEM_FREE_BUF_AND_NULL_PTR(response);
    return ret;
}

CmsRet remote_addObjectInstance(const char *fullpath,
                                UINT32 flags,
                                UINT32 *newInstanceId)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjAddInstanceRequest)] = {0};
    CmsMsgHeader *request = (CmsMsgHeader *)&requestBuf;
    CmsMsgHeader *response = NULL;
    RemoteObjAddInstanceRequest *reqBody = (RemoteObjAddInstanceRequest *)(request+1);

    if (IS_EMPTY_STRING(fullpath))
    {
        cmsLog_error("Invalid fullpath %s", fullpath);
        return CMSRET_INVALID_ARGUMENTS;
    }
    if (newInstanceId == NULL)
    {
        cmsLog_error("newInstanceId is NULL!");
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Fill out request header
    request->type = CMS_MSG_REMOTE_OBJ_ADD_INSTANCE;
    request->dataLength = sizeof(RemoteObjAddInstanceRequest);

    // Need to be careful of the fullpath because with aliases, the fullpath
    // can be longer than expected.  We could just get rid of this structure
    // and send the string right after the CmsMsgHeader.
    if (strlen(fullpath) >= sizeof(reqBody->fullpath))
    {
        cmsLog_error("fullpath %s too long, max %d",
                     fullpath, sizeof(reqBody->fullpath));
        return CMSRET_INVALID_ARGUMENTS;
    }
    cmsUtl_strncpy(reqBody->fullpath, fullpath, sizeof(reqBody->fullpath));
    reqBody->flags = flags;

    ret = remote_sendAndGetReply(request, &response);
    if (ret == CMSRET_SUCCESS)
    {
        RemoteObjAddInstanceResponse *respBody;
        respBody = (RemoteObjAddInstanceResponse *)(response + 1);
        *newInstanceId = respBody->newInstanceId;
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(response);
    return ret;
}

CmsRet remote_deleteObjectInstance(const char *fullpath, UINT32 flags)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjDelInstanceRequest)] = {0};
    CmsMsgHeader *request = (CmsMsgHeader *)&requestBuf;
    CmsMsgHeader *response = NULL;
    RemoteObjDelInstanceRequest *reqBody = (RemoteObjDelInstanceRequest *)(request+1);

    if (IS_EMPTY_STRING(fullpath))
    {
        cmsLog_error("Invalid fullpath %s", fullpath);
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Fill out request header
    request->type = CMS_MSG_REMOTE_OBJ_DEL_INSTANCE;
    request->dataLength = sizeof(RemoteObjDelInstanceRequest);

    // Need to be careful of the fullpath because with aliases, the fullpath
    // can be longer than expected.  We could just get rid of this structure
    // and send the string right after the CmsMsgHeader.
    if (strlen(fullpath) >= sizeof(reqBody->fullpath))
    {
        cmsLog_error("fullpath %s too long, max %d",
                     fullpath, sizeof(reqBody->fullpath));
        return CMSRET_INVALID_ARGUMENTS;
    }
    cmsUtl_strncpy(reqBody->fullpath, fullpath, sizeof(reqBody->fullpath));
    reqBody->flags = flags;

    ret = remote_sendAndGetReply(request, &response);
    CMSMEM_FREE_BUF_AND_NULL_PTR(response);

    return ret;
}

CmsRet remote_clearStatistics(const char *fullpath, UINT32 flags)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjClearStatisticsRequest)] = {0};
    CmsMsgHeader *request = (CmsMsgHeader *)&requestBuf;
    CmsMsgHeader *response = NULL;
    RemoteObjClearStatisticsRequest *reqBody = (RemoteObjClearStatisticsRequest *)(request+1);

    if (IS_EMPTY_STRING(fullpath))
    {
        cmsLog_error("Invalid fullpath %s", fullpath);
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Fill out request header
    request->type = CMS_MSG_REMOTE_OBJ_CLEAR_STATISTICS;
    request->dataLength = sizeof(RemoteObjClearStatisticsRequest);

    // Need to be careful of the fullpath because with aliases, the fullpath
    // can be longer than expected.  We could just get rid of this structure
    // and send the string right after the CmsMsgHeader.
    if (strlen(fullpath) >= sizeof(reqBody->fullpath))
    {
        cmsLog_error("fullpath %s too long, max %d",
                     fullpath, sizeof(reqBody->fullpath));
        return CMSRET_INVALID_ARGUMENTS;
    }
    cmsUtl_strncpy(reqBody->fullpath, fullpath, sizeof(reqBody->fullpath));
    reqBody->flags = flags;

    ret = remote_sendAndGetReply(request, &response);
    CMSMEM_FREE_BUF_AND_NULL_PTR(response);

    return ret;
}

/* Set remote object from remote_objd. */
CmsRet remote_setObject(const void *mdmObj,
                        const InstanceIdStack *iidStack,
                        UINT32 flags)
{
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 filterReadOnly;
    SINT32 numParams = 0;
    BcmGenericParamInfo *objParams = NULL;

    filterReadOnly = (flags & OSF_NO_ACCESSPERM_CHECK) ? FALSE : TRUE;
    objParams = robj_dissemble(mdmObj, iidStack, filterReadOnly, &numParams);
    if (objParams == NULL)
    {
        cmsLog_error("Failed to disemble mdmObj into parameters!");
        return CMSRET_INTERNAL_ERROR;
    }

    ret = remote_setParamValues(objParams, numParams, flags);
    cmsUtl_freeParamInfoArray(&objParams, numParams);
    return ret;
}

CmsRet remote_getParamAttributes(const char       **fullpathArray,
                                 SINT32             numEntries,
                                 UBOOL8             nextLevelOnly,
                                 UINT32             flags,
                                 BcmGenericParamAttr  **pParamAttrList,
                                 SINT32             *pNumParamAttrEntries)
{
    CmsRet ret = CMSRET_SUCCESS;
    size_t data_len, msg_len, serialized_len;
    SINT32 i;
    char *requestBuf;
    CmsMsgHeader *request, *response = NULL;
    RemoteObjGetParamAttributesRequest *req_data;
    CmsVbuf *vbuf;

    // Serialize the array of fullpaths into a vbuf
    vbuf = cmsVbuf_new();
    if (vbuf == NULL)
    {
        cmsLog_error("Failed to create input vbuf!");
        return CMSRET_INTERNAL_ERROR;
    }

    for (i=0; i < numEntries; i++)
    {
        cmsVbuf_putString(vbuf, fullpathArray[i]);
    }
    serialized_len = cmsVbuf_getSize(vbuf);

    data_len = sizeof(RemoteObjGetParamAttributesRequest) + serialized_len;

    // Allocate and fill out the request message
    msg_len = sizeof(CmsMsgHeader) + data_len;
    requestBuf = cmsMem_alloc(msg_len, ALLOC_ZEROIZE);
    if (requestBuf == NULL)
    {
        cmsVbuf_destroy(vbuf);
        return CMSRET_RESOURCE_EXCEEDED;
    }
    request = (CmsMsgHeader *)requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_GET_PARAMATTRS;
    request->dataLength = data_len;
    req_data = (RemoteObjGetParamAttributesRequest *)(request + 1);
    req_data->nextLevelOnly = nextLevelOnly;
    req_data->numEntries = numEntries;
    req_data->flags = flags;
    memcpy((char *)(req_data + 1), vbuf->data, serialized_len);

    /*
     * Send the request to remote_objd and wait for the reply.
     */
    ret = remote_sendAndGetReply(request, &response);

    // we are done with these, can free them now.
    cmsMem_free(requestBuf);
    cmsVbuf_destroy(vbuf);
    vbuf = NULL;

    if (ret == CMSRET_SUCCESS)
    {
        BcmGenericParamAttr *result;
        RemoteObjGetParamAttributesResponse *res_data;

        vbuf = cmsVbuf_new();
        if (vbuf == NULL)
        {
            cmsLog_error("Failed to create vbuf!");
            cmsMem_free(response);
            return CMSRET_INTERNAL_ERROR;
        }

        res_data = (RemoteObjGetParamAttributesResponse *)(response + 1);

        // copy the data from the response msg to our local vbuf.
        ret = cmsVbuf_put(vbuf, (void *)(res_data + 1), res_data->size);
        if (ret != CMSRET_SUCCESS)
        {
            cmsVbuf_destroy(vbuf);
            cmsMem_free(response);
            return ret;
        }

        // allocate buffer for the result, returned to caller.
        data_len = res_data->numEntries * sizeof(BcmGenericParamAttr);
        result = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (result == NULL)
        {
            cmsVbuf_destroy(vbuf);
            cmsMem_free(response);
            return CMSRET_RESOURCE_EXCEEDED;
        }

        deserializeParamAttrArray(vbuf, result, res_data->numEntries);
        cmsVbuf_destroy(vbuf);
        *pParamAttrList = result;
        *pNumParamAttrEntries = res_data->numEntries;
    }
    else
    {
        cmsLog_error("Failed to receieve from remote_objd! ret=%d", ret);
    }

    cmsMem_free(response);
    return ret;
}

CmsRet remote_setParamAttributes(const BcmGenericParamAttr *paramAttrArray,
                                 SINT32 numEntries,
                                 UINT32 setFlags __attribute__((unused)))
{
    size_t data_len, msg_len, serialized_len;
    char *requestBuf;
    CmsMsgHeader *request, *response = NULL;
    RemoteObjSetParamAttributesRequest *req_data;
    BcmGenericParamAttr *attrs = NULL;
    CmsVbuf *vbuf;
    SINT32 i;
    CmsRet ret = CMSRET_SUCCESS;

    /*
     * The format of this message is:
     * CmsMsgHeader
     * RemoteObjSetParamAttributesRequest
     * The entire paramAttrArray
     * All the fullpaths from the paramAttrArray (serialized into vbuf).
     */

    // Serialize all fullpaths so we can calculate the total length of the msg.
    vbuf = cmsVbuf_new();
    if (vbuf == NULL)
    {
        cmsLog_error("Failed to create input vbuf!");
        return CMSRET_INTERNAL_ERROR;
    }

    for (i=0; i < numEntries; i++)
    {
        cmsVbuf_putString(vbuf, paramAttrArray[i].fullpath);
    }
    serialized_len = cmsVbuf_getSize(vbuf);

    // Allocate and fill in the request message
    data_len = sizeof(RemoteObjSetParamAttributesRequest) +
               (numEntries * sizeof(BcmGenericParamAttr)) +
               serialized_len;
    msg_len = sizeof(CmsMsgHeader) + data_len;
    requestBuf = cmsMem_alloc(msg_len, ALLOC_ZEROIZE);
    if (requestBuf == NULL)
    {
        cmsVbuf_destroy(vbuf);
        return CMSRET_RESOURCE_EXCEEDED;
    }
    request = (CmsMsgHeader *)requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_SET_PARAMATTRS;
    request->dataLength = data_len;

    req_data = (RemoteObjSetParamAttributesRequest *)(request + 1);
    req_data->numEntries = numEntries;

    // Copy the paramAttrArray into the msg, right after the RemoteObjSetParamAttributesRequest.
    attrs = (BcmGenericParamAttr *)(req_data + 1);
    memcpy((void *) attrs, paramAttrArray, numEntries * sizeof(BcmGenericParamAttr));
    // To avoid any confusion, NULL out the fullpath pointers in the copied
    // data since they are not valid when received by remote_objd.
    for (i=0; i < numEntries; i++)
    {
        attrs[i].fullpath = NULL;
    }

    // Copy the serialized fullpaths into the msg, right after the paramAttrArray.
    memcpy((void *) &(attrs[numEntries]), vbuf->data, serialized_len);
    cmsVbuf_destroy(vbuf);
    vbuf = NULL;

    // send to remote_objd and get response.  Using a single ret variable 
    // for success or failure is problematic because it is difficult to figure
    // out if there was a communication error or an error setting the params
    // themselves.  Regardless of ret, always free response.
    ret = remote_sendAndGetReply(request, &response);

    cmsMem_free(requestBuf);
    cmsMem_free(response);
    return ret;
}

/* Get remote parameter values from remote_objd. */
//const MdmPathDescriptor  *pPathList,
//                             SINT32             numEntries,
CmsRet remote_getParamValues(const char       **fullpathArray,
                             SINT32             numEntries,
                             UBOOL8             nextLevelOnly,
                             UINT32             getFlags,
                             BcmGenericParamInfo **pParamInfoArray,
                             SINT32             *pNumParamInfoEntries)
{
    CmsRet ret = CMSRET_SUCCESS;
    size_t data_len, msg_len, serialized_len;
    SINT32 i;
    char *requestBuf;
    CmsMsgHeader *request, *response = NULL;
    RemoteObjGetParamsRequest *req_data;
    CmsVbuf *vbuf;

    // Serialize the array of fullpaths into a vbuf
    vbuf = cmsVbuf_new();
    if (vbuf == NULL)
    {
        cmsLog_error("Failed to create input vbuf!");
        return CMSRET_INTERNAL_ERROR;
    }

    for (i=0; i < numEntries; i++)
    {
        cmsVbuf_putString(vbuf, fullpathArray[i]);
    }
    serialized_len = cmsVbuf_getSize(vbuf);

    data_len = sizeof(RemoteObjGetParamsRequest) + serialized_len;

    // Allocate and fill out the request message
    msg_len = sizeof(CmsMsgHeader) + data_len;
    requestBuf = cmsMem_alloc(msg_len, ALLOC_ZEROIZE);
    if (requestBuf == NULL)
    {
        cmsVbuf_destroy(vbuf);
        return CMSRET_RESOURCE_EXCEEDED;
    }
    request = (CmsMsgHeader *)requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_GET_PARAMS;
    request->dataLength = data_len;
    req_data = (RemoteObjGetParamsRequest *)(request + 1);
    req_data->nextLevelOnly = nextLevelOnly;
    req_data->flags = getFlags;
    req_data->numEntries = numEntries;
    memcpy((char *)(req_data + 1), vbuf->data, serialized_len);

    /*
     * Send the request to remote_objd and wait for the reply.
     */
    ret = remote_sendAndGetReply(request, &response);

    // we are done with these, can free them now.
    cmsMem_free(requestBuf);
    cmsVbuf_destroy(vbuf);
    vbuf = NULL;

    if (ret == CMSRET_SUCCESS)
    {
        BcmGenericParamInfo *result;
        RemoteObjGetParamsResponse *res_data;

        vbuf = cmsVbuf_new();
        if (vbuf == NULL)
        {
            cmsLog_error("Failed to create vbuf!");
            cmsMem_free(response);
            return CMSRET_INTERNAL_ERROR;
        }

        res_data = (RemoteObjGetParamsResponse *)(response + 1);

        // copy the data from the response msg to our local vbuf.
        ret = cmsVbuf_put(vbuf, (void *)(res_data + 1), res_data->size);
        if (ret != CMSRET_SUCCESS)
        {
            cmsVbuf_destroy(vbuf);
            cmsMem_free(response);
            return ret;
        }

        // allocate buffer for the result, returned to caller.
        data_len = res_data->numEntries * sizeof(BcmGenericParamInfo);
        result = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (result == NULL)
        {
            cmsVbuf_destroy(vbuf);
            cmsMem_free(response);
            return CMSRET_RESOURCE_EXCEEDED;
        }

        deserializeParamInfoArray(vbuf, result, res_data->numEntries);
        cmsVbuf_destroy(vbuf);
        *pParamInfoArray = result;
        *pNumParamInfoEntries = res_data->numEntries;
    }
    else
    {
        cmsLog_error("Failed to receieve from remote_objd! ret=%d", ret);
    }

    cmsMem_free(response);
    return ret;
}

CmsRet remote_getParamNamesEx(const char           *fullpath,
                              UBOOL8                nextLevelOnly,
                              UINT32                flags,
                              PhlGetParamNameEx_t **paramNameArray,
                              SINT32               *numParamNames)
{
    CmsRet ret = CMSRET_SUCCESS;
    char requestBuf[sizeof(CmsMsgHeader) + sizeof(RemoteObjGetParamNamesRequest)] = {0};
    CmsMsgHeader *request, *response = NULL;
    RemoteObjGetParamNamesRequest *req_data = NULL;
    PhlGetParamNameEx_t *result = NULL;

    cmsLog_notice("Entered: %s (flags=0x%x)", fullpath, flags);

    request = (CmsMsgHeader *)&requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_GET_PARAMNAMES;
    request->dataLength = sizeof(RemoteObjGetParamNamesRequest);
    req_data = (RemoteObjGetParamNamesRequest *)(request + 1);

    strncpy((char *)&req_data->fullPath, fullpath, sizeof(req_data->fullPath) - 1);

    req_data->nextLevelOnly = nextLevelOnly;
    req_data->flags = flags;
    ret = remote_sendAndGetReply(request, &response);
    if (ret == CMSRET_SUCCESS)
    {
        int i;
        size_t data_len;
        CmsVbuf *vbuf = NULL;
        RemoteObjGetParamNamesResponse *res_data;
        BcmGenericParamInfo *pParamInfoArray, *p;

        res_data = (RemoteObjGetParamNamesResponse *)(response + 1);

        data_len = res_data->numEntries * sizeof(PhlGetParamNameEx_t);
        result = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (result == NULL)
        {
            ret = CMSRET_RESOURCE_EXCEEDED;
            cmsLog_error("Allocating %d entries failed", res_data->numEntries);
            goto exit;
        }

        vbuf = cmsVbuf_new();
        if (vbuf == NULL)
        {
            cmsLog_error("Failed to create vbuf!");
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }

        ret = cmsVbuf_put(vbuf, (void *)(res_data + 1), res_data->size);
        if (ret != CMSRET_SUCCESS)
        {
            cmsVbuf_destroy(vbuf);
            goto exit;
        }

        // allocate buffer for the result, returned to caller.
        data_len = res_data->numEntries * sizeof(BcmGenericParamInfo);
        pParamInfoArray = cmsMem_alloc(data_len, ALLOC_ZEROIZE);
        if (pParamInfoArray == NULL)
        {
            cmsVbuf_destroy(vbuf);
            ret = CMSRET_RESOURCE_EXCEEDED;
            goto exit;
        }
        deserializeParamInfoArray(vbuf, pParamInfoArray, res_data->numEntries);
        for (i = 0; i < res_data->numEntries; i++)
        {
            p = &pParamInfoArray[i];
            //steal fullpath to result[i]
            result[i].fullpath = p->fullpath;  
            p->fullpath = NULL;
            result[i].type = cmsMdm_getConstTypeString(p->type);
            result[i].profile = cmsMdm_getConstProfileString(p->profile);
            result[i].writable = p->writable;
            result[i].isTr69Password = p->isPassword;
        }
        cmsVbuf_destroy(vbuf);
        cmsUtl_freeParamInfoArray(&pParamInfoArray, res_data->numEntries);
        *paramNameArray = result;
        *numParamNames = res_data->numEntries;
    }

exit:
    cmsMem_free(response);
    /* paramNameArray from caller will free result */
    /* coverity[leaked_storage] */
    return ret;
}

/* Send (remote) setParamValueList to remote_objd. */
CmsRet remote_setParamValues(BcmGenericParamInfo *paramInfoArray,
                             SINT32 numEntries,
                             UINT32 setFlags)
{
    size_t data_len, msg_len, inner_len;
    char *requestBuf;
    int i, origIdx;
    int goodParams = 0;
    int *goodToActualParamsArray;
    CmsMsgHeader *request, *response = NULL;
    RemoteObjSetParamsRequest *req_data;
    void *data;
    CmsVbuf *vbuf;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_notice("Entered: numEntries=%d setFlags=0x%x", numEntries, setFlags);

    goodToActualParamsArray = (int *) cmsMem_alloc(numEntries * sizeof(int),
                                                   ALLOC_ZEROIZE);
    if (goodToActualParamsArray == NULL)
    {
        cmsLog_error("failed to allocate %d entries", numEntries);
        return CMSRET_INTERNAL_ERROR;
    }

    //Serialize paramInfoArray into vbuf for sending to remote_objd
    vbuf = cmsVbuf_new();
    if (vbuf == NULL)
    {
        cmsLog_error("Failed to create vbuf!");
        cmsMem_free(goodToActualParamsArray);
        return CMSRET_INTERNAL_ERROR;
    }

    for (origIdx = 0; origIdx < numEntries; origIdx++)
    {
        if (paramInfoArray[origIdx].errorCode != CMSRET_SUCCESS)
        {
           // param already has error detected, do not send it to remote side.
           continue;
        }

        cmsVbuf_putString(vbuf, paramInfoArray[origIdx].fullpath);
        cmsVbuf_putString(vbuf, paramInfoArray[origIdx].type);
        cmsVbuf_putString(vbuf, paramInfoArray[origIdx].value);

        // record where this good param is in the actual param array
        goodToActualParamsArray[goodParams] = origIdx;
        goodParams++;
    }

    cmsLog_debug("numEntries=%d goodParams=%d", numEntries, goodParams);
    // If no good params to send to remote_objd, then just return now.
    // Same logic as local_setParameterValues.
    if (goodParams == 0)
    {
        cmsVbuf_destroy(vbuf);
        cmsMem_free(goodToActualParamsArray);
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Allocate and fill in CMS msg.
    inner_len = cmsVbuf_getSize(vbuf);
    data_len = sizeof(RemoteObjSetParamsRequest) + inner_len;
    msg_len = sizeof(CmsMsgHeader) + data_len;
    requestBuf = cmsMem_alloc(msg_len, ALLOC_ZEROIZE);
    if (requestBuf == NULL)
    {
        cmsLog_error("Could not allocate req msg");
        cmsVbuf_destroy(vbuf);
        cmsMem_free(goodToActualParamsArray);
        return CMSRET_RESOURCE_EXCEEDED;
    }
    request = (CmsMsgHeader *)requestBuf;
    request->type = CMS_MSG_REMOTE_OBJ_SET_PARAMS;
    request->dataLength = data_len;
    req_data = (RemoteObjSetParamsRequest *)(request + 1);
    req_data->flags = setFlags;
    req_data->numEntries = goodParams;
    req_data->size = inner_len;
    data = (void *)(req_data + 1);
    memcpy(data, vbuf->data, inner_len);

    // send to remote_objd and get response.  Checking ret for success or
    // failure is problematic because it is difficult to figure out if there
    // was a communication error or an error setting the params themselves.
    ret = remote_sendAndGetReply(request, &response);
    if (response != NULL)
    {
        if (response->dataLength >= sizeof(RemoteObjSetParamsResponse))
        {
            // We got a response, process regardless of return code.
            // TR69 requires us to return error codes for each param.
            RemoteObjSetParamsResponse *res_data;
            UINT32 *data;

            res_data = (RemoteObjSetParamsResponse*)(response + 1);
            if (res_data->numEntries != goodParams)
            {
                cmsLog_error("count mismatch: got %d goodParams=%d numEntries=%d",
                             res_data->numEntries, goodParams, numEntries);
                ret = CMSRET_INTERNAL_ERROR;
            }
            else
            {
                data = (UINT32 *)(res_data + 1);
                for (i = 0; i < res_data->numEntries; i++)
                {
                    // Copy returned result codes to their actual position in the
                    // original setParamValueList.
                    origIdx = goodToActualParamsArray[i];
                    paramInfoArray[origIdx].errorCode = data[i];
                }
            }
        }
        else
        {
            // reponse msg received, but wrong length.  This is probably a
            // programming error (internal error), or a communications error.
            cmsLog_error("sendAndGetReply returned bad dataLength=%d ret=%d",
                         response->dataLength, ret);
            ret = CMSRET_INTERNAL_ERROR;
        }
    }
    else
    {
       // No response msg, must be communications error.
       cmsLog_error("sendAndGetReply failed, no response msg, ret=%d", ret);
    }

    cmsVbuf_destroy(vbuf);
    cmsMem_free(requestBuf);
    cmsMem_free(response);
    cmsMem_free(goodToActualParamsArray);
    return ret;
}

void remote_clearAllParamValueChanges(void)
{
    CmsRet ret;
    CmsMsgHeader request=EMPTY_MSG_HEADER;
    CmsMsgHeader *response=NULL;

    request.type = CMS_MSG_REMOTE_CLEAR_ALL_PARAM_VALUE_CHANGES;
    request.dataLength = 0;
    ret = remote_sendAndGetReply(&request, &response);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("remote_clear_all_param_value_change error %d",ret);
    }
    cmsMem_free(response);
}

UINT32 remote_getNumberOfParamValueChanges(void)
{
    UINT32 count = 0;
    CmsMsgHeader request=EMPTY_MSG_HEADER;
    CmsMsgHeader *response=NULL;

    request.type = CMS_MSG_REMOTE_GET_NUM_PARAM_VALUE_CHANGES;
    request.dataLength = 0;
    count = remote_sendAndGetReply(&request, &response);
    cmsMem_free(response);
    return (count);
}

CmsRet remote_getChangedParams(char ***arrayParams, UINT32 *numParams)
{
    CmsMsgHeader request=EMPTY_MSG_HEADER;
    CmsMsgHeader *response=NULL;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_notice("Entered:");

    if ((arrayParams == NULL) || (numParams == NULL))
    {
       cmsLog_error("null input params %p/%p", arrayParams, numParams);
       return CMSRET_INVALID_ARGUMENTS;
    }
    *numParams=0;
    *arrayParams=NULL;

    // Ask remote_objd to get all changed params from remote components.
    request.type = CMS_MSG_REMOTE_GET_CHANGED_PARAMS;
    ret = remote_sendAndGetReply(&request, &response);

    if (ret != CMSRET_SUCCESS)
    {
       cmsMem_free(response);
       return ret;
    }

    if (response == NULL)
       return CMSRET_INTERNAL_ERROR;

    // No data or only a single null term: no changed params in remote MDMs.
    if (response->dataLength <= 1)
    {
       cmsMem_free(response);
       return CMSRET_SUCCESS;
    }

    {
        UINT32 num=0;
        UINT32 i=0;
        char **array=NULL;
        char *data;
        char *begin, *end;

        /* remoteobj gives a combined string of changed parameters from all
         * the remote components in this format (see bcm_generic_hal_defs.h)
         * <_DELIM_>fullpath,type,value</_DELIM_><_DELIM_>fullpath,type,value</_DELIM_>
         * Reformat this single string into an array of strings, E.g.
         * array[0] = fullpath,type,value
         * array[1] = fullpath,type,value
         * array[2] = fullpath,type,value
         */

        // First count the number of fullpath,type,string tuples in the response.
        data = (char *)(response + 1);
        end = strstr(data, BCM_DATABASEOP_DELIM_END);
        while (end != NULL)
        {
           num++;
           data = end+BCM_DATABASEOP_DELIM_END_LEN;
           end = strstr(data, BCM_DATABASEOP_DELIM_END);
        }

        if (num == 0)
        {
           cmsLog_error("Invalid response string ==>%s<==", (char *)response);
           CMSMEM_FREE_BUF_AND_NULL_PTR(response);
           return CMSRET_INTERNAL_ERROR;
        }

        // Allocate the array.
        array = cmsMem_alloc((sizeof(char*))*num,ALLOC_ZEROIZE);
        if (array == NULL)
        {
           cmsLog_error("Cannot allocate array, num=%d", num);
           CMSMEM_FREE_BUF_AND_NULL_PTR(response);
           return CMSRET_RESOURCE_EXCEEDED;
        }

        // Find tuples, allocate a buffer for it, and copy.
        data = (char *)(response + 1);
        begin = strstr(data, BCM_DATABASEOP_DELIM_BEGIN);
        end = strstr(data, BCM_DATABASEOP_DELIM_END);
        while ((begin != NULL) && (end != NULL) && (i < num))
        {
           // advance begin ptr past the begining <_DELIM_> marker
           begin += BCM_DATABASEOP_DELIM_BEGIN_LEN;

           // allocate a string buffer to hold the current tuple
           array[i] = cmsMem_alloc(end-begin+1, ALLOC_ZEROIZE);
           if (array[i] == NULL)
           {
              cmsLog_error("Could not allocate at %d, len=%d", i, end-begin+1);
              ret = CMSRET_RESOURCE_EXCEEDED;
              break;
           }

           // copy current tuple into array slot.  Note we cannot use strcpy
           // because each tuple is not null teriminated.
           memcpy(array[i], begin, end-begin);

           // find the next tuple
           data = end+BCM_DATABASEOP_DELIM_END_LEN;
           begin = strstr(data, BCM_DATABASEOP_DELIM_BEGIN);
           end = strstr(data, BCM_DATABASEOP_DELIM_END);
           i++;
        }

        if (ret == CMSRET_SUCCESS)
        {
            *arrayParams=array;
            *numParams=num;
        }
        else
        {
           cmsUtl_freeArrayOfStrings(&array, num);
        }
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(response);

    cmsLog_debug("Exit: ret=%d numParams=%d", ret, *numParams);
    return (ret);
}


// Send a CMS_MSG_TERMINATE msg to remote_objd
void remote_terminate(void)
{
    CmsMsgHeader request=EMPTY_MSG_HEADER;
    CmsMsgHeader *response=NULL;

    request.type = CMS_MSG_TERMINATE;
    remote_sendAndGetReply(&request, &response);
    cmsMem_free(response);
    return;
}

