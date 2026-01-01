/***********************************************************************
<:copyright-BRCM:2012:proprietary:standard

   Copyright (c) 2012 Broadcom
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

/*!\file mdm_cmddebug.c
 * \brief Generic MDM access and debug functions.  This code came from
 * consoled/CMS CLI.  But now that it is in a separate library, it can
 * be used to access the Distributed MDM using mdm_cli.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// should be in string.h, but including it did not fix compiler warning
// so I just declare the function here.
extern char *strcasestr(const char *haystack, const char *needle);
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "prctl.h"
#include "mdm.h"
#include "mdm_cli.h"
#include "mdm_types.h"
#include "bcm_ulog.h"
#include "bcm_generic_hal_utils.h"
#include "sysutil_proc.h"


void processSaveCmd(char *cmdLine __attribute__((unused)))
{
   CmsRet ret;

   ret = cmsMgm_saveConfigToFlash();
   if (ret != CMSRET_SUCCESS)
   {
      printf("Could not save config to flash, ret=%d\n", ret);
   }
   else
   {
      printf("config saved.\n");
   }

   return;
}

void processInvalidateCfgCmd(char *cmdLine __attribute__((unused)))
{
   cmsLog_notice("invalidate config file");

   cmsMgm_invalidateConfigFlash();

   /*
    * Note that unliked the CMS Classic RestoreToDefault command, this one
    * does not reboot the system.  It is possible that something else will
    * save the config file right after this command has finished.
    */

   return;
}


void processDumpCfgCmd(char *cmdLine)
{
   char *cfgBuf;
   UINT32 origCfgBufLen, cfgBufLen;
   CmsRet ret;
   UBOOL8 readFromFlash = TRUE;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpcfg [dynamic]\n");
      printf("by default, dump contents of config flash.\n");
      printf("dynamic: dump what would get written to config flash from MDM.\n");
      return;
   }

   if (strlen(cmdLine) > 0)
   {
      if (!strcasecmp(cmdLine, "dynamic"))
      {
         readFromFlash = FALSE;
      }
      else
      {
         printf("invalid arguments\n");
         return;
      }
   }


   origCfgBufLen = cfgBufLen = cmsImg_getConfigFlashSize();
   if (cfgBufLen == 0)
   {
      bcmuLog_error("Could not get config flash size");
      return;
   }
   else
   {
      bcmuLog_debug("configBufLen=%d", cfgBufLen);
   }

   cfgBuf = cmsMem_alloc(cfgBufLen, 0);
   if (cfgBuf == NULL)
   {
      bcmuLog_error("malloc of %d bytes failed", cfgBufLen);
      return;
   }


   // Due to the new CMS auto-locking feature, cmsMgm_readConfigFlashToBuf and
   // cmsMgm_writeConfigToBuf will do the locking for us, so we did not need
   // to acquire the global lock before entering this function.
   if (readFromFlash)
   {
      char *buf = NULL;
      ret = cmsMgm_readConfigFlashToBufEx(&buf, 0);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("read config failed, ret=%d", ret);
      }
      else
      {
         printf("%s", buf);
         CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
      }
   }
   else
   {
      // dynamic option: dump the config from the MDM into the memory buffer,
      // this may be different than the config that is currently written in flash.
      // This is a rarely used option and only supports monolithic MDM.
      ret = cmsMgm_writeConfigToBuf(cfgBuf, &cfgBufLen);
      if (ret != CMSRET_SUCCESS)
      {
         bcmuLog_error("read config failed, ret=%d cfgBufLen=%d", ret, cfgBufLen);
      }
      else
      {
         printf("%s", cfgBuf);
         printf("dump bytes allocated=%d used=%d\n", origCfgBufLen, cfgBufLen);
      }
   }


   cmsMem_free(cfgBuf);

   return;
}


static void cmdMemInfoHelp(void)
{

   printf("usage: meminfo [app name] [operation]\n");
   printf("  App name can be httpd, tr69c, or ssk.  If app name is omitted, then the operation is done for CLI app.\n");
   printf("  operation is one of stats, traceAll, trace50, traceClones.  If operation is omitted, then stats.\n");
   printf("Examples:\n");
   printf("  meminfo : dumps the memory stats as seen by the CLI app.  Same as meminfo self stats.\n");
   printf("  meminfo ssk : send a message to ssk to tell it to dump its memory stats.  Same as meminfo ssk stats.\n");
   printf("  meminfo httpd trace50 : send a message to httpd to tell it to dump last 50 leak tracing records\n");
   printf("  meminfo traceClones : tell this CLI app to dump leak trace records with 5 or more clones.\n\n");

   return;
}

void processMeminfoCmd(char *cmdLine)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   msg.dst = EID_INVALID;
   msg.type = CMS_MSG_MEM_DUMP_STATS;  /* default op */

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      cmdMemInfoHelp();
   }

   if (!strncasecmp(cmdLine, "httpd", 5))
   {
      msg.dst = EID_HTTPD;
   }
   else if (!strncasecmp(cmdLine, "ssk", 3))
   {
      msg.dst = EID_SSK;
   }
   else if (!strncasecmp(cmdLine, "tr69c", 5))
   {
      msg.dst = EID_TR69C;
   }
   else if (!strncasecmp(cmdLine, "omcid", 5))
   {
      msg.dst = EID_OMCID;
   }
   else if (!strncasecmp(cmdLine, "remote_objd", 11))
   {
      msg.dst = EID_REMOTE_OBJD;
   }

   if (strcasestr(cmdLine, "traceall"))
   {
      msg.type = CMS_MSG_MEM_DUMP_TRACEALL;
   }
   else if (strcasestr(cmdLine, "trace50"))
   {
      msg.type = CMS_MSG_MEM_DUMP_TRACE50;
   }
   else if (strcasestr(cmdLine, "traceClones"))
   {
      msg.type = CMS_MSG_MEM_DUMP_TRACECLONES;
   }

   if (msg.dst == EID_INVALID)
   {
      /* Run operation for this app */
      if (msg.type == CMS_MSG_MEM_DUMP_STATS)
      {
         MdmInfo mInfo;
         cmsMdm_getInfo(&mInfo);
         printf("MDM Shared Memory Region %p-%p (%dKB)\n\n",
                mInfo.shmBegin, mInfo.shmEnd,
                ((UINT32)(mInfo.shmEnd - mInfo.shmBegin))/1024);
         cmsMem_dumpMemStats();
      }

#ifdef CMS_MEM_LEAK_TRACING
      else if (msg.type == CMS_MSG_MEM_DUMP_TRACEALL)
      {
         cmsMem_dumpTraceAll();
      }
      else if (msg.type == CMS_MSG_MEM_DUMP_TRACE50)
      {
         cmsMem_dumpTrace50();
      }
      else if (msg.type == CMS_MSG_MEM_DUMP_TRACECLONES)
      {
         cmsMem_dumpTraceClones();
      }
#endif /* CMS_MEM_LEAK_TRACING */
   }
   else
   {
      CmsRet ret;
      const CmsEntityInfo *eInfo = cmsEid_getEntityInfo(msg.dst);

      if (eInfo)
         printf("sending msg 0x%x to %s\n", msg.type, eInfo->name);
      else
         printf("sending msg 0x%x to 0x%x\n", msg.type, msg.dst);

      /* send the message to another app */
      msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
      msg.flags_event = 1;
      if ((ret = cmsMsg_send(cliPrvtMsgHandle, &msg)) != CMSRET_SUCCESS)
      {
         bcmuLog_error("msg send failed, ret=%d", ret);
      }
   }

   return;
}


