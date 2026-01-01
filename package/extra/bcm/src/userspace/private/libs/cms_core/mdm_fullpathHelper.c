/***********************************************************************
 *
 *  Copyright (c) 2022  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2022:proprietary:standard

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

#include <string.h>
#include <ctype.h>

#include "cms.h"
#include "cms_util.h"
#include "sysutil.h"
#include "mdm.h"
#include "mdm_private.h"

/*
 * This file contains fullpath and Path Descriptor helper functions for
 * the MDM.  Originally, these functions were in the binary only mdm.c, but
 * moved here so customers can inspect the code if necessary.
 */

const char *mdm_oidToGenericPath(MdmObjectId oid)
{
   MdmObjectNode *node;

   // First look in the local table.
   node = mdm_getLocalObjNode(oid);
   if (node != NULL)
   {
      return node->genericFullpath;
   }

   // Also look in the remoteOidTable if there is one.  Caller should not
   // need to know if this is local or remote, since the fullpath is an
   // abstract concept.  Does not imply whether it is present on the system.
   if (mdmShmCtx->remoteOidNodeTableBegin != NULL)
   {
      node = mdm_getRemoteObjNode(oid);
      if (node != NULL)
      {
         return node->genericFullpath;
      }
   }

   cmsLog_error("Could not find genericPath for oid %d", oid);
   return NULL;
}


static UBOOL8 mdm_compGenericPath(const char *dmGenericPath, const char *genericPath)
{
   UINT32 fullLen, genericLen;

   fullLen = strlen(dmGenericPath);
   genericLen = strlen(genericPath);
   if (!strcmp(dmGenericPath, genericPath))
   {
       return TRUE;
   }
   else if ((fullLen - 4 == genericLen) &&
            (!strncmp(dmGenericPath, genericPath, fullLen - 4)) &&
            (!strcmp(&(dmGenericPath[fullLen - 4]), "{i}.")))
   {
       /*
        * This is for the case where the ACS sends us a getNextParamName
        * with path of WanDevice.1.WanConnectionDevice.
        * and expects to see all the instances of WanConnectionDevice.1, .2, etc.
        * There are tests in suite_phl and suite_mdm that test for this.
        */
       return TRUE;
   }
   return FALSE;
}


CmsRet cmsMdm_genericPathToLocalOid(const char *genericPath, MdmObjectId *oid)
{
   const MdmOidInfoEntry *begin=NULL;
   const MdmOidInfoEntry *end=NULL;

   mdm_getOidInfoPtrs(&begin, &end);
   if (begin != NULL && end != NULL)
   {
       while (begin <= end)
       {
          if (mdm_compGenericPath(begin->fullPath, genericPath) == TRUE)
          {
             *oid = begin->oid;
             return CMSRET_SUCCESS;
          }
          begin++;
       }
   }

   *oid = mdm_getMaxOid() + 1; /* set oid to an invalid oid */
   return CMSRET_INVALID_PARAM_NAME;
}

CmsRet cmsMdm_genericPathToRemoteOid(const char *genericPath, MdmObjectId *oid)
{
   OidNodeEntry *oidNode = mdmShmCtx->remoteOidNodeTableBegin;

   if (oidNode != NULL)
   {
       while ((oidNode->oid != 0) &&
              (oidNode < mdmShmCtx->remoteOidNodeTableEnd))
       {
           if (oidNode->objNode != NULL)
           {
               if (mdm_compGenericPath(oidNode->objNode->genericFullpath, genericPath) == TRUE)
               {
                  *oid = oidNode->oid;
                  return CMSRET_SUCCESS;
               }
           }
           else
           {
              cmsLog_error("remoteOidNode[%d]->objNode is NULL!", oidNode->oid);
           }
           oidNode++;
       }
   }

   *oid = mdm_getMaxOid() + 1; /* set oid to an invalid oid */
   return CMSRET_INVALID_PARAM_NAME;
}

CmsRet cmsMdm_genericPathToOid(const char *genericPath, MdmObjectId *oid)
{
   CmsRet ret;

   ret = cmsMdm_genericPathToLocalOid(genericPath, oid);
   if (ret != CMSRET_SUCCESS)
   {
      ret = cmsMdm_genericPathToRemoteOid(genericPath, oid);
   }

   return ret;
}


