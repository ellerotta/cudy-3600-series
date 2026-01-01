/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_BRIDGE_1

#include "odl.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_ipv6.h"
#include "rut2_util.h"
#include "rut2_ethernet.h"
#include "rut2_bridging.h"
#include "beep_networking.h"
#include "qdm_route.h"



/*!\file rcl2_bridging.c
 * \brief This file contains Device.Bridging. objects.
 * This file currently contains the vlan objects, but they can be moved
 * out to another file if we want more separation of the code.
 *
 */
CmsRet rcl_dev2BridgingObject( _Dev2BridgingObject *newObj __attribute__((unused)),
                     const _Dev2BridgingObject *currObj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


/* The valid bridge name should follow below rules:
 * 1. Must begin with "br"
 * 2. If the pattern is "br%d", the number following must agree with X_BROADCOM_COM_Index.
 * 3. The name should be unique acoss all Bridge instances.
 */
UBOOL8 validateBridgeName(const char *brName, int index, const InstanceIdStack *myIidStack)
{
    long int brIndex;
    char *end;
    Dev2BridgeObject *brObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("brName:%s, index:%d", brName, index);

    //Check if brName begins with "br"
    if (1 == cmsUtl_strncmp(brName, "br", strlen("br")))
    {
        return FALSE;
    }

    //If brName is following "br%d" pattern, check if the number agrees with
    //X_BROADCOM_COM_Index
    brIndex = strtol(&brName[2], &end, 10);
    if (*end == '\0')
    {
        //X_BROADCOM_COM_IfName is following "br%d" pattern
        if (brIndex != index)
        {
            cmsLog_error("Bridge IfName:%s doesn't agree with Bridge index number %d!",
                         brName, index);
            return FALSE;
        }
        //If brName is following "br%d" pattern and the number agrees with index,
        //it must be a valid bridge name.
        return TRUE;
    }

    //Check if the brName is unique across all Bridge instances
    while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &brObj)) == CMSRET_SUCCESS)
    {
       if ((0 != memcmp(&iidStack, myIidStack, sizeof(InstanceIdStack))) &&
           (0 == cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brName)))
       {
           cmsObj_free((void **) &brObj);
           cmsLog_error("Found existing bridge with the same name!");
           return FALSE;
       }
       cmsObj_free((void **) &brObj);
    }

    return TRUE;
}

