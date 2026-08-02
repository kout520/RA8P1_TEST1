/**
 * TJC USART HMI 串口屏通信模块
 * 使用说明:
 *    1. 将 tjc_usart_hmi.c 和 tjc_usart_hmi.h 加入工程
 *    2. 在需要使用函数的源文件中 #include "tjc_usart_hmi.h"
 */
#include "tjc_usart_hmi.h"
#include "headfile.h"


#define FRAME_LENGTH 7
#define u8 uint8_t

uint8_t lock_flag = 0;

typedef struct
{
    uint16_t Head;
    uint16_t Tail;
    uint16_t Length;
    uint8_t  Ring_data[RINGBUFFER_LEN];
}RingBuffer_t;

RingBuffer_t ringBuffer;    // 定义一个 ringBuffer 环形缓冲区
uint8_t RxBuffer[1];


void UART_HIM_Init(void)
{
    fsp_err_t err6;

    err6 = g_uart6.p_api->open(g_uart6.p_ctrl, g_uart6.p_cfg);// 打开 UART6



    initRingBuffer();// 环形缓冲区初始化

}

/********************************************************
函数名称：      intToStr
日期：          2024.09.18
功能：          将整数转换为字符串
输入参数：      要转换的整数, 存放结果的字符指针
返回值：        无
修改记录：
**********************************************************/
void intToStr(int num, char* str) {
    int i = 0;
    int isNegative = 0;

    // 处理负数
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    // 获取每一位数字
    do {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num);

    // 如果是负数则添加负号
    if (isNegative) {
        str[i++] = '-';
    }

    // 添加字符串终止符
    str[i] = '\0';

    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
    return ;
}


/********************************************************
函数名称：      uart_send_char
日期：          2024.09.18
功能：          串口发送单个字符
输入参数：      要发送的单个字符
返回值：        无
修改记录：
**********************************************************/
void uart_send_char(char ch)
{
    uint8_t ch2 = (uint8_t)ch;



        /* 发送单个字符 */
    g_uart6.p_api->write(g_uart6.p_ctrl, &ch2, 1);

    /* 等待发送完成 */
    uart6_wait_for_tx();

//    // 发送单个字符
//    USART_SendData(USART2, ch2);
//
//    // 等待发送完成
//    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//
//    return;
}

// /********************************************************
// 函数名称：      uart_send_char
// 日期：          2024.09.18
// 功能：          串口发送单个字符
// 输入参数：      要发送的单个字符
// 返回值：        无
// 修改记录：
// **********************************************************/
// void uart_send_char(char ch)
// {
//     uint8_t ch2 = (uint8_t)ch;
//     // 检查串口忙状态, 等待空闲时再发送下一个字符
//     //while(__HAL_UART_GET_FLAG(&TJC_UART, UART_FLAG_TXE) == RESET);    // 等待发送完成
//     while(__HAL_UART_GET_FLAG(&TJC_UART, UART_FLAG_TC) == RESET);
//     // 发送单个字符
//     HAL_UART_Transmit_IT(&TJC_UART, &ch2, 1);




//     // USART_SendData(pUSARTx, data);

//     // while(USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);



//     return;
// }



void uart_send_string(char* str)
{
    // 当前字符不是字符串结尾 且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        // 将字符串首地址中的字符发送, 发送完之后首地址递增
        uart_send_char(*str++);
    }
    return;
}

/********************************************************
函数名称：      tjc_send_string
日期：          2024.09.18
功能：          串口发送字符串(带 TJC 结束符 0xFF 0xFF 0xFF)
输入参数：      要发送的字符串
返回值：        无
示例:           tjc_send_string("n0.val=100"); 发送数据即 n0.val=100
修改记录：
**********************************************************/
void tjc_send_string(char* str)
{
    // 当前字符不是字符串结尾 且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        // 将字符串首地址中的字符发送, 发送完之后首地址递增
        uart_send_char(*str++);
    }
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
    return;
}

