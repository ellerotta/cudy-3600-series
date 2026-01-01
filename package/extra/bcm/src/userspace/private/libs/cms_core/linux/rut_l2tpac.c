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

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

#include <sys/stat.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wanlayer2.h"
#include "rut_l2tpac.h"

const char *l2tpdFolder  = "/var/run/xl2tpd";

void rutL2tp_refreshL2tp()
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack2          = EMPTY_INSTANCE_ID_STACK;
    L2tpAcIntfConfigObject *L2tpAcIntf = NULL;
    L2tpAcLinkConfigObject *L2tpAclinkCfg = NULL; 
    InstanceIdStack	iidStack = EMPTY_INSTANCE_ID_STACK;
    void *obj = NULL;
    
    if (rutWl2_getL2tpWanIidStack(&iidStack) == CMSRET_SUCCESS)
    {
        while(cmsObj_getNextInSubTree(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack, &iidStack2, (void **)&L2tpAcIntf) == CMSRET_SUCCESS)
        {
            cmsLog_debug("refresh L2tpAcIntf \n");
            if ((ret = cmsObj_set(L2tpAcIntf, &iidStack2)) != CMSRET_SUCCESS)
            {
                cmsLog_error("refresh L2tpAcIntf error.");
            }

            cmsObj_free((void **) &L2tpAcIntf);
        }

        INIT_INSTANCE_ID_STACK(&iidStack2);
        while (cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &iidStack, &iidStack2, (void **)&obj)== CMSRET_SUCCESS)
        {
            if ((ret = cmsObj_get(MDMOID_L2TP_AC_LINK_CONFIG, &iidStack2, OGF_NO_VALUE_UPDATE, (void **) &L2tpAclinkCfg)) == CMSRET_SUCCESS)
            {
                cmsLog_debug("refresh L2tpAclinkCfg \n");
                if ((ret = cmsObj_set(L2tpAclinkCfg, &iidStack2)) != CMSRET_SUCCESS)
                {
                    cmsLog_error("refresh L2tpAclinkCfg error.");
                }
                cmsObj_free((void **)&L2tpAclinkCfg);
            }
            cmsObj_free((void **)&obj); 
        }
    }	   
}

CmsRet rutL2tp_startL2tpd(char *tunnelName)
{   
    CmsRet ret = CMSRET_SUCCESS;
    char l2tpcmd[BUFLEN_256]={0};
    char xl2tpdConf[BUFLEN_64]={0};
    char xl2tpdPidConf[BUFLEN_64]={0};

    cmsLog_notice("Entered: tunnelName=%s", tunnelName);

    snprintf(xl2tpdConf, sizeof(xl2tpdConf), "%s/l2tp_%s.conf", l2tpdFolder, tunnelName);
    snprintf(xl2tpdPidConf, sizeof(xl2tpdPidConf), "%s/l2tp_%s.pid", l2tpdFolder, tunnelName);

    snprintf(l2tpcmd, sizeof(l2tpcmd), " -D -c %s -p %s", xl2tpdConf, xl2tpdPidConf);  
    ret = rutL2tp_startApp(l2tpcmd);

    return ret;
}


CmsRet rutL2tp_startApp(const char *cmd)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 pid = 0;

   cmsLog_notice("Entered: cmd=%s", cmd);

   // It is tempting to use prctl_launchApp() here instead of sending a msg to
   // smd to launch the app.  But the problem with directly launching the app
   // is that the process which calls this function becomes the parent of
   // xl2tpd, but later, a different process might try to stop and collect it.
   // For example, on system startup, ssk will start xl2tpd and
   // becomes the parent, but if user on httpd tries to stop it, httpd cannot
   // collect the child.  (xl2tpd has an option to run in daemon mode, so init
   // is the parent, but daemon code cannot log debug messages to stderr).
   pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_XL2TPD, cmd, strlen(cmd)+1);
   if (pid == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart l2tpd (cmd=%s)", cmd);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_notice("l2tpd (cmd=%s) started, pid=%d", cmd, pid);
   }

   return ret;
}

void rutL2tp_stopApp(UINT32 pid)
{
   UINT32 specificEid = MAKE_SPECIFIC_EID(pid, EID_XL2TPD);
   CmsRet ret;

   cmsLog_notice("Entered: pid=%d", pid);

   if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS))
   {
      cmsLog_error("failed to stop l2tpd");
   }
   return;
}


