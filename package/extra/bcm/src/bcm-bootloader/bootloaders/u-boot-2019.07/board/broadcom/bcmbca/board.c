/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 Broadcom Ltd.
 */
#include <common.h>
#include <fdtdec.h>
#include <dm.h>
#include <mtd.h>
#include <cli.h>

#include <asm/sections.h>
#include <linux/ctype.h>
#include <stdlib.h>
#include <string.h>
#include <environment.h>
#if defined(CONFIG_ARM64)
#include <asm/armv8/mmu.h>
#else
#include <asm/armv7.h>
#endif
#include <asm/arch/misc.h>
#include "bca_common.h"
#if defined(CONFIG_BCMBCA_PMC)
#include "pmc_drv.h"
#endif
#if defined(CONFIG_BCMBCA_BUTTON)
#include "bcmbca_button.h"
#endif
#include <linux/io.h>
#if defined(CONFIG_WATCHDOG)
#include <wdt.h>
#endif
#if defined(CONFIG_BCMBCA_UBUS4)
#include "bcm_ubus4.h"
#endif
#if defined(CONFIG_BCMBCA_STRAP)
#include "bcm_strap_drv.h"
#endif
#ifdef CONFIG_BCMBCA_VFBIO
#include "vfbio.h"
#endif
#if defined(CONFIG_BCMBCA_ITC_RPC)
#include "itc_rpc.h"
#include "ba_svc.h"
void board_prep_linux(bootm_headers_t *images)
{
	rpc_exit();
}
#endif

int nand_register(int devnum, struct mtd_info *mtd);

#if defined(CONFIG_BCM_BCA_LED)
void bca_led_probe(void);
#endif

void bcmbca_xrdp_eth_init(void);

void pmc_init(void);
#if defined(CONFIG_BCMBCA_BUTTON)
void reset_button_init(void);

/* since btn_poll declaration doesn't match register_cli_job_cb
 * we define this static function to reduce the warnings
 */
static inline void local_btn_poll(void)
{
	btn_poll();
}
#endif

#if defined(CONFIG_BCM_BOOTSTATE)
#include "bcm_bootstate.h"
#endif

#if defined(CONFIG_BCM_THERMAL)
#include "bcm_thermal.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

__weak void boot_secondary_cpu(unsigned long vector)
{
}

__weak int set_cpu_freq(int freqMHz)
{
	return 0;
}
/* overrideable callback to insert within board _init */
__weak int board_sdk_init_e(void)
{
	return 0;
}

#if !defined(CONFIG_TPL_ATF)

#if !defined(CONFIG_ARM64)
void(*__waitloop_rel)(void) = (void(*)(void))CONFIG_SMP_PEN_ADDR;
void __iomem *boot_addr_reg = NULL;

void __iomem *smp_get_core_boot_addr(void)
{
	const void* fdt = gd->fdt_blob;
	u32 addr = 0;
	int cpus_offset, offset;
	const char *prop;
	const fdt32_t * nodep = NULL;

	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		return NULL;

	for (offset = fdt_first_subnode(fdt, cpus_offset);
	     offset >= 0;
	     offset = fdt_next_subnode(fdt, offset)) {
		prop = fdt_getprop(fdt, offset, "device_type", NULL);
		if (!prop || strcmp(prop, "cpu"))
			continue;

		nodep = fdt_getprop(fdt, offset, "cpu-release-addr", (int*)&addr);
		if (nodep) {
			addr = fdt32_to_cpu(*nodep);
			printf("Using cpu release address 0x%x\n", addr);
			break;
		}
	}

	return (void __iomem *)addr; 
}

void smp_set_core_boot_addr(unsigned long addr, int corenr)
{
}

/* stack is not available in the below tow functions during
   this early smp boost strap code */
static void __waitloop(void)
{
	/* use register so secondary cpu don't load the variable from memory again
	 which may not be valid when linux wait it up */
	register void __iomem *reg = boot_addr_reg;

	while(!readl(reg))
		wfi();
	/* do nonsecure entry */
	__asm__(".arch_extension sec");
	__asm__("mov ip, %0" : : "r" (readl(reg)));
	__asm__("smc #0");
}

