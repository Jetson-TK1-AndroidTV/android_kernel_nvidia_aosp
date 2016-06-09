/*
 * arch/arm/mach-tegra/board-norrin-power.c
 *
 * Copyright (c) 2013-2014 NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <linux/pda_power.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/io.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/as3722-plat.h>
#include <linux/gpio.h>
#include <linux/regulator/userspace-consumer.h>
#include <linux/pid_thermal_gov.h>
#include <linux/tegra-fuse.h>

#include <asm/mach-types.h>

#include <mach/irqs.h>
#include <mach/edp.h>
#include <mach/gpio-tegra.h>

#include "cpu-tegra.h"
#include "pm.h"
#include "tegra-board-id.h"
#include "board.h"
#include "gpio-names.h"
#include "board-common.h"
#include "board-pmu-defines.h"
#include "board-ardbeg.h"
#include "tegra_cl_dvfs.h"
#include "devices.h"
#include "tegra11_soctherm.h"
#include "iomap.h"
#include "tegra3_tsensor.h"

#define PMC_CTRL		0x0
#define PMC_CTRL_INTR_LOW	(1 << 17)
#define AS3722_SUPPLY(_name)	"as3722_"#_name

static struct regulator_consumer_supply as3722_ldo0_supply[] = {
	REGULATOR_SUPPLY("avdd_pll_m", NULL),
	REGULATOR_SUPPLY("avdd_pll_ap_c2_c3", NULL),
	REGULATOR_SUPPLY("avdd_pll_cud2dpd", NULL),
	REGULATOR_SUPPLY("avdd_pll_c4", NULL),
	REGULATOR_SUPPLY("avdd_lvds0_io", NULL),
	REGULATOR_SUPPLY("vddio_ddr_hs", NULL),
	REGULATOR_SUPPLY("avdd_pll_erefe", NULL),
	REGULATOR_SUPPLY("avdd_pll_x", NULL),
	REGULATOR_SUPPLY("avdd_pll_cg", NULL),
};

static struct regulator_consumer_supply as3722_ldo1_supply[] = {
	REGULATOR_SUPPLY("vddio_cam", "vi"),
	REGULATOR_SUPPLY("pwrdet_cam", NULL),
	REGULATOR_SUPPLY("vdd_cam_1v8_cam", NULL),
	REGULATOR_SUPPLY("vif", "2-0010"),
	REGULATOR_SUPPLY("vdd_i2c", "2-000e"),
	REGULATOR_SUPPLY("vif", "2-0036"),
	REGULATOR_SUPPLY("vdd_i2c", "2-000c"),
	REGULATOR_SUPPLY("vi2c", "2-0030"),
};

static struct regulator_consumer_supply as3722_ldo2_supply[] = {
	REGULATOR_SUPPLY("avdd_dsi_csi", "tegradc.0"),
	REGULATOR_SUPPLY("avdd_dsi_csi", "tegradc.1"),
	REGULATOR_SUPPLY("avdd_dsi_csi", "vi.0"),
	REGULATOR_SUPPLY("avdd_dsi_csi", "vi.1"),
	REGULATOR_SUPPLY("pwrdet_mipi", NULL),
	REGULATOR_SUPPLY("avdd_hsic_com", NULL),
	REGULATOR_SUPPLY("avdd_hsic_mdm", NULL),
	REGULATOR_SUPPLY("vddio_hsic", "tegra-ehci.1"),
	REGULATOR_SUPPLY("vddio_hsic", "tegra-ehci.2"),
	REGULATOR_SUPPLY("vddio_hsic", "tegra-xhci"),
};

static struct regulator_consumer_supply as3722_ldo3_supply[] = {
	REGULATOR_SUPPLY("vdd_rtc", NULL),
};

static struct regulator_consumer_supply as3722_ldo4_supply[] = {
	REGULATOR_SUPPLY("vdd_2v7_hv", NULL),
	REGULATOR_SUPPLY("avdd_cam2_cam", NULL),
	REGULATOR_SUPPLY("vana", "2-0010"),
};

static struct regulator_consumer_supply as3722_ldo5_supply[] = {
	REGULATOR_SUPPLY("vdd_1v2_cam", NULL),
	REGULATOR_SUPPLY("vdig", "2-0010"),
	REGULATOR_SUPPLY("vdig", "2-0036"),
};

static struct regulator_consumer_supply as3722_ldo6_supply[] = {
	REGULATOR_SUPPLY("vddio_sdmmc", "sdhci-tegra.2"),
	REGULATOR_SUPPLY("pwrdet_sdmmc3", NULL),
};

static struct regulator_consumer_supply as3722_ldo7_supply[] = {
	REGULATOR_SUPPLY("vdd_cam_1v1_cam", NULL),
	REGULATOR_SUPPLY("imx135_reg2", NULL),
	REGULATOR_SUPPLY("vdig_lv", "2-0010"),
	REGULATOR_SUPPLY("dvdd", "2-0010"),
};

static struct regulator_consumer_supply as3722_ldo9_supply[] = {
	REGULATOR_SUPPLY("avdd", "spi0.0"),
	REGULATOR_SUPPLY("avdd", "spi2.1"),
};

static struct regulator_consumer_supply as3722_ldo10_supply[] = {
	REGULATOR_SUPPLY("avdd_af1_cam", NULL),
	REGULATOR_SUPPLY("avdd_cam1_cam", NULL),
	REGULATOR_SUPPLY("imx135_reg1", NULL),
	REGULATOR_SUPPLY("vdd", "2-000e"),
	REGULATOR_SUPPLY("vana", "2-0036"),
	REGULATOR_SUPPLY("vdd", "2-000c"),
};

static struct regulator_consumer_supply as3722_ldo11_supply[] = {
	REGULATOR_SUPPLY("vpp_fuse", NULL),
};

static struct regulator_consumer_supply as3722_sd0_supply[] = {
	REGULATOR_SUPPLY("vdd_cpu", NULL),
};

static struct regulator_consumer_supply as3722_sd1_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
};

static struct regulator_consumer_supply as3722_sd2_supply[] = {
	REGULATOR_SUPPLY("vddio_ddr", NULL),
	REGULATOR_SUPPLY("vddio_ddr_mclk", NULL),
	REGULATOR_SUPPLY("vddio_ddr3", NULL),
	REGULATOR_SUPPLY("vcore1_ddr3", NULL),
};

static struct regulator_consumer_supply as3722_sd4_supply[] = {
	REGULATOR_SUPPLY("avdd_pex_pll", NULL),
	REGULATOR_SUPPLY("avddio_pex_pll", NULL),
	REGULATOR_SUPPLY("dvddio_pex", NULL),
	REGULATOR_SUPPLY("pwrdet_pex_ctl", NULL),
	REGULATOR_SUPPLY("avdd_sata", NULL),
	REGULATOR_SUPPLY("vdd_sata", NULL),
	REGULATOR_SUPPLY("avdd_sata_pll", NULL),
	REGULATOR_SUPPLY("avddio_usb", "tegra-xhci"),
	REGULATOR_SUPPLY("avdd_hdmi", "tegradc.1"),
};

static struct regulator_consumer_supply as3722_sd5_supply[] = {
	REGULATOR_SUPPLY("vddio_sys", NULL),
	REGULATOR_SUPPLY("vddio_sys_2", NULL),
	REGULATOR_SUPPLY("vddio_audio", NULL),
	REGULATOR_SUPPLY("pwrdet_audio", NULL),
	REGULATOR_SUPPLY("vddio_sdmmc", "sdhci-tegra.0"),
	REGULATOR_SUPPLY("pwrdet_sdmmc1", NULL),
	REGULATOR_SUPPLY("vddio_sdmmc", "sdhci-tegra.3"),
	REGULATOR_SUPPLY("pwrdet_sdmmc4", NULL),
	REGULATOR_SUPPLY("vddio_uart", NULL),
	REGULATOR_SUPPLY("pwrdet_uart", NULL),
	REGULATOR_SUPPLY("vddio_bb", NULL),
	REGULATOR_SUPPLY("pwrdet_bb", NULL),
	REGULATOR_SUPPLY("vddio_gmi", NULL),
	REGULATOR_SUPPLY("pwrdet_nand", NULL),
	REGULATOR_SUPPLY("avdd_osc", NULL),
	/* emmc 1.8v misssing
	keyboard & touchpad 1.8v missing */
};