void processTraceLocksCmd(char *cmdLine)
{
   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: traceLocks\n");
      printf("toggles lock tracing.  If currently off, then will start.  If currently on, then will stop.\n");
      return;
   }

   cmsLck_toggleTracing();
}


void processDumpLocksCmd(char *cmdLine)
{
   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumplocks\n");
      printf("dump all CMS MDM lock info and stats.\n");
      return;
   }

   cmsLck_dumpInfo();
}


static void dumpOidInfoEntry(const MdmOidInfoEntry *entry)
{
   const MdmObjectNode *node = mdm_getObjectNodeFlags(entry->oid,
                                                   GET_OBJNODE_LOCAL, NULL);

   printf("%d: %s\n", entry->oid, entry->fullPath);
   printf("MDM Lock Zone        : %d\n", cmsLck_getLockZone(entry->oid));
   if (NULL != node)
   {
      printf("InstanceDepth        : %d\n", node->instanceDepth);
      printf("Flags                : 0x%x ", node->flags);
      if (node->flags & OBN_MULTI_COMP_OBJ)
      {
         printf("<MULTI_COMP_OBJ>");
      }
   }
   printf("\n");
   printf("rcl handler pre-hook : %p\n", entry->rclHandlerFuncPreHook);
   printf("rcl handler          : %p\n", entry->rclHandlerFunc);
   printf("stl handler          : %p\n", entry->stlHandlerFunc);
   printf("stl handler post-hook: %p\n", entry->stlHandlerFuncPostHook);
}


void processShmInfoCmd(char *cmdLine)
{
   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: shmInfo [-a] \n");
      printf("   dump shared memory info for current process\n");
      printf("   -a: dump all shared memory regions in system\n");
      return;
   }

   {
      MdmInfo mInfo;
      memset(&mInfo, 0, sizeof(mInfo));
      cmsMdm_getInfo(&mInfo);
      printf("MDM Shared Memory Info for this process:\n");
      printf("  shmId  =%d\n", mInfo.shmId);
      printf("  address=%p-%p (%dKB)\n",
                mInfo.shmBegin, mInfo.shmEnd,
                ((UINT32)(mInfo.shmEnd - mInfo.shmBegin))/1024);
      printf("  numAttached=%d\n\n", mInfo.numAttached);
   }


   if (!strstr(cmdLine, "-a"))
   {
      return;
   }

   {
      FILE *fp;
      int n;

      printf("== Dumping all shared memory regions (some may not be created by BDK) ==\n");
      fp = fopen("/proc/sysvipc/shm", "r");
      if (fp != NULL)
      {
         char line[1024]={0};
         char buf[1024]={0};
         SINT32 key, shmId, perms, s, cpid, lpid, numAttached;
         ProcThreadInfo procInfo;

         // read the first line and discard
         if (fgets(line, sizeof(line), fp) != NULL)
         {
             cmsLog_debug("first line=%s", line);
         }

         do
         {
            // the last field is rss, which may be useful.
            shmId = 0;
            cpid = 0;
            numAttached = 0;
            n = fscanf(fp, "%d %d %d %d %d %d %d %*[^\n]",
                       &key, &shmId, &perms, &s, &cpid, &lpid, &numAttached);

            if (n < 7)
            {
               break;
            }

            memset(&procInfo, 0, sizeof(procInfo));
            sysUtl_getThreadInfoFromProc(cpid, &procInfo);
            snprintf(buf, sizeof(buf), "pid %d (%s)", cpid,
                     (IS_EMPTY_STRING(procInfo.name) ? "unknown" : procInfo.name));

            printf("  shmId=%d numAttached=%d [created by %s] size=%dKB\n",
                   shmId, numAttached, buf, s/1024);

            // do cmsLog_debug of unused vars to avoid compiler warnings
            cmsLog_debug("key=%d perms=%d lpid=%d", key, perms, lpid);
         } while (n >= 7);
      }
      printf("\n");

      fclose(fp);
   }

   return;
}


// Returns TRUE if found
static UBOOL8 dumpRemoteNode(UINT16 oid)
{
   const MdmObjectNode *node = mdm_getObjectNodeFlags(oid, GET_OBJNODE_REMOTE,
                                                      NULL);

   if (node != NULL)
   {
      printf("remote %d: %s (instanceDepth=%d flags=0x%x",
             node->oid, node->genericFullpath,
             node->instanceDepth, node->flags);
      if (node->flags & OBN_MULTI_COMP_OBJ)
      {
         printf(" <MULTI_COMP_OBJ>");
      }
      printf(")\n");
      return TRUE;
   }

   return FALSE;
}

void processDumpOidCmd(char *cmdLine)
{
   const MdmOidInfoEntry *entry = NULL;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpOid [oid]\n");
      printf("dump object id info.  if no OID is given, all OIDs will be dumped.\n");
      return;
   }

   if (isdigit(cmdLine[0]))
   {
      UINT16 oid = atoi(cmdLine);
      UBOOL8 found = FALSE;

      // An OID may be in both the local and remote MDM tables.
      if ((entry = cmsMdm_getOidInfoEntry(oid)) != NULL)
      {
         dumpOidInfoEntry(entry);
         found = TRUE;
      }

      found |= dumpRemoteNode(oid);

      if (!found)
      {
         printf("Unknown oid %d\n", oid);
      }
   }
   else
   {
      UINT32 numEntries=0;
      UINT32 i;

      entry = cmsMdm_getAllOidInfoEntries(&numEntries);
      for (i=0; i < numEntries; i++)
      {
         dumpOidInfoEntry(&(entry[i]));
      }
      printf("\nTotal Entries: %d\n\n", numEntries);

      if (cmsMdm_isRemoteCapable())
      {
         UINT32 maxOid = mdm_getMaxOid();

         printf("This component is capable of accessing remote objs.\n");
         printf("Dump of the remote OIDs:\n");
         for (i=0; i <= maxOid; i++)
         {
            dumpRemoteNode(i);
         }
      }
   }

   return;
}

void processDumpMsgConnCmd(char *cmdLine)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpMsgConn\n");
      printf("       dump all msg connections in bcm_msgd... but if running \n");
      printf("          in sysmgmt component, dump msg connections in smd.\n");
      return;
   }

   msg.type = CMS_MSG_DUMP_MSG_CONNECTIONS;
   msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg.dst = (cmsMdm_isBdkSysmgmt()) ? EID_SMD : EID_BCM_MSGD;
   msg.flags_request = 1;

   cmsMsg_sendAndGetReply(cliPrvtMsgHandle, &msg);
   return;
}

