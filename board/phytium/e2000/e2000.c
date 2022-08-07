// SPDX-License-Identifier: GPL-2.0+
/*
 * Board init entry for Phytium E2000 SoC
 *
 * Copyright (C) 2022, Phytium Technology Co., Ltd.
 */
#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#include <scsi.h>
#include <blk.h>
#include <init.h>
#include <ahci.h>
#include <nvme.h>
#include <e_uart.h>
#include "cpu.h"
#include "board_init/board.h"
#include "power_manage/power_manage.h"
#include "pfdi/pfdi_service.h"

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	uint32_t s3_flag;

	gd->mem_clk = 0;
	gd->ram_size = PHYS_SDRAM_1_SIZE;

	s3_flag = 0;//get_s3_flag();
	//printf("s3_flag = %d\n", s3_flag);
	//pwr_s3_clean();

	printf("Phytium ddr init \n");
	e2000_ddr_init(s3_flag);

	sec_init(s3_flag);
	printf("PBF relocate done \n");

	register_pfdi();
	e2000_phy_init();

	return 0;
}

int board_init(void)
{
#ifdef PHYTIUM_PINCTRL_TOOLS
	e2000_pin_ctrl();
#endif
	return 0;
}

void reset_cpu(void)
{
	struct arm_smccc_res res;
	printf("run in reset_cpu\n");
	arm_smccc_smc(0x84000009, 0, 0, 0, 0, 0, 0, 0, &res);
	printf("reset cpu error , %lx\n", res.a0);
}

int mach_cpu_init(void)
{
	pll_init();

	return 0;
}

int board_early_init_f (void)
{
	pcie_init();

	return 0;
}

int board_early_init_r (void)
{
	return 0;
}

static struct mm_region e2000_mem_map[] = {
	{
	        .virt = 0x0UL,
	        .phys = 0x0UL,
	        .size = 0x80000000UL,
	        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |PTE_BLOCK_NON_SHARE|PTE_BLOCK_PXN|PTE_BLOCK_UXN
	},
	{
	        .virt = (u64)PHYS_SDRAM_1,
	        .phys = (u64)PHYS_SDRAM_1,
	        .size = (u64)PHYS_SDRAM_1_SIZE,
	        .attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |PTE_BLOCK_NS|PTE_BLOCK_INNER_SHARE
	},
#ifdef  PHYS_SDRAM_2
	{
	        .virt = (u64)PHYS_SDRAM_2,
	        .phys = (u64)PHYS_SDRAM_2,
	        .size = (u64)PHYS_SDRAM_2_SIZE,
	        .attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |PTE_BLOCK_NS|PTE_BLOCK_INNER_SHARE
	},
#endif
	{
	        0,
	}
};
struct mm_region *mem_map = e2000_mem_map;

int last_stage_init(void)
{
	int ret;

	pci_init();
	scsi_scan(true);
	ret = nvme_scan_namespace();
	if(ret != 0)
		printf("nvme scan failed\n");

#ifdef PHYTIUM_SAVE_TRAIN_DATA
	save_train_data();
#endif

	return 0;
}
