/*
 * ov5640.c
 *
 *  Created on: 2026年5月16日
 *      Author: kout
 */
#include "ov5640.h"
#include "ov5640_regs.h"
#include "ov5640_init_table.h"
#include <string.h>
#include <stdio.h>

/* ===== 外部引用 FSP 生成的实例 ===== */
extern const i2c_master_instance_t g_i2c_master0;

/* ===== 私有变量 ===== */
static volatile bool g_i2c_tx_complete = false;
static volatile bool g_i2c_rx_complete = false;
static volatile bool g_i2c_error = false;

static ov5640_frame_t g_frame = {0};
static volatile bool g_ceu_frame_done = false;



/* 捕获完成标志 */
bool g_capture_complete = false;

/* ===== I2C 回调 ===== */
void i2c_master0_callback(i2c_master_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case I2C_MASTER_EVENT_TX_COMPLETE:
            g_i2c_tx_complete = true;
            break;
        case I2C_MASTER_EVENT_RX_COMPLETE:
            g_i2c_rx_complete = true;
            break;
        case I2C_MASTER_EVENT_ABORTED:
        default:
            g_i2c_error = true;
            break;
    }
}

/* ===== CEU 回调 ===== */
void ceu_callback(capture_callback_args_t *p_args)
{
    if (CEU_EVENT_FRAME_END == p_args->event)
    {
        g_ceu_frame_done = true;
        g_frame.frame_ready = true;

        g_capture_complete= true;
    }
}