CmsRet cmsMdm_aliasedFullPathToPathDescriptor(const char *fullpath,
                                              MdmPathDescriptor *pathDesc,
                                              char *alias)
{
   char *fullpath2=NULL;
   UINT32 idx=0;
   CmsRet ret;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   idx = cmsUtl_strlen(fullpath);
   if (idx == 0)
   {
      cmsLog_error("NULL or zero length fullpath");
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Look for a . or ] at the end to see if this is an aliased fullpath
   idx--;
   if (fullpath[idx] == '.')
   {
      idx--;
      if (fullpath[idx] != ']')
      {
         // This is not an aliased FullPath, do normal conversion.
         return (cmsMdm_fullPathToPathDescriptor(fullpath, pathDesc));
      }
   }
   else
   {
      // fullpath does not end in . so check if it ends in a bracketed alias.
      // Not sure if spec actually allows this,  but given confusion of when
      // fullpath can or cannot end in a dot, handle this case.
      if (fullpath[idx] != ']')
      {
         // This is not an aliased FullPath, do normal conversion.
         return (cmsMdm_fullPathToPathDescriptor(fullpath, pathDesc));
      }
   }

   idx--;  // go back past the end ]
   /*
    * Everything from here is dealing with aliased fullpath.
    */
   // Find the beginning of the alias
   while (idx > 0 && fullpath[idx] != '[')
   {
      idx--;
   }
   if (idx == 0)
   {
      cmsLog_error("Malformed fullpath %s", fullpath);
      return CMSRET_INVALID_ARGUMENTS;
   }
   fullpath2 = cmsMem_alloc(idx+1, ALLOC_ZEROIZE);
   if (fullpath2 == NULL)
   {
      cmsLog_error("failed to alloc %d bytes for %s", idx, fullpath);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   // Copy the fullpath without alias to fullpath2
   memcpy(fullpath2, fullpath, idx);

   // TODO: should copy out the alias first, regardless of whether we
   // recognize the fullpath.  Could be a remote fullpath.  Or make this
   // function just grab the alias without trying to process the fullpath.
   ret = cmsMdm_fullPathToPathDescriptor(fullpath2, pathDesc);
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullpath2);

   if (ret == CMSRET_SUCCESS)
   {
      // Conversion func was successful, copy out the alias
      if (alias != NULL)
      {
         UINT32 i=0;
         idx++;  // advance past the [
         while (fullpath[idx] != ']')
         {
            alias[i] = fullpath[idx];
            i++; idx++;
         }
      }
   }
   return ret;
}

