// SPDX-License-Identifier: GPL-2.0+
/*
 * PHYTUIM FT-2000/4 SDCI
 *
 *
 * Author: hongbin ji<jihongbin@phytium.com>
 *
 */
#include <common.h>
#include <clk.h>
#include <errno.h>
#include <malloc.h>
#include <mmc.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm-generic/gpio.h>

#ifdef CONFIG_DM_MMC
#include <dm.h>
#endif

#define MMC_CLOCK_MAX   50000000
#define MMC_CLOCK_MIN   400000
#define SDCI_CLOCK_DIV_DEFAELT  749
#define SDCI_TIMEOUT_CMD_VALUE  1000000
#define SDCI_TIMEOUT_DATA_VALUE 2000000

/* SDCI CMD_SETTING_REG */
#define SDCI_CMD_ADTC_MASK          (0x00008000)    /* ADTC 类型指令*/
#define SDCI_CMD_CICE_MASK          (0x00000010)    /* 命令响应执行索引检查 */
#define SDCI_CMD_CRCE_MASK          (0x00000008)    /* 命令响应执行crc检查 */
#define SDCI_CMD_RES_NONE_MASK      (0x00000000)    /* 无响应 */
#define SDCI_CMD_RES_LONG_MASK      (0x00000001)    /* 长响应 */
#define SDCI_CMD_RES_SHORT_MASK     (0x00000002)    /* 短响应 */

#define SDCI_CMD_INDEX_MASK     (0x00003F00)
#define SDCI_CMD_INDEX_SHIFT    (8)

/* SDCI SOFTWARE_RESET_REG */
#define SDCI_RESET_CFCLF    (0x00000008)    /* 卡插入拔出状态触发标志清 0 */
#define SDCI_BDRST          (0x00000004)    /* DMA BD 复位 */
#define SDCI_SRST           (0x00000001)    /* 控制器软复位 */

/* SDCI NORMAL_INT_EN_REG */
#define SDCI_NORMAL_EEI_EN  (0x00008000)    /* 命令错误中断使能 */
#define SDCI_NORMAL_ECR_EN  (0x00000002)    /* 卡拔出去中断使能 */
#define SDCI_NORMAL_ECC_EN  (0x00000001)    /* 命令完成中断使能 */

/* SDCI ERROR_INT_EN_REG */
#define SDCI_ERROR_ECIE_EN      (0x00000008)    /* 命令索引错误中断使能 */
#define SDCI_ERROR_ECCCRCE_EN   (0x00000002)    /* 命令 CRC 错误中断使能 */
#define SDCI_ERROR_ECTE_EN      (0x00000001)    /* 命令超时中断使能 */

/* SDCI BD_ISR_EN_REG */
#define SDCI_BD_EDAISE_EN       (0X00008000)    /* DMA 错误中断使能 */
#define SDCI_BD_RESPE_EN        (0X00000100)    /* 读 SD 卡操作，AXI BR 通道完成中断 */
#define SDCI_BD_EDATFRAXE_EN    (0x00000080)    /* AXI 总线错误中断使能 */
#define SDCI_BD_ENRCRCE_EN      (0x00000040)    /* CRC 校验错误中断使能 */
#define SDCI_BD_ETRE_EN         (0x00000020)    /* 传输错误中断使能 */
#define SDCI_BD_ECMDE_EN        (0x00000010)    /* 命令响应错误中断使能 */
#define SDCI_BD_EDTE_EN         (0x00000008)    /* 数据超时中断使能 */
#define SDCI_BD_ETRS_EN         (0x00000001)    /* DMA 传输完成中断使能 */

/* SDCI NORMAL_INT_STATUS_REG */
#define SDCI_NORMAL_EI_STATUS   (0x00008000)    /* 命令错误中断 */
#define SDCI_NORMAL_CR_STATUS   (0x00000002)    /* 卡移除中断 */
#define SDCI_NORMAL_CC_STATUS   (0x00000001)    /* 命令完成中断 */

