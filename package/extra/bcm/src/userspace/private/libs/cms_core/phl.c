/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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
#include "cms_phl.h"
#include "cms_obj.h"
#include "cms_mdm.h"
#include "cms_mem.h"
#include "cms_log.h"
#include "cms_util.h"
#include "mdm.h"
#include "odl.h"
#include "remote.h"
#include "phl_ene.h"
#include "phl_merge.h"
#include "bcm_generic_hal_defs.h"
#include "bcm_generic_hal_utils.h"


/*
 * CMS PHL API is used by TR69.  Use a longer timeout to increase chance of
 * getting a lock and also to enable backoff-retry when we lock all zones.
 */
#define PHL_LOCK_TIMEOUT  (CMSLCK_MAX_HOLDTIME * 2)

#define PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(o) {\
   if (phl_isAllZones(zones)) rc = lck_autoLockAllZonesWithBackoff(\
                                     o, PHL_LOCK_TIMEOUT, __FUNCTION__);\
   else rc = lck_autoLockZones(zones, o, __FUNCTION__); \
   if (rc != CMSRET_SUCCESS) return rc; }

static UBOOL8 phl_isAllZones(const UBOOL8 *zones)
{
   UINT32 i;
   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      if (zones[i] == FALSE)
      {
         return FALSE;
      }
   }
   // All zones are marked TRUE
   return TRUE;
}

static CmsRet phl_fillLockZones(const MdmPathDescriptor *pathDescs,
                                UINT32 numPathDescs, UBOOL8 nextLevelOnly,
                                UBOOL8 *zones)
{
    UINT32 i;
    UBOOL8 isContained=FALSE;
    UINT8 zone;

    memset(zones, FALSE, MDM_MAX_LOCK_ZONES);

    for (i=0; i < numPathDescs; i++)
    {
      zone = cmsLck_getLockZoneFlags(pathDescs[i].oid, GET_OBJNODE_LOCAL, NULL);
      if (zone == MDM_INVALID_LOCK_ZONE)
      {
         // This is a remote or unknown object.  From a zone lock
         // point of view, just ignore it.  (If truly a bad OID, let upper
         // layers deal with it).
         continue;
      }

      // If the pathDesc contains a parameter name, it is an operation on that
      // parameter, will not go into other objects or zones, so it is "contained".
      // And if the operation is nextLevelOnly, it is also "contained".
      if (IS_PARAM_NAME_PRESENT(&(pathDescs[i])) || nextLevelOnly)
      {
         isContained = TRUE;
      }

      // !isContained means the operation can go beyond the specified pathDesc.
      if (!isContained && cmsLck_isTopLevelLockZone(zone))
      {
         // To be conservative, if a non-contained operation starts from a
         // top level zone, meaning it can walk into other zones, just lock
         // all zones.
         memset(zones, TRUE, MDM_MAX_LOCK_ZONES);

         /* For slower platforms, doing operation from root DM takes a little longer */
         cmsLck_setHoldTimeWarnThresh(CMSLCK_MAX_HOLDTIME * 2);

         return CMSRET_SUCCESS;
      }
      zones[zone] = TRUE;
   }

   return CMSRET_SUCCESS;
}

static CmsRet phl_fillLockZonesSetParamValue(const PhlSetParamValue_t *paramValues,
                                          UINT32 numParamValues, UBOOL8 *zones)
{
   MdmPathDescriptor *pathDescs;
   UINT32 i;
   UINT32 count=0;
   UBOOL8 nextLevelOnly = TRUE;  // setParamValue will never go into a subtree
   CmsRet ret;

   // Allocate an array of pathDesc and copy the pathDesc from paramValues
   pathDescs = cmsMem_alloc(sizeof(MdmPathDescriptor) * numParamValues, ALLOC_ZEROIZE);
   if (pathDescs == NULL)
   {
      cmsLog_error("Could not allocate %d pathDescs", numParamValues);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamValues; i++)
   {
      // tr69c will detect invalid param name and set error status
      // but expects to execute the rest of the PHL code to handle it.
      if (paramValues[i].status == CMSRET_SUCCESS)
      {
         memcpy((void *) &(pathDescs[count]), (void *) &(paramValues[i].pathDesc),
                sizeof(MdmPathDescriptor));
         count++;
      }
   }

   ret = phl_fillLockZones(pathDescs, count, nextLevelOnly, zones);

   // free the locally allocated pathDesc array
   cmsMem_free(pathDescs);

   return ret;
}

static CmsRet phl_fillLockZonesSetParamAttr(const PhlSetParamAttr_t *paramAttrs,
                                          UINT32 numParamAttrs, UBOOL8 *zones)
{
   MdmPathDescriptor *pathDescs;
   UINT32 i;
   UBOOL8 nextLevelOnly = FALSE;  // setParamAttrs will go into subtree if pathDesc is an object
   CmsRet ret;

   // Allocate an array of pathDesc and copy the pathDesc from paramAttrs
   pathDescs = cmsMem_alloc(sizeof(MdmPathDescriptor) * numParamAttrs, ALLOC_ZEROIZE);
   if (pathDescs == NULL)
   {
      cmsLog_error("Could not allocate %d pathDescs", numParamAttrs);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamAttrs; i++)
   {
      memcpy((void *) &(pathDescs[i]), (void *) &(paramAttrs[i].pathDesc),
              sizeof(MdmPathDescriptor));
   }

   ret = phl_fillLockZones(pathDescs, numParamAttrs, nextLevelOnly, zones);

   // free the locally allocated pathDesc array
   cmsMem_free(pathDescs);

   return ret;
}


static UBOOL8 phl_sameObjDescs(const MdmPathDescriptor *pPathDesc1,
                               const MdmPathDescriptor *pPathDesc2)
{
   return ((pPathDesc1->oid == pPathDesc2->oid) &&
           (cmsMdm_compareIidStacks(&(pPathDesc1->iidStack), &(pPathDesc2->iidStack)) == 0));

}


static CmsRet phl_wrappedGetNextObjPathDesc(const MdmPathDescriptor *pRootPath,
                                            MdmPathDescriptor *pNextPath, UINT32 flags)
{
   MdmPathDescriptor lastHiddenPath;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   INIT_PATH_DESCRIPTOR(&lastHiddenPath);

   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextObjPathDesc(pRootPath, pNextPath);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         if (flags & (OGF_OMIT_HIDDEN_OBJ_PARAM|OGF_EID_TR69C_1
                                   |OGF_EID_TR69C_2))
         {
            UBOOL8 hidden = FALSE;
            char protoStr[MDM_BBF_PROTO_LEN] = {0};

            mdm_getPathDescHiddenFromAcs(pNextPath, &hidden);

            /* If this is not hidden from ACS, still set hidden if it's a USP item.
             * USP will never call this routine for a TR69c data model because we only
             * register USP items to the obuspa schema
             */
            if (!hidden)
            {
               mdm_getPathDescProto(pNextPath,protoStr,MDM_BBF_PROTO_LEN);
               if (strcmp(protoStr,MDM_BBF_PROTO_USP) == 0)
               {
                  hidden = TRUE;
               }
            }

            if (!hidden)
            {
               /* This was under #if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
                * However, to make phl.o binary only, we do a runtime check
                * which creates a little overhead in most cases, but should not
                * be too much.
                */
               ene_getPathDescHiddenFromAcs(pNextPath, &hidden, flags);
            }

            if (hidden)
            {
               /* this object is hidden, so go around again and get the next object */
               found = FALSE;

               /* remember the top of a sub-tree which is hidden from ACS */
               if ((lastHiddenPath.oid == 0) ||
                   (FALSE == mdm_isPathDescContainedInSubTree(&lastHiddenPath, pNextPath)))
               {
                  lastHiddenPath = *pNextPath;
               }
            }
         }


         /*
          * We have to do this in case the user marks a top level object as
          * hideObjectFromAcs="true", but does not mark the objects below that
          * object with hideObjectFromAcs.  mdm_getNextObjPathDesc() will keep
          * traversing the tree, so we have to detect those objects and skip them.
          */
         if (lastHiddenPath.oid != 0 && found)
         {
            if (mdm_isPathDescContainedInSubTree(&lastHiddenPath, pNextPath))
            {
               found = FALSE;
            }
         }
      }
   }
   return ret;
}


/** call mdm_getNextChildObjectPathDesc() with additional check for objects
 *  that should be hidden from tr69c.
 */
static CmsRet phl_wrappedGetNextChildObjPathDesc(const MdmPathDescriptor *pRootPath,
                                                 MdmPathDescriptor *pNextPath,
                                                 UINT32 flags)
{
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextChildObjPathDesc(pRootPath, pNextPath);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         if (flags & (OGF_OMIT_HIDDEN_OBJ_PARAM|OGF_EID_TR69C_1
                                   |OGF_EID_TR69C_2))
         {
            UBOOL8 hidden=FALSE;
            char protoStr[MDM_BBF_PROTO_LEN] = {0};
            mdm_getPathDescHiddenFromAcs(pNextPath, &hidden);

            /* If this is not hidden from ACS, still set hidden if it's a USP item.
             * USP will never call this routine for a TR69c data model because we only
             * register USP items to the obuspa schema
             */
            if (!hidden)
            {
               mdm_getPathDescProto(pNextPath,protoStr,MDM_BBF_PROTO_LEN);
               if (strcmp(protoStr,MDM_BBF_PROTO_USP) == 0)
               {
                  hidden = TRUE;
               }
            }

            if (!hidden)
            {
               /* See comments for the same call above */
               ene_getPathDescHiddenFromAcs(pNextPath, &hidden, flags);
            }

            if (hidden)
            {
               /* this object is hidden, so go around again and get the next object */
               found = FALSE;
            }
         }
      }
   }

   return ret;
}


/** call mdm_getNextParamName() with additional check for parameters taht should be
 * hidden from tr69c.
 */
static CmsRet phl_wrappedGetNextParamName(MdmPathDescriptor *path, UINT32 flags)
{
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextParamName(path);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         if (flags & (OGF_OMIT_HIDDEN_OBJ_PARAM|OGF_EID_TR69C_1
                                   |OGF_EID_TR69C_2))
         {
            UBOOL8 hidden=FALSE;
            char protoStr[MDM_BBF_PROTO_LEN] = {0};
            mdm_getPathDescHiddenFromAcs(path, &hidden);

            /* If this is not hidden from ACS, still set hidden if it's a USP item.
             * USP will never call this routine for a TR69c data model because we only
             * register USP items to the obuspa schema
             */
            if (!hidden)
            {
               mdm_getPathDescProto(path,protoStr,MDM_BBF_PROTO_LEN);
               if (strcmp(protoStr,MDM_BBF_PROTO_USP) == 0)
               {
                  hidden = TRUE;
               }
            }

            if (!hidden)
            {
               /* see comments for the same call above */
               ene_getPathDescHiddenFromAcs(path, &hidden, flags);
            }

            if (hidden)
            {
               /* this parameter is hidden, so go around again and get the next parameter */
               found = FALSE;
            }
         }
      }
   }

   return ret;
}


/** Get the next param or object name in the walk.
 *  Must be called with lock held.
 *
 * @param paramOnly (IN) Report parameter names only.  Do not report object names.
 *                       GetParameterValues and GetParameterAttributes calls this
 *                       function with paramOnly=TRUE because they don't want the
 *                       object names.  But GetParameterNames calls this function with
 *                       paramOnly=FALSE because it wants the object names as well as
 *                       the paramNames.
 */
static CmsRet phl_getNextPath(UBOOL8             paramOnly,
                              UBOOL8             nextLevelOnly,
                              const MdmPathDescriptor  *pRootPath,
                              MdmPathDescriptor  *pNextPath, UINT32 flags)
{
   MdmPathDescriptor path;
   CmsRet            rc = CMSRET_SUCCESS;

   if (pRootPath->oid == 0 || pRootPath->paramName[0] != 0)
   {
      cmsLog_error("invalid root object path");
      return CMSRET_INTERNAL_ERROR;
   }

   if (pNextPath->oid == 0)
   {
      /* This is the first call to tree traversal.
       * Return the root object path.
       */
      *pNextPath = *pRootPath;

      if (!paramOnly)
      {
         return rc;
      }
   }

   /* Set path to pNextPath and do the traversing */
   path = *pNextPath;

   /* If this getnext is not for paramOnly and is nextLevelOnly and
    * pNextPath and pRootPath are not the same object, pNextPath must be the
    * direct child object of pRootPath.  We want to return the next
    * direct child object of pRootPath.
    */
   if (!paramOnly && nextLevelOnly && !phl_sameObjDescs(pRootPath, &path))
   {
      /* Return the next direct child object of pRootPath. */
      rc = phl_wrappedGetNextChildObjPathDesc(pRootPath, &path, flags);
      if (rc != CMSRET_SUCCESS && rc != CMSRET_NO_MORE_INSTANCES)
      {
         cmsLog_error("mdm_getNextChildObjPathDesc error %d", rc);
      }
   }
   else
   {
      /* In this case, we want to return either the next parameter or
       * the next hierarchical object after pNextPath object.
       */
      while (rc == CMSRET_SUCCESS)
      {
         /* Get the next parameter of the path object */
         rc = phl_wrappedGetNextParamName(&path,flags);
         if (rc == CMSRET_SUCCESS)
         {
            /* done */
            break;   /* out of while (rc == CMSRET_SUCCESS) */
         }
         else if (rc == CMSRET_NO_MORE_INSTANCES)
         {
            /* There are no more parameter in the path object.
             * If this getnext is for paramOnly and is nextLevelOnly,
             * then done with finding nextParam.
             * Otherwise, find the next hierarchical object after
             * path and continue traversing.
             */
            if (!paramOnly || !nextLevelOnly)
            {
               /* find the next hierarchical object path */

               /* if pRootPath and path have the same object path,
                * we know that this will be the first call to
                * mdm_getNextObjPathDesc. path must be set to 0
                * prior to the first call to mdm_getNextObjPathDesc.
                */
               if (phl_sameObjDescs(pRootPath, &path))
               {
                  memset(&path, 0, sizeof(MdmPathDescriptor));
               }

               if(nextLevelOnly)
               {
                  rc = phl_wrappedGetNextChildObjPathDesc(pRootPath, &path, flags);
               }
               else
               {
                  rc = phl_wrappedGetNextObjPathDesc(pRootPath, &path, flags);
               }

               if (rc == CMSRET_SUCCESS)
               {
                  /* If not paramOnly, just return path object */
                  if (!paramOnly)
                  {
                     /* done */
                     break;   /* out of while (rc == CMSRET_SUCCESS) */
                  }
               }
               else if (rc != CMSRET_NO_MORE_INSTANCES)
               {
                  cmsLog_error("mdm_getNextObjPathDesc error %d", rc);
               }
            }
         }
         else
         {
            cmsLog_error("mdm_getNextParamName error %d", rc);
         }
      }  /* while (rc == CMSRET_SUCCESS) */
   }

   if (rc == CMSRET_SUCCESS)
   {
      /* Return the next path */
      *pNextPath = path;
   }

   return rc;

}  /* End of phl_getNextPath() */