/********************************************************
函数名称：      tjc_send_txt
日期：          2024.09.18
功能：          向 TJC 串口屏发送文本属性值
输入参数：      objname: 控件名, attribute: 属性名, txt: 文本内容
返回值：        无
示例:           tjc_send_txt("t0", "txt", "ABC"); 发送数据即 t0.txt="ABC"
修改记录：
**********************************************************/
void tjc_send_txt(char* objname, char* attribute, char* txt)
{

    uart_send_string(objname);
    uart_send_char('.');
    uart_send_string(attribute);
    uart_send_string("=\"");
    uart_send_string(txt);
    uart_send_char('\"');
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
    return;
}


/********************************************************
函数名称：      tjc_send_val
日期：          2024.09.18
功能：          向 TJC 串口屏发送数值属性值
输入参数：      objname: 控件名, attribute: 属性名, val: 数值
返回值：        无
示例:           tjc_send_val("n0", "val", 100); 发送数据即 n0.val=100
修改记录：
**********************************************************/
void tjc_send_val(char* objname, char* attribute, int val)
{
    // 拼接字符串, 如 n0.val=123
    uart_send_string(objname);
    uart_send_char('.');
    uart_send_string(attribute);
    uart_send_char('=');
    // C语言中整型的取值范围是(-2147483648 ~ 2147483647), 最长为-2147483648, 加上结束符\0一共12个字符
    char txt[12]="";
    intToStr(val, txt);
    uart_send_string(txt);
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
    return;
}

/********************************************************
函数名称：      tjc_send_nstring
日期：          2024.09.18
功能：          向 TJC 串口屏发送指定长度的字符串
输入参数：      str: 要发送的字符串, str_length: 字符串长度
返回值：        无
修改记录：
**********************************************************/
void tjc_send_nstring(char* str, unsigned char str_length)
{
    // 发送指定长度的字符
    for (int var = 0; var < str_length; ++var)
    {
        // 发送当前字符后指针递增
        uart_send_char(*str++);
    }
    uart_send_char(0xff);
    uart_send_char(0xff);
    uart_send_char(0xff);
    return;
}

u8 USART_Rbuffer_Num = 0;
u8 USART_Tbuffer_Num = 0;
// u8 USART_Rbuffer[buffer_len];// 串口接收缓冲区



/********************************************************
函数名称：      initRingBuffer
日期：          2022.10.08
功能：          初始化环形缓冲区
输入参数：      无
返回值：        void
修改记录：
**********************************************************/
void initRingBuffer(void)
{
    // 初始化缓冲区信息
    ringBuffer.Head = 0;
    ringBuffer.Tail = 0;
    ringBuffer.Length = 0;
    return;
}

// 串口中断
//void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
//void USART2_IRQHandler(void)
//{
//
//    // 确认接收到 HEX 数据
//    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
//    {
//
//        uint8_t receivedData = (uint8_t)USART_ReceiveData(USART2);
//        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
//
//        write1ByteToRingBuffer(receivedData);
//        USART_Rbuffer_Num++;
//
//    }
//}
/**********************************************************/
static volatile int g_uart6_tx_complete = 0;
static volatile int g_uart6_rx_complete = 0;

void uart6_wait_for_tx(void)
{
    while (!g_uart6_tx_complete);
    g_uart6_tx_complete = 0;
}

void uart6_wait_for_rx(void)
{
    while (!g_uart6_rx_complete);
    g_uart6_rx_complete = 0;
}

// 串口中断回调函数
void uart6_callback(uart_callback_args_t * p_args)
{
    static uint8_t rx_data;

    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE: // 发送完成
        {
            g_uart6_tx_complete = 1;
            break;
        }

        case UART_EVENT_RX_CHAR: // 接收字符(逐个中断, 需要使能)
        {
            // 读取数据
            rx_data = (uint8_t)p_args->data;

            // 写入环形缓冲区
            write1ByteToRingBuffer(rx_data);

            // 增加缓冲区计数
            USART_Rbuffer_Num++;


            break;
        }

        case UART_EVENT_RX_COMPLETE: // 接收完成(DMA传输到指定长度后)
        {
            g_uart6_rx_complete = 1;
            // 可以在这里处理接收数据完成的逻辑
            break;
        }

        case UART_EVENT_ERR_PARITY:   // 奇偶校验错误
        case UART_EVENT_ERR_FRAMING:  // 帧错误
        {
            // 错误处理
            // 可以设置错误标志或触发重传机制
            break;
        }

        default:
        {
            break;
        }
    }
}