/* SDCI ERROR_INT_STATUS_REG */
#define SDCI_ERROR_CNR_ERROR_STATUS     (0x00000010)    /* 命令响应错误中断 */
#define SDCI_ERROR_CIR_ERROR_STATUS     (0x00000008)    /* 命令索引错误中断 */
#define SDCI_ERROR_CCRCE_ERROR_STATUS   (0x00000002)    /* 命令 CRC 错误中断 */
#define SDCI_ERROR_CTE_ERROR_STATUS     (0x00000001)    /* 命令超时错误中断 */

/* SDCI BD_ISR_REG */
#define SDCI_BD_DAIS_ERROR_STATUS       (0x00008000)    /* DMA 错误中断 */
#define SDCI_BD_RESPE_STATUS            (0x00000100)    /* 读 SD 卡操作，AXI BR 通道完成中断*/
#define SDCI_BD_DATFRAX_ERROR_STATUS    (0x00000080)    /* axi 总线强制释放中断 */
#define SDCI_BD_NRCRC_ERROR_STATUS      (0x00000040)    /* 无 CRC 响应中断 */
#define SDCI_BD_TRE_ERROR_STATUS        (0x00000020)    /* CRC 响应错误中断 */
#define SDCI_BD_CMDE_ERROR_STATUS       (0x00000010)    /* 命令响应错误中断 */
#define SDCI_BD_DTE_ERROR_STATUS        (0x00000008)    /* 数据超时中断*/
#define SDCI_BD_TRS_STATUS              (0x00000001)    /* DMA 传输完成中断 */

/* SDCI STATUS_REG */
#define SDCI_STATUS_IDIE                (0x00001000)    /* R */

struct phytium_sdci_registers {
    u32 CONTROLLER_SETTING_REG;
    u32 ARGUMENT_REG;
    u32 CMD_SETTING_REG;
    u32 CLOCK_DIV_REG;
    u32 SOFTWARE_RESET_REG;
    u32 POWER_CONTROL_REG;
    u32 TIMEOUT_CMD_REG;
    u32 TIMEOUT_DATA_REG;
    u32 NORMAL_INT_EN_REG;
    u32 ERROR_INT_EN_REG;
    u32 BD_ISR_EN_REG;
    u32 CAPABILIES_REG;
    u32 SD_DRV_REG;
    u32 SD_SAMP_REG;
    u32 SD_SEN_REG;
    u32 HDS_AXI_REG_CONF1;
    u32 DAT_IN_M_RX_BD;
    u32 __pad1[7];
    u32 DAT_IN_M_TX_BD;
    u32 __pad2[7];
    u32 BLK_CNT_REG;
    u32 __pad3[9];
    u32 HDS_AXI_REG_CONF2;
    u32 __pad4[5];
    u32 NORMAL_INT_STATUS_REG;
    u32 ERROR_INT_STATUS_REG;
    u32 BD_ISR_REG;
    u32 BD_STATUS;
    u32 STATUS_REG;
    u32 BLOCK;
    u32 __pad5[2];
    u32 CMD_RESP_1;
    u32 CMD_RESP_2;
    u32 CMD_RESP_3;
    u32 CMD_RESP_4;
};

struct phytium_sdci_host {
    struct phytium_sdci_registers *base;
    char name[32];
    unsigned int b_max;
    unsigned int voltages;
    unsigned int caps;
    unsigned int clock_in;
    unsigned int clock_min;
    unsigned int clock_max;
    unsigned int clkdiv_init;
    int version2;
    struct mmc_config cfg;
    unsigned int *dma_buf;
    u64 bd_dma;
};

struct phytium_sdci_plat {
    struct mmc_config cfg;
    struct mmc mmc;
};

static void sdr_set_bits(u32 *reg, u32 bs)
{
    u32 val;

    val = readl(reg);
    val |= bs;

    writel(val, reg);
}

static void sdr_clr_bits(u32 *reg, u32 bs)
{
    u32 val;

    val = readl(reg);
    val &= ~bs;

    writel(val, reg);
}

