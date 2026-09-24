/**
 * ESP32 通信模块 - 实现文件
 * 通过 USART3 (115200bps, 8N1) 与 ESP32 进行串口通信
 *
 * 协议:
 *   RA8 → ESP32:  @1~@6 (考勤) / @WIFI:ssid,password (WiFi配置)
 *   ESP32 → RA8:  @g (LED开) / @h (LED关) / @HH:MM:SS (时间同步) /
 *                 @1\r\n (WiFi断开) / @2\r\n (WiFi已连)
 *
 * 依赖: FSP UART API (g_uart3), headfile.h
 */

#include "esp32_comm.h"
#include "headfile.h"
#include "speaker_verify.h"
#include <string.h>
#include <stdlib.h>

/* ==================================================================
 *                        环形缓冲区
 * ================================================================== */
typedef struct {
    uint16_t head;
    uint16_t tail;
    uint16_t length;
    uint8_t  data[ESP32_RINGBUFFER_LEN];
} esp32_ringbuf_t;

static esp32_ringbuf_t esp32_rb;

void esp32_ringbuf_init(void)
{
    esp32_rb.head   = 0;
    esp32_rb.tail   = 0;
    esp32_rb.length = 0;
}

void esp32_ringbuf_write(uint8_t data)
{
    if (esp32_rb.length >= ESP32_RINGBUFFER_LEN) {
        return;
    }
    esp32_rb.data[esp32_rb.tail] = data;
    esp32_rb.tail = (uint16_t)((esp32_rb.tail + 1) % ESP32_RINGBUFFER_LEN);
    esp32_rb.length++;
}

uint8_t esp32_ringbuf_read(uint16_t position)
{
    uint16_t real_pos = (esp32_rb.head + position) % ESP32_RINGBUFFER_LEN;
    return esp32_rb.data[real_pos];
}

void esp32_ringbuf_delete(uint16_t size)
{
    if (size >= esp32_rb.length) {
        esp32_ringbuf_init();
        return;
    }
    esp32_rb.head = (uint16_t)((esp32_rb.head + size) % ESP32_RINGBUFFER_LEN);
    esp32_rb.length -= size;
}

uint16_t esp32_ringbuf_length(void)
{
    return esp32_rb.length;
}

/* ==================================================================
 *                        UART 发送 (RA8 → ESP32)
 * ================================================================== */

static volatile uint8_t g_uart3_tx_done = 1;

static void esp32_wait_tx(void)
{
    uint32_t timeout = 100000;  /* safety timeout ~100ms */
    while (!g_uart3_tx_done && --timeout) {
        ;
    }
    g_uart3_tx_done = 0;
}

static void esp32_send_char(char ch)
{
    uint8_t byte = (uint8_t)ch;
    g_uart3_tx_done = 0;
    g_uart3.p_api->write(g_uart3.p_ctrl, &byte, 1);
    esp32_wait_tx();
}

static void esp32_send_raw(const char *str)
{
    if (NULL == str) return;
    while (*str != '\0') {
        esp32_send_char(*str++);
    }
}

/* ==================================================================
 *                        公共发送接口 (RA8 → ESP32)
 * ================================================================== */

/**
 * 发送字符串到 ESP32 (自动追加 \n)
 * ESP32 端通过 uart2.readline() 按行接收
 */
void esp32_send_string(const char *str)
{
    if (NULL == str) return;
    esp32_send_raw(str);
    esp32_send_char('\n');
}

/**
 * 发送考勤打卡命令: @1 ~ @6
 * ESP32 收到后发布到 MQTT topic "aa", 值为对应的 temp
 */
void esp32_send_attendance(uint8_t id)
{
    if (id < 1 || id > 6) return;
    char buf[4];
    sprintf(buf, "@%d", id);
    esp32_send_string(buf);
    printf("ESP32 TX: attendance %s\r\n", buf);
}

/**
 * 发送 WiFi 配置: @WIFI:ssid,password
 * ESP32 收到后会尝试连接到新 WiFi
 */
