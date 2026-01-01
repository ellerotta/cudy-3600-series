/*
#
#  Copyright 2011, Broadcom
#
# <:label-BRCM:2011:proprietary:standard
#
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
#
#  Except as expressly set forth in the Authorized License,
#
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
#
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
#
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_lan.h"
#include "rut_upnp.h"
#include "rut_network.h"
#include "rut_virtualserver.h"
#include "rut_pmap.h"
#include "rut_wan.h"
#include "rut_ipsec.h"
#include "prctl.h"
#include "beep_networking.h"

extern void rutIpt_initFw(const char *ifName, UBOOL8 isWan);
extern void rutIpt_removeFw(const char *ifName);

/*
 * directory prefix is: /lib/modules/"kernel_version"/
 */
#define NF_DIR "kernel/net/netfilter"
#define IPV4_NF_DIR "kernel/net/ipv4/netfilter"
#define BCM_NF_DIR "bcmkernel/net/netfilter"
#ifdef SUPPORT_IPSEC_PASSTHROUGH
#define BRCM_NF_DIR "ipv4/netfilter/broadcom"
#endif //SUPPORT_IPSEC_PASSTHROUGH
#define BR_NF_DIR "kernel/net/bridge"

#ifdef SUPPORT_IPV6
#define IPV6_NF_DIR "kernel/net/ipv6/netfilter"
#endif

#ifdef SUPPORT_NF_TABLES
#define BRIDGE_NF_DIR "kernel/net/bridge/netfilter"
#endif

#define MAX_NAME_LEN 32
#define MAX_DEP_NUM  8
#define MAX_DIR_LEN  32


typedef enum
{
   x_tables=1,
   nf_defrag_ipv4,
   nf_conntrack,
   nfnetlink,
   xt_tcpudp,
   xt_multiport,
#ifdef SUPPORT_NF_FIREWALL
   xt_conntrack,
   nf_conncount,
   xt_connlimit,
#endif
#ifdef SUPPORT_NF_MANGLE
   xt_dscp,
   xt_DSCP,
   xt_mac,
   xt_mac_extend,
   xt_flowlabel,
   xt_u32,
   xt_iprange,
   xt_esp,
   xt_hashlimit,
   xt_mark,
   xt_scs,
   xt_mscs,
   xt_mark_extend,
   xt_FSMARK,
#endif // SUPPORT_NF_MANGLE
#ifdef SUPPORT_NF_FIREWALL
   xt_limit,
#endif
   xt_addrtype,
   xt_pkttype,
   xt_blog,
#ifdef SUPPORT_IPSET
   xt_set,
#endif
   xt_TCPMSS,
   xt_SKIPLOG,
   nfnetlink_queue,
   xt_state,
   ip_tables,
#ifdef SUPPORT_NF_NAT
#if defined(BCM_KERNEL_VER_LINUX419)
   nf_nat,      /* nat ipv4 */
   nf_nat_core, /* nat common driver for kernel v4.19 */
#else
   nf_nat, /* nat common driver for new kernel v5.15 */
#endif
   iptable_nat,
#endif // SUPPORT_NF_NAT
   iptable_filter,
#ifdef SUPPORT_NF_MANGLE
   iptable_mangle,
#endif
   iptable_raw,
#ifdef SUPPORT_NF_FIREWALL
   xt_LOG,
#if defined(BCM_KERNEL_VER_LINUX419)
   nf_log_common,
   nf_log_ipv4,
#else
   nf_log_syslog,
#endif
#endif
#ifdef SUPPORT_NF_NAT
   xt_nat,
#if defined(BCM_KERNEL_VER_LINUX419)
   ipt_MASQUERADE,
#else
   xt_MASQUERADE,
#endif
   xt_REDIRECT,
   nf_conntrack_ftp,
   nf_conntrack_h323,
   nf_conntrack_irc,
   nf_conntrack_tftp,
#ifdef SUPPORT_SIP
   nf_conntrack_sip,
#endif
   nf_conntrack_rtsp,
   nf_nat_ftp,
   nf_nat_tftp,
   nf_nat_irc,
   nf_nat_h323,
#ifdef SUPPORT_SIP
   nf_nat_sip,
#endif
   nf_nat_rtsp,
#if defined(BCM_KERNEL_VER_LINUX419)
#if defined(SUPPORT_PPTP) || defined(SUPPORT_GRE_TUNNEL)
   nf_conntrack_proto_gre,
   nf_nat_proto_gre,
#endif
#endif
#ifdef SUPPORT_PPTP
   nf_conntrack_pptp,
   nf_nat_pptp,
#endif
#ifdef SUPPORT_IPSEC_PASSTHROUGH
   nf_conntrack_proto_esp,
   nf_nat_proto_esp,
   nf_conntrack_ipsec,
   nf_nat_ipsec,
#endif //SUPPORT_IPSEC_PASSTHROUGH
#endif // SUPPORT_NF_NAT

#ifdef SUPPORT_IPV6
   xt_sctp,
   xt_NETMAP,
#ifdef SUPPORT_NF_NAT
#if defined(BCM_KERNEL_VER_LINUX419)
   nf_nat_ipv6,
#endif
   ip6table_nat,
#endif // SUPPORT_NF_NAT
   ip6_tables,
   ip6table_filter,
#ifdef SUPPORT_NF_MANGLE
   ip6table_mangle,
   ip6t_NPT,
#endif
   ip6t_REJECT,
   nf_defrag_ipv6,
#if defined(BCM_KERNEL_VER_LINUX419)
#ifdef SUPPORT_NF_FIREWALL
   nf_log_ipv6,
#endif
#endif
#endif //SUPPORT_IPV6
#if defined(SUPPORT_CONNTRACK_TOOLS) || defined(SUPPORT_DPI)
   nf_conntrack_netlink,
#endif
#if defined(SUPPORT_OPENPLAT)
   br_netfilter,
#endif
#ifdef SUPPORT_NF_TABLES
#if defined(BCM_KERNEL_VER_LINUX419)
   nf_tables,
   nf_tables_set,
   nft_limit,
   nft_counter,
   nft_ct,
   nft_log,
   nft_nat,
   nft_masq,
   nft_reject,
   nft_reject_inet,   
   nft_reject_ipv4,
   nft_reject_ipv6,
   nft_reject_bridge,         
   nft_compat,
   nft_chain_nat_ipv4,
   nft_chain_nat_ipv6,
   nft_masq_ipv4,
   nft_masq_ipv6,
   nft_chain_route_ipv4,
   nft_chain_route_ipv6,
   nft_bcm_extend,
#else  // kernel 515
   nf_tables,
   nft_limit,
   nft_counter,
   nft_ct,
   nft_log,
   nft_nat,
   nft_masq,
   nft_reject,
   nft_compat,
   nft_chain_nat,
   nft_bcm_extend,
#endif   
#endif
}moduleEnum;


typedef int (*mod_callback_t)(void);
typedef struct
{
   moduleEnum moduleIdx;            /**< module index defined in moduleEnum above */
   char moduleName[MAX_NAME_LEN];   /**< module name */
   moduleEnum depends[MAX_DEP_NUM]; /**< dependency module index list */
   char directory[MAX_DIR_LEN];     /**< the directory the module resides */
   char *options;                   /**< optional parameters for the module */
   UBOOL8 isStatic;                 /**< if TRUE, the module is static nad will not do an insmod */
   mod_callback_t postinsert_cb;   /**< optional callback fucntion after module insert */
}MODULE_INFO, *PMODULE_INFO;

static CmsRet insertModulesWithDep(char *name, moduleEnum Idx, moduleEnum *depends);
static PMODULE_INFO findModule(moduleEnum Idx, PMODULE_INFO modules);
UBOOL8 isModuleInserted(char *moduleName);
static CmsRet insertModuleByIndex(moduleEnum Idx);
void insertModuleByName(char *moduleName);

int nf_conntrack_cb(void)
{
    rut_doSystemAction("insertModuleByIndex", "echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal");
    return 0;
}

#if defined(SUPPORT_CONNTRACK_TOOLS) || defined(SUPPORT_DPI)
int nf_conntrack_netlink_cb(void)
{
#if defined(SUPPORT_DPI)
    rut_doSystemAction("DPI conntrack",  "echo 1 > /proc/sys/net/netfilter/nf_conntrack_acct");
#endif
    return 0;
  }
#endif

/*
 * If you want to insert more modules, please do the follows.
 * 1. give an entry in moduleEnum
 * 2. give the directory that the module resides in
 * 3. Removing modules depends on the orders in this table(reverse direction).
 *     Please find the the dependencies of the module first.
 *     If the dependencies are the superset of an existing module, insert it AFTER the existing one.
 *     If the dependencies are the subset of an existing module, insert it BEFORE the existing one.
 *
 * NOTE: For a static module, make sure to do the following 2 steps:
 *      1). Replace the module build in gendefconfig from "m" to "y".
 *      2). change the FALSE to TRUE on the isStatic field (the last parameter) on
 *         the corresponding module below.
 */