static struct regulator_consumer_supply as3722_sd6_supply[] = {
	REGULATOR_SUPPLY("vdd_gpu", NULL),
};

AMS_PDATA_INIT(sd0, NULL, 700000, 1400000, 1, 1, 1,
	       AS3722_EXT_CONTROL_ENABLE2);
AMS_PDATA_INIT(sd1, NULL, 700000, 1350000, 1, 1, 1,
	       AS3722_EXT_CONTROL_ENABLE1);
AMS_PDATA_INIT(sd2, NULL, 1350000, 1350000, 1, 1, 1, 0);
AMS_PDATA_INIT(sd4, NULL, 1050000, 1050000, 1, 1, 1,
	       AS3722_EXT_CONTROL_ENABLE1);
AMS_PDATA_INIT(sd5, NULL, 1800000, 1800000, 1, 1, 1, 0);
#ifdef CONFIG_ARCH_TEGRA_13x_SOC
AMS_PDATA_INIT(sd6, NULL, 650000, 1200000, 1, 1, 1, 0);
#else
AMS_PDATA_INIT(sd6, NULL, 650000, 1200000, 0, 1, 1, 0);
#endif
AMS_PDATA_INIT(ldo0, AS3722_SUPPLY(sd2), 1050000, 1250000, 1, 1, 1,
	       AS3722_EXT_CONTROL_ENABLE1);
AMS_PDATA_INIT(ldo1, NULL, 1800000, 1800000, 0, 1, 1, 0);
AMS_PDATA_INIT(ldo2, AS3722_SUPPLY(sd5), 1200000, 1200000, 0, 1, 1, 0);
AMS_PDATA_INIT(ldo3, NULL, 800000, 800000, 1, 1, 1, 0);
AMS_PDATA_INIT(ldo4, NULL, 2700000, 2700000, 0, 0, 1, 0);
AMS_PDATA_INIT(ldo5, AS3722_SUPPLY(sd5), 1200000, 1200000, 0, 0, 1, 0);
AMS_PDATA_INIT(ldo6, NULL, 1800000, 3300000, 0, 0, 1, 0);
AMS_PDATA_INIT(ldo7, AS3722_SUPPLY(sd5), 1275000, 1275000, 0, 0, 1, 0);
AMS_PDATA_INIT(ldo9, NULL, 3300000, 3300000, 0, 1, 1, 0);
AMS_PDATA_INIT(ldo10, NULL, 2700000, 2700000, 0, 0, 1, 0);
AMS_PDATA_INIT(ldo11, NULL, 1800000, 1800000, 0, 0, 1, 0);

