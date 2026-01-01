/*
* <:copyright-BRCM:2023:proprietary:standard
* 
*    Copyright (c) 2023 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#ifdef DMP_X_BROADCOM_COM_RPSRFS_1

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_rpsrfs.h"
#include "rdpactl_api.h"
#include "sysutil.h"

#define MAX_RPS_FLOW_CNT_NUM 4096
#define DEF_RPS_FLOW_CNT_NUM 0

static
UINT8 rutRpsRfs_getCpuMask(void)
{
    UINT32 cpuAmount = 1;
    UINT8 cpuMask = 0;

    cpuAmount = sysUtil_getNumCpuThreads();

    /* exclude cpu core 0 */
    switch (cpuAmount)
    {
        case 2:
            cpuMask = 0x02;
            break;
        case 3:
            cpuMask = 0x06;
            break;
        case 4:
            cpuMask = 0x0e;
            break;
        default:
            cpuMask = 0;
    }

#ifdef CONFIG_BCM_SKIP_RTPOLICY
    /* include cpu core 0 */
    if (cpuAmount > 0)
        cpuMask |= 0x01;
#endif

    return cpuMask;
}

static
void rutRpsRfs_ifSet(const char *ifName, UINT8 cpuMask)
{
    char cmdStr[BUFLEN_128];
    char cpusStr[BUFLEN_4];
    UINT8 curCpuMask = 0;
    FILE* fp = NULL; 
    int flowNum = 0;

    if (ifName == NULL)
    {
        return;
    }

    sprintf(cmdStr, "cat /sys/class/net/%s/queues/rx-0/rps_cpus", ifName);
    fp = popen(cmdStr, "r");
    if (!fp)
    {
        cmsLog_error("Failed to run: %s\n", cmdStr);
    }
    else
    {
        if (fgets(cpusStr, sizeof(cpusStr), fp))
        {
            curCpuMask = strtol(cpusStr, NULL, 16);
        }

        pclose(fp);
    }

    if (curCpuMask != cpuMask)
    {
        flowNum = (cpuMask == 0)? DEF_RPS_FLOW_CNT_NUM : MAX_RPS_FLOW_CNT_NUM;

        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %x > /sys/class/net/%s/queues/rx-0/rps_cpus", cpuMask, ifName);
        rut_doSystemAction("rutRpsRfs_ifSet", cmdStr);
        snprintf(cmdStr, sizeof(cmdStr), 
            "echo %d > /sys/class/net/%s/queues/rx-0/rps_flow_cnt", flowNum, ifName);
        rut_doSystemAction("rutRpsRfs_ifSet", cmdStr);
    }
}

void rutRpsRfs_checkSet(const char *ifName) 
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    RpsRfsObject *rpsObj = NULL;
    UBOOL8 rpsEnable = FALSE;
    UINT8 rpsCpuMask = 0;
    CmsRet ret;

    if (ifName == NULL)
        return;

    cmsLog_debug("Enter");

    if ((ret = cmsObj_get(MDMOID_RPS_RFS, &iidStack, 
        OGF_NO_VALUE_UPDATE, (void **) &rpsObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not get RpsRfs_CFG, ret=%d", ret);
        return;
    }
    else
    {
        rpsEnable = rpsObj->enable;
        cmsObj_free((void **) &rpsObj);
    }

    if (rpsEnable)
    {
        rpsCpuMask = rutRpsRfs_getCpuMask();
        rutRpsRfs_ifSet(ifName, rpsCpuMask);
    }
}


static
void rutRpsRfs_allPonSet(UINT8 rpsCpuMask)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

#ifdef DMP_DEVICE2_BASELINE_1
#ifdef DMP_DEVICE2_OPTICAL_1
    _OpticalInterfaceObject *optIfObj = NULL;

    while((CMSRET_SUCCESS == cmsObj_getNext(
        MDMOID_OPTICAL_INTERFACE, &iidStack, (void **)&optIfObj)))
    {
        if (optIfObj->enable == TRUE && 
            cmsUtl_strcmp(optIfObj->status, MDMVS_UP) == 0)
        {
            if (cmsUtl_strcmp(optIfObj->X_BROADCOM_COM_PonType, MDMVS_EPON) == 0)
            {
                char epon_port_name[CMS_IFNAME_LENGTH];
                if (rdpactl_has_rdpa_port_type_epon(epon_port_name))
                {
                    rutRpsRfs_ifSet(epon_port_name, rpsCpuMask);
                }
            }

            if ((cmsUtl_strcmp(optIfObj->X_BROADCOM_COM_PonType, MDMVS_XGS_PON) == 0) || 
                (cmsUtl_strcmp(optIfObj->X_BROADCOM_COM_PonType, MDMVS_XGPON) == 0) || 
                (cmsUtl_strcmp(optIfObj->X_BROADCOM_COM_PonType, MDMVS_NGPON2) == 0) || 
                (cmsUtl_strcmp(optIfObj->X_BROADCOM_COM_PonType, MDMVS_GPON) == 0))
            {
                char gpon_port_name[CMS_IFNAME_LENGTH];
                if (rdpactl_has_rdpa_port_type_gpon(gpon_port_name))
                {
                    rutRpsRfs_ifSet(gpon_port_name, rpsCpuMask);
                }
            }
        }

        cmsObj_free((void **) &optIfObj);
    }
#endif 
#endif

#ifdef DMP_BASELINE_1
#ifdef DMP_X_BROADCOM_COM_PONWAN_1
    _WanPonIntfObject *ponIfObj = NULL;
    INIT_INSTANCE_ID_STACK(&iidStack);

    while((CMSRET_SUCCESS == cmsObj_getNext(
        MDMOID_WAN_PON_INTF, &iidStack, (void **)&ponIfObj)))
    {
        if (ponIfObj->enable == TRUE && 
            cmsUtl_strcmp(ponIfObj->status, MDMVS_UP) == 0)
        {
            if (cmsUtl_strcmp(ponIfObj->ponType, MDMVS_EPON) == 0)
            {
                char epon_port_name[CMS_IFNAME_LENGTH];
                if (rdpactl_has_rdpa_port_type_epon(epon_port_name))
                {
                    rutRpsRfs_ifSet(epon_port_name, rpsCpuMask);
                }
            }

            if (cmsUtl_strcmp(ponIfObj->ponType, MDMVS_GPON) == 0)
            {
                char gpon_port_name[CMS_IFNAME_LENGTH];
                if (rdpactl_has_rdpa_port_type_gpon(gpon_port_name))
                {
                    rutRpsRfs_ifSet(gpon_port_name, rpsCpuMask);
                }
            }
        }

        cmsObj_free((void **) &ponIfObj);
    }
#endif
#endif
}


