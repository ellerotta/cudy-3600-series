/***********************************************************************
 *
 *  Copyright (c) 2023  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2023:proprietary:standard

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

#ifndef _FPICTL_API_H_
#define _FPICTL_API_H_
#include "fpi_ioctl.h"


/* mode-related APIs */
int bcm_fpictl_set_mode(fpi_mode_t mode);
int bcm_fpictl_get_mode(fpi_mode_t *pMode);
int bcm_fpictl_set_default_priority(uint8_t prio);
int bcm_fpictl_get_default_priority(uint8_t *pPrio);
int bcm_fpictl_set_gre_mode(fpi_gre_mode_t mode);
int bcm_fpictl_set_l2lkp_on_etype(uint8_t enable, uint16_t etype);
int bcm_fpictl_set_lkp_enable(uint8_t enable);

/* flow-related APIs */
int bcm_fpictl_add_flow(fpictl_data_t *fpi);
int bcm_fpictl_delete_flow_by_handle(uint32_t handle);
int bcm_fpictl_delete_flow_by_key(fpictl_data_t *fpi);
int bcm_fpictl_get_flow(uint32_t handle, fpictl_data_t *fpi);
int bcm_fpictl_dump_flow(uint32_t handle);
int bcm_fpictl_dump_flowtbl(void);

/* stat-related APIs */
int bcm_fpictl_get_stat(uint32_t handle, uint32_t *pkt_cnt, uint64_t *byte_cnt);

/* AP MAC-related APIs */
int bcm_fpictl_add_ap_mac(uint8_t *mac);
int bcm_fpictl_delete_ap_mac(uint8_t *mac);
int bcm_fpictl_dump_ap_mac(void);

/* error code parsing */
char *bcm_fpictl_err_message(int err);

#endif /* _FPICTL_API_H_ */