static struct as3722_pinctrl_platform_data as3722_pctrl_pdata[] = {
	AS3722_PIN_CONTROL("gpio0", "gpio", NULL, NULL, NULL, "output-low"),
	AS3722_PIN_CONTROL("gpio1", "gpio", NULL, NULL, NULL, "output-high"),
	AS3722_PIN_CONTROL("gpio2", "gpio", NULL, NULL, NULL, "output-high"),
	AS3722_PIN_CONTROL("gpio3", "gpio", NULL, NULL, "enabled", NULL),
	AS3722_PIN_CONTROL("gpio4", "gpio", NULL, NULL, NULL, "output-high"),
	AS3722_PIN_CONTROL("gpio5", "clk32k-out", NULL, NULL, NULL, NULL),
	AS3722_PIN_CONTROL("gpio6", "gpio", NULL, NULL, "enabled", NULL),
	AS3722_PIN_CONTROL("gpio7", "gpio", NULL, NULL, NULL, "output-high"),
};

static struct as3722_adc_extcon_platform_data as3722_adc_extcon_pdata = {
	.connection_name = "as3722-extcon",
	.enable_adc1_continuous_mode = true,
	.enable_low_voltage_range = true,
	.adc_channel = 12,
	.hi_threshold =  0x100,
	.low_threshold = 0x80,
};

static struct as3722_platform_data as3722_pdata = {
	.reg_pdata[AS3722_LDO0] = &as3722_ldo0_reg_pdata,
	.reg_pdata[AS3722_LDO1] = &as3722_ldo1_reg_pdata,
	.reg_pdata[AS3722_LDO2] = &as3722_ldo2_reg_pdata,
	.reg_pdata[AS3722_LDO3] = &as3722_ldo3_reg_pdata,
	.reg_pdata[AS3722_LDO4] = &as3722_ldo4_reg_pdata,
	.reg_pdata[AS3722_LDO5] = &as3722_ldo5_reg_pdata,
	.reg_pdata[AS3722_LDO6] = &as3722_ldo6_reg_pdata,
	.reg_pdata[AS3722_LDO7] = &as3722_ldo7_reg_pdata,
	.reg_pdata[AS3722_LDO9] = &as3722_ldo9_reg_pdata,
	.reg_pdata[AS3722_LDO10] = &as3722_ldo10_reg_pdata,
	.reg_pdata[AS3722_LDO11] = &as3722_ldo11_reg_pdata,

	.reg_pdata[AS3722_SD0] = &as3722_sd0_reg_pdata,
	.reg_pdata[AS3722_SD1] = &as3722_sd1_reg_pdata,
	.reg_pdata[AS3722_SD2] = &as3722_sd2_reg_pdata,
	.reg_pdata[AS3722_SD4] = &as3722_sd4_reg_pdata,
	.reg_pdata[AS3722_SD5] = &as3722_sd5_reg_pdata,
	.reg_pdata[AS3722_SD6] = &as3722_sd6_reg_pdata,

	.gpio_base = AS3722_GPIO_BASE,
	.irq_base = AS3722_IRQ_BASE,
	.use_internal_int_pullup = 0,
	.use_internal_i2c_pullup = 0,
	.pinctrl_pdata = as3722_pctrl_pdata,
	.num_pinctrl = ARRAY_SIZE(as3722_pctrl_pdata),
	.enable_clk32k_out = true,
	.use_power_off = true,
	.extcon_pdata = &as3722_adc_extcon_pdata,
	.major_rev = 1,
	.minor_rev = 1,
};

static struct pca953x_platform_data tca6416_pdata = {
	.gpio_base = PMU_TCA6416_GPIO_BASE,
	.irq_base = PMU_TCA6416_IRQ_BASE,
};

static struct i2c_board_info tca6416_expander[] = {
	{
		I2C_BOARD_INFO("tca6416", 0x20),
		.platform_data = &tca6416_pdata,
	},
};

static struct i2c_board_info __initdata as3722_regulators[] = {
	{
		I2C_BOARD_INFO("as3722", 0x40),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_EXTERNAL_PMU,
		.platform_data = &as3722_pdata,
	},
};