#define MDM_CLI_LOGLEVEL_NUM 3
void processLogLevelCmd(char *cmdLine)
{
   MdmParamNode *paramNode;
   MdmObjectNode *objNode;
   MdmPathDescriptor parent = EMPTY_PATH_DESCRIPTOR;
   MdmPathDescriptor child = EMPTY_PATH_DESCRIPTOR;
   PhlSetParamValue_t setParamValue = EMPTY_PHL_SETPARAM_VALUE;
   PhlGetParamValue_t *pGetParamValue;

   UBOOL8 setMode;
   SINT32 i = 0;
   char *logString[MDM_CLI_LOGLEVEL_NUM] = {"Error", "Notice", "Debug"};
   char sInfo[BUFLEN_128];

   CmsRet ret = CMSRET_SUCCESS;

   printf("cmdLine is ->%s<-\n", cmdLine);
   if (!strncasecmp(cmdLine, "help", 4) || !strncasecmp(cmdLine, "--help", 6))
   {
      printf("usage: loglevel get appname\n");
      printf("       loglevel set appname loglevel\n");

      printf("loglevel is one of \"Error\", \"Notice\", or \"Debug\" (use these exact strings).\n");
      return;
   }

   setMode = (strstr(cmdLine, "set") != NULL);
   if (setMode)
   {
       for (i=0; i<MDM_CLI_LOGLEVEL_NUM; i++)
       {
           if (strstr(cmdLine, logString[i]) != NULL)
               break;
       }
       if (i == MDM_CLI_LOGLEVEL_NUM)
       {
           printf("set failed, unsupported log level\n");
           return;
       }
   }

    ret = cmsMdm_fullPathToPathDescriptor(mdm_oidToGenericPath(MDMOID_APP_CFG), &parent);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("Could not convert %s", mdm_oidToGenericPath(MDMOID_APP_CFG));
       return;
    }

    while (mdm_getNextChildObjPathDesc(&parent, &child) == CMSRET_SUCCESS)
    {
      objNode = mdm_getObjectNode(child.oid);

      memset(sInfo, 0, sizeof(sInfo));
      /* remove postfix 'Cfg' in obj node name*/
      strncpy(sInfo, objNode->name, strlen(objNode->name)-3);
      if (strcasestr(cmdLine, sInfo) != NULL)
      {
        paramNode = mdm_getParamNode(child.oid, "LoggingLevel");
        if (paramNode == NULL)
        {
          printf("Parameter node error: LoggingLevel not found\n");
          return;
        }

        strncpy(sInfo, mdm_oidToGenericPath(child.oid), sizeof(sInfo)-1);
        sInfo[sizeof(sInfo)-1] = '\0';

        if ((strlen(sInfo) + strlen(paramNode->name)) < BUFLEN_128)
        {
            strcat(sInfo, paramNode->name);
        }
        else
        {
            printf("Too long node path\n");
            return;
        }

        ret = cmsMdm_fullPathToPathDescriptor(sInfo, &child);
        if (ret != CMSRET_SUCCESS)
        {
           cmsLog_error("Could not convert %s", sInfo);
           return;
        }

        if (setMode)
        {
           setParamValue.pParamType = (char*)cmsMdm_paramTypeToString(paramNode->type);
           setParamValue.pValue = logString[i];
           setParamValue.pathDesc = child;
           setParamValue.status = CMSRET_SUCCESS;
           ret = cmsPhl_setParameterValues(&setParamValue, 1);
           if (ret == CMSRET_SUCCESS)
              printf("new log level set is %s, ret %d, name %s\n", logString[i], ret, paramNode->name);
           else
              printf("loglevel set failed, ret %d\n", ret);

        }
        else
        {
           ret = cmsPhl_getParameterValues(&child, 1, TRUE, &pGetParamValue, &i);
           if (ret == CMSRET_SUCCESS)
           {
              printf("current log level is %s\n", pGetParamValue->pValue);
              cmsPhl_freeGetParamValueBuf(pGetParamValue, i);
           }
           else
              printf("loglevel get failed, ret %d\n", ret);
        }
      }
    }

   return;
}

void processDumpMdmCmd(char *cmdLine)
{
   char *cfgBuf = NULL;
   UINT32 flags = 0;
   CmsRet ret;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: dumpmdm\n");
      printf("dump entire contents of the MDM, this is not what would be written to the config flash.\n");
      return;
   }

   ret = cmsMgm_writeMdmToBufEx(&cfgBuf, flags);
   if (ret != CMSRET_SUCCESS)
   {
      bcmuLog_error("read config failed, ret=%d", ret);
   }
   else
   {
      int cfgBufLen = cmsUtl_strlen(cfgBuf);
      printf("%s", cfgBuf);
      printf("dump bytes used=%d bytes\n", cfgBufLen+1);
      cmsMem_free(cfgBuf);
   }

   return;
}


#define SETPV_MAX_NAME_VALUE_PAIRS  30

/** Parse/convert string of up to SETPV_MAX_NAME_VALUE_PAIRS name/value pairs
 *  into an array of MdmPathDescriptors and array of char *.
 *
 *  @param (IN) cmdLine the string of name/value pairs
 *  @param (OUT) an array of string pointers to fullpaths.  This function will allocate
 *               an array of SETPV_MAX_NAME_VALUE_PAIRS string pointers.
 *               The caller is responsible for freeing it using cmsUtl_freeArrayOfStrings.
 *  @param (OUT) an array of values.  This function will allocate an array
 *               of SETPV_MAX_NAME_VALUE_PAIRS char *, each char * pointing
 *               to another allocated buffer big enough to hold the value.
 *               Caller is responsible for freeing it using cmsUtl_freeArrayOfStrings.
 *
 *  @param (OUT) -1 on error, otherwise the number of fullpath/value pairs returned.
 */
