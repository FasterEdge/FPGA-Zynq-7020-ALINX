# AX7020.xdc — ALINX AX7020 (Zynq-7020 CLG484) 引脚约束
# 本工程 PL 侧仅 KV 存储（纯 AXI），常规使用无需 PL 引脚。
# 若需要 PL 侧调试 LED/按键，请参照 ALINX 官方工程模板（ALINX 开源资料
# ALINX_ZYNQ/01_TEST_EXPERIMENT）填写对应 BANK 引脚，示例占位如下：
#
# set_property -dict { PACKAGE_PIN <LED_PIN> IOSTANDARD LVCMOS33 } [get_ports { led[0] }]
# set_property -dict { PACKAGE_PIN <KEY_PIN> IOSTANDARD LVCMOS33 } [get_ports { key[0] }]
#
# 时钟由 PS FCLK_CLK0（100MHz）提供，无需 PL 时钟约束。
