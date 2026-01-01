/***********************************************************************
 *
<:copyright-BRCM:2013:DUAL/GPL:standard

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
 *
 ************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>     /* for isDigit, really should be in oal_strconv.c */
#include <sys/stat.h>  /* this really should be in oal_strconv.c */
#include <arpa/inet.h> /* for inet_aton */
#include <sys/time.h> /* for inet_aton */

#include "bdk.h"
#include "cms_util.h"
#include "cms_params.h"  /* for MAX_MDM_SINGLE_FULLPATH_LEN */
#include "oal.h"


UBOOL8 cmsUtl_hasNamespaceRange(const char *fullpath)
{
   size_t len, i=0;
   UBOOL8 foundStartChar = FALSE;  // start char is [
   UBOOL8 foundSep = FALSE;    // separator is -

   if (IS_EMPTY_STRING(fullpath))
      return FALSE;

   len = strlen(fullpath);
   while (i < len)
   {
      if (fullpath[i] == '[')
         foundStartChar = TRUE;

      if (foundStartChar && (fullpath[i] == '-'))
         foundSep = TRUE;

      // The last 2 chars are ].
      if (foundStartChar && foundSep && (i == len-2) &&
          (fullpath[i] == ']')  && (fullpath[i+1] == '.'))
         return TRUE;

      i++;
   }
   return FALSE;
}

CmsRet cmsUtl_parseNamespaceRange(const char *fullpath,
      UINT32 *firstInstanceId, UINT32 *lastInstanceId, char *baseFullpath)
{
   size_t j = 0;
   size_t i;

   if (IS_EMPTY_STRING(fullpath))
   {
      cmsLog_error("NULL or empty fullpath");
      return CMSRET_INVALID_ARGUMENTS;
   }
   else if ((firstInstanceId == NULL) || (lastInstanceId == NULL) ||
       (baseFullpath == NULL))
   {
      cmsLog_error("One or more null input args %p/%p/%p", 
                firstInstanceId, lastInstanceId, baseFullpath);
      return CMSRET_INVALID_ARGUMENTS;
   }

   // Since we verify the format first, the checking below can be simplified.
   if (!cmsUtl_hasNamespaceRange(fullpath))
   {
      return CMSRET_NO_MORE_INSTANCES;
   }

   // caller must pass in a baseFullpath buf that can hold the orig fullpath.
   strcpy(baseFullpath, fullpath);
   i = strlen(baseFullpath);
   baseFullpath[i-2] = '\0';  // null terminate at the end char ]

   // find and null terminate the start char [
   i = 0;
   while (baseFullpath[i] != '[')
      i++;
   baseFullpath[i] = '\0';
   i++;

   // Find and null terminate the -
   j = i;
   while (baseFullpath[j] != '-')
      j++;
   baseFullpath[j] = '\0';
   *firstInstanceId = (UINT32) atoi(&(baseFullpath[i]));

   // Get the second number
   j++;
   *lastInstanceId = (UINT32) atoi(&(baseFullpath[j]));

   return CMSRET_SUCCESS;
}

char *cmsUtl_substring(const char *input, size_t start, size_t end)
{
    char *substring = NULL;
    size_t len;

    if (end - start <= 0)
    {
        return NULL;
    }

    len = cmsUtl_strlen(input);
    if (end > len)
    {
        return NULL;
    }

    substring = cmsMem_alloc(end - start + 1, ALLOC_ZEROIZE);
    if (substring == NULL)
    {
        return NULL;
    }

    strncpy(substring, input + start, end - start);
    return substring;
}


UINT32 cmsImg_countConfigSections(const char *buf)
{
    const char *delim = CMS_CONFIG_XML_TAG;
    char *pBegin, *pNext;
    UINT32 count=0;

    if (buf == NULL)
        return count;

    pBegin = strstr(buf, delim);
    while (pBegin != NULL)
    {
       count++;
       pNext= strstr((pBegin+strlen(delim)), delim);
       pBegin = pNext;
    }
    cmsLog_debug("Exit: sections=%d", count);
    return count;
}


