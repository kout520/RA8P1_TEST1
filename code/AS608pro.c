/**
 * AS608 指纹识别模块驱动 (USART2)
 * 通信协议: 串口 57600bps, 8N1, 半双工(命令-应答)
 */
#include "AS608pro.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include "headfile.h"
#if AS608_START_STATE

/* ===== 工具宏 ===== */
#define U8(x)  ((uint8_t)(x))
#define U16(x) ((uint16_t)(x))
#define U32(x) ((uint32_t)(x))

/*
 * 接收状态寄存器 (16bit)
 * bit15     : 接收完成标志
 * bit14     : 接收到 0x0D
 * bit13~0   : 接收到的有效字节数
 */
uint32_t AS608Addr = 0XFFFFFFFF;
uint16_t AS608_USART_RX_STA = 0;
uint8_t AS608_USART_RX_BUF[400];

char str1[100];
int aa;
bsp_io_level_t level = BSP_IO_LEVEL_LOW;
static volatile uint8_t g_uart2_tx_complete = 0;
static volatile uint8_t g_uart2_rx_complete = 0;
static volatile uint32_t rx_timeout_counter = 0;

/* ==================================================================
 *                        应用层函数
 * ================================================================== */

// 交互式录入指纹
void finger_addtion(void)
{
    // 等待 5 秒，检测是否要录入指纹
    bool add_mode = false;
    for(uint8_t i = 0; i < 5; i++)  // 5x1s = 5 秒
    {
        if(PS_Sta)  // 检测到手指
        {
            add_mode = true;
            break;
        }
        printf("waiting for finger...\r\n");
        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    }

    if(add_mode)
    {
        sprintf(str1, "t0.txt=\"请按指纹\"");
        tjc_send_string(str1);

        printf(">>> Starting fingerprint enrollment <<<\r\n\r\n");

        // 第一次按指纹
        printf("Step 1: Place your finger on sensor...\r\n");

        bool first_success = false;
        for(uint8_t i = 0; i < 10; i++)  // 最多等待 10 次
        {
            uint8_t result = AS608_AddFR();

            if(result == 0x01)  // 录入成功
            {
                printf(">>> Fingerprint enrolled successfully! <<<\r\n\r\n");
                first_success = true;
                break;
            }
            else if(result == 0x00)  // 录入失败
            {
                printf(">>> Enrollment failed! Please try again. <<<\r\n\r\n");
                break;
            }
            // result == 0x02 表示未检测到手指，继续等待

            R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);
        }

        if(!first_success)
        {
            printf(">>> Enrollment timeout or failed! <<<\r\n\r\n");
        }

        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    }
    else
    {
        printf(">>> Skipping enrollment, entering verification mode <<<\r\n\r\n");
    }
}

// 刷指纹验证
void finger_search(void)
{
    uint16_t matched_id = 0;
    uint8_t result1 = AS608_PressFR(&matched_id);

    switch(result1)
    {
        case 1:  // 验证成功
            printf(">>> Fingerprint MATCHED! ID=%d <<<\r\n\r\n", matched_id);
            // 根据匹配 ID 执行不同操作
            if (matched_id == 1) {

            } else if (matched_id == 2) {

            } else if (matched_id == 3) {

            } else {
                printf("Hello User %d\r\n", matched_id);
            }
            break;

        case 2:  // 验证失败
            printf("Fingerprint verification failed\r\n");
            R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
            break;

        case 0:  // 手指未松开
        case 3:  // 无手指
        default:
            // 不处理，等待下次刷指纹
            break;
    }

    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
}

/* ==================================================================
 *                      UART2 回调 (AS608)
 * ================================================================== */

// USART2 中断回调函数 (AS608 指纹模块专用)
void uart2_callback(uart_callback_args_t * p_args)
{
    level = !level;
    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE: // 发送完成
        {
            g_uart2_tx_complete = 1;
            break;
        }

        case UART_EVENT_RX_CHAR: // 接收字符(逐个中断, 需要使能)
        {
            uint8_t res = (uint8_t)p_args->data;

            if((AS608_USART_RX_STA & (1<<15)) == 0) {
                if(AS608_USART_RX_STA < 400) {
                    rx_timeout_counter = 100;  // 100ms 超时
                    AS608_USART_RX_BUF[AS608_USART_RX_STA++] = res;
                } else {
                    AS608_USART_RX_STA |= 1 << 15;
                }
            }
            break;
        }

        case UART_EVENT_RX_COMPLETE: // 接收完成(DMA 传输到指定长度后)
        {
            g_uart2_rx_complete = 1;
            break;
        }

        case UART_EVENT_ERR_PARITY:   // 奇偶校验错误
        case UART_EVENT_ERR_FRAMING:  // 帧错误
        {
            break;
        }

        default:
        {
            break;
        }
    }
}

