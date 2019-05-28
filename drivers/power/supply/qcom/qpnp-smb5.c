/* Copyright (c) 2018 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "SMB2: %s: " fmt, __func__

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/log2.h>
#include <linux/qpnp/qpnp-revid.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/iio/consumer.h>
#include <linux/pmic-voter.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/msm-bus.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/oneplus/boot_mode.h>
#include "smb5-reg.h"
#include "smb5-lib.h"
#include "schgm-flash.h"

static struct smb_params smb5_pmi632_params = {
	.fcc			= {
		.name   = "fast charge current",
		.reg    = CHGR_FAST_CHARGE_CURRENT_CFG_REG,
		.min_u  = 0,
		.max_u  = 3000000,
		.step_u = 50000,
	},
	.fv			= {
		.name   = "float voltage",
		.reg    = CHGR_FLOAT_VOLTAGE_CFG_REG,
		.min_u  = 3600000,
		.max_u  = 4800000,
		.step_u = 10000,
	},
	.usb_icl		= {
		.name   = "usb input current limit",
		.reg    = USBIN_CURRENT_LIMIT_CFG_REG,
		.min_u  = 0,
		.max_u  = 3000000,
		.step_u = 50000,
	},
	.icl_max_stat		= {
		.name   = "dcdc icl max status",
		.reg    = ICL_MAX_STATUS_REG,
		.min_u  = 0,
		.max_u  = 3000000,
		.step_u = 50000,
	},
	.icl_stat		= {
		.name   = "input current limit status",
		.reg    = AICL_ICL_STATUS_REG,
		.min_u  = 0,
		.max_u  = 3000000,
		.step_u = 50000,
	},
	.otg_cl			= {
		.name	= "usb otg current limit",
		.reg	= DCDC_OTG_CURRENT_LIMIT_CFG_REG,
		.min_u	= 500000,
		.max_u	= 1000000,
		.step_u	= 250000,
	},
	.dc_icl		= {
		.name   = "DC input current limit",
		.reg    = DCDC_CFG_REF_MAX_PSNS_REG,
		.min_u  = 0,
		.max_u  = 1500000,
		.step_u = 50000,
	},
	.jeita_cc_comp_hot	= {
		.name	= "jeita fcc reduction",
		.reg	= JEITA_CCCOMP_CFG_HOT_REG,
		.min_u	= 0,
		.max_u	= 1575000,
		.step_u	= 25000,
	},
	.jeita_cc_comp_cold	= {
		.name	= "jeita fcc reduction",
		.reg	= JEITA_CCCOMP_CFG_COLD_REG,
		.min_u	= 0,
		.max_u	= 1575000,
		.step_u	= 25000,
	},
	.freq_switcher		= {
		.name	= "switching frequency",
		.reg	= DCDC_FSW_SEL_REG,
		.min_u	= 600,
		.max_u	= 1200,
		.step_u	= 400,
		.set_proc = smblib_set_chg_freq,
	},
};

static struct smb_params smb5_pm8150b_params = {
	.fcc			= {
		.name   = "fast charge current",
		.reg    = CHGR_FAST_CHARGE_CURRENT_CFG_REG,
		.min_u  = 0,
		.max_u  = 8000000,
		.step_u = 50000,
	},
	.fv			= {
		.name   = "float voltage",
		.reg    = CHGR_FLOAT_VOLTAGE_CFG_REG,
		.min_u  = 3600000,
		.max_u  = 4790000,
		.step_u = 10000,
	},
	.usb_icl		= {
		.name   = "usb input current limit",
		.reg    = USBIN_CURRENT_LIMIT_CFG_REG,
		.min_u  = 0,
		.max_u  = 5000000,
		.step_u = 50000,
	},
	.icl_max_stat		= {
		.name   = "dcdc icl max status",
		.reg    = ICL_MAX_STATUS_REG,
		.min_u  = 0,
		.max_u  = 5000000,
		.step_u = 50000,
	},
	.icl_stat		= {
		.name   = "aicl icl status",
		.reg    = AICL_ICL_STATUS_REG,
		.min_u  = 0,
		.max_u  = 5000000,
		.step_u = 50000,
	},
	.otg_cl			= {
		.name	= "usb otg current limit",
		.reg	= DCDC_OTG_CURRENT_LIMIT_CFG_REG,
		.min_u	= 500000,
		.max_u	= 3000000,
		.step_u	= 500000,
	},
	.dc_icl		= {
		.name   = "DC input current limit",
		.reg    = DCDC_CFG_REF_MAX_PSNS_REG,
		.min_u  = 0,
		.max_u  = 1500000,
		.step_u = 50000,
	},
	.jeita_cc_comp_hot	= {
		.name	= "jeita fcc reduction",
		.reg	= JEITA_CCCOMP_CFG_HOT_REG,
		.min_u	= 0,
		.max_u	= 8000000,
		.step_u	= 25000,
		.set_proc = NULL,
	},
	.jeita_cc_comp_cold	= {
		.name	= "jeita fcc reduction",
		.reg	= JEITA_CCCOMP_CFG_COLD_REG,
		.min_u	= 0,
		.max_u	= 8000000,
		.step_u	= 25000,
		.set_proc = NULL,
	},
	.freq_switcher		= {
		.name	= "switching frequency",
		.reg	= DCDC_FSW_SEL_REG,
		.min_u	= 600,
		.max_u	= 1200,
		.step_u	= 400,
		.set_proc = smblib_set_chg_freq,
	},
};

struct smb_dt_props {
	int			usb_icl_ua;
	struct device_node	*revid_dev_node;
	enum float_options	float_option;
	int			chg_inhibit_thr_mv;
	bool			no_battery;
	bool			hvdcp_disable;
	int			sec_charger_config;
	int			auto_recharge_soc;
	int			auto_recharge_vbat_mv;
	int			wd_bark_time;
	int			batt_profile_fcc_ua;
	int			batt_profile_fv_uv;
	int			term_current_src;
	int			term_current_thresh_hi_ma;
	int			term_current_thresh_lo_ma;
	int			disable_suspend_on_collapse;
};

struct smb5 {
	struct smb_charger	chg;
	struct dentry		*dfs_root;
	struct smb_dt_props	dt;
};

static int smbchg_cutoff_volt_with_charger = 3240;
struct smb_charger *g_chip;
static const struct file_operations proc_ship_mode_operations;
module_param_named(
	cutoff_volt_with_charger,
	smbchg_cutoff_volt_with_charger,
	int, 0600);

#define OF_PROP_READ(node, dt_property, prop, retval, optional)		\
do {									\
	if (retval)							\
		break;							\
	if (optional)							\
		prop = -EINVAL;						\
									\
	retval = of_property_read_u32(node,		\
					dt_property,	\
					&prop);				\
									\
	if ((retval == -EINVAL) && optional)				\
		retval = 0;						\
	else if (retval)						\
		pr_err("Error reading " #dt_property	\
				" property rc = %d\n", rc);		\
} while (0)

#ifdef	CONFIG_OP_DEBUG_CHG
	static int __debug_mask = PR_OP_DEBUG;
#else
static int __debug_mask;
#endif
module_param_named(
	debug_mask, __debug_mask, int, 0600
);

static int __pd_disabled;
module_param_named(
	pd_disabled, __pd_disabled, int, 0600
);

static int __weak_chg_icl_ua = 500000;
module_param_named(
	weak_chg_icl_ua, __weak_chg_icl_ua, int, 0600
);

int __lpd_enable;

int __lpd_result;
module_param_named(
	lpd_result, __lpd_result, int, 0600
);

static int lpd_enable_set(const char *val, const struct kernel_param *kp);
module_param_call(lpd_enable, lpd_enable_set, param_get_int,
			&__lpd_enable, 0644);
MODULE_PARM_DESC(lpd_enable, "Enable lpd trigger control by user");


#define PMI632_MAX_ICL_UA	3000000
#define PM6150_MAX_FCC_UA	3000000
static int smb5_chg_config_init(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	struct pmic_revid_data *pmic_rev_id;
	struct device_node *revid_dev_node;
	int rc = 0;

	revid_dev_node = of_parse_phandle(chip->chg.dev->of_node,
					  "qcom,pmic-revid", 0);
	if (!revid_dev_node) {
		pr_err("Missing qcom,pmic-revid property\n");
		return -EINVAL;
	}

	pmic_rev_id = get_revid_data(revid_dev_node);
	if (IS_ERR_OR_NULL(pmic_rev_id)) {
		/*
		 * the revid peripheral must be registered, any failure
		 * here only indicates that the rev-id module has not
		 * probed yet.
		 */
		rc =  -EPROBE_DEFER;
		goto out;
	}

	switch (pmic_rev_id->pmic_subtype) {
	case PM8150B_SUBTYPE:
		chip->chg.smb_version = PM8150B_SUBTYPE;
		chg->param = smb5_pm8150b_params;
		chg->name = "pm8150b_charger";
		chg->wa_flags |= CHG_TERMINATION_WA;
		break;
	case PM6150_SUBTYPE:
		chip->chg.smb_version = PM6150_SUBTYPE;
		chg->param = smb5_pm8150b_params;
		chg->name = "pm6150_charger";
		chg->wa_flags |= SW_THERM_REGULATION_WA | CHG_TERMINATION_WA;
		if (pmic_rev_id->rev4 >= 2)
			chg->uusb_moisture_protection_enabled = true;
		chg->main_fcc_max = PM6150_MAX_FCC_UA;
		break;
	case PMI632_SUBTYPE:
		chip->chg.smb_version = PMI632_SUBTYPE;
		chg->wa_flags |= CHG_TERMINATION_WA;
		chg->param = smb5_pmi632_params;
		chg->use_extcon = true;
		chg->name = "pmi632_charger";
		/* PMI632 does not support PD */
		chg->pd_not_supported = true;
		chg->hw_max_icl_ua =
			(chip->dt.usb_icl_ua > 0) ? chip->dt.usb_icl_ua
						: PMI632_MAX_ICL_UA;
		break;
	default:
		pr_err("PMIC subtype %d not supported\n",
				pmic_rev_id->pmic_subtype);
		rc = -EINVAL;
		goto out;
	}

	chg->chg_freq.freq_5V			= 600;
	chg->chg_freq.freq_6V_8V		= 800;
	chg->chg_freq.freq_9V			= 1050;
	chg->chg_freq.freq_12V                  = 1200;
	chg->chg_freq.freq_removal		= 1050;
	chg->chg_freq.freq_below_otg_threshold	= 800;
	chg->chg_freq.freq_above_otg_threshold	= 800;

out:
	of_node_put(revid_dev_node);
	return rc;
}