CmsRet rutL2tp_stopL2tpd(char *tunnelName)
{   
    struct stat st;
    FILE *fp;
    char xl2tpdPidConf[BUFLEN_64]={0};
    char xl2tpdConf[BUFLEN_64]={0};
    char xl2tpdPpp[BUFLEN_64]={0};
    UINT32 xl2tpdPid   = 0; 

    cmsLog_notice("Entered: tunnelName=%s", tunnelName);

    snprintf(xl2tpdPidConf, sizeof(xl2tpdPidConf), "%s/l2tp_%s.pid", l2tpdFolder, tunnelName);
    snprintf(xl2tpdConf, sizeof(xl2tpdConf), "%s/l2tp_%s.conf", l2tpdFolder, tunnelName);
    snprintf(xl2tpdPpp, sizeof(xl2tpdPpp), "%s/ppp-l2tp_%s", l2tpdFolder, tunnelName);

    if (stat(xl2tpdPidConf, &st) == -1) 
    {
        return CMSRET_SUCCESS;
    }

    /* coverity[toctou] */
    fp = fopen(xl2tpdPidConf, "r");
    if (fp != NULL)
    {
        if( fscanf(fp, "%u", &xl2tpdPid) != 1 )
        {
            cmsLog_error("fscanf pid from xl2tpdPidFile (%s) error!!", xl2tpdPidConf);
        }
        fclose(fp);
    }
    else
    {
        cmsLog_error("Cannot open pidfile %s", xl2tpdPidConf);
    }

    if (xl2tpdPid)
    {
        rutL2tp_stopApp(xl2tpdPid);
    }

    // Typically, in linux we do not report errors if we cannot delete
    // a file.  It usually means the file has already been deleted.
    remove(xl2tpdPidConf);
    remove(xl2tpdConf);
    remove(xl2tpdPpp);

    return CMSRET_SUCCESS;
}

CmsRet rutL2tp_createTunnelConfig(_L2tpAcIntfConfigObject *l2tpIntfObj)
{   
    FILE *fp_conf;
    struct stat st;
    char xl2tpdConf[BUFLEN_64];
    char xl2tpdPpp[BUFLEN_64];
    
    /* create /var/run/xl2tpd folder for holding xl2tpd related config files */   
    if (stat(l2tpdFolder, &st) == -1)
    {
        /* coverity[toctou] */
        if (mkdir(l2tpdFolder, 0700))
        {
            cmsLog_error("/var/run/xl2tpd: unable to create folder\n");
            return CMSRET_INTERNAL_ERROR;
        }
    }

    snprintf(xl2tpdConf, sizeof(xl2tpdConf), "%s/l2tp_%s.conf", l2tpdFolder, l2tpIntfObj->tunnelName);
    snprintf(xl2tpdPpp, sizeof(xl2tpdPpp), "%s/ppp-l2tp_%s", l2tpdFolder, l2tpIntfObj->tunnelName);

    /* write out xl2tpd.conf file */
    fp_conf = fopen(xl2tpdConf, "w");
    if (fp_conf == NULL)
    {
        cmsLog_error("%s: unable to open file\n",xl2tpdConf);
        return CMSRET_INTERNAL_ERROR;
    }

    fprintf(fp_conf, "[global]\n"); // global context
    if (l2tpIntfObj->sourcePort != L2tpDefaultPort)
    {    
        /*Specify which UDP port xl2tpd should use. The default is 1701*/ 
        fprintf(fp_conf, "port = %d\n", l2tpIntfObj->sourcePort); 
    }
    
    fprintf(fp_conf, "[lac %s]\n", l2tpIntfObj->tunnelName);
    fprintf(fp_conf, "lns = %s\n", l2tpIntfObj->lnsIpAddress);
    fprintf(fp_conf, "ppp debug = yes\n");
    fprintf(fp_conf, "autodial = yes\n");
    fprintf(fp_conf, "pppoptfile = %s\n", xl2tpdPpp);
   
    fclose(fp_conf);

    return CMSRET_SUCCESS;
}

CmsRet rutL2tp_createLinkConfig(_L2tpAcLinkConfigObject *l2tpLinkObj, _WanPppConnObject *pppObj)
{   
    FILE *fp_ppp;
    char xl2tpdPpp[BUFLEN_64];

    snprintf(xl2tpdPpp, sizeof(xl2tpdPpp), "%s/ppp-l2tp_%s", l2tpdFolder, l2tpLinkObj->tunnelName);
    
    /* write out ppp-options file */
    fp_ppp = fopen(xl2tpdPpp, "w");
    if (fp_ppp == NULL)
    {
        cmsLog_error("%s: unable to open file\n", xl2tpdPpp);
        return CMSRET_INTERNAL_ERROR;
    }

    fprintf(fp_ppp, "linkname %s\n", l2tpLinkObj->sessionName);
    fprintf(fp_ppp, "user %s\n", pppObj->username);
    fprintf(fp_ppp, "password %s\n", pppObj->password);

    fclose(fp_ppp);  

    return CMSRET_SUCCESS;
}

#endif
