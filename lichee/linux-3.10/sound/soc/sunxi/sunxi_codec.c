/*
 * sound\soc\sunxi\sunxi_sun50iw1codec.c
 * (C) Copyright 2014-2017
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 * Liu shaohua <liushaohua@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>
#include "sunxi_codec.h"
#include "sunxi_rw_func.h"

//#define AIF1_FPGA_LOOPBACK_TEST
#define codec_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define codec_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
#define DRV_NAME "sunxi-internal-codec"

void __iomem *codec_digitaladress;
void __iomem *codec_analogadress;
struct spk_gpio_ spk_gpio;

static bool src_function_en = false;

static const DECLARE_TLV_DB_SCALE(headphone_vol_tlv, -6300, 100, 0);
static const DECLARE_TLV_DB_SCALE(lineout_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(speaker_vol_tlv, -4800, 150, 0);
static const DECLARE_TLV_DB_SCALE(earpiece_vol_tlv, -4350, 150, 0);

static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(aif2_ad_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_da_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_ad_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(dig_vol_tlv, -7308, 116, 0);

static const DECLARE_TLV_DB_SCALE(mic1_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic1_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(mic2_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic2_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_input_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(phoneout_vol_tlv, -450, 150, 0);

struct aif1_fs {
	unsigned int samplerate;
	int aif1_bclk_div;
	int aif1_srbit;
};

struct aif1_lrck {
	int aif1_lrlk_div;
	int aif1_lrlk_bit;
};

struct aif1_word_size {
	int aif1_wsize_val;
	int aif1_wsize_bit;
};

static const struct aif1_fs codec_aif1_fs[] = {
	{44100, 4, 7},
	{48000, 4, 8},
	{8000, 9, 0},
	{11025, 8, 1},
	{12000, 8, 2},
	{16000, 7, 3},
	{22050, 6, 4},
	{24000, 6, 5},
	{32000, 5, 6},
	{96000, 2, 9},
	{192000, 1, 10},
};

static const struct aif1_lrck codec_aif1_lrck[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

static const struct aif1_word_size codec_aif1_wsize[] = {
	{8, 0},
	{16, 1},
	{20, 2},
	{24, 3},
};

static struct label reg_labels[]={
    LABEL(SUNXI_DA_CTL),
    LABEL(SUNXI_DA_FAT0),
    LABEL(SUNXI_DA_FAT1),
    LABEL(SUNXI_DA_FCTL),
    LABEL(SUNXI_DA_INT),
    LABEL(SUNXI_DA_TXFIFO),
    LABEL(SUNXI_DA_CLKD),
    LABEL(SUNXI_DA_TXCNT),
    LABEL(SUNXI_DA_RXCNT),
	LABEL(SUNXI_SYSCLK_CTL),
	LABEL(SUNXI_MOD_CLK_ENA),
	LABEL(SUNXI_MOD_RST_CTL),
	LABEL(SUNXI_SYS_SR_CTRL),
	LABEL(SUNXI_SYS_SRC_CLK),
	LABEL(SUNXI_SYS_DVC_MOD),

	LABEL(SUNXI_AIF1_CLK_CTRL),
	LABEL(SUNXI_AIF1_ADCDAT_CTRL),
	LABEL(SUNXI_AIF1_DACDAT_CTRL),
	LABEL(SUNXI_AIF1_MXR_SRC),
	LABEL(SUNXI_AIF1_VOL_CTRL1),
	LABEL(SUNXI_AIF1_VOL_CTRL2),
	LABEL(SUNXI_AIF1_VOL_CTRL3),
	LABEL(SUNXI_AIF1_VOL_CTRL4),
	LABEL(SUNXI_AIF1_MXR_GAIN),
	LABEL(SUNXI_AIF1_RXD_CTRL),
	LABEL(SUNXI_AIF2_CLK_CTRL),
	LABEL(SUNXI_AIF2_ADCDAT_CTRL),
	LABEL(SUNXI_AIF2_DACDAT_CTRL),
	LABEL(SUNXI_AIF2_MXR_SRC),
	LABEL(SUNXI_AIF2_VOL_CTRL1),
	LABEL(SUNXI_AIF2_VOL_CTRL2),
	LABEL(SUNXI_AIF2_MXR_GAIN),
	LABEL(SUNXI_AIF2_RXD_CTRL),
	LABEL(SUNXI_AIF3_CLK_CTRL),
	LABEL(SUNXI_AIF3_ADCDAT_CTRL),
	LABEL(SUNXI_AIF3_DACDAT_CTRL),
	LABEL(SUNXI_AIF3_SGP_CTRL),
	LABEL(SUNXI_AIF3_RXD_CTRL),
	LABEL(SUNXI_ADC_DIG_CTRL),
	LABEL(SUNXI_ADC_VOL_CTRL),
	LABEL(SUNXI_ADC_DBG_CTRL),
	LABEL(SUNXI_HMIC_CTRL1),
	LABEL(SUNXI_HMIC_CTRL2),
	LABEL(SUNXI_HMIC_STS),
	LABEL(SUNXI_DAC_DIG_CTRL),
	LABEL(SUNXI_DAC_VOL_CTRL),
	LABEL(SUNXI_DAC_DBG_CTRL),
	LABEL(SUNXI_DAC_MXR_SRC),
	LABEL(SUNXI_DAC_MXR_GAIN),
	LABEL(SUNXI_AGC_ENA),
	LABEL(SUNXI_DRC_ENA),

    LABEL(HP_CTRL),
    LABEL(OL_MIX_CTRL),
    LABEL(OR_MIX_CTRL),
    LABEL(EARPIECE_CTRL0),
    LABEL(EARPIECE_CTRL1),
	LABEL(SPKOUT_CTRL0),
    LABEL(SPKOUT_CTRL1),
    LABEL(MIC1_CTRL),
    LABEL(MIC2_CTRL),
    LABEL(LINEIN_CTRL),
    LABEL(MIX_DAC_CTRL),
    LABEL(L_ADCMIX_SRC),
    LABEL(R_ADCMIX_SRC),
    LABEL(ADC_CTRL),
	LABEL(HS_MBIAS_CTRL),
	LABEL(APT_REG),
	LABEL(OP_BIAS_CTRL0),
	LABEL(OP_BIAS_CTRL1),
	LABEL(ZC_VOL_CTRL),
	LABEL(BIAS_CAL_DATA),
	LABEL(BIAS_CAL_SET),
	LABEL(BD_CAL_CTRL),
	LABEL(HP_PA_CTRL),
	LABEL(RHP_CAL_DAT),
	LABEL(RHP_CAL_SET),
	LABEL(LHP_CAL_DAT),
	LABEL(LHP_CAL_SET),
	LABEL(MDET_CTRL),
	LABEL(JACK_MIC_CTRL),
    LABEL_END,
};

static void adcagc_config(struct snd_soc_codec *codec)
{
}

static void adcdrc_config(struct snd_soc_codec *codec)
{
}

static void adchpf_config(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, SUNXI_AC_DAPHHPFC, (0x7ff<<HPF_H_COEFFICIENT_SET), (0xff << HPF_H_COEFFICIENT_SET));
	snd_soc_update_bits(codec, SUNXI_AC_DAPLHPFC, (0xffff<<HPF_L_COEFFICIENT_SET), (0xfac1 << HPF_L_COEFFICIENT_SET));
}

static void dacdrc_config(struct snd_soc_codec *codec)
{
}

static void dachpf_config(struct snd_soc_codec *codec)
{

}

static void adcdrc_enable(struct snd_soc_codec *codec, bool on)
{
	if (on) {
	} else {
	}
}

static void dacdrc_enable(struct snd_soc_codec *codec, bool on)
{
	if (on) {
	} else {
	}
}

static void adcagc_enable(struct snd_soc_codec *codec, bool on)
{
	if (on) {
	} else {
	}
}

static void dachpf_enable(struct snd_soc_codec *codec, bool on)
{
	if (on) {
	} else {
	}
}

static void adchpf_enable(struct snd_soc_codec *codec, bool on)
{
	if (on) {
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<HPF_AGC_MOD_CLK_EN), (0x01 << HPF_AGC_MOD_CLK_EN));
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<HPF_AGC_MOD_RST_CTL), (0x01 << HPF_AGC_MOD_RST_CTL));
		snd_soc_update_bits(codec, SUNXI_AC_ADC_DAPLCTRL, (0x1<<LEFT_HPF_EN), (0x01 << LEFT_HPF_EN));
		snd_soc_update_bits(codec, SUNXI_AC_ADC_DAPRCTRL, (0x1<<RIGHT_HPF_EN), (0x01 << RIGHT_HPF_EN));
		snd_soc_update_bits(codec, SUNXI_AGC_ENA, (0x1<<ADCL_AGC_ENA), (0x01 << ADCL_AGC_ENA));
		snd_soc_update_bits(codec, SUNXI_AGC_ENA, (0x1<<ADCR_AGC_ENA), (0x01 << ADCR_AGC_ENA));
	} else {
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<HPF_AGC_MOD_CLK_EN), (0x0 << HPF_AGC_MOD_CLK_EN));
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<HPF_AGC_MOD_RST_CTL), (0x0 << HPF_AGC_MOD_RST_CTL));
		snd_soc_update_bits(codec, SUNXI_AC_ADC_DAPLCTRL, (0x1<<LEFT_HPF_EN), (0x01 << LEFT_HPF_EN));
		snd_soc_update_bits(codec, SUNXI_AC_ADC_DAPRCTRL, (0x1<<RIGHT_HPF_EN), (0x00 << RIGHT_HPF_EN));
		snd_soc_update_bits(codec, SUNXI_AGC_ENA, (0x1<<ADCL_AGC_ENA), (0x00 << ADCL_AGC_ENA));
		snd_soc_update_bits(codec, SUNXI_AGC_ENA, (0x1<<ADCR_AGC_ENA), (0x00 << ADCR_AGC_ENA));
	}
}

/*
*enable the codec function which should be enable during system init.
*/
static int codec_init(struct sunxi_codec *sunxi_internal_codec)
{
	int ret = 0;

	snd_soc_write(sunxi_internal_codec->codec, HP_CAL_CTRL, 0x87);
	snd_soc_update_bits(sunxi_internal_codec->codec, SPKOUT_CTRL1, (0x1f<<SPKOUT_VOL), (sunxi_internal_codec->gain_config.spkervol<<SPKOUT_VOL));
	snd_soc_update_bits(sunxi_internal_codec->codec, HP_CTRL, (0x3f<<HPVOL), (sunxi_internal_codec->gain_config.headphonevol<<HPVOL));
	snd_soc_update_bits(sunxi_internal_codec->codec, EARPIECE_CTRL1, (0x1f<<ESP_VOL), (sunxi_internal_codec->gain_config.earpiecevol<<ESP_VOL));
	snd_soc_update_bits(sunxi_internal_codec->codec, MIC1_CTRL, (0x7<<MIC1BOOST), (sunxi_internal_codec->gain_config.maingain<<MIC1BOOST));
	snd_soc_update_bits(sunxi_internal_codec->codec, MIC2_CTRL, (0x7<<MIC2BOOST), (sunxi_internal_codec->gain_config.headsetmicgain<<MIC2BOOST));
	snd_soc_update_bits(sunxi_internal_codec->codec, MIX_DAC_CTRL, (0x1<<DACALEN), (1<<DACALEN));
	snd_soc_update_bits(sunxi_internal_codec->codec, MIX_DAC_CTRL, (0x1<<DACAREN), (1<<DACAREN));

	if (sunxi_internal_codec->hwconfig.adcagc_cfg)
		adcagc_config(sunxi_internal_codec->codec);

	if (sunxi_internal_codec->hwconfig.adcdrc_cfg)
		adcdrc_config(sunxi_internal_codec->codec);

	if (sunxi_internal_codec->hwconfig.adchpf_cfg)
		adchpf_config(sunxi_internal_codec->codec);

	if (sunxi_internal_codec->hwconfig.dacdrc_cfg)
		dacdrc_config(sunxi_internal_codec->codec);

	if (sunxi_internal_codec->hwconfig.dachpf_cfg)
		dachpf_config(sunxi_internal_codec->codec);
	if (sunxi_internal_codec->aif_config.aif2config || sunxi_internal_codec->aif_config.aif3config) {
		if (!sunxi_internal_codec->pinctrl) {
			sunxi_internal_codec->pinctrl = devm_pinctrl_get(sunxi_internal_codec->codec->dev);
			if (IS_ERR_OR_NULL(sunxi_internal_codec->pinctrl)) {
				pr_warn("[audio-codec]request pinctrl handle for audio failed\n");
				return -EINVAL;
			}
		}
	}

	if (sunxi_internal_codec->aif_config.aif2config) {
		if (!sunxi_internal_codec->aif2_pinstate){
			sunxi_internal_codec->aif2_pinstate = pinctrl_lookup_state(sunxi_internal_codec->pinctrl, "aif2-default");
			if (IS_ERR_OR_NULL(sunxi_internal_codec->aif2_pinstate)) {
				pr_warn("[audio-codec]lookup aif2-default state failed\n");
				return -EINVAL;
			}
		}

		if (!sunxi_internal_codec->aif2sleep_pinstate){
			sunxi_internal_codec->aif2sleep_pinstate = pinctrl_lookup_state(sunxi_internal_codec->pinctrl, "aif2-sleep");
			if (IS_ERR_OR_NULL(sunxi_internal_codec->aif2sleep_pinstate)) {
				pr_warn("[audio-codec]lookup aif2-sleep state failed\n");
				return -EINVAL;
			}
		}
		ret = pinctrl_select_state(sunxi_internal_codec->pinctrl, sunxi_internal_codec->aif2_pinstate);
		if (ret) {
			pr_warn("[audio-codec]select aif2-default state failed\n");
			return ret;
		}
	}
	if (sunxi_internal_codec->aif_config.aif3config) {
		if (!sunxi_internal_codec->aif3_pinstate){
			sunxi_internal_codec->aif3_pinstate = pinctrl_lookup_state(sunxi_internal_codec->pinctrl, "aif3-default");
			if (IS_ERR_OR_NULL(sunxi_internal_codec->aif3_pinstate)) {
				pr_warn("[audio-codec]lookup aif3-default state failed\n");
				return -EINVAL;
			}
		}

		if (!sunxi_internal_codec->aif3sleep_pinstate){
			sunxi_internal_codec->aif3sleep_pinstate = pinctrl_lookup_state(sunxi_internal_codec->pinctrl, "aif3-sleep");
			if (IS_ERR_OR_NULL(sunxi_internal_codec->aif3sleep_pinstate)) {
				pr_warn("[audio-codec]lookup aif3-sleep state failed\n");
				return -EINVAL;
			}
		}

		ret = pinctrl_select_state(sunxi_internal_codec->pinctrl, sunxi_internal_codec->aif3_pinstate);
		if (ret) {
			pr_warn("[audio-codec]select aif3-default state failed\n");
			return ret;
		}
	}

	return ret;
}

