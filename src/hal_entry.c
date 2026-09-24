#include "hal_data.h"
#include "headfile.h"
#include "speaker_verify.h"
#include "command_dispatch.h"
//#include "ov5640.h"  /* 摄像头临时禁用 */
#include <math.h>

#if 0 /* ── Camera + LCD self-test (disabled, uncomment when hardware ready) ── */
#include "ov5640.h"

/* ===== Camera self-test buffer (QVGA 320x240 RGB565) ===== */
#define CAM_BUF_QVGA  (320u * 240u * 2u)
static uint8_t g_cam_buf[CAM_BUF_QVGA]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(".sdram_noinit");

/* ====================================================================
 *   Camera + LCD 分层自检 (不改业务逻辑, 只看每层通不通)
 * ==================================================================== */
static void cam_lcd_self_test(void)
{
    fsp_err_t err;
    int step = 0;

    /* ── Step 1: LCD 自检 ── */
    printf("\r\n[SELF-TEST] %d: LCD init...\r\n", ++step);
    LCD_Init();
    LCD_Clear(BLACK);
    LCD_Fill(0, 0, 100, 40, 0xF800);       /* 红 */
    LCD_Fill(100, 0, 200, 40, 0x07E0);      /* 绿 */
    LCD_Fill(200, 0, 300, 40, 0x001F);      /* 蓝 (BGR565) */
    LCD_BLK_On;
    printf("[SELF-TEST] %d: LCD PASS (check top strip)\r\n", step);

    /* ── Step 2: OV5640 I2C 芯片 ID ── */
    printf("[SELF-TEST] %d: OV5640 I2C probe...\r\n", ++step);
    err = g_i2c_master0.p_api->open(g_i2c_master0.p_ctrl, g_i2c_master0.p_cfg);
    if (FSP_SUCCESS != err && FSP_ERR_ALREADY_OPEN != err) {
        printf("[SELF-TEST] %d: FAIL — I2C0 open err=%d\r\n", step, err);
        return;
    }
    err = g_i2c_master0.p_api->slaveAddressSet(g_i2c_master0.p_ctrl,
                                                OV5640_I2C_ADDR, I2C_MASTER_ADDR_MODE_7BIT);
    if (FSP_SUCCESS != err) {
        printf("[SELF-TEST] %d: FAIL — I2C addr set err=%d\r\n", step, err);
        return;
    }
    uint16_t chip_id = 0;
    err = ov5640_check_chip_id(&chip_id);
    if (FSP_SUCCESS != err) {
        printf("[SELF-TEST] %d: FAIL — chip ID err=%d (got 0x%04X)\r\n", step, err, chip_id);
        return;
    }
    printf("[SELF-TEST] %d: PASS — chip ID=0x%04X\r\n", step, chip_id);

    /* ── Step 2.5: Enable CEU clock (MSTPCRC bit 16) ── */
    printf("[SELF-TEST] %d: enable CEU clock...\r\n", ++step);
    {
        uint16_t prcr_save = R_SYSTEM->PRCR;  /* save protection state */
        R_SYSTEM->PRCR = (uint16_t)((prcr_save & 0xFF00) | 0xA501);  /* unlock */
        R_MSTP->MSTPCRC &= ~(1U << 16);       /* release CEU from module-stop */
        R_SYSTEM->PRCR = prcr_save;            /* restore protection */
        printf("[SELF-TEST] %d: CEU clock enabled\r\n", step);
    }

    /* ── Step 3: OV5640 init (QVGA + RGB565 + CEU open) ── */
    printf("[SELF-TEST] %d: OV5640 full init...\r\n", ++step);
    err = ov5640_init();
    if (FSP_SUCCESS != err) {
        printf("[SELF-TEST] %d: FAIL — ov5640_init err=%d\r\n", step, err);
        return;
    }
    /* re-apply QVGA to match CEU 320x240 */
    err = ov5640_set_resolution(OV5640_RES_QVGA);
    printf("[SELF-TEST] %d: PASS (OV5640 init OK, res set to QVGA)\r\n", step);

    /* ── Step 4: stream on ── */
    printf("[SELF-TEST] %d: stream on...\r\n", ++step);
    err = ov5640_stream_on();
    if (FSP_SUCCESS != err) {
        printf("[SELF-TEST] %d: FAIL — stream_on err=%d\r\n", step, err);
        return;
    }
    printf("[SELF-TEST] %d: PASS\r\n", step);

    /* ── Step 5: CEU captureStart ── */
    printf("[SELF-TEST] %d: captureStart...\r\n", ++step);
    err = ov5640_capture_start(g_cam_buf, CAM_BUF_QVGA);
    if (FSP_SUCCESS != err) {
        printf("[SELF-TEST] %d: FAIL — captureStart err=%d\r\n", step, err);
        return;
    }
    printf("[SELF-TEST] %d: PASS (waiting for first frame...)\r\n", step);

    /* ── Step 6: wait first frame (max 2s) ── */
    printf("[SELF-TEST] %d: polling frame...\r\n", ++step);
    for (int tick = 0; tick < 2000; tick++) {
        if (g_capture_complete) {
            printf("[SELF-TEST] %d: PASS — frame arrived after %d ms\r\n", step, tick);
            return;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    printf("[SELF-TEST] %d: FAIL — no frame in 2s (CEU vsync/pclk?)\r\n", step);
}
#endif /* ── Camera + LCD self-test ── */

#if 0 /* ── 摄像头 LCD 实时显示 (临时禁用: CEU 与 I2C 底层冲突待解决) ── */
/* ====================================================================
 *   摄像头 LCD 实时显示 (非阻塞, 不干扰语音识别)
 *   OV5640 YUV422 QVGA → CEU(DMA中断) → 分块 LCD 显示
 *   分块原因: LCD 是 GPIO 模拟 SPI, 整帧显示会阻塞数百 ms 导致丢音频
 * ==================================================================== */
#define CAM_W            320
#define CAM_H            240
#define CAM_BUF_SIZE     (CAM_W * CAM_H * 2)   /* YUV422, 每像素2字节 */

static uint8_t g_cam_buf[CAM_BUF_SIZE]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(".sdram_noinit");

/* 分块显示: 每块约 9600 像素 (~30ms), 分散到多个主循环迭代 */
#define CAM_DISP_BLOCK_PIXELS  9600

static bool      g_cam_enabled = false;
static uint8_t   g_cam_state   = 0;   /* 0=idle 1=capturing 2=displaying */
static uint32_t  g_disp_pos    = 0;
static uint16_t *g_disp_img    = NULL;
static uint32_t  g_disp_total  = 0;

/* 摄像头初始化 (YUV422 + QVGA + LCD) */
static void cam_init(void)
{
    fsp_err_t err;

    LCD_Init();
    LCD_Display_Dir(LCD_DIR_Mode);
    LCD_Clear(BLACK);

    err = ov5640_init();
    if (FSP_SUCCESS != err) {
        printf("CAM: ov5640 init fail %d\r\n", err);
        return;
    }
    uint16_t cid = 0;
    ov5640_check_chip_id(&cid);
    printf("CAM: OV5640 chip id=0x%04X\r\n", cid);
    ov5640_set_resolution(OV5640_RES_QVGA);
    ov5640_set_output_format(OV5640_FORMAT_YUV422);   /* 关键: 匹配 LCD_ShowImage 的 YVYU 解析 */

    g_cam_enabled = true;
    g_cam_state   = 0;
    printf("CAM: ready (QVGA YUV422)\r\n");
}

/* 开始分块显示一帧 (设置窗口 + 拉低CS, 保持连续发送) */
static void cam_disp_start(uint16_t *img, uint16_t w, uint16_t h)
{
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA16(0);
    LCD_WR_DATA16(w - 1);
    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA16(0);
    LCD_WR_DATA16(h - 1);
    LCD_WR_REG(lcddev.wramcmd);
    LCD_CS_CLR;
    LCD_RS_SET;
    g_disp_pos   = 0;
    g_disp_img   = img;
    g_disp_total = (uint32_t)w * h;
}

/* 显示一块, 返回 true = 整帧完成 */
static bool cam_disp_step(void)
{
    uint32_t end = g_disp_pos + CAM_DISP_BLOCK_PIXELS;
    if (end > g_disp_total) end = g_disp_total;

    for (; g_disp_pos < end; g_disp_pos += 2) {
        uint16_t p0 = g_disp_img[g_disp_pos];
        uint16_t p1 = g_disp_img[g_disp_pos + 1];
        int y0 = p0 & 0xFF, v = (p0 >> 8) & 0xFF;
        int y1 = p1 & 0xFF, u = (p1 >> 8) & 0xFF;

        int c0 = y0 - 16, c1 = y1 - 16, d = u - 128, e = v - 128;
        int r0 = (298 * c0 + 409 * e + 128) >> 8;
        int g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
        int b0 = (298 * c0 + 516 * d + 128) >> 8;
        int r1 = (298 * c1 + 409 * e + 128) >> 8;
        int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
        int b1 = (298 * c1 + 516 * d + 128) >> 8;
        if (r0 < 0) r0 = 0;
        if (r0 > 255) r0 = 255;
        if (g0 < 0) g0 = 0;
        if (g0 > 255) g0 = 255;
        if (b0 < 0) b0 = 0;
        if (b0 > 255) b0 = 255;
        if (r1 < 0) r1 = 0;
        if (r1 > 255) r1 = 255;
        if (g1 < 0) g1 = 0;
        if (g1 > 255) g1 = 255;
        if (b1 < 0) b1 = 0;
        if (b1 > 255) b1 = 255;

        uint16_t px0 = (uint16_t)(((b0 >> 3) << 11) | ((r0 >> 3) << 5) | (g0 >> 2));
        uint16_t px1 = (uint16_t)(((b1 >> 3) << 11) | ((r1 >> 3) << 5) | (g1 >> 2));

        SPI2_ReadWriteByte((uint8_t)(px0 >> 8));
        SPI2_ReadWriteByte((uint8_t)px0);
        SPI2_ReadWriteByte((uint8_t)(px1 >> 8));
        SPI2_ReadWriteByte((uint8_t)px1);
    }

    if (g_disp_pos >= g_disp_total) {
        LCD_CS_SET;
        return true;
    }
    return false;
}

/* 摄像头非阻塞步进: 捕获(DMA) → 分块显示, 每轮只做一小步 */
static void cam_step(void)
{
    if (!g_cam_enabled) return;

    static int cam_dbg = 0;
    if (cam_dbg < 8) { printf("  cam_step st=%d\r\n", g_cam_state); cam_dbg++; }

    switch (g_cam_state) {
    case 0:  /* idle → 启动一次 DMA 捕获 (非阻塞) */
        if (FSP_SUCCESS == ov5640_capture_start(g_cam_buf, CAM_BUF_SIZE)) {
            g_cam_state = 1;
        }
        break;
    case 1:  /* capturing → 等 CEU 中断置位 frame_ready */
        if (ov5640_is_frame_ready()) {
            ov5640_clear_frame_flag();
            /* CEU DMA 写入了 SDRAM, 刷新 D-Cache 让 CPU 读到新帧 */
            SCB_InvalidateDCache_by_Addr(g_cam_buf, CAM_BUF_SIZE);
            cam_disp_start((uint16_t *)g_cam_buf, CAM_W, CAM_H);
            g_cam_state = 2;
        }
        break;
    case 2:  /* displaying → 显示一小块 */
        if (cam_disp_step()) {
            g_cam_state = 0;   /* 整帧显示完, 开始下一帧 */
        }
        break;
    default:
        g_cam_state = 0;
        break;
    }
}
#endif /* ── 摄像头 LCD 实时显示 ── */

/* ===== 1ms Timer callback ===== */
volatile uint32_t msTicks = 0;   /* 全局 1ms 计数 (LCD 动画/提示计时用) */

/* 语音播报盲区: 任何语音播报后屏蔽识别, 防麦克风拾取播报声形成反馈循环 (voice.c 里设置) */
volatile uint32_t g_voice_blind_until = 0;

/* VAD 触发灵敏度 (串口屏可调, 默认 1800 = 原 SPEECH_TH_MIN) */
int g_vad_th_min = 1800;

void timer_1ms_callback(timer_callback_args_t *p_args)
{
    if (TIMER_EVENT_CYCLE_END == p_args->event)
    {
        msTicks++;
        g_system_tick();
        timeout_search_1ms();   /* AS608 RX timeout detection */
        esp32_rtc_tick_1ms();   /* ESP32 comm timeout */
    }
}

#if (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
bsp_ipc_semaphore_handle_t g_core_start_semaphore = { .semaphore_num = 0 };
#endif

volatile bool g_uart9_tx_done = true;
volatile int  motor_flag = 0;

/* ====================================================================
 *           Serial RX ring buffer (non-blocking character input)
 * ==================================================================== */
#define RX_RING_SIZE  32
static volatile uint8_t g_rx_ring[RX_RING_SIZE];
static volatile int     g_rx_head = 0;   /* ISR writes here */
static volatile int     g_rx_tail = 0;   /* main loop reads here */

void user_uart9_callback(uart_callback_args_t * p_args)
{
    if (NULL == p_args) { return; }

    if (UART_EVENT_TX_COMPLETE == p_args->event) {
        g_uart9_tx_done = true;
    }

    /* Single character received (no active read() in progress) */
    if (UART_EVENT_RX_CHAR == p_args->event) {
        int next = (g_rx_head + 1) % RX_RING_SIZE;
        if (next != g_rx_tail) {   /* drop if ring full */
            g_rx_ring[g_rx_head] = (uint8_t)(p_args->data & 0xFF);
            g_rx_head = next;
        }
    }
}

/** Non-blocking: returns 1 and stores char, or 0 if none available */
static int serial_poll_char(uint8_t *ch)
{
    if (g_rx_tail == g_rx_head) return 0;
    *ch = g_rx_ring[g_rx_tail];
    g_rx_tail = (g_rx_tail + 1) % RX_RING_SIZE;
    return 1;
}

int _write(int fd, char * pBuffer, int size)
{
    FSP_PARAMETER_NOT_USED(fd);
    if ((size <= 0) || (NULL == pBuffer)) { return 0; }
    g_uart9_tx_done = false;
    g_uart9.p_api->write(g_uart9.p_ctrl, (uint8_t *)pBuffer, (uint32_t)size);
    while (!g_uart9_tx_done) { ; }
    return size;
}


/* ====================================================================
 *                         MAIN ENTRY
 * ==================================================================== */
#define SILENCE_LIMIT      14   /* ~0.85s total silence to end utterance */
#define SILENCE_FEED_MAX   5    /* only feed first ~0.3s of silence to MFCC */
#define COOLDOWN_CHUNKS   3    /* ~0.18s 盲区: SILENCE_LIMIT(14=~0.85s) 已等够静音, 这里只做极小防回音 */
#define TMPL_SAMPLES       (8347 * 2)  /* 2 seconds */

static int16_t tmpl_buf[TMPL_SAMPLES]
    BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(".sdram_noinit");

typedef enum {
    MODE_RECOG = 0,    /* VAD + DTW recognition */
    MODE_COLLECT = 1,  /* template collection */
    MODE_ENROLL = 2    /* speaker enrollment */
} work_mode_t;

void hal_entry(void)
{
    fsp_err_t err;

    /* --- 0. Debug UART --- */
    g_uart9.p_api->open(g_uart9.p_ctrl, g_uart9.p_cfg);

    /* --- 1ms timer --- */
    g_timer_1ms.p_api->open(g_timer_1ms.p_ctrl, g_timer_1ms.p_cfg);
    g_timer_1ms.p_api->start(g_timer_1ms.p_ctrl);

    /* --- INMP441 mic --- */
    err = inmp441_init();
    if (FSP_SUCCESS != err) {
        printf("INMP441 init fail: %d\r\n", err);
        while(1);
    }

    /* --- MFCC engine --- */
    mfcc_engine_init();

    /* --- LCD 显示 (待机笑脸 + 中文提示, 独立于摄像头) --- */
    LCD_Init();
    LCD_Display_Dir(LCD_DIR_Mode);
    LCD_Clear(BLACK);
    LCD_ShowAvatar(70, 19);    /* 180x162 头像, 居中于原笑脸位置(160,100) */

    /* --- Voice command engine --- */
    voice_cmd_init();

    /* --- Load speaker models from NVRAM (.noinit) --- */
    speaker_load();

    /* --- TJC serial screen (UART6) --- */
    UART_HIM_Init();
    printf("TJC: screen init done\r\n");

    /* --- 天问语音模块 (UART1) --- */
    Voice_Init();
    printf("Voice: module init done\r\n");

    /* --- AS608 fingerprint (UART2) --- */
    AS608_Init();
    printf("AS608: fingerprint init done\r\n");

    /* --- PN532 NFC (I2C2) --- */
    PN532_INIT();
    printf("PN532: NFC init done\r\n");

    /* --- ESP32 comm (UART3) --- */
    ESP32_Comm_Init();
    printf("ESP32: comm init done\r\n");

    /* --- 打开 CEU (临时禁用: 摄像头停用, CEU open 会与 I2C 冲突) --- */
    //{
    //    fsp_err_t ceu_err = g_ceu0.p_api->open(g_ceu0.p_ctrl, g_ceu0.p_cfg);
    //    if (FSP_SUCCESS == ceu_err || FSP_ERR_ALREADY_OPEN == ceu_err) {
    //        printf("CEU: ready\r\n");
    //    } else {
    //        printf("CEU: open failed (%d)\r\n", ceu_err);
    //    }
    //}

    /* 初始化网络状态为断开 (连接成功后 ESP32 发 @2 → va1.val=1) */
    tjc_send_val("va1", "val", 0);

    /* 请求 ESP32 发回声纹 (断电恢复) — ESP32 收到 @GETSPK 后读 speaker.txt 发回 */
    esp32_send_string("@GETSPK");

    printf("========================================\r\n");
    printf("  MODE: RECOG  (r=recog c=collect e=enroll g=erase)\r\n");
    printf("========================================\r\n\r\n");

    /* ================================================================
     *   Per-mode state
     * ================================================================ */
    work_mode_t  mode        = MODE_RECOG;
    bool         collecting  = false;
    int          coll_pos    = 0;

    /* ---- recognition mode state ---- */
    int          silence_cnt = 0;
    bool         speaking    = false;
    int32_t      noise_floor = 400;   /* adaptive noise floor, initial estimate */
    int32_t      noise_max   = 400;   /* peak noise for spike rejection */
    int          noise_run   = 0;     /* consecutive noise-chunk counter */
    int          cooldown    = 0;     /* cooldown counter after trigger */

    /* ---- wake word state ---- */
    #define AWAKE_WINDOW_MS 5000      /* 唤醒后5秒内可说指令 */
    bool         awake       = false;  /* false=休眠(只听"大白"), true=已唤醒(听指令) */
    uint32_t     awake_until = 0;      /* 唤醒窗口截止时间 (msTicks) */

    while (1)
    {
        /* ============================================================
         *  1. Process serial commands (non-blocking)
         * ============================================================ */
        uint8_t ch;
        while (serial_poll_char(&ch))
        {
            if (ch == 'c') {
                /* Switch to collection mode */
                mode        = MODE_COLLECT;
                collecting  = false;
                coll_pos    = 0;
                speaking    = false;
                mfcc_engine_reset();
                printf("\r\n=== MODE: COLLECT ===\r\n");
                printf("  k = record 2s  r = switch to recog\r\n\r\n");
            }
            else if (ch == 'r') {
                /* Switch to recognition mode */
                mode        = MODE_RECOG;
                collecting  = false;
                coll_pos    = 0;
                silence_cnt = 0;
                speaking    = false;
                cooldown    = 0;
                mfcc_engine_reset();
                voice_cmd_reset();
                printf("\r\n=== MODE: RECOG ===\r\n\r\n");
            }
            else if (ch == 'k') {
                if (mode == MODE_COLLECT && !collecting) {
                    collecting = true;
                    coll_pos   = 0;
                    printf("COLLECTING...\r\n");
                }
                else if (mode == MODE_ENROLL && !collecting) {
                    collecting = true;
                    coll_pos   = 0;
                    printf("ENROLLING...\r\n");
                }
            }
            else if (ch == 'e') {
                mode        = MODE_ENROLL;
                collecting  = false;
                coll_pos    = 0;
                speaking    = false;
                mfcc_engine_reset();
                printf("\r\n=== MODE: ENROLL ===\r\n");
                printf("  k=record 8s  f=finalize  r=recog\r\n");
                if (speaker_get_count() >= SPK_MAX_ENROLLED)
                    printf("  (speaker table FULL)\r\n");
                printf("\r\n");
            }
            else if (ch == 'f') {
                if (mode == MODE_ENROLL) {
                    int idx = speaker_finalize();
                    if (idx >= 0) {
                        printf("SPEAKER %d ENROLLED\r\n\r\n", idx);
                        /* 导出声纹 → ESP32 存储 (断电恢复) */
                        char spk_hex[1024];
                        char spk_cmd[1040];
                        speaker_export_hex(spk_hex, sizeof(spk_hex));
                        sprintf(spk_cmd, "@SPK:%s", spk_hex);
                        esp32_send_string(spk_cmd);
                        printf("SPK: send ESP32\r\n");
                    } else {
                        printf("ENROLL FAILED\r\n\r\n");
                    }
                }
            }
            else if (ch == 'g') {
                speaker_erase();
                esp32_send_string("@DELSPK");  /* 通知 ESP32 删除 speaker.txt */
                printf("SPEAKER DATA ERASED (RA8P1+ESP32)\r\n\r\n");
            }
        }

        /* ============================================================
         *  2. Poll background tasks (non-blocking)
         * ============================================================ */
        dispatch_poll();
        finger_search();   /* 后台持续指纹扫描 (非阻塞) */
        esp32_process();   /* 处理 ESP32 发来的数据 (时间/库存/WiFi状态) */
        cabinet_timeout_check();  /* 柜门70秒超时自动关闭+取消认证 */
        auth_timeout_check();     /* 认证70秒超时退出(纯认证无柜门时) */
        HIM_connection();  /* 串口屏命令帧处理 (开柜门/关柜门/WiFi等) */
        tjc_pending_poll();  /* 发送到期的延迟串口屏命令 (非阻塞) */
        voice_pending_poll(); /* 发送到期的延迟语音播报 (非阻塞) */
        LCD_Poll();          /* LCD轮询: 3秒变"..." + 表情眨眼 (非阻塞) */
        //cam_step();          /* 摄像头非阻塞 (临时禁用) */

        /* 每秒推送 RTC 时间到 TJC 串口屏 (ESP32 未连接时用默认时间继续走) */
        if (esp32_rtc_second_changed()) {
            char time_buf[24];
            char tjc_buf[40];
            esp32_rtc_get_string(time_buf, sizeof(time_buf));
            sprintf(tjc_buf, "t8.txt=\"%s\"", time_buf);
            tjc_send_string(tjc_buf);

            /* 声纹恢复重试: ESP32 后上电时, 启动发的 @GETSPK 会丢,
               周期性重发直到声纹导入 (每2秒一次, 最多120秒) */
            static int spk_retry_sec = 0;
            if (!speaker_has_enrolled()) {
                spk_retry_sec++;
                if (spk_retry_sec <= 120 && (spk_retry_sec % 2) == 0) {
                    esp32_send_string("@GETSPK");
                }
            } else {
                spk_retry_sec = 0;
            }
        }

        /* ============================================================
         *  4. Get audio buffer
         * ============================================================ */
        int16_t *ab = inmp441_get_buffer();
        if (ab == NULL) continue;

        /* ============================================================
         *  4a. COLLECTION MODE
         * ============================================================ */
        if (mode == MODE_COLLECT)
        {
            if (!collecting) continue;

            int n = INMP441_SAMPLES_PER_BUF;
            if (coll_pos + n > TMPL_SAMPLES) n = TMPL_SAMPLES - coll_pos;
            memcpy(&tmpl_buf[coll_pos], ab, n * sizeof(int16_t));
            coll_pos += n;

            if (coll_pos >= TMPL_SAMPLES) {
                /* Collection complete: VAD-trim then compute MFCC */
                collecting = false;
                coll_pos   = 0;

                /* ---- VAD: find speech segment in 2s buffer ---- */
                #define COLL_CHUNK  512
                int n_chunks = TMPL_SAMPLES / COLL_CHUNK;
                int first_spk = -1, last_spk = -1;
                int32_t max_rms = 0;
                static int32_t chunk_rms[TMPL_SAMPLES / COLL_CHUNK];

                for (int c = 0; c < n_chunks; c++) {
                    int64_t sq = 0;
                    for (int s = 0; s < COLL_CHUNK; s++)
                        { int32_t v = tmpl_buf[c*COLL_CHUNK+s]; sq += (int64_t)v*v; }
                    int32_t r = (int32_t)sqrtf((float)(sq / COLL_CHUNK));
                    chunk_rms[c] = r;
                    if (r > max_rms) max_rms = r;
                }
                int32_t vad_th = max_rms / 3;     /* 峰值 1/3: 跟随每条指令自身音量 */
                if (vad_th < 1500) vad_th = 1500; /* 绝对下限: 排除环境噪声, 又不切掉可乐的元音 */

                for (int c = 0; c < n_chunks; c++) {
                    if (chunk_rms[c] >= vad_th) {
                        if (first_spk < 0) first_spk = c;
                        last_spk = c;
                    }
                }

                /* ---- Quality checks ---- */
                int spk_chunks = (first_spk >= 0) ? (last_spk - first_spk + 1) : 0;
                float spk_sec  = spk_chunks * COLL_CHUNK / 8347.0f;

                if (first_spk < 0) {
                    printf("\r\nBAD: no speech detected (k=retry)\r\n");
                    continue;
                }
                if (spk_chunks < 5) {
                    printf("\r\nBAD: too short %.1fs (k=retry)\r\n", (double)spk_sec);
                    continue;
                }
                if (max_rms < 2000) {
                    printf("\r\nBAD: too quiet peak=%ld (k=retry)\r\n", (long)max_rms);
                    continue;
                }

                /* Pad: 2 chunks before, 3 after */
                int pad_before = 2, pad_after = 3;
                int start_chunk = first_spk - pad_before;
                if (start_chunk < 0) start_chunk = 0;
                int end_chunk = last_spk + pad_after;
                if (end_chunk >= n_chunks) end_chunk = n_chunks - 1;

                int trim_start = start_chunk * COLL_CHUNK;
                int trim_len   = (end_chunk - start_chunk + 1) * COLL_CHUNK;

                mfcc_engine_process_all(&tmpl_buf[trim_start], trim_len);
                int nf = mfcc_engine_get_frame_count();
                printf("\r\nM:%d ", nf);
                for (int i = 0; i < nf * 13; i++)
                    printf("%.6f%s", (double)mfcc_engine_get_features()[i],
                           i < nf * 13 - 1 ? "," : "");
                printf("\r\n");
                mfcc_engine_reset();
                printf("OK nf=%d peak=%ld dur=%.1fs (k=next r=recog)\r\n",
                       nf, (long)max_rms, (double)spk_sec);
            }
            continue;
        }

        /* ============================================================
         *  4b. ENROLLMENT MODE (record fixed length, extract MFCC, enroll)
         * ============================================================ */
        if (mode == MODE_ENROLL)
        {
            if (!collecting) continue;

            #define ENROLL_SAMPLES  (8347 * 10)  /* 10 seconds */
            static int16_t enroll_buf[ENROLL_SAMPLES]
                BSP_ALIGN_VARIABLE(32) BSP_PLACE_IN_SECTION(".sdram_noinit");
            static int enroll_pos = 0;

            int n = INMP441_SAMPLES_PER_BUF;
            if (enroll_pos + n > ENROLL_SAMPLES) n = ENROLL_SAMPLES - enroll_pos;
            memcpy(&enroll_buf[enroll_pos], ab, n * sizeof(int16_t));
            enroll_pos += n;

            if (enroll_pos >= ENROLL_SAMPLES) {
                collecting  = false;
                enroll_pos = 0;

                /* ---- VAD: 在 10s 缓冲里定位语音段 (与模板采集一致) ----
                   之前把整整 10s (含 8~9s 静音) 全部拿去求均值,
                   导致声纹均值被静音污染, 与验证时(仅语音帧)不匹配 → sim 偏低。 */
                #define ENROLL_CHUNK  512
                int n_chunks = ENROLL_SAMPLES / ENROLL_CHUNK;
                int first_spk = -1, last_spk = -1;
                int32_t max_rms = 0;
                static int32_t enroll_chunk_rms[ENROLL_SAMPLES / ENROLL_CHUNK];

                for (int c = 0; c < n_chunks; c++) {
                    int64_t sq = 0;
                    for (int s = 0; s < ENROLL_CHUNK; s++)
                        { int32_t v = enroll_buf[c*ENROLL_CHUNK+s]; sq += (int64_t)v*v; }
                    int32_t r = (int32_t)sqrtf((float)(sq / ENROLL_CHUNK));
                    enroll_chunk_rms[c] = r;
                    if (r > max_rms) max_rms = r;
                }
                int32_t vad_th = max_rms / 3;     /* 峰值 1/3: 跟随每条指令自身音量 */
                if (vad_th < 2000) vad_th = 2000; /* 绝对下限: 高于环境噪声(1850~2090), 防止声纹被噪声污染 */

                for (int c = 0; c < n_chunks; c++) {
                    if (enroll_chunk_rms[c] >= vad_th) {
                        if (first_spk < 0) first_spk = c;
                        last_spk = c;
                    }
                }

                if (first_spk < 0) {
                    printf("\r\nENROLL: no speech (k=retry)\r\n");
                    continue;
                }

                /* 前后各留少量余量 */
                int pad_before = 2, pad_after = 3;
                int start_chunk = first_spk - pad_before;
                if (start_chunk < 0) start_chunk = 0;
                int end_chunk = last_spk + pad_after;
                if (end_chunk >= n_chunks) end_chunk = n_chunks - 1;

                int trim_start = start_chunk * ENROLL_CHUNK;
                int trim_len   = (end_chunk - start_chunk + 1) * ENROLL_CHUNK;

                /* 分段提取: MFCC 单次上限 250 帧(≈2.5s), 整段语音(可达10s)
                   若只 process 一次会丢弃 2.5s 之后的内容。分段让整段都进声纹。 */
                #define ENROLL_SEG_SAMPLES  (MFCC_MAX_FRAMES * MFCC_HOP_SAMPLES)  /* 250*84=21000 ≈2.5s */
                int seg_count = 0;
                int total_nf  = 0;
                for (int off = 0; off < trim_len; off += ENROLL_SEG_SAMPLES) {
                    int seg_len = trim_len - off;
                    if (seg_len > ENROLL_SEG_SAMPLES) seg_len = ENROLL_SEG_SAMPLES;
                    mfcc_engine_process_all(&enroll_buf[trim_start + off], seg_len);
                    int nf = mfcc_engine_get_frame_count();
                    if (nf >= 15) {
                        speaker_enroll(mfcc_engine_get_features(), nf, NULL);
                        total_nf += nf;
                        seg_count++;
                    }
                    mfcc_engine_reset();
                }
                if (seg_count > 0) {
                    printf("ENROLL: done %d seg(s) %d frames dur=%.1fs (k=again f=finalize)\r\n",
                           seg_count, total_nf,
                           (double)((end_chunk - start_chunk + 1) * ENROLL_CHUNK) / 8347.0);
                } else {
                    printf("ENROLL: too short (k=retry)\r\n");
                }
            }
            continue;
        }

        /* ============================================================
         *  4c. RECOGNITION MODE (adaptive VAD + utterance-end DTW + speaker verify)
         * ============================================================ */

        /* Compute RMS of this chunk */
        int64_t sum_sq = 0;
        for (int i = 0; i < INMP441_SAMPLES_PER_BUF; i++) {
            int32_t s = ab[i]; sum_sq += (int64_t)s * s;
        }
        int32_t rms = (int32_t)sqrtf((float)(sum_sq / INMP441_SAMPLES_PER_BUF));

        /* 施密特滞回: 进入说话用高阈值, 说话中判静音用低阈值,
           防止一句话内部的轻微停顿被误判成句子结束(误拒来源之一) */
        int32_t speech_th_on  = noise_floor + 500;
        int32_t speech_th_off = noise_floor + 200;
        if (speech_th_on  < g_vad_th_min) speech_th_on  = g_vad_th_min;
        if (speech_th_off < g_vad_th_min) speech_th_off = g_vad_th_min;

        /* Cooldown after trigger: ignore all audio briefly */
        if (cooldown > 0) {
            cooldown--;
            continue;
        }

        /* 指令触发后盲区: 语音播报期间屏蔽识别, 防止麦克风拾取播报声形成反馈循环 */
        if (msTicks < g_voice_blind_until) {
            continue;
        }

        /* 唤醒超时: 唤醒后5秒没指令, 回休眠 */
        if (awake && msTicks > awake_until) {
            awake = false;
            printf("[WAKE] 超时, 回休眠\r\n");
        }

        if (rms >= (speaking ? speech_th_off : speech_th_on)) {
            /* ---- VOICE detected ---- */
            noise_run = 0;
            if (!speaking) {
                speaking = true;
                mfcc_engine_reset();
                /* 诊断: 打印触发时 rms/noise_floor/th/t(msTicks), 定位声源 (测完可删) */
                //printf("[VAD] trig rms=%ld floor=%ld th=%ld t=%lu\r\n",
                //       (long)rms, (long)noise_floor, (long)speech_th_on, (unsigned long)msTicks);
            }
            silence_cnt = 0;
            mfcc_engine_feed(ab, INMP441_SAMPLES_PER_BUF);
        }
        else if (speaking) {
            /* ---- Silent while speaking: feed a few to capture tail ---- */
            silence_cnt++;
            if (silence_cnt <= SILENCE_FEED_MAX) {
                mfcc_engine_feed(ab, INMP441_SAMPLES_PER_BUF);
            }
            if (silence_cnt >= SILENCE_LIMIT) {
                /* End of utterance — run DTW once */
                speaking = false;
                cooldown = COOLDOWN_CHUNKS;
                int nf = mfcc_engine_get_frame_count();
                if (nf >= 15) {
                    const float *feat = mfcc_engine_get_features();

                    if (!awake) {
                        /* 休眠: 只检测唤醒词"大白", 不识别指令 */
                        if (voice_wake_detect(feat, nf)) {
                            awake = true;
                            awake_until = msTicks + AWAKE_WINDOW_MS;
                            voice_send_string("@23");   /* 语音: "在" (内部设8秒盲区) */
                            g_voice_blind_until = msTicks + 1000;  /* 盲区1秒: "在"播报很短, 防尾音混入即可 */
                            printf("[WAKE] 已唤醒, 5秒内可说指令\r\n");
                        }
                    } else {
                        /* 已唤醒: 检测指令 */
                        int spk = speaker_verify(feat, nf);   /* >=0 本人 / -1 灰区 / -2 硬拒 */
                        if (spk == -2 && speaker_has_enrolled()) {
                            /* 硬拒: sim < 0.45, 陌生声音, 跳过 DTW (环境噪声频繁触发, 不打印) */
                        } else {
                            /* 灰区安全网: 声纹在 [0.45,0.52) 时, 命令必须极确信(margin 额外+0.04)
                               才放行 —— 给"本人 sim 偶尔偏低(离远/小声)"留兜底, 同时挡陌生边缘声音 */
                            g_dtw_extra_margin = (spk == -1 && speaker_has_enrolled()) ? 0.04f : 0.0f;

                            /* 声纹验证通过 → 记录当前用户 (仅当已录入声纹时) */
                            if (spk >= 0 && speaker_has_enrolled()) {
                                g_current_user = spk + 1;  /* 0→张三,1→李四,2→王五,3→老六 */
                                auth_start();  /* 记录认证时间戳, 启动70秒超时退出 */
                            }
                            int cmd = voice_cmd_predict(feat, nf);
                            g_dtw_extra_margin = 0.0f;
                            if (cmd >= 0) {
                                dispatch_command(cmd);
                                /* 触发成功后进入盲区, 覆盖语音播报时长, 防反馈循环 */
                                g_voice_blind_until = msTicks + VOICE_BLIND_MS;
                                /* 执行完指令, 回休眠 */
                                awake = false;
                                awake_until = 0;
                            }
                        }
                    }
                }
                mfcc_engine_reset();
            }
        }
        else {
            /* ---- Not speaking: track noise floor (EMA) + spike rejection ---- */
            if (rms < noise_floor + 500) {
                /* Require 3 consecutive quiet chunks before updating floor */
                if (rms > noise_max) noise_max = rms;
                noise_run++;
                if (noise_run >= 6) {
                    /* 防污染: 若这段"安静"里混入过 spike(关门/旁人说话),
                       noise_max 会远高于真实噪声底, 直接用会抬高 floor
                       导致下一条真命令被漏掉。门控丢弃 + 更慢 EMA 15:1。 */
                    if (noise_max < noise_floor * 3 / 2) {
                        noise_floor = (noise_floor * 15 + noise_max) / 16;
                    }
                    noise_max = rms;
                    noise_run = 0;
                }
            } else {
                /* Spike: reset run counter, don't update floor */
                noise_run = 0;
            }
        }
    }
}

#if (0 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && !BSP_TZ_NONSECURE_BUILD
#if BSP_TZ_SECURE_BUILD
    R_BSP_IpcSemaphoreTake(&g_core_start_semaphore);
#endif
    R_BSP_SecondaryCoreStart();
#if BSP_TZ_SECURE_BUILD
    while(FSP_ERR_IN_USE == R_BSP_IpcSemaphoreTake(&g_core_start_semaphore)) { ; }
#endif
#endif

#if (1 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
    R_BSP_IpcSemaphoreGive(&g_core_start_semaphore);
#endif

#if BSP_TZ_SECURE_BUILD
    R_BSP_NonSecureEnter();
#endif

#if BSP_TZ_SECURE_BUILD
FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable () { }
FSP_CPP_FOOTER
#endif
