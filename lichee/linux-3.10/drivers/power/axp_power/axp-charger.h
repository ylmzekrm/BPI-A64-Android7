#ifndef AXP_CHARGER_H
#define AXP_CHARGER_H

#include <linux/power_supply.h>

#define BATRDC          100
#define INTCHGCUR       300000      /* set initial charging current limite */
#define SUSCHGCUR       1000000     /* set suspend charging current limite */
#define RESCHGCUR       INTCHGCUR   /* set resume charging current limite */
#define CLSCHGCUR       SUSCHGCUR   /* set shutdown charging current limite */
#define INTCHGVOL       4200000     /* set initial charing target voltage */
#define INTCHGENDRATE   10          /* set initial charing end current rate */
#define INTCHGENABLED   1           /* set initial charing enabled */
#define INTADCFREQ      25          /* set initial adc frequency */
#define INTADCFREQC     100         /* set initial coulomb adc coufrequency */
#define INTCHGPRETIME   50          /* set initial pre-charging time */
#define INTCHGCSTTIME   480         /* set initial pre-charging time */
#define BATMAXVOL       4200000     /* set battery max design volatge */
#define BATMINVOL       3500000     /* set battery min design volatge */
#define UPDATEMINTIME   30          /* set bat percent update min time */

#define OCVREG0         0x00        /* 2.99V */
#define OCVREG1         0x00        /* 3.13V */
#define OCVREG2         0x00        /* 3.27V */
#define OCVREG3         0x00        /* 3.34V */
#define OCVREG4         0x00        /* 3.41V */
#define OCVREG5         0x00        /* 3.48V */
#define OCVREG6         0x00        /* 3.52V */
#define OCVREG7         0x00        /* 3.55V */
#define OCVREG8         0x04        /* 3.57V */
#define OCVREG9         0x05        /* 3.59V */
#define OCVREGA         0x06        /* 3.61V */
#define OCVREGB         0x07        /* 3.63V */
#define OCVREGC         0x0a        /* 3.64V */
#define OCVREGD         0x0d        /* 3.66V */
#define OCVREGE         0x1a        /* 3.70V */
#define OCVREGF         0x24        /* 3.73V */
#define OCVREG10        0x29        /* 3.77V */
#define OCVREG11        0x2e        /* 3.78V */
#define OCVREG12        0x32        /* 3.80V */
#define OCVREG13        0x35        /* 3.84V */
#define OCVREG14        0x39        /* 3.85V */
#define OCVREG15        0x3d        /* 3.87V */
#define OCVREG16        0x43        /* 3.91V */
#define OCVREG17        0x49        /* 3.94V */
#define OCVREG18        0x4f        /* 3.98V */
#define OCVREG19        0x54        /* 4.01V */
#define OCVREG1A        0x58        /* 4.05V */
#define OCVREG1B        0x5c        /* 4.08V */
#define OCVREG1C        0x5e        /* 4.10V */
#define OCVREG1D        0x60        /* 4.12V */
#define OCVREG1E        0x62        /* 4.14V */
#define OCVREG1F        0x64        /* 4.15V */

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

struct axp_charger_dev;

enum AW_CHARGE_TYPE {
	CHARGE_AC,
	CHARGE_USB_20,
	CHARGE_USB_30,
	CHARGE_MAX
};

struct axp_config_info {
	u32 pmu_used;
	u32 pmu_id;
	u32 pmu_battery_rdc;
	u32 pmu_battery_cap;
	u32 pmu_batdeten;
	u32 pmu_chg_ic_temp;
	u32 pmu_runtime_chgcur;
	u32 pmu_suspend_chgcur;
	u32 pmu_shutdown_chgcur;
	u32 pmu_init_chgvol;
	u32 pmu_init_chgend_rate;
	u32 pmu_init_chg_enabled;
	u32 pmu_init_bc_en;
	u32 pmu_init_adc_freq;
	u32 pmu_init_adcts_freq;
	u32 pmu_init_chg_pretime;
	u32 pmu_init_chg_csttime;
	u32 pmu_batt_cap_correct;
	u32 pmu_chg_end_on_en;
	u32 ocv_coulumb_100;

