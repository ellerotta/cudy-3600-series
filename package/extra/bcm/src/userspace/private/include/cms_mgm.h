/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
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


#ifndef __CMS_MGM_H__
#define __CMS_MGM_H__

/*!\file cms_mgm.h
 * \brief Header file for CPE management functions.
 *
 */

#include "cms.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Invalidate the config area in persistent memory.
 *
 * Typically, the caller will reboot the CPE after calling this function.
 * When the CPE boots again and sees that the config flash is invalid,
 * it will use factory defaults.
 * This call should always work.
 */
void cmsMgm_invalidateConfigFlash(void);

/** Newer version of saveConfigToFlash, which allows caller to control
 * whether this is a local or global operation.  By default, when flags = 0,
 * it will be a global op, just like cmsMgm_invalidateConfigToFlash().
 *
 * @param flags (IN)   The only flag supported right now is OGF_LOCAL_MDM_ONLY.
 */
void cmsMgm_invalidateConfigFlashEx(UINT32 flags);

/** This is mostly an internal function.  New code should just call
 *  cmsMgm_invalidateConfigFlashEx().
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_invalidateRemoteConfigFlash(void);


/** Read the MDM configuration from remote component(s), into the given
 *  buffer. 
 *
 * @param buf (IN/OUT) Caller provided pointer CMS header response message.
 *                     The callee function will allocate a buffer, read the config file from the 
 *                     persistent storage into this buffer passed back in CMS response.  Caller to free the CMS response message
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_readRemoteMdmToBuf(void **response);


/** Save CPE configuration to the config area in flash.
 *
 * This API function may be used to save CPE configuration
 * in the memory data model(MDM) to FLASH in XML format.
 * If this function is called in a BDK component which is capable of
 * controlling remote components, then a MDM operation of
 * BCM_DATABASEOP_SAVECONFIG_LOCAL will be sent to all remote components.
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_saveConfigToFlash(void);


/** Newer version of saveConfigToFlash, which allows caller to control
 * whether this is a local or global operation.  By default, when flags = 0,
 * it will be a global op, just like cmsMgm_saveConfigToFlash().
 *
 * @param flags (IN)   The only flag supported right now is OGF_LOCAL_MDM_ONLY.
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_saveConfigToFlashEx(UINT32 flags);


/** This is mostly an internal function which saves the local MDM to flash
 *  (will not trigger saves on remote components).  New code should just call
 *  cmsMgm_saveConfigToFlashEx();
 */
CmsRet cmsMgm_saveLocalConfigToFlash(void);

/** This is mostly an internal function which tells all
 *  remote components to save their config to flash.   New code should just
 *  call cmsMgm_saveConfigToFlashEx();
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_saveRemoteConfigToFlash(void);


/** Write CPE configuration to the given buffer.
 *
 * This function is provided to external apps as a debugging tool.
 * To write the current MDM configuration to flash, simply call
 * cmsMgm_saveConfigToFlash().
 *
 * @param buf (IN/OUT) Caller provides this buffer, and this function will fill
 *                     the buffer with the XML representation of the MDM that
 *                     would get written to the config flash.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 * @return CmsRet enum.
 */
CmsRet cmsMgm_writeConfigToBuf(char *buf, UINT32 *len);


/** Write entire MDM to the given buffer.
 *
 * This function is provided to external apps as a debugging tool.
 * The output of this function should NOT be written to the config flash.
 * This function is deprecated.  It only works for monolithic MDM.  New code
 * should use cmsMgm_writeMdmToBufEx().
 *
 * @param buf (IN/OUT) Caller provides this buffer, and this function will fill
 *                     the buffer with the XML representation of the MDM.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 * @return CmsRet enum.
 */
CmsRet cmsMgm_writeMdmToBuf(char *buf, UINT32 *len);


/** Write entire MDM to a buffer, handles remote components/Distributed MDM.
 *
 * @param buf (IN/OUT) This function will alloc a buffer and return to caller.
 *                     Caller is responsible for freeing it.
 * @param flags (IN)   The only flag supported right now is OGF_LOCAL_MDM_ONLY.
 * @return CmsRet enum.
 */
CmsRet cmsMgm_writeMdmToBufEx(char **buf, UINT32 flags);


/** Write given object information to the given buffer.
 *
 * This function is provided to external apps as a debugging tool.
 *
 * @param oid      IN) The object id whose parameters to traverse.
 * @param buf (IN/OUT) Caller provides this buffer, and this function will fill
 *                     the buffer with the XML representation of the MDM.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 * @return CmsRet enum.
 */
CmsRet cmsMgm_writeObjectToBuf(const MdmObjectId oid, char *buf, UINT32 *len);


/** Read the configuration from persistent storage (flash) into the given
 *  buffer.
 *
 * This function is provided to external apps as a debugging tool.
 * The MDM will automatically read the config flash on startup (during cmsMdm_init),
 * so external apps do not have to explicitly deal with that.  However, if an
 * apps wants to inspect the current contents of the config flash, then this
 * function can be used.  Note that the config flash may not be the same as the MDM
 * because the MDM could have been updated and not written back out to 
 * config flash yet.  See cmsMgm_writeConfigToBuf().
 *
 * @param buf (IN/OUT) Caller provided buffer for holding the config, and this
 *                     function will read the config file from the 
 *                     persistent storage into this buffer.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_readConfigFlashToBuf(char *buf, UINT32 *len);

/** This is a newer version of cmsMgm_readConfigFlashToBuf().  New code
 *  should use this version.  This version handles remote MDM's.
 *
 * @param buf (IN/OUT) This function will alloc a buffer and return to caller.
 *                     Caller is responsible for freeing it.
 * @param flags (IN)   The only flag supported right now is OGF_LOCAL_MDM_ONLY.
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_readConfigFlashToBufEx(char **buf, UINT32 flags);


/** This is mostly an internal function.  New code should just call
 *  cmsMgm_readConfigFlashToBufEx().
 */
CmsRet cmsMgm_readRemoteConfigFlashToBuf(void **response);


/** validate a buf containing a config file.
 *
 * Validate a buf by going through the motions of a traversal of the MDM.
 * But don't add any objects to the MDM.
 * This function should be able to detect:
 * - invalid format XML file.
 * - invalid MDM hierarchy.
 * - bad attributes.
 * - bad values as specified in the MDM.
 * - presence of a read-only var in the config file.
 *
 * But this function cannot detect the following conditions:
 * - use of the same instance number for multiple objects.
 * - improper specification of attributes within the hierarchy.
 * - RCL handler function rejecting the config.
 *
 * @param buf (IN) buffer containing a config file in XML format.
 * @param len (IN) length of buffer.
 * @return cmsRet enum.
 */
CmsRet cmsMgm_validateConfigBuf(const char *buf, UINT32 len);


/** Write the given buffer to persistent storage (flash).
 *
 * The buffer must contain a validated buffer.
 *
 * @param buf (IN) buffer containing a valid config file.
 * @param len (IN) length of buffer.
 *
 * @return CmsRet enum.
 */
CmsRet cmsMgm_writeValidatedBufToConfigFlash(const char *buf, UINT32 len);

CmsRet cmsMgm_writeValidatedBufToConfigFlashCompName(const char *buf,
                                           UINT32 len, const char *compName);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* __CMS_MGM_H__ */