CmsRet rcl_dev2BridgeObject( _Dev2BridgeObject *newObj,
                     const _Dev2BridgeObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    SINT32 bridgeNum = -1;
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        Dev2BridgingObject *bridgingObj = NULL;

        ret = cmsObj_get(MDMOID_DEV2_BRIDGING, &brIidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&bridgingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_get failed. ret=%d MDMOID_DEV2_BRIDGING", ret);
            return ret;
        }

        if (bridgingObj->bridgeNumberOfEntries == bridgingObj->maxBridgeEntries)
        {
            cmsLog_error("Number of bridges exceeds maximum %d", bridgingObj->maxBridgeEntries);
            cmsObj_free((void **)&bridgingObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
        if (bridgingObj->bridgeNumberOfEntries == bridgingObj->maxQBridgeEntries)
        {
            cmsLog_error("Number of bridges exceeds maximum %d", bridgingObj->maxQBridgeEntries);
            cmsObj_free((void **)&bridgingObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }
#endif
        cmsObj_free((void **)&bridgingObj);

        //bootup case
        if (mdmShmCtx->inMdmInit)
        {
           if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_SECONDARY_MODE)
           {
              char activeGwIfName[CMS_IFNAME_LENGTH]={0};
              UBOOL8 isIPv4 = TRUE;

              /* firewall rules similar to guest wifi */
              rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_SECONDARY_MODE);

              qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
              if (!IS_EMPTY_STRING(activeGwIfName))
              {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                     qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                    rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
              }
           }
           else if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_WANONLY_MODE)
           {
              char activeGwIfName[CMS_IFNAME_LENGTH]={0};
              UBOOL8 isIPv4 = TRUE;

              /* firewall rules to only allow WAN access */
              rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_WANONLY_MODE);

              qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
              if (!IS_EMPTY_STRING(activeGwIfName))
              {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                     qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                    rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
              }
           }
        }
        else
        {
            //allocate bridge index number to the newly added bridge instance
            if ((bridgeNum = rutLan_getNextAvailableBridgeNumber()) < 0)
            {
                return CMSRET_RESOURCE_EXCEEDED;
            }
            newObj->X_BROADCOM_COM_Index = bridgeNum;
            cmsLog_debug("set index %d for %s", bridgeNum, newObj->X_BROADCOM_COM_IfName);
        }

        rutUtil_modifyNumBridge(iidStack, 1);
    }

    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        UBOOL8 enableBridge=TRUE;
        UBOOL8 isOVS = FALSE;
        char ipAddr[CMS_IPADDR_LENGTH];
        char ipMask[CMS_IPADDR_LENGTH];
        char bCast[CMS_IPADDR_LENGTH];
        Dev2IpInterfaceObject *ipIntfObj=NULL;
        InstanceIdStack intfIidStack=EMPTY_INSTANCE_ID_STACK;
        UBOOL8 foundBrIpIntf=FALSE;

        sprintf(ipAddr, "0.0.0.0");
        sprintf(ipMask, "0.0.0.0");
        sprintf(bCast, "0.0.0.0");

        /** when bridge is enabled, its index should be assigned and validated
         *   this check is for backward compatibility with existing boards 
         *   which doesn't have X_BROADCOM_COM_Index (default value -1)
         */
        if (newObj->X_BROADCOM_COM_Index == -1)
        {
            //allocate bridge index number to the newly added bridge instance
            if ((bridgeNum = rutLan_getNextAvailableBridgeNumber()) < 0)
            {
                return CMSRET_RESOURCE_EXCEEDED;
            }
            newObj->X_BROADCOM_COM_Index = bridgeNum;
            cmsLog_debug("set index %d for %s", bridgeNum, newObj->X_BROADCOM_COM_IfName);
        }

        if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_IfName))
        {
            //Generate bridge name using "br%d" pattern, where %d is the value
            //of X_BROADCOM_COM_Index of this instance.
            char brIntfNameBuf[CMS_IFNAME_LENGTH] = {0};
            snprintf(brIntfNameBuf, sizeof(brIntfNameBuf), "br%d", newObj->X_BROADCOM_COM_Index);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_IfName, brIntfNameBuf, mdmLibCtx.allocFlags);
            cmsLog_notice("generated new bridge name %s", newObj->X_BROADCOM_COM_IfName);
        }
        else
        {
            if (FALSE == validateBridgeName(newObj->X_BROADCOM_COM_IfName,
                                            newObj->X_BROADCOM_COM_Index,
                                            iidStack))
            {
                cmsLog_error("Failed to validate bridge name!");
                return CMSRET_INVALID_ARGUMENTS;
            }
        }

        if (currObj != NULL)
        {
            /* Enable existing bridge. */

            /* Find bridge ip interface. */
            while (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, 
                                                &intfIidStack,
                                                OGF_NO_VALUE_UPDATE, 
                                                (void **) &ipIntfObj) == CMSRET_SUCCESS)
            {
                if (!cmsUtl_strcmp(ipIntfObj->name, newObj->X_BROADCOM_COM_IfName))
                {
                    /* need this obj, so break now and free it later */
                    foundBrIpIntf = TRUE;
                    break;
                }
                cmsObj_free((void **) &ipIntfObj);
            }

            if (!foundBrIpIntf)
            {
                cmsLog_notice("Could not find IP.Interface for %s", newObj->X_BROADCOM_COM_IfName);
            }
            else
            {
                /* Restore bridge interface ipv4 address */
                if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP))
                {
                    Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
                    InstanceIdStack addrIidStack=EMPTY_INSTANCE_ID_STACK;

                    if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_ADDRESS,
                                                                &intfIidStack,
                                                                &addrIidStack,
                                                                OGF_NO_VALUE_UPDATE,
                                                                (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
                    {
                        if (!cmsUtl_strcmp(ipv4AddrObj->status, MDMVS_ENABLED) &&
                             !cmsUtl_strcmp(ipv4AddrObj->addressingType, MDMVS_STATIC))
                        {
                            cmsUtl_strncpy(ipAddr, ipv4AddrObj->IPAddress, sizeof(ipAddr));
                            cmsUtl_strncpy(ipMask, ipv4AddrObj->subnetMask, sizeof(ipMask));
                            rut_getBCastFromIpSubnetMask(ipAddr, ipMask, bCast);
                        }
                        cmsObj_free((void **) &ipv4AddrObj);
                    }
                }
            }
        }


        if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_Type, MDMVS_OVS) == 0)
        {
            isOVS = TRUE;
        }

       /*
        * Just enable the bridge here.  IP addrs are enabled from the RCL
        * handler functions for the IPv4 and IPv6 addr objects.
        */
        cmsLog_debug("enable bridge %s: type=%s ip=%s mask=%s bcast=%s",
                             newObj->X_BROADCOM_COM_IfName,
                             newObj->X_BROADCOM_COM_Type,
                             ipAddr, ipMask, bCast);
        ret = rutLan_enableBridge(newObj->X_BROADCOM_COM_IfName, isOVS, enableBridge,
                                  ipAddr, ipMask, bCast);

        if (ret == CMSRET_SUCCESS)
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
        }

        if (foundBrIpIntf)
        {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
            /* Restore bridge interface ipv6 address. */
            if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP))
            {
                Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
                InstanceIdStack addrIidStack=EMPTY_INSTANCE_ID_STACK;

                if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                                                            &intfIidStack,
                                                            &addrIidStack,
                                                            OGF_NO_VALUE_UPDATE,
                                                            (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
                {
                    if (!cmsUtl_strcmp(ipv6AddrObj->status, MDMVS_ENABLED) &&
                         !cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_STATIC))
                    {
                        char prefix[CMS_IPADDR_LENGTH];

                        if (qdmIpv6_fullPathToPefixLocked_dev2(ipv6AddrObj->prefix, prefix) != CMSRET_SUCCESS)
                        {
                            cmsLog_error("cannot get prefix from %s", ipv6AddrObj->prefix);
                        }
                        else
                        {
                            rutIp_configureIpv6Addr(ipIntfObj->name, ipv6AddrObj->IPAddress, prefix);
                        }
                    }
                    cmsObj_free((void **) &ipv6AddrObj);
                }
            }
