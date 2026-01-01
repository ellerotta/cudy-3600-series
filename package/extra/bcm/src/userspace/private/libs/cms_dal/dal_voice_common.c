/***********************************************************************
 *
 *
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom
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

/****************************************************************************
*
*  dal_voice_common.c
*
*  PURPOSE: Provide common interface to the DAL functions related to
*           voice configuration.
*
*  NOTES:
*
****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <sched.h>
#include <pthread.h>

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_cli.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "cms_qos.h"
#include "cms_net.h"
#include "dal_voice.h"
#include "cms_msg.h"
#include "rut_network.h"

#include <mdm.h>

#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)
static CmsRet dalVoice_performFilterOperation_igd( DAL_VOICE_PARMS *parms, DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk );
static CmsRet dalVoice_GetNetworkIntfList_igd( DAL_VOICE_PARMS *parms, char* intfList, unsigned int length );
#elif defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
static CmsRet dalVoice_performFilterOperation_dev2( DAL_VOICE_PARMS *parms, DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk );
static void addVoiceFirewallException_dev2(const char *ifName, const DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk);
static void deleteVoiceFirewallException_dev2(const char *ifName, const char *filterName);
static CmsRet dalVoice_GetNetworkIntfList_dev2( DAL_VOICE_PARMS *parms __attribute__((unused)), char *intfList, unsigned int length );
#endif

/*================================= Common Functions =========================================*/

CmsRet dalVoice_performFilterOperation( DAL_VOICE_PARMS *parms, DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk )
{
#if defined(SUPPORT_DM_LEGACY98)
   return  dalVoice_performFilterOperation_igd(parms, fwCtlBlk );
#elif defined(SUPPORT_DM_HYBRID)
   return  dalVoice_performFilterOperation_igd(parms, fwCtlBlk );
#elif defined(SUPPORT_DM_PURE181)
   return  dalVoice_performFilterOperation_dev2(parms, fwCtlBlk );
#elif defined(SUPPORT_DM_DETECT)
   if(cmsMdm_isDataModelDevice2())
   {
      return dalVoice_performFilterOperation_dev2(parms, fwCtlBlk );
   }
   else
   {
      return dalVoice_performFilterOperation_igd(parms, fwCtlBlk );
   }
#endif
   return CMSRET_SUCCESS;
}

CmsRet dalVoice_GetNetworkIntfList( DAL_VOICE_PARMS *parms, char* intfList, unsigned int length )
{
#if defined(SUPPORT_DM_LEGACY98)
   return dalVoice_GetNetworkIntfList_igd(parms, intfList, length);
#elif defined(SUPPORT_DM_HYBRID)
   return dalVoice_GetNetworkIntfList_igd(parms, intfList, length);
#elif defined(SUPPORT_DM_PURE181)
   return dalVoice_GetNetworkIntfList_dev2(parms, intfList, length);
#elif defined(SUPPORT_DM_DETECT)
   if (cmsMdm_isDataModelDevice2())
   {
      return dalVoice_GetNetworkIntfList_dev2(parms, intfList, length);
   }
   else
   {
      return dalVoice_GetNetworkIntfList_igd(parms, intfList, length);
   }
#endif
}

CmsRet dalVoice_getIpvxDnsServersFromIfName(UINT32 ipvx, const char *ifName, char *DNSServers)
{
   if (!cmsUtl_strcmp(ifName, MDMVS_LAN) || !cmsUtl_strcmp(ifName, MDMVS_ANY_WAN))
   {
      DNSServers[0] = '\0';
      return CMSRET_SUCCESS;
   }
   return rutNtwk_getIpvxDnsServersFromIfName(ipvx, ifName, DNSServers);
}

void dalVoice_removeUnprintableCharacters(char *inputStr)
{
   for (unsigned int i = 0; i < strlen(inputStr); i++)
   {
       if (!isprint(inputStr[i]))
       {
           inputStr[i] = '\0';
           cmsLog_error( "%s: non-printable character removed at position %d from returned %s string\n", __FUNCTION__, i, inputStr );
           return;
       }
    }
}

/*=== Common Nwk Interface/Firewall Helper Functions for TR98 IGD ====================*/

