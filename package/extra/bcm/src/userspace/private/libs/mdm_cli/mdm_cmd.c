/***********************************************************************
 *
 *
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom
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


/*!\file mdm_cmd.c
 * \brief Top level command processor.
 *
 */

#include <errno.h>
#include <sys/time.h>
#include "number_defs.h"
#include "os_defs.h"
#include "cms_mem.h"
#include "cms_fil.h"
#include "cms_msg.h"
#include "mdm_cli.h"
#include "bcm_ulog.h"
#include "framework_cli.h"



// Legacy variables from libcms_cli.so
void *cliPrvtMsgHandle=NULL;


// This table contains commands related to the MDM.
// Register this table with the underlying CLI framework by calling
// mdmCli_registerMdmCmds().
static const CLI_CMD_ITEM cliMdmCmdTable[] = {
   { "save", "Save MDM config to flash",        processSaveCmd },
   { "invalidatecfg", "Invalidate config file", processInvalidateCfgCmd },
   { "dumpcfg", "inspect config info",          processDumpCfgCmd },
   { "dumpmdm", "inspect MDM",                  processDumpMdmCmd },
   { "meminfo", "various memory info",          processMeminfoCmd },
   { "shminfo", "shared memory info",           processShmInfoCmd },
   { "mdm", "various MDM debug operations",     processMdmCmd },
   { "dumplocks", "dump all MDM lock info",     processDumpLocksCmd },
   { "tracelocks", "show MDM lock activity",    processTraceLocksCmd },
   { "dumpoid", "dump MDM object id info",      processDumpOidCmd },
   { "dumpMsgConn", "dump bcm_msgd connection info",  processDumpMsgConnCmd },
   { "islocalFullpath", "test fullpath for local",  processIsLocalFullpathCmd },
   { "isPossibleRemoteFullpath", "test fullpath for possible remote", processIsPossibleRemoteFullpathCmd },
   { "remoteobjstats", "dump remote_objd stats", processDumpRemoteObjStatCmd},
   { "remoteobjnslookup", "do a namespace lookup on given fullpath", processRemoteObjNsLookupCmd},
   { "loglevel", "loglevel set",                processLogLevelCmd },
};


void mdmCli_registerMdmCmds()
{
   // Register the set of MDM CLI cmds with the underlying CLI framework.
   frameworkCli_registerAppCmds(cliMdmCmdTable,
                                sizeof(cliMdmCmdTable)/sizeof(CLI_CMD_ITEM));

}

void mdmCli_registerAppCmds(const CLI_CMD_ITEM *appCmdTable, UINT32 numOfEntries)
{
   frameworkCli_registerAppCmds(appCmdTable, numOfEntries);
   return;
}

void mdmCli_processCliInput()
{
   frameworkCli_processCliInput();
   return;
}

int mdmCli_keepLooping(void)
{
   return (frameworkCli_keepLooping());
}

void mdmCli_setKeepLooping(int v)
{
   frameworkCli_setKeepLooping(v);
   return;
}

int mdmCli_getExitOnIdleTimeout(void)
{
   return (frameworkCli_getExitOnIdleTimeout());
}

void mdmCli_setExitOnIdleTimeout(int secs)
{
   frameworkCli_setExitOnIdleTimeout(secs);
   return;
}

void mdmCli_setPrompt(const char *prompt)
{
   frameworkCli_setPrompt(prompt);
   return;
}

void mdmCli_msgHandlerFunc(void)
{
   CmsMsgHeader *msg = NULL;
   CmsRet ret;

   /* we got a message on the comm fd, read it */
   ret = cmsMsg_receive(cliPrvtMsgHandle, &msg);
   if (ret == CMSRET_SUCCESS)
   {
      // We should not receive any CMS messages here, log it and free it.
      bcmuLog_error("unsupported msg type 0x%x", msg->type);
      CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   }
   else if (ret == CMSRET_DISCONNECTED)
   {
      if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
      {
         bcmuLog_error("lost connection to message bus, exit now.");
      }
      frameworkCli_setKeepLooping(0);
   }
   else
   {
      bcmuLog_error("error during receive, ret=%d", ret);
   }

   return;
}
