/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
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
#ifdef DMP_X_BROADCOM_COM_L2TPAC_2    /* this file touches TR181 objects */
#include <sys/stat.h>
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "dal_wan.h"
#include "rut_l2tpac.h"

CmsRet dalL2tpAc_addL2tpAcInterface_dev2(const WEB_NTWK_VAR *webVar)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack tmpiidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2PppInterfaceObject *pppIntf=NULL;
    Dev2PppInterfaceObject *tmpPppIntf = NULL;
    Dev2PppInterfaceL2tpObject *l2tpIntf=NULL;
    Dev2PppInterfaceL2tpObject *tmpL2tpIntf=NULL;
    CmsRet ret;
    
    /* Add PPP interface instance*/
    if ((ret = cmsObj_addInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Failed to add PPP intf Instance, ret = %d", ret);
       return ret;
    } 
    
    /* Get PPP interface object */
    if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE, &iidStack, 0, (void **) &pppIntf)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Failed to get pppIntfObj, ret = %d", ret);
       cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
       return ret;
    }
      
    /* Get L2TP interface object */
    if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_L2TP, &iidStack, 0, (void **) &l2tpIntf)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get l2tpIntf, ret = %d", ret);
        cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
        cmsObj_free((void **) &pppIntf);    
        return ret;
    }
    
    /* Get PPPOE interface object if needed*/
    if (cmsUtl_strlen(webVar->pppServerName) > 0 )
    {
        Dev2PppInterfacePpoeObject *pppoeObj=NULL;
      
        /* Get PPPoE object */
        if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_PPOE, &iidStack, 0, (void **) &pppoeObj)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get pppoeObj, ret = %d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
            cmsObj_free((void **) &pppIntf);
            cmsObj_free((void **) &l2tpIntf);    
            return ret;
        }
   
        CMSMEM_REPLACE_STRING(pppoeObj->serviceName, webVar->pppServerName);
        
        /* Set PPPoE object */
         ret = cmsObj_set(pppoeObj,  &iidStack);
         cmsObj_free((void **) &pppoeObj);    
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set pppoeObj. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
            cmsObj_free((void **) &pppIntf);
            cmsObj_free((void **) &l2tpIntf);       
            return ret;      
         } 
	}
    
    CMSMEM_REPLACE_STRING(pppIntf->username, webVar->pppUserName);
    CMSMEM_REPLACE_STRING(pppIntf->password, webVar->pppPassword);
    CMSMEM_REPLACE_STRING(pppIntf->authenticationProtocol, cmsUtl_numToPppAuthString(webVar->pppAuthMethod));    

    /* get on demand ideltime out in seconds if it is enabled */
    if (webVar->enblOnDemand)
    {
       CMSMEM_REPLACE_STRING(pppIntf->connectionTrigger, MDMVS_ONDEMAND);
       pppIntf->idleDisconnectTime = webVar->pppTimeOut ;
    }
    else
    {
       /* 0 is no on demand feature */
       CMSMEM_REPLACE_STRING(pppIntf->connectionTrigger, MDMVS_ALWAYSON);
       pppIntf->idleDisconnectTime = 0;
    }
    
    
    /* get ppp debug flag */
    pppIntf->X_BROADCOM_COM_Enable_Debug = webVar->enblPppDebug;
    
    pppIntf->enable = TRUE;
    
    /* fill in service name */
    CMSMEM_REPLACE_STRING(pppIntf->name, "PPPoL2tpAc");   /* todo. just assign some thing for now */
    
    /* Set and Activate PPP interface */
    if ((ret = cmsObj_set(pppIntf, &iidStack)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Set pppIntf failed , ret = %d", ret);
       cmsObj_free((void **) &pppIntf);
    }
    else
    {
	   cmsLog_debug("cmsObj_set pppIntf ok.");
       cmsObj_free((void **) &pppIntf);

       /* enable the L2TPAC config object and set the tunnel name and Lns Ip Address after
       * WanPPPConn object is created since L2tpd needs ppp username/password info
       */
       CMSMEM_REPLACE_STRING(l2tpIntf->tunnelName, webVar->tunnelName);
       CMSMEM_REPLACE_STRING(l2tpIntf->lnsIpAddress, webVar->lnsIpAddress);
       l2tpIntf->enable = TRUE;

       /* Check if the new tunnelName has been used in the existed ppp/l2tp objects */
       while (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &tmpiidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&tmpPppIntf) == CMSRET_SUCCESS)
       {
          if (0 == cmsUtl_strcmp(tmpPppIntf->name, "PPPoL2tpAc"))
          {
             InstanceIdStack l2tpIidStack = EMPTY_INSTANCE_ID_STACK;
               
             if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP, 
                                                &tmpiidStack, 
                                                &l2tpIidStack,
                                                (void **) &tmpL2tpIntf)) != CMSRET_SUCCESS)
             {
                cmsLog_error("Failed to get l2tpObj, error=%d", ret);
             }
             else
             {
                if (0 == cmsUtl_strcmp(webVar->tunnelName, tmpL2tpIntf->tunnelName))
                {
                   /* Currently, xl2tpd support multi tunnels, but only one session on one tunnel. */
                   cmsLog_error("Tunnel Name should be unique");
                   cmsObj_free((void **)&tmpL2tpIntf);
                   cmsObj_free((void **)&tmpPppIntf);
                   cmsObj_free((void **) &l2tpIntf);
                   return CMSRET_INTERNAL_ERROR;
                }   
                cmsObj_free((void **)&tmpL2tpIntf); 
             }
          }
      
          cmsObj_free((void **)&tmpPppIntf);
       }	   
             
       if ((ret = cmsObj_set(l2tpIntf, &iidStack)) != CMSRET_SUCCESS)
       {
          cmsLog_error("Failed to set l2tpIntf object, ret = %d", ret);
          cmsObj_free((void **) &l2tpIntf);
       }
       else
       {         
          cmsLog_debug("Set l2tpIntf ok.");
          cmsObj_free((void **) &l2tpIntf);
       }
	}
   
    if (ret != CMSRET_SUCCESS)
    {
       cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
    }
   
    return ret;
}

