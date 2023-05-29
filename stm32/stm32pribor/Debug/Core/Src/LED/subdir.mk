################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/LED/led_helper.c \
../Core/Src/LED/ssd1306.c \
../Core/Src/LED/ssd1306_fonts.c \
../Core/Src/LED/ssd1306_tests.c 

OBJS += \
./Core/Src/LED/led_helper.o \
./Core/Src/LED/ssd1306.o \
./Core/Src/LED/ssd1306_fonts.o \
./Core/Src/LED/ssd1306_tests.o 

C_DEPS += \
./Core/Src/LED/led_helper.d \
./Core/Src/LED/ssd1306.d \
./Core/Src/LED/ssd1306_fonts.d \
./Core/Src/LED/ssd1306_tests.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/LED/%.o Core/Src/LED/%.su: ../Core/Src/LED/%.c Core/Src/LED/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-LED

clean-Core-2f-Src-2f-LED:
	-$(RM) ./Core/Src/LED/led_helper.d ./Core/Src/LED/led_helper.o ./Core/Src/LED/led_helper.su ./Core/Src/LED/ssd1306.d ./Core/Src/LED/ssd1306.o ./Core/Src/LED/ssd1306.su ./Core/Src/LED/ssd1306_fonts.d ./Core/Src/LED/ssd1306_fonts.o ./Core/Src/LED/ssd1306_fonts.su ./Core/Src/LED/ssd1306_tests.d ./Core/Src/LED/ssd1306_tests.o ./Core/Src/LED/ssd1306_tests.su

.PHONY: clean-Core-2f-Src-2f-LED