#define MICRO_1P5A			1500000
#define MICRO_P1A			100000
#define MICRO_1PA			1000000
#define MICRO_3PA			3000000
#define OTG_DEFAULT_DEGLITCH_TIME_MS	50
#define DEFAULT_WD_BARK_TIME		64
static int smb5_parse_dt(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	struct device_node *node = chg->dev->of_node;
	int byte_len, rc = 0;
	enum of_gpio_flags flags;

	if (!node) {
		pr_err("device tree node missing\n");
		return -EINVAL;
	}

	of_property_read_u32(node, "qcom,sec-charger-config",
					&chip->dt.sec_charger_config);
	chg->sec_cp_present =
		chip->dt.sec_charger_config == POWER_SUPPLY_CHARGER_SEC_CP ||
		chip->dt.sec_charger_config == POWER_SUPPLY_CHARGER_SEC_CP_PL;

	chg->sec_pl_present =
		chip->dt.sec_charger_config == POWER_SUPPLY_CHARGER_SEC_PL ||
		chip->dt.sec_charger_config == POWER_SUPPLY_CHARGER_SEC_CP_PL;

/*read ffc param*/
	OF_PROP_READ(node, "ffc-pre-normal-decidegc",
			chg->FFC_TEMP_T1, rc, 1);
	OF_PROP_READ(node, "ffc-normal-decidegc",
		chg->FFC_TEMP_T2, rc, 1);
	OF_PROP_READ(node, "ffc-warm-decidegc",
		chg->FFC_TEMP_T3, rc, 1);
	OF_PROP_READ(node, "ffc-normal-fcc-ma",
		chg->FFC_NOR_FCC, rc, 1);
	OF_PROP_READ(node, "ffc-warm-fcc-ma",
		chg->FFC_WARM_FCC, rc, 1);
	OF_PROP_READ(node, "ffc-normal-cutoff-ma",
		chg->FFC_NORMAL_CUTOFF, rc, 1);
	OF_PROP_READ(node, "ffc-warm-cutoff-ma",
		chg->FFC_WARM_CUTOFF, rc, 1);
	OF_PROP_READ(node, "ffc-full-vbat-mv",
		chg->FFC_VBAT_FULL, rc, 1);
	pr_info("T1:%d, T2:%d, T3:%d, fcc1:%d, fcc1:%d, cut1:%d, cut2:%d,full:%d\n",
		chg->FFC_TEMP_T1, chg->FFC_TEMP_T2, chg->FFC_TEMP_T3,
		chg->FFC_NOR_FCC, chg->FFC_WARM_FCC, chg->FFC_NORMAL_CUTOFF,
		chg->FFC_WARM_CUTOFF, chg->FFC_VBAT_FULL);

/* @bsp, 2018/07/13 Battery & Charging porting */
	/* read ibatmax setting for different temp regions */
	OF_PROP_READ(node, "ibatmax-little-cold-ma",
			chg->ibatmax[BATT_TEMP_LITTLE_COLD], rc, 1);
	OF_PROP_READ(node, "ibatmax-cool-ma",
			chg->ibatmax[BATT_TEMP_COOL], rc, 1);
	OF_PROP_READ(node, "ibatmax-little-cool-ma",
			chg->ibatmax[BATT_TEMP_LITTLE_COOL], rc, 1);
	OF_PROP_READ(node, "ibatmax-pre-normal-ma",
			chg->ibatmax[BATT_TEMP_PRE_NORMAL], rc, 1);
	OF_PROP_READ(node, "ibatmax-normal-ma",
			chg->ibatmax[BATT_TEMP_NORMAL], rc, 1);
	OF_PROP_READ(node, "ibatmax-warm-ma",
			chg->ibatmax[BATT_TEMP_WARM], rc, 1);

	/* read vbatmax setting for different temp regions */
	OF_PROP_READ(node, "vbatmax-little-cold-mv",
			chg->vbatmax[BATT_TEMP_LITTLE_COLD], rc, 1);
	OF_PROP_READ(node, "vbatmax-cool-mv",
			chg->vbatmax[BATT_TEMP_COOL], rc, 1);
	OF_PROP_READ(node, "vbatmax-little-cool-mv",
			chg->vbatmax[BATT_TEMP_LITTLE_COOL], rc, 1);
	OF_PROP_READ(node, "vbatmax-pre-normal-mv",
			chg->vbatmax[BATT_TEMP_PRE_NORMAL], rc, 1);
	OF_PROP_READ(node, "vbatmax-normal-mv",
			chg->vbatmax[BATT_TEMP_NORMAL], rc, 1);
	OF_PROP_READ(node, "vbatmax-warm-mv",
			chg->vbatmax[BATT_TEMP_WARM], rc, 1);

	/* read vbatdet setting for different temp regions */
	OF_PROP_READ(node, "vbatdet-little-cold-mv",
			chg->vbatdet[BATT_TEMP_LITTLE_COLD], rc, 1);
	OF_PROP_READ(node, "vbatdet-cool-mv",
			chg->vbatdet[BATT_TEMP_COOL], rc, 1);
	OF_PROP_READ(node, "vbatdet-little-cool-mv",
			chg->vbatdet[BATT_TEMP_LITTLE_COOL], rc, 1);
	OF_PROP_READ(node, "vbatdet-pre-normal-mv",
			chg->vbatdet[BATT_TEMP_PRE_NORMAL], rc, 1);
	OF_PROP_READ(node, "vbatdet-normal-mv",
			chg->vbatdet[BATT_TEMP_NORMAL], rc, 1);
	OF_PROP_READ(node, "vbatdet-warm-mv",
			chg->vbatdet[BATT_TEMP_WARM], rc, 1);
	OF_PROP_READ(node, "little-cool-vbat-thr-mv",
			chg->temp_littel_cool_voltage, rc, 1);
	if (rc < 0)
		chg->temp_littel_cool_voltage = 4180;
	OF_PROP_READ(node, "cool-vbat-thr-mv",
			chg->temp_cool_voltage, rc, 1);
	if (rc < 0)
		chg->temp_cool_voltage = 4180;
	OF_PROP_READ(node, "ibatmax-little-cool-thr-ma",
			chg->temp_littel_cool_current, rc, 1);
	if (rc < 0)
		chg->temp_littel_cool_current = 2000;
	OF_PROP_READ(node, "ibatmax-cool-thr-ma",
			chg->temp_cool_current, rc, 1);
	if (rc < 0)
		chg->temp_cool_current = 1140;
	/* read temp region settings */
	OF_PROP_READ(node, "cold-bat-decidegc",
			chg->BATT_TEMP_T0, rc, 1);
	chg->BATT_TEMP_T0 = 0 - chg->BATT_TEMP_T0;
	OF_PROP_READ(node, "little-cold-bat-decidegc",
			chg->BATT_TEMP_T1, rc, 1);
	OF_PROP_READ(node, "cool-bat-decidegc",
			chg->BATT_TEMP_T2, rc, 1);
	OF_PROP_READ(node, "little-cool-bat-decidegc",
			chg->BATT_TEMP_T3, rc, 1);
	OF_PROP_READ(node, "pre-normal-bat-decidegc",
			chg->BATT_TEMP_T4, rc, 1);
	OF_PROP_READ(node, "warm-bat-decidegc",
			chg->BATT_TEMP_T5, rc, 1);
	OF_PROP_READ(node, "hot-bat-decidegc",
			chg->BATT_TEMP_T6, rc, 1);
	chg->pd_not_supported = of_property_read_bool(node,
						"disable-pd");

	chg->check_batt_full_by_sw = of_property_read_bool(node,
				"op,sw-check-full-enable");
	rc = of_property_read_u32(node,
					"op,sw-iterm-ma",
					&chg->sw_iterm_ma);
	if (rc < 0)
		chg->sw_iterm_ma = 150;
	pr_info("sw_iterm_ma=%d,check_batt_full_by_sw=%d",
				chg->sw_iterm_ma, chg->check_batt_full_by_sw);
/*yangfb@bsp, 20171023 otg-icl set 1A if battery lower than 15%*/
	chg->OTG_ICL_CTRL = of_property_read_bool(node,
						"op,otg-icl-ctrl-enable");
	OF_PROP_READ(node, "otg-low-battery-thr",
			chg->OTG_LOW_BAT, rc, 1);
	if (rc < 0)
		chg->OTG_LOW_BAT = -EINVAL;
	OF_PROP_READ(node, "otg-low-bat-icl-thr",
			chg->OTG_LOW_BAT_ICL, rc, 1);
	if (rc < 0)
		chg->OTG_LOW_BAT_ICL = -EINVAL;
	OF_PROP_READ(node, "otg-normal-bat-icl-thr",
			chg->OTG_NORMAL_BAT_ICL, rc, 1);
	if (rc < 0)
		chg->OTG_NORMAL_BAT_ICL = -EINVAL;
	pr_info("OTG_ICL:enable:%d,CapThr:%d,LowThr:%d,NorThr:%d\n",
		chg->OTG_ICL_CTRL,
		chg->OTG_LOW_BAT,
		chg->OTG_LOW_BAT_ICL,
		chg->OTG_NORMAL_BAT_ICL);
/* @bsp, 2018/07/26 enable stm6620 sheepmode */
	chg->shipmode_en = of_get_named_gpio_flags(node,
						"op,stm-ctrl-gpio", 0, &flags);
	chg->vbus_ctrl = of_get_named_gpio_flags(node,
					"op,vbus-ctrl-gpio", 0, &flags);
	chg->plug_irq = of_get_named_gpio_flags(node,
					"op,usb-check", 0, &flags);
	/* read other settings */
	OF_PROP_READ(node, "qcom,cutoff-voltage-with-charger",
				smbchg_cutoff_volt_with_charger, rc, 1);
	chg->chg_enabled = !(of_property_read_bool(node,
						"qcom,charging-disabled"));

	pr_info("T0=%d, T1=%d, T2=%d, T3=%d, T4=%d, T5=%d, T6=%d\n",
		chg->BATT_TEMP_T0, chg->BATT_TEMP_T1, chg->BATT_TEMP_T2,
		chg->BATT_TEMP_T3, chg->BATT_TEMP_T4, chg->BATT_TEMP_T5,
		chg->BATT_TEMP_T6);
	pr_info("BATT_TEMP_LITTLE_COLD=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_LITTLE_COLD],
		chg->vbatmax[BATT_TEMP_LITTLE_COLD],
		chg->vbatdet[BATT_TEMP_LITTLE_COLD]);
	pr_info("BATT_TEMP_COOL=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_COOL],
		chg->vbatmax[BATT_TEMP_COOL],
		chg->vbatdet[BATT_TEMP_COOL]);
	pr_info("BATT_TEMP_LITTLE_COOL=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_LITTLE_COOL],
		chg->vbatmax[BATT_TEMP_LITTLE_COOL],
		chg->vbatdet[BATT_TEMP_LITTLE_COOL]);
	pr_info("BATT_TEMP_PRE_NORMAL=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_PRE_NORMAL],
		chg->vbatmax[BATT_TEMP_PRE_NORMAL],
		chg->vbatdet[BATT_TEMP_PRE_NORMAL]);
	pr_info("BATT_TEMP_NORMAL=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_NORMAL],
		chg->vbatmax[BATT_TEMP_NORMAL],
		chg->vbatdet[BATT_TEMP_NORMAL]);
	pr_info("BATT_TEMP_WARM=%d, %d, %d\n",
		chg->ibatmax[BATT_TEMP_WARM],
		chg->vbatmax[BATT_TEMP_WARM],
		chg->vbatdet[BATT_TEMP_WARM]);
	pr_info("cutoff_volt_with_charger=%d, disable-pd=%d\n",
		smbchg_cutoff_volt_with_charger, *chg->pd_disabled);

	/* disable step_chg */
	chg->step_chg_enabled = false;

	chg->sw_jeita_enabled = of_property_read_bool(node,
				"qcom,sw-jeita-enable");

	rc = of_property_read_u32(node, "qcom,wd-bark-time-secs",
					&chip->dt.wd_bark_time);
	if (rc < 0 || chip->dt.wd_bark_time < MIN_WD_BARK_TIME)
		chip->dt.wd_bark_time = DEFAULT_WD_BARK_TIME;

	chip->dt.no_battery = of_property_read_bool(node,
						"qcom,batteryless-platform");

	rc = of_property_read_u32(node,
			"qcom,fcc-max-ua", &chip->dt.batt_profile_fcc_ua);
	if (rc < 0)
		chip->dt.batt_profile_fcc_ua = -EINVAL;

	rc = of_property_read_u32(node,
				"qcom,fv-max-uv", &chip->dt.batt_profile_fv_uv);
	if (rc < 0)
		chip->dt.batt_profile_fv_uv = -EINVAL;

	rc = of_property_read_u32(node,
				"qcom,usb-icl-ua", &chip->dt.usb_icl_ua);
	if (rc < 0)
		chip->dt.usb_icl_ua = -EINVAL;

	rc = of_property_read_u32(node,
				"qcom,otg-cl-ua", &chg->otg_cl_ua);
	if (rc < 0)
		chg->otg_cl_ua = (chip->chg.smb_version == PMI632_SUBTYPE) ?
							MICRO_1PA : MICRO_3PA;

	rc = of_property_read_u32(node, "qcom,chg-term-src",
			&chip->dt.term_current_src);
	if (rc < 0)
		chip->dt.term_current_src = ITERM_SRC_UNSPECIFIED;

	rc = of_property_read_u32(node, "qcom,chg-term-current-ma",
			&chip->dt.term_current_thresh_hi_ma);

	if (chip->dt.term_current_src == ITERM_SRC_ADC)
		rc = of_property_read_u32(node, "qcom,chg-term-base-current-ma",
				&chip->dt.term_current_thresh_lo_ma);

	if (of_find_property(node, "qcom,thermal-mitigation", &byte_len)) {
		chg->thermal_mitigation = devm_kzalloc(chg->dev, byte_len,
			GFP_KERNEL);

		if (chg->thermal_mitigation == NULL)
			return -ENOMEM;

		chg->thermal_levels = byte_len / sizeof(u32);
		rc = of_property_read_u32_array(node,
				"qcom,thermal-mitigation",
				chg->thermal_mitigation,
				chg->thermal_levels);
		if (rc < 0) {
			dev_err(chg->dev,
				"Couldn't read threm limits rc = %d\n", rc);
			return rc;
		}
	}

	rc = of_property_read_u32(node, "qcom,charger-temp-max",
			&chg->charger_temp_max);
	if (rc < 0)
		chg->charger_temp_max = -EINVAL;

	rc = of_property_read_u32(node, "qcom,smb-temp-max",
			&chg->smb_temp_max);
	if (rc < 0)
		chg->smb_temp_max = -EINVAL;

	rc = of_property_read_u32(node, "qcom,float-option",
						&chip->dt.float_option);
	if (!rc && (chip->dt.float_option < 0 || chip->dt.float_option > 4)) {
		pr_err("qcom,float-option is out of range [0, 4]\n");
		return -EINVAL;
	}

	chip->dt.hvdcp_disable = of_property_read_bool(node,
						"qcom,hvdcp-disable");
	chg->hvdcp_disable = chip->dt.hvdcp_disable;

	rc = of_property_read_u32(node, "qcom,chg-inhibit-threshold-mv",
				&chip->dt.chg_inhibit_thr_mv);
	if (!rc && (chip->dt.chg_inhibit_thr_mv < 0 ||
				chip->dt.chg_inhibit_thr_mv > 300)) {
		pr_err("qcom,chg-inhibit-threshold-mv is incorrect\n");
		return -EINVAL;
	}

	chip->dt.auto_recharge_soc = -EINVAL;
	rc = of_property_read_u32(node, "qcom,auto-recharge-soc",
				&chip->dt.auto_recharge_soc);
	if (!rc && (chip->dt.auto_recharge_soc < 0 ||
			chip->dt.auto_recharge_soc > 100)) {
		pr_err("qcom,auto-recharge-soc is incorrect\n");
		return -EINVAL;
	}
	chg->auto_recharge_soc = chip->dt.auto_recharge_soc;

	chip->dt.auto_recharge_vbat_mv = -EINVAL;
	rc = of_property_read_u32(node, "qcom,auto-recharge-vbat-mv",
				&chip->dt.auto_recharge_vbat_mv);
	if (!rc && (chip->dt.auto_recharge_vbat_mv < 0)) {
		pr_err("qcom,auto-recharge-vbat-mv is incorrect\n");
		return -EINVAL;
	}

	chg->dcp_icl_ua = chip->dt.usb_icl_ua;

	chg->suspend_input_on_debug_batt = of_property_read_bool(node,
					"qcom,suspend-input-on-debug-batt");

	rc = of_property_read_u32(node, "qcom,otg-deglitch-time-ms",
					&chg->otg_delay_ms);
	if (rc < 0)
		chg->otg_delay_ms = OTG_DEFAULT_DEGLITCH_TIME_MS;

	rc = of_property_match_string(node, "io-channel-names",
			"gpio1_voltage");
	if (rc >= 0) {
		chg->iio.op_connector_temp_chan = iio_channel_get(chg->dev,
				"gpio1_voltage");
		if (IS_ERR(chg->iio.op_connector_temp_chan)) {
			rc = PTR_ERR(chg->iio.op_connector_temp_chan);
			if (rc != -EPROBE_DEFER)
				dev_err(chg->dev,
				"op_connector_temp_chan channel unavailable,%ld\n",
				rc);
			chg->iio.op_connector_temp_chan = NULL;
			return rc;
		}
	}

	chg->lpd_disabled = of_property_read_bool(node,
				"qcom,lpd-disable");
	chg->fcc_stepper_enable = of_property_read_bool(node,
					"qcom,fcc-stepping-enable");
	chg->low_voltage_charger = of_property_read_bool(node,
					"op,low-voltage-charger");
	chg->uusb_moisture_protection_enabled =
				chg->uusb_moisture_protection_enabled &&
				of_property_read_bool(node,
				"qcom,uusb-moisture-protection-enable");

	/* Extract ADC channels */
	rc = smblib_get_iio_channel(chg, "mid_voltage", &chg->iio.mid_chan);
	if (rc < 0)
		return rc;

	if (!chg->iio.mid_chan || chg->low_voltage_charger) {
		rc = smblib_get_iio_channel(chg, "usb_in_voltage",
				&chg->iio.usbin_v_chan);
		if (rc < 0)
			return rc;

		if (!chg->iio.usbin_v_chan) {
			dev_err(chg->dev, "No voltage channel defined");
			return -EINVAL;
		}
	}

	rc = smblib_get_iio_channel(chg, "chg_temp", &chg->iio.temp_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "usb_in_current",
					&chg->iio.usbin_i_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "sbux_res", &chg->iio.sbux_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "vph_voltage", &chg->iio.vph_v_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "die_temp", &chg->iio.die_temp_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "conn_temp",
					&chg->iio.connector_temp_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "skin_temp", &chg->iio.skin_temp_chan);
	if (rc < 0)
		return rc;

	rc = smblib_get_iio_channel(chg, "smb_temp", &chg->iio.smb_temp_chan);
	if (rc < 0)
		return rc;

	chip->dt.disable_suspend_on_collapse = of_property_read_bool(node,
					"qcom,disable-suspend-on-collapse");
	return 0;
}

/************************
 * USB PSY REGISTRATION *
 ************************/
