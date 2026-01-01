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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "bcm_retcodes.h"
#include "bcm_ulog.h"
#include "framework_cli.h"


// These are private variables to the framework_cli.  They should be
// get/set via API funcs and not by directly accessing the variables.
static int cli_keepLooping = 1;
static int exitOnIdleTimeout = 0;  // in seconds, if not set (0), no timeout
static char menuPathBuf[1024];  // for the prompt, MAX_MENU_PATH_LENGTH (1024) in cli.h
static int extraFd = -1;
static EXTRA_FD_HANDLER_FUNC *extraFdFunc = NULL;


// This is the dynamically allocated table of all CLI functions.
static CLI_CMD_ITEM *cliCmdTable = NULL;
static int cliCmdCount = 0;


// foward declarations
static void processHelpCmd(char *cmdLine);
static void processLogoutCmd(char *cmdLine);


// This table contains the base commands that this lib provides.
static const CLI_CMD_ITEM baseCmdTable[] = {
   { "?",    "List of all commands.",           processHelpCmd },
   { "help", "List of all commands.",           processHelpCmd },
   { "logout", "Exit from CLI.",                processLogoutCmd },
   { "exit", "Exit from CLI.",                  processLogoutCmd },
   { "quit", "Exit from CLI.",                  processLogoutCmd },
};

static void registerBaseCmds()
{
   if (cliCmdTable == NULL)
   {
      // copy our base table to new table.
      cliCmdTable = malloc(sizeof(baseCmdTable));
      if (cliCmdTable == NULL)
      {
         bcmuLog_error("malloc of baseCmdTable failed");
         return;
      }
      memcpy(cliCmdTable, baseCmdTable, sizeof(baseCmdTable));
      cliCmdCount = sizeof(baseCmdTable) / sizeof(CLI_CMD_ITEM);
   }

   return;
}

void frameworkCli_registerAppCmds(const CLI_CMD_ITEM *appCmdTable, int numEntries)
{
   CLI_CMD_ITEM *tmpTable = NULL;

   // If we have not registered our base table yet, do it now.
   if (cliCmdTable == NULL)
   {
      registerBaseCmds();
   }

   if ((appCmdTable == NULL) || (numEntries <= 0))
   {
      return;
   }

   // append more cli commands to existing table.
   // Check for adding duplicate commands?
   tmpTable = malloc((cliCmdCount + numEntries) * sizeof(CLI_CMD_ITEM));
   if (tmpTable == NULL)
   {
      bcmuLog_error("malloc of %d entries failed", cliCmdCount + numEntries);
      return;
   }

   memcpy(tmpTable, cliCmdTable, cliCmdCount * sizeof(CLI_CMD_ITEM));
   memcpy(&(tmpTable[cliCmdCount]), appCmdTable, numEntries * sizeof(CLI_CMD_ITEM));
   free(cliCmdTable);
   cliCmdTable = tmpTable;
   cliCmdCount += numEntries;

   return;
}


void processHelpCmd(char *cmdLine __attribute((unused)))
{
   int i;

   for (i=0; i < cliCmdCount; i++)
   {
      printf("%s\r\n", cliCmdTable[i].cmdName);
   }

   return;
}

void processLogoutCmd(char *cmdLine __attribute__((unused)))
{
   cli_keepLooping = 0;
   return;
}

int frameworkCli_keepLooping(void)
{
   return cli_keepLooping;
}

void frameworkCli_setKeepLooping(int v)
{
   cli_keepLooping = v;
   return;
}

int frameworkCli_getExitOnIdleTimeout(void)
{
   return exitOnIdleTimeout;
}

void frameworkCli_setExitOnIdleTimeout(int secs)
{
   exitOnIdleTimeout = secs;
   return;
}

void frameworkCli_setPrompt(const char *prompt)
{
   snprintf(menuPathBuf, sizeof(menuPathBuf), "%s$ ", prompt);
   return;
}


void frameworkCli_setExtraFd(int fd, EXTRA_FD_HANDLER_FUNC *func)
{
   if ((extraFd != -1) || (extraFdFunc != NULL))
   {
      bcmuLog_error("extraFd/extraFdFunc already set! (%d/%p)", extraFd, extraFdFunc);
      return;
   }

   // extraFd and extraFdFunc must be either both set or not set.
   if ((fd == -1) || (func == NULL))
   {
      bcmuLog_error("invalid combination for extraFd %d, %p", fd, func);
      return;
   }

   extraFd = fd;
   extraFdFunc = func;
   return;
}