#ifdef DMP_BASELINE_1
/*
** Macro used to conveniently update the fields of exception object. This macro
** has been created because IP WAN and PPP WAN have seperate firewall exception
** objects but those objects have the exact same fields.
*/
#define SET_EXPCEPTION_OBJ_FIELDS(obj, fwCtlBlk) \
   obj->enable = fwCtlBlk->enable; \
   cmsMem_free(obj->filterName); \
   obj->filterName = cmsMem_strdup(fwCtlBlk->name); \
   cmsMem_free(obj->protocol); \
   obj->protocol = cmsMem_strdup(fwCtlBlk->protocol); \
   obj->sourcePortStart = obj->sourcePortEnd = fwCtlBlk->sourcePort; \
   obj->destinationPortStart = obj->destinationPortEnd = fwCtlBlk->destinationPort; \
   cmsMem_free(obj->IPVersion); \
   obj->IPVersion = cmsMem_strdup( (strchr(fwCtlBlk->sourceIPAddress, ':') == NULL) ? MDMVS_4 : MDMVS_6 ); \
   cmsMem_free(obj->sourceIPAddress); \
   obj->sourceIPAddress = cmsMem_strdup( (!strncmp(fwCtlBlk->sourceIPAddress, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                          !strncmp(fwCtlBlk->sourceIPAddress, ZERO_ADDRESS_IPV6, 2) \
                                          ? "" : fwCtlBlk->sourceIPAddress) ); \
   cmsMem_free(obj->sourceNetMask); \
   obj->sourceNetMask = cmsMem_strdup( (!strncmp(fwCtlBlk->sourceNetMask, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                          !strncmp(fwCtlBlk->sourceNetMask, ZERO_ADDRESS_IPV6, 2) \
                                          ? "" : fwCtlBlk->sourceNetMask) ); \
   cmsMem_free(obj->destinationIPAddress); \
   obj->destinationIPAddress = cmsMem_strdup( (!strncmp(fwCtlBlk->destinationIPAddress, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                               !strncmp(fwCtlBlk->destinationIPAddress, ZERO_ADDRESS_IPV6, 2) \
                                               ? "" : fwCtlBlk->destinationIPAddress) ); \
   cmsMem_free(obj->destinationNetMask); \
   obj->destinationNetMask = cmsMem_strdup( (!strncmp(fwCtlBlk->destinationNetMask, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                               !strncmp(fwCtlBlk->destinationNetMask, ZERO_ADDRESS_IPV6, 2) \
                                               ? "" : fwCtlBlk->destinationNetMask) )


/****************************************************************************
* FUNCTION: performFilterOperation
*
* PURPOSE:  Adds or deletes a filter to the firewall based on the value of the
*           enabled field in the firewall control block.
*
* PARAMETERS: iidStack - Instance ID stack of WAN object
*             fwCtlBlk - Firewall control block
*             oid      - ID of the exception object to be added/deleted
*             obj      - Address of the prointer the object. This field
*                        is updated when a new object is added to MDM
*
* RETURNS: CMSRET_SUCCESS when successful.
****************************************************************************/
static CmsRet performFilterOperation(InstanceIdStack *iidStack, DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk, MdmObjectId oid, void **obj)
{
   CmsRet ret;
   InstanceIdStack searchIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lastIidStack = EMPTY_INSTANCE_ID_STACK;

   /* Enable a rule means add a filter to the firewall */
   if ( fwCtlBlk->enable )
   {
      if ( (ret = cmsObj_addInstance(oid, iidStack)) != CMSRET_SUCCESS )
      {
         cmsLog_error("could not add Exception obj, ret=%d", ret);
         return ret;
      }

      ret = cmsObj_get(oid, iidStack, 0, obj);
   }
   /* Find the filter and remove it from the firewall */
   else
   {
      while ( (ret = cmsObj_getNextInSubTree(oid, iidStack, &searchIidStack, obj )) == CMSRET_SUCCESS )
      {
         /* Object is identified based on the filter name.
          * Compare FirewallExceptionObject filterNames only up to the length of cwCtlBlk's filterName.
          *  This is because in the case of the DELETE ALL operation, the substring of "VoiceFilter"
          *  is used for comparsion. If the object's filter name contains "VoiceFilter", this
          *  indicates that the filter should be deleted.
          */
         if( !strncmp((oid == MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION)
                         ? (*((WanIpConnFirewallExceptionObject**)obj))->filterName
                         : (*((WanPppConnFirewallExceptionObject**)obj))->filterName,
                      fwCtlBlk->name,
                      strlen(fwCtlBlk->name)) )
         {
            cmsObj_deleteInstance(oid, &searchIidStack);
            searchIidStack = lastIidStack;
         }
         else
         {
            lastIidStack = searchIidStack;
         }

         cmsObj_free((void**)obj);
      }
   }

   return (ret == CMSRET_NO_MORE_INSTANCES) ? CMSRET_SUCCESS : ret;
}

/****************************************************************************
* FUNCTION: dalVoice_performFilterOperation
*
* PURPOSE:  Adds or deletes a filter to the firewall based on the WAN interface
*           used for voice.
*
* PARAMETERS: parms    - Not used
*             fwCtlBlk - Firewall control block. Contains information about the
*                        filter.
*
* RETURNS: CMSRET_SUCCESS - Success
*          other failed, check with reason code
*
* NOTE:   This function is both a get and set function and if relies on the
*       boundIfName object to determine the WAN object to which the filter
*       should be added or deleted.
*
****************************************************************************/
static CmsRet dalVoice_performFilterOperation_igd( DAL_VOICE_PARMS *parms __attribute__((unused)),
                                       DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk )
{
   CmsRet ret;
   InstanceIdStack iidStack;
   WanIpConnObject  *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
   WanIpConnFirewallExceptionObject *ipExpObj = NULL;
   WanPppConnFirewallExceptionObject *pppExpObj = NULL;

   cmsLog_notice("Entered: boundIfName=%s enable=%d filtername=%s",
                 fwCtlBlk->intfName, fwCtlBlk->enable, fwCtlBlk->name);

   /* Check for IP WAN interfaces */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ( (ret = cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void**)&ipConn )) == CMSRET_SUCCESS )
   {
      if( ipConn->X_BROADCOM_COM_IfName != NULL &&
          !(cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_ROUTED)) &&
          (!strncmp(ipConn->X_BROADCOM_COM_IfName, fwCtlBlk->intfName, strlen(fwCtlBlk->intfName)) ||
           !strncmp(fwCtlBlk->intfName, MDMVS_ANY_WAN, strlen(fwCtlBlk->intfName))) )
      {
         cmsObj_free((void**)&ipConn);
         if ( (ret = performFilterOperation(&iidStack, fwCtlBlk,
                        MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION, (void**)&ipExpObj)) != CMSRET_SUCCESS )
         {
            cmsLog_error( "Can't perform firewall operation = %d\n", ret );
            return ret;
         }

         /* Update object only if a filter is being added */
         if ( fwCtlBlk->enable )
         {
            /* Populate firewall exception object and update MDM */
            SET_EXPCEPTION_OBJ_FIELDS(ipExpObj, fwCtlBlk);
            if ( ( ret = cmsObj_set(ipExpObj, &iidStack)) != CMSRET_SUCCESS )
            {
               /* Setting values failed, must delete previously created ipExpObj */
               cmsObj_deleteInstance(MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION, &iidStack);
               cmsLog_error( "Can't set IP firewall exception object = %d\n", ret );
            }
         }

         cmsObj_free((void**)&ipExpObj);
         return ret;
      }

      cmsObj_free((void**)&ipConn);
   }

   if( ret != CMSRET_NO_MORE_INSTANCES )
   {
      cmsLog_error("Failed to get IP WAN Interface object");
      return ret;
   }

   /* Check for PPP WAN interfaces */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ( (ret = cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void**)&pppConn )) == CMSRET_SUCCESS )
   {
      if(pppConn->X_BROADCOM_COM_IfName != NULL && !(cmsUtl_strcmp(pppConn->connectionType, MDMVS_IP_ROUTED)) &&
         (!strncmp(pppConn->X_BROADCOM_COM_IfName, fwCtlBlk->intfName, strlen(pppConn->X_BROADCOM_COM_IfName)) ||
          !strncmp(fwCtlBlk->intfName, MDMVS_ANY_WAN, strlen(fwCtlBlk->intfName))) )
      {
         cmsObj_free((void**)&pppConn);
         if ( (ret = performFilterOperation(&iidStack, fwCtlBlk,
                        MDMOID_WAN_PPP_CONN_FIREWALL_EXCEPTION, (void**)&pppExpObj)) != CMSRET_SUCCESS )
         {
            cmsLog_error( "Can't perform firewall operation = %d\n", ret );
            return ret;
         }

         /* Update object only if a filter is being added */
         if ( fwCtlBlk->enable )
         {
            /* Populate firewall exception object and update MDM */
            SET_EXPCEPTION_OBJ_FIELDS(pppExpObj, fwCtlBlk);
            if ( ( ret = cmsObj_set(pppExpObj,  &iidStack)) != CMSRET_SUCCESS )
            {
               /* Setting values failed, must delete previously created pppExpObj */
               cmsObj_deleteInstance(MDMOID_WAN_PPP_CONN_FIREWALL_EXCEPTION, &iidStack);
               cmsLog_error( "Can't set PPP firewall exception object = %d\n", ret );
            }
         }

         cmsObj_free((void**)&pppExpObj);
         return ret;
      }

      cmsObj_free((void**)&pppConn);
   }

   if( ret != CMSRET_NO_MORE_INSTANCES )
   {
      cmsLog_error("Failed to get PPP WAN Interface object");
      return ret;
   }
   return ( ret );
}

