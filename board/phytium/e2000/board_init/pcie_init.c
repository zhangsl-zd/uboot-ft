// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/gic.h>
#include <asm/armv8/mmu.h>
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include "../parameter/parameter.h"
#include "board.h"
#include <e_uart.h>
#include "../cpu.h"

DECLARE_GLOBAL_DATA_PTR;

peu_config_t const peu_base_info  = {
	.magic = PARAMETER_PCIE_MAGIC,
	.version = 0x2,
	.size = 0x100,
	.independent_tree = CONFIG_INDEPENDENT_TREE,
	.base_cfg = ((CONFIG_PCI_PEU1 | (X1X1X1X1<< 1)) << PEU1_OFFSET | \
   				(CONFIG_PCI_PEU0 | (X1X1 << 1))),
   	.ctr_cfg[0].base_config[0] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[0].base_config[1] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
   	.ctr_cfg[1].base_config[0] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[1] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[2] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[3] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[0].equalization[0] = 0x7,
	.ctr_cfg[0].equalization[1] = 0x7,
	.ctr_cfg[1].equalization[0] = 0x7,
	.ctr_cfg[1].equalization[1] = 0x7,
	.ctr_cfg[1].equalization[2] = 0x7,
	.ctr_cfg[1].equalization[3] = 0x7,
};

void pcie_init(void)
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;
#if 0
	if((get_parameter_info(PM_PCIE, buff, sizeof(buff)))){ //从参数表中获取参数
		p_printf("use default parameter\n");
		memcpy(buff, &peu_base_info, sizeof(peu_base_info));
	}
#endif

	p_printf("use default parameter\n");
	memcpy(buff, &peu_base_info, sizeof(peu_base_info));

	arm_smccc_smc(CPU_INIT_PCIE, 0, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("error ret  %lx\n", res.a0);
		while(1);
	}
}