void esp32_send_wifi_config(const char *ssid, const char *password)
{
    if (NULL == ssid || NULL == password) return;

    char buf[128];
    int len = snprintf(buf, sizeof(buf), "@WIFI:%s,%s", ssid, password);
    if (len >= (int)sizeof(buf)) {
        printf("ESP32: WiFi config string too long!\r\n");
        return;
    }
    esp32_send_string(buf);
    printf("ESP32 TX: WiFi config SSID=%s\r\n", ssid);
}

/* ==================================================================
 *                        接收数据存储
 * ================================================================== */

static uint8_t  g_esp32_wifi_state  = ESP32_WIFI_DISCONNECTED;
static uint8_t  g_esp32_led_state   = ESP32_LED_OFF;
static uint8_t  g_esp32_cmd         = ESP32_CMD_NONE;

/* 日期 + 时间同步数据 */
static uint16_t g_sync_year  = 2026;
static uint8_t  g_sync_month = 1;
static uint8_t  g_sync_day   = 1;
static uint8_t  g_sync_hour   = 0;
static uint8_t  g_sync_minute = 0;
static uint8_t  g_sync_second = 0;

/* K230 物品库存 (ESP32 转发) */
static int g_k230_cola   = 0;
static int g_k230_sprite = 0;
static int g_k230_milk   = 0;

/* 上一次库存值 (用于检测减少 → 物品被取走) */
static int g_k230_cola_prev   = -1;
static int g_k230_sprite_prev = -1;
static int g_k230_milk_prev   = -1;

/* 用户积分余额: index 1-4 (张三/李四/王五/老六)
   默认 0 (未联网时安全值), 联网后由网站 @P 同步真实积分 */
static int g_user_points[5] = {0, 0, 0, 0, 0};

/* 物品积分价格: C=可乐5分, S=宏宝莱5分, M=牛奶10分 */
static int item_price(char item_code)
{
    switch (item_code) {
        case 'C': return 5;
        case 'S': return 5;   /* 宏宝莱 */
        case 'M': return 10;
        default:  return 0;
    }
}

static const char *item_name_cn(char item_code)
{
    switch (item_code) {
        case 'C': return "可乐";
        case 'S': return "宏宝莱";
        case 'M': return "牛奶";
        default:  return "未知";
    }
}

/* 上报 "谁取走了什么" 给 ESP32 → 网站 */
static void report_item_taken(char item_code)
{
    if (g_current_user <= 0) {
        /* 未识别身份就取物 → 语音警告 @5 + 通知 ESP32 发微信 @U */
        voice_send_string("@5");
        LCD_ShowMsg(5);   /* LCD: 你的信息未录入此系统，取物失败 */
        esp32_send_string("@U");
        printf(">>> 未授权取物! 物品 %s, 语音告警\r\n", item_name_cn(item_code));
        return;
    }

    int price = item_price(item_code);

    /* 积分不足 → @10 语音提醒, 不扣积分 */
    if (g_user_points[g_current_user] < price) {
        voice_send_string("@10");
        LCD_ShowMsg(10);   /* LCD: 您的积分不足，请充值或明天再来 */
        printf(">>> 用户%d 积分不足, 无法取 %s (余额%d)\r\n",
               g_current_user, item_name_cn(item_code), g_user_points[g_current_user]);
        return;
    }

    g_user_points[g_current_user] -= price;

    /* 语音播报: @7=可乐 @8=宏宝莱 @9=牛奶 */
    switch (item_code) {
    case 'C': voice_send_string("@7"); LCD_ShowMsg(7); break;   /* 你已取走可乐，减五积分 */
    case 'S': voice_send_string("@8"); LCD_ShowMsg(8); break;   /* 你已取走宏宝莱，减五积分 */
    case 'M': voice_send_string("@9"); LCD_ShowMsg(9); break;   /* 你已取走牛奶，减十积分 */
    default: break;
    }

    /* 发 @T:用户:物品:扣分 给 ESP32 → 网站同步扣分 */
    char buf[24];
    sprintf(buf, "@T:%d:%c:%d", g_current_user, item_code, price);
    esp32_send_string(buf);

    /* 跳转个人信息页 + 显示积分余额 */
    tjc_send_string("va0.val=11");
    esp32_show_points(g_current_user);  /* 内部带延迟, 等跳页完成再发积分 */

    printf(">>> 用户%d 取走 %s, 扣%d分, 余额%d\r\n",
           g_current_user, item_name_cn(item_code), price, g_user_points[g_current_user]);
    /* 不在此清除 g_current_user — 认证持续到关闭柜门,
       允许一次开柜门周期内连续取多个物品 */
}

