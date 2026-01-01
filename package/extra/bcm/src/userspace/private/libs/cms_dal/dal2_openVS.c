/*
* <:copyright-BRCM:2013:proprietary:standard
* 
*    Copyright (c) 2013 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_2

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "rut_openvswitch.h"

UBOOL8 dalOpenVS_isEnabled_dev2()
{
   UBOOL8 openVSEnable = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2OpenvswitchCfgObject *openVSCfg=NULL;
   CmsRet ret;
  
   if ((ret = cmsObj_get(MDMOID_DEV2_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get Dev2OpenvswitchCfgObect, ret=%d", ret);
   }
   else
   {
      openVSEnable = openVSCfg->enable;
      cmsObj_free((void **) &openVSCfg);
   }
  
   return openVSEnable;
}

CmsRet dalOpenVS_getCfg_dev2(char *info)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2OpenvswitchCfgObject *openVsCfg = NULL;

    if (info == NULL)
    {
        return CMSRET_INVALID_PARAM_TYPE;
    }

    ret = cmsObj_get(MDMOID_DEV2_OPENVSWITCH_CFG, &iidStack, 0, (void **)&openVsCfg);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get Dev2OpenvswitchCfgObject, ret=%d", ret);
        return ret;
    }
    
    sprintf(info, "%d|%s|%d|,", openVsCfg->enable, openVsCfg->OFControllerIPAddress,
                                openVsCfg->OFControllerPortNumber);

    cmsObj_free((void **)&openVsCfg);
    return ret;
}

CmsRet dalOpenVS_setCfg_dev2(const char *enable, const char *ofControllerAddr,
                             UINT32 ofControllerPort)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2OpenvswitchCfgObject *openVsCfg = NULL;

    ret = cmsObj_get(MDMOID_DEV2_OPENVSWITCH_CFG, &iidStack, 0, (void **)&openVsCfg);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get Dev2OpenvswitchCfgObject, ret=%d", ret);
        return ret;
    }
    
    if (strcmp(enable, "1") == 0)
    {
        openVsCfg->enable = TRUE;
        CMSMEM_REPLACE_STRING(openVsCfg->OFControllerIPAddress, ofControllerAddr);
        openVsCfg->OFControllerPortNumber = ofControllerPort;
    } 
    else
    {
        openVsCfg->enable = FALSE;
    }
    ret = cmsObj_set(openVsCfg, &iidStack);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set Dev2OpenvswitchCfgObject, ret=%d", ret);
    }

    cmsObj_free((void **)&openVsCfg);
    return ret;
}

CmsRet dalOpenVS_getBridgeList_dev2(char **bridgeList)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj = NULL;
   char *result = NULL;
   const char *sep = ",";

   cmsLog_debug("enter:");

   result = cmsMem_strdup("");
   if (result == NULL)
   {
       cmsLog_error("Failed to realloc buffer!");
       return CMSRET_RESOURCE_EXCEEDED;
   }

   while (CMSRET_SUCCESS == cmsObj_getNext(MDMOID_DEV2_BRIDGE, &iidStack, (void *)&brObj))
   {
       if (0 == cmsUtl_strcmp(brObj->X_BROADCOM_COM_Type, MDMVS_OVS))
       {
           UINT32 newLen;
           newLen = cmsUtl_strlen(result) + cmsUtl_strlen(brObj->X_BROADCOM_COM_IfName) + 2;
           result = cmsMem_realloc(result, newLen);
           if (result == NULL)
           {
               cmsLog_error("Failed to realloc buffer!");
               cmsObj_free((void **)&brObj);
               return CMSRET_RESOURCE_EXCEEDED;
           }
           cmsUtl_strcat(result, sep);
           cmsUtl_strcat(result, brObj->X_BROADCOM_COM_IfName);
       }
       cmsObj_free((void **)&brObj);
   }

   *bridgeList = result;
   cmsLog_debug("returning bridge list %s", result);

   return ret;
}

CmsRet dalOpenVS_addBridge_dev2(const char *bridgeName)
{
   /* OVS bridge's IPInterfaceObject will set its groupName to be the same
    * as its bridgeName.
    */
   return dalBridge_addBridgeEx_dev2(INTFGRP_BR_HOST_MODE, bridgeName, bridgeName, TRUE);
}