	u32 pmu_bat_para1;
	u32 pmu_bat_para2;
	u32 pmu_bat_para3;
	u32 pmu_bat_para4;
	u32 pmu_bat_para5;
	u32 pmu_bat_para6;
	u32 pmu_bat_para7;
	u32 pmu_bat_para8;
	u32 pmu_bat_para9;
	u32 pmu_bat_para10;
	u32 pmu_bat_para11;
	u32 pmu_bat_para12;
	u32 pmu_bat_para13;
	u32 pmu_bat_para14;
	u32 pmu_bat_para15;
	u32 pmu_bat_para16;
	u32 pmu_bat_para17;
	u32 pmu_bat_para18;
	u32 pmu_bat_para19;
	u32 pmu_bat_para20;
	u32 pmu_bat_para21;
	u32 pmu_bat_para22;
	u32 pmu_bat_para23;
	u32 pmu_bat_para24;
	u32 pmu_bat_para25;
	u32 pmu_bat_para26;
	u32 pmu_bat_para27;
	u32 pmu_bat_para28;
	u32 pmu_bat_para29;
	u32 pmu_bat_para30;
	u32 pmu_bat_para31;
	u32 pmu_bat_para32;

	u32 pmu_ac_vol;
	u32 pmu_ac_cur;
	u32 pmu_usbpc_vol;
	u32 pmu_usbpc_cur;
	u32 pmu_pwroff_vol;
	u32 pmu_pwron_vol;
	u32 pmu_powkey_off_time;
	u32 pmu_powkey_off_en;
	u32 pmu_powkey_off_delay_time;
	u32 pmu_powkey_off_func;
	u32 pmu_powkey_long_time;
	u32 pmu_powkey_on_time;
	u32 pmu_pwrok_time;
	u32 pmu_pwrnoe_time;
	u32 pmu_reset_shutdown_en;
	u32 pmu_battery_warning_level1;
	u32 pmu_battery_warning_level2;
	u32 pmu_restvol_adjust_time;
	u32 pmu_ocv_cou_adjust_time;
	u32 pmu_chgled_func;
	u32 pmu_chgled_type;
	u32 pmu_vbusen_func;
	u32 pmu_reset;
	u32 pmu_irq_wakeup;
	u32 pmu_hot_shutdown;
	u32 pmu_inshort;
	u32 power_start;
	u32 pmu_as_slave;
	u32 pmu_bat_unused;
	u32 pmu_ocv_en;
	u32 pmu_cou_en;
	u32 pmu_update_min_time;

	u32 pmu_bat_temp_enable;
	u32 pmu_bat_charge_ltf;
	u32 pmu_bat_charge_htf;
	u32 pmu_bat_shutdown_ltf;
	u32 pmu_bat_shutdown_htf;
	u32 pmu_bat_temp_para1;
	u32 pmu_bat_temp_para2;
	u32 pmu_bat_temp_para3;
	u32 pmu_bat_temp_para4;
	u32 pmu_bat_temp_para5;
	u32 pmu_bat_temp_para6;
	u32 pmu_bat_temp_para7;
	u32 pmu_bat_temp_para8;
	u32 pmu_bat_temp_para9;
	u32 pmu_bat_temp_para10;
	u32 pmu_bat_temp_para11;
	u32 pmu_bat_temp_para12;
	u32 pmu_bat_temp_para13;
	u32 pmu_bat_temp_para14;
	u32 pmu_bat_temp_para15;
	u32 pmu_bat_temp_para16;
};

struct axp_ac_info {
	int det_bit;    /* ac detect */
	int det_offset;
	int valid_bit;  /* ac vali */
	int valid_offset;
	int in_short_bit;
	int in_short_offset;
	int ac_vol;
	int ac_cur;
	int (*get_ac_voltage)(struct axp_charger_dev *cdev);
	int (*get_ac_current)(struct axp_charger_dev *cdev);
	int (*set_ac_vhold)(struct axp_charger_dev *cdev, int vol);
	int (*get_ac_vhold)(struct axp_charger_dev *cdev);
	int (*set_ac_ihold)(struct axp_charger_dev *cdev, int cur);
	int (*get_ac_ihold)(struct axp_charger_dev *cdev);
};

