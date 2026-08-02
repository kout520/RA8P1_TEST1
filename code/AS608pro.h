#ifndef __AS608pro_H
#define __AS608pro_H

#define AS608_START_STATE   1   /* 0: 不编译模块  1: 编译模块 */
#if AS608_START_STATE
#include "hal_data.h"
#include <stdio.h>
#include <string.h>

/************************************ 引脚宏定义 *******************************************************/

#define PS_Sta  R_BSP_PinRead(BSP_IO_PORT_08_PIN_07)  /* 指纹触摸检测引脚 */


#define CharBuffer1 0x01  /* 特征缓冲区 1 */
#define CharBuffer2 0x02  /* 特征缓冲区 2 */

extern uint32_t AS608Addr;  /* 模块地址 */

/* 搜索结果结构体 */
typedef struct
{
    uint16_t pageID;     /* 指纹 ID */
    uint16_t mathscore;  /* 匹配得分 */
}SearchResult;

/* 系统参数结构体 */
typedef struct
{
    uint16_t PS_max;     /* 指纹最大容量 */
    uint8_t  PS_level;   /* 安全等级 */
    uint32_t PS_addr;    /* 模块地址 */
    uint8_t  PS_size;    /* 通讯数据包大小 */
    uint8_t  PS_N;       /* 指纹库中已录入指纹数 N */
}SysPara;

/* ===== 公共接口 ===== */
void AS608_Check(void);                     /* 检测并连接 AS608 模块 */
void AS608_Init(void);                     /* 初始化 AS608 */
void finger_addtion(void);                 /* 录入指纹(交互式) */
void finger_search(void);                  /* 刷指纹验证 */

uint8_t AS608_AddFR(void);                 /* 录入指纹流程 */
uint8_t AS608_PressFR(uint16_t *matched_id);/* 刷指纹 (返回 0:未松开 1:匹配成功 2:匹配失败 3:无指纹) */
uint8_t AS608_DeleteFR(uint16_t delete_id);/* 删除指纹 (0xFFFF=全部删除) */
uint8_t AS608_GetFRNumber(void);           /* 获取已录入指纹数 */

/* ===== 底层协议接口 ===== */
extern bsp_io_level_t level;

void SendHead(void);                       /* 发送包头 EF01 */

uint8_t PS_GetImage(void);                 /* 录入图像 */

uint8_t PS_GenChar(uint8_t BufferID);      /* 生成特征 */

uint8_t PS_Match(void);                    /* 精确比对两枚指纹特征 */

uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage,
                  uint16_t PageNum, SearchResult *p);  /* 搜索指纹 */

uint8_t PS_RegModel(void);                 /* 合并特征(生成模板) */

uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID);  /* 储存模板 */

uint8_t PS_DeletChar(uint16_t PageID, uint16_t N);  /* 删除模板 */

uint8_t PS_Empty(void);                    /* 清空指纹库 */

uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA);  /* 写系统寄存器 */

uint8_t PS_ReadSysPara(SysPara *p);        /* 读系统基本参数 */

uint8_t PS_SetAddr(uint32_t addr);         /* 设置模块地址 */

uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *content);  /* 写记事本 */

uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *note);  /* 读记事本 */

uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage,
                           uint16_t PageNum, SearchResult *p);  /* 高速搜索 */

uint8_t PS_ValidTempleteNum(uint16_t *ValidN);  /* 读有效模板个数 */

uint8_t PS_HandShake(uint32_t *PS_Addr);   /* 与 AS608 模块握手 */

void timeout_search_1ms(void);             /* 1ms 超时检测(定时器中断中调用) */


#endif

#endif
