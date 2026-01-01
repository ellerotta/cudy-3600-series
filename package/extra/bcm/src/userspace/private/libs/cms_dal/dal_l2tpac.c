/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "dal_wan.h"
#include "rut_l2tpac.h"

extern CmsRet rutWan_fillPppIfName(UBOOL8 isPPPoE, char *pppName);

CmsRet dalL2tpAc_deleteL2tpAcInterface_igd(const WEB_NTWK_VAR *webVar)
{
	void   *obj                           = NULL;
	CmsRet ret                            = CMSRET_SUCCESS;
	InstanceIdStack iidStack              = EMPTY_INSTANCE_ID_STACK;
	InstanceIdStack iidStack2             = EMPTY_INSTANCE_ID_STACK;
	L2tpAcIntfConfigObject *L2tpAcIntf    = NULL;
	L2tpAcLinkConfigObject *L2tpAclinkCfg = NULL; 
	cmsLog_debug("Deleting %s", webVar->tunnelName);
   
	if ((ret = dalWan_getWanL2tpIidStatck(&iidStack)) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not get L2tp Wan IidStatck");
		return ret;
	}

	/* delete all seesions on same tunnel */
	while ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &iidStack, &iidStack2, (void **)&obj))== CMSRET_SUCCESS)
	{
		if ((ret = cmsObj_get(MDMOID_L2TP_AC_LINK_CONFIG, &iidStack2, OGF_NO_VALUE_UPDATE, (void **) &L2tpAclinkCfg)) == CMSRET_SUCCESS)
		{
			if (0 == cmsUtl_strcmp(webVar->tunnelName, L2tpAclinkCfg->tunnelName))
			{
				if ((ret = cmsObj_deleteInstance(MDMOID_WAN_CONN_DEVICE, &iidStack2)) != CMSRET_SUCCESS)
				{
				 	cmsLog_error("Failed to delete wanConn Object, ret = %d", ret);
				}
				
				INIT_INSTANCE_ID_STACK(&iidStack2);
			}
			cmsObj_free((void **)&L2tpAclinkCfg);
		}

		cmsObj_free((void **)&obj);	
	}

	/* delete the tunnel*/
	INIT_INSTANCE_ID_STACK(&iidStack2);
	while ((ret = cmsObj_getNextInSubTree(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack, &iidStack2, (void **)&L2tpAcIntf)) == CMSRET_SUCCESS)
	{
		if (0 == cmsUtl_strcmp(webVar->tunnelName, L2tpAcIntf->tunnelName))
		{
			if ((ret = cmsObj_deleteInstance(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack2)) != CMSRET_SUCCESS)
			{
				cmsLog_error("Failed to delete INTF CONFIG Object, ret = %d", ret);
			}

			cmsObj_free((void **)&L2tpAcIntf);
			break;
		}

		cmsObj_free((void **)&L2tpAcIntf);
	}

	cmsLog_debug("Exit, ret=%d", ret);

	return ret;
}


