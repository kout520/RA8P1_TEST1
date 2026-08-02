/**
 * PN532 NFC 模块驱动 - 头文件 (I2C2)
 * 支持: MIFARE Classic 1K/4K, NTAG2xx
 */
#ifndef PN532_PN532_H_
#define PN532_PN532_H_

#include "PN532_def.h"  // 硬件适配层
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

// ---------------------------- HARDWARE -----------------------------

// I2C 设备句柄 (RA8P1 FSP)
#ifndef PN532_I2C_HANDLE
#define PN532_I2C_HANDLE                g_i2c2
#endif

#ifndef PN532_I2C_ADDR
#define PN532_I2C_ADDR                  0x24  // 7-bit 地址
#endif

// 发送数据到 PN532
#ifndef PN532_TRANSMIT
#define PN532_TRANSMIT(b, s, t)         PN532_I2C_Transmit(b, s, t)
#endif

// 从 PN532 接收数据
#ifndef PN532_RECEIVE
#define PN532_RECEIVE(b, s, t)          PN532_I2C_Receive(b, s, t)
#endif

// 延时函数 (毫秒)
#ifndef PN532_DELAY
#define PN532_DELAY(ms)                 R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS)
#endif

// 获取系统 tick (毫秒)
#ifndef PN532_GET_TICK
#define PN532_GET_TICK()                PN532_GetTick()
#endif

// 计算 tick 差值
#ifndef PN532_TICK_DIFF
#define PN532_TICK_DIFF(t)              ((PN532_GET_TICK() >= t) ? (PN532_GET_TICK() - t) : (0xFFFFFFFF - t + PN532_GET_TICK()))
#endif

// ---------------------------- SOFTWARE -----------------------------

#ifndef BOOL
#define BOOL uint8_t
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// 发送缓冲区大小
#ifndef PN532_SEND_BUF_SIZE
#define PN532_SEND_BUF_SIZE             64
#endif

// 接收缓冲区大小
#ifndef PN532_RECV_BUF_SIZE
#define PN532_RECV_BUF_SIZE             64
#endif

// 发送超时 (ms)
#ifndef PN532_SEND_TIMEOUT
#define PN532_SEND_TIMEOUT              100
#endif

// 等待 Ready 超时 (ms)
#ifndef PN532_RECV_READY_TIMEOUT
#define PN532_RECV_READY_TIMEOUT        25
#endif

// 接收数据超时 (ms)
#ifndef PN532_RECV_DATA_TIMEOUT
#define PN532_RECV_DATA_TIMEOUT         15
#endif

// ------ PN532 帧格式定义 ------
#define PN532_I2C_READY                     (0x01)

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

// PN532 命令码
#define PN532_COMMAND_DIAGNOSE              (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define PN532_COMMAND_READREGISTER          (0x06)
#define PN532_COMMAND_WRITEREGISTER         (0x08)
#define PN532_COMMAND_READGPIO              (0x0C)
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define PN532_COMMAND_SETPARAMETERS         (0x12)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_POWERDOWN             (0x16)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define PN532_COMMAND_INJUMPFORDEP          (0x56)
#define PN532_COMMAND_INJUMPFORPSL          (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INATR                 (0x50)
#define PN532_COMMAND_INPSL                 (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define PN532_COMMAND_INDESELECT            (0x44)
#define PN532_COMMAND_INRELEASE             (0x52)
#define PN532_COMMAND_INSELECT              (0x54)
#define PN532_COMMAND_INAUTOPOLL            (0x60)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGSETMETADATA         (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)

#define PN532_RESPONSE_INDATAEXCHANGE       (0x41)
#define PN532_RESPONSE_INLISTPASSIVETARGET  (0x4B)

#define PN532_WAKEUP                        (0x55)

#define PN532_MIFARE_ISO14443A              (0x00)