static SINT32 getFullPathAndValuePairs(const char *cmdLine,
                                       char ***fullpathArray,
                                       char ***valueArray)
{
   char **fpArray;
   char **valArray;
   UINT32 fpIdx=0;
   UINT32 valIdx=0;
   UINT32 i=0;


   fpArray = cmsMem_alloc(SETPV_MAX_NAME_VALUE_PAIRS * sizeof(char *), ALLOC_ZEROIZE);
   if (fpArray == NULL)
   {
      bcmuLog_error("Could not allocate memory for fullpaths");
      return -1;
   }

   valArray = cmsMem_alloc(SETPV_MAX_NAME_VALUE_PAIRS * sizeof(char *), ALLOC_ZEROIZE);
   if (valArray == NULL)
   {
      bcmuLog_error("Could not allocate memory for char buf array");
      cmsUtl_freeArrayOfStrings(&fpArray, SETPV_MAX_NAME_VALUE_PAIRS);
      return -1;
   }

   // skip over any leading white spaces
   while (*cmdLine == ' ') { cmdLine++; }

   /* this is the main parsing body */
   while (cmdLine[i] != '\0')
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN];
      char valueBuf[4096];
      UINT32 j;

      /* first get the parameterName fullpath */
      memset(fullPathBuf, 0, sizeof(fullPathBuf));
      j = 0;
      while (cmdLine[i] != '\0' && cmdLine[i] != ' ')
      {
         fullPathBuf[j++] = cmdLine[i++];
      }

      // fpArray gets its own copy of the string buf.
      fpArray[fpIdx] = cmsMem_strdup(fullPathBuf);
      fpIdx++;

      // skip over any white spaces after the paramName
      while (cmdLine[i] == ' ') { i++; }

      /* now get the value.  A NULL value is represented by "",
       * a value which has a space must be enclosed by quotes, e.g. "hello there".
       */
      memset(valueBuf, 0, sizeof(valueBuf));
      j = 0;

      if ((cmdLine[i] == '"') && (cmdLine[i+1] == '"'))
      {
         /* special case: convert "" to empty string */
         valueBuf[0] = '\0';
         i += 2;
      }
      else if (cmdLine[i] == '"')
      {
         // special case: value surrounded by "" 
         // However, this logic does not handle the (rare) case where a value
         // begins with double quotes, or conains double quotes AND spaces.
         // TODO: add processing for escape character to fix rare case above.
         i++;
         while (cmdLine[i] != '\0' && cmdLine[i] != '"' && i < sizeof(valueBuf)-1)
         {
            valueBuf[j++] = cmdLine[i++];
         }
         i++;
      }
      else
      {
         while (cmdLine[i] != '\0' && cmdLine[i] != ' ' && i < sizeof(valueBuf)-1)
         {
            valueBuf[j++] = cmdLine[i++];
         }
      }

      // valArray gets its own copy of the string buf.
      valArray[valIdx] = cmsMem_strdup(valueBuf);
      valIdx++;

      // skip over any white spaces after the value
      while (cmdLine[i] == ' ') { i++; }
   }

   if (fpIdx != valIdx)
   {
      printf("Mis-match between number of parameter names (%d) and values (%d)\n", fpIdx, valIdx);
      cmsUtl_freeArrayOfStrings(&fpArray, SETPV_MAX_NAME_VALUE_PAIRS);
      cmsUtl_freeArrayOfStrings(&valArray, SETPV_MAX_NAME_VALUE_PAIRS);
      return -1;
   }

   if (fpIdx == 0)
   {
      printf("No fullpath or value detected\n");
      cmsUtl_freeArrayOfStrings(&fpArray, SETPV_MAX_NAME_VALUE_PAIRS);
      cmsUtl_freeArrayOfStrings(&valArray, SETPV_MAX_NAME_VALUE_PAIRS);
      return -1;
   }

   /* good return */
   *fullpathArray = fpArray;
   *valueArray = valArray;
   return fpIdx;
}

// Get zero, 1, or more fullpaths separated by comma.  Returns the number of
// fullpaths returned in the fullpathArray.  Regardless of return value, the
// caller must call cmsUtl_freeArrayOfStrings() to free the fullpathArray.
// This function is similar to the getFullPathAndValuePairs above, except
// it does not grab values.
static UINT32 getFullpaths(const char *cmdLine,
                           char ***fullpathArray)
{
   char **fpArray;
   UINT32 fpIdx=0;
   UINT32 i=0;

   fpArray = cmsMem_alloc(SETPV_MAX_NAME_VALUE_PAIRS * sizeof(char *), ALLOC_ZEROIZE);
   if (fpArray == NULL)
   {
      bcmuLog_error("Could not allocate memory for fullpaths");
      return 0;
   }

   // skip over any leading white spaces
   while (*cmdLine == ' ') { cmdLine++; }

   /* this is the main parsing body */
   while ((cmdLine[i] != '\0') && (fpIdx < SETPV_MAX_NAME_VALUE_PAIRS))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN];
      UINT32 j;

      // get fullpath
      memset(fullPathBuf, 0, sizeof(fullPathBuf));
      j = 0;
      while (cmdLine[i] != '\0' && cmdLine[i] != ',')
      {
         fullPathBuf[j++] = cmdLine[i++];
      }

      // fpArray gets its own copy of the string buf.
      fpArray[fpIdx] = cmsMem_strdup(fullPathBuf);
      fpIdx++;

      // advance over the comma separator
      i++;
   }

   // give array back to caller, who is responsible for free.
   // Note we return an empy array if fpIdx == 0, so caller still has to free.
   *fullpathArray = fpArray;

   return fpIdx;
}


static void getFullPathAndBool(const char *cmdLine,
                               char *fullPathBuf, UINT32 bufLen,
                               UBOOL8 *nextLevelOnly)
{
   char nextLevelOnlyBuf[BUFLEN_16]={0};
   UINT32 i=0;
   UINT32 j=0;

   // skip over any leading white spaces
   while (*cmdLine == ' ') { cmdLine++; }

   /* copy fullpath(s) to user buf. */
   while (cmdLine[i] != '\0' && cmdLine[i] != ' ' && i < bufLen - 1)
   {
      fullPathBuf[j++] = cmdLine[i++];
   }

   if (cmdLine[i] == '\0' || nextLevelOnly == NULL)
   {
      return;
   }

   /* skip over space and get the nextLevelOnly value */
   i++;
   j=0;
   while (cmdLine[i] != '\0' && cmdLine[i] != ' ' && j < sizeof(nextLevelOnlyBuf)-1)
   {
      nextLevelOnlyBuf[j++] = cmdLine[i++];
   }

   if (nextLevelOnlyBuf[0] == '0' && nextLevelOnlyBuf[1] == '\0')
   {
      *nextLevelOnly = FALSE;
   }
   else if (nextLevelOnlyBuf[0] == '1' && nextLevelOnlyBuf[1] == '\0')
   {
      *nextLevelOnly = TRUE;
   }
   else
   {
      printf("Invalid nextLevelOnly value, must be 0 or 1\n");
   }

   return;
}


