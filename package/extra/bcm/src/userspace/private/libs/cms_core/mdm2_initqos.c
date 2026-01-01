/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"

#ifdef DMP_DEVICE2_QOS_1
CmsRet mdmInit_addQosQueue_dev2(UINT32 precedence, UINT32 qid,
                                const char *fullPath, const char *qname,
                                UBOOL8 enable)
{
   MdmPathDescriptor  pathDesc;
   Dev2QosObject      *mdmObj = NULL;
   Dev2QosQueueObject *qObj   = NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_QOS_QUEUE;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance returns error. ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&qObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }

    qObj->enable                 = enable;
    qObj->precedence             = precedence;
    qObj->X_BROADCOM_COM_QueueId = qid;
    CMSMEM_REPLACE_STRING_FLAGS(qObj->interface, fullPath, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(qObj->X_BROADCOM_COM_QueueName, qname, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **)&qObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
   }

   mdm_freeObject((void **)&qObj);

   /* increment the queue count */   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_QOS;

   /* get the queue management object */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&mdmObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }
   
   mdmObj->queueNumberOfEntries++;
   if ((ret = mdm_setObject((void **)&mdmObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
   }

   mdm_freeObject((void **)&mdmObj);
   return ret;
}
#endif  /* DMP_DEVICE2_QOS_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
