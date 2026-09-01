// ─────────────────────────────────────────────────────────────
// FasterEdge 开源项目
// Github: https://github.com/FasterEdge
// Gitee:  https://gitee.com/FasterEdge
// ─────────────────────────────────────────────────────────────
// fe_hmac_sha256.h — FasterEdge Vitis HLS 版 SHA-256 / HMAC-SHA256 接口
#ifndef FE_HMAC_SHA256_H
#define FE_HMAC_SHA256_H

#include <cstdint>

extern "C" {
// SHA-256 摘要
void fe_sha256(const std::uint8_t *data, std::uint32_t len, std::uint8_t digest[32]);
// HMAC-SHA256（key_len<=64 直接用，>64 先散列；msg_len<=128）
void fe_hmac_sha256(const std::uint8_t *key, std::uint32_t key_len,
                    const std::uint8_t *msg, std::uint32_t msg_len,
                    std::uint8_t mac[32]);
}

#endif // FE_HMAC_SHA256_H
