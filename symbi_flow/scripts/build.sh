#!/bin/sh
# build.sh — 用开源工具链（Yosys）对 PL 侧 RTL 做综合检查
# 用法：sh scripts/build.sh
# 说明：开源流程（Yosys synth_xilinx / 未来的 nextpnr-xilinx）不支持
# Zynq PS7 与 BD，本目录仅对纯 RTL（fe_axi_config_kv）做综合与等价性
# 检查；完整比特流仍请走 vivado/ 工程。
set -e
cd "$(dirname "$0")/.."

yosys -p "
    read_verilog -sv rtl/fe_axi_config_kv.sv
    hierarchy -top fe_axi_config_kv
    synth_xilinx -family xc7 -top fe_axi_config_kv
    stat
    write_json build/fe_axi_config_kv.json
"
echo "OK: synth done -> build/fe_axi_config_kv.json"