#endif
            /* free ipIntfObj */
            cmsObj_free((void **) &ipIntfObj);
        }

        if (currObj != NULL)
        {
            /* Enable existing bridge */

            Dev2BridgePortObject *brPortObj=NULL;
            InstanceIdStack brPortIidStack=EMPTY_INSTANCE_ID_STACK;
            UBOOL8 isUpstream;

            /* Add all the non-management ports to bridge. */
            while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                                            iidStack,
                                                            &brPortIidStack,
                                                            OGF_NO_VALUE_UPDATE,
                                                            (void **) &brPortObj) == CMSRET_SUCCESS)
            {
                if (!brPortObj->managementPort &&
                     !cmsUtl_strcmp(brPortObj->status, MDMVS_UP))
                {
                    char l2IntfName[BUFLEN_32];
                    char *ptr;

                    cmsLog_debug("bridge port %s is UP! add to %s",
                                     brPortObj->name, newObj->X_BROADCOM_COM_IfName);

                    strncpy(l2IntfName, brPortObj->name, sizeof(l2IntfName)-1);
                    l2IntfName[sizeof(l2IntfName)-1] = '\0';
                    ptr = strchr(l2IntfName, '.');
                    if (ptr != NULL)
                        *ptr = '\0';

                    isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

                    rutLan_addInterfaceToBridgeEx(brPortObj->name,
                                                  isUpstream,
                                                  newObj->X_BROADCOM_COM_IfName,
                                                  isOVS);

                    rutMulti_updateIgmpMldProxyIntfList();
                }
                cmsObj_free((void **) &brPortObj);
            }
        }
        else
        {
            /* currObj == NULL, bootup case */

            Dev2BridgePortObject *brPortObj=NULL;
            InstanceIdStack brPortIidStack=EMPTY_INSTANCE_ID_STACK;
            UBOOL8 isUpstream=FALSE;
            UBOOL8 isPresent=FALSE;

            /* Add all the non-management LAN ports to bridge. */
            while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                                            iidStack,
                                                            &brPortIidStack,
                                                            OGF_NO_VALUE_UPDATE,
                                                            (void **) &brPortObj) == CMSRET_SUCCESS)
            {
                if (!brPortObj->managementPort && strncmp(brPortObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0 )
                {
                    char l2IntfName[BUFLEN_32];
                    char *ptr;

                    cmsLog_debug("got bridge port %s (parent bridge %s)",
                                 brPortObj->name, newObj->X_BROADCOM_COM_IfName);

                    strncpy(l2IntfName, brPortObj->name, sizeof(l2IntfName));
                    ptr = strchr(l2IntfName, '.');
                    if (ptr != NULL)
                        *ptr = '\0';

                    isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

                    // Dynamic active ethernet is not present during bootup,
                    // so don't try to add it to bridge if not present.
                    // Assume all other port types are present.
                    if (rutEth_isDynamicActiveEthernet(l2IntfName) &&
                        (sysUtl_getIfindexByIfname(l2IntfName) < 0))
                    {
                        cmsLog_debug("%s does not exist yet, probably dynamic active ethernet", l2IntfName);
                        isPresent = FALSE;
                    }
                    else
                    {
                        isPresent = TRUE;
                    }

                    // TODO: here we check for !upStream, but other places we do not.
                    if (!isUpstream && isPresent)
                    {
                        rutLan_addInterfaceToBridgeEx(brPortObj->name,
                                                      isUpstream,
                                                      newObj->X_BROADCOM_COM_IfName,
                                                      isOVS);
                        rutMulti_updateIgmpMldProxyIntfList();
                    }
                }
                cmsObj_free((void **) &brPortObj);
            }

        }

        /*
         * BEEP requires interface group to create separated bridge
         * and need to add firewall rules and NAT rules
         */
        if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_SECONDARY_MODE)
        {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             /* firewall rules similar to guest wifi */
             rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_SECONDARY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                      qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                      rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
             }
        }
        else if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_WANONLY_MODE)
        {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             /* firewall rules to only allow WAN access */
             rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_WANONLY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                      qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                      rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
             }
        }
    }


    if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
        UBOOL8 isOVS = FALSE;

        cmsLog_debug("disable bridge %s", currObj->X_BROADCOM_COM_IfName);

        if (0 == cmsUtl_strcmp(currObj->X_BROADCOM_COM_Type, MDMVS_OVS))
        {
            isOVS = TRUE;
        }

        rutLan_disableBridge(currObj->X_BROADCOM_COM_IfName, isOVS);

        if (DELETE_EXISTING(newObj, currObj))
        {
            rutUtil_modifyNumBridge(iidStack, -1);
        }
        else
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
        }
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2BridgePortObject( _Dev2BridgePortObject *newObj,
                                          const _Dev2BridgePortObject *currObj,
                                          const InstanceIdStack *iidStack,
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    char brIntfNameBuf[CMS_IFNAME_LENGTH]={0};
    UBOOL8 statusChanged = FALSE;
    CmsRet ret;
    UBOOL8 isOVS = FALSE;

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgePort(iidStack, 1);
    }

    if (newObj && !newObj->managementPort)
    {
        cmsLog_notice("Entered: port name=%s status=%s enable=%d lowerLayer=%s",
                      newObj->name, newObj->status, newObj->enable,
                      newObj->lowerLayers);
    }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
    /* We don't allow a bridge port be configured with both VLANTermination and
     * VLANBridge objects.
     */
    if (newObj && !newObj->managementPort &&
         cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination"))
    {
        MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
        Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
        char *brPortFullPath = NULL;
        UBOOL8 foundVlanPort = FALSE;

        /* This port is to be configured on top of VLANTermination object.
         * We want to check that it had not been configured with VLANBridge object.
         */
        pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
        pathDesc.iidStack = *iidStack;
        ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("pathDescriptorToFullPathNoEndDot failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                             ret, cmsMdm_dumpIidStack(iidStack));
            return ret;
        }

        /* The parent Bridge object is 1 level above the port object. */
        brIidStack = *iidStack;
        POP_INSTANCE_ID(&brIidStack);

        /* find the associated vlan port */
        while (!foundVlanPort &&
                 (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                             &brIidStack,
                                             &brVlanPortIidStack,
                                             OGF_NO_VALUE_UPDATE,
                                             (void **)&brVlanPortObj)) == CMSRET_SUCCESS)
        {
            if (!cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
            {
                foundVlanPort = TRUE;
            }
            cmsObj_free((void **)&brVlanPortObj);
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

        if (foundVlanPort)
        {
            cmsLog_error("bridge port had been configured with VLANBridge object.");
            return CMSRET_INVALID_ARGUMENTS;
        }
        if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_getNextInSubTreeFlags failed. ret=%d DEV2_BRIDGE_VLAN_PORT iidStack=%s",
                             ret, cmsMdm_dumpIidStack(&brVlanPortIidStack));
            return ret;
        }
    }
