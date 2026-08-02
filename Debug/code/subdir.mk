################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../code/AS608pro.c \
../code/PN532_hal.c \
../code/esp32_comm.c \
../code/font.c \
../code/inmp441.c \
../code/key.c \
../code/lcd.c \
../code/mfcc_engine.c \
../code/ov5640.c \
../code/pn532.c \
../code/text.c \
../code/tjc_usart_hmi.c \
../code/voice.c \
../code/voice_cmd.c 

C_DEPS += \
./code/AS608pro.d \
./code/PN532_hal.d \
./code/esp32_comm.d \
./code/font.d \
./code/inmp441.d \
./code/key.d \
./code/lcd.d \
./code/mfcc_engine.d \
./code/ov5640.d \
./code/pn532.d \
./code/text.d \
./code/tjc_usart_hmi.d \
./code/voice.d \
./code/voice_cmd.d 

OBJS += \
./code/AS608pro.o \
./code/PN532_hal.o \
./code/esp32_comm.o \
./code/font.o \
./code/inmp441.o \
./code/key.o \
./code/lcd.o \
./code/mfcc_engine.o \
./code/ov5640.o \
./code/pn532.o \
./code/text.o \
./code/tjc_usart_hmi.o \
./code/voice.o \
./code/voice_cmd.o 

SREC += \
RA8P1_TEST1.srec 

MAP += \
RA8P1_TEST1.map 


# Each subdirectory must supply rules for building sources it contributes
code/%.o: ../code/%.c
	$(file > $@.in,-mthumb -mfloat-abi=hard -mcpu=cortex-m85+nopacbti -O2 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -g -D_RENESAS_RA_ -D_RA_CORE=CPU0 -D_RA_ORDINAL=1 -I"D:/e2_workplace/RA8P1_TEST1/ra_gen" -I"D:/e2_workplace/RA8P1_TEST1/code" -I"." -I"D:/e2_workplace/RA8P1_TEST1/ra_cfg/fsp_cfg/bsp" -I"D:/e2_workplace/RA8P1_TEST1/ra_cfg/fsp_cfg" -I"D:/e2_workplace/RA8P1_TEST1/src" -I"D:/e2_workplace/RA8P1_TEST1/ra/fsp/inc" -I"D:/e2_workplace/RA8P1_TEST1/ra/fsp/inc/api" -I"D:/e2_workplace/RA8P1_TEST1/ra/fsp/inc/instances" -I"D:/e2_workplace/RA8P1_TEST1/ra/arm/CMSIS_6/CMSIS/Core/Include" -std=c99 -Wno-stringop-overflow -Wno-format-truncation -flax-vector-conversions --param=min-pagesize=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" -x c "$<")
	@echo Building file: $< && arm-none-eabi-gcc @"$@.in"