/* 上报 "谁放回了什么" 给 ESP32 → 网站加积分 (取走又放回 → 净0, 多放 → 加积分) */
static void report_item_returned(char item_code)
{
    if (g_current_user <= 0) {
        /* 未认证, 不知道谁放回, 不记积分 */
        printf(">>> 未认证放回 %s, 不记积分\r\n", item_name_cn(item_code));
        return;
    }

    int price = item_price(item_code);

    /* 发 @B:用户:物品:加分 给 ESP32 → 网站同步加积分 */
    char buf[24];
    sprintf(buf, "@B:%d:%c:%d", g_current_user, item_code, price);
    esp32_send_string(buf);

    g_user_points[g_current_user] += price;   /* 主控同步加积分 (会被网站 @P 覆盖校准) */

    printf(">>> 用户%d 放回 %s, 加%d分, 余额%d\r\n",
           g_current_user, item_name_cn(item_code), price, g_user_points[g_current_user]);
}

/* 显示指定用户的积分余额到串口屏 t66 (认证成功/取物后调用) */
void esp32_show_points(int user)
{
    if (user < 1 || user > 4) return;
    char pts[20];
    sprintf(pts, "t66.txt=\"%d\"", g_user_points[user]);
    tjc_send_delayed(pts, 300);  /* 非阻塞: 延迟300ms后发送, 等跳页完成 */
    printf(">>> 显示用户%d余额=%d\r\n", user, g_user_points[user]);
}

/* 刷新库存显示到串口屏 t60/t61/t62 (进入库存页面时调用) */
void esp32_refresh_inventory(void)
{
    char buf[20];
    sprintf(buf, "t60.txt=\"%d\"", g_k230_sprite);  /* 宏宝莱 → t60 */
    tjc_send_delayed(buf, 300);  /* 非阻塞延迟发送 */
    sprintf(buf, "t61.txt=\"%d\"", g_k230_cola);     /* 可乐 → t61 */
    tjc_send_delayed(buf, 300);
    sprintf(buf, "t62.txt=\"%d\"", g_k230_milk);     /* 牛奶 → t62 */
    tjc_send_delayed(buf, 300);

    /* 柜里没有任何物品 → @11 语音提醒 (每次刷新时检测, 不持续发送) */
    if (g_k230_sprite <= 0 && g_k230_cola <= 0 && g_k230_milk <= 0) {
        voice_send_string("@11");
        LCD_ShowMsg(11);   /* LCD: 库存无货，请下次再来 */
    }

    printf(">>> 刷新库存: 宏宝莱=%d 可乐=%d 牛奶=%d\r\n",
           g_k230_sprite, g_k230_cola, g_k230_milk);
}

/* ==================================================================
 *                        接收解析 (ESP32 → RA8)
 * ================================================================== */

/* 前向声明 */
static void rtc_calibrate_date(uint16_t year, uint8_t month, uint8_t day);
static void rtc_calibrate_time(uint8_t hour, uint8_t min, uint8_t sec);

#define ESP32_LINE_BUF_LEN  1024
static char    esp32_line_buf[ESP32_LINE_BUF_LEN];

/**
 * 解析时间同步命令: @HH:MM:SS
 * 返回 true 表示成功解析
 */
static bool esp32_parse_time(const char *line, uint8_t *h, uint8_t *m, uint8_t *s)
{
    /* 格式: @H:M:S  其中 H/M/S 可以是 1-2 位数字 */
    int hour = 0, min = 0, sec = 0;

    if (line[0] != '@') return false;

    /* 尝试解析: @%d:%d:%d */
    if (3 == sscanf(line, "@%d:%d:%d", &hour, &min, &sec)) {
        if (hour >= 0 && hour <= 23 && min >= 0 && min <= 59 && sec >= 0 && sec <= 59) {
            *h = (uint8_t)hour;
            *m = (uint8_t)min;
            *s = (uint8_t)sec;
            return true;
        }
    }
    return false;
}

/**
 * 解析 ESP32 发来的单行命令
 */
