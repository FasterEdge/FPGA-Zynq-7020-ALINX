// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// fe_onekey_hls.h — OneKeyAbility HLS 加速核接口
#ifndef FE_ONEKEY_HLS_H
#define FE_ONEKEY_HLS_H

#include <cstdint>

// secret 固定 32 字节（与 MCU 版 fe_onekey_ability_t.secret 一致）
// token 输出 43 字符 base64url + NUL（45 字节缓冲）
extern "C" {
void fe_onekey_issue(const std::uint8_t secret[33],
                     std::uint32_t seq,
                     const char subject[17],
                     std::uint8_t mac[32],
                     char token[45]);

// 返回 1 = 有效，0 = 无效
int fe_onekey_verify(const std::uint8_t secret[33],
                     std::uint32_t seq,
                     const char subject[17],
                     const char token[45]);
}

#endif // FE_ONEKEY_HLS_H
