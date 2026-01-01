/***********************************************************************
 *
 *  Copyright (c) 2017 Broadcom
 *  All Rights Reserved
 *
 * <:label-BRCM:2017:proprietary:standard
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
 * :>
 *
************************************************************************/

#ifdef DMP_X_ITU_ORG_GPON_1

#include "cms_obj.h"
#include "cms_log.h"
#include "cms_util.h"
#include "rut_omci.h"
#include "ethswctl_api.h"

#if defined(OMCI_TR69_DUAL_STACK)
#include "mdm.h"
#include "cms_lck.h"
#include "cms_util.h"
#include "rut_util.h"
#endif

static OmciEthPortType omciRut_getPortType(UINT8 port, UINT32 typesAll)
{
    OmciEthPortType portType = omciGetEthPortType(typesAll, port);

    return portType;
}

void rutOmci_printPorts(void)
{
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

    UINT32 port = 0;
    OmciEthPortType_t eth;
    OmciEthPortType portType;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSysObj = NULL;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&omciSysObj)) == CMSRET_SUCCESS)
    {
        eth.types.all = omciSysObj->ethernetTypes;
        for (port = 0; port < omciSysObj->numberOfEthernetPorts; port++)
        {
            portType = omciRut_getPortType(port, eth.types.all);
            printf("   Ethernet %d is in ", port);
            switch (portType)
            {
                case OMCI_ETH_PORT_TYPE_RG:
                    printf("RG mode\n");
                    break;
                case OMCI_ETH_PORT_TYPE_ONT:
                    printf("ONT mode\n");
                    break;
                case OMCI_ETH_PORT_TYPE_RG_ONT:
                    printf("RG_ONT mode\n");
                    break;
                default:
                    printf("unknown\n");
                    break;
            }
        }
        printf("\n");

        cmsObj_free((void **)&omciSysObj);
    }
}

CmsRet rutOmci_getEthPortTypeByName(char *name, OmciEthPortType *type)
{
    UINT32 port = 0;
    OmciEthPortType_t eth;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSysObj = NULL;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&omciSysObj)) == CMSRET_SUCCESS)
    {
        eth.types.all = omciSysObj->ethernetTypes;
        for (port = 0; port < omciSysObj->numberOfEthernetPorts;
          port++)
        {
            if (cmsUtl_strcmp(name, bcm_enet_util_get_lan_port_name(port)) == 0)
            {
                *type = omciRut_getPortType(port, eth.types.all);
                ret = CMSRET_SUCCESS;
                break;
            }
        }

        cmsObj_free((void**)&omciSysObj);
    }

    return ret;
}

CmsRet rutOmci_getBrgFwdMask(UINT32 *brgFwdMask)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSysObj = NULL;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, OGF_NO_VALUE_UPDATE,
      (void*)&omciSysObj)) == CMSRET_SUCCESS)
    {
        *brgFwdMask = omciSysObj->bridgeGroupFwdMask;
        cmsObj_free((void**)&omciSysObj);
    }

    return ret;
}

UBOOL8 rutOmci_isEthPortControlledByOmci(char *devIfName)
{
    UINT32 port = 0;
    OmciEthPortMgmt_t eth;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSysObj = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 isControlledByOmci = FALSE;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack,
      OGF_NO_VALUE_UPDATE, (void*)&omciSysObj)) == CMSRET_SUCCESS)
    {
        eth.omciManaged.all = omciSysObj->ethOmciManaged;
        for (port = 0; port < omciSysObj->numberOfEthernetPorts;
          port++)
        {
            if (cmsUtl_strcmp(devIfName, bcm_enet_util_get_lan_port_name(port)) == 0)
            {
                isControlledByOmci = omciIsEthPortOmciManaged(eth.omciManaged.all, port);
                break;
            }
        }
        cmsObj_free((void**)&omciSysObj);
    }

    return isControlledByOmci;
}


#if defined(OMCI_TR69_DUAL_STACK)

CmsRet rutOmci_getIpHostAddress(char *ifname, char **ipAddress, UBOOL8 *isIPv4)
{
   InstanceIdStack iidIpHost = EMPTY_INSTANCE_ID_STACK;
   BcmOmciRtdIpHostConfigDataObject *ipHost = NULL;
   UBOOL8 foundIpHost = FALSE;
   UINT32 ipHostMeId = 0;
   CmsRet ret;

   ret = rut_isGponIpHostInterface(ifname, &ipHostMeId);
   if (ret != CMSRET_SUCCESS)
       return ret;

   // Nested locks are allowed, but there must be an equal number of unlocks.
   ret = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);
   if (ret != CMSRET_SUCCESS)
      return ret;

   ret = CMSRET_OBJECT_NOT_FOUND;

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   BcmOmciRtdIpv6HostConfigDataObject *ipv6Host = NULL;
   while (ipHostMeId && !foundIpHost &&
          cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA,
                          &iidIpHost, (void **) &ipv6Host) == CMSRET_SUCCESS)
   {
       foundIpHost = (ipv6Host->managedEntityId == ipHostMeId);
       if (foundIpHost && ipv6Host->currentAddressTable)
       {
            char ipv6Address[CMS_IPADDR_LENGTH]={0};
            UINT32 size = 0;
            UINT8 *buf = NULL;

            // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->currentAddressTable, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, ipv6Address, sizeof(ipv6Address));
            // free temporary memory
            cmsMem_free(buf);

           *ipAddress = cmsMem_strdupFlags(ipv6Address, mdmLibCtx.allocFlags);
           *isIPv4 = FALSE;
           ret = CMSRET_SUCCESS;
       }
       cmsObj_free((void **) &ipv6Host);
   }

    INIT_INSTANCE_ID_STACK(&iidIpHost);
#endif    // DMP_X_BROADCOM_COM_IPV6_1

   while (ipHostMeId && !foundIpHost &&
          cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA,
                          &iidIpHost, (void **) &ipHost) == CMSRET_SUCCESS)
   {
       foundIpHost = (ipHost->managedEntityId == ipHostMeId);
       if (foundIpHost && ipHost->currentAddress)
       {
           char ipAddr[CMS_IPADDR_LENGTH]={0};
           struct in_addr inAddr;

           inAddr.s_addr = htonl(ipHost->currentAddress);
           cmsUtl_strncpy(ipAddr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
           *ipAddress = cmsMem_strdupFlags(ipAddr, mdmLibCtx.allocFlags);
           *isIPv4 = TRUE;
           ret = CMSRET_SUCCESS;
       }
       cmsObj_free((void **) &ipHost);
   }

   cmsLck_releaseLock();

   return ret;
}
#endif  // defined(OMCI_TR69_DUAL_STACK)

#endif /* DMP_X_ITU_ORG_GPON_1 */