#endif

    /*
     * Always force management Port status to UP.  This allows brx which sits
     * on top of Management Port to always be UP.
     */
    if (newObj && newObj->managementPort && cmsUtl_strcmp(newObj->status, MDMVS_UP))
    {
        cmsLog_debug("force mgmtPort %s to status UP", cmsMdm_dumpIidStack(iidStack));
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
        newObj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
    }

    IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

    /*
     * First figure out what my bridge intf name is.
     */
    ret = rutBridge_getParentBridgeIntfName_dev2(iidStack, brIntfNameBuf);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get parent bridge intfName, newObj=%p currObj=%p iidStack=%s",
                         newObj, currObj, cmsMdm_dumpIidStack(iidStack));
        return ret;
    }


    /* Get the type of my bridge */
    ret = rutBridge_getParentBridgeType_dev2(iidStack, &isOVS);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get parent bridge type, newObj=%p currObj=%p iidStack=%s",
                         newObj, currObj, cmsMdm_dumpIidStack(iidStack));
        return ret;
    }

    cmsLog_debug("parentBridgeName=%s isOVS=%d", brIntfNameBuf, isOVS);

    /*
     * If we are not in a delete situation (newObj != NULL) and
     * we don't have a Linux interface name yet, try to figure it out.
     * (a) if this is a management port, ifName is in parent bridge.
     * (b) if this is not a management port, go down on LowerLayers
     */
    if ((newObj != NULL) && (IS_EMPTY_STRING(newObj->name) ||
                             !(cmsUtl_strcmp(newObj->name,"unassigned"))))
    {
        if (newObj->managementPort)
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, brIntfNameBuf, mdmLibCtx.allocFlags);
        }
        else if (!IS_EMPTY_STRING(newObj->X_BROADCOM_COM_Name))
        {
            /* DAL has provided a port name. Just use it. This is a use case for
             * interface grouping where the port name shall be maintained when
             * the port is moving from the old bridge to the new bridge.
             * Note that this parameter is invisible to ACS.
             */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, newObj->X_BROADCOM_COM_Name, mdmLibCtx.allocFlags);

            /* X_BROADCOM_COM_Name is no longer needed.  Free it. */
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_Name);
        }
        else if (!IS_EMPTY_STRING(newObj->lowerLayers))
        {
            char lowerIntfName[CMS_IFNAME_LENGTH*2]={0};

            if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->lowerLayers, lowerIntfName)) != CMSRET_SUCCESS)
            {
                cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
                return ret;
            }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
            /* If this port is to be on top of a VLANTermination object,
             * then get the interface name from the VLANTermination object.
             * Otherwise, create a virtual interface name using the next
             * available virtual interface index.
             */
            if (cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination") == NULL &&
                 cmsUtl_strncmp(lowerIntfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) &&
                 cmsUtl_strncmp(lowerIntfName, VXLAN_IFC_STR, strlen(VXLAN_IFC_STR)) &&
                 cmsUtl_strncmp(lowerIntfName, GRE_IFC_STR, strlen(GRE_IFC_STR)))
            {
                SINT32 vlanIndex = 0;

                if (rutUtil_getAvailVlanIndex_dev2(lowerIntfName, &vlanIndex) == CMSRET_SUCCESS)
                {
                    char vlanIfname[CMS_IFNAME_LENGTH]={0};

                    strcpy(vlanIfname, lowerIntfName);
                    snprintf(lowerIntfName, sizeof(lowerIntfName), "%s.%d", vlanIfname, vlanIndex);
                }
            }
