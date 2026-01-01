/***********************************************************************
 *
 *  Copyright (c) 2018 Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

/*****************************************************************************
*    Description:
*
*      This file implements the BCM rdpactl utility wrapper functions
*      with uni port index (a.k.a oam index) as input.
*      The oam index represents the physical Ethernet UNI port numbering
*      order on the CPE device face plate, the numbering is used by the
*      management protocols such as OMCI, EPON OAM, etc. A dynamically
*      created virtual interface or an internal interface does not have
*      OAM index. Typically, the OAM index value is 0-based.
*
*      This file is also a place holder for certain Ethernet-like
*      configurations on a non-Ethernet WAN interface. Such WAN interface is
*      not a real physical Ethernet port, and may not have a directly
*      associated object in the management data model. One example is the
*      MAC address look up configuration on the RDPA wan0 port in the
*      GPON configuration. For those functions, WAN type is used as input.
*
*****************************************************************************/

#ifndef _RDPACTL_WRAP_H_
#define _RDPACTL_WRAP_H_

/* ---- Include Files ----------------------------------------------------- */

#include "rdpactl_api.h"
#include "ethswctl_api.h"


/* ---- Constants and Types ----------------------------------------------- */


/* ---- Macro API definitions --------------------------------------------- */

/* #define CC_RDPACTL_DEBUG */

#if defined(CC_RDPACTL_DEBUG)
#define rdpaCtlTrace(fmt, arg...) \
  printf("[RDPACTL_WRAP] %s(): " fmt "\n", __FUNCTION__, ##arg)
#else
#define rdpaCtlTrace(fmt, arg...)
#endif

#define rdpaCtlGetPortParamRetOnErr(ifName, param, rc) \
  rc = rdpaCtl_get_port_param(ifName, &param); \
  if (rc < 0) \
  { \
      printf("rdpaCtl_get_port_param() failed, rdpaIf=%s\n", ifName); \
      return rc; \
  }

#define rdpaCtlSetPortParam(ifName, param, rc) \
  rc = rdpaCtl_set_port_param(ifName, &param); \
  if (rc < 0) \
  { \
      printf("rdpaCtl_set_port_param() failed, rdpaIf=%s\n", ifName); \
  }


/* ---- Variable Externs -------------------------------------------------- */


/* ---- Inline functions -------------------------------------------------- */
static inline int set_port_unknownmac_discard(const char *ifName, 
  unsigned char discard)
{
    rdpa_drv_ioctl_port_param_t param;
    int rc = 0;

    rdpaCtlGetPortParamRetOnErr(ifName, param, rc);

    if (discard == TRUE)
    {
        param.dal_miss_action = rdpa_forward_action_drop;
    }
    else
    {
        /* Let Linux perform the flooding. */
        param.dal_miss_action = rdpa_forward_action_host;
    }

    rdpaCtlSetPortParam(ifName, param, rc);
    return rc;
}

static inline int rdpaCtl_get_port_dal_miss_action_wrap(const char *ifName,
  rdpa_forward_action *act)
{
    int rc = 0;
    rc = rdpaCtl_get_port_dal_miss_action(ifName, act);
    return rc;
}

static inline int rdpaCtl_set_port_sal_miss_action_wrap(const char *ifName,
  rdpa_forward_action act)
{
    int rc = 0;
    rc = rdpaCtl_set_port_sal_miss_action(ifName, act);
    return rc;
}

static inline int rdpaCtl_set_port_dal_miss_action_wrap(const char *ifName,
  rdpa_forward_action act)
{
    int rc = 0;
    rc = rdpaCtl_set_port_dal_miss_action(ifName, act);
    return rc;
}

static inline int rdpaCtl_set_port_unknownmac_discard_wrap(const char *ifName,
  unsigned char discard)
{
    int rc = 0;
    rc = set_port_unknownmac_discard(ifName, discard);
    return rc;
}

/* WAN-side configuration. */
static inline int rdpaCtl_set_wan_dal_miss_action_wrap(
  const char *ifName, rdpa_forward_action act)
{
    int rc = 0;
    rc = rdpaCtl_set_port_dal_miss_action(ifName, act);
    return rc;
}

static inline int rdpaCtl_get_wan_dal_miss_action_wrap(
  const char *ifName, rdpa_forward_action *act)
{
    int rc = 0;
    rc = rdpaCtl_get_port_dal_miss_action(ifName, act);
    return rc;
}

#endif /* _RDPACTL_WRAP_H_ */