int __init norrin_as3722_regulator_init(void)
{
	void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
	u32 pmc_ctrl;
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	/* AS3722: Normal state of INT request line is LOW.
	 * configure the power management controller to trigger PMU
	 * interrupts when HIGH.
	 */
	pmc_ctrl = readl(pmc + PMC_CTRL);
	writel(pmc_ctrl | PMC_CTRL_INTR_LOW, pmc + PMC_CTRL);
	regulator_has_full_constraints();
	/* Set vdd_gpu init uV to 1V */
	as3722_sd6_reg_idata.constraints.init_uV = 1000000;

	/* Set overcurrent of rails. */
	as3722_sd6_reg_idata.constraints.min_uA = 3500000;
	as3722_sd6_reg_idata.constraints.max_uA = 3500000;

	as3722_sd0_reg_idata.constraints.min_uA = 3500000;
	as3722_sd0_reg_idata.constraints.max_uA = 3500000;

	as3722_sd1_reg_idata.constraints.min_uA = 2500000;
	as3722_sd1_reg_idata.constraints.max_uA = 2500000;

	as3722_ldo3_reg_pdata.enable_tracking = true;
	as3722_ldo3_reg_pdata.disable_tracking_suspend = true;

	if ((board_info.board_id == BOARD_PM374) &&
				(board_info.fab == BOARD_FAB_B))
		as3722_pdata.minor_rev = 2;
	pr_info("%s: i2c_register_board_info\n", __func__);
	i2c_register_board_info(4, as3722_regulators,
			ARRAY_SIZE(as3722_regulators));
	tca6416_expander[0].irq = gpio_to_irq(TEGRA_GPIO_PQ5);
	i2c_register_board_info(0, tca6416_expander,
			ARRAY_SIZE(tca6416_expander));
	return 0;
}

static struct tegra_suspend_platform_data norrin_suspend_data = {
	.cpu_timer	= 2000,
	.cpu_off_timer	= 2000,
	.suspend_mode	= TEGRA_SUSPEND_LP0,
	.core_timer	= 0x7e7e,
	.core_off_timer = 2000,
	.corereq_high	= true,
	.sysclkreq_high	= true,
	.cpu_lp2_min_residency = 1000,
};

#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
/* board parameters for cpu dfll */
static struct tegra_cl_dvfs_cfg_param norrin_cl_dvfs_param = {
	.sample_rate = 12500,

	.force_mode = TEGRA_CL_DVFS_FORCE_FIXED,
	.cf = 10,
	.ci = 0,
	.cg = 2,

	.droop_cut_value = 0xF,
	.droop_restore_ramp = 0x0,
	.scale_out_ramp = 0x0,
};

/* Norrin: fixed 10mV steps from 700mV to 1400mV */
#define PMU_CPU_VDD_MAP_SIZE ((1400000 - 700000) / 10000 + 1)
static struct voltage_reg_map pmu_cpu_vdd_map[PMU_CPU_VDD_MAP_SIZE];
static inline void fill_reg_map(struct board_info *board_info)
{
	int i;
	u32 reg_init_value = 0x0a;

	if ((board_info->board_id == BOARD_PM374) &&
			(board_info->fab == 0x01))
		reg_init_value = 0x1e;

	for (i = 0; i < PMU_CPU_VDD_MAP_SIZE; i++) {
		pmu_cpu_vdd_map[i].reg_value = i + reg_init_value;
		pmu_cpu_vdd_map[i].reg_uV = 700000 + 10000 * i;
	}
}

static struct tegra_cl_dvfs_platform_data norrin_cl_dvfs_data = {
	.dfll_clk_name = "dfll_cpu",
	.pmu_if = TEGRA_CL_DVFS_PMU_I2C,
	.u.pmu_i2c = {
		.fs_rate = 400000,
		.slave_addr = 0x80,
		.reg = 0x00,
	},
	.vdd_map = pmu_cpu_vdd_map,
	.vdd_map_size = PMU_CPU_VDD_MAP_SIZE,

	.cfg_param = &norrin_cl_dvfs_param,
};


static const struct of_device_id dfll_of_match[] = {
	{ .compatible	= "nvidia,tegra124-dfll", },
	{ .compatible	= "nvidia,tegra132-dfll", },
	{ },
};

static int __init norrin_cl_dvfs_init(void)
{
	struct board_info board_info;
	struct device_node *dn = of_find_matching_node(NULL, dfll_of_match);

	/*
	 * Norrin platforms maybe used with different DT variants. Some of them
	 * include DFLL data in DT, some - not. Check DT here, and continue with
	 * platform device registration only if DT DFLL node is not present.
	 */
	if (dn) {
		bool available = of_device_is_available(dn);
		of_node_put(dn);
		if (available)
			return 0;
	}

	tegra_get_board_info(&board_info);

	fill_reg_map(&board_info);
	norrin_cl_dvfs_data.flags = TEGRA_CL_DVFS_DYN_OUTPUT_CFG;
	if (board_info.board_id == BOARD_PM374)
		norrin_cl_dvfs_data.flags |= TEGRA_CL_DVFS_DATA_NEW_NO_USE;
	tegra_cl_dvfs_device.dev.platform_data = &norrin_cl_dvfs_data;
	platform_device_register(&tegra_cl_dvfs_device);

	return 0;
}
#endif

/* Always ON /Battery regulator */
static struct regulator_consumer_supply fixed_reg_battery_supply[] = {
	REGULATOR_SUPPLY("vdd_sys_bl", NULL),
	REGULATOR_SUPPLY("vddio_pex_sata", "tegra-sata.0"),
};

/* Always ON 1.8v */
static struct regulator_consumer_supply fixed_reg_aon_1v8_supply[] = {
	REGULATOR_SUPPLY("vdd_1v8_emmc", NULL),
	REGULATOR_SUPPLY("vdd_1v8b_com_f", NULL),
	REGULATOR_SUPPLY("vdd_1v8b_gps_f", NULL),
	REGULATOR_SUPPLY("vdd", "0-004c"),
};