void smp_waitloop(unsigned previous_address)
{
#ifdef CONFIG_ARM_CORTEX_A15_CVE_2017_5715
	/* Enable invalidates of BTB for secondary cpu */
	asm volatile ("mrc p15, 0, r1, c1, c0, 1");
	asm volatile ("orr r1, r1, #0x1");
	asm volatile ("mcr p15, 0, r1, c1, c0, 1");
#endif

#ifdef CONFIG_BCM63138
	/* set non-secure ACR. Allow SMP, L2ERR, CP10 and CP11 and Enable Neon/VFP bit 
	 for non-secure mode */
	asm volatile ("movw	r0, #0x0c00");
	asm volatile ("movt	r0, #0x0006");
	asm volatile ("mcr	p15, 0, r0, c1, c1, 2");
		  
	/* set FW bit in ACTRL for 63138 */
	asm volatile ("mrc p15, 0, r1, c1, c0, 1");
	asm volatile ("orr r1, r1, #0x1");
	asm volatile ("mcr p15, 0, r1, c1, c0, 1");

	/* invalid L1 cache */	
	asm volatile ("bl __v7_invalidate_dcache_all");
#endif
	__waitloop_rel();
}
#endif

void board_boot_smp(void)
{
	unsigned long vector;
#if defined(CONFIG_ARM64)
	vector  = (unsigned long)&_start;
#else
	unsigned long aligned_addr;

	vector  = (unsigned long)_smp_pen;
	boot_addr_reg = smp_get_core_boot_addr();
	if (!boot_addr_reg) {
		printf("cpu_release_addr not found, secondary cpu failed to boot\n");
		return;
	}
	writel(0, boot_addr_reg);
	/* flush the memory that store this boot_addr variable so other cpu can see it */
	aligned_addr = ALIGN_DOWN(((unsigned long)&boot_addr_reg), CONFIG_SYS_CACHELINE_SIZE);
	flush_dcache_range(aligned_addr, aligned_addr+CONFIG_SYS_CACHELINE_SIZE);

	memcpy(__waitloop_rel, __waitloop, 0x1000);
	flush_dcache_range((unsigned long)__waitloop_rel, (unsigned long)__waitloop_rel + 0x1000);
#endif
	boot_secondary_cpu(vector);
}
#endif

#if defined(CONFIG_SPI_FLASH) && defined(CONFIG_DM_SPI_FLASH)
void board_spinor_init(void)
{
	struct udevice *dev;
	int ret;

	debug("SPI NOR  enabled in configuration, checking for device\n");
	ret = uclass_get_device_by_driver(UCLASS_SPI_FLASH, DM_GET_DRIVER(spi_flash_std), &dev);
	if (ret)
		debug("SPI NOR failed to initialize. (error %d)\n", ret);
	else
		mtd_probe_devices();
}
#endif

__weak void reset_plls(void)
{
}

