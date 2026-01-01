/*
*  Copyright 2011, Broadcom Corporation
*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
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
* :>
*/



#ifndef __MAC_DRV_AE_H__
#define __MAC_DRV_AE_H__

#include "Laser.h"

/* epon_ae_reent_lock Task-aware mutex
 * The same task can take the same mutex multiple times.
 * However, if task B attempts to take mutex that is already taken by task A,
 * it locks until the mutex becomes available.
 * If task takes task aware mutex multiple times, number of
 * epon_ae_unlock() calls must match epon_ae_lock()
 * @{
 */

typedef struct {
    pid_t pid;
    int count;
    struct mutex m;
} epon_ae_reent_lock;


/** Initialize task-aware mutex
 * \param[in]   pmutex  mutex handle
 */
void epon_ae_lock_init(epon_ae_reent_lock *pmutex);

/** Lock task-aware mutex
 * \param[in]   pmutex  mutex handle
 * If the mutex is taken by different task, the caller blocks
 * \return  0=mutex taken, <0-error (interrupted)
 */
int epon_ae_lock(epon_ae_reent_lock *pmutex);

/** Unlock task-aware mutex
 * \param[in]   pmutex  mutex handle
 */
void epon_ae_unlock(epon_ae_reent_lock *pmutex);

extern epon_ae_reent_lock ae_reent_lock;


#define FUNCTION_WITH_LOCK(FUNCTION_NAME,args) \
    do{\
        int rc;\
        rc = epon_ae_lock(&ae_reent_lock); \
        if (rc) \
            return rc; \
        rc = FUNCTION_NAME args;\
        epon_ae_unlock(&ae_reent_lock); \
        return rc;\
} while(0);
	
		
#define FUNCTION_WITH_TRYLOCK(FUNCTION_NAME,args) \
    do{\
        int rc;\
        rc = epon_ae_trylock(&ae_reent_lock); \
        if (rc) \
            return rc; \
        rc = FUNCTION_NAME args;\
        epon_ae_unlock(&ae_reent_lock); \
        return rc;\
} while(0);

int epon_ae_mac_cfg_set(mac_dev_t *mac_dev, mac_cfg_t *mac_cfg);

int epon_ae_mac_pause_set(mac_dev_t *mac_dev, int rx_enable, int tx_enable, char *src_addr);

int epon_ae_mac_pause_get(mac_dev_t *mac_dev, int *rx_enable, int *tx_enable);

int epon_ae_mac_locked(LaserRate rate);

int epon_ae_mac_rate_detect(LaserRate cur_rate);

int epon_ae_is_silent_start_enabled(void);

void epon_ae_set_silent_start_enable(int enable);

int epon_ae_is_silent_start_supported(void);

void epon_ae_mac_drv_exit(void);

#endif