/* Always ON 3.3v */
static struct regulator_consumer_supply fixed_reg_aon_3v3_supply[] = {
	REGULATOR_SUPPLY("vdd_3v3_emmc", NULL),
	REGULATOR_SUPPLY("vdd_com_3v3", NULL),
};

/* Always ON 1v2 */
static struct regulator_consumer_supply fixed_reg_aon_1v2_supply[] = {
	REGULATOR_SUPPLY("vdd_1v2_bb_hsic", NULL),
};

/* EN_USB0_VBUS From TEGRA GPIO PN4 */
static struct regulator_consumer_supply fixed_reg_usb0_vbus_supply[] = {
	REGULATOR_SUPPLY("usb_vbus", "tegra-ehci.0"),
	REGULATOR_SUPPLY("usb_vbus", "tegra-otg"),
	REGULATOR_SUPPLY("usb_vbus0", "tegra-xhci"),
};

/* EN_USB1_USB2_VBUS From TEGRA GPIO PN5 */
static struct regulator_consumer_supply fixed_reg_usb1_usb2_vbus_supply[] = {
	REGULATOR_SUPPLY("usb_vbus", "tegra-ehci.1"),
	REGULATOR_SUPPLY("usb_vbus1", "tegra-xhci"),
	REGULATOR_SUPPLY("usb_vbus", "tegra-ehci.2"),
	REGULATOR_SUPPLY("usb_vbus2", "tegra-xhci"),
};

/* Gated by GPIO_PK6 */
static struct regulator_consumer_supply fixed_reg_vdd_hdmi_5v0_supply[] = {
	REGULATOR_SUPPLY("vdd_hdmi_5v0", "tegradc.1"),
};

/* Gated by GPIO_PH7 */
static struct regulator_consumer_supply fixed_reg_vdd_hdmi_supply[] = {
	REGULATOR_SUPPLY("avdd_hdmi_pll", "tegradc.1"),
};

/* VDD_LCD_BL DAP3_DOUT */
static struct regulator_consumer_supply fixed_reg_vdd_lcd_bl_supply[] = {
	REGULATOR_SUPPLY("vdd_lcd_bl", NULL),
};

/* LCD_BL_EN GMI_AD10 */
static struct regulator_consumer_supply fixed_reg_lcd_bl_en_supply[] = {
	REGULATOR_SUPPLY("vdd_lcd_bl_en", NULL),
};

/* AS3722 GPIO1*/
static struct regulator_consumer_supply fixed_reg_3v3_supply[] = {
	REGULATOR_SUPPLY("hvdd_pex", NULL),
	REGULATOR_SUPPLY("hvdd_pex_pll", NULL),
	REGULATOR_SUPPLY("vdd_sys_cam_3v3", NULL),
	REGULATOR_SUPPLY("micvdd", "tegra-snd-rt5645.0"),
	REGULATOR_SUPPLY("micvdd", "tegra-snd-rt5639.0"),
	REGULATOR_SUPPLY("vdd_gps_3v3", NULL),
	REGULATOR_SUPPLY("vdd_nfc_3v3", NULL),
	REGULATOR_SUPPLY("vdd_3v3_sensor", NULL),
	REGULATOR_SUPPLY("vdd_kp_3v3", NULL),
	REGULATOR_SUPPLY("vdd_tp_3v3", NULL),
	REGULATOR_SUPPLY("vdd_dtv_3v3", NULL),
	REGULATOR_SUPPLY("vdd_modem_3v3", NULL),
	REGULATOR_SUPPLY("vdd", "0-0048"),
	REGULATOR_SUPPLY("vdd", "0-0069"),
	REGULATOR_SUPPLY("vdd", "0-000c"),
	REGULATOR_SUPPLY("vdd", "0-0077"),
	REGULATOR_SUPPLY("vin", "2-0030"),
};

/* AS3722 GPIO1*/
static struct regulator_consumer_supply fixed_reg_5v0_supply[] = {
	REGULATOR_SUPPLY("spkvdd", "tegra-snd-rt5645.0"),
	REGULATOR_SUPPLY("spkvdd", "tegra-snd-rt5639.0"),
	REGULATOR_SUPPLY("vdd_5v0_sensor", NULL),
};

static struct regulator_consumer_supply fixed_reg_dcdc_1v8_supply[] = {
	REGULATOR_SUPPLY("avdd_lvds0_pll", NULL),
	REGULATOR_SUPPLY("dvdd_lcd", NULL),
	REGULATOR_SUPPLY("vdd_ds_1v8", NULL),
	REGULATOR_SUPPLY("avdd", "tegra-snd-rt5645.0"),
	REGULATOR_SUPPLY("dbvdd", "tegra-snd-rt5645.0"),
	REGULATOR_SUPPLY("avdd", "tegra-snd-rt5639.0"),
	REGULATOR_SUPPLY("dbvdd", "tegra-snd-rt5639.0"),
	REGULATOR_SUPPLY("dmicvdd", "tegra-snd-rt5639.0"),
	REGULATOR_SUPPLY("dmicvdd", "tegra-snd-rt5645.0"),
	REGULATOR_SUPPLY("vdd_1v8b_nfc", NULL),
	REGULATOR_SUPPLY("vdd_1v8_sensor", NULL),
	REGULATOR_SUPPLY("vdd_1v8_sdmmc", NULL),
	REGULATOR_SUPPLY("vdd_kp_1v8", NULL),
	REGULATOR_SUPPLY("vdd_tp_1v8", NULL),
	REGULATOR_SUPPLY("vdd_modem_1v8", NULL),
	REGULATOR_SUPPLY("vdd_1v8b", "0-0048"),
	REGULATOR_SUPPLY("dvdd", "spi0.0"),
	REGULATOR_SUPPLY("dvdd", "spi2.1"),
	REGULATOR_SUPPLY("vlogic", "0-0069"),
	REGULATOR_SUPPLY("vid", "0-000c"),
	REGULATOR_SUPPLY("vddio", "0-0077"),
	REGULATOR_SUPPLY("avdd_pll_utmip", "tegra-udc.0"),
	REGULATOR_SUPPLY("avdd_pll_utmip", "tegra-ehci.0"),
	REGULATOR_SUPPLY("avdd_pll_utmip", "tegra-ehci.1"),
	REGULATOR_SUPPLY("avdd_pll_utmip", "tegra-ehci.2"),
	REGULATOR_SUPPLY("avdd_pll_utmip", "tegra-xhci"),
};