__weak u32 bcmbca_get_chipid(void)
{
	return (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
}

__weak u32 bcmbca_get_chiprev(void)
{
#if defined (CONFIG_SMC_BASED)
	return ((PERF->RevID & REV_ID_MASK) + 0xA0); /* in case revID is 1 (B) print B0 and not 10 and in case 0 print A0. Both case need to add A0 */
#else  	
	return PERF->RevID & REV_ID_MASK;
#endif	
}

__weak void print_chipinfo(void)
{
	unsigned int chipId = bcmbca_get_chipid();
  	unsigned int revId = bcmbca_get_chiprev();
	printf("Chip ID: BCM%X_%X\n",chipId,revId);
}


#if defined (CONFIG_SMC_BASED) && defined(CONFIG_BCMBCA_ITC_RPC)
static void print_smcos_ver_info(void)
{
	smcbl_ver_t  smcbl_ver;
	smcos_ver_t  smcos_ver;

	ba_get_smcbl_ver(&smcbl_ver);
	ba_get_smcos_ver(&smcos_ver);	
}
#endif	
 

int board_init(void)
{
#if defined(CONFIG_BCMBCA_ITC_RPC) && !defined(CONFIG_BCMBCA_IKOS)
	rpc_tunnel_init(RPC_TUNNEL_ARM_SMC_NS, false);
	rpc_tunnel_init(RPC_TUNNEL_VFLASH_SMC_NS, false);
	rpc_tunnel_init(RPC_TUNNEL_AVS_SMC_NS, false);
#endif
#ifdef CONFIG_BCMBCA_VFBIO
	vfbio_init();
#endif
	board_sdk_init_e();

#if defined(CONFIG_BCMBCA_STRAP)
	bcm_strap_drv_reg();
#endif
#if defined(CONFIG_BCMBCA_PMC)
	bcm_pmc_drv_reg();
    reset_plls();
	pmc_init();
#endif

#if defined(CONFIG_BCMBCA_UBUS4)
	bcm_ubus_drv_init();
#endif

#if defined(CONFIG_BCM_BCA_LED)
	bca_led_probe();
#endif

#if defined (CONFIG_SMC_BASED) && defined(CONFIG_BCMBCA_ITC_RPC)
	print_smcos_ver_info();
#endif	

	print_chipinfo();
#if defined(BUILD_TAG)
	printf("$Uboot: "BUILD_TAG" $\n");
#endif

#if !defined(CONFIG_TPL_ATF)
	board_boot_smp();
#endif

#if defined(CONFIG_BCM_BOOTSTATE)
	bca_bootstate_probe();
#endif

#if defined(CONFIG_BCM_THERMAL)
	bcm_thermal_init();
#endif

#if defined(CONFIG_BCMBCA_BUTTON)
	bcmbca_button_init();
	reset_button_init();
#endif

#if defined(CONFIG_SPI_FLASH) && defined(CONFIG_DM_SPI_FLASH)
	board_spinor_init();
#endif
	return 0;
}

int board_fix_fdt(void * fdt_addr)
{
	return 0;
}

void board_nand_init(void)
{

#if defined(CONFIG_NAND_BRCMNAND) || defined(CONFIG_MTD_SPI_NAND)
	struct udevice *dev;
	int ret;
	int device = 0;
#endif

#if defined(CONFIG_NAND_BRCMNAND)
	debug("parallel NAND enabled in configuration, checking for device\n");
#if defined(CONFIG_BCMBCA_IKOS) && (defined(CONFIG_BCM6888) || defined(CONFIG_BCM68880) || defined(CONFIG_BCM6837))
    ret = -1;
#else
	ret = uclass_get_device_by_driver(UCLASS_MTD, DM_GET_DRIVER(brcm_nand), &dev);
#endif
	if (ret < 0)
		debug("parallel NAND failed to initialize. (error %d)\n", ret);
	else
		device++;
#endif

#if defined(CONFIG_MTD_SPI_NAND)
	debug("SPI NAND enabled in configuration, checking for device\n");
	ret = uclass_get_device_by_driver(UCLASS_MTD, DM_GET_DRIVER(spinand), &dev);
	if (ret < 0)
		debug("SPI NAND failed to initialize. (error %d)\n", ret);
	else
	{
		struct mtd_info *mtd = dev_get_uclass_priv(dev);
		nand_register(device, mtd);
	}
#endif

}
__weak int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}

__weak int board_sdk_late_init_e(void)
{
	return 0;
}

__weak int board_sdk_late_init_l(void)
{
	return 0;
}

int board_late_init(void)
{
	board_sdk_late_init_e();

	init_cli_cb_arr();
	cli_jobs_cb = run_cli_jobs;
	
#ifdef CONFIG_BCMBCA_XRDP_ETH
	register_cli_job_cb(0, bcmbca_xrdp_eth_init);
#endif

#if defined(CONFIG_BCMBCA_BUTTON)
	register_cli_job_cb(100, local_btn_poll);
#endif
	board_sdk_late_init_l();

	return 0;
}


/* FIXME FIXME FIXME 
 * This file has a few things that are common to spl and tpl ( move to board_spltpl.c )
 * and a few things that are universal to uboot on bcmbca devices (keep here)
 * and a few things that are specific to the reference SDK's conventions for environment, flash layout, etc..  (move to bcmbca_uboot.c)
 */

__weak void hook_dram_init(void)
{
	return;
}


int dram_init(void)
{
	if (fdtdec_setup_mem_size_base() != 0)
		printf("fdtdec_setup_mem_size_base() has failed\n");
	else {
		hook_dram_init();
#if defined(CONFIG_ARM64)
		/* update memory size in mmu table*/
		mem_map[0].virt = mem_map[0].phys = gd->ram_base;
		mem_map[0].size = gd->ram_size;
#endif
	}
	return 0;
}

int dram_init_banksize(void)
{
	fdtdec_setup_memory_banksize();

	hook_dram_init();
	return 0;
}

int print_cpuinfo(void)
{
	return 0;
}

__weak void board_mtdparts_default(const char **mtdids, const char **mtdparts)
{
}

void board_quiesce_devices(void)
{
#if defined(CONFIG_WATCHDOG)  
	int diswd = 0;

	if (!(gd->flags & GD_FLG_WDT_READY) || !(gd->watchdog_dev))
		return;
	
	diswd = env_get_hex("disable_wdt", 0);
	if (diswd || IS_ENABLED(CONFIG_BCMBCA_IKOS)) {
		wdt_stop(gd->watchdog_dev);
		printf("watchdog stoppped\n");
	} else {
		/* Pinging WDT last time before linux boot */
		wdt_reset(gd->watchdog_dev);
	}

	return;
#endif
}