/****************************************************************************
* FUNCTION:    dalVoice_GetNetworkIntfList
*
* PURPOSE:     Get list of available network instances
*
* PARAMETERS:  None
*
* RETURNS:     Supported signalling protocols
*
* NOTE:
*
****************************************************************************/
static CmsRet dalVoice_GetNetworkIntfList_igd( DAL_VOICE_PARMS *parms, char* intfList, unsigned int length )
{
   WanPppConnObject *pppConn=NULL;
   WanIpConnObject *ipConn=NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   char tmp[20];

   /* Add the default interfaces */
   snprintf(intfList, length, "%s %s",MDMVS_LAN, MDMVS_ANY_WAN);

   /* Add WAN ip interfaces */
   while( (ret = cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack1, (void **) &ipConn)) == CMSRET_SUCCESS )
   {
      cmsLog_debug("WAN IP CONNECTION INTERFACE=%s", ipConn->X_BROADCOM_COM_IfName);
      if( ( !(cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_ROUTED)) || !(cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED)) )
           && ipConn->X_BROADCOM_COM_IfName != NULL )
      {
         sprintf(tmp, " %s", ipConn->X_BROADCOM_COM_IfName);
         strncat(intfList, tmp, length);
      }

      cmsObj_free((void **) &ipConn);
   }

   if( ret != CMSRET_NO_MORE_INSTANCES )
   {
      cmsLog_error("failed to get MDMOID_WAN_IP_CONN object ret=%d", ret);
      return ret;
   }

   /* Add WAN ppp interfces */
   while( (ret = cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack2, (void **) &pppConn)) == CMSRET_SUCCESS )
   {
      cmsLog_debug("WAN PPP CONNECTION INTERFACE=%s", pppConn->X_BROADCOM_COM_IfName);
      if( !(cmsUtl_strcmp(pppConn->connectionType, MDMVS_IP_ROUTED)) && pppConn->X_BROADCOM_COM_IfName != NULL )
      {
         sprintf(tmp, " %s", pppConn->X_BROADCOM_COM_IfName);
         strncat(intfList, tmp, length);
      }

      cmsObj_free((void **) &pppConn);
   }

   if( ret != CMSRET_NO_MORE_INSTANCES )
   {
      cmsLog_error("failed to get MDMOID_WAN_PPP_CONN object ret=%d", ret);
   }

   return CMSRET_SUCCESS;

}