static CmsRet dalL2tpAc_createL2tpAcIntf(const WEB_NTWK_VAR *webVar, 
												   const InstanceIdStack *iidStack)
{
	CmsRet          ret                = CMSRET_SUCCESS;
	InstanceIdStack iidStack2          = EMPTY_INSTANCE_ID_STACK;
	L2tpAcIntfConfigObject *L2tpAcIntf = NULL;
	InstanceIdStack L2tpAciidStack = EMPTY_INSTANCE_ID_STACK;
	L2tpAcClientCfgObject *L2tpAc  = NULL;
	UINT32 sourcePort = L2tpDefaultPort;  //default port of l2tp protocol
	UBOOL8 isSameLns  = FALSE;
    
	while ((ret = cmsObj_getNextInSubTree(MDMOID_L2TP_AC_INTF_CONFIG, iidStack, &iidStack2, (void **)&L2tpAcIntf)) == CMSRET_SUCCESS)
	{
		if (0 == cmsUtl_strcmp(webVar->tunnelName, L2tpAcIntf->tunnelName))
		{
			/*Currently, xl2tpd support multi tunnels, but only one session on one tunnel.*/
			cmsLog_error("Tunnel Name should be unique");
			cmsObj_free((void **)&L2tpAcIntf);
			return CMSRET_INTERNAL_ERROR;
		}
        
		if (0 == cmsUtl_strcmp(webVar->lnsIpAddress, L2tpAcIntf->lnsIpAddress))
		{    
			isSameLns = TRUE;
		}

		cmsObj_free((void **)&L2tpAcIntf);
	}

	if (isSameLns)
	{
		if ((ret = cmsObj_get(MDMOID_L2TP_AC_CLIENT_CFG, &L2tpAciidStack, 0, (void **) &L2tpAc)) != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to get L2tpAcClientCfgObject, ret=%d", ret);
			return ret;
		}

		if (L2tpAc->numberOfClient)   
		{
			sourcePort = L2tpDefaultPort + L2tpAc->numberOfClient;
		}
        
		cmsObj_free((void **)&L2tpAc);
	}

	/* Add new instance of L2tpAcIntfConfigObject */ 
	iidStack2 = *iidStack;
	if ((ret = cmsObj_addInstance(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack2)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Could not create new WanConnectionDevice, ret=%d", ret); 
		return ret;
	}

	/* Get the MDMOID_L2TP_AC_INTF_CONFIG to set the new values to it */
	if ((ret = cmsObj_get(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack2, 0, (void **)&L2tpAcIntf)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Failed to get iidStack, ret = %d", ret);
		cmsObj_deleteInstance(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack2);
		return ret;
	}

	CMSMEM_REPLACE_STRING(L2tpAcIntf->tunnelName, webVar->tunnelName);
	CMSMEM_REPLACE_STRING(L2tpAcIntf->lnsIpAddress, webVar->lnsIpAddress);
	L2tpAcIntf->enable        = TRUE;
	L2tpAcIntf->sourcePort    = sourcePort;

	cmsLog_debug("create new tunnel L2tpAcIntf->tunnelName=%s,L2tpAcIntf->lnsIpAddress=%s, L2tpAcIntf->sourcePort=%d\n",L2tpAcIntf->tunnelName,L2tpAcIntf->lnsIpAddress, L2tpAcIntf->sourcePort);
	if ((ret = cmsObj_set(L2tpAcIntf, &iidStack2)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Set L2tpAcIntfConfigObject failed");
		cmsObj_deleteInstance(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack2);
	}

	cmsObj_free((void **) &L2tpAcIntf);
		 
	return ret;
}


static CmsRet dalL2tpAc_createL2tpAcLink(const WEB_NTWK_VAR *webVar, 
										           const InstanceIdStack *iidStack)
{
	CmsRet ret                            = CMSRET_SUCCESS;
	InstanceIdStack  connDevStack         = EMPTY_INSTANCE_ID_STACK;
	InstanceIdStack  iidStack2            = EMPTY_INSTANCE_ID_STACK;
	L2tpAcLinkConfigObject *L2tpAclinkCfg = NULL;   
	WanPppConnObject *pppConn             = NULL; 
	char pppIfName[BUFLEN_32]             = {0};
	char pppConnName[BUFLEN_80]           = {0};

	/* add new instance of WanConnectionDevice */
	connDevStack = *iidStack;
	if ((ret = cmsObj_addInstance(MDMOID_WAN_CONN_DEVICE, &connDevStack)) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
		return ret;
	}

	cmsLog_debug(" Adding PPP_CONN, iidStack=%s\n", cmsMdm_dumpIidStack(&connDevStack));

	/* add WAN_PPP_CONN instance */
	iidStack2 = connDevStack;
	if ((ret = cmsObj_addInstance(MDMOID_WAN_PPP_CONN, &iidStack2)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Failed to add pppConnInstance, ret = %d", ret);
	}
	else
	{
		if ((ret = cmsObj_get(MDMOID_WAN_PPP_CONN, &iidStack2, 0, (void **) &pppConn)) != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to get pppConn, ret = %d", ret);
		}
	}

	if (ret != CMSRET_SUCCESS)
	{
		cmsObj_deleteInstance(MDMOID_WAN_CONN_DEVICE, &connDevStack);
		return ret;
	}

	/* get sevice name */
	if (strlen(webVar->pppServerName) > 0)
	{
		CMSMEM_REPLACE_STRING(pppConn->PPPoEServiceName, webVar->pppServerName);
	}
	
	CMSMEM_REPLACE_STRING(pppConn->connectionType, MDMVS_IP_ROUTED);
	CMSMEM_REPLACE_STRING(pppConn->username, webVar->pppUserName);
	CMSMEM_REPLACE_STRING(pppConn->password, webVar->pppPassword);

	/* Convert the number to string for auth protocol (method) */
	CMSMEM_REPLACE_STRING(pppConn->PPPAuthenticationProtocol, cmsUtl_numToPppAuthString(webVar->pppAuthMethod)); 

	/* get on demand ideltime out in seconds if it is enabled */
	if (webVar->enblOnDemand)
	{
		CMSMEM_REPLACE_STRING(pppConn->connectionTrigger, MDMVS_ONDEMAND);
		pppConn->idleDisconnectTime = webVar->pppTimeOut;
	}
	else
	{
		/* 0 is no on demand feature */
		CMSMEM_REPLACE_STRING(pppConn->connectionTrigger, MDMVS_ALWAYSON);
		pppConn->idleDisconnectTime = 0;
	}
   
	/* todo: Assuming NO IpExtension and static IP, passthru and connection mode here */
	pppConn->NATEnabled = TRUE;
	pppConn->X_BROADCOM_COM_FirewallEnabled = TRUE;

	/* get ppp debug flag */
	pppConn->X_BROADCOM_COM_Enable_Debug = webVar->enblPppDebug;

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
	if (webVar->enblIpVer != ENBL_IPV4_ONLY)
	{
		pppConn->X_BROADCOM_COM_IPv6Enabled = TRUE;
		CMSMEM_REPLACE_STRING(pppConn->X_BROADCOM_COM_IPv6AddressingType, webVar->wanAddr6Type);
		if (cmsUtl_strcmp(webVar->wanAddr6Type, MDMVS_STATIC) == 0)
		{
			CMSMEM_REPLACE_STRING(pppConn->X_BROADCOM_COM_ExternalIPv6Address, webVar->wanAddr6);
		}
		CMSMEM_REPLACE_STRING(pppConn->X_BROADCOM_COM_DefaultIPv6Gateway, webVar->wanGtwy6);
	}
#endif

	pppConn->enable = TRUE;

#ifdef DMP_X_BROADCOM_COM_IGMP_1
	pppConn->X_BROADCOM_COM_IGMPEnabled	= webVar->enblIgmp;
	pppConn->X_BROADCOM_COM_IGMP_SOURCEEnabled = webVar->enblIgmpMcastSource;
#endif
#ifdef DMP_X_BROADCOM_COM_MLD_1 /* aka SUPPORT_IPV6 */
	pppConn->X_BROADCOM_COM_MLDEnabled  = webVar->enblMld;
	pppConn->X_BROADCOM_COM_MLD_SOURCEEnabled  = webVar->enblMldMcastSource;
#endif

	/* it is PPoE. Need to form the L3 interface name for PPPoE  */
	if ((ret = rutWan_fillPppIfName(TRUE, pppIfName)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Failed to fill ppp ifname, ret = %d", ret);
		cmsObj_free((void **) &pppConn);
		cmsObj_deleteInstance(MDMOID_WAN_CONN_DEVICE, &connDevStack);
		return ret;
	}

	CMSMEM_REPLACE_STRING(pppConn->X_BROADCOM_COM_IfName, pppIfName);

	sprintf(pppConnName, "L2tp_%s", webVar->tunnelName);

	CMSMEM_REPLACE_STRING(pppConn->name, pppConnName);

	/* set and activate pppConnObj */
	if ((ret = cmsObj_set(pppConn, &iidStack2)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Set pppConn failed");
	}
	else
	{
		/* Get the MDMOID_L2TP_AC_LINK_CONFIG to set the new values to it */
		if ((ret = cmsObj_get(MDMOID_L2TP_AC_LINK_CONFIG, &connDevStack, 0, (void **)&L2tpAclinkCfg)) != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to get iidStack, ret = %d", ret);
		}
		else 
		{
			/* Active L2TPAC link Config object ? or later after L2tpd link is up - cwu todo */
			L2tpAclinkCfg->enable = TRUE;
			CMSMEM_REPLACE_STRING(L2tpAclinkCfg->tunnelName, webVar->tunnelName);
			CMSMEM_REPLACE_STRING(L2tpAclinkCfg->ifName, pppIfName);
			CMSMEM_REPLACE_STRING(L2tpAclinkCfg->sessionName, pppIfName);

			if ((ret = cmsObj_set(L2tpAclinkCfg, &connDevStack)) != CMSRET_SUCCESS)
			{
				cmsLog_error("set of L2tpAclinkCfg failed");
			}
			else
			{
				cmsLog_debug("cmsObj_set L2tpAclinkCfg ok.");
			}
			cmsObj_free((void **) &L2tpAclinkCfg);
		}   

		cmsLog_debug("cmsObj_set pppConn ok.");
	}

	if (ret != CMSRET_SUCCESS)
	{
		cmsObj_deleteInstance(MDMOID_WAN_CONN_DEVICE, &connDevStack);
	}

	cmsObj_free((void **) &pppConn);

	return ret;
}


CmsRet dalL2tpAc_addL2tpAcInterface_igd(const WEB_NTWK_VAR *webVar)
{
	InstanceIdStack  iidStack = EMPTY_INSTANCE_ID_STACK;
	CmsRet           ret      = CMSRET_SUCCESS;

	cmsLog_debug("Add L2tpAc Interface tunnelName=%s, lnsIpAddress=%s", webVar->tunnelName, webVar->lnsIpAddress);

	if (dalWan_getWanL2tpIidStatck(&iidStack) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not get L2tp Wan IidStatck");
		return ret;
	}

	/* add tunnel */
	if ((ret = dalL2tpAc_createL2tpAcIntf(webVar, &iidStack)) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not create L2tpAc Intf");
		return ret;
	}

	/* add session */
	if ((ret = dalL2tpAc_createL2tpAcLink(webVar, &iidStack)) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not create L2tpAc Intf");
		return ret;
	}

	return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

