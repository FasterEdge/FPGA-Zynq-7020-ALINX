# symbi_flow — 开源工具链（Yosys）综合检查

开源 FPGA 工具链（Yosys / SymbiFlow-F4PGA / nextpnr-xilinx）目前不支持
Zynq-7020 的 PS7 与 Block Design 流程，因此本目录只做两件事：

1. 对 `vivado/rtl` 中的**纯 RTL**（`fe_axi_config_kv.sv`）运行
   `yosys synth_xilinx -family xc7` 综合与统计，作为开源 CI 门禁；
2. 复用 `vivado/sim` 的自校验仿真（iverilog / Vivado xsim 均可）。

## 运行

```sh
# 综合检查（需要 yosys >= 0.27）
sh scripts/build.sh

# 仿真（需要 iverilog）
iverilog -g2012 -o tb.vvp sim/tb_fe_axi_config_kv.sv rtl/fe_axi_config_kv.sv
vvp tb.vvp
```

完整比特流请使用 `vivado/` 工程（PS7 + AXI 外设 + PL KV 存储）。