static void phytium_sdci_reset_hw(struct phytium_sdci_host *host)
{
    u32 timeout = 1000;
    sdr_set_bits(&host->base->SOFTWARE_RESET_REG, SDCI_SRST);
    udelay(1);
    sdr_clr_bits(&host->base->SOFTWARE_RESET_REG, SDCI_SRST);

    while(timeout--)
    {
        if (readl(&host->base->STATUS_REG) & SDCI_STATUS_IDIE)
        {
            return ;
        }
    }

    if (timeout-- <= 0)
        debug("reset sdci controller timeout!\n");
}

static int wait_for_command_end(struct mmc *dev, struct mmc_cmd *cmd)
{
    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_CMD_VALUE;
    struct phytium_sdci_host *host = dev->priv;

    statusmask = SDCI_NORMAL_CC_STATUS | SDCI_NORMAL_EI_STATUS;

    do
    {
        status = readl(&host->base->NORMAL_INT_STATUS_REG) & statusmask;
        debug("cmd->status:[0x%08x]\n",readl(&host->base->NORMAL_INT_STATUS_REG));
        udelay(1);
        timeout--;
    } while ((!status) && timeout);

    if(status & SDCI_NORMAL_EI_STATUS)
    {
        status = readl(&host->base->ERROR_INT_STATUS_REG);
        debug("cmd error->status:[0x%08x]\n",readl(&host->base->ERROR_INT_STATUS_REG));

        if (!timeout) {
            printf("CMD%d time out\n", cmd->cmdidx);
            return -ETIMEDOUT;
        } else if ((status & SDCI_ERROR_CCRCE_ERROR_STATUS) &&
            (cmd->resp_type & MMC_RSP_CRC)) {
            printf("CMD%d CRC error\n", cmd->cmdidx);
            return -EILSEQ;
        }
    }

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        cmd->response[0] = readl(&host->base->CMD_RESP_1);
        cmd->response[1] = readl(&host->base->CMD_RESP_2);
        cmd->response[2] = readl(&host->base->CMD_RESP_3);
        cmd->response[3] = readl(&host->base->CMD_RESP_4);
        debug("CMD%d response[0]:0x%08X, response[1]:0x%08X, "
            "response[2]:0x%08X, response[3]:0x%08X\n",
            cmd->cmdidx, cmd->response[0], cmd->response[1],
            cmd->response[2], cmd->response[3]);
    }

    return 0;
}

static inline u32 phytium_sdci_cmd_find_resp(struct mmc_cmd *cmd)
{
    u32 resp;

    switch (cmd->resp_type) {
    case MMC_RSP_R1:
        resp = SDCI_CMD_RES_SHORT_MASK;
        break;
    case MMC_RSP_R1b:
        resp = SDCI_CMD_RES_SHORT_MASK;
        break;
    case MMC_RSP_R2:
        resp = SDCI_CMD_RES_LONG_MASK;
        break;
    case MMC_RSP_R3:
        resp = SDCI_CMD_RES_SHORT_MASK;
        break;
    case MMC_RSP_NONE:
    default:
        resp = SDCI_CMD_RES_NONE_MASK;
        break;
    }

    return resp;
}

static inline u32 phytium_sdci_cmd_prepare_raw_cmd(struct mmc_cmd *cmd)
{
    /*
     * rawcmd :
     *   trty << 14 | opcode << 8 | cmdw << 6 | cice << 4 | crce << 3 | resp
     */
    u32 resp, rawcmd;
    u32 opcode = cmd->cmdidx;

    resp = phytium_sdci_cmd_find_resp(cmd);

    rawcmd = ((opcode << SDCI_CMD_INDEX_SHIFT) & SDCI_CMD_INDEX_MASK);

    rawcmd |= resp;

    if((cmd->resp_type & MMC_RSP_CRC) && (cmd->resp_type != MMC_RSP_R2))
        rawcmd |= SDCI_CMD_CRCE_MASK;

    if(cmd->resp_type & MMC_RSP_OPCODE)
        rawcmd |= SDCI_CMD_CICE_MASK;

    return rawcmd;
}