int a = 0;
char str[100];

// 接收 WiFi 信息相关变量
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PWD_MAX_LEN 64

static char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
static char wifi_pwd[WIFI_PWD_MAX_LEN] = {0};
static uint8_t wifi_ssid_len = 0;
static uint8_t wifi_pwd_len = 0;
static uint8_t wifi_data_type = 0; // 0:空闲 1:等待SSID 2:等待密码
static uint8_t wifi_frame_received = 0; // 标记是否已完成当前帧

// 串口屏数据交互处理
void HIM_connection(void)
{
    a++;
    while (usize >= 4)
    {
        // 命令帧: 55 XX 00 00 FF FF FF
        if (usize >= 7 && u(0) == 0x55 && u(2) == 0x00 && u(3) == 0x00 &&
            u(4) == 0xff && u(5) == 0xff && u(6) == 0xff)
        {
            if (u(1) == 0x01) // 确认连接
            {
                 /* 发送测试指令 */
                voice_send_string("@0");
                printf("Voice: test command @0 sent\r\n");
            }
            else if (u(1) == 0x02) // 开始接收 WiFi SSID
            {
                wifi_data_type = 1;
                wifi_frame_received = 0; // 重置接收标志
                wifi_ssid_len = 0;
                memset(wifi_ssid, 0, WIFI_SSID_MAX_LEN);
                printf("开始接收SSID\r\n");
            }
            else if (u(1) == 0x03) // 开始接收 WiFi 密码
            {
                printf("SSID接收完成: %s\r\n", wifi_ssid);
                wifi_data_type = 2;
                wifi_frame_received = 0; // 重置接收标志
                wifi_pwd_len = 0;
                memset(wifi_pwd, 0, WIFI_PWD_MAX_LEN);
                printf("开始接收密码\r\n");
            }
            else if (u(1) == 0x04) // 连接 WiFi
            {
                // 接收完成
                wifi_data_type = 0;
                wifi_frame_received = 0;

                // 打印结果
                printf("WiFi SSID: %s\r\n", wifi_ssid);
                printf("WiFi PWD: %s\r\n", wifi_pwd);


                // 清空缓冲区
                memset(wifi_ssid, 0, WIFI_SSID_MAX_LEN);
                memset(wifi_pwd, 0, WIFI_PWD_MAX_LEN);
                wifi_ssid_len = 0;
                wifi_pwd_len = 0;
            }
            else if (u(1) == 0x05) // 开锁命令
            {
                a = 0;
                lock_flag = 1;
                R_IOPORT_PinWrite(g_ioport.p_ctrl, BSP_IO_PORT_04_PIN_11, BSP_IO_LEVEL_HIGH);
            }
            else if (u(1) == 0x06) // 指纹录入
            {
                    sprintf(str, "t0.txt=\"请按指纹\"");
                    tjc_send_string(str);
                    finger_addtion();
                    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
                    sprintf(str, "t0.txt=\"%d\"",AS608_GetFRNumber());
                    tjc_send_string(str);
            }
            else if (u(1) == 0x07) // 删除指纹
            {
                    AS608_DeleteFR(0xffff);
                    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
                    sprintf(str, "t0.txt=\"%d\"",AS608_GetFRNumber());
                    tjc_send_string(str);
            }
            else if (u(1) == 0x08) // 启动摄像头
            {
                    uart8_send_value(222);
            }
            else if (u(1) == 0x09) // 其他命令
            {

            }
            udelete(7);
        }
        // 数据帧: 70 XX XX ... FF FF FF (WiFi SSID/密码数据)
        else if (u(0) == 0x70 && wifi_data_type != 0 && wifi_frame_received == 0)
        {
            // 仅在接收状态且未完成数据帧时才处理

            // 查找帧尾: FF FF FF
            uint8_t frame_end = 0;
            for (uint8_t i = 1; i < usize - 2; i++)
            {
                if (u(i) == 0xff && u(i+1) == 0xff && u(i+2) == 0xff)
                {
                    frame_end = i;
                    break;
                }
            }

            if (frame_end > 0) // 找到完整帧
            {
                uint8_t data_len = frame_end - 1; // 数据长度(去掉0x70帧头)

                if (wifi_data_type == 1) // 接收 SSID
                {
                    if (data_len < WIFI_SSID_MAX_LEN)
                    {
                        for (uint8_t i = 0; i < data_len; i++)
                        {
                            wifi_ssid[wifi_ssid_len++] = u(i + 1);
                        }
                        wifi_ssid[wifi_ssid_len] = '\0';
                        wifi_frame_received = 1; // 标记已完成
                        printf("接收SSID数据: %s (长度:%d)\r\n", wifi_ssid, wifi_ssid_len);
                    }
                }
                else if (wifi_data_type == 2) // 接收密码
                {
                    if (data_len < WIFI_PWD_MAX_LEN)
                    {
                        for (uint8_t i = 0; i < data_len; i++)
                        {
                            wifi_pwd[wifi_pwd_len++] = u(i + 1);
                        }
                        wifi_pwd[wifi_pwd_len] = '\0';
                        wifi_frame_received = 1; // 标记已完成
                        printf("接收密码数据: %s (长度:%d)\r\n", wifi_pwd, wifi_pwd_len);
                    }
                }

                udelete(frame_end + 3); // 删除整个数据帧(含3个0xFF)
            }
            else
            {
                break; // 数据不完整, 等待更多数据
            }
        }
        else
        {
            udelete(1); // 不匹配则删除1字节
        }
    }
}