static enum power_supply_property smb5_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_PD_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_TYPEC_MODE,
	POWER_SUPPLY_PROP_TYPEC_POWER_ROLE,
	POWER_SUPPLY_PROP_TYPEC_CC_ORIENTATION,
	POWER_SUPPLY_PROP_OTG_SWITCH,
	POWER_SUPPLY_PROP_HW_DETECT,
	POWER_SUPPLY_PROP_OEM_TYPEC_CC_ORIENTATION,
	POWER_SUPPLY_PROP_LOW_POWER,
	POWER_SUPPLY_PROP_PD_ACTIVE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED,
	POWER_SUPPLY_PROP_INPUT_CURRENT_NOW,
	POWER_SUPPLY_PROP_BOOST_CURRENT,
	POWER_SUPPLY_PROP_PE_START,
	POWER_SUPPLY_PROP_CTM_CURRENT_MAX,
	POWER_SUPPLY_PROP_HW_CURRENT_MAX,
	POWER_SUPPLY_PROP_REAL_TYPE,
	POWER_SUPPLY_PROP_PD_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_PD_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_CONNECTOR_TYPE,
	POWER_SUPPLY_PROP_CONNECTOR_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_SMB_EN_MODE,
	POWER_SUPPLY_PROP_SMB_EN_REASON,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_MOISTURE_DETECTED,
};

static int smb5_usb_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	union power_supply_propval pval;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		rc = smblib_get_prop_usb_present(chg, val);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		rc = smblib_get_prop_usb_online(chg, val);
		if (!val->intval)
			break;

		if (((chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_DEFAULT) ||
			(chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_MEDIUM) ||
			(chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_HIGH) ||
		   (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB))
			&& (chg->real_charger_type == POWER_SUPPLY_TYPE_USB ||
			chg->real_charger_type == POWER_SUPPLY_TYPE_USB_CDP))
			val->intval = 0;
		else
			val->intval = 1;

