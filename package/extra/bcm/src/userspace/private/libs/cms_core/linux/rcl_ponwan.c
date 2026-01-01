/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
#include "rut_rpsrfs.h"
#include "rdpactl_api.h"
#endif

/*!\file rcl_ponwan.c
 * \brief This file contains pon WAN interface object related functions.
 *
 */

#ifdef DMP_X_BROADCOM_COM_PONWAN_1


CmsRet rcl_wanPonIntfObject( _WanPonIntfObject *newObj,
                const _WanPonIntfObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{

   CmsRet ret = CMSRET_SUCCESS;   

   if (ADD_NEW(newObj, currObj) && newObj->enable)
   {
      /*
       * This will happen at bootup time.  Do we need to do any thing for any pon interface later on ?.
       * Currently for gpon, no action needed.
       *
       * ret = rutPon_configPonIntf(newObj); 
       */
   }

   else if (ENABLE_EXISTING(newObj, currObj))
   {
      /* enable existing pon intf.  Currently for gpon, no action needed.
      *
      *   ret = rutPon_enablePonIntf(newObj); 
      */
   }
   
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* disable or delete existing pon intf.  Currently for gpon, no action needed.
      *
      * if ( currObj->enable )
      *   ret = rutPon_disablePonIntf(newObj); 
      */   
   }
   
#ifdef DMP_X_BROADCOM_COM_RPSRFS_1
   if (STATUS_CHANGE(newObj, currObj))
   {
      if ((cmsUtl_strcmp(newObj->status, MDMVS_UP) == 0) && newObj->enable)
      {
         if (cmsUtl_strcmp(newObj->ponType, MDMVS_EPON) == 0)
         {
            char epon_port_name[CMS_IFNAME_LENGTH];
            if (rdpactl_has_rdpa_port_type_epon(epon_port_name))
            {
               rutRpsRfs_checkSet(epon_port_name);
            }
            else
            {
               cmsLog_error("Could not find epon port");
            }
         }
		
         if (cmsUtl_strcmp(newObj->ponType, MDMVS_GPON) == 0)
         {
            char gpon_port_name[CMS_IFNAME_LENGTH];
            if (rdpactl_has_rdpa_port_type_gpon(gpon_port_name))
            {
               rutRpsRfs_checkSet(gpon_port_name);
            }
            else
            {
               cmsLog_error("Could not find gpon port");
            }
         }
      }
   }
#endif   /* DMP_X_BROADCOM_COM_RPSRFS_1 */

   return ret;
}



CmsRet stl_wanPonIntfObject( _WanPonIntfObject *newObj __attribute__((unused)),
                const _WanPonIntfObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a status object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanPonIntfStatusObject( _WanPonIntfStatusObject *newObj __attribute__((unused)),
                const _WanPonIntfStatusObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a status object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


CmsRet stl_wanPonIntfStatusObject( _WanPonIntfStatusObject *newObj __attribute__((unused)),
                const _WanPonIntfStatusObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a stats object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


#endif  /* DMP_X_BROADCOM_COM_PONWAN_1 */

