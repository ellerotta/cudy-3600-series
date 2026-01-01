/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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
#include "cms_util.h"
#include "cms_obj.h"
#include "mdm.h"
#include "odl.h"


static void dumpLockFailureInfo(MdmObjectId oid,
                                const InstanceIdStack *iidStack,
                                const char *func)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   char *fullPath=NULL;

   if (iidStack == NULL)
   {
      cmsLog_error("%s [oid=%d]", func, oid);
      return;
   }

   pathDesc.oid = oid;
   pathDesc.iidStack = *iidStack;
   if (cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath) == CMSRET_SUCCESS)
   {
      cmsLog_error("%s [oid=%d] %s", func, oid, fullPath);
      cmsMem_free(fullPath);
   } else {
      cmsLog_error("%s [oid=%d iidStack=%s]", func, oid, cmsMdm_dumpIidStack(iidStack));
   }
}


CmsRet cmsObj_get(MdmObjectId oid,
                  const InstanceIdStack *iidStack,
                  UINT32 getFlags,
                  void **mdmObj)
{
   void *internalMdmObj=NULL;
   CmsRet ret;

   /* do some sanity checking first */
   if (((getFlags == 0) && (iidStack == NULL)) ||
       ((getFlags != 0) && ((getFlags & (OGF_DEFAULT_VALUES|OGF_NO_VALUE_UPDATE)) == 0)))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("oid=%d flags=0x%x iidStack=%s",
                 oid, getFlags, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }


   if ((getFlags == 0) || (getFlags & OGF_NO_VALUE_UPDATE))
   {
      ret = odl_getObject(oid, iidStack, getFlags, &internalMdmObj);
   }
   else if (getFlags == OGF_DEFAULT_VALUES)
   {
      ret = odl_getDefaultObject(oid, &internalMdmObj);
   }
   else
   {
      ret = CMSRET_INVALID_ARGUMENTS;
   }

   if (CMSRET_SUCCESS == ret)
   {
      if (IS_EXTERNAL_CALLER)
      {
         /*
          * convert from internal object, which may be in shared mem, to
          * private heap memory for the external caller.
          */
         if (((*mdmObj) = mdm_dupObject(internalMdmObj, ALLOC_ZEROIZE)) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }

         /* always free the internal mdm object */
         mdm_freeObject(&internalMdmObj);
      }
      else
      {
         /* return the internal object directly to the caller */
         *mdmObj = internalMdmObj;
      }
   }

   lck_autoTrackMdmObj(*mdmObj);
   lck_autoUnlockZone(oid, __FUNCTION__);

   if (ret == CMSRET_SUCCESS)
      cmsLog_debug("returning obj[oid=%d] %p", oid, *mdmObj);

   return ret;
}


CmsRet cmsObj_getNext(MdmObjectId oid,
                      InstanceIdStack *iidStack,
                      void **mdmObj)
{
   InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;

   return (cmsObj_getNextInSubTreeFlags(oid, &parentIidStack, iidStack, 0, mdmObj));
}


CmsRet cmsObj_getNextFlags(MdmObjectId oid,
                           InstanceIdStack *iidStack,
                           UINT32 getFlags,
                           void **mdmObj)
{
   InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;

   return (cmsObj_getNextInSubTreeFlags(oid, &parentIidStack, iidStack, getFlags, mdmObj));
}


CmsRet cmsObj_getNextInSubTree(MdmObjectId oid,
                               const InstanceIdStack *parentIidStack,
                               InstanceIdStack *iidStack,
                               void **mdmObj)
{
   return (cmsObj_getNextInSubTreeFlags(oid, parentIidStack, iidStack, 0, mdmObj));
}


