<div align="center">
<img src="https://avatars.githubusercontent.com/u/245985800?s=200&v=4" style="width:100px;" width="100"/>
<h2>FasterEdge FPGA - Zynq-7020 (ALINX AX7020)</h2>
<h3>FasterEdge 框架的 Zynq-7020 平台实现（MicroBlaze / HLS / Vivado）</h3>
</div>

### 一、简介

本项目是 **[FasterEdge](https://github.com/FasterEdge/FasterEdge)** 框架在 **Xilinx Zynq-7020（ALINX AX7020 开发板）** 上的多路实现：**MicroBlaze 软核** 以 C 运行 FasterEdge 完整能力子集，**HLS / 纯 RTL** 实现 HMAC-SHA256 等硬件加速，**SymbiFlow** 提供开源工具链 CI 门禁。

- ✅ micro_blaze：MicroBlaze 软核 C 实现（串口 / 定时器 / GPIO，含 MQTT/EdgeRole/NetMap 等网络能力集）
- ✅ vitis_hls：HMAC-SHA256 / OneKey 的 HLS IP 硬件加速
- ✅ vivado：Block Design 工程（AXI 总线接入软核 / PS）
- ✅ symbi_flow：Yosys / SymbiFlow 开源综合与仿真
- ✅ 与主仓库**同名同命令**，云边协同对等编程

### 二、目录结构

```
FPGA-Zynq-7020-ALINX/
├── micro_blaze/               # MicroBlaze 软核 C 工程（能力最全）
│   ├── Core/                  # fe.h / fe.c / fe_hmac_sha256.c
│   ├── Inc/                   # fe_ability.h / fe_data.h / fe_port.h
│   ├── Ability/               # ability_*.c（含 mqtt/edgerole/configfile）
│   ├── Data/                  # data_*.c（含 keyring/netmap）
│   ├── User/                  # main.c / register.c / fe_port.c（移植层）
│   ├── fe_nvs/                # 持久化文件（NVS 示例）
│   ├── Makefile               # mb-gcc 编译
│   └── lscript.ld             # 链接脚本
├── vitis_hls/                 # HLS 工程（HMAC-SHA256 / OneKey IP）
│   ├── src/                   # fe_hmac_sha256.cpp / fe_onekey_hls.cpp
│   └── tb/                    # 测试台
├── vivado/                    # Vivado 工程（Block Design + AXI）
│   ├── rtl/                   # 自研 RTL（fe_axi_config_kv.sv 等）
│   ├── scripts/               # create_project.tcl
│   ├── sim/                   # 仿真
│   └── xdc/                   # AX7020 约束
├── symbi_flow/                # 开源工具链（Yosys）综合检查
├── LICENSE                    # Apache-2.0
└── README.md
```

### 三、使用说明

**micro_blaze（软核 C）：**

```sh
cd micro_blaze
make            # mb-gcc 交叉编译
# 在 SDK/Vitis 中导入 ELF 到 MicroBlaze 运行，串口 115200
```

**vitis_hls（硬件加速）：**

```sh
cd vitis_hls
vivado_hls -f scripts/run_hls.tcl   # 综合导出 fe_hmac/fe_onekey IP
```

**vivado（Block Design）：**

```sh
cd vivado
vivado -source scripts/create_project.tcl   # 建工程（AX7020 板卡）
```

**symbi_flow（开源 CI）：**

```sh
cd symbi_flow
sh scripts/build.sh   # yosys 综合检查；iverilog 仿真见该目录 README
```

**串口命令示例（micro_blaze 启动后）：**

```
help
ability_BaseAbility list_ability_names
ability_RoleAbility set_role edge
ability_MqttAbility set_broker 192.168.1.10,1883
ability_OneKeyAbility issue_token sensor01
ability_ModbusAbility set_unit_id 3
data_NetMapData get node1
```

> MicroBlaze 版是 FPGA 系列中**能力最全**的实现：除网络剔除项外，还保留 MQTTAbility / EdgeRoleAbility / ConfigFileAbility / NetMapData / KeyringData（与 ESP32 同构）。

### 四、能力说明

| 类别 | 能力 | 说明 |
|------|------|------|
| 网络 | MQTTAbility / NetMapData / EdgeRoleAbility | 软核带网络协议栈（lwIP），可按需裁剪 |
| 基础 | Base/Role/Time/OneKey/Serial/Modbus | 与 MCU 各平台命令一致 |
| 存储 | ConfigData / KeyringData | fe_nvs 文件持久化 |
| 加速 | RegAbility / HMAC | HLS + RTL 硬件实现 |

### 五、版本

- **1.0.20260829**（与 FasterEdge MCU 各平台版本同步）

### 六、姊妹项目

- **[FasterEdge FPGA - Artix-7 XC7A35T](https://github.com/FasterEdge/FPGA-Artix7-XC7A35T)**：纯 RTL 实现
- **[FasterEdge](https://github.com/FasterEdge/FasterEdge)**：框架主仓库
