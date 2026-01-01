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

#ifndef __MDM_CLI_H__
#define __MDM_CLI_H__

/*!\file mdm_cli.h
 * \brief Header file for libmdm_cli.so.  Users of this library (mainly
 *  cms_cli should include this header file.  See framework_cli.h for 
 *  additional documentation.
 *
 */

#include <unistd.h>
#include "number_defs.h"
#include "os_defs.h"
#include "bcm_retcodes.h"
#include "framework_cli.h"



// Apps which have a CMS msgHandle can share it with the CLI framework by
// setting this var.  See also frameworkCli_setExtraFd().
extern void *cliPrvtMsgHandle;

/** this is the handler func for events on the cliPrvtMsgHandle
 */
void mdmCli_msgHandlerFunc(void);


/** Tell the MDM CLI lib to register its commands with the underlying CLI framework lib.
 */
void mdmCli_registerMdmCmds(void);


/** Register application-specific CLI commands.
 *
 * @param appCmdTable (IN) Application-specific CLI command table.
 * @param numOfEntries (IN) number of entries in the CLI command table.
 *
 * @return None.
 */
void mdmCli_registerAppCmds(const CLI_CMD_ITEM *appCmdTable, UINT32 numOfEntries);


// This is the main entry point to read and process a command.
void mdmCli_processCliInput();


int mdmCli_keepLooping(void);
void mdmCli_setKeepLooping(int v);

int mdmCli_getExitOnIdleTimeout(void);
void mdmCli_setExitOnIdleTimeout(int secs);


/** Set the prompt to be used by CLI.
 *
 * @param prompt (IN) The prompt, which is copied to internal storage.
 *
 * @return None.
 */
void mdmCli_setPrompt(const char *prompt);


// in mdm_cmddbug.c
void processSaveCmd(char *cmdLine);
void processInvalidateCfgCmd(char *cmdLine);
void processDumpCfgCmd(char *cmdLine);
void processDumpMdmCmd(char *cmdLine);
void processMeminfoCmd(char *cmdLine);
void processMdmCmd(char *cmdLine);
void processTraceLocksCmd(char *cmdLine);
void processDumpLocksCmd(char *cmdLine);
void processDumpOidCmd(char *cmdLine);
void processDumpMsgConnCmd(char *cmdLine);
void processShmInfoCmd(char *cmdLine);
void processIsLocalFullpathCmd(char *cmdLine);
void processIsPossibleRemoteFullpathCmd(char *cmdLine);
void processDumpRemoteObjStatCmd(char *cmdLine);
void processRemoteObjNsLookupCmd(char *cmdLine);
void processLogLevelCmd(char *cmdLine);


#endif /* __MDM_CLI_H__ */