static void esp32_parse_line(const char *line)
{
    if (NULL == line || line[0] == '\0') return;

    //printf("ESP32 RX: %s\r\n", line);

    /* --- 声纹同步: @SPK:hex数据 (ESP32 → 主控, 断电恢复) --- */
    if (0 == strncmp(line, "@SPK:", 5)) {
        if (speaker_import_hex(&line[5])) {
            printf(">>> SPK: 声纹已从 ESP32 恢复\r\n");
        } else {
            printf(">>> SPK: 声纹解析失败\r\n");
        }
        return;
    }

    /* --- 积分同步: @P:用户:余额 (网站 → ESP32 → 主控) --- */
    if (0 == strncmp(line, "@P:", 3)) {
        int user = 0, pts = 0;
        if (2 == sscanf(line, "@P:%d:%d", &user, &pts) && user >= 1 && user <= 4) {
            /* 仅积分真正变化时才打印 + 刷新, 心跳同步(值没变)静默跳过,
               避免 printf 阻塞主循环影响音频采集和指令识别 */
            if (g_user_points[user] != pts) {
                g_user_points[user] = pts;
                printf(">>> 积分同步: 用户%d 余额=%d\r\n", user, pts);
                /* 若同步的是当前认证用户, 立即刷新串口屏 t66 (不用等下次进页面) */
                if (g_current_user == user) {
                    esp32_show_points(user);  /* 内部带 300ms 延时 */
                }
            }
            return;
        }
    }

    /* --- LED 控制命令 (来自 MQTT) --- */
    if (0 == strcmp(line, "@g")) {
        g_esp32_led_state = ESP32_LED_ON;
        g_esp32_cmd       = ESP32_CMD_LED_ON;
        printf(">>> ESP32: LED ON\r\n");
        return;
    }
    if (0 == strcmp(line, "@h")) {
        g_esp32_led_state = ESP32_LED_OFF;
        g_esp32_cmd       = ESP32_CMD_LED_OFF;
        printf(">>> ESP32: LED OFF\r\n");
        return;
    }

    /* --- WiFi 状态 --- */
    if (0 == strcmp(line, "@1")) {
        g_esp32_wifi_state = ESP32_WIFI_DISCONNECTED;
        g_esp32_cmd        = ESP32_CMD_WIFI_DOWN;
        tjc_send_val("va1", "val", 0);  /* 网络断开 → va1.val=0 */
        printf(">>> ESP32: WiFi DISCONNECTED\r\n");
        return;
    }
    if (0 == strcmp(line, "@2")) {
        g_esp32_wifi_state = ESP32_WIFI_CONNECTED;
        g_esp32_cmd        = ESP32_CMD_WIFI_UP;
        tjc_send_val("va1", "val", 1);  /* 网络连接 → va1.val=1 */
        voice_send_string("@0");        /* 语音: 你好, 我叫小白, 很高兴为您服务 */
        LCD_ShowMsg(0);                 /* LCD: 你好，我叫小白，很高兴为您服务 */
        printf(">>> ESP32: WiFi CONNECTED\r\n");
        return;
    }

    /* --- K230 库存数据: COLA:N / SPRITE:N / MILK:N --- */
    if (0 == strncmp(line, "COLA:", 5)) {
        int val = 0;
        if (1 == sscanf(line, "COLA:%d", &val) && val >= 0) {
            /* 库存变化: 减少=取物, 增加=放回 */
            if (g_k230_cola_prev >= 0) {
                if (val < g_k230_cola_prev) {
                    report_item_taken('C');
                } else if (val > g_k230_cola_prev) {
                    report_item_returned('C');
                }
            }
            g_k230_cola_prev = val;
            /* 只在库存变化时才更新串口屏+打印, 避免高频刷新阻塞主循环 */
            if (val != g_k230_cola) {
                char buf[16];
                sprintf(buf, "t61.txt=\"%d\"", val);  /* 可乐库存 → t61 */
                tjc_send_string(buf);
                printf(">>> K230: cola stock=%d\r\n", val);
            }
            g_k230_cola = val;
            return;
        }
    }
    if (0 == strncmp(line, "SPRITE:", 7)) {
        int val = 0;
        if (1 == sscanf(line, "SPRITE:%d", &val) && val >= 0) {
            if (g_k230_sprite_prev >= 0) {
                if (val < g_k230_sprite_prev) {
                    report_item_taken('S');
                } else if (val > g_k230_sprite_prev) {
                    report_item_returned('S');
                }
            }
            g_k230_sprite_prev = val;
            if (val != g_k230_sprite) {
                char buf[16];
                sprintf(buf, "t60.txt=\"%d\"", val);  /* 宏宝莱库存 → t60 */
                tjc_send_string(buf);
                printf(">>> K230: sprite stock=%d\r\n", val);
            }
            g_k230_sprite = val;
            return;
        }
    }
    if (0 == strncmp(line, "MILK:", 5)) {
        int val = 0;
        if (1 == sscanf(line, "MILK:%d", &val) && val >= 0) {
            if (g_k230_milk_prev >= 0) {
                if (val < g_k230_milk_prev) {
                    report_item_taken('M');
                } else if (val > g_k230_milk_prev) {
                    report_item_returned('M');
                }
            }
            g_k230_milk_prev = val;
            if (val != g_k230_milk) {
                char buf[16];
                sprintf(buf, "t62.txt=\"%d\"", val);  /* 牛奶库存 → t62 */
                tjc_send_string(buf);
                printf(">>> K230: milk stock=%d\r\n", val);
            }
            g_k230_milk = val;
            return;
        }
    }

    /* --- 日期同步: @DATE:YYYY:MM:DD --- */
    if (0 == strncmp(line, "@DATE:", 6)) {
        int y = 0, mo = 0, d = 0;
        if (3 == sscanf(line, "@DATE:%d:%d:%d", &y, &mo, &d)) {
            if (y >= 2000 && y <= 2100 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31) {
                g_sync_year  = (uint16_t)y;
                g_sync_month = (uint8_t)mo;
                g_sync_day   = (uint8_t)d;
                g_esp32_cmd  = ESP32_CMD_DATE_SYNC;
                rtc_calibrate_date((uint16_t)y, (uint8_t)mo, (uint8_t)d);
                //printf(">>> ESP32: date sync %04d-%02d-%02d\r\n", y, mo, d);
                // 合并时间刷新 TJC t8
                {
                    char buf[40];
                    sprintf(buf, "t8.txt=\"%04d-%02d-%02d %02d:%02d:%02d\"",
                            y, mo, d, g_sync_hour, g_sync_minute, g_sync_second);
                    tjc_send_string(buf);
                }
                return;
            }
        }
    }

    /* --- 时间同步: @HH:MM:SS --- */
    {
        uint8_t h, m, s;
        if (esp32_parse_time(line, &h, &m, &s)) {
            g_sync_hour   = h;
            g_sync_minute = m;
            g_sync_second = s;
            g_esp32_cmd   = ESP32_CMD_TIME_SYNC;
            rtc_calibrate_time(h, m, s);
            //printf(">>> ESP32: time sync %02d:%02d:%02d\r\n", h, m, s);
            // 合并日期刷新 TJC t8
            {
                char buf[40];
                sprintf(buf, "t8.txt=\"%04d-%02d-%02d %02d:%02d:%02d\"",
                        g_sync_year, g_sync_month, g_sync_day, h, m, s);
                tjc_send_string(buf);
            }
            return;
        }
    }

    /* 其他文本信息 (仅打印到调试串口) */
}