// MIFARE Classic 命令
#define MIFARE_CMD_AUTH_A                   (0x60)
#define MIFARE_CMD_AUTH_B                   (0x61)
#define MIFARE_CMD_READ                     (0x30)
#define MIFARE_CMD_WRITE                    (0xA0)
#define MIFARE_CMD_TRANSFER                 (0xB0)
#define MIFARE_CMD_DECREMENT                (0xC0)
#define MIFARE_CMD_INCREMENT                (0xC1)
#define MIFARE_CMD_STORE                    (0xC2)
#define MIFARE_ULTRALIGHT_CMD_WRITE         (0xA2)

#define MIFARE_UID_MAX_LENGTH               MIFARE_UID_TRIPLE_LENGTH
#define MIFARE_UID_SINGLE_LENGTH            (4)
#define MIFARE_UID_DOUBLE_LENGTH            (7)
#define MIFARE_UID_TRIPLE_LENGTH            (10)
#define MIFARE_KEY_LENGTH                   (6)
#define MIFARE_BLOCK_LENGTH                 (16)

// NTAG2xx 命令
#define NTAG2XX_BLOCK_LENGTH                (4)

// NDEF URI 前缀定义
#define NDEF_URIPREFIX_NONE                 (0x00)
#define NDEF_URIPREFIX_HTTP_WWWDOT          (0x01)
#define NDEF_URIPREFIX_HTTPS_WWWDOT         (0x02)
#define NDEF_URIPREFIX_HTTP                 (0x03)
#define NDEF_URIPREFIX_HTTPS                (0x04)
#define NDEF_URIPREFIX_TEL                  (0x05)
#define NDEF_URIPREFIX_MAILTO               (0x06)
#define NDEF_URIPREFIX_FTP_ANONAT           (0x07)
#define NDEF_URIPREFIX_FTP_FTPDOT           (0x08)
#define NDEF_URIPREFIX_FTPS                 (0x09)
#define NDEF_URIPREFIX_SFTP                 (0x0A)
#define NDEF_URIPREFIX_SMB                  (0x0B)
#define NDEF_URIPREFIX_NFS                  (0x0C)
#define NDEF_URIPREFIX_FTP                  (0x0D)
#define NDEF_URIPREFIX_DAV                  (0x0E)
#define NDEF_URIPREFIX_NEWS                 (0x0F)
#define NDEF_URIPREFIX_TELNET               (0x10)
#define NDEF_URIPREFIX_IMAP                 (0x11)
#define NDEF_URIPREFIX_RTSP                 (0x12)
#define NDEF_URIPREFIX_URN                  (0x13)
#define NDEF_URIPREFIX_POP                  (0x14)
#define NDEF_URIPREFIX_SIP                  (0x15)
#define NDEF_URIPREFIX_SIPS                 (0x16)
#define NDEF_URIPREFIX_TFTP                 (0x17)
#define NDEF_URIPREFIX_BTSPP                (0x18)
#define NDEF_URIPREFIX_BTL2CAP              (0x19)
#define NDEF_URIPREFIX_BTGOEP               (0x1A)
#define NDEF_URIPREFIX_TCPOBEX              (0x1B)
#define NDEF_URIPREFIX_IRDAOBEX             (0x1C)
#define NDEF_URIPREFIX_FILE                 (0x1D)
#define NDEF_URIPREFIX_URN_EPC_ID           (0x1E)
#define NDEF_URIPREFIX_URN_EPC_TAG          (0x1F)
#define NDEF_URIPREFIX_URN_EPC_PAT          (0x20)
#define NDEF_URIPREFIX_URN_EPC_RAW          (0x21)
#define NDEF_URIPREFIX_URN_EPC              (0x22)
#define NDEF_URIPREFIX_URN_NFC              (0x23)

#define PN532_GPIO_VALIDATIONBIT            (0x80)