// 1ms 超时检测回调(在定时器中断中调用)
void timeout_search_1ms(void)
{
    if(rx_timeout_counter > 0) {
        rx_timeout_counter--;
        if(rx_timeout_counter == 0 && AS608_USART_RX_STA > 0) {
            AS608_USART_RX_STA |= 1 << 15;  // 标记接收完成
        }
    }
}

/* ==================================================================
 *                      底层协议函数
 * ================================================================== */

// 初始化 AS608 模块
void AS608_Init(void)
{
    fsp_err_t err;

    // 打开 UART2 (AS608 通信)
    err = g_uart2.p_api->open(g_uart2.p_ctrl, g_uart2.p_cfg);
    if(FSP_SUCCESS != err)
    {
        return;
    }

    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    AS608_Check();
}

// 阻塞发送一个字节
static void MYUSART_SendData(uint8_t data) {
    g_uart2_tx_complete = 0;
    g_uart2.p_api->write(g_uart2.p_ctrl, &data, 1);
    while(!g_uart2_tx_complete);  // 等待发送完成
}

// 发送包头: EF 01
void SendHead(void)
{
    MYUSART_SendData(0xEF);
    MYUSART_SendData(0x01);
}

// 发送模块地址 (4 bytes, 大端)
static void SendAddr(void)
{
    MYUSART_SendData(AS608Addr>>24);
    MYUSART_SendData(AS608Addr>>16);
    MYUSART_SendData(AS608Addr>>8);
    MYUSART_SendData(AS608Addr);
}

// 发送包标识
static void SendFlag(uint8_t flag)
{
    MYUSART_SendData(flag);
}

// 发送包长度 (2 bytes, 大端)
static void SendLength(int length)
{
    MYUSART_SendData(length>>8);
    MYUSART_SendData(length);
}

// 发送指令码
static void Sendcmd(uint8_t cmd)
{
    MYUSART_SendData(cmd);
}

// 发送校验和 (2 bytes, 大端)
static void SendCheck(uint16_t check)
{
    MYUSART_SendData(check>>8);
    MYUSART_SendData(check);
}

// 判断中断接收的数据是否有应答包
// waittime: 等待超时时间 (单位 1ms)
// 返回值: 数据包首地址 (NULL=超时)
static uint8_t *JudgeStr(uint16_t waittime)
{
    char *data;
    uint8_t str[8];
    str[0]=0xEF;                    str[1]=0x01;
    str[2]=AS608Addr>>24;          str[3]=AS608Addr>>16;
    str[4]=AS608Addr>>8;           str[5]=AS608Addr;
    str[6]=0x07;                   str[7]='\0';
    AS608_USART_RX_STA=0;
    while(--waittime)
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        if(AS608_USART_RX_STA & 0X8000)  // 接收到一个完整包
        {
            AS608_USART_RX_STA=0;
            data=strstr((const char*)AS608_USART_RX_BUF,(const char*)str);
            if(data)
                return (uint8_t*)data;
        }
    }
    return 0;
}

// ===================== AS608 协议指令实现 =====================

// 录入图像
// 功能: 探测手指，探测到后录入指纹图像存于 ImageBuffer
uint8_t PS_GetImage(void)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);         // 命令包标识
    SendLength(0x03);
    Sendcmd(0x01);
    temp = 0x01+0x03+0x01;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 生成特征
// 功能: 将 ImageBuffer 中的原始图像生成指纹特征文件存于 CharBuffer1 或 CharBuffer2
// 参数: BufferID --> CharBuffer1:0x01   CharBuffer2:0x02
uint8_t PS_GenChar(uint8_t BufferID)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x04);
    Sendcmd(0x02);
    MYUSART_SendData(BufferID);
    temp = 0x01+0x04+0x02+BufferID;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 精确比对两枚指纹特征
// 功能: 精确比对 CharBuffer1 与 CharBuffer2 中的特征文件
uint8_t PS_Match(void)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x03);
    temp = 0x01+0x03+0x03;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 搜索指纹