/* add to fix wrong charging type when boot with pd cable connected */
		if (chg->real_charger_type == POWER_SUPPLY_TYPE_UNKNOWN &&
			!chg->pd_active)
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		rc = smblib_get_prop_usb_voltage_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		rc = smblib_get_prop_usb_voltage_now(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_CURRENT_MAX:
		val->intval = get_client_vote(chg->usb_icl_votable, PD_VOTER);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_get_prop_input_current_settled(chg, val);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = POWER_SUPPLY_TYPE_USB_PD;
		break;
	case POWER_SUPPLY_PROP_REAL_TYPE:
		val->intval = chg->real_charger_type;
		break;
	case POWER_SUPPLY_PROP_TYPEC_MODE:
		if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB)
			val->intval = POWER_SUPPLY_TYPEC_NONE;
		else
			val->intval = chg->typec_mode;
		break;
	case POWER_SUPPLY_PROP_OTG_SWITCH:
		val->intval = chg->otg_switch;
		break;
	case POWER_SUPPLY_PROP_HW_DETECT:
		val->intval = chg->hw_detect;
		break;

	case POWER_SUPPLY_PROP_TYPEC_POWER_ROLE:
		if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB)
			val->intval = POWER_SUPPLY_TYPEC_PR_NONE;
		else
			rc = smblib_get_prop_typec_power_role(chg, val);
		break;
	case POWER_SUPPLY_PROP_TYPEC_CC_ORIENTATION:
	case POWER_SUPPLY_PROP_OEM_TYPEC_CC_ORIENTATION:
		if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB)
			val->intval = 0;
		else
			rc = smblib_get_prop_typec_cc_orientation(chg, val);
		break;
	case POWER_SUPPLY_PROP_TYPEC_SRC_RP:
		rc = smblib_get_prop_typec_select_rp(chg, val);
		break;
	case POWER_SUPPLY_PROP_LOW_POWER:
		if (chg->sink_src_mode == SRC_MODE)
			rc = smblib_get_prop_low_power(chg, val);
		else
			rc = -ENODATA;
		break;
	case POWER_SUPPLY_PROP_PD_ACTIVE:
		val->intval = chg->pd_active;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED:
		rc = smblib_get_prop_input_current_settled(chg, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_NOW:
		rc = smblib_get_prop_usb_current_now(chg, val);
		break;
	case POWER_SUPPLY_PROP_BOOST_CURRENT:
		val->intval = chg->boost_current_ua;
		break;
	case POWER_SUPPLY_PROP_PD_IN_HARD_RESET:
		rc = smblib_get_prop_pd_in_hard_reset(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_USB_SUSPEND_SUPPORTED:
		val->intval = chg->system_suspend_supported;
		break;
	case POWER_SUPPLY_PROP_PE_START:
		rc = smblib_get_pe_start(chg, val);
		break;
	case POWER_SUPPLY_PROP_CTM_CURRENT_MAX:
		val->intval = get_client_vote(chg->usb_icl_votable, CTM_VOTER);
		break;
	case POWER_SUPPLY_PROP_HW_CURRENT_MAX:
		rc = smblib_get_charge_current(chg, &val->intval);
		break;
	case POWER_SUPPLY_PROP_PR_SWAP:
		rc = smblib_get_prop_pr_swap_in_progress(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_VOLTAGE_MAX:
		val->intval = chg->voltage_max_uv;
		break;
	case POWER_SUPPLY_PROP_PD_VOLTAGE_MIN:
		val->intval = chg->voltage_min_uv;
		break;
	case POWER_SUPPLY_PROP_SDP_CURRENT_MAX:
		val->intval = get_client_vote(chg->usb_icl_votable,
					      USB_PSY_VOTER);
		break;
	case POWER_SUPPLY_PROP_CONNECTOR_TYPE:
		val->intval = chg->connector_type;
		break;
	case POWER_SUPPLY_PROP_CONNECTOR_HEALTH:
		if (chg->connector_health == -EINVAL)
			val->intval = smblib_get_prop_connector_health(chg);
		else
			val->intval = chg->connector_health;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_UNKNOWN;
		rc = smblib_get_prop_usb_present(chg, &pval);
		if (rc < 0)
			break;
		val->intval = pval.intval ? POWER_SUPPLY_SCOPE_DEVICE
				: chg->otg_present ? POWER_SUPPLY_SCOPE_SYSTEM
						: POWER_SUPPLY_SCOPE_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_SMB_EN_MODE:
		mutex_lock(&chg->smb_lock);
		val->intval = chg->sec_chg_selected;
		mutex_unlock(&chg->smb_lock);
		break;
	case POWER_SUPPLY_PROP_SMB_EN_REASON:
		val->intval = chg->cp_reason;
		break;
	case POWER_SUPPLY_PROP_MOISTURE_DETECTED:
		val->intval = chg->moisture_present;
		break;
	default:
		pr_err("get prop %d is not supported in usb\n", psp);
		rc = -EINVAL;
		break;
	}

	if (rc < 0) {
		pr_debug("Couldn't get prop %d rc = %d\n", psp, rc);
		return -ENODATA;
	}

	return 0;
}

static int smb5_usb_set_prop(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_PD_CURRENT_MAX:
		rc = smblib_set_prop_pd_current_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_OTG_SWITCH:
		rc = vote(chg->otg_toggle_votable, USER_VOTER,
						val->intval, 0);
		break;
	case POWER_SUPPLY_PROP_TYPEC_POWER_ROLE:
		rc = smblib_set_prop_typec_power_role(chg, val);
		break;
	case POWER_SUPPLY_PROP_TYPEC_SRC_RP:
		rc = smblib_set_prop_typec_select_rp(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_ACTIVE:
		rc = smblib_set_prop_pd_active(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_IN_HARD_RESET:
		rc = smblib_set_prop_pd_in_hard_reset(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_USB_SUSPEND_SUPPORTED:
		chg->system_suspend_supported = val->intval;
		break;
	case POWER_SUPPLY_PROP_BOOST_CURRENT:
		rc = smblib_set_prop_boost_current(chg, val);
		break;
	case POWER_SUPPLY_PROP_CTM_CURRENT_MAX:
		rc = vote(chg->usb_icl_votable, CTM_VOTER,
						val->intval >= 0, val->intval);
		break;
	case POWER_SUPPLY_PROP_PR_SWAP:
		rc = smblib_set_prop_pr_swap_in_progress(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_VOLTAGE_MAX:
		rc = smblib_set_prop_pd_voltage_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_PD_VOLTAGE_MIN:
		rc = smblib_set_prop_pd_voltage_min(chg, val);
		break;
	case POWER_SUPPLY_PROP_SDP_CURRENT_MAX:
		rc = smblib_set_prop_sdp_current_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_CONNECTOR_HEALTH:
		chg->connector_health = val->intval;
		power_supply_changed(chg->usb_psy);
		break;
	default:
		pr_err("set prop %d is not supported\n", psp);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int smb5_usb_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_OTG_SWITCH:
	case POWER_SUPPLY_PROP_CTM_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CONNECTOR_HEALTH:
		return 1;
	default:
		break;
	}

	return 0;
}

static const struct power_supply_desc usb_psy_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB_PD,
	.properties = smb5_usb_props,
	.num_properties = ARRAY_SIZE(smb5_usb_props),
	.get_property = smb5_usb_get_prop,
	.set_property = smb5_usb_set_prop,
	.property_is_writeable = smb5_usb_prop_is_writeable,
};

static int smb5_init_usb_psy(struct smb5 *chip)
{
	struct power_supply_config usb_cfg = {};
	struct smb_charger *chg = &chip->chg;

	usb_cfg.drv_data = chip;
	usb_cfg.of_node = chg->dev->of_node;
	chg->usb_psy = devm_power_supply_register(chg->dev,
						  &usb_psy_desc,
						  &usb_cfg);
	if (IS_ERR(chg->usb_psy)) {
		pr_err("Couldn't register USB power supply\n");
		return PTR_ERR(chg->usb_psy);
	}

	return 0;
}

/********************************
 * USB PC_PORT PSY REGISTRATION *
 ********************************/
static enum power_supply_property smb5_usb_port_props[] = {
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};

static int smb5_usb_port_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = POWER_SUPPLY_TYPE_USB;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		rc = smblib_get_prop_usb_online(chg, val);
		if (!val->intval)
			break;

		if (((chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_DEFAULT) ||
			(chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_MEDIUM) ||
			(chg->typec_mode == POWER_SUPPLY_TYPEC_SOURCE_HIGH) ||
		   (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB))
			&& (chg->real_charger_type == POWER_SUPPLY_TYPE_USB ||
			chg->real_charger_type == POWER_SUPPLY_TYPE_USB_CDP))
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = 5000000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_get_prop_input_current_settled(chg, val);
		break;
	default:
		pr_err_ratelimited("Get prop %d is not supported in pc_port\n",
				psp);
		return -EINVAL;
	}

	if (rc < 0) {
		pr_debug("Couldn't get prop %d rc = %d\n", psp, rc);
		return -ENODATA;
	}

	return 0;
}

static int smb5_usb_port_set_prop(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	int rc = 0;

	switch (psp) {
	default:
		pr_err_ratelimited("Set prop %d is not supported in pc_port\n",
				psp);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static const struct power_supply_desc usb_port_psy_desc = {
	.name		= "pc_port",
	.type		= POWER_SUPPLY_TYPE_USB,
	.properties	= smb5_usb_port_props,
	.num_properties	= ARRAY_SIZE(smb5_usb_port_props),
	.get_property	= smb5_usb_port_get_prop,
	.set_property	= smb5_usb_port_set_prop,
};

static int smb5_init_usb_port_psy(struct smb5 *chip)
{
	struct power_supply_config usb_port_cfg = {};
	struct smb_charger *chg = &chip->chg;

	usb_port_cfg.drv_data = chip;
	usb_port_cfg.of_node = chg->dev->of_node;
	chg->usb_port_psy = devm_power_supply_register(chg->dev,
						  &usb_port_psy_desc,
						  &usb_port_cfg);
	if (IS_ERR(chg->usb_port_psy)) {
		pr_err("Couldn't register USB pc_port power supply\n");
		return PTR_ERR(chg->usb_port_psy);
	}

	return 0;
}

/*****************************
 * USB MAIN PSY REGISTRATION *
 *****************************/

static enum power_supply_property smb5_usb_main_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_SETTLED,
	POWER_SUPPLY_PROP_FCC_DELTA,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_FLASH_ACTIVE,
	POWER_SUPPLY_PROP_FLASH_TRIGGER,
	POWER_SUPPLY_PROP_TOGGLE_STAT,
	POWER_SUPPLY_PROP_MAIN_FCC_MAX,
};

static int smb5_usb_main_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		rc = smblib_get_charge_param(chg, &chg->param.fv, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		rc = smblib_get_charge_param(chg, &chg->param.fcc,
							&val->intval);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = POWER_SUPPLY_TYPE_MAIN;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_SETTLED:
		rc = smblib_get_prop_input_current_settled(chg, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_SETTLED:
		rc = smblib_get_prop_input_voltage_settled(chg, val);
		break;
	case POWER_SUPPLY_PROP_FCC_DELTA:
		rc = smblib_get_prop_fcc_delta(chg, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_get_icl_current(chg, &val->intval);
		break;
	case POWER_SUPPLY_PROP_FLASH_ACTIVE:
		val->intval = chg->flash_active;
		break;
	case POWER_SUPPLY_PROP_FLASH_TRIGGER:
		rc = schgm_flash_get_vreg_ok(chg, &val->intval);
		break;
	case POWER_SUPPLY_PROP_TOGGLE_STAT:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_MAIN_FCC_MAX:
		val->intval = chg->main_fcc_max;
		break;
	default:
		pr_debug("get prop %d is not supported in usb-main\n", psp);
		rc = -EINVAL;
		break;
	}
	if (rc < 0) {
		pr_debug("Couldn't get prop %d rc = %d\n", psp, rc);
		return -ENODATA;
	}

	return 0;
}

static int smb5_usb_main_set_prop(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		rc = smblib_set_charge_param(chg, &chg->param.fv, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		rc = smblib_set_charge_param(chg, &chg->param.fcc, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_set_icl_current(chg, val->intval);
		break;
	case POWER_SUPPLY_PROP_FLASH_ACTIVE:
		chg->flash_active = val->intval;
		break;
	case POWER_SUPPLY_PROP_TOGGLE_STAT:
		rc = smblib_toggle_smb_en(chg, val->intval);
		break;
	case POWER_SUPPLY_PROP_MAIN_FCC_MAX:
		chg->main_fcc_max = val->intval;
		rerun_election(chg->fcc_votable);
		break;
	default:
		pr_err("set prop %d is not supported\n", psp);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int smb5_usb_main_prop_is_writeable(struct power_supply *psy,
				enum power_supply_property psp)
{
	int rc;

	switch (psp) {
	case POWER_SUPPLY_PROP_TOGGLE_STAT:
	case POWER_SUPPLY_PROP_MAIN_FCC_MAX:
		rc = 1;
		break;
	default:
		rc = 0;
		break;
	}

	return rc;
}

static const struct power_supply_desc usb_main_psy_desc = {
	.name		= "main",
	.type		= POWER_SUPPLY_TYPE_MAIN,
	.properties	= smb5_usb_main_props,
	.num_properties	= ARRAY_SIZE(smb5_usb_main_props),
	.get_property	= smb5_usb_main_get_prop,
	.set_property	= smb5_usb_main_set_prop,
	.property_is_writeable = smb5_usb_main_prop_is_writeable,
};

static int smb5_init_usb_main_psy(struct smb5 *chip)
{
	struct power_supply_config usb_main_cfg = {};
	struct smb_charger *chg = &chip->chg;

	usb_main_cfg.drv_data = chip;
	usb_main_cfg.of_node = chg->dev->of_node;
	chg->usb_main_psy = devm_power_supply_register(chg->dev,
						  &usb_main_psy_desc,
						  &usb_main_cfg);
	if (IS_ERR(chg->usb_main_psy)) {
		pr_err("Couldn't register USB main power supply\n");
		return PTR_ERR(chg->usb_main_psy);
	}

	return 0;
}

/*************************
 * DC PSY REGISTRATION   *
 *************************/

static enum power_supply_property smb5_dc_props[] = {
	POWER_SUPPLY_PROP_INPUT_SUSPEND,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION,
	POWER_SUPPLY_PROP_REAL_TYPE,
};

static int smb5_dc_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_SUSPEND:
		val->intval = get_effective_result(chg->dc_suspend_votable);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		rc = smblib_get_prop_dc_present(chg, val);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		rc = smblib_get_prop_dc_online(chg, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		rc = smblib_get_prop_dc_voltage_now(chg, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_get_prop_dc_current_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		rc = smblib_get_prop_dc_voltage_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_REAL_TYPE:
		val->intval = POWER_SUPPLY_TYPE_WIPOWER;
		break;
	default:
		return -EINVAL;
	}
	if (rc < 0) {
		pr_debug("Couldn't get prop %d rc = %d\n", psp, rc);
		return -ENODATA;
	}
	return 0;
}

static int smb5_dc_set_prop(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct smb5 *chip = power_supply_get_drvdata(psy);
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_SUSPEND:
		rc = vote(chg->dc_suspend_votable, WBC_VOTER,
				(bool)val->intval, 0);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		rc = smblib_set_prop_dc_current_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		rc = smblib_set_prop_voltage_wls_output(chg, val);
		break;
	default:
		return -EINVAL;
	}

	return rc;
}

static int smb5_dc_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	int rc;

	switch (psp) {
	default:
		rc = 0;
		break;
	}

	return rc;
}

static const struct power_supply_desc dc_psy_desc = {
	.name = "dc",
	.type = POWER_SUPPLY_TYPE_WIRELESS,
	.properties = smb5_dc_props,
	.num_properties = ARRAY_SIZE(smb5_dc_props),
	.get_property = smb5_dc_get_prop,
	.set_property = smb5_dc_set_prop,
	.property_is_writeable = smb5_dc_prop_is_writeable,
};

static int smb5_init_dc_psy(struct smb5 *chip)
{
	struct power_supply_config dc_cfg = {};
	struct smb_charger *chg = &chip->chg;

	dc_cfg.drv_data = chip;
	dc_cfg.of_node = chg->dev->of_node;
	chg->dc_psy = devm_power_supply_register(chg->dev,
						  &dc_psy_desc,
						  &dc_cfg);
	if (IS_ERR(chg->dc_psy)) {
		pr_err("Couldn't register USB power supply\n");
		return PTR_ERR(chg->dc_psy);
	}

	return 0;
}

/*************************
 * BATT PSY REGISTRATION *
 *************************/
static enum power_supply_property smb5_batt_props[] = {
	POWER_SUPPLY_PROP_INPUT_SUSPEND,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_FASTCHG_IS_OK,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHG_PROTECT_STATUS,
	POWER_SUPPLY_PROP_FASTCHG_STATUS,
	POWER_SUPPLY_PROP_FASTCHG_STARTING,
	POWER_SUPPLY_CUTOFF_VOLT_WITH_CHARGER,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_INPUT_CURRENT_MAX,
	POWER_SUPPLY_PROP_IS_AGING_TEST,
	POWER_SUPPLY_PROP_CONNECTER_TEMP,
	POWER_SUPPLY_PROP_CONNECT_DISABLE,
	POWER_SUPPLY_PROP_CHARGER_TEMP,
	POWER_SUPPLY_PROP_CHARGER_TEMP_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMITED,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_QNOVO,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_QNOVO,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_STEP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_SW_JEITA_ENABLED,
	POWER_SUPPLY_PROP_CHARGE_DONE,
	POWER_SUPPLY_PROP_PARALLEL_DISABLE,
	POWER_SUPPLY_PROP_SET_SHIP_MODE,
	POWER_SUPPLY_PROP_DIE_HEALTH,
	POWER_SUPPLY_PROP_RERUN_AICL,
	POWER_SUPPLY_PROP_DP_DM,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_RECHARGE_SOC,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_FORCE_RECHARGE,
	POWER_SUPPLY_PROP_FCC_STEPPER_ENABLE,
};

static int smb5_batt_get_prop(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct smb_charger *chg = power_supply_get_drvdata(psy);
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = get_prop_batt_status(chg);
		break;
	case POWER_SUPPLY_PROP_FASTCHG_IS_OK:
		val->intval = get_prop_fastchg_is_ok(chg)?1:0;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		rc = smblib_get_prop_batt_health(chg, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		rc = smblib_get_prop_batt_present(chg, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_SUSPEND:
		rc = smblib_get_prop_input_suspend(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		rc = smblib_get_prop_batt_charge_type(chg, val);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		rc = smblib_get_prop_batt_capacity(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		rc = smblib_get_prop_usb_voltage_now(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHG_PROTECT_STATUS:
		val->intval = get_prop_chg_protect_status(chg);
		break;
	case POWER_SUPPLY_PROP_FASTCHG_STATUS:
		val->intval = get_prop_fastchg_status(chg);
		break;
	case POWER_SUPPLY_CUTOFF_VOLT_WITH_CHARGER:
		val->intval = smbchg_cutoff_volt_with_charger;
		break;
	case POWER_SUPPLY_PROP_FASTCHG_STARTING:
		val->intval = op_get_fastchg_ing(chg);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = chg->chg_enabled;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		rc = smblib_get_prop_input_current_limited(chg, val);
		break;
	case POWER_SUPPLY_PROP_IS_AGING_TEST:
		val->intval = chg->is_aging_test;
		break;
	case POWER_SUPPLY_PROP_CONNECTER_TEMP:
		val->intval = chg->connecter_temp;
		break;
	case POWER_SUPPLY_PROP_CONNECT_DISABLE:
		val->intval = chg->disconnect_vbus;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		rc = smblib_get_prop_system_temp_level(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		rc = smblib_get_prop_system_temp_level_max(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGER_TEMP:
		rc = smblib_get_prop_charger_temp(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGER_TEMP_MAX:
		val->intval = chg->charger_temp_max;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMITED:
		rc = smblib_get_prop_input_current_limited(chg, val);
		break;
	case POWER_SUPPLY_PROP_STEP_CHARGING_ENABLED:
		val->intval = chg->step_chg_enabled;
		break;
	case POWER_SUPPLY_PROP_SW_JEITA_ENABLED:
		val->intval = chg->sw_jeita_enabled;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		rc = smblib_get_prop_from_bms(chg,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = get_client_vote(chg->fv_votable,
				BATT_PROFILE_VOTER);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_QNOVO:
		val->intval = get_client_vote_locked(chg->fv_votable,
				QNOVO_VOTER);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		rc = smblib_get_prop_from_bms(chg,
				POWER_SUPPLY_PROP_CURRENT_NOW, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_QNOVO:
		val->intval = get_client_vote_locked(chg->fcc_votable,
				QNOVO_VOTER);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = get_client_vote(chg->fcc_votable,
					      BATT_PROFILE_VOTER);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		rc = smblib_get_prop_batt_iterm(chg, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		rc = smblib_get_prop_batt_temp(chg, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_DONE:
		rc = smblib_get_prop_batt_charge_done(chg, val);
		break;
	case POWER_SUPPLY_PROP_PARALLEL_DISABLE:
		val->intval = get_client_vote(chg->pl_disable_votable,
					      USER_VOTER);
		break;
	case POWER_SUPPLY_PROP_SET_SHIP_MODE:
		/* Not in ship mode as long as device is active */
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_DIE_HEALTH:
		if (chg->die_health == -EINVAL)
			val->intval = smblib_get_prop_die_health(chg);
		else
			val->intval = chg->die_health;
		break;
	case POWER_SUPPLY_PROP_DP_DM:
		val->intval = chg->pulse_cnt;
		break;
	case POWER_SUPPLY_PROP_RERUN_AICL:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		rc = smblib_get_prop_from_bms(chg,
				POWER_SUPPLY_PROP_CHARGE_COUNTER, val);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		rc = smblib_get_prop_from_bms(chg,
				POWER_SUPPLY_PROP_CYCLE_COUNT, val);
		break;
	case POWER_SUPPLY_PROP_RECHARGE_SOC:
		val->intval = chg->auto_recharge_soc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_QNOVO_ENABLE:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		rc = smblib_get_prop_from_bms(chg,
				POWER_SUPPLY_PROP_CHARGE_FULL, val);
		break;
	case POWER_SUPPLY_PROP_FORCE_RECHARGE:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_FCC_STEPPER_ENABLE:
		val->intval = chg->fcc_stepper_enable;
		break;
	default:
		pr_err("batt power supply prop %d not supported\n", psp);
		return -EINVAL;
	}

	if (rc < 0) {
		pr_debug("Couldn't get prop %d rc = %d\n", psp, rc);
		return -ENODATA;
	}

	return 0;
}

static int smb5_batt_set_prop(struct power_supply *psy,
		enum power_supply_property prop,
		const union power_supply_propval *val)
{
	int rc = 0;
	struct smb_charger *chg = power_supply_get_drvdata(psy);

	switch (prop) {
	case POWER_SUPPLY_PROP_STATUS:
		rc = smblib_set_prop_batt_status(chg, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_SUSPEND:
		rc = smblib_set_prop_input_suspend(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		rc = smblib_set_prop_system_temp_level(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHECK_USB_UNPLUG:
		if (chg->vbus_present && !chg->dash_present)
			update_dash_unplug_status();
		break;
	case POWER_SUPPLY_PROP_SWITCH_DASH:
		rc = check_allow_switch_dash(chg, val);
		break;
	case POWER_SUPPLY_PROP_FASTCHG_IS_OK:
		rc = 0;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		pr_info("set iusb %d mA\n", val->intval);
		if (__debug_mask == PR_OP_DEBUG
			|| val->intval == 2000000 || val->intval == 1700000
			|| val->intval == 1500000 || val->intval == 1000000)
		op_usb_icl_set(chg, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONNECT_DISABLE:
		op_disconnect_vbus(chg, (bool)val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		rc = smblib_set_prop_chg_voltage(chg, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		rc = smblib_set_prop_batt_temp(chg, val);
		break;
	case POWER_SUPPLY_PROP_CHG_PROTECT_STATUS:
		rc = smblib_set_prop_chg_protect_status(chg, val);
		break;
	case POWER_SUPPLY_PROP_NOTIFY_CHARGER_SET_PARAMETER:
		rc = smblib_set_prop_charge_parameter_set(chg);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		if (!val->intval) {
			chg->dash_on = get_prop_fast_chg_started(chg);
			if (chg->dash_on) {
				switch_mode_to_normal();
				op_set_fast_chg_allow(chg, false);
			}
		}
		rc = vote(chg->usb_icl_votable, USER_VOTER,
				!val->intval, 0);
		rc = vote(chg->dc_suspend_votable, USER_VOTER,
				!val->intval, 0);
		chg->chg_enabled = (bool)val->intval;
		break;
	case POWER_SUPPLY_PROP_IS_AGING_TEST:
		chg->is_aging_test = (bool)val->intval;
		__debug_mask = PR_OP_DEBUG;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		rc = smblib_set_prop_batt_capacity(chg, val);
		break;
	case POWER_SUPPLY_PROP_PARALLEL_DISABLE:
		vote(chg->pl_disable_votable, USER_VOTER, (bool)val->intval, 0);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		chg->batt_profile_fv_uv = val->intval;
		vote(chg->fv_votable, BATT_PROFILE_VOTER, true, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_QNOVO:
		vote(chg->fv_votable, QNOVO_VOTER, (val->intval >= 0),
			val->intval);
		break;
	case POWER_SUPPLY_PROP_STEP_CHARGING_ENABLED:
		chg->step_chg_enabled = !!val->intval;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		chg->batt_profile_fcc_ua = val->intval;
		vote(chg->fcc_votable, BATT_PROFILE_VOTER, true, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_QNOVO:
		vote(chg->pl_disable_votable, PL_QNOVO_VOTER,
			val->intval != -EINVAL && val->intval < 2000000, 0);
		if (val->intval == -EINVAL) {
			vote(chg->fcc_votable, BATT_PROFILE_VOTER,
					true, chg->batt_profile_fcc_ua);
			vote(chg->fcc_votable, QNOVO_VOTER, false, 0);
		} else {
			vote(chg->fcc_votable, QNOVO_VOTER, true, val->intval);
			vote(chg->fcc_votable, BATT_PROFILE_VOTER, false, 0);
		}
		break;
	case POWER_SUPPLY_PROP_SET_SHIP_MODE:
		/* Not in ship mode as long as the device is active */
		if (!val->intval)
			break;
		if (chg->pl.psy)
			power_supply_set_property(chg->pl.psy,
				POWER_SUPPLY_PROP_SET_SHIP_MODE, val);
		rc = smblib_set_prop_ship_mode(chg, val);
		break;
	case POWER_SUPPLY_PROP_RERUN_AICL:
		rc = smblib_rerun_aicl(chg);
		break;
	case POWER_SUPPLY_PROP_DP_DM:
		rc = smblib_dp_dm(chg, val->intval);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMITED:
		rc = smblib_set_prop_input_current_limited(chg, val);
		break;
	case POWER_SUPPLY_PROP_DIE_HEALTH:
		chg->die_health = val->intval;
		power_supply_changed(chg->batt_psy);
		break;
	case POWER_SUPPLY_PROP_RECHARGE_SOC:
		rc = smblib_set_prop_rechg_soc_thresh(chg, val);
		break;
	case POWER_SUPPLY_PROP_FORCE_RECHARGE:
			/* toggle charging to force recharge */
			vote(chg->chg_disable_votable, FORCE_RECHARGE_VOTER,
					true, 0);
			/* charge disable delay */
			msleep(50);
			vote(chg->chg_disable_votable, FORCE_RECHARGE_VOTER,
					false, 0);
		break;
	case POWER_SUPPLY_PROP_FCC_STEPPER_ENABLE:
		chg->fcc_stepper_enable = val->intval;
		break;
	default:
		rc = -EINVAL;
	}

	return rc;
}

static int smb5_batt_prop_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_INPUT_SUSPEND:
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_CHARGE_NOW:
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_CHG_PROTECT_STATUS:
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
	case POWER_SUPPLY_PROP_IS_AGING_TEST:
	case POWER_SUPPLY_PROP_CONNECT_DISABLE:
	case POWER_SUPPLY_PROP_FASTCHG_IS_OK:
	case POWER_SUPPLY_PROP_PARALLEL_DISABLE:
	case POWER_SUPPLY_PROP_DP_DM:
	case POWER_SUPPLY_PROP_RERUN_AICL:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMITED:
	case POWER_SUPPLY_PROP_STEP_CHARGING_ENABLED:
	case POWER_SUPPLY_PROP_DIE_HEALTH:
		return 1;
	default:
		break;
	}

	return 0;
}

static const struct power_supply_desc batt_psy_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = smb5_batt_props,
	.num_properties = ARRAY_SIZE(smb5_batt_props),
	.get_property = smb5_batt_get_prop,
	.set_property = smb5_batt_set_prop,
	.property_is_writeable = smb5_batt_prop_is_writeable,
};

static int smb5_init_batt_psy(struct smb5 *chip)
{
	struct power_supply_config batt_cfg = {};
	struct smb_charger *chg = &chip->chg;
	int rc = 0;

	batt_cfg.drv_data = chg;
	batt_cfg.of_node = chg->dev->of_node;
	chg->batt_psy = devm_power_supply_register(chg->dev,
					   &batt_psy_desc,
					   &batt_cfg);
	if (IS_ERR(chg->batt_psy)) {
		pr_err("Couldn't register battery power supply\n");
		return PTR_ERR(chg->batt_psy);
	}

	return rc;
}

/******************************
 * VBUS REGULATOR REGISTRATION *
 ******************************/

static struct regulator_ops smb5_vbus_reg_ops = {
	.enable = smblib_vbus_regulator_enable,
	.disable = smblib_vbus_regulator_disable,
	.is_enabled = smblib_vbus_regulator_is_enabled,
};

static int smb5_init_vbus_regulator(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	struct regulator_config cfg = {};
	int rc = 0;

	chg->vbus_vreg = devm_kzalloc(chg->dev, sizeof(*chg->vbus_vreg),
				      GFP_KERNEL);
	if (!chg->vbus_vreg)
		return -ENOMEM;

	cfg.dev = chg->dev;
	cfg.driver_data = chip;

	chg->vbus_vreg->rdesc.owner = THIS_MODULE;
	chg->vbus_vreg->rdesc.type = REGULATOR_VOLTAGE;
	chg->vbus_vreg->rdesc.ops = &smb5_vbus_reg_ops;
	chg->vbus_vreg->rdesc.of_match = "qcom,smb5-vbus";
	chg->vbus_vreg->rdesc.name = "qcom,smb5-vbus";

	chg->vbus_vreg->rdev = devm_regulator_register(chg->dev,
						&chg->vbus_vreg->rdesc, &cfg);
	if (IS_ERR(chg->vbus_vreg->rdev)) {
		rc = PTR_ERR(chg->vbus_vreg->rdev);
		chg->vbus_vreg->rdev = NULL;
		if (rc != -EPROBE_DEFER)
			pr_err("Couldn't register VBUS regulator rc=%d\n", rc);
	}

	return rc;
}

/******************************
 * VCONN REGULATOR REGISTRATION *
 ******************************/

static struct regulator_ops smb5_vconn_reg_ops = {
	.enable = smblib_vconn_regulator_enable,
	.disable = smblib_vconn_regulator_disable,
	.is_enabled = smblib_vconn_regulator_is_enabled,
};

static int smb5_init_vconn_regulator(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	struct regulator_config cfg = {};
	int rc = 0;

	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB)
		return 0;

	chg->vconn_vreg = devm_kzalloc(chg->dev, sizeof(*chg->vconn_vreg),
				      GFP_KERNEL);
	if (!chg->vconn_vreg)
		return -ENOMEM;

	cfg.dev = chg->dev;
	cfg.driver_data = chip;

	chg->vconn_vreg->rdesc.owner = THIS_MODULE;
	chg->vconn_vreg->rdesc.type = REGULATOR_VOLTAGE;
	chg->vconn_vreg->rdesc.ops = &smb5_vconn_reg_ops;
	chg->vconn_vreg->rdesc.of_match = "qcom,smb5-vconn";
	chg->vconn_vreg->rdesc.name = "qcom,smb5-vconn";

	chg->vconn_vreg->rdev = devm_regulator_register(chg->dev,
						&chg->vconn_vreg->rdesc, &cfg);
	if (IS_ERR(chg->vconn_vreg->rdev)) {
		rc = PTR_ERR(chg->vconn_vreg->rdev);
		chg->vconn_vreg->rdev = NULL;
		if (rc != -EPROBE_DEFER)
			pr_err("Couldn't register VCONN regulator rc=%d\n", rc);
	}

	return rc;
}

/***************************
 * HARDWARE INITIALIZATION *
 ***************************/
static int smb5_configure_typec(struct smb_charger *chg)
{
	int rc;
	u8  stat, val;

	smblib_apsd_enable(chg, true);
	smblib_hvdcp_detect_enable(chg, false);

	rc = smblib_masked_write(chg, TYPE_C_CFG_REG,
				BC1P2_START_ON_CC_BIT, 0);
	if (rc < 0) {
		dev_err(chg->dev, "failed to write TYPE_C_CFG_REG rc=%d\n",
				rc);

		return rc;
	}

	/* add to fix huawei cable compatible issue */
	rc = smblib_write(chg, DEBUG_ACCESS_SNK_CFG_REG, 0x7);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't write DEBUG_ACCESS_SNK_CFG_REG rc=%d\n", rc);
		return rc;
	}

	/* Use simple write to clear interrupts */
	/* For GCE-2351 issue Debug patch Set 0x155E[3]=1
	 * TYPEC_CCOUT_DETACH_INT_EN
	 */
	rc = smblib_write(chg, TYPE_C_INTERRUPT_EN_CFG_1_REG,
				TYPEC_CCOUT_DETACH_INT_EN_BIT);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure Type-C interrupts rc=%d\n", rc);
		return rc;
	}

	rc = smblib_read(chg, TYPE_C_INTERRUPT_EN_CFG_1_REG, &stat);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure Type-C interrupts rc=%d\n", rc);
	}
	pr_info("TYPE_C_INTERRUPT_EN_CFG_1_REG:0x%02x=0x%02x\n",
			TYPE_C_INTERRUPT_EN_CFG_1_REG, stat);

	val = chg->lpd_disabled ? 0 : TYPEC_WATER_DETECTION_INT_EN_BIT;

	/* Use simple write to enable only required interrupts */
	rc = smblib_write(chg, TYPE_C_INTERRUPT_EN_CFG_2_REG,
				TYPEC_SRC_BATT_HPWR_INT_EN_BIT |
				val);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure Type-C interrupts rc=%d\n", rc);
		return rc;
	}

	if (chg->otg_switch) {
		rc = smblib_masked_write(chg, TYPE_C_MODE_CFG_REG,
					EN_TRY_SNK_BIT, EN_TRY_SNK_BIT);
		if (rc < 0) {
			dev_err(chg->dev,
				"Couldn't enable try.snk rc=%d\n", rc);
			return rc;
		}
	} else {
		rc = smblib_masked_write(chg, TYPE_C_MODE_CFG_REG,
					EN_SNK_ONLY_BIT, EN_SNK_ONLY_BIT);
		if (rc < 0) {
			dev_err(chg->dev,
				"Couldn't enable snk.only rc=%d\n", rc);
			return rc;
		}
	}
/*
	rc = smblib_masked_write(chg, TYPE_C_MODE_CFG_REG,
				EN_TRY_SNK_BIT, EN_TRY_SNK_BIT);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't enable try.snk rc=%d\n", rc);
		return rc;
	} else {
		chg->typec_try_mode |= EN_TRY_SNK_BIT;
	}

*/
	/* configure VCONN for software control */
	rc = smblib_masked_write(chg, TYPE_C_VCONN_CONTROL_REG,
				 VCONN_EN_SRC_BIT | VCONN_EN_VALUE_BIT,
				 VCONN_EN_SRC_BIT);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure VCONN for SW control rc=%d\n", rc);
		return rc;
	}

	/* Enable detection of unoriented debug accessory in source mode */
	rc = smblib_masked_write(chg, DEBUG_ACCESS_SRC_CFG_REG,
				 EN_UNORIENTED_DEBUG_ACCESS_SRC_BIT,
				 EN_UNORIENTED_DEBUG_ACCESS_SRC_BIT);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure TYPE_C_DEBUG_ACCESS_SRC_CFG_REG rc=%d\n",
				rc);
		return rc;
	}

	rc = smblib_masked_write(chg, USBIN_LOAD_CFG_REG,
		USBIN_IN_COLLAPSE_GF_SEL_MASK | USBIN_AICL_STEP_TIMING_SEL_MASK,
		0);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't set USBIN_LOAD_CFG_REG rc=%d\n", rc);
		return rc;
	}

	/* Set CC threshold to 1.6 V in source mode */
	rc = smblib_masked_write(chg, TYPE_C_EXIT_STATE_CFG_REG,
				SEL_SRC_UPPER_REF_BIT, SEL_SRC_UPPER_REF_BIT);
	if (rc < 0)
		dev_err(chg->dev,
			"Couldn't configure CC threshold voltage rc=%d\n", rc);

	return rc;
}

static int smb5_configure_micro_usb(struct smb_charger *chg)
{
	int rc;

	/* For micro USB connector, use extcon by default */
	chg->use_extcon = true;
	chg->pd_not_supported = true;

	rc = smblib_masked_write(chg, TYPE_C_INTERRUPT_EN_CFG_2_REG,
					MICRO_USB_STATE_CHANGE_INT_EN_BIT,
					MICRO_USB_STATE_CHANGE_INT_EN_BIT);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure Type-C interrupts rc=%d\n", rc);
		return rc;
	}

	if (chg->uusb_moisture_protection_enabled) {
		/* Enable moisture detection interrupt */
		rc = smblib_masked_write(chg, TYPE_C_INTERRUPT_EN_CFG_2_REG,
				TYPEC_WATER_DETECTION_INT_EN_BIT,
				TYPEC_WATER_DETECTION_INT_EN_BIT);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't enable moisture detection interrupt rc=%d\n",
				rc);
			return rc;
		}

		/* Enable uUSB factory mode */
		rc = smblib_masked_write(chg, TYPEC_U_USB_CFG_REG,
					EN_MICRO_USB_FACTORY_MODE_BIT,
					EN_MICRO_USB_FACTORY_MODE_BIT);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't enable uUSB factory mode c=%d\n",
				rc);
			return rc;
		}

		/* Disable periodic monitoring of CC_ID pin */
		rc = smblib_write(chg, TYPEC_U_USB_WATER_PROTECTION_CFG_REG, 0);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't disable periodic monitoring of CC_ID rc=%d\n",
				rc);
			return rc;
		}
	}

	return rc;
}

static int smb5_configure_iterm_thresholds_adc(struct smb5 *chip)
{
	u8 *buf;
	int rc = 0;
	s16 raw_hi_thresh, raw_lo_thresh;
	struct smb_charger *chg = &chip->chg;

	if (chip->dt.term_current_thresh_hi_ma < -10000 ||
			chip->dt.term_current_thresh_hi_ma > 10000 ||
			chip->dt.term_current_thresh_lo_ma < -10000 ||
			chip->dt.term_current_thresh_lo_ma > 10000) {
		dev_err(chg->dev, "ITERM threshold out of range rc=%d\n", rc);
		return -EINVAL;
	}

	/*
	 * Conversion:
	 *	raw (A) = (scaled_mA * ADC_CHG_TERM_MASK) / (10 * 1000)
	 * Note: raw needs to be converted to big-endian format.
	 */

	if (chip->dt.term_current_thresh_hi_ma) {
		raw_hi_thresh = ((chip->dt.term_current_thresh_hi_ma *
						ADC_CHG_TERM_MASK) / 10000);
		raw_hi_thresh = sign_extend32(raw_hi_thresh, 15);
		buf = (u8 *)&raw_hi_thresh;
		raw_hi_thresh = buf[1] | (buf[0] << 8);

		rc = smblib_batch_write(chg, CHGR_ADC_ITERM_UP_THD_MSB_REG,
				(u8 *)&raw_hi_thresh, 2);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure ITERM threshold HIGH rc=%d\n",
					rc);
			return rc;
		}
	}

	if (chip->dt.term_current_thresh_lo_ma) {
		raw_lo_thresh = ((chip->dt.term_current_thresh_lo_ma *
					ADC_CHG_TERM_MASK) / 10000);
		raw_lo_thresh = sign_extend32(raw_lo_thresh, 15);
		buf = (u8 *)&raw_lo_thresh;
		raw_lo_thresh = buf[1] | (buf[0] << 8);

		rc = smblib_batch_write(chg, CHGR_ADC_ITERM_LO_THD_MSB_REG,
				(u8 *)&raw_lo_thresh, 2);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure ITERM threshold LOW rc=%d\n",
					rc);
			return rc;
		}
	}

	return rc;
}

