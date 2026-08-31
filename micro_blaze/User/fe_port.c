/* FasterEdge 开源项目
 * GitHub: https://github.com/FasterEdge
 * Gitee:  https://gitee.com/FasterEdge
 */
// fe_port.c — FasterEdge FPGA 平台移植层实现（MicroBlaze 软核版）
// 目标环境：Zynq-7020 PL 中的 MicroBlaze（vivado/ 工程例化），
// 通过 AXI4-Lite 挂载 AXI UART Lite（控制台）与 AXI Timer（时间）。
// 外设基地址由编译宏给出（与 vivado/scripts/create_project.tcl 中的
// 地址分配一致），可按实际工程通过 -D 覆盖。
//
//   FE_UARTLITE_BASE   AXI UART Lite 控制台基地址（默认 0x40600000）
//   FE_TIMER_BASE      AXI Timer 基地址（默认 0x41C00000，级联 64 位自由计数）
//   FE_TIMER_FREQ      AXI Timer 计数频率（默认 100000000）
//   FE_PL_CONFIG_BASE  可选：PL KV 存储基地址（定义后 NVS 走 PL 硬件）
//
// 编译工具链：microblazeel-xilinx-elf-gcc（Vitis / ISE 版 SDK 均可）。
#include "fe_port.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------
// AXI 外设寄存器访问（Xilinx 驱动库或裸写均可用，这里用裸写保证零依赖）
// ------------------------------------------------------------
#ifndef FE_UARTLITE_BASE
#define FE_UARTLITE_BASE   0x40600000u
#endif
#ifndef FE_TIMER_BASE
#define FE_TIMER_BASE      0x41C00000u
#endif
#ifndef FE_TIMER_FREQ
#define FE_TIMER_FREQ      100000000u
#endif

#define UARTLITE_RX_TX     (*(volatile uint32_t *)(FE_UARTLITE_BASE + 0x00))
#define UARTLITE_STATUS    (*(volatile uint32_t *)(FE_UARTLITE_BASE + 0x08))

// AXI UART Lite 状态位
#define UL_STAT_RX_VALID   0x01u
#define UL_STAT_RX_FULL    0x02u
#define UL_STAT_TX_EMPTY   0x04u
#define UL_STAT_TX_FULL    0x08u

// AXI Timer 寄存器（级联模式：TCR0 低 32 位，TCR1 高 32 位）
#define AXI_TIMER_TCSR0    (*(volatile uint32_t *)(FE_TIMER_BASE + 0x00))
#define AXI_TIMER_TCR0     (*(volatile uint32_t *)(FE_TIMER_BASE + 0x08))
#define AXI_TIMER_TCSR1    (*(volatile uint32_t *)(FE_TIMER_BASE + 0x10))
#define AXI_TIMER_TCR1     (*(volatile uint32_t *)(FE_TIMER_BASE + 0x18))
#define AXI_TIMER_TCSR_ENT 0x00000080u   // Enable Timer
#define AXI_TIMER_TCSR_CASC 0x00000400u  // Cascade mode（双定时器级联 64 位）
#define AXI_TIMER_TCSR_UDT0 0x00000000u  // 递增计数（UDT=0）

static uint32_t g_timer_ticks_hi = 0;    // 软件扩展的高 32 位（轮询进位）
static uint64_t g_epoch_offset   = 0;    // 时间偏移（epoch 起点）

static uint64_t timer_read64(void) {
    uint32_t hi = AXI_TIMER_TCR1;
    uint32_t lo = AXI_TIMER_TCR0;
    uint32_t hi2 = AXI_TIMER_TCR1;
    if (hi != hi2) { lo = AXI_TIMER_TCR0; hi = hi2; }  // 读翻转保护
    return ((uint64_t)hi << 32) | lo;
}

// ------------------------------------------------------------
// 串口（AXI UART Lite）
// ------------------------------------------------------------
static fe_port_uart_rx_cb_t g_rx_cb = NULL;
static void *g_rx_user = NULL;

void fe_port_uart_init(uint8_t port, uint32_t baud, fe_port_uart_rx_cb_t rx_cb, void *user) {
    (void)baud;   // AXI UART Lite 波特率由 Vivado 工程配置，软件不可改
    (void)port;   // 仅支持控制台（port 0）
    g_rx_cb = rx_cb;
    g_rx_user = user;
    // 启动 AXI Timer（级联 64 位自由计数），作为系统节拍
    AXI_TIMER_TCSR0 = AXI_TIMER_TCSR_CASC;
    AXI_TIMER_TCSR1 = AXI_TIMER_TCSR_ENT;
    AXI_TIMER_TCSR0 = AXI_TIMER_TCSR_CASC | AXI_TIMER_TCSR_ENT;
}

size_t fe_port_uart_write(uint8_t port, const uint8_t *data, size_t len) {
    (void)port;
    size_t n = 0;
    while (n < len) {
        if (UARTLITE_STATUS & UL_STAT_TX_FULL) continue;  // FIFO 满则等待
        UARTLITE_RX_TX = data[n++];
    }
    return n;
}

