#ifndef OV5640_H
#define OV5640_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>



extern bool g_capture_complete;


/* OV5640 I2C 地址 (7-bit) */
#define OV5640_ADDR             0x3C    /* SIOD 接 GND: 0x3C, 接 VCC: 0x3D */

/* OV5640 芯片 ID */

#define OV5640_REG_SYS_RESET    OV5640_REG_SYSCTL
#define OV5640_REG_CHIP_ID_H    0x300A
#define OV5640_REG_CHIP_ID_L    0x300B
#define OV5640_CHIP_ID          0x5640
#define OV5640_I2C_ADDR         OV5640_ADDR



/* 输出格式 */
typedef enum
{
    OV5640_FORMAT_RGB565 = 0,
    OV5640_FORMAT_YUV422,
    OV5640_FORMAT_JPEG,
} ov5640_output_format_t;

/* 输出分辨率 */
typedef enum
{
    OV5640_RES_QVGA   = 0,   /* 320x240 */
    OV5640_RES_VGA,           /* 640x480 */
    OV5640_RES_SVGA,          /* 800x600 */
    OV5640_RES_XGA,           /* 1024x768 */
    OV5640_RES_720P,          /* 1280x720 */
    OV5640_RES_SXGA,          /* 1280x1024 */
    OV5640_RES_1080P,         /* 1920x1080 */
    OV5640_RES_QSXGA,         /* 2592x1944 */
} ov5640_resolution_t;

/* 配置结构体 */
typedef struct
{
    ov5640_output_format_t  format;
    ov5640_resolution_t     resolution;
    uint8_t                 light_mode;     /* 0:Auto 1:Sunny 2:Cloudy 3:Office 4:Home */
    uint8_t                 saturation;     /* 0~6, 3=default */
    uint8_t                 brightness;     /* 0~8, 4=default */
    uint8_t                 contrast;       /* 0~6, 3=default */
} ov5640_config_t;

/* 帧缓冲区信息 */
/* 帧缓冲区信息 */
typedef struct
{
    uint8_t                *buffer;
    uint32_t                size;
    uint16_t                width;
    uint16_t                height;
    ov5640_output_format_t  format;
    volatile bool           frame_ready;
} ov5640_frame_t;


/* ov5640_reg_t 的 val 改为 uint8_t，与实际寄存器写入一致 */
typedef struct {
    uint16_t reg;
    uint8_t  val;
} ov5640_reg_t;

/* API 声明统一 —— 删除旧声明，替换为以下 */
fsp_err_t ov5640_init(void);
fsp_err_t ov5640_stream_on(void);
fsp_err_t ov5640_deinit(void);
fsp_err_t ov5640_check_chip_id(uint16_t *chip_id);
fsp_err_t ov5640_set_resolution(ov5640_resolution_t res);
fsp_err_t ov5640_set_output_format(ov5640_output_format_t fmt);
fsp_err_t ov5640_set_light_mode(uint8_t mode);
fsp_err_t ov5640_set_brightness(int8_t level);
fsp_err_t ov5640_set_contrast(uint8_t level);
fsp_err_t ov5640_set_saturation(uint8_t level);
fsp_err_t ov5640_set_test_pattern(bool enable);
fsp_err_t ov5640_set_window(uint16_t x_offset, uint16_t y_offset,
                             uint16_t width, uint16_t height);
fsp_err_t ov5640_auto_focus_init(void);
fsp_err_t ov5640_auto_focus_single(void);
fsp_err_t ov5640_capture_start(uint8_t *p_buffer, uint32_t buffer_size);
fsp_err_t ov5640_capture_wait(uint32_t timeout_ms);
bool      ov5640_is_frame_ready(void);
void      ov5640_clear_frame_flag(void);
ov5640_frame_t *ov5640_get_frame_info(void);

/* I2C 底层 */
fsp_err_t ov5640_write_reg(uint16_t reg, uint8_t value);
fsp_err_t ov5640_read_reg(uint16_t reg, uint8_t *value);


#endif /* OV5640_H */