const char* cmsImg_configTagToComponentName(const char *buf)
{
    const char *errorStr = "ERROR_INVALID_CONFIG";
    const char *componentName = NULL;
    UINT32 tagCount=0;
    UINT32 sectionCount;

    sectionCount = cmsImg_countConfigSections(buf);
    if (sectionCount == 0)
       goto detect_done;

    /* Search the entire config buf for a tag unique to that component.
     * In 504L02, all config files will have a "bdkConfigIdent" tag to
     * precisely identify which component this config file came from.  However,
     * to detect older config files (including old CMS monolithic config files)
     * which do not have this "bdkConfigIdent" tag, we have to use a heuristic
     * to look for a tag which is unique to that component.  Should be fine,
     * but there is a tiny possibility of misidentification.
     */
    if ((strstr(buf, "bdkConfigIdent=\"sysmgmt\"") != NULL) ||
        (strstr(buf, "<DNS>") != NULL))
    {
       /* DNS is required for Device2 */
       componentName = BDK_COMP_SYSMGMT;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"wifi\"") != NULL) ||
        (strstr(buf, "<WiFi>") != NULL))
    {
       componentName = BDK_COMP_WIFI;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"devinfo\"") != NULL) ||
        (strstr(buf, "<DeviceInfo>") != NULL))
    {
       componentName = BDK_COMP_DEVINFO;
       if (++tagCount > 1)
           goto detect_done;
    }

    if (strstr(buf, "bdkConfigIdent=\"diag\"") != NULL)
    {
       componentName = BDK_COMP_DIAG;
       if (++tagCount > 1)
           goto detect_done;
    }

    else if ((strstr(buf, "bdkConfigIdent=\"dsl\"") != NULL) ||
             (strstr(buf, "<DSL>") != NULL) ||
             (strstr(buf, "<FAST>") != NULL))
    {
       componentName = BDK_COMP_DSL;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"voice\"") != NULL) ||
        (strstr(buf, "<VoiceService>") != NULL))
    {
       componentName = BDK_COMP_VOICE;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"gpon\"") != NULL) ||
        (strstr(buf, "<OmciSystemCfg>") != NULL))
    {
       componentName = BDK_COMP_GPON;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"epon\"") != NULL) ||
        (strstr(buf, "<X_BROADCOM_COM_EPON>") != NULL))
    {
       componentName = BDK_COMP_EPON;
       if (++tagCount > 1)
           goto detect_done;
    }

    if ((strstr(buf, "bdkConfigIdent=\"openplat\"") != NULL) ||
        (strstr(buf, "<SoftwareModules>") != NULL))
    {
       componentName = BDK_COMP_OPENPLAT;
       if (++tagCount > 1)
           goto detect_done;
    }

    if (strstr(buf, "bdkConfigIdent=\"tr69\"") != NULL)
    {
       // This is a new component, so it always has the bdkConfigIdent tag.
       componentName = BDK_COMP_TR69;
       if (++tagCount > 1)
           goto detect_done;
    }

    if (strstr(buf, "bdkConfigIdent=\"usp\"") != NULL)
    {
       // This is a new component, so it always has the bdkConfigIdent tag.
       componentName = BDK_COMP_USP;
       if (++tagCount > 1)
           goto detect_done;
    }

detect_done:
    if (sectionCount == 0)
    {
        cmsLog_error("Could not find Config File section marker!");
        componentName = errorStr;
    }
    else if ((sectionCount == 1) && (tagCount == 1))
    {
        // This is just one section of a BDK config file, as fed to us by
        // remote_objd.  Do nothing in this block.  componentName is correct.
    }
    else if ((sectionCount == 1) && (tagCount > 1))
    {
        // CMS classic monolithic config file
        componentName = BDK_COMP_CMS_CLASSIC;
    }
    else if (sectionCount > 1)
    {
        // BDK concatentated config file, each component writes out a section
        componentName = "BDK_Concatenated_Config";
    }
    else
    {
       char debugBuf[1000]={0};
       int len=(int)strlen(buf);
       strncpy(debugBuf, buf, sizeof(debugBuf)-1);
       if ((sectionCount == 1) && (tagCount == 0) && (len < 300))
       {
          // EPON module may have an "empty" config, so we cannot identify it.
          // If it is small and trivial looking (<300 bytes), it could be ok to
          // skip.  See Jira 39943.  A better solution is to put the component
          // name in one of the top level tags.
          printf("Skip trivial and non-identifiable config file (len=%d):\n%s\n\n", len, buf);
       }
       else
       {
          cmsLog_error("Unsupported config buf (sectionCount=%d tagCount=%d len=%d), first %d bytes: %s\n",
                        sectionCount, tagCount, len,
                        sizeof(debugBuf), debugBuf);
          componentName = errorStr;
       }
    }

    cmsLog_debug("Exit: componentName=%s", componentName);
    return componentName;
}


