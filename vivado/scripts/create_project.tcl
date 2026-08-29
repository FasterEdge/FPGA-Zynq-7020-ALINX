# create_project.tcl — 生成 FPGA-Zynq-7020-ALINX Vivado 工程（ALINX AX7020）
# 用法：
#   cd vivado/scripts
#   vivado -mode batch -source create_project.tcl
#   # 或在 Vivado Tcl Console 中: cd <本目录>; source create_project.tcl
#
# 工程内容：
#   - RTL: rtl/fe_axi_config_kv.sv（PL 侧 KV 配置存储，AXI4-Lite 从属）
#   - Block Design: Zynq7 PS + AXI UART Lite(控制台) + AXI Timer(系统时间)
#                   + fe_axi_config_kv（模块引用），与 micro_blaze/Makefile
#                   中的默认基地址保持一致：
#                     AXI UART Lite  0x40600000
#                     AXI Timer      0x41C00000
#                     PL KV 存储     0x40000000（micro_blaze 加 -DFE_PL_CONFIG_BASE 启用）

set script_dir [file dirname [file normalize [info script]]]
set repo_dir   [file normalize [file join $script_dir ../..]]
set proj_dir   [file join $repo_dir vivado project]

create_project fe_zynq7020 $proj_dir -part xc7z020clg484-1 -force

set_property target_language Verilog [current_project]
set_property simulator_language Verilog [current_project]

# ------------------------------------------------------------
# 添加 RTL
# ------------------------------------------------------------
add_files -fileset sources_1 [file join $repo_dir vivado rtl]
set_property top fe_axi_config_kv [current_fileset]
# Block Design 里的顶层单独设（见下）

add_files -fileset sim_1 [file join $repo_dir vivado sim]
set_property top tb_fe_axi_config_kv [get_filesets sim_1]

add_files -fileset constrs_1 [file join $repo_dir vivado xdc]

# ------------------------------------------------------------
# Block Design：PS7 + 外设 + PL KV
# ------------------------------------------------------------
create_bd_design "fe_bd"

# Zynq7 Processing System
set ps7 [create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7]
# ALINX AX7020 默认 PSU 配置：DDR3 + UART0(MIO) + ETH0 + QSPI。
# 这里仅启用最小集（DDR + UART0），其余按 ALINX 工程模板按需追加。
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
    make_external {FIXED_IO, DDR} } [get_bd_cells ps7]

# AXI UART Lite —— MicroBlaze / PS 控制台串口（115200）
set uart [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uartlite:2.0 axi_uartlite_0]
set_property -dict [list C_BAUDRATE {115200} C_DATA_BITS {8}] $uart

# AXI Timer —— 64 位级联自由计数，作系统时间基准
set timer [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0]
set_property -dict [list C_COUNT_WIDTH {32} C_CASCADE_MODE {1}] $timer

# PL KV 存储（模块引用，来自 rtl/fe_axi_config_kv.sv）
set kv [create_bd_cell -type module -reference fe_axi_config_kv fe_axi_config_kv_0]

# 连接：UART Lite + Timer + KV 都挂到 PS 的 M_AXI_GP0
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] [get_bd_intf_pins axi_uartlite_0/s_axi]
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] [get_bd_intf_pins axi_timer_0/s_axi]
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] [get_bd_intf_pins fe_axi_config_kv_0/s_axi]

# 时钟与复位（PS FCLK_CLK0 100MHz 驱动 PL；外设复用同一时钟）
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_pins axi_uartlite_0/s_axi_aclk]
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_pins axi_timer_0/s_axi_aclk]
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_pins fe_axi_config_kv_0/s_axi_aclk]
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] [get_bd_pins axi_uartlite_0/s_axi_aresetn]
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] [get_bd_pins axi_timer_0/s_axi_aresetn]
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] [get_bd_pins fe_axi_config_kv_0/s_axi_aresetn]

# 地址分配（与 micro_blaze/User/fe_port.c 默认宏一致）
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs axi_uartlite_0/S_AXI/Reg] -range 0x10000 -offset 0x40600000
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs axi_timer_0/S_AXI/Reg] -range 0x10000 -offset 0x41C00000
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs fe_axi_config_kv_0/s_axi/Reg] -range 0x10000 -offset 0x40000000

validate_bd_design
save_bd_design

# 生成 HDL wrapper 并设为顶层
set bd_path [get_property DIRECTORY [current_project]]/fe_bd.srcs/sources_1/bd/fe_bd
make_wrapper -files [file join $bd_path fe_bd.bd] -top
add_files -norecurse [file join $bd_path hdl fe_bd_wrapper.v]
set_property top fe_bd_wrapper [current_fileset]
update_compile_order -fileset sources_1

puts "INFO: project created at $proj_dir"
puts "INFO: 基地址: UARTLite=0x40600000 Timer=0x41C00000 KV=0x40000000"
