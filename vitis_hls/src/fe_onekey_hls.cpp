// fe_onekey_hls.cpp — FasterEdge Vitis HLS 版 OneKeyAbility 核心算法
// 令牌 = base64url(HMAC-SHA256(secret, "seq:subject"))，与 MCU 版
// ability_onekey.c 完全同构（seq 十进制 + ':' + subject）。
// 本模块可综合为 PL 加速核；一个 top 即一个 Issue/Verify 流水。
#include "fe_onekey_hls.h"

extern "C" {
void fe_hmac_sha256(const std::uint8_t *key, std::uint32_t key_len,
                    const std::uint8_t *msg, std::uint32_t msg_len,
                    std::uint8_t mac[32]);
}

static const char B64URL_TBL[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// u32 -> 十进制（返回位数）
static int u32_to_dec(uint32_t v, char *out) {
    char tmp[10];
    int n = 0;
    do { tmp[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    for (int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    return n;
}

// base64url（标准 43 字符输出 = ceil(32/3)*4 - 1 pad 位省略，与 MCU 版一致）
static void b64url_encode_32(const std::uint8_t mac[32], char out[45]) {
    int n = 0;
    int i = 0;
    for (i = 0; i + 2 < 32; i += 3) {
        uint32_t v = ((uint32_t)mac[i] << 16) | ((uint32_t)mac[i+1] << 8) | mac[i+2];
        out[n++] = B64URL_TBL[(v >> 18) & 63];
        out[n++] = B64URL_TBL[(v >> 12) & 63];
        out[n++] = B64URL_TBL[(v >> 6) & 63];
        out[n++] = B64URL_TBL[v & 63];
    }
    // 剩 2 字节（32 % 3 == 2）
    uint32_t v = ((uint32_t)mac[30] << 16) | ((uint32_t)mac[31] << 8);
    out[n++] = B64URL_TBL[(v >> 18) & 63];
    out[n++] = B64URL_TBL[(v >> 12) & 63];
    out[n++] = B64URL_TBL[(v >> 6) & 63];
    out[n] = 0;
}

// 构造 payload "seq:subject"（seq 十进制），返回长度
static int build_payload(uint32_t seq, const char subject[17], char payload[32]) {
    char seqbuf[10];
    int sl = u32_to_dec(seq, seqbuf);
    int n = 0;
    for (int i = 0; i < sl; i++) payload[n++] = seqbuf[i];
    payload[n++] = ':';
    for (int i = 0; i < 16 && subject[i]; i++) payload[n++] = subject[i];
    payload[n] = 0;
    return n;
}

extern "C" void fe_onekey_issue(const std::uint8_t secret[33],
                                std::uint32_t seq,
                                const char subject[17],
                                std::uint8_t mac[32],
                                char token[45]) {
    char payload[32];
    int plen = build_payload(seq, subject, payload);
    fe_hmac_sha256(secret, 32, (const std::uint8_t *)payload, (std::uint32_t)plen, mac);
    b64url_encode_32(mac, token);
}

extern "C" int fe_onekey_verify(const std::uint8_t secret[33],
                                std::uint32_t seq,
                                const char subject[17],
                                const char token[45]) {
    std::uint8_t mac[32];
    char expect[45];
    char payload[32];
    int plen = build_payload(seq, subject, payload);
    fe_hmac_sha256(secret, 32, (const std::uint8_t *)payload, (std::uint32_t)plen, mac);
    b64url_encode_32(mac, expect);
    for (int i = 0; i < 44; i++) {
        if (expect[i] != token[i]) return 0;
        if (expect[i] == 0) return token[i] == 0 ? 1 : 0;
    }
    return 1;
}
