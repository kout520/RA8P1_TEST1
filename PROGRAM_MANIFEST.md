# RA8P1_TEST1 -- Complete Program Manifest

**MCU:** Renesas RA8P1 (Cortex-M85 with Helium)  
**Toolchain:** Renesas e2 Studio / FSP  
**Repository:** `d:\e2_workplace\RA8P1_TEST1`  
**Branch:** main  
**Last commit:** `9f217584`  
**Generated:** 2026-07-23  

---

## Table of Contents

1. [Source File Inventory](#1-source-file-inventory)
2. [Generated FSP Files (ra_gen)](#2-generated-fsp-files-ra_gen)
3. [Per-File Function Listing](#3-per-file-function-listing)
4. [Hardware Peripheral Mapping](#4-hardware-peripheral-mapping)
5. [Pin Mapping](#5-pin-mapping)
6. [Interrupt/ISR Mapping](#6-interruptisr-mapping)
7. [Clock Tree](#7-clock-tree)
8. [System Architecture Summary](#8-system-architecture-summary)

---

## 1. Source File Inventory

### 1.1 `src/` -- Application Entry

| File | Lines | Type | Purpose |
|------|-------|------|---------|
| `src/hal_entry.c` | 181 | C | Main application entry point. INMP441 microphone recording and PCM export over UART9. Timer callback (1ms tick), debug UART9 TX callback, `_write()` for printf redirection. |
| `src/hal_warmstart.c` | 55 | C | `R_BSP_WarmStart()` handler. Initializes IOPORT pin configuration and SDRAM after C runtime setup. |

### 1.2 `code/` -- Peripheral Drivers and Modules

| File | Lines | Type | Purpose |
|------|-------|------|---------|
| `code/headfile.h` | 51 | H | Master include header. Includes all module headers and FSP HAL headers. Declares cross-module shared variables (`motor_flag`, color type/enum). |
| `code/key.h` | 12 | H | Key/button module header (empty stub). |
| `code/key.c` | 9 | C | Key/button module implementation (empty stub). |
| `code/lcd.h` | 196 | H | LCD display driver header. Defines ILI9341 register commands, colour constants (`WHITE`, `BLACK`, `RED`, etc.), GPIO pin macros for software SPI (`LCD_SDA`, `LCD_SCL`, `LCD_CS`, `LCD_RST`, `LCD_RS`, `LCD_BLK`), and drawing API declarations. |
| `code/lcd.c` | 1363 | C | LCD display driver implementation. Software SPI bit-bang, ILI9341 register init sequence, graphics primitives (point, line, rect, circle, fill), character display (12/16/24 px ASCII), image display with YVYU-to-RGB565 conversion. GD25Qxx/QSPI flash commented out. |
| `code/font.h` | 57 | H | Font data header. Externs for ASCII 12x12/16x16/24x24 and GB2312 Chinese 16x16/24x24 bitmaps. |
| `code/font.c` | 487 | C | Font glyph data. ASCII 12x12 (95 glyphs), 16x16 (95 glyphs), 24x24 (partial). Chinese 16x16 (56 chars) and 24x24 (9 chars) stubs at end of file. |
| `code/text.h` | 25 | H | Chinese text display API. Declares `Get_HzMat()`, `Show_Font()`, `Show_Str()`, `Draw_Font16B()`, `Draw_Font24B()`. |
| `code/text.c` | 345 | C | Chinese text rendering implementation. GB2312 code lookup, glyph extraction from font tables, display with auto-wrapping. |
| `code/tjc_usart_hmi.h` | 44 | H | TJC serial HMI display header (UART6). Command macros, ring buffer API, control flags. |
| `code/tjc_usart_hmi.c` | 524 | C | TJC serial HMI driver. Ring buffer (500 bytes), command frame parser (0x55 command frames, 0x70 data frames), WiFi SSID/password data reception, touch button handling (lock/unlock, fingerprint enroll/delete, camera start). |
| `code/pn532.h` | 333 | H | PN532 NFC module header (I2C2). Command codes, error codes, MIFARE/NTAG definitions, NDEF URI prefix constants, blocking and non-blocking API declarations. |
| `code/pn532.c` | 479 | C | PN532 NFC protocol implementation. I2C frame encode/decode, SAM config, passive target reading, MIFARE Classic auth/read/write, NTAG2xx auth/read/write. Contains NFC card authorization database and access control logic. |
| `code/PN532_def.h` | 38 | H | PN532 hardware abstraction layer definitions. Declares `PN532_I2C_Transmit()`, `PN532_I2C_Receive()`, `PN532_GetTick()`, `PN532_Init_Hardware()`. |
| `code/PN532_hal.c` | 163 | C | PN532 HAL implementation for RA8P1 FSP. Blocking I2C2 read/write with timeout, system tick counter (`g_system_tick_ms`), I2C callback (`pn532_i2c_callback`). |
| `code/AS608pro.h` | 92 | H | AS608 fingerprint module header (UART2). Command codes, data structures (`SearchResult`, `SysPara`), sensor touch detect pin macro (`PS_Sta` on P807). |
| `code/AS608pro.c` | 833 | C | AS608 fingerprint driver implementation. UART2 protocol: 57600 bps, EF01 header, checksum frame format. Functions: image capture, feature extraction, fingerprint matching/search, template storage/deletion, database management. Application functions: `finger_addition()` (interactive enrollment), `finger_search()` (verification). |
| `code/ov5640.h` | 104 | H | OV5640 5MP camera header. Resolution enums (QVGA to QSXGA), output format enums (RGB565/YUV422/JPEG), config structures, I2C and CEU API. |
| `code/ov5640_regs.h` | 47 | H | OV5640 register address definitions. |
| `code/ov5640_init_table.h` | 373 | H | OV5640 initialization register table (reg-value pairs). `#define OV5640_REG_DELAY` used to insert delays between writes. |
| `code/ov5640.c` | 540 | C | OV5640 camera driver. I2C0 (400 kHz) register read/write via SCCB protocol (16-bit register addresses). CEU frame capture callback. Public API: init, resolution, format, brightness/contrast/saturation, light mode, auto-focus, test pattern. |
| `code/esp32_comm.h` | 100 | H | ESP32 communication module header (UART3). Ring buffer API, WiFi state definitions, command type enums, software RTC API. |
| `code/esp32_comm.c` | 504 | C | ESP32 serial communication driver. Ring buffer (512 bytes), line-delimited protocol parsing (`@g`/`@h` LED control, `@1`/`@2` WiFi status, `@DATE:YYYY:MM:DD` date sync, `@HH:MM:SS` time sync, `@VOICE:N` voice recognition). Software RTC with leap-year support, calibrated by ESP32 NTP data. |
| `code/voice.h` | 31 | H | Tianwen voice module header (UART1). Sends `@1`-`@6` commands for attendance vocal announcements. |
| `code/voice.c` | 93 | C | Tianwen voice module driver. UART1 TX-only, 115200 bps. `voice_send_attendance()` and `voice_send_string()` formatting. |
| `code/inmp441.h` | 92 | H | INMP441 MEMS I2S microphone header. Buffer size constants (4096 bytes / 512 samples), function declarations for init, get_buffer, get_peak, stop. |
| `code/inmp441.c` | 269 | C | INMP441 microphone driver. SSI1 I2S Master + DTC DMA double-buffering. Raw 32-bit I2S to 16-bit mono PCM conversion with DC offset removal. Peak level detection. Cache maintenance (`SCB_InvalidateDCache_by_Addr`). |

**Total source files:** 30 (13 .c + 17 .h in `src/` and `code/`)  
**Total lines (approx.):** ~8,350  

---

## 2. Generated FSP Files (`ra_gen/`)

These files are auto-generated by the Renesas FSP Configurator and **should not be hand-edited**.

| File | Lines | Description |
|------|-------|-------------|
| `ra_gen/main.c` | 7 | Generated `main()` function, calls `hal_entry()`. |
| `ra_gen/hal_data.h` | 151 | Generated HAL header. Externs for all driver instances (UART, I2C, SSI, GPT, CEU, DTC), callback forward declarations. |
| `ra_gen/hal_data.c` | 964 | Generated HAL configuration data. Driver instance structs, extended configs, baud rate settings, interrupt priority/IPL assignments. |
| `ra_gen/common_data.h` | 19 | Generated common data header. IOPORT instance and control struct externs. |
| `ra_gen/common_data.c` | 9 | Generated common data. IOPORT instance struct initialization. |
| `ra_gen/pin_data.c` | 368 | Generated pin configuration data. `g_bsp_pin_cfg_data[]` array defining all I/O pin assignments. Includes secure zone pin initialization. |
| `ra_gen/vector_data.h` | 96 | Generated interrupt vector number `#define` macros. ISR prototypes. 32 vectors allocated (`VECTOR_DATA_IRQ_COUNT = 32`). |
| `ra_gen/vector_data.c` | 77 | Generated interrupt vector table. `g_vector_table[]` mapping ISR functions to vector indices. Event-link select table for ICU. |
| `ra_gen/bsp_clock_cfg.h` | 75 | Generated clock configuration header. XTAL=24 MHz, PLL multipliers, CPUCLK=1 GHz, PCLKD=250 MHz, SCICLK=120 MHz. |

**Total generated files:** 9  
**Total lines (approx.):** ~1,766  

---

## 3. Per-File Function Listing

### 3.1 `src/hal_entry.c`

| Function | Kind | Description |
|----------|------|-------------|
| `timer_1ms_callback` | Callback (ISR context) | 1 ms GPT0 callback. Calls `g_system_tick()` on `TIMER_EVENT_CYCLE_END`. |
| `user_uart9_callback` | Callback (ISR context) | UART9 callback. Sets `g_uart9_tx_done = true` on `UART_EVENT_TX_COMPLETE`. |
| `_write` | Library override | printf redirection. Blocking write to `g_uart9`. |
| `check_serial_trigger` | Static | Polls UART9 for character 'r' to trigger recording. |
| `record_start` | Static | Audio recording loop. Collects `RECORD_SAMPLES` (64000) from INMP441 via double-buffered DMA. Prints progress every 0.25 s. |
| `record_export` | Static | PCM data binary export over UART9. Sends sync word (0xAA55AA55), sample count, then chunked raw PCM. |
| `hal_entry` | Global (entry) | Main application entry. Opens UART9, starts 1 ms GPT timer, initializes INMP441. Main loop polls for serial trigger, runs record/export on 'r' command. |

### 3.2 `src/hal_warmstart.c`

| Function | Kind | Description |
|----------|------|-------------|
| `R_BSP_WarmStart` | Global (BSP hook) | Startup hook called at multiple points: `BSP_WARM_START_RESET` (flash read enable), `BSP_WARM_START_POST_C` (IOPORT open, SDRAM init). |

### 3.3 `code/headfile.h`

No functions -- header only. Declares shared globals (`motor_flag`) and forward declarations (`uart2_send_value`, `uart8_send_value`, `uart9_send_value`, `dakai_clock_display`, `attendance_check_in`, `color_t` enum, `color_name`, `detect_color`).

### 3.4 `code/lcd.h` / `code/lcd.c`

| Function | Kind | Description |
|----------|------|-------------|
| `SPI2_Init` | Global | Software SPI initialization (set GPIO default levels). |
| `LCD_GPIO_Init` | Global | LCD GPIO initialization, hardware reset, backlight on. |
| `LCD_SPI_Delay` | Static | Short NOP delay for software SPI timing (~5-10 MHz). |
| `SPI2_ReadWriteByte` | Global | Software SPI clock out 1 byte (CPOL=1, MSB first). Always returns 0 (read not used). |
| `LCD_WR_REG` | Global | Write 8-bit command register (RS=0). |
| `LCD_WR_DATA8` | Global | Write 8-bit data (RS=1). |
| `LCD_WR_DATA16` | Global | Write 16-bit data as two 8-bit writes. |
| `LCD_WriteReg` | Global | Write command + 8-bit data. |
| `LCD_WriteRAM_Prepare` | Global | Send "write GRAM" command. |
| `LCD_WriteRAM` | Global | Write 16-bit pixel to GRAM. |
| `LCD_DisplayOn` | Global | Send display-on command (0x29). |
| `LCD_DisplayOff` | Global | Send display-off command (0x28). |
| `LCD_SoftRest` | Global | Send software reset command (0x01). |
| `LCD_HardwareRest` | Global | Hardware reset via RST pin toggling (120 ms recovery). |
| `LCD_SetCursor` | Global | Set column/row address. |
| `LCD_DrawPoint` | Global | Draw single pixel using `POINT_COLOR`. |
| `LCD_Fast_DrawPoint` | Global | Draw single pixel with explicit colour. |
| `LCD_Scan_Dir` | Global | Set memory access control (8 scan directions). |
| `LCD_Display_Dir` | Global | Set display orientation (0-3: portrait/landscape), update `lcddev` struct. |
| `LCD_Set_Window` | Global | Set display window region. |
| `LCD_Init` | Global | Full ILI9341 initialization sequence (power control, gamma, frame rate, etc.). |
| `LCD_Clear` | Global | Fill entire screen with single colour. |
| `LCD_Fill` | Global | Fill rectangular region with single colour. |
| `LCD_Color_Fill` | Global | Fill rectangular region with colour from pointer. |
| `LCD_DrawLine` | Global | Bresenham line drawing. |
| `LCD_DrawRectangle` | Global | Rectangle outline via 4 lines. |
| `LCD_Draw_Circle` | Global | Bresenham circle drawing. |
| `LCD_ShowChar` | Global | Display ASCII character (12/16/24 px). Supports overlay mode. |
| `LCD_Pow` | Static | Integer power function (m^n). |
| `LCD_ShowNum` | Global | Display decimal number, suppressing leading zeros. |
| `LCD_ShowxNum` | Global | Display decimal number with zero-fill option. |
| `LCD_ShowString` | Global | Display null-terminated ASCII string with auto-wrap. |
| `DisplayButtonDown` | Global | Draw raised button frame (3D effect). |
| `DisplayButtonUp` | Global | Draw depressed button frame (3D effect). |
| `LCD_ShowImage` | Global | Display RGB565 image buffer. Optimized with continuous chip-select, YVYU-to-RGB565 ITU-R BT.601 conversion. |

### 3.5 `code/font.h` / `code/font.c`

No functions -- data only. Three ASCII font tables (`asc2_1206`, `asc2_1608`, `asc2_2412`) and two Chinese font tables (`hz16`, `hz24`).

### 3.6 `code/text.h` / `code/text.c`

| Function | Kind | Description |
|----------|------|-------------|
| `Copy_Mem` | Global | Memory copy helper. |
| `Copy_HZK16` | Global | Lookup 16x16 Chinese glyph from `hz16[]` table by GB2312区位 code. |
| `Copy_HZK24` | Global | Lookup 24x24 Chinese glyph from `hz24[]` table. |
| `Get_HzMat` | Global | Extract Chinese glyph data by GB2312 code and font size. Fills with blank if not found. |
| `Show_Font` | Global | Display single Chinese character (12/16/24 px). |
| `Show_Str` | Global | Display mixed ASCII/Chinese string with auto-wrap. Detects multi-byte (>0x80) characters. |
| `Draw_Font16B` | Global | Display 16-px Chinese string (wrapper). |
| `Draw_Font24B` | Global | Display 24-px Chinese string (wrapper). |

### 3.7 `code/tjc_usart_hmi.h` / `code/tjc_usart_hmi.c`

| Function | Kind | Description |
|----------|------|-------------|
| `UART_HIM_Init` | Global | Open UART6, initialize ring buffer. |
| `intToStr` | Global | Integer-to-string conversion. |
| `uart_send_char` | Global | Send single character over UART6 with blocking wait. |
| `uart_send_string` | Global | Send null-terminated string (no trailing 0xFF). |
| `tjc_send_string` | Global | Send string with TJC end-of-frame marker (0xFF 0xFF 0xFF). |
| `tjc_send_txt` | Global | Send text attribute to named TJC widget (e.g. `t0.txt="ABC"`). |
| `tjc_send_val` | Global | Send numeric attribute to named TJC widget (e.g. `n0.val=100`). |
| `tjc_send_nstring` | Global | Send fixed-length string with TJC framing. |
| `initRingBuffer` | Global | Reset ring buffer head/tail/length to zero. |
| `uart6_wait_for_tx` | Global | Block until UART6 TX complete flag is set. |
| `uart6_wait_for_rx` | Global | Block until UART6 RX complete flag is set. |
| `uart6_callback` | Callback (ISR context) | UART6 interrupt handler. TX complete, RX char (fills ring buffer), RX complete, error events. |
| `HIM_connection` | Global | TJC HMI command frame processor. Handles 7 touch commands: Wi-Fi config (SSID + password over 0x70 data frames), lock/unlock, fingerprint enroll, fingerprint delete, camera start. |
| `write1ByteToRingBuffer` | Global | Append byte to ring buffer. |
| `read1ByteFromRingBuffer` | Global | Read byte from ring buffer at offset. |
| `deleteRingBuffer` | Global | Remove bytes from head of ring buffer (has bug: returns after first iteration). |
| `getRingBufferLength` | Global | Return current ring buffer data length. |
| `isRingBufferOverflow` | Global | Return 1 if buffer not full, 0 if full. |

### 3.8 `code/pn532.h` / `code/pn532.c`

| Function | Kind | Description |
|----------|------|-------------|
| `PN532_INIT` | Global | Initialize PN532: open I2C2, verify firmware version, configure SAM. |
| `PN532_connect` | Global | NFC card detection loop. Reads passive target, matches UID against `authorized_cards[]` database, triggers access control actions (TJC screen update, UART notifications, motor flag). |
| `get_PN532_recv_buf` | Global | Return pointer to internal RX buffer. |
| `PN532_send_data` | Global | Pack and send PN532 frame (PREAMBLE + STARTCODE + LEN + LCS + DATA + DCS + POSTAMBLE). |
| `PN532_WaitReady` | Global | Poll PN532 I2C until 0x01 ready byte received or timeout. |
| `PN532_recv` | Global | Receive `how` bytes from PN532 after ready. |
| `PN532_recv_ack` | Global | Receive and validate PN532 ACK frame. |
| `PN532_parse_frame` | Global | Parse PN532 response frame, validate checksums, return data pointer. |
| `PN532_recv_data` | Global | Receive frame, skip I2C ready byte, find and parse valid frame. |
| `PN532_SendGetFirmwareVersion` / `PN532_GetFirmVersion` | Global | Send/full firmware version command (non-blocking/blocking pair). |
| `PN532_SendSamConfig` / `PN532_SetSamConfig` | Global | Send/full SAM configuration command. |
| `PN532_SendReadPassiveTarget` / `PN532_ReadPassTarget` | Global | Send/full passive target scanning. |
| `PN532_SendMifareClassicAuthBlock` / `PN532_MifareClassicAuthBlock` | Global | Send/full MIFARE Classic authentication. |
| `PN532_SendReadDataBlock` / `PN532_ReadDataBlock` | Global | Send/full MIFARE Classic 16-byte block read. |
| `PN532_SendWriteDataBlock` / `PN532_WriteDataBlock` | Global | Send/full MIFARE Classic 16-byte block write. |
| `PN532_SendSetParameters` / `PN532_SetParameters` | Global | Send/full parameter configuration. |
| `PN532_SendNtag2xxAuth` / `PN532_Ntag2xxAuth` | Global | Send/full NTAG2xx PWD_AUTH. |
| `PN532_SendNtag2xxReadBlock` / `PN532_Ntag2xxReadBlock` | Global | Send/full NTAG2xx 4-byte page read. |
| `PN532_SendNtag2xxWriteBlock` / `PN532_Ntag2xxWriteBlock` | Global | Send/full NTAG2xx 4-byte page write. |

### 3.9 `code/PN532_hal.c`

| Function | Kind | Description |
|----------|------|-------------|
| `debug_print` | Static | Debug print via UART7 (non-secure). |
| `pn532_i2c_callback` | Callback (ISR context) | I2C2 callback. TX/RX complete and abort events. |
| `g_system_tick` | Global | 1 ms tick counter increment (called from GPT0 ISR). |
| `PN532_GetTick` | Global | Return current millisecond tick count. |
| `PN532_I2C_Transmit` | Global | Blocking I2C2 write with timeout. |
| `PN532_I2C_Receive` | Global | Blocking I2C2 read with timeout. |
| `PN532_Init_Hardware` | Global | Open I2C2 peripheral for PN532. |

### 3.10 `code/AS608pro.c`

| Function | Kind | Description |
|----------|------|-------------|
| `finger_addition` | Global | Interactive fingerprint enrollment (5-second wait). Calls `AS608_AddFR()`. |
| `finger_search` | Global | Fingerprint verification. Calls `AS608_PressFR()`. |
| `uart2_callback` | Callback (ISR context) | UART2 interrupt handler. RX char buffering with timeout, TX complete flag. |
| `timeout_search_1ms` | Global | 1 ms timeout tick (called from GPT0 ISR). Detects end of AS608 response packet. |
| `AS608_Init` | Global | Open UART2, call `AS608_Check()`. |
| `MYUSART_SendData` | Static | Blocking single-byte UART2 send. |
| `SendHead` / `SendAddr` / `SendFlag` / `SendLength` / `Sendcmd` / `SendCheck` | Global/Static | AS608 frame assembly helpers. |
| `JudgeStr` | Static | Wait for AS608 response packet, return pointer to data or NULL on timeout. |
| `PS_GetImage` | Global | Command: detect and capture fingerprint image. |
| `PS_GenChar` | Global | Command: generate feature from image into CharBuffer. |
| `PS_Match` | Global | Command: compare CharBuffer1 vs CharBuffer2. |
| `PS_Search` | Global | Command: search fingerprint library for match. |
| `PS_RegModel` | Global | Command: merge two features into template. |
| `PS_StoreChar` | Global | Command: store template to flash database. |
| `PS_DeletChar` | Global | Command: delete template(s) from database. |
| `PS_Empty` | Global | Command: erase entire fingerprint database. |
| `PS_WriteReg` | Global | Command: write system register. |
| `PS_ReadSysPara` | Global | Command: read system parameters (capacity, security level, address, etc.). |
| `PS_SetAddr` | Global | Command: set module address. |
| `PS_WriteNotepad` | Global | Command: write 32 bytes to user flash notepad page. |
| `PS_ReadNotepad` | Global | Command: read 32 bytes from user flash notepad page. |
| `PS_HighSpeedSearch` | Global | Command: fast fingerprint search. |
| `PS_ValidTempleteNum` | Global | Command: get count of stored fingerprints. |
| `PS_HandShake` | Global | Command: handshake/auto-detect module address (0xFFFFFFFF → actual address). |
| `AS608_Check` | Global | Module self-test: handshake up to 10 retries, read fingerprint count, read system parameters. |
| `AS608_PressFR` | Global | Stateful fingerprint press state machine. Debounce, capture, feature gen, search, returns: 0=held, 1=match, 2=no match, 3=no finger. |
| `AS608_AddFR` | Global | Fingerprint enrollment state machine. Two presses, feature match check, template generation, storage. |
| `AS608_DeleteFR` | Global | Delete single or all fingerprints. |
| `AS608_GetFRNumber` | Global | Get current fingerprint count from module. |

### 3.11 `code/ov5640.c`

| Function | Kind | Description |
|----------|------|-------------|
| `i2c_master0_callback` | Callback (ISR context) | I2C0 callback. TX/RX complete and abort events. |
| `ceu_callback` | Callback (ISR context) | CEU callback. Sets capture complete flag on `CEU_EVENT_FRAME_END`. |
| `i2c_wait_complete` | Static | Busy-wait for I2C0 transfer completion with microsecond timeout. |
| `ov5640_write_reg` | Global | Write 16-bit register address + 8-bit value via I2C0 (SCCB). |
| `ov5640_read_reg` | Global | Write 16-bit register address, then read 1 byte (restart). |
| `ov5640_write_reg_array` | Static | Write array of reg-value pairs, with `OV5640_REG_DELAY` support. |
| `ov5640_soft_reset` | Static | Software reset OV5640 (reg 0x3008 = 0x80). |
| `ov5640_check_chip_id` | Global | Read chip ID registers (0x300A/B), verify against 0x5640. |
| `ov5640_set_window` | Global | Set output window (offset, width, height). |
| `ov5640_set_output_format` | Global | Set RGB565/YUV422/JPEG output format. |
| `ov5640_set_resolution` | Global | Set resolution (QVGA/VGA/720P/1080P/QSXGA) with window config. |
| `ov5640_auto_focus_init` | Global | Initialize auto-focus engine (FW boot). |
| `ov5640_auto_focus_single` | Global | Trigger single AF operation, wait for completion. |
| `ov5640_set_brightness` | Global | Set brightness (-4 to +4). |
| `ov5640_set_contrast` | Global | Set contrast (0-6). |
| `ov5640_set_saturation` | Global | Set saturation (0-6). |
| `ov5640_set_test_pattern` | Global | Enable/disable colour bar test pattern. |
| `ov5640_set_aec_auto` | Global | Set auto exposure control (AEC) to auto mode. |
| `ov5640_init` | Global | Full OV5640 init: I2C open, slave addr set, soft reset, chip ID check, init register table, default VGA+RGB565. |
| `ov5640_stream_on` | Global | Start image data stream output. |
| `ov5640_deinit` | Global | Software power down, close I2C. |
| `ov5640_capture_start` | Global | Set CEU frame buffer pointer, clear flags. |
| `ov5640_capture_wait` | Global | Poll until CEU frame done or timeout. |
| `ov5640_get_frame_info` | Global | Return pointer to internal `ov5640_frame_t`. |
| `ov5640_set_light_mode` | Global | Set white balance/light mode (Auto/Sunny/Cloudy/Office/Home). |
| `ov5640_is_frame_ready` | Global | Query frame ready flag. |
| `ov5640_clear_frame_flag` | Global | Clear frame ready flag. |

### 3.12 `code/esp32_comm.c`

| Function | Kind | Description |
|----------|------|-------------|
| `esp32_ringbuf_init` / `_write` / `_read` / `_delete` / `_length` | Global | Ring buffer operations (512 bytes). |
| `esp32_wait_tx` / `esp32_send_char` / `esp32_send_raw` | Static | UART3 byte-level TX with blocking wait. |
| `esp32_send_string` | Global | Send string + newline. |
| `esp32_send_attendance` | Global | Send `@1`-`@6` attendance command. |
| `esp32_send_wifi_config` | Global | Send `@WIFI:ssid,password` command. |
| `esp32_parse_time` | Static | Parse `@HH:MM:SS` or `@H:M:S` format. |
| `esp32_parse_line` | Static | Parse single line from ESP32: `@g`/`@h` (LED), `@1`/`@2` (WiFi), `@VOICE:N`, `@DATE:YYYY:MM:DD`, `@HH:MM:SS`. |
| `esp32_extract_lines` | Static | Extract newline-delimited lines from ring buffer, call `esp32_parse_line()`. |
| `esp32_process` | Global | Public RX processing (call in main loop). Overflow protection. |
| `esp32_get_command` / `esp32_get_wifi_state` / `esp32_get_led_state` | Global | State query accessors. |
| `esp32_get_time` / `esp32_get_date` | Global | Time/date sync value accessors. |
| `rtc_get_month_days` | Static | Days-in-month with leap year. |
| `esp32_rtc_tick_1ms` | Global | Software RTC tick (millisecond → second → minute → hour → day → month → year). |
| `rtc_calibrate_date` / `rtc_calibrate_time` | Static | Calibrate RTC from ESP32 NTP sync. |
| `esp32_rtc_get_string` | Global | Format RTC as `YYYY-MM-DD HH:MM:SS`. |
| `esp32_rtc_second_changed` | Global | Query-and-clear second-change flag. |
| `uart3_callback` | Callback (ISR context) | UART3 interrupt handler. TX complete, RX char → ring buffer. |
| `ESP32_Comm_Init` | Global | Open UART3, dynamic baud rate change to 115200, send handshake. |

### 3.13 `code/voice.c`

| Function | Kind | Description |
|----------|------|-------------|
| `voice_wait_tx` / `voice_send_char` / `voice_send_raw` | Static | UART1 byte-level TX with blocking wait. |
| `voice_send_string` | Global | Send string + `\r\n` to voice module. |
| `voice_send_attendance` | Global | Send `@1`-`@6` attendance command. |
| `uart1_callback` | Callback (ISR context) | UART1 callback. TX complete flag only. |
| `Voice_Init` | Global | Open UART1 (115200 bps). |

### 3.14 `code/inmp441.c`

| Function | Kind | Description |
|----------|------|-------------|
| `i2s1_callback` | Callback (ISR context) | SSI1 I2S callback. On RX_FULL: invalidates D-Cache, marks buffer ready, switches to other buffer, starts next DMA. On IDLE: clears running flag. |
| `inmp441_start_read` | Static | Start DMA read from SSI1 into specified raw buffer. |
| `convert_raw_to_samples` | Static | Convert raw 32-bit I2S words to 16-bit mono samples. Extracts left channel, removes DC offset, computes peak. |
| `process_completed_buffer` | Static | Call `convert_raw_to_samples()` on a completed raw buffer. |
| `inmp441_init` | Global | Open GPT1 (audio clock source, direct GTIOR write for PWM mode1), open SSI1 (I2S Master + DTC), start first DMA. |
| `inmp441_get_buffer` | Global | Return pointer to converted 16-bit samples if available. |
| `inmp441_get_peak` | Global | Return last buffer's audio peak level. |
| `inmp441_stop` | Global | Stop SSI1, wait for IDLE, close SSI1 and GPT1. |

---

## 4. Hardware Peripheral Mapping

### 4.1 UART / SCI-B Modules

| Instance | HW Channel | Baud Rate | IPL | External Device | Callback | Function |
|----------|-----------|-----------|-----|-----------------|----------|----------|
| `g_uart1` | SCI1 | ~921,600 | 12 | Tianwen Voice Module | `uart1_callback` | Attendance voice announcements (TX only) |
| `g_uart2` | SCI2 | ~460,800 | 12 | AS608 Fingerprint Sensor | `uart2_callback` | Fingerprint data exchange (EF01 protocol) |
| `g_uart3` | SCI3 | 115,200 (dynamic) | 12 | ESP32 WiFi Module | `uart3_callback` | MQTT/WiFi/NTP communication |
| `g_uart6` | SCI6 | ~921,600 | 12 | TJC USART HMI (serial LCD) | `uart6_callback` | Touch screen GUI, WiFi config input |
| `g_uart9` | SCI9 | ~921,600 | 12 | PC Debug Console | `user_uart9_callback` | printf output, recording trigger, PCM export |

### 4.2 I2C / IIC Modules

| Instance | HW Channel | Bitrate | IPL | Slave Addr | External Device | Callback | Function |
|----------|-----------|---------|-----|------------|-----------------|----------|----------|
| `g_i2c_master0` | IIC0 | ~393 kHz (Fast) | 12 | 0x3C | OV5640 Camera SCCB | `i2c_master0_callback` | Register read/write, AF control |
| `g_i2c2` | IIC2 | ~97 kHz (Standard) | 12 | 0x24 | PN532 NFC Module | `pn532_i2c_callback` | NFC card communication |

### 4.3 SSI / I2S Module

| Instance | HW Channel | Mode | Word Length | WS | External Device | Callback | Function |
|----------|-----------|------|-------------|----|-----------------|----------|----------|
| `g_i2s1` | SSI1 | I2S Master | 32-bit PCM | 32-bit | INMP441 MEMS Mic | `i2s1_callback` | 16 kHz audio capture, DTC DMA RX |

### 4.4 GPT Timers

| Instance | HW Channel | Mode | Period | IPL | Callback | Function |
|----------|-----------|------|--------|-----|----------|----------|
| `g_timer_1ms` | GPT0 | Periodic | 0x3D090 (~1 ms @ PCLKD=250 MHz) | 12 | `timer_1ms_callback` | System tick (1 ms), drives `g_system_tick()`, software RTC, AS608 RX timeout |
| `g_timer1` | GPT1 | Periodic (PWM output) | 0xEA (~936 ns) | Disabled | NULL | Audio clock source for SSI1 (direct GTIOR write for PWM mode1 output) |
| `g_timer4` | GPT4 | PWM | 0xA (~40 ns) | Disabled | NULL | PWM output on P102, GTIOCA enabled (motor/servo/buzzer?) |

### 4.5 CEU (Capture Engine Unit)

| Instance | Resolution | Bytes/Pixel | Format | IPL | Callback | Function |
|----------|-----------|-------------|--------|-----|----------|----------|
| `g_ceu0` | 320 x 240 | 2 | YCbCr422 (8-bit bus) | 12 | `ceu_callback` | Frame capture from OV5640 camera |

### 4.6 DTC (Data Transfer Controller)

| Instance | Source Trigger | Dest Mode | Src Mode | Transfer Mode | Function |
|----------|---------------|-----------|----------|---------------|----------|
| `g_transfer_rx` | SSI1 RXI (VECTOR_NUMBER_SSI1_RXI) | Incremented | Fixed | Block | DMA RX: SSI1 FIFO → raw audio buffer (4096 bytes per block) |

---

## 5. Pin Mapping

### 5.1 Individual GPIO / Peripheral Pins

| MCU Pin | Port Config | Peripheral Function | External Connection | Notes |
|---------|------------|---------------------|---------------------|-------|
| P102 | GPT1 | GPT1 GTIOCA | GPT1 PWM output | Audio clock (SSI1 external clock source) |
| P104 | GPT1 | GPT1 GTIOCB | GPT1 PWM output | Unused / spare |
| P105 | GPT1 | GPT1 GTIOCA/B | GPT1 output | Unused / spare |
| P110 | GPIO Output Low | GPIO | -- | General-purpose digital output |
| P111 | SDHI_MMC | SDHI CLK | SD card | SD card interface |
| P112 | BUS | External Bus D0 | SDRAM | SDRAM data bus |
| P113 | BUS | External Bus D1 | SDRAM | SDRAM data bus |
| P114 | BUS | External Bus D2 | SDRAM | SDRAM data bus |
| P115 | BUS | External Bus D3 | SDRAM | SDRAM data bus |
| P206 | SSI | SSI1 SCK | INMP441 SCK | I2S bit clock |
| P208 | SCI1_3_5_7_9 | SCI1 TXD | Voice Module (Tianwen) RX | UART1 TX (voice commands) |
| P209 | SCI1_3_5_7_9 | SCI1 RXD | Voice Module TX | UART1 RX (unused in current code) |
| P210 | DEBUG | SWCLK | J-Link / Debugger | SWD clock |
| P211 | DEBUG | SWDIO | J-Link / Debugger | SWD data |
| P300 | BUS | External Bus A0 | SDRAM | SDRAM address |
| P301 | BUS | External Bus A1 | SDRAM | SDRAM address |
| P302 | BUS | External Bus A2 | SDRAM | SDRAM address |
| P310 | SCI1_3_5_7_9 | SCI3 TXD | ESP32 Module RX | UART3 TX (ESP32 communication) |
| P400 | CEU | CEU D0 | OV5640 D0 | Camera data bit 0 |
| P401 | CEU | CEU D1 | OV5640 D1 | Camera data bit 1 |
| P402 | SSI | SSI1 WS | INMP441 WS | I2S word select |
| P405 | CEU | CEU D2 | OV5640 D2 | Camera data bit 2 |
| P406 | CEU | CEU D3 | OV5640 D3 | Camera data bit 3 |
| P409 | IIC (Drive Mid) | IIC0 SCL | OV5640 SIOC | Camera SCCB clock |
| P410 | IIC (Drive Mid) | IIC0 SDA | OV5640 SIOD | Camera SCCB data |
| P414 | CEU | CEU D4 | OV5640 D4 | Camera data bit 4 |
| P415 | CEU | CEU D5 | OV5640 D5 | Camera data bit 5 |
| P503-P510 | BUS | External Bus D4-D11 | SDRAM | SDRAM data bus |
| P514 | IIC (Drive Mid) | IIC2 SCL | PN532 SCL | NFC module I2C clock |
| P515 | IIC (Drive Mid) | IIC2 SDA | PN532 SDA | NFC module I2C data |
| P601 | GPIO Output Low | LCD SDA | ILI9341 SDA (MOSI) | LCD SPI data (software bit-bang) |
| P602 | GPIO Output Low | LCD SCL | ILI9341 SCK | LCD SPI clock (software bit-bang) |
| P603 | GPIO Output High | LCD CS | ILI9341 CS | LCD chip select |
| P604 | GPIO Output High | LCD RST | ILI9341 RST | LCD hardware reset |
| P605 | GPIO Output Low | LCD RS (D/C) | ILI9341 D/CX | LCD data/command select |
| P607-P615 | BUS | External Bus D12-D15 | SDRAM | SDRAM data bus |
| P700 | CEU | CEU D6 | OV5640 D6 | Camera data bit 6 |
| P701 | CEU | CEU D7 | OV5640 D7 | Camera data bit 7 |
| P702 | CEU | CEU synchronization | OV5640 HSYNC/VSYNC | Camera sync signals |
| P703 | CEU | CEU PCLK | OV5640 PCLK | Camera pixel clock |
| P706 | SCI1_3_5_7_9 | SCI9 TXD | PC (USB-UART) RX | Debug console TX |
| P707 | SCI1_3_5_7_9 | SCI9 RXD | PC (USB-UART) TX | Debug console RX |
| P708 | CEU | CEU synchronization | OV5640 sync | Camera sync |
| P713 | GPT1 | GPT1 PWM output | -- | GPT1 PWM output (buzzer/servo) |
| P715 | GPIO Output Low | LCD BLK | ILI9341 LED_K / Backlight | LCD backlight control (active high) |
| P801 | SCI0_2_4_6_8 | SCI2 TXD | AS608 RX | Fingerprint sensor UART TX |
| P802 | SCI0_2_4_6_8 | SCI2 RXD | AS608 TX | Fingerprint sensor UART RX |
| P807 | GPIO Input | GPIO Input | AS608 TOUCH_OUT | Fingerprint touch detect (`PS_Sta`) |
| P813 | BUS | External Bus | SDRAM | SDRAM control/address |
| P905 | SCI1_3_5_7_9 | SCI6 TXD (?) | TJC HMI RX | Serial screen UART TX |
| P906 | SSI | SSI1 WS (alternate) | INMP441 WS | I2S word select |
| P907 | SSI | SSI1 RXD (alternate) | INMP441 SD | I2S serial data |
| P908 | SCI0_2_4_6_8 | SCI6 RXD (?) | TJC HMI TX | Serial screen UART RX |
| P909 | SCI0_2_4_6_8 | SCI6 RTS/CTS (?) | TJC HMI | Serial screen flow control |
| P1000-P1015 | BUS | External Bus D16-D31 | SDRAM | SDRAM data bus |
| P1200-P1215 | BUS | External Bus A3-A18 | SDRAM | SDRAM address bus |
| P1300 | BUS | External Bus CS | SDRAM | SDRAM chip select |
| P1301-P1305 | SDHI_MMC | SDHI CMD/DAT0-3 | SD Card | SD card interface |
| P1307 | SDHI_MMC | SDHI CD | SD Card | Card detect |

### 5.2 Device-to-Pin Summary

| External Device | MCU Pins | Interface |
|-----------------|----------|-----------|
| **ILI9341 LCD (2.4"/2.8")** | P601 (SDA), P602 (SCL), P603 (CS), P604 (RST), P605 (RS), P715 (BLK) | Software SPI (GPIO) |
| **OV5640 Camera** | P400-P401, P405-P406, P414-P415, P700-P703, P708 (CEU data/sync), P409-P410 (IIC0) | 8-bit CEU + I2C0 |
| **INMP441 Microphone** | P206 (SCK), P402/P906 (WS), P907 (SD) | I2S (SSI1) |
| **AS608 Fingerprint** | P801 (TXD), P802 (RXD), P807 (Touch) | UART2 (SCI2) |
| **PN532 NFC** | P514 (SCL), P515 (SDA) | I2C2 |
| **TJC HMI Serial Screen** | P905 (TXD), P908 (RXD), P909 | UART6 (SCI6) |
| **ESP32 WiFi Module** | P310 (TXD) | UART3 (SCI3) |
| **Tianwen Voice Module** | P208 (TXD), P209 (RXD) | UART1 (SCI1) |
| **PC Debug Console** | P706 (TXD), P707 (RXD) | UART9 (SCI9) |
| **SD Card** | P111, P1301-P1305, P1307 | SDHI/MMC |
| **SDRAM** | P112-P115, P300-P302, P503-P510, P607-P615, P813, P1000-P1015, P1200-P1215, P1300 | External Bus |
| **SWD Debug** | P210-P211 | SWCLK/SWDIO |

---

## 6. Interrupt / ISR Mapping

The vector table contains **32 entries** (index 0 through 31) in the `g_vector_table[]` array.

| Vector Index | ISR Function | Event Source | IRQn Name | Group | Hardware Instance | Purpose |
|-------------|-------------|-------------|-----------|-------|-------------------|---------|
| 0 | `sci_b_uart_rxi_isr` | SCI9 RXI | SCI9_RXI_IRQn | GROUP0 | g_uart9 | Debug UART receive |
| 1 | `sci_b_uart_txi_isr` | SCI9 TXI | SCI9_TXI_IRQn | GROUP1 | g_uart9 | Debug UART transmit |
| 2 | `sci_b_uart_tei_isr` | SCI9 TEI | SCI9_TEI_IRQn | GROUP2 | g_uart9 | Debug UART transmit end |
| 3 | `sci_b_uart_eri_isr` | SCI9 ERI | SCI9_ERI_IRQn | GROUP3 | g_uart9 | Debug UART receive error |
| 4 | `ceu_isr` | CEU CEUI | CEU_CEUI_IRQn | GROUP4 | g_ceu0 | Camera frame end |
| 5 | `iic_master_rxi_isr` | IIC0 RXI | IIC0_RXI_IRQn | GROUP5 | g_i2c_master0 | OV5640 I2C receive |
| 6 | `iic_master_txi_isr` | IIC0 TXI | IIC0_TXI_IRQn | GROUP6 | g_i2c_master0 | OV5640 I2C transmit |
| 7 | `iic_master_tei_isr` | IIC0 TEI | IIC0_TEI_IRQn | GROUP7 | g_i2c_master0 | OV5640 I2C transmit end |
| 8 | `iic_master_eri_isr` | IIC0 ERI | IIC0_ERI_IRQn | GROUP0 | g_i2c_master0 | OV5640 I2C error |
| 9 | `sci_b_uart_rxi_isr` | SCI6 RXI | SCI6_RXI_IRQn | GROUP1 | g_uart6 | TJC HMI UART receive |
| 10 | `sci_b_uart_txi_isr` | SCI6 TXI | SCI6_TXI_IRQn | GROUP2 | g_uart6 | TJC HMI UART transmit |
| 11 | `sci_b_uart_tei_isr` | SCI6 TEI | SCI6_TEI_IRQn | GROUP3 | g_uart6 | TJC HMI UART transmit end |
| 12 | `sci_b_uart_eri_isr` | SCI6 ERI | SCI6_ERI_IRQn | GROUP4 | g_uart6 | TJC HMI UART error |
| 13 | `iic_master_rxi_isr` | IIC2 RXI | IIC2_RXI_IRQn | GROUP5 | g_i2c2 | PN532 I2C receive |
| 14 | `iic_master_txi_isr` | IIC2 TXI | IIC2_TXI_IRQn | GROUP6 | g_i2c2 | PN532 I2C transmit |
| 15 | `iic_master_tei_isr` | IIC2 TEI | IIC2_TEI_IRQn | GROUP7 | g_i2c2 | PN532 I2C transmit end |
| 16 | `iic_master_eri_isr` | IIC2 ERI | IIC2_ERI_IRQn | GROUP0 | g_i2c2 | PN532 I2C error |
| 17 | `sci_b_uart_rxi_isr` | SCI1 RXI | SCI1_RXI_IRQn | GROUP1 | g_uart1 | Voice module UART receive |
| 18 | `sci_b_uart_txi_isr` | SCI1 TXI | SCI1_TXI_IRQn | GROUP2 | g_uart1 | Voice module UART transmit |
| 19 | `sci_b_uart_tei_isr` | SCI1 TEI | SCI1_TEI_IRQn | GROUP3 | g_uart1 | Voice module UART transmit end |
| 20 | `sci_b_uart_eri_isr` | SCI1 ERI | SCI1_ERI_IRQn | GROUP4 | g_uart1 | Voice module UART error |
| 21 | `sci_b_uart_rxi_isr` | SCI2 RXI | SCI2_RXI_IRQn | GROUP5 | g_uart2 | AS608 fingerprint UART receive |
| 22 | `sci_b_uart_txi_isr` | SCI2 TXI | SCI2_TXI_IRQn | GROUP6 | g_uart2 | AS608 fingerprint UART transmit |
| 23 | `sci_b_uart_tei_isr` | SCI2 TEI | SCI2_TEI_IRQn | GROUP7 | g_uart2 | AS608 fingerprint UART transmit end |
| 24 | `sci_b_uart_eri_isr` | SCI2 ERI | SCI2_ERI_IRQn | GROUP0 | g_uart2 | AS608 fingerprint UART error |
| 25 | `sci_b_uart_rxi_isr` | SCI3 RXI | SCI3_RXI_IRQn | GROUP1 | g_uart3 | ESP32 UART receive |
| 26 | `sci_b_uart_txi_isr` | SCI3 TXI | SCI3_TXI_IRQn | GROUP2 | g_uart3 | ESP32 UART transmit |
| 27 | `sci_b_uart_tei_isr` | SCI3 TEI | SCI3_TEI_IRQn | GROUP3 | g_uart3 | ESP32 UART transmit end |
| 28 | `sci_b_uart_eri_isr` | SCI3 ERI | SCI3_ERI_IRQn | GROUP4 | g_uart3 | ESP32 UART error |
| 29 | `gpt_counter_overflow_isr` | GPT0 Overflow | GPT0_COUNTER_OVERFLOW_IRQn | GROUP5 | g_timer_1ms | 1 ms system tick |
| 30 | `ssi_rxi_isr` | SSI1 RXI | SSI1_RXI_IRQn | GROUP6 | g_i2s1 | INMP441 DMA RX trigger |
| 31 | `ssi_int_isr` | SSI1 INT | SSI1_INT_IRQn | GROUP7 | g_i2s1 | INMP441 I2S error |

### 6.1 Callback Chain (ISR → Application Callback)

| ISR (FSP) | User Callback | Called From | What It Does |
|-----------|--------------|-------------|--------------|
| `sci_b_uart_rxi_isr` [17-20] | `uart1_callback` | voice.c | TX complete flag |
| `sci_b_uart_rxi_isr` [21-24] | `uart2_callback` | AS608pro.c | RX char buffering + timeout; TX complete |
| `sci_b_uart_rxi_isr` [25-28] | `uart3_callback` | esp32_comm.c | RX char → ring buffer; TX complete |
| `sci_b_uart_rxi_isr` [9-12] | `uart6_callback` | tjc_usart_hmi.c | RX char → ring buffer; TX/RX complete |
| `sci_b_uart_rxi_isr` [0-3] | `user_uart9_callback` | hal_entry.c | TX complete |
| `iic_master_rxi_isr` [5-8] | `i2c_master0_callback` | ov5640.c | I2C TX/RX complete |
| `iic_master_rxi_isr` [13-16] | `pn532_i2c_callback` | PN532_hal.c | I2C TX/RX complete |
| `ceu_isr` [4] | `ceu_callback` | ov5640.c | Frame end → sets `g_capture_complete` |
| `ssi_rxi_isr` [30] | `i2s1_callback` | inmp441.c | DMA buffer full → invalidate cache, swap buffers |
| `ssi_int_isr` [31] | (via `i2s1_callback`) | inmp441.c | Error/IDLE events |
| `gpt_counter_overflow_isr` [29] | `timer_1ms_callback` | hal_entry.c | 1 ms tick → `g_system_tick()`, `esp32_rtc_tick_1ms()`, `timeout_search_1ms()` |

---

## 7. Clock Tree

```
XTAL: 24 MHz
  |
  +-- PLL1 (x250, /3) → 2,000 MHz
  |     +-- PLL1P (/2)  → 1,000 MHz  → CPUCLK (/1)  = 1,000 MHz (Cortex-M85)
  |     +-- PLL1Q (/6)  →   333 MHz
  |     +-- PLL1R (/9)  →   222 MHz
  |
  +-- PLL2 (x300, /3) → 2,400 MHz
  |     +-- PLL2P (/4)  →   600 MHz
  |     +-- PLL2Q (/3)  →   800 MHz
  |     +-- PLL2R (/5)  →   480 MHz  → SCICLK (/4) = 120 MHz (UART SCI clock)
  |
  +-- HOCO: 48 MHz (not used as system clock)
  |
  +-- PCLKD: ICLK (/4) = 250 MHz  → GPT_COUNT_CLOCK source (GPT timers)
  +-- PCLKB: ICLK/16
  +-- PCLKA: ICLK/8
```

Key frequencies:
- **CPUCLK:** 1.00 GHz (PLL1P /1)
- **ICLK:** 250 MHz (System bus /4)
- **PCLKD:** 250 MHz (GPT timer peripheral clock)
- **SCICLK:** 120 MHz (PLL2R /4 = 480/4, source for all SCI-B UARTs)
- **BCLK:** 125 MHz (External bus /8, SDRAM)
- **SDCLK:** enabled (SD card)
- **GPTCLK:** disabled (GPT uses PCLKD instead)
- **SPICLK, CANFDCLK, I3CCLK, ADCCLK, LCDCLK, USBCLK:** all disabled

---

## 8. System Architecture Summary

```
+---------------------+
|    RA8P1 MCU       |
|  Cortex-M85 @1 GHz |
+---------------------+
          |
    +-----+-----+-----+------+------+------+------+------+
    |     |     |     |      |      |      |      |      |
  UART1  UART2 UART3 UART6 UART9  I2C0  I2C2  SSI1   CEU
  (SCI1) (SCI2)(SCI3)(SCI6)(SCI9) (IIC0)(IIC2)(I2S) (Capture)
    |     |     |     |      |      |      |      |      |
  Voice  AS608 ESP32 TJC   Debug  OV5640 PN532 INMP441 OV5640
  Module FPR   WiFi  HMI   ConsoleCamera NFC   Mic    Camera
                        Screen
```

**Application purpose:** Smart lock / attendance system with multi-factor authentication:
1. **NFC Card** (PN532 via I2C2) -- reads MIFARE/NTAG card UIDs for access control
2. **Fingerprint** (AS608 via UART2) -- biometric verification
3. **Camera** (OV5640 via I2C0 + CEU) -- 320x240 image capture with YUV422 output
4. **Microphone** (INMP441 via SSI1 I2S + DTC DMA) -- 16 kHz audio recording, PCM export
5. **Serial Touch Screen** (TJC HMI via UART6) -- user interface, WiFi configuration
6. **WiFi / Cloud** (ESP32 via UART3) -- MQTT attendance reporting, NTP time sync, voice recognition (`@VOICE:N`)
7. **Voice Announcements** (Tianwen module via UART1) -- vocal attendance confirmation
8. **LCD Display** (ILI9341 via software SPI) -- auxiliary 2.4"/2.8" display for clock, camera preview
9. **SD Card** (via SDHI/MMC) -- storage
10. **SDRAM** (via External Bus) -- frame buffer, recording buffer

**RTOS:** Bare-metal (no RTOS). Main loop in `hal_entry()` handles recording trigger polling. ESP32 processing called in main loop via `esp32_process()`. PN532 NFC polling via `PN532_connect()`. All timing driven by GPT0 1 ms system tick.