bool fe_port_uart_available(uint8_t port) {
    (void)port;
    bool avail = (UARTLITE_STATUS & UL_STAT_RX_VALID) != 0;
    // 顺带维护 timer 高位进位（主循环每轮都会调用到）
    static uint32_t last_lo = 0;
    uint32_t lo = (uint32_t)timer_read64();
    if (lo < last_lo) g_timer_ticks_hi++;
    last_lo = lo;
    return avail;
}

int fe_port_uart_read(uint8_t port) {
    (void)port;
    if (!(UARTLITE_STATUS & UL_STAT_RX_VALID)) return -1;
    uint8_t b = (uint8_t)(UARTLITE_RX_TX & 0xFFu);
    if (g_rx_cb) g_rx_cb(b, g_rx_user);
    return b;
}

void fe_port_uart_close(uint8_t port) {
    (void)port;
    // AXI UART Lite 无需关闭
}

// ------------------------------------------------------------
// NVS：默认 RAM KV 表；定义 FE_PL_CONFIG_BASE 后走 PL KV 存储硬件
// ------------------------------------------------------------
#ifndef FE_PL_CONFIG_BASE

#define NVS_MAX_ENTRIES 32
#define NVS_KEY_LEN     32
#define NVS_VAL_LEN     128

typedef struct {
    bool used;
    char ns[16];
    char key[NVS_KEY_LEN];
    char val[NVS_VAL_LEN];
    uint32_t u32;      // 整数键直接存值（val 置空）
    bool is_u32;
} nvs_entry_t;

static nvs_entry_t g_nvs[NVS_MAX_ENTRIES];

static nvs_entry_t *nvs_find(const char *ns, const char *key) {
    for (int i = 0; i < NVS_MAX_ENTRIES; i++) {
        if (g_nvs[i].used && strcmp(g_nvs[i].ns, ns) == 0 && strcmp(g_nvs[i].key, key) == 0)
            return &g_nvs[i];
    }
    return NULL;
}

static nvs_entry_t *nvs_alloc(void) {
    for (int i = 0; i < NVS_MAX_ENTRIES; i++) {
        if (!g_nvs[i].used) return &g_nvs[i];
    }
    return NULL;
}

bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen) {
    nvs_entry_t *e = nvs_find(ns, key);
    if (!e || e->is_u32) return false;
    snprintf(out, outlen, "%s", e->val);
    return true;
}

bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value) {
    nvs_entry_t *e = nvs_find(ns, key);
    if (!e) {
        e = nvs_alloc();
        if (!e) return false;
        e->used = true;
        snprintf(e->ns, sizeof(e->ns), "%s", ns);
        snprintf(e->key, sizeof(e->key), "%s", key);
    }
    e->is_u32 = false;
    snprintf(e->val, sizeof(e->val), "%s", value);
    return true;
}

bool fe_port_nvs_remove(const char *ns, const char *key) {
    nvs_entry_t *e = nvs_find(ns, key);
    if (!e) return true;
    memset(e, 0, sizeof(*e));
    return true;
}

bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out) {
    nvs_entry_t *e = nvs_find(ns, key);
    if (!e) return false;
    if (e->is_u32) { *out = e->u32; return true; }
    *out = (uint32_t)strtoul(e->val, NULL, 10);
    return true;
}

bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value) {
    nvs_entry_t *e = nvs_find(ns, key);
    if (!e) {
        e = nvs_alloc();
        if (!e) return false;
        e->used = true;
        snprintf(e->ns, sizeof(e->ns), "%s", ns);
        snprintf(e->key, sizeof(e->key), "%s", key);
    }
    e->is_u32 = true;
    e->u32 = value;
    e->val[0] = 0;
    return true;
}

#else /* FE_PL_CONFIG_BASE：NVS 走 PL KV 存储硬件（vivado/ 工程 fe_axi_config_kv） */

// PL KV 存储寄存器映射（与 rtl/fe_axi_config_kv.sv 一致）：
//   每槽位 64B = 16 个 32 位字：[0]=key 哈希 [1]=value 长度 [2..15]=value 内容（56B）
#define PL_KV_SLOT_WORDS 16
#define PL_KV_SLOTS      16
#define PL_KV_REG(addr)  (*(volatile uint32_t *)(FE_PL_CONFIG_BASE + (addr)))