CmsRet cmsObj_getNextInSubTreeFlags(MdmObjectId oid,
                                    const InstanceIdStack *parentIidStack,
                                    InstanceIdStack *iidStack,
                                    UINT32 getFlags,
                                    void **mdmObj)
{
   void *internalMdmObj=NULL;
   CmsRet ret;

   cmsLog_debug("oid=%d flags=0x%x parentIidStack=%s iidStack=%s",
                 oid, getFlags,
                 cmsMdm_dumpIidStack(parentIidStack),
                 cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, parentIidStack, __FUNCTION__);
      return ret;
   }

   ret = odl_getNextObjectInSubTree(oid, parentIidStack, iidStack, getFlags, &internalMdmObj);
   if (CMSRET_SUCCESS == ret)
   {
      if (IS_EXTERNAL_CALLER)
      {
         /*
          * convert from internal object, which may be in shared mem, to
          * private heap memory for the external caller.
          */
         if (((*mdmObj) = mdm_dupObject(internalMdmObj, ALLOC_ZEROIZE)) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }

         /* always free the internal mdm object */
         mdm_freeObject(&internalMdmObj);
      }
      else
      {
         /* return the internal object directly to the internal caller */
         *mdmObj = internalMdmObj;
      }
   }
   else
   {
      *mdmObj = NULL;
   }

   lck_autoTrackMdmObj(*mdmObj);
   lck_autoUnlockZone(oid, __FUNCTION__);

   if (ret == CMSRET_SUCCESS)
      cmsLog_debug("returning obj[oid=%d] %p", oid, *mdmObj);

   return ret;
}


CmsRet cmsObj_getAncestor(MdmObjectId ancestorOid,
                          MdmObjectId decendentOid,
                          InstanceIdStack *iidStack,
                          void **mdmObj)
{
   return (cmsObj_getAncestorFlags(ancestorOid, decendentOid, iidStack, 0, mdmObj));
}