static
void rutRpsRfs_allEthSet(UINT8 rpsCpuMask)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

#ifdef DMP_DEVICE2_BASELINE_1
#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
    _Dev2EthernetInterfaceObject *ethIfObj = NULL;

    while((CMSRET_SUCCESS == cmsObj_getNext(
        MDMOID_DEV2_ETHERNET_INTERFACE, &iidStack, (void **)&ethIfObj)))
    {
        if ((ethIfObj->enable == TRUE) && (ethIfObj->lowerLayers == NULL))
        {
            rutRpsRfs_ifSet(ethIfObj->name, rpsCpuMask) ;
        }

        cmsObj_free((void **) &ethIfObj);
    }
#endif /* DMP_DEVICE2_ETHERNETINTERFACE_1 */
#endif /* DMP_DEVICE2_BASELINE_1 */

#ifdef DMP_BASELINE_1
    _LanEthIntfObject *lanIfObj = NULL;
    INIT_INSTANCE_ID_STACK(&iidStack);

    while((CMSRET_SUCCESS == cmsObj_getNext(
        MDMOID_LAN_ETH_INTF, &iidStack, (void **)&lanIfObj)))
    {
        if (lanIfObj->enable == TRUE)
        {
            rutRpsRfs_ifSet(lanIfObj->X_BROADCOM_COM_IfName, rpsCpuMask) ;
        }

        cmsObj_free((void **) &lanIfObj);
    }
#endif /* DMP_BASELINE_1 */

#if defined(DMP_ETHERNETWAN_1)
    _WanEthIntfObject *ethWanObj = NULL;
    INIT_INSTANCE_ID_STACK(&iidStack);

    while((CMSRET_SUCCESS == cmsObj_getNext(
        MDMOID_WAN_ETH_INTF, &iidStack, (void **)&ethWanObj)))
    {
        if ((ethWanObj->enable == TRUE) && ethWanObj->X_BROADCOM_COM_IfName && 
            (cmsUtl_strcmp(ethWanObj->status, MDMVS_UP) == 0))
        {
            rutRpsRfs_ifSet(ethWanObj->X_BROADCOM_COM_IfName, rpsCpuMask) ;
        }

        cmsObj_free((void **) &ethWanObj);
    }
#endif /* DMP_ETHERNETWAN_1 */
}


void rutRpsRfs_globalSet(UBOOL8 enable)
{
    UINT8 rpsCpuMask = enable? rutRpsRfs_getCpuMask() : 0;

    cmsLog_debug("Enter");

#ifndef CONFIG_BCM_SKIP_RTPOLICY
    // When SKIP_RTPOLICY is not defined, kernel thread recycle_sysb is bounded to the last cpu.
    // The kernel threads recycle_sysb and ksoftirqd/{last cpu num} will fight for CPU since
    // they shared same priority.
    // When enable RPS/RFS, to set the priority of recycle_sysb higher(10 to 5) than
    // RPS/RFS effected receving kernel thread ksoftirqd/{last cpu num}. 
    // Then the packets will not be discarded on the tx stage after consuming the recv datapath resource.
    // When disable RPS/RFS, retore the priority of recycle_sysb.
    UINT8 priority = enable? 10 : 5;
    char cmdStr[BUFLEN_128];
    snprintf(cmdStr, sizeof(cmdStr), 
        "chrt -p -r %d $(ps |grep %s |awk 'NR==1 {print $1}')", priority, "recycle_sysb");
    rut_doSystemAction("rutRpsRfs_ifSet", cmdStr);
#endif

    rutRpsRfs_allPonSet(rpsCpuMask) ;
    rutRpsRfs_allEthSet(rpsCpuMask);
}

#endif /* DMP_X_BROADCOM_COM_RPSRFS_1 */