void processMdmCmd(char *cmdLine)
{

   // skip over any leading white spaces
   while (*cmdLine == ' ') { cmdLine++; }

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: mdm setpv <full path to parameter value> <param value> [plus %d more name/value pairs]\n",
                     SETPV_MAX_NAME_VALUE_PAIRS - 1);
      printf("       mdm getpv <comma separated list of param or obj fullpaths> <nextLevelOnly=(0|1>)\n");
      printf("       mdm getov <full path to object> <nextLevelOnly(0|1)>\n");
      printf("       mdm getpn <full path to parameter or partial path to object> <nextLevelOnly(0|1)>\n");
      printf("       mdm getpa <comma separated list of param or obj fullpaths> <nextLevelOnly(0|1)>\n");
      printf("       mdm setpa <full path> <setAccess(0|1)> accessBitMask <setNotification(0|1)> notification <setAltNotify(0|1)> altNotify\n");
      printf("                          values for notification: no_notif=0, passive_notif=0x1, active_notif=0x2, clear_single_notif=0x40\n");
      printf("       mdm getchangedparams\n");
      printf("       mdm clearchangedparams [optional list of comma separated fullpaths]\n");
      printf("       mdm addobj <full path to object>\n");
      printf("       mdm delobj <full path to object instance>\n");
      printf("       mdm setnonpersistent <full path to object instance>\n");
      return;
   }

   if (!strncasecmp(cmdLine, "setpv", 5))
   {
      char **fullpathArray = NULL;
      char **valueArray = NULL;
      BcmGenericParamInfo *paramInfoArray = NULL;
      SINT32 numFullpaths = 0;
      SINT32 numParamInfos = 0;
      SINT32 i, j;
      UBOOL8 found;
      CmsRet ret;

      numFullpaths = getFullPathAndValuePairs(&cmdLine[6], &fullpathArray, &valueArray);
      if (numFullpaths <= 0)
      {
         return;
      }

      // First do a getParameterValues to get the param types
      ret = bcmGeneric_getParameterValuesFlags((const char **)fullpathArray, numFullpaths,
                                               TRUE, 0,
                                               &paramInfoArray, &numParamInfos);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Could not get paramInfos, ret=%d\n", ret);
         if (paramInfoArray != NULL)
         {
            // if we got a paramInfoArray back, get error on specific param.
            for (i=0; i < numParamInfos; i++)
            {
               if (paramInfoArray[i].errorCode != CMSRET_SUCCESS)
               {
                  printf("Error detected at [%d] %s, errorCode=%d\n", i,
                         paramInfoArray[i].fullpath, paramInfoArray[i].errorCode);
               }
            }
         }

         cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
         cmsUtl_freeArrayOfStrings(&valueArray, SETPV_MAX_NAME_VALUE_PAIRS);
         cmsUtl_freeArrayOfStrings(&fullpathArray, SETPV_MAX_NAME_VALUE_PAIRS);
         return;
      }

      // The number of input fullpaths should equal the numParamInfos.
      // This is a set operation, so all fullpaths should be params, no walking
      // over sub-trees.
      if (numFullpaths != numParamInfos)
      {
         printf("Mismatch in num params, fullpaths=%d numParamInfos=%d\n",
                numFullpaths, numParamInfos);
         for (i=0; i < numFullpaths; i++)
         {
             printf("fullpath[%d] %s\n", i, fullpathArray[i]);
         }
         for (i=0; i < numParamInfos; i++)
         {
             printf("paramInfo[%d] %s\n", i, paramInfoArray[i].fullpath);
         }

         cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
         cmsUtl_freeArrayOfStrings(&valueArray, SETPV_MAX_NAME_VALUE_PAIRS);
         cmsUtl_freeArrayOfStrings(&fullpathArray, SETPV_MAX_NAME_VALUE_PAIRS);
         return;
      }

      // We can re-use the paramInfoArray for the set operation.  The param
      // type has been filled in by getParameterValues.  Just need
      // to set the new value.  But the ordering of the fullpaths may
      // have changed, so need to match up the fullpaths one by one.
      for (i=0; i < numParamInfos; i++)
      {
         found = FALSE;
         for (j=0; j < numFullpaths && !found; j++)
         {
            if (!cmsUtl_strcmp(paramInfoArray[i].fullpath, fullpathArray[j]))
            {
               char *prevValue;
               // Existing output shows the prev value, so save the prev value
               // in the valueArray, and put the new value in the
               // paramInfoArray.  We are just swapping ownership of
               // string buffers, so no malloc or free needed here.
               prevValue = paramInfoArray[i].value;
               paramInfoArray[i].value = valueArray[j];
               valueArray[j] = prevValue;
               found = TRUE;
            }
         }
         if (!found)
         {
            printf("Could not find [%d]%s in input fullpath\n",
                   i, paramInfoArray[i].fullpath);
            cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
            cmsUtl_freeArrayOfStrings(&valueArray, SETPV_MAX_NAME_VALUE_PAIRS);
            cmsUtl_freeArrayOfStrings(&fullpathArray, SETPV_MAX_NAME_VALUE_PAIRS);
            return;
         }
      }

      // Now we can do the set
      ret = bcmGeneric_setParameterValuesFlags(paramInfoArray, numParamInfos, 0);
      if (ret != CMSRET_SUCCESS)
      {
         printf("setparametervalues failed, ret=%d\n", ret);
         // dump all params to get specific error info
         for (i=0; i < numParamInfos; i++)
         {
            printf("[%d] %s (type=%s) value=%s, errorCode=%d\n", i,
                   paramInfoArray[i].fullpath,
                   paramInfoArray[i].type,
                   paramInfoArray[i].value,
                   paramInfoArray[i].errorCode);
          }
      }
      else
      {
         for (i=0; i < numParamInfos; i++)
         {
            printf("[%2d] set %s to %s\n", i, paramInfoArray[i].fullpath, paramInfoArray[i].value);
            printf("        (Type=%s) Previous value=%s\n\n", paramInfoArray[i].type, valueArray[i]);
         }

         cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
         cmsUtl_freeArrayOfStrings(&valueArray, SETPV_MAX_NAME_VALUE_PAIRS);
         cmsUtl_freeArrayOfStrings(&fullpathArray, SETPV_MAX_NAME_VALUE_PAIRS);
      }
   }
   else if (!strncasecmp(cmdLine, "getpv", 5))
   {
      /* fill CMS PHL structure and use CMS PHL API to send into MDM */
      {
         char fullPathBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};
         UBOOL8 nextLevelOnly=TRUE;
         UINT32 getFlags=0; // could set OGF_NO_VALUE_UPDATE for testing
         CmsRet ret;
         char *savePtr=NULL;
         char *path=NULL;
         UINT32 idx=0;

         // Get the fullpaths and optional nextLevelOnly arg
         getFullPathAndBool(&cmdLine[6],
                            fullPathBuf, sizeof(fullPathBuf), &nextLevelOnly);

         if (fullPathBuf[0] == '\0')
         {
            printf("a fullpath must be specified.\n");
            return;
         }

#ifdef USE_OLD_GETPARAMVALUE_API
         {
         MdmPathDescriptor pathDescArray[30];  // 30 should be enough
         UINT32 numPathDesc = sizeof(pathDescArray)/sizeof(MdmPathDescriptor);
         PhlGetParamValue_t *getParamValueList=NULL;
         SINT32 numGetParamValues=0;

         memset(pathDescArray, 0, sizeof(pathDescArray));

         // grab the paths from comma separated list
         path = strtok_r(fullPathBuf, ",", &savePtr);
         // printf("idx[%d]=%s\n", idx, path);
         ret = cmsMdm_fullPathToPathDescriptor(path, &(pathDescArray[idx]));
         if (ret != CMSRET_SUCCESS)
         {
            printf("Invalid fullpath %s\n", path);
            return;
         }
         idx++;
         while ((path = strtok_r(NULL, ",", &savePtr)) != NULL &&
                (idx < numPathDesc))
         {
            // printf("idx[%d]=%s\n", idx, path);
            ret = cmsMdm_fullPathToPathDescriptor(path, &(pathDescArray[idx]));
            if (ret != CMSRET_SUCCESS)
            {
               printf("Invalid fullpath %s\n", path);
               return;
            }
            idx++;
         }

         ret = cmsPhl_getParameterValuesFlags(pathDescArray, idx,
                                   nextLevelOnly, getFlags,
                                   &getParamValueList, &numGetParamValues);
         if (ret != CMSRET_SUCCESS)
         {
            printf("Could not get param value, ret=%d\n", ret);
            return;
         }

         printf("\n\nSent in %d paths, got back %d results:\n",
                idx, numGetParamValues);
         for (idx=0; (SINT32 )idx < numGetParamValues; idx++)
         {
            path = NULL;
            cmsMdm_pathDescriptorToFullPath(&(getParamValueList[idx].pathDesc), &path);
            printf("[%d] %s\n", idx, path);
            CMSMEM_FREE_BUF_AND_NULL_PTR(path);
            printf("    Value=%s\n", getParamValueList[idx].pValue);
            printf("    Type=%s\n", getParamValueList[idx].pParamType);
         }
         printf("\n");

         cmsPhl_freeGetParamValueBuf(getParamValueList, numGetParamValues);
         }
#else
         // Demonstrate new bcmGeneric API for getting param values, which can
         // handle remote fullpaths that are not in our data model.
         {
         char *fullpathArray[30];            // TODO: 30 should be enough
         UINT32 numFullpaths = sizeof(fullpathArray) / sizeof(char *);
         BcmGenericParamInfo *paramInfoList=NULL;
         SINT32 numParamInfos=0;

         // grab the paths from comma separated list
         path = strtok_r(fullPathBuf, ",", &savePtr);
         // printf("idx[%d]=%s\n", idx, path);
         fullpathArray[idx] = path;
         idx++;
         while ((path = strtok_r(NULL, ",", &savePtr)) != NULL &&
                (idx < numFullpaths))
         {
            // printf("idx[%d]=%s\n", idx, path);
            fullpathArray[idx] = path;
            idx++;
         }

         ret = bcmGeneric_getParameterValuesFlags((const char **)fullpathArray, idx,
                                   nextLevelOnly, getFlags,
                                   &paramInfoList, &numParamInfos);
         if (ret != CMSRET_SUCCESS)
         {
            printf("Could not get param value, ret=%d\n", ret);
            return;
         }

         printf("\n\nSent in %d paths, got back %d generic results:\n",
                idx, numParamInfos);
         for (idx=0; (SINT32 )idx < numParamInfos; idx++)
         {
            printf("[%d] %s\n", idx, paramInfoList[idx].fullpath);
            printf("    Value=%s\n", paramInfoList[idx].value);
            printf("    Type=%s\n", paramInfoList[idx].type);
            printf("    Profile=%s isPassword=%d writable=%d\n",
                   paramInfoList[idx].profile,
                   paramInfoList[idx].isPassword,
                   paramInfoList[idx].writable);
         }
         printf("\n");

         cmsUtl_freeParamInfoArray(&paramInfoList, numParamInfos);
         }
#endif  /* USE_OLD_GETPARAMVALUE_API */
      }
   }
   else if (!strncasecmp(cmdLine, "getov", 5))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      CmsRet ret = CMSRET_SUCCESS;
      MdmPathDescriptor pathDesc;
      UBOOL8 nextLevelOnly = FALSE;
      SINT32 numGetParamValues = 0;
      SINT32 idxParam = 0;
      PhlGetParamValue_t *getParamValueList = NULL;

      getFullPathAndBool(&cmdLine[6],
                         fullPathBuf, sizeof(fullPathBuf), &nextLevelOnly);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      INIT_PATH_DESCRIPTOR(&pathDesc);

      ret = cmsMdm_fullPathToPathDescriptor(fullPathBuf, &pathDesc);
      if (ret == CMSRET_SUCCESS)
      {
         /*iterate on all requested elements to get each sub-parameter value*/
         ret = cmsPhl_getParameterValues(&pathDesc, 1, nextLevelOnly,
                                         &getParamValueList, &numGetParamValues);
         if (ret != CMSRET_SUCCESS)
         {
            fprintf( stderr, "Could not get all parameters value, ret=%d\n", ret);
            return;
         }
      }
      else
      {
         fprintf( stderr, "Invalid fullpath %s\n", fullPathBuf);
         return;
      }

      printf("numGetParamValues: %d\n\n", numGetParamValues);

      for (idxParam = 0; idxParam < numGetParamValues; idxParam++)
      {
         if (strlen(getParamValueList[idxParam].pathDesc.paramName) > 0)
            printf("<Name>%s</Name>  ", getParamValueList[idxParam].pathDesc.paramName);
         else
            printf("<Name>NULL</Name>  ");
         if (getParamValueList[idxParam].pParamType != NULL)
            printf("<Type>%s</Type>  ", getParamValueList[idxParam].pParamType);
         else
            printf("<Type>NULL</Type>  ");
         if (getParamValueList[idxParam].pValue != NULL)
            printf("<Value>%s</Value>\n", getParamValueList[idxParam].pValue);
         else
            printf("<Value>NULL</Value>\n");
      }

      cmsPhl_freeGetParamValueBuf(getParamValueList, numGetParamValues);
   }
   else if (!strncasecmp(cmdLine, "getpn", 5))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      CmsRet ret = CMSRET_SUCCESS;
      UBOOL8 nextLevelOnly = FALSE;
      CmsTimestamp beginTs, endTs;
      SINT32 numParamInfos = 0;
      SINT32 idxParam = 0;
      BcmGenericParamInfo *paramInfo = NULL;
      BcmGenericParamInfo *paramInfoArray = NULL;

      getFullPathAndBool(&cmdLine[6],
                         fullPathBuf, sizeof(fullPathBuf), &nextLevelOnly);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      cmsTms_get(&beginTs);
      ret = bcmGeneric_getParameterNamesFlags(fullPathBuf, nextLevelOnly, 0,
                                       &paramInfoArray, &numParamInfos);
      if (ret != CMSRET_SUCCESS)
      {
         fprintf(stderr, "Could not get parameter names for %s (nextLevelOnly=%d), ret=%d\n",
                 fullPathBuf, nextLevelOnly, ret);
         return;
      }
      cmsTms_get(&endTs);

      printf("got %d param names in %d ms\n\n",
             numParamInfos,
             cmsTms_deltaInMilliSeconds(&endTs, &beginTs));

      for (idxParam = 0, paramInfo = paramInfoArray;
           idxParam < numParamInfos;
           idxParam++, paramInfo++)
      {
         printf("[%04d] %s (type=%s profile=%s writable=%d isPassword=%d)\n",
                idxParam, paramInfo->fullpath, paramInfo->type, paramInfo->profile,
                paramInfo->writable, paramInfo->isPassword);
      }

      cmsUtl_freeParamInfoArray(&paramInfoArray, numParamInfos);
   }
   else if (!strncasecmp(cmdLine, "getpa", 5))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      UBOOL8 nextLevelOnly = FALSE;
      char *fullpathArray[30];            // TODO: 30 should be enough
      UINT32 numFullpaths = sizeof(fullpathArray) / sizeof(char *);
      CmsRet ret = CMSRET_SUCCESS;
      char *savePtr=NULL;
      char *path=NULL;
      UINT32 idx = 0;
      BcmGenericParamAttr *paramAttrArray = NULL;
      SINT32            numParamAttrs = 0;

      getFullPathAndBool(&cmdLine[6],
                         fullPathBuf, sizeof(fullPathBuf), &nextLevelOnly);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      // grab the paths from comma separated list
      path = strtok_r(fullPathBuf, ",", &savePtr);
      // printf("idx[%d]=%s\n", idx, path);
      fullpathArray[idx] = path;
      idx++;
      while ((path = strtok_r(NULL, ",", &savePtr)) != NULL &&
             (idx < numFullpaths))
      {
         // printf("idx[%d]=%s\n", idx, path);
         fullpathArray[idx] = path;
         idx++;
      }

      ret = bcmGeneric_getParameterAttributesFlags((const char **)fullpathArray, idx,
                                          nextLevelOnly, 0,
                                          &paramAttrArray, &numParamAttrs);
      if (ret != CMSRET_SUCCESS)
      {
         fprintf( stderr, "Failed to get parameter attrs, ret=%d\n", ret);
         return;
      }

      printf("\n\nSent in %d paths, got back %d generic results:\n\n",
             idx, numParamAttrs);

      for (idx = 0; (SINT32)idx < numParamAttrs; idx++)
      {
         printf("<Name>%s</Name> \n", paramAttrArray[idx].fullpath);
         printf("<access>0x%x</access> \n", paramAttrArray[idx].access);
         printf("<notif>0x%x</notif> \n", paramAttrArray[idx].notif);
         printf("<valueChanged>%d</valueChanged>\n", paramAttrArray[idx].valueChanged);
         printf("<alt_notif>0x%x</alt_notif> \n", paramAttrArray[idx].altNotif);
      }

      cmsUtl_freeParamAttrArray(&paramAttrArray, numParamAttrs);
   }
   else if (!strncasecmp(cmdLine, "setpa", 5))
   {
      char buf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      CmsRet ret = CMSRET_SUCCESS;
      BcmGenericParamAttr paramAttr;
      UINT32 i=0;
      UINT32 j=6; // start of args, &cmdLine[6]

      memset(&paramAttr, 0, sizeof(BcmGenericParamAttr));

      // the expected format of args are:
      // <fullpath> <0|1> accessBitMask <0|1> notification <0|1> altNotification
      // First copy args to our local buffer.
      while (cmdLine[j] != '\0' && i < sizeof(buf)-1)
      {
         buf[i++] = cmdLine[j++];
      }

      // advance past the fullpath and terminate
      i=0;
      while (buf[i] != ' ' && buf[i] != '\0')
         i++;
      buf[i] = '\0';

      paramAttr.fullpath = cmsMem_strdup(buf);

      // now get the 0|1 for the setAccess and the accessBitMask
      i++;
      paramAttr.setAccess = ((buf[i] == '1') ? 1 : 0);
      i += 2;  // advance past 1|0 and space
      j = i;
      while (buf[j] != ' ' && j < sizeof(buf)-1)
         j++;
      buf[j] = '\0';
      paramAttr.access = strtoul(&(buf[i]), NULL, 0);

      // now get the 0|1 for the setNotification and notification
      i = ++j;
      if(i >= sizeof(buf))
        i = sizeof(buf)-1;

      paramAttr.setNotif = ((buf[i] == '1') ? 1 : 0);
      i += 2;  // advance past 1|0 and space
      if(i >= sizeof(buf))
         i = sizeof(buf)-1;
      j = i;
      while (buf[j] != ' ' && j < sizeof(buf)-1)
         j++;
      buf[j] = '\0';
      paramAttr.notif = strtoul(&(buf[i]), NULL, 0);

      // now get the 0|1 for the setAltNotification
      i = ++j;
      if(i >= sizeof(buf))
         i = sizeof(buf)-1;
      paramAttr.setAltNotif = ((buf[i] == '1') ? 1 : 0);
      i += 2;  // advance past 1|0 and space
      if(i >= sizeof(buf))
         i = sizeof(buf)-1;
      j = i;
      while (buf[j] != ' ' && j < sizeof(buf)-1)
         j++;

      if(j >= sizeof(buf))
         j = sizeof(buf)-1;

      buf[j] = '\0';
      paramAttr.altNotif = strtoul(&(buf[i]), NULL, 0);

      fprintf( stderr, "setting attrs for %s:\n", paramAttr.fullpath);
      fprintf( stderr, "access  %d 0x%x\n",
                       paramAttr.setAccess,
                       paramAttr.access);
      fprintf( stderr, "notif  %d 0x%x\n",
                       paramAttr.setNotif,
                       paramAttr.notif);

      fprintf( stderr, "altNotif  %d 0x%x\n",
                       paramAttr.setAltNotif,
                       paramAttr.altNotif);

      // Even though this function can take multiple setParamAttrs,
      // we only pass in 1.
      ret = bcmGeneric_setParameterAttributesFlags(&paramAttr, 1, 0);

      CMSMEM_FREE_BUF_AND_NULL_PTR(paramAttr.fullpath);

      if (ret != CMSRET_SUCCESS)
      {
         fprintf( stderr, "Failed to set parameter attrs, ret=%d\n", ret);
         return;
      }
      else
      {
         fprintf( stderr, "setParamAttr was successful.\n");
      }
   }
   else if (!strncasecmp(cmdLine, "getchangedparams", 16))
   {
      char **array_of_params = NULL;
      UINT32 numParams = 0;
      UINT32 i;
      CmsRet ret;

      ret = cmsPhl_getChangedParams(&array_of_params, &numParams);
      if (ret == CMSRET_SUCCESS)
      {
         printf("Successful call, numParams=%d\n", numParams);
         for (i=0; i < numParams; i++)
            printf("[%02d] %s\n", i, array_of_params[i]);

         cmsUtl_freeArrayOfStrings(&array_of_params, numParams);
      }
      else
      {
         printf("cmsPhl_getChangedParams failed, ret=%d\n", ret);
      }
      printf("\n\n");
   }
   else if (!strncasecmp(cmdLine, "clearchangedparams", 18))
   {
      char **array_of_fullpaths = NULL;
      UINT32 numFps;

      numFps = getFullpaths(&(cmdLine[18]), &array_of_fullpaths);
      if (numFps > 0)
      {
         // clear notification on individual fullpaths.  Note this clears
         // the presence of a notification, not the notification setting itself,
         // i.e. the passive or active notification setting on the param is still there.
         BcmGenericParamAttr paramAttr[SETPV_MAX_NAME_VALUE_PAIRS];
         CmsRet ret;
         UINT32 i=0;

         memset(paramAttr, 0, sizeof(paramAttr));
         for (i=0; (i < numFps) && (i < SETPV_MAX_NAME_VALUE_PAIRS); i++)
         {
            printf("[%d] clearing notification on %s\n", i, array_of_fullpaths[i]);
            paramAttr[i].fullpath = array_of_fullpaths[i];
            paramAttr[i].setNotif = 1;
            paramAttr[i].notif = GENATTR_CLEAR_NOTIFICATION;
         }

         ret = bcmGeneric_setParameterAttributesFlags(paramAttr, i, 0);
         printf("clearing single params, ret=%d\n", ret);
      }
      else
      {
         cmsPhl_clearAllParamValueChanges();
         printf("Cleared ALL param value changes (no return code)\n\n");
      }

      // In all cases, free the array, the function can handle null input.
      cmsUtl_freeArrayOfStrings(&array_of_fullpaths, numFps);
   }
   else if (!strncasecmp(cmdLine, "addobj", 6))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      UINT32 instanceId=0;
      CmsRet ret;

      getFullPathAndBool(&cmdLine[7], fullPathBuf, sizeof(fullPathBuf), NULL);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      printf("Adding object %s\n", fullPathBuf);

      ret = cmsPhl_addObjInstanceByFullPathFlags(fullPathBuf, 0, &instanceId);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Add object failed, ret=%d\n", ret);
      }
      else
      {
         printf("Added new obj instance at %d\n", instanceId);
      }
   }
   else if (!strncasecmp(cmdLine, "delobj", 6))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      CmsRet ret;

      getFullPathAndBool(&cmdLine[7], fullPathBuf, sizeof(fullPathBuf), NULL);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      printf("deleting object %s\n", fullPathBuf);

      ret = cmsPhl_delObjInstanceByFullPathFlags(fullPathBuf, 0);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Del object failed, ret=%d\n", ret);
      }
      else
      {
         printf("Deleted obj instance at %s\n", fullPathBuf);
      }
   }
   else if (!strncasecmp(cmdLine, "setnonpersistent", 16))
   {
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      MdmPathDescriptor pathDesc;
      CmsRet ret;

      getFullPathAndBool(&cmdLine[17], fullPathBuf, sizeof(fullPathBuf), NULL);

      if (fullPathBuf[0] == '\0')
      {
         printf("a fullpath must be specified.\n");
         return;
      }

      memset(&pathDesc, 0, sizeof(pathDesc));
      ret = cmsMdm_fullPathToPathDescriptor(fullPathBuf, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Invalid fullpath %s\n", fullPathBuf);
         return;
      }

      if (pathDesc.paramName[0] != '\0')
      {
         printf("The fullpath must specify an object (not parameter)\n");
         return;
      }

      printf("Setting NON-PERSISTENT on oid %d iidStack=%s\n", pathDesc.oid, cmsMdm_dumpIidStack(&pathDesc.iidStack));

      ret = cmsObj_setNonpersistentInstance(pathDesc.oid, &pathDesc.iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         printf("Set NON-PERSISTENT failed, ret=%d\n", ret);
      }
      else
      {
         printf("Marked object at %s as NON-PERSISTENT\n", fullPathBuf);
      }
   }
   else
   {
      printf("No sub-command detected, type mdm -h for usage\n");
   }

}


