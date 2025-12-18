################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Stepper_UltrasonicSensor.c \
../Core/Src/adc.c \
../Core/Src/bme280_sensor_driver.c \
../Core/Src/comms_service.c \
../Core/Src/freertos.c \
../Core/Src/gaz_sensor_driver.c \
../Core/Src/gpio.c \
../Core/Src/i2c.c \
../Core/Src/main.c \
../Core/Src/movement_service.c \
../Core/Src/rtc.c \
../Core/Src/rtc_service.c \
../Core/Src/sensors_service.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_manager.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/tim.c \
../Core/Src/usart.c \
../Core/Src/wifi_basic_driver.c \
../Core/Src/wifi_http_support.c \
../Core/Src/wifi_sensors.c 

OBJS += \
./Core/Src/Stepper_UltrasonicSensor.o \
./Core/Src/adc.o \
./Core/Src/bme280_sensor_driver.o \
./Core/Src/comms_service.o \
./Core/Src/freertos.o \
./Core/Src/gaz_sensor_driver.o \
./Core/Src/gpio.o \
./Core/Src/i2c.o \
./Core/Src/main.o \
./Core/Src/movement_service.o \
./Core/Src/rtc.o \
./Core/Src/rtc_service.o \
./Core/Src/sensors_service.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_manager.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/tim.o \
./Core/Src/usart.o \
./Core/Src/wifi_basic_driver.o \
./Core/Src/wifi_http_support.o \
./Core/Src/wifi_sensors.o 

C_DEPS += \
./Core/Src/Stepper_UltrasonicSensor.d \
./Core/Src/adc.d \
./Core/Src/bme280_sensor_driver.d \
./Core/Src/comms_service.d \
./Core/Src/freertos.d \
./Core/Src/gaz_sensor_driver.d \
./Core/Src/gpio.d \
./Core/Src/i2c.d \
./Core/Src/main.d \
./Core/Src/movement_service.d \
./Core/Src/rtc.d \
./Core/Src/rtc_service.d \
./Core/Src/sensors_service.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_manager.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/tim.d \
./Core/Src/usart.d \
./Core/Src/wifi_basic_driver.d \
./Core/Src/wifi_http_support.d \
./Core/Src/wifi_sensors.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F439xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Stepper_UltrasonicSensor.cyclo ./Core/Src/Stepper_UltrasonicSensor.d ./Core/Src/Stepper_UltrasonicSensor.o ./Core/Src/Stepper_UltrasonicSensor.su ./Core/Src/adc.cyclo ./Core/Src/adc.d ./Core/Src/adc.o ./Core/Src/adc.su ./Core/Src/bme280_sensor_driver.cyclo ./Core/Src/bme280_sensor_driver.d ./Core/Src/bme280_sensor_driver.o ./Core/Src/bme280_sensor_driver.su ./Core/Src/comms_service.cyclo ./Core/Src/comms_service.d ./Core/Src/comms_service.o ./Core/Src/comms_service.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/gaz_sensor_driver.cyclo ./Core/Src/gaz_sensor_driver.d ./Core/Src/gaz_sensor_driver.o ./Core/Src/gaz_sensor_driver.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/movement_service.cyclo ./Core/Src/movement_service.d ./Core/Src/movement_service.o ./Core/Src/movement_service.su ./Core/Src/rtc.cyclo ./Core/Src/rtc.d ./Core/Src/rtc.o ./Core/Src/rtc.su ./Core/Src/rtc_service.cyclo ./Core/Src/rtc_service.d ./Core/Src/rtc_service.o ./Core/Src/rtc_service.su ./Core/Src/sensors_service.cyclo ./Core/Src/sensors_service.d ./Core/Src/sensors_service.o ./Core/Src/sensors_service.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_manager.cyclo ./Core/Src/system_manager.d ./Core/Src/system_manager.o ./Core/Src/system_manager.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/wifi_basic_driver.cyclo ./Core/Src/wifi_basic_driver.d ./Core/Src/wifi_basic_driver.o ./Core/Src/wifi_basic_driver.su ./Core/Src/wifi_http_support.cyclo ./Core/Src/wifi_http_support.d ./Core/Src/wifi_http_support.o ./Core/Src/wifi_http_support.su ./Core/Src/wifi_sensors.cyclo ./Core/Src/wifi_sensors.d ./Core/Src/wifi_sensors.o ./Core/Src/wifi_sensors.su

.PHONY: clean-Core-2f-Src

