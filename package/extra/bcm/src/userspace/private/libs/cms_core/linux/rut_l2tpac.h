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

#ifndef __RUT_L2TPAC_H__
#define __RUT_L2TPAC_H__
#define xl2tpdFolder "/var/run/xl2tpd"
#define xl2tpdAssignedPort  "/var/run/xl2tpdAssignedPort"
#define L2TP_MTU 1460
#define L2TP_MRU 1460
#define DEFAULT_LAC_LINKNAME "ppp2"
#define L2tpDefaultPort 1701 // l2tp default port

/** Start L2tpd as daemon.  
 *
 * @return CmsRet enum.
 */
CmsRet rutL2tp_startL2tpd(char *tunnelName);

/** Stop L2tpd as daemon.  
 *
 * @return CmsRet enum.
 */
CmsRet rutL2tp_stopL2tpd(char *tunnelName);


/** send msg to smd to start the app.
 */
CmsRet rutL2tp_startApp(const char *cmd);

/** Send msg to smd to kill and collect the app.
 */
void rutL2tp_stopApp(UINT32 pid);


/** Create the L2TPAC tunnel config file.
 *
 * @param l2tpAcIntfObj (IN) 
 *
 * @return CmsRet enum.
 */
CmsRet rutL2tp_createTunnelConfig(_L2tpAcIntfConfigObject *l2tpIntfObj);


/** Create the L2TPAC Link config file.
 *
 * @param l2tpLinkObj (IN) 
 *
 * @param pppObj (IN) 
 *
 * @return CmsRet enum.
 */
CmsRet rutL2tp_createLinkConfig(_L2tpAcLinkConfigObject *l2tpLinkObj, _WanPppConnObject *pppObj);


/** Refresh L2tpd config.  
 *
 * @return void.
 */
void rutL2tp_refreshL2tp();


#endif  /* __RUT_L2TPAC_H__ */


