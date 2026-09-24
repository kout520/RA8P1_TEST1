/**
 * Command Dispatcher — voice → peripherals (non-blocking)
 */

#include "command_dispatch.h"
#include "templates_dtw.h"
#include <stdio.h>
#include <string.h>

#define AS608_START_STATE  1
#include "AS608pro.h"
#include "esp32_comm.h"
//#include "tjc_usart_hmi.h"
#include "pn532.h"
//#include "lcd.h"

/* ── PN532 globals (defined in pn532.c) ──────────────────────────── */
extern uint8_t result;
extern uint8_t uid_len;

/* ── 跨模块: 当前认证用户 + 串口屏接口 + 语音模块 ─────────────── */
extern int  g_current_user;                                     /* 0=无,1=张三,2=李四,3=王五,4=老六 */
extern void tjc_send_val(char *objname, char *attribute, int val);
extern void tjc_send_string(char *str);
extern void voice_send_string(const char *str);
extern void voice_send_delayed(const char *str, uint32_t delay_ms);
extern void voice_send_attendance(uint8_t id);
extern void LCD_ShowMsg(uint8_t msg_id);
extern void cabinet_open(void);    /* 打开柜门 + 记录70秒超时时间戳 (tjc_usart_hmi.c) */

/* ── Stubs ────────────────────────────────────────────────────────── */
void uart2_send_value(int val)  { (void)val; }
void uart9_send_value(int val)  { (void)val; }
void dakai_clock_display(void)  { }

/* ── State machine ────────────────────────────────────────────────── */
typedef enum {
    TASK_NONE = 0,
    TASK_SHOW_RESULT,
} task_type_t;

static task_type_t g_task      = TASK_NONE;
static int         g_task_cmd  = -1;
static int         g_task_tick = 0;

#define RESULT_TICKS  60

/* 延迟播报物品库存: base=@14(牛奶)/@17(可乐)/@20(宏宝莱), stock clamp 0~2, delay_ms后播报 */
static void voice_announce_stock(int base, int stock, uint32_t delay_ms)
{
    if (stock < 0) stock = 0;
    if (stock > 2) stock = 2;
    char buf[8];
    sprintf(buf, "@%d", base + stock);
    voice_send_delayed(buf, delay_ms);
    printf("Voice: 库存延迟播报 %s (%lums后)\r\n", buf, (unsigned long)delay_ms);
}

