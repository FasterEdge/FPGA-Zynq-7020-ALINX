// fe_onekey_tb.cpp — HLS OneKey / HMAC 自校验测试
// 向量来源：RFC 4231（HMAC-SHA256 测试向量 TC1 / TC2）
// 运行（本机 / csim 通用）：g++ -std=c++11 -I src tb/fe_onekey_tb.cpp \
//   src/fe_hmac_sha256.cpp src/fe_onekey_hls.cpp -o /tmp/fe_hls_tb && /tmp/fe_hls_tb
#include "../src/fe_onekey_hls.h"
#include "../src/fe_hmac_sha256.h"

#include <cstdio>
#include <cstring>

static int hex2bytes(const char *hex, uint8_t *out, int n) {
    for (int i = 0; i < n; i++) {
        unsigned v;
        if (sscanf(hex + i * 2, "%2x", &v) != 1) return -1;
        out[i] = (uint8_t)v;
    }
    return 0;
}

static int check_mac(const char *name, const uint8_t mac[32], const char *expect_hex) {
    uint8_t expect[32];
    hex2bytes(expect_hex, expect, 32);
    if (memcmp(mac, expect, 32) != 0) {
        printf("FAIL %s\n  got    ", name);
        for (int i = 0; i < 4; i++) printf("%02x", mac[i]);
        printf("...\n  expect %02x%02x%02x%02x...\n", expect[0], expect[1], expect[2], expect[3]);
        return 1;
    }
    printf("PASS %s\n", name);
    return 0;
}

int main() {
    int errors = 0;

    // RFC 4231 TC1: key = 0x0b x20, data = "Hi There"
    {
        uint8_t key[20]; for (int i = 0; i < 20; i++) key[i] = 0x0b;
        const char *data = "Hi There";
        uint8_t mac[32];
        fe_hmac_sha256(key, 20, (const uint8_t *)data, 8, mac);
        errors += check_mac("RFC4231 TC1", mac,
            "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    }
    // RFC 4231 TC2: key = "Jefe", data = "what do ya want for nothing?"
    {
        const char *key = "Jefe";
        const char *data = "what do ya want for nothing?";
        uint8_t mac[32];
        fe_hmac_sha256((const uint8_t *)key, 4, (const uint8_t *)data, 28, mac);
        errors += check_mac("RFC4231 TC2", mac,
            "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    }
    // SHA-256("abc")
    {
        uint8_t d[32];
        fe_sha256((const uint8_t *)"abc", 3, d);
        errors += check_mac("SHA256 abc", d,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }

    // OneKey issue / verify 往返
    {
        uint8_t secret[33];
        for (int i = 0; i < 32; i++) secret[i] = (uint8_t)('A' + i);
        uint8_t mac[32];
        char token[45];
        fe_onekey_issue(secret, 0, "sensor01", mac, token);
        printf("token(seq=0,sensor01) = %s\n", token);
        if (!fe_onekey_verify(secret, 0, "sensor01", token)) {
            printf("FAIL verify roundtrip (seq=0)\n");
            errors++;
        }
        fe_onekey_issue(secret, 7, "edge-2", mac, token);
        if (!fe_onekey_verify(secret, 7, "edge-2", token)) {
            printf("FAIL verify roundtrip (seq=7)\n");
            errors++;
        }
        token[0] ^= 1;  // 篡改 1 位
        if (fe_onekey_verify(secret, 7, "edge-2", token)) {
            printf("FAIL tampered token accepted\n");
            errors++;
        } else {
            printf("PASS tampered token rejected\n");
        }
    }

    if (errors == 0) { printf("ALL TESTS PASSED\n"); return 0; }
    printf("%d ERRORS\n", errors);
    return 1;
}
