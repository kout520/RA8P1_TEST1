/**
 * ESP32 通信模块 - 头文件
 * 通过 USART3 (115200bps, 8N1) 与 ESP32 进行串口通信
 *
 * 协议说明:
 *   RA8 → ESP32 (RA8发送, ESP32接收):
 *     @1 ~ @6              考勤打卡命令 (ESP32 收到后通过 MQTT 上报)
 *     @WIFI:ssid,password  WiFi 配置命令
 *
 *   ESP32 → RA8 (ESP32发送, RA8接收):
 *     @g                    LED 开 (来自 MQTT ledctl:on)
 *     @h                    LED 关 (来自 MQTT ledctl:off)
 *     @DATE:YYYY:MM:DD      日期同步 (年/月/日)
 *     @HH:MM:SS             时间同步 (时:分:秒)
 *     @1\r\n                WiFi 已断开
 *     @2\r\n                WiFi 已连接
 *     其他文本               状态信息
 */
#ifndef __ESP32_COMM_H__
#define __ESP32_COMM_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ===== 环形缓冲区大小 ===== */
#define ESP32_RINGBUFFER_LEN   (512)

/* ===== ESP32 WiFi 状态 ===== */
#define ESP32_WIFI_DISCONNECTED   0
#define ESP32_WIFI_CONNECTED      1

/* ===== LED 状态 ===== */
#define ESP32_LED_OFF   0
#define ESP32_LED_ON    1

/* ===== 接收到的命令类型 ===== */
#define ESP32_CMD_NONE          0
#define ESP32_CMD_LED_ON        1   /* @g */
#define ESP32_CMD_LED_OFF       2   /* @h */
#define ESP32_CMD_TIME_SYNC     3   /* @HH:MM:SS */
#define ESP32_CMD_WIFI_UP       4   /* @2 WiFi已连接 */
#define ESP32_CMD_WIFI_DOWN     5   /* @1 WiFi已断开 */
#define ESP32_CMD_DATE_SYNC     6   /* @DATE:YYYY:MM:DD */

/* ===== 公共接口 ===== */

/** 初始化 ESP32 通信 (打开 USART3, 配置 115200) */
void ESP32_Comm_Init(void);

/* ---------- 发送函数 (RA8 → ESP32) ---------- */

/** 发送考勤打卡命令: @1 ~ @6 */
void esp32_send_attendance(uint8_t id);

/** 发送 WiFi 配置: @WIFI:ssid,password */
void esp32_send_wifi_config(const char *ssid, const char *password);

/** 发送原始字符串到 ESP32 (自动追加 \n) */
void esp32_send_string(const char *str);

/* ---------- 接收处理 ---------- */

/** 处理 ESP32 接收数据 (在主循环中周期调用) */
void esp32_process(void);

/** 获取并清除最新收到的命令类型 (读取后自动清零) */
uint8_t esp32_get_command(void);

/** 获取 ESP32 WiFi 连接状态 */
uint8_t esp32_get_wifi_state(void);

/** 获取接收到的 LED 状态 */
uint8_t esp32_get_led_state(void);

/** 获取接收到的时间同步值 (时/分/秒) */
void esp32_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second);

/** 获取接收到的日期同步值 (年/月/日) */
void esp32_get_date(uint16_t *year, uint8_t *month, uint8_t *day);

/* ===== 软件 RTC (基于 1ms 定时器, ESP32 校准) ===== */

/** 1ms 时基 (在 timer_1ms_callback 中调用) */
void esp32_rtc_tick_1ms(void);

/** 获取当前 RTC 日期时间字符串 (格式: YYYY-MM-DD HH:MM:SS) */
void esp32_rtc_get_string(char *buf, uint16_t buf_size);

/** RTC 秒是否已改变 (用于触发显示更新, 读取后自动清零) */
bool esp32_rtc_second_changed(void);

/* ===== 环形缓冲区操作 (内部使用) ===== */
void esp32_ringbuf_init(void);
void esp32_ringbuf_write(uint8_t data);
uint8_t esp32_ringbuf_read(uint16_t position);
void esp32_ringbuf_delete(uint16_t size);
uint16_t esp32_ringbuf_length(void);

#endif /* __ESP32_COMM_H__ */