#endif  /* DMP_BASELINE_1 */

/*=== Common Nwk Interface/Firewall Helper Functions for TR143 ===============*/

#ifdef DMP_DEVICE2_BASELINE_1
/*
** Macro used to conveniently update the fields of exception object. This macro
** has been created because IP WAN and PPP WAN have seperate firewall exception
** objects but those objects have the exact same fields.
*/

#define SET_EXPCEPTION_OBJ_FIELDS(obj, fwCtlBlk) \
   obj->enable = fwCtlBlk->enable; \
   cmsMem_free(obj->filterName); \
   obj->filterName = cmsMem_strdup(fwCtlBlk->name); \
   cmsMem_free(obj->protocol); \
   obj->protocol = cmsMem_strdup(fwCtlBlk->protocol); \
   obj->sourcePortStart = obj->sourcePortEnd = fwCtlBlk->sourcePort; \
   obj->destinationPortStart = obj->destinationPortEnd = fwCtlBlk->destinationPort; \
   cmsMem_free(obj->IPVersion); \
   obj->IPVersion = cmsMem_strdup( (strchr(fwCtlBlk->sourceIPAddress, ':') == NULL) ? MDMVS_4 : MDMVS_6 ); \
   cmsMem_free(obj->sourceIPAddress); \
   obj->sourceIPAddress = cmsMem_strdup( (!strncmp(fwCtlBlk->sourceIPAddress, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                          !strncmp(fwCtlBlk->sourceIPAddress, ZERO_ADDRESS_IPV6, 2) \
                                          ? "" : fwCtlBlk->sourceIPAddress) ); \
   cmsMem_free(obj->sourceNetMask); \
   obj->sourceNetMask = cmsMem_strdup( (!strncmp(fwCtlBlk->sourceNetMask, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                          !strncmp(fwCtlBlk->sourceNetMask, ZERO_ADDRESS_IPV6, 2) \
                                          ? "" : fwCtlBlk->sourceNetMask) ); \
   cmsMem_free(obj->destinationIPAddress); \
   obj->destinationIPAddress = cmsMem_strdup( (!strncmp(fwCtlBlk->destinationIPAddress, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                               !strncmp(fwCtlBlk->destinationIPAddress, ZERO_ADDRESS_IPV6, 2) \
                                               ? "" : fwCtlBlk->destinationIPAddress) ); \
   cmsMem_free(obj->destinationNetMask); \
   obj->destinationNetMask = cmsMem_strdup( (!strncmp(fwCtlBlk->destinationNetMask, ZERO_ADDRESS_IPV4, sizeof(ZERO_ADDRESS_IPV4)) || \
                                               !strncmp(fwCtlBlk->destinationNetMask, ZERO_ADDRESS_IPV6, 2) \
                                               ? "" : fwCtlBlk->destinationNetMask) )

/*****************************************************************
**  FUNCTION:       dalVoice_GetNetworkIntfList_dev2
**
**  PURPOSE:        Obtains the list of available network interfaces
**
**  INPUT PARMS:    None
**
**  OUTPUT PARMS:   intfList - obtained list of network interfaces
**
**  RETURNS:        CMSRET_SUCCESS - Success
**
**  NOTE:
*******************************************************************/
static CmsRet dalVoice_GetNetworkIntfList_dev2( DAL_VOICE_PARMS *parms __attribute__((unused)),
                                         char *intfList, unsigned int length )
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   /* Add the default interfaces */
   snprintf(intfList, length, "%s %s", MDMVS_LAN, MDMVS_ANY_WAN);

   /*
    * Append all non bridged WAN interface.  Why does TR98 code also add
    * bridged interfaces?  The voice code in ssk will not start voice on a
    * bridged WAN connection.
    */
   while (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream &&
          !ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
         cmsUtl_strncat(intfList, length, " ");
         cmsUtl_strncat(intfList, length, ipIntfObj->name);
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   return CMSRET_SUCCESS;
}

/* helper function dalVoice_performFilterOperation_dev2 */
static CmsRet dalVoice_performFilterOperation_dev2( DAL_VOICE_PARMS *parms,
                                       DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk )
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_notice("Entered: boundIfName=%s enable=%d filtername=%s",
                 fwCtlBlk->intfName, fwCtlBlk->enable, fwCtlBlk->name);

   /* loop through all WAN, non-bridged, and UP IP.Interface and call helper
    * functions for each matching IP.Interface
    */
   while (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream &&
          !ipIntfObj->X_BROADCOM_COM_BridgeService &&
          !cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP) &&
          (  !cmsUtl_strcmp(fwCtlBlk->intfName, MDMVS_ANY_WAN) ||
             !cmsUtl_strncmp(fwCtlBlk->intfName, ipIntfObj->name, strlen(fwCtlBlk->intfName)))  )
      {
         if (fwCtlBlk->enable)
         {
            addVoiceFirewallException_dev2(ipIntfObj->name, fwCtlBlk);
         }
         else
         {
            deleteVoiceFirewallException_dev2(ipIntfObj->name, fwCtlBlk->name);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   return CMSRET_SUCCESS;
}


void addVoiceFirewallException_dev2(const char *ifName,
                                    const DAL_VOICE_FIREWALL_CTL_BLK *fwCtlBlk)
{
   char srcPortBuf[BUFLEN_32]={0};
   char dstPortBuf[BUFLEN_32]={0};
   CmsRet ret;

   cmsLog_debug("Entered: ifName=%s fwCtlBlk->name=%s",
                ifName, fwCtlBlk->name);

   sprintf(srcPortBuf, "%d", fwCtlBlk->sourcePort);
   sprintf(dstPortBuf, "%d", fwCtlBlk->destinationPort);


   /*
    * Everybody should just use the DAL function to add exception.  But
    * to avoid regression, I left the original TR98 function and its
    * SET_EXPCEPTION_OBJ_FIELDS macro alone.  The logic for extracting values
    * from fwCtlBlk is the same as that macro.
    */
   ret = dalSec_addIpFilterIn_dev2(fwCtlBlk->name,
               ((strchr(fwCtlBlk->sourceIPAddress, ':') == NULL) ?
                                                        MDMVS_4 : MDMVS_6),
               fwCtlBlk->protocol,
               (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX,
                                         fwCtlBlk->sourceIPAddress) ?
                                         "" : fwCtlBlk->sourceIPAddress),
               (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX,
                                         fwCtlBlk->sourceNetMask) ?
                                         "" : fwCtlBlk->sourceNetMask),
               srcPortBuf,
               (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX,
                                         fwCtlBlk->destinationIPAddress) ?
                                         "" : fwCtlBlk->destinationIPAddress),
               (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX,
                                         fwCtlBlk->destinationNetMask) ?
                                         "" : fwCtlBlk->destinationNetMask),
               dstPortBuf,
               ifName);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("dalSec_addIpFilterIn_dev2 failed, ret=%d", ret);
   }

   return;
}