/**
 * 从环形缓冲区中提取完整行并解析
 * ESP32 发送的行以 \n 结尾 (部分带 \r)
 */
static void esp32_extract_lines(void)
{
    uint16_t len = esp32_ringbuf_length();

    while (len > 0) {
        /* 查找 \n */
        int16_t line_end = -1;
        for (uint16_t i = 0; i < len; i++) {
            if (esp32_ringbuf_read(i) == '\n') {
                line_end = (int16_t)i;
                break;
            }
        }

        if (line_end < 0) {
            break;  /* 没有完整行 */
        }

        /* 计算数据长度 (去掉 \r\n 中的 \r) */
        uint16_t data_len = (uint16_t)line_end;
        if (data_len > 0 && esp32_ringbuf_read(data_len - 1) == '\r') {
            data_len--;
        }

        /* 提取到行缓冲区 */
        if (data_len > 0 && data_len < ESP32_LINE_BUF_LEN) {
            for (uint16_t i = 0; i < data_len; i++) {
                esp32_line_buf[i] = (char)esp32_ringbuf_read(i);
            }
            esp32_line_buf[data_len] = '\0';
            esp32_parse_line(esp32_line_buf);
        }

        /* 删除已处理数据 (含 \n) */
        esp32_ringbuf_delete((uint16_t)line_end + 1);
        len = esp32_ringbuf_length();
    }
}