// 功能: 以 CharBuffer1 或 CharBuffer2 中的特征文件搜索整个或部分指纹库
// 参数: BufferID, StartPage(起始页), PageNum(页数)
// 说明: 模块返回确认字 + 页码(匹配的指纹模板)
uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x08);
    Sendcmd(0x04);
    MYUSART_SendData(BufferID);
    MYUSART_SendData(StartPage>>8);
    MYUSART_SendData(StartPage);
    MYUSART_SendData(PageNum>>8);
    MYUSART_SendData(PageNum);
    temp = 0x01+0x08+0x04+BufferID
        +(StartPage>>8)+(uint8_t)StartPage
        +(PageNum>>8)+(uint8_t)PageNum;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
    {
        ensure = data[9];
        p->pageID    = (data[10]<<8)+data[11];
        p->mathscore = (data[12]<<8)+data[13];
    }
    else
        ensure = 0xFF;
    return ensure;
}

// 合并特征(生成模板)
// 功能: 将 CharBuffer1 与 CharBuffer2 中的特征文件合并生成模板, 结果存于 CharBuffer1 与 CharBuffer2
uint8_t PS_RegModel(void)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x05);
    temp = 0x01+0x03+0x05;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 储存模板
// 功能: 将 CharBuffer1 或 CharBuffer2 中的模板文件保存到 PageID 号 flash 数据库位置
// 参数: BufferID, PageID(指纹库位置号)
uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x06);
    Sendcmd(0x06);
    MYUSART_SendData(BufferID);
    MYUSART_SendData(PageID>>8);
    MYUSART_SendData(PageID);
    temp = 0x01+0x06+0x06+BufferID
        +(PageID>>8)+(uint8_t)PageID;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 删除模板
// 功能: 删除 flash 数据库中指定 ID 号开始的 N 个指纹模板
// 参数: PageID(指纹库模板号), N(删除的模板个数)
uint8_t PS_DeletChar(uint16_t PageID, uint16_t N)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x07);
    Sendcmd(0x0C);
    MYUSART_SendData(PageID>>8);
    MYUSART_SendData(PageID);
    MYUSART_SendData(N>>8);
    MYUSART_SendData(N);
    temp = 0x01+0x07+0x0C
        +(PageID>>8)+(uint8_t)PageID
        +(N>>8)+(uint8_t)N;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 清空指纹库
// 功能: 删除 flash 数据库中所有指纹模板
uint8_t PS_Empty(void)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x0D);
    temp = 0x01+0x03+0x0D;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 写系统寄存器
// 参数: RegNum: 寄存器号(4/5/6), DATA: 数据
uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x05);
    Sendcmd(0x0E);
    MYUSART_SendData(RegNum);
    MYUSART_SendData(DATA);
    temp = RegNum+DATA+0x01+0x05+0x0E;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 读系统基本参数
// 功能: 读取模块的基本参数(波特率, 指纹个数, 安全等级等)
// 说明: 模块返回确认字 + 基本参数(16 bytes)
uint8_t PS_ReadSysPara(SysPara *p)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x0F);
    temp = 0x01+0x03+0x0F;
    SendCheck(temp);
    data=JudgeStr(1000);
    if(data)
    {
        ensure = data[9];
        p->PS_max   = (data[14]<<8)+data[15];
        p->PS_level = data[17];
        p->PS_addr  = (data[18]<<24)+(data[19]<<16)+(data[20]<<8)+data[21];
        p->PS_size  = data[23];
        p->PS_N     = data[25];
    }
    else
        ensure=0xFF;
    return ensure;
}

// 设置模块地址
// 参数: PS_addr(新地址)
uint8_t PS_SetAddr(uint32_t PS_addr)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x07);
    Sendcmd(0x15);
    MYUSART_SendData(PS_addr>>24);
    MYUSART_SendData(PS_addr>>16);
    MYUSART_SendData(PS_addr>>8);
    MYUSART_SendData(PS_addr);
    temp = 0x01+0x07+0x15
        +(uint8_t)(PS_addr>>24)+(uint8_t)(PS_addr>>16)
        +(uint8_t)(PS_addr>>8)+(uint8_t)PS_addr;
    SendCheck(temp);
    AS608Addr=PS_addr;  // 保存新地址
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 写记事本
// 功能: 模块内部为用户预留 256bytes 的 FLASH 空间, 分成 16 个页
// 参数: NotePageNum(0~15), Byte32(要写入的数据，32 个字节)
uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
    uint16_t temp;
    uint8_t  ensure,i;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(36);
    Sendcmd(0x18);
    MYUSART_SendData(NotePageNum);
    for(i=0;i<32;i++)
    {
        MYUSART_SendData(Byte32[i]);
        temp += Byte32[i];
    }
    temp = 0x01+36+0x18+NotePageNum+temp;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
        ensure=data[9];
    else
        ensure=0xFF;
    return ensure;
}