static int smb5_configure_iterm_thresholds(struct smb5 *chip)
{
	int rc = 0;

	switch (chip->dt.term_current_src) {
	case ITERM_SRC_ADC:
		rc = smb5_configure_iterm_thresholds_adc(chip);
		break;
	default:
		break;
	}

	return rc;
}

static int smb5_init_hw(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	int rc, type = 0;
	u8 val = 0, mask = 0;
	union power_supply_propval pval;

	if (chip->dt.no_battery)
		chg->fake_capacity = 50;

	if (chip->dt.batt_profile_fcc_ua < 0)
		smblib_get_charge_param(chg, &chg->param.fcc,
				&chg->batt_profile_fcc_ua);

	if (chip->dt.batt_profile_fv_uv < 0)
		smblib_get_charge_param(chg, &chg->param.fv,
				&chg->batt_profile_fv_uv);

	smblib_get_charge_param(chg, &chg->param.usb_icl,
				&chg->default_icl_ua);
	pr_info("vbat_max=%d, ibat_max=%d, iusb_max=%d\n",
		chg->batt_profile_fv_uv,
		chg->batt_profile_fcc_ua, chip->dt.usb_icl_ua);
	if (chg->charger_temp_max == -EINVAL) {
		rc = smblib_get_thermal_threshold(chg,
					DIE_REG_H_THRESHOLD_MSB_REG,
					&chg->charger_temp_max);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't get charger_temp_max rc=%d\n",
					rc);
			return rc;
		}
	}

	/*
	 * If SW thermal regulation WA is active then all the HW temperature
	 * comparators need to be disabled to prevent HW thermal regulation,
	 * apart from DIE_TEMP analog comparator for SHDN regulation.
	 */
	if (chg->wa_flags & SW_THERM_REGULATION_WA) {
		rc = smblib_write(chg, MISC_THERMREG_SRC_CFG_REG,
					THERMREG_DIE_CMP_SRC_EN_BIT);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't disable HW thermal regulation rc=%d\n",
				rc);
			return rc;
		}
	} else {
		/* Allows software thermal regulation only */
		rc = smblib_write(chg, MISC_THERMREG_SRC_CFG_REG,
					 THERMREG_SW_ICL_ADJUST_BIT);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure SMB thermal regulation rc=%d\n",
					rc);
			return rc;
		}
	}

	/*
	 * Disable HVDCP autonomous mode operation by default. Additionally, if
	 * specified in DT: disable HVDCP and HVDCP authentication algorithm.
	 */
	val = (chg->hvdcp_disable) ? 0 :
		(HVDCP_AUTH_ALG_EN_CFG_BIT | HVDCP_EN_BIT);
	rc = smblib_masked_write(chg, USBIN_OPTIONS_1_CFG_REG,
			(HVDCP_AUTH_ALG_EN_CFG_BIT | HVDCP_EN_BIT |
			 HVDCP_AUTONOMOUS_MODE_EN_CFG_BIT),
			val);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure HVDCP rc=%d\n", rc);
		return rc;
	}

	/*
	 * PMI632 can have the connector type defined by a dedicated register
	 * TYPEC_MICRO_USB_MODE_REG or by a common TYPEC_U_USB_CFG_REG.
	 */
	if (chg->smb_version == PMI632_SUBTYPE) {
		rc = smblib_read(chg, TYPEC_MICRO_USB_MODE_REG, &val);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't read USB mode rc=%d\n", rc);
			return rc;
		}
		type = !!(val & MICRO_USB_MODE_ONLY_BIT);
	}

	/*
	 * If TYPEC_MICRO_USB_MODE_REG is not set and for all non-PMI632
	 * check the connector type using TYPEC_U_USB_CFG_REG.
	 */
	if (!type) {
		rc = smblib_read(chg, TYPEC_U_USB_CFG_REG, &val);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't read U_USB config rc=%d\n",
					rc);
			return rc;
		}

		type = !!(val & EN_MICRO_USB_MODE_BIT);
	}

	pr_debug("Connector type=%s\n", type ? "Micro USB" : "TypeC");

	if (type) {
		chg->connector_type = POWER_SUPPLY_CONNECTOR_MICRO_USB;
		rc = smb5_configure_micro_usb(chg);
	} else {
		chg->connector_type = POWER_SUPPLY_CONNECTOR_TYPEC;
		rc = smb5_configure_typec(chg);
	}
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure TypeC/micro-USB mode rc=%d\n", rc);
		return rc;
	}

	/*
	 * PMI632 based hw init:
	 * - Rerun APSD to ensure proper charger detection if device
	 *   boots with charger connected.
	 * - Initialize flash module for PMI632
	 */
	if (chg->smb_version == PMI632_SUBTYPE) {
		schgm_flash_init(chg);
		smblib_rerun_apsd_if_required(chg);
	}

	/* Use ICL results from HW */
	rc = smblib_icl_override(chg, HW_AUTO_MODE);
	if (rc < 0) {
		pr_err("Couldn't disable ICL override rc=%d\n", rc);
		return rc;
	}
	vote(chg->usb_icl_votable,
		DEFAULT_VOTER, !chg->chg_enabled, 0);
	vote(chg->dc_suspend_votable,
		DEFAULT_VOTER, !chg->chg_enabled, 0);
	smblib_set_charge_param(chg, &chg->param.fcc,
			chg->ibatmax[BATT_TEMP_NORMAL] * 1000);
	smblib_set_charge_param(chg, &chg->param.fv,
			chg->vbatmax[BATT_TEMP_NORMAL] * 1000);
	/* set OTG current limit */
	rc = smblib_set_charge_param(chg, &chg->param.otg_cl, chg->otg_cl_ua);
	if (rc < 0) {
		pr_err("Couldn't set otg current limit rc=%d\n", rc);
		return rc;
	}
	/* Some h/w limit maximum supported ICL */
	vote(chg->usb_icl_votable, HW_LIMIT_VOTER,
			chg->hw_max_icl_ua > 0, chg->hw_max_icl_ua);

	/* set DC icl_max 1A */
	rc = smblib_set_charge_param(chg, &chg->param.dc_icl, 1000000);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't set dc_icl rc=%d\n", rc);
		return rc;
	}
	/* disable HVDCP */
	rc = smblib_masked_write(chg, USBIN_OPTIONS_1_CFG_REG,
		HVDCP_EN_BIT, 0);
	if (rc < 0)
		dev_err(chg->dev, "Couldn't disable HVDCP rc=%d\n", rc);

	/* aicl rerun time */
	rc = smblib_masked_write(chg, AICL_RERUN_TIME_CFG_REG,
		BIT(0)|BIT(1), 0);
	if (rc < 0)
		dev_err(chg->dev, "Couldn't set aicl rerunTimerc=%d\n", rc);

	/* Disable DC Input missing poller function */
	rc = smblib_masked_write(chg, DCIN_LOAD_CFG_REG,
					INPUT_MISS_POLL_EN_BIT, 0);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't disable DC Input missing poller rc=%d\n", rc);
		return rc;
	}

	/*
	 * AICL configuration:
	 * start from min and AICL ADC disable, and enable aicl rerun
	 */
	if (chg->smb_version != PMI632_SUBTYPE) {
		mask = USBIN_AICL_PERIODIC_RERUN_EN_BIT | USBIN_AICL_ADC_EN_BIT
			| USBIN_AICL_EN_BIT | SUSPEND_ON_COLLAPSE_USBIN_BIT;
		val = USBIN_AICL_PERIODIC_RERUN_EN_BIT | USBIN_AICL_EN_BIT;
		if (!chip->dt.disable_suspend_on_collapse)
			val |= SUSPEND_ON_COLLAPSE_USBIN_BIT;

		rc = smblib_masked_write(chg, USBIN_AICL_OPTIONS_CFG_REG,
				mask, val);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't config AICL rc=%d\n", rc);
			return rc;
		}
	}

	rc = smblib_write(chg, AICL_RERUN_TIME_CFG_REG,
				AICL_RERUN_TIME_12S_VAL);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure AICL rerun interval rc=%d\n", rc);
		return rc;
	}

	/* enable the charging path */
	rc = vote(chg->chg_disable_votable, DEFAULT_VOTER, false, 0);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't enable charging rc=%d\n", rc);
		return rc;
	}

	/* configure VBUS for software control */
	rc = smblib_masked_write(chg, DCDC_OTG_CFG_REG, OTG_EN_SRC_CFG_BIT, 0);
	if (rc < 0) {
		dev_err(chg->dev,
			"Couldn't configure VBUS for SW control rc=%d\n", rc);
		return rc;
	}

	val = (ilog2(chip->dt.wd_bark_time / 16) << BARK_WDOG_TIMEOUT_SHIFT)
			& BARK_WDOG_TIMEOUT_MASK;
	val |= BITE_WDOG_TIMEOUT_8S;
	rc = smblib_masked_write(chg, SNARL_BARK_BITE_WD_CFG_REG,
			BITE_WDOG_DISABLE_CHARGING_CFG_BIT |
			BARK_WDOG_TIMEOUT_MASK | BITE_WDOG_TIMEOUT_MASK,
			val);
	if (rc < 0) {
		pr_err("Couldn't configue WD config rc=%d\n", rc);
		return rc;
	}

	/* enable WD BARK and enable it on plugin */
	rc = smblib_masked_write(chg, WD_CFG_REG,
			WATCHDOG_TRIGGER_AFP_EN_BIT |
			WDOG_TIMER_EN_ON_PLUGIN_BIT |
			BARK_WDOG_INT_EN_BIT,
			WDOG_TIMER_EN_ON_PLUGIN_BIT |
			BARK_WDOG_INT_EN_BIT);
	if (rc < 0) {
		pr_err("Couldn't configue WD config rc=%d\n", rc);
		return rc;
	}

	/* set termination current threshold values */
	rc = smb5_configure_iterm_thresholds(chip);
	if (rc < 0) {
		pr_err("Couldn't configure ITERM thresholds rc=%d\n",
				rc);
		return rc;
	}

	/* configure float charger options */
	switch (chip->dt.float_option) {
	case FLOAT_DCP:
		rc = smblib_masked_write(chg, USBIN_OPTIONS_2_CFG_REG,
				FLOAT_OPTIONS_MASK, 0);
		break;
	case FLOAT_SDP:
		rc = smblib_masked_write(chg, USBIN_OPTIONS_2_CFG_REG,
				FLOAT_OPTIONS_MASK, FORCE_FLOAT_SDP_CFG_BIT);
		break;
	case DISABLE_CHARGING:
		rc = smblib_masked_write(chg, USBIN_OPTIONS_2_CFG_REG,
				FLOAT_OPTIONS_MASK, FLOAT_DIS_CHGING_CFG_BIT);
		break;
	case SUSPEND_INPUT:
		rc = smblib_masked_write(chg, USBIN_OPTIONS_2_CFG_REG,
				FLOAT_OPTIONS_MASK, SUSPEND_FLOAT_CFG_BIT);
		break;
	default:
		rc = 0;
		break;
	}

	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure float charger options rc=%d\n",
			rc);
		return rc;
	}

	rc = smblib_read(chg, USBIN_OPTIONS_2_CFG_REG, &chg->float_cfg);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't read float charger options rc=%d\n",
			rc);
		return rc;
	}

	switch (chip->dt.chg_inhibit_thr_mv) {
	case 50:
		rc = smblib_masked_write(chg, CHARGE_INHIBIT_THRESHOLD_CFG_REG,
				CHARGE_INHIBIT_THRESHOLD_MASK,
				INHIBIT_ANALOG_VFLT_MINUS_50MV);
		break;
	case 100:
		rc = smblib_masked_write(chg, CHARGE_INHIBIT_THRESHOLD_CFG_REG,
				CHARGE_INHIBIT_THRESHOLD_MASK,
				INHIBIT_ANALOG_VFLT_MINUS_100MV);
		break;
	case 200:
		rc = smblib_masked_write(chg, CHARGE_INHIBIT_THRESHOLD_CFG_REG,
				CHARGE_INHIBIT_THRESHOLD_MASK,
				INHIBIT_ANALOG_VFLT_MINUS_200MV);
		break;
	case 300:
		rc = smblib_masked_write(chg, CHARGE_INHIBIT_THRESHOLD_CFG_REG,
				CHARGE_INHIBIT_THRESHOLD_MASK,
				INHIBIT_ANALOG_VFLT_MINUS_300MV);
		break;
	case 0:
		rc = smblib_masked_write(chg, CHGR_CFG2_REG,
				CHARGER_INHIBIT_BIT, 0);
	default:
		break;
	}

	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure charge inhibit threshold rc=%d\n",
			rc);
		return rc;
	}

	rc = smblib_write(chg, CHGR_FAST_CHARGE_SAFETY_TIMER_CFG_REG,
					FAST_CHARGE_SAFETY_TIMER_768_MIN);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't set CHGR_FAST_CHARGE_SAFETY_TIMER_CFG_REG rc=%d\n",
			rc);
		return rc;
	}

	rc = smblib_masked_write(chg, CHGR_CFG2_REG, RECHG_MASK,
				(chip->dt.auto_recharge_vbat_mv != -EINVAL) ?
				VBAT_BASED_RECHG_BIT : 0);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure VBAT-rechg CHG_CFG2_REG rc=%d\n",
			rc);
		return rc;
	}

	/* program the auto-recharge VBAT threshold */
	if (chip->dt.auto_recharge_vbat_mv != -EINVAL) {
		u32 temp = VBAT_TO_VRAW_ADC(chip->dt.auto_recharge_vbat_mv);

		temp = ((temp & 0xFF00) >> 8) | ((temp & 0xFF) << 8);
		rc = smblib_batch_write(chg,
			CHGR_ADC_RECHARGE_THRESHOLD_MSB_REG, (u8 *)&temp, 2);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure ADC_RECHARGE_THRESHOLD REG rc=%d\n",
				rc);
			return rc;
		}
		/* Program the sample count for VBAT based recharge to 3 */
		rc = smblib_masked_write(chg, CHGR_NO_SAMPLE_TERM_RCHG_CFG_REG,
					NO_OF_SAMPLE_FOR_RCHG,
					2 << NO_OF_SAMPLE_FOR_RCHG_SHIFT);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure CHGR_NO_SAMPLE_FOR_TERM_RCHG_CFG rc=%d\n",
				rc);
			return rc;
		}
	}

	rc = smblib_masked_write(chg, CHGR_CFG2_REG, RECHG_MASK,
				(chip->dt.auto_recharge_soc != -EINVAL) ?
				SOC_BASED_RECHG_BIT : VBAT_BASED_RECHG_BIT);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure SOC-rechg CHG_CFG2_REG rc=%d\n",
			rc);
		return rc;
	}

	/* program the auto-recharge threshold */
	if (chip->dt.auto_recharge_soc != -EINVAL) {
		pval.intval = chip->dt.auto_recharge_soc;
		rc = smblib_set_prop_rechg_soc_thresh(chg, &pval);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure CHG_RCHG_SOC_REG rc=%d\n",
					rc);
			return rc;
		}

		/* Program the sample count for SOC based recharge to 1 */
		rc = smblib_masked_write(chg, CHGR_NO_SAMPLE_TERM_RCHG_CFG_REG,
						NO_OF_SAMPLE_FOR_RCHG, 0);
		if (rc < 0) {
			dev_err(chg->dev, "Couldn't configure CHGR_NO_SAMPLE_FOR_TERM_RCHG_CFG rc=%d\n",
				rc);
			return rc;
		}
	}

	rc = smblib_disable_hw_jeita(chg, true);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't set hw jeita rc=%d\n", rc);
		return rc;
	}

	rc = smblib_masked_write(chg, DCDC_ENG_SDCDC_CFG5_REG,
			ENG_SDCDC_BAT_HPWR_MASK, BOOST_MODE_THRESH_3P6_V);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure DCDC_ENG_SDCDC_CFG5 rc=%d\n",
				rc);
		return rc;
	}

	return rc;
}

