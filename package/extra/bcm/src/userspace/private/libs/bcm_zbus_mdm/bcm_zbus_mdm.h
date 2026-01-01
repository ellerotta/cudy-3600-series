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

#ifndef __BCM_ZBUS_MDM_H__
#define __BCM_ZBUS_MDM_H__

/*!\file bcm_zbus_mdm.h
 * \brief MDM dependent functions provided by bcm_zbus_mdm.
 *
 */

#include "cms_mdm.h"
#include "cms_phl.h"
#include "bcm_zbus_intf.h"

BcmRet zbus_in_getParameterNames(const char             *fullpath,
                                 UBOOL8                  nextLevelOnly,
                                 UINT32                  flags,
                                 BcmGenericParamInfo   **paramInfoArray,
                                 UINT32                 *numParamInfos);


BcmRet zbus_in_getParameterValues(const char          **fullpathArray,
                                  UINT32                numFullpaths,
                                  UBOOL8                nextLevel,
                                  UINT32                flags,
                                  BcmGenericParamInfo **paramInfoArray,
                                  UINT32               *numParamInfos);


BcmRet zbus_in_setParameterValues(BcmGenericParamInfo *paramInfoArray,
                                  UINT32               numParamInfos,
                                  UINT32               flags);


BcmRet zbus_in_getParameterAttributes(const char **fullpathArray,
                                      UINT32       numFullpaths,
                                      UBOOL8       nextLevel,
                                      UINT32       flags,
                                      BcmGenericParamAttr **paramAttrArray,
                                      UINT32      *numParamAttrs);

// In the future, the error code in the paramAttrArray may be set on return, that is why no const.
BcmRet zbus_in_setParameterAttributes(BcmGenericParamAttr *paramAttrArray,
                                      UINT32               numParamAttrs,
                                      UINT32               flags);


BcmRet zbus_in_addObject(const char *fullpath, UINT32 flags,
                         UINT32 *newInstanceId);

BcmRet zbus_in_deleteObject(const char *fullpath, UINT32 flags);

BcmRet zbus_in_mdmOp(const char *op, const char *arg, char **config);

#endif /* __BCM_ZBUS_MDM_H__ */
