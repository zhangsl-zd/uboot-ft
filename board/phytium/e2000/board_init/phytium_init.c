// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/gic.h>
#include "cpu.h"
#include <asm/armv8/mmu.h>
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include <e_uart.h>
#include "board.h"
#include "../cpu.h"
#include "../cpu.h"
#include "../parameter/parameter.h"

DECLARE_GLOBAL_DATA_PTR;

void inline set_freq_div(uint8_t rank)
{
	uint32_t temp;

	if(rank > 7)
		rank = 7;

	temp = readl(QSPI_ADDR_READ_CFG_REG);
	temp = (temp & (~0x7)) | rank;

	writel(temp, QSPI_ADDR_READ_CFG_REG);
}

void set_flash_speed(void)
{
	uint32_t speed_rank;

	speed_rank = pm_get_qspi_freq_rank();
	p_printf("get flash speed = %d\n", speed_rank);

	speed_rank += 4;
	set_freq_div(speed_rank);
	p_printf("flash set done\n");
}

typedef struct qspi_ops_s{
	uint32_t id;
	void (*fun)(void);
	uint8_t str[16];
}qspi_ops_t;

//QUAD set
void w25q128fw_config(void)
{
	writel(0x06000000, QSPI_CMD_PORT_REG);
	writel(0x1, QSPI_LD_PORT_REG);

	writel(0x31402000, QSPI_CMD_PORT_REG);
	writel(0x2, QSPI_LD_PORT_REG);

	writel((0x6B<<24) | (0x24 << 16) |(0x7 << 4) | (1 << 3), QSPI_ADDR_READ_CFG_REG);
}

const qspi_ops_t qspi_ops[] = {
	{0x01182001, NULL, "flash debug"},
	{0x001860ef, w25q128fw_config, "w25q128fw"},
	{0x001960ef, w25q128fw_config, "w25q128fw"},
	{0x0, NULL, "default flash"}
};

uint32_t get_qspi_ops(void)
{
	uint32_t jedec_id, i;

	writel(0x9F002040, QSPI_CMD_PORT_REG);
	dsb();
	isb();
	jedec_id = readl(QSPI_LD_PORT_REG);

	for(i = 0; 0 != qspi_ops[i].id; i++){
		if(qspi_ops[i].id == jedec_id)
			break;
	}
	p_printf("flash tpye = %s\n", qspi_ops[i].str);
	return i;
}

void qspi_flash_setup(void)
{
	void (*ops_fun)(void);
	uint32_t ops_id;

	ops_id = get_qspi_ops();

	if(0 != ops_id){
		ops_fun = qspi_ops[ops_id].fun;

		if(NULL != ops_fun)
			ops_fun();

		set_flash_speed();
	}
}

/*core 安全等级配置*/
void core_security_level_cfg(void)
{
	struct arm_smccc_res res;
	//level: 0, 1, 2
	uint8_t level_cfg = 0x0;

	printf("security level cfg...\n");
	arm_smccc_smc(CPU_SECURITY_CFG, 0, level_cfg, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

/*get reset source*/
uint32_t get_reset_source(void)
{
	struct arm_smccc_res res;

	p_printf("get reset source\n");
	arm_smccc_smc(CPU_GET_RST_SOURCE, 0, 0, 0, 0, 0, 0, 0, &res);
	p_printf("reset source = %lx \n", res.a0);
	return res.a0;
}

common_config_t const common_base_info = {
	.magic = PARAMETER_COMMON_MAGIC,
	.version = 0x1,
	.core_bit_map = 0x3333,
};

/*sec init*/
void sec_init(uint8_t s3_flag)
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;
	uint32_t s3_fun = 0;

	/*从参数表中获取参数*/
	if((get_parameter_info(PM_COMMON, buff, sizeof(buff)))){
		printf("use default parameter\n");
		memcpy(buff, &common_base_info, sizeof(common_base_info));
	}
	if(s3_flag)
		s3_fun = 1;
	arm_smccc_smc(CPU_INIT_SEC_SVC, s3_fun, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

int e2000_phy_init(void)
{
	/*调试将全部phy domain选上*/
	uint32_t phy_domain = 0x3f;
	uint32_t phy_sel = pm_get_phy_sel_all();
	uint32_t mode = pm_get_mac_mode_all()<<8;
	uint32_t speed = pm_get_speed_mode_all()<<8;
	struct arm_smccc_res res;

	p_printf("phy_sel = 0x%x\n", phy_sel);
	arm_smccc_smc(SET_PHY_MODE, phy_domain, phy_sel, mode, speed, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("set phy error ret  %lx\n", res.a0);
		while(1);
	}

#if 0
	if(pm_get_gsd_phy2_sel() == 0x0){
		printf("sgmii2 pad.\n");
		if(pm_get_mac2_mode_sel() == RGMII_WD_0)
			gmac_pad_set(GMAC_PAD_RGMII, 0);
		if(pm_get_mac2_mode_sel() == RMII_WD_0)
			gmac_pad_set(GMAC_PAD_RMII, 0);
	}

	if(pm_get_gsd_phy3_sel() == 0x0){
		printf("sgmii3 pad.\n");
		if(pm_get_mac3_mode_sel() == RGMII_WD_0)
			gmac_pad_set(GMAC_PAD_RGMII, 1);
		if(pm_get_mac3_mode_sel() == RMII_WD_0)
			gmac_pad_set(GMAC_PAD_RMII, 1);
	}
#endif
	return 0;
}

#ifdef PHYTIUM_PINCTRL_TOOLS
int e2000_pin_ctrl(void)
{
	int groups, size;
	uint32_t buffer[256] = {0};//max 1k

	if (readl((size_t)(FLASH_BASE + PAD_CFG_BASE + PAD_MAGIC_OFFSET)) != PARAMETER_PAD_MAGIC) {
		p_printf("ERROR: invalid gpio pad data!\n");
		return -1;
	}

	groups = readl((size_t)(FLASH_BASE + PAD_CFG_BASE + PAD_GROUPS_OFFSET));
	size = groups * 2 * (sizeof(int));
	memcpy((void *)buffer, (void *)((size_t)(FLASH_BASE + PAD_CFG_BASE + PAD_DATA_OFFSET)), size);
	for(int i=0; i<groups; i++){
		p_printf("value = 0x%x, addr = 0x%x\n", buffer[i*2+1], buffer[i*2]);
		writel(buffer[i*2+1], (u64)buffer[i*2]);
	}

	return 0;
}
#endif