/* gated by TCA6416 GPIO EXP GPIO0 */
static struct regulator_consumer_supply fixed_reg_dcdc_1v2_supply[] = {
	REGULATOR_SUPPLY("vdd_1v2_en", NULL),
};

/* AMS GPIO2 */
static struct regulator_consumer_supply fixed_reg_as3722_gpio2_supply[] = {
	REGULATOR_SUPPLY("avdd_usb", "tegra-udc.0"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.0"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.1"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.2"),
	REGULATOR_SUPPLY("hvdd_usb", "tegra-xhci"),
	REGULATOR_SUPPLY("vddio_hv", "tegradc.1"),
	REGULATOR_SUPPLY("pwrdet_hv", NULL),
	REGULATOR_SUPPLY("hvdd_sata", NULL),
};

/* gated by AS3722 GPIO4 */
static struct regulator_consumer_supply fixed_reg_lcd_supply[] = {
	REGULATOR_SUPPLY("avdd_lcd", NULL),
};

/* gated by GPIO_PR0 */
static struct regulator_consumer_supply fixed_reg_sdmmc_en_supply[] = {
	REGULATOR_SUPPLY("vddio_sd_slot", "sdhci-tegra.1"),
	REGULATOR_SUPPLY("vddio_sd_slot", "sdhci-tegra.2"),
};

static struct regulator_consumer_supply fixed_reg_vdd_cdc_1v2_aud_supply[] = {
	REGULATOR_SUPPLY("ldoen", "tegra-snd-rt5639.0"),
};

static struct regulator_consumer_supply fixed_reg_vdd_amp_shut_aud_supply[] = {
	REGULATOR_SUPPLY("epamp", "tegra-snd-rt5645.0"),
};

static struct regulator_consumer_supply fixed_reg_vdd_dsi_mux_supply[] = {
	REGULATOR_SUPPLY("vdd_3v3_dsi", "NULL"),
};

/* Macro for defining fixed regulator sub device data */
#define FIXED_SUPPLY(_name) "fixed_reg_"#_name
#define FIXED_REG(_id, _var, _name, _in_supply,			\
	_always_on, _boot_on, _gpio_nr, _open_drain,		\
	_active_high, _boot_state, _millivolts, _sdelay)	\
static struct regulator_init_data ri_data_##_var =		\
{								\
	.supply_regulator = _in_supply,				\
	.num_consumer_supplies =				\
	ARRAY_SIZE(fixed_reg_##_name##_supply),			\
	.consumer_supplies = fixed_reg_##_name##_supply,	\
	.constraints = {					\
		.valid_modes_mask = (REGULATOR_MODE_NORMAL |	\
				REGULATOR_MODE_STANDBY),	\
		.valid_ops_mask = (REGULATOR_CHANGE_MODE |	\
				REGULATOR_CHANGE_STATUS |	\
				REGULATOR_CHANGE_VOLTAGE),	\
		.always_on = _always_on,			\
		.boot_on = _boot_on,				\
	},							\
};								\
static struct fixed_voltage_config fixed_reg_##_var##_pdata =	\
{								\
	.supply_name = FIXED_SUPPLY(_name),			\
	.microvolts = _millivolts * 1000,			\
	.gpio = _gpio_nr,					\
	.gpio_is_open_drain = _open_drain,			\
	.enable_high = _active_high,				\
	.enabled_at_boot = _boot_state,				\
	.init_data = &ri_data_##_var,				\
	.startup_delay = _sdelay,				\
};								\
static struct platform_device fixed_reg_##_var##_dev = {	\
	.name = "reg-fixed-voltage",				\
	.id = _id,						\
	.dev = {						\
		.platform_data = &fixed_reg_##_var##_pdata,	\
	},							\
}

FIXED_REG(0,	battery,	battery,	NULL,	0,	0,
		-1,	false, true,	0,	8400,	0);

FIXED_REG(1,	aon_1v8,	aon_1v8,	NULL,	0,	0,
		-1,	false, true,	0,	1800,	0);

FIXED_REG(2,	aon_3v3,	aon_3v3,	NULL,	0,	0,
		-1,	false, true,	0,	3300,	0);