static uint32_t kv_hash(const char *ns, const char *key) {
    // FNV-1a：与 RTL 侧 fe_kv_hash 一致，保证软硬同一套键索引
    uint32_t h = 2166136261u;
    for (const char *p = ns; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    h ^= '#'; h *= 16777619u;
    for (const char *p = key; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    return h;
}

static int kv_find_slot(uint32_t h, bool match) {
    for (int i = 0; i < PL_KV_SLOTS; i++) {
        uint32_t sh = PL_KV_REG(i * PL_KV_SLOT_WORDS * 4);
        if (match && sh == h) return i;
        if (!match && sh == 0) return i;   // 空槽
    }
    return -1;
}

bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen) {
    uint32_t h = kv_hash(ns, key);
    int slot = kv_find_slot(h, true);
    if (slot < 0) return false;
    uint32_t len = PL_KV_REG(slot * PL_KV_SLOT_WORDS * 4 + 4);
    if (len > PL_KV_SLOT_WORDS * 4 - 8 || len >= outlen) return false;
    const volatile uint32_t *base = &PL_KV_REG(slot * PL_KV_SLOT_WORDS * 4 + 8);
    for (uint32_t i = 0; i < len; i++) out[i] = (char)(base[i >> 2] >> ((i & 3) * 8));
    out[len] = 0;
    return true;
}

bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value) {
    uint32_t h = kv_hash(ns, key);
    int slot = kv_find_slot(h, true);
    if (slot < 0) slot = kv_find_slot(0, false);
    if (slot < 0) return false;
    uint32_t len = 0;
    while (value[len] && len < PL_KV_SLOT_WORDS * 4 - 8) len++;
    uint32_t *base = (uint32_t *)&PL_KV_REG(slot * PL_KV_SLOT_WORDS * 4);
    base[0] = h;
    base[1] = len;
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t w = 0;
        for (uint32_t j = 0; j < 4 && i + j < len; j++)
            w |= (uint32_t)(uint8_t)value[i + j] << (j * 8);
        base[2 + (i >> 2)] = w;
    }
    return true;
}

bool fe_port_nvs_remove(const char *ns, const char *key) {
    uint32_t h = kv_hash(ns, key);
    int slot = kv_find_slot(h, true);
    if (slot < 0) return true;
    uint32_t *base = (uint32_t *)&PL_KV_REG(slot * PL_KV_SLOT_WORDS * 4);
    for (int w = 0; w < PL_KV_SLOT_WORDS; w++) base[w] = 0;
    return true;
}

bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out) {
    char buf[64];
    if (!fe_port_nvs_get_str(ns, key, buf, sizeof(buf))) return false;
    *out = (uint32_t)strtoul(buf, NULL, 10);
    return true;
}

bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", value);
    return fe_port_nvs_set_str(ns, key, buf);
}

#endif /* FE_PL_CONFIG_BASE */

// ------------------------------------------------------------
// 系统时间
// ------------------------------------------------------------
uint64_t fe_port_time_now(void) {
    uint64_t ticks = (((uint64_t)g_timer_ticks_hi) << 32) | timer_read64();
    return g_epoch_offset + ticks / FE_TIMER_FREQ;
}

void fe_port_time_set(uint64_t epoch) {
    uint64_t ticks = (((uint64_t)g_timer_ticks_hi) << 32) | timer_read64();
    g_epoch_offset = epoch - ticks / FE_TIMER_FREQ;
}

int fe_port_time_sync_ntp(const char *server) {
    // MicroBlaze 默认无网络协议栈；挂 AXI Ethernet + lwIP 后可实现 SNTP。
    (void)server;
    return -1;
}

// ------------------------------------------------------------
// 随机数（xorshift32，种子取自上电时的 timer 计数值）
// ------------------------------------------------------------
void fe_port_random_fill(uint8_t *buf, size_t len) {
    static uint32_t rng_state = 0;
    if (rng_state == 0) rng_state = (uint32_t)timer_read64() | 1u;
    for (size_t i = 0; i < len; i++) {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        buf[i] = (uint8_t)(rng_state >> 24);
    }
}

// ------------------------------------------------------------
// 网络（MicroBlaze 默认无协议栈，预留 lwIP 接入点）
// ------------------------------------------------------------
bool fe_port_wifi_connected(void) { return false; }

void fe_port_wifi_ip(char *out, size_t outlen) {
    snprintf(out, outlen, "0.0.0.0");
}

int fe_port_tcp_connect(const char *host, uint16_t port) {
    // REQUIRED_PORT_HOOK: 接入 lwIP socket API（axi_ethernet + lwIP 2023.x）
    // MicroBlaze 侧尚未挂载 lwIP 协议栈；返回 -1 表示暂时无网络。
    (void)host; (void)port;
    return -1;
}

size_t fe_port_tcp_write(const uint8_t *data, size_t len) {
    (void)data;
    return len;
}

int fe_port_tcp_read(uint8_t *buf, size_t len) {
    (void)buf; (void)len;
    return 0;
}

void fe_port_tcp_close(void) {}

// ------------------------------------------------------------
// 延时（毫秒，基于 AXI Timer）
// ------------------------------------------------------------
void fe_port_delay_ms(uint32_t ms) {
    uint64_t start = timer_read64();
    uint64_t wait = (uint64_t)ms * (FE_TIMER_FREQ / 1000u);
    while (timer_read64() - start < wait) { /* busy wait */ }
}