int ac_aif1clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->aifclk_mutex);
	pr_debug("aif1 interface clk power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->aif1_clken == 0) {
			/*enable AIF1CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF1CLK_ENA), (0x1<<AIF1CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF1_MOD_CLK_EN), (0x1<<AIF1_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF1_MOD_RST_CTL), (0x1<<AIF1_MOD_RST_CTL));
			/*enable systemclk*/
			if (sunxi_internal_codec->aif2_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sunxi_internal_codec->aif1_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->aif1_clken > 0) {
			sunxi_internal_codec->aif1_clken--;
			if (sunxi_internal_codec->aif1_clken == 0) {
				/*disable AIF1CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF1CLK_ENA), (0x0<<AIF1CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF1_MOD_CLK_EN), (0x0<<AIF1_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF1_MOD_RST_CTL), (0x0<<AIF1_MOD_RST_CTL));
				/*DISABLE systemclk*/
				if (sunxi_internal_codec->aif2_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->aifclk_mutex);

	return 0;
}

int ac_aif2clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->aifclk_mutex);
	pr_debug("aif2 interface clk power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->aif2_clken == 0) {
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x1<<AIF2_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x1<<AIF2_MOD_RST_CTL));
			/*enable systemclk*/
			if (sunxi_internal_codec->aif1_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sunxi_internal_codec->aif2_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->aif2_clken > 0) {
			sunxi_internal_codec->aif2_clken--;
			if (sunxi_internal_codec->aif2_clken == 0) {
				/*disable AIF2CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x0<<AIF2_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x0<<AIF2_MOD_RST_CTL));
				/*DISABLE systemclk*/
				if (sunxi_internal_codec->aif1_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->aifclk_mutex);

	return 0;
}

int ac_aif3clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->aifclk_mutex);
	pr_debug("aif3 interface clk power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->aif2_clken == 0) {
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x1<<AIF2_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x1<<AIF2_MOD_RST_CTL));
			/*enable systemclk*/
			if (sunxi_internal_codec->aif1_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sunxi_internal_codec->aif2_clken++;
		if (sunxi_internal_codec->aif3_clken == 0) {
			/*enable AIF3CLK*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF3_MOD_CLK_EN), (0x1<<AIF3_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF3_MOD_RST_CTL), (0x1<<AIF3_MOD_RST_CTL));
		}
		sunxi_internal_codec->aif3_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->aif2_clken > 0) {
			sunxi_internal_codec->aif2_clken--;
			if (sunxi_internal_codec->aif2_clken == 0) {
				/*disable AIF2CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x0<<AIF2_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x0<<AIF2_MOD_RST_CTL));
				/*disable systemclk*/
				if (sunxi_internal_codec->aif1_clken == 0 && sunxi_internal_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		if (sunxi_internal_codec->aif3_clken > 0) {
			sunxi_internal_codec->aif3_clken--;
			if (sunxi_internal_codec->aif3_clken == 0) {
				/*disable AIF3CLK*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF3_MOD_CLK_EN), (0x0<<AIF3_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF3_MOD_RST_CTL), (0x0<<AIF3_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->aifclk_mutex);

	return 0;
}

static int late_enable_dac(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->dac_mutex);
	pr_debug("..dac power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->dac_enable == 0) {
			/*enable dac module clk*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x1<<DAC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x1<<DAC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
		}
		sunxi_internal_codec->dac_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->dac_enable > 0) {
			sunxi_internal_codec->dac_enable--;
			if (sunxi_internal_codec->dac_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
				/*disable dac module clk*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x0<<DAC_DIGITAL_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x0<<DAC_DIGITAL_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->dac_mutex);

	return 0;
}

static int late_enable_adc(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->adc_mutex);
	pr_debug("..adc power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->adc_enable == 0) {
			/*enable adc module clk*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<ADC_DIGITAL_MOD_CLK_EN), (0x1<<ADC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<ADC_DIGITAL_MOD_RST_CTL), (0x1<<ADC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		sunxi_internal_codec->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->adc_enable > 0) {
			sunxi_internal_codec->adc_enable--;
			if (sunxi_internal_codec->adc_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<ADC_DIGITAL_MOD_CLK_EN), (0x0<<ADC_DIGITAL_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<ADC_DIGITAL_MOD_RST_CTL), (0x0<<ADC_DIGITAL_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->adc_mutex);

	return 0;
}

static int ac_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("..headphone power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		//snd_soc_update_bits(codec, HP_PA_CTRL, (0xf<<HPOUTPUTENABLE), (0xf<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPA_EN), (0x1<<HPPA_EN));
		msleep(10);
		snd_soc_update_bits(codec, MIX_DAC_CTRL, (0x3<<LHPPAMUTE), (0x3<<LHPPAMUTE));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPA_EN), (0x0<<HPPA_EN));
		//snd_soc_update_bits(codec, HP_PA_CTRL, (0xf<<HPOUTPUTENABLE), (0x0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, MIX_DAC_CTRL, (0x3<<LHPPAMUTE), (0x0<<LHPPAMUTE));
		break;
	}

	return 0;
}
static int ac_earpiece_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = w->codec;

	pr_debug("..earpiece power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		snd_soc_update_bits(codec, EARPIECE_CTRL1, (0x1<<ESPPA_EN), (0x1<<ESPPA_EN));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_update_bits(codec, EARPIECE_CTRL1, (0x1<<ESPPA_EN), (0x0<<ESPPA_EN));
		break;
	}

	return 0;
}
static int ac_speaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	pr_debug("..speaker power state change.\n");
	switch (event) {
		case SND_SOC_DAPM_POST_PMU:
			sunxi_internal_codec->spkenable = true;
			msleep(50);
			if (spk_gpio.cfg)
				gpio_set_value(spk_gpio.gpio, 1);

			break;
		case SND_SOC_DAPM_PRE_PMD :
			sunxi_internal_codec->spkenable = false;
			if (spk_gpio.cfg)
				gpio_set_value(spk_gpio.gpio, 0);

		default:
			break;
	}

	return 0;
}
static int aif2inl_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x1<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}

	return 0;
}

static int aif2inr_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x2<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}

	return 0;
}

static int dmic_mux_ev(struct snd_soc_dapm_widget *w,
		      struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&sunxi_internal_codec->adc_mutex);
	switch (event){
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->adc_enable == 0){
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x1<<DAC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x1<<DAC_DIGITAL_MOD_RST_CTL));
			//snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
			snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENDM), (0x1<<ENDM));
		}
		sunxi_internal_codec->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->adc_enable > 0){
			sunxi_internal_codec->adc_enable--;
			if (sunxi_internal_codec->adc_enable == 0){
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x0<<DAC_DIGITAL_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x0<<DAC_DIGITAL_MOD_RST_CTL));
				//snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
				snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENDM), (0x0<<ENDM));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->adc_mutex);
	pr_debug("%s,line:%d,SUNXI_ADC_DIG_CTRL(300):%x\n", __func__, __LINE__,snd_soc_read(codec,SUNXI_ADC_DIG_CTRL));

	return 0;
}

/*
*	use for enable src function
*/
static int set_src_function(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	src_function_en = ucontrol->value.integer.value[0];
	if (src_function_en) {
		pr_debug("Enable src clk , config src 8k-8k.\n");
		/*enable srcclk*/
		clk_prepare_enable(sunxi_internal_codec->srcclk);
		/*src1*/
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<SRC1_MOD_CLK_EN), (0x1<<SRC1_MOD_CLK_EN));
		/*src2*/
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<SRC2_MOD_CLK_EN), (0x1<<SRC2_MOD_CLK_EN));
		/*src2*/
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<SRC2_MOD_RST_CTL), (0x1<<SRC2_MOD_RST_CTL));
		/*src1*/
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<SRC1_MOD_RST_CTL), (0x1<<SRC1_MOD_RST_CTL));

		/*select src1 source from aif2*/
		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC1_SRC), (0x1<<SRC1_SRC));
		/*select src2 source from aif2*/
		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC2_SRC), (0x1<<SRC2_SRC));
		/*enable src1*/
		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC1_ENA), (0x1<<SRC1_ENA));
		/*enable src2*/
		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC1_ENA), (0x1<<SRC1_ENA));
	} else {
		pr_debug("disable src clk.\n");
		clk_disable_unprepare(sunxi_internal_codec->srcclk);
		/*src1*/
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<SRC1_MOD_CLK_EN), (0x0<<SRC1_MOD_CLK_EN));
		/*src2*/
		snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<SRC2_MOD_CLK_EN), (0x0<<SRC2_MOD_CLK_EN));
		/*src2*/
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<SRC2_MOD_RST_CTL), (0x0<<SRC2_MOD_RST_CTL));
		/*src1*/
		snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<SRC1_MOD_RST_CTL), (0x0<<SRC1_MOD_RST_CTL));

		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC1_ENA), (0x0<<SRC1_ENA));
		snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0x1<<SRC1_ENA), (0x0<<SRC1_ENA));
	}

	return 0;
}