/* send command to the mmc card and wait for results */
static int do_command(struct mmc *dev, struct mmc_cmd *cmd)
{
    int result = 0;
    u32 sdci_cmd = 0;
    struct phytium_sdci_host * host = dev->priv;

    /* 清除命令状态寄存器 */
    writel(1, &host->base->NORMAL_INT_STATUS_REG);

    /* 设置命令和参数寄存器，触发发送命令 */
    sdci_cmd = phytium_sdci_cmd_prepare_raw_cmd(cmd);

    writel(sdci_cmd, &host->base->CMD_SETTING_REG);
    writel(cmd->cmdarg, &host->base->ARGUMENT_REG);

    debug("cmd->sned raw:0x%08x\n",sdci_cmd);
    debug("cmd->sned reg:0x%08x\n",readl(&host->base->CMD_SETTING_REG));
    debug("arg->sned reg:0x%08x\n",readl(&host->base->ARGUMENT_REG));

    result = wait_for_command_end(dev, cmd);

    /* After CMD2 set RCA to a none zero value. */
    if ((result == 0) && (cmd->cmdidx == MMC_CMD_ALL_SEND_CID))
        dev->rca = 10;

    return result;
}

static int read_bytes(struct mmc *dev, struct mmc_cmd *cmd, u32 *dest, u32 blkcount, u32 blksize)
{
    struct phytium_sdci_host * host = dev->priv;
    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_DATA_VALUE;
    u32 sdci_cmd;

    u32 *buf = host->dma_buf;
    u32 addrl = (u32)((u64)buf & 0xFFFFFFFF);
    u32 addrh = (u32)(((u64)buf >> 32) & 0xFFFFFFFF);

    if(blksize < 512)
    {
        /* ADTC 类命令 */
        /* 清除命令状态寄存器 */
        writel(1, &host->base->NORMAL_INT_STATUS_REG);

        /* 设置命令和参数寄存器，触发发送命令 */
        sdci_cmd = phytium_sdci_cmd_prepare_raw_cmd(cmd);
        sdci_cmd |= SDCI_CMD_ADTC_MASK;
        writel(sdci_cmd, &host->base->CMD_SETTING_REG);
    }

    invalidate_dcache_range((u64)buf, (u64)((u64)buf + (blkcount * blksize)));

    /* 清除数据状态寄存器和命令状态寄存器 */
    writel(1, &host->base->BD_ISR_REG);
    writel(1, &host->base->NORMAL_INT_STATUS_REG);

    /* 复位 DMA BD （必须） */
    sdr_set_bits(&host->base->SOFTWARE_RESET_REG, SDCI_BDRST);
    sdr_clr_bits(&host->base->SOFTWARE_RESET_REG, SDCI_BDRST);

    /* 设置传输块长度 */
    writel(blkcount, &host->base->BLK_CNT_REG);

    /* 设置DMA 描述 （数据低地址，数据高地址，卡低地址，卡高地址） */
    writel(addrl, &host->base->DAT_IN_M_RX_BD);
    writel(addrh, &host->base->DAT_IN_M_RX_BD);
    writel(cmd->cmdarg, &host->base->DAT_IN_M_RX_BD);
    writel(0, &host->base->DAT_IN_M_RX_BD);

    wait_for_command_end(dev, cmd);

    statusmask = SDCI_BD_RESPE_STATUS | SDCI_BD_DAIS_ERROR_STATUS;
    do
    {
        status = readl(&host->base->BD_ISR_REG) & statusmask;
        udelay(1);
        timeout--;
    } while ((!status) && timeout);

    if(status & SDCI_BD_DAIS_ERROR_STATUS)
    {
        printf("sd date read error:[0x%08x]\n",readl(&host->base->BD_ISR_REG));
        if(status & (SDCI_BD_NRCRC_ERROR_STATUS | SDCI_BD_TRE_ERROR_STATUS | SDCI_BD_CMDE_ERROR_STATUS))
            return -EILSEQ;
        if(!timeout)
        {
            printf("sd date read timeout:[0x%08x]\n",readl(&host->base->BD_ISR_REG));
            return -ETIMEDOUT;
        }
    }

    memcpy(dest, buf, blkcount * blksize);

    return 0;
}