static CmsRet phl_getPossibleRemoteFullpaths(const char **fullpathArray,
                                     SINT32 numEntries,
                                     char ***remoteFullpathArray,
                                     SINT32 *numRemote)
{
   SINT32 src, dst=0;
   char **remArray = NULL;

   if ((fullpathArray == NULL) || (remoteFullpathArray == NULL) || (numRemote == NULL))
   {
      cmsLog_error("NULL input args %p/%p/%p",
                   fullpathArray, remoteFullpathArray, numRemote);
      return CMSRET_INVALID_ARGUMENTS;
   }
   *remoteFullpathArray = NULL;
   *numRemote = 0;


   cmsLog_notice("Entered: numEntries=%d isRemoteCapable=%d",
                numEntries, mdmShmCtx->isRemoteCapable);

   if (mdmShmCtx->isRemoteCapable == FALSE)
   {
      return CMSRET_SUCCESS;
   }

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }

   // Just allocate the maximum possible number of remote entries
   remArray = (char **) cmsMem_alloc(numEntries * sizeof(char *), ALLOC_ZEROIZE);
   if (remArray == NULL)
   {
      cmsLog_error("failed to allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (src=0; src < numEntries; src++)
   {
      if (mdm_isPossibleRemoteFullpath(fullpathArray[src]))
      {
         remArray[dst] = cmsMem_strdup(fullpathArray[src]);
         cmsLog_debug("remote [%d] %s", dst, remArray[dst]);
         dst++;
      }
   }

   if (dst == 0)
   {
      // No fullpaths were copied into the array, so we can just free the
      // array buf itself instead of using cmsUtl_freeArrayOfStrings
      CMSMEM_FREE_BUF_AND_NULL_PTR(remArray);
   }

   *numRemote = dst;
   *remoteFullpathArray = remArray;

   cmsLog_notice("Exit: numEntries=%d ==> numRemote=%d", numEntries, *numRemote);
   return CMSRET_SUCCESS;
}


static CmsRet phl_getLocalPathDescListFromFullpath(const char **fullpathArray,
                                       SINT32 numEntries,
                                       MdmPathDescriptor **pLocalPathList,
                                       SINT32 *numLocal)
{
   MdmPathDescriptor *pathArray = NULL;
   SINT32 i;
   SINT32 iLocal = 0;

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }

   // Allocate max possible number of entries, even though not all may be used.
   pathArray = (MdmPathDescriptor *) cmsMem_alloc(numEntries * sizeof(MdmPathDescriptor), ALLOC_ZEROIZE);
   if (pathArray == NULL)
   {
      cmsLog_error("Could not allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // put local fullpaths into pathArray
   for (i=0; i < numEntries; i++)
   {
      if (mdm_isLocalFullpath(fullpathArray[i]))
      {
         if (CMSRET_SUCCESS == cmsMdm_aliasedFullPathToPathDescriptor(
                                 fullpathArray[i], &(pathArray[iLocal]), NULL))
         {
            iLocal++;
         }
         else
         {
            // Return error to caller to the whole operation will
            // fail (there is no way to report an error on a single param).
            cmsLog_error("bad local fullpath [%d] %s", i, fullpathArray[i]);
            CMSMEM_FREE_BUF_AND_NULL_PTR(pathArray);
            return CMSRET_INVALID_PARAM_NAME;
         }
      }
   }

   // Return results to user.  Note it is possible for pLocalPathList to
   // point to an array that needs to be freed, but numLocal == 0.
   *pLocalPathList = pathArray;
   *numLocal = iLocal;

   return CMSRET_SUCCESS;
}

CmsRet cmsPhl_getNextPathFlags(UBOOL8             paramOnly,
                               UBOOL8             nextLevelOnly,
                               const MdmPathDescriptor  *pRootPath,
                               MdmPathDescriptor  *pNextPath,
                               UINT32 flags)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   CmsRet rc;

   if (pRootPath == NULL || pNextPath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((rc = phl_fillLockZones(pRootPath, 1, nextLevelOnly, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pRootPath->oid);

   rc = phl_getNextPath(paramOnly, nextLevelOnly, pRootPath, pNextPath,flags);

   lck_autoUnlockZones(zones, pRootPath->oid, __FUNCTION__);

   return rc;

}   /* End of cmsPhl_getNextPath() */

CmsRet local_setParamValuesFlags(PhlSetParamValue_t *pSetParamValueList,
                                 SINT32 numEntries,
                                 UINT32 setFlags)
{
   UBOOL8               zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32               i = 0;
   PhlSetParamValue_t   *pSetParamValue;
   UBOOL8               writable;
   MdmNodeAttributes    nodeAttr;
   CmsRet               rc = CMSRET_SUCCESS;

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }

   if ((rc = phl_fillLockZonesSetParamValue(pSetParamValueList, numEntries, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pSetParamValueList->pathDesc.oid);

   cmsLog_debug("Entered, numEntries=%d setFlags=0x%x", numEntries, setFlags);

   for (i = 0, pSetParamValue = pSetParamValueList;
        i < numEntries;
        i++, pSetParamValue++)
   {
      // An error already detected on this param, skip it.
      if (pSetParamValue->status != CMSRET_SUCCESS)
      {
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      if (pSetParamValue->pathDesc.paramName[0] == 0)
      {
         /* setParameterValue must specify a param Name */
         cmsLog_error("No param name in pathDesc [%d] oid=%d",
                      i, pSetParamValue->pathDesc.oid);
         pSetParamValue->status = CMSRET_INVALID_PARAM_NAME;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /*
       * make sure the caller is in the access list.
       * A good side effect of getting the attributes is that we verify
       * the pathDesc points to a valid object.
       */
      if (CMSRET_SUCCESS != mdm_getParamAttributes(&(pSetParamValue->pathDesc), &nodeAttr))
      {
         cmsLog_error("invalid param [%d] %s%s", i,
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName);
         pSetParamValue->status = CMSRET_INVALID_PARAM_NAME;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      if (!mdm_isInAccessList(nodeAttr.accessBitMask) &&
          !mdm_isFullWriteAccessEid(mdmLibCtx.eid))
      {
         const CmsEntityInfo *eInfo;

         eInfo = cmsEid_getEntityInfo(GENERIC_EID(mdmLibCtx.eid));
         cmsLog_error("caller %s (eid=%d) is not in access list 0x%x",
                      eInfo ? eInfo->name:"Unknown", mdmLibCtx.eid, nodeAttr.accessBitMask);
         pSetParamValue->status = CMSRET_REQUEST_DENIED;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure pathDesc points to a writable param */
      mdm_getPathDescWritable(&(pSetParamValue->pathDesc), &writable);
      if (!writable &&
          !mdm_isFullWriteAccessEid(mdmLibCtx.eid) &&
          ((setFlags & OSF_NO_ACCESSPERM_CHECK) == 0))
      {
         cmsLog_error("param [%d] %s%s is NOT writable", i,
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName);
         /*
          * Plugfest 1/21/08: should be 9008 (SET_NON_WRITABLE_PARAM)
          * not 9001 (REQUEST_DENIED).  Also, do not break out of the
          * loop.  Keep iterating through the params in the array to
          * gather more error codes on the other parameters.
          */
         pSetParamValue->status = CMSRET_SET_NON_WRITABLE_PARAM;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure the type given by the ACS matches what we think it is */
      if (cmsUtl_strcmp(pSetParamValue->pParamType,
                        mdm_getParamType(&pSetParamValue->pathDesc)) &&
          cmsUtl_strcmp(pSetParamValue->pParamType,
                        mdm_getParamBaseType(&pSetParamValue->pathDesc)))
      {
         cmsLog_error("invalid param type at [%d] %s%s, got=%s exp=%s", i,
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName,
                      pSetParamValue->pParamType,
                      mdm_getParamBaseType(&pSetParamValue->pathDesc));
         pSetParamValue->status = CMSRET_INVALID_PARAM_TYPE;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure the new string value is valid */
      pSetParamValue->status = mdm_validateString(&(pSetParamValue->pathDesc), pSetParamValue->pValue);
      if (pSetParamValue->status != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid param value at [%d] %s%s, value=%s", i,
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName,
                      pSetParamValue->pValue);
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }
   }  /* for (i = 0; ....) */

   // Proceed with the set only if OSF_VALIDATE_ONLY is not set,
   // and we did not detect any errors in list.
   if (((setFlags & OSF_VALIDATE_ONLY) == 0) &&
       (rc == CMSRET_SUCCESS))
   {
      /* call the object dispatch layer api */
      rc = odl_setFlags(pSetParamValueList, numEntries, setFlags);
      if (rc != CMSRET_SUCCESS && rc != CMSRET_SUCCESS_REBOOT_REQUIRED)
      {
         cmsLog_error("odl_set error %d", rc);
      }
   }

   lck_autoUnlockZones(zones, pSetParamValueList->pathDesc.oid, __FUNCTION__);

   return rc;

}  /* End of local_setParamValuesFlags() */

CmsRet cmsPhl_setParameterValues(PhlSetParamValue_t *pSetParamValueList,
                                 SINT32 numEntries)
{
   return (cmsPhl_setParameterValuesFlags(pSetParamValueList, numEntries, 0));
}

static CmsRet localGeneric_setParamValuesFlags(BcmGenericParamInfo *paramInfoArray,
                                               SINT32 numEntries,
                                               UINT32 setFlags)
{
   PhlSetParamValue_t *setParamValueList = NULL;
   SINT32 i;
   CmsRet ret;

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }
   if (paramInfoArray == NULL)
   {
      cmsLog_error("NULL paramInfoArray");
      return CMSRET_INTERNAL_ERROR;
   }

   // Convert input paramInfoArray to phlSetParamValue_t
   // First, allocate array
   setParamValueList = (PhlSetParamValue_t *) cmsMem_alloc(numEntries * sizeof(PhlSetParamValue_t), ALLOC_ZEROIZE);
   if (setParamValueList == NULL)
   {
      cmsLog_error("Failed to allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // Do manual xlate/copying because the standard conversion function
   // will free the source array, which we do not want to do.  Also we have
   // custom behavior with regard to invalid fullpaths.
   for (i=0; i < numEntries; i++)
   {
      // Always copy the status over, and do this first.
      setParamValueList[i].status = paramInfoArray[i].errorCode;

      if (setParamValueList[i].status == CMSRET_SUCCESS)
      {
         if (CMSRET_SUCCESS != cmsMdm_aliasedFullPathToPathDescriptor(paramInfoArray[i].fullpath,
                                      &(setParamValueList[i].pathDesc), NULL))
         {
            cmsLog_error("Invalid fullpath at [%d] %s", i, paramInfoArray[i].fullpath);
            setParamValueList[i].status = CMSRET_INVALID_PARAM_NAME;
            paramInfoArray[i].errorCode = CMSRET_INVALID_PARAM_NAME;
            setFlags |= OSF_VALIDATE_ONLY;
            continue;
         }

         // Note this is a shallow copy: setParamValueList does not take ownership
         // of any string pointers.
         setParamValueList[i].pParamType = paramInfoArray[i].type;
         setParamValueList[i].pValue = paramInfoArray[i].value;
      }
   }

   ret = local_setParamValuesFlags(setParamValueList, numEntries, setFlags);

   // Copy back the error codes (only copy back the error codes for the
   // entries which did not already have an error in it.)
   for (i=0; i < numEntries; i++)
   {
      if (paramInfoArray[i].errorCode == CMSRET_SUCCESS)
      {
         paramInfoArray[i].errorCode = setParamValueList[i].status;
      }
   }

   // setParamValueList never took ownership of any strings, so we just
   // need to free the array buffer itself.
   CMSMEM_FREE_BUF_AND_NULL_PTR(setParamValueList);

   return ret;
}


// This was the old way, which uses PhlSetParamValue_t, which includes a
// MdmPathDescriptor, which cannot handle remote fullpaths which are not in
// the CMS/BDK data model.
CmsRet cmsPhl_setParameterValuesFlags(PhlSetParamValue_t *pSetParamValueList,
                                      SINT32 numEntries,
                                      UINT32 setFlags)
{
   BcmGenericParamInfo *paramInfoArray;
   SINT32 i;
   CmsRet ret;

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (pSetParamValueList == NULL)
   {
      cmsLog_error("NULL input arg");
      return CMSRET_INVALID_ARGUMENTS;
   }

   paramInfoArray = (BcmGenericParamInfo *) cmsMem_alloc(numEntries * sizeof(BcmGenericParamInfo), ALLOC_ZEROIZE);
   if (paramInfoArray == NULL)
   {
      cmsLog_error("Could not allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // Do manual xlate/copying because the standard conversion function
   // will free the source array, which we do not want to do.  Also we have
   // custom behavior with regard to invalid fullpaths.
   for (i=0; i < numEntries; i++)
   {
      // Always copy over the error code, and do it first.
      paramInfoArray[i].errorCode = pSetParamValueList[i].status;

      if (paramInfoArray[i].errorCode == CMSRET_SUCCESS)
      {
         CmsRet r2;
         r2 = cmsMdm_pathDescriptorToFullPath(&(pSetParamValueList[i].pathDesc),
                                              &(paramInfoArray[i].fullpath));
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Invalid pathDesc at [%d] %d %s %s (value=%s, ret=%d)", i,
                    pSetParamValueList[i].pathDesc.oid,
                    cmsMdm_dumpIidStack(&pSetParamValueList[i].pathDesc.iidStack),
                    pSetParamValueList[i].pathDesc.paramName,
                    pSetParamValueList[i].pValue, r2);
            pSetParamValueList[i].status = CMSRET_INVALID_PARAM_NAME;
            paramInfoArray[i].errorCode = CMSRET_INVALID_PARAM_NAME;
            continue;
         }

         // By convention, BcmGenercParamInfo always owns its string pointers, so
         // make a copy of the strings when copying over.
         paramInfoArray[i].type = cmsMem_strdup(pSetParamValueList[i].pParamType);
         paramInfoArray[i].value = cmsMem_strdup(pSetParamValueList[i].pValue);
      }
   }

   // Call the actual set function
   ret = bcmGeneric_setParameterValuesFlags(paramInfoArray, numEntries, setFlags);

   // Regardless of return code, always copy back the individual error codes.
   for (i=0; i < numEntries; i++)
   {
      if (pSetParamValueList[i].status == CMSRET_SUCCESS)
      {
         pSetParamValueList[i].status = paramInfoArray[i].errorCode;
      }
   }

   cmsUtl_freeParamInfoArray(&paramInfoArray, numEntries);

   return ret;
}

// This is the new (generic) way as of 504L04.  Takes array of
// BcmGenericParamInfo's.  On return, the errorCode in the
// BcmGenericParamInfo's may be set to indicate errors on the individual params.
CmsRet bcmGeneric_setParameterValuesFlags(BcmGenericParamInfo *paramInfoArray,
                                      SINT32 numEntries,
                                      UINT32 setFlags)
{
   SINT32 i, j, localIdx, remoteIdx, origindex;
   SINT32 *localIndexMap, *remoteIndexMap;
   BcmGenericParamInfo *pLocalList, *pRemoteList;
   CmsRet rc = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (paramInfoArray == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   pLocalList = pRemoteList = NULL;
   localIndexMap = remoteIndexMap = NULL;

   // Do initial pass over the params to check for (simple) errors.
   // If error is detected at this point, do not set object into MDM or call
   // RCL handler, but still continue to validate all other params as required
   // by TR69.
   for (i = 0; i < numEntries; i++)
   {
     /*
       * An early fault may have been detected in TR69 doSetParameterValues.
       */
      if (paramInfoArray[i].errorCode != CMSRET_SUCCESS)
      {
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      if (paramInfoArray[i].fullpath == NULL)
      {
         cmsLog_error("NULL fullpath detected at param %d", i);
         paramInfoArray[i].errorCode = CMSRET_INVALID_ARGUMENTS;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      // TODO: we could verify there is a paramName here.

      // If value is a NULL ptr, that is an error, even if the
      // param is a string type.  When using the PHL or BCM Generic HAL
      // (string based API), there is no concept of a NULL ptr, so the value of
      // a string type or any type cannot be NULL.  CMS OBJ API can set a
      // string param to NULL because NULL is a C ptr concept.
      if (paramInfoArray[i].value == NULL)
      {
         cmsLog_error("NULL value detected at param %d:%s", i, paramInfoArray[i].fullpath);
         paramInfoArray[i].errorCode = CMSRET_INVALID_PARAM_VALUE;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      // ParamTpe also must be non-NULL
      if (paramInfoArray[i].type == NULL)
      {
         cmsLog_error("NULL type detected at param %d:%s", i, paramInfoArray[i].fullpath);
         paramInfoArray[i].errorCode = CMSRET_INVALID_PARAM_TYPE;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* Look for duplicate fullpath in paramInfoArray.
       * Required by UNH TR-069 Certification test case 5.59
       * SetParameterValues Same Parameter Multiple Times
       */
      for (j = (i + 1); j < numEntries; j++)
      {
         if (!cmsUtl_strcmp(paramInfoArray[i].fullpath, paramInfoArray[j].fullpath))
         {
            paramInfoArray[i].errorCode = CMSRET_INVALID_ARGUMENTS;
            paramInfoArray[j].errorCode = CMSRET_INVALID_ARGUMENTS;

            cmsLog_error("duplicate fullpath detected for [%d, %d] %s value=%s",
                         i, j,
                         paramInfoArray[i].fullpath,
                         paramInfoArray[i].value);
            rc = CMSRET_INVALID_ARGUMENTS;
         }
      }
      if (paramInfoArray[i].errorCode != CMSRET_SUCCESS)
      {
         continue;
      }

      // TODO: we are now able to verify the remote obj's param names,
      // writable, types, validValues at the local side before we send over
      // to remote.  See cmsMdm_fullPathToPathDescriptorEx() and mdm_lookupPathDescEx().
   }

   // early error detected, but we still need to validate the other params.
   // Tell local and remote setParameterValues to validate only, but do not
   // set to MDM or call RCL.
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_notice("early error detected, continue with OSF_VALIDATE_ONLY");
      setFlags |= OSF_VALIDATE_ONLY;
   }

   // For sake of simplicity, allocate local and remote arrays that are
   // big enough to hold the entire input array.
   pRemoteList = cmsMem_alloc(sizeof(BcmGenericParamInfo) * numEntries, ALLOC_ZEROIZE);
   if (pRemoteList == NULL)
   {
      cmsLog_error("Failed to allocate pRemoteList! (numEntries=%d)", numEntries);
      rc = CMSRET_RESOURCE_EXCEEDED;
      goto exit;
   }

   remoteIndexMap = cmsMem_alloc(sizeof(SINT32) * numEntries, ALLOC_ZEROIZE);
   if (remoteIndexMap == NULL)
   {
      cmsLog_error("Failed to allocate remoteIndexMap! (numEntries=%d)", numEntries);
      rc = CMSRET_RESOURCE_EXCEEDED;
      goto exit;
   }

   pLocalList = cmsMem_alloc(sizeof(BcmGenericParamInfo) * numEntries, ALLOC_ZEROIZE);
   if (pLocalList == NULL)
   {
      cmsLog_error("Failed to allocate pLocalList! (numEntries=%d)", numEntries);
      rc =  CMSRET_RESOURCE_EXCEEDED;
      goto exit;
   }

   localIndexMap = cmsMem_alloc(sizeof(SINT32) * numEntries, ALLOC_ZEROIZE);
   if (localIndexMap == NULL)
   {
      cmsLog_error("Failed to allocate localIndexMap! (numEntries=%d)", numEntries);
      rc = CMSRET_RESOURCE_EXCEEDED;
      goto exit;
   }

   // Copy orig ParamInfoArray to remote and local lists
   localIdx = remoteIdx = 0;
   for (origindex = 0; origindex < numEntries; origindex++)
   {
      if (paramInfoArray[origindex].errorCode != CMSRET_SUCCESS)
      {
         // error has been set on this param, no need to process further.
         continue;
      }

      if (mdm_isLocalFullpath(paramInfoArray[origindex].fullpath))
      {
         // Note remoteList is just "borrowing" the fullpath, value, type,
         // and profile pointers from paramInfoArray.  paramInfoArray still
         // has ownership of the strings.
         memcpy(pLocalList+localIdx, paramInfoArray+origindex, sizeof(BcmGenericParamInfo));
         localIndexMap[localIdx] = origindex;
         localIdx++;
      }
      else
      {
         memcpy(pRemoteList+remoteIdx, paramInfoArray+origindex, sizeof(BcmGenericParamInfo));
         remoteIndexMap[remoteIdx] = origindex;
         remoteIdx++;
      }
   }

   // tmp hack for CDRouter atomic set test.  This test does a set with a bad
   // local fullpath and a good remote fullpath.  So we want to process the
   // bad local fullpath first so we don't set the remote value.
   if (localIdx > 0)
   {
      CmsRet r2;
      r2 = localGeneric_setParamValuesFlags(pLocalList, localIdx, setFlags);
      if (r2 != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to set local parameter values! r2=%d", r2);
          rc = r2;
          setFlags |= OSF_VALIDATE_ONLY;
      }

      // Copy local status back to the param's original position
      for (j = 0; j < localIdx; j++)
      {
          origindex = localIndexMap[j];
          cmsLog_debug("Copy local status[%d] to back to orig status[%d]", j, origindex);
          paramInfoArray[origindex].errorCode = pLocalList[j].errorCode;
      }
   }

   if (remoteIdx > 0)
   {
      CmsRet r3;
      r3 = remote_setParamValues(pRemoteList, remoteIdx, setFlags);
      if (r3 != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to set remote parameter values! r3=%d", r3);
          rc = r3;
      }

      for (j = 0; j < remoteIdx; j++)
      {
          origindex = remoteIndexMap[j];
          cmsLog_notice("Copy remote status[%d] back to orig status[%d]", j, origindex);
          paramInfoArray[origindex].errorCode = pRemoteList[j].errorCode;
      }
   }

exit:
   // Even though pRemoteList and pLocalList are arrays of BcmGenericParamInfo's, they
   // never took ownership of the string pointers, so they must be freed using
   // a simple cmsMem_free and not with cmsUtl_freeParamInfoArray.
   cmsMem_free(pRemoteList);
   cmsMem_free(remoteIndexMap);
   cmsMem_free(pLocalList);
   cmsMem_free(localIndexMap);
   cmsLog_debug("Exit: rc=%d, status list (%d):", rc, numEntries);
   for (i=0; i < numEntries; i++)
      cmsLog_debug("  [%d] status=%d", i, paramInfoArray[i].errorCode);
   return rc;
}


CmsRet cmsPhl_getParamValueFlags(const MdmPathDescriptor   *pPath,
                                 UINT32 getFlags,
                                 PhlGetParamValue_t  **pParamValue)
{
   PhlGetParamValue_t  *returnedArray = NULL;
   SINT32               numReturnedEntries = 0;
   CmsRet               rc = CMSRET_SUCCESS;

   if ((pPath == NULL) || (pParamValue == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamValue = NULL;

   if (pPath->paramName[0] == 0)
   {
      cmsLog_error("parameter name must be specified");
      return CMSRET_INVALID_PARAM_NAME;
   }

   // Internally, just call the full featured cmsPhl_getParameterValuesFlags().
   // Convert the input and output args here.
   rc = cmsPhl_getParameterValuesFlags(pPath, 1, TRUE, getFlags,
                                       &returnedArray, &numReturnedEntries);
   if (rc == CMSRET_SUCCESS)
   {
      if (numReturnedEntries != 1)
      {
         cmsLog_error("Get %s (flags=0x%x) retEntries=%d (expect 1)",
                      (char *)pPath, getFlags, numReturnedEntries);
         CMSMEM_FREE_BUF_AND_NULL_PTR(returnedArray);
         rc = (numReturnedEntries == 0) ? CMSRET_INVALID_PARAM_NAME : CMSRET_INTERNAL_ERROR;
      }
      else
      {
         // Success!  Give array of 1 element back to caller.
         *pParamValue = returnedArray;
      }
   }

   return rc;

}  /* End of cmsPhl_getParamValueFlags() */


static CmsRet local_getParamValuesFlags(const MdmPathDescriptor  *pPathList,
                                              SINT32             numEntries,
                                              UBOOL8             nextLevelOnly,
                                              UINT32             getFlags,
                                              CmsVbuf          **pResultVbuf,
                                              SINT32             *pNumParamValueEntries)
{
   const MdmPathDescriptor *pPath;
   SINT32               i, numRespEntries=0;
   UBOOL8               zones[MDM_MAX_LOCK_ZONES]={FALSE};
   UINT32               omitNullValues=(getFlags & OGF_OMIT_NULL_VALUES);
   PhlGetParamValue_t    paramValue;
   CmsVbuf              *vbuf = NULL;
   CmsRet rc;

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }

   if ((pPathList == NULL) || (pResultVbuf == NULL) || (pNumParamValueEntries == NULL))
   {
      cmsLog_error("NULL input params %p/%p/%p",
                   pPathList, pResultVbuf, pNumParamValueEntries);
      return CMSRET_INVALID_ARGUMENTS;
   }
   *pNumParamValueEntries = 0;


   if ((rc = phl_fillLockZones(pPathList, numEntries, nextLevelOnly, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }

   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPathList->oid);

   /* allocate memory for parameter value response */
   vbuf = cmsVbuf_new();
   if (vbuf == NULL)
   {
      cmsLog_error("cmsVbuf_new failed");
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }

   /* loop through the requested name list */
   for (i = 0, pPath = pPathList;
        i < numEntries && rc == CMSRET_SUCCESS;
        i++, pPath++)
   {
      if (pPath->paramName[0] != 0)
      {
         /* this is a parameter path */
         memset(&paramValue, 0, sizeof(paramValue));
         rc = odl_getFlags(pPath, getFlags, &(paramValue.pValue));
         if (rc == CMSRET_SUCCESS)
         {
            if (omitNullValues && paramValue.pValue == NULL)
            {
               /* omit this NULL value from response, do nothing here */
            }
            else
            {
               MdmParamNode *paramNode;
               if ((paramNode = mdm_getParamNode(pPath->oid, pPath->paramName)) == NULL)
               {
                  cmsLog_error("bad param name %s (oid=%d)",
                               pPath->paramName, pPath->oid);
               }
               else
               {
                  paramValue.pathDesc   = *pPath;
                  paramValue.pParamType = mdm_getParamBaseType(pPath);
                  paramValue.profile = paramNode->profile;
                  paramValue.writable =((paramNode->flags & PRN_WRITABLE) == PRN_WRITABLE);
                  paramValue.isTr69Password = ((paramNode->flags & PRN_TR69_PASSWORD) == PRN_TR69_PASSWORD);
                  cmsVbuf_put(vbuf, &paramValue, sizeof(paramValue));  // vbuf is the owner of the value string
                  numRespEntries++;
               }
            }
         }
         else
         {
            cmsLog_debug("odl_getFlags error %d: oid=%d paramName=%s iid=%s",
                          rc, pPath->oid, pPath->paramName,
                          cmsMdm_dumpIidStack(&(pPath->iidStack)));
         }
      }
      else
      {
         /* this is an object path */
         /* traverse the sub-tree below the object path */
         MdmPathDescriptor nextPath=EMPTY_PATH_DESCRIPTOR;

         while (rc == CMSRET_SUCCESS)
         {
            rc = phl_getNextPath(TRUE, nextLevelOnly, pPath, &nextPath, getFlags);
            if (rc == CMSRET_SUCCESS)
            {
               memset(&paramValue, 0, sizeof(paramValue));
               rc = odl_getFlags(&nextPath, getFlags, &(paramValue.pValue));
               if (rc == CMSRET_SUCCESS)
               {
                  if (omitNullValues && paramValue.pValue == NULL)
                  {
                     /* omit this NULL value from response, do nothing here */
                  }
                  else
                  {
                     MdmParamNode *paramNode;
                     if ((paramNode = mdm_getParamNode(nextPath.oid, nextPath.paramName)) == NULL)
                     {
                        cmsLog_error("bad param name %s (oid=%d)",
                                     nextPath.paramName, nextPath.oid);
                     }
                     else
                     {
                        paramValue.pathDesc   = nextPath;
                        paramValue.pParamType = mdm_getParamBaseType(&nextPath);
                        paramValue.profile = paramNode->profile;
                        paramValue.writable =((paramNode->flags & PRN_WRITABLE) == PRN_WRITABLE);
                        paramValue.isTr69Password = ((paramNode->flags & PRN_TR69_PASSWORD) == PRN_TR69_PASSWORD);
                        cmsVbuf_put(vbuf, &paramValue, sizeof(paramValue));  // vbuf is the owner of the value string
                        numRespEntries++;
                     }
                  }
               }
               else
               {
                  cmsLog_error("odl_getFlags error %d", rc);
               }
            }
            else if (rc == CMSRET_NO_MORE_INSTANCES)
            {
               rc = CMSRET_SUCCESS;
               break;   /* out of while (rc == CMSRET_SUCCESS) */
            }
            else
            {
               cmsLog_error("phl_getNextPath error %d", rc);
            }
         }  /* while (rc == CMSRET_SUCCESS) */
      }
   }  /* for (i = 0; ....) */

   if (rc == CMSRET_SUCCESS)
   {
      *pResultVbuf           = vbuf;
      *pNumParamValueEntries = numRespEntries;
   }
   else
   {
      cmsPhl_freeGetParamValueVbuf(vbuf);
   }

   lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
   return rc;
}

static CmsRet localGeneric_getParamValuesFlags(const MdmPathDescriptor  *pPathList,
                                              SINT32             numEntries,
                                              UBOOL8             nextLevelOnly,
                                              UINT32             getFlags,
                                              BcmGenericParamInfo **pParamInfoList,
                                              SINT32             *pNumParamInfoEntries)
{
   CmsVbuf *vbuf = NULL;
   SINT32 numParamValueEntries = 0;
   BcmGenericParamInfo *result = NULL;
   CmsRet ret;

   ret = local_getParamValuesFlags(pPathList, numEntries, nextLevelOnly, getFlags,
                                   &vbuf, &numParamValueEntries);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   // Convert VBuf of PhlGetParamValue_t's to BcmGenericParamInfo
   ret = cmsPhl_convertParamValueVbufToGenericParamInfo(
                                         &vbuf, numParamValueEntries, &result);
   if (ret != CMSRET_SUCCESS)
   {
      // on failure, caller needs to free the Vbuf of PhlGetParamValue_t's
      cmsPhl_freeGetParamValueVbuf(vbuf);
      return ret;
   }

   // On success, vbuf is freed by the conversion function.
   // Give result (BcmGenericParamInfo) to caller.
   *pParamInfoList = result;
   *pNumParamInfoEntries = numParamValueEntries;

   return ret;
}



// This was the old way, which uses PhlGetParamValue_t and MdmPathDescriptor,
// which cannot handle remote parameters which are not in the CMS/BDK data model.
CmsRet cmsPhl_getParameterValuesFlags(const MdmPathDescriptor  *pPathList,
                                      SINT32             numEntries,
                                      UBOOL8             nextLevelOnly,
                                      UINT32             getFlags,
                                      PhlGetParamValue_t **pParamValueList,
                                      SINT32             *pNumParamValueEntries)
{
   char **fullpathArray;
   BcmGenericParamInfo *paramInfoList = NULL;
   SINT32 i, numParamInfos = 0;
   PhlGetParamValue_t *result = NULL;
   CmsRet ret, r2;

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }
   if ((pPathList == NULL) ||
       (pParamValueList == NULL) ||
       (pNumParamValueEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Convert pPathList (MdmPathDescriptors) to fullpaths
   fullpathArray = (char **) cmsMem_alloc(numEntries * sizeof(char *), ALLOC_ZEROIZE);
   if (fullpathArray == NULL)
   {
      cmsLog_error("Failed to allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   for (i=0; i < numEntries; i++)
   {
      r2 = cmsMdm_pathDescriptorToFullPath(&(pPathList[i]), &(fullpathArray[i]));
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Bad pathDesc at [%d] oid=%d paramName=%s", i,
                      pPathList[i].oid, pPathList[i].paramName);
         cmsUtl_freeArrayOfStrings(&fullpathArray, numEntries);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   ret = bcmGeneric_getParameterValuesFlags((const char **) fullpathArray, numEntries,
                                            nextLevelOnly, getFlags,
                                            &paramInfoList, &numParamInfos);

   // Done with fullpathArray, free it.
   cmsUtl_freeArrayOfStrings(&fullpathArray, numEntries);

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = cmsPhl_convertGenericParamInfoToParamValue(&paramInfoList, numParamInfos,
                                                    &result);
   if (ret != CMSRET_SUCCESS)
   {
      // on failure, caller needs to free the paramInfoList
      cmsUtl_freeParamInfoArray(&paramInfoList, numParamInfos);
      return ret;
   }

   // On success, paramInfoList is freed by the conversion func.
   // Give result (PhlGetParamValue_t) to caller.
   *pParamValueList = result;
   *pNumParamValueEntries = numParamInfos;

   return ret;
}

// This is the new (generic) way as of 504L04.  Takes array of fullpaths.
// Returns BcmGenericParamInfo.
CmsRet bcmGeneric_getParameterValuesFlags(const char   **fullpathArray,
                                      SINT32             numEntries,
                                      UBOOL8             nextLevelOnly,
                                      UINT32             getFlags,
                                      BcmGenericParamInfo **pParamInfoList,
                                      SINT32             *pNumParamInfoEntries)
{
   SINT32 numRemote = 0, numLocal = 0;
   char **remoteFullpathArray = NULL;
   MdmPathDescriptor *pLocalPathList = NULL;
   BcmGenericParamInfo *pRemoteList = NULL, *pLocalList = NULL, *pResult = NULL;
   char **pStrRemoteList, **pStrLocalList;
   ParamIndexEntry *pIndexTable, *p, *q;
   SINT32 i, numRemoteEntries, numLocalEntries, numIndex;
   CmsRet rc = CMSRET_SUCCESS;

   if ((pParamInfoList == NULL) || (pNumParamInfoEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamInfoList = NULL;
   *pNumParamInfoEntries = 0;
   numRemoteEntries = numLocalEntries = numIndex = 0;
   pStrRemoteList = pStrLocalList = NULL;
   pIndexTable = NULL;

   // Does it have to be external caller only?  rut2_qos_queue_stats.c calls
   // this via cmsPhl_getParamValue.  qdm2_intf calls this a lot, and qdm
   // functions may be called from inside MDM.
   // Maybe we want to prevent a full local+remote walk, but in that case,
   // caller should use the OGF_LOCAL_MDM_ONLY flag.
   // CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Preserve existng behavior, even though it is questionable.
      return CMSRET_SUCCESS;
   }

   if (fullpathArray == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((getFlags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      rc = phl_getPossibleRemoteFullpaths(fullpathArray, numEntries,
                                  &remoteFullpathArray, &numRemote);
      if (rc != CMSRET_SUCCESS)
      {
         return rc;
      }
   }

   rc = phl_getLocalPathDescListFromFullpath(fullpathArray, numEntries,
                                             &pLocalPathList, &numLocal);
   if (rc != CMSRET_SUCCESS)
   {
       cmsUtl_freeArrayOfStrings(&remoteFullpathArray, numRemote);
       return rc;
   }

   cmsLog_notice("numEntries=%d, numRemote=%d, numLocal=%d", numEntries, numRemote, numLocal);

   numLocalEntries = 0;
   numRemoteEntries = 0;
   if (numRemote > 0)
   {
      rc = remote_getParamValues((const char **)remoteFullpathArray, numRemote,
                                 nextLevelOnly, getFlags,
                                 &pRemoteList, &numRemoteEntries);
      if (rc != CMSRET_SUCCESS)
      {
          // XXX should we really jump to exit here?  should we try to return
          // the local params?
          cmsLog_error("Failed to get remote parameters! numRemote=%d, rc=%d",
                       numRemote, rc);
          goto exit;
      }

      // create an array of fullpaths for merging.
      // Note that numRemoteEntries might be 0, but pStrRemoteList will still be
      // non-null.  The algorithm below will still work correctly.
      pStrRemoteList = cmsMem_alloc(sizeof(char *) * numRemoteEntries, ALLOC_ZEROIZE);
      if (pStrRemoteList == NULL)
      {
          cmsLog_error("failed to allocate %d remote entries", numRemoteEntries);
          rc = CMSRET_RESOURCE_EXCEEDED;
          goto exit;
      }
      for (i = 0; i < numRemoteEntries; i++)
      {
          // phl_merge will make a copy of the string before merging, so it
          // is safe to give it a pointer to our own string.
          pStrRemoteList[i] = pRemoteList[i].fullpath;
      }
   }

   if (numLocal > 0)
   {
      rc = localGeneric_getParamValuesFlags(pLocalPathList, numLocal,
                                            nextLevelOnly, getFlags,
                                            &pLocalList, &numLocalEntries);
      if (rc != CMSRET_SUCCESS)
      {
          // XXX should we really jump to exit here?  should we try to return
          // the remote params that we got?
          cmsLog_debug("Failed to get local parameters! numLocal=%d rc=%d)",
                       numLocal, rc);
          goto exit;
      }

      // See comments above for the remote case.
      pStrLocalList = cmsMem_alloc(sizeof(char *) * numLocalEntries, ALLOC_ZEROIZE);
      if (pStrLocalList == NULL)
      {
          cmsLog_error("failed to allocate %d local entries", numLocalEntries);
          rc = CMSRET_RESOURCE_EXCEEDED;
          goto exit;
      }
      for (i = 0; i < numLocalEntries; i++)
      {
          pStrLocalList[i] = pLocalList[i].fullpath;
      }
   }

   cmsLog_notice("numLocal=%d, numRemote=%d", numLocalEntries, numRemoteEntries);

   rc = cmsPhl_mergeParams(pStrLocalList, numLocalEntries,
                           pStrRemoteList, numRemoteEntries,
                           &pIndexTable, &numIndex, TRUE);
   if (rc != CMSRET_SUCCESS)
   {
       cmsLog_error("Failed to merge parameters! ret=%d", rc);
       goto exit;
   }

   pResult = cmsMem_alloc(numIndex * sizeof(BcmGenericParamInfo), ALLOC_ZEROIZE);
   if (pResult == NULL)
   {
       cmsLog_error("Failed to allocate pResult! (numIndex=%d)", numIndex);
       rc = CMSRET_RESOURCE_EXCEEDED;
       goto exit;
   }

   cmsLog_notice("merged count numIndex=%d", numIndex);

   for (i = 0; i < numIndex; i++)
   {
       p = pIndexTable + i;

       // Do a shallow copy of fields from pLocalList/pRemoteList to pResult.
       if (p->source == LOCAL)
       {
           if (pLocalList != NULL)
           {
               pResult[i] = pLocalList[p->index];
               // pLocalList transfered ownership of strings to pResult.
               pLocalList[p->index].fullpath = NULL;
               pLocalList[p->index].type = NULL;
               pLocalList[p->index].value = NULL;
               pLocalList[p->index].profile = NULL;
           }
       }
       else if (p->source == REMOTE)
       {
           if (pRemoteList != NULL)
           {
               pResult[i] = pRemoteList[p->index];
               // pRemoteList transfered ownership of strings to pResult.
               pRemoteList[p->index].fullpath = NULL;
               pRemoteList[p->index].type = NULL;
               pRemoteList[p->index].value = NULL;
               pRemoteList[p->index].profile = NULL;
           }
       }
       else
       {
           cmsLog_error("Unhandled indexTable entry %d", i);
           continue;
       }

       if (p->next != NULL)
       {
           //This is the case of adding values of parameters
           UINT32 value, v;
           char valBuf[32];
           BcmGenericParamInfo *t;

           cmsLog_notice("converting %s to unsigned int", pResult[i].value);

           value = (UINT32) strtoul(pResult[i].value, NULL, 10);
           for (q = p->next; q != NULL; q = q->next)
           {
               //We assume only MPT_UNSIGNED_INT parameter can be added together.
               if (q->source == LOCAL && pLocalList != NULL)
               {
                   t = &pLocalList[q->index];
               }
               else if (q->source == REMOTE && pRemoteList != NULL)
               {
                   t = &pRemoteList[q->index];
               }
               else
               {
                   cmsLog_error("Unrecognized souce to merge!");
                   continue;
               }
               v = (UINT32) strtoul(t->value, NULL, 10);
               value += v;
           }

           sprintf(valBuf, "%u", value);
           CMSMEM_REPLACE_STRING(pResult[i].value, valBuf);
           cmsLog_debug("new added value is %s", pResult[i].value);
       }
   }

   *pParamInfoList = pResult;
   *pNumParamInfoEntries = numIndex;

exit:
   if (pIndexTable != NULL)
   {
       freeParamIndexTable(pIndexTable, numIndex);
   }

   if (pLocalList)
   {
      cmsUtl_freeParamInfoArray(&pLocalList, numLocalEntries);
   }
   if (pLocalPathList)
   {
       cmsMem_free(pLocalPathList);
   }
   // pStrLocalList did not take any ownership of the strings, so we just
   // need to free the array buffer here.
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrLocalList);

   if (pRemoteList)
   {
       cmsUtl_freeParamInfoArray(&pRemoteList, numRemoteEntries);
   }
   cmsUtl_freeArrayOfStrings(&remoteFullpathArray, numRemote);
   // pStrRemoteList did not take any ownership of the strings, so we just
   // need to free the array buffer here.
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrRemoteList);

   return rc;
}  /* End of bcmGeneric_getParameterValuesFlags() */


CmsRet cmsPhl_getParamInfo(const MdmPathDescriptor *pPath,
                           PhlGetParamInfo_t **pParamInfo)
{
   PhlGetParamInfo_t *pResp;
   CmsRet            rc = CMSRET_SUCCESS;
   CmsRet ret;

   if ((pPath == NULL) || (pParamInfo == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamInfo = NULL;

   /* allocate memory for parameter name info */
   pResp = cmsMem_alloc(sizeof(PhlGetParamInfo_t), ALLOC_ZEROIZE);
   if (pResp == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      return CMSRET_INTERNAL_ERROR;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(pResp);
      return rc;
   }

   /* get info from MDM */
   if ((ret = mdm_getPathDescWritable(pPath, &(pResp->writable))) == CMSRET_SUCCESS)
   {
      if (IS_PARAM_NAME_PRESENT(pPath))
      {
         /*
          * The MdmPathDescriptor can point to an object or a parameter.
          * Only query about tr69c password if the MdmPathDescriptor points
          * to a parameter name.
          */
         ret = mdm_getParamIsTr69Password(pPath, &(pResp->isTr69Password));
         if (ret == CMSRET_SUCCESS)
         {
            MdmParamNode *paramNode;

            /* get the profile name of this parameter */
            if ((paramNode = mdm_getParamNode(pPath->oid, pPath->paramName)))
            {
               pResp->profile = paramNode->profile;
            }
         }
      }
      else
      {
         MdmObjectNode *objectNode;

         /* get the profile name of this object */
         if ((objectNode = mdm_getObjectNode(pPath->oid)))
         {
            pResp->profile = objectNode->profile;
         }
      }
   }

   if (ret == CMSRET_SUCCESS)
   {
      pResp->pathDesc = *pPath;
      *pParamInfo     = pResp;
   }
   else
   {
      rc = ret;
      cmsLog_error("error %d", rc);
      cmsMem_free(pResp);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParamInfo() */



static CmsRet fillNameInfoEx(const MdmPathDescriptor *pathDesc,
                      PhlGetParamNameEx_t *nameInfoEx)
{
   MdmObjectNode *objNode = NULL;
   char *fullpath = NULL;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if ((pathDesc == NULL) ||
       ((objNode = mdm_getObjectNode(pathDesc->oid)) == NULL) ||
       (nameInfoEx == NULL))
   {
      cmsLog_error("Null or bad input args");
      return CMSRET_INVALID_ARGUMENTS;
   }
   memset(nameInfoEx, 0, sizeof(PhlGetParamNameEx_t));

   ret = cmsMdm_pathDescriptorToFullPath(pathDesc, &fullpath);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (IS_PARAM_NAME_PRESENT(pathDesc))
   {
      MdmParamNode *paramNode;

      if ((paramNode = mdm_getParamNode(pathDesc->oid, pathDesc->paramName)) == NULL)
      {
         cmsLog_error("bad param name %s", pathDesc->paramName);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
         ret = CMSRET_INVALID_PARAM_NAME;
      }
      else
      {
         nameInfoEx->fullpath = fullpath;  // give ownership to caller, who is responsible for freeing.
         nameInfoEx->type = cmsMdm_paramTypeToString(paramNode->type);  // static buffer, do not free.
         nameInfoEx->profile = paramNode->profile;  // static buffer, do not free.
         nameInfoEx->isTr69Password = (paramNode->flags & PRN_TR69_PASSWORD) ? 1 : 0;
         getParamWritable(paramNode, &(nameInfoEx->writable));
      }
   }
   else
   {
      // Not clear if this function ever gets called on an object, but
      // add the code here anyways for completeness.
      nameInfoEx->fullpath = fullpath;
      nameInfoEx->type = "OBJECT";  // objects don't have a type, but we need a string here.
      nameInfoEx->profile = objNode->profile;
      nameInfoEx->isTr69Password = 0;  // not applicable for objects
      getObjectWritable(objNode, &(nameInfoEx->writable));
   }

   return ret;
}

// Free all fullpaths in the Vbuf of PhlGetParamNameEx.
static void freeFullpathsInParamNamesExVbuf(CmsVbuf *vbuf)
{
   PhlGetParamNameEx_t paramNameEx;

   if (vbuf == NULL)
   {
      return;
   }

   cmsVbuf_resetIndex(vbuf);
   while (cmsVbuf_get(vbuf, &paramNameEx, sizeof(paramNameEx)) == CMSRET_SUCCESS)
   {
      cmsMem_free(paramNameEx.fullpath);
      // Note that at this point, the fullpath pointer in the vbuf is pointing
      // to a freed buffer, which is dangerous.  The assumption is the entire
      // vbuf is about to be destroyed and not used after this function.
   }

   return;
}


CmsRet local_getParameterNamesEx(MdmPathDescriptor *pPath,
                                UBOOL8              nextLevelOnly,
                                UINT32              flags,
                                CmsVbuf           **resultVbuf,
                                SINT32             *numParamNames)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            numRespEntries=0;
   PhlGetParamNameEx_t paramNameEx;
   CmsVbuf           *vbuf = NULL;
   CmsRet            rc = CMSRET_SUCCESS;

   cmsLog_notice("Entered: (flags=0x%x)", flags);

   if ((rc = phl_fillLockZones(pPath, 1, nextLevelOnly, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }

   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPath->oid);

   /* allocate memory for parameter name response */
   vbuf = cmsVbuf_new();
   if (vbuf == NULL)
   {
      cmsLog_error("cmsVbuf_new failed");
      lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }

   if (IS_PARAM_NAME_PRESENT(pPath))
   {
      /* this is a parameter name, just get this one.  no tree traversal */
      rc = fillNameInfoEx(pPath, &paramNameEx);
      if (rc == CMSRET_SUCCESS)
      {
         cmsVbuf_put(vbuf, &paramNameEx, sizeof(PhlGetParamNameEx_t));
         numRespEntries = 1;
      }
      else
      {
         cmsLog_error("mdm_getNameInfoEx error %d", rc);
      }
   }
   else
   {
      /* this is an object name */
      /* traverse the sub-tree below the object node (pPath) */
      MdmPathDescriptor nextPath = EMPTY_PATH_DESCRIPTOR;

      while (rc == CMSRET_SUCCESS)
      {
         rc = phl_getNextPath(FALSE, nextLevelOnly, pPath, &nextPath,flags);
         if (rc == CMSRET_SUCCESS)
         {
            rc = fillNameInfoEx(&nextPath, &paramNameEx);
            if (rc == CMSRET_SUCCESS)
            {
               cmsVbuf_put(vbuf, &paramNameEx, sizeof(PhlGetParamNameEx_t));
               numRespEntries++;
            }
            else
            {
               cmsLog_error("mdm_getNameInfoEx error %d", rc);
            }
         }
         else if (rc == CMSRET_NO_MORE_INSTANCES)
         {
            rc = CMSRET_SUCCESS;
            break;   /* out of while (rc == CMSRET_SUCCESS) */
         }
         else
         {
            cmsLog_error("phl_getNextPath error %d", rc);
         }
      }  /* while (rc == CMSRET_SUCCESS) */
   }

   if (rc == CMSRET_SUCCESS)
   {
      *resultVbuf = vbuf;
      *numParamNames  = numRespEntries;
   }
   else
   {
      freeFullpathsInParamNamesExVbuf(vbuf);
      cmsVbuf_destroy(vbuf);
   }

   lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);
   return rc;
}


// This is the new (generic) way as of 504L04.
// Returns BcmGenericParamInfo.  Since cmsPhl_getParameterNamesEx is already
// very close to the generic way, just call cmsPhl_getParameterNamesEx and
// convert the returned array.
CmsRet bcmGeneric_getParameterNamesFlags(const char    *fullpath,
                                  UBOOL8         nextLevelOnly,
                                  UINT32         flags,
                                  BcmGenericParamInfo **paramInfoArray,
                                  SINT32              *numParamInfos)
{
   PhlGetParamNameEx_t *paramNameArray = NULL;
   SINT32 numParamNames = 0;
   CmsRet ret;

   if ((fullpath == NULL) || (paramInfoArray == NULL) || (numParamInfos == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_notice("Entered: fullpath=%s nextLevelOnly=%d flags=0x%x",
                  fullpath, nextLevelOnly, flags);

   // The PHL interface will return array of PhlGetParamNameEx
   ret = cmsPhl_getParameterNamesEx(fullpath, nextLevelOnly, flags,
                                    &paramNameArray, &numParamNames);
   if (ret == CMSRET_SUCCESS)
   {
      // convert PHL paramNamesEx to BcmGenericParamInfo.
      // This function will free the source on success.
      ret = cmsPhl_convertParamNameExToGenericParamInfo(&paramNameArray,
                                                        numParamNames,
                                                        paramInfoArray);
      if (ret == CMSRET_SUCCESS)
      {
         *numParamInfos = numParamNames;
      }

      // Free it again in case of error
      cmsPhl_freeParamNamesEx(&paramNameArray, numParamNames);
   }

   return ret;
}


CmsRet cmsPhl_getParameterNamesEx(const char    *fullpath,
                                  UBOOL8         nextLevelOnly,
                                  UINT32         flags,
                                  PhlGetParamNameEx_t  **paramNameArray,
                                  SINT32                *numParamNames)
{
   MdmPathDescriptor *pLocalPathList = NULL;
   SINT32            numLocalPathList, numLocalEntries, numRemoteEntries, numIndex;
   SINT32            i, index = 0;
   CmsVbuf *pLocalVbuf = NULL;
   PhlGetParamNameEx_t *pRemoteList, *pResult;
   char              **pStrLocalList, **pStrRemoteList;
   ParamIndexEntry   *pIndexTable = NULL;
   CmsRet            rc = CMSRET_SUCCESS;

   if ((fullpath == NULL) || (paramNameArray == NULL) || (numParamNames == NULL))
   {
      cmsLog_error("Invalid input args");
      return CMSRET_INVALID_ARGUMENTS;
   }

   *paramNameArray = NULL;
   *numParamNames  = 0;
   numLocalPathList = numLocalEntries = numRemoteEntries = numIndex = 0;
   pRemoteList = pResult = NULL;
   pStrLocalList = pStrRemoteList = NULL;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);


   // Handle possible local results
   rc = phl_getLocalPathDescListFromFullpath(&fullpath, 1,
                                       &pLocalPathList, &numLocalPathList);
   if (rc != CMSRET_SUCCESS)
   {
      return rc;
   }

   if (numLocalPathList > 0)
   {
      // There is at most 1 pathDescriptor in pLocalPathList
      rc = local_getParameterNamesEx(pLocalPathList, nextLevelOnly, flags,
                                     &pLocalVbuf, &numLocalEntries);
      if (rc != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get from local database! ret=%d", rc);
         goto exit;
      }

      // allocate an array of fullpath string ptrs for merging purpose
      pStrLocalList = cmsMem_alloc(sizeof(char *) * numLocalEntries, ALLOC_ZEROIZE);
      if (pStrLocalList == NULL)
      {
          rc = CMSRET_RESOURCE_EXCEEDED;
          goto exit;
      }

      for (i = 0; i < numLocalEntries; i++)
      {
          PhlGetParamNameEx_t paramNameEx;

          // pStrLocalList will just point to the fullpath string buffer in
          // the pLocalVbuf.  So pStrLocalList does not own the fullpath
          // buffer.  So don't free fullpath when pStrLocalList is freed.
          cmsVbuf_get(pLocalVbuf, &paramNameEx, sizeof(paramNameEx));
          pStrLocalList[i] = paramNameEx.fullpath;
      }
   }

   // Handle remote results
   if ((flags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      char **remoteFullpathArray = NULL;
      SINT32 numRemote = 0;

      phl_getPossibleRemoteFullpaths(&fullpath, 1, &remoteFullpathArray, &numRemote);
      if (numRemote > 0)
      {
         // we don't need this remote array, if numRemote > 0, the fullpath
         // is remote.
         cmsUtl_freeArrayOfStrings(&remoteFullpathArray, numRemote);

         rc = remote_getParamNamesEx(fullpath, nextLevelOnly, flags,
                                     &pRemoteList, &numRemoteEntries);
         if (rc != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get from remote database! ret=%d", rc);
            goto exit;
         }

         // allocate an array of fullpath string ptrs for merging purpose
         // Note that numRemoteEntries might be 0, but pStrRemoteList will still be
         // non-null.  The algorithm below will still work correctly.
         pStrRemoteList = cmsMem_alloc(sizeof(char *) * numRemoteEntries, ALLOC_ZEROIZE);
         if (pStrRemoteList == NULL)
         {
            rc = CMSRET_RESOURCE_EXCEEDED;
            goto exit;
         }

         for (i = 0; i < numRemoteEntries; i++)
         {
            // pStrRemoteList will just point to the fullpath string buffer in
            // the pRemoteList.  So pStrRemoteList does not own the fullpath
            // buffer.  So don't free it when pStrRemoteList is freed.
            pStrRemoteList[i] = pRemoteList[i].fullpath;
         }
      }
   }

   cmsLog_notice("%s => numLocal=%d, numRemote=%d",
                 fullpath, numLocalEntries, numRemoteEntries);

   rc = cmsPhl_mergeParams(pStrLocalList, numLocalEntries,
                           pStrRemoteList, numRemoteEntries,
                           &pIndexTable, &numIndex, FALSE);

   // We are done with pStrLocalList and pStrRemoteList now.  (They are only
   // needed for mergeParams.  Note that since these arrays did not own the
   // fullpath buffer in the first place, just free the array and not any
   // internal buffer pointers.
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrLocalList);
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrRemoteList);

   if (rc != CMSRET_SUCCESS)
   {
       cmsLog_error("Failed to merge param info! ret=%d", rc);
       goto exit;
   }

   // This is the final, merged result array
   pResult = cmsMem_alloc(numIndex * sizeof(PhlGetParamNameEx_t), ALLOC_ZEROIZE);
   if (pResult == NULL)
   {
       cmsLog_error("Failed to allocate pResult! (numIndex=%d)", numIndex);
       rc = CMSRET_RESOURCE_EXCEEDED;
       goto exit;
   }

   cmsLog_notice("numIndex=%d", numIndex);

   for (i = 0; i < numIndex; i++)
   {
       index = pIndexTable[i].index;
       if (pIndexTable[i].source == LOCAL)
       {
           if (pLocalVbuf != NULL)
           {
               // calculate the offset into the VBuf
               size_t offset = index * sizeof(PhlGetParamNameEx_t);

               // copy the entire PhlGetParamNameEx struct from pLocalVbuf
               // to pResult[i], which now owns the fullpath.  (the type and
               // profile strings are "static", so they should not be freed.)
               cmsVbuf_getAtOffset(pLocalVbuf, offset,
                                   &(pResult[i]), sizeof(PhlGetParamNameEx_t));
           }
       }
       else if (pIndexTable[i].source == REMOTE)
       {
           // See comment above.
           if (pRemoteList != NULL)
           {
               pResult[i] = pRemoteList[index];
               pRemoteList[index].fullpath = NULL;
           }
       }
       else
       {
           cmsLog_error("Unhandled indexTable entry %d", i);
       }
   }

   *paramNameArray = pResult;
   *numParamNames = numIndex;

exit:
   if (pIndexTable != NULL)
   {
       freeParamIndexTable(pIndexTable, numIndex);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(pLocalPathList);
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrLocalList);
   if (rc != CMSRET_SUCCESS)
   {
      // There was an error, so we did not transfer ownership of fullpaths
      // in the Vbuf to the caller, so we need to free them.
      freeFullpathsInParamNamesExVbuf(pLocalVbuf);
   }
   cmsVbuf_destroy(pLocalVbuf);  // regardless of error or success, always destroy vbuf.
   cmsPhl_freeParamNamesEx(&pRemoteList, numRemoteEntries);

   cmsLog_debug("ret=%d (numParamNames=%d)", rc, *numParamNames);
   return rc;
}

CmsRet cmsPhl_getParameterNames(const MdmPathDescriptor *pPath,
                                UBOOL8            nextLevelOnly,
                                PhlGetParamInfo_t **pParamInfoList,
                                SINT32            *pNumEntries)
{
   char *fullpath = NULL;
   SINT32 i, numParamNames=0;
   PhlGetParamNameEx_t *paramNameArray = NULL;
   CmsRet ret;

   if ((pPath == NULL) || (pParamInfoList == NULL) || (pNumEntries == NULL))
   {
      cmsLog_error("Invalid input args");
      return CMSRET_INVALID_ARGUMENTS;
   }

   ret = cmsMdm_pathDescriptorToFullPath(pPath, &fullpath);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = cmsPhl_getParameterNamesEx(fullpath, nextLevelOnly, 0,
                                    &paramNameArray, &numParamNames);

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);

   if (ret == CMSRET_SUCCESS)
   {
      PhlGetParamInfo_t *result;

      result = cmsMem_alloc(numParamNames * sizeof(PhlGetParamInfo_t), ALLOC_ZEROIZE);
      if (result == NULL)
      {
         cmsLog_error("failed to allocate %d entries", numParamNames);
         cmsPhl_freeParamNamesEx(&paramNameArray, numParamNames);
         return CMSRET_RESOURCE_EXCEEDED;
      }

      for (i=0; i < numParamNames; i++)
      {
         // convert new PhlGetParamNameEx_t to the old PhlGetParamInfo_t
         ret = cmsMdm_fullPathToPathDescriptor(paramNameArray[i].fullpath,
                                         &(result[i].pathDesc));
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", paramNameArray[i].fullpath, ret);
         }

         result[i].profile = paramNameArray[i].profile;  // const string ptr
         result[i].writable = paramNameArray[i].writable;
         result[i].isTr69Password = paramNameArray[i].isTr69Password;
      }

      cmsPhl_freeParamNamesEx(&paramNameArray, numParamNames);
      *pParamInfoList = result;
      *pNumEntries = numParamNames;
   }

   return ret;
}

void cmsPhl_freeParamNamesEx(PhlGetParamNameEx_t **paramNameArray,
                             SINT32                 numParamNames)
{
   SINT32 i;
   PhlGetParamNameEx_t *nameEx;

   if (paramNameArray == NULL)
      return;

   nameEx = *paramNameArray;
   if (nameEx == NULL)
      return;

   for (i=0; i < numParamNames; i++, nameEx++)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(nameEx->fullpath);
      // type and profile point to static string bufs, so do not free.
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(*paramNameArray);
   return;
}

CmsRet cmsPhl_convertParamNameExToGenericParamInfo(
                           PhlGetParamNameEx_t **paramNameArray,
                           SINT32 numParamNames,
                           BcmGenericParamInfo **paramInfoArray)
{
   SINT32 i;
   UINT32 len;
   PhlGetParamNameEx_t *src = NULL;
   BcmGenericParamInfo *dest = NULL;

   if (numParamNames == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (paramNameArray == NULL || paramInfoArray == NULL)
   {
      cmsLog_error("NULL input args %p/%p", paramNameArray, paramInfoArray);
      return CMSRET_INVALID_ARGUMENTS;
   }

   src = *paramNameArray;
   if (src == NULL)
   {
      cmsLog_error("No source array");
      return CMSRET_INVALID_ARGUMENTS;
   }

   len =  ((UINT32) numParamNames) * sizeof(BcmGenericParamInfo);
   dest = cmsMem_alloc(len, ALLOC_ZEROIZE);
   if (dest == NULL)
   {
      cmsLog_error("allocate %d bytes failed", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i = 0; i < numParamNames; i++)
   {
      dest[i].fullpath = src[i].fullpath;
      src[i].fullpath = NULL;  // transfering ownership to new array
      dest[i].type = cmsMem_strdup(src[i].type);  // src is a const string (not freeable), dest is a freeable string.
      dest[i].profile = cmsMem_strdup(src[i].profile);  // same as type
      dest[i].writable = src[i].writable;
      dest[i].isPassword = src[i].isTr69Password;
   }

   // Since we have stolen the fullpath from the source array, free it
   // completely so caller does not try to use it again.  The array ptr will be
   // set to NULL by this free.
   cmsPhl_freeParamNamesEx(paramNameArray, numParamNames);

   *paramInfoArray = dest;
   return CMSRET_SUCCESS;
}


CmsRet cmsPhl_convertParamValueVbufToGenericParamInfo(
                           CmsVbuf **vbuf,
                           SINT32 numParamValues,
                           BcmGenericParamInfo **paramInfoArray)
{
   UINT32 i = 0;
   UINT32 len;
   CmsVbuf *vb = NULL;
   PhlGetParamValue_t paramValue;
   BcmGenericParamInfo *dest = NULL;

   if (numParamValues == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (vbuf == NULL || paramInfoArray == NULL)
   {
      cmsLog_error("NULL input args %p/%p", vbuf, paramInfoArray);
      return CMSRET_INVALID_ARGUMENTS;
   }
   vb = *vbuf;

   len =  ((UINT32) numParamValues) * sizeof(BcmGenericParamInfo);
   dest = (BcmGenericParamInfo *) cmsMem_alloc(len, ALLOC_ZEROIZE);
   if (dest == NULL)
   {
      cmsLog_error("allocate %d bytes failed", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   cmsVbuf_resetIndex(vb);
   while (cmsVbuf_get(vb, &paramValue, sizeof(paramValue)) == CMSRET_SUCCESS)
   {
      CmsRet r2;
      r2 = cmsMdm_pathDescriptorToFullPath(&(paramValue.pathDesc),
                                           &(dest[i].fullpath));  // paramInfoArray owns fullpath
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid pathDesc oid=%d paramName=%s(ret=%d)",
                      paramValue.pathDesc.oid, paramValue.pathDesc.paramName, r2);
         // This should not happen since we are converting an array returned
         // by cmsPhl_getParameterValues.  Hard to undo anyways, so just keep
         // going.
      }
      dest[i].value = paramValue.pValue;  // paramInfoArray owns value
      dest[i].type = cmsMem_strdup(paramValue.pParamType);  // src is a const string (not freeable), dest is a freeable string.
      dest[i].profile = cmsMem_strdup(paramValue.profile);  // same as type
      dest[i].writable = paramValue.writable;
      dest[i].isPassword = paramValue.isTr69Password;
      i++;
   }

   // Since we have stolen the value from the source Vbuf, there are no
   // internal string pointers to free.  Just need to free the vbuf itself,
   // and set to NULL so the caller does not try to use it again.
   cmsVbuf_destroy(vb);
   *vbuf = NULL;

   *paramInfoArray = dest;
   return CMSRET_SUCCESS;
}

CmsRet cmsPhl_convertGenericParamInfoToParamValue(
                           BcmGenericParamInfo **paramInfoArray,
                           SINT32 numParamInfos,
                           PhlGetParamValue_t **paramValueArray)
{
   CmsRet ret = CMSRET_SUCCESS;
   BcmGenericParamInfo *paramInfo;
   PhlGetParamValue_t *result = NULL;
   SINT32 i;

   if (numParamInfos == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (paramInfoArray == NULL || paramValueArray == NULL)
   {
      cmsLog_error("NULL input args %p/%p", paramInfoArray, paramValueArray);
      return CMSRET_INVALID_ARGUMENTS;
   }

   paramInfo = *paramInfoArray;
   if (paramInfo == NULL)
   {
      cmsLog_error("No source array");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Allocate array of GetParamValues
   result = cmsMem_alloc(numParamInfos * sizeof(PhlGetParamValue_t), ALLOC_ZEROIZE);
   if (result == NULL)
   {
      cmsLog_error("Could not allocate array of %d PhlGetParamValue_t's", numParamInfos);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // convert GenericParamInfo to PhlGetParamValue_t
   for (i=0; i < numParamInfos; i++)
   {
      ret = cmsMdm_fullPathToPathDescriptor(paramInfo[i].fullpath, &(result[i].pathDesc));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("fullPathToPathDescriptor failed on [%d] %s, ret=%d",
                      i, paramInfo[i].fullpath, ret);
         cmsPhl_freeGetParamValueBuf(result, numParamInfos);
         break;
      }
      result[i].pValue = paramInfo[i].value;  // transfer ownership of value buffer.
      paramInfo[i].value = NULL;
      // PhlGetParamValue_t does not free type and profile, so they have to
      // point to a non-allocated (const) string.
      result[i].pParamType = cmsMdm_getConstTypeString(paramInfo[i].type);
      result[i].profile = cmsMdm_getConstProfileString(paramInfo[i].profile);
      result[i].writable = paramInfo[i].writable;
      result[i].isTr69Password = paramInfo[i].isPassword;
   }

   if (ret == CMSRET_SUCCESS)
   {
      cmsUtl_freeParamInfoArray(paramInfoArray, numParamInfos);
      *paramValueArray = result;
   }

   return ret;
}



CmsRet local_setParamAttributes(const PhlSetParamAttr_t *pSetParamAttrList,
                                SINT32            numEntries)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            i;
   const PhlSetParamAttr_t *pSetParamAttr;
   UBOOL8            testOnly = TRUE;
   CmsRet            rc = CMSRET_SUCCESS;

   if ((rc = phl_fillLockZonesSetParamAttr(pSetParamAttrList, numEntries, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pSetParamAttrList->pathDesc.oid);

   /*
    * We need to loop through the setParamAttrList twice,
    * first with testOnly = TRUE, and if there are no errors, loop
    * through a second time with testOnly = FALSE.  This is to satisfy
    * TR69 atomic set requirements.  See A.3.2.4.
    */

   /* Perhaps we should only allow EID_TR69C to do this?  i.e. check mdmLibCtx.eid? */

   /* loop through the set parameter attribute list */
   pSetParamAttr = pSetParamAttrList;
   for (i = 0; i < numEntries && rc == CMSRET_SUCCESS; i++, pSetParamAttr++)
   {
      if ((pSetParamAttr->attributes.accessBitMaskChange == 0) &&
          (pSetParamAttr->attributes.notificationChange  == 0) &&
          (pSetParamAttr->attributes.setAltNotif  == 0) &&
          (pSetParamAttr->attributes.clearAltNotifValue  == 0))
      {
         continue;   /* does not change anything */
      }

      rc = mdm_setParamAttributes(&(pSetParamAttr->pathDesc), &(pSetParamAttr->attributes), testOnly);
      if (rc != CMSRET_SUCCESS)
      {
         lck_autoUnlockZones(zones, pSetParamAttrList->pathDesc.oid, __FUNCTION__);
         return rc;
      }
   }  /* for (i = 0; ....) */

   /* OK, the test run succeeded, now do the real set */
   testOnly = FALSE;
   pSetParamAttr = pSetParamAttrList;
   for (i = 0; i < numEntries && rc == CMSRET_SUCCESS; i++, pSetParamAttr++)
   {
      if ((pSetParamAttr->attributes.accessBitMaskChange == 0) &&
          (pSetParamAttr->attributes.notificationChange  == 0) &&
          (pSetParamAttr->attributes.setAltNotif== 0) &&
          (pSetParamAttr->attributes.clearAltNotifValue  == 0))
      {
         continue;   /* does not change anything */
      }

      rc = mdm_setParamAttributes(&(pSetParamAttr->pathDesc), &(pSetParamAttr->attributes), FALSE);
      if (rc != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_setParamAttributes failure %d", rc);
      }
   }  /* for (i = 0; ....) */

   lck_autoUnlockZones(zones, pSetParamAttrList->pathDesc.oid, __FUNCTION__);

   return rc;

}

CmsRet localGeneric_setParamAttributesFlags(BcmGenericParamAttr *paramAttrArray,
                                      SINT32 numEntries,
                                      UINT32 setFlags __attribute__((unused)))
{
   PhlSetParamAttr_t *setParamAttrList;
   CmsRet ret;

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }
   if (paramAttrArray == NULL)
   {
      cmsLog_error("NULL paramAttrArray");
      return CMSRET_INTERNAL_ERROR;
   }

   ret = cmsPhl_copyGenericParamAttrToSetParamAttr(paramAttrArray,
                                                   numEntries,
                                                   &setParamAttrList);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = local_setParamAttributes(setParamAttrList, numEntries);

   // setParamAttrList does not contain any strings, so a simple cmsMem_free is sufficient.
   CMSMEM_FREE_BUF_AND_NULL_PTR(setParamAttrList);

   return ret;
}

// This was the old way, which uses PhlSetParamValue_t and MdmPathDescriptor,
// which cannot handle remote parameters which are not in the CMS/BDK data model.
CmsRet cmsPhl_setParameterAttributes(const PhlSetParamAttr_t *pSetParamAttrList,
                                     SINT32            numEntries)
{
   BcmGenericParamAttr *paramAttrArray = NULL;
   CmsRet ret;

   ret = cmsPhl_copySetParamAttrToGenericParamAttr(pSetParamAttrList,
                                                   numEntries,
                                                   &paramAttrArray);

   ret = bcmGeneric_setParameterAttributesFlags(paramAttrArray, numEntries, 0);

   cmsUtl_freeParamAttrArray(&paramAttrArray, numEntries);

   return ret;
}

// This is the new (generic) way as of 504L04.  Takes array of
// BcmGenericParamAttr's.  On return, the errorCode in the
// BcmGenericParamAttr's may be set to indicate errors on the individual params.
CmsRet bcmGeneric_setParameterAttributesFlags(BcmGenericParamAttr *paramAttrArray,
                                              SINT32 numEntries,
                                              UINT32 setFlags)
{
   SINT32 i, localIdx, remoteIdx;
   BcmGenericParamAttr *pLocalList, *pRemoteList;
   CmsRet rc = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (paramAttrArray == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   for (i = 0; i < numEntries; i++)
   {
      if (paramAttrArray[i].fullpath == NULL)
      {
         cmsLog_error("NULL fullpath detected at param %d", i);
         paramAttrArray[i].errorCode = CMSRET_INVALID_ARGUMENTS;
         rc = CMSRET_INVALID_ARGUMENTS;
      }
   }
   if (rc != CMSRET_SUCCESS)
   {
      return rc;
   }

   // For sake of simplicity, just allocate a local and remote array that
   // can hold the entire input array.
   pRemoteList = cmsMem_alloc(sizeof(BcmGenericParamAttr) * numEntries, ALLOC_ZEROIZE);
   if (pRemoteList == NULL)
   {
      cmsLog_error("Failed to allocate pRemoteList! (numEntries=%d)", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   pLocalList = cmsMem_alloc(sizeof(BcmGenericParamAttr) * numEntries, ALLOC_ZEROIZE);
   if (pLocalList == NULL)
   {
      cmsLog_error("Failed to allocate pLocalList! (numEntries=%d)", numEntries);
      cmsMem_free(pRemoteList);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // Fill in remote and local arrays
   localIdx = remoteIdx = 0;
   for (i = 0; i < numEntries; i++)
   {
      if (mdm_isLocalFullpath(paramAttrArray[i].fullpath))
      {
         // Note this is a "shallow copy": pRemoteList and pLocalList does not
         // take ownership of fullpath.
         memcpy(pLocalList+localIdx, paramAttrArray + i, sizeof(BcmGenericParamAttr));
         localIdx++;
      }
      else
      {
         memcpy(pRemoteList+remoteIdx, paramAttrArray + i, sizeof(BcmGenericParamAttr));
         remoteIdx++;
      }
   }

   // CDRouter test: sends a bad fullpaths such as "Device.ManagementServer.PeriodicInformEnabl".
   // Try to process this local bad fullpath first so we don't process any
   // of the remote attributes.  TODO: we now have enough info verify all the
   // the remote param names here before sending to remote side.
   // see cmsMdm_fullPathToPathDescriptorEx() and mdm_lookupPathDescEx().
   if (localIdx > 0)
   {
      cmsLog_notice("Setting %d local paramAttrs", localIdx);
      rc = localGeneric_setParamAttributesFlags(pLocalList, localIdx, setFlags);
      if (rc != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to set local param attributes! rc=%d", rc);
          goto exit;
      }
   }
   if (remoteIdx > 0)
   {
      cmsLog_notice("Setting %d remote paramAttrs", remoteIdx);
      rc = remote_setParamAttributes(pRemoteList, remoteIdx, setFlags);
      if (rc != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to set remote param attributes! rc=%d", rc);
          goto exit;
      }
   }

   /*
    * Currently, the low level setParameterAttributes code does not report
    * errors on an individual parameter basis, so there is no need to copy
    * back the individual errors.
    */

exit:
   // Even though pRemoteList and pLocalList are arrays of BcmGenericParamAttr's, they
   // never took ownership of the string pointers, so they must be freed using
   // a simple cmsMem_free and not with cmsUtl_freeParamAttrArray.
   cmsMem_free(pRemoteList);
   cmsMem_free(pLocalList);
   return rc;
}  /* End of bcmGeneric_setParameterAttributesFlags() */


// XXX This simplified version of cmsPhl_getParameterAttributes can only
// get from the local MDM.  It has not been converted for Distributed MDM.
// All callers should use cmsPhl_getParameterAttributes().  This function
// will be deleted by 504L03.
CmsRet cmsPhl_getParamAttr(const MdmPathDescriptor *pPath,
                           PhlGetParamAttr_t **pParamAttr)
{
   PhlGetParamAttr_t    *pResp;
   CmsRet               rc = CMSRET_SUCCESS;

   if ((pPath == NULL) || (pParamAttr == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamAttr = NULL;

   if (pPath->paramName[0] == 0)
   {
      cmsLog_error("invalid parameter name");
      return CMSRET_INVALID_PARAM_NAME;
   }

   /* allocate memory for parameter value response */
   pResp = cmsMem_alloc(sizeof(PhlGetParamAttr_t), ALLOC_ZEROIZE);
   if (pResp == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      return CMSRET_INTERNAL_ERROR;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(pResp);
      return rc;
   }

   /* get parameter attributes from MDM */
   rc = mdm_getParamAttributes(pPath, &(pResp->attributes));
   if (rc == CMSRET_SUCCESS)
   {
      pResp->pathDesc = *pPath;
      *pParamAttr     = pResp;
   }
   else
   {
      cmsLog_error("mdm_getParamAttributes error %d", rc);
      cmsMem_free(pResp);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParamAttr() */


static CmsRet local_getParamAttributes(const MdmPathDescriptor *pPathList,
                                     SINT32            numEntries,
                                     UBOOL8            nextLevelOnly,
                                     UINT32            flags,
                                     CmsVbuf          **pResultVbuf,
                                     SINT32            *pNumParamAttrEntries)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            i, numRespEntries=0;
   const MdmPathDescriptor *pPath;
   PhlGetParamAttr_t paramAttr;
   CmsVbuf          *vbuf = NULL;
   CmsRet            rc = CMSRET_SUCCESS;

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }

   if ((rc = phl_fillLockZones(pPathList, numEntries, nextLevelOnly, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPathList->oid);

   /* allocate memory for parameter attribute response */
   vbuf = cmsVbuf_new();
   if (vbuf == NULL)
   {
      cmsLog_error("cmsVbuf_new failed");
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }


   /* loop through the requested attributes list */
   for (i = 0, pPath = pPathList;
        i < numEntries && rc == CMSRET_SUCCESS;
        i++, pPath++)
   {
      if (pPath->paramName[0] != 0)
      {
         /* this is a parameter path */
         memset(&paramAttr, 0, sizeof(paramAttr));
         rc = mdm_getParamAttributes(pPath, &(paramAttr.attributes));
         if (rc == CMSRET_SUCCESS)
         {
            paramAttr.pathDesc = *pPath;
            cmsVbuf_put(vbuf, &paramAttr, sizeof(paramAttr));
            numRespEntries++;
         }
         else
         {
            cmsLog_error("mdm_getParamAttributes error on oid %d param %s, rc=%d",
                         pPath->oid, pPath->paramName, rc);
         }
      }
      else
      {
         /* this is an object path */
         /* traverse the sub-tree below the object node */
         MdmPathDescriptor nextPath;

         /* set nextPath to 0 to start traversing */
         memset(&nextPath, 0, sizeof(MdmPathDescriptor));

         while (rc == CMSRET_SUCCESS)
         {
            rc = phl_getNextPath(TRUE, nextLevelOnly, pPath, &nextPath, flags);
            if (rc == CMSRET_SUCCESS)
            {
               memset(&paramAttr, 0, sizeof(paramAttr));
               rc = mdm_getParamAttributes(&nextPath, &(paramAttr.attributes));
               if (rc == CMSRET_SUCCESS)
               {
                  paramAttr.pathDesc = nextPath;
                  cmsVbuf_put(vbuf, &paramAttr, sizeof(paramAttr));
                  numRespEntries++;
               }
               else
               {
                  cmsLog_error("mdm_getParamAttributes error on oid %d param %s, rc=%d",
                               nextPath.oid, nextPath.paramName, rc);
               }
            }
            else if (rc == CMSRET_NO_MORE_INSTANCES)
            {
               rc = CMSRET_SUCCESS;
               break;
            }
            else
            {
               cmsLog_error("phl_getNextPath error %d", rc);
            }
         }  /* while (rc == CMSRET_SUCCESS) */
      }
   }  /* for (i = 0; ....) */

   if (rc == CMSRET_SUCCESS)
   {
      *pResultVbuf          = vbuf;
      *pNumParamAttrEntries = numRespEntries;
   }
   else
   {
      // There are no embedded string pointers inside this vbuf, so just
      // destroy the vbuf itself.
      cmsVbuf_destroy(vbuf);
   }

   lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);

   return rc;

}

static CmsRet localGeneric_getParamAttributes(const MdmPathDescriptor *pPathList,
                                     SINT32            numEntries,
                                     UBOOL8            nextLevelOnly,
                                     UINT32            flags,
                                     BcmGenericParamAttr **pParamAttrList,
                                     SINT32            *pNumParamAttrEntries)
{
   CmsVbuf *vbuf = NULL;
   SINT32 numParamAttrs = 0;
   BcmGenericParamAttr *result = NULL;
   CmsRet ret;

   ret = local_getParamAttributes(pPathList, numEntries,
                                  nextLevelOnly, flags,
                                  &vbuf, &numParamAttrs);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   // Convert Vbuf of PhlGetParamAttr_t's to BcmGenericParamAttr
   ret = cmsPhl_convertGetParamAttrVbufToGenericParamAttr(
                                 &vbuf, numParamAttrs, &result);
   if (ret != CMSRET_SUCCESS)
   {
      // on failure, caller needs to free the Vbuf of phlParamAttr_t's
      cmsVbuf_destroy(vbuf);
      return ret;
   }

   // On success, vbuf is freed by the conversion function.
   // Give result (BcmGenericParamAttr) to caller.
   *pParamAttrList = result;
   *pNumParamAttrEntries = numParamAttrs;

   return ret;
}


// This was the old way, which uses MdmPathDescriptor and PhlGetParamAttr_t,
// which cannot handle remote parameters which are not in the CMS/BDK data model.
CmsRet cmsPhl_getParameterAttributesFlags(const MdmPathDescriptor *pPathList,
                                     SINT32            numEntries,
                                     UBOOL8            nextLevelOnly,
                                     UINT32            getFlags,
                                     PhlGetParamAttr_t **pParamAttrList,
                                     SINT32            *pNumParamAttrEntries)
{
   char **fullpathArray;
   BcmGenericParamAttr *paramAttrArray = NULL;
   SINT32 i, numParamAttrs = 0;
   PhlGetParamAttr_t *result = NULL;
   CmsRet ret, r2;

   if (numEntries == 0)
   {
      return CMSRET_SUCCESS;
   }
   if ((pPathList == NULL) ||
       (pParamAttrList == NULL) ||
       (pNumParamAttrEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Convert pPathList (MdmPathDescriptors) to fullpaths
   fullpathArray = (char **) cmsMem_alloc(numEntries * sizeof(char *), ALLOC_ZEROIZE);
   if (fullpathArray == NULL)
   {
      cmsLog_error("Failed to allocate %d entries", numEntries);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   for (i=0; i < numEntries; i++)
   {
      r2 = cmsMdm_pathDescriptorToFullPath(&(pPathList[i]), &(fullpathArray[i]));
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Bad pathDesc at [%d] oid=%d paramName=%s", i,
                      pPathList[i].oid, pPathList[i].paramName);
         cmsUtl_freeArrayOfStrings(&fullpathArray, numEntries);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   ret = bcmGeneric_getParameterAttributesFlags((const char **) fullpathArray, numEntries,
                                            nextLevelOnly, getFlags,
                                            &paramAttrArray, &numParamAttrs);

   // Done with fullpathArray, free it.
   cmsUtl_freeArrayOfStrings(&fullpathArray, numEntries);

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = cmsPhl_convertGenericParamAttrToGetParamAttr(
                                   &paramAttrArray, numParamAttrs, &result);
   if (ret != CMSRET_SUCCESS)
   {
      // on failure, caller needs to free the paramAttrArray
      cmsUtl_freeParamAttrArray(&paramAttrArray, numParamAttrs);
      return ret;
   }

   // On success, paramAttrArray is freed by the conversion func.
   // Give result (PhlGetParamAttr_t) to caller.
   *pParamAttrList = result;
   *pNumParamAttrEntries = numParamAttrs;

   return ret;

}

// This is the new (generic) way as of 504L04.  Takes array of fullpaths.
// Returns BcmGenericParamAttr.
CmsRet bcmGeneric_getParameterAttributesFlags(const char **fullpathArray,
                                     SINT32            numEntries,
                                     UBOOL8            nextLevelOnly,
                                     UINT32            getFlags,
                                     BcmGenericParamAttr **pParamAttrList,
                                     SINT32            *pNumParamAttrEntries)
{
   SINT32 numRemote = 0, numLocal = 0;
   char **remoteFullpathArray = NULL;
   MdmPathDescriptor *pLocalPathList = NULL;
   BcmGenericParamAttr *pRemoteList = NULL, *pLocalList = NULL, *pResult = NULL;
   char              **pStrLocalList, **pStrRemoteList;
   SINT32            i, index, numRemoteEntries, numLocalEntries, numIndex;
   ParamIndexEntry   *pIndexTable;
   CmsRet rc = CMSRET_SUCCESS;

   if ((pParamAttrList == NULL) || (pNumParamAttrEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamAttrList       = NULL;
   *pNumParamAttrEntries = 0;
   pStrLocalList = pStrRemoteList = NULL;
   numRemoteEntries = numLocalEntries = numIndex = 0;
   pIndexTable = NULL;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Our long-standing existing behavior is when the input numEntries is 0,
      // we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (fullpathArray == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((getFlags & OGF_LOCAL_MDM_ONLY) == 0)
   {
      rc = phl_getPossibleRemoteFullpaths(fullpathArray, numEntries,
                                  &remoteFullpathArray, &numRemote);
      if (rc != CMSRET_SUCCESS)
      {
         goto exit;
      }
   }

   rc = phl_getLocalPathDescListFromFullpath(fullpathArray, numEntries,
                                             &pLocalPathList, &numLocal);
   if (rc != CMSRET_SUCCESS)
   {
       goto exit;
   }

   cmsLog_notice("numEntries=%d, numRemote=%d, numLocal=%d", numEntries, numRemote, numLocal);

   numLocalEntries = 0;
   numRemoteEntries = 0;
   if (numRemote > 0)
   {
      rc = remote_getParamAttributes((const char **) remoteFullpathArray, numRemote,
                                     nextLevelOnly, getFlags,
                                     &pRemoteList, &numRemoteEntries);
      if (rc != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to get remote param attrs! numRemote=%d rc=%d",
                       numRemote, rc);
          goto exit;
      }
      // Note that numRemoteEntries might be 0, but pStrRemoteList will still be
      // non-null.  The algorithm below will still work correctly.
      pStrRemoteList = cmsMem_alloc(sizeof(char *) * numRemoteEntries, ALLOC_ZEROIZE);
      if (pStrRemoteList == NULL)
      {
          rc = CMSRET_RESOURCE_EXCEEDED;
          goto exit;
      }

      // create an array of fullpaths for merging.
      // Note that numRemoteEntries might be 0, but pStrRemoteList will still be
      // non-null.  The algorithm below will still work correctly.
      for (i = 0; i < numRemoteEntries; i++)
      {
          // phl_merge will make a copy of the string before merging, so it
          // is safe to give it a pointer to our own string.
          pStrRemoteList[i] = pRemoteList[i].fullpath;
      }
   }

   if (numLocal > 0)
   {
      rc = localGeneric_getParamAttributes(pLocalPathList, numLocal,
                                           nextLevelOnly, getFlags,
                                           &pLocalList, &numLocalEntries);
      if (rc != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to get local param attrs! numLocal=%d rc=%d",
                       numLocal, rc);
          goto exit;
      }

      // See comments above
      pStrLocalList = cmsMem_alloc(sizeof(char *) * numLocalEntries, ALLOC_ZEROIZE);
      if (pStrLocalList == NULL)
      {
          cmsLog_error("failed to allocate %d local entries", numLocalEntries);
          rc = CMSRET_RESOURCE_EXCEEDED;
          goto exit;
      }

      for (i = 0; i < numLocalEntries; i++)
      {
          pStrLocalList[i] = pLocalList[i].fullpath;
      }
   }

   rc = cmsPhl_mergeParams(pStrLocalList, numLocalEntries,
                           pStrRemoteList, numRemoteEntries,
                           &pIndexTable, &numIndex, FALSE);

   if (rc != CMSRET_SUCCESS)
   {
       cmsLog_error("Failed to merge param attr! ret=%d", rc);
       goto exit;
   }

   pResult = cmsMem_alloc(numIndex * sizeof(BcmGenericParamAttr), ALLOC_ZEROIZE);
   if (pResult == NULL)
   {
       cmsLog_error("Failed to allocate pResult! numIndex=%d", numIndex);
       rc = CMSRET_RESOURCE_EXCEEDED;
       goto exit;
   }

   cmsLog_notice("merged count numIndex=%d", numIndex);

   for (i = 0; i < numIndex; i++)
   {
       index = pIndexTable[i].index;
       if (pIndexTable[i].source == LOCAL)
       {
           if (pLocalList != NULL)
           {
               pResult[i] = pLocalList[index];
               // pLocalList transfered the ownership of fullpath to pResult
               pLocalList[index].fullpath = NULL;
           }
       }
       else if (pIndexTable[i].source == REMOTE)
       {
           if (pRemoteList != NULL)
           {
               pResult[i] = pRemoteList[index];
               // pRemoteList transfered the ownership of fullpath to pResult
               pRemoteList[index].fullpath = NULL;
           }
       }
       else
       {
           cmsLog_error("Unhandled indexTable entry %d", i);
       }
   }

   *pParamAttrList = pResult;
   *pNumParamAttrEntries = numIndex;

exit:
   if (pIndexTable != NULL)
   {
       freeParamIndexTable(pIndexTable, numIndex);
   }

   if (pLocalList != NULL)
   {
       cmsUtl_freeParamAttrArray(&pLocalList, numLocalEntries);
   }
   if (pLocalPathList != NULL)
   {
       cmsMem_free(pLocalPathList);
   }
   // pStrLocalList did not take any ownership of the strings, so we just
   // need to free the array buffer here.
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrLocalList);

   if (pRemoteList != NULL)
   {
       cmsUtl_freeParamAttrArray(&pRemoteList, numRemoteEntries);
   }
   cmsUtl_freeArrayOfStrings(&remoteFullpathArray, numRemote);
   // pStrRemoteList did not take any ownership of the strings, so we just
   // need to free the array buffer here.
   CMSMEM_FREE_BUF_AND_NULL_PTR(pStrRemoteList);

   return rc;
}  /* End of bcmGeneric_getParameterAttributes() */


CmsRet cmsPhl_convertGetParamAttrVbufToGenericParamAttr(
                           CmsVbuf **vbuf,
                           SINT32 numParamAttrs,
                           BcmGenericParamAttr **paramAttrArray)
{
   PhlGetParamAttr_t phlAttrs;
   CmsVbuf *vb = NULL;
   BcmGenericParamAttr *dst = NULL;
   SINT32 i = 0;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   if (numParamAttrs == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (vbuf == NULL || paramAttrArray == NULL)
   {
      cmsLog_error("NULL input args %p/%p", vbuf, paramAttrArray);
      return CMSRET_INVALID_ARGUMENTS;
   }
   vb = *vbuf;

   dst = cmsMem_alloc(numParamAttrs * sizeof(BcmGenericParamAttr),
                      ALLOC_ZEROIZE);
   if (dst == NULL)
   {
      cmsLog_error("failed to allocate %d BcmGenericParamAttr", numParamAttrs);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   cmsVbuf_resetIndex(vb);
   while (cmsVbuf_get(vb, &phlAttrs, sizeof(phlAttrs)) == CMSRET_SUCCESS)
   {
      ret = cmsMdm_pathDescriptorToFullPath(&(phlAttrs.pathDesc),
                                            &(dst[i].fullpath));  // paramAttrArray owns fullpath
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid pathDesc oid=%d paramName=%s (ret=%d)",
                      phlAttrs.pathDesc.oid, phlAttrs.pathDesc.paramName, ret);
         cmsUtl_freeParamAttrArray(&dst, numParamAttrs);
         break;
      }

      dst[i].setAccess = phlAttrs.attributes.accessBitMaskChange;
      dst[i].access = phlAttrs.attributes.accessBitMask;
      dst[i].setNotif = phlAttrs.attributes.notificationChange;
      dst[i].notif = phlAttrs.attributes.notification;
      dst[i].valueChanged= phlAttrs.attributes.valueChanged;
      dst[i].setAltNotif = phlAttrs.attributes.setAltNotif;
      dst[i].altNotif = phlAttrs.attributes.altNotif;
      dst[i].clearAltNotifValue = phlAttrs.attributes.clearAltNotifValue;
      dst[i].altNotifValue= phlAttrs.attributes.altNotifValue;
      i++;
   }

   if (ret == CMSRET_SUCCESS)
   {
      *paramAttrArray = dst;
      cmsVbuf_destroy(vb);  // on success, we destroy the Vbuf
      *vbuf = NULL;         // null out caller's pointer
   }

   return ret;
}

CmsRet cmsPhl_convertGenericParamAttrToGetParamAttr(
                           BcmGenericParamAttr **paramAttrArray,
                           SINT32 numParamAttrs,
                           PhlGetParamAttr_t **phlAttrArray)
{
   CmsRet ret = CMSRET_SUCCESS;
   BcmGenericParamAttr *paramAttr;
   PhlGetParamAttr_t *result = NULL;
   SINT32 i;

   if (numParamAttrs == 0)
   {
      // Our long-standing existing behavior is that when the input numEntries
      // is 0, we return SUCCESS.
      return CMSRET_SUCCESS;
   }
   if (paramAttrArray == NULL || phlAttrArray == NULL)
   {
      cmsLog_error("NULL input args %p/%p", paramAttrArray, phlAttrArray);
      return CMSRET_INVALID_ARGUMENTS;
   }

   paramAttr = *paramAttrArray;
   if (paramAttr == NULL)
   {
      cmsLog_error("No source array");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Allocate array of PhlGetParamAttrs
   result = cmsMem_alloc(numParamAttrs * sizeof(PhlGetParamAttr_t), ALLOC_ZEROIZE);
   if (result == NULL)
   {
      cmsLog_error("Could not allocate array of %d PhlGetParamAttr_t's", numParamAttrs);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   // convert GenericParamAttr to PhlGetParamAttr_t
   for (i=0; i < numParamAttrs; i++)
   {
      ret = cmsMdm_fullPathToPathDescriptor(paramAttr[i].fullpath, &(result[i].pathDesc));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("fullPathToPathDescriptor failed on [%d] %s, ret=%d",
                      i, paramAttr[i].fullpath, ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(result);
         break;
      }

      result[i].attributes.accessBitMaskChange = paramAttr[i].setAccess;
      result[i].attributes.accessBitMask = paramAttr[i].access;
      result[i].attributes.notificationChange = paramAttr[i].setNotif;
      result[i].attributes.notification = paramAttr[i].notif;
      result[i].attributes.valueChanged = paramAttr[i].valueChanged;
      result[i].attributes.setAltNotif = paramAttr[i].setAltNotif;
      result[i].attributes.altNotif = paramAttr[i].altNotif;
      result[i].attributes.clearAltNotifValue = paramAttr[i].clearAltNotifValue;
      result[i].attributes.altNotifValue = paramAttr[i].altNotifValue;
   }

   if (ret == CMSRET_SUCCESS)
   {
      cmsUtl_freeParamAttrArray(paramAttrArray, numParamAttrs);
      *phlAttrArray = result;
   }

   return ret;
}


CmsRet cmsPhl_copyGenericParamAttrToSetParamAttr(
                           const BcmGenericParamAttr *paramAttrArray,
                           SINT32                     numParamAttrs,
                           PhlSetParamAttr_t        **phlAttrArray)
{
   const BcmGenericParamAttr *src = paramAttrArray;  // just shorten the var name
   PhlSetParamAttr_t *phlAttrs = NULL;
   SINT32 i = 0;
   CmsRet ret = CMSRET_SUCCESS;

   phlAttrs = cmsMem_alloc(numParamAttrs * sizeof(PhlSetParamAttr_t),
                           ALLOC_ZEROIZE);
   if (phlAttrs == NULL)
   {
      cmsLog_error("failed to allocate %d PhlSetParamAttr_t", numParamAttrs);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamAttrs; i++)
   {
      ret = cmsMdm_aliasedFullPathToPathDescriptor(src[i].fullpath,
                                            &(phlAttrs[i].pathDesc), NULL);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid fullpath %s", src[i].fullpath);
         break;
      }

      // TODO: log error if more data in src than can be accepted by phlAttr bitfields.
      phlAttrs[i].attributes.accessBitMaskChange = src[i].setAccess;
      phlAttrs[i].attributes.accessBitMask = src[i].access & 0x7fff;
      phlAttrs[i].attributes.notificationChange = src[i].setNotif;
      phlAttrs[i].attributes.notification = src[i].notif & 0x7f;
      phlAttrs[i].attributes.setAltNotif = src[i].setAltNotif;
      phlAttrs[i].attributes.altNotif = src[i].altNotif & 0xff;
      phlAttrs[i].attributes.clearAltNotifValue = src[i].clearAltNotifValue;
   }

   if (ret == CMSRET_SUCCESS)
   {
      // On success, return array to caller, who is responsible for freeing.
      *phlAttrArray = phlAttrs;
   }
   else
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(phlAttrs);
   }

   // Note this function does NOT free the source array.
   return ret;
}

CmsRet cmsPhl_copySetParamAttrToGenericParamAttr(
                               const PhlSetParamAttr_t *phlAttrArray,
                               SINT32                   numParamAttrs,
                               BcmGenericParamAttr    **paramAttrArray)
{
   BcmGenericParamAttr *dst = NULL;
   SINT32 i = 0;
   CmsRet ret = CMSRET_SUCCESS;

   dst = cmsMem_alloc(numParamAttrs * sizeof(BcmGenericParamAttr), ALLOC_ZEROIZE);
   if (dst == NULL)
   {
      cmsLog_error("failed to allocate %d BcmGenericParamAttr", numParamAttrs);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamAttrs; i++)
   {
      ret = cmsMdm_pathDescriptorToFullPath(&(phlAttrArray[i].pathDesc),
                                            &(dst[i].fullpath));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid pathDesc at %d (oid=%d)",
                      i, phlAttrArray[i].pathDesc.oid);
         break;
      }

      dst[i].setAccess = phlAttrArray[i].attributes.accessBitMaskChange;
      dst[i].access = phlAttrArray[i].attributes.accessBitMask;
      dst[i].setNotif = phlAttrArray[i].attributes.notificationChange;
      dst[i].notif = phlAttrArray[i].attributes.notification;
      dst[i].setAltNotif = phlAttrArray[i].attributes.setAltNotif;
      dst[i].altNotif = phlAttrArray[i].attributes.altNotif;
      dst[i].clearAltNotifValue = phlAttrArray[i].attributes.clearAltNotifValue;
   }

   if (ret == CMSRET_SUCCESS)
   {
      // On success, return array to caller, who is responsible for freeing
      // using cmsUtl_freeParamAttrArray.
      *paramAttrArray = dst;
   }
   else
   {
      cmsUtl_freeParamAttrArray(&dst, numParamAttrs);
   }

   // Note this function does NOT free the source array.
   return ret;
}



CmsRet cmsPhl_addObjInstanceByFullPath(const char *fullpath,
                                       UINT32 *newInstanceId)
{
   return (cmsPhl_addObjInstanceByFullPathFlags(fullpath, 0, newInstanceId));
}

CmsRet cmsPhl_addObjInstanceByFullPathFlags(const char *fullpath,
                                            UINT32 flags,
                                            UINT32 *newInstanceId)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   char aliasBuf[MDM_ALIAS_BUFLEN]={0};
   CmsRet ret;

   cmsLog_notice("Entered: fullpath=%s flags=0x%x", fullpath, flags);

   // it is ok if fullpath does not contain alias.
   // It is also ok if we cannot convert fullpath, it is probably a 
   // remote fullpath, which will be handled in odl_addObjectInstance.
   ret = cmsMdm_aliasedFullPathToPathDescriptor(fullpath, &pathDesc, aliasBuf);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("Failed to convert %s, probably remote", fullpath);
      // Normally, we don't lock for remote operations
      ret = odl_addObjectInstanceByFullPathFlags(fullpath, flags, newInstanceId);
   }
   else
   {
      // This could still be a remote fullpath, or local.
      ret = lck_autoLockZone(pathDesc.oid, __FUNCTION__);
      RETURN_IF_NOT_SUCCESS(ret);

      ret = odl_addObjectInstanceByFullPathFlags(fullpath, flags, newInstanceId);

      lck_autoUnlockZone(pathDesc.oid, __FUNCTION__);
   }

   // Do this after the unlock so that it becomes a new PHL operation.
   // TODO: setting the alias to the alias hint is only supported with objects
   // known to the MDM.  If the FullPathToPathDescriptor above failed, that
   // means the fullpath was not known to the MDM, so we cannot do this block.
   if ((ret == CMSRET_SUCCESS) && !IS_EMPTY_STRING(aliasBuf) &&
       (pathDesc.oid != 0))
   {
      // Set aliasBuf into fullpath.instance.Alias
      CmsRet r2;
      PhlSetParamValue_t setParamValue;
      memset(&setParamValue, 0, sizeof(setParamValue));
      cmsLog_notice("Setting Alias to %s", aliasBuf);

      PUSH_INSTANCE_ID(&(pathDesc.iidStack), *newInstanceId);
      setParamValue.pathDesc = pathDesc;
      sprintf(setParamValue.pathDesc.paramName, "Alias");
      setParamValue.pParamType = (char *) cmsMdm_paramTypeToString(MPT_STRING);
      setParamValue.pValue = aliasBuf;
      r2 = cmsPhl_setParameterValues(&setParamValue, 1);
      if (r2 != CMSRET_SUCCESS)
      {
         // TODO: setting Alias is very unlikely to fail, but if it does,
         // we need to delete the object instance.
         cmsLog_error("set of alias %s failed! r2=%d", aliasBuf, r2);
      }
   }

   return ret;
}

CmsRet cmsPhl_addObjInstance(MdmPathDescriptor *pathDesc)
{
   CmsRet ret;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   ret = lck_autoLockZone(pathDesc->oid, __FUNCTION__);
   RETURN_IF_NOT_SUCCESS(ret);

   ret = odl_addObjectInstance(pathDesc);

   lck_autoUnlockZone(pathDesc->oid, __FUNCTION__);
   return ret;
}

CmsRet cmsPhl_delObjInstance(const MdmPathDescriptor *pathDesc)
{
   return (cmsPhl_delObjInstanceFlags(pathDesc, 0));
}

CmsRet cmsPhl_delObjInstanceFlags(const MdmPathDescriptor *pathDesc,
                                  UINT32 flags)
{
   char *fullpath = NULL;
   CmsRet ret;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_notice("Entered: oid=%d iidStack=%s flags=0x%x",
                 pathDesc->oid,
                 cmsMdm_dumpIidStack(&(pathDesc->iidStack)), flags);

   ret = cmsMdm_pathDescriptorToFullPath(pathDesc, &fullpath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Bad pathDesc, oid=%d iidStack=%s",
                   pathDesc->oid,
                   cmsMdm_dumpIidStack(&(pathDesc->iidStack)));
      return ret;
   }

   ret = cmsPhl_delObjInstanceByFullPathFlags(fullpath, flags);

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
   return ret;
}

CmsRet cmsPhl_delObjInstanceByFullPathFlags(const char *fullpath,
                                            UINT32 flags)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   CmsRet ret;

   if (fullpath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_notice("Entered: fullpath=%s flags=0x%x", fullpath, flags);

   // It is ok if we cannot convert fullpath, it is probably a 
   // remote fullpath, which will be handled in odl_deleteObjectInstance.
   ret = cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("Failed to convert %s, probably remote", fullpath);
      // Normally, we don't lock for remote operations
      ret = odl_deleteObjectInstanceByFullPathFlags(fullpath, flags);
   }
   else
   {
      // This could still be a remote fullpath, or local.
      ret = lck_autoLockZone(pathDesc.oid, __FUNCTION__);
      RETURN_IF_NOT_SUCCESS(ret);

      ret = odl_deleteObjectInstanceByFullPathFlags(fullpath, flags);

      lck_autoUnlockZone(pathDesc.oid, __FUNCTION__);
   }

   return ret;
}


CmsRet cmsPhl_clearStatisticsByFullPathFlags(const char *fullpath, UINT32 flags)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   CmsRet ret;

   if (fullpath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_notice("Entered: fullpath=%s flags=0x%x", fullpath, flags);

   // It is ok if we cannot convert fullpath, it is probably a 
   // remote fullpath, which will be handled in odl_clearStatistics.
   ret = cmsMdm_fullPathToPathDescriptor(fullpath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("Failed to convert %s, probably remote", fullpath);
      // Normally, we don't lock for remote operations
      ret = odl_clearStatisticsByFullPathFlags(fullpath, flags);
   }
   else
   {
      // This could still be a remote fullpath, or local.
      ret = lck_autoLockZone(pathDesc.oid, __FUNCTION__);
      RETURN_IF_NOT_SUCCESS(ret);

      ret = odl_clearStatisticsByFullPathFlags(fullpath, flags);

      lck_autoUnlockZone(pathDesc.oid, __FUNCTION__);
   }

   return ret;
}


void cmsPhl_freeGetParamValueVbuf(CmsVbuf *vbuf)
{
   PhlGetParamValue_t paramValue;

   if (NULL == vbuf)
   {
      return;
   }
   else
   {
      cmsVbuf_resetIndex(vbuf);
      while (cmsVbuf_get(vbuf, &paramValue, sizeof(paramValue)) == CMSRET_SUCCESS)
      {
         cmsMem_free(paramValue.pValue);
      }
      cmsVbuf_destroy(vbuf);
   }
   return;
}

void cmsPhl_freeGetParamValueBuf(PhlGetParamValue_t *pBuf,
                                 SINT32             numEntries)
{
   SINT32               i;
   PhlGetParamValue_t   *ptr = pBuf;

   if (NULL == pBuf)
   {
      return;
   }
   else
   {
      for (i = 0; i < numEntries; i++, ptr++)
      {
         if (ptr->pValue)
            CMSMEM_FREE_BUF_AND_NULL_PTR(ptr->pValue);
      }
      cmsMem_free(pBuf);
   }

}  /* End of cmsPhl_freeGetParamValueBuf() */


UINT32 local_getNumberOfParamValueChanges(void)
{
   UINT32 n = 0;
   CmsRet ret;

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      n = mdm_getNumberOfParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }
   return n;
}

UINT32 local_getAltNumberOfParamValueChanges(void)
{
   UINT32 n = 0;
   CmsRet ret;

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      n = mdm_getNumberOfAltParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }
   return n;
}

UINT32 cmsPhl_getNumberOfParamValueChanges(void)
{
   UINT32 remoteChanges = 0;
   UINT32 n = 0;

   CHECK_MDM_EXTERNAL_CALLER_0(__FUNCTION__);

   n = local_getNumberOfParamValueChanges();

   if (mdmShmCtx->isRemoteCapable == TRUE)
   {
       remoteChanges = remote_getNumberOfParamValueChanges();
   }

   return (n+remoteChanges);

}  /* End of cmsPhl_getNumberOfParamValueChanges() */

UBOOL8 cmsPhl_isParamValueChanged(const MdmPathDescriptor *pathDesc)
{
   UBOOL8 ret = FALSE;

   if (pathDesc == NULL)
   {
      cmsLog_error("NULL pathDesc");
      return FALSE;
   }

   CHECK_MDM_EXTERNAL_CALLER_0(__FUNCTION__);

   if (lck_autoLockZone(pathDesc->oid, __FUNCTION__) == CMSRET_SUCCESS)
   {
      if (NULL != mdm_getObjectNode(pathDesc->oid))
      {
         ret = mdm_isParamValueChanged(pathDesc);
      }
      else
      {
         cmsLog_error("couldn't find object node for oid %d", pathDesc->oid);
      }
      lck_autoUnlockZone(pathDesc->oid, __FUNCTION__);
   }

   return ret;

}  /* End of cmsPhl_isParamValueChanged() */

void local_clearAllParamValueChanges(void)
{
   CmsRet ret;

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      mdm_clearAllParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }
}

void local_clearAllAltParamValueChanges(void)
{
   CmsRet ret;

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      mdm_clearAllAltParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }
}

/* No need to provide version of altchange because this method is called
* by TR69c
*/
void cmsPhl_clearAllParamValueChanges(void)
{
   CHECK_MDM_EXTERNAL_CALLER_V(__FUNCTION__);

   local_clearAllParamValueChanges();
   if (mdmShmCtx->isRemoteCapable == TRUE)
   {
       remote_clearAllParamValueChanges();
   }
}  /* End of cmsPhl_clearAllParamValueChanges() */


/* get the changed value name, type, value string */
UBOOL8 cmsPhl_getChangedParam(const MdmPathDescriptor *pathDesc, char **valueStr)
{
   MdmNodeAttributes nodeAttr;
   char *value;
   const char *type;
   char *fullpath;
   CmsRet rc = CMSRET_SUCCESS;
   UINT32 neededLen;
   char *valueTypeStr;

   if (pathDesc == NULL)
   {
      cmsLog_error("NULL pathDesc");
      return FALSE;
   }

   nodeAttr.notification = 0;
   mdm_getParamAttributes(pathDesc, &nodeAttr);

   if (nodeAttr.notification == 0)
   {
      /* not active nor passive notification for this parameter */
      return FALSE;
   }
   if (nodeAttr.valueChanged == 0)
   {
      /* value not changed */
      return FALSE;
   }

   // Get the value currently in the MDM. Do not call STL handler because we
   // already locked the entire MDM and got a count of params changed, so we
   // do not want any values to change now.
   rc = odl_getFlags(pathDesc, OGF_NO_VALUE_UPDATE, &value);
   if (rc == CMSRET_SUCCESS)
   {
      UBOOL8 isPassword=FALSE;

      if ((rc = cmsMdm_pathDescriptorToFullPath(pathDesc, &fullpath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. rc=%d", rc);
      }

      type = mdm_getParamBaseType(pathDesc);
      if(value != NULL)
         neededLen = strlen(fullpath) + strlen(type) + strlen(value) + 3;
      else
         neededLen = strlen(fullpath) + strlen(type) + 3;

      valueTypeStr = cmsMem_alloc(neededLen,ALLOC_ZEROIZE);
      mdm_getParamIsTr69Password(pathDesc,&isPassword);
      if (isPassword)
      {
         sprintf(valueTypeStr,"%s,%s,%s",fullpath,type,"");
      }
      else
      {
         sprintf(valueTypeStr,"%s,%s,%s",fullpath,type,((value != NULL) ? value : ""));
      }
      if (NULL != value)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(value);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
      *valueStr = valueTypeStr;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

/* get the changed value name, type, value string */
UBOOL8 cmsPhl_getAltChangedParam(const MdmPathDescriptor *pathDesc, char **valueStr)
{
   MdmNodeAttributes nodeAttr;
   char *value;
   const char *type;
   char *fullpath;
   CmsRet rc = CMSRET_SUCCESS;
   UINT32 neededLen;
   char *valueTypeStr;

   if (pathDesc == NULL)
   {
      cmsLog_error("NULL pathDesc");
      return FALSE;
   }

   nodeAttr.notification = 0;
   mdm_getParamAttributes(pathDesc, &nodeAttr);

   if (nodeAttr.altNotif== 0)
   {
      /* not altchange notification for this parameter */
      return FALSE;
   }
   if (nodeAttr.altNotifValue== 0)
   {
      /* value not changed */
      return FALSE;
   }

   // Get the value currently in the MDM. Do not call STL handler because we
   // already locked the entire MDM and got a count of params changed, so we
   // do not want any values to change now.
   rc = odl_getFlags(pathDesc, OGF_NO_VALUE_UPDATE, &value);
   if (rc == CMSRET_SUCCESS)
   {
      if ((rc = cmsMdm_pathDescriptorToFullPath(pathDesc, &fullpath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. rc=%d", rc);
      }

      type = mdm_getParamBaseType(pathDesc);
      if(value != NULL)
         neededLen = strlen(fullpath) + strlen(type) + strlen(value) + 3;
      else
         neededLen = strlen(fullpath) + strlen(type) + 3;

      valueTypeStr = cmsMem_alloc(neededLen,ALLOC_ZEROIZE);
      sprintf(valueTypeStr,"%s,%s,%s",fullpath,type,((value != NULL) ? value : ""));
      if(NULL != value)
      {
          CMSMEM_FREE_BUF_AND_NULL_PTR(value);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath);
      *valueStr = valueTypeStr;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/* traverse the local tree, and return an array of string of changed param */
CmsRet cmsPhl_getChangedParamsLocal(char ***arrayParams, UINT32 *numParams)
{
   CmsRet rc = CMSRET_SUCCESS;
   char **array;
   UINT32 numChanged=0;
   UINT32 numScannedChanged=0;
   MdmPathDescriptor rootPath=EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor nextPath=EMPTY_PATH_DESCRIPTOR;
   MdmObjectNode *rootNode;

   rootNode = mdm_getRootObjectNode();
   rc = cmsLck_acquireAllZoneLocksWithBackoff(rootNode->oid, PHL_LOCK_TIMEOUT);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get all locks! rc=%d", rc);
      return CMSRET_INTERNAL_ERROR;
   }

   // Get a simple count of changed params from MDM.
   numChanged= mdm_getNumberOfParamValueChanges();
   if (numChanged == 0)
   {
      cmsLck_releaseAllZoneLocks();
      return CMSRET_SUCCESS;
   }

   array = (char **)cmsMem_alloc(numChanged*sizeof(char*), ALLOC_ZEROIZE);
   if (array == NULL)
   {
      cmsLck_releaseAllZoneLocks();
      cmsLog_error("Unable to allocate array of %d", numChanged);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* traverse MDM and fill in the array of changed params */
   rc = cmsMdm_fullPathToPathDescriptor(rootNode->genericFullpath, &rootPath);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", rootNode->genericFullpath, rc);
   }

   while (rc == CMSRET_SUCCESS)
   {
      char *valueStr = NULL;

      rc = phl_getNextPath(TRUE, FALSE, &rootPath, &nextPath, 0);
      if (rc == CMSRET_SUCCESS)
      {
         if (cmsPhl_getChangedParam(&nextPath,&valueStr))
         {
            if (numScannedChanged < numChanged)
            {
               // caller is now responsible for free
               array[numScannedChanged] = valueStr;
               valueStr = NULL;
            }
            else
            {
               // We found more changed params in MDM than initially
               // reported.  This should not happen since we have the MDM lock.
               cmsLog_error("more value changed (%d) than initially reported (%d) valueStr=%s",
                            numScannedChanged, numChanged, valueStr);
               CMSMEM_FREE_BUF_AND_NULL_PTR(valueStr);
            }
            numScannedChanged++;
         }
      }
   }

   cmsLck_releaseAllZoneLocks();

   if (numScannedChanged != numChanged)
   {
      cmsLog_error("Scanned change %d not equal to numChanged %d",
                   numScannedChanged, numChanged);
      // But return what we got anyways.
   }

   // catch weird case
   if (numScannedChanged == 0)
   {
      cmsMem_free(array);
      rc = CMSRET_SUCCESS;
   }
   else
   {
      *arrayParams = array;
      // Return actual number of params in the array
      *numParams = (numScannedChanged < numChanged) ? numScannedChanged : numChanged;
      rc = CMSRET_SUCCESS;
   }

   return (rc);
}

/* traverse the local tree, and return an array of string of altchanged param */
// XXXTODO: unify with cmsPhl_getChangedParamsLocal.
CmsRet cmsPhl_getAltChangedParamsLocal(char ***arrayParams, UINT32 *numParams)
{
   CmsRet rc = CMSRET_SUCCESS;
   char **array;
   UINT32 numChanged=0;
   UINT32 numScannedChanged=0;
   MdmPathDescriptor rootPath=EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor nextPath=EMPTY_PATH_DESCRIPTOR;
   MdmObjectNode *rootNode;

   *numParams = 0;

   rootNode = mdm_getRootObjectNode();
   rc = cmsLck_acquireAllZoneLocksWithBackoff(rootNode->oid, PHL_LOCK_TIMEOUT);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get all locks! rc=%d", rc);
      return CMSRET_INTERNAL_ERROR;
   }

   // Get a simple count of changed params from MDM.
   numChanged= mdm_getNumberOfAltParamValueChanges();
   if (numChanged == 0)
   {
      cmsLck_releaseAllZoneLocks();
      return CMSRET_SUCCESS;
   }

   array = (char **)cmsMem_alloc(numChanged*sizeof(char*), ALLOC_ZEROIZE);
   if (array == NULL)
   {
      cmsLck_releaseAllZoneLocks();
      cmsLog_error("Unable to allocate array of %d", numChanged);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* traverse tree and get an array of all changed param */
   rc = cmsMdm_fullPathToPathDescriptor(rootNode->genericFullpath, &rootPath);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", rootNode->genericFullpath, rc);
   }

   while (rc == CMSRET_SUCCESS)
   {
      char *valueStr = NULL;

      rc = phl_getNextPath(TRUE, FALSE, &rootPath, &nextPath, 0);
      if (rc == CMSRET_SUCCESS)
      {
         if (cmsPhl_getAltChangedParam(&nextPath,&valueStr))
         {
            if (numScannedChanged < numChanged)
            {
               // caller is now responsible for free
               array[numScannedChanged] = valueStr;
               valueStr = NULL;
            }
            else
            {
               // We found more changed params in MDM than initially
               // reported.  This should not happen since we have the MDM lock.
               cmsLog_error("more value changed (%d) than initially reported (%d) valueStr=%s",
                            numScannedChanged, numChanged, valueStr);
               CMSMEM_FREE_BUF_AND_NULL_PTR(valueStr);
            }
            numScannedChanged++;
         }
      }
   }

   cmsLck_releaseAllZoneLocks();

   if (numScannedChanged != numChanged)
   {
      cmsLog_error("Scanned change %d not equal to numChanged %d",
                   numScannedChanged, numChanged);
      // But return what we got anyways.
   }

   // catch weird case
   if (numScannedChanged == 0)
   {
      cmsMem_free(array);
      rc = CMSRET_SUCCESS;
   }
   else
   {
      *arrayParams = array;
      // Return actual number of params in the array
      *numParams = (numScannedChanged < numChanged) ? numScannedChanged : numChanged;
      rc = CMSRET_SUCCESS;
   }

   return (rc);
}

/**
* Calling cmsPhl_getAltChangedParamsLocal and then clearAllParamValueChanges is non-atomic
*/
CmsRet cmsPhl_getAndClearAltChangedParamsLocal(char ***arrayParams, UINT32 *numParams)
{
   CmsRet rc = CMSRET_SUCCESS;
   MdmObjectNode *rootNode;

   rootNode = mdm_getRootObjectNode();
   rc = cmsLck_acquireAllZoneLocksWithBackoff(rootNode->oid, PHL_LOCK_TIMEOUT);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get all locks! rc=%d", rc);
      return CMSRET_INTERNAL_ERROR;
   }
   rc = cmsPhl_getAltChangedParamsLocal(arrayParams, numParams);
   mdm_clearAllAltParamValueChanges();
   cmsLck_releaseAllZoneLocks();
   
   return (rc);
}

/* caller gets an array of strings of parameter changed.  Caller to free the array */
CmsRet cmsPhl_getChangedParams(char ***array_of_changed_params, UINT32 *numParams)
{
   CmsRet r2;  // used locally only
   UINT32 localChangedCnt=0;
   UINT32 remoteChangedCnt=0;
   UINT32 i;
   char **arrayLocal=NULL;
   char **arrayRemote=NULL;


   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((array_of_changed_params == NULL) || (numParams == NULL))
   {
      cmsLog_error("NULL input args %p/%p", array_of_changed_params, numParams);
      return CMSRET_INVALID_ARGUMENTS;
   }
   *array_of_changed_params = NULL;
   *numParams = 0;

   /* get array of changed params in the local MDM. */
   r2 = cmsPhl_getChangedParamsLocal(&arrayLocal, &localChangedCnt);
   if (r2 != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get local changed params, r2=%d", r2);
   }

   /* call remote_objd to get array of changed params in all remote MDs */
   if (mdmShmCtx->isRemoteCapable == TRUE)
   {
       r2 = remote_getChangedParams(&arrayRemote, &remoteChangedCnt);
       if (r2 != CMSRET_SUCCESS)
       {
          cmsLog_error("Cannot get remote changed params, r2=%d", r2);
       }
   }

   if ((localChangedCnt !=0) && (remoteChangedCnt !=0))
   {
      UINT32 totalCnt = localChangedCnt + remoteChangedCnt;

      /* enlarged the local array, and copy the remote paramsChanged over */
      arrayLocal = (char **)cmsMem_realloc(arrayLocal,totalCnt*sizeof(char*));
      if (arrayLocal != NULL)
      {
         for (i= 0; i <remoteChangedCnt; i++)
         {
            arrayLocal[localChangedCnt+i] = arrayRemote[i];
         }
         CMSMEM_FREE_BUF_AND_NULL_PTR(arrayRemote);
         *numParams= totalCnt;
         *array_of_changed_params = arrayLocal;
      }
      else
      {
         cmsLog_error("could not allocate merged array, totalCnt=%d", totalCnt);
         /* free everything and return error */
         cmsUtl_freeArrayOfStrings(&arrayLocal, localChangedCnt);
         cmsUtl_freeArrayOfStrings(&arrayRemote, remoteChangedCnt);
         return CMSRET_RESOURCE_EXCEEDED;
      }
   }
   else if (localChangedCnt)
   {
      *array_of_changed_params = arrayLocal;
      *numParams = localChangedCnt;
   }
   else if (remoteChangedCnt)
   {
      *array_of_changed_params = arrayRemote;
      *numParams = remoteChangedCnt;
   }

   cmsLog_debug("Exit: array_of_changed_params=%p numParams=%d",
                *array_of_changed_params, *numParams);
   return CMSRET_SUCCESS;  // always return success
}  /* End of cmsPhl_getChangedParams */