CmsRet dalOpenVS_delBridge_dev2(const char *bridgeName)
{
    if (bridgeName == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_notice("Deleting %s", bridgeName);
   /* OVS bridge's IPInterfaceObject will set its groupName to be the same
    * as its bridgeName.
    */
    return dalBridge_deleteBridge_dev2(bridgeName);
}

CmsRet dalOpenVS_getBridgePorts_dev2(const char *bridgeName, char **ports)
{
    InstanceIdStack briidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2BridgeObject *brObj = NULL;
    Dev2BridgePortObject *brPortObj = NULL;
    char *temp = NULL;;
    UBOOL8 found = FALSE;
    CmsRet ret;

    if (bridgeName == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    while (found == FALSE && (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &briidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &brObj)) == CMSRET_SUCCESS)
    {
        //Do we need to check if the bridge type is Ovs? Since the bridge name
        //should be unique, don't do that for now.
        if (0 == cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridgeName))
        {
            found = TRUE;
        }
        cmsObj_free((void **)&brObj);
    }

    if (found == FALSE)
    {
        return CMSRET_OBJECT_NOT_FOUND;
    }

    while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT, &briidStack,
                                               &iidStack,
                                               OGF_NO_VALUE_UPDATE,
                                               (void **) &brPortObj)) == CMSRET_SUCCESS)
    {
        if (brPortObj->managementPort == FALSE)
        {
            if (temp == NULL)
            {
                temp = cmsMem_strdup(brPortObj->name);
            }
            else
            {
                temp = cmsMem_realloc(temp, strlen(temp) + strlen(brPortObj->name) + 2);
                cmsUtl_strcat(temp, ",");
                cmsUtl_strcat(temp, brPortObj->name);
            }
        }
        cmsObj_free((void **)&brPortObj);
    }

    if (temp != NULL)
    {
        *ports = cmsMem_strdup(temp);
        cmsMem_free(temp);
    }
    else
    {
        //There is no ports under the target bridge. Just return an empty string.
        *ports = cmsMem_strdup("");
    }
    return CMSRET_SUCCESS;
}

CmsRet dalOpenVS_setBridgePorts_dev2(const char *bridgeName, const char *ports)
{
    InstanceIdStack briidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2BridgeObject *brObj = NULL;
    UBOOL8 found = FALSE;
    CmsRet ret;

    if (bridgeName == NULL)
    {
        cmsLog_error("Empty bridgeName!");
        return CMSRET_INVALID_ARGUMENTS;
    }

    while (found == FALSE && (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &briidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &brObj)) == CMSRET_SUCCESS)
    {
        //Do we need to check if the bridge type is Ovs? Since the bridge name
        //should be unique, don't do that for now.
        if (0 == cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, bridgeName))
        {
            found = TRUE;
        }
        cmsObj_free((void **)&brObj);
    }

    if (found == TRUE)
    {
        char *buf, *port, *saveptr;
        char ifListBr0[BUFLEN_256] = {0};
        Dev2BridgePortObject *brPortObj = NULL;

        //Remove any non-management ports which are not present in
        //the specified list from the OVS bridge to the default bridge (br0)
        while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT, &briidStack,
                            &iidStack, OGF_NO_VALUE_UPDATE,
                            (void **) &brPortObj)) == CMSRET_SUCCESS)
        {
            if (brPortObj->managementPort == FALSE &&
                cmsUtl_findInList(ports, brPortObj->name) == NULL)
            {
                if (strlen(ifListBr0) == 0)
                {
                    sprintf(ifListBr0, "%s,", brPortObj->name);                    
                }
                else if ((strlen(ifListBr0) + strlen(brPortObj->name) + 1) >= BUFLEN_256)
                {
                    cmsLog_error("Interface list is too long! (%s)", ifListBr0);
                }
                else
                {
                    strcat(ifListBr0, brPortObj->name);
                    strcat(ifListBr0, ",");
                }
            }
            cmsObj_free((void **)&brPortObj);
        }

        for (port = strtok_r(ifListBr0, ",", &saveptr); port != NULL; port = strtok_r(NULL, ",", &saveptr))
        {
            cmsLog_notice("Removing port %s from bridge %s to br0",
                              port, bridgeName);
            ret = dalBridge_assocFilterIntfToBridge_dev2(port, "Default");
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to move port %s from %s to br0",
                               port, bridgeName);
            }
        }

        //Move ports in the specified list into the target bridge
        buf = cmsMem_strdup(ports);
        if (buf == NULL)
        {
            cmsLog_error("Failed to allocate memory!");
            return CMSRET_RESOURCE_EXCEEDED;
        }

        for (port = strtok_r(buf, ",", &saveptr); port != NULL; port = strtok_r(NULL, ",", &saveptr))
        {
            UBOOL8 portFound = FALSE;

            memset(&iidStack, 0x0, sizeof(InstanceIdStack));
            while (portFound == FALSE &&
                   (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                            &briidStack, &iidStack, OGF_NO_VALUE_UPDATE,
                            (void **) &brPortObj)) == CMSRET_SUCCESS)
            {
                if (brPortObj->managementPort == FALSE &&
                    cmsUtl_strcmp(brPortObj->name, port) == 0)
                {
                    portFound = TRUE;
                }
                cmsObj_free((void **)&brPortObj);
            }

            if (!portFound)
            {
                cmsLog_notice("Move port %s to bridge %s", port, bridgeName);
                dalBridge_assocFilterIntfToBridge_dev2(port, bridgeName);
            }
             
        }
        cmsMem_free(buf);
    }
    return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_OPENVSWITCH_1 */

