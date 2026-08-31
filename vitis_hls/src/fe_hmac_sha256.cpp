// fe_hmac_sha256.cpp — FasterEdge Vitis HLS 版 SHA-256 / HMAC-SHA256
// 纯 C++（无动态内存、无递归），兼容 Vitis HLS 综合与本机 g++ 编译。
// 与 micro_blaze/User/fe_hmac_sha256.c 语义一致，便于交叉验证。
#include "fe_hmac_sha256.h"

#include <cstdint>
#include <cstring>

namespace {

const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

inline uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

inline void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

// 压缩单个 64B 块（state 为 8 个 32 位字，输入输出同址）
void sha256_compress(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
#pragma HLS ARRAY_PARTITION variable=w complete dim=1
    for (int t = 0; t < 16; t++)
        w[t] = load_be32(block + t * 4);
    for (int t = 16; t < 64; t++) {
        uint32_t s0 = rotr(w[t-15], 7) ^ rotr(w[t-15], 18) ^ (w[t-15] >> 3);
        uint32_t s1 = rotr(w[t-2], 17) ^ rotr(w[t-2], 19) ^ (w[t-2] >> 10);
        w[t] = w[t-16] + s0 + w[t-7] + s1;
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int t = 0; t < 64; t++) {
        uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K[t] + w[t];
        uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

} // namespace

extern "C" void fe_sha256(const uint8_t *data, uint32_t len, uint8_t digest[32]) {
    const uint32_t H0[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    uint32_t state[8];
    for (int i = 0; i < 8; i++) state[i] = H0[i];

    uint8_t block[64];
    uint64_t total = (uint64_t)len * 8u;
    uint32_t i = 0;
    while (len - i >= 64) {
        sha256_compress(state, data + i);
        i += 64;
    }
    uint32_t rest = len - i;
    for (uint32_t j = 0; j < rest; j++) block[j] = data[i + j];
    block[rest] = 0x80;
    for (uint32_t j = rest + 1; j < 64; j++) block[j] = 0;
    if (rest + 1 > 56) {
        sha256_compress(state, block);
        for (int j = 0; j < 56; j++) block[j] = 0;
    }
    for (int j = 0; j < 8; j++) block[56 + j] = (uint8_t)(total >> (56 - j * 8));
    sha256_compress(state, block);

    for (int j = 0; j < 8; j++) store_be32(digest + j * 4, state[j]);
}

extern "C" void fe_hmac_sha256(const uint8_t *key, uint32_t key_len,
                               const uint8_t *msg, uint32_t msg_len,
                               uint8_t mac[32]) {
    uint8_t kprime[64];
    memset(kprime, 0, sizeof(kprime));
    if (key_len > 64) {
        fe_sha256(key, key_len, kprime);       // 长密钥先散列
    } else {
        for (uint32_t i = 0; i < key_len; i++) kprime[i] = key[i];
    }

    uint8_t ipad[64], opad[64], inner[32];
    for (int i = 0; i < 64; i++) {
        ipad[i] = kprime[i] ^ 0x36u;
        opad[i] = kprime[i] ^ 0x5cu;
    }
    // inner = SHA256(ipad || msg)：流式拼接
    // （消息较短，直接复制到 64+MSG 缓冲，避免实现增量接口）
    uint8_t buf[64 + 128];
    const uint32_t bounded_msg_len = msg_len > 128u ? 128u : msg_len;
    for (int i = 0; i < 64; i++) buf[i] = ipad[i];
    for (uint32_t i = 0; i < bounded_msg_len; i++) buf[64 + i] = msg[i];
    // The HLS interface intentionally supports at most 128 message bytes.
    // Hash the bounded length as well as copying the bounded length; using
    // msg_len here would make fe_sha256 read beyond buf for oversized input.
    fe_sha256(buf, 64 + bounded_msg_len, inner);
    // outer = SHA256(opad || inner)
    uint8_t obuf[96];
    for (int i = 0; i < 64; i++) obuf[i] = opad[i];
    for (int i = 0; i < 32; i++) obuf[64 + i] = inner[i];
    fe_sha256(obuf, 96, mac);
}
