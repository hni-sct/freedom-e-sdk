TARGET = led-bmi160-demonstator
C_SRCS += demo.c spi.c common.c UART_driver.c bmi160.c led-matrix.c fault.c watchdog.c
CFLAGS += -O2 -fno-builtin-printf -DUSE_PLIC

C_SRCS += $(BSP_BASE)/drivers/plic/plic_driver.c
BSP_BASE = ../../bsp
include $(BSP_BASE)/env/common.mk