void deleteVoiceFirewallException_dev2(const char *ifName,
                                       const char *filterName)
{
   UBOOL8 found=FALSE;
   char *ipIntfFullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2FirewallExceptionObject *fwExceptionObj=NULL;

   if (qdmIntf_intfnameToFullPathLocked(ifName, FALSE, &ipIntfFullPath) != CMSRET_SUCCESS)
   {
         cmsLog_error("cannot get %s's full path", ifName);
         return;
   }
   cmsLog_debug("Entered: ipIntfFullPath=%s filterName=%s",
                ipIntfFullPath, filterName);


   /* loop through all firewall exceptions and delete the exception under
    * the matching ipIntfFullPath.
    */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_FIREWALL_EXCEPTION, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&fwExceptionObj) == CMSRET_SUCCESS))
   {
      if(!cmsUtl_strcmp(fwExceptionObj->IPInterface, ipIntfFullPath))
      {
         InstanceIdStack searchIidStack = EMPTY_INSTANCE_ID_STACK;
         InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
         Dev2FirewallExceptionRuleObject *fwExRuleObj = NULL;

         found = TRUE;

         while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_FIREWALL_EXCEPTION_RULE,
                                     &iidStack, &searchIidStack, 0,
                                     (void **) &fwExRuleObj) == CMSRET_SUCCESS)
         {
            /* See comment in performFilterOperation in dal_voice.c */
            if (!cmsUtl_strncmp(fwExRuleObj->filterName, filterName, strlen(filterName)))
            {
               cmsObj_deleteInstance(MDMOID_DEV2_FIREWALL_EXCEPTION_RULE,
                                     &searchIidStack);

               /* since we deleted this instance, restore searchIidStack to last good instance */
               searchIidStack = savedIidStack;
            }

            savedIidStack = searchIidStack;
            cmsObj_free((void **) &fwExRuleObj);
         }
      }

      cmsObj_free((void **) &fwExceptionObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);

   return;
}

#endif /* DMP_DEVICE2_BASELINE_1 */