// 读记事本
// 功能: 读取 FLASH 用户区 128bytes 内容
// 参数: NotePageNum(0~15)
uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
    uint16_t temp;
    uint8_t  ensure,i;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x04);
    Sendcmd(0x19);
    MYUSART_SendData(NotePageNum);
    temp = 0x01+0x04+0x19+NotePageNum;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
    {
        ensure=data[9];
        for(i=0;i<32;i++)
        {
            Byte32[i]=data[10+i];
        }
    }
    else
        ensure=0xFF;
    return ensure;
}

// 高速搜索
// 功能: 以 CharBuffer1 或 CharBuffer2 中的特征文件高速搜索整个或部分指纹库
// 说明: 若搜索到，则返回页码。该指令针对指纹库中存在着很好的指纹，
//       可快速查找出来
// 参数: BufferID, StartPage(起始页), PageNum(页数)
uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x08);
    Sendcmd(0x1B);
    MYUSART_SendData(BufferID);
    MYUSART_SendData(StartPage>>8);
    MYUSART_SendData(StartPage);
    MYUSART_SendData(PageNum>>8);
    MYUSART_SendData(PageNum);
    temp = 0x01+0x08+0x1B+BufferID
        +(StartPage>>8)+(uint8_t)StartPage
        +(PageNum>>8)+(uint8_t)PageNum;
    SendCheck(temp);
    data=JudgeStr(2000);
    if(data)
    {
        ensure=data[9];
        p->pageID     = (data[10]<<8) +data[11];
        p->mathscore  = (data[12]<<8) +data[13];
    }
    else
        ensure=0xFF;
    return ensure;
}

// 读有效模板个数
uint8_t PS_ValidTempleteNum(uint16_t *ValidN)
{
    uint16_t temp;
    uint8_t  ensure;
    uint8_t  *data;
    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x1D);
    temp = 0x01+0x03+0x1D;
    SendCheck(temp);
    data=JudgeStr(200);
    if(data)
    {
        ensure=data[9];
        *ValidN = (data[10]<<8) +data[11];
    }
    else
        ensure=0xFF;
    return ensure;
}

// 与 AS608 模块握手
// 参数: PS_Addr 地址指针
// 说明: 模块返回新地址(正确地址)
uint8_t PS_HandShake(uint32_t *PS_Addr)
{
    SendHead();
    SendAddr();
    MYUSART_SendData(0X01);
    MYUSART_SendData(0X00);
    MYUSART_SendData(0X00);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    if(AS608_USART_RX_STA & 0X8000)  // 接收到数据
    {
        if(// 判断是否是模块返回的应答包
                AS608_USART_RX_BUF[0]==0XEF
                &&AS608_USART_RX_BUF[1]==0X01
                &&AS608_USART_RX_BUF[6]==0X07
            )
        {
            *PS_Addr=(AS608_USART_RX_BUF[2]<<24) + (AS608_USART_RX_BUF[3]<<16)
                        +(AS608_USART_RX_BUF[4]<<8) + (AS608_USART_RX_BUF[5]);
            AS608_USART_RX_STA=0;
            return 0;
        }
        AS608_USART_RX_STA=0;
    }
    return 1;
}

/* ==================================================================
 *                      AS608 模块初始化和自检
 * ================================================================== */
SysPara AS608Para;
uint16_t ValidN;

