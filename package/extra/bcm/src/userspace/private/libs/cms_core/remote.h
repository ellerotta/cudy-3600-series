/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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


#ifndef __REMOTE_H__
#define __REMOTE_H__

/*!\file remote.h
 * \brief header file for accessing remote object handling.
 * The interfaces in this file are only used by phl and obj.
 * Management entities should not include this file or call any
 * of its functions.
 */
#include "cms_phl.h"

CmsRet remote_getObject(MdmObjectId oid,
                        const InstanceIdStack *iidStack,
                        UINT32 getFlags, void** mdmObj);


CmsRet remote_getNextObject(MdmObjectId oid,
                            const InstanceIdStack *parentIidStack,
                            InstanceIdStack *iidStack,
                            UINT32 getFlags, void** mdmObj);

CmsRet remote_setObject(const void *mdmObj,
                        const InstanceIdStack *iidStack,
                        UINT32 flags);

CmsRet remote_addObjectInstance(const char *fullpath,
                                UINT32 flags,
                                UINT32 *newInstanceId);

CmsRet remote_deleteObjectInstance(const char *fullpath, UINT32 flags);

CmsRet remote_clearStatistics(const char *fullpath, UINT32 flags);

CmsRet remote_getParamValues(const char       **fullpathArray,
                             SINT32             numEntries,
                             UBOOL8             nextLevelOnly,
                             UINT32             getFlags,
                             BcmGenericParamInfo **pParamValueList,
                             SINT32             *pNumParamInfoEntries);

CmsRet remote_setParamValues(BcmGenericParamInfo *paramInfoArray,
                             SINT32 numEntries,
                             UINT32 setFlags);

CmsRet remote_getAncestorObject(MdmObjectId ancestorOid,
                                MdmObjectId decendentOid,
                                InstanceIdStack *iidStack,
                                UINT32 getFlags,
                                void **mdmObj);

CmsRet remote_getParamNamesEx(const char           *fullpath,
                              UBOOL8                nextLevelOnly,
                              UINT32                flags,
                              PhlGetParamNameEx_t **paramNameArray,
                              SINT32               *numParamNames);

void remote_clearAllParamValueChanges(void);
UINT32 remote_getNumberOfParamValueChanges(void);
CmsRet remote_getChangedParams(char ***arrayParams, UINT32 *numParams);
CmsRet remote_getParamAttributes(const char       **fullpathArray,
                                 SINT32             numEntries,
                                 UBOOL8             nextLevelOnly,
                                 UINT32             flags,
                                 BcmGenericParamAttr  **pParamAttrList,
                                 SINT32             *pNumParamAttrEntries);
CmsRet remote_setParamAttributes(const BcmGenericParamAttr *paramAttrArray,
                                 SINT32 numEntries, UINT32 setFlags);

void remote_terminate(void);

#endif /* __REMOTE_H__ */