/* ===== I2C 等待完成 ===== */
static fsp_err_t i2c_wait_complete(uint32_t timeout_ms)
{
    uint32_t count = 0;
    uint32_t max_count = timeout_ms * 1000;

    while ((!g_i2c_tx_complete) && (!g_i2c_rx_complete) && (!g_i2c_error))
    {
        count++;
        if (count > max_count)
        {
            return FSP_ERR_TIMEOUT;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    }

    if (g_i2c_error)
    {
        return FSP_ERR_ABORTED;
    }

    return FSP_SUCCESS;
}

/* ===== OV5640 写寄存器 (SCCB 16-bit addr) ===== */
fsp_err_t ov5640_write_reg(uint16_t reg, uint8_t value)
{
    fsp_err_t err;
    uint8_t buf[3];

    buf[0] = (uint8_t)(reg >> 8);
    buf[1] = (uint8_t)(reg & 0xFF);
    buf[2] = value;

    g_i2c_tx_complete = false;
    g_i2c_rx_complete = false;
    g_i2c_error = false;

    err = g_i2c_master0.p_api->write(g_i2c_master0.p_ctrl, buf, 3, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = i2c_wait_complete(100);
    return err;
}

/* ===== OV5640 读寄存器 (SCCB 16-bit addr) ===== */

fsp_err_t ov5640_read_reg(uint16_t reg, uint8_t *value)
{
    fsp_err_t err;
    uint8_t reg_buf[2];

    reg_buf[0] = (uint8_t)(reg >> 8);
    reg_buf[1] = (uint8_t)(reg & 0xFF);

    g_i2c_tx_complete = false;
    g_i2c_rx_complete = false;
    g_i2c_error = false;

    err = g_i2c_master0.p_api->write(g_i2c_master0.p_ctrl, reg_buf, 2, true);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = i2c_wait_complete(100);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_i2c_tx_complete = false;
    g_i2c_rx_complete = false;
    g_i2c_error = false;

    err = g_i2c_master0.p_api->read(g_i2c_master0.p_ctrl, value, 1, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = i2c_wait_complete(100);
    return err;
}

/* ===== OV5640 写多个寄存器 ===== */
static fsp_err_t ov5640_write_reg_array(const ov5640_reg_t *reg_list, uint32_t size)
{
    fsp_err_t err;

    for (uint32_t i = 0; i < size; i++)
    {
        if (reg_list[i].reg == OV5640_REG_DELAY)
        {
            R_BSP_SoftwareDelay(reg_list[i].val, BSP_DELAY_UNITS_MILLISECONDS);
        }
        else
        {
            err = ov5640_write_reg(reg_list[i].reg, reg_list[i].val);
            if (FSP_SUCCESS != err)
            {
                return err;
            }
        }
    }

    return FSP_SUCCESS;
}

/* ===== OV5640 软复位 ===== */
static fsp_err_t ov5640_soft_reset(void)
{
    fsp_err_t err;

    err = ov5640_write_reg(OV5640_REG_SYS_RESET, 0x80);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);

    return FSP_SUCCESS;
}

/* ===== OV5640 检查芯片 ID (公开接口) ===== */
fsp_err_t ov5640_check_chip_id(uint16_t *chip_id)
{
    fsp_err_t err;
    uint8_t id_h = 0, id_l = 0;

    err = ov5640_read_reg(OV5640_REG_CHIP_ID_H, &id_h);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = ov5640_read_reg(OV5640_REG_CHIP_ID_L, &id_l);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    uint16_t id = ((uint16_t)id_h << 8) | id_l;

    if (NULL != chip_id)
    {
        *chip_id = id;
    }

    if (id != OV5640_CHIP_ID)
    {
        return FSP_ERR_NOT_FOUND;
    }

    return FSP_SUCCESS;
}

/* ===== OV5640 设置输出窗口 ===== */
fsp_err_t ov5640_set_window(uint16_t x_offset, uint16_t y_offset,
                             uint16_t width, uint16_t height)
{
    fsp_err_t err;

    err = ov5640_write_reg(0x3808, (uint8_t)(width >> 8));
    err |= ov5640_write_reg(0x3809, (uint8_t)(width & 0xFF));
    err |= ov5640_write_reg(0x380A, (uint8_t)(height >> 8));
    err |= ov5640_write_reg(0x380B, (uint8_t)(height & 0xFF));

    err |= ov5640_write_reg(0x3810, (uint8_t)(x_offset >> 8));
    err |= ov5640_write_reg(0x3811, (uint8_t)(x_offset & 0xFF));
    err |= ov5640_write_reg(0x3812, (uint8_t)(y_offset >> 8));
    err |= ov5640_write_reg(0x3813, (uint8_t)(y_offset & 0xFF));

    return err;
}

/* ===== OV5640 设置输出格式 ===== */
fsp_err_t ov5640_set_output_format(ov5640_output_format_t format)
{
    fsp_err_t err;

    switch (format)
    {
        case OV5640_FORMAT_RGB565:
            err = ov5640_write_reg_array(ov5640_rgb565_reg_table,
                  sizeof(ov5640_rgb565_reg_table) / sizeof(ov5640_rgb565_reg_table[0]));
            break;
        case OV5640_FORMAT_YUV422:
            err  = ov5640_write_reg(0x501F, 0x00);
            err |= ov5640_write_reg(0x4300, 0x30);
            break;
        case OV5640_FORMAT_JPEG:
            err  = ov5640_write_reg(0x501F, 0x00);
            err |= ov5640_write_reg(0x4300, 0x30);
            break;
        default:
            err = FSP_ERR_INVALID_ARGUMENT;
            break;
    }

    return err;
}

/* ===== OV5640 设置分辨率 ===== */
fsp_err_t ov5640_set_resolution(ov5640_resolution_t resolution)
{
    fsp_err_t err = FSP_SUCCESS;

    switch (resolution)
    {
        case OV5640_RES_QVGA:
            err = ov5640_set_window(0, 0, 320, 240);
            g_frame.width = 320;
            g_frame.height = 240;
            break;
        case OV5640_RES_VGA:
            err = ov5640_set_window(0, 0, 640, 480);
            g_frame.width = 640;
            g_frame.height = 480;
            break;
        case OV5640_RES_720P:
            err = ov5640_set_window(0, 0, 1280, 720);
            g_frame.width = 1280;
            g_frame.height = 720;
            break;
        case OV5640_RES_1080P:
            err = ov5640_set_window(0, 0, 1920, 1080);
            g_frame.width = 1920;
            g_frame.height = 1080;
            break;
        case OV5640_RES_QSXGA:
            err = ov5640_set_window(0, 0, 2592, 1944);
            g_frame.width = 2592;
            g_frame.height = 1944;
            break;
        default:
            err = FSP_ERR_INVALID_ARGUMENT;
            break;
    }

    return err;
}

/* ===== OV5640 设置自动对焦 ===== */
fsp_err_t ov5640_auto_focus_init(void)
{
    fsp_err_t err;
    uint8_t state = 0;
    uint32_t retry = 0;

    err = ov5640_write_reg(0x3022, 0x00);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    while (retry < 100)
    {
        err = ov5640_read_reg(0x3029, &state);
        if (FSP_SUCCESS != err)
        {
            return err;
        }
        if (state == 0x70)
        {
            break;
        }
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        retry++;
    }

    if (retry >= 100)
    {
        return FSP_ERR_TIMEOUT;
    }

    return FSP_SUCCESS;
}

/* ===== OV5640 触发单次自动对焦 ===== */
fsp_err_t ov5640_auto_focus_single(void)
{
    fsp_err_t err;
    uint8_t state = 0;
    uint32_t retry = 0;

    err = ov5640_write_reg(0x3022, 0x03);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    while (retry < 200)
    {
        err = ov5640_read_reg(0x3029, &state);
        if (FSP_SUCCESS != err)
        {
            return err;
        }
        if (state == 0x10)
        {
            break;
        }
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        retry++;
    }

    if (retry >= 200)
    {
        return FSP_ERR_TIMEOUT;
    }

    return FSP_SUCCESS;
}

/* ===== OV5640 设置亮度 ===== */
fsp_err_t ov5640_set_brightness(int8_t level)
{
    fsp_err_t err;
    uint8_t val;

    if (level >= 0)
    {
        val = (uint8_t)level;
        err = ov5640_write_reg(0x5587, val << 4);
        err |= ov5640_write_reg(0x5588, 0x01);
    }
    else
    {
        val = (uint8_t)(-level);
        err = ov5640_write_reg(0x5587, val << 4);
        err |= ov5640_write_reg(0x5588, 0x09);
    }

    return err;
}

/* ===== OV5640 设置对比度 ===== */
fsp_err_t ov5640_set_contrast(uint8_t level)
{
    fsp_err_t err;

    err = ov5640_write_reg(0x5585, level);
    err |= ov5640_write_reg(0x5586, level);

    return err;
}

/* ===== OV5640 设置饱和度 ===== */
fsp_err_t ov5640_set_saturation(uint8_t level)
{
    fsp_err_t err;

    err = ov5640_write_reg(0x5583, level);
    err |= ov5640_write_reg(0x5584, level);

    return err;
}

/* ===== OV5640 设置测试图案 ===== */
fsp_err_t ov5640_set_test_pattern(bool enable)
{
    if (enable)
    {
        return ov5640_write_reg(0x503D, 0x80);
    }
    else
    {
        return ov5640_write_reg(0x503D, 0x00);
    }
}
//自动曝光
fsp_err_t ov5640_set_aec_auto(void)
{
    fsp_err_t err;
    err  = ov5640_write_reg(0x3503, 0x00);
    return err;
}

/* ===== OV5640 初始化 ===== */
fsp_err_t ov5640_init(void)
{
    fsp_err_t err;

    /* 打开 I2C */
    err = g_i2c_master0.p_api->open(g_i2c_master0.p_ctrl, g_i2c_master0.p_cfg);
    if (FSP_SUCCESS != err && FSP_ERR_ALREADY_OPEN != err)
    {
        return err;
    }

    /* 设置从机地址 */
    err = g_i2c_master0.p_api->slaveAddressSet(g_i2c_master0.p_ctrl,
                                                 OV5640_I2C_ADDR,
                                                 I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 硬件复位延时 */
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);

    /* 软复位 */
    err = ov5640_soft_reset();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 检查芯片 ID */
    err = ov5640_check_chip_id(NULL);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 写入初始化寄存器表 */
    err = ov5640_write_reg_array(ov5640_init_reg_table,
                                  sizeof(ov5640_init_reg_table) / sizeof(ov5640_init_reg_table[0]));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 默认设置: VGA, RGB565 */
    err = ov5640_set_resolution(OV5640_RES_VGA);
    if (FSP_SUCCESS != err) return err;

    err = ov5640_set_output_format(OV5640_FORMAT_RGB565);
    if (FSP_SUCCESS != err) return err;

    /* 初始化帧信息 */
    g_frame.format = OV5640_FORMAT_RGB565;
    g_frame.frame_ready = false;

    return FSP_SUCCESS;
}
fsp_err_t ov5640_stream_on(void) {
    return ov5640_write_reg(0x3008, 0x02);
}
/* ===== OV5640 反初始化 ===== */
fsp_err_t ov5640_deinit(void)
{
    fsp_err_t err;

    err = ov5640_write_reg(OV5640_REG_SYS_RESET, 0x40);

    g_i2c_master0.p_api->close(g_i2c_master0.p_ctrl);

    return err;
}

/* ===== CEU 启动采集 ===== */
fsp_err_t ov5640_capture_start(uint8_t *p_buffer, uint32_t buffer_size)
{
    (void)buffer_size;

    if (NULL == p_buffer)
    {
        return FSP_ERR_INVALID_POINTER;
    }

    g_frame.buffer = p_buffer;
    g_frame.frame_ready = false;
    g_ceu_frame_done = false;

    return FSP_SUCCESS;
}

/* ===== 等待帧完成 ===== */
fsp_err_t ov5640_capture_wait(uint32_t timeout_ms)
{
    uint32_t count = 0;
    uint32_t max_count = timeout_ms * 10;

    while (!g_ceu_frame_done)
    {
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);
        count++;
        if (count > max_count)
        {
            return FSP_ERR_TIMEOUT;
        }
    }

    return FSP_SUCCESS;
}

/* ===== 获取帧信息 ===== */
ov5640_frame_t *ov5640_get_frame_info(void)
{
    return &g_frame;
}

/* ===== OV5640 设置白平衡/光照模式 ===== */
fsp_err_t ov5640_set_light_mode(uint8_t mode)
{
    fsp_err_t err;

    switch (mode)
    {
        case 0: /* Auto */
            err = ov5640_write_reg(0x3212, 0x03);
            err |= ov5640_write_reg(0x3406, 0x00);
            err |= ov5640_write_reg(0x3212, 0x13);
            err |= ov5640_write_reg(0x3212, 0xA3);
            break;
        case 1: /* Sunny */
            err = ov5640_write_reg(0x3212, 0x03);
            err |= ov5640_write_reg(0x3406, 0x01);
            err |= ov5640_write_reg(0x3400, 0x06);
            err |= ov5640_write_reg(0x3401, 0x1C);
            err |= ov5640_write_reg(0x3402, 0x04);
            err |= ov5640_write_reg(0x3403, 0x00);
            err |= ov5640_write_reg(0x3404, 0x04);
            err |= ov5640_write_reg(0x3405, 0xF3);
            err |= ov5640_write_reg(0x3212, 0x13);
            err |= ov5640_write_reg(0x3212, 0xA3);
            break;
        case 2: /* Cloudy */
            err = ov5640_write_reg(0x3212, 0x03);
            err |= ov5640_write_reg(0x3406, 0x01);
            err |= ov5640_write_reg(0x3400, 0x06);
            err |= ov5640_write_reg(0x3401, 0x48);
            err |= ov5640_write_reg(0x3402, 0x04);
            err |= ov5640_write_reg(0x3403, 0x00);
            err |= ov5640_write_reg(0x3404, 0x04);
            err |= ov5640_write_reg(0x3405, 0xD3);
            err |= ov5640_write_reg(0x3212, 0x13);
            err |= ov5640_write_reg(0x3212, 0xA3);
            break;
        case 3: /* Office (fluorescent) */
            err = ov5640_write_reg(0x3212, 0x03);
            err |= ov5640_write_reg(0x3406, 0x01);
            err |= ov5640_write_reg(0x3400, 0x05);
            err |= ov5640_write_reg(0x3401, 0x48);
            err |= ov5640_write_reg(0x3402, 0x04);
            err |= ov5640_write_reg(0x3403, 0x00);
            err |= ov5640_write_reg(0x3404, 0x07);
            err |= ov5640_write_reg(0x3405, 0xCF);
            err |= ov5640_write_reg(0x3212, 0x13);
            err |= ov5640_write_reg(0x3212, 0xA3);
            break;
        case 4: /* Home (incandescent) */
            err = ov5640_write_reg(0x3212, 0x03);
            err |= ov5640_write_reg(0x3406, 0x01);
            err |= ov5640_write_reg(0x3400, 0x04);
            err |= ov5640_write_reg(0x3401, 0x10);
            err |= ov5640_write_reg(0x3402, 0x04);
            err |= ov5640_write_reg(0x3403, 0x00);
            err |= ov5640_write_reg(0x3404, 0x08);
            err |= ov5640_write_reg(0x3405, 0x40);
            err |= ov5640_write_reg(0x3212, 0x13);
            err |= ov5640_write_reg(0x3212, 0xA3);
            break;
        default:
            err = FSP_ERR_INVALID_ARGUMENT;
            break;
    }

    return err;
}
/* ===== 查询帧就绪状态 ===== */
bool ov5640_is_frame_ready(void)
{
    return g_frame.frame_ready;
}

/* ===== 清除帧就绪标志 ===== */
void ov5640_clear_frame_flag(void)
{
    g_frame.frame_ready = false;
    g_ceu_frame_done = false;
}