void AS608_Check(void)
{
    uint8_t cnt = 0;
    volatile uint8_t ensure;

    // 与 AS608 模块握手
    while(PS_HandShake(&AS608Addr))
    {
        cnt++;
        printf("AS608 handshake retry %d...\r\n", cnt);
        if(cnt > 10)
        {
            printf("AS608 handshake FAILED!\r\n");
            return;  // 握手失败，直接返回
        }
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    }
    printf("AS608 connected! Addr=0x%08lX\r\n", AS608Addr);
    // 获取指纹个数
    ensure = PS_ValidTempleteNum(&ValidN);
    if(ensure != 0x00)
    {
        printf("Read valid template count failed!\r\n");
    }
    else
    {
        printf("Fingerprint count: %d\r\n", ValidN);
    }
    // 读取系统参数
    ensure = PS_ReadSysPara(&AS608Para);
    if(ensure != 0x00)
    {
        printf("Read sys para failed!\r\n");
    }
}
/**
 * @brief   刷指纹函数 - 只返回一次查询成功的结果，直到下次按下才再次查询
 * @param   matched_id   :   输出匹配到的指纹 ID
 * @retval  0: 上次查询指纹成功但未松开  1: 指纹查询成功
 *          2: 指纹查询失败  3: 没有检测到指纹
 */
uint8_t AS608_PressFR(uint16_t *matched_id)
{
    static uint8_t press_state = 0;     /* 指纹按压状态 */

    /* 去抖: 连续两次读到高电平才确认手指按下 */
    if(PS_Sta && press_state == 0)
    {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        if(!PS_Sta) return 0x03;  /* 抖动, 忽略 */
    }

    if(PS_Sta && press_state == 0)      /* PS_Sta 高电平为手指按下 */
    {
        SearchResult seach;             /* 搜索结果 */
        uint8_t ensure;

        ensure = PS_GetImage();         /* 获取图像 */
        if(ensure == 0x00)              /* 获取图像成功 */
        {
            ensure=PS_GenChar(CharBuffer2); /* 生成特征 */
            if(ensure == 0x00)          /* 生成特征成功 */
            {
                ensure=PS_HighSpeedSearch(CharBuffer2,0,AS608Para.PS_max,&seach);/* 搜索指纹 */
                if(ensure == 0x00)      /* 搜索指纹成功 */
                {
                    if (matched_id) *matched_id = seach.pageID;  // 返回匹配 ID
                    press_state = 1;    /* 成功扫描到指纹 - 下次扫描需手指松开 */
                    return 1;
                }
            }
        }
        return 2;
    }
    else if(press_state == 1 && PS_Sta)
    {
        /* 手指还没有松开 */
        return 0x00;
    }
    else
    {
        if(press_state == 1)
        {
            press_state = 0;    /* 可以扫描下一个指纹了 */
        }
    }
    return 0x03;
}
/**
 * @brief   录入指纹
 * @param   none
 * @retval  0: 录入指纹失败  1: 录入指纹成功  2: 没有检测到指纹
 */
uint8_t AS608_AddFR(void)
{
    uint8_t ensure;
    if(PS_Sta)
    {
        ensure=PS_GetImage();

        if(ensure==0x00)
        {
            ensure=PS_GenChar(CharBuffer1);         /* 指纹特征 1 */
            if(ensure==0x00)
            {
                /* 第二次按指纹前，检测手指是否松开再按下 */
                if(PS_Sta)
                {
                    ensure=PS_GetImage();
                    if(ensure == 0x00)
                    {
                        ensure=PS_GenChar(CharBuffer2); /* 指纹特征 2 */
                        if(ensure == 0x00)
                        {
                            ensure = PS_Match();        /* 匹配两次指纹是否相同 */
                            if(ensure == 0x00)
                            {
                                ensure=PS_RegModel();   /* 生成指纹模板 */
                                if(ensure==0x00)
                                {
                                    PS_ValidTempleteNum(&ValidN);               /* 获取指纹个数 */
                                    ensure=PS_StoreChar(CharBuffer2, ValidN + 1);/* 储存模板 */
                                    if(ensure == 0x00)
                                    {
                                        return 0x01;    /* 录入成功 */
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return 0x00;
    }
    return 0x02;
}
/**
 * @brief   删除指纹 (指定指纹 ID 或清空指纹库)
 * @param   delete_id   :   需要删除的指纹 ID，0xFFFF 删除所有指纹
 * @retval  1: 删除成功  0: 删除失败
 */
uint8_t AS608_DeleteFR(uint16_t delete_id)
{
    uint8_t ensure;
    if(delete_id==0xFFFF)
        ensure=PS_Empty();  /* 删除所有指纹 */
    else
        ensure = PS_DeletChar(delete_id,1); /* 删除指定指纹 */
    if(ensure == 0x00)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief   获取当前指纹库中指纹的个数
 * @param   none
 * @retval  返回当前指纹数量
 */
uint8_t AS608_GetFRNumber(void)
{
    PS_ValidTempleteNum(&ValidN);
    return ValidN;
}

#endif
