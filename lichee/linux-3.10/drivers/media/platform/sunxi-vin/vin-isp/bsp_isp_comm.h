
/*
 ******************************************************************************
 *
 * bsp_isp_comm.h
 *
 * Hawkview ISP - bsp_isp_comm.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/12/25	    First Version
 *
 ******************************************************************************
 */

#ifndef __BSP__ISP__COMM__H
#define __BSP__ISP__COMM__H

#define ALIGN_4K(x)	(((x) + (4095)) & ~(4095))
#define ALIGN_32B(x)	(((x) + (31)) & ~(31))
#define ALIGN_16B(x)	(((x) + (15)) & ~(15))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a, i, s)	(((a) > (s)) ? (s) : MAX(a, i))

enum isp_platform {
	ISP_PLATFORM_SUN8IW1P1 = 0,
	ISP_PLATFORM_SUN8IW3P1,
	ISP_PLATFORM_SUN9IW1P1,
	ISP_PLATFORM_SUN8IW5P1,
	ISP_PLATFORM_SUN8IW6P1,
	ISP_PLATFORM_SUN8IW7P1,
	ISP_PLATFORM_SUN8IW8P1,
	ISP_PLATFORM_SUN8IW9P1,
	ISP_PLATFORM_SUN8IW12P1,

	ISP_PLATFORM_SUN50IW1P1,
	ISP_PLATFORM_NUM,
};
/*
 *  update table
 */
#define LUT_UPDATE	(1 << 3)
#define LINEAR_UPDATE	(1 << 3)
#define LENS_UPDATE	(1 << 4)
#define GAMMA_UPDATE	(1 << 5)
#define DRC_UPDATE	(1 << 6)
#define DISC_UPDATE	(1 << 7)
#define SATU_UPDATE	(1 << 8)
#define WDR_UPDATE	(1 << 9)
#define TDNF_UPDATE	(1 << 10)
#define PLTM_UPDATE	(1 << 11)
#define CEM_UPDATE	(1 << 12)

#define TABLE_UPDATE_ALL 0xffffffff

/*
 *  ISP Module enable
 */
#define ISP_AE_EN	(1 << 0)
#define OBC_EN		(1 << 1)
#define LC_EN		(1 << 1)
#define LUT_DPC_EN	(1 << 2)
#define OTF_DPC_EN	(1 << 3)
#define BDNF_EN		(1 << 4)
#define ISP_AWB_EN	(1 << 6)
#define WB_EN		(1 << 7)
#define LSC_EN		(1 << 8)
#define BGC_EN		(1 << 9)
#define SAP_EN		(1 << 10)
#define AF_EN		(1 << 11)
#define RGB2RGB_EN	(1 << 12)
#define RGB_DRC_EN	(1 << 13)
#define TDF_EN		(1 << 15)
#define AFS_EN		(1 << 16)
#define HIST_EN		(1 << 17)
#define YCbCr_GAIN_OFFSET_EN	(1 << 18)
#define YCbCr_DRC_EN	(1 << 19)
#define TG_EN		(1 << 20)
#define ROT_EN		(1 << 21)
#define CONTRAST_EN	(1 << 22)
#define CNR_EN		(1 << 23)
#define SATU_EN		(1 << 24)
#define DISC_EN		(1 << 25)
#define SRC1_EN		(1 << 30)
#define SRC0_EN		(1 << 31)
#define ISP_MODULE_EN_ALL	(0xffffffff)

/*
 *  ISP interrupt enable
 */
#define FINISH_INT_EN		(1 << 0)
#define START_INT_EN		(1 << 1)
#define PARA_SAVE_INT_EN	(1 << 2)
#define PARA_LOAD_INT_EN	(1 << 3)
#define SRC0_FIFO_INT_EN	(1 << 4)
#define SRC1_FIFO_INT_EN	(1 << 5)
#define DISC_FIFO_INT_EN	(1 << 5)
#define ROT_FINISH_INT_EN	(1 << 6)
#define N_LINE_START_INT_EN	(1 << 7)
#define FRAME_ERROR_INT_EN	(1 << 8)
#define FRAME_LOST_INT_EN	(1 << 14)

#define ISP_IRQ_EN_ALL	0xffffffff

/*
 *  ISP interrupt status
 */
#define FINISH_PD	(1 << 0)
#define START_PD	(1 << 1)
#define PARA_SAVE_PD	(1 << 2)
#define PARA_LOAD_PD	(1 << 3)
#define SRC0_FIFO_OF_PD	(1 << 4)
#define N_LINE_START_PD	(1 << 7)
#define FRAME_ERROR_PD	(1 << 8)
#define CIN_FIFO_OF_PD	(1 << 9)
#define DPC_FIFO_OF_PD	(1 << 10)
#define D2D_FIFO_OF_PD	(1 << 11)
#define BIS_FIFO_OF_PD	(1 << 12)
#define CNR_FIFO_OF_PD	(1 << 13)
#define FRAME_LOST_PD	(1 << 14)
#define D3D_HB_PD	(1 << 24)
#define PLTM_FIFO_OF_PD	(1 << 25)
#define D3D_WRITE_FIFO_OF_PD	(1 << 26)
#define D3D_READ_FIFO_OF_PD	(1 << 27)
#define D3D_WT2CMP_FIFO_OF_PD	(1 << 28)
#define WDR_WRITE_FIFO_OF_PD	(1 << 29)
#define WDR_WT2CMP_FIFO_OF_PD	(1 << 30)
#define WDR_READ_FIFO_OF_PD	(1 << 31)

#define ISP_IRQ_STATUS_ALL	0xffffffff

enum isp_channel {
	MAIN_CH = 0,
	SUB_CH = 1,
	ROT_CH = 2,
	ISP_MAX_CH_NUM,
};

