//SPDX-License-Identifier: GPL-2.0+ */
/*
* header file for Phytium E2000 SoC
*
* Copyright (C) 2022, Phytium Technology Co., Ltd.
*/

#ifndef __E2000_H
#define __E2000_H

#define PHYS_SDRAM_1				0x80000000  /* SDRAM Bank #1 start address */
#define PHYS_SDRAM_1_SIZE			0x74000000  
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_INIT_SP_ADDR     (0x30c00000 + 0x1a000)

/*PLAT AHCI*/
#ifdef CONFIG_PHYTIUM_AHCI
#define CONFIG_SCSI_AHCI_PLAT
#endif

/*DDR TRAIN*/
/*#define PHYTIUM_SAVE_TRAIN_DATA*/
#ifdef PHYTIUM_SAVE_TRAIN_DATA
#define TRAIN_DATA_ADDR				0x600000//flash 6M
#endif

/*use pinctrl tools*/
#define PHYTIUM_PINCTRL_TOOLS

/* BOOT */
#define CONFIG_SYS_BOOTM_LEN		(60 * 1024 * 1024)

#if 0
#define CONFIG_EXTRA_ENV_SETTINGS	\
 					"kernel_addr=0x90100000\0"	   \
 					"initrd_addr=0x95000000\0"	   \
					"fdt_addr=0x3827C000\0"		\
					"ft_fdt_addr=0x90000000\0"		\
					"ft_fdt_name=boot/dtb/e2000.dtb\0"		\
 					"scsi_load_kernel=ext4load scsi 0:2 $kernel_addr boot/uImage-2004\0"	\
 					"scsi_load_initrd=ext4load scsi 0:2 $initrd_addr initrd.img-4.19.0.e2000\0"	\
 					"scsi_load_fdt=ext4load scsi 0:2 $ft_fdt_addr $ft_fdt_name\0" \
 					"nvme_load_kernel=ext4load nvme 0:2 $kernel_addr boot/uImage-2004\0"	\
 					"nvme_load_initrd=ext4load nvme 0:2 $initrd_addr initrd.img-4.19.0.e2000\0"	\
 					"nvme_load_fdt=ext4load nvme 0:2 $ft_fdt_addr $ft_fdt_name\0" \
 					"scsi_boot_os=setenv bootargs 'console=115200 earlycon=pl011,0x2800d000 root=/dev/sda2 rw'; run scsi_load_kernel; bootm $kernel_addr -:- $fdt_addr\0" \
 					"nvme_boot_os=setenv bootargs 'console=115200 earlycon=pl011,0x2800d000 root=/dev/nvme0n1p2 rw'; run nvme_load_kernel; bootm $kernel_addr -:- $fdt_addr\0" \
 					"ipaddr=202.197.67.2\0"       \
 					"gatewayip=202.197.67.1\0"      \
 					"netdev=eth0\0"                 \
 					"netmask=255.255.255.0\0"       \
 					"ethaddr=00:11:22:33:44:55\0"    \
 					"eth1addr=10:22:33:44:55:66\0"	\
 					"serverip=202.197.67.3\0" 	\
                    "distro_bootcmd=run scsi_boot_os; run nvme_boot_os\0"
#else//BMC
#define CONFIG_EXTRA_ENV_SETTINGS  \
                   "bmc_kernel_addr=0x38600000\0"     \
                   "bmc_initrd_addr=0x39e00000\0"     \
                   "bmc_fdt_addr=0x38500400\0"     \
                   "bmc_initrd_size=0x0\0"     \
                   "bmc_boot_os=bootm $bmc_kernel_addr $bmc_initrd_addr:$bmc_initrd_size $bmc_fdt_addr\0" \
                   "ipaddr=202.197.67.2\0"       \
                   "gatewayip=202.197.67.1\0"      \
                   "netdev=eth0\0"                 \
                   "netmask=255.255.255.0\0"       \
                   "ethaddr=00:11:22:33:44:55\0"    \
                   "eth1addr=10:22:33:44:55:66\0"  \
                   "serverip=202.197.67.3\0"   \
                    "distro_bootcmd=run bmc_boot_os\0"
#endif
#endif	/* __E2000_H */