/********************************************************
函数名称：      write1ByteToRingBuffer
日期：          2022.10.08
功能：          向环形缓冲区写入数据
输入参数：      要写入的1字节数据
返回值：        void
修改记录：
**********************************************************/
void write1ByteToRingBuffer(uint8_t data)
{
    if(ringBuffer.Length >= RINGBUFFER_LEN) // 判断缓冲区是否已满
    {
        return ;
    }
    ringBuffer.Ring_data[ringBuffer.Tail]=data;
    ringBuffer.Tail = (ringBuffer.Tail+1)%RINGBUFFER_LEN;// 防止越界访问
    ringBuffer.Length++;
    return ;
}
/********************************************************
函数名称：      read1ByteFromRingBuffer
作者：
日期：          2022.10.08
功能：          从环形缓冲区读取1字节数据
输入参数：      position: 读取的位置(从Head偏移)
返回值：        对应位置的数据(1字节)
修改记录：
**********************************************************/
uint8_t read1ByteFromRingBuffer(uint16_t position)
{
    uint16_t realPosition = (ringBuffer.Head + position) % RINGBUFFER_LEN;

    return ringBuffer.Ring_data[realPosition];
}






/********************************************************
函数名称：      deleteRingBuffer
作者：
日期：          2022.10.08
功能：          删除环形缓冲区中指定长度的数据
输入参数：      要删除的长度
返回值：        void
修改记录：
**********************************************************/
void deleteRingBuffer(uint16_t size)
{
    if(size >= ringBuffer.Length)
    {
        initRingBuffer();
        return;
    }
    for(int i = 0; i < size; i++)
    {
        ringBuffer.Head = (ringBuffer.Head+1)%RINGBUFFER_LEN;// 防止越界访问
        ringBuffer.Length--;
        return;
    }

}







/********************************************************
函数名称：      getRingBufferLength
作者：
日期：          2022.10.08
功能：          获取环形缓冲区中有效数据长度
输入参数：      无
返回值：        环形缓冲区中有效数据长度
修改记录：
**********************************************************/
uint16_t getRingBufferLength()
{
    return ringBuffer.Length;
}


/********************************************************
函数名称：      isRingBufferOverflow
作者：
日期：          2022.10.08
功能：          判断环形缓冲区是否溢出
输入参数：      无
返回值：        0: 环形缓冲区已满, 1: 环形缓冲区未满
修改记录：
**********************************************************/
uint8_t isRingBufferOverflow()
{
    return ringBuffer.Length < RINGBUFFER_LEN;
}
