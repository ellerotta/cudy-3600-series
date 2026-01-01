/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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
#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "qdm_intf.h"


#ifdef DMP_DEVICE2_OPTICAL_1

// forward declaration
int mdm_isOpticalInterfaceObjectExists(void);

CmsRet mdm_addOpticalInterfaceObject(const char *ifName)
{
    CmsRet ret = CMSRET_SUCCESS;
    MdmPathDescriptor  pathDesc;
    DeviceOpticalObject *mdmObjDev = NULL;
    OpticalInterfaceObject *mdmObj = NULL;

    cmsLog_notice("Entered: ifName=%s", ifName);

    /*
     * Note that even though TR181 defines this object as multi-instance, the
     * PON team has designed it so that there is at most 1 instance (a singleton).
     */
    if (mdm_isOpticalInterfaceObjectExists())
    {
        cmsLog_error("An instance of OPTICAL_INTERFACE already exists, cannot add a new one (%s)!", ifName);
        return CMSRET_INTERNAL_ERROR;
    }

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_OPTICAL_INTERFACE;

    if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to add OpticalInterfaceObject, ret=%d", ret);
        return ret;
    }

    if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &mdmObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get OpticalInterfaceObject, ret=%d", ret);
        mdm_deleteObjectInstance(&pathDesc, NULL, NULL);
        return ret;
    }

    CMSMEM_REPLACE_STRING_FLAGS(mdmObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(mdmObj->name, ifName, mdmLibCtx.allocFlags);
    mdmObj->upstream = TRUE;
    mdmObj->enable = TRUE;

    ret = mdm_setObject((void **)&mdmObj, &pathDesc.iidStack, FALSE);
    mdm_freeObject((void **) &mdmObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set OpticalInterfaceObject. ret=%d", ret);
        return ret;
    }

    // Update the count of optical interfaces
    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEVICE_OPTICAL;

    if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &mdmObjDev)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get DeviceOpticalObject, ret=%d", ret);
        return ret;
    }

    mdmObjDev->interfaceNumberOfEntries++;
    ret = mdm_setObject((void **)&mdmObjDev, &pathDesc.iidStack, FALSE);
    mdm_freeObject((void **) &mdmObjDev);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set DeviceOpticalObject. ret=%d", ret);
        return ret;
    }

    return ret;
}

int mdm_isOpticalInterfaceObjectExists(void)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    OpticalInterfaceObject *mdmObj = NULL;
    int found = 0;

    while (!found && mdm_getNextObject(MDMOID_OPTICAL_INTERFACE, &iidStack, (void **) &mdmObj) == CMSRET_SUCCESS)
    {
        found = 1;
        mdm_freeObject((void **) &mdmObj);
    }

    return found;
}

#endif /* DMP_DEVICE2_OPTICAL_1 */

