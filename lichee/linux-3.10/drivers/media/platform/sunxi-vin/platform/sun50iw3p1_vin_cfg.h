
/*
 ******************************************************************************
 *
 * sun50iw1p1_vfe_cfg.h
 *
 * Hawkview ISP - sun50iw1p1_vfe_cfg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/07/24	      Second Version
 *
 ******************************************************************************
 */

#ifndef _SUN50IW3P1_VIN_CFG_H_
#define _SUN50IW3P1_VIN_CFG_H_

#define CSI0_REGS_BASE          		0x06621000
#define CSI1_REGS_BASE          		0x06621000
#define CSI0_CCI_REG_BASE			0x0662e000
#define VIPP0_REGS_BASE          		0x02101000
#define VIPP1_REGS_BASE				0x02101400

#define ISP_REGS_BASE           		0x00000000
#define GPIO_REGS_VBASE				0x0300b000
#define CPU_DRAM_PADDR_ORG 			0x40000000
#define HW_DMA_OFFSET				0x00000000

/*set vin core clk base on sensor size*/
#define CORE_CLK_RATE_FOR_2M (108*1000*1000)
#define CORE_CLK_RATE_FOR_3M (216*1000*1000)
#define CORE_CLK_RATE_FOR_5M (216*1000*1000)
#define CORE_CLK_RATE_FOR_8M (432*1000*1000)
#define CORE_CLK_RATE_FOR_16M (432*1000*1000)

/*CSI & ISP size configs*/

#define CSI0_REG_SIZE			0x1000
#define MIPI_CSI_REG_SIZE		0x1000
#define MIPI_DPHY_REG_SIZE		0x1000
#define CSI0_CCI_REG_SIZE		0x1000
#define CSI1_REG_SIZE			0x1000
#define CSI1_CCI_REG_SIZE		0x1000
#define ISP_REG_SIZE			0x1000
#define ISP_LOAD_REG_SIZE		0x1000
#define ISP_SAVED_REG_SIZE		0x1000
#define VIPP0_REG_SIZE			0x400
#define VIPP1_REG_SIZE			0x400

/*ISP size configs*/

/*stat size configs*/

#define ISP_STAT_TOTAL_SIZE         0xAB00

#define ISP_STAT_HIST_MEM_SIZE      0x0200
#define ISP_STAT_AE_MEM_SIZE        0x4800
#define ISP_STAT_AF_MEM_SIZE        0x0500
#define ISP_STAT_AFS_MEM_SIZE       0x0200
#define ISP_STAT_AWB_RGB_MEM_SIZE   0x4800
#define ISP_STAT_AWB_CNT_MEM_SIZE   0x0C00
#define ISP_STAT_PLTM_LST_MEM_SIZE  0x0600

#define ISP_STAT_HIST_MEM_OFS       0x0000
#define ISP_STAT_AE_MEM_OFS         0x0200
#define ISP_STAT_AF_MEM_OFS         0x4a00
#define ISP_STAT_AFS_MEM_OFS        0x4f00
#define ISP_STAT_AWB_MEM_OFS        0x5100
#define ISP_STAT_PLTM_LST_MEM_OFS   0xa500

/*table size configs*/

#define ISP_LINEAR_LUT_LENS_GAMMA_MEM_SIZE 0x1000
#define ISP_LUT_MEM_SIZE            0x0400
#define ISP_LENS_MEM_SIZE           0x0600
#define ISP_GAMMA_MEM_SIZE          0x0200
#define ISP_LINEAR_MEM_SIZE         0x0

#define ISP_DRC_DISC_MEM_SIZE       0x0200
#define ISP_DRC_MEM_SIZE            0x0200
#define ISP_DISC_MEM_SIZE           0

#define MAX_CH_NUM      4
#define MAX_ISP_STAT_BUF  5	/*the max num of isp statistic buffer*/
#define MAX_DETECT_NUM 3	/*the max num of detect sensor on the same bus*/

#endif /*_SUN50IW3P1_VIN_CFG_H_*/