/* ==================================================================
 *                        公共接收接口
 * ================================================================== */

/**
 * 处理 ESP32 接收数据 (在主循环中周期调用)
 */
void esp32_process(void)
{
    esp32_extract_lines();

    /* 缓冲区溢出保护 */
    if (esp32_ringbuf_length() > 1800) {
        printf("ESP32: RX buffer overflow(%d bytes), cleared\r\n", esp32_ringbuf_length());
        esp32_ringbuf_init();
    }
}

/**
 * 获取并清除最新收到的命令
 * 返回 ESP32_CMD_NONE 表示无新命令
 */
uint8_t esp32_get_command(void)
{
    uint8_t cmd = g_esp32_cmd;
    g_esp32_cmd = ESP32_CMD_NONE;
    return cmd;
}

/**
 * 获取 ESP32 WiFi 连接状态
 */
uint8_t esp32_get_wifi_state(void)
{
    return g_esp32_wifi_state;
}

/**
 * 获取接收到的 LED 状态 (来自 MQTT 的 @g/@h 命令)
 */
uint8_t esp32_get_led_state(void)
{
    return g_esp32_led_state;
}

/**
 * 获取接收到的时间同步值
 */
void esp32_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    if (NULL != hour)   *hour   = g_sync_hour;
    if (NULL != minute) *minute = g_sync_minute;
    if (NULL != second) *second = g_sync_second;
}

/**
 * 获取接收到的日期同步值
 */
/**
 * 获取 K230 库存数据 (由 ESP32 转发)
 */
void esp32_get_k230_stock(int *cola, int *sprite, int *milk)
{
    if (NULL != cola)   *cola   = g_k230_cola;
    if (NULL != sprite) *sprite = g_k230_sprite;
    if (NULL != milk)   *milk   = g_k230_milk;
}

void esp32_get_date(uint16_t *year, uint8_t *month, uint8_t *day)
{
    if (NULL != year)  *year  = g_sync_year;
    if (NULL != month) *month = g_sync_month;
    if (NULL != day)   *day   = g_sync_day;
}

/* ==================================================================
 *                   软件 RTC (1ms 时基 + ESP32 校准)
 * ================================================================== */

/* RTC 当前值 */
static uint16_t rtc_year  = 2026;
static uint8_t  rtc_month = 7;
static uint8_t  rtc_day   = 11;
static uint8_t  rtc_hour  = 0;
static uint8_t  rtc_min   = 0;
static uint8_t  rtc_sec   = 0;
static uint16_t rtc_ms    = 0;  /* 毫秒计数器 0~999 */

static bool     rtc_sec_flag = false;  /* 秒翻转标志, 供显示更新 */

/* 每月天数 */
static const uint8_t rtc_days_in_month[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static uint8_t rtc_get_month_days(uint8_t month, uint16_t year)
{
    if (month == 2) {
        /* 闰年判断 */
        if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0))) {
            return 29;
        }
        return 28;
    }
    if (month >= 1 && month <= 12) {
        return rtc_days_in_month[month - 1];
    }
    return 30;
}

/**
 * 1ms 时基 (由 timer_1ms_callback 调用)
 * 自动累加时分秒日月年
 */
void esp32_rtc_tick_1ms(void)
{
    rtc_ms++;
    if (rtc_ms >= 1000) {
        rtc_ms = 0;
        rtc_sec++;
        rtc_sec_flag = true;

        if (rtc_sec >= 60) {
            rtc_sec = 0;
            rtc_min++;
            if (rtc_min >= 60) {
                rtc_min = 0;
                rtc_hour++;
                if (rtc_hour >= 24) {
                    rtc_hour = 0;
                    rtc_day++;
                    if (rtc_day > rtc_get_month_days(rtc_month, rtc_year)) {
                        rtc_day = 1;
                        rtc_month++;
                        if (rtc_month > 12) {
                            rtc_month = 1;
                            rtc_year++;
                        }
                    }
                }
            }
        }
    }
}

