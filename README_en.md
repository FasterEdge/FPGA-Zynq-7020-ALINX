<div align="center">
<img src="https://avatars.githubusercontent.com/u/245985800?s=200&v=4" style="width:100px;" width="100"/>
<h2>FasterEdge FPGA - Zynq-7020 (ALINX AX7020)</h2>
<h3>FasterEdge framework on Zynq-7020 (MicroBlaze / HLS / Vivado)</h3>
</div>

### 1. Introduction

This repo implements the **[FasterEdge](https://github.com/FasterEdge/FasterEdge)** framework on a **Xilinx Zynq-7020 (ALINX AX7020 board)** in multiple flavours: a **MicroBlaze soft core** runs the full FasterEdge subset in C, **HLS / pure RTL** provide hardware acceleration (HMAC-SHA256 etc), and **SymbiFlow** gates the design with open-source tooling.

- ✅ micro_blaze: MicroBlaze soft core in C (UART / timer / GPIO; includes MQTT/EdgeRole/NetMap network set)
- ✅ vitis_hls: HMAC-SHA256 / OneKey HLS IP acceleration
- ✅ vivado: Block Design project (AXI bus to soft core / PS)
- ✅ symbi_flow: Yosys / SymbiFlow open-source synthesis & simulation
- ✅ Same names & commands as the main repo

### 2. Directory Layout

```
FPGA-Zynq-7020-ALINX/
├── micro_blaze/               # MicroBlaze soft core C project (fullest abilities)
│   ├── Core/                  # fe.h / fe.c / fe_hmac_sha256.c
│   ├── Inc/                   # fe_ability.h / fe_data.h / fe_port.h
│   ├── Ability/               # ability_*.c (incl. mqtt/edgerole/configfile)
│   ├── Data/                  # data_*.c (incl. keyring/netmap)
│   ├── User/                  # main.c / register.c / fe_port.c (porting layer)
│   ├── fe_nvs/                # persistent files (NVS sample)
│   ├── Makefile               # mb-gcc build
│   └── lscript.ld             # linker script
├── vitis_hls/                 # HLS project (HMAC-SHA256 / OneKey IP)
│   ├── src/                   # fe_hmac_sha256.cpp / fe_onekey_hls.cpp
│   └── tb/                    # testbench
├── vivado/                    # Vivado project (Block Design + AXI)
│   ├── rtl/                   # custom RTL (fe_axi_config_kv.sv etc)
│   ├── scripts/               # create_project.tcl
│   ├── sim/                   # simulation
│   └── xdc/                   # AX7020 constraints
├── symbi_flow/                # open-source toolchain (Yosys) checks
├── LICENSE                    # Apache-2.0
└── README.md
```

### 3. Usage

**micro_blaze (soft core C):**

```sh
cd micro_blaze
make            # mb-gcc cross build
# Import the ELF in SDK/Vitis and run on MicroBlaze; serial 115200
```

**vitis_hls (hardware acceleration):**

```sh
cd vitis_hls
vivado_hls -f scripts/run_hls.tcl   # synthesize fe_hmac/fe_onekey IP
```

**vivado (Block Design):**

```sh
cd vivado
vivado -source scripts/create_project.tcl   # create project (AX7020 board)
```

**symbi_flow (open-source CI):**

```sh
cd symbi_flow
sh scripts/build.sh   # yosys synthesis check; iverilog sim: see README there
```

**Serial command examples (after micro_blaze boots):**

```
help
ability_BaseAbility list_ability_names
ability_RoleAbility set_role edge
ability_MqttAbility set_broker 192.168.1.10,1883
ability_OneKeyAbility issue_token sensor01
ability_ModbusAbility set_unit_id 3
data_NetMapData get node1
```

> The MicroBlaze edition is the **fullest** in the FPGA family: besides network-required items, it keeps MQTTAbility / EdgeRoleAbility / ConfigFileAbility / NetMapData / KeyringData (isomorphic with MCU-ESP32).

### 4. Capabilities

| Category | Capability | Description |
|----------|------------|-------------|
| Network | MQTTAbility / NetMapData / EdgeRoleAbility | soft core with network stack (lwIP), trim as needed |
| Base | Base/Role/Time/OneKey/Serial/Modbus | commands identical to MCU platforms |
| Storage | ConfigData / KeyringData | fe_nvs file persistence |
| Accel | RegAbility / HMAC | HLS + RTL hardware implementation |

### 5. Version

- **1.0.20260829** (in sync with all FasterEdge MCU platform versions)

### 6. Sibling Projects

- **[FasterEdge FPGA - Artix-7 XC7A35T](https://github.com/FasterEdge/FPGA-Artix7-XC7A35T)**: pure RTL implementation
- **[FasterEdge](https://github.com/FasterEdge/FasterEdge)**: framework main repo