static void mdm_lookupPathDescEx(MdmPathDescriptorEx *pathDescEx)
{
   UINT32 k=0;
   char *lastDotPtr, *savedPtr=NULL;
   char savedChar = '\0';

   if (pathDescEx == NULL)
   {
      cmsLog_error("NULL pathDescEx");
      return;
   }
   else if (IS_EMPTY_STRING(pathDescEx->genericFullpath))
   {
      cmsLog_error("NULL or empty genericFullpath");
      return;
   }

   cmsLog_debug("Entered: genericFullpath=%s", pathDescEx->genericFullpath);

   // If genericFullpath does not end in dot, assume the final part is
   // a parameter name.
   lastDotPtr = strrchr(pathDescEx->genericFullpath, '.');
   if (lastDotPtr != NULL)
   {
      lastDotPtr++;  // advance past the dot
      if (strlen(lastDotPtr) > 1)
      {
         pathDescEx->paramNamePresent = TRUE;
         savedPtr = lastDotPtr;
         while (*lastDotPtr != '\0')
         {
            pathDescEx->paramName[k++] = *lastDotPtr;
            lastDotPtr++;
         }

         // Terminate the genericFullpath so it only contains the object path
         // with end dot.
         savedChar = *savedPtr;
         *savedPtr = '\0';
         cmsLog_debug("possible paramName=%s (genericPath is now =%s)",
                      pathDescEx->paramName, pathDescEx->genericFullpath);
      }
   }

   // Look for obj in MDM based on current genericFullpath
   {
      MdmObjectId oid = 0;
      pathDescEx->oid = mdm_getMaxOid() + 1; // first set to invalid oid
      pathDescEx->objInLocalMdm = (cmsMdm_genericPathToLocalOid(pathDescEx->genericFullpath, &oid) == CMSRET_SUCCESS);
      if (pathDescEx->objInLocalMdm)
      {
         pathDescEx->oid = oid;
      }
      pathDescEx->objInRemoteMdm = (cmsMdm_genericPathToRemoteOid(pathDescEx->genericFullpath, &oid) == CMSRET_SUCCESS);
      if (pathDescEx->objInRemoteMdm)
      {
         pathDescEx->oid = oid;
      }
      if (pathDescEx->objInLocalMdm || pathDescEx->objInRemoteMdm)
      {
         MdmObjectNode *objNode;
         objNode = mdm_getObjectNodeFlags(pathDescEx->oid, GET_OBJNODE_ANY, NULL);

         if ( !objNode )
         {
            cmsLog_error("NULL objNode");
            return;
         }

         pathDescEx->multiCompObj = ((objNode->flags & OBN_MULTI_COMP_OBJ) != 0);
         pathDescEx->instanceDepth = objNode->instanceDepth;

         // verify paramName.
         if (pathDescEx->paramNamePresent)
         {
            UINT16 p;
            UBOOL8 found = FALSE;
            for (p = 0; (p < objNode->numParamNodes) && (!found); p++)
            {
               if (!strcmp(objNode->params[p].name, pathDescEx->paramName))
               {
                  found = TRUE;
                  pathDescEx->paramNameValid = TRUE;
               }
            }

            // If paramName not found, then maybe the paramName was actually
            // an object name without end dot, e.g. Device.DSL.Line.1.Stats
            if (!found)
            {
               cmsLog_debug("paramName %s not found, is it object? check %d childnodes",
                            pathDescEx->paramName, objNode->numChildObjNodes);
               for (p = 0; (p < objNode->numChildObjNodes) && (!found); p++)
               {
                  cmsLog_debug("child[%d]=%s", p, objNode->childObjNodes[p].name);
                  if (!strcmp(objNode->childObjNodes[p].name, pathDescEx->paramName))
                  {
                     found = TRUE;
                     pathDescEx->oid = objNode->childObjNodes[p].oid;
                     *savedPtr = savedChar;  // undo the null term we put in front of the "paramName", which is actually an obj name.
                     pathDescEx->genericFullpath[strlen(pathDescEx->genericFullpath)] = '.';  // add an end dot after obj name to make it clear.
                     cmsLog_debug("paramName=%s is actually an obj, new oid=%d restored/normalized genericFullpath=%s",
                                  pathDescEx->paramName, pathDescEx->oid, pathDescEx->genericFullpath);
                     pathDescEx->paramNamePresent = FALSE;
                     memset(&(pathDescEx->paramName), 0, sizeof(pathDescEx->paramName));
                  }
               }
            }
         }
      }
   }

   if (pathDescEx->objInLocalMdm || pathDescEx->objInRemoteMdm)
   {
      cmsLog_debug("(oid=%d) %s paramName=%s (valid=%d) inLocalMdm=%d inRemoteMdm=%d multiComp=%d instanceDepth=%d",
                    pathDescEx->oid, pathDescEx->genericFullpath,
                    pathDescEx->paramName, pathDescEx->paramNameValid,
                    pathDescEx->objInLocalMdm, pathDescEx->objInRemoteMdm,
                    pathDescEx->multiCompObj, pathDescEx->instanceDepth);
   }
   else
   {
      cmsLog_debug("%s paramName=%s not found in local or remote MDM",
                   pathDescEx->genericFullpath, pathDescEx->paramName);
      // if we think we found a param name, restore the genericFullpath to
      // the original form, since we don't know if the param name is really
      // a param.
      if (pathDescEx->paramNamePresent)
      {
         pathDescEx->paramNamePresent = FALSE;
         memset(&(pathDescEx->paramName), 0, sizeof(pathDescEx->paramName));
         *savedPtr = savedChar;  // undo the null term we put in front of the "paramName"
         cmsLog_debug("revert genericFullpath to %s", pathDescEx->genericFullpath);
      }
   }

   return;
}