FIXED_REG(3,	aon_1v2,	aon_1v2,	NULL,	0,	0,
		-1,	false, true,	0,	1200,	0);

FIXED_REG(4,	vdd_hdmi_5v0,	vdd_hdmi_5v0,	NULL,	0,	0,
		TEGRA_GPIO_PK6,	false,	true,	0,	5000,	5000);

FIXED_REG(5,	vdd_hdmi,	vdd_hdmi,	AS3722_SUPPLY(sd4),
		0,	0,
		TEGRA_GPIO_PH7,	false,	false,	0,	3300,	0);

FIXED_REG(6,	usb0_vbus,	usb0_vbus,	NULL,	0,	0,
		TEGRA_GPIO_PN4,	true,	true,	0,	5000,	0);

FIXED_REG(7,	usb1_usb2_vbus,	usb1_usb2_vbus,	NULL,	0,	0,
		TEGRA_GPIO_PN5,	true,	true,	0,	5000, 0);

FIXED_REG(8,	vdd_lcd_bl,	vdd_lcd_bl,	NULL,	0,	0,
		TEGRA_GPIO_PP2,	false,	true,	0,	3300, 0);

FIXED_REG(9,	lcd_bl_en,	lcd_bl_en,	NULL,	0,	0,
		TEGRA_GPIO_PH2,	false,	true,	0,	5000,	0);

FIXED_REG(10,	3v3,		3v3,		NULL,	0,	0,
		-1,	false,	true,	0,	3300,	0);

FIXED_REG(11,	5v0,		5v0,		NULL,	0,	0,
		-1,	false,	true,	0,	5000,	0);

FIXED_REG(12,	dcdc_1v8,	dcdc_1v8,	NULL,	0,	0,
		-1,	false,	true,	0,	1800,	0);

FIXED_REG(13,   dcdc_1v2,	dcdc_1v2,	NULL,	0,	0,
		PMU_TCA6416_GPIO_BASE,	false,	true,	0,	1200,
		0);

FIXED_REG(14,	as3722_gpio2,	as3722_gpio2,		NULL,	0,	true,
		AS3722_GPIO_BASE + AS3722_GPIO2,	false,	true,	true,
		3300,	0);

FIXED_REG(15,	lcd,		lcd,		NULL,	0,	0,
		AS3722_GPIO_BASE + AS3722_GPIO4,	false,	true,	0,
		3300,	0);

FIXED_REG(16,	sdmmc_en,	sdmmc_en,	NULL,	0,	0,
		TEGRA_GPIO_PR0,	false,	true,	0,	3300,	0);

FIXED_REG(17,	vdd_cdc_1v2_aud,	vdd_cdc_1v2_aud,	NULL,	0,
		0,	PMU_TCA6416_GPIO(2),	false,	true,	0,
		1200,	250000);

FIXED_REG(18,	vdd_amp_shut_aud,	vdd_amp_shut_aud,	NULL,	0,
		0,	PMU_TCA6416_GPIO(3),	false,	true,	0,
		1200,	0);

FIXED_REG(19,	vdd_dsi_mux,		vdd_dsi_mux,	NULL,	0,	0,
		PMU_TCA6416_GPIO(13),	false,	true,	0,	3300,	0);

/*
 * Creating the fixed regulator device tables
 */

#define ADD_FIXED_REG(_name)    (&fixed_reg_##_name##_dev)

#define NORRIN_COMMON_FIXED_REG			\
	ADD_FIXED_REG(battery),			\
	ADD_FIXED_REG(aon_1v8),			\
	ADD_FIXED_REG(aon_3v3),			\
	ADD_FIXED_REG(aon_1v2),			\
	ADD_FIXED_REG(vdd_hdmi_5v0),		\
	ADD_FIXED_REG(vdd_hdmi),		\
	ADD_FIXED_REG(vdd_lcd_bl),		\
	ADD_FIXED_REG(lcd_bl_en),		\
	ADD_FIXED_REG(3v3),			\
	ADD_FIXED_REG(5v0),			\
	ADD_FIXED_REG(dcdc_1v8),		\
	ADD_FIXED_REG(as3722_gpio2),		\
	ADD_FIXED_REG(lcd),			\
	ADD_FIXED_REG(sdmmc_en),		\
	ADD_FIXED_REG(usb0_vbus),		\
	ADD_FIXED_REG(usb1_usb2_vbus),		\
	ADD_FIXED_REG(dcdc_1v2),		\
	ADD_FIXED_REG(vdd_cdc_1v2_aud),		\
	ADD_FIXED_REG(vdd_amp_shut_aud),	\
	ADD_FIXED_REG(vdd_dsi_mux)

/* Gpio switch regulator platform data for Norrin ERS*/
static struct platform_device *fixed_reg_devs[] = {
	NORRIN_COMMON_FIXED_REG,
};

static int __init norrin_fixed_regulator_init(void)
{
	if (!of_machine_is_compatible("nvidia,norrin"))
		return 0;

	return platform_add_devices(fixed_reg_devs,
			ARRAY_SIZE(fixed_reg_devs));

	return 0;
}

subsys_initcall_sync(norrin_fixed_regulator_init);

int __init norrin_regulator_init(void)
{

#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
	norrin_cl_dvfs_init();
#endif
	norrin_as3722_regulator_init();

	return 0;
}