// PN532 参数位定义
#define PN532_PARAM_NADUSED_BIT                (0)
#define PN532_PARAM_DIDUSED_BIT                (1)
#define PN532_PARAM_AUTOMATIC_ATR_BIT          (2)
#define PN532_PARAM_AUTOMATIC_RATS_BIT         (4)
#define PN532_PARAM_ISO14443_4_BIT             (5)
#define PN532_PARAM_REMOVE_PRE_POST_AMBLE_BIT  (6)

// ------ PN532 错误码定义 ------
#define PN532_ERROR_NONE                                                (0x00)
#define PN532_ERROR_TIMEOUT                                             (0x01)  // 目标无应答
#define PN532_ERROR_CRC                                                 (0x02)  // CRC 错误
#define PN532_ERROR_PARITY                                              (0x03)  // 奇偶校验错误
#define PN532_ERROR_COLLISION_BITCOUNT                                  (0x04)  // 冲突检测位计数错误
#define PN532_ERROR_MIFARE_FRAMING                                      (0x05)  // MIFARE 帧错误
#define PN532_ERROR_COLLISION_BITCOLLISION                              (0x06)  // 位冲突
#define PN532_ERROR_NOBUFS                                              (0x07)  // 缓冲区不足
#define PN532_ERROR_RFNOBUFS                                            (0x09)  // RF 缓冲区溢出
#define PN532_ERROR_ACTIVE_TOOSLOW                                      (0x0A)  // 主动模式响应慢
#define PN532_ERROR_RFPROTO                                             (0x0B)  // RF 协议错误
#define PN532_ERROR_TOOHOT                                              (0x0D)  // 温度过高
#define PN532_ERROR_INTERNAL_NOBUFS                                     (0x0E)  // 内部缓冲区溢出
#define PN532_ERROR_INVAL                                               (0x10)  // 无效参数
#define PN532_ERROR_DEP_INVALID_COMMAND                                 (0x12)  // DEP 无效命令
#define PN532_ERROR_DEP_BADDATA                                         (0x13)  // DEP 数据错误
#define PN532_ERROR_MIFARE_AUTH                                         (0x14)  // MIFARE 认证错误
#define PN532_ERROR_NOSECURE                                            (0x18)  // 不支持 NFC 安全
#define PN532_ERROR_I2CBUSY                                             (0x19)  // I2C 忙
#define PN532_ERROR_UIDCHECKSUM                                         (0x23)  // UID 校验错误
#define PN532_ERROR_DEPSTATE                                            (0x25)  // DEP 状态错误
#define PN532_ERROR_HCIINVAL                                            (0x26)  // HCI 无效
#define PN532_ERROR_CONTEXT                                             (0x27)  // 上下文错误
#define PN532_ERROR_RELEASED                                            (0x29)  // 目标已释放
#define PN532_ERROR_CARDSWAPPED                                         (0x2A)  // 卡片被更换
#define PN532_ERROR_NOCARD                                              (0x2B)  // 无卡片
#define PN532_ERROR_MISMATCH                                            (0x2C)  // NFCID3 不匹配
#define PN532_ERROR_OVERCURRENT                                         (0x2D)  // 过流
#define PN532_ERROR_NONAD                                               (0x2E)  // DEP 帧缺少 NAD

// 自定义错误码
#define PN532_ERROR_SEND_DATA                                           (0xE1)  // 发送数据失败
#define PN532_ERROR_RECV_ACK                                            (0xE2)  // 接收 ACK 失败
#define PN532_ERROR_RECV_DATA                                           (0xE3)  // 接收数据失败
#define PN532_ERROR_RECV_COMMAND                                        (0xE4)  // 命令错误
#define PN532_ERROR_MORE_ONE_CARD                                       (0xE5)  // 检测到多张卡
#define PN532_ERROR_CARD_LONG_UID                                       (0xE6)  // UID 过长
#define PN532_ERROR_RECV_TOHOST                                         (0xE7)  // 未收到 ToHost 帧
#define PN532_ERROR_FRAMING_LEN                                         (0xE8)  // 帧长度错误