static int get_src_function(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = src_function_en;

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/*AIF1*/
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 volume", SUNXI_AIF1_VOL_CTRL1, AIF1_AD0L_VOL, AIF1_AD0R_VOL, 0xff, 0, aif1_ad_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 volume", SUNXI_AIF1_VOL_CTRL2, AIF1_AD1L_VOL, AIF1_AD1R_VOL, 0xff, 0, aif1_ad_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 0 volume", SUNXI_AIF1_VOL_CTRL3, AIF1_DA0L_VOL, AIF1_DA0R_VOL, 0xff, 0, aif1_da_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 1 volume", SUNXI_AIF1_VOL_CTRL4, AIF1_DA1L_VOL, AIF1_DA1R_VOL, 0xff, 0, aif1_da_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 mixer gain", SUNXI_AIF1_MXR_GAIN, AIF1_AD0L_MXR_GAIN, AIF1_AD0R_MXR_GAIN, 0xf, 0, aif1_ad_slot0_mix_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 mixer gain", SUNXI_AIF1_MXR_GAIN, AIF1_AD1L_MXR_GAIN, AIF1_AD1R_MXR_GAIN, 0x3, 0, aif1_ad_slot1_mix_vol_tlv),
	/*AIF2*/
	SOC_DOUBLE_TLV("AIF2 ADC volume", SUNXI_AIF2_VOL_CTRL1, AIF2_ADCL_VOL,AIF2_ADCR_VOL, 0xff, 0, aif2_ad_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 DAC volume", SUNXI_AIF2_VOL_CTRL2, AIF2_DACL_VOL,AIF2_DACR_VOL, 0xff, 0, aif2_da_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 ADC mixer gain", SUNXI_AIF2_MXR_GAIN, AIF2_ADCL_MXR_GAIN,AIF2_ADCR_MXR_GAIN, 0xf, 0, aif2_ad_mix_vol_tlv),
	/*ADC*/
	SOC_DOUBLE_TLV("ADC volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R, 0xff, 0, adc_vol_tlv),
	/*DAC*/
	SOC_DOUBLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R, 0xff, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC mixer gain", SUNXI_DAC_MXR_GAIN, DACL_MXR_GAIN, DACR_MXR_GAIN, 0xf, 0, dac_mix_vol_tlv),

	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DBG_CTRL, DVC, 0x3f, 0, dig_vol_tlv),

	/*analog control*/
	SOC_SINGLE_TLV("earpiece volume", EARPIECE_CTRL1, ESP_VOL, 0x1f, 0, earpiece_vol_tlv),
	SOC_SINGLE_TLV("speaker volume", SPKOUT_CTRL1, SPKOUT_VOL, 0x1f, 0, speaker_vol_tlv),
	SOC_SINGLE_TLV("headphone volume", HP_CTRL, HPVOL, 0x3f, 0, headphone_vol_tlv),

	SOC_SINGLE_TLV("MIC1_G boost stage output mixer control", MIC1_CTRL, MIC1G, 0x7, 0, mic1_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC1 boost AMP gain control", MIC1_CTRL, MIC1BOOST, 0x7, 0, mic1_boost_vol_tlv),

	SOC_SINGLE_TLV("MIC2 BST stage to L_R outp mixer gain", MIC2_CTRL, MIC2G, 0x7, 0, mic2_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC2 boost AMP gain control", MIC2_CTRL, MIC2BOOST, 0x7, 0, mic2_boost_vol_tlv),

	SOC_SINGLE_TLV("LINEINL/R to L_R output mixer gain", LINEIN_CTRL, LINEING, 0x7, 0, linein_to_l_r_mix_vol_tlv),
	/*ADC*/
	SOC_SINGLE_TLV("ADC input gain control", ADC_CTRL, ADCG, 0x7, 0, adc_input_vol_tlv),

	SOC_SINGLE_TLV("Phoneout gain control", PHONEOUT_CTRL, PHONEOUTGAIN, 0x7, 0, phoneout_vol_tlv),
	SOC_SINGLE_BOOL_EXT("SRC FUCTION", 	0, get_src_function, 	set_src_function),
};

/*0x244:AIF1 AD0 OUT */
static const char *aif1out0l_text[] = {
	"AIF1_AD0L", "AIF1_AD0R","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};
static const char *aif1out0r_text[] = {
	"AIF1_AD0R", "AIF1_AD0L","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};

static const struct soc_enum aif1out0l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 10, 4, aif1out0l_text);

static const struct snd_kcontrol_new aif1out0l_mux =
	SOC_DAPM_ENUM("AIF1OUT0L Mux", aif1out0l_enum);

static const struct soc_enum aif1out0r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 8, 4, aif1out0r_text);

static const struct snd_kcontrol_new aif1out0r_mux =
	SOC_DAPM_ENUM("AIF1OUT0R Mux", aif1out0r_enum);

/*0x244:AIF1 AD1 OUT */
static const char *aif1out1l_text[] = {
	"AIF1_AD1L", "AIF1_AD1R","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};
static const char *aif1out1r_text[] = {
	"AIF1_AD1R", "AIF1_AD1L","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};

static const struct soc_enum aif1out1l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 6, 4, aif1out1l_text);

static const struct snd_kcontrol_new aif1out1l_mux =
	SOC_DAPM_ENUM("AIF1OUT1L Mux", aif1out1l_enum);

static const struct soc_enum aif1out1r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 4, 4, aif1out1r_text);

static const struct snd_kcontrol_new aif1out1r_mux =
	SOC_DAPM_ENUM("AIF1OUT1R Mux", aif1out1r_enum);

/*0x248:AIF1 DA0 IN*/
static const char *aif1in0l_text[] = {
	"AIF1_DA0L", "AIF1_DA0R", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};
static const char *aif1in0r_text[] = {
	"AIF1_DA0R", "AIF1_DA0L", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};

static const struct soc_enum aif1in0l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 10, 4, aif1in0l_text);

static const struct snd_kcontrol_new aif1in0l_mux =
	SOC_DAPM_ENUM("AIF1IN0L Mux", aif1in0l_enum);

static const struct soc_enum aif1in0r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 8, 4, aif1in0r_text);

static const struct snd_kcontrol_new aif1in0r_mux =
	SOC_DAPM_ENUM("AIF1IN0R Mux", aif1in0r_enum);

/*0x248:AIF1 DA1 IN*/
static const char *aif1in1l_text[] = {
	"AIF1_DA1L", "AIF1_DA1R","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};
static const char *aif1in1r_text[] = {
	"AIF1_DA1R", "AIF1_DA1L","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};

static const struct soc_enum aif1in1l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 6, 4, aif1in1l_text);

static const struct snd_kcontrol_new aif1in1l_mux =
	SOC_DAPM_ENUM("AIF1IN1L Mux", aif1in1l_enum);

static const struct soc_enum aif1in1r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 4, 4, aif1in1r_text);

static const struct snd_kcontrol_new aif1in1r_mux =
	SOC_DAPM_ENUM("AIF1IN1R Mux", aif1in1r_enum);

/*0x24c:AIF1 ADC0 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad0l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF1DA0L, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACR, 1, 0),
};

static const struct snd_kcontrol_new aif1_ad0r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF1DA0R, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_ADCR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACL, 1, 0),
};

/*0x24c:AIF1 ADC1 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad1l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD1L_MXR_AIF2_DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD1L_MXR_ADCL, 1, 0),
};

static const struct snd_kcontrol_new aif1_ad1r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD1R_MXR_AIF2_DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD1R_MXR_ADCR, 1, 0),
};

/*0x330 dac digital mixer source select*/
static const struct snd_kcontrol_new dacl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_DAC_MXR_SRC,  DACL_MXR_SRC_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF2DACL Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1L Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF1DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0L Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF1DA0L, 1, 0),
};

static const struct snd_kcontrol_new dacr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_DAC_MXR_SRC,  DACR_MXR_SRC_ADCR, 1, 0),
	SOC_DAPM_SINGLE("AIF2DACR Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1R Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF1DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0R Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF1DA0R, 1, 0),
};

/*output mixer source select*/
/*analog:0x01:defined left output mixer*/
static const struct snd_kcontrol_new ac_loutmix_controls[] = {
	SOC_DAPM_SINGLE("DACR Switch", OL_MIX_CTRL, LMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OL_MIX_CTRL, LMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", OL_MIX_CTRL, LMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OL_MIX_CTRL, LMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OL_MIX_CTRL, LMIXMUTEMIC1BOOST, 1, 0),

	SOC_DAPM_SINGLE("PHONEINP Switch", OL_MIX_CTRL, LMIXMUTEPHONEP, 1, 0),
	SOC_DAPM_SINGLE("PHONEINP-PHONEINN Switch", OL_MIX_CTRL, LMIXMUTEPHONEPN, 1, 0),
};

/*analog:0x02:defined right output mixer*/
static const struct snd_kcontrol_new ac_routmix_controls[] = {
	SOC_DAPM_SINGLE("DACL Switch", OR_MIX_CTRL, RMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OR_MIX_CTRL, RMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", OR_MIX_CTRL, RMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OR_MIX_CTRL, RMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OR_MIX_CTRL, RMIXMUTEMIC1BOOST, 1, 0),

	SOC_DAPM_SINGLE("PHONEINN Switch", OR_MIX_CTRL, LMIXMUTEPHONEN, 1, 0),
	SOC_DAPM_SINGLE("PHONEINN-PHONEINP Switch", OR_MIX_CTRL, LMIXMUTEPHONENP, 1, 0),
};

/*hp source select*/
/*0x0a:headphone input source*/
static const char *ac_hp_r_func_sel[] = {
	"DACR HPR Switch", "Right Analog Mixer HPR Switch"};
static const struct soc_enum ac_hp_r_func_enum =
	SOC_ENUM_SINGLE(MIX_DAC_CTRL, RHPIS, 2, ac_hp_r_func_sel);

static const struct snd_kcontrol_new ac_hp_r_func_controls =
	SOC_DAPM_ENUM("HP_R Mux", ac_hp_r_func_enum);

static const char *ac_hp_l_func_sel[] = {
	"DACL HPL Switch", "Left Analog Mixer HPL Switch"};
static const struct soc_enum ac_hp_l_func_enum =
	SOC_ENUM_SINGLE(MIX_DAC_CTRL, LHPIS, 2, ac_hp_l_func_sel);

static const struct snd_kcontrol_new ac_hp_l_func_controls =
	SOC_DAPM_ENUM("HP_L Mux", ac_hp_l_func_enum);

/*0x05:spk source select*/
static const char *ac_rspks_func_sel[] = {
	"MIXER Switch", "MIXR MIXL Switch"};
static const struct soc_enum ac_rspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL0, RIGHT_SPK_SRC_SEL, 2, ac_rspks_func_sel);

static const struct snd_kcontrol_new ac_rspks_func_controls =
	SOC_DAPM_ENUM("SPK_R Mux", ac_rspks_func_enum);

static const char *ac_lspks_l_func_sel[] = {
	"MIXEL Switch", "MIXL MIXR  Switch"};
static const struct soc_enum ac_lspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL0, LEFT_SPK_SRC_SEL, 2, ac_lspks_l_func_sel);