// usage: isLocalFullpath [-v] fullpath
// Here are some interesting/important fullpaths to test.  Eventually, this
// should be put into a unittest, but for now, just list them here:
// (the same paths can be used in isPossibleRemoteFullpath below, and can
// be tried in different components, but mainly from tr69 and sysmgmt).
//
// Device.  (multi-comp obj)
// Device.DSL.
// Device.DSL.Line.1   (ends with instance id with no end dot)
// Device.DSL.Line.1.  (ends with instance id with end dot)
// Device.DSL.Line.1.Status (valid param name)
// Device.DSL.Line.1.blahxx (invalid param name)
// Device.DSL.Line.1.Stats  (looks like a param name but is an object)
//
// Device.QoS.Queue.          (multi-comp, local and remote)
// Device.QoS.Queue.1         (if running from sysmgmt, this is a local queue)
// Device.QoS.Queue.1.Status
// Device.QoS.Queue.1.blahxx  (if running from sysmgmt, this is a local queue even though the param name is bad)
// Device.QoS.Queue.800001  (this should be a remote queue based on instance id range)
// Device.QoS.Queue.800001.Status
// Device.UnknownObj.5.paramorobj (unknown object with possible param name or obj name)
//
// Device.QoS.Queue.[dsl-cpq-q2]  (alias hint specifying DSL, no end dot)
// Device.QoS.Queue.[dsl-cpq-q2]. (alias hint specifying DSL, with end dot)
// Device.QoS.Queue.[blah]        (alias present, but unknowncomponent name, is local and remote)
//
// Device.QoS.Queue.*  (wildcards are detected, but no actual processing, both local and remote)
// Device.QoS.Queue.*. (wildcards with end dot)

