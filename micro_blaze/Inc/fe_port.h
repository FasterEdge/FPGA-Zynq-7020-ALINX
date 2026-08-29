// fe_port.h — FasterEdge FPGA 平台移植层（MicroBlaze 软核版）
// 平台相关能力在此抽象：串口收发、NVS 存储、系统时间、随机数。
// MicroBlaze 运行于 Zynq-7020 PL，通过 AXI 总线挂载外设：
//   - AXI UART Lite：控制台（命令行 CLI）
//   - AXI Timer    ：系统时间（级联 64 位自由计数）
//   - PL KV 存储   ：可选，经 AXI4-Lite 读写（与 vivado/ 工程的
//     fe_axi_config_kv 配合，宏 FE_PL_CONFIG_BASE 打开）
#ifndef FE_PORT_H
#define FE_PORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 串口（UART）
// ============================================================
typedef void (*fe_port_uart_rx_cb_t)(uint8_t byte, void *user);

// 初始化串口：port 编号（0 = AXI UART Lite 控制台），baud 波特率，rx 回调（可为 NULL）
void fe_port_uart_init(uint8_t port, uint32_t baud, fe_port_uart_rx_cb_t rx_cb, void *user);
// 发送 len 字节，返回实际发送字节数
size_t fe_port_uart_write(uint8_t port, const uint8_t *data, size_t len);
// 是否有数据可读
bool fe_port_uart_available(uint8_t port);
// 读一个字节
int fe_port_uart_read(uint8_t port);
// 关闭串口
void fe_port_uart_close(uint8_t port);

// ============================================================
// 非易失存储（NVS）
// 默认实现：DDR/BRAM 内 KV 表（重启丢失，仅保序演示）。
// 定义 FE_PL_CONFIG_BASE 后改走 PL AXI4-Lite KV 存储硬件。
// ============================================================
// 读字符串：ns 命名空间，key 键，out 输出缓冲，outlen 缓冲长度。
// 存在返回 true。
bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen);
// 写字符串：成功返回 true
bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value);
// 删除键：成功返回 true
bool fe_port_nvs_remove(const char *ns, const char *key);
// 读无符号整数
bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out);
// 写无符号整数
bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value);

// ============================================================
// 系统时间（epoch 秒）
// ============================================================
// 读取当前 epoch 秒（AXI Timer 自由计数 + 偏移量合成）
uint64_t fe_port_time_now(void);
// 设置 epoch 秒
void fe_port_time_set(uint64_t epoch);
// 从 NTP 同步（server 可为 NULL 用默认）。MicroBlaze 无协议栈，返回 -1。
// 如需真实校时：挂 AXI Ethernet + lwIP 后在移植层实现。
int fe_port_time_sync_ntp(const char *server);

// ============================================================
// 随机数
// ============================================================
// 填充 len 字节随机数（xorshift，种子取自上电计数器）
void fe_port_random_fill(uint8_t *buf, size_t len);

// ============================================================
// 网络（TCP）——MQTT 等需要
// ============================================================
// WiFi/以太网是否已连接
bool fe_port_wifi_connected(void);
// 获取本机 IP 字符串（写入 out）
void fe_port_wifi_ip(char *out, size_t outlen);
// 建立 TCP 连接：host, port。返回 0 成功
int fe_port_tcp_connect(const char *host, uint16_t port);
// 发送数据，返回实际发送
size_t fe_port_tcp_write(const uint8_t *data, size_t len);
// 读取数据，返回字节数（0 = 无数据，-1 = 断开）
int fe_port_tcp_read(uint8_t *buf, size_t len);
// 断开 TCP
void fe_port_tcp_close(void);

// ============================================================
// 延时（毫秒）
// ============================================================
void fe_port_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // FE_PORT_H