static const struct snd_kcontrol_new ac_lspks_func_controls =
	SOC_DAPM_ENUM("SPK_L Mux", ac_lspks_func_enum);

/*0x03:earpiece source select*/
static const char *ac_earpiece_func_sel[] = {
	"DACR", "DACL", "Right Analog Mixer", "Left Analog Mixer"};
static const struct soc_enum ac_earpiece_func_enum =
	SOC_ENUM_SINGLE(EARPIECE_CTRL0, ESPSR, 4, ac_earpiece_func_sel);

static const struct snd_kcontrol_new ac_earpiece_func_controls =
	SOC_DAPM_ENUM("EAR Mux", ac_earpiece_func_enum);

/*0x284:AIF2 out*/
static const char *aif2outl_text[] = {
	"AIF2_ADCL", "AIF2_ADCR","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};
static const char *aif2outr_text[] = {
	"AIF2_ADCR", "AIF2_ADCL","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};

static const struct soc_enum aif2outl_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_ADCDAT_CTRL, 10, 4, aif2outl_text);

static const struct snd_kcontrol_new aif2outl_mux =
	SOC_DAPM_ENUM("AIF2OUTL Mux", aif2outl_enum);

static const struct soc_enum aif2outr_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_ADCDAT_CTRL, 8, 4, aif2outr_text);

static const struct snd_kcontrol_new aif2outr_mux =
	SOC_DAPM_ENUM("AIF2OUTR Mux", aif2outr_enum);

/*0x288:AIF2 IN*/
static const char *aif2inl_text[] = {
	"AIF2_DACL", "AIF2_DACR","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};
static const char *aif2inr_text[] = {
	"AIF2_DACR", "AIF2_DACL","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};

static const struct soc_enum aif2inl_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_DACDAT_CTRL, 10, 4, aif2inl_text);
static const struct snd_kcontrol_new aif2inl_mux =
	SOC_DAPM_ENUM("AIF2INL Mux", aif2inl_enum);

static const struct soc_enum aif2inr_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_DACDAT_CTRL, 8, 4, aif2inr_text);
static const struct snd_kcontrol_new aif2inr_mux =
	SOC_DAPM_ENUM("AIF2INR Mux", aif2inr_enum);

/*0x28c:AIF2 source select*/
static const struct snd_kcontrol_new aif2_adcl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF1DA0L, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1L Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF1DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_ADCL, 1, 0),
};

static const struct snd_kcontrol_new aif2_adcr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", SUNXI_AIF2_MXR_SRC,	AIF2_ADCR_MXR_SRC_AIF1DA0R, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1R Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_AIF1DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_ADCR, 1, 0),
};

/*0x2cc:aif3 out, AIF3 PCM output source select*/
static const char *aif3out_text[] = {
	"NULL", "AIF2 ADC left channel", "AIF2 ADC right channel"
};

static const unsigned int aif3out_values[] = {0, 1, 2};

static const struct soc_enum aif3out_enum =
		SOC_VALUE_ENUM_SINGLE(SUNXI_AIF3_SGP_CTRL, 10, 3,
		ARRAY_SIZE(aif3out_text),aif3out_text, aif3out_values);

static const struct snd_kcontrol_new aif3out_mux =
	SOC_DAPM_ENUM("AIF3OUT Mux", aif3out_enum);

/*0x2cc:aif2 DAC input source select*/
static const char *aif2dacin_text[] = {
	"Left_s right_s AIF2","Left_s AIF3 Right_s AIF2", "Left_s AIF2 Right_s AIF3"
};

static const struct soc_enum aif2dacin_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF3_SGP_CTRL, 8, 3, aif2dacin_text);

static const struct snd_kcontrol_new aif2dacin_mux =
	SOC_DAPM_ENUM("AIF2 DAC SRC Mux", aif2dacin_enum);

/*ADC SOURCE SELECT*/
/*0x0b:defined left input adc mixer*/
static const struct snd_kcontrol_new ac_ladcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", L_ADCMIX_SRC, LADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", L_ADCMIX_SRC, LADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", L_ADCMIX_SRC, LADCMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("l_output mixer Switch", L_ADCMIX_SRC, LADCMIXMUTELOUTPUT, 1, 0),
	SOC_DAPM_SINGLE("r_output mixer Switch", L_ADCMIX_SRC, LADCMIXMUTEROUTPUT, 1, 0),

	SOC_DAPM_SINGLE("PHONINP Switch", L_ADCMIX_SRC, LADCMIXMUTEPHONEP, 1, 0),
	SOC_DAPM_SINGLE("PHONINP-PHONINN Switch", L_ADCMIX_SRC, LADCMIXMUTEPHONEPN, 1, 0),
};

/*0x0c:defined right input adc mixer*/
static const struct snd_kcontrol_new ac_radcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", R_ADCMIX_SRC, RADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", R_ADCMIX_SRC, RADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", R_ADCMIX_SRC, RADCMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("r_output mixer Switch", R_ADCMIX_SRC, RADCMIXMUTEROUTPUT, 1, 0),
	SOC_DAPM_SINGLE("l_output mixer Switch", R_ADCMIX_SRC, RADCMIXMUTELOUTPUT, 1, 0),

	SOC_DAPM_SINGLE("PHONINN Switch", R_ADCMIX_SRC, LADCMIXMUTEPHONEN, 1, 0),
	SOC_DAPM_SINGLE("PHONINN-PHONINP Switch", R_ADCMIX_SRC, LADCMIXMUTEPHONENP, 1, 0),
};

/*0x08:mic2 source select*/
static const char *mic2src_text[] = {"MIC3","MIC2"};

static const struct soc_enum mic2src_enum =
	SOC_ENUM_SINGLE(MIC2_CTRL, 7, 2, mic2src_text);

static const struct snd_kcontrol_new mic2src_mux =
	SOC_DAPM_ENUM("MIC2 SRC", mic2src_enum);

/*DMIC*/
static const char *adc_mux_text[] = {
	"ADC",
	"DMIC",
};
static const struct soc_enum adc_enum =
	SOC_ENUM_SINGLE(0, 0, 2, adc_mux_text);
static const struct snd_kcontrol_new adcl_mux =
	SOC_DAPM_ENUM_VIRT("ADCL Mux", adc_enum);
static const struct snd_kcontrol_new adcr_mux =
	SOC_DAPM_ENUM_VIRT("ADCR Mux", adc_enum);

static const struct snd_kcontrol_new aif2inl_aif2switch =
	SOC_DAPM_SINGLE("aif2inl aif2", SUNXI_AIF1_RXD_CTRL, 8, 1, 0);
static const struct snd_kcontrol_new aif2inr_aif2switch =
	SOC_DAPM_SINGLE("aif2inr aif2", SUNXI_AIF1_RXD_CTRL, 9, 1, 0);

static const struct snd_kcontrol_new aif2inl_aif3switch =
	SOC_DAPM_SINGLE("aif2inl aif3", SUNXI_AIF1_RXD_CTRL, 10, 1, 0);
static const struct snd_kcontrol_new aif2inr_aif3switch =
	SOC_DAPM_SINGLE("aif2inr aif3", SUNXI_AIF1_RXD_CTRL, 11, 1, 0);

/*defined lineout mixer*/
static const struct snd_kcontrol_new phoneout_mix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", PHONEOUT_CTRL, PHONEOUTS3, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", PHONEOUT_CTRL, PHONEOUTS2, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", PHONEOUT_CTRL, PHONEOUTS1, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", PHONEOUT_CTRL, PHONEOUTS0, 1, 0),
};