CmsRet cmsObj_getAncestorFlags(MdmObjectId ancestorOid,
                               MdmObjectId decendentOid,
                               InstanceIdStack *iidStack,
                               UINT32 getFlags,
                               void **mdmObj)
{
   void *internalMdmObj=NULL;
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   UINT8 ancestorZone, decendentZone;
   UINT32 totalZoneEntryCounts = 0;
   UINT32 ancFlags=0;
   UINT32 decFlags=0;
   CmsRet ret;

   cmsLog_debug("ancestorOid=%d descOid=%d flags=0x%x iidStack=%s",
                 ancestorOid, decendentOid, getFlags,
                 cmsMdm_dumpIidStack(iidStack));

   totalZoneEntryCounts = lck_getTotalZoneEntryCounts();
   ancestorZone = cmsLck_getLockZoneFlags(ancestorOid, GET_OBJNODE_ANY, &ancFlags);
   decendentZone = cmsLck_getLockZoneFlags(decendentOid, GET_OBJNODE_ANY, &decFlags);
   if (ancestorZone == MDM_INVALID_LOCK_ZONE)
   {
      cmsLog_error("Invalid access to ancestor oid %d", ancestorOid);
      return CMSRET_INVALID_ARGUMENTS;
   }
   else if (ancFlags & GET_OBJNODE_REMOTE)
   {
      // TODO: remote_locking?  For now, do nothing.
   }
   else
   {
      // Local MDM obj
      zones[ancestorZone] = TRUE;
   }
   
   if (decendentZone == MDM_INVALID_LOCK_ZONE)
   {
      cmsLog_error("Invalid access to decendent oid %d", decendentOid);
      return CMSRET_INVALID_ARGUMENTS;
   }
   else if (decFlags & GET_OBJNODE_REMOTE)
   {
      // TODO: remote_locking?  For now, do nothing.
   }
   else
   {
      // Local MDM obj
      zones[decendentZone] = TRUE;
   }

   // Grab locks (it is ok if zones[] is all FALSE)
   if ((ret = lck_autoLockZones(zones, decendentOid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(decendentOid, iidStack, __FUNCTION__);
      return ret;
   }

   ret = odl_getAncestorObject(ancestorOid, decendentOid, iidStack, getFlags, &internalMdmObj);
   if (CMSRET_SUCCESS == ret)
   {
      // Cannot use IS_EXTERNAL_CALLER here because we might have gotten 2
      // zone entry counts above (ancestorZone and decendentZone).  So use the
      // zone entry count prior to the auto locking to determine if we are
      // external caller.
      if (totalZoneEntryCounts == 0)
      {
         /*
          * convert from internal object, which may be in shared mem, to
          * private heap memory for the external caller.
          */
         if (((*mdmObj) = mdm_dupObject(internalMdmObj, ALLOC_ZEROIZE)) == NULL)
         {
            ret = CMSRET_RESOURCE_EXCEEDED;
         }

         /* always free the internal mdm object */
         mdm_freeObject(&internalMdmObj);
      }
      else
      {
         /* return the internal object directly to the internal caller */
         *mdmObj = internalMdmObj;
      }
   }

   lck_autoTrackMdmObj(*mdmObj);
   lck_autoUnlockZones(zones, decendentOid, __FUNCTION__);

   if (ret == CMSRET_SUCCESS)
      cmsLog_debug("returning obj[oid=%d] %p", ancestorOid, *mdmObj);

   return ret;
}


CmsRet cmsObj_getNthParam(const void *mdmObj,
                          const UINT32 paramNbr,
                          MdmObjParamInfo *paramInfo)
{
    MdmObjectId oid;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    CmsRet ret;

    if((mdmObj == NULL) || (paramInfo == NULL))
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    oid = *((MdmObjectId *) mdmObj);

    if((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
    {
        dumpLockFailureInfo(oid, NULL, __FUNCTION__);
        return ret;
    }

     objNode = mdm_getObjectNode(oid);
     if(objNode == NULL)
     {
         ret = CMSRET_INVALID_ARGUMENTS;
     }
     else if(paramNbr >= objNode->numParamNodes)
     {
         cmsLog_error("Invalid parameter number (%d >= %d)",
                      paramNbr, objNode->numParamNodes);

         ret = CMSRET_INVALID_ARGUMENTS;
     }

    if(ret != CMSRET_SUCCESS)
    {
        lck_autoUnlockZone(oid, __FUNCTION__);
        return ret;
    }

    paramNode = &(objNode->params[paramNbr]);

    paramInfo->totalParams = objNode->numParamNodes;
    strncpy(paramInfo->name, paramNode->name, sizeof(paramInfo->name)-1);
    paramInfo->name[sizeof(paramInfo->name)-1] = '\0';
    
    paramInfo->type = paramNode->type;
    paramInfo->minVal = (UINT32)(uintptr_t)paramNode->vData.min;
    paramInfo->maxVal = (UINT32)(uintptr_t)paramNode->vData.max;

    switch(paramNode->type)
    {
        case MPT_STRING:
        case MPT_DATE_TIME:
        case MPT_BASE64:
        case MPT_HEX_BINARY:
        case MPT_UUID:
        case MPT_IP_ADDR:
        case MPT_MAC_ADDR:
        case MPT_DECIMAL:
            {
                char **pMdmObj = (char **) mdmObj;
                paramInfo->val = pMdmObj[paramNode->offsetInObject/sizeof(char *)];
                paramInfo->minVal = 0;
                break;
            }

        case MPT_INTEGER:
        case MPT_LONG64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                SINT32 *pMdmObj = (SINT32 *) mdmObj;
                paramInfo->val = &pMdmObj[paramNode->offsetInObject/sizeof(SINT32)];
                break;
            }

        case MPT_UNSIGNED_INTEGER:
        case MPT_UNSIGNED_LONG64:
        case MPT_STATS_COUNTER32:
        case MPT_STATS_COUNTER64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                UINT32 *pMdmObj = (UINT32 *) mdmObj;
                paramInfo->val = &pMdmObj[paramNode->offsetInObject/sizeof(UINT32)];
                break;
            }

        case MPT_BOOLEAN:
            {
                UBOOL8 *pMdmObj = (UBOOL8 *) mdmObj;
                paramInfo->val = &pMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)];
                break;
            }

        default:
            {
                cmsLog_error("invalid type of paramNode, %s %d",
                             paramNode->name, paramNode->type);
                cmsAst_assert(0);
                ret = CMSRET_INTERNAL_ERROR;
                break;
            }

    } /* end of switch on paramNode->type */

    lck_autoUnlockZone(oid, __FUNCTION__);
    return ret;
}


void cmsObj_free(void **mdmObj)
{
   /*
    * If this is an external caller, a lock is not required because
    * mdm_freeObject only traverses the part of the MDM that does not change,
    * and caller only uses private heap memory.
    * If this is an internal caller, then we already have the lock.
    * So in both cases, we can call odl_freeObject.
    */
   if (mdmObj == NULL || (*mdmObj) == NULL)
   {
      return;
   }

   // MDM objects are tracked regardless of whether they are in the setQ or
   // not.  So when they are freed, we must untrack them.
   lck_autoUntrackMdmObj(*mdmObj);

   cmsLog_debug("freeing obj[oid=%d] %p", GET_MDM_OBJECT_ID(*mdmObj), *mdmObj);

   odl_freeObject(mdmObj);

   return;
}


