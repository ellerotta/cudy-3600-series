/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#include "cms_util.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_cli.h"
#include "cms_seclog.h"
#include "prctl.h"
#include "cms_boardcmds.h"
#include "cli.h"
#include "mdm_cli.h"
#include "framework_cli.h"
#include <errno.h>
#include <sys/time.h>


#ifndef MIN
#define   MIN(a,b) (((a)<(b))?(a):(b))
#endif




/* global */
char menuPathBuf[MAX_MENU_PATH_LENGTH] = " > ";
WEB_NTWK_VAR glbWebVar;

/* authentication information */
char   currUser[BUFLEN_64];
UINT8  currPerm=0;
UINT16 currAppPort = 0;
char   currAppName[BUFLEN_16] = { 0 };
char   currIpAddr[CMS_IPADDR_LENGTH];

static void processInput(void);


void cmsCli_printWelcomeBanner(void)
{
   UINT32 chipId;
   CmsRet ret;

   if ((ret = devCtl_getChipId(&chipId)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get chipid, fake it");
      chipId = 0x9999;
   }

   printf("BCM9%x Broadband Router\n", chipId);
}


CmsRet cmsCli_authenticate(NetworkAccessMode accessMode, UINT32 eoiTimeout)
{
   char login[BUFLEN_256], pwd[BUFLEN_256];
   char *pc = NULL;
   int done = FALSE, authNum = 0;
   CmsRet ret=CMSRET_SUCCESS;
   UBOOL8 bAuth;
   CmsSecurityLogData logData = { 0 };
   HttpLoginType authLevel = LOGIN_INVALID;

   mdmCli_setExitOnIdleTimeout((int) eoiTimeout);

   if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return ret;
   }

   if (CMSRET_SUCCESS != (ret = cmsDal_getCurrentLoginCfg(&glbWebVar)))
   {
      cmsLog_error("failed to get login info, ret=%d", ret);
      return(ret);
   }

   cmsLck_releaseLock();

#ifdef CMS_BYPASS_LOGIN
   /*
    * The ifdef symbol name is a little confusing.  For now, it only applies to
    * logins from console port, although it may be extended to cover telnet and
    * ssh in the future.
    * So if this is a console login, then just set the perm as admin and return.
    */
   if (accessMode == NETWORK_ACCESS_CONSOLE)
   {
      cmsUtl_strncpy(currUser, glbWebVar.adminUserName, sizeof(currUser));
      currPerm = PERM_ADMIN;

      CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
      CMSLOG_SEC_SET_USER(&logData, glbWebVar.adminUserName);
      if (currAppPort != 0 )
      {
         CMSLOG_SEC_SET_PORT(&logData, currAppPort);
         CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
      }
      cmsLog_security(LOG_SECURITY_AUTH_LOGIN_PASS, &logData, NULL);

      return CMSRET_SUCCESS;
   }
#endif


   // read empty line to wait for input, for accessing from the console only
   // Also solve the carriage return problem for windows 2000 telnet client
   if (accessMode == NETWORK_ACCESS_CONSOLE) {
      if ((ret = cli_readString(login, sizeof(login))) != CMSRET_SUCCESS) {
         return ret;
      }
   }


   while ( done == FALSE ) {
         login[0] = '\0';
         printf("Login: ");

         // When the serial port is not configured, telnet sessions need
         // stdout to be flushed here.
         fflush(stdout);

         // Read username string, while checking for idle timeout
         if ((ret = cli_readString(login, sizeof(login))) != CMSRET_SUCCESS) {
            break;
         }


         /* mwang_todo: uh-oh, this will not time out.  Need to explicitly code something
          * to turn off echoing, do a read, and then turn echo back on. */
         pc = getpass("Password: ");
         memset(pwd, 0, sizeof(pwd));
         if ( pc != NULL ) {
            strncpy(pwd, pc, sizeof(pwd)-1);
            bzero(pc, strlen(pc));
         }
         authNum++;

         if ((ret = cmsLck_acquireLockWithTimeout(CLI_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get lock, ret=%d", ret);
            cmsLck_dumpInfo();
            break;
         }

         bAuth = cmsDal_authenticate(&authLevel, accessMode, login, pwd);

         cmsLck_releaseLock();

         CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
         CMSLOG_SEC_SET_USER(&logData, &login[0]);
         if (currAppPort != 0 )
         {
            CMSLOG_SEC_SET_PORT(&logData, currAppPort);
            CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
         }
         if ( 1 == bAuth ) {
            cmsLog_security(LOG_SECURITY_AUTH_LOGIN_PASS, &logData, NULL);
            done = TRUE;
            /* remember who has logged in and set the permission bits */
            cmsUtl_strncpy(currUser, login, sizeof(currUser));
            if (!strcmp(currUser, glbWebVar.adminUserName))
            {
               currPerm = PERM_ADMIN;
            }
            else if (!strcmp(currUser, glbWebVar.sptUserName))
            {
               currPerm = PERM_SUPPORT;
            }
            else if (!strcmp(currUser, glbWebVar.usrUserName))
            {
               currPerm = PERM_USER;
            }
            else
            {
               cmsLog_error("unrecognized user %s, unable to set permission", currUser);
            }
         } else if ( authNum >= AUTH_NUM_MAX ) {
            printf("Authorization failed after trying %d times!!!.\n", authNum);
            fflush(stdout);
            cmsLog_security(LOG_SECURITY_LOCKOUT_START, &logData, NULL);
            sleep(AUTH_FAIL_SLEEP);
            cmsLog_security(LOG_SECURITY_LOCKOUT_END, &logData, NULL);
            authNum = 0;
         } else {
            cmsLog_security(LOG_SECURITY_AUTH_LOGIN_FAIL, &logData, NULL);
            printf("Login incorrect. Try again.\n");
            fflush(stdout);
         }
   }

   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("current logged in user %s perm=0x%x", currUser, currPerm);
   }

   return ret;
}


void cmsCli_run(void *mh, UINT32 eoiTimeout)
{
   CmsSecurityLogData logData = { 0 };

   cmsLog_debug("CLI library entered");
   mdmCli_setExitOnIdleTimeout((int) eoiTimeout);

   cliPrvtMsgHandle = mh;
   if (cliPrvtMsgHandle != NULL)
   {
      int msgFd = -1;
      cmsMsg_getEventHandle(cliPrvtMsgHandle, &msgFd);
      frameworkCli_setExtraFd(msgFd, mdmCli_msgHandlerFunc);
   }

   while ( mdmCli_keepLooping() ) {
      processInput();
   }

   CMSLOG_SEC_SET_APP_NAME(&logData, &currAppName[0]);
   CMSLOG_SEC_SET_USER(&logData, &currUser[0]);
   if (currAppPort != 0 )
   {
      CMSLOG_SEC_SET_PORT(&logData, currAppPort);
      CMSLOG_SEC_SET_SRC_IP(&logData, &currIpAddr[0]);
   }

   cmsLog_security(LOG_SECURITY_AUTH_LOGOUT, &logData, NULL);

   printf("\nBye bye. Have a nice day!!!\n");

   memset(currUser, 0, sizeof(currUser));
   currPerm = 0;
}


void cmsCli_setAppData(char * appName, char * ipAddr, char * curUser, UINT16 appPort)
{
   if ( NULL != appName )
   {
      memset(&currAppName[0], 0, sizeof(currAppName));
      strncpy(&currAppName[0], appName, MIN(sizeof(currAppName), strlen(appName)));
   }

   if ( NULL != ipAddr )
   {
      memset(&currIpAddr[0], 0, sizeof(currIpAddr));
      strncpy(&currIpAddr[0], ipAddr, MIN(sizeof(currIpAddr), strlen(ipAddr)));
   }

   if ( NULL != curUser )
   {
      memset(&currUser[0], 0, sizeof(currUser));
      strncpy(&currUser[0], curUser, MIN(sizeof(currUser), strlen(curUser)));
   }

   currAppPort = appPort;

}

void cmsCli_setPrompt(const char *prompt)
{
   snprintf(menuPathBuf, sizeof(menuPathBuf), "%s ", prompt);
}

void cmsCli_setPermAdmin()
{
   currPerm = PERM_ADMIN;
}


/** Reads an input line from user and processes it.
 *
 */
void processInput() {
   char cmdLine[CLI_MAX_BUF_SZ];
   int supportedModes=0;


#ifdef SUPPORT_CLI_CMD
   supportedModes++;
#endif

   /* if cmd cli isn't enabled, just spawn a shell */
   if (supportedModes == 0)
   {
      prctl_runCommandInShellBlocking("sh");
      mdmCli_setKeepLooping(0);
      return;
   }


   /*
    * Read an input line from the user and process it using
    * menu and/or cmd code.
    */
   bzero(cmdLine,CLI_MAX_BUF_SZ);

#ifdef CLI_CMD_EDIT
   if (cmdedit_read_input(menuPathBuf, cmdLine) < 0)
   {
      mdmCli_setKeepLooping(0);
      return;
   }

   // remove the trailing return
   int l = strlen(cmdLine);
   if (l > 0 && cmdLine[l-1] == '\n')
      cmdLine[l-1] = 0;
#else
   printf("%s", menuPathBuf);
   fflush(stdout);
   if (cli_readString(cmdLine, CLI_MAX_BUF_SZ) != CMSRET_SUCCESS)
   {
      mdmCli_setKeepLooping(0);
      return;
   }
#endif

   cmsLog_debug("read =>%s<=", cmdLine);

   if ( strlen(cmdLine) == 0 )
      return;


#ifdef SUPPORT_CLI_CMD
   if ( cli_processCliCmd(cmdLine) == TRUE ) {
      /* input is command line command */
      /* no need to do anything in here */
   }
   else if ( cli_processHiddenCmd(cmdLine) == TRUE ) {
      /* input is hidden command */
      /* no need to do anything in here */
   }
   else {
      cmsLog_error("unrecognized command %s", cmdLine);
   }

   if (mdmCli_keepLooping())
   {
      cli_waitForContinue();
   }
#endif  /* SUPPORT_CLI_CMD */

   return;
}


#define CLI_BACKSPACE        '\x08'

CmsRet cli_readString(char *buf, int size)
{
   SINT32 nchars = 0;
   int ch = 0;
   CmsRet ret;

   memset(buf, 0, size);

   if ((ret = frameworkCli_waitForInputAvailable()) != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* read individual characters until we get a newline or until
    * we exceed given buffer size.
    */
   for ( ch = fgetc( stdin );
         ch != '\r' && ch != '\n' && ch != EOF && nchars < (size-1) ;
         ch = fgetc( stdin ) ) {
      if ( ch == CLI_BACKSPACE ) {
         if ( nchars > 0 )
            nchars--;
      } else {
         buf[nchars++] = ch;
      }
   }


   if (ch == EOF)
   {
      printf("EOF detected, terminate login session.\n");
      exit(0);
   }

   buf[nchars] = '\0';

   return ret;
}


/** This is only needed when menu driven CLI is enabled
 *  because everytime we print out the menu, we blank out the output
 *  from the previous command.
 */
void cli_waitForContinue(void)
{

}