struct isp_size {
	unsigned int width;
	unsigned int height;
};

struct coor {
	unsigned int hor;
	unsigned int ver;
};
struct isp_size_settings {
	struct coor ob_start;
	struct isp_size ob_black_size;
	struct isp_size ob_valid_size;
	struct isp_size full_size;
	struct isp_size scale_size;
	struct isp_size ob_rot_size;
};

struct isp_int_status {
	unsigned char finish_int;
	unsigned char start_int;
	unsigned char para_save_int;
	unsigned char para_load_int;
	unsigned char src0_fifo_int;
	unsigned char src1_fifo_int;
	unsigned char rot_finish_int;
};

enum ready_flag {
	PARA_NOT_READY = 0,
	PARA_READY = 1,
};

enum enable_flag {
	DISABLE = 0,
	ENABLE = 1,
};

enum isp_src_interface {
	ISP_DRAM = 0,
	ISP_CSI0 = 1,
	ISP_CSI1 = 2,
	ISP_CSI2 = 3,
};

enum isp_input_tables {
	LUT_LENS_GAMMA_TABLE = 0,
	DRC_TABLE = 1,
};

enum isp_capture_mode {
	SCAP_EN = 0,
	VCAP_EN = 1,
};

enum isp_src {
	ISP_SRC0 = 0,
	ISP_SRC1 = 1,
};

enum isp_hist_mode {
	AVG_MODE = 0,
	MIN_MODE = 1,
	MAX_MODE = 2,
};

enum isp_bndf_mode {
	NORM_MODE = 0,
	STRONG_MODE = 1,
};

enum isp_rgb_drc_mode {
	INDEPEND = 0,
	COMBINED = 1,
};

enum isp_lut_dpc_mode {
	HOR = 0,
	VER = 1,
	HOR_VER = 2,
};

enum isp_input_fmt {
	ISP_YUV420 = 0,
	ISP_YUV422 = 1,
	ISP_RAW = 2,
};

enum isp_input_seq {
	ISP_YUYV = 0,
	ISP_YVYU = 1,
	ISP_UYVY = 2,
	ISP_VYUY = 3,

	ISP_BGGR = 4,
	ISP_RGGB = 5,
	ISP_GBRG = 6,
	ISP_GRBG = 7,
};

enum isp_rot_angle {
	ANGLE_0 = 0,
	ANGLE_90 = 1,
	ANGLE_180 = 2,
	ANGLE_270 = 3,
};

enum isp_scale_mode {
	ORIGINAL = 0,
	RATIO_1_2 = 2,
	RATIO_1_4 = 3,
};

enum isp_output_fmt {
	ISP_YUV420_SP = 0,
	ISP_YUV422_SP = 1,
	ISP_YUV420_P = 2,
	ISP_YUV422_P = 3,
};

enum isp_output_seq {
	ISP_UV = 0,
	ISP_VU = 1,
};

enum isp_thumb_sel {
	NORM = 0,
	THUMB = 1,
};

enum isp_output_speed {
	ISP_OUTPUT_SPEED_0 = 0,
	ISP_OUTPUT_SPEED_1 = 1,
	ISP_OUTPUT_SPEED_2 = 2,
	ISP_OUTPUT_SPEED_3 = 3,
};

struct isp_denoise {
	unsigned char in_dis_min;
	unsigned char in_dis_max;
	int denoise_table[12];
};

struct isp_bayer_gain_offset {
	unsigned short r_gain;
	unsigned short gr_gain;
	unsigned short gb_gain;
	unsigned short b_gain;

	short r_offset;
	short gr_offset;
	short gb_offset;
	short b_offset;
};

struct isp_yuv_gain_offset {
	unsigned short y_gain;
	unsigned short u_gain;
	unsigned short v_gain;

	short y_offset;
	short u_offset;
	short v_offset;
};

struct isp_white_balance_gain {
	unsigned short r_gain;
	unsigned short gr_gain;
	unsigned short gb_gain;
	unsigned short b_gain;
};

/**
 * struct isp_rgb2rgb_gain_offset - RGB to RGB Blending
 * @matrix:
 *              [RR] [GR] [BR]
 *              [RG] [GG] [BG]
 *              [RB] [GB] [BB]
 * @offset: Blending offset value for R,G,B.
 */
struct isp_rgb2rgb_gain_offset {
	short matrix[3][3];
	short offset[3];
};

struct isp_lsc_config {
	unsigned short ct_x;
	unsigned short ct_y;
	unsigned short rs_val;
};

struct isp_disc_config {
	unsigned short disc_ct_x;
	unsigned short disc_ct_y;
	unsigned short disc_rs_val;
};

struct isp_awb_avp_stat {
	unsigned int avp_r;
	unsigned int avp_g;
	unsigned int avp_b;
	unsigned int avp_cnt;
};

struct isp_wb_diff_threshold {
	unsigned int diff_th1;
	unsigned int diff_th2;
};

struct isp_h3a_reg_win {
	unsigned char hor_num;
	unsigned char ver_num;
	unsigned int width;
	unsigned int height;
	unsigned int hor_start;
	unsigned int ver_start;
};

struct isp_h3a_coor_win {
	int x1;
	int y1;
	int x2;
	int y2;
};

struct isp_yuv_channel_addr {
	unsigned long y_addr;
	unsigned long u_addr;
	unsigned long v_addr;
};

struct isp_3d_denoise_config {
	unsigned short filter_ch_sel;
	unsigned short filter_slope0;
	unsigned short filter_slope1;
	unsigned short y_core0_val;
	unsigned short y_core1_val;
	unsigned short uv_core0_val;
	unsigned short uv_core1_val;
};

#endif