CmsRet cmsUtl_parseFullpathToGeneric(const char *fullpath, char **pGenericPath,
                                     InstanceIdStack *iidStack)
{
   size_t i, j, k;
   size_t len, maxLen;
   char *genericPath = NULL;
   UBOOL8 foundDigit;

   if (IS_EMPTY_STRING(fullpath))
   {
      cmsLog_error("NULL or empty fullpath");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (iidStack == NULL)
   {
       cmsLog_error("iidStack must be passed in.");
       return CMSRET_INVALID_ARGUMENTS;
   }

   len = strlen(fullpath);
   //We will replace instance number such as: ".1." to ".{i}.", 
   //in this worst case, the length grows from 3 to 5. 
   //A crude calculation is just double the origin string length.
   //But consider the extrem case of ".8" as the input fullpath,
   //which is legal in our function. The generated genericPath will be
   //".{i}.". So add additional 1 bytes here to compasate for the special
   //case. Finally, 1 byte is needed for the '\0' termination byte.
   maxLen = 2 * len + 2;
   genericPath = cmsMem_alloc(maxLen, ALLOC_ZEROIZE);
   if (genericPath == NULL)
   {
       return CMSRET_RESOURCE_EXCEEDED;
   }
   
   memset(iidStack, 0x00, sizeof(InstanceIdStack));
   k = 0;
   for (i = 0; i < len; i++)
   {
       cmsLog_debug("i=%d, k=%d, genericPath=%s", i, k, genericPath);
       genericPath[k++] = fullpath[i];
       if (fullpath[i] == '.')
       {
           foundDigit = FALSE;
           //look ahead for instance number.
           for (j = i + 1; j < len; j++)
           {
               if (fullpath[j] >= '0' && fullpath[j] <= '9')
               {
                   foundDigit = TRUE;
               }
               else
               {
                   break;
               }
           }

           if ((j < len && fullpath[j] == '.') ||
               (j == len && foundDigit == TRUE))
           {   
               UINT32 instanceId;
               char *substring = cmsUtl_substring(fullpath, i + 1, j);

               //Retrieve instanceId.
               instanceId = (UINT32) atoi(substring);
               cmsMem_free(substring);
               if (iidStack->currentDepth < sizeof(iidStack->instance))
               {
                   iidStack->instance[iidStack->currentDepth++] = instanceId;
               }

               //Replace the substring to ".{i}."
               strcpy(genericPath + k, "{i}.");
               k += strlen("{i}.");
               i = j;
           }
       }
   }

   *pGenericPath = genericPath;

   return CMSRET_SUCCESS;
}

CmsRet cmsUtl_genericPathToFullPath(const char *genericPath,
                                    const InstanceIdStack *iidStack,
                                    char **fullpath,
                                    int truncate)
{
   CmsRet ret = CMSRET_SUCCESS;
   char *fullpathStr = NULL;
   UINT32 i, k, lastDot, depth, elen;
   UINT32 genericLen, fullpathLen;

   /*
    * Each instance id can have a maximum of 10 digits.  The generic string
    * has a "{i}" where the instance id would go, so we have to allocate 7 more
    * bytes per instance number for the fullpathStr.
    */
   genericLen = strlen(genericPath);
   fullpathLen =  genericLen + (7 * MAX_MDM_INSTANCE_DEPTH) + 1 /* for the null */;
   if ((fullpathStr = cmsMem_alloc(fullpathLen, ALLOC_ZEROIZE)) == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }
   
   k = 0;
   depth = 0;
   lastDot = 0;
   elen = strlen(".{i}.");
   for (i = 0; i < genericLen; i++)
   {
       cmsLog_debug("i=%d, k=%d, lastDot=%d, fullpathStr=%s", i, k, lastDot, fullpathStr);
       fullpathStr[k++] = genericPath[i];
       if (genericPath[i] == '.')
       {
           //look ahead for "{i}" substring
           if (strncmp(genericPath + i, ".{i}.", elen) == 0)
           {
               UINT32 instance = 0;
               char numStr[64] = {0};

               if (depth < iidStack->currentDepth)
               {
                   instance = iidStack->instance[depth];
                   depth++;
               }
               else
               {
                   if (truncate)
                   {
                       cmsLog_debug("rollback to lastDot[%d] and return fullpathStr.", lastDot);
                       fullpathStr[lastDot + 1] = '\0';
                       break;
                   }
                   else
                   {
                       ret = CMSRET_INVALID_ARGUMENTS;
                       cmsMem_free(fullpathStr);
                       fullpathStr = NULL;
                       break;
                   }
               }

               snprintf((char *)numStr, sizeof(numStr) - 1, "%u", instance);
               memcpy(fullpathStr + k, numStr, strlen(numStr));
               k += strlen(numStr);
               lastDot = k;
               fullpathStr[k++] = '.';
               i += elen - 1;
           }
           else
           {
               lastDot = k - 1;
           }
       }
   }
   *fullpath = fullpathStr;
   return ret;
}

UBOOL8 cmsUtl_hasInstanceAlias(const char *fullpath)
{
   char baseFullpath[MAX_MDM_SINGLE_FULLPATH_BUFLEN] = {0};
   char aliasBuf[MAX_MDM_ALIAS_BUFLEN]={0};
   CmsRet ret;

   if (IS_EMPTY_STRING(fullpath))
      return FALSE;

   if (strlen(fullpath) >= sizeof(baseFullpath))
   {
      cmsLog_error("fullpath %s is too long", fullpath);
      return FALSE;
   }

   ret = cmsUtl_parseInstanceAlias(fullpath,
                                   baseFullpath, aliasBuf, sizeof(aliasBuf));
   // if parse was successful, then fullpath contained an alias
   return (ret == CMSRET_SUCCESS);
}

CmsRet cmsUtl_parseInstanceAlias(const char *fullpath,
                                 char *baseFullpath,
                                 char *aliasBuf, UINT32 aliasBufLen)
{
   size_t i;

   if (IS_EMPTY_STRING(fullpath))
      return FALSE;

   if ((aliasBuf == NULL) || (baseFullpath == NULL))
   {
      cmsLog_error("One or more null input args %p/%p", 
                aliasBuf, baseFullpath);
      return CMSRET_INVALID_ARGUMENTS;
   }

   // caller must pass in a baseFullpath buf that can hold the orig fullpath.
   strcpy(baseFullpath, fullpath);

   // For now, only detect the instance alias at the end of the fullpath.
   // Detect either Device.QoS.Queue.[somealias]. or
   // Device.QoS.Queue.[somealias]
   i = strlen(baseFullpath);

   if (i < 3)  // protect against crazy short string
      return CMSRET_NO_MORE_INSTANCES;

   if ((baseFullpath[i-2] != ']') && (baseFullpath[i-1] != ']'))
   {
      // no alias found.
      return CMSRET_NO_MORE_INSTANCES;
   }

   // null terminate at the end char ]
   if (baseFullpath[i-2] == ']')
   {
      baseFullpath[i-2] = '\0';
      i -= 2;
   }
   else
   {
      baseFullpath[i-1] = '\0';
      i -=1;
   }

   // Now go back to find the left bracket.
   // We shoulld find left bracket before i == 0, DESKTOP gcc does not allow
   // checking for i >=0.
   while ((i > 0) && (baseFullpath[i] != '['))
      i--;

   if (baseFullpath[i] != '[')
   {
      cmsLog_error("Bad or suspicious fullpath %s", fullpath);
      return CMSRET_NO_MORE_INSTANCES;
   }

   // null terminate at [
   baseFullpath[i] = '\0';
   i++;

   if (strlen(&(baseFullpath[i])) >= aliasBufLen-1)
   {
      cmsLog_error("aliasBufLen(%u) too short for alias %s",
                   aliasBufLen, &(baseFullpath[i]));
      return CMSRET_INVALID_ARGUMENTS;
   }

   strcpy(aliasBuf, &(baseFullpath[i]));
   return CMSRET_SUCCESS;
}


#ifdef DMP_DEVICE2_BASELINE_1

extern UBOOL8 cmsUtl_isValidVpiVci(SINT32 vpi, SINT32 vci);

/* The vpiVciStr is in this format: VPI/VCI (e.g. 0/35) */
CmsRet cmsUtl_atmVpiVciStrToNum_dev2(const char *vpiVciStr, SINT32 *vpi, SINT32 *vci)
{
   char *pSlash;
   char vpiStr[BUFLEN_256];
   char vciStr[BUFLEN_256];
   
   *vpi = *vci = -1;   
   if (vpiVciStr == NULL)
   {
      cmsLog_error("vpiVciStr is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }      

   strncpy(vpiStr, vpiVciStr, sizeof(vpiStr)-1);
   vpiStr[sizeof(vpiStr)-1] = '\0';

   pSlash = (char *) strchr(vpiStr, '/');
   if (pSlash == NULL)
   {
      cmsLog_error("vpiVciStr %s is invalid", vpiVciStr);
      return CMSRET_INVALID_ARGUMENTS;
   }
   strncpy(vciStr, (pSlash + 1), sizeof(vciStr)-1);
   vciStr[sizeof(vciStr)-1] = '\0';
   *pSlash = '\0';       
   *vpi = atoi(vpiStr);
   *vci = atoi(vciStr);
   if (cmsUtl_isValidVpiVci(*vpi, *vci) == FALSE)
   {
      return CMSRET_INVALID_PARAM_VALUE;
   }     

   return CMSRET_SUCCESS;
}


CmsRet cmsUtl_atmVpiVciNumToStr_dev2(const SINT32 vpi, const SINT32 vci, char *vpiVciStr)
{
   if (vpiVciStr == NULL)
   {
      cmsLog_error("vpiVciStr is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }         
   if (cmsUtl_isValidVpiVci(vpi, vci) == FALSE)
   {
      return CMSRET_INVALID_PARAM_VALUE;
   }     

   sprintf(vpiVciStr, "%d/%d", vpi, vci);

   return CMSRET_SUCCESS;
   
}


#endif  /* DMP_DEVICE2_BASELINE_1 */


/*
 * The next couple were orginally intended for manipulating fullpaths in
 * the TR181 LowerLayers param, but turns out they can be used for
 * manipulating intfNames in a comma separated list or anything in
 * a comma separated list.
 * XXX TOOD: many of thse funcs can use strtok_r.
 * XXX TODO: consolidate with cmsUtl_isSubOptionPresent
 */
CmsRet cmsUtl_addFullPathToCSL(const char *fullPath, char *CSLBuf, UINT32 CSLlen)
{
   /* fully implemented and unit tested (suite_str.c:addFullPathToCSL1) */

   if (fullPath == NULL || CSLBuf == NULL)
   {
      cmsLog_error("NULL input params");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (cmsUtl_isFullPathInCSL(fullPath, CSLBuf))
   {
      return CMSRET_SUCCESS;
   }

   if (cmsUtl_strlen(fullPath)+cmsUtl_strlen(CSLBuf)+2 > (SINT32) CSLlen)
   {
      cmsLog_error("CSLBuf len %d too small to add %s (currently %s)",
                    CSLlen, fullPath, CSLBuf);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if (cmsUtl_strlen(CSLBuf) > 0)
   {
      /* there are already elements in the list, so add a comma first */
      strcat(CSLBuf, ",");
   }

   strcat(CSLBuf, fullPath);

   return CMSRET_SUCCESS;
}


void cmsUtl_deleteFullPathFromCSL(const char *fullPath, char *CSLBuf)
{
   UINT32 CSLlen;
   UINT32 fpLen;
   char *start;
   char *end;

   /* fully implemented and unit tested (suite_str.c:deleteFullPathFromCSL1) */

   fpLen = cmsUtl_strlen(fullPath);
   CSLlen = cmsUtl_strlen(CSLBuf);
   if ((fpLen == 0) || (CSLlen == 0))
   {
      /* nothing to delete, return success anyways. */
      return;
   }

   if (!cmsUtl_isFullPathInCSL(fullPath, CSLBuf))
   {
      /* fullpath is not in CSLBuf, so nothing to do */
      return;
   }

   start = strstr(CSLBuf, fullPath);
   end = start + fpLen;

   /* find the start of next fullpath, or end of CSLBuf */
   while (*end != '\0' && ((*end == ',') ||
                           (isspace(*end))))
   {
      end++;
   }

   if (*end == '\0')
   {
      /* we are deleting the last fullpath in CSLBuf, back up to delete
       * the last comma.
       */
      if (start > CSLBuf)
      {
         start--;
      }
   }

   /* pull up all the characters in CSLBuf */
   while (*end != '\0')
   {
      *start = *end;
      start++;
      end++;
   }

   /* zero out the rest of the buffer */
   while (start < (CSLBuf + CSLlen))
   {
      *start = '\0';
      start++;
   }

   return;
}


UBOOL8 cmsUtl_isFullPathInCSL(const char *fullPath, const char *CSLBuf)
{
   UINT32 i=0;
   UINT32 n;
   UINT32 CSLlen;

   /* fully implemented and unit tested (suite_str.c:isFullPathInCSL1) */

   CSLlen = cmsUtl_strlen(CSLBuf);
   if (fullPath == NULL || CSLlen == 0)
   {
      return FALSE;
   }

   n = strlen(fullPath);
   while (i < CSLlen)
   {
      /* advance past any leading white space.  This seems overly
       * paranoid.  Our code controls how this field is written, and we
       * never put any white space between comma and next fullpath.
       */
      while (i < CSLlen && isspace(CSLBuf[i]))
      {
         i++;
      }
      if (i >= CSLlen)
         break;

      /* a match is a match on the length of the fullpath string,
       * and the fullpath in the CSLBuf must be terminated with comma,
       * space, or end of string.
       */
      if (!strncmp(fullPath, &CSLBuf[i], n) &&
                 ((CSLBuf[i+n] == ',') ||
                  (isspace(CSLBuf[i+n]) ||
                  (CSLBuf[i+n] == '\0'))))
      {
         return TRUE;
      }

      /* advance i past next comma */
      while (i < CSLlen)
      {
         if (CSLBuf[i] == ',')
         {
            i++;
            break;
         }
         i++;
      }
   }

   return FALSE;
}



#ifdef DMP_DEVICE2_BASELINE_1

/** Look for fullpaths that are in srcBuf and not in dstBuf.
 *
 */
static CmsRet calcDiffFullPathCSLs(const char op,
                                   const char *srcBuf, const char *dstBuf,
                                   char *diffCSLBuf, UINT32 diffCSLBufLen)
{
   UINT32 srcIdx=0;
   CmsRet ret=CMSRET_SUCCESS;


   /* First iterate through the srcBuf and process fullpath one by one */
   while (srcBuf[srcIdx] != '\0')
   {
      char tmpBuf[MAX_MDM_SINGLE_FULLPATH_BUFLEN+1];
      UINT32 tmpIdx;

      /* suck in the first fullpath from the src buf */
      memset(tmpBuf, 0, sizeof(tmpBuf));
      tmpBuf[0] = op;
      tmpIdx = 1;
      while (srcBuf[srcIdx] != ',' && srcBuf[srcIdx] != '\0')
      {
         tmpBuf[tmpIdx++] = srcBuf[srcIdx++];
         if (tmpIdx >= sizeof(tmpBuf)-1)
         {
            cmsLog_error("fullpath too long (len=%d, tmpBuf=%s)", tmpIdx, tmpBuf);
            return CMSRET_RESOURCE_EXCEEDED;
         }
      }

      if (!cmsUtl_isFullPathInCSL(&tmpBuf[1], dstBuf))
      {
         /*
          * This fullpath in the srcBuf is not in the dstBuf, so add it to
          * the diffCSLBuf.
          */
         ret = cmsUtl_addFullPathToCSL(tmpBuf, diffCSLBuf, diffCSLBufLen);
         if (ret != CMSRET_SUCCESS)
         {
            return ret;
         }
      }

      /* now advance past the comma and any white space */
      while ((srcBuf[srcIdx] == ',') ||
              isspace(srcBuf[srcIdx]))
      {
         srcIdx++;
      }
   }

   return ret;
}


CmsRet cmsUtl_diffFullPathCSLs(const char *newLowerLayerBuf,
                               const char *currLowerLayerBuf,
                               char *diffCSLBuf,
                               UINT32 diffCSLBufLen)
{
   CmsRet ret=CMSRET_SUCCESS;

   /* Fully implemented and unit tested (suite_str.c:diffFullPathToCSL1,2,3) */

   memset(diffCSLBuf, 0, diffCSLBufLen);

   /* Look for fullpaths that were deleted from newLowerLayerBuf.
    * Do the delete first so interface stack processing can delete
    * these entries before adding new ones.
    */
   if ((ret == CMSRET_SUCCESS) && (cmsUtl_strlen(currLowerLayerBuf) > 0))
   {
      ret = calcDiffFullPathCSLs('-', currLowerLayerBuf, newLowerLayerBuf,
                                      diffCSLBuf, diffCSLBufLen);
   }

   /* Look for new fullpaths added in newLowerLayerBuf */
   if (cmsUtl_strlen(newLowerLayerBuf) > 0)
   {
      ret = calcDiffFullPathCSLs('+', newLowerLayerBuf, currLowerLayerBuf,
                                      diffCSLBuf, diffCSLBufLen);
   }

   return ret;
}


#endif /* DMP_DEVICE2_BASELINE_1 */