int __init norrin_suspend_init(void)
{
	tegra_init_suspend(&norrin_suspend_data);
	return 0;
}

int __init norrin_edp_init(void)
{
	unsigned int regulator_mA;

	regulator_mA = get_maximum_cpu_current_supported();
	if (!regulator_mA)
		regulator_mA = 15000;

	pr_info("%s: CPU regulator %d mA\n", __func__, regulator_mA);

	tegra_init_cpu_edp_limits(regulator_mA);

	/* gpu maximum current */
	regulator_mA = 8000;
	pr_info("%s: GPU regulator %d mA\n", __func__, regulator_mA);

	tegra_init_gpu_edp_limits(regulator_mA);
	return 0;
}

static struct pid_thermal_gov_params soctherm_pid_params = {
	.max_err_temp = 9000,
	.max_err_gain = 1000,

	.gain_p = 1000,
	.gain_d = 0,

	.up_compensation = 20,
	.down_compensation = 20,
};

static struct thermal_zone_params soctherm_tzp = {
	.governor_name = "pid_thermal_gov",
	.governor_params = &soctherm_pid_params,
};

static struct tegra_tsensor_pmu_data tpdata_as3722 = {
	.reset_tegra = 1,
	.pmu_16bit_ops = 0,
	.controller_type = 0,
	.pmu_i2c_addr = 0x40,
	.i2c_controller_id = 4,
	.poweroff_reg_addr = 0x36,
	.poweroff_reg_data = 0x2,
};

static struct soctherm_platform_data norrin_soctherm_data = {
	.therm = {
		[THERM_CPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 6000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 99000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "cpu-balanced",
					.trip_temp = 90000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_GPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 6000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 99000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "gpu-balanced",
					.trip_temp = 90000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_MEM] = {
			.zone_enable = true,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 101000, /* = GPU shut */
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &soctherm_tzp,
		},
		[THERM_PLL] = {
			.zone_enable = true,
			.tzp = &soctherm_tzp,
		},
	},
	.throttle = {
		[THROTTLE_HEAVY] = {
			.priority = 100,
			.devs = {
				[THROTTLE_DEV_CPU] = {
					.enable = true,
					.depth = 80,
					.throttling_depth = "heavy_throttling",
				},
				[THROTTLE_DEV_GPU] = {
					.enable = true,
					.throttling_depth = "heavy_throttling",
				},
			},
		},
	},
};

int __init norrin_soctherm_init(void)
{
	s32 base_cp, shft_cp;
	u32 base_ft, shft_ft;
	struct board_info pmu_board_info;
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	/* do this only for supported CP,FT fuses */
	if ((tegra_fuse_calib_base_get_cp(&base_cp, &shft_cp) >= 0) &&
	    (tegra_fuse_calib_base_get_ft(&base_ft, &shft_ft) >= 0)) {
		tegra_platform_edp_init(
			norrin_soctherm_data.therm[THERM_CPU].trips,
			&norrin_soctherm_data.therm[THERM_CPU].num_trips,
			7000); /* edp temperature margin */
		tegra_platform_gpu_edp_init(
			norrin_soctherm_data.therm[THERM_GPU].trips,
			&norrin_soctherm_data.therm[THERM_GPU].num_trips,
			7000);
		tegra_add_cpu_vmax_trips(
			norrin_soctherm_data.therm[THERM_CPU].trips,
			&norrin_soctherm_data.therm[THERM_CPU].num_trips);
		tegra_add_tgpu_trips(
			norrin_soctherm_data.therm[THERM_GPU].trips,
			&norrin_soctherm_data.therm[THERM_GPU].num_trips);
		tegra_add_core_vmax_trips(
			norrin_soctherm_data.therm[THERM_PLL].trips,
			&norrin_soctherm_data.therm[THERM_PLL].num_trips);
	}

	if (board_info.board_id == BOARD_PM374 ||
		board_info.board_id == BOARD_PM375 ||
		board_info.board_id == BOARD_E1971 ||
		board_info.board_id == BOARD_E1991) {
		tegra_add_cpu_vmin_trips(
			norrin_soctherm_data.therm[THERM_CPU].trips,
			&norrin_soctherm_data.therm[THERM_CPU].num_trips);
		tegra_add_gpu_vmin_trips(
			norrin_soctherm_data.therm[THERM_GPU].trips,
			&norrin_soctherm_data.therm[THERM_GPU].num_trips);
		tegra_add_core_vmin_trips(
			norrin_soctherm_data.therm[THERM_PLL].trips,
			&norrin_soctherm_data.therm[THERM_PLL].num_trips);
	}

	if (board_info.board_id == BOARD_PM375)
		tegra_add_cpu_clk_switch_trips(
			norrin_soctherm_data.therm[THERM_CPU].trips,
			&norrin_soctherm_data.therm[THERM_CPU].num_trips);
	tegra_get_pmu_board_info(&pmu_board_info);

	if ((pmu_board_info.board_id == BOARD_PM374) ||
		(pmu_board_info.board_id == BOARD_PM375))
		norrin_soctherm_data.tshut_pmu_trip_data = &tpdata_as3722;
	else
		pr_warn("soctherm THERMTRIP not supported on PMU (BOARD_P%d)\n",
			pmu_board_info.board_id);

	return tegra11_soctherm_init(&norrin_soctherm_data);
}