static int lookupAndCall(char *cmdLine)
{
   int c, found = 0;
   size_t i, cmdNameLen;
   char cmdName[256] = {0};

   // If we have not registered our base table yet, do it now.
   if (cliCmdTable == NULL)
   {
      registerBaseCmds();
   }

   // Get the first word of cmdLine, which will be cmdName
   cmdNameLen = strlen(cmdLine);
   for (i=0; i < cmdNameLen && i < sizeof(cmdName)-1; i++)
   {
      if (cmdLine[i] == ' ')
      {
         cmdNameLen = i;
         break;
      }
      cmdName[i] = cmdLine[i];
   }

   for (c=0; c < cliCmdCount; c++)
   {
      /*
       * Note that strcasecmp is used here, which means a command should not
       * differ from another command only in capitalization.
       */
      if ((cmdNameLen == strlen(cliCmdTable[c].cmdName)) &&
          (!strncasecmp(cmdName, cliCmdTable[c].cmdName, cmdNameLen)))
      {
         // Call the function.
         if (strlen(cmdLine) == cmdNameLen)
         {
            /* there is no additional args, just pass in null terminator */
            (*(cliCmdTable[c].cliProcessingFunc))(&(cmdLine[cmdNameLen]));
         }
         else
         {
            /* pass the additional args to the processing func */
            (*(cliCmdTable[c].cliProcessingFunc))(&(cmdLine[cmdNameLen + 1]));
         }

         found = 1;
         break;
      }
   }

   return found;
}


static int processCmd(char *cmdLine)
{
   if (cmdLine == NULL || cmdLine[0] == '\0')
   {
      // return true so we don't complain about cmd not found.
      return 1;
   }

   // skip over any leading white spaces
   while (*cmdLine == ' ')
   {
      cmdLine++;
   }

   return (lookupAndCall(cmdLine));
}


void frameworkCli_processCliInput()
{
   char cmdLine[FRAMEWORK_CLI_MAX_BUF_SZ]={0};

   // Assume CLI_CMD_EDIT is always defined.
   if (cmdedit_read_input(menuPathBuf, cmdLine) < 0)
   {
      cli_keepLooping = 0;
      return;
   }

   // remove the trailing return
   int l = strlen(cmdLine);
   if (l > 0 && cmdLine[l-1] == '\n')
      cmdLine[l-1] = 0;

   if ( strlen(cmdLine) == 0 )
      return;

   if (!processCmd(cmdLine))
   {
      printf("unrecognized command %s\n", cmdLine);
   }

   return;
}


BcmRet frameworkCli_waitForInputAvailable()
{
   struct timeval timeout;
   struct timeval *timeoutPtr=NULL;
   struct timeval  tv1, tv0;
   struct timezone tz;
   int mainFd=0;  // this is the main input fd, usually stdin, fd=0
   int maxFd=0;
   fd_set readfds;
   ssize_t n;
   int remainedTimeout=exitOnIdleTimeout;

   if (exitOnIdleTimeout > 0)
   {
      timeout.tv_sec = (long) exitOnIdleTimeout;
      timeout.tv_usec = 0;
      timeoutPtr = &timeout;
   }
   else
   {
      /*
       * If user has set exitOnIdleTimeout to 0, that means no timeout.  Wait indefinately.
       */
      timeoutPtr = NULL;
   }

again:
   FD_ZERO(&readfds);
   FD_SET(mainFd, &readfds);  // mainFd is typically stdin, fd=0

   if ((extraFd != -1) && (extraFdFunc != NULL))
   {
      FD_SET(extraFd, &readfds);
   }

   gettimeofday(&tv0, &tz);

   maxFd = (mainFd > extraFd) ? mainFd : extraFd;
   n = select(maxFd+1, &readfds, NULL, NULL, timeoutPtr);
   if (n == 0)
   {
      printf("session terminated due to idle timeout (%d seconds)\n", exitOnIdleTimeout);
      return BCMRET_TIMED_OUT;
   }
   else if (n < 0)
   {
      bcmuLog_notice("select interrupted");
      if(errno == EINTR)
      {
         if(exitOnIdleTimeout > 0)
         {
            int dt;

            gettimeofday(&tv1, &tz);
            dt = (int) (tv1.tv_sec - tv0.tv_sec);
            if(dt >= remainedTimeout)
               timeout.tv_sec = 1;
            else
               timeout.tv_sec = remainedTimeout = remainedTimeout - dt;
            timeout.tv_usec = 0;
         }
         goto again;
      }
      // we got woken up for some unknown reason (not EINTR).
      return BCMRET_OP_INTR;
   }
   else if ((extraFd != -1) && (FD_ISSET(extraFd, &readfds)))
   {
      // call handler func to handle an event on this fd.
      (*extraFdFunc)();

      if (FD_ISSET(mainFd, &readfds))
      {
         // In theory, we could get an event on mainFd and extraFd at the same
         // time, so we will want to indicate there is an event on mainFd.
         return BCMRET_SUCCESS;
      }
      else
      {
         // Overloading this return code to indicate that the extraFd event
         // was handled, but there is no event on the mainFd (stdin).
         return BCMRET_SUCCESS_OBJECT_UNCHANGED;
      }
   }

   // There is an event on mainFd.
   return BCMRET_SUCCESS;
}