static int write_bytes(struct mmc *dev, struct mmc_cmd *cmd, u32 *src, u32 blkcount, u32 blksize)
{
    struct phytium_sdci_host * host = dev->priv;
    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_DATA_VALUE;
    u32 *buf = host->dma_buf;
    u32 addrl = (u32)((u64)buf & 0xFFFFFFFF);
    u32 addrh = (u32)(((u64)buf >> 32) & 0xFFFFFFFF);

    memcpy(buf, src, blkcount * blksize);
    flush_dcache_range((u64)buf, (u64)((u64)buf + blkcount * blksize));

    /* 清除数据状态寄存器和命令状态寄存器 */
    writel(1, &host->base->BD_ISR_REG);
    writel(1, &host->base->NORMAL_INT_STATUS_REG);

    /* 复位 DMA BD （必须） */
    sdr_set_bits(&host->base->SOFTWARE_RESET_REG, SDCI_BDRST);
    sdr_clr_bits(&host->base->SOFTWARE_RESET_REG, SDCI_BDRST);

    /* 设置传输块长度 */
    writel(blkcount, &host->base->BLK_CNT_REG);

    /* 设置DMA 描述 （数据低地址，数据高地址，卡低地址，卡高地址） */
    writel(addrl, &host->base->DAT_IN_M_TX_BD);
    writel(addrh, &host->base->DAT_IN_M_TX_BD);
    writel(cmd->cmdarg, &host->base->DAT_IN_M_TX_BD);
    writel(0, &host->base->DAT_IN_M_TX_BD);

    wait_for_command_end(dev, cmd);

    statusmask = SDCI_BD_TRS_STATUS | SDCI_BD_DAIS_ERROR_STATUS;
    do
    {
        status = readl(&host->base->BD_ISR_REG) & statusmask;
        udelay(1);
        timeout--;
    } while ((!status) && timeout);

    if(status & SDCI_BD_DAIS_ERROR_STATUS)
    {
        printf("sd date write error:[0x%08x]\n",readl(&host->base->BD_ISR_REG));
        if(status & (SDCI_BD_NRCRC_ERROR_STATUS | SDCI_BD_TRE_ERROR_STATUS | SDCI_BD_CMDE_ERROR_STATUS))
            return -EILSEQ;
        if(!timeout)
        {
            printf("sd date write timeout:[0x%08x]\n",readl(&host->base->BD_ISR_REG));
            return -ETIMEDOUT;
        }
    }

    return 0;
}

static int do_data_transfer(struct mmc *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int error = -ETIMEDOUT;

    if (data->flags & MMC_DATA_READ) {
        error = read_bytes(dev, cmd, (u32 *)data->dest, (u32)data->blocks,
                (u32)data->blocksize);
    } else if (data->flags & MMC_DATA_WRITE) {
        error = write_bytes(dev, cmd, (u32 *)data->src, (u32)data->blocks,
                        (u32)data->blocksize);
    }

    return error;
}

static int host_request(struct mmc *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int result;

    if (data)
        result = do_data_transfer(dev, cmd, data);
    else
        result = do_command(dev, cmd);

    return result;
}

static int  host_set_ios(struct mmc *dev)
{
    struct phytium_sdci_host *host = dev->priv;
    u32 div;

    if(dev->clock)
    {
        div = (host->clock_in / (2 * dev->clock) - 1);
        writel(div, &host->base->CLOCK_DIV_REG);

        if(dev->clock > 400000)
            writel(11, &host->base->SD_SAMP_REG);
    }

    return 0;
}

