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

#ifndef __BCM_GENERIC_HAL_H__
#define __BCM_GENERIC_HAL_H__

/*!\file bcm_generic_hal.h
 * \brief Header file for libbcm_generic_hal.so.  Originally, this HAL
 *        supported the first version of the RDK generic HAL for Telco Voice.
 *        Now updated to support the RDK JSON Schema/HAL.
 *        Another way to think about this Generic HAL is that it is a non-CMS
 *        and even non-BDK interface to the CMS/BDK MDM.
 *
 */
 
#include "bcm_generic_hal_defs.h"
#include "bcm_retcodes.h"

BcmRet bcm_generic_getParameterNames(const char            *fullpath,
                                     UBOOL8                 nextLevel,
                                     UINT32                 flags,
                                     BcmGenericParamInfo  **paramInfoArray,
                                     UINT32                *numParamInfos);

BcmRet bcm_generic_getParameterValues(const char          **fullpathArray,
                                      UINT32                numFullpaths,
                                      UBOOL8                nextLevel,
                                      UINT32                flags,
                                      BcmGenericParamInfo **paramInfoArray,
                                      UINT32               *numParamInfos);

BcmRet bcm_generic_setParameterValues(BcmGenericParamInfo *paramInfoArray,
                                      UINT32               numParamInfos,
                                      UINT32               flags);

BcmRet bcm_generic_getParameterAttributes(const char **fullpathArray,
                                          UINT32       numFullpaths,
                                          UBOOL8       nextLevel,
                                          UINT32       flags,
                                          BcmGenericParamAttr **paramAttrArray,
                                          UINT32      *numParamAttrs);

// The set operation may set individual error codes in the paramAttrArray (not implemented yet),
// that is why not const *
BcmRet bcm_generic_setParameterAttributes(BcmGenericParamAttr *paramAttrArray,
                                          UINT32               numParamAttrs,
                                          UINT32               flags);


// Free the BcmGenericParamInfo array returned by bcm_generic_getParameterNames
// and bcm_generic_getParameterValues.  These are internally alloc'd with
// cmsMem_alloc, and have additional string fields which need to be freed,
// so provide wrappers so callers don't have to figure out how to do it right.
void bcm_generic_freeParamInfoArray(BcmGenericParamInfo  **paramInfoArray,
                                    UINT32                 numParamInfos);

void bcm_generic_freeParamAttrArray(BcmGenericParamAttr  **paramAttrArray,
                                    UINT32                 numParamAttrs);

/** Free all strings in the array, then free the array itself, and then set
 *  the pointer to NULL (that is why ***).  Also assumes everything was
 *  allocated with cmsMem_alloc.
 */
void bcm_generic_freeArrayOfStrings(char ***array, UINT32 len);


BcmRet bcm_generic_addObject(const char *fullpath, UINT32 flags,
                             UINT32 *newInstance);

BcmRet bcm_generic_deleteObject(const char *fullpath, UINT32 flags);


// op and input are both strings.
// op is defined as BCM_DATABASEOP_xxx in bcm_generic_hal_defs.h
// On successful return, output may contain a string buffer which caller
// is responsible for freeing by calling bcm_generic_freeDatabaseOutput.
BcmRet bcm_generic_databaseOp(const char *op, const char *input,
                              char **output);

// Output of databaseOp is alloc'd internally with cmsMem_alloc, so provide
// a wrapper function so callers do not need to call cmsMem_free directly.
void bcm_generic_freeDatabaseOutput(char **output);


#endif /* __BCM_GENERIC_HAL_H__ */