void processIsLocalFullpathCmd(char *cmdLine)
{
   UBOOL8 isLocal;
   UINT32 i=0;

   if (!strncmp(cmdLine, "-v", 2))
   {
      i = 3;  // advance past -v and point to fullpath
      bcmuLog_setLevel(BCMULOG_LEVEL_DEBUG);
   }

   isLocal = mdm_isLocalFullpath(&(cmdLine[i]));
   printf("%s ==> isLocalFullpath=%d\n\n",
          &(cmdLine[i]), isLocal);

   if (i != 0)
   {
      bcmuLog_setLevel(BCMULOG_LEVEL_ERR);
   }

   return;
}

// usage: isPossibleRemoteFullpath [-v] fullpath
void processIsPossibleRemoteFullpathCmd(char *cmdLine)
{
   UBOOL8 isRemote;
   UINT32 i=0;

   if (!strncmp(cmdLine, "-v", 2))
   {
      i = 3;  // advance past -v and point to fullpath
      bcmuLog_setLevel(BCMULOG_LEVEL_DEBUG);
   }


   isRemote = mdm_isPossibleRemoteFullpath(&(cmdLine[i]));
   printf("%s ==> isPossibleRemoteFullpath=%d\n\n",
          &(cmdLine[i]), isRemote);

   if (i != 0)
   {
      bcmuLog_setLevel(BCMULOG_LEVEL_ERR);
   }

   return;
}


