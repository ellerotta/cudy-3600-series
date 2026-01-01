/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2023:DUAL/GPL:standard
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 *
 ************************************************************************/

#ifndef __FRAMEWORK_CLI_H__
#define __FRAMEWORK_CLI_H__

/*!\file framework_cli.h
 * \brief Header file for the CLI framwork used by mdm_cli, cms_cli, and consoled.
 *
 * To use this CLI framework, register your specific commands via
 * frameworkCli_registerAppCmds().  Then, run this loop:
 *
 *   while (frameworkCli_keepLooping()) {
 *     frameworkCli_processCliInput();
 *   }
 * 
 * Apps often have other fd's (CMS msgHandle) which can have events, so in 
 * that case, the app can register the fd and a handler func using
 * frameworkCli_setExtraFd().
 */

#if defined(__cplusplus)
extern "C" {
#endif

#include "bcm_retcodes.h"


int  frameworkCli_keepLooping(void);
void frameworkCli_setKeepLooping(int v);

int  frameworkCli_getExitOnIdleTimeout(void);
void frameworkCli_setExitOnIdleTimeout(int secs);

void frameworkCli_setPrompt(const char *prompt);


typedef void (*CLI_CMD_FNC) (char *cmdLine);

// This struct defines a single command name and its handler func.
typedef struct {
   const char *cmdName;
   const char *cmdHelp;
   CLI_CMD_FNC cliProcessingFunc;
} CLI_CMD_ITEM;


/** Add the given array of commands to the list of CLI commands.
 */
void frameworkCli_registerAppCmds(const CLI_CMD_ITEM *appCmdTable, int numEntries);


/** Read and process a line of input on CLI.  This is the main entry point
 *  into the CLI framework.
 */
void frameworkCli_processCliInput();


/** set an extra fd to monitor in waitForInputAvailable.  Usually, this is the
 *  fd from the CMS msgHandle.
 */
typedef void (EXTRA_FD_HANDLER_FUNC) (void);
void frameworkCli_setExtraFd(int extraFd, EXTRA_FD_HANDLER_FUNC *func);


/** Wait for input to become available on 1 or 2 file descriptors.
 *  This should be an internal function, but cms_cli calls it directly.
 *
 *  This function will always detect input available on stdin (fd=0).
 *  If an extra fd was added via frameworkCli_setExtraFd(), this function will
 *  also detect input available on that fd and call the associated handler
 *  func.  This is used for detecting CMS msg bus / system shutdown.
 *
 * @return BCMRET_SUCCESS if input available on stdin.
 *         BCMRET_TIMEOUT_OUT if timeoutSec has elapsed with no input available.
 *         BCMRET_SUCCESS_OBJECT_UNCHANGED if event on extraFd was detected (but not on stdin).
 *         BCMRET_OP_INTR if select was woken up for some unknown reason (not EINTR).
 */
BcmRet frameworkCli_waitForInputAvailable();

// This should be an internal function, but cms_cli calls this function
// directly.
int cmdedit_read_input(char* promptStr, char* command);

// matches CLI_MAX_BUF_SZ in cms_cli/cli.h
// This is basically an internal variable.
#define FRAMEWORK_CLI_MAX_BUF_SZ   4096


#if defined(__cplusplus)
}
#endif

#endif  /* __FRAMEWORK_CLI_H__ */
