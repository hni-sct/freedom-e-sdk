TARGET = led-bmi160-demonstrator
CLANG_C_SRCS += demo.c spi.c common.c bmi160.c led-matrix.c
C_SRCS += UART_driver.c fault.c
CFLAGS += -O2 -fno-builtin-printf -DUSE_PLIC

C_SRCS += $(BSP_BASE)/drivers/plic/plic_driver.c
BSP_BASE = ../../bsp
include $(BSP_BASE)/env/common.mk
