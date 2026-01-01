/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_core.h"


CmsRet qdmXmpp_getJabberIdLocked_dev2(const InstanceIdStack *iidStack, char *jabberID)
{
   Dev2XmppConnObject *xmppConn = NULL;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_XMPP_CONN,iidStack,0,(void **) &xmppConn)) == CMSRET_SUCCESS)
   {
      cmsUtl_generateJabberId(xmppConn->username,
                              xmppConn->domain,
                              xmppConn->resource,
                              jabberID);
      cmsObj_free((void**)&xmppConn);
   }
   return (ret);
}

int qdmXmpp_getXmppConnectionStatusLocked_dev2(const char *fullPath)
{
   MdmPathDescriptor pathDesc;
   PhlGetParamValue_t   *pParamValue = NULL;
   int ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      // invalid XMPP connection instance fullpath
      cmsLog_error("Bad fullpath %s", fullPath);
      return XMPP_NONE;
   }

   if (pathDesc.oid != MDMOID_DEV2_XMPP_CONN)
   {
      cmsLog_error("fullpath must point to Device.XMPP.Connection.{i}.");
      return XMPP_NONE;
   }

   sprintf(pathDesc.paramName, "Status");


   ret = cmsPhl_getParamValue(&pathDesc, &pParamValue);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsPhl_getParamValue error: %d", ret);
      return XMPP_NONE;
   }
   else
   {
      if (cmsUtl_strlen(pParamValue->pValue) > 0)
      {
         if (cmsUtl_strcmp(pParamValue->pValue, "Up") == 0)
            ret = XMPP_STATUS_UP;
         else
            ret = XMPP_STATUS_DOWN;
      }
      else
         ret = XMPP_STATUS_UP;
      cmsPhl_freeGetParamValueBuf(pParamValue, 1);
   }
   return ret;
}