CmsRet cmsMdm_fullPathToPathDescriptorEx(const char *fullpath, MdmPathDescriptorEx **pathDescEx)
{
   MdmPathDescriptorEx *tmpEx = NULL;
   UINT32 fullpathLen, genericPathLen, i=0, j=0;
   UBOOL8 prevCharWasDot, isInstance;
   CmsRet ret = CMSRET_SUCCESS;

   if ((fullpath == NULL) || (pathDescEx == NULL))
   {
      cmsLog_error("NULL input args %p/%p", fullpath, pathDescEx);
      return CMSRET_INVALID_ARGUMENTS;
   }
   else if (fullpath[0] == '\0')
   {
      cmsLog_error("fullpath is empty string!");
      return FALSE;
   }

   cmsLog_debug("Entered: fullpath=%s", fullpath);

   // we can allocate this in the heap since this will not go into the MDM.
   tmpEx = cmsMem_alloc(sizeof(MdmPathDescriptorEx), ALLOC_ZEROIZE);
   if (tmpEx == NULL)
   {
      cmsLog_error("cmsMem_alloc of MdmPathDescriptorEx failed");
      return CMSRET_RESOURCE_EXCEEDED;
   }


   /*
    * worst case is in the full path, instance numbers have only a single digit,
    * i.e. .1. while the generic path needs a .{i}. for each instance number, so
    * genericPathLen is (MAX_MDM_INSTANCE_DEPTH * 2) bytes longer than fullpathLen
    * plus 1 for the NULL plus 16 for some extra safety (we might add an extra
    * final end dot).
    */
   fullpathLen = cmsUtl_strlen(fullpath);
   genericPathLen = fullpathLen + (MAX_MDM_INSTANCE_DEPTH * 2) + 1 + 16;
   if ((tmpEx->genericFullpath = cmsMem_alloc(genericPathLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("alloc of %d bytes failed", genericPathLen);
      cmsMem_free(tmpEx);
      return CMSRET_RESOURCE_EXCEEDED;
   }


   // Move through the fullpath from begining to end, translating to genericPath.
   // Each instance id position can be a number, alias, or wildcard (asterisK).
   // Wildcards are not supported yet.
   // The last element is ambiguous, could be an object or param name.
   while (i < fullpathLen)
   {
      prevCharWasDot = FALSE;
      isInstance = FALSE;

      if (fullpath[i] == '.')
      {
         tmpEx->genericFullpath[j++] = fullpath[i++]; // copy the dot and advance
         if (fullpath[i] != '\0')
         {
            // dot in the middle of fullpath, could be followed by instance specifier
            prevCharWasDot = TRUE;
         }
      }
      else
      {
         // normal character, just copy and advance
         tmpEx->genericFullpath[j++] = fullpath[i++];
      }

      // Look for possible instance specifier after a dot
      if (prevCharWasDot)
      {
         if (fullpath[i] == '[')
         {
            UINT32 k=0;
            isInstance = TRUE;
            // This is an alias, copy and advance i to end dot or null term
            i++;  // advance past [
            tmpEx->aliasPresent = TRUE;
            while (fullpath[i] != ']')
            {
               if (k >= MDM_ALIAS_BUFLEN - 1)
               {
                  cmsLog_error("alias (%s) too long", tmpEx->aliasStack.aliasArray[tmpEx->aliasStack.currentDepth]);
                  goto pathdescex_exit;
               }
               tmpEx->aliasStack.aliasArray[tmpEx->aliasStack.currentDepth][k++] = fullpath[i++];
            }
            i++;  // advance past ]
         }
         else if (isdigit(fullpath[i]))
         {
            char digitsBuf[11]={0}; // UINT32 has 10 digits max plus null 
            UINT32 k=0, instanceId;
            // This is a normal instance id, copy and advance
            isInstance = TRUE;
            while ((i < fullpathLen) && (isdigit(fullpath[i])))
            {
               if (k >= 10)
               {
                  cmsLog_error("instance number (%s) too long!", digitsBuf);
                  goto pathdescex_exit;
               }
               digitsBuf[k++] = fullpath[i++];
            }

            if (cmsUtl_strtoul(digitsBuf, NULL, 0, &instanceId) != CMSRET_SUCCESS)
            {
               cmsLog_error("conversion of %s to instanceId failed", digitsBuf);
               goto pathdescex_exit;
            }
            tmpEx->iidStack.instance[tmpEx->iidStack.currentDepth] = instanceId;
         }
         else if (fullpath[i] == '*')
         {
            cmsLog_error("unsupported wildcard detected in fullpath %s", fullpath);
            isInstance = TRUE;

            // we do not expand wildcard to matching pathDescriptors, but at
            // least we can record its presence.
            tmpEx->wildcardPresent = TRUE;
            tmpEx->wildcardStack.wildcardArray[tmpEx->wildcardStack.currentDepth] = TRUE;

            i++;  // advance past *
         }
         else
         {
            // after the dot, just another obj or param name, so just copy and advance
            tmpEx->genericFullpath[j++] = fullpath[i++];
         }

         if (isInstance)
         {
            // advance all 3 instance stacks even though only 1 of them will
            // have valid data at this level.
            tmpEx->iidStack.currentDepth++;
            tmpEx->aliasStack.currentDepth++;
            tmpEx->wildcardStack.currentDepth++;

            // in all 3 of the above cases, we insert {i}. in the generic path.
            tmpEx->genericFullpath[j++] = '{';
            tmpEx->genericFullpath[j++] = 'i';
            tmpEx->genericFullpath[j++] = '}';

            // fullpath[i] must point to end dot or end of string
            if ((fullpath[i] != '.') && (fullpath[i] != '\0'))
            {
               cmsLog_error("malformed fullpath %s (instance must end with dot or null)", fullpath);
               goto pathdescex_exit;
            }
            else
            {
               // we always want to have an end dot in the generic string
               tmpEx->genericFullpath[j++] = '.';
               i++;
            }
         }  // end of isInstance
      }
   }

   // Dump out the results of the parsing.
   {
      cmsLog_debug("genericFullpath=%s (iidStack depth=%d aliasPresent=%d wildcardPresent=%d)",
                   tmpEx->genericFullpath,
                   tmpEx->iidStack.currentDepth,
                   tmpEx->aliasPresent, tmpEx->wildcardPresent);
      // instanceDepth is not filled out yet, so use the currentDepth of the
      // iidStack to see if any instances were detected.
      if (tmpEx->iidStack.currentDepth > 0)
      {
         UINT8 d;
         cmsLog_debug("iidStack=%s", cmsMdm_dumpIidStack(&(tmpEx->iidStack)));
         for (d=0; d < tmpEx->aliasStack.currentDepth; d++)
         {
            if (tmpEx->aliasStack.aliasArray[d][0] != '\0')
            {
               cmsLog_debug("[alias depth=%d]=%s", d, tmpEx->aliasStack.aliasArray[d]);
            }
         }

         for (d=0; d < tmpEx->wildcardStack.currentDepth; d++)
         {
            if (tmpEx->wildcardStack.wildcardArray[d])
            {
               cmsLog_debug("[wildcard depth=%d]=*", d);
            }
         }
      }
   }

   /*
    * Now that we have a generic fullpath, we can look for
    * the object in the local and remote MDM's.
    */
   mdm_lookupPathDescEx(tmpEx);

pathdescex_exit:
   if (ret == CMSRET_SUCCESS)
   {
      *pathDescEx = tmpEx;  // caller is responsible for calling cmsMdm_freeMdmPathDescriptorEx() to free
   }
   else
   {
      cmsMdm_freeMdmPathDescriptorEx(&tmpEx);
   }

   return ret;
}


void cmsMdm_freeMdmPathDescriptorEx(MdmPathDescriptorEx **pathDescEx)
{
   if ((pathDescEx == NULL) || (*pathDescEx == NULL))
      return;

   CMSMEM_FREE_BUF_AND_NULL_PTR((*pathDescEx)->genericFullpath);

#ifdef future_work
   // MdmPathDescriptorEx may point to a vbuf which needs to be freed.
   if ((*pathDescEx)->additionalPathDescVbuf)
   {
      cmsVbuf_destroy((*pathDescEx)->additionalPathDescVbuf);
      (*pathDescEx)->additionalPathDescVbuf = NULL;
   }
#endif

   cmsMem_free(*pathDescEx);
   *pathDescEx = NULL;
   return;
}


CmsRet cmsMdm_fullPathToPathDescriptor(const char *fullpath, MdmPathDescriptor *pathDesc)
{
   char *genericStr;
   const char *paramName = NULL;
   UINT32 fullpathLen, genericPathLen, i=0, j=0;
   CmsRet ret;

   if (pathDesc == NULL)
   {
      cmsLog_error("pathDesc pointer is NULL.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   mdm_initPathDescriptor(pathDesc);

   fullpathLen = cmsUtl_strlen(fullpath);
   if (fullpathLen == 0)
   {
      cmsLog_error("NULL or zero length fullpath");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /*
    * worst case is in the full path, instance numbers have only a single digit,
    * i.e. .1. while the generic path needs a .{i}. for each instance number, so
    * genericPathLen is (MAX_MDM_INSTANCE_DEPTH * 2) bytes longer than fullpathLen
    * plus 1 for the NULL.
    */
   genericPathLen = fullpathLen + (MAX_MDM_INSTANCE_DEPTH * 2) + 1;

   if ((genericStr = cmsMem_alloc(genericPathLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("alloc of %d bytes failed", genericPathLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if (fullpath[fullpathLen - 1] != '.')
   {
      UBOOL8 allDigits=TRUE;
      UINT32 origFullpathLen = fullpathLen;

      /*
       * This fullpath can be either a parameter, e.g.
       * LanDevice.1.LanEthernetInterfaceConfig.Enabled
       * or a fullpath to an object instance generated by
       * cmsMdm_pathDescriptorToFullPathNoEndDot(), e.g.
       * LanDevice.1.LanEthernetInterfaceConfig.156
       */
      fullpathLen--;
      while ((fullpathLen != 0) && (fullpath[fullpathLen] != '.'))
      {
         if (!isdigit(fullpath[fullpathLen]))
         {
            allDigits = FALSE;
         }
         fullpathLen--;
      }

      if (fullpathLen == 0)
      {
         cmsLog_error("invalid fullpath %s", fullpath);
         cmsMem_free(genericStr);
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (allDigits)
      {
         /* the last component of the fullpath was all digits, so it must be
          * an instance number.  Pass the whole thing down to the next block
          * of code for conversion.
          */
         fullpathLen = origFullpathLen;
      }
      else
      {
         paramName = &(fullpath[fullpathLen+1]);
         fullpathLen++;  /* leave the . at the end */
      }
   }

   /* now do the object name to oid/iidStack translation. */
   while (i < fullpathLen)
   {
      if (!isdigit(fullpath[i]) || (isdigit(fullpath[i]) && fullpath[i-1] != '.'))
      {
         genericStr[j++] = fullpath[i++];
      }
      else
      {
         char digitsBuf[11]={0}; /* UINT32 number has max 10 digits plus null */
         UINT32 k=0, instanceId;

         while ((i < fullpathLen) && (isdigit(fullpath[i])))
         {
            if (k >= 10)
            {
               cmsLog_error("instance number (%s) too long!", digitsBuf);
               cmsMem_free(genericStr);
               return CMSRET_INVALID_ARGUMENTS;
            }
            digitsBuf[k++] = fullpath[i++];
         }

         if (cmsUtl_strtoul(digitsBuf, NULL, 0, &instanceId) != CMSRET_SUCCESS)
         {
            cmsLog_error("conversion of %s to instanceId failed", digitsBuf);
            cmsMem_free(genericStr);
            return CMSRET_INVALID_ARGUMENTS;
         }
         PUSH_INSTANCE_ID(&(pathDesc->iidStack), instanceId);

         genericStr[j++] = '{';
         genericStr[j++] = 'i';
         genericStr[j++] = '}';

         if ((i < fullpathLen) && fullpath[i] == '.')
         {
            genericStr[j++] = fullpath[i++]; /* for the . */
         }
         else if (i >= fullpathLen)
         {
            /* ended in an instance number with no dot, add it anyways */
            genericStr[j++] = '.';
         }
         else
         {
            cmsLog_error("bad pathname %s genericStr=%s", fullpath, genericStr);
            cmsMem_free(genericStr);
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
   }

   cmsAst_assert(j <= genericPathLen);

   /* get the oid based on genericStr */
   if ((ret = cmsMdm_genericPathToOid(genericStr, &(pathDesc->oid))) == CMSRET_SUCCESS)
   {
      if (paramName != NULL)
      {
         /*
          * Now find the internal param name string and pass it back to the caller.
          */
         MdmObjectNode *objNode=NULL;
         UINT16 p;
         UBOOL8 found = FALSE;

         if ((objNode = mdm_getObjectNodeFlags(pathDesc->oid, GET_OBJNODE_ANY, NULL)) != NULL)
         {
            for (p = 0; (p < objNode->numParamNodes) && (!found); p++)
            {
               if (!strcmp(objNode->params[p].name, paramName))
               {
                  strncpy(pathDesc->paramName, objNode->params[p].name, MAX_MDM_PARAM_NAME_LENGTH);
                  pathDesc->paramName[MAX_MDM_PARAM_NAME_LENGTH] = '\0';
                  found = TRUE;
               }
            }

            if (!found)
            {
               /*
                * We could have been given a fullpath like this:
                * Device.DSL.Line.1.Stats
                * (Stats is not a param, it is an object)
                */
               for (p = 0; (p < objNode->numChildObjNodes) && (!found); p++)
               {
                  cmsLog_debug("checking paramName %s as object name", paramName);
                  if (!strcmp(objNode->childObjNodes[p].name, paramName))
                  {
                     pathDesc->oid = objNode->childObjNodes[p].oid;
                     found = TRUE;
                  }
               }

               if (!found)
               {
                  cmsLog_error("could not find paramName %s in %s (%d params)",
                               paramName,
                               mdm_oidToGenericPath(objNode->oid),
                               objNode->numParamNodes);
                  ret = CMSRET_INVALID_PARAM_NAME;
               }
            }
         }
         else
         {
            /* even though the generic string to oid conversion worked, the
             * object may still not exist in the MDM due to data model profile
             * selection.
             */
            cmsLog_error("mdm_getObjectNode failed. oid=%d", pathDesc->oid);
            ret = CMSRET_INVALID_PARAM_NAME;
         }
      }
   }
   else
   {
      cmsLog_notice("mdm_genericPathToOid ret=%d. fullPath %s genericStr=%s oid=%d",
                     ret, fullpath, genericStr, pathDesc->oid);
   }

   /* don't need the genericStr anymore */
   cmsMem_free(genericStr);

   return ret;
}


CmsRet cmsMdm_pathDescriptorToFullPath(const MdmPathDescriptor *pathDesc, char **fullpath)
{
   const char *genericStr;
   char *fullpathStr;
   UINT32 i=0, j=0, genericLen, fullpathLen, savedFullpathLen, depth=0;
   UBOOL8 isGenericObject=FALSE;


   if ((genericStr = mdm_oidToGenericPath(pathDesc->oid)) == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   genericLen = strlen(genericStr);


   /*
    * Each instance id can have a maximum of 10 digits.  The generic string
    * has a "{i}" where the instance id would go, so we have to allocate 7 more
    * bytes per instance number for the fullpathStr.
    */
   fullpathLen =  genericLen + (7 * MAX_MDM_INSTANCE_DEPTH) + 1 /* for the null */;
   if (IS_PARAM_NAME_PRESENT(pathDesc))
   {
      fullpathLen += strlen(pathDesc->paramName);
   }

   savedFullpathLen = fullpathLen;
   if ((fullpathStr = cmsMem_alloc(fullpathLen, ALLOC_ZEROIZE)) == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }

   while (i < genericLen)
   {
      if (genericStr[i] != '{')
      {
         fullpathStr[j++] = genericStr[i++];
         fullpathLen--;
      }
      else
      {
         if (depth < DEPTH_OF_IIDSTACK(&(pathDesc->iidStack)))
         {
            UINT32 consumed;
            consumed = snprintf(&(fullpathStr[j]), fullpathLen, "%u",
                                INSTANCE_ID_AT_DEPTH(&(pathDesc->iidStack), depth));
            i += 3;
            j += consumed;
            fullpathLen -= consumed;
            depth++;
         }
         else
         {
            /*
             * we've hit a {i} in the generic path, but there are no more
             * instance numbers in the iidStack, so this must be fullpath
             * that specifies an object name with no trailing instance number.
             */
            isGenericObject = TRUE;
            break;
         }
      }
   }

   if (!isGenericObject && (IS_PARAM_NAME_PRESENT(pathDesc)))
   {
      /* we are assuming that the pathDesc->paramName is a valid
       * paramName in the oid. 
       */
      UINT32 consumed;
      consumed = snprintf(&(fullpathStr[j]), fullpathLen, "%s", pathDesc->paramName);
      j += consumed;
      fullpathLen -= consumed;
   }

   cmsAst_assert(j < savedFullpathLen);

   *fullpath = fullpathStr;
   return CMSRET_SUCCESS;
}


CmsRet cmsMdm_pathDescriptorToFullPathNoEndDot(const MdmPathDescriptor *pathDesc, char **fullpath)
{
   CmsRet ret;

   if (pathDesc->paramName[0] != 0)
   {
      cmsLog_error("This function can only be used on objects.");
      *fullpath = NULL;
      return CMSRET_INVALID_ARGUMENTS;
   }

   ret = cmsMdm_pathDescriptorToFullPath(pathDesc, fullpath);
   if (ret == CMSRET_SUCCESS)
   {
      char *tmp = *fullpath;
      UINT32 len = cmsUtl_strlen(tmp);

      if (len > 0 && tmp[len-1] == '.')
      {
         tmp[len-1] = '\0';
      }
   }

   return ret;
}

UBOOL8 cmsMdm_isPathDescriptorExist(const MdmPathDescriptor *pathDesc)
{
   MdmObjectNode *objNode=NULL;
   UBOOL8 exist = FALSE;

   if (pathDesc == NULL)
   {
      return FALSE;
   }

   objNode = mdm_getObjectNode(pathDesc->oid);
   if (objNode == NULL)
   {
      return FALSE;
   }

   if (IS_INDIRECT0(objNode))
   {
      /* indirect 0 objects always exist */
      exist = TRUE;
   }
   else if (IS_INDIRECT1(objNode))
   {
      if (DEPTH_OF_IIDSTACK(&(pathDesc->iidStack)) == objNode->instanceDepth)
      {
         if (mdm_getInstanceHead(objNode, &(pathDesc->iidStack)) != NULL)
         {
            exist = TRUE;
         }
      }
   }
   else if (IS_INDIRECT2(objNode))
   {
      if (DEPTH_OF_IIDSTACK(&(pathDesc->iidStack)) == objNode->instanceDepth)
      {
         if (mdm_getInstanceDescFromObjNode(objNode, &(pathDesc->iidStack)) != NULL)
         {
            exist = TRUE;
         }
      }
   }


   /*
    * Now verify that the parameter name is a parameter of the
    * specified object.  Even though fullPathToPathDescriptor also
    * verifies the parameter name, the parameter name could have been inserted
    * into the pathDesc by the caller.
    */
   if (pathDesc->paramName[0] != 0)
   {
      UINT16 p;
      UBOOL8 found = FALSE;

      for (p=0; (p < objNode->numParamNodes) && (!found); p++)
      {
         if (!strcmp(objNode->params[p].name, pathDesc->paramName))
         {
            found = TRUE;
         }
      }

      if (!found)
      {
         exist = FALSE;
      }
   }

   return exist;
}



