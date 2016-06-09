/*
 * drivers/video/tegra/dc/dpaux_regs.h
 *
 * Copyright (c) 2011-2014, NVIDIA CORPORATION, All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DRIVER_VIDEO_TEGRA_DC_DPAUX_REGS_H__
#define __DRIVER_VIDEO_TEGRA_DC_DPAUX_REGS_H__


#define DPAUX_INTR_EN_AUX				(0x1)
#define DPAUX_INTR_EN_AUX_PLUG_EVENT	(0x1 << 0)
#define DPAUX_INTR_EN_AUX_TX_DONE		(0x1 << 3)
#define DPAUX_INTR_AUX					(0x5)
#define DPAUX_INTR_AUX_PLUG_EVENT_PENDING		(0x1 << 0)
#define DPAUX_INTR_AUX_IRQ_EVENT_PENDING		(0x1 << 2)
#define DPAUX_INTR_AUX_TX_DONE_PENDING		(0x1 << 3)
#define DPAUX_DP_AUXDATA_WRITE_W(i)		     (0x9 + 4*(i))
#define DPAUX_DP_AUXDATA_READ_W(i)		     (0x19 + 4*(i))
#define DPAUX_DP_AUXADDR				(0x29)
#define DPAUX_DP_AUXCTL					(0x2d)
#define DPAUX_DP_AUXCTL_CMDLEN_SHIFT			(0)
#define DPAUX_DP_AUXCTL_CMDLEN_MASK		(0xff)
#define DPAUX_DP_AUXCTL_ADDRESS_ONLY_SHIFT	(8)
#define DPAUX_DP_AUXCTL_ADDRESS_ONLY_MASK	(1 << 8)
#define DPAUX_DP_AUXCTL_ADDRESS_ONLY_TRUE	(1 << 8)
#define DPAUX_DP_AUXCTL_ADDRESS_ONLY_FALSE	(0 << 8)
#define DPAUX_DP_AUXCTL_CMD_SHIFT			(12)
#define DPAUX_DP_AUXCTL_CMD_MASK			(0xf << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CWR			(0 << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CRD			(1 << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CREQWSTAT			(2 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTWR			(4 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTRD			(5 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTREQWSTAT			(6 << 12)
#define DPAUX_DP_AUXCTL_CMD_AUXWR			(8 << 12)
#define DPAUX_DP_AUXCTL_CMD_AUXRD			(9 << 12)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_SHIFT		(16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_MASK		(0x1 << 16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_DONE		(0 << 16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_PENDING		(1 << 16)
#define DPAUX_DP_AUXCTL_RST_SHIFT			(31)
#define DPAUX_DP_AUXCTL_RST_DEASSERT			(0 << 31)
#define DPAUX_DP_AUXCTL_RST_ASSERT			(1 << 31)
#define DPAUX_DP_AUXSTAT				(0x31)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_SHIFT		(28)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_UNPLUG		(0 << 28)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_PLUGGED		(1 << 28)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_SHIFT		(20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_MASK		(0xf << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_IDLE		(0 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_SYNC		(1 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_START1		(2 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_COMMAND		(3 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_ADDRESS		(4 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_LENGTH		(5 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_WRITE1		(6 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_READ1		(7 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_GET_M		(8 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_STOP1		(9 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_STOP2		(10 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_REPLY		(11 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_CLEANUP		(12 << 20)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_SHIFT		(16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_MASK			(0xf << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_ACK			(0 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_NACK			(1 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_DEFER		(2 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_I2CNACK		(4 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_I2CDEFER		(8 << 16)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_SHIFT		(11)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_NOT_PENDING	(0 << 11)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_PENDING		(1 << 11)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_SHIFT		(10)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_NOT_PENDING	(0 << 10)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_PENDING		(1 << 10)
#define DPAUX_DP_AUXSTAT_RX_ERROR_SHIFT			(9)
#define DPAUX_DP_AUXSTAT_RX_ERROR_NOT_PENDING		(0 << 9)
#define DPAUX_DP_AUXSTAT_RX_ERROR_PENDING		(1 << 9)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_SHIFT		(8)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_NOT_PENDING	(0 << 8)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_PENDING		(1 << 8)
#define DPAUX_DP_AUXSTAT_REPLY_M_SHIFT			(0)
#define DPAUX_DP_AUXSTAT_REPLY_M_MASK			(0xff << 0)
#define DPAUX_DP_AUX_SINKSTATLO			(0x35)
#define DPAUX_DP_AUX_SINKSTATLO_DSIV_MASK		(0xff << 8)
#define DPAUX_DP_AUX_SINKSTATLO_DSIV_SHIFT		(8)
#define DPAUX_HPD_CONFIG				(0x3d)
#define DPAUX_HPD_CONFIG_UNPLUG_MIN_TIME_SHIFT		(16)
#define DPAUX_HPD_IRQ_CONFIG				(0x41)
#define DPAUX_DP_AUX_CONFIG				(0x45)
#define DPAUX_HYBRID_PADCTL				(0x49)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_SHIFT	(15)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_DISABLE	(0 << 15)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_ENABLE	(1 << 15)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_SHIFT	(14)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_DISABLE	(0 << 14)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_ENABLE	(1 << 14)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_SHIFT		(12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_DEFAULT_MASK	(0x3 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_60		(0 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_64		(1 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_70		(2 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_56		(3 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_SHIFT		(8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_DEFAULT_MASK	(0x7 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_78		(0 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_60		(1 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_54		(2 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_45		(3 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_50		(4 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_42		(5 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_39		(6 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_34		(7 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVI_SHIFT		(2)
#define DPAUX_HYBRID_PADCTL_AUX_DRVI_DEFAULT_MASK	(0x3f << 2)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_SHIFT		(1)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_DISABLE	(0 << 1)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_ENABLE	(1 << 1)
#define DPAUX_HYBRID_PADCTL_MODE_SHIFT			(0)
#define DPAUX_HYBRID_PADCTL_MODE_AUX			(0)
#define DPAUX_HYBRID_PADCTL_MODE_I2C			(1)
#define DPAUX_HYBRID_SPARE				(0x4d)
#define DPAUX_HYBRID_SPARE_PAD_PWR_POWERUP		(0)
#define DPAUX_HYBRID_SPARE_PAD_PWR_POWERDOWN		(1)


#endif