static MODULE_INFO modules[] =
{
   {x_tables, "x_tables", {0}, {NF_DIR}, NULL, TRUE, NULL},
   {nf_defrag_ipv4, "nf_defrag_ipv4", {0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_FIREWALL
#ifdef SUPPORT_IPV6
   {nf_conntrack, "nf_conntrack", {nf_defrag_ipv4, nf_defrag_ipv6, 0}, {NF_DIR}, "nf_conntrack_helper=1", FALSE, nf_conntrack_cb},
#else
   {nf_conntrack, "nf_conntrack", {nf_defrag_ipv4, 0}, {NF_DIR}, "nf_conntrack_helper=1", FALSE, nf_conntrack_cb},
#endif
#endif
   {nfnetlink, "nfnetlink", {0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_tcpudp, "xt_tcpudp", {x_tables, 0}, {NF_DIR}, NULL, TRUE, NULL},
   {xt_multiport, "xt_multiport", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_FIREWALL
   {xt_conntrack, "xt_conntrack", {x_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_conncount, "nf_conncount", {x_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_connlimit, "xt_connlimit", {x_tables, nf_conntrack, nf_conncount, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
#ifdef SUPPORT_NF_MANGLE
   {xt_dscp, "xt_dscp", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_DSCP, "xt_DSCP", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_mac, "xt_mac", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_mac_extend, "xt_mac_extend", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {xt_flowlabel, "xt_flowlabel", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {xt_u32, "xt_u32", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_iprange, "xt_iprange", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_esp, "xt_esp", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_hashlimit, "xt_hashlimit", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_mark, "xt_mark", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_mark_extend, "xt_mark_extend", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {xt_scs,  "xt_scs",  {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {xt_mscs, "xt_mscs", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {xt_FSMARK, "xt_FSMARK", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#endif
#ifdef SUPPORT_NF_FIREWALL
   {xt_limit, "xt_limit", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
   {xt_addrtype, "xt_addrtype", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_pkttype, "xt_pkttype", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_blog, "xt_blog", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_IPSET
   {xt_set, "xt_set", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
   {xt_TCPMSS, "xt_TCPMSS", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_SKIPLOG, "xt_SKIPLOG", {x_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {nfnetlink_queue, "nfnetlink_queue", {nfnetlink, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_state, "xt_state", {x_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {ip_tables, "ip_tables", {x_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_NAT
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_nat_core, "nf_nat", {ip_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat, "nf_nat_ipv4", {nf_nat_core, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#else
   {nf_nat, "nf_nat", {ip_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
   {iptable_nat, "iptable_nat", {nf_nat, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#endif // SUPPORT_NF_NAT
   {iptable_filter, "iptable_filter", {ip_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_MANGLE
   {iptable_mangle, "iptable_mangle", {ip_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#endif
   {iptable_raw, "iptable_raw", {ip_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_FIREWALL
   {xt_LOG, "xt_LOG", {ip_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_log_common, "nf_log_common", {ip_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_log_ipv4, "nf_log_ipv4", {nf_log_common, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#else
   {nf_log_syslog, "nf_log_syslog", {ip_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
#endif
#ifdef SUPPORT_NF_NAT
   {xt_nat, "xt_nat", {iptable_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
   {ipt_MASQUERADE, "ipt_MASQUERADE", {nf_nat, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},

   {xt_REDIRECT, "xt_REDIRECT", {nf_nat_core, 0}, {NF_DIR}, NULL, FALSE, NULL},
#else
   {xt_MASQUERADE, "xt_MASQUERADE", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {xt_REDIRECT, "xt_REDIRECT", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
   {nf_conntrack_ftp, "nf_conntrack_ftp", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_conntrack_h323, "nf_conntrack_h323", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_conntrack_irc, "nf_conntrack_irc", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_conntrack_tftp, "nf_conntrack_tftp", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_SIP
   {nf_conntrack_sip, "nf_conntrack_sip",  {nf_conntrack, 0}, {NF_DIR}, "sip_direct_media=0", FALSE, NULL},
#endif
   {nf_conntrack_rtsp, "nf_conntrack_rtsp",  {nf_conntrack, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_nat_ftp, "nf_nat_ftp", {nf_nat_core, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_tftp, "nf_nat_tftp", {nf_nat_core, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_irc, "nf_nat_irc", {nf_nat_core, 0}, {NF_DIR}, NULL, FALSE, NULL},
#else
   {nf_nat_ftp, "nf_nat_ftp", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_tftp, "nf_nat_tftp", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_irc, "nf_nat_irc", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
   {nf_nat_h323, "nf_nat_h323", {nf_nat, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_SIP
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_nat_sip, "nf_nat_sip", {nf_nat_core, 0}, {NF_DIR}, NULL, FALSE, NULL},
#else
   {nf_nat_sip, "nf_nat_sip", {nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif
#endif
   {nf_nat_rtsp, "nf_nat_rtsp", {nf_nat, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
#if defined(SUPPORT_PPTP) || defined(SUPPORT_GRE_TUNNEL)
   {nf_conntrack_proto_gre, "nf_conntrack_proto_gre", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_proto_gre, "nf_nat_proto_gre", {nf_nat, nf_conntrack_proto_gre, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#endif
#endif
#ifdef SUPPORT_PPTP
   {nf_conntrack_pptp, "nf_conntrack_pptp", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_pptp, "nf_nat_pptp", {nf_nat, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
#endif
#ifdef SUPPORT_IPSEC_PASSTHROUGH
   {nf_conntrack_proto_esp, "nf_conntrack_proto_esp", {nf_conntrack, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_proto_esp, "nf_nat_proto_esp", {nf_nat, nf_conntrack_proto_esp, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {nf_conntrack_ipsec, "nf_conntrack_ipsec", {nf_conntrack, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
   {nf_nat_ipsec, "nf_nat_ipsec", {nf_nat, nf_conntrack_ipsec, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#endif //SUPPORT_IPSEC_PASSTHROUGH
#endif // SUPPORT_NF_NAT
#ifdef SUPPORT_IPV6
   {xt_sctp, "xt_sctp", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_NAT
   {xt_NETMAP, "xt_NETMAP", {nf_nat, x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_nat_ipv6, "nf_nat_ipv6", {nf_nat_core, nf_conntrack, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#endif
   {ip6table_nat, "ip6table_nat", {ip6_tables, iptable_nat, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#else
   {xt_NETMAP, "xt_NETMAP", {x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
#endif // SUPPORT_NF_NAT
   {ip6_tables, "ip6_tables", {x_tables, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {ip6table_filter, "ip6table_filter", {ip6_tables, iptable_filter, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#ifdef SUPPORT_NF_MANGLE
   {ip6table_mangle, "ip6table_mangle", {ip6_tables, iptable_mangle, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {ip6t_NPT, "ip6t_NPT", {iptable_mangle, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#endif
   {ip6t_REJECT, "ip6t_REJECT", {0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {nf_defrag_ipv6, "nf_defrag_ipv6", {0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#if defined(BCM_KERNEL_VER_LINUX419)
#ifdef SUPPORT_NF_FIREWALL
   {nf_log_ipv6, "nf_log_ipv6", {nf_log_common, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
#endif
#endif
#endif //SUPPORT_IPV6
#if defined(SUPPORT_CONNTRACK_TOOLS) || defined(SUPPORT_DPI)
   {nf_conntrack_netlink, "nf_conntrack_netlink", {nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, nf_conntrack_netlink_cb},
#endif
#if defined(SUPPORT_OPENPLAT)
   {br_netfilter, "br_netfilter", {x_tables, 0}, {BR_NF_DIR}, NULL, FALSE, NULL},
#endif
#ifdef SUPPORT_NF_TABLES
#if defined(BCM_KERNEL_VER_LINUX419)
   {nf_tables, "nf_tables", {0}, {NF_DIR}, NULL, FALSE, NULL},
   {nf_tables_set, "nf_tables_set", {nf_tables}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_limit, "nft_limit", {nf_tables}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_counter, "nft_counter", {nf_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_compat, "nft_compat", {nf_tables, x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_nat, "nft_nat", {nf_tables, nf_conntrack, nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_masq, "nft_masq", {nf_tables, nf_conntrack, nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_ct, "nft_ct", {nf_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_log, "nft_log", {nf_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_reject, "nft_reject", {nf_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_reject_inet, "nft_reject_inet", {nf_tables, nf_conntrack, nft_reject, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_reject_ipv4, "nft_reject_ipv4", {nf_tables, nf_conntrack, nft_reject, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
   {nft_reject_ipv6, "nft_reject_ipv6", {nf_tables, nf_conntrack, nft_reject, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {nft_reject_bridge, "nft_reject_bridge", {nf_tables, nf_conntrack, nft_reject, 0}, {BRIDGE_NF_DIR}, NULL, FALSE, NULL},      
   {nft_chain_nat_ipv4, "nft_chain_nat_ipv4", {nf_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
   {nft_chain_nat_ipv6, "nft_chain_nat_ipv6", {nf_tables, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {nft_masq_ipv4, "nft_masq_ipv4", {nf_tables, nft_masq, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
   {nft_masq_ipv6, "nft_masq_ipv6", {nf_tables, nft_masq, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {nft_chain_route_ipv4, "nft_chain_route_ipv4", {nf_tables, 0}, {IPV4_NF_DIR}, NULL, FALSE, NULL},
   {nft_chain_route_ipv6, "nft_chain_route_ipv6", {nf_tables, 0}, {IPV6_NF_DIR}, NULL, FALSE, NULL},
   {nft_bcm_extend, "nft_bcm_extend", {nf_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#else
   {nf_tables, "nf_tables", {0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_limit, "nft_limit", {nf_tables}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_counter, "nft_counter", {nf_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_compat, "nft_compat", {nf_tables, x_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_nat, "nft_nat", {nf_tables, nf_conntrack, nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_masq, "nft_masq", {nf_tables, nf_conntrack, nf_nat, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_ct, "nft_ct", {nf_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_log, "nft_log", {nf_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_reject, "nft_reject", {nf_tables, nf_conntrack, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_chain_nat, "nft_chain_nat", {nf_tables, 0}, {NF_DIR}, NULL, FALSE, NULL},
   {nft_bcm_extend, "nft_bcm_extend", {nf_tables, 0}, {BCM_NF_DIR}, NULL, FALSE, NULL},
#endif   
#endif
   {0, "", {0}, {NF_DIR}, NULL, FALSE, NULL}
};
CmsRet rutIpt_vrtsrvRunIptables(char action, char *device,  char *protocol, char *inPort, char *srvAddress, char *srvPort, char *remoteHost);


#ifdef DMP_X_BROADCOM_COM_SECURITY_1
static CmsRet rutIpt_ipFilterInRunIptables(char *ipver, char action, const char *ifName, char *protocol, char *src, char *sport, char *dst, char *dport);
#endif


/*
 * find the module in the MODULE_INFO table
 */
static PMODULE_INFO findModule(moduleEnum Idx, PMODULE_INFO pMods)
{
   PMODULE_INFO pMod = NULL;

   while( pMods->moduleIdx != 0 )
   {
      if( Idx == pMods->moduleIdx )
      {
         pMod = pMods;
         break;
      }

      pMods++;
   }

   return pMod;
}


/*
 * This is a recursive function and will find all the dependencies (defined in the global struction
 * modules) and call inserModulebyIndex to perform the actual "insmod"
 * There is possibilities the dependencies kmod isn't actually built, so it will return ERROR in
 * this condition to avoid spending to much time in the loop.
 */
static CmsRet insertModulesWithDep(char *name, moduleEnum Idx, moduleEnum *depends)
{
   PMODULE_INFO pMod;
   CmsRet ret = CMSRET_SUCCESS;

   if( isModuleInserted(name) )
   {
      return ret;
   }
   else
   {
      while( *depends != 0 )
      {
         if( (pMod = findModule(*depends, modules)) != NULL )
         {
            if ((ret = insertModulesWithDep(pMod->moduleName, pMod->moduleIdx, pMod->depends)) == CMSRET_SUCCESS)
            {
                depends++;
            }
            else
            {
                break;
            }
         }
         else
         {
            cmsLog_error("WRONG dependent module idx: %d\n", *depends);
            ret = CMSRET_INTERNAL_ERROR;
            break;
         }
      }

      if( (ret = insertModuleByIndex(Idx)) != CMSRET_SUCCESS )
         cmsLog_error("insmod %s.ko FAILED in insertModuleByIndex()\n", name);
   }

   return ret;
}


/* This function will insert the module if it is found in the modules array
* (global structure for defined iptable modules)
*/
static CmsRet insertModuleByIndex(moduleEnum Idx)
{
   PMODULE_INFO pMod;
   char moduleName[MAX_NAME_LEN+3], DirPrefix[BUFLEN_128];
   char DirPath[CMS_MAX_FULLPATH_LENGTH];
   struct utsname kernel;
   CmsRet ret = CMSRET_SUCCESS;

   /* if idx is not in the range or pMod can not be filled, just return error */
   if (!(Idx > 0) || (pMod = findModule(Idx, modules)) == NULL)
   {
      cmsLog_error("Invalid index %d", Idx);
      return CMSRET_INTERNAL_ERROR;
   }

   if (pMod->isStatic)
   {
      cmsLog_debug("%s is static and does not need to be inserted.", pMod->moduleName);
      return ret;
   }

   /* Try to insert the module */
   if (uname(&kernel) == -1)
   {
      cmsLog_error("Failed to get kernel version");
      return CMSRET_INTERNAL_ERROR;
   }
   snprintf(DirPrefix, sizeof(DirPrefix), "/lib/modules/%s/", kernel.release);
   snprintf(moduleName, sizeof(moduleName), "%s.ko", pMod->moduleName);
   snprintf(DirPath, sizeof(DirPath), "%s/%s", DirPrefix, pMod->directory);

#ifdef DESKTOP_LINUX
   cmsLog_debug("fake insert of module %s/%s", DirPath, moduleName);
#else
   {
       struct dirent *entry;
       DIR *dir;
       UBOOL8 found = FALSE;
       char cmdStr[CMS_MAX_FULLPATH_LENGTH*2];

       dir = opendir(DirPath);
       if (dir == NULL)
       {
          cmsLog_error("ERROR!! No such directory: %s\n", DirPath);
          return CMSRET_INTERNAL_ERROR;
       }

       while ( !found && (entry = readdir(dir)) != NULL )
       {
          if ( !cmsUtl_strcmp(entry->d_name, moduleName) )
          {
             if (pMod->options)
                 snprintf(cmdStr, sizeof(cmdStr), "insmod %s/%s %s", DirPath,
                        moduleName, pMod->options);
             else
                 snprintf(cmdStr, sizeof(cmdStr), "insmod %s/%s", DirPath, moduleName);

             rut_doSystemAction("insertModuleByIndex", cmdStr);
             found = TRUE;

             /* call back function to configure ant module specific settings */
             if(pMod->postinsert_cb)
                 pMod->postinsert_cb();
          }
       }

       closedir(dir);

       if ( !found )
       {
          cmsLog_error("Failed on module %s (%d)", moduleName, Idx);
          ret = CMSRET_INVALID_PARAM_VALUE;
       }
   }
#endif /* DESKTOP_LINUX */

   return ret;

}



/* If the moduleName is ready in the /proc/modules file,
* return TRUE.  Otherwise return FALSE.
*/
UBOOL8 isModuleInserted(char *moduleName)
{
   UBOOL8 isInserted=FALSE;
   char buf[BUFLEN_512];
   FILE* fs = fopen("/proc/modules", "r");

   if (fs != NULL)
   {
      while (fgets(buf, BUFLEN_512, fs) != NULL)
      {
         char *p = strtok(buf, " ");
         if ((p != NULL) && !cmsUtl_strcmp(p, moduleName))
         {
            isInserted = TRUE;
            break;
         }
      }
      fclose(fs);;
   }

   cmsLog_debug("%s is %s", moduleName, isInserted ? "in." : "not in.");

   return isInserted;

}



/* Insert the module by name.  It calls insertModulesWithDep to find depedency and
* does real insert in that function.  Also for some modules, it will do a '-F'
* for flush.
*/
void insertModuleByName(char *moduleName)
{
   SINT32 i=0;
   UBOOL8 found = FALSE;

   cmsLog_debug("moduleName=%s", moduleName);

   /*
    * find the module and the dependencies from
    * MODULE_INFO table and insmod them
    */
   while(!found && modules[i].moduleIdx != 0)
   {

      if( !cmsUtl_strcmp(moduleName, modules[i].moduleName) )
      {
         found = TRUE;
         if (!isModuleInserted(moduleName))
         {
            if (insertModulesWithDep(modules[i].moduleName, modules[i].moduleIdx, modules[i].depends) != CMSRET_SUCCESS)
            {
               return;
            }
#ifndef SUPPORT_NF_TABLES
            if (cmsUtl_strcmp(moduleName, "iptable_filter") == 0)
            {
               rut_doSystemAction("insertModuleByIndex", "iptables -w -t filter -F");
            }
            else if (cmsUtl_strcmp(moduleName, "iptable_nat") == 0)
            {
               rut_doSystemAction("insertModuleByIndex", "iptables -w -t nat -F");
            }
            else if (cmsUtl_strcmp(moduleName, "iptable_mangle") == 0)
            {
               rut_doSystemAction("insertModuleByIndex", "iptables -w -t mangle -F");
            }
#ifdef SUPPORT_IPV6
            else if (cmsUtl_strcmp(moduleName, "ip6table_filter") == 0)
            {
               rut_doSystemAction("insertModuleByIndex", "ip6tables -w -t filter -F");
            }
            else if (cmsUtl_strcmp(moduleName, "ip6table_mangle") == 0)
            {
               rut_doSystemAction("insertModuleByIndex", "ip6tables -w -t mangle -F");
            }
#endif
#endif /* SUPPORT_NF_TABLES */
         }
      }
      else
      {
         i++;
      }
   }

   if( !found )
   {
      cmsLog_error("ERROR!! %s.ko is not in module information list", moduleName);
   }

}


#ifdef SUPPORT_IPSET
CmsRet rutIpt_insertIpsetModule()
{
   char cmd[CMS_MAX_FULLPATH_LENGTH];
   struct utsname kernel;

   /* Try to insert the module */
   if (uname(&kernel) == -1)
   {
      cmsLog_error("Failed to get kernel version");
      return CMSRET_INTERNAL_ERROR;
   }
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_bitmap_ip.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_bitmap_port.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_hash_ip.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_hash_ipport.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_hash_ipportip.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_hash_mac.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_hash_net.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);
   snprintf(cmd, sizeof(cmd),
            "insmod /lib/modules/%s/kernel/net/netfilter/ipset/ip_set_list_set.ko 2>/dev/null",
            kernel.release);
   rut_doSystemAction("ipset", cmd);

   return CMSRET_SUCCESS;
}
#endif


#ifdef SUPPORT_URLFILTER
void rut_UrlFilterLoadModule(void)
{
   insertModuleByName("iptable_filter");
   insertModuleByName("iptable_nat");
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");
   insertModuleByName("xt_SKIPLOG");
   /* We will apply nfnetlink_queue instead of ip_queue later */
   insertModuleByName("nfnetlink_queue");
#ifdef SUPPORT_IPV6
   insertModuleByName("ip6table_filter");
#endif
}

#endif /*SUPPORT_URLFILTER*/

void rutIpt_qosLoadModule(void)
{
   static UBOOL8 isQosModsInserted = FALSE;
   if (isQosModsInserted == TRUE)
   {
       cmsLog_notice("Skip rutIpt_qosLoadModule() since it is done once.");
       return;
   }
#ifdef SUPPORT_NF_MANGLE
   insertModuleByName("xt_dscp");
   insertModuleByName("xt_DSCP");
   insertModuleByName("xt_mac");
   insertModuleByName("xt_mac_extend");
   insertModuleByName("xt_flowlabel");
   insertModuleByName("xt_u32");
   insertModuleByName("xt_iprange");
   insertModuleByName("xt_mark");
   insertModuleByName("xt_mark_extend");
   insertModuleByName("xt_scs");
   insertModuleByName("xt_mscs");
//   insertModuleByName("xt_FSMARK");
#endif
   insertModuleByName("xt_multiport");
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");

   insertModuleByName("iptable_filter");
#ifdef SUPPORT_NF_MANGLE
   insertModuleByName("iptable_mangle");
#endif
//   insertModuleByName("ipt_MARK");
//   insertModuleByName("ipt_IMQ");         /* NOT WORKING YET!! */
//   insertModuleByName("ipt_FTOS");        /* NOT WORKING YET!! */
#if defined(SUPPORT_IPV6) && defined(SUPPORT_NF_MANGLE)
   insertModuleByName("ip6table_mangle");
#endif
   insertModuleByName("nfnetlink");
#ifdef SUPPORT_IPSET
   rutIpt_insertIpsetModule();
   insertModuleByName("xt_set");
#endif
   isQosModsInserted = TRUE;
}  /* End of rutIpt_qosLoadModule() */

void rutIpt_McpdLoadModules(void)
{
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");
   insertModuleByName("xt_SKIPLOG");
   insertModuleByName("xt_TCPMSS");
   insertModuleByName("nfnetlink");
   insertModuleByName("nfnetlink_queue");
}

void rutIpt_BeepSecurityModule(void)
{
   insertModuleByName("nfnetlink");
   insertModuleByName("iptable_filter");
   insertModuleByName("xt_state");
   insertModuleByName("xt_mac");
   insertModuleByName("xt_mac_extend");
   insertModuleByName("xt_flowlabel");
   insertModuleByName("xt_u32");
   insertModuleByName("xt_iprange");
#ifdef SUPPORT_IPSET
   rutIpt_insertIpsetModule();
   insertModuleByName("xt_set");
#endif
}

void rutIpt_BeepPortMappingModule(void)
{
#if defined(SUPPORT_OPENPLAT)
   insertModuleByName("iptable_nat");
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");
   insertModuleByName("xt_addrtype");
   insertModuleByName("xt_nat");
#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("ipt_MASQUERADE");
#else
   insertModuleByName("xt_MASQUERADE");
#endif
   insertModuleByName("xt_REDIRECT");
   insertModuleByName("br_netfilter");
#endif
}

#ifndef SUPPORT_NF_TABLES
void rutIpt_createCWMPChain(void)
{
   char cmd[BUFLEN_256] = {0};

   cmsLog_debug("Create CWMP CHECK chain.");
   snprintf(cmd, sizeof(cmd), "iptables -N %s 2>/dev/null", CWMP_CHECK_CHAIN);
   rut_doSystemAction("cwmp_chk", cmd);
#ifdef SUPPORT_IPV6
   snprintf(cmd, sizeof(cmd), "ip6tables -N %s 2>/dev/null", CWMP_CHECK_CHAIN);
   rut_doSystemAction("cwmp_chk", cmd);
#endif
}
#endif /* !SUPPORT_NF_TABLES */

#ifdef SUPPORT_IPV6

void rutIpt_insertIpModules6(void)
{
   cmsLog_debug("insmod IPv6 modules");

   /* basic set of modules for iptables */
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");
   insertModuleByName("xt_SKIPLOG");
   insertModuleByName("xt_TCPMSS");
   insertModuleByName("nfnetlink");
#if defined(SUPPORT_CONNTRACK_TOOLS) || defined(SUPPORT_DPI)
   insertModuleByName("nf_conntrack_netlink");
#endif

   /* firewall modules */
#ifdef SUPPORT_NF_FIREWALL
   insertModuleByName("xt_conntrack");
   insertModuleByName("xt_state");
   insertModuleByName("xt_limit");
#endif

   /* other modules */
#ifdef SUPPORT_IPSET
   rutIpt_insertIpsetModule();
   insertModuleByName("xt_set");
#endif
   insertModuleByName("nfnetlink_queue");
   insertModuleByName("xt_NETMAP");
#ifdef SUPPORT_NF_NAT
#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("nf_nat_ipv6");
#endif
   insertModuleByName("ip6table_nat");
#endif // SUPPORT_NF_NAT
   insertModuleByName("ip6table_filter");
#ifdef SUPPORT_NF_MANGLE
   insertModuleByName("ip6table_mangle");
   insertModuleByName("ip6t_NPT");
#endif
#ifdef SUPPORT_NF_FIREWALL
   insertModuleByName("xt_LOG");

#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("nf_log_ipv6");
#endif
#endif

   /* to pass the CDR v6ALG in IPv6 only WAN */
   insertModuleByName("nf_conntrack_ftp");
   insertModuleByName("nf_conntrack_tftp");

   /* to pass the CDR v6RFC6092 in IPv6 only WAN */
   insertModuleByName("xt_sctp");
   insertModuleByName("ip6t_REJECT");

   cmsLog_debug("done");
}

#ifndef SUPPORT_NF_TABLES
void rutIpt_createRoutingChain6(void)
{
   char col[IP_TBL_COL_MAX][BUFLEN_40];
   char comment[BUFLEN_264];
   char line[BUFLEN_1024];
   char tblfile[BUFLEN_16];
   SINT32 count = 0;
   UBOOL8 found = FALSE;
   FILE* fs = NULL;

   rut_doSystemAction("rtchain", "ip6tables -w -N rtchain 2>/dev/null");

   sprintf(tblfile, "/var/tmp_file");

   /* Before inserting rtchain to FORWARD chain, check if it already exists */
   snprintf(line, sizeof(line), "ip6tables -w -n -L FORWARD -v --line-numbers > %s", tblfile);
   rut_doSystemAction("rtchain", line);

   if ((fs = fopen(tblfile, "r")) != NULL)
   {
      while (fgets(line, sizeof(line), fs))
      {
         /* read pass 2 header lines */
         if (count++ < 2)
         {
            continue;
         }

         /* the opt column is -- in ip6tables list */
         sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
                col[IP_TBL_COL_NUM], col[IP_TBL_COL_PKTS], col[IP_TBL_COL_BYTES],
                col[IP_TBL_COL_TARGET], col[IP_TBL_COL_PROT], col[IP_TBL_COL_OPT],
                col[IP_TBL_COL_IN], col[IP_TBL_COL_OUT], col[IP_TBL_COL_SRC],
                col[IP_TBL_COL_DST], comment);

         /* if rtchain is found */
         if (cmsUtl_strcmp(col[IP_TBL_COL_TARGET], "rtchain") == 0)
         {
            found = TRUE;
            break;
         }
      }

      fclose(fs);
      unlink(tblfile);
   }

   if (!found)
   {
      rut_doSystemAction("rtchain", "ip6tables -w -I FORWARD -j rtchain 2>/dev/null");
   }
}


void rutIpt_configRoutingChain6(const char *prefix, const char *ifName, UBOOL8 add)
{
   char col[IP_TBL_COL_MAX][BUFLEN_40];
   char comment[BUFLEN_32];
   char line[BUFLEN_1024];
   char tblfile[BUFLEN_16];
   SINT32 count = 0;
   SINT32 ret_count = 0;
   UBOOL8 ret_found = FALSE;
   UBOOL8 drop_found = FALSE;
   FILE* fs = NULL;

   cmsLog_debug("prefix=%s, ifname=%s, add<%d>", prefix, ifName, add);

   /*
    * In the future, we will support IPv6 interface group. So the ifName could be brx.
    * For each bridge, we have to validate the associated IPv6 prefix.
    * Take br0 as an example:
    * If prefixA/prefixB is at br0, we have to allow all packets with source IP within
    * prefixA/prefixB and block all packets with source IP outside of prefixA/prefixB.
    * Therefore, we append a rule at the end to drop all packets from br0. And we
    * insert the rules to allow packets with source IP within the prefixes.
    * We need to be careful when deleting the rules. If a prefix is the last one
    * associated with br0, we MUST delete the br0 drop rule. Otherwise, all traffic from
    * br0 will be drop.
    */
   sprintf(tblfile, "/var/tmp_file");
   snprintf(line, sizeof(line), "ip6tables -w -n -L rtchain -v --line-numbers > %s", tblfile);
   rut_doSystemAction("rtchain", line);

   if ((fs = fopen(tblfile, "r")) != NULL)
   {
      while (fgets(line, sizeof(line), fs))
      {
         /* read pass 2 header lines */
         if (count++ < 2)
         {
            continue;
         }

         /* the opt column is -- in ip6tables list */
         sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
                col[IP_TBL_COL_NUM], col[IP_TBL_COL_PKTS], col[IP_TBL_COL_BYTES],
                col[IP_TBL_COL_TARGET], col[IP_TBL_COL_PROT], col[IP_TBL_COL_OPT],
                col[IP_TBL_COL_IN], col[IP_TBL_COL_OUT], col[IP_TBL_COL_SRC],
                col[IP_TBL_COL_DST], comment);

         /* if chain rule for the given interface is found */
         if (cmsUtl_strcmp(col[IP_TBL_COL_IN], ifName) == 0)
         {
            if (cmsUtl_strcmp(col[IP_TBL_COL_TARGET], "RETURN") == 0)
            {
               ret_count++;
            }

            if (cmsUtl_strcmp(col[IP_TBL_COL_SRC], prefix) == 0)
            {
               ret_found = TRUE;
            }
            else if (cmsUtl_strcmp(col[IP_TBL_COL_TARGET], "DROP") == 0)
            {
               drop_found = TRUE;
            }
         }
      }

      fclose(fs);
      unlink(tblfile);
   }

   if (add)
   {
      if (!ret_found)
      {
         snprintf(line, sizeof(line), "ip6tables -w -I rtchain -i %s -s %s -j RETURN 2>/dev/null", ifName, prefix);
         rut_doSystemAction("rtchain", line);
      }

      if (!drop_found)
      {
         snprintf(line, sizeof(line), "ip6tables -w -A rtchain -i %s -o %s "
                                      "-s fe80::/10 -d fe80::/10 -j RETURN 2>/dev/null", ifName, ifName);
         rut_doSystemAction("rtchain", line);
         snprintf(line, sizeof(line), "ip6tables -w -A rtchain -i %s ! -d ff00::/8 -j DROP 2>/dev/null", ifName);
         rut_doSystemAction("rtchain", line);
      }
   }
   else
   {
      snprintf(line, sizeof(line), "ip6tables -w -D rtchain -i %s -s %s -j RETURN 2>/dev/null", ifName, prefix);
      rut_doSystemAction("rtchain", line);

   }
}
#else
void rutIpt_createRoutingChain6(void)
{
    char line[BUFLEN_512];
    char tblfile[BUFLEN_40];
    FILE* fs = NULL;
    UBOOL8 found = FALSE;
    
    sprintf(tblfile, "/var/tmp_rutIpt_createRoutingChain6");
    snprintf(line, sizeof(line), "nft list ruleset ip6 > %s", tblfile);
    rut_doSystemAction("rtchain", line);

    if ((fs = fopen(tblfile, "r")) != NULL) 
    {
      while (fgets(line, sizeof(line), fs)) 
      {
         if (cmsUtl_strstr(line, "rtchain") != NULL)
         {
            found = TRUE;
            break;
         }
      }
    }
    
    if(fs != NULL)
    {
        fclose(fs);
        unlink(tblfile);
    }
    
    if (found)
        return;
        
   rut_doSystemAction("rtchain", "nft add table ip6 ip6_filter 2>/dev/null");                
   rut_doSystemAction("rtchain", "nft 'add chain ip6 ip6_filter rtchain { type filter hook forward priority 0; }' 2>/dev/null");        
}

void rutIpt_configRoutingChain6(const char *prefix, const char *ifName, UBOOL8 add)
{
   char col[IP_TBL_COL_MAX][BUFLEN_40];
   char line[BUFLEN_1024];
   char tblfile[BUFLEN_40];
   char *pch = NULL;
   SINT32 count = 0;
   SINT32 ret_count = 0;
   UBOOL8 ret_found = FALSE;
   UBOOL8 drop_found = FALSE;
   FILE* fs = NULL;

   cmsLog_debug("prefix=%s, ifname=%s, add<%d>", prefix, ifName, add);

   /*
    * In the future, we will support IPv6 interface group. So the ifName could be brx.
    * For each bridge, we have to validate the associated IPv6 prefix.
    * Take br0 as an example:
    * If prefixA/prefixB is at br0, we have to allow all packets with source IP within
    * prefixA/prefixB and block all packets with source IP outside of prefixA/prefixB.
    * Therefore, we append a rule at the end to drop all packets from br0. And we
    * insert the rules to allow packets with source IP within the prefixes.
    * We need to be careful when deleting the rules. If a prefix is the last one
    * associated with br0, we MUST delete the br0 drop rule. Otherwise, all traffic from
    * br0 will be drop.
    */
   sprintf(tblfile, "/var/tmp_rutIpt_configRoutingChain6");
   snprintf(line, sizeof(line), "nft list ruleset ip6 > %s", tblfile);
   rut_doSystemAction("rtchain", line);

   if ((fs = fopen(tblfile, "r")) != NULL)
   {
   
      while (fgets(line, sizeof(line), fs)) 
      {
         if (cmsUtl_strstr(line, "rtchain") != NULL)
         {
            break;
         }
      }
      
      while (fgets(line, sizeof(line), fs))
      {
         /* read pass 1 header lines */
         if (count++ < 1)
         {
            continue;
         }

         if ((pch = cmsUtl_strstr(line, "iifname")) != NULL)
         {
            scanf(pch, "iifname %s", col[IP_TBL_COL_IN]);
         
            /* if chain rule for the given interface is found */
            if (cmsUtl_strcmp(col[IP_TBL_COL_IN], ifName) == 0)
            {
             
               if (cmsUtl_strstr(line, "return") != NULL)
               {
                  ret_count++;
               }
               
               if ((pch = cmsUtl_strstr(line, "saddr")) != NULL)
               {
                  scanf(pch, "iifname %s", col[IP_TBL_COL_SRC]);
               }
               
               if (cmsUtl_strcmp(col[IP_TBL_COL_SRC], prefix) == 0)
               {
                  ret_found = TRUE;
               }
               else if (cmsUtl_strstr(line, "drop") != NULL)
               {
                  drop_found = TRUE;
               }
            }   
         }
      }

      fclose(fs);
      unlink(tblfile);
   }

   if (add)
   {
      if (!ret_found)
      {
         snprintf(line, sizeof(line), "nft 'add rule ip6 ip6_filter rtchain iifname %s ip6 saddr %s return' 2>/dev/null", ifName, prefix);
         rut_doSystemAction("rtchain", line);
      }

      if (!drop_found)
      {
         snprintf(line, sizeof(line), "nft 'add rule ip6 ip6_filter rtchain iifname %s oifname %s ip6 saddr fe80::/10 ip6 daddr fe80::/10 return' 2>/dev/null", ifName, ifName);
         rut_doSystemAction("rtchain", line);
         snprintf(line, sizeof(line), "nft 'add rule ip6 ip6_filter rtchain iifname %s ip6 daddr != ff00::/8' drop 2>/dev/null", ifName);
         rut_doSystemAction("rtchain", line);
      }
   }
   else
   {
      snprintf(line, sizeof(line), " nft 'delete rule ip6 ip6_filter rtchain iifname %s ip6 saddr %s return' 2>/dev/null", ifName, prefix);
      rut_doSystemAction("rtchain", line);

   }
}
#endif /* SUPPORT_NF_TABLES*/

void rutIpt_mldRules(UBOOL8 add, const char *ifname)
{
   char cmd[256];
   if (add == TRUE)
   {
      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -D INPUT -i %s -p icmpv6 "
                   "--icmpv6-type 130 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -D FORWARD -i %s "
                   "-d FF00::0/8 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -I INPUT 1 -i %s -p icmpv6 "
                   "--icmpv6-type 130 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -I FORWARD 1 -i %s "
                   "-d FF00::0/8 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);
   }
   else if (add == FALSE)
   {
      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -D INPUT -i %s -p icmpv6 "
                   "--icmpv6-type 130 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "ip6tables -w -t filter -D FORWARD -i %s "
                   "-d FF00::0/8 -j ACCEPT 2>/dev/null",
                   ifname );
      rut_doSystemAction("mldRules", &cmd[0]);
   }
}
#endif   /* SUPPORT_IPV6 */


#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
void rutIpt_TCPMSSforIPTunnel(const char *wanIfName, UBOOL8 is6rd)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void *obj=NULL;
   UBOOL8 isPpp;
   UBOOL8 isVlan;
   char cmd[BUFLEN_128];
   SINT32 mss;
   CmsRet ret;

   cmsLog_debug("ifName=%s", wanIfName);

   ret = rutWan_getIpOrPppObjectByIfname(wanIfName, &iidStack, &obj, &isPpp);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ifName %s", wanIfName);
      return;
   }

   if (isPpp)
   {
      WanPppConnObject *pppObj = (WanPppConnObject *)obj;
      isVlan = (pppObj->X_BROADCOM_COM_VlanMuxID > 0)?1:0;
   }
   else
   {
      WanIpConnObject *ipObj = (WanIpConnObject *) obj;
      isVlan = (ipObj->X_BROADCOM_COM_VlanMuxID > 0)?1:0;
   }

   cmsObj_free(&obj);

   /*
    * Calculate mss:
    * IPv4(20) + IPv6(40) + TCP(20) = 80
    * Vlan(4)  PPPoE(8)
    */
   if (isPpp && isVlan)
   {
      mss = 1500 - 80 - 4 - 8;
   }
   else if (isPpp)
   {
      mss = 1500 - 80 - 8;
   }
   else if (isVlan)
   {
      mss = 1500 - 80 - 4;
   }
   else
   {
      mss = 1500 - 80;
   }

   snprintf(cmd, sizeof(cmd), "%s -w -I FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %d", is6rd?"ip6tables":"iptables", mss);
   rut_doSystemAction("TCPMSSRules", cmd);
}
#endif   /* DMP_X_BROADCOM_COM_IPV6_1 aka SUPPORT_IPV6 */

#ifdef SUPPORT_NF_TABLES
void rutNft_insertModules()
{
   insertModuleByName("nf_tables");
#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("nf_tables_set");
   insertModuleByName("nf_log_ipv6");
#endif
   insertModuleByName("nft_limit");
   insertModuleByName("nft_counter");
   insertModuleByName("nft_ct");
   insertModuleByName("nft_log");
   insertModuleByName("nft_nat");
   insertModuleByName("nft_masq");
   insertModuleByName("nft_reject");
   insertModuleByName("nft_reject_inet");
   insertModuleByName("nft_reject_ipv4");
   insertModuleByName("nft_reject_ipv6");
   insertModuleByName("nft_reject_bridge");   
   insertModuleByName("nft_compat");
#if defined(BCM_KERNEL_VER_LINUX419)   
   insertModuleByName("nft_chain_nat_ipv4");
   //insertModuleByName("nft_chain_nat_ipv6");
   insertModuleByName("nft_masq_ipv4");
   //insertModuleByName("nft_masq_ipv6");
   insertModuleByName("nft_chain_route_ipv4");
   //insertModuleByName("nft_chain_route_ipv6");
#else
   insertModuleByName("nft_chain_nat");     
#endif

   insertModuleByName("nft_bcm_extend");

   // add default tables/chains
   rut_doSystemAction("rut", "nft add table inet filter");
   rut_doSystemAction("rut", "nft add table ip ip_filter");
   rut_doSystemAction("rut", "nft 'add chain inet filter Input { type filter hook input priority filter; policy accept; }'");
   rut_doSystemAction("rut", "nft 'add chain inet filter Forward { type filter hook forward priority filter; policy accept; }'");
   rut_doSystemAction("rut", "nft 'add chain inet filter Output { type filter hook output priority filter; policy accept; }'");
   rut_doSystemAction("rut", "nft 'add chain inet filter Mangle { type filter hook prerouting priority -150; policy accept; }'");

   rut_doSystemAction("rut", "nft add table ip nat");
   //rut_doSystemAction("rut", "nft 'add chain ip nat PREROUTING { type nat hook prerouting priority dstnat; policy accept; }'");
   //rut_doSystemAction("rut", "nft 'add chain ip nat INPUT { type nat hook input priority 100; policy accept; }'");
   //rut_doSystemAction("rut", "nft 'add chain ip nat POSTROUTING { type nat hook postrouting priority srcnat; policy accept; }'");
   //rut_doSystemAction("rut", "nft 'add chain ip nat OUTPUT { type nat hook output priority -100; policy accept; }' ");
   rut_doSystemAction("rut", "nft add table bridge br_filter");
   //rut_doSystemAction("rut", "nft 'add chain bridge br_filter Forward { type filter hook forward priority filter; policy accept; }'");
   
   return;
}
#endif

void rutIpt_insertIpModules(void)
{
   /* basic set of modules for iptables */
   insertModuleByName("iptable_filter"); //depends on x_tables, ip_tables
   insertModuleByName("xt_tcpudp");
   insertModuleByName("xt_blog");
   insertModuleByName("xt_SKIPLOG");
   insertModuleByName("xt_TCPMSS");
   insertModuleByName("nfnetlink");
#if defined(SUPPORT_CONNTRACK_TOOLS) || defined(SUPPORT_DPI)
   insertModuleByName("nf_conntrack_netlink");
#endif

   /* nat modules */
#ifdef SUPPORT_NF_NAT
   insertModuleByName("iptable_nat");    //depends on nf_conntrack, nf_conntrack_ipv4, nf_nat
   insertModuleByName("xt_nat");
#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("ipt_MASQUERADE");
#else
   insertModuleByName("xt_MASQUERADE");
#endif
   insertModuleByName("xt_REDIRECT");
#endif // SUPPORT_NF_NAT

   /* firewall modules */
#ifdef SUPPORT_NF_FIREWALL
   insertModuleByName("xt_conntrack");
   insertModuleByName("xt_state");
   insertModuleByName("xt_limit");
   insertModuleByName("xt_LOG");
   insertModuleByName("xt_connlimit");

#if defined(BCM_KERNEL_VER_LINUX419)
   insertModuleByName("nf_log_ipv4");
#else
   insertModuleByName("nf_log_syslog");
#endif
#endif

#ifdef SUPPORT_NF_MANGLE
   /* mangle modules */
   insertModuleByName("xt_mark");
   insertModuleByName("xt_mark_extend");
   insertModuleByName("xt_scs");
   insertModuleByName("xt_mscs");
//   insertModuleByName("xt_FSMARK");
   insertModuleByName("iptable_mangle");
#endif
//   insertModuleByName("ipt_ftos");              /* NOT WORKING YET!! */

   /* other modules */
   insertModuleByName("nfnetlink_queue");
#ifdef SUPPORT_IPSET
   rutIpt_insertIpsetModule();
   insertModuleByName("xt_set");
#endif

#ifdef SUPPORT_NF_NAT
#if defined(BCM_KERNEL_VER_LINUX419)
#if defined(SUPPORT_PPTP) || defined(SUPPORT_GRE_TUNNEL)
   /* for gre and pptp */
   insertModuleByName("nf_conntrack_proto_gre");
   insertModuleByName("nf_nat_proto_gre");
#endif
#endif
#ifdef SUPPORT_PPTP
   insertModuleByName("nf_conntrack_pptp");
   insertModuleByName("nf_nat_pptp");
#endif /* SUPPORT_PPTP */

   insertModuleByName("nf_conntrack_h323");

   /* if firewall is enabled but NAT is not, NAT related modules will not be inserted */
   if (rut_isAnyNatEnabled() == TRUE)
   {
      insertModuleByName("nf_conntrack_ftp");
      insertModuleByName("nf_conntrack_h323");
#ifdef SUPPORT_SIP
      insertModuleByName("nf_conntrack_sip");
#endif
      insertModuleByName("nf_conntrack_rtsp");
      insertModuleByName("nf_conntrack_irc");
      insertModuleByName("nf_conntrack_tftp");
      insertModuleByName("nf_nat_ftp");
      insertModuleByName("nf_nat_h323");
#ifdef SUPPORT_SIP
      insertModuleByName("nf_nat_sip");
#endif
      insertModuleByName("nf_nat_rtsp");
      insertModuleByName("nf_nat_irc");
      insertModuleByName("nf_nat_tftp");
#ifdef SUPPORT_IPSEC_PASSTHROUGH
      insertModuleByName("nf_conntrack_ipsec");
      insertModuleByName("nf_nat_ipsec");
      insertModuleByName("nf_conntrack_proto_esp");
      insertModuleByName("nf_nat_proto_esp");
#endif //SUPPORT_IPSEC_PASSTHROUGH
   }
#if (defined(CONFIG_BCM_INGQOS) || defined(CONFIG_BCM_INGQOS_MODULE))
   else
   {
#ifdef SUPPORT_SIP
      insertModuleByName("nf_conntrack_sip");
#endif
      insertModuleByName("nf_conntrack_rtsp");
   }
#endif
#endif // SUPPORT_NF_NAT

#ifdef SUPPORT_NF_TABLES
   rutNft_insertModules();
#endif
}

/***************************************************************************
// Function Name: rutIpt_isRedirectOnNAT(void)
// Description  : check for nat redirect for .
// Parameters   : none
// Returns      : 0 --> tcp port 21, 22, 23, 80 is redirected. -1 --> not redirected
****************************************************************************/
UBOOL8 rutIpt_isRedirectOnNAT(void)
{
   char col[11][32];
   char line[512];
   FILE* fs;
   SINT32 count = 0;
   UBOOL8 redirect = FALSE;

   rut_doSystemAction("rut", "iptables -w -t nat -L > /var/nat_redirect");

   fs = fopen("/var/nat_redirect", "r");
   if ( fs != NULL )
   {
      while ( fgets(line, sizeof(line), fs) )
      {
         /* read pass 3 header lines */
         if ( count++ < 3 )
            continue;
         sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
               col[0], col[1], col[2], col[3], col[4], col[5],
               col[6], col[7], col[8], col[9], col[10]);
        if ((cmsUtl_strcmp(col[0], "REDIRECT") == 0) && (cmsUtl_strcmp(col[1], "tcp") == 0) && (cmsUtl_strcmp(col[8], "ports") == 0))
        {
          if (cmsUtl_strcmp(col[9], "80") == 0 || cmsUtl_strcmp(col[9], "23") == 0 || cmsUtl_strcmp(col[9], "21") == 0 ||
              cmsUtl_strcmp(col[9], "22") == 0 || cmsUtl_strcmp(col[9], "69") == 0)
          {
            redirect = TRUE;
            break;
          }
        }
      }
      fclose(fs);
   }

   unlink("/var/nat_redirect");

   return redirect;

}



/* exexutes commands to remove DNS servers */
static void rut_unfakeDnsForwarding(void)
{
   char cmd[BUFLEN_256], addr[BUFLEN_128];

#ifdef todo_ppp_ipex
   char ipExt[BUFLEN_128];
   ipExt[0] = '\0';
   // get PPP IP extension mode
   bcmGetIpExtension(ipExt, BUFLEN_16);
   // do nothing if dns is already set or router is in PPP IP extension mode
   if ( glbInited == TRUE || atoi(ipExt) == 1 ) return;
#endif

   /* get local ip address */
   if(rut_getIfAddr("br0", addr) == FALSE)
      return ;

   /* setup dns forwarding with fake DNS server is the root IP address
    * to make PPP on demand can be waked up when there is traffic from LAN to WAN
    * No need to enable the following fake MAQUERADING, it could cause
    * an ASSURED (180s timeout) DNS query connectiontracking entry established between
    * the fake DNS 128.9.0.107 and ppp33's IP address during a short time window
    * between PPP is up and before the fake DNS relay rule's removal
    * sprintf(cmd, "iptables -w -t nat -D POSTROUTING -p ALL -j MASQUERADE");
    */

   snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i br0 -d %s -p udp --dport 53 -j DNAT --to 128.9.0.107 2>/dev/null", addr);
   rut_doSystemAction("unfakeDnsForwarding", cmd);

}


/*  exexutes commands to setup fake DNS servers */
static void rut_fakeDnsForwarding(void)
{
   char cmd[BUFLEN_256],  addr[BUFLEN_128];

   /* get local ip address */
   if(rut_getIfAddr("br0", addr) == FALSE)
      return ;


   /* setup dns forwarding with fake DNS server is the root IP address
    * to make PPP on demand can be waked up when there is traffic from LAN to WAN
    * No need to enable the following fake MAQUERADING, it could cause
    * an ASSURED (180s timeout) DNS query connectiontracking entry established between
    * the fake DNS 128.9.0.107 and ppp33's IP address during a short time window
    * between PPP is up and before the fake DNS relay rule's removal
    */

   /* remove it first */
   snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i br0 -d %s -p udp --dport 53 -j DNAT --to 128.9.0.107 2>/dev/null", addr);
   rut_doSystemAction("fakeDnsForwarding", cmd);

   snprintf(cmd, sizeof(cmd), "iptables -w -t nat -A PREROUTING -i br0 -d %s -p udp --dport 53 -j DNAT --to 128.9.0.107 2>/dev/null", addr);
   rut_doSystemAction("fakeDnsForwarding", cmd);
}


#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
static void rut_enableDNSForward(const char *bridgeName, const char *addr, const char *dns)
{
   char cmd[BUFLEN_256];

   /*
    * setup dns forwarding with real DNS server
    * addr is the LAN side ip address.
    * dns is the real DNS server ip address in the WAN.
    */
   snprintf(cmd, sizeof(cmd),
      "iptables -w -t nat -D PREROUTING -i %s -d %s -p udp --dport 53 -j DNAT --to %s 2>/dev/null",
      bridgeName, addr, dns);
   rut_doSystemAction("enableDnsForward", cmd);

   snprintf(cmd, sizeof(cmd), "iptables -w -t nat -A PREROUTING -i %s -d %s -p udp --dport 53 -j DNAT --to %s",
      bridgeName, addr, dns);
   rut_doSystemAction("enableDnsForward", cmd);

   return;
}
#endif

/* executes commands to setup NAT Masquerade for PPP on demand */
void rutIpt_initNatForPppOnDemand(char *ifcName)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];
   char localsubnet[CMS_IPADDR_LENGTH];
   char localsubnetmask[CMS_IPADDR_LENGTH];
   SINT32 retry = 0;

   if (!rut_getIfSubnet("br0", localsubnet)  || !rut_getIfMask("br0", localsubnetmask))
   {
      cmsLog_notice("No LAN info ?");
      return;
   }



   /* For ppp on demand, the ppp interface has to be up with a default gateway set over
   * it.  Need to wait the interface up in a loop for slow processors.
   */
   while (!cmsNet_isInterfaceUp(ifcName) && retry < INTERFACE_RETRY_COUNT)
   {
      usleep(USLEEP_COUNT);
      retry++;
   }
   if (retry >= INTERFACE_RETRY_COUNT)
   {
      cmsLog_error("Interface %s is not up after %d retries", ifcName, retry);
      return;
   }

   /* redirect LAN traffice to ppp interface */
   /* remove it first */
   rutIpt_deleteNatMasquerade(ifcName, localsubnet, localsubnetmask);
   rutIpt_insertNatMasquerade(ifcName, localsubnet, localsubnetmask);

   /* set fake Dns forwording iptables rule */
   rut_fakeDnsForwarding();

   /* set the default gateway to ppp interface */
   snprintf(cmd, sizeof(cmd), "route add default dev %s 2>/dev/null",  ifcName);
   rut_doSystemAction("rut", cmd);

}

/* executes commands to set default gateway for PPP on demand */
void rutIpt_setDefaultGwForPppOnDemand(char *ifcName)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];
   char localsubnet[CMS_IPADDR_LENGTH];
   char localsubnetmask[CMS_IPADDR_LENGTH];
   SINT32 retry = 0;

   if (!rut_getIfSubnet("br0", localsubnet)  || !rut_getIfMask("br0", localsubnetmask))
   {
      cmsLog_notice("No LAN info ?");
      return;
   }

   /* For ppp on demand, the ppp interface has to be up with a default gateway set over
   * it.  Need to wait the interface up in a loop for slow processors.
   */
   while (!cmsNet_isInterfaceUp(ifcName) && retry < INTERFACE_RETRY_COUNT)
   {
      usleep(USLEEP_COUNT);
      retry++;
   }
   if (retry >= INTERFACE_RETRY_COUNT)
   {
      cmsLog_error("Interface %s is not up after %d retries", ifcName, retry);
      return;
   }

   /* set the default gateway to ppp interface */
   snprintf(cmd, sizeof(cmd), "route add default dev %s 2>/dev/null",  ifcName);
   rut_doSystemAction("rut", cmd);

}

#ifndef SUPPORT_NF_TABLES
void rutIpt_deleteNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];

   cmsLog_debug("ifname=%s localSubnet=%s localSubnetMask=%s", ifName, localSubnet, localSubnetMask);

   /* Delete current private subnet rule */
   snprintf(cmd, sizeof(cmd),  "iptables -w -t nat -D POSTROUTING -o %s -s %s/%s -j MASQUERADE 2>/dev/null",
            ifName, localSubnet, localSubnetMask);
   rut_doSystemAction("deleteNatMasq", cmd);

   return;
}


void rutIpt_insertNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask)
{
   char cmd[CMS_IPADDR_LENGTH*2+CMS_IFNAME_LENGTH+BUFLEN_64];

   cmsLog_debug("ifName=%s localSubnet=%s localSubnetMask=%s", ifName, localSubnet, localSubnetMask);

   /* enable nat for the local private subnet only. */
   snprintf(cmd, sizeof(cmd),  "iptables -w -t nat -A POSTROUTING -o %s -s %s/%s -j MASQUERADE",
            ifName, localSubnet, localSubnetMask);
   rut_doSystemAction("insertNatMasq", cmd);

   return;
}
#endif

void rutIpt_initNat(const char *ifcName)
{
   char localSubnet[CMS_IPADDR_LENGTH];
   char localSubnetmask[CMS_IPADDR_LENGTH];

   /* mwang, 6/17/09: don't know why there is a fakeDnsForwarding and right below
    * is an unfakeDnsForwarding.  WHen do we fake it again?  This is for ppp on-demand?
    */
   rut_fakeDnsForwarding();

   /*
    * remove dns forwarding with fake DNS server 128.9.0.107
    * It must be done before setting up the Masquerading server
    * otherwise a DNS query conntracking entry will be established
    * using the fake DNS server
    */
   rut_unfakeDnsForwarding();


#ifdef DESKTOP_LINUX
   strcpy(localSubnet, "192.168.1.0");
   strcpy(localSubnetmask, "255.255.255.0");
#else
   if (!rut_getIfSubnet("br0", localSubnet) || !rut_getIfMask("br0", localSubnetmask))
   {
      cmsLog_error("failed to get subnet or netmask info for br0!");
      return;
   }
#endif

   rutIpt_deleteNatMasquerade(ifcName, localSubnet, localSubnetmask);
   rutIpt_insertNatMasquerade(ifcName, localSubnet, localSubnetmask);

   /*
    *  BcmNtwk_startNatTraveral; do it for Reaim code ?
    */

   /* execute iptables rules for port mapping feature */
   rutIpt_activatePortMappingEntries(ifcName);

   /* execute iptables rule for DMZ feature */
   rut_activateDmzRule();

   if (!cmsUtl_strncmp(ifcName, PPPOA_IFC_STR, strlen(PPPOA_IFC_STR)) ||
       !cmsUtl_strncmp(ifcName, IPOA_IFC_STR, strlen(IPOA_IFC_STR)))
   {
      /* pppoa or ipoa is not associated with a bridge */
      return;
   }

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   {
      char bridgeIfName[CMS_IFNAME_LENGTH];
      if ((rutPMap_getBridgeIfNameFromIfName(ifcName, bridgeIfName, TRUE)) == CMSRET_SUCCESS)
      {
         if (rutPMap_isWanUsedForIntfGroup(ifcName) == TRUE)
         {
            cmsLog_debug("%s is in a non-br0 interface group", ifcName);

            /* there is a problem when wan interface is assocated with br1 and
             * this wan interface is up before wan interface bond to br0, there will
             * be a problem when access to outside.  So always bond to br0 first
             * then br1 and lastly remove the bond to br0 later on in the next function
             */

            /* do the special NAT setting for this interface */
            rutIpt_initNatForIntfGroup(ifcName, bridgeIfName);
         }
         else
         {
            /* disassociate Other Bridges From WanIntf */
            if (rut_getIfSubnet(bridgeIfName, localSubnet) && rut_getIfMask(bridgeIfName, localSubnetmask))
            {
                rutIpt_disassociateOtherBridgesFromWanIntf('A', PF_INET, ifcName, localSubnet, localSubnetmask);
            }
         }
      }
      else
      {
         cmsLog_error("Fail to get bridge interface name for %s.", ifcName);
         return;
      }

      /* For BEEP security */
      {
         UINT32 key;
         if (qdmIntf_getIntfKeyByGroupName(BEEP_NETWORKING_GROUP_SECONDARY, &key) == CMSRET_SUCCESS)
         {
            sprintf(bridgeIfName, "br%d", key);
            rutIpt_beepNetworkingMasqueurade(bridgeIfName, ifcName);
         }
         else
         {
            cmsLog_debug("cannot find group name of BEEP_NETWORKING_GROUP_SECONDARY\n");
         }

         if (qdmIntf_getIntfKeyByGroupName(BEEP_NETWORKING_GROUP_WANONLY, &key) == CMSRET_SUCCESS)
         {
            sprintf(bridgeIfName, "br%d", key);
            rutIpt_beepNetworkingMasqueurade(bridgeIfName, ifcName);
         }
         else
         {
            cmsLog_debug("cannot find group name of BEEP_NETWORKING_GROUP_WANONLY\n");
         }
      }
   }
#endif

#ifdef DMP_DEVICE2_BRIDGE_1  /* aka SUPPORT_PORT_MAP */

   if (!IS_EMPTY_STRING(ifcName) && qdmIpIntf_isNatEnabledOnIntfNameLocked_dev2(ifcName))
   {
      char bridgeIfName[CMS_IFNAME_LENGTH];

      if(qdmIntf_getBridgeNameByIntfName(ifcName,bridgeIfName) == CMSRET_SUCCESS)
      {
         cmsLog_debug("ifname:%s, bridgeIfName=%s Nat:%d"
                      ,ifcName
                      ,bridgeIfName
                      ,(qdmIpIntf_isNatEnabledOnIntfNameLocked_dev2(ifcName)));

         rutIpt_configNatForIntfGroup_dev2(TRUE, ifcName, bridgeIfName);
      }
   }

   /* For BEEP security */
   {
      UINT32 key;
      char bridgeIfName[CMS_IFNAME_LENGTH];
      if (qdmIntf_getIntfKeyByGroupName(BEEP_NETWORKING_GROUP_SECONDARY, &key) == CMSRET_SUCCESS)
      {
         sprintf(bridgeIfName, "br%d", key);
         rutIpt_beepNetworkingMasqueurade(bridgeIfName, ifcName);
      }
      else
      {
         cmsLog_debug("cannot find group name of BEEP_NETWORKING_GROUP_SECONDARY\n");
      }

      if (qdmIntf_getIntfKeyByGroupName(BEEP_NETWORKING_GROUP_WANONLY, &key) == CMSRET_SUCCESS)
      {
         sprintf(bridgeIfName, "br%d", key);
         rutIpt_beepNetworkingMasqueurade(bridgeIfName, ifcName);
      }
      else
      {
         cmsLog_debug("cannot find group name of BEEP_NETWORKING_GROUP_WANONLY\n");
      }
   }
#endif
   return;
}


void rutIpt_initNatAndFirewallOnWanConnection(const char *ifName,
                                              UINT32 pppIdleDisconnectTime,
                                              UBOOL8 isNatEnabled,
                                              UBOOL8 isFirewallEnabled)
{

   cmsLog_debug("ifName=%s idleDisconnect=%d isNat=%d isFirewall=%d",
                ifName, pppIdleDisconnectTime, isNatEnabled, isFirewallEnabled);

   /* insert related modules if NAT/Firewall is enabled  */
   if (isNatEnabled || isFirewallEnabled)
   {
      /* if on demand is enabled, ip modules are already insmoded */
      if (pppIdleDisconnectTime == 0)
      {
         rutIpt_insertIpModules();
      }

      if (isNatEnabled)
      {
         rutIpt_initNat(ifName);
      }

#if (defined(CONFIG_BCM_INGQOS) || defined(CONFIG_BCM_INGQOS_MODULE))
      isFirewallEnabled = TRUE;
#endif
      if (isFirewallEnabled)
      {
         rutIpt_initFirewall(PF_INET, ifName);
         rutIpt_initFirewallExceptions(ifName);
      }

      /* set TCPMSS for this interface */
      rutIpt_insertTCPMSSRules(PF_INET, ifName);
   }

   cmsLog_debug("done");
   return;
}



#ifdef SUPPORT_IPV6
void rutIpt_setupFirewallForDHCPv6(UBOOL8 unblock, const char *ifName)
{
   char cmd[BUFLEN_128];
   char fmt[BUFLEN_128];

   /* To avoid adding duplicated rules, first delete the rule then add it. */

   strcpy(fmt, "ip6tables -w -%c INPUT -p tcp --dport 546 -i %s -j ACCEPT %s");
   snprintf(cmd, sizeof(cmd), fmt, 'D', ifName, "2>/dev/null");
   rut_doSystemAction("rut", cmd);
   if (unblock)
   {
      snprintf(cmd, sizeof(cmd), fmt, 'I', ifName, "");
      rut_doSystemAction("rut", cmd);
   }

   strcpy(fmt, "ip6tables -w -%c INPUT -p udp --dport 546 -i %s -j ACCEPT %s");
   snprintf(cmd, sizeof(cmd), fmt, 'D', ifName, "2>/dev/null");
   rut_doSystemAction("rut", cmd);
   if (unblock)
   {
      snprintf(cmd, sizeof(cmd), fmt, 'I', ifName, "");
      rut_doSystemAction("rut", cmd);
   }

   strcpy(fmt, "ip6tables -w -%c INPUT -p icmpv6 --icmpv6-type neighbor-solicitation -i %s -j ACCEPT %s");
   snprintf(cmd, sizeof(cmd), fmt, 'D', ifName, "2>/dev/null");
   rut_doSystemAction("rut", cmd);
   if (unblock)
   {
      snprintf(cmd, sizeof(cmd), fmt, 'I', ifName, "");
      rut_doSystemAction("rut", cmd);
   }

   strcpy(fmt, "ip6tables -w -%c INPUT -p icmpv6 --icmpv6-type neighbor-advertisement -i %s -j ACCEPT %s");
   snprintf(cmd, sizeof(cmd), fmt, 'D', ifName, "2>/dev/null");
   rut_doSystemAction("rut", cmd);
   if (unblock)
   {
      snprintf(cmd, sizeof(cmd), fmt, 'I', ifName, "");
      rut_doSystemAction("rut", cmd);
   }

   strcpy(fmt, "ip6tables -w -%c INPUT -p icmpv6 --icmpv6-type router-advertisement -i %s -j ACCEPT %s");
   snprintf(cmd, sizeof(cmd), fmt, 'D', ifName, "2>/dev/null");
   rut_doSystemAction("rut", cmd);
   if (unblock)
   {
      snprintf(cmd, sizeof(cmd), fmt, 'I', ifName, "");
      rut_doSystemAction("rut", cmd);
   }

}  /* End of rutIpt_setupFirewallForDHCPv6() */

void rutIpt_defaultLANSetup6(const char *ifName)
{
#ifndef SUPPORT_NF_TABLES
   insertModuleByName("ip6table_filter");
   insertModuleByName("xt_pkttype");
#else
   rutIpt_insertIpModules();
#endif   
   rutIpt_createRoutingChain6();
   /* empty prefix case */
   rutIpt_configRoutingChain6("", ifName, TRUE);
}
#endif


void rutIpt_initFirewallExceptions_igd(const char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   _IpFilterCfgObject *ipfilter = NULL;
   UBOOL8 found = FALSE;
   UBOOL8 isOnWanSide = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, ifName=%s", ifName);

   /*
    * If this firewall is for a LAN side interface, we do things a bit differently.
    * LAN side is determined if ifName starts with br.  This may not be the best way.
    */
   isOnWanSide = cmsUtl_strncmp(ifName, "br", 2);

   /*
    *iptables rule for IP firewall exception feature (Incoming traffic)
    */
   if (isOnWanSide)
   {
      _WanPppConnObject *wan_ppp_con = NULL;
      _WanIpConnObject *wan_ip_con = NULL;
      _WanPppConnFirewallExceptionObject *ppp_firewall_obj = NULL;
      _WanIpConnFirewallExceptionObject *ip_firewall_obj = NULL;

      /* For PPP connection */
      while (!found && cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &wan_ppp_con) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(ifName, wan_ppp_con->X_BROADCOM_COM_IfName) == 0)
         {
            found = TRUE;
            while (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN_FIREWALL_EXCEPTION, &iidStack,
                                           &iidStack1, (void **) &ppp_firewall_obj) == CMSRET_SUCCESS)
            {
               ret = cmsObj_set((void *)ppp_firewall_obj, &iidStack1);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Set of _WanPppConnFirewallExceptionObject failed, ret=%d", ret);
               }
               cmsObj_free((void **) &ppp_firewall_obj);
            }
         }

         cmsObj_free((void **) &wan_ppp_con);
      }


      /* For IP connection */
      INIT_INSTANCE_ID_STACK(&iidStack);
      INIT_INSTANCE_ID_STACK(&iidStack1);

      while (!found && cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &wan_ip_con) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(ifName, wan_ip_con->X_BROADCOM_COM_IfName) == 0)
         {
            found = TRUE;
            while (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION, &iidStack,
                                           &iidStack1, (void **) &ip_firewall_obj) == CMSRET_SUCCESS)
            {
               ret = cmsObj_set((void *)ip_firewall_obj, &iidStack1);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Set of _WanIpConnFirewallExceptionObject failed, ret=%d", ret);
               }
               cmsObj_free((void **) &ip_firewall_obj);
            }
         }

         cmsObj_free((void **) &wan_ip_con);
      }
   }
   else
   {
      /* do firewall exception on LAN side */
      INIT_INSTANCE_ID_STACK(&iidStack);
      INIT_INSTANCE_ID_STACK(&iidStack1);
      _LanIpIntfObject *ipIntfObj=NULL;
      _LanIpIntfFirewallExceptionObject *exceptionObj=NULL;

      while (!found && cmsObj_getNext(MDMOID_LAN_IP_INTF, &iidStack, (void **) &ipIntfObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(ifName, ipIntfObj->X_BROADCOM_COM_IfName) == 0)
         {
            found = TRUE;
            while (cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF_FIREWALL_EXCEPTION, &iidStack,
                                           &iidStack1, (void **) &exceptionObj) == CMSRET_SUCCESS)
            {
               ret = cmsObj_set((void *)exceptionObj, &iidStack1);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Set of _LanIpIntfFirewallExceptionObject failed, ret=%d", ret);
               }
               cmsObj_free((void **) &exceptionObj);
            }
         }

         cmsObj_free((void **) &ipIntfObj);
      }
   }

   /*
    *iptables rule for IP filter feature (Outgoing traffic)
    * Hmm, this block is executed everytime initFirewall gets called.
    * But initFirewall can be called multiple times with different ifNames.
    * Wouldn't that end up creating multiple rules from the IP_FILTER_CFG?
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while(cmsObj_getNext(MDMOID_IP_FILTER_CFG, &iidStack, (void **) &ipfilter) == CMSRET_SUCCESS)
   {
      ret = cmsObj_set((void *)ipfilter, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of _IpFilterCfgObject failed, ret=%d", ret);
      }
      cmsObj_free((void **) &ipfilter);
   }

   cmsLog_debug("done");

}  /* End of rutIpt_initFirewallExceptions_igd() */


#ifdef SUPPORT_NF_MANGLE
#ifdef SUPPORT_IPSEC
#ifndef SUPPORT_NF_TABLES
void rutIpt_initIpSecPolicy(const char *ifName)
{
   char cmd[BUFLEN_256];

   // allow isakmp
   snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -p udp --dport 500 -i %s -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // allow esp
   snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -p esp -i %s -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // allow ah
   snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -p ah -i %s -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // mark all esp/ah packets
   snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -p esp -A PREROUTING -i %s -j MARK --set-mark 0x10000000", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -p ah -A PREROUTING -i %s -j MARK --set-mark 0x10000000", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // marked non-esp packets are the output from 2.6 kernel ipsec code, accept
   // (ipsec output re-enters ip chains, mark is preserved when encapsulated ip packet in esp is extracted)
   snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT ! -p esp -i %s -m mark --mark 0x10000000/0x10000000 -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);
   snprintf(cmd, sizeof(cmd), "iptables -w -A FORWARD ! -p esp -i %s  -m mark --mark 0x10000000/0x10000000 -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT ! -p ah -i %s -m mark --mark 0x10000000/0x10000000 -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);
   snprintf(cmd, sizeof(cmd), "iptables -w -A FORWARD ! -p ah -i %s  -m mark --mark 0x10000000/0x10000000 -j ACCEPT", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   return;
}

void rutIpt_removeIpSecPolicy(const char *ifName)
{
   char cmd[BUFLEN_256];

   // mark all esp packets
   snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -p esp -D PREROUTING -i %s -j MARK --set-mark 0x10000000 2>/dev/null", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   // mark all ah packets
   snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -p ah -D PREROUTING -i %s -j MARK --set-mark 0x10000000 2>/dev/null", ifName);
   rut_doSystemAction("ipsecPolicy", cmd);

   return;
}
#endif
#endif /* SUPPORT_IPSEC */
#endif // SUPPORT_NF_MANGLE

void rutIpt_initFirewall(SINT32 domain, const char *ifName)
{
#ifndef SUPPORT_NF_TABLES
   char cmd[BUFLEN_256];
#endif
   char ipt[BUFLEN_16];
   UBOOL8 isOnWanSide = FALSE;

   sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");

   cmsLog_debug("entered, ipt=%s ifName=%s", ipt, ifName);

   /*
    * If this firewall is for a LAN side interface, we do things a bit differently.
    * LAN side is determined if ifName starts with br.  This may not be the best way.
    */
   isOnWanSide = cmsUtl_strncmp(ifName, "br", 2);

   if (domain == PF_INET)
   {
      /* Enable anti-IPspoofing */
      rutIpt_activateFirewallAntiIPspoofing(ifName);
   }

   /* The following rules are mainly handling INPUT and FORWARD chain
    * setup stateful packet inspection
    */

#ifndef SUPPORT_NF_TABLES
#ifdef SUPPORT_NF_FIREWALL
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -p ALL -i %s -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT", ipt, ifName);
   rut_doSystemAction("initFirewall", cmd);

   if (isOnWanSide)
   {
      snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -p ALL -i %s -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);
   }
#endif

   /* TODO:  setup RIP hole if RIP on WAN is enabled
    * doRipPolicy(ifName);
    * setup messenger proxy hole if NAT is enabled.
    * doMessengerProxyPolicy(ifName);  reaim stuff
    * doIPSecPolicy(ifName);
    */

#ifdef SUPPORT_NF_MANGLE
#ifdef SUPPORT_IPSEC
   rutIpt_initIpSecPolicy(ifName);
#endif
#endif

#ifdef SUPPORT_TR69C
#ifdef DMP_DEVICE2_BASELINE_1  // only TR181 support EnableCWMP parameter
   {
      Dev2ManagementServerObject *mgmtObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      CmsRet ret = CMSRET_SUCCESS;
      if ((ret = cmsObj_get(MDMOID_DEV2_MANAGEMENT_SERVER, &iidStack,
                     OGF_NO_VALUE_UPDATE, (void **)&mgmtObj)) == CMSRET_SUCCESS)
      {
         rutIpt_initCWMPChk(domain, mgmtObj->enableCWMP);
         cmsObj_free((void **)&mgmtObj);
      }
      else
      {
         cmsLog_error("Failed to get ManagementServerObject, ret=%d", ret);
      }
   }
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p tcp --dport %d -j %s", ipt, ifName, TR69C_CONN_REQ_PORT, CWMP_CHECK_CHAIN);
   rut_doSystemAction("initFirewall", cmd);
#else
   /* for tr69c_1 */
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p tcp --dport %d -j ACCEPT", ipt, ifName, TR69C_CONN_REQ_PORT);
   rut_doSystemAction("initFirewall", cmd);
#endif // DMP_DEVICE2_BASELINE_1

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)  //TODO: replace this rule when supporting EnableCWMP for 2nd tr69c
   /* for tr69c_2 */
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p tcp --dport %d -j ACCEPT", ipt, ifName, TR69C_2_CONN_REQ_PORT);
   rut_doSystemAction("initFirewall", cmd);
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
#endif // SUPPORT_TR69C

#if defined(DMP_UDPECHO_1) || defined(DMP_DEVICE2_UDPECHO_1)
   {
      UBOOL8 enable = FALSE;
      char fullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      UINT32 port=0;
      char intfName[CMS_IFNAME_LENGTH]={0};
      UINT32 l;

      qdmDiag_getUdpEchoCfg(&enable,fullPathBuf,&port);


      if (enable)
      {
         if (fullPathBuf[0])
         {
            /* ensure fullpath ends with . before conversion (not necessary?) */
            l = strlen(fullPathBuf);
            if ((fullPathBuf[l-1]) != '.' && l < sizeof(fullPathBuf) -1)
            {
               fullPathBuf[l] = '.';
               fullPathBuf[l + 1] = '\0';
            }

            if (qdmIntf_fullPathToIntfnameLocked(fullPathBuf, intfName) == CMSRET_SUCCESS)
            {
               snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p udp --dport %d -j ACCEPT", ipt, intfName, port);
            }
            else
            {
               cmsLog_error("Could not convert %s to intfName", fullPathBuf);
            }
         }
         else
         {
            snprintf(cmd, sizeof(cmd),"%s -w -A INPUT -p udp --dport %d -j ACCEPT", ipt, port);
         }

         rut_doSystemAction("initFirewall", cmd);
      }
   }
#endif /* (DMP_UDPECHO_1) || define(DMP_DEVICE2_UDPECHO_1) */

   /* Log malicious packets right before dropping them
    * setup intrusion detection, logging 6 packets per hour to avoid sync log flooding
    */
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p tcp --syn -m limit --limit 6/h -j LOG --log-level 1 --log-prefix=\"Intrusion -> \"", ipt, ifName);
   rut_doSystemAction("initFirewall", cmd);
   if (domain == PF_INET6)
   {
      snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p tcp --syn -m limit --limit 6/h -j REJECT --reject-with icmp6-adm-prohibited", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);

      /* logging 6 packets per hour to avoid init log flooding */
      snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p sctp --chunk-types only INIT -m limit --limit 6/h -j LOG --log-level 1 --log-prefix=\"Intrusion -> \"", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);
      snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -p sctp --chunk-types only INIT -m limit --limit 6/h -j REJECT --reject-with icmp6-adm-prohibited", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);
   }

   /* diable everything else */
   snprintf(cmd, sizeof(cmd), "%s -w -A INPUT -i %s -j DROP", ipt, ifName);
   rut_doSystemAction("initFirewall", cmd);

   if (isOnWanSide)
   {
      snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s -p tcp --syn -m limit --limit 6/h -j LOG --log-level 1 --log-prefix=\"Intrusion -> \"", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);
      if (domain == PF_INET6)
      {
         snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s -p tcp --syn -m limit --limit 6/h -j REJECT --reject-with icmp6-adm-prohibited", ipt, ifName);
         rut_doSystemAction("initFirewall", cmd);

         snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s -p sctp --chunk-types only INIT -m limit --limit 6/h -j LOG --log-level 1 --log-prefix=\"Intrusion -> \"", ipt, ifName);
         rut_doSystemAction("initFirewall", cmd);
         snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s -p sctp --chunk-types only INIT -m limit --limit 6/h -j REJECT --reject-with icmp6-adm-prohibited", ipt, ifName);
         rut_doSystemAction("initFirewall", cmd);
      }

      snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s -j DROP", ipt, ifName);
      rut_doSystemAction("initFirewall", cmd);
   }
#else // SUPPORT_NF_TABLES
   rutIpt_initFw(ifName, isOnWanSide);
#endif

   cmsLog_debug("done");

   return;
}

void rutIpt_initCWMPChk(SINT32 domain, SINT32 enable)
{
#ifndef SUPPORT_NF_TABLES
   char cmd[BUFLEN_256];
   char ipt[BUFLEN_16];

   sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");

   // Flush rules
   rutIpt_createCWMPChain();

   snprintf(cmd, sizeof(cmd)-1, "%s -w -F %s", ipt, CWMP_CHECK_CHAIN);
   rut_doSystemAction(CWMP_CHECK_CHAIN, cmd);

   if (enable) // add ACCEPT rule when cwmp enabled
   {
      snprintf(cmd, sizeof(cmd)-1, "%s -w -A %s -p tcp --dport %d -j ACCEPT", ipt, CWMP_CHECK_CHAIN, TR69C_CONN_REQ_PORT);
      rut_doSystemAction(CWMP_CHECK_CHAIN, cmd);
   }
#endif
}

#define RUTIPT_DELETE_BATCH  25

static void removeAllIptRulesByIntfName(SINT32 domain, const char *ifcName, const char *table, const char *chain)
{
   SINT32 numArray[RUTIPT_DELETE_BATCH];
   char col[IP_TBL_COL_MAX][BUFLEN_32];
   char comment[BUFLEN_264];
   char line[BUFLEN_1024];
   char tblfile[BUFLEN_64];
   char ipt[BUFLEN_16];
   UBOOL8 done = FALSE;
   SINT32 idx;
   FILE* fs = NULL;

   cmsLog_debug("Entered: domain=%d ifcName=%s table=%s chain=%s",
                domain, ifcName, table, chain);

   sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");
   sprintf(tblfile, "/var/%s", (domain == PF_INET)? "iptable" : "ip6table");


   while (!done)
   {
      /* start of a new batch: init array, get listing of rules */
      memset(numArray, 0, sizeof(numArray));
      idx=0;
      done = TRUE;  // assume we can finish in 1 pass, if not, we will set this to FALSE below


      /* execute iptables command to create iptable file */
      snprintf(line, sizeof(line), "%s -w -t %s -n -L %s -v --line-numbers > %s", ipt, table, chain, tblfile);
      rut_doSystemAction("rut", line);

      /* go through the file and remember which row nums we want to delete */
      if ((fs = fopen(tblfile, "r")) != NULL)
      {
         SINT32 lineCount=0;
         while (fgets(line, sizeof(line), fs))
         {
            /* read pass 2 header lines */
            if (lineCount++ < 2)
            {
               continue;
            }

            /* the opt column is -- in iptables/ip6tables list */
            sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
                   col[IP_TBL_COL_NUM], col[IP_TBL_COL_PKTS], col[IP_TBL_COL_BYTES],
                   col[IP_TBL_COL_TARGET], col[IP_TBL_COL_PROT], col[IP_TBL_COL_OPT],
                   col[IP_TBL_COL_IN], col[IP_TBL_COL_OUT], col[IP_TBL_COL_SRC],
                   col[IP_TBL_COL_DST], comment);

            /* if chain rule for the given interface is found */
            if (cmsUtl_strcmp(col[IP_TBL_COL_IN], ifcName) == 0 ||
                cmsUtl_strcmp(col[IP_TBL_COL_OUT], ifcName) == 0)
            {
               numArray[idx] = atoi(col[IP_TBL_COL_NUM]);
               cmsLog_debug("storing numArray[%d]=%d", idx, numArray[idx]);
               idx++;
               if (idx >= RUTIPT_DELETE_BATCH)
               {
                  /* This batch is full, delete this batch and go around
                   * to top again.
                   */
                  done = FALSE;
                  break;
               }
            }
         }  // while fgets

         fclose(fs);
         unlink(tblfile);
      }  // if fopen is OK

      /* move idx back to the last valid entry */
      idx--;

      /*
       * Now move backwards through the numArray and delete.  We have to
       * go backwards so the row numbers in the earlier rows do not change.
       */
      while (idx >= 0)
      {
         cmsLog_debug("Deleting numArray[%d]=%d", idx, numArray[idx]);
         snprintf(line, sizeof(line), "%s -w -t %s -D %s %d 2>/dev/null",
                  ipt, table, chain, numArray[idx]);
         rut_doSystemAction("rut", line);

         idx--;
      }

   } // end while (!done)

   cmsLog_debug("Exit");
}


void rutIpt_removeInterfaceIptableRules(const char *ifcName, UBOOL8 isIPv4)
{
   cmsLog_debug("Entered: ifcName=%s isIpv4=%d", ifcName, isIPv4);

#ifndef SUPPORT_NF_TABLES
   if (isIPv4)
   {
      if (isModuleInserted("iptable_filter"))
      {
         removeAllIptRulesByIntfName(PF_INET, ifcName, "filter", "INPUT");

         removeAllIptRulesByIntfName(PF_INET, ifcName, "filter", "FORWARD");

         removeAllIptRulesByIntfName(PF_INET, ifcName, "filter", "OUTPUT");
      }

      if (isModuleInserted("iptable_nat"))
      {
         removeAllIptRulesByIntfName(PF_INET, ifcName, "nat", "PREROUTING");

         /* Also delete the masquerading rules (?) */
         removeAllIptRulesByIntfName(PF_INET, ifcName, "nat", "POSTROUTING");

         removeAllIptRulesByIntfName(PF_INET, ifcName, "nat", "OUTPUT");
      }
   }
#ifdef SUPPORT_IPV6
   else
   {
      if (isModuleInserted("ip6table_filter"))
      {
         removeAllIptRulesByIntfName(PF_INET6, ifcName, "filter", "INPUT");

         removeAllIptRulesByIntfName(PF_INET6, ifcName, "filter", "FORWARD");

         removeAllIptRulesByIntfName(PF_INET6, ifcName, "filter", "OUTPUT");
      }
   }
#endif
#else
   {
      char cmd[BUFLEN_1024];

      // remove nat rule
      snprintf(cmd, sizeof(cmd),  "nft delete chain ip nat NatMasq_%s 2>/dev/null", ifcName );
      rut_doSystemAction("deleteNatMasq", cmd);

#ifdef SUPPORT_UPNP
      rutIpt_upnpConfigStopMulticast((char *)ifcName, FALSE);
#endif

      rutIpt_deleteTCPMSSRules(PF_INET, ifcName);
      rutIpt_removeFw(ifcName);
   }
#endif

#ifdef SUPPORT_NF_MANGLE
#ifdef SUPPORT_IPSEC
   rutIpt_removeIpSecPolicy(ifcName);
#endif
#endif

}

#ifdef SUPPORT_IPV6
void rutIpt_updateNPTv6Rules(UBOOL8 add, const char *ifname, const char *lanPrefix, const char *wanPrefix)
{
   char cmd[256];
   if (add == TRUE)
   {
      snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -I POSTROUTING -s %s -o %s -j SNPT --src-pfx %s --dst-pfx %s 2>/dev/null",
                   lanPrefix, ifname, lanPrefix, wanPrefix);
      rut_doSystemAction("NPTv6Rules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -I PREROUTING  -d %s -i %s -j DNPT --src-pfx %s --dst-pfx %s 2>/dev/null",
                   wanPrefix, ifname, wanPrefix, lanPrefix);
      rut_doSystemAction("NPTv6Rules", &cmd[0]);

   }
   else
   {
       snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -D POSTROUTING -s %s -o %s -j SNPT --src-pfx %s --dst-pfx %s 2>/dev/null",
               lanPrefix, ifname, lanPrefix, wanPrefix);
       rut_doSystemAction("NPTv6Rules", &cmd[0]);

       snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -D PREROUTING  -d %s -i %s -j DNPT --src-pfx %s --dst-pfx %s 2>/dev/null",
               wanPrefix, ifname, wanPrefix, lanPrefix);
       rut_doSystemAction("NPTv6Rules", &cmd[0]);
   }
}

void rutIpt_updateNetMapv6Rules(UBOOL8 add, const char *ifname, const char *lanPrefix, const char *wanPrefix)
{
   char cmd[256];
   if (add == TRUE)
   {
      if (lanPrefix == NULL || wanPrefix == NULL || ifname == NULL)
          return;

      snprintf(cmd, sizeof(cmd), "ip6tables -t nat -A POSTROUTING -o %s -p tcp --destination-port 21 -j NETMAP -s %s --to %s 2>/dev/null",
                   ifname, lanPrefix, wanPrefix);
      rut_doSystemAction("ALGv6Rules", &cmd[0]);
   }
   else if (add == FALSE)
   {
       if (lanPrefix == NULL || wanPrefix == NULL)
       {
           if (ifname)
           {
               removeAllIptRulesByIntfName(PF_INET6, ifname, "nat", "POSTROUTING");
           }

           return;
       }

       snprintf(cmd, sizeof(cmd), "ip6tables -t nat -D POSTROUTING -o %s -p tcp --destination-port 21 -j NETMAP -s %s --to %s 2>/dev/null",
               ifname, lanPrefix, wanPrefix);
       rut_doSystemAction("ALGv6Rules", &cmd[0]);
   }
}
#endif


#ifdef SUPPORT_MAP
void rutIpt_updateSNATMapRules(UBOOL8 add, const char *callerStr, const char *tunintf, const char *addr, const PortSet *ps)
{
   char script[BUFLEN_64];
   char cmd[BUFLEN_128];
   FILE *fp;

   snprintf(script, sizeof(script), "/var/%s", __FUNCTION__);

   fp = fopen(script, "w");
   if (fp)
   {
      fprintf(fp, "#!/bin/sh\n");

      if (add)
      {
         fprintf(fp, "iptables -w -t nat -N mapchain\n");
         fprintf(fp, "iptables -w -t nat -I POSTROUTING -j mapchain\n");

         if (ps)
         {
            char *proto[3] = {"icmp", "tcp", "udp"};
            UINT16 i, j, min, max;

            /* express the MAP multiple ranges logic */
            for (i = 0; i < ps->numRanges; i++)
            {
               min = ps->range[i].min;
               max = ps->range[i].max;

               for (j = 0; j < sizeof(proto) / sizeof(proto[0]); j++)
               {
                  fprintf(fp, "iptables -w -t nat -A mapchain -p %s -o %s -m connlimit --connlimit-upto %d --connlimit-daddr-dport "
                              "-j SNAT --to-source %s:%d-%d\n", proto[j], tunintf, max - min + 1, addr, min, max);
               }
            }
         }
         else
         {
            fprintf(fp, "iptables -w -t nat -A mapchain -o %s -j SNAT --to-source %s\n", tunintf, addr);
         }
      }
      else
      {
         fprintf(fp, "iptables -w -t nat -D POSTROUTING -j mapchain\n");
         fprintf(fp, "iptables -w -t nat -F mapchain\n");
         fprintf(fp, "iptables -w -t nat -X mapchain\n");
      }

      fclose(fp);
      snprintf(cmd, sizeof(cmd), "sh %s 2>/dev/null", script);
      rut_doSystemAction(callerStr, cmd);
      unlink(script);
   }
}
#endif


#ifdef SUPPORT_UPNP
#ifndef SUPPORT_NF_TABLES
void rutIpt_upnpConfigStopMulticast(char *wanIfcName, UBOOL8 addRules)
{
   char cmd[BUFLEN_128];
   char action = 'I';

   if (!addRules)
   {
      action = 'D';
   }

   snprintf(cmd, sizeof(cmd), "iptables -w -t filter -%c OUTPUT -o %s -d %s -j DROP 2>/dev/null", action, wanIfcName, UPNP_IP_ADDRESS);
   rut_doSystemAction("upnp", cmd);

}
#endif
#endif /* SUPPORT_UPNP */


#ifdef SUPPORT_RIP


void rutIpt_ripAddIptableRule(const char *ifcName)
{
   char cmd[BUFLEN_128]={0};

   cmsLog_debug("adding exception for %s", ifcName);

   snprintf(cmd, sizeof(cmd), "iptables -w -I INPUT 1 -p udp --dport 520 -i %s -j ACCEPT", ifcName);
   rut_doSystemAction("rip", cmd);
}


/*
 * Dump the iptables exceptions list and remove the first UDP dport 520
 * rule we see.  There may be more than 1, but this function only removes
 * the first one.
 *
 * @return TRUE if RIP rule was found and removed
 */
static UBOOL8 removeSingleRipIptableRule(void)
{
   char col[IP_TBL_COL_MAX][BUFLEN_32];
   char prot[BUFLEN_32];
   char comment[BUFLEN_264], line[BUFLEN_1024];
   char cmd[BUFLEN_128]={0};
   SINT32  count = 0;
   FILE* fs = NULL;
   UBOOL8 found = FALSE;

   /* make sure iptables modules are inserted (is this really necessary?) */
   rutIpt_insertIpModules();

   /* execute iptables command to create iptable file */
   sprintf(line, "iptables -w -n -L INPUT -v --line-numbers > /var/iptable");
   rut_doSystemAction("rip", line);

   if ((fs = fopen("/var/iptable", "r")) == NULL)
   {
      cmsLog_notice("No iptable file?");
      return found;
   }

   while (fgets(line, sizeof(line), fs))
   {
      /* read pass 2 header lines */
      if ( count++ < 2 )
      {
         continue;
      }
      sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s %s",
             col[IP_TBL_COL_NUM], col[IP_TBL_COL_PKTS], col[IP_TBL_COL_BYTES],
             col[IP_TBL_COL_TARGET], col[IP_TBL_COL_PROT], col[IP_TBL_COL_OPT],
             col[IP_TBL_COL_IN], col[IP_TBL_COL_OUT], col[IP_TBL_COL_SRC],
             col[IP_TBL_COL_DST], prot, comment);
      /*
       *if protocol column is "udp" and last colum is "udp dpt:520"
       * then delete rule since it is RIP IP tables rule
       */
      if (cmsUtl_strcmp(col[IP_TBL_COL_PROT], "udp") == 0 &&
           cmsUtl_strcmp(prot, "udp") == 0 &&
           cmsUtl_strcmp(comment, "dpt:520") == 0)
      {
         cmsLog_debug("matched line: %s", line);
         snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT %s 2>/dev/null", col[IP_TBL_COL_NUM]);
         rut_doSystemAction("rip", cmd);
         found = TRUE;
         break;
      }
   }
   fclose(fs);

   /* remove iptable file */
   unlink("/var/iptable");

   return found;
}


void rutIpt_ripRemoveAllIptableRules(void)
{
   /* keep removing RIP iptables rules as long as we keep finding them */
   while (removeSingleRipIptableRule())
   {
      cmsLog_debug("removed a RIP iptables rule, go back and remove another");
   }
}

#endif /* SUPPORT_RIP */


#ifdef SUPPORT_URLFILTER
void rutIpt_activeUrlFilter(void)
{
   rut_UrlFilterLoadModule();
   rut_doSystemAction("UrlFIlter", "iptables -w -N urlfilter");
   rut_doSystemAction("UrlFIlter", "iptables -w -A FORWARD -p tcp --tcp-flags SYN,RST,FIN NONE -j urlfilter");

#ifdef SUPPORT_IPV6
   rut_doSystemAction("UrlFIlter", "ip6tables -w -N urlfilter");
   rut_doSystemAction("UrlFIlter", "ip6tables -w -A FORWARD -p tcp --tcp-flags SYN,RST,FIN NONE -j urlfilter");
#endif
   return;
}

void rutIpt_deactiveUrlFilter(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _UrlFilterCfgObject *urlCfg = NULL;

   if (cmsObj_get(MDMOID_URL_FILTER_CFG, &iidStack, 0, (void **) &urlCfg) == CMSRET_SUCCESS)
   {
      if (urlCfg->enable == TRUE)
      {
         rut_doSystemAction("UrlFIlter",
           "iptables -w -D FORWARD -p tcp --tcp-flags SYN,RST,FIN NONE -j urlfilter");
         rut_doSystemAction("UrlFIlter",
           "iptables -w -X urlfilter");
#ifdef SUPPORT_IPV6
         rut_doSystemAction("UrlFIlter",
           "ip6tables -w -D FORWARD -p tcp --tcp-flags SYN,RST,FIN NONE -j urlfilter");
         rut_doSystemAction("UrlFIlter",
           "ip6tables -w -X urlfilter");
#endif
      }
      cmsObj_free((void **) &urlCfg);
   }

   return;
}

void rutIpt_configUrlFilterMode(char *type)
{
   SINT32 pid;

   cmsLog_debug("Url filter mode set: %s", type);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_URLFILTERD, type, strlen(type)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart UrlFilterd");
   }

   return;
}

void rutIpt_urlFilterConfig(void *Obj, UBOOL8 add)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   _UrlFilterListObject *listObj = NULL;
   _UrlFilterListObject * inputObj = (_UrlFilterListObject *) Obj;
   _UrlFilterCfgObject *urlCfg = NULL;
   UINT32 i=0, j=0, ports[100];
   char cmd[128]="", check = 'n';;
   SINT32 pid;
   FILE *fs = fopen("/var/url_list", "w") ;
   if (fs == NULL)
   {
      cmsLog_error("Failed to open /var/url_list for read");
      return;
   }

   cmsLog_debug("entered");
   memset(&ports, 0, sizeof(ports));
   while (cmsObj_getNext(MDMOID_URL_FILTER_LIST, &iidStack, (void **) &listObj) == CMSRET_SUCCESS)
   {
      if (! (!cmsUtl_strcmp(listObj->urlAddress, inputObj->urlAddress) && add==FALSE))
      {
         snprintf(cmd, sizeof(cmd), "%s\n", listObj->urlAddress);
         fputs(cmd, fs);

         for (j=0;j<=i;j++)
         {
            if (ports[j] == listObj->portNumber)
            {
               check = 'y';
               break;
            }
         }

         if (check == 'n')
         {
            ports[i] = listObj->portNumber;
            i++;
         }
         else
         {
            check = 'n';
         }
      }
      cmsObj_free((void * *) &listObj);

   }

   fclose(fs);

   rut_doSystemAction("UrlFilter", "iptables -w -F urlfilter");
#ifdef SUPPORT_IPV6
   rut_doSystemAction("UrlFilter", "ip6tables -w -F urlfilter");
#endif

   for (i=0;ports[i]!=0;i++)
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -I urlfilter -p tcp --dport %d -j SKIPLOG", ports[i]);
      rut_doSystemAction("UrlFilter", cmd);

      snprintf(cmd, sizeof(cmd), "iptables -w -A urlfilter -p tcp --dport %d -j QUEUE", ports[i]);
      rut_doSystemAction("UrlFilter", cmd);
#ifdef SUPPORT_IPV6
      snprintf(cmd, sizeof(cmd), "ip6tables -w -I urlfilter -p tcp --dport %d -j SKIPLOG", ports[i]);
      rut_doSystemAction("UrlFilter", cmd);

      snprintf(cmd, sizeof(cmd), "ip6tables -w -A urlfilter -p tcp --dport %d -j QUEUE", ports[i]);
      rut_doSystemAction("UrlFilter", cmd);
#endif
   }

   if ((ret = cmsObj_get(MDMOID_URL_FILTER_CFG, &iidStack1, 0, (void **) &urlCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get UrlFilterCfgObject, ret=%d", ret);

   }

   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_URLFILTERD, urlCfg->excludeMode, strlen(urlCfg->excludeMode)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart urlfilterd");
   }
   cmsObj_free((void **) &urlCfg);

   return;
}

#endif /* SUPPORT_URLFILTER */


void rutIpt_activatePortMappingEntries_igd(const char *ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   void *obj=NULL;
   UBOOL8 isPpp;
   UBOOL8 isPcpDslite;
   CmsRet ret;

   cmsLog_debug("ifName=%s", ifName);

   ret = rutWan_getIpOrPppObjectByIfname(ifName, &iidStack, &obj, &isPpp);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ifName %s", ifName);
      return;
   }

   if (isPpp)
   {
      _WanPppConnPortmappingObject *port_mapping = NULL;
      isPcpDslite = (((WanPppConnObject *)obj)->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE)?TRUE:FALSE;

      while (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN_PORTMAPPING, &iidStack, &iidStack1, (void **) &port_mapping) == CMSRET_SUCCESS)
      {
         if(port_mapping->portMappingEnabled && (!isPcpDslite || !cmsUtl_strstr(port_mapping->X_BROADCOM_COM_AppName, "upnp")))
         {
            rutIpt_vrtsrvCfg(NULL, (WanPppConnObject *) obj, port_mapping, NULL, TRUE);
         }
         cmsObj_free((void **) &port_mapping);
      }
   }
   else
   {
      _WanIpConnPortmappingObject *port_mapping = NULL;
      isPcpDslite = (((WanIpConnObject *)obj)->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE)?TRUE:FALSE;

      while (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN_PORTMAPPING, &iidStack, &iidStack1, (void **) &port_mapping) == CMSRET_SUCCESS)
      {
         if(port_mapping->portMappingEnabled && (!isPcpDslite || !cmsUtl_strstr(port_mapping->X_BROADCOM_COM_AppName, "upnp")))
         {
            rutIpt_vrtsrvCfg((WanIpConnObject *) obj, NULL, NULL, port_mapping, TRUE);
         }
         cmsObj_free((void **) &port_mapping);

      }
   }

   /* free the parent ppp or ip connection object */
   cmsObj_free(&obj);

   return;
}




CmsRet rutIpt_vrtsrvCfg(const _WanIpConnObject *wan_ip_con, const _WanPppConnObject *wan_ppp_con,
                                const _WanPppConnPortmappingObject *pppObj, const _WanIpConnPortmappingObject *ipObj, const UBOOL8 add)
{
   CmsRet ret = CMSRET_SUCCESS;
   char interface[CMS_IFNAME_LENGTH];
   char action = 'A';
   char cmd[BUFLEN_256];
   char extPort[BUFLEN_16];
   char intPort[BUFLEN_16];
   char dmzHost[BUFLEN_16];
   UINT32 externalPort;
   UINT32 X_BROADCOM_COM_ExternalPortEnd;
   UINT32 internalPort;
   UINT32 X_BROADCOM_COM_InternalPortEnd;
   char portMappingProtocol[BUFLEN_4];
   char internalClient[CMS_IPADDR_LENGTH];
   char remoteHost[CMS_IPADDR_LENGTH];
   char portMappingDescription[BUFLEN_256];
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _SecDmzHostCfgObject *dmzCfg = NULL;

   if(wan_ip_con != NULL)
   {
      cmsUtl_strncpy(interface, wan_ip_con->X_BROADCOM_COM_IfName, sizeof(interface));
      externalPort = ipObj->externalPort;
      X_BROADCOM_COM_ExternalPortEnd = ipObj->X_BROADCOM_COM_ExternalPortEnd;
      internalPort = ipObj->internalPort;
      X_BROADCOM_COM_InternalPortEnd = ipObj->X_BROADCOM_COM_InternalPortEnd;
      if (cmsUtl_strcmp(ipObj->portMappingProtocol, "TCP") == 0)
      {
         strcpy(portMappingProtocol, "1");
      }
      else if (cmsUtl_strcmp(ipObj->portMappingProtocol, "UDP") == 0)
      {
         strcpy(portMappingProtocol, "2");
      }
      else
      {
         strcpy(portMappingProtocol, "0");
      }

      cmsUtl_strncpy(internalClient, ipObj->internalClient, sizeof(internalClient));

      if(ipObj->remoteHost != NULL)
      {
         cmsUtl_strncpy(remoteHost, ipObj->remoteHost, sizeof(remoteHost));
      }
      else
      {
         remoteHost[0] = '\0';
      }

      cmsUtl_strncpy(portMappingDescription, ipObj->portMappingDescription, sizeof(portMappingDescription));

   }
   else if(wan_ppp_con != NULL)
   {
      cmsUtl_strncpy(interface, wan_ppp_con->X_BROADCOM_COM_IfName, sizeof(interface));
      externalPort = pppObj->externalPort;
      X_BROADCOM_COM_ExternalPortEnd = pppObj->X_BROADCOM_COM_ExternalPortEnd;
      internalPort = pppObj->internalPort;
      X_BROADCOM_COM_InternalPortEnd = pppObj->X_BROADCOM_COM_InternalPortEnd;
      if (cmsUtl_strcmp(pppObj->portMappingProtocol, "TCP") == 0)
      {
         strcpy(portMappingProtocol, "1");
      }
      else if (cmsUtl_strcmp(pppObj->portMappingProtocol, "UDP") == 0)
      {
         strcpy(portMappingProtocol, "2");
      }
      else
      {
         strcpy(portMappingProtocol, "0");
      }
      cmsUtl_strncpy(internalClient, pppObj->internalClient, sizeof(internalClient));

      if(pppObj->remoteHost != NULL)
      {
         cmsUtl_strncpy(remoteHost, pppObj->remoteHost, sizeof(remoteHost));
      }
      else
      {
         remoteHost[0] = '\0';
      }

      cmsUtl_strncpy(portMappingDescription, pppObj->portMappingDescription, sizeof(portMappingDescription));

   }
   else
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("Getting Dmz host info");

   if ((ret = cmsObj_get(MDMOID_SEC_DMZ_HOST_CFG, &iidStack, 0, (void **) &dmzCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get SecDmzHostCfgObject, ret=%d", ret);
      return ret;
   }

   if (dmzCfg->IPAddress == NULL)
   {
      dmzHost[0] = '\0';
   }
   else
   {
      strncpy(dmzHost, dmzCfg->IPAddress, sizeof(dmzHost)-1);
      dmzHost[sizeof(dmzHost)-1] = '\0';
   }

   cmsObj_free((void **) &dmzCfg);

   if(add)
   {
       /* if DMZ exis */
      if ( dmzHost[0] != '\0' )
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i %s -j DNAT --to-destination %s", interface, dmzHost);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
         snprintf(cmd, sizeof(cmd), "iptables -w -D FORWARD -i %s -d %s -j ACCEPT", interface, dmzHost);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);

         snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i %s -p udp --dport 67:68 -j RETURN  2>/dev/null", interface);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
         snprintf(cmd, sizeof(cmd), "iptables -w -D FORWARD -i %s -p udp --dport 67:68 -j RETURN  2>/dev/null", interface);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }

      action = 'A';
   }
   else
   {
      action = 'D';
   }

   if (externalPort == X_BROADCOM_COM_ExternalPortEnd)
   {
      sprintf(extPort, "%u", externalPort);
   }
   else
   {
      sprintf(extPort, "%u:%u", externalPort, X_BROADCOM_COM_ExternalPortEnd);
   }

   /* if internal ports are same as enternal, leave intPort to "" */
   if (internalPort == externalPort && X_BROADCOM_COM_InternalPortEnd == X_BROADCOM_COM_ExternalPortEnd)
   {
      intPort[0] = '\0';
   }
   else
   {
      if (internalPort == X_BROADCOM_COM_InternalPortEnd)
      {
         sprintf(intPort, "%u", internalPort);
      }
      else
      {
         sprintf(intPort, "%u:%u", internalPort, X_BROADCOM_COM_InternalPortEnd);
      }
   }

   switch ( atoi(portMappingProtocol) )
   {
      case 1:  // TCP
      {
         ret = rutIpt_vrtsrvRunIptables(action, interface, "tcp", extPort, internalClient, intPort, remoteHost);
      }
      break;
      case 2: // UDP
      {
         ret = rutIpt_vrtsrvRunIptables(action, interface, "udp", extPort, internalClient, intPort, remoteHost);
      }
      break;
      default:
      {
         ret = rutIpt_vrtsrvRunIptables(action, interface, "tcp", extPort, internalClient, intPort, remoteHost);
         ret = rutIpt_vrtsrvRunIptables(action, interface, "udp", extPort, internalClient, intPort, remoteHost);
      }
      break;
   }

   if ( cmsUtl_strcmp(portMappingDescription, "IPSEC") == 0 )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p 50 -j DNAT --to %s",
              action, interface, internalClient);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      if ( action == 'A' )
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -I FORWARD 1 -i %s -p 50 -d %s -j ACCEPT",
                 interface, internalClient);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }
      else
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -D FORWARD -i %s -p 50 -d %s -j ACCEPT",
                 interface, internalClient);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }
   }
   else if ( cmsUtl_strcmp(portMappingDescription, "PPTP") == 0 )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p 47 -j DNAT --to %s",
              action, interface, internalClient);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      if ( action == 'A' )
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -I FORWARD 1 -i %s -p 47 -d %s -j ACCEPT",
                 interface, internalClient);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }
      else
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -D FORWARD -i %s -p 47 -d %s -j ACCEPT",
                 interface, internalClient);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }
   }
   /*
     ** if virtual server is configured and wan access is enabled for this service then
     ** add the prerouting rule to redirect port so that system can support this service by itself
    */
#ifdef SUPPORT_FTPD
   if( (externalPort == FTP_SERVER_PORT_21) ||
       ((externalPort < FTP_SERVER_PORT_21) && (X_BROADCOM_COM_ExternalPortEnd >= FTP_SERVER_PORT_21)) )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p tcp --dport %d -j REDIRECT --to-ports %d 2>/dev/null",
              action, interface, FTP_SERVER_PORT_2121, FTP_SERVER_PORT_21);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }
#endif
   if( (externalPort == WEB_SERVER_PORT_80) ||
       ((externalPort < WEB_SERVER_PORT_80) && (X_BROADCOM_COM_ExternalPortEnd >= WEB_SERVER_PORT_80)) )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p tcp --dport %d -j REDIRECT --to-ports %d 2>/dev/null",
              action, interface, WEB_SERVER_PORT_8080, WEB_SERVER_PORT_80);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }
/*
#ifdef SUPPORT_SNMP
      if ( cmsUtl_strcmp(Obj->srvName, "SNMP") == 0 &&
           ( rut_checkAccessMode(EID_SNMPD, 6 * MSECS_IN_SEC)  ) ) {
         sprintf(cmd, "iptables -w -t nat -%c PREROUTING -i %s -p udp --dport %s -j REDIRECT --to-ports %s",
                 action, interface, SNMP_AGENT_PORT_16116, SNMP_AGENT_PORT_161);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
      }
#endif
*/
#ifdef SUPPORT_SSHD
   if( (externalPort == SSH_SERVER_PORT_22) ||
       ((externalPort < SSH_SERVER_PORT_22) && (X_BROADCOM_COM_ExternalPortEnd >= SSH_SERVER_PORT_22)) )
   {
         snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p tcp --dport %d -j REDIRECT --to-ports %d 2>/dev/null",
                 action, interface, SSH_SERVER_PORT_2222, SSH_SERVER_PORT_22);
         rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }
#endif
#ifdef SUPPORT_TELNETD
   if( (externalPort == TELNET_SERVER_PORT_23) ||
       ((externalPort < TELNET_SERVER_PORT_23) && (X_BROADCOM_COM_ExternalPortEnd >= TELNET_SERVER_PORT_23)) )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p tcp --dport %d -j REDIRECT --to-ports  %d 2>/dev/null",
              action, interface, TELNET_SERVER_PORT_2323, TELNET_SERVER_PORT_23);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }
#endif
#ifdef SUPPORT_TFTPD
   if( (externalPort == TFTP_SERVER_PORT_69) ||
       ((externalPort < TFTP_SERVER_PORT_69) && (X_BROADCOM_COM_ExternalPortEnd >= TFTP_SERVER_PORT_69)) )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p udp --dport %d -j REDIRECT --to-ports %d 2>/dev/null",
              action, interface, TFTP_SERVER_PORT_6969, TFTP_SERVER_PORT_69);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }
#endif

   if(add && dmzHost[0] != '\0' )
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination %s 2>/dev/null", interface, dmzHost);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);

      /* Don't preroute DHCP Offer to LAN Host */
      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -I PREROUTING 1 -i %s -p udp --dport 67:68 -j RETURN  2>/dev/null", interface);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);

      snprintf(cmd, sizeof(cmd), "iptables -w -I FORWARD 1 -i %s -d %s -j ACCEPT 2>/dev/null", interface, dmzHost);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);

      /* Don't forward DHCP Offer to LAN Host */
      snprintf(cmd, sizeof(cmd), "iptables -w -I FORWARD 1 -i %s -p udp --dport 67:68 -j RETURN  2>/dev/null", interface);
      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
   }

   return ret;

}


CmsRet rutIpt_vrtsrvRunIptables(char action, char *device,  char *protocol,
                              char *inPort, char *srvAddress, char *srvPort, char *remoteHost)
{
   char cmd[BUFLEN_256];
   CmsRet ret = CMSRET_SUCCESS;


   if ( srvPort[0] == '\0' )
   {

      if(action=='A')
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p %s%s%s --dport %s -j DNAT --to %s",
                 'D', device, protocol,
                 (remoteHost[0] != '\0'? " --source " :""), (remoteHost[0] != '\0'? remoteHost :""),
                 inPort, srvAddress);
         cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

         rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
      }

      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p %s%s%s --dport %s -j DNAT --to %s",
              action=='A' ? 'A':'D', device, protocol,
              (remoteHost[0] != '\0'? " --source " :""), (remoteHost[0] != '\0'? remoteHost :""),
              inPort, srvAddress);
      cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

      rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
/*
 *    char str[BUFLEN_264]={0};
 *    FILE* errFs = NULL;
      errFs = fopen("/var/rutIpt_vrtsrvRunIptables", "r");
      if (errFs != NULL )
      {
         fgets(str, BUFLEN_264, errFs);
         fclose(errFs);
         unlink("/var/rutIpt_vrtsrvRunIptables");
      }

      if ( str[0] != '\0' )
      {
         cmsLog_error("in rutIpt_vrtsrvRunIptables, str = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
*/
      /* Since FORWARD chain is blocking, we need to allow */

      if(action=='A')
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -%c FORWARD %c -i %s -p %s -d %s --dport %s -j ACCEPT",
                 'D', ' ', device, protocol, srvAddress, inPort);
         cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

         rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
      }


      snprintf(cmd, sizeof(cmd), "iptables -w -%c FORWARD %c -i %s -p %s -d %s --dport %s -j ACCEPT",
              action=='A' ? 'I':'D', action=='A' ? '1':' ', device, protocol, srvAddress, inPort);
      cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

      rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
/*
      errFs = fopen("/var/rutIpt_vrtsrvRunIptables", "r");
      if (errFs != NULL )
      {
         fgets(str, BUFLEN_264, errFs);
         fclose(errFs);
         unlink("/var/rutIpt_vrtsrvRunIptables");
      }

      if ( str[0] != '\0' )
      {
         cmsLog_error("in rutIpt_vrtsrvRunIptables, str = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
*/
   }
   else
   {
      if(action=='A')
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p %s%s%s --dport %s -j DNAT --to %s:%s",
                 'D', device, protocol,
                 (remoteHost [0]!= '\0'? " --source " :""), (remoteHost[0] != '\0'? remoteHost :""),
                 inPort, srvAddress, srvPort);
         cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

         rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
      }

      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -%c PREROUTING -i %s -p %s%s%s --dport %s -j DNAT --to %s:%s",
              action=='A' ? 'A':'D', device, protocol,
              (remoteHost [0]!= '\0'? " --source " :""), (remoteHost[0] != '\0'? remoteHost :""),
              inPort, srvAddress, srvPort);
      cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

      rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
/*
      errFs = fopen("/var/rutIpt_vrtsrvRunIptables", "r");
      if (errFs != NULL )
      {
         fgets(str, BUFLEN_264, errFs);
         fclose(errFs);
         unlink("/var/rutIpt_vrtsrvRunIptables");
      }

      if ( str[0] != '\0' )
      {
         cmsLog_error("in rutIpt_vrtsrvRunIptables, str = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
*/
      /* Since FORWARD chain is blocking, we need to allow */

      if(action=='A')
      {
         snprintf(cmd, sizeof(cmd), "iptables -w -%c FORWARD %c -i %s -p %s -d %s --dport %s -j ACCEPT",
                 'D', ' ', device, protocol, srvAddress, srvPort);
         cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

         rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
      }

      snprintf(cmd, sizeof(cmd), "iptables -w -%c FORWARD %c -i %s -p %s -d %s --dport %s -j ACCEPT",
              action=='A' ? 'I':'D', action=='A' ? '1':' ', device, protocol, srvAddress, srvPort);
      cmsUtl_strncat(cmd, BUFLEN_256," 2> /var/rutIpt_vrtsrvRunIptables");

      rut_doSystemAction("rutIpt_vrtsrvRunIptables", cmd);
/*
      errFs = fopen("/var/rutIpt_vrtsrvRunIptables", "r");
      if (errFs != NULL )
      {
         fgets(str, BUFLEN_264, errFs);
         fclose(errFs);
         unlink("/var/rutIpt_vrtsrvRunIptables");
      }

      if ( str[0] != '\0' )
      {
         cmsLog_error("in rutIpt_vrtsrvRunIptables, str = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
*/
   }

   return ret;

}

void rutIpt_insertPortTriggeringModules(void)
{
   struct utsname kernel;
   char cmd[128];

   if (!isModuleInserted("nf_nat_pt")) {
      insertModuleByName("nf_nat");

      if(uname(&kernel) < 0) {
         cmsLog_error("Fail to insert nf_nat_pt.ko - uname() error\n");
         return;
      }
      snprintf(cmd, sizeof(cmd), "insmod /lib/modules/%s/bcmkernel/net/netfilter/nf_nat_pt.ko ", kernel.release);
      rut_doSystemAction("rut", cmd);
   }
}

void rutIpt_igmpRules(UBOOL8 add, const char *ifname)
{
   char cmd[256];
   if (add == TRUE)
   {
      /* delete then add to prevent duplication */
      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -D INPUT -i %s "
                   "-p 2 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -D FORWARD -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);


      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -I INPUT 1 -i %s "
                   "-p 2 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -I FORWARD 1 -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -I PREROUTING 1 -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);
   }
   else if (add == FALSE)
   {
      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -D INPUT -i %s "
                   "-p 2 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t filter -D FORWARD -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);

      snprintf(cmd, sizeof(cmd), "iptables -w -t nat -D PREROUTING -i %s "
                   "-d 224.0.0.0/4 -j ACCEPT 2>/dev/null", ifname);
      rut_doSystemAction("igmpRules", &cmd[0]);
   }
}


#ifdef DMP_X_BROADCOM_COM_SECURITY_1

void rutIpt_addIpFilterOut(const IpFilterCfgObject *obj, const char *ifName, UBOOL8 add)
{
   char fmt[BUFLEN_256];
   char cmd[BUFLEN_256];
   char ipt[BUFLEN_16];
   char protocol[BUFLEN_16];
   char src[BUFLEN_40], dst[BUFLEN_40];
   char sport[BUFLEN_40], dport[BUFLEN_40];
   UBOOL8 tcpOrUdp = FALSE;

   cmd[0] = src[0] = dst[0] = sport[0] = dport[0] = '\0';

   insertModuleByName("iptable_filter");
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   insertModuleByName("ip6table_filter");
#endif

   sprintf(ipt, "%s", (atoi(obj->IPVersion) == 4)? "iptables" : "ip6tables");

   /* source address/mask */
   if ( cmsUtl_strcmp(obj->sourceIPAddress, "\0") != 0 )
   {
      if (strchr(obj->sourceIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(obj->sourceNetMask, "\0") != 0 )
            sprintf(src, "-s %s/%d", obj->sourceIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->sourceNetMask));
         else
            sprintf(src, "-s %s", obj->sourceIPAddress);
      }
      else
      {
         /* IPv6 address */
         sprintf(src, "-s %s", obj->sourceIPAddress);
      }
   }

   /* source port */
   if ( obj->sourcePortStart != 0)
   {
      if ( obj->sourcePortEnd != 0)
      {
         sprintf(sport, "--sport %d:%d", obj->sourcePortStart, obj->sourcePortEnd);
      }
      else
      {
         sprintf(sport, "--sport %d", obj->sourcePortStart);
      }
   }

   /* destination address/mask */
   if ( cmsUtl_strcmp(obj->destinationIPAddress, "\0") != 0 )
   {
      if (strchr(obj->destinationIPAddress, ':') == NULL)
      {
         /* IPv4 address */
      if ( cmsUtl_strcmp(obj->destinationNetMask, "\0") != 0 )
         sprintf(dst, "-d %s/%d", obj->destinationIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->destinationNetMask));
      else
         sprintf(dst, "-d %s", obj->destinationIPAddress);
   }
      else
      {
         /* IPv6 address */
         sprintf(dst, "-d %s", obj->destinationIPAddress);
      }
   }

   /* destination port */
   if ( obj->destinationPortStart != 0)
   {
      if ( obj->destinationPortEnd != 0)
      {
         sprintf(dport, "--dport %d:%d", obj->destinationPortStart, obj->destinationPortEnd);
      }
      else
      {
         sprintf(dport, "--dport %d", obj->destinationPortStart);
      }
   }

   /* protocol */
   if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP) == 0 )
   {
      sprintf(protocol, "-p tcp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_UDP) == 0 )
   {
      sprintf(protocol, "-p udp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP_OR_UDP) == 0 )
   {
      tcpOrUdp = TRUE;
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMP) == 0 )
   {
      sprintf(protocol, "-p icmp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMPV6) == 0 )
   {
      sprintf(protocol, "-p icmpv6");
   }
   else
   {
      // None
      protocol[0] = '\0';
   }

   strcpy(fmt, "%s -w -%c FORWARD -i %s %s %s %s %s %s -j DROP %s");

   if (tcpOrUdp)
   {
      snprintf(cmd, sizeof(cmd), fmt, ipt, 'D', ifName, "-p tcp", src, sport, dst, dport, "2>/dev/null");
      rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
      snprintf(cmd, sizeof(cmd), fmt, ipt, 'D', ifName, "-p udp", src, sport, dst, dport, "2>/dev/null");
      rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
      if (add)
      {
         snprintf(cmd, sizeof(cmd), fmt, ipt, 'A', ifName, "-p tcp", src, sport, dst, dport, "");
         rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
         snprintf(cmd, sizeof(cmd), fmt, ipt, 'A', ifName, "-p udp", src, sport, dst, dport, "");
         rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
      }
   }
   else
   {
      snprintf(cmd, sizeof(cmd), fmt, ipt, 'D', ifName, protocol, src, sport, dst, dport, "2>/dev/null");
      rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
      if (add)
      {
         snprintf(cmd, sizeof(cmd), fmt, ipt, 'A', ifName, protocol, src, sport, dst, dport, "");
         rut_doSystemAction("rutIpt_addIpFilterOut", cmd);
      }
   }
}




#define FILTERIN_OUTFILE  "/var/filterin_out.txt"

static void runIpv4FilterInCmdWithRetry(const char *cmd)
{
   static UINT32 total=0;
   int fd;
   SINT32 fsize;
   SpawnProcessInfo spInfo;
   SpawnedProcessInfo prInfo;
   CmsRet ret __attribute__ ((unused));

   total++;

   /* create a file to capture stdout and stderr */
   fd = open(FILTERIN_OUTFILE, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);

   memset(&prInfo, 0, sizeof(prInfo));
   memset(&spInfo, 0, sizeof(spInfo));
   spInfo.exe = "/bin/iptables";
   spInfo.args = &(cmd[9]);
   spInfo.spawnMode = SPAWN_AND_WAIT;
   spInfo.timeout = 4000;  // 4000ms
   spInfo.stdinFd = -1;
   spInfo.stdoutFd = fd;
   spInfo.stderrFd = fd;
   spInfo.serverFd = -1;
   spInfo.maxFd = 30;


   cmsLog_debug("[%d] cmd=%s", total, spInfo.args);

   ret = prctl_spawnProcess(&spInfo, &prInfo);

   if(fd >= 0)
      close(fd);
   fsize = cmsFil_getSize(FILTERIN_OUTFILE);

   cmsLog_debug("[%d] ret=%d pid=%d status=%d signal=%d exitCode=%d fsize=%d",
                total, ret,
                prInfo.pid, prInfo.status, prInfo.signalNumber, prInfo.exitCode,
                fsize);

   if (fsize > 0)
   {
      char buf[256]={0};
      UINT32 bufSize = sizeof(buf);
      ret = cmsFil_copyToBuffer(FILTERIN_OUTFILE, (UINT8 *) buf, &bufSize);
      cmsLog_error("cmd=%s", cmd);
      cmsLog_error("ret=%d bufSize=%d buf=%s", ret, bufSize, buf);
   }

   unlink(FILTERIN_OUTFILE);


   return;
}


static void parseFirewallExceptionObject(const void *voidObj, char *src, char *dst, char *sport, char *dport, char *protocol, UBOOL8 *tcpOrUdp)
{
   /* always cast to the WanPppConnFirewallException object,
    * it is exactly the same as the other firewall exception objects. */

   const _WanPppConnFirewallExceptionObject *obj = (_WanPppConnFirewallExceptionObject *) voidObj;

   /* source address/mask */
   if ( cmsUtl_strcmp(obj->sourceIPAddress, "\0") != 0 )
   {
      if (strchr(obj->sourceIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(obj->sourceNetMask, "\0") != 0 )
            sprintf(src, "-s %s/%d", obj->sourceIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->sourceNetMask));
         else
            sprintf(src, "-s %s", obj->sourceIPAddress);
      }
      else
      {
         /* IPv6 address */
         sprintf(src, "-s %s", obj->sourceIPAddress);
      }
   }

   /* source port */
   if ( obj->sourcePortStart != 0)
   {
      if ( obj->sourcePortEnd != 0)
      {
         sprintf(sport, "--sport %d:%d", obj->sourcePortStart, obj->sourcePortEnd);
      }
      else
      {
         sprintf(sport, "--sport %d", obj->sourcePortStart);
      }
   }

   /* destination address/mask */
   if ( cmsUtl_strcmp(obj->destinationIPAddress, "\0") != 0 )
   {
      if (strchr(obj->destinationIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(obj->destinationNetMask, "\0") != 0 )
            sprintf(dst, "-d %s/%d", obj->destinationIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->destinationNetMask));
         else
            sprintf(dst, "-d %s", obj->destinationIPAddress);
      }
      else
      {
         /* IPv6 address */
         sprintf(dst, "-d %s", obj->destinationIPAddress);
      }
   }

   /* destination port */
   if ( obj->destinationPortStart != 0)
   {
      if ( obj->destinationPortEnd != 0)
      {
         sprintf(dport, "--dport %d:%d", obj->destinationPortStart, obj->destinationPortEnd);
      }
      else
      {
         sprintf(dport, "--dport %d", obj->destinationPortStart);
      }
   }

   /* protocol */
   if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP) == 0 )
   {
      sprintf(protocol, "-p tcp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_UDP) == 0 )
   {
      sprintf(protocol, "-p udp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP_OR_UDP) == 0 )
   {
      *tcpOrUdp = TRUE;
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMP) == 0 )
   {
      sprintf(protocol, "-p icmp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMPV6) == 0 )
   {
      sprintf(protocol, "-p icmpv6");
   }
   else
   {
      // None
      protocol[0] = '\0';
   }

   return;
}


void rutIpt_doIpFilterIn(const void *InObj, const char *ifName, UBOOL8 add)
{
   char src[BUFLEN_64], dst[BUFLEN_64];
   char sport[BUFLEN_40], dport[BUFLEN_40], protocol[BUFLEN_32];
   char action;
   char *ipver;
   UBOOL8 tcpOrUdp = FALSE;

   src[0] = dst[0] = sport[0] = dport[0] = protocol[0] = '\0';

   insertModuleByName("iptable_filter");
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   insertModuleByName("ip6table_filter");
#endif

   ipver = ((_WanPppConnFirewallExceptionObject *)InObj)->IPVersion;

   /* action */
   action = (add)? 'I' : 'D';

   parseFirewallExceptionObject(InObj, src, dst, sport, dport, protocol, &tcpOrUdp);

   if (tcpOrUdp)
   {
      rutIpt_ipFilterInRunIptables(ipver, action, ifName, "-p tcp", src, sport, dst, dport);
      rutIpt_ipFilterInRunIptables(ipver, action, ifName, "-p udp", src, sport, dst, dport);
   }
   else
   {
      rutIpt_ipFilterInRunIptables(ipver, action, ifName, protocol, src, sport, dst, dport);
   }
}


CmsRet rutIpt_ipFilterInRunIptables(char *ipver, char action, const char *ifName, char *protocol, char *src, char *sport, char *dst, char *dport)
{
   char cmd[BUFLEN_256];
   char ipt[BUFLEN_16];

   snprintf(ipt, sizeof(ipt), "%s", (atoi(ipver) == 4)? "iptables" : "ip6tables");

   snprintf(cmd, sizeof(cmd), "%s -w -%c INPUT -i %s %s %s %s %s %s -j ACCEPT", ipt, action, ifName, protocol, src, sport, dst, dport);

   if (atoi(ipver) == 4)
   {
      runIpv4FilterInCmdWithRetry(cmd);
   }
   else
   {
      rut_doSystemAction("rutIpt_ipFilterInRunIptables", cmd);
   }


   if (cmsUtl_strstr(ifName, "br") == NULL)
   {
      snprintf(cmd, sizeof(cmd), "%s -w -%c FORWARD -i %s %s %s %s %s %s -j ACCEPT", ipt, action, ifName, protocol, src, sport, dst, dport);

      if (atoi(ipver) == 4)
      {
         runIpv4FilterInCmdWithRetry(cmd);
      }
      else
      {
         rut_doSystemAction("rutIpt_ipFilterInRunIptables", cmd);
      }
   }

   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_SECURITY_1 */




#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */

void rutIpt_initNatForIntfGroup(const char *ifName, const char *bridgeIfName)
{
   char localSubnet[BUFLEN_64];
   char localSubnetmask[BUFLEN_64];
   char ipAddress[BUFLEN_64];
   char ifcIpAddress[BUFLEN_64];

   cmsLog_debug("wan ifName=%s bridgeIfName=%s",
                ifName, bridgeIfName);

#ifdef DESKTOP_LINUX
   strcpy(localSubnet, "192.168.2.0");
   strcpy(localSubnetmask, "255.255.255.0");
#else
   if (rut_getIfSubnet(bridgeIfName, localSubnet) && rut_getIfMask(bridgeIfName, localSubnetmask))
#endif
   {
      /* enable nat for the local private subnet only. */
      rutIpt_deleteNatMasquerade(ifName, localSubnet, localSubnetmask);
      rutIpt_insertNatMasquerade(ifName, localSubnet, localSubnetmask);

      /* disassociate Other Bridges From WanIntf */
      rutIpt_disassociateOtherBridgesFromWanIntf('A', PF_INET, ifName, localSubnet, localSubnetmask);

      /* use the  "br0"  ipAddress as the dns for this subnet */
      if (rut_getIfAddr(bridgeIfName, ipAddress) && rut_getIfAddr(bridgeIfName, ifcIpAddress))
      {
         rut_enableDNSForward(bridgeIfName, ifcIpAddress, ipAddress);
      }
      else
      {
         cmsLog_error("Failed to get bridge %s info.", bridgeIfName);
      }

      /* Remove  the binding to br0 here (was done int rutIpt_initNat)
        * for the problem maybe caused by MASQUERADE.
       */
      if (rut_getIfSubnet("br0", localSubnet) && rut_getIfMask("br0", localSubnetmask))
      {
         rutIpt_deleteNatMasquerade(ifName, localSubnet, localSubnetmask);
      }
      else
      {
         cmsLog_error("Failed to get br0 info.");
      }

      /* Need to add back the TCPMSS rules during the rutPMap_disassociateWanIntfFromBridge */
      rutIpt_insertTCPMSSRules(PF_INET, ifName);
   }
}

void rutIpt_removeBridgeIfNameIptableRules(const char *bridgeIfName)
{
   removeAllIptRulesByIntfName(PF_INET, bridgeIfName, "nat", "PREROUTING");
}

#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */

void rutIpt_redirectHttpTelnetPorts(const char *ifName, const char *ipAddress)
{
   char cmd[BUFLEN_128];

   /* Move local HTTP server to port 8080 and redirect the 8080 traffic to
     * local LAN on port 80.
     */
   snprintf(cmd,  sizeof(cmd), "iptables -w -t nat -A PREROUTING -i %s -p tcp --dport %d -j DNAT --to-destination %s:%d",
     ifName, WEB_SERVER_PORT_8080, ipAddress, WEB_SERVER_PORT_80);
   rut_doSystemAction("rut", cmd);

   /* Move local Telnet server to port 2323 and redirect TCP traffic on port
     * 2323 to local Telnet deamon.
     */
   snprintf(cmd,  sizeof(cmd), "iptables -w -t nat -A PREROUTING -i %s -p tcp --dport %d -j DNAT --to-destination %s:%d",
      ifName,  TELNET_SERVER_PORT_2323, ipAddress, TELNET_SERVER_PORT_23);
   rut_doSystemAction("rut", cmd);
}

#ifndef SUPPORT_NF_TABLES
void rutIpt_deleteTCPMSSRules(SINT32 domain, const char *ifName)
{
   char cmd[BUFLEN_128];
   char ipt[BUFLEN_16];

   sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");

   /* remove TCP MSS option manipulation */

   snprintf(cmd, sizeof(cmd), "%s -w -D FORWARD -i %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu 2>/dev/null", ipt,ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
   snprintf(cmd, sizeof(cmd), "%s -w -D FORWARD -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu 2>/dev/null", ipt,ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
}

void rutIpt_insertTCPMSSRules(SINT32 domain, const char *ifName)
{
   char cmd[BUFLEN_128];
   char ipt[BUFLEN_16];

   sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");

   /* setup TCP MSS option manipulation */

   rutIpt_deleteTCPMSSRules(domain,ifName);

   snprintf(cmd, sizeof(cmd), "%s -w -I FORWARD -i %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", ipt,ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
   snprintf(cmd, sizeof(cmd), "%s -w -I FORWARD -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", ipt,ifName);
   rut_doSystemAction("TCPMSSRules", cmd);
}
#endif

void rutIpt_activateFirewallAntiIPspoofing(const char *ifName)
{
   /* Enable anti-IPspoofing */
   rut_configIpv4RpFilter(ifName, 1); // 1: Strict mode
}



#ifdef DMP_X_ITU_ORG_GPON_1

void rutIpt_omciIpHostRules(UINT32 oid, UINT8 options, char *ifName)
{
    char cmd[BUFLEN_1024];

    switch (oid)
    {
        case MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA:
            if (!(options & 0x6)) // don't respond to ping or traceroute
            {
                insertModuleByName("iptable_filter");
            }
            break;
        case MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA:
            if (!(options & 0x8)) // don't respond to ping
            {
                insertModuleByName("iptable_filter");
            }
            break;
        default :
            return;
    }

    if (oid == MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA)
    {
        // flush all rules
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 8 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 13 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 15 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 17 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 35 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 37 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D INPUT -i %s -p icmp --icmp-type 42 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D OUTPUT -o %s -p icmp --icmp-type 3/3 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);
        snprintf(cmd, sizeof(cmd), "iptables -w -D OUTPUT -o %s -p icmp --icmp-type 11 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);

        /* The "IP options" attribute bitmask 0x02 in G.988 Clause 9.4.1 is to enable/disable "Respond to Pings". 
         * For security considerations, make the host hide from all ping-type probes. */
        if (!(options & 0x2))
        {
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 8 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 13 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 15 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 17 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 35 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 37 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            snprintf(cmd, sizeof(cmd), "iptables -w -A INPUT -i %s -p icmp --icmp-type 42 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
        }

        if (!(options & 0x4))
        {
            //for UDP TRACEROUTE(linux) , CPE reply ICMP Type 3 Error Code 3 (Type = Distination Unreachable Error Code = Port Unreachable)
            snprintf(cmd, sizeof(cmd), "iptables -w -A OUTPUT -o %s -p icmp --icmp-type 3/3 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
            //drop icmp Time Exceeded
            snprintf(cmd, sizeof(cmd), "iptables -w -A OUTPUT -o %s -p icmp --icmp-type 11 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
        }
    }
    else if (oid == MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA)
    {
        // flush all rules
        snprintf(cmd, sizeof(cmd), "ip6tables -w -D INPUT -i %s -p icmpv6 --icmpv6-type 128 -j DROP 2>/dev/null", ifName);
        rut_doSystemAction("omciIpHostRules", cmd);

        // block icmpv6 echo request: don't response ping.
        if (!(options & 0x8))
        {
            snprintf(cmd, sizeof(cmd), "ip6tables -w -A INPUT -i %s -p icmpv6 --icmpv6-type 128 -j DROP 2>/dev/null", ifName);
            rut_doSystemAction("omciIpHostRules", cmd);
        }
    }
}

#endif    /* DMP_X_ITU_ORG_GPON_1 */

#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
void rutIpt_disassociateOtherBridgesFromWanIntf(char action, SINT32 domain, const char *wanIfcName, const char *localSubnet, const char *localSubnetmask)
{
    char cmd[BUFLEN_256];
    char ipt[BUFLEN_16];

    cmsLog_debug("action=%c domain=%d wanIfcName=%s localSubnet=%s localSubnetmask=%s", action, domain, wanIfcName, localSubnet, localSubnetmask);

    sprintf(ipt, "%s", (domain == PF_INET)? "iptables" : "ip6tables");
    if (('D' == action) || ('A' == action))
    {
        snprintf(cmd, sizeof(cmd), "%s -w -D FORWARD -i %s ! -d %s/%s -j DROP 2>/dev/null", ipt, wanIfcName, localSubnet, localSubnetmask);
        rut_doSystemAction("WanIntfRules", cmd);
        snprintf(cmd, sizeof(cmd), "%s -w -D FORWARD -o %s ! -s %s/%s -j DROP 2>/dev/null", ipt,wanIfcName, localSubnet, localSubnetmask);
        rut_doSystemAction("WanIntfRules", cmd);
        if ('A' == action)
        {
            snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -i %s ! -d %s/%s -j DROP", ipt, wanIfcName, localSubnet, localSubnetmask);
            rut_doSystemAction("WanIntfRules", cmd);
            snprintf(cmd, sizeof(cmd), "%s -w -A FORWARD -o %s ! -s %s/%s -j DROP", ipt,wanIfcName, localSubnet, localSubnetmask);
            rut_doSystemAction("WanIntfRules", cmd);
        }
    }
    else
    {
        cmsLog_error("action=%c not supported!", action);
    }
}
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */


#ifdef DMP_DEVICE2_ETHLAG_1
CmsRet insertEthBondingModule(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("insmod bonding module.");
#ifdef DESKTOP_LINUX
   cmsLog_debug("fake insert bonding module");
#else
   if (isModuleInserted("bonding"))
   {
      cmsLog_debug("bonding is already insmoded");
      return ret;
   }
   else
   {

      char cmd[BUFLEN_1024];
      struct utsname kernel;

      /* Try to get kernel version */
      if (uname(&kernel) == -1)
      {
         cmsLog_error("Failed to get kernel version");
         return CMSRET_INTERNAL_ERROR;
      }
      snprintf(cmd, sizeof(cmd)-1, "insmod /lib/modules/%s/kernel/drivers/net/bonding/bonding.ko max_bonds=2", kernel.release);
      rut_doSystemAction("insertBondingModule", cmd);
   }

   cmsLog_debug("Exit");

#endif

   return ret;
}
#endif /* DMP_DEVICE2_ETHLAG_1 */