#endif

            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, lowerIntfName, mdmLibCtx.allocFlags);
        }
        else
        {
           /* cannot figure out the name, give it an "unassigned" value */
           CMSMEM_REPLACE_STRING_FLAGS(newObj->name, "unassigned", mdmLibCtx.allocFlags);
        }

        cmsLog_debug("assigned port (mgmt=%d) name %s",
                         newObj->managementPort, newObj->name);
    }

    if (newObj && currObj && !newObj->managementPort)
    {
        if ((currObj->enable == FALSE) && (newObj->enable == TRUE))
        {
            // Going from disabled to ENABLED, need to check for a few other
            // conditions before we can set status to UP.
            if (IS_EMPTY_STRING(newObj->name) || IS_EMPTY_STRING(newObj->lowerLayers))
            {
                cmsLog_error("Cannot enable port without name and lowerLayers.");
                return CMSRET_INVALID_ARGUMENTS;
            }

            if (qdmIntf_isStatusUpOnFullPathLocked_dev2(newObj->lowerLayers))
            {
                // objObj->name is assigned, lowerLayer is up, so we can set
                // this port to UP.
                cmsLog_notice("setting status on %s to UP (curr status=%s)", newObj->name, currObj->status);
                CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
            }
            else
            {
                // Even if we want to enable this bridge port, if the lower layer is down,
                // the bridge port must be in lowerlayerdown.
                cmsLog_notice("%s remains LOWERLAYERDOWN because %s is LOWERLAYERDOWN",
                              newObj->name, newObj->lowerLayers);
                CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            }
        }
        else if ((currObj->enable == TRUE) && (newObj->enable == FALSE))
        {
            // going from enabled to DISABLED
            cmsLog_notice("%s is now DISABLED", newObj->name);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
        }

        if (cmsUtl_strcmp(newObj->status, currObj->status))
        {
            newObj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
            statusChanged = TRUE;
            cmsLog_notice("%s status changed from %s => %s",
                          currObj->name, currObj->status, newObj->status);
        }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
        if (statusChanged ||
             newObj->defaultUserPriority != currObj->defaultUserPriority ||
             newObj->TPID != currObj->TPID ||
             newObj->priorityTagging != currObj->priorityTagging)
#else
        if (statusChanged)
#endif
        {
            if (!cmsUtl_strcmp(newObj->status, MDMVS_UP))
            {
                UBOOL8 isUpstream = FALSE;
                UBOOL8 isPresent = FALSE;
                char l2IntfName[BUFLEN_32] = {0};
                char *ptr = NULL;

                cmsLog_debug("bridge port %s is UP! add to %s",
                             newObj->name, brIntfNameBuf);

#ifdef DMP_DEVICE2_VLANBRIDGE_1
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
                /* Port status changed to UP, we want to configure vlan rules for
                 * the interface provided that the interface:
                 * - is a virtual interface. Per design, VLAN is only supported on
                 *    virtual interface with dotted name.
                 * - is not on top of a VLANTermination object where vlan rules
                 *    had already been configured.
                 * - is not a member of another bridge. If the port is a member
                 *    of another bridge, no need to re-configure vlan rules for it.
                 *    This is likely the case when DAL is moving the port from its
                 *    old bridge to this bridge for interface grouping.
                 */
                if (strchr(newObj->name, '.') != NULL &&
                    cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination") == NULL &&
                    !rutBridge_isPortInOtherBridge(brIntfNameBuf, newObj->name))
                {
                    char l2Ifname[CMS_IFNAME_LENGTH]={0};

                    ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->lowerLayers, l2Ifname);
                    if (ret != CMSRET_SUCCESS)
                    {
                        cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret=%d lowerLayers=%s",
                                     ret, newObj->lowerLayers);
                    }
                    else
                    {
                        UBOOL8 untagged;
                        SINT32 vlanId;

                        ret = rutBridge_getVlanPortInfo_dev2(iidStack, &untagged, &vlanId);
                        if (ret != CMSRET_SUCCESS)
                        {
                            cmsLog_error("rutBridge_getVlanPortInfo_dev2 failed. ret=%d", ret);
                        }
                        else
                        {
                            ret = rutBridge_configVlanRules_dev2(newObj->name,
                                                                 l2Ifname,
                                                                 newObj->TPID,
                                                                 newObj->priorityTagging,
                                                                 newObj->defaultUserPriority,
                                                                 untagged,
                                                                 vlanId);
                            if (ret != CMSRET_SUCCESS)
                            {
                                cmsLog_error("rutBridge_configVlanRules_dev2 failed. ret=%d", ret);
                            }
                        }
                    }
                }
#endif
#endif
                strncpy(l2IntfName, newObj->name, sizeof(l2IntfName)-1);
                l2IntfName[sizeof(l2IntfName)-1] = '\0';
                ptr = strchr(l2IntfName, '.');
                if (ptr != NULL)
                    *ptr = '\0';

                isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

                // Dynamic active ethernet is not present during bootup,
                // so don't try to add it to bridge if not present.
                // Assume all other port types are present.
                // At this point, even the dynamic ethernet interface should
                // be present in the system.  This is just a sanity check and
                // isPresent is only used in the cmsLog_debug below.
                if (rutEth_isDynamicActiveEthernet(l2IntfName) &&
                    (sysUtl_getIfindexByIfname(l2IntfName) < 0))
                {
                    cmsLog_debug("%s does not exist yet,probably dynamic active ethernet", l2IntfName);
                    isPresent = FALSE;
                }
                else
                {
                    isPresent = TRUE;
                }

                cmsLog_debug("name=%s l2IntfName=%s isUpstream=%d isPresent=%d",
                             newObj->name, l2IntfName, isUpstream, isPresent);

                if (strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0)
                {
                  if (isUpstream)
                  {
                    /* WAN : add interface to bridge when link up */
                    rutLan_addInterfaceToBridgeEx(newObj->name, isUpstream, brIntfNameBuf, isOVS);
                    rutMulti_updateIgmpMldProxyIntfList();
                  }
                  else
                  {
                    /* LAN : when status changes from any non-UP status (DOWN, LOWERLAYERDOWN, DORMANT, ERROR, etc)
                     *       to UP, add it to bridge.  This is needed for:
                     *       OVS bridge port operation, and
                     *       dynamic active ethernet ports which come up after system boots
                     */
                    if (cmsUtl_strcmp(currObj->status, MDMVS_UP) != 0)
                    {
                        rutLan_addInterfaceToBridgeEx(newObj->name, isUpstream, brIntfNameBuf, isOVS);
                        rutMulti_updateIgmpMldProxyIntfList();
                    }
                  }
                }
                else // ex, bond0.0
                {
                    rutLan_addInterfaceToBridgeEx(newObj->name, isUpstream, brIntfNameBuf, isOVS);
                    rutMulti_updateIgmpMldProxyIntfList();
                }
            }
        }

        if (strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0)
        {
          /* when move down-LAN_ethx to brx : state LowerLayerDown -> LowerLayerDown */
          if (!cmsUtl_strcmp(currObj->status, MDMVS_LOWERLAYERDOWN) && !cmsUtl_strcmp(newObj->status, MDMVS_LOWERLAYERDOWN))
          {  
            UBOOL8 isUpstream;
            char l2IntfName[BUFLEN_32];
            char *ptr;

            strncpy(l2IntfName, newObj->name, sizeof(l2IntfName)-1);
            l2IntfName[sizeof(l2IntfName)-1] = '\0';
            ptr = strchr(l2IntfName, '.');
            if (ptr != NULL)
                *ptr = '\0';

            isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

            // TODO: here we check for !upStream, but other places we do not.
            if (!isUpstream)
            {
                cmsLog_notice("add bridge port %s to %s",
                                     newObj->name, brIntfNameBuf);

                rutLan_addInterfaceToBridgeEx(newObj->name, isUpstream, brIntfNameBuf, isOVS);
                rutMulti_updateIgmpMldProxyIntfList();
            }
            statusChanged = FALSE;
          }
        }

        if (statusChanged && !cmsUtl_strcmp(currObj->status, MDMVS_UP))
        {
            /* remove from bridge only when going from UP to any non-UP state */
            /* don't care if going from non-UP to non-UP state */
            cmsLog_debug("bridge port %s is %s, remove from %s",
                         newObj->name, newObj->status, brIntfNameBuf);
            if (strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) != 0)
            {
                rutLan_removeInterfaceFromBridgeEx(newObj->name, brIntfNameBuf, isOVS);
                rutMulti_updateIgmpMldProxyIntfList();
            }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANSUBIF)
            if (!cmsUtl_strcmp(newObj->status, MDMVS_LOWERLAYERDOWN))
            { 
                /* Port status changed to MDMVS_LOWERLAYERDOWN, we want to delete
                 * the port interface provided that the interface:
                 * - is a virtual interface with the exception of default LANVLAN,
                 *    e.g. eth1.0, eth2.0, which shall not be deleted.
                 * - is not a wlan. e.g. wl0.1, wl0.2
                 * - is not a member of another bridge.
                 */
                if (strchr(newObj->name, '.') != NULL &&
                    cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination") == NULL &&
                    cmsUtl_strncmp(newObj->name, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) &&
                    (cmsUtl_strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
                     cmsUtl_strstr(newObj->name, ".0") == NULL) &&
                    rut_wanGetIntfIndex(newObj->name) > 0 &&
                    !rutBridge_isPortInOtherBridge(brIntfNameBuf, newObj->name))
                {
                    /* Delete the virtual interface.
                     * All rules associated with it will be purged.
                     */
                    vlanSubif_deleteVlanInterface(newObj->name);
                }
            }