/* ── Immediate actions ───────────────────────────────────────────── */
static void cmd_immediate(int cmd_class)
{
    switch (cmd_class) {
    case 0: /* DA_KA — 打卡: 跳转打卡页 + 发打卡 + 刷新积分 + 显示打卡时间 */
        if (g_current_user > 0) {
            tjc_send_val("va0", "val", g_current_user);   /* 打卡页: 张三1 李四2 王五3 */
            esp32_send_attendance(g_current_user);        /* 发打卡 @1/@2/@3 */
            voice_send_attendance(g_current_user);        /* 语音播报: 张三/李四/王五打卡 */
            LCD_ShowMsg(g_current_user);                  /* LCD: 员工张三/李四/王五已打卡 */
            esp32_show_points(g_current_user);            /* 刷新积分 t66 */

            /* 显示打卡时间到 t25 (格式与考勤页 0x10 一致) */
            char time_buf[24];
            char t25_buf[40];
            esp32_rtc_get_string(time_buf, sizeof(time_buf));
            sprintf(t25_buf, "t25.txt=\"%s\"", time_buf);
            tjc_send_string(t25_buf);
        }
        g_task = TASK_SHOW_RESULT; g_task_cmd = 0; g_task_tick = 0;
        break;
    case 1: /* KE_LE — 可乐: 刷新库存 + 播报库存 + 开柜门 */
        if (g_current_user > 0) {
            int cola, sprite, milk;
            esp32_get_k230_stock(&cola, &sprite, &milk);
            tjc_send_val("va0", "val", 7);
            cabinet_open();                    /* 打开柜门 + 记录70秒超时时间戳 */
            voice_send_string("@12");          /* 语音: 已打开柜门, 请取物 */
            voice_announce_stock(17, cola, 3000);  /* 3秒后播报可乐库存 @17/18/19 */
            LCD_ShowMsg(12);                   /* LCD: 好的，已为你打开柜门，请取物 */
        }
        g_task = TASK_SHOW_RESULT; g_task_cmd = 1; g_task_tick = 0;
        break;
    case 2: /* HONG_BAO_LAI — 宏宝莱: 刷新库存 + 播报库存 + 开柜门 */
        if (g_current_user > 0) {
            int cola, sprite, milk;
            esp32_get_k230_stock(&cola, &sprite, &milk);
            tjc_send_val("va0", "val", 7);
            cabinet_open();                      /* 打开柜门 + 记录70秒超时时间戳 */
            voice_send_string("@12");            /* 语音: 已打开柜门, 请取物 */
            voice_announce_stock(20, sprite, 3000);  /* 3秒后播报宏宝莱库存 @20/21/22 */
            LCD_ShowMsg(12);                     /* LCD: 好的，已为你打开柜门，请取物 */
        }
        g_task = TASK_SHOW_RESULT; g_task_cmd = 2; g_task_tick = 0;
        break;
    case 3: /* NIU_NAI — 牛奶: 刷新库存 + 播报库存 + 开柜门 */
        if (g_current_user > 0) {
            int cola, sprite, milk;
            esp32_get_k230_stock(&cola, &sprite, &milk);
            tjc_send_val("va0", "val", 7);
            cabinet_open();                    /* 打开柜门 + 记录70秒超时时间戳 */
            voice_send_string("@12");          /* 语音: 已打开柜门, 请取物 */
            voice_announce_stock(14, milk, 3000);  /* 3秒后播报牛奶库存 @14/15/16 */
            LCD_ShowMsg(12);                   /* LCD: 好的，已为你打开柜门，请取物 */
        }
        g_task = TASK_SHOW_RESULT; g_task_cmd = 3; g_task_tick = 0;
        break;
    case 4: /* XIN_XI — 查看信息: 跳转身份页 va0.val=4/5/6 (张三/李四/王五) */
        if (g_current_user > 0) {
            tjc_send_val("va0", "val", g_current_user + 3);
            voice_send_string("@13");   /* 语音: 您的信息已显示 */
            LCD_ShowMsg(13);             /* LCD: 好的，您的信息已显示 */
        }
        g_task = TASK_SHOW_RESULT; g_task_cmd = 4; g_task_tick = 0;
        break;
    }
}

/* ── Dispatch ────────────────────────────────────────────────────── */

int dispatch_command(int cmd_class)
{
    const char *name = (cmd_class >= 0 && cmd_class < DTW_N_CLASSES)
                       ? g_dtw_names[cmd_class] : "???";

    printf("\r\n[DISPATCH] %s (class=%d)\r\n", name, cmd_class);

    if (g_task != TASK_NONE && g_task != TASK_SHOW_RESULT) {
        printf("  -> Busy, ignoring\r\n");
        return -1;
    }

    cmd_immediate(cmd_class);
    return 0;
}

/* ── Background poll ─────────────────────────────────────────────── */

void dispatch_poll(void)
{
    /* ── NFC: bidirectional debounce (suppress RF bounce on attach & detach) ── */
    static int  nfc_on_cnt  = 0;   /* consecutive uid_len>0 counts */
    static int  nfc_off_cnt = 0;   /* consecutive uid_len==0 counts */
    static bool nfc_present = false;
    #define NFC_DB  10              /* ~450ms debounce window */

    PN532_connect();

    if (uid_len > 0) {
        nfc_on_cnt++;
        nfc_off_cnt = 0;
    } else {
        nfc_off_cnt++;
        nfc_on_cnt = 0;
    }

    bool just_detected = false;
    if (nfc_on_cnt >= NFC_DB && !nfc_present) {
        just_detected = true;
        nfc_present   = true;
    }
    if (nfc_off_cnt >= NFC_DB && nfc_present) {
        nfc_present = false;   /* card truly removed */
    }

    if (g_task == TASK_NONE) return;
    g_task_tick++;

    switch (g_task) {

    case TASK_SHOW_RESULT:
        if (g_task_tick > RESULT_TICKS) {
            g_task = TASK_NONE;
        }
        break;

    default: break;
    }
}
