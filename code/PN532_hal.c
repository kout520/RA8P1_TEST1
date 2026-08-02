/**
 * PN532 硬件适配层 - RA8P1 FSP I2C2 实现
 */
#include "PN532.h"
#include "hal_data.h"
#include <stdio.h>

extern const i2c_master_instance_t g_i2c2;
extern const uart_instance_t g_uart7;

static volatile uint8_t g_pn532_i2c_tx_complete = 0;
static volatile uint8_t g_pn532_i2c_rx_complete = 0;
static volatile uint32_t g_system_tick_ms = 0;
static volatile fsp_err_t g_pn532_i2c_last_error = FSP_SUCCESS;

// 非安全UART打印函数(调试用, 通过 UART7 输出)
static void debug_print(const char *msg)
{
    uint16_t len = strlen(msg);
    g_uart7.p_api->write(g_uart7.p_ctrl, (uint8_t*)msg, len);
    // 等待发送完成
    R_BSP_SoftwareDelay(len / 10 + 2, BSP_DELAY_UNITS_MILLISECONDS);
}

// PN532 专用 I2C 回调 (区别于 OV5640 的 i2c_master0_callback)
// 注意: 需要在 FSP 配置中将此函数绑定到 g_i2c2 的中断
void pn532_i2c_callback(i2c_master_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case I2C_MASTER_EVENT_TX_COMPLETE:
            g_pn532_i2c_tx_complete = 1;
            g_pn532_i2c_last_error = FSP_SUCCESS;
            break;

        case I2C_MASTER_EVENT_RX_COMPLETE:
            g_pn532_i2c_rx_complete = 1;
            g_pn532_i2c_last_error = FSP_SUCCESS;
            break;

        case I2C_MASTER_EVENT_ABORTED:
            g_pn532_i2c_tx_complete = 1;
            g_pn532_i2c_rx_complete = 1;
            g_pn532_i2c_last_error = FSP_ERR_ABORTED;
            break;

        default:
            break;
    }
}

// 系统 tick 递增 (在定时器中断中调用, 每1ms)
void g_system_tick(void)
{
    g_system_tick_ms++;
}

uint32_t PN532_GetTick(void)
{
    return g_system_tick_ms;
}

// I2C 发送 (阻塞式, 带超时)
BOOL PN532_I2C_Transmit(uint8_t *data, uint16_t len, uint32_t timeout)
{
    fsp_err_t err;
    uint32_t start_tick = PN532_GetTick();

    g_pn532_i2c_tx_complete = 0;
    g_pn532_i2c_last_error = FSP_SUCCESS;

    // 设置从机地址 (PN532: 0x24)
    err = g_i2c2.p_api->slaveAddressSet(g_i2c2.p_ctrl, PN532_I2C_ADDRESS, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err)
    {
        return FALSE;
    }

    // 发送数据
    err = g_i2c2.p_api->write(g_i2c2.p_ctrl, data, len, false);
    if (FSP_SUCCESS != err)
    {
        return FALSE;
    }

    // 等待发送完成
    while (!g_pn532_i2c_tx_complete)
    {
        if (PN532_TICK_DIFF(start_tick) > timeout)
        {
            return FALSE;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }

    if (g_pn532_i2c_last_error != FSP_SUCCESS)
    {
        return FALSE;
    }

    return TRUE;
}

// I2C 接收 (阻塞式, 带超时)
BOOL PN532_I2C_Receive(uint8_t *data, uint16_t len, uint32_t timeout)
{
    fsp_err_t err;
    uint32_t start_tick = PN532_GetTick();

    g_pn532_i2c_rx_complete = 0;
    g_pn532_i2c_last_error = FSP_SUCCESS;

    // 设置从机地址
    err = g_i2c2.p_api->slaveAddressSet(g_i2c2.p_ctrl, PN532_I2C_ADDRESS, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err)
    {
        return FALSE;
    }

    // 读取数据
    err = g_i2c2.p_api->read(g_i2c2.p_ctrl, data, len, false);
    if (FSP_SUCCESS != err)
    {
        return FALSE;
    }

    // 等待接收完成
    while (!g_pn532_i2c_rx_complete)
    {
        if (PN532_TICK_DIFF(start_tick) > timeout)
        {
            return FALSE;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }

    if (g_pn532_i2c_last_error != FSP_SUCCESS)
    {
        return FALSE;
    }

    // 读取后加短暂延时，让 PN532 准备下一次通信
    R_BSP_SoftwareDelay(2, BSP_DELAY_UNITS_MILLISECONDS);

    return TRUE;
}

// 初始化 PN532 硬件 (打开 I2C2)
void PN532_Init_Hardware(void)
{
    fsp_err_t err;

    // 打开 I2C2 (PN532)
    err = g_i2c2.p_api->open(g_i2c2.p_ctrl, g_i2c2.p_cfg);
    if (FSP_SUCCESS != err)
    {
        printf("PN532 I2C2 open failed!\r\n");
        while(1);
    }

    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    printf("PN532 I2C2 hardware initialized\r\n");
}