CmsRet cmsObj_setFlags(const void *mdmObj,
                       const InstanceIdStack *iidStack,
                       UINT32 setFlags)

{
   MdmObjectId oid;
   CmsRet ret;

   if ((mdmObj == NULL) || (iidStack == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   oid = *((MdmObjectId *) mdmObj);

   cmsLog_debug("oid=%d flags=0x%x iidStack=%s",
                 oid, setFlags, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   ret = odl_setObject(mdmObj, iidStack, setFlags);

   if (!(IS_CMSRET_A_SUCCESS_VARIANT(ret)))
   {
      cmsLog_error("set of %s %s failed",
                   mdm_oidToGenericPath(oid),
                   cmsMdm_dumpIidStack(iidStack));
   }

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}


CmsRet cmsObj_set(const void *mdmObj, const InstanceIdStack *iidStack)
{
   return (cmsObj_setFlags(mdmObj, iidStack, OSF_NORMAL_SET));
}


CmsRet cmsObj_setNthParam(void *mdmObj,
                          const UINT32 paramNbr,
                          const void *paramVal)
{
    MdmObjectId oid;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    CmsRet ret;

    if((mdmObj == NULL) || (paramVal == NULL))
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    oid = *((MdmObjectId *) mdmObj);

    if((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
    {
        dumpLockFailureInfo(oid, NULL, __FUNCTION__);
        return ret;
    }

     objNode = mdm_getObjectNode(oid);
     if(objNode == NULL)
     {
         ret = CMSRET_INVALID_ARGUMENTS;
     }
     else if(paramNbr >= objNode->numParamNodes)
     {
         cmsLog_error("Invalid parameter number (%d >= %d)",
                      paramNbr, objNode->numParamNodes);

         ret = CMSRET_INVALID_ARGUMENTS;
     }

    if(ret != CMSRET_SUCCESS)
    {
        lck_autoUnlockZone(oid, __FUNCTION__);
        return ret;
    }

    paramNode = &(objNode->params[paramNbr]);

    switch(paramNode->type)
    {
        case MPT_STRING:
        case MPT_DATE_TIME:
        case MPT_BASE64:
        case MPT_HEX_BINARY:
        case MPT_UUID:
        case MPT_IP_ADDR:
        case MPT_MAC_ADDR:
        case MPT_DECIMAL:
            {
                char **pMdmObj = (char **) mdmObj;

                cmsMem_free(pMdmObj[paramNode->offsetInObject/sizeof(char *)]);
                pMdmObj[paramNode->offsetInObject/sizeof(char *)] = (char *)paramVal;
                break;
            }

        case MPT_INTEGER:
            {
                SINT32 *pMdmObj = (SINT32 *) mdmObj;
                pMdmObj[paramNode->offsetInObject/sizeof(SINT32)] = *((SINT32 *)paramVal);
                break;
            }

        case MPT_UNSIGNED_INTEGER:
        case MPT_STATS_COUNTER32:
            {
                UINT32 *pMdmObj = (UINT32 *) mdmObj;
                pMdmObj[paramNode->offsetInObject/sizeof(UINT32)] = *((UINT32 *)paramVal);
                break;
            }

        case MPT_LONG64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                SINT32 *pMdmObj = (SINT32 *) mdmObj;
                SINT64 *p64;

                p64 = (SINT64 *) &(pMdmObj[paramNode->offsetInObject/sizeof(SINT32)]);
                *p64 = *((SINT64 *)paramVal);
                break;
            }

        case MPT_UNSIGNED_LONG64:
        case MPT_STATS_COUNTER64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                UINT32 *pMdmObj = (UINT32 *) mdmObj;
                UINT64 *p64;

                p64 = (UINT64 *) &(pMdmObj[paramNode->offsetInObject/sizeof(UINT32)]);
                *p64 = *((UINT64 *)paramVal);
                break;
            }

        case MPT_BOOLEAN:
            {
                UBOOL8 *pMdmObj = (UBOOL8 *) mdmObj;
                pMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)] = *((UBOOL8 *)paramVal);
                break;
            }

        default:
            {
                cmsLog_error("invalid type of paramNode, %s %d",
                             paramNode->name, paramNode->type);
                cmsAst_assert(0);
                ret = CMSRET_INTERNAL_ERROR;
                break;
            }

    } /* end of switch on paramNode->type */

    lck_autoUnlockZone(oid, __FUNCTION__);
    return ret;
}


CmsRet cmsObj_addInstance(MdmObjectId oid,
                          InstanceIdStack *iidStack)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   if (iidStack == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("oid=%d iidStack=%s", oid, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = oid;
   pathDesc.iidStack = *iidStack;

   ret = odl_addObjectInstance(&pathDesc);
   if (IS_CMSRET_A_SUCCESS_VARIANT(ret))
   {
      *iidStack = pathDesc.iidStack;
   }

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}


CmsRet cmsObj_deleteInstance(MdmObjectId oid,
                             const InstanceIdStack *iidStack)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   if (iidStack == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("oid=%d iidStack=%s", oid, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = oid;
   pathDesc.iidStack = *iidStack;

   ret = odl_deleteObjectInstance(&pathDesc);

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}

/*sets a particular instance of an object as Non-persistent i.e
 * the instance is not written to config-file/flash its just maintained
 * in MDM,so the instance will not sustain reboots.
 */
CmsRet cmsObj_setNonpersistentInstance(const MdmObjectId oid,
                             const InstanceIdStack *iidStack)
{

   CmsRet ret;
   UINT16 flags=0;

   if (iidStack == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("oid=%d iidStack=%s", oid, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   if ((ret = odl_getInstanceFlags(oid, iidStack, &flags)) == CMSRET_SUCCESS)
   {
      flags = (flags | IDN_NON_PERSISTENT_INSTANCE);
      ret = odl_setInstanceFlags(oid, iidStack, flags);
   }

   lck_autoUnlockZone(oid, __FUNCTION__);
   return ret;
}

CmsRet cmsObj_clearNonpersistentInstance(const MdmObjectId oid,
                             const InstanceIdStack *iidStack)
{

   CmsRet ret;
   UINT16 flags=0;
   
   if (iidStack == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("oid=%d iidStack=%s", oid, cmsMdm_dumpIidStack(iidStack));

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   if ((ret = odl_getInstanceFlags(oid, iidStack, &flags)) == CMSRET_SUCCESS)
   {
      flags = (flags & (~IDN_NON_PERSISTENT_INSTANCE));
      ret = odl_setInstanceFlags(oid, iidStack, flags);
   }

   lck_autoUnlockZone(oid, __FUNCTION__);
   return ret;
}

UBOOL8 cmsObj_isNonpersistentInstance(const MdmObjectId oid,
                             const InstanceIdStack *iidStack)
{

   CmsRet ret;
   UINT16 flags=0;

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return FALSE;
   }

   ret = odl_getInstanceFlags(oid, iidStack, &flags);

   lck_autoUnlockZone(oid, __FUNCTION__);

   if((ret == CMSRET_SUCCESS) && (flags & IDN_NON_PERSISTENT_INSTANCE) )
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }

}

CmsRet cmsObj_clearStatistics(MdmObjectId oid,
                              const InstanceIdStack *iidStack)
{
   CmsRet ret;

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      dumpLockFailureInfo(oid, iidStack, __FUNCTION__);
      return ret;
   }

   ret = odl_clearStatistics(oid, iidStack);

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}

CmsRet cmsObj_dumpObjectLegacy(const void *mdmObj)
{
    MdmObjectId oid;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    CmsRet ret;
    void *pObjVal __attribute__ ((unused));
    UINT32 i;

    if(mdmObj == NULL)
    {
        cmsLog_error("NULL mdmObj!");
        return CMSRET_INVALID_ARGUMENTS;
    }

    oid = *((MdmObjectId *) mdmObj);

    if((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
    {
        dumpLockFailureInfo(oid, NULL, __FUNCTION__);
        return ret;
    }

    objNode = mdm_getObjectNode(oid);
    if(objNode == NULL)
    {
        cmsLog_error("Could not find Object node for oid %d", oid);
        lck_autoUnlockZone(oid, __FUNCTION__);
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_notice("Object       : %s, %d parameters",
                  objNode->name, objNode->numParamNodes);

    for(i=0; i<objNode->numParamNodes; i++)
    {
        paramNode = &(objNode->params[i]);

        cmsLog_notice("Parameter #%02d: %s", i, paramNode->name);

        switch(paramNode->type)
        {
            case MPT_STRING:
            case MPT_DATE_TIME:
            case MPT_BASE64:
            case MPT_HEX_BINARY:
            case MPT_UUID:
            case MPT_IP_ADDR:
            case MPT_MAC_ADDR:
            case MPT_DECIMAL:
                {
                    char **pMdmObj = (char **) mdmObj;
                    pObjVal = pMdmObj[paramNode->offsetInObject/sizeof(char *)];
                    cmsLog_notice("               value : '%s'", (char *)pObjVal);
                    break;
                }

            case MPT_INTEGER:
                {
                    SINT32 *pMdmObj = (SINT32 *) mdmObj;
                    pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(SINT32)];
                    cmsLog_notice("               value : %d (0x%08X)",
                                  *(SINT32 *)pObjVal, *(SINT32 *)pObjVal);
                    break;
                }

            case MPT_UNSIGNED_INTEGER:
            case MPT_STATS_COUNTER32:
                {
                    UINT32 *pMdmObj = (UINT32 *) mdmObj;
                    pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(UINT32)];
                    cmsLog_notice("               value : %d (0x%08X)",
                                  *(UINT32 *)pObjVal, *(UINT32 *)pObjVal);
                    break;
                }

            case MPT_BOOLEAN:
                {
                    UBOOL8 *pMdmObj = (UBOOL8 *) mdmObj;
                    pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)];
                    cmsLog_notice("               value : %d", *(UBOOL8 *)pObjVal);
                    break;
                }

            case MPT_LONG64:
            case MPT_UNSIGNED_LONG64:
            case MPT_STATS_COUNTER64:
                cmsLog_notice("FIXME: Add 64-bit print support\n");
                break;

            default:
                {
                    cmsLog_error("invalid type of paramNode, %s %d",
                                 paramNode->name, paramNode->type);
                    cmsAst_assert(0);
                    ret = CMSRET_INTERNAL_ERROR;
                    break;
                }
        }

        cmsLog_notice("               type  : %d", paramNode->type);
        cmsLog_notice("               min   : %" PRIuPTR, (uintptr_t)paramNode->vData.min);
        cmsLog_notice("               max   : %" PRIuPTR, (uintptr_t)paramNode->vData.max);
    } /* for */

    lck_autoUnlockZone(oid, __FUNCTION__);
    return CMSRET_SUCCESS;
}

#define CMSOBJ_MAX_STR_DUMP_SIZE 80
static char cmsObj_valStr[CMSOBJ_MAX_STR_DUMP_SIZE];

CmsRet cmsObj_dumpObject(const void *mdmObj)
{
    MdmObjectId oid;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    CmsRet ret;
    void *pObjVal;
    UINT32 i;

    if(mdmObj == NULL)
    {
        cmsLog_error("Null mdmObj!");
        return CMSRET_INVALID_ARGUMENTS;
    }

    oid = *((MdmObjectId *) mdmObj);

    if((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
    {
        dumpLockFailureInfo(oid, NULL, __FUNCTION__);
        return ret;
    }

    objNode = mdm_getObjectNode(oid);
    if(objNode == NULL)
    {
        cmsLog_error("Could not find object node for oid %d", oid);
        lck_autoUnlockZone(oid, __FUNCTION__);
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_notice("Object: name <%s>, parameters <%d>",
                  objNode->name, objNode->numParamNodes);

    for(i=0; i<objNode->numParamNodes; i++)
    {
        paramNode = &(objNode->params[i]);

        switch(paramNode->type)
        {
            case MPT_STRING:
            case MPT_DATE_TIME:
            case MPT_BASE64:
            case MPT_HEX_BINARY:
            case MPT_UUID:
            case MPT_IP_ADDR:
            case MPT_MAC_ADDR:
            case MPT_DECIMAL:
            {
                char **pMdmObj = (char **) mdmObj;
                pObjVal = pMdmObj[paramNode->offsetInObject/sizeof(char *)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<%s>", (char *)pObjVal);
                break;
            }

            case MPT_INTEGER:
            {
                SINT32 *pMdmObj = (SINT32 *) mdmObj;
                pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(SINT32)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<%d><0x%08X>",
                         *(SINT32 *)pObjVal, *(SINT32 *)pObjVal);
                break;
            }

            case MPT_UNSIGNED_INTEGER:
            case MPT_STATS_COUNTER32:
            {
                UINT32 *pMdmObj = (UINT32 *) mdmObj;
                pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(UINT32)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<%d><0x%08X>",
                         *(UINT32 *)pObjVal, *(UINT32 *)pObjVal);
                break;
            }

            case MPT_BOOLEAN:
            {
                UBOOL8 *pMdmObj = (UBOOL8 *) mdmObj;
                pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(UBOOL8)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<%s>",
                         (*(UBOOL8 *)pObjVal) ? "TRUE" : "FALSE");
                break;
            }

            case MPT_LONG64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                SINT32 *pMdmObj = (SINT32 *) mdmObj;
                pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(SINT32)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<0x%08X%08X>",
                         *(UINT32 *)pObjVal, *(((UINT32 *)(pObjVal))+1));
                break;
            }

            case MPT_UNSIGNED_LONG64:
            case MPT_STATS_COUNTER64:
            {
               /*
                * On 32 bit DESKTOP_LINUX, 64 bit type not aligned on 8 word boundaries,
                * so treat it somewhat like a 32 bit type.
                */
                UINT32 *pMdmObj = (UINT32 *) mdmObj;
                pObjVal = &pMdmObj[paramNode->offsetInObject/sizeof(UINT32)];
                snprintf(cmsObj_valStr, CMSOBJ_MAX_STR_DUMP_SIZE, "<0x%08X%08X>",
                         *(UINT32 *)pObjVal, *(((UINT32 *)(pObjVal))+1));
                break;
            }

            default:
            {
                cmsLog_error("invalid type of paramNode, %s %d",
                             paramNode->name, paramNode->type);
                cmsAst_assert(0);
                ret = CMSRET_INTERNAL_ERROR;
                break;
            }
        }

        cmsLog_notice("#%02d: name <%s>, value %s, type <%d>",
                      i, paramNode->name, cmsObj_valStr, paramNode->type);
        cmsLog_notice("   min=%" PRIuPTR, (uintptr_t)paramNode->vData.min);
        cmsLog_notice("   max=%" PRIuPTR, (uintptr_t)paramNode->vData.max);
    } /* for */

    lck_autoUnlockZone(oid, __FUNCTION__);
    return CMSRET_SUCCESS;
}


CmsRet cmsObj_compareObjects(const void *mdmObj1, const void *mdmObj2, paramNodeList **differedParamList)
{
    MdmObjectId oid;
    MdmObjectId oid2;
    MdmObjectNode *objNode;
    MdmParamNode *paramNode;
    CmsRet ret;
    void *pObjVal1, *pObjVal2;
    UINT32 i;

    if(mdmObj1 == NULL || mdmObj2 == NULL || differedParamList == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    *differedParamList = NULL;
    oid = *((MdmObjectId *) mdmObj1);
    oid2 = *((MdmObjectId *) mdmObj2);

    // Check these two mdmObjects are the same oid
    if (oid != oid2)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    if((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
    {
        dumpLockFailureInfo(oid, NULL, __FUNCTION__);
        return ret;
    }

    objNode = mdm_getObjectNode(oid);
    if (objNode == NULL)
    {
        cmsLog_error("Could not find Object node for oid %d", oid);
        lck_autoUnlockZone(oid, __FUNCTION__);
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_debug("Object: name <%s>, parameters <%d>",
                  objNode->name, objNode->numParamNodes);

    ret = CMSRET_SUCCESS;
    for(i=0; i<objNode->numParamNodes; i++)
    {
       paramNode = &(objNode->params[i]);

       switch(paramNode->type)
       {
          case MPT_STRING:
          case MPT_DATE_TIME:
          case MPT_BASE64:
          case MPT_HEX_BINARY:
          case MPT_UUID:
          case MPT_IP_ADDR:
          case MPT_MAC_ADDR:
          case MPT_DECIMAL:
          {
             char **pMdmObj1 = (char **) mdmObj1;
             char **pMdmObj2 = (char **) mdmObj2;
             pObjVal1 = pMdmObj1[paramNode->offsetInObject/sizeof(char *)];
             pObjVal2 = pMdmObj2[paramNode->offsetInObject/sizeof(char *)];
             if (pObjVal1 != NULL && pObjVal2 != NULL && strcmp(pObjVal1, pObjVal2) == 0)
                break;
             else if (pObjVal1 == NULL && pObjVal2 == NULL)
                break;
             else // find different value between this parameter
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                //differedNode->value = pObjVal1;
                //differedNode->size = pObjVal1 ? (strlen(pObjVal1)+1) : 0;
                if(pObjVal1 != NULL)
                   strncpy(differedNode->value,pObjVal1, strlen(pObjVal1)+1);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }
          case MPT_INTEGER:
          {
             SINT32 *pMdmObj1 = (SINT32 *) mdmObj1;
             SINT32 *pMdmObj2 = (SINT32 *) mdmObj2;
             pObjVal1 = &pMdmObj1[paramNode->offsetInObject/sizeof(SINT32)];
             pObjVal2 = &pMdmObj2[paramNode->offsetInObject/sizeof(SINT32)];
             if (*(SINT32 *)pObjVal1 != *(SINT32 *)pObjVal2)
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                //differedNode->value = pMdmObj1 + (paramNode->offsetInObject/sizeof(SINT32));
                //differedNode->size = sizeof(SINT32);
                sprintf(differedNode->value, "%d", *(SINT32*)pObjVal1);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }

          case MPT_UNSIGNED_INTEGER:
          case MPT_STATS_COUNTER32:
          {
             UINT32 *pMdmObj1 = (UINT32 *) mdmObj1;
             UINT32 *pMdmObj2 = (UINT32 *) mdmObj2;
             pObjVal1 = &pMdmObj1[paramNode->offsetInObject/sizeof(UINT32)];
             pObjVal2 = &pMdmObj2[paramNode->offsetInObject/sizeof(UINT32)];
             if (*(UINT32 *)pObjVal1 != *(UINT32 *)pObjVal2)
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                sprintf(differedNode->value, "%u", *(UINT32*)pObjVal1);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }

          case MPT_BOOLEAN:
          {
             UBOOL8 *pMdmObj1 = (UBOOL8 *) mdmObj1;
             UBOOL8 *pMdmObj2 = (UBOOL8 *) mdmObj2;
             pObjVal1 = &pMdmObj1[paramNode->offsetInObject/sizeof(UBOOL8)];
             pObjVal2 = &pMdmObj2[paramNode->offsetInObject/sizeof(UBOOL8)];
             if (*(UBOOL8 *)pObjVal1 != *(UBOOL8 *)pObjVal2)
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                //differedNode->value = pMdmObj1 + (paramNode->offsetInObject/sizeof(UBOOL8));
                //differedNode->size = sizeof(UBOOL8);
                sprintf(differedNode->value, "%d", (*(UBOOL8*)pObjVal1)?1:0);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }

          case MPT_LONG64:
          {
             SINT32 *pMdmObj1 = (SINT32 *) mdmObj1;
             SINT32 *pMdmObj2 = (SINT32 *) mdmObj2;
             pObjVal1 = &pMdmObj1[paramNode->offsetInObject/sizeof(SINT32)];
             pObjVal2 = &pMdmObj2[paramNode->offsetInObject/sizeof(SINT32)];
             if (*(SINT64 *)pObjVal1 != *(SINT64 *)pObjVal2)
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                sprintf(differedNode->value, "%" PRId64, *(SINT64*)pObjVal1);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }

          case MPT_UNSIGNED_LONG64:
          case MPT_STATS_COUNTER64:
          {
             UINT32 *pMdmObj1 = (UINT32 *) mdmObj1;
             UINT32 *pMdmObj2 = (UINT32 *) mdmObj2;
             pObjVal1 = &pMdmObj1[paramNode->offsetInObject/sizeof(UINT32)];
             pObjVal2 = &pMdmObj2[paramNode->offsetInObject/sizeof(UINT32)];
             if (*(UINT64 *)pObjVal1 != *(UINT64 *)pObjVal2)
             {
                struct paramNodeList *differedNode = cmsMem_alloc(sizeof(struct paramNodeList), ALLOC_ZEROIZE); ;
                differedNode->offset = paramNode->offsetInObject;
                sprintf(differedNode->value, "%" PRIu64, *(UINT64*)pObjVal1);
                differedNode->nextNode = *differedParamList;
                *differedParamList = differedNode;
             }
             break;
          }

          default:
          {
             cmsLog_error("invalid type of paramNode, %s, %d",
                   paramNode->name, paramNode->type);
             cmsAst_assert(0);
             ret = CMSRET_INTERNAL_ERROR;
             break;
          }
       } /* switch */
    } /* for */

    lck_autoUnlockZone(oid, __FUNCTION__);
    return ret;
}