/*built widget*/
static const struct snd_soc_dapm_widget ac_dapm_widgets[] = {
	SND_SOC_DAPM_SWITCH("AIF2INL Mux switch", SND_SOC_NOPM, 0, 1,
			    &aif2inl_aif2switch),
	SND_SOC_DAPM_SWITCH("AIF2INR Mux switch", SND_SOC_NOPM, 0, 1,
			    &aif2inr_aif2switch),

	SND_SOC_DAPM_SWITCH("AIF2INL Mux VIR switch", SND_SOC_NOPM, 0, 1,
			    &aif2inl_aif3switch),
	SND_SOC_DAPM_SWITCH("AIF2INR Mux VIR switch", SND_SOC_NOPM, 0, 1,
			    &aif2inr_aif3switch),

	/*0x244*/
	SND_SOC_DAPM_MUX("AIF1OUT0L Mux", SUNXI_AIF1_ADCDAT_CTRL, 15, 0, &aif1out0l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT0R Mux", SUNXI_AIF1_ADCDAT_CTRL, 14, 0, &aif1out0r_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1L Mux", SUNXI_AIF1_ADCDAT_CTRL, 13, 0, &aif1out1l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1R Mux", SUNXI_AIF1_ADCDAT_CTRL, 12, 0, &aif1out1r_mux),
	/*0x248*/
	SND_SOC_DAPM_MUX("AIF1IN0L Mux", SUNXI_AIF1_DACDAT_CTRL, 15, 0, &aif1in0l_mux),
	SND_SOC_DAPM_MUX("AIF1IN0R Mux", SUNXI_AIF1_DACDAT_CTRL, 14, 0, &aif1in0r_mux),
	SND_SOC_DAPM_MUX("AIF1IN1L Mux", SUNXI_AIF1_DACDAT_CTRL, 13, 0, &aif1in1l_mux),
	SND_SOC_DAPM_MUX("AIF1IN1R Mux", SUNXI_AIF1_DACDAT_CTRL, 12, 0, &aif1in1r_mux),
	/*0x24c*/
#ifdef AIF1_FPGA_LOOPBACK_TEST
	SND_SOC_DAPM_MIXER_E("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0,
		aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl), late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0,
		aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl), late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#else
	SND_SOC_DAPM_MIXER("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl)),
#endif
	SND_SOC_DAPM_MIXER("AIF1 AD1L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1l_mxr_src_ctl, ARRAY_SIZE(aif1_ad1l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD1R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1r_mxr_src_ctl, ARRAY_SIZE(aif1_ad1r_mxr_src_ctl)),
	/*analog:0x0a*/
	SND_SOC_DAPM_MIXER_E("DACL Mixer", SND_SOC_NOPM, 0, 0, dacl_mxr_src_controls, ARRAY_SIZE(dacl_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("DACR Mixer", SND_SOC_NOPM, 0, 0, dacr_mxr_src_controls, ARRAY_SIZE(dacr_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*0x0a*/
	SND_SOC_DAPM_MIXER("Left Output Mixer", MIX_DAC_CTRL, LMIXEN, 0,
			ac_loutmix_controls, ARRAY_SIZE(ac_loutmix_controls)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", MIX_DAC_CTRL, RMIXEN, 0,
			ac_routmix_controls, ARRAY_SIZE(ac_routmix_controls)),

	SND_SOC_DAPM_MUX("HP_R Mux", SND_SOC_NOPM, 0, 0,	&ac_hp_r_func_controls),
	SND_SOC_DAPM_MUX("HP_L Mux", SND_SOC_NOPM, 0, 0,	&ac_hp_l_func_controls),
	/*0x05*/
	SND_SOC_DAPM_MUX("SPK_R Mux", SPKOUT_CTRL0, SPKOUT_R_EN, 0,	&ac_rspks_func_controls),
	SND_SOC_DAPM_MUX("SPK_L Mux", SPKOUT_CTRL0, SPKOUT_L_EN, 0,	&ac_lspks_func_controls),

	SND_SOC_DAPM_PGA("SPK_LR Adder", SND_SOC_NOPM, 0, 0, NULL, 0),
	/*0x04*/
	SND_SOC_DAPM_MUX("EAR Mux", EARPIECE_CTRL1, ESPPA_MUTE, 0,	&ac_earpiece_func_controls),

	/*output widget*/
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),
	SND_SOC_DAPM_OUTPUT("EAROUTP"),
	SND_SOC_DAPM_OUTPUT("EAROUTN"),
	/*spk is diff with ac100. need TODO...*/
	SND_SOC_DAPM_OUTPUT("SPKL"),
	SND_SOC_DAPM_OUTPUT("SPKR"),

	/*0x284*/
	SND_SOC_DAPM_MUX("AIF2OUTL Mux", SUNXI_AIF2_ADCDAT_CTRL, 15, 0, &aif2outl_mux),
	SND_SOC_DAPM_MUX("AIF2OUTR Mux", SUNXI_AIF2_ADCDAT_CTRL, 14, 0, &aif2outr_mux),
	/*0x288*/
	SND_SOC_DAPM_MUX("AIF2INL Mux", SUNXI_AIF2_DACDAT_CTRL, 15, 0, &aif2inl_mux),
	SND_SOC_DAPM_MUX("AIF2INR Mux", SUNXI_AIF2_DACDAT_CTRL, 14, 0, &aif2inr_mux),

	SND_SOC_DAPM_PGA("AIF2INL_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("AIF2INR_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),
	/*0x28c*/
	SND_SOC_DAPM_MIXER("AIF2 ADL Mixer", SND_SOC_NOPM, 0, 0, aif2_adcl_mxr_src_controls, ARRAY_SIZE(aif2_adcl_mxr_src_controls)),
	SND_SOC_DAPM_MIXER("AIF2 ADR Mixer", SND_SOC_NOPM, 0, 0, aif2_adcr_mxr_src_controls, ARRAY_SIZE(aif2_adcr_mxr_src_controls)),
	/*0x2cc*/
	SND_SOC_DAPM_MUX("AIF3OUT Mux", SND_SOC_NOPM, 0, 0, &aif3out_mux),
	/*0x2cc virtual widget*/
	SND_SOC_DAPM_PGA_E("AIF2INL Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inl_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("AIF2INR Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inr_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#ifdef AIF1_FPGA_LOOPBACK_TEST
	/*0x0d 0x0b 0x0c ADC_CTRL*/
	SND_SOC_DAPM_MIXER("LADC input Mixer", ADC_CTRL, ADCLEN, 0, ac_ladcmix_controls, ARRAY_SIZE(ac_ladcmix_controls)),
	SND_SOC_DAPM_MIXER("RADC input Mixer", ADC_CTRL, ADCREN, 0, ac_radcmix_controls, ARRAY_SIZE(ac_radcmix_controls)),
#else
	/*0x0d 0x0b 0x0c ADC_CTRL*/
	SND_SOC_DAPM_MIXER_E("LADC input Mixer", ADC_CTRL, ADCLEN, 0,
		ac_ladcmix_controls, ARRAY_SIZE(ac_ladcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("RADC input Mixer", ADC_CTRL, ADCREN, 0,
		ac_radcmix_controls, ARRAY_SIZE(ac_radcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#endif
	/*0x07 mic1 reference*/
	SND_SOC_DAPM_PGA("MIC1 PGA", MIC1_CTRL, MIC1AMPEN, 0, NULL, 0),
	/*0x08 mic2 reference*/
	SND_SOC_DAPM_PGA("MIC2 PGA", MIC2_CTRL, MIC2AMPEN, 0, NULL, 0),

	/*0x08*/
	SND_SOC_DAPM_MUX("MIC2 SRC", SND_SOC_NOPM, 0, 0, &mic2src_mux),

	/*INPUT widget*/
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),
	/*0x0e Headset Microphone Bias Control Register*/
	SND_SOC_DAPM_MICBIAS("MainMic Bias", HS_MBIAS_CTRL, MMICBIASEN, 0),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),

	SND_SOC_DAPM_INPUT("LINEINP"),
	SND_SOC_DAPM_INPUT("LINEINN"),
	/*DMIC*/
	SND_SOC_DAPM_INPUT("D_MIC"),
	SND_SOC_DAPM_VIRT_MUX("ADCL Mux", SND_SOC_NOPM, 0, 0, &adcl_mux),
	SND_SOC_DAPM_VIRT_MUX("ADCR Mux", SND_SOC_NOPM, 0, 0, &adcr_mux),

	SND_SOC_DAPM_PGA_E("DMICL VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMICR VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif1 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF1DACL", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF1DACR", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCL", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCR", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif2 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF2DACL", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("AIF2DACR", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF2ADCL", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF2ADCR", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif3 interface*/
	SND_SOC_DAPM_AIF_OUT_E("AIF3OUT", "AIF3 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif3clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF3IN", "AIF3 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif3clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	/*headphone*/
	SND_SOC_DAPM_HP("Headphone", ac_headphone_event),
	/*earpiece*/
	SND_SOC_DAPM_SPK("Earpiece", ac_earpiece_event),
	/*speaker*/
	SND_SOC_DAPM_SPK("External Speaker", ac_speaker_event),

	SND_SOC_DAPM_PGA("PHONEIN PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_INPUT("PHONEINP"),
	SND_SOC_DAPM_INPUT("PHONEINN"),

	SND_SOC_DAPM_OUTPUT("PHONEOUTP"),
	SND_SOC_DAPM_OUTPUT("PHONEOUTN"),

	SND_SOC_DAPM_MIXER("Phoneout Mixer", PHONEOUT_CTRL, PHONEOUTEN, 0,
		phoneout_mix_controls, ARRAY_SIZE(phoneout_mix_controls)),
};

static const struct snd_soc_dapm_route ac_dapm_routes[] = {
	{"AIF1ADCL", NULL, "AIF1OUT0L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT0R Mux"},

	{"AIF1ADCL", NULL, "AIF1OUT1L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT1R Mux"},

	/* aif1out0 mux 11---13*/
	{"AIF1OUT0L Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},
	{"AIF1OUT0L Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},

	{"AIF1OUT0R Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},
	{"AIF1OUT0R Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},

	/*AIF1OUT1 mux 11--13 */
	{"AIF1OUT1L Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},
	{"AIF1OUT1L Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},

	{"AIF1OUT1R Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},
	{"AIF1OUT1R Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},

	/*AIF1 AD0L Mixer*/
	{"AIF1 AD0L Mixer", "AIF1 DA0L Switch",		"AIF1IN0L Mux"},
	{"AIF1 AD0L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	#ifdef AIF1_FPGA_LOOPBACK_TEST
	{"AIF1 AD0L Mixer", "ADCL Switch",		"MIC1P"},
	#else
	{"AIF1 AD0L Mixer", "ADCL Switch",		"ADCL Mux"},
	#endif
	{"AIF1 AD0L Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},

	/*AIF1 AD0R Mixer*/
	{"AIF1 AD0R Mixer", "AIF1 DA0R Switch",		"AIF1IN0R Mux"},
	{"AIF1 AD0R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},

	#ifdef AIF1_FPGA_LOOPBACK_TEST
	{"AIF1 AD0R Mixer", "ADCR Switch",		"MIC1N"},
	#else
	{"AIF1 AD0R Mixer", "ADCR Switch",		"ADCR Mux"},
	#endif
	{"AIF1 AD0R Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},

	/*AIF1 AD1L Mixer*/
	{"AIF1 AD1L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	{"AIF1 AD1L Mixer", "ADCL Switch",		"ADCL Mux"},

	/*AIF1 AD1R Mixer*/
	{"AIF1 AD1R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},
	{"AIF1 AD1R Mixer", "ADCR Switch",		"ADCR Mux"},

	/*AIF1 DA0 IN 12h*/
	{"AIF1IN0L Mux", "AIF1_DA0L",		"AIF1DACL"},
	{"AIF1IN0L Mux", "AIF1_DA0R",		"AIF1DACR"},

	{"AIF1IN0R Mux", "AIF1_DA0R",		"AIF1DACR"},
	{"AIF1IN0R Mux", "AIF1_DA0L",		"AIF1DACL"},

	/*AIF1 DA1 IN 12h*/
	{"AIF1IN1L Mux", "AIF1_DA1L",		"AIF1DACL"},
	{"AIF1IN1L Mux", "AIF1_DA1R",		"AIF1DACR"},

	{"AIF1IN1R Mux", "AIF1_DA1R",		"AIF1DACR"},
	{"AIF1IN1R Mux", "AIF1_DA1L",		"AIF1DACL"},

	/*aif2 virtual*/
	{"AIF2INL Mux switch", "aif2inl aif2",		"AIF2INL Mux"},
	{"AIF2INR Mux switch", "aif2inr aif2",		"AIF2INR Mux"},

	{"AIF2INL_VIR", NULL,		"AIF2INL Mux switch"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux switch"},

	{"AIF2INL_VIR", NULL,		"AIF2INL Mux VIR"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux VIR"},

	/*4c*/
	{"DACL Mixer", "AIF1DA0L Switch",		"AIF1IN0L Mux"},
	{"DACL Mixer", "AIF1DA1L Switch",		"AIF1IN1L Mux"},

	{"DACL Mixer", "ADCL Switch",		"ADCL Mux"},
	{"DACL Mixer", "AIF2DACL Switch",		"AIF2INL_VIR"},

	{"DACR Mixer", "AIF1DA0R Switch",		"AIF1IN0R Mux"},
	{"DACR Mixer", "AIF1DA1R Switch",		"AIF1IN1R Mux"},

	{"DACR Mixer", "ADCR Switch",		"ADCR Mux"},
	{"DACR Mixer", "AIF2DACR Switch",		"AIF2INR_VIR"},

	{"Right Output Mixer", "DACR Switch",		"DACR Mixer"},
	{"Right Output Mixer", "DACL Switch",		"DACL Mixer"},

	{"Right Output Mixer", "LINEINR Switch",		"LINEINN"},
	{"Right Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Right Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	{"Left Output Mixer", "DACL Switch",		"DACL Mixer"},
	{"Left Output Mixer", "DACR Switch",		"DACR Mixer"},

	{"Left Output Mixer", "LINEINL Switch",		"LINEINP"},
	{"Left Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Left Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	/*hp mux*/
	{"HP_R Mux", "DACR HPR Switch",		"DACR Mixer"},
	{"HP_R Mux", "Right Analog Mixer HPR Switch",		"Right Output Mixer"},

	{"HP_L Mux", "DACL HPL Switch",		"DACL Mixer"},
	{"HP_L Mux", "Left Analog Mixer HPL Switch",		"Left Output Mixer"},

	/*hp endpoint*/
	{"HPOUTR", NULL,				"HP_R Mux"},
	{"HPOUTL", NULL,				"HP_L Mux"},

	{"Headphone", NULL,				"HPOUTR"},
	{"Headphone", NULL,				"HPOUTL"},

	/*spk mux*/
	{"SPK_LR Adder", NULL,				"Right Output Mixer"},
	{"SPK_LR Adder", NULL,				"Left Output Mixer"},

	{"SPK_L Mux", "MIXL MIXR  Switch",				"SPK_LR Adder"},
	{"SPK_L Mux", "MIXEL Switch",				"Left Output Mixer"},

	{"SPK_R Mux", "MIXR MIXL Switch",				"SPK_LR Adder"},
	{"SPK_R Mux", "MIXER Switch",				"Right Output Mixer"},

	{"SPKR", NULL,				"SPK_R Mux"},
	{"SPKL", NULL,				"SPK_L Mux"},

	/*earpiece mux*/
	{"EAR Mux", "DACR",				"DACR Mixer"},
	{"EAR Mux", "DACL",				"DACL Mixer"},
	{"EAR Mux", "Right Analog Mixer",				"Right Output Mixer"},
	{"EAR Mux", "Left Analog Mixer",				"Left Output Mixer"},
	{"EAROUTP", NULL,		"EAR Mux"},
	{"EAROUTN", NULL,		"EAR Mux"},
	{"Earpiece", NULL,				"EAROUTP"},
	{"Earpiece", NULL,				"EAROUTN"},

	/*LADC SOURCE mixer*/
	{"LADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"LADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"LADC input Mixer", "LINEINL Switch",				"LINEINN"},
	{"LADC input Mixer", "l_output mixer Switch",				"Left Output Mixer"},
	{"LADC input Mixer", "r_output mixer Switch",				"Right Output Mixer"},

	/*RADC SOURCE mixer*/
	{"RADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"RADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"RADC input Mixer", "LINEINR Switch",				"LINEINP"},
	{"RADC input Mixer", "r_output mixer Switch",				"Right Output Mixer"},
	{"RADC input Mixer", "l_output mixer Switch",				"Left Output Mixer"},

	{"MIC1 PGA", NULL,				"MIC1P"},
	{"MIC1 PGA", NULL,				"MIC1N"},

	{"MIC2 PGA", NULL,				"MIC2 SRC"},

	{"MIC2 SRC", "MIC2",				"MIC2"},
	{"MIC2 SRC", "MIC3",				"MIC3"},

	/*AIF2 out */
	{"AIF2ADCL", NULL,				"AIF2OUTL Mux"},
	{"AIF2ADCR", NULL,				"AIF2OUTR Mux"},

	{"AIF2OUTL Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},
	{"AIF2OUTL Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},

	{"AIF2OUTR Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},
	{"AIF2OUTR Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},

	/*23*/
	{"AIF2 ADL Mixer", "AIF1 DA0L Switch",				"AIF1IN0L Mux"},
	{"AIF2 ADL Mixer", "AIF1 DA1L Switch",				"AIF1IN1L Mux"},
	{"AIF2 ADL Mixer", "AIF2 DACR Switch",				"AIF2INR_VIR"},
	{"AIF2 ADL Mixer", "ADCL Switch",				"ADCL Mux"},

	{"AIF2 ADR Mixer", "AIF1 DA0R Switch",				"AIF1IN0R Mux"},
	{"AIF2 ADR Mixer", "AIF1 DA1R Switch",				"AIF1IN1R Mux"},
	{"AIF2 ADR Mixer", "AIF2 DACL Switch",				"AIF2INL_VIR"},
	{"AIF2 ADR Mixer", "ADCR Switch",				"ADCR Mux"},

	/*aif2*/
	{"AIF2INL Mux", "AIF2_DACL",				"AIF2DACL"},
	{"AIF2INL Mux", "AIF2_DACR",				"AIF2DACR"},

	{"AIF2INR Mux", "AIF2_DACR",				"AIF2DACR"},
	{"AIF2INR Mux", "AIF2_DACL",				"AIF2DACL"},

	/*aif3*/
	{"AIF2INL Mux VIR switch", "aif2inl aif3",				"AIF3IN"},
	{"AIF2INR Mux VIR switch", "aif2inr aif3",				"AIF3IN"},

	{"AIF2INL Mux VIR", NULL,				"AIF2INL Mux VIR switch"},
	{"AIF2INR Mux VIR", NULL,				"AIF2INR Mux VIR switch"},

	{"AIF3OUT", NULL,				"AIF3OUT Mux"},
	{"AIF3OUT Mux", "AIF2 ADC left channel",				"AIF2 ADL Mixer"},
	{"AIF3OUT Mux", "AIF2 ADC right channel",				"AIF2 ADR Mixer"},

	/*ADC--ADCMUX*/
	{"ADCR Mux", "ADC", "RADC input Mixer"},
	{"ADCL Mux", "ADC", "LADC input Mixer"},

	/*DMIC*/
	{"ADCR Mux", "DMIC", "DMICR VIR"},
	{"ADCL Mux", "DMIC", "DMICL VIR"},

	{"DMICR VIR", NULL, "D_MIC"},
	{"DMICL VIR", NULL, "D_MIC"},
	{"External Speaker", NULL, "SPKL"},
	{"External Speaker", NULL, "SPKR"},

	{"PHONEIN PGA", NULL,"PHONEINP"},
	{"PHONEIN PGA", NULL,"PHONEINN"},

	{"LADC input Mixer", "PHONINP-PHONINN Switch","PHONEIN PGA"},
	{"LADC input Mixer", "PHONINP Switch","PHONEINP"},


	{"RADC input Mixer", "PHONINN-PHONINP Switch","PHONEIN PGA"},
	{"RADC input Mixer", "PHONINN Switch","PHONEINN"},

	{"Phoneout Mixer", "MIC1 boost Switch","MIC1 PGA"},
	{"Phoneout Mixer", "MIC2 boost Switch","MIC2 PGA"},
	{"Phoneout Mixer", "Rout_Mixer_Switch","Right Output Mixer"},
	{"Phoneout Mixer", "Lout_Mixer_Switch","Left Output Mixer"},

	{"PHONEOUTP", NULL,"Phoneout Mixer"},
	{"PHONEOUTN", NULL,"Phoneout Mixer"},

	{"Right Output Mixer", "PHONEINN Switch","PHONEINN"},
	{"Right Output Mixer", "PHONEINN-PHONEINP Switch","PHONEIN PGA"},

	{"Left Output Mixer", "PHONEINP Switch","PHONEINP"},
	{"Left Output Mixer", "PHONEINP-PHONEINN Switch","PHONEIN PGA"},
};

static int codec_start(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (sunxi_internal_codec->hwconfig.dacdrc_cfg)
			dacdrc_enable(codec, 1);
		if (sunxi_internal_codec->hwconfig.dachpf_cfg)
			dachpf_enable(codec, 1);
	} else {
		if (sunxi_internal_codec->hwconfig.adcagc_cfg)
			adcagc_enable(codec, 1);

		if (sunxi_internal_codec->hwconfig.adcdrc_cfg)
			adcdrc_enable(codec, 1);

		if (sunxi_internal_codec->hwconfig.adchpf_cfg)
			adchpf_enable(codec, 1);
	}

	return 0;

}
static int codec_aif_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (mute) {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL, 0);
	} else {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL, sunxi_internal_codec->gain_config.dac_digital_vol);
	}
	if(sunxi_internal_codec->spkenable == true)
		msleep(sunxi_internal_codec->pa_sleep_time);

	return 0;
}

static void codec_aif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (sunxi_internal_codec->hwconfig.dacdrc_cfg)
			dacdrc_enable(codec, 0);
		if (sunxi_internal_codec->hwconfig.dachpf_cfg)
			dachpf_enable(codec, 0);
	} else {
		if (sunxi_internal_codec->hwconfig.adcagc_cfg)
			adcagc_enable(codec, 0);

		if (sunxi_internal_codec->hwconfig.adcdrc_cfg)
			adcdrc_enable(codec, 0);

		if (sunxi_internal_codec->hwconfig.adchpf_cfg)
			adchpf_enable(codec, 0);
	}
}

static int codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	int AIF_CLK_CTRL = 0;
	int aif1_word_size = 16;
	int aif1_lrlk_div = 64;
	int bclk_div_factor = 0;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	switch (codec_dai->id) {
		case 1:
			AIF_CLK_CTRL = SUNXI_AIF1_CLK_CTRL;
			if (sunxi_internal_codec->aif1_lrlk_div == 0)
				aif1_lrlk_div = 64;
			else
				aif1_lrlk_div = sunxi_internal_codec->aif1_lrlk_div;
			break;
		case 2:
			AIF_CLK_CTRL = SUNXI_AIF2_CLK_CTRL;
			if (sunxi_internal_codec->aif2_lrlk_div == 0)
				aif1_lrlk_div = 64;
			else
				aif1_lrlk_div = sunxi_internal_codec->aif2_lrlk_div;
			break;
		default:
			return -EINVAL;
	}

	/* FIXME make up the codec_aif1_lrck factor
	 * adjust for more working scene
	 */
	switch (aif1_lrlk_div) {
	case	16:
		bclk_div_factor = 4;
		break;
	case	32:
		bclk_div_factor = 2;
		break;
	case	64:
		bclk_div_factor = 0;
		break;
	case	128:
		bclk_div_factor = -2;
		break;
	case	256:
		bclk_div_factor = -4;
		break;
	default:
		pr_err("invalid lrlk_div setting in sysconfig!\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].aif1_lrlk_div == aif1_lrlk_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x7<<AIF1_LRCK_DIV), ((codec_aif1_lrck[i].aif1_lrlk_bit)<<AIF1_LRCK_DIV));
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samplerate ==  params_rate(params)) {
			snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0xf<<AIF1_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF1_FS));
			snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0xf<<AIF2_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF2_FS));
			bclk_div_factor += codec_aif1_fs[i].aif1_bclk_div;
			snd_soc_update_bits(codec, AIF_CLK_CTRL,
						(0xf<<AIF1_BCLK_DIV),
				((bclk_div_factor)<<AIF1_BCLK_DIV));
			break;
		}
	}
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S32_LE:
			aif1_word_size = 24;
		break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif1_word_size = 16;
		break;
	}

	if(params_channels(params) == 1 && (AIF_CLK_CTRL == SUNXI_AIF2_CLK_CTRL))
		snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x1<<DSP_MONO_PCM), (0x1<<DSP_MONO_PCM));
	else
		snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x1<<DSP_MONO_PCM), (0x0<<DSP_MONO_PCM));

	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].aif1_wsize_val == aif1_word_size) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x3<<AIF1_WORD_SIZ), ((codec_aif1_wsize[i].aif1_wsize_bit)<<AIF1_WORD_SIZ));
			break;
		}
	}
	return 0;
}

static int codec_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (clk_id) {
		case AIF1_CLK:
			/*system clk from aif1*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_SRC), (0x0<<SYSCLK_SRC));
			break;
		case AIF2_CLK:
			/*system clk from aif2*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_SRC), (0x1<<SYSCLK_SRC));
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int codec_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	int AIF_CLK_CTRL = 0;
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (codec_dai->id) {
	case 1:
		AIF_CLK_CTRL = SUNXI_AIF1_CLK_CTRL;
		break;
	case 2:
		AIF_CLK_CTRL = SUNXI_AIF2_CLK_CTRL;
		break;
	default:
		return -EINVAL;
	}

	/*
	* 	master or slave selection
	*	0 = Master mode
	*	1 = Slave mode
	*/
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val &=~(0x1<<AIF1_MSTR_MOD);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val |= (0x0<<AIF1_MSTR_MOD);
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* i2s mode selection */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val&=~(3<<AIF1_DATA_FMT);
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val |= (0x0<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val |= (0x2<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val |= (0x1<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L reg_val msb after FRM LRC */
			reg_val |= (0x3<<AIF1_DATA_FMT);
			break;
		default:
			pr_err("%s, line:%d\n", __func__, __LINE__);
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	return 0;
}

static int codec_set_fll(struct snd_soc_dai *codec_dai, int pll_id, int source,
									unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	if (!freq_out)
		return 0;
	if ((freq_in < 128000) || (freq_in > 24576000)) {
		return -EINVAL;
	} else if ((freq_in == 24576000) || (freq_in == 22579200)) {
		switch (pll_id) {
			case PLLCLK:
				/*select aif1/aif2 clk source from pll*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF1CLK_SRC), (0x3<<AIF1CLK_SRC));
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF2CLK_SRC), (0x3<<AIF2CLK_SRC));
				break;
			case MCLK:
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF1CLK_SRC), (0x0<<AIF1CLK_SRC));
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF2CLK_SRC), (0x0<<AIF2CLK_SRC));
			default:
				return -EINVAL;
		}
		return 0;
	}

	return 0;
}

static int codec_aif3_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	struct snd_soc_codec *codec = codec_dai->codec;

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, SUNXI_AIF3_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
	}
	snd_soc_write(codec, SUNXI_AIF3_CLK_CTRL, reg_val);

	return 0;
}

static int codec_aif3_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int aif3_word_size = 0;
	int aif3_size =0;
	struct snd_soc_codec *codec = codec_dai->codec;

	/*0x2c0 config aif3clk from aif2clk*/
	snd_soc_update_bits(codec, SUNXI_AIF3_CLK_CTRL, (0x3<<AIF3_CLOC_SRC), (0x1<<AIF3_CLOC_SRC));
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
			aif3_word_size = 24;
			aif3_size = 3;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif3_word_size = 16;
			aif3_size = 1;
			break;
	}
	snd_soc_update_bits(codec, SUNXI_AIF3_CLK_CTRL, (0x3<<AIF3_WORD_SIZ), aif3_size<<AIF3_WORD_SIZ);

	return 0;
}

static int codec_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		pr_debug("%s,line:%d, SND_SOC_BIAS_ON\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_PREPARE:
		pr_debug("%s,line:%d, SND_SOC_BIAS_PREPARE\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_STANDBY:
		/*on*/
		//switch_hw_config(codec);
		pr_debug("%s,line:%d, SND_SOC_BIAS_STANDBY\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_OFF:
		/*off*/
		pr_debug("%s,line:%d, SND_SOC_BIAS_OFF\n", __func__, __LINE__);
		break;
	}
	codec->dapm.bias_level = level;

	return 0;
}

static const struct snd_soc_dai_ops codec_aif1_dai_ops = {
	.startup	= codec_start,
	.set_sysclk	= codec_set_dai_sysclk,
	.set_fmt	= codec_set_dai_fmt,
	.hw_params	= codec_hw_params,
	.shutdown	= codec_aif_shutdown,
	.digital_mute	= codec_aif_mute,
	.set_pll	= codec_set_fll,
};

static const struct snd_soc_dai_ops codec_aif2_dai_ops = {
	.set_sysclk	= codec_set_dai_sysclk,
	.set_fmt	= codec_set_dai_fmt,
	.hw_params	= codec_hw_params,
	//.shutdown	= codec_aif_shutdown,
	.set_pll	= codec_set_fll,
};

static const struct snd_soc_dai_ops codec_aif3_dai_ops = {
	.hw_params	= codec_aif3_hw_params,
	.set_fmt	= codec_aif3_set_dai_fmt,
};

static struct snd_soc_dai_driver codec_dai[] = {
	{
		.name = "codec-aif1",
		.id = 1,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		 },
		.ops = &codec_aif1_dai_ops,
	},
	{
		.name = "codec-aif2",
		.id = 2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.ops = &codec_aif2_dai_ops,
	},
	{
		.name = "codec-aif3",
		.id = 3,
		.playback = {
			.stream_name = "AIF3 Playback",
			.channels_min = 1,
			.channels_max = 1,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF3 Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		 },
		.ops = &codec_aif3_dai_ops,
	}
};

static int codec_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	sunxi_internal_codec->codec = codec;
	sunxi_internal_codec->dac_enable = 0;
	sunxi_internal_codec->adc_enable = 0;
	sunxi_internal_codec->aif1_clken = 0;
	sunxi_internal_codec->aif2_clken = 0;
	sunxi_internal_codec->aif3_clken = 0;
	mutex_init(&sunxi_internal_codec->dac_mutex);
	mutex_init(&sunxi_internal_codec->adc_mutex);
	mutex_init(&sunxi_internal_codec->aifclk_mutex);
	/* Add virtual switch */
	ret = snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret) {
		pr_err("[audio-codec] Failed to register audio mode control, "
				"will continue without it.\n");
	}
	snd_soc_dapm_new_controls(dapm, ac_dapm_widgets, ARRAY_SIZE(ac_dapm_widgets));
 	snd_soc_dapm_add_routes(dapm, ac_dapm_routes, ARRAY_SIZE(ac_dapm_routes));
	codec_init(sunxi_internal_codec);

	return 0;
}

int audio_gpio_iodisable(u32 gpio){
	char pin_name[8];
	u32 config,ret;
	sunxi_gpio_to_name(gpio, pin_name);
	config = (((7) << 16) | (0 & 0xFFFF));
	ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
	return ret;
}

static int codec_suspend(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);
	pr_debug("[audio codec]:suspend start.\n");

	if (sunxi_internal_codec->aif_config.aif3config) {
		ret = pinctrl_select_state(sunxi_internal_codec->pinctrl, sunxi_internal_codec->aif3sleep_pinstate);
		if (ret) {
			pr_warn("[audio-codec]select aif3-sleep state failed\n");
			return ret;
		}
	}
	if (sunxi_internal_codec->aif_config.aif2config) {
		ret = pinctrl_select_state(sunxi_internal_codec->pinctrl, sunxi_internal_codec->aif2sleep_pinstate);
		if (ret) {
			pr_warn("[audio-codec]select aif2-sleep state failed\n");
			return ret;
		}
	}

	if (sunxi_internal_codec->aif_config.aif2config || sunxi_internal_codec->aif_config.aif3config){
		devm_pinctrl_put(sunxi_internal_codec->pinctrl);
		sunxi_internal_codec->pinctrl = NULL;
		sunxi_internal_codec->aif3_pinstate = NULL;
		sunxi_internal_codec->aif2_pinstate = NULL;
		sunxi_internal_codec->aif3sleep_pinstate = NULL;
		sunxi_internal_codec->aif2sleep_pinstate = NULL;
	}
	if (spk_gpio.cfg) {
		audio_gpio_iodisable(spk_gpio.gpio);
	}

	if (sunxi_internal_codec->vol_supply.cpvdd){
		regulator_disable(sunxi_internal_codec->vol_supply.cpvdd);
	}

	if (sunxi_internal_codec->vol_supply.avcc) {
		regulator_disable(sunxi_internal_codec->vol_supply.avcc);
	}

	pr_debug("[audio codec]:suspend end..\n");

	return 0;
}

static int codec_resume(struct snd_soc_codec *codec)
{
	int ret ;
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	pr_debug("[audio codec]:resume start\n");
	if (sunxi_internal_codec->vol_supply.cpvdd){
		ret = regulator_enable(sunxi_internal_codec->vol_supply.cpvdd);
		if (ret) {
			pr_err("[%s]: cpvdd:regulator_enable() failed!\n",__func__);
		}
	}

	if (sunxi_internal_codec->vol_supply.avcc) {
		ret = regulator_enable(sunxi_internal_codec->vol_supply.avcc);
		if (ret) {
			pr_err("[%s]: avcc:regulator_enable() failed!\n",__func__);
		}
	}

	codec_init(sunxi_internal_codec);
	if (spk_gpio.cfg) {
		gpio_direction_output(spk_gpio.gpio, 1);
		gpio_set_value(spk_gpio.gpio, 0);
	}
	pr_debug("[audio codec]:resume end..\n");

	return 0;
}

/* power down chip */
static int codec_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static unsigned int codec_read(struct snd_soc_codec *codec,
					  unsigned int reg)
{
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (reg <= 0x1e) {
		/*analog reg*/
		return read_prcm_wvalue(reg,sunxi_internal_codec->codec_abase);
	} else {
		/*digital reg*/
		return codec_rdreg(sunxi_internal_codec->codec_dbase + reg);
	}
}

static int codec_write(struct snd_soc_codec *codec,
				  unsigned int reg, unsigned int value)
{
	struct sunxi_codec *sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (reg <= 0x1e) {
		/*analog reg*/
		write_prcm_wvalue(reg, value,sunxi_internal_codec->codec_abase);
	} else {
		/*digital reg*/
		codec_wrreg(sunxi_internal_codec->codec_dbase + reg, value);
	}
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_codec = {
	.probe 	 = codec_soc_probe,
	.remove  = codec_soc_remove,
	.suspend = codec_suspend,
	.resume  = codec_resume,
	.set_bias_level = codec_set_bias_level,
	.read 	 = codec_read,
	.write 	 = codec_write,
	.ignore_pmdown_time = 1,
};

static ssize_t show_audio_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int count = 0;
	int i = 0;
	int reg_group =0;

	count += sprintf(buf, "dump audio reg:\n");

	while (reg_labels[i].name != NULL){
		if (reg_labels[i].value == 0){
			reg_group++;
		}
		if (reg_group == 1){
			count +=sprintf(buf + count, "%s 0x%p: 0x%x\n", reg_labels[i].name,
							(codec_digitaladress + reg_labels[i].value),
							readl(codec_digitaladress + reg_labels[i].value) );
		} else 	if (reg_group == 2){
			count +=sprintf(buf + count, "%s 0x%x: 0x%x\n", reg_labels[i].name,
							(reg_labels[i].value),
						        read_prcm_wvalue(reg_labels[i].value,codec_analogadress) );
		}
		i++;
	}

	return count;
}

/* ex:
*param 1: 0 read;1 write
*param 2: 1 digital reg; 2 analog reg
*param 3: reg value;
*param 4: write value;
	read:
		echo 0,1,0x00> audio_reg
   		echo 0,2,0x00> audio_reg
	write:
		echo 1,1,0x00,0xa > audio_reg
		echo 1,2,0x00,0xff > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	int input_reg_val =0;
	int input_reg_group =0;
	int input_reg_offset =0;

	ret = sscanf(buf, "%d,%d,0x%x,0x%x", &rw_flag,&input_reg_group, &input_reg_offset, &input_reg_val);
	printk("ret:%d, reg_group:%d, reg_offset:%d, reg_val:0x%x\n", ret, input_reg_group, input_reg_offset, input_reg_val);

	if (!(input_reg_group ==1 || input_reg_group ==2)){
		pr_err("not exist reg group\n");
		ret = count;
		goto out;
	}
	if (!(rw_flag ==1 || rw_flag ==0)){
		pr_err("not rw_flag\n");
		ret = count;
		goto out;
	}
	if (input_reg_group == 1){
		if (rw_flag){
			writel(input_reg_val, codec_digitaladress + input_reg_offset);
		}else{
			reg_val_read = readl(codec_digitaladress + input_reg_offset);
			printk("\n\n Reg[0x%x] : 0x%x\n\n",input_reg_offset,reg_val_read);
		}
	} else if (input_reg_group == 2){
		if(rw_flag) {
			write_prcm_wvalue(input_reg_offset, input_reg_val & 0xff,codec_analogadress);
		}else{
			 reg_val_read = read_prcm_wvalue(input_reg_offset,codec_analogadress);
			 printk("\n\n Reg[0x%x] : 0x%x\n\n",input_reg_offset,reg_val_read);
		}
	}

	ret = count;

out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};
static const struct of_device_id sunxi_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static int sunxi_internal_codec_probe(struct platform_device *pdev)
{
	s32 ret = 0;
	u32 temp_val;
	struct gpio_config config;
	const struct of_device_id *device;
	struct sunxi_codec *sunxi_internal_codec;
	struct device_node *node = pdev->dev.of_node;

	if (!node) {
		dev_err(&pdev->dev,
			"can not get dt node for this device.\n");
		ret = -EINVAL;
		goto err0;
	}
	sunxi_internal_codec = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_codec), GFP_KERNEL);
	if (!sunxi_internal_codec) {
		dev_err(&pdev->dev, "Can't allocate sunxi_codec\n");
		ret = -ENOMEM;
		goto err0;
	}
	dev_set_drvdata(&pdev->dev, sunxi_internal_codec);
	device = of_match_device(sunxi_codec_of_match, &pdev->dev);
	if (!device) {
		ret = -ENODEV;
		goto err1;
	}

	/*voltage*/
	sunxi_internal_codec->vol_supply.cpvdd =  regulator_get(NULL, "vcc-cpvdd");
	if (!sunxi_internal_codec->vol_supply.cpvdd) {
		pr_err("get audio cpvdd failed\n");
		ret = -EFAULT;
		goto err1;
	}else{
		ret = regulator_enable(sunxi_internal_codec->vol_supply.cpvdd);
		if (ret) {
			pr_err("[%s]: cpvdd:regulator_enable() failed!\n",__func__);
			goto err1;
		}
	}

	sunxi_internal_codec->vol_supply.avcc = regulator_get(NULL, "vcc-avcc");
	if (!sunxi_internal_codec->vol_supply.avcc) {
		pr_err("[%s]:get audio avcc failed\n",__func__);
		ret = -EFAULT;
		goto err1;
	}else{
		ret = regulator_enable(sunxi_internal_codec->vol_supply.avcc);
		if (ret) {
			pr_err("[%s]: avcc:regulator_enable() failed!\n",__func__);
			goto err1;
		}
	}

	sunxi_internal_codec->codec_abase = NULL;
	sunxi_internal_codec->codec_dbase = NULL;
	sunxi_internal_codec->codec_dbase = of_iomap(node, 0);
	if (NULL == sunxi_internal_codec->codec_dbase) {
		pr_err("[audio-codec]Can't map codec digital registers\n");
	} else {
		codec_digitaladress = sunxi_internal_codec->codec_dbase;
	}
	sunxi_internal_codec->codec_abase = of_iomap(node, 1);
	if (NULL == sunxi_internal_codec->codec_abase) {
		pr_err("[audio-codec]Can't map codec analog registers\n");
	} else {
		codec_analogadress = sunxi_internal_codec->codec_abase;
	}
	sunxi_internal_codec->srcclk = of_clk_get(node, 0);
	if (IS_ERR(sunxi_internal_codec->srcclk)){
		dev_err(&pdev->dev, "[audio-codec]Can't get src clocks\n");
		ret = PTR_ERR(sunxi_internal_codec->srcclk);
		goto err1;
	}
	/*initial speaker gpio */
	spk_gpio.gpio = of_get_named_gpio_flags(node, "gpio-spk", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(spk_gpio.gpio)) {
		pr_err("failed to get gpio-spk gpio from dts,spk_gpio:%d\n",spk_gpio.gpio);
		spk_gpio.cfg = 0;
	} else {
		ret = devm_gpio_request(&pdev->dev, spk_gpio.gpio, "SPK");
		if (ret) {
			spk_gpio.cfg = 0;
			pr_err("failed to request gpio-spk gpio\n");
			goto err1;
		} else {
			spk_gpio.cfg = 1;
			gpio_direction_output(spk_gpio.gpio, 1);
			gpio_set_value(spk_gpio.gpio, 0);
		}
	}

	ret = of_property_read_u32(node, "headphonevol",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]headphonevol configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.headphonevol = temp_val;
	}
	ret = of_property_read_u32(node, "spkervol",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]spkervol configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.spkervol = temp_val;
	}

	ret = of_property_read_u32(node, "earpiecevol",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]earpiecevol configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.earpiecevol = temp_val;
	}

	ret = of_property_read_u32(node, "maingain",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]maingain configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.maingain = temp_val;
	}

	ret = of_property_read_u32(node, "headsetmicgain",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]headsetmicgain configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.headsetmicgain = temp_val;
	}

	ret = of_property_read_u32(node, "adcagc_cfg",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]adcagc_cfg configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hwconfig.adcagc_cfg = temp_val;
	}

	ret = of_property_read_u32(node, "adcdrc_cfg",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]adcdrc_cfg configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hwconfig.adcdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(node, "adchpf_cfg",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]adchpf_cfg configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hwconfig.adchpf_cfg = temp_val;
	}

	ret = of_property_read_u32(node, "dacdrc_cfg",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]dacdrc_cfg configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hwconfig.dacdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(node, "dachpf_cfg",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]dachpf_cfg configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hwconfig.dachpf_cfg = temp_val;
	}

	ret = of_property_read_u32(node, "aif2config",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]aif2config configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->aif_config.aif2config = temp_val;
	}

	ret = of_property_read_u32(node, "aif3config",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]aif3config configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->aif_config.aif3config = temp_val;
	}

	ret = of_property_read_u32(node, "aif1_lrlk_div",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]aif1_lrlk_div configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->aif1_lrlk_div = temp_val;
	}
	ret = of_property_read_u32(node, "aif2_lrlk_div",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]aif2_lrlk_div configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->aif2_lrlk_div = temp_val;
	}

	ret = of_property_read_u32(node, "pa_sleep_time",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]pa_sleep_time configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->pa_sleep_time = temp_val;
	}
	    ret = of_property_read_u32(node, "dac_digital_vol",&temp_val);
                if (ret < 0) {
                    pr_err("[audio-codec]dac_digital_vol configurations missing or invalid.\n");
                    ret = -EINVAL;
                    goto err1;
                } else {
                     sunxi_internal_codec->gain_config.dac_digital_vol = temp_val;
                }

	ret = of_property_read_u32(node, "dac_digital_vol",&temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]dac_digital_vol configurations missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.dac_digital_vol = temp_val;
	}

	pr_debug("headphonevol:%d,spkervol:%d, earpiecevol:%d maingain:%d headsetmicgain:%d \
			adcagc_cfg:%d, adcdrc_cfg:%d, adchpf_cfg:%d, dacdrc_cfg:%d \
			dachpf_cfg:%d,aif2config:%d,aif3config:%d,aif1_lrlk_div:%d,aif2_lrlk_div:%d,pa_sleep_time:%d,dac_digital_vol:%d\n",
		sunxi_internal_codec->gain_config.headphonevol,
		sunxi_internal_codec->gain_config.spkervol,
		sunxi_internal_codec->gain_config.earpiecevol,
		sunxi_internal_codec->gain_config.maingain,
		sunxi_internal_codec->gain_config.headsetmicgain,
		sunxi_internal_codec->hwconfig.adcagc_cfg,
		sunxi_internal_codec->hwconfig.adcdrc_cfg,
		sunxi_internal_codec->hwconfig.adchpf_cfg,
		sunxi_internal_codec->hwconfig.dacdrc_cfg,
		sunxi_internal_codec->hwconfig.dachpf_cfg,
		sunxi_internal_codec->aif_config.aif2config,
		sunxi_internal_codec->aif_config.aif3config,
		sunxi_internal_codec->aif1_lrlk_div,
		sunxi_internal_codec->aif2_lrlk_div,
		sunxi_internal_codec->pa_sleep_time,
		sunxi_internal_codec->gain_config.dac_digital_vol
	);

	snd_soc_register_codec(&pdev->dev, &soc_codec_dev_codec, codec_dai, ARRAY_SIZE(codec_dai));

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret){
		pr_err("[audio-codec]failed to create attr group\n");
	}
	return 0;

