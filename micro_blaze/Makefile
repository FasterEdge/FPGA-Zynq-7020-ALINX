# Makefile — FasterEdge FPGA / MicroBlaze 版（Zynq-7020 ALINX）
# 工具链：microblazeel-xilinx-elf-gcc（Vitis 安装目录下，或 ISE 版 SDK）
#   export PATH=/tools/Xilinx/Vitis/2023.2/gnu/microblaze/lin/bin:$PATH
#
# 用法：
#   make                # 生成 build/fasteredge-microblaze.elf
#   make SIZE=1 size    # 查看体积
#
# 联动 vivado/ 工程：
#  vivado/scripts/create_project.tcl 生成的工程会例化 MicroBlaze +
#   AXI UART Lite(0x40600000) + AXI Timer(0x41C00000) + PL KV 存储
#   （可选 0x40000000，加 -DFE_PL_CONFIG_BASE 启用）。
#   若地址不同，用 make CFLAGS+=-DFE_UARTLITE_BASE=0x... 覆盖。

CROSS   ?= microblazeel-xilinx-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

CFLAGS  := -Os -g3 -Wall -Wextra -mxl-soft-mul -mxl-barrel-shift \
           -ffunction-sections -fdata-sections
LDFLAGS := -Wl,--gc-sections -T lscript.ld

SRC     := Core/fe.c Core/fe_hmac_sha256.c \
           Ability/ability_base.c Ability/ability_role.c Ability/ability_time.c \
           Ability/ability_onekey.c Ability/ability_configfile.c \
           Ability/ability_serial.c Ability/ability_modbus.c Ability/ability_mqtt.c \
           Ability/ability_edgerole.c \
           Data/data_base.c Data/data_config.c Data/data_keyring.c Data/data_netmap.c \
           User/register.c User/main.c User/fe_port.c
OBJ     := $(patsubst %.c,build/%.o,$(SRC))
INC     := -ICore -IInc -IUser

all: build/fasteredge-microblaze.elf

build:
	mkdir -p build

build/%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

build/fasteredge-microblaze.elf: $(OBJ) lscript.ld
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@
	$(SIZE) $@

size: build/fasteredge-microblaze.elf
	$(SIZE) -A $<

clean:
	rm -rf build

.PHONY: all size clean
