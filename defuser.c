#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t lo = (uint32_t)challenge;
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t first = (hi * 5u + mid) ^ 0xec30u;
    uint32_t second = (lo + hi * 8u) ^ 0x7b21u;
    uint32_t values[] = {(second - first) * 3u + 0x691bu};
    return write_answer(out, cap, values, 1);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((challenge >> 44) & 127u) ^ 2u;
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t nibble = (uint32_t)((challenge >> (17u + 5u * i)) & 15u);
        values[i] = (values[i - 1] ^ (nibble + i)) * 5u;
    }
    return write_answer(out, cap, values, 6);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    /* Jump-table order is 0, 6, 5, 1, 4, 7, 3, 2, not symbol order. */
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = (uint32_t)challenge & 7u;
    switch (values[0]) {
        case 0: values[1] = (x ^ 0xa85eu) + (x >> 1); break;
        case 1: values[1] = 3u * x + 0x5a16u; break;
        case 2: values[1] = 5u * x + 0x466eu; break;
        case 3: values[1] = 5u * x + 0x1798u; break;
        case 4: values[1] = (2u * x) ^ 0x425fu; break;
        case 5: values[1] = (x ^ 0xb495u) + (x >> 2); break;
        case 6: values[1] = 5u * x + 0x31b9bu; break;
        case 7: values[1] = (4u * x) ^ 0x7b4au; break;
    }
    return write_answer(out, cap, values, 2);
}

static uint32_t walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + 0x49u) ^ salt;
    if (target < middle) {
        return (5u * walk(target, low, middle - 1u, salt)) ^ (middle + salt) ^ 0x1au;
    }
    return (5u * walk(target, middle + 1u, high, salt)) ^ (middle + salt) ^ 0x0du;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    uint32_t values[] = {walk((uint32_t)challenge & 127u, 0, 127, salt)};
    return write_answer(out, cap, values, 1);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* Nodes ordered by their addresses, 0x405ba0 through 0x405c50. */
    static const struct { uint32_t value, tag; unsigned next; } nodes[] = {
        {0x596b, 0xb10, 5}, {0x6f75, 0x643, 3}, {0xd1ec, 0x570, 12},
        {0x4df1, 0x31e, 6}, {0x7c6e, 0x90a, 9}, {0x23d7, 0x475, 2},
        {0xa5a8, 0x052, 0}, {0xd8e0, 0xa20, 11}, {0xb847, 0x11f, 7},
        {0x7763, 0x220, 8}, {0xa381, 0x86f, 1}, {0x601a, 0x72c, 10}
    };
    static const unsigned heads[] = {4, 9, 8, 7, 11, 10, 1, 3};
    unsigned node = heads[challenge & 7u];
    unsigned count = 1u + (unsigned)((challenge >> 3) & 3u);
    uint32_t value = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32) ^ 0x24a0u;
    uint32_t tag = 0;
    for (unsigned i = 0; i < count; ++i) {
        tag = nodes[node].tag;
        value = (3u * value + nodes[node].value) ^ tag;
        node = nodes[node].next;
    }
    uint32_t values[] = {value, tag};
    return write_answer(out, cap, values, 2);
}

static uint32_t op0(uint32_t x) { return (x ^ (x >> 7)) + 0xea0bu; }
static uint32_t op1(uint32_t x) { return (4u * x + 0xfcc9u) ^ (3u * x); }
static uint32_t op2(uint32_t x) { return (x >> 1) | (x << 31); }
static uint32_t op3(uint32_t x) { return (x ^ 0xb0c3u) + (x << 7); }
static uint32_t op4(uint32_t x) { return (x + 0xab65u) ^ (x >> 7); }
static uint32_t op5(uint32_t x) { return ((x << 2) | (x >> 30)) ^ 0xb27fu; }

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    static uint32_t (*const ops[])(uint32_t) = {op2, op5, op3, op3, op0, op1, op1, op4};
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = ops[challenge & 7u](x) ^ 0x3442u;
    uint32_t values[] = {ops[(challenge >> 3) & 7u](first) + 0x2d2bu};
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t mode = (uint32_t)context & 3u;
    uint32_t a = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t b = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t x = a ^ b;
    uint32_t second = ((x >> 7) | (x << 25)) ^ 0xbf87u
                    ^ (mode * 0xceb9u) ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ (b + mode + 0x713eu + (a >> 7));
    /* Offset 32 overwrites only the adjacent OPEN field within the local object. */
    int size = snprintf(out, cap, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                        " %" PRIu32 " %" PRIu32, mode, second, third);
    return size < 0 || (size_t)size >= cap;
}