static int smb5_post_init(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	union power_supply_propval pval;
	int rc;

	/*
	 * In case the usb path is suspended, we would have missed disabling
	 * the icl change interrupt because the interrupt could have been
	 * not requested
	 */
	rerun_election(chg->usb_icl_votable);

	/* configure power role for dual-role */
	pval.intval = POWER_SUPPLY_TYPEC_PR_DUAL;
	rc = smblib_set_prop_typec_power_role(chg, &pval);
	if (rc < 0) {
		dev_err(chg->dev, "Couldn't configure DRP role rc=%d\n",
				rc);
		return rc;
	}

	rerun_election(chg->usb_irq_enable_votable);

	return 0;
}

/****************************
 * DETERMINE INITIAL STATUS *
 ****************************/

static int smb5_determine_initial_status(struct smb5 *chip)
{
	struct smb_irq_data irq_data = {chip, "determine-initial-status"};
	struct smb_charger *chg = &chip->chg;
	union power_supply_propval val;
	int rc;

	rc = smblib_get_prop_usb_present(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get usb present rc=%d\n", rc);
		return rc;
	}
	chg->early_usb_attach = val.intval;

	if (chg->bms_psy)
		smblib_suspend_on_debug_battery(chg);

	usb_plugin_irq_handler(0, &irq_data);
	typec_attach_detach_irq_handler(0, &irq_data);
	typec_state_change_irq_handler(0, &irq_data);
	usb_source_change_irq_handler(0, &irq_data);
	chg_state_change_irq_handler(0, &irq_data);
	icl_change_irq_handler(0, &irq_data);
	batt_temp_changed_irq_handler(0, &irq_data);
	wdog_bark_irq_handler(0, &irq_data);
	typec_or_rid_detection_change_irq_handler(0, &irq_data);
	wdog_snarl_irq_handler(0, &irq_data);

	return 0;
}

/**************************
 * INTERRUPT REGISTRATION *
 **************************/

