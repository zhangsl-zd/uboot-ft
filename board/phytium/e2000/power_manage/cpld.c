// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/io.h>
#include "power_manage.h"
#include <e_uart.h>
#include "power_manage.h"
#include "../parameter/parameter.h"
#include "../cpu.h"

#define DS_AG55		0x005c
#define DS_AG53		0x0060
#define DS_AE55		0x0064

void send_cpld_ctr(uint32_t cmd)
{
	p_printf("u-boot : send cmd to cpld : %d \n", cmd);
	int ret, cfg;
	u32 port_level = 0;
	int port = 0;

	//gpio pad
	ret = readl((size_t)(PAD_BASE + DS_AG55));//gpio1_0:func5
	//int pad_recv0 = ret;
	ret |= (1<<0);
	ret &= ~(1<<1);
	ret |= (1<<2);
	writel(ret, (size_t)(PAD_BASE + DS_AG55));

	ret = readl((size_t)(PAD_BASE + DS_AG53));//gpio1_1:func5
	//int pad_recv1 = ret;
	ret |= (1<<0);
	ret &= ~(1<<1);
	ret |= (1<<2);
	writel(ret, (size_t)(PAD_BASE + DS_AG53));

	//send cpld
	//读取之前配置方向寄存器
	cfg = readl((u64)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	//int cfg_recv = ret;
	cfg |= (1<<port);
	port = 1;
	cfg |= (1<<port);
	//配置为输出模式, 0:输入，1:输出
	writel(cfg, (u64)(GPIO1_BASE + GPIO_SWPORTA_DDR));

	//发送电平值
	port_level = readl((u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
	port = 0;
	port_level |= (1<<port);
	writel(port_level, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));//start
	mdelay(2);

	port = 1;
	port_level |= (1<<port);
	int high = port_level;
	port_level &= ~(1<<port);
	int low  = port_level;

	for(int i = 0; i < cmd; i++){
		writel(high, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
		mdelay(1);
		writel(low, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
		mdelay(1);
	}
	mdelay(2);
	port = 0;
	port_level &= ~(1<<port);
	writel(port_level, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));//end

	//恢复(专属，不需要恢复)
	//writel(cfg_recv, (size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	//writel(pad_recv0, (size_t)(PAD_BASE + DS_AG55));//gpio1_0:func5
	//writel(pad_recv1, (size_t)(PAD_BASE + DS_AG53));//gpio1_1:func5
}

int gpio_get_s3_flag(void)
{
	int ret, cfg;
	int port = 0;

	//gpio pad
	ret = readl((size_t)(PAD_BASE + DS_AE55));//gpio1_2:func5
	int pad_recv0 = ret;
	ret |= (1<<0);
	ret &= ~(1<<1);
	ret |= (1<<2);
	writel(ret, (size_t)(PAD_BASE + DS_AE55));
	mdelay(10);

	//读取之前配置方向寄存器
	cfg = readl((size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	int cfg_recv = ret;
	//配置为输出模式, 0:输入，1:输出
	port = 2;
	cfg &= ~(1<<port);
	writel(cfg, (size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));

	//读取当前控io寄存器的值
	ret = readl((size_t)(GPIO1_BASE + GPIO_EXT_PORTA));

	//恢复(专属，不需要恢复)
	writel(cfg_recv, (size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	writel(pad_recv0, (size_t)(PAD_BASE + DS_AE55));

	//判断对应io引脚是否为高电平
	if((1 << port) == ret)
		return  1;
	else
		return  0;
}