/**
 * 用 ESP32 发来的日期数据校准 RTC
 */
static void rtc_calibrate_date(uint16_t year, uint8_t month, uint8_t day)
{
    if (year >= 2000 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        rtc_year  = year;
        rtc_month = month;
        rtc_day   = day;
    }
}

/**
 * 用 ESP32 发来的时间数据校准 RTC (同步到秒, 清零毫秒)
 */
static void rtc_calibrate_time(uint8_t hour, uint8_t min, uint8_t sec)
{
    if (hour <= 23 && min <= 59 && sec <= 59) {
        rtc_hour = hour;
        rtc_min  = min;
        rtc_sec  = sec;
        rtc_ms   = 0;  /* 重新对齐秒边界 */
    }
}

/**
 * 获取当前 RTC 日期时间字符串: "YYYY-MM-DD HH:MM:SS"
 */
void esp32_rtc_get_string(char *buf, uint16_t buf_size)
{
    if (NULL == buf || buf_size < 20) return;
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            rtc_year, rtc_month, rtc_day, rtc_hour, rtc_min, rtc_sec);
}

/**
 * 查询 RTC 秒是否变化 (用于触发 TJC 显示更新, 读取后自动清零)
 */
bool esp32_rtc_second_changed(void)
{
    bool changed = rtc_sec_flag;
    rtc_sec_flag = false;
    return changed;
}

/* ==================================================================
 *                        UART 回调函数
 * ================================================================== */

/**
 * UART3 中断回调
 * 由 FSP 在 UART 中断上下文中调用
 */
void uart3_callback(uart_callback_args_t *p_args)
{
    if (NULL == p_args) return;

    switch (p_args->event) {
        case UART_EVENT_TX_COMPLETE:
            g_uart3_tx_done = 1;
            break;

        case UART_EVENT_RX_CHAR:
            esp32_ringbuf_write((uint8_t)p_args->data);
            break;

        case UART_EVENT_ERR_PARITY:
        case UART_EVENT_ERR_FRAMING:
            /* 通信错误 */
            break;

        default:
            break;
    }
}

/* ==================================================================
 *                        初始化
 * ================================================================== */

/**
 * 初始化 ESP32 通信模块
 * 1. 打开 USART3
 * 2. 切换波特率到 115200 (匹配 ESP32)
 * 3. 初始化环形缓冲区
 *
 * 注意: FSP 配置默认 57600, 已通过 baudSet 动态切换为 115200。
 *       推荐在 RA Configuration 中将 UART3 波特率改为 115200 并重新生成代码。
 */
void ESP32_Comm_Init(void)
{
    fsp_err_t err;

    /* --- 1. 打开 UART3 --- */
    err = g_uart3.p_api->open(g_uart3.p_ctrl, g_uart3.p_cfg);
    if (FSP_SUCCESS != err) {
        return;
    }

    /* --- 2. 切换波特率到 115200 --- */
    {
        sci_b_baud_setting_t baud_115200 = {
            .baudrate_bits_b.abcse = 0,
            .baudrate_bits_b.abcs  = 0,
            .baudrate_bits_b.bgdm  = 1,
            .baudrate_bits_b.cks   = 0,
            .baudrate_bits_b.brr   = 64,
            .baudrate_bits_b.mddr  = (uint8_t)256,
            .baudrate_bits_b.brme  = false
        };

        err = g_uart3.p_api->baudSet(g_uart3.p_ctrl, &baud_115200);
        if (FSP_SUCCESS != err) {
        }
    }

    /* --- 3. 初始化环形缓冲区和状态变量 --- */
    esp32_ringbuf_init();
    g_esp32_wifi_state = ESP32_WIFI_DISCONNECTED;
    g_esp32_led_state  = ESP32_LED_OFF;
    g_esp32_cmd        = ESP32_CMD_NONE;
    g_uart3_tx_done    = 1;

    /* --- 4. 向 ESP32 发送握手信号 --- */
    esp32_send_string("connect ok");

}
