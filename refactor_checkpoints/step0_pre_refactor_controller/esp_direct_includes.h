#pragma once

#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <string.h>
#include <stdint.h>

// --- Direct-mode compact protocol (binary) ---
// Roles
constexpr uint8_t DIRECT_ROLE_NONE = 0;
constexpr uint8_t DIRECT_ROLE_FRESH = 1;
constexpr uint8_t DIRECT_ROLE_WASTE = 2;

// Message types
constexpr uint8_t DIRECT_MSG_STATE_FAST = 1;
constexpr uint8_t DIRECT_MSG_STATE_SLOW = 2;
constexpr uint8_t DIRECT_MSG_EVENT = 3;

// Flags bitfield (u16)
constexpr uint16_t DIRECT_FLAG_LEAK = 1 << 0;
constexpr uint16_t DIRECT_FLAG_FAULT = 1 << 1;
constexpr uint16_t DIRECT_FLAG_PAIRED = 1 << 2;
constexpr uint16_t DIRECT_FLAG_FILL = 1 << 3;
constexpr uint16_t DIRECT_FLAG_DRAIN = 1 << 4;
constexpr uint16_t DIRECT_FLAG_SENSOR_ERR = 1 << 5;

// Event codes
constexpr uint8_t DIRECT_EVENT_RELAY_ACK = 1;
constexpr uint8_t DIRECT_EVENT_FAULT_SET = 2;
constexpr uint8_t DIRECT_EVENT_FAULT_CLEAR = 3;
constexpr uint8_t DIRECT_EVENT_SENSOR_ERR = 4;
constexpr uint8_t DIRECT_EVENT_SENSOR_OK = 5;

constexpr uint8_t DIRECT_PROTOCOL_VER = 1;

// Tiny SipHash-2-4 (64-bit output) adapted for 32-bit keys (expanded to 128-bit)
static inline uint64_t siphash24(const uint8_t *data, size_t len, uint32_t key) {
    // Expand 32-bit key into 128-bit by repetition.
    uint32_t k0 = key;
    uint32_t k1 = key ^ 0x736f6d65U;
    uint32_t k2 = key ^ 0x646f7261U;
    uint32_t k3 = key ^ 0x6c796765U;
    uint64_t v0 = ((uint64_t)k0 << 32) | k1;
    uint64_t v1 = ((uint64_t)k2 << 32) | k3;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t b = ((uint64_t)len) << 56;
    const uint8_t *end = data + len - (len % 8);
    while (data != end) {
        uint64_t m;
        memcpy(&m, data, sizeof(uint64_t));
        v3 ^= m;
        for (int i = 0; i < 2; ++i) {
            v0 += v1; v2 += v3; v1 = (v1 << 13) | (v1 >> (64 - 13)); v3 = (v3 << 16) | (v3 >> (64 - 16));
            v1 ^= v0; v3 ^= v2; v0 = (v0 << 32) | (v0 >> (64 - 32));
            v2 += v1; v0 += v3; v1 = (v1 << 17) | (v1 >> (64 - 17)); v3 = (v3 << 21) | (v3 >> (64 - 21));
            v1 ^= v2; v3 ^= v0; v2 = (v2 << 32) | (v2 >> (64 - 32));
        }
        v0 ^= m;
        data += 8;
    }
    uint64_t m = b;
    switch (len & 7) {
        case 7: m |= ((uint64_t)data[6]) << 48;
        case 6: m |= ((uint64_t)data[5]) << 40;
        case 5: m |= ((uint64_t)data[4]) << 32;
        case 4: m |= ((uint64_t)data[3]) << 24;
        case 3: m |= ((uint64_t)data[2]) << 16;
        case 2: m |= ((uint64_t)data[1]) << 8;
        case 1: m |= ((uint64_t)data[0]); break;
        default: break;
    }
    v3 ^= m;
    for (int i = 0; i < 2; ++i) {
        v0 += v1; v2 += v3; v1 = (v1 << 13) | (v1 >> (64 - 13)); v3 = (v3 << 16) | (v3 >> (64 - 16));
        v1 ^= v0; v3 ^= v2; v0 = (v0 << 32) | (v0 >> (64 - 32));
        v2 += v1; v0 += v3; v1 = (v1 << 17) | (v1 >> (64 - 17)); v3 = (v3 << 21) | (v3 >> (64 - 21));
        v1 ^= v2; v3 ^= v0; v2 = (v2 << 32) | (v2 >> (64 - 32));
    }
    v0 ^= m;
    v2 ^= 0xff;
    for (int i = 0; i < 4; ++i) {
        v0 += v1; v2 += v3; v1 = (v1 << 13) | (v1 >> (64 - 13)); v3 = (v3 << 16) | (v3 >> (64 - 16));
        v1 ^= v0; v3 ^= v2; v0 = (v0 << 32) | (v0 >> (64 - 32));
        v2 += v1; v0 += v3; v1 = (v1 << 17) | (v1 >> (64 - 17)); v3 = (v3 << 21) | (v3 >> (64 - 21));
        v1 ^= v2; v3 ^= v0; v2 = (v2 << 32) | (v2 >> (64 - 32));
    }
    return v0 ^ v1 ^ v2 ^ v3;
}

static inline uint32_t direct_auth_tag(uint32_t key, const uint8_t *buf, size_t len) {
    return (uint32_t)(siphash24(buf, len, key) & 0xFFFFFFFFu);
}

// Encode helpers (little endian)
static inline void enc_u16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
static inline void enc_u32(uint8_t *p, uint32_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF; }
static inline uint16_t dec_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static inline uint32_t dec_u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
// Build direct packets into caller-provided buffer. Returns length (excluding auth), total len includes auth.
static inline size_t direct_build_state_fast(uint8_t *buf, uint16_t seq, uint8_t role, uint16_t flags,
                                             uint8_t fault_code, uint8_t relay_state, uint8_t level_pct_x1,
                                             int16_t temp_c_x10, uint32_t uptime_s) {
    // Layout: ver(1), type(1), seq(2), role(1), flags(2), fault_code(1), relay(1), lvl(1), temp(2), uptime(4), auth(4)
    buf[0] = DIRECT_PROTOCOL_VER;
    buf[1] = DIRECT_MSG_STATE_FAST;
    enc_u16(&buf[2], seq);
    buf[4] = role;
    enc_u16(&buf[5], flags);
    buf[7] = fault_code;
    buf[8] = relay_state;
    buf[9] = level_pct_x1;
    enc_u16(&buf[10], (uint16_t)temp_c_x10);
    enc_u32(&buf[12], uptime_s);
    return 16;  // bytes before auth
}

static inline size_t direct_build_state_slow(uint8_t *buf, uint16_t seq, uint8_t role, uint16_t vbat_mv, int8_t rssi) {
    buf[0] = DIRECT_PROTOCOL_VER;
    buf[1] = DIRECT_MSG_STATE_SLOW;
    enc_u16(&buf[2], seq);
    buf[4] = role;
    enc_u16(&buf[5], vbat_mv);
    buf[7] = (uint8_t)rssi;
    buf[8] = 0;  // reserved
    return 9;    // before auth
}

static inline size_t direct_build_event(uint8_t *buf, uint16_t seq, uint8_t role, uint8_t evt, uint16_t value,
                                        uint16_t flags) {
    buf[0] = DIRECT_PROTOCOL_VER;
    buf[1] = DIRECT_MSG_EVENT;
    enc_u16(&buf[2], seq);
    buf[4] = role;
    buf[5] = evt;
    enc_u16(&buf[6], value);
    enc_u16(&buf[8], flags);
    return 10;  // before auth
}