void processDumpRemoteObjStatCmd(char *cmdLine __attribute__((unused)))
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   if (!cmsMdm_isRemoteCapable())
   {
      printf("This component does not have a remote_objd\n");
      return;
   }

   msg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg.dst = EID_REMOTE_OBJD;
   msg.type = CMS_MSG_REMOTE_OBJ_DUMP_STATS;  /* default op */
   msg.flags_request = 1;
   msg.dataLength = 0;

   if (!strcasecmp(cmdLine, "help") || !strcasecmp(cmdLine, "-h") || !strcasecmp(cmdLine, "--help"))
   {
      printf("usage: remoteobjstats [clear]\n");
      printf("by default, dump statistics of remote_objd.\n");
      printf("clear: clear all statistics data.\n");
      return;
   }
   else if (!strncasecmp(cmdLine, "clear", 5))
   {
       msg.wordData = 1;
   }

   if (cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, &msg, 1000) != CMSRET_SUCCESS)
   {
       bcmuLog_error("could not get dump robj stats!");
   }

   return;
}

void processRemoteObjNsLookupCmd(char *cmdLine)
{
   char msgBuf[1024] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;

   if (!cmsMdm_isRemoteCapable())
   {
      printf("This component does not have a remote_objd\n");
      return;
   }

   if (!strcasecmp(cmdLine, "help") ||
       !strcasecmp(cmdLine, "-h") ||
       !strcasecmp(cmdLine, "--help") ||
       (cmdLine[0] == '\0'))
   {
      printf("usage: remoteobjnslookup fullpath\n");
      return;
   }

   if (strlen(cmdLine)+1 > sizeof(msgBuf)-sizeof(CmsMsgHeader))
   {
      printf("fullpath %s too long!", cmdLine);
      return;
   }

   msgHdr->src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msgHdr->dst = EID_REMOTE_OBJD;
   msgHdr->type = CMS_MSG_REMOTE_OBJ_NS_LOOKUP;
   msgHdr->flags_request = 1;
   msgHdr->dataLength = strlen(cmdLine)+1;
   snprintf((char *)(msgHdr+1), sizeof(msgBuf)-sizeof(CmsMsgHeader), "%s", cmdLine);

   if (cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, msgHdr, 1000) != CMSRET_SUCCESS)
   {
       bcmuLog_error("error during sendAndGetReply!");
   }

   return;
}