err1:
	devm_kfree(&pdev->dev, sunxi_internal_codec);
err0:
	return ret;
}

static int __exit sunxi_internal_codec_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static void sunxi_internal_codec_shutdown(struct platform_device *pdev)
{
	struct sunxi_codec * sunxi_internal_codec = dev_get_drvdata(&pdev->dev);

	snd_soc_update_bits(sunxi_internal_codec->codec, HP_CTRL, (0x1<<HPPA_EN), (0x0<<HPPA_EN));
	snd_soc_update_bits(sunxi_internal_codec->codec, MIX_DAC_CTRL, (0x3<<LHPPAMUTE), (0x0<<LHPPAMUTE));
	snd_soc_update_bits(sunxi_internal_codec->codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x0<<HMICBIASEN));
	if (sunxi_internal_codec->hp_en)
		clk_disable_unprepare(sunxi_internal_codec->hp_en);
	if (spk_gpio.cfg)
		gpio_set_value(spk_gpio.gpio, 0);
}

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_codec_of_match,
	},
	.probe = sunxi_internal_codec_probe,
	.remove = __exit_p(sunxi_internal_codec_remove),
	.shutdown = sunxi_internal_codec_shutdown,
};

module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("codec ALSA soc codec driver");
MODULE_AUTHOR("huanxin<huanxin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");