#endif
#endif
        }
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgePort(iidStack, -1);

        if (!currObj->managementPort)
        {
            if (!cmsUtl_strcmp(currObj->status, MDMVS_UP))
            {
               cmsLog_debug("bridge port %s is being deleted, remove from %s",
                            currObj->name, brIntfNameBuf);
               rutLan_removeInterfaceFromBridgeEx(currObj->name, brIntfNameBuf, isOVS);
            }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
            /* Need to modify the vlan port table */
            ret = rutBridge_deRefPortFromVlanPort_dev2(iidStack);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_deRefPortFromVlanPort_dev2 failed. ret=%d", ret);
            }

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANSUBIF)
            /* We need to delete the port interface provided that the interface:
             * - is a virtual interface with the exception of default LANVLAN,
             *    e.g. eth1.0, eth2.0, which shall not be deleted.
             * - is not a wlan. e.g. wl0.1, wl0.2
             * - is not a member of another bridge.
             */
            if (currObj->name && strchr(currObj->name, '.') != NULL &&
                cmsUtl_strcasestr(currObj->lowerLayers, "VLANTermination") == NULL &&
                cmsUtl_strncmp(currObj->name, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) &&
                (cmsUtl_strncmp(currObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
                 cmsUtl_strstr(currObj->name, ".0") == NULL) &&
                rut_wanGetIntfIndex(currObj->name) > 0 &&
                !rutBridge_isPortInOtherBridge(brIntfNameBuf, currObj->name))
            {
                /* Delete the virtual interface.
                 * All rules associated with it will be purged.
                 */
                vlanSubif_deleteVlanInterface(currObj->name);
            }
#endif
#endif
        }
    }


    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2BridgePortStatsObject( _Dev2BridgePortStatsObject *newObj __attribute__((unused)),
                     const _Dev2BridgePortStatsObject *currObj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


#ifdef DMP_DEVICE2_VLANBRIDGE_1
CmsRet rcl_dev2BridgeVlanObject( _Dev2BridgeVlanObject *newObj,
                     const _Dev2BridgeVlanObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        InstanceIdStack ancestorIidStack = *iidStack;
        Dev2BridgingObject *bridgingObj = NULL;
        Dev2BridgeObject *brObj = NULL;

        ret = cmsObj_get(MDMOID_DEV2_BRIDGING, &brIidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&bridgingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_get failed. ret=%d DEV2_BRIDGING", ret);
            return ret;
        }

        ret = cmsObj_getAncestorFlags(MDMOID_DEV2_BRIDGE, MDMOID_DEV2_BRIDGE_VLAN,
                                                &ancestorIidStack, OGF_NO_VALUE_UPDATE,
                                                (void **)&brObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_getAncestorFlags failed. ret=%d ancestor=DEV2_BRIDGE decendent=DEV2_BRIDGE_VLAN iidStack=%s",
                             ret, cmsMdm_dumpIidStack(iidStack));
            cmsObj_free((void **)&bridgingObj);
            return ret;
        }

        if (brObj->VLANNumberOfEntries == bridgingObj->maxVLANEntries)
        {
            cmsLog_error("Number of VLANs exceeds maximum %d", bridgingObj->maxVLANEntries);
            cmsObj_free((void **)&bridgingObj);
            cmsObj_free((void **)&brObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }
        cmsObj_free((void **)&bridgingObj);
        cmsObj_free((void **)&brObj);
 
        rutUtil_modifyNumBridgeVlan(iidStack, 1);

        if (newObj->enable)
        {
            ret = rutBridge_configVlan_dev2(iidStack, newObj->VLANID);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlan(iidStack, -1);

        cmsLog_debug("Delete VLAN object. iidStack=%s", cmsMdm_dumpIidStack(iidStack));

        ret = rutBridge_configVlan_dev2(iidStack, -1);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            return ret;
        }

        /* Need to modify the vlan port table */
        ret = rutBridge_deRefVlanFromVlanPort_dev2(iidStack);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_deRefVlanFromVlanPort_dev2 failed. ret=%d", ret);
        }
    }
    else
    {
        /* modify existing object */

        if (newObj->enable &&
             (!currObj->enable || (newObj->VLANID != currObj->VLANID)))
        {
            ret = rutBridge_configVlan_dev2(iidStack, newObj->VLANID);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
        else if (!newObj->enable && currObj->enable)
        {
            ret = rutBridge_configVlan_dev2(iidStack, -1);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
    }
    
    return ret;
}

CmsRet rcl_dev2BridgeVlanPortObject( _Dev2BridgeVlanPortObject *newObj,
                     const _Dev2BridgeVlanPortObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    Dev2BridgeVlanObject *brVlanObj = NULL;
    MdmPathDescriptor pathDesc;
    UBOOL8 untagged = TRUE;
    SINT32 vlanId = -1;

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlanPort(iidStack, 1);

        if (IS_EMPTY_STRING(newObj->VLAN) || IS_EMPTY_STRING(newObj->port))
        {
            newObj->enable = FALSE;
        }
        return ret;
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlanPort(iidStack, -1);

        cmsLog_debug("Delete VLAN port object. iidStack=%s bridge port=%s",
                         cmsMdm_dumpIidStack(iidStack), currObj->port);

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
        /* update bridge port vlanctl rule for either untagged or priority tagging. */
        ret = rutBridge_configPort_dev2(currObj->port, TRUE, -1);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
        }
#endif
    }
    else
    {
        /* modify existing object */

        if (IS_EMPTY_STRING(newObj->VLAN) || IS_EMPTY_STRING(newObj->port))
        {
            newObj->enable = FALSE;
        }

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
        if (newObj->enable && 
             (!currObj->enable ||
              (newObj->untagged != currObj->untagged) ||
              cmsUtl_strcmp(newObj->VLAN, currObj->VLAN) ||
              cmsUtl_strcmp(newObj->port, currObj->port)))
        {
            if (IS_EMPTY_STRING(newObj->VLAN))
            {
                /* This vlan port is not a member of any vlan. Disable vlan tagging. */
                untagged = TRUE;
                vlanId    = -1;
            }
            else
            {
                INIT_PATH_DESCRIPTOR(&pathDesc);
                ret = cmsMdm_fullPathToPathDescriptor(newObj->VLAN, &pathDesc);
                if (ret != CMSRET_SUCCESS)
                {
                    cmsLog_error("cmsMdm_fullPathToPathDescriptor failed. ret=%d", ret);
                    return ret;
                }
                ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN, &pathDesc.iidStack,
                                      OGF_NO_VALUE_UPDATE, (void **)&brVlanObj);
                if (ret != CMSRET_SUCCESS)
                {
                    cmsLog_error("cmsObj_get failed. MDMOID_DEV2_BRIDGE_VLAN, ret=%d", ret);
                    return ret;
                }

                untagged = newObj->untagged;
                vlanId    = brVlanObj->VLANID;

                cmsObj_free((void **)&brVlanObj);
            }

            ret = rutBridge_configPort_dev2(newObj->port, untagged, vlanId);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
            }
        }
        else if (!newObj->enable && currObj->enable)
        {
            ret = rutBridge_configPort_dev2(currObj->port, TRUE, -1);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
            }
        }
#endif
    }

    return ret;
}
#endif

CmsRet rcl_dev2BridgeFilterObject( _Dev2BridgeFilterObject *newObj,
                     const _Dev2BridgeFilterObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgeFilter(iidStack, 1);
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeFilter(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_BRIDGE_1 */

#endif     /* DMP_DEVICE2_BASELINE_1 */