#ifdef CONFIG_DM_MMC
static void phytium_sdci_init(struct phytium_sdci_host *host)
{
    phytium_sdci_reset_hw(host);

    /* Disable card detection */
    writel(0, &host->base->SD_SEN_REG);

    /* Disable all interrupts */
    writel(0, &host->base->NORMAL_INT_EN_REG);
    writel(0, &host->base->ERROR_INT_EN_REG);
    writel(0, &host->base->BD_ISR_EN_REG);

    /* clear status register */
    writel(1, &host->base->NORMAL_INT_STATUS_REG);
    writel(1, &host->base->ERROR_INT_STATUS_REG);
    writel(1, &host->base->BD_ISR_REG);

    writel(0x0F00, &host->base->CONTROLLER_SETTING_REG);

    writel(0, &host->base->SD_DRV_REG);
    writel(0, &host->base->SD_SAMP_REG);

    /* Configure to default cmd data timeout */
    writel(0xFFFFFFFF, &host->base->TIMEOUT_CMD_REG);
    writel(0xFFFFFFFF, &host->base->TIMEOUT_DATA_REG);

    debug("init phytium sdci hardware done!\n");
}

static int phytium_sdci_probe(struct udevice *dev)
{
    struct phytium_sdci_plat *pdata = dev_get_plat(dev);
    struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
    struct mmc *mmc = &pdata->mmc;
    struct phytium_sdci_host *host = dev->priv_;
    struct mmc_config *cfg = &pdata->cfg;
    u32 clk;

    host->clkdiv_init = SDCI_CLOCK_DIV_DEFAELT;

    clk = dev_read_u32_default(dev, "clock", 600000000);
    host->clock_in = clk;

    cfg->name = "PHYTIUM SDCI";
    cfg->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
    cfg->host_caps |= MMC_MODE_4BIT;
    cfg->f_min = MMC_CLOCK_MIN;
    cfg->f_max = MMC_CLOCK_MAX;
    cfg->b_max = 8;

    host->dma_buf = memalign(4096, 8*512);
    if(!host->dma_buf)
    {
        debug("alloc buffer failed!\n");
        return -ENOMEM;
    }

    phytium_sdci_init(host);

    mmc->priv = host;
    mmc->dev = dev;
    upriv->mmc = mmc;

    return 0;
}

int phytium_sdci_bind(struct udevice *dev)
{
    struct phytium_sdci_plat *plat = dev_get_plat(dev);

    return mmc_bind(dev, &plat->mmc, &plat->cfg);
}

static int dm_host_request(struct udevice *dev, struct mmc_cmd *cmd,
               struct mmc_data *data)
{
    struct mmc *mmc = mmc_get_mmc_dev(dev);

    return host_request(mmc, cmd, data);
}

static int dm_host_set_ios(struct udevice *dev)
{
    struct mmc *mmc = mmc_get_mmc_dev(dev);

    return host_set_ios(mmc);
}

static int dm_mmc_getcd(struct udevice *dev)
{
    struct phytium_sdci_host *host = dev->priv_;

    if(readl(&host->base->STATUS_REG) & (1<<26))
        return 1;
    else
        return 0;
}

static const struct dm_mmc_ops phytium_dm_sdc_ops = {
    .send_cmd = dm_host_request,
    .set_ios = dm_host_set_ios,
    .get_cd = dm_mmc_getcd,
};

static int phytium_sdci_ofdata_to_platdata(struct udevice *dev)
{
    struct phytium_sdci_host *host = dev->priv_;
    fdt_addr_t addr;

    addr = dev_read_addr(dev);
    if (addr == FDT_ADDR_T_NONE)
        return -EINVAL;

    host->base = (void *)addr;

    return 0;
}

static const struct udevice_id phytium_sdci_match[] = {
    { .compatible = "phytium,sdci" },
    { .compatible = "phytium,mci" },
    { /* sentinel */ }
};

U_BOOT_DRIVER(phytium_sdci) = {
    .name = "phytium_sdci",
    .id = UCLASS_MMC,
    .of_match = phytium_sdci_match,
    .ops = &phytium_dm_sdc_ops,
    .probe = phytium_sdci_probe,
    .of_to_plat = phytium_sdci_ofdata_to_platdata,
    .bind = phytium_sdci_bind,
    .priv_auto = sizeof(struct phytium_sdci_host),
    .plat_auto = sizeof(struct phytium_sdci_plat),
};
#endif
