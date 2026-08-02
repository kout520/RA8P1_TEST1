#ifndef __TJCUSARTHMI_H__
#define __TJCUSARTHMI_H__

#include <stdio.h>
#include <stdint.h>

/**
 * TJC 串口屏通信模块 - 头文件
 */



#define TJC_UART huart1
#define TJC_UART_INS USART1


void UART_HIM_Init(void);
void tjc_send_string(char* str);
void tjc_send_txt(char* objname, char* attribute, char* txt);
void tjc_send_val(char* objname, char* attribute, int val);
void tjc_send_nstring(char* str, unsigned char str_length);
void initRingBuffer(void);
void write1ByteToRingBuffer(uint8_t data);
void deleteRingBuffer(uint16_t size);
uint16_t getRingBufferLength(void);
uint8_t read1ByteFromRingBuffer(uint16_t position);

void uart6_wait_for_tx(void);
void uart6_wait_for_rx(void);
void HIM_connection(void);

#define RINGBUFFER_LEN  (500)     // 环形缓冲区最大字节数 500

#define usize getRingBufferLength()
#define code_c() initRingBuffer()
#define udelete(x) deleteRingBuffer(x)
#define u(x) read1ByteFromRingBuffer(x)

extern uint8_t RxBuffer[1];
extern uint32_t msTicks;
extern uint8_t lock_flag;
extern int a;

#endif
