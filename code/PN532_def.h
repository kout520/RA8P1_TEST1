/**
 * PN532 硬件适配层定义 (RA8P1 / I2C2)
 */

#ifndef PN532_DEF_H_
#define PN532_DEF_H_

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// 布尔类型定义
#ifndef BOOL
#define BOOL uint8_t
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// PN532 I2C 地址 (7-bit 地址)
#ifndef PN532_I2C_ADDRESS
#define PN532_I2C_ADDRESS    0x24
#endif

// 硬件抽象接口函数
BOOL PN532_I2C_Transmit(uint8_t *data, uint16_t len, uint32_t timeout);
BOOL PN532_I2C_Receive(uint8_t *data, uint16_t len, uint32_t timeout);
uint32_t PN532_GetTick(void);
void PN532_Init_Hardware(void);

#endif /* PN532_DEF_H_ */