CmsRet dalL2tpAc_deleteL2tpAcInterface_dev2(const WEB_NTWK_VAR *webVar)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2PppInterfaceObject *pppIntf=NULL;
    Dev2PppInterfaceL2tpObject *l2tpIntf=NULL;
    UBOOL8 found = FALSE;
    CmsRet ret = CMSRET_SUCCESS;
    
    cmsLog_debug("Deleting %s", webVar->tunnelName);

    while (!found &&
           (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **)&pppIntf) == CMSRET_SUCCESS))
   {
      if (0 == cmsUtl_strcmp(pppIntf->name, "PPPoL2tpAc"))
      {
         InstanceIdStack l2tpIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP, 
                                            &iidStack, 
                                            &l2tpIidStack,
                                            (void **) &l2tpIntf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get l2tpObj, error=%d", ret);
            cmsObj_free((void **)&pppIntf);
            return ret;
         }
         if (0 == cmsUtl_strcmp(webVar->tunnelName, l2tpIntf->tunnelName))
         {  
            found = TRUE;
            /* Delete L2TP interface */
            CMSMEM_FREE_BUF_AND_NULL_PTR(l2tpIntf->tunnelName);
            CMSMEM_FREE_BUF_AND_NULL_PTR(l2tpIntf->lnsIpAddress);
            CMSMEM_REPLACE_STRING(l2tpIntf->intfStatus, MDMVS_DOWN);
            l2tpIntf->enable = FALSE;
            if ((ret = cmsObj_set(l2tpIntf, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set l2tpIntf object, ret = %d", ret);
            }
            cmsObj_free((void **) &l2tpIntf);
            /* If the tunnel is removed, just delete the ppp interface */
            if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to delete dev2ppp Object, ret = %d", ret);
            }
         }
      }

      cmsObj_free((void **)&pppIntf);
   }
   
   if(!found)
   {
      cmsLog_error("Failed to find/remove tunnel: %s", webVar->tunnelName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;    	
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_2 */

