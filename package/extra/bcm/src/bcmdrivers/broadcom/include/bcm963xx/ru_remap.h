/*
  <:copyright-BRCM:2021:proprietary:standard
  
     Copyright (c) 2021 Broadcom 
     All Rights Reserved
  
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
*/

/****************************************************************************
 *
 * ru_remap.h -- Bcm Wan Driver Head File
 *
 * Description:
 *      local remap functions that don't not use a global array per SoC
 *  Note: internal head file
 *
 * Author: Marc Jalfon
 * Initial version.
 *
 ****************************************************************************/

#ifndef __RU_REMAP_H_INCLUDED__
#define __RU_REMAP_H_INCLUDED__
// #define DEBUG_RU_IOREMAP
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/bcm_log.h>

#if (__SIZEOF_POINTER__ == 8)
#define be_to_cpu be64_to_cpu
#else
#define be_to_cpu be32_to_cpu
#endif

__attribute__((unused)) static void adjust_ru_block_to_virtbase(const ru_block_rec *ru_block, uint64_t virt_base)
{
    uint32_t addr_itr;

    for (addr_itr = 0; addr_itr < ru_block->addr_count; addr_itr++)
    {
        ru_block->addr[addr_itr] += virt_base;
#ifdef DEBUG_RU_IOREMAP
        printk(KERN_INFO "       Remapped Block %s idx = %d Phys Address = 0x%lx to Virtual = 0x%lx\n",
                ru_block->name, addr_itr, (long unsigned int) ru_block->addr[addr_itr], (long unsigned int) ru_block->addr[addr_itr]);
#endif
    }
}

/* This function is a utility to fix virtual address of any RU/HAL based drivers when running in Kernel
 * after calling this function all RU addresses will turn virtual and no translation is needed during read/write
 * ru_blocks[] is the main ru object of the block
 * might not be 64bit proof */
__attribute__((unused)) static void adjust_ru_block_array_to_virtbase(const void *virt_address, const unsigned long phys_address, const ru_block_rec *ru_blocks[])
{
    uint32_t blk;
    /* new_address = old_address + block_virtual_base - block_physical_base */
    uintptr_t virt_base = (unsigned long)virt_address - phys_address;

    for (blk = 0; ru_blocks[blk]; blk++)
    {
        adjust_ru_block_to_virtbase(ru_blocks[blk], virt_base);
    }
}

/* read the first 4 or 8 bytes of reg, the physical address */
__attribute__((unused)) static int scan_reg_property(const struct device_node *np, const char *pr, uintptr_t *var)
{
    const struct property *prop;

    if ((prop = of_find_property(np, pr, NULL)) && prop->value)
    {
        *var = be_to_cpu(*(uintptr_t *)prop->value);
        return 0;
    }
    else
    {
        printk("property %s not found\n", pr);
        return 1;
    }
}

__attribute__((unused)) static int scan_reg_by_compatible(const char *compatible, uintptr_t *addr, uint32_t *len, int index)
{
    int ret = 1;
    unsigned int flags;
    u64 size;
    struct device_node *np = of_find_compatible_node(NULL, NULL, compatible);

    if (!np)
    {
        printk("%s: Failed to find Device tree configuration", __FUNCTION__);
        return ret;
    }
  
    *addr = be_to_cpu(*(uintptr_t *)of_get_address(np, index, &size, &flags));
    if (!*addr)
    {
        printk("%s: Failed to find reg in device tree", __FUNCTION__);
        goto Exit;;
    }
    
    *len = (uint32_t)size;
    ret = 0;

Exit:
    of_node_put(np);
    return ret;
}

__attribute__((unused)) static int ru_remap_unmap_properties(const char *compatible, const uintptr_t ru_addr, const ru_block_rec *ru_blocks[], int map)
{
    int ret = 1;
    struct device_node *np = of_find_compatible_node(NULL, NULL, compatible);
    uintptr_t phys_address;
    static void *virt_address;
    static int num_maps = 0;

    if (map)
    {
        if (!np)
        {
            printk("%s: Failed to find Device tree configuration\n", __FUNCTION__);
            goto Exit;
        }

        if (num_maps++)
        {
            printk("%s: Note: mapping another range\n", __FUNCTION__);
        }
            
        if (!scan_reg_property(np, "reg", &phys_address))
        {
            if (IS_ERR(virt_address = of_io_request_and_map(np, 0, NULL)))                
            {
                printk("%s: of_io_request_and_map failed ret=%ld\n", __FUNCTION__, PTR_ERR(virt_address));
                goto Exit;
            }
            if (ru_addr)
                BCM_ASSERT(phys_address == ru_addr);
            adjust_ru_block_array_to_virtbase(virt_address, phys_address, ru_blocks);
        }
        else
        {
            printk("%s: Failed to find reg in device tree\n", __FUNCTION__);
            goto Exit;
        }
    }
    else
    {
        if (num_maps != 1)
            printk("%s: unexpected call to ru_iounmap_properties(), num_maps=%d\n", __FUNCTION__, num_maps);
        iounmap(virt_address);
    }

    ret = 0;

Exit:
    if (np)
        of_node_put(np);
    return ret;
}

__attribute__((unused)) static int ru_remap_properties(const char *compatible, const uintptr_t ru_addr, const ru_block_rec *ru_blocks[])
{
    return ru_remap_unmap_properties(compatible, ru_addr, ru_blocks, 1);
}

__attribute__((unused)) static int ru_iounmap_properties(void)
{
    return ru_remap_unmap_properties(NULL, 0, NULL, 0);
}

__attribute__((unused)) static void ru_unmap_properties(const char *compatible)
{
    struct device_node *np = of_find_compatible_node(NULL, NULL, compatible);
    uintptr_t phys_address;
    u64 size;
    unsigned int flags;

    if (!np)
    {
        printk("%s: Failed to find Device tree configuration\n", __FUNCTION__);
        return;
    }
    phys_address = be_to_cpu(*(uintptr_t *)of_get_address(np, 0, &size, &flags));

    release_mem_region((phys_addr_t)phys_address, (phys_addr_t)size);

    if (np)
        of_node_put(np);

}

#endif  /* __RU_REMAP__H_INCLUDED__ */