struct axp_usb_info {
	int det_bit;
	int det_offset;
	int valid_bit;
	int valid_offset;
	int det_unused;
	int usb_pc_vol;
	int usb_pc_cur;
	int usb_ad_vol;
	int usb_ad_cur;
	int (*get_usb_voltage)(struct axp_charger_dev *cdev);
	int (*get_usb_current)(struct axp_charger_dev *cdev);
	int (*set_usb_vhold)(struct axp_charger_dev *cdev, int vol);
	int (*get_usb_vhold)(struct axp_charger_dev *cdev);
	int (*set_usb_ihold)(struct axp_charger_dev *cdev, int cur);
	int (*get_usb_ihold)(struct axp_charger_dev *cdev);
};

struct axp_battery_info {
	int chgstat_bit;
	int chgstat_offset;
	int det_bit;
	int det_offset;
	int det_valid_bit;
	int det_valid;
	int det_unused;
	int cur_direction_bit;
	int cur_direction_offset;
	int polling_delay;
	int runtime_chgcur;
	int suspend_chgcur;
	int shutdown_chgcur;
	int (*get_rest_cap)(struct axp_charger_dev *cdev);
	int (*get_bat_health)(struct axp_charger_dev *cdev);
	int (*get_vbat)(struct axp_charger_dev *cdev);
	int (*get_ibat)(struct axp_charger_dev *cdev);
	int (*get_disibat)(struct axp_charger_dev *cdev);
	int (*set_chg_cur)(struct axp_charger_dev *cdev, int cur);
	int (*set_chg_vol)(struct axp_charger_dev *cdev, int vol);
	int (*pre_time_set)(struct axp_charger_dev *cdev, int min);
	int (*pos_time_set)(struct axp_charger_dev *cdev, int min);
};

struct axp_supply_info {
	struct axp_ac_info *ac;
	struct axp_usb_info *usb;
	struct axp_battery_info *batt;
};

struct axp_charger_dev {
	struct power_supply batt;
	struct power_supply ac;
	struct power_supply usb;
	struct power_supply_info *battery_info;
	struct axp_supply_info *spy_info;
	struct device *dev;
	struct axp_dev *chip;
	struct timer_list usb_status_timer;
	struct delayed_work work;
	struct delayed_work usbwork;
	unsigned int interval;
	spinlock_t charger_lock;

	int rest_vol;
	int usb_vol;
	int usb_cur;
	int ac_vol;
	int ac_cur;
	int bat_vol;
	int bat_cur;
	int bat_discur;

	bool bat_det;
	bool ac_det;
	bool usb_det;
	bool ac_valid;
	bool usb_valid;
	bool ext_valid;
	bool in_short;
	bool charging;
	bool ac_charging;
	bool usb_pc_charging;
	bool usb_adapter_charging;
	bool bat_current_direction;

	void (*private_debug)(struct axp_charger_dev *cdev);
};

struct axp_adc_res {
	uint16_t vbat_res;
	uint16_t ocvbat_res;
	uint16_t ibat_res;
	uint16_t ichar_res;
	uint16_t idischar_res;
	uint16_t vac_res;
	uint16_t iac_res;
	uint16_t vusb_res;
	uint16_t iusb_res;
	uint16_t ts_res;
};

struct axp_charger_dev *axp_power_supply_register(struct device *dev,
					struct axp_dev *axp_dev,
					struct power_supply_info *battery_info,
					struct axp_supply_info *info);
void axp_power_supply_unregister(struct axp_charger_dev *chg_dev);
void axp_change(struct axp_charger_dev *chg_dev);
void axp_usbac_in(struct axp_charger_dev *chg_dev);
void axp_usbac_out(struct axp_charger_dev *chg_dev);
void axp_capchange(struct axp_charger_dev *chg_dev);
void axp_charger_suspend(struct axp_charger_dev *chg_dev);
void axp_charger_resume(struct axp_charger_dev *chg_dev);
void axp_charger_shutdown(struct axp_charger_dev *chg_dev);
int axp_charger_dt_parse(struct device_node *node,
					struct axp_config_info *axp_config);

extern int axp_debug;
#endif /* AXP_ChARGER_H */