static struct smb_irq_info smb5_irqs[] = {
	/* CHARGER IRQs */
	[CHGR_ERROR_IRQ] = {
		.name		= "chgr-error",
		.handler	= default_irq_handler,
	},
	[CHG_STATE_CHANGE_IRQ] = {
		.name		= "chg-state-change",
		.handler	= chg_state_change_irq_handler,
		.wake		= true,
	},
	[STEP_CHG_STATE_CHANGE_IRQ] = {
		.name		= "step-chg-state-change",
	},
	[STEP_CHG_SOC_UPDATE_FAIL_IRQ] = {
		.name		= "step-chg-soc-update-fail",
	},
	[STEP_CHG_SOC_UPDATE_REQ_IRQ] = {
		.name		= "step-chg-soc-update-req",
	},
	[FG_FVCAL_QUALIFIED_IRQ] = {
		.name		= "fg-fvcal-qualified",
	},
	[VPH_ALARM_IRQ] = {
		.name		= "vph-alarm",
	},
	[VPH_DROP_PRECHG_IRQ] = {
		.name		= "vph-drop-prechg",
	},
	/* DCDC IRQs */
	[OTG_FAIL_IRQ] = {
		.name		= "otg-fail",
		.handler	= default_irq_handler,
	},
	[OTG_OC_DISABLE_SW_IRQ] = {
		.name		= "otg-oc-disable-sw",
	},
	[OTG_OC_HICCUP_IRQ] = {
		.name		= "otg-oc-hiccup",
	},
	[BSM_ACTIVE_IRQ] = {
		.name		= "bsm-active",
	},
	[HIGH_DUTY_CYCLE_IRQ] = {
		.name		= "high-duty-cycle",
		.handler	= high_duty_cycle_irq_handler,
		.wake		= true,
	},
	[INPUT_CURRENT_LIMITING_IRQ] = {
		.name		= "input-current-limiting",
		.handler	= default_irq_handler,
	},
	[CONCURRENT_MODE_DISABLE_IRQ] = {
		.name		= "concurrent-mode-disable",
	},
	[SWITCHER_POWER_OK_IRQ] = {
		.name		= "switcher-power-ok",
		.handler	= switcher_power_ok_irq_handler,
	},
	/* BATTERY IRQs */
	[BAT_TEMP_IRQ] = {
		.name		= "bat-temp",
		.handler	= batt_temp_changed_irq_handler,
		.wake		= true,
	},
	[ALL_CHNL_CONV_DONE_IRQ] = {
		.name		= "all-chnl-conv-done",
	},
	[BAT_OV_IRQ] = {
		.name		= "bat-ov",
		.handler	= batt_psy_changed_irq_handler,
	},
	[BAT_LOW_IRQ] = {
		.name		= "bat-low",
		.handler	= batt_psy_changed_irq_handler,
	},
	[BAT_THERM_OR_ID_MISSING_IRQ] = {
		.name		= "bat-therm-or-id-missing",
		.handler	= batt_psy_changed_irq_handler,
	},
	[BAT_TERMINAL_MISSING_IRQ] = {
		.name		= "bat-terminal-missing",
		.handler	= batt_psy_changed_irq_handler,
	},
	[BUCK_OC_IRQ] = {
		.name		= "buck-oc",
	},
	[VPH_OV_IRQ] = {
		.name		= "vph-ov",
	},
	/* USB INPUT IRQs */
	[USBIN_COLLAPSE_IRQ] = {
		.name		= "usbin-collapse",
		.handler	= default_irq_handler,
	},
	[USBIN_VASHDN_IRQ] = {
		.name		= "usbin-vashdn",
		.handler	= default_irq_handler,
	},
	[USBIN_UV_IRQ] = {
		.name		= "usbin-uv",
		.handler	= usbin_uv_irq_handler,
	},
	[USBIN_OV_IRQ] = {
		.name		= "usbin-ov",
		.handler	= default_irq_handler,
	},
	[USBIN_PLUGIN_IRQ] = {
		.name		= "usbin-plugin",
		.handler	= usb_plugin_irq_handler,
		.wake           = true,
	},
	[USBIN_REVI_CHANGE_IRQ] = {
		.name		= "usbin-revi-change",
	},
	[USBIN_SRC_CHANGE_IRQ] = {
		.name		= "usbin-src-change",
		.handler	= usb_source_change_irq_handler,
		.wake           = true,
	},
	[USBIN_ICL_CHANGE_IRQ] = {
		.name		= "usbin-icl-change",
		.handler	= icl_change_irq_handler,
		.wake           = true,
	},
	/* DC INPUT IRQs */
	[DCIN_VASHDN_IRQ] = {
		.name		= "dcin-vashdn",
	},
	[DCIN_UV_IRQ] = {
		.name		= "dcin-uv",
		.handler	= default_irq_handler,
	},
	[DCIN_OV_IRQ] = {
		.name		= "dcin-ov",
		.handler	= default_irq_handler,
	},
	[DCIN_PLUGIN_IRQ] = {
		.name		= "dcin-plugin",
		.handler	= dc_plugin_irq_handler,
		.wake           = true,
	},
	[DCIN_REVI_IRQ] = {
		.name		= "dcin-revi",
	},
	[DCIN_PON_IRQ] = {
		.name		= "dcin-pon",
		.handler	= default_irq_handler,
	},
	[DCIN_EN_IRQ] = {
		.name		= "dcin-en",
		.handler	= default_irq_handler,
	},
	/* TYPEC IRQs */
	[TYPEC_OR_RID_DETECTION_CHANGE_IRQ] = {
		.name		= "typec-or-rid-detect-change",
		.handler	= typec_or_rid_detection_change_irq_handler,
		.wake           = true,
	},
	[TYPEC_VPD_DETECT_IRQ] = {
		.name		= "typec-vpd-detect",
	},
	[TYPEC_CC_STATE_CHANGE_IRQ] = {
		.name		= "typec-cc-state-change",
		.handler	= typec_state_change_irq_handler,
		.wake           = true,
	},
	[TYPEC_VCONN_OC_IRQ] = {
		.name		= "typec-vconn-oc",
		.handler	= default_irq_handler,
	},
	[TYPEC_VBUS_CHANGE_IRQ] = {
		.name		= "typec-vbus-change",
	},
	[TYPEC_ATTACH_DETACH_IRQ] = {
		.name		= "typec-attach-detach",
		.handler	= typec_attach_detach_irq_handler,
		.wake		= true,
	},
	[TYPEC_LEGACY_CABLE_DETECT_IRQ] = {
		.name		= "typec-legacy-cable-detect",
		.handler	= default_irq_handler,
	},
	[TYPEC_TRY_SNK_SRC_DETECT_IRQ] = {
		.name		= "typec-try-snk-src-detect",
	},
	/* MISCELLANEOUS IRQs */
	[WDOG_SNARL_IRQ] = {
		.name		= "wdog-snarl",
		.handler	= wdog_snarl_irq_handler,
		.wake		= true,
	},
	[WDOG_BARK_IRQ] = {
		.name		= "wdog-bark",
		.handler	= wdog_bark_irq_handler,
		.wake		= true,
	},
	[AICL_FAIL_IRQ] = {
		.name		= "aicl-fail",
	},
	[AICL_DONE_IRQ] = {
		.name		= "aicl-done",
		.handler	= default_irq_handler,
	},
	[SMB_EN_IRQ] = {
		.name		= "smb-en",
	},
	[IMP_TRIGGER_IRQ] = {
		.name		= "imp-trigger",
	},
	/*
	 * triggered when DIE or SKIN or CONNECTOR temperature across
	 * either of the _REG_L, _REG_H, _RST, or _SHDN thresholds
	 */
	[TEMP_CHANGE_IRQ] = {
		.name		= "temp-change",
		.handler	= temp_change_irq_handler,
		.wake		= true,
	},
	[TEMP_CHANGE_SMB_IRQ] = {
		.name		= "temp-change-smb",
	},
	/* FLASH */
	[VREG_OK_IRQ] = {
		.name		= "vreg-ok",
	},
	[ILIM_S2_IRQ] = {
		.name		= "ilim2-s2",
		.handler	= schgm_flash_ilim2_irq_handler,
	},
	[ILIM_S1_IRQ] = {
		.name		= "ilim1-s1",
	},
	[VOUT_DOWN_IRQ] = {
		.name		= "vout-down",
	},
	[VOUT_UP_IRQ] = {
		.name		= "vout-up",
	},
	[FLASH_STATE_CHANGE_IRQ] = {
		.name		= "flash-state-change",
		.handler	= schgm_flash_state_change_irq_handler,
	},
	[TORCH_REQ_IRQ] = {
		.name		= "torch-req",
	},
	[FLASH_EN_IRQ] = {
		.name		= "flash-en",
	},
};

static int smb5_get_irq_index_byname(const char *irq_name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(smb5_irqs); i++) {
		if (strcmp(smb5_irqs[i].name, irq_name) == 0)
			return i;
	}

	return -ENOENT;
}

static int smb5_request_interrupt(struct smb5 *chip,
				struct device_node *node, const char *irq_name)
{
	struct smb_charger *chg = &chip->chg;
	int rc, irq, irq_index;
	struct smb_irq_data *irq_data;

	irq = of_irq_get_byname(node, irq_name);
	if (irq < 0) {
		pr_err("Couldn't get irq %s byname\n", irq_name);
		return irq;
	}

	irq_index = smb5_get_irq_index_byname(irq_name);
	if (irq_index < 0) {
		pr_err("%s is not a defined irq\n", irq_name);
		return irq_index;
	}

	if (!smb5_irqs[irq_index].handler)
		return 0;

	irq_data = devm_kzalloc(chg->dev, sizeof(*irq_data), GFP_KERNEL);
	if (!irq_data)
		return -ENOMEM;

	irq_data->parent_data = chip;
	irq_data->name = irq_name;
	irq_data->storm_data = smb5_irqs[irq_index].storm_data;
	mutex_init(&irq_data->storm_data.storm_lock);

	rc = devm_request_threaded_irq(chg->dev, irq, NULL,
					smb5_irqs[irq_index].handler,
					IRQF_ONESHOT, irq_name, irq_data);
	if (rc < 0) {
		pr_err("Couldn't request irq %d\n", irq);
		return rc;
	}

	smb5_irqs[irq_index].irq = irq;
	smb5_irqs[irq_index].irq_data = irq_data;
	if (smb5_irqs[irq_index].wake)
		enable_irq_wake(irq);

	return rc;
}

static int smb5_request_interrupts(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	struct device_node *node = chg->dev->of_node;
	struct device_node *child;
	int rc = 0;
	const char *name;
	struct property *prop;

	for_each_available_child_of_node(node, child) {
		of_property_for_each_string(child, "interrupt-names",
					    prop, name) {
			rc = smb5_request_interrupt(chip, child, name);
			if (rc < 0)
				return rc;
		}
	}
	if (chg->irq_info[USBIN_ICL_CHANGE_IRQ].irq)
		chg->usb_icl_change_irq_enabled = true;
	if (chg->irq_info[USBIN_ICL_CHANGE_IRQ].irq)
		chg->init_irq_done = true;

	/*
	 * WDOG_SNARL_IRQ is required for SW Thermal Regulation WA only. In
	 * case the WA is not required, disable the WDOG_SNARL_IRQ to prevent
	 * interrupt storm.
	 */

	if (chg->irq_info[WDOG_SNARL_IRQ].irq && !(chg->wa_flags &
						SW_THERM_REGULATION_WA)) {
		disable_irq_wake(chg->irq_info[WDOG_SNARL_IRQ].irq);
		disable_irq_nosync(chg->irq_info[WDOG_SNARL_IRQ].irq);
	}

	return rc;
}

static void smb5_free_interrupts(struct smb_charger *chg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(smb5_irqs); i++) {
		if (smb5_irqs[i].irq > 0) {
			if (smb5_irqs[i].wake)
				disable_irq_wake(smb5_irqs[i].irq);

			devm_free_irq(chg->dev, smb5_irqs[i].irq,
						smb5_irqs[i].irq_data);
		}
	}
}

static void smb5_disable_interrupts(struct smb_charger *chg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(smb5_irqs); i++) {
		if (smb5_irqs[i].irq > 0)
			disable_irq(smb5_irqs[i].irq);
	}
}

#if defined(CONFIG_DEBUG_FS)