// -------------------------------------------------------------------------
// 以下为阻塞式 API (所有函数返回 PN532_ERROR_NONE 表示成功)
// -------------------------------------------------------------------------

void PN532_INIT(void);
void PN532_connect(void);
void g_system_tick(void);

// 配置 SAM (Secure Access Module)
uint8_t PN532_SetSamConfig(uint8_t mode, uint8_t timeout, uint8_t use_irq);
// 获取固件版本
uint8_t PN532_GetFirmVersion(uint8_t * version);
// 读取被动目标 ID (uid 缓冲区, uid_size 其大小, uid_len 读取长度)
uint8_t PN532_ReadPassTarget(uint8_t card_baud, uint8_t * uid, uint8_t uid_size, uint8_t *uid_len);
// MIFARE Classic 认证块
uint8_t PN532_MifareClassicAuthBlock(uint8_t *uid, uint8_t uid_len,  uint8_t block_number,  uint8_t key_number, uint8_t * key);
// 读取 16 字节数据块
uint8_t PN532_ReadDataBlock(uint8_t block_number, uint8_t * block_data);
// 写入 16 字节数据块
uint8_t PN532_WriteDataBlock(uint8_t block_number,  uint8_t * block_data);
// 设置参数
uint8_t PN532_SetParameters(uint8_t params);

// NTAG2xx 认证
uint8_t PN532_Ntag2xxAuth(uint8_t * pwd, uint8_t* response);
// NTAG2xx 读取 4 字节页
uint8_t PN532_Ntag2xxReadBlock(uint8_t block_number, uint8_t * block_data);
// NTAG2xx 写入 4 字节页
uint8_t PN532_Ntag2xxWriteBlock(uint8_t block_number, uint8_t * block_data);

// -------------------------------------------------------------------------
// 以下为非阻塞式 API (用于状态机)
// -------------------------------------------------------------------------

BOOL PN532_SendSamConfig(uint8_t mode, uint8_t timeout, uint8_t use_irq);
BOOL PN532_SendGetFirmwareVersion(void);
BOOL PN532_SendReadPassiveTarget(uint8_t card_baud);
BOOL PN532_SendMifareClassicAuthBlock(uint8_t *uid, uint8_t uid_len,  uint8_t block_number,  uint8_t key_number, uint8_t * key);
BOOL PN532_SendReadDataBlock(uint8_t block_number);
BOOL PN532_SendWriteDataBlock(uint8_t block_number,  uint8_t * block_data);
BOOL PN532_SendSetParameters(uint8_t params);
BOOL PN532_SendNtag2xxAuth(uint8_t * pwd);
BOOL PN532_SendNtag2xxReadBlock(uint8_t block_number);
BOOL PN532_SendNtag2xxWriteBlock(uint8_t block_number, uint8_t * block_data);

// -------------------------------------------------------------------------
// PN532 底层通信
// -------------------------------------------------------------------------

// 获取 PN532 接收缓冲区指针
uint8_t * get_PN532_recv_buf(void);
// 发送数据到 PN532 (含帧封装)
BOOL PN532_send_data(uint8_t *data, uint8_t data_len);
// 等待 PN532_I2C_READY 字节
BOOL PN532_WaitReady(uint32_t timeout);
// 从 PN532 获取数据
uint8_t PN532_recv(uint8_t * buf, uint8_t how);
// 获取 ACK
BOOL PN532_recv_ack(void);
// 解析 PN532 帧，返回数据指针和数据长度 data_len
uint8_t * PN532_parse_frame(uint8_t * buf, uint8_t buf_size, uint8_t * data_len);
// 从 PN532 获取数据并解析帧
uint8_t * PN532_recv_data(uint8_t * buf, uint8_t buf_size, uint8_t * data_len);

#endif /* PN532_PN532_H_ */
