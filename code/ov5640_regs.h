/*
 * ov5640_regs.h
 *
 *  Created on: 2026年5月18日
 *      Author: kout
 */

#ifndef OV5640_REGS_H
#define OV5640_REGS_H

/* ===== 系统控制寄存器 ===== */
#define OV5640_REG_SYSCTL      0x3008
#define OV5640_REG_PAD_OUT1    0x3017
#define OV5640_REG_PAD_OUT2    0x3018
#define OV5640_REG_PAD_OUT_EN1 0x3019
#define OV5640_REG_PAD_OUT_EN2 0x301C

/* ===== PLL 配置 ===== */
#define OV5640_REG_SC_PLL1     0x3034
#define OV5640_REG_SC_PLL2     0x3035
#define OV5640_REG_SC_PLL3     0x3036
#define OV5640_REG_SC_PLL4     0x3037

/* ===== 时序控制 ===== */
#define OV5640_REG_TIMING_HS_H   0x3800
#define OV5640_REG_TIMING_HS_L   0x3801
#define OV5640_REG_TIMING_VS_H   0x3802
#define OV5640_REG_TIMING_VS_L   0x3803
#define OV5640_REG_TIMING_HW_H   0x3804
#define OV5640_REG_TIMING_HW_L   0x3805
#define OV5640_REG_TIMING_VH_H   0x3806
#define OV5640_REG_TIMING_VH_L   0x3807
#define OV5640_REG_TIMING_DVPHO_H 0x3808
#define OV5640_REG_TIMING_DVPHO_L 0x3809
#define OV5640_REG_TIMING_DVPVO_H 0x380A
#define OV5640_REG_TIMING_DVPVO_L 0x380B
#define OV5640_REG_TIMING_HTS_H  0x380C
#define OV5640_REG_TIMING_HTS_L  0x380D
#define OV5640_REG_TIMING_VTS_H  0x380E
#define OV5640_REG_TIMING_VTS_L  0x380F

/* ===== 格式控制 ===== */
#define OV5640_REG_FMT_MUX_CTRL  0x501F
#define OV5640_REG_FMT_CTRL00    0x4300

/* ===== ISP 控制 ===== */
#define OV5640_REG_ISP_CTRL00    0x5000
#define OV5640_REG_ISP_CTRL01    0x5001

/* ===== AWB 控制 ===== */
#define OV5640_REG_AWB_CTRL      0x3406

/* ===== JPEG ===== */
#define OV5640_REG_JPEG_CTRL     0x4407

#endif /* OV5640_REGS_H */