static int force_batt_psy_update_write(void *data, u64 val)
{
	struct smb_charger *chg = data;

	power_supply_changed(chg->batt_psy);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(force_batt_psy_update_ops, NULL,
			force_batt_psy_update_write, "0x%02llx\n");

static int force_usb_psy_update_write(void *data, u64 val)
{
	struct smb_charger *chg = data;

	power_supply_changed(chg->usb_psy);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(force_usb_psy_update_ops, NULL,
			force_usb_psy_update_write, "0x%02llx\n");

static int force_dc_psy_update_write(void *data, u64 val)
{
	struct smb_charger *chg = data;

	power_supply_changed(chg->dc_psy);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(force_dc_psy_update_ops, NULL,
			force_dc_psy_update_write, "0x%02llx\n");

static void smb5_create_debugfs(struct smb5 *chip)
{
	struct dentry *file;

	chip->dfs_root = debugfs_create_dir("charger", NULL);
	if (IS_ERR_OR_NULL(chip->dfs_root)) {
		pr_err("Couldn't create charger debugfs rc=%ld\n",
			(long)chip->dfs_root);
		return;
	}

	file = debugfs_create_file("force_batt_psy_update", 0600,
			    chip->dfs_root, chip, &force_batt_psy_update_ops);
	if (IS_ERR_OR_NULL(file))
		pr_err("Couldn't create force_batt_psy_update file rc=%ld\n",
			(long)file);

	file = debugfs_create_file("force_usb_psy_update", 0600,
			    chip->dfs_root, chip, &force_usb_psy_update_ops);
	if (IS_ERR_OR_NULL(file))
		pr_err("Couldn't create force_usb_psy_update file rc=%ld\n",
			(long)file);

	file = debugfs_create_file("force_dc_psy_update", 0600,
			    chip->dfs_root, chip, &force_dc_psy_update_ops);
	if (IS_ERR_OR_NULL(file))
		pr_err("Couldn't create force_dc_psy_update file rc=%ld\n",
			(long)file);
}

#else

static void smb5_create_debugfs(struct smb5 *chip)
{}

#endif

#ifdef CONFIG_PROC_FS
static ssize_t write_ship_mode(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{

	if (count) {
		g_chip->ship_mode = true;
		pr_err(" * * * XCB * * * write_ship_mode\n");
	}
	return count;
}

static const struct file_operations proc_ship_mode_operations = {
	.write		= write_ship_mode,
	.llseek		= noop_llseek,
};
#endif

static int op_ship_mode_gpio_request(struct smb_charger *chip)
{
	int rc;

	chip->pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		dev_err(chip->dev,
				"Unable to acquire pinctrl\n");
		chip->pinctrl = NULL;
		return -EINVAL;
	}

	chip->ship_mode_default =
		pinctrl_lookup_state(chip->pinctrl, "op_ship_mode_default");
	if (IS_ERR_OR_NULL(chip->ship_mode_default)) {
		dev_err(chip->dev,
				"Can not lookup ship_mode_default\n");
		devm_pinctrl_put(chip->pinctrl);
		chip->pinctrl = NULL;
		return PTR_ERR(chip->ship_mode_default);
	}

	if (pinctrl_select_state(chip->pinctrl,
				chip->ship_mode_default) < 0)
		dev_err(chip->dev, "pinctrl set ship_mode_default fail\n");

	if (gpio_is_valid(chip->shipmode_en)) {
		rc = gpio_request(chip->shipmode_en, "stm6620_ctrl");
		if (rc) {
			pr_err("gpio_request failed for %d rc=%d\n",
				chip->shipmode_en, rc);
			return -EINVAL;
		}
		gpio_direction_output(chip->shipmode_en, 0);

		pr_info("ship_mode_gpio_request default mode success!\n");
	}

	return 0;

}

void requset_vbus_ctrl_gpio(struct smb_charger *chg)
{
	int ret;

	if (!gpio_is_valid(chg->vbus_ctrl))
		return;
	ret = gpio_request(chg->vbus_ctrl, "VbusCtrl");
	if (ret)
		pr_err("request failed,gpio:%d ret=%d\n", chg->vbus_ctrl, ret);
}

static int op_config_usb_temperature_adc(struct smb_charger *chip)
{
	if (IS_ERR_OR_NULL(chip->pinctrl)) {
		chip->pinctrl = devm_pinctrl_get(chip->dev);
		if (IS_ERR_OR_NULL(chip->pinctrl)) {
			dev_err(chip->dev,
					"Unable to acquire pinctrl\n");
			chip->pinctrl = NULL;
			return -EINVAL;
		}
	}

	chip->usb_temperature_default =
		pinctrl_lookup_state(chip->pinctrl, "op_usb_temp_adc_default");
	if (IS_ERR_OR_NULL(chip->usb_temperature_default)) {
		dev_err(chip->dev,
				"Can not lookup op_usb_temp_adc_default\n");
		devm_pinctrl_put(chip->pinctrl);
		chip->pinctrl = NULL;
		return PTR_ERR(chip->usb_temperature_default);
	}

	if (pinctrl_select_state(chip->pinctrl,
				chip->usb_temperature_default) < 0)
		dev_err(chip->dev, "pinctrl set op_usb_temp_adc_default fail\n");
	return 0;
}

/*usb connector hw auto detection*/
static irqreturn_t op_usb_plugin_irq_handler(int irq, void *dev_id)
{
	schedule_work(&g_chip->otg_switch_work);
	return IRQ_HANDLED;
}

static void request_plug_irq(struct smb_charger *chip)
{
	int ret;

	if (!gpio_is_valid(chip->plug_irq))
		return;
	ret = gpio_request(chip->plug_irq, "op_usb_plug");
	if (ret) {
		pr_err("request failed,gpio:%d ret=%d\n", chip->plug_irq, ret);
		return;
	}
	gpio_direction_input(chip->plug_irq);
	ret = request_irq(gpio_to_irq(chip->plug_irq),
			op_usb_plugin_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			"op_usb_plug", chip);
	if (ret < 0) {
		pr_err("request usb_plug irq failed.\n");
		return;
	}
	enable_irq_wake(gpio_to_irq(chip->plug_irq));
	pr_info("request usb_plug irq success\n");
	/*connect with usb cable when reboot, give a vote 1*/
	if (!gpio_get_value(chip->plug_irq)) {
		pr_info("%s:reboot time hw detect gpio low, vote 1\n",
			__func__);
		vote(chip->otg_toggle_votable, HW_DETECT_VOTER, 1, 0);
		chip->hw_detect = 1;
	}
}

static void check_factory_mode_disable_charge(struct smb_charger *chip)
{
	int boot_mode = get_boot_mode();
	int rc;

	if (boot_mode == MSM_BOOT_MODE__RF || boot_mode == MSM_BOOT_MODE__WLAN
			|| boot_mode == MSM_BOOT_MODE__FACTORY) {
		pr_info("RF/WLAN, suspending...\n");
		rc = vote(chip->usb_icl_votable, USER_VOTER,
				true, 0);
		rc = vote(chip->dc_suspend_votable, USER_VOTER,
				true, 0);
	}
}

static int lpd_enable_set(const char *val, const struct kernel_param *kp)
{
	struct smb_charger *chg = g_chg;
	unsigned long enable = 0;
	int ret = 0;

	ret = kstrtoul(val, 10, &enable);
	if (ret)
		return ret;

	pr_err("Enable:%d\n", enable);

	if (enable) {
		__lpd_enable = 1;
		/* lpd disabled by dts config*/
		if (chg->lpd_disabled)
			return 0;
		/* enable water detection irq*/
		ret = smblib_masked_write(chg, TYPE_C_INTERRUPT_EN_CFG_2_REG,
					TYPEC_WATER_DETECTION_INT_EN_BIT,
					TYPEC_WATER_DETECTION_INT_EN_BIT);
		if (ret < 0) {
			pr_err("Couldn't write TYPE_C_INTERRUPT_EN_CFG_2_REG rc=%d\n",
				ret);
			return ret;
		}
		pr_info("Enable water detection irq by user\n");
	} else {
		__lpd_enable = 0;
		__lpd_result = 0;
		/* disable water detection irq*/
		ret = smblib_masked_write(chg, TYPE_C_INTERRUPT_EN_CFG_2_REG,
					TYPEC_WATER_DETECTION_INT_EN_BIT,
					0);
		if (ret < 0) {
			pr_err("Couldn't write TYPE_C_INTERRUPT_EN_CFG_2_REG rc=%d\n",
				ret);
			return ret;
		}
		pr_info("Disable water detection irq by user\n");
	}

	return 0;
}

static int smb5_show_charger_status(struct smb5 *chip)
{
	struct smb_charger *chg = &chip->chg;
	union power_supply_propval val;
	int usb_present, batt_present, batt_health, batt_charge_type;
	int rc;

	rc = smblib_get_prop_usb_present(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get usb present rc=%d\n", rc);
		return rc;
	}
	usb_present = val.intval;

	rc = smblib_get_prop_batt_present(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt present rc=%d\n", rc);
		return rc;
	}
	batt_present = val.intval;

	rc = smblib_get_prop_batt_health(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt health rc=%d\n", rc);
		val.intval = POWER_SUPPLY_HEALTH_UNKNOWN;
	}
	batt_health = val.intval;

	rc = smblib_get_prop_batt_charge_type(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get batt charge type rc=%d\n", rc);
		return rc;
	}
	batt_charge_type = val.intval;

	pr_info("SMB5 status - usb:present=%d type=%d batt:present = %d health = %d charge = %d\n",
		usb_present, chg->real_charger_type,
		batt_present, batt_health, batt_charge_type);
	return rc;
}

static int smb5_probe(struct platform_device *pdev)
{
	struct smb5 *chip;
	struct smb_charger *chg;
	int rc = 0;
	struct msm_bus_scale_pdata *pdata;
	union power_supply_propval val;
	int usb_present;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chg = &chip->chg;
	g_chip = chg;
	chg->dev = &pdev->dev;
	chg->debug_mask = &__debug_mask;
	chg->pd_disabled = &__pd_disabled;
	chg->weak_chg_icl_ua = &__weak_chg_icl_ua;
	chg->lpd_enable = &__lpd_enable;
	chg->lpd_result = &__lpd_result;
	chg->mode = PARALLEL_MASTER;
	chg->irq_info = smb5_irqs;
	chg->die_health = -EINVAL;
	chg->connector_health = -EINVAL;
	chg->otg_present = false;
	chg->main_fcc_max = -EINVAL;

	chg->regmap = dev_get_regmap(chg->dev->parent, NULL);
	if (!chg->regmap) {
		pr_err("parent regmap is missing\n");
		return -EINVAL;
	}

	rc = smb5_chg_config_init(chip);
	if (rc < 0) {
		if (rc != -EPROBE_DEFER)
			pr_err("Couldn't setup chg_config rc=%d\n", rc);
		return rc;
	}

	rc = smb5_parse_dt(chip);
	if (rc < 0) {
		pr_err("Couldn't parse device tree rc=%d\n", rc);
		return rc;
	}

	if (alarmtimer_get_rtcdev())
		alarm_init(&chg->lpd_recheck_timer, ALARM_REALTIME,
				smblib_lpd_recheck_timer);
	else
		return -EPROBE_DEFER;

	rc = smblib_init(chg);
	if (rc < 0) {
		pr_err("Smblib_init failed rc=%d\n", rc);
		return rc;
	}

	/* set driver data before resources request it */
	platform_set_drvdata(pdev, chip);

	op_charge_info_init(chg);
	pdata = msm_bus_cl_get_pdata(pdev);
	if (!pdata)
		pr_err("GPIO** failed get_pdata client_id\n");
	else
		chg->bus_client = msm_bus_scale_register_client(pdata);

	rc = smb5_init_vbus_regulator(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize vbus regulator rc=%d\n",
			rc);
		goto cleanup;
	}

	rc = smb5_init_vconn_regulator(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize vconn regulator rc=%d\n",
				rc);
		goto cleanup;
	}

	/* extcon registration */
	chg->extcon = devm_extcon_dev_allocate(chg->dev, smblib_extcon_cable);
	if (IS_ERR(chg->extcon)) {
		rc = PTR_ERR(chg->extcon);
		dev_err(chg->dev, "failed to allocate extcon device rc=%d\n",
				rc);
		goto cleanup;
	}

	rc = devm_extcon_dev_register(chg->dev, chg->extcon);
	if (rc < 0) {
		dev_err(chg->dev, "failed to register extcon device rc=%d\n",
				rc);
		goto cleanup;
	}

	rc = smb5_init_hw(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize hardware rc=%d\n", rc);
		goto cleanup;
	}

	/*
	 * VBUS regulator enablement/disablement for host mode is handled
	 * by USB-PD driver only. For micro-USB and non-PD typeC designs,
	 * the VBUS regulator is enabled/disabled by the smb driver itself
	 * before sending extcon notifications.
	 * Hence, register vbus and vconn regulators for PD supported designs
	 * only.
	 */
	if (!chg->pd_not_supported) {
		rc = smb5_init_vbus_regulator(chip);
		if (rc < 0) {
			pr_err("Couldn't initialize vbus regulator rc=%d\n",
				rc);
			goto cleanup;
		}

		rc = smb5_init_vconn_regulator(chip);
		if (rc < 0) {
			pr_err("Couldn't initialize vconn regulator rc=%d\n",
				rc);
			goto cleanup;
		}
	}

	switch (chg->smb_version) {
	case PM8150B_SUBTYPE:
	case PM6150_SUBTYPE:
		rc = smb5_init_dc_psy(chip);
		if (rc < 0) {
			pr_err("Couldn't initialize dc psy rc=%d\n", rc);
			goto cleanup;
		}
		break;
	default:
		break;
	}

	rc = smb5_init_usb_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb5_init_usb_main_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb main psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb5_init_usb_port_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize usb pc_port psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb5_init_batt_psy(chip);
	if (rc < 0) {
		pr_err("Couldn't initialize batt psy rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb5_determine_initial_status(chip);
	if (rc < 0) {
		pr_err("Couldn't determine initial status rc=%d\n",
			rc);
		goto cleanup;
	}

	rc = smb5_request_interrupts(chip);
	if (rc < 0) {
		pr_err("Couldn't request interrupts rc=%d\n", rc);
		goto cleanup;
	}

	rc = smb5_post_init(chip);
	if (rc < 0) {
		pr_err("Failed in post init rc=%d\n", rc);
		goto free_irq;
	}

	smb5_create_debugfs(chip);

	rc = smb5_show_charger_status(chip);
	if (rc < 0) {
		pr_err("Failed in getting charger status rc=%d\n", rc);
		goto free_irq;
	}

#ifdef CONFIG_PROC_FS
	if (!proc_create("ship_mode", 0644, NULL, &proc_ship_mode_operations))
		pr_err("Failed to register proc interface\n");
#endif
	rc = smblib_get_prop_usb_present(chg, &val);
	if (rc < 0) {
		pr_err("Couldn't get usb present rc=%d\n", rc);
		goto cleanup;
	}
	usb_present = val.intval;

	if (usb_present) {
		schedule_delayed_work(&chg->non_standard_charger_check_work,
				msecs_to_jiffies(TIME_1000MS));
		chg->boot_usb_present = true;
	}
	if (!usb_present && chg->vbus_present)
		op_handle_usb_plugin(chg);

	device_init_wakeup(chg->dev, true);

	op_ship_mode_gpio_request(chg);
	requset_vbus_ctrl_gpio(chg);
	op_config_usb_temperature_adc(chg);
	request_plug_irq(chg);
	check_factory_mode_disable_charge(chg);

	pr_info("QPNP SMB5 probed successfully\n");
	return rc;

free_irq:
	smb5_free_interrupts(chg);
cleanup:
	smblib_deinit(chg);
	platform_set_drvdata(pdev, NULL);

	return rc;
}

static int smb5_remove(struct platform_device *pdev)
{
	struct smb5 *chip = platform_get_drvdata(pdev);
	struct smb_charger *chg = &chip->chg;

	/* force enable APSD */
	smblib_masked_write(chg, USBIN_OPTIONS_1_CFG_REG,
				BC1P2_SRC_DETECT_BIT, BC1P2_SRC_DETECT_BIT);

	smb5_free_interrupts(chg);
	smblib_deinit(chg);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static void stm6620_enter_ship_mode(struct smb_charger *chg)
{
	int i;

	for (i = 0; i < 5; i++) {
		gpio_set_value(chg->shipmode_en, 1);
		usleep_range(4000, 4001);
		gpio_set_value(chg->shipmode_en, 0);
		usleep_range(4000, 4001);
	}
}

static void smb5_shutdown(struct platform_device *pdev)
{
	struct smb5 *chip = platform_get_drvdata(pdev);
	struct smb_charger *chg = &chip->chg;

#ifdef CONFIG_PROC_FS
	pr_info("smbchg_shutdown\n");

	if (chg->ship_mode) {
		pr_info("smbchg_shutdown enter ship_mode\n");
		if (gpio_is_valid(chg->shipmode_en)) {
			vote(chg->usb_icl_votable,
					DEFAULT_VOTER, true, 0);
			stm6620_enter_ship_mode(chg);
		} else {
			smblib_masked_write(chg, SHIP_MODE_REG,
			SHIP_MODE_EN_BIT, SHIP_MODE_EN_BIT);
		}
		clean_backup_soc_ex();
		msleep(1000);
		pr_err("after 1s\n");
		while (1)
			;
	}
#endif

	/* disable all interrupts */
	smb5_disable_interrupts(chg);

	/* configure power role for UFP */
	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_TYPEC)
		smblib_masked_write(chg, TYPE_C_MODE_CFG_REG,
				TYPEC_POWER_ROLE_CMD_MASK, EN_SNK_ONLY_BIT);

	/* force HVDCP to 5V */
	smblib_masked_write(chg, USBIN_OPTIONS_1_CFG_REG,
				HVDCP_AUTONOMOUS_MODE_EN_CFG_BIT, 0);
	smblib_write(chg, CMD_HVDCP_2_REG, FORCE_5V_BIT);

	/* force enable and rerun APSD */
	smblib_apsd_enable(chg, true);
	smblib_masked_write(chg, CMD_APSD_REG, APSD_RERUN_BIT, APSD_RERUN_BIT);
}

static const struct of_device_id match_table[] = {
	{ .compatible = "qcom,qpnp-smb5", },
	{ },
};

static struct platform_driver smb5_driver = {
	.driver		= {
		.name		= "qcom,qpnp-smb5",
		.owner		= THIS_MODULE,
		.of_match_table	= match_table,
	},
	.probe		= smb5_probe,
	.remove		= smb5_remove,
	.shutdown	= smb5_shutdown,
};
module_platform_driver(smb5_driver);

MODULE_DESCRIPTION("QPNP SMB5 Charger Driver");
MODULE_LICENSE("GPL v2");
