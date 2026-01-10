/* ver:2026_01_10__09_07 */
#include "i1d3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

// 11 Master Keys from Argyll CMS
typedef struct { char *name; uint32_t key[2]; } i1d3_key_entry;
static const i1d3_key_entry I1D3_CODES[] = {
    {"Retail", {0xe9622e9f, 0x8d63e133}}, {"Munki", {0xe01e6e0a, 0x257462de}},
    {"OEM", {0xcaa62b2c, 0x30815b61}},    {"NEC", {0xa9119479, 0x5b168761}},
    {"Quato", {0x160eb6ae, 0x14440e70}},  {"HP", {0x291e41d7, 0x51937bdd}},
    {"Wacom", {0x1abfae03, 0xf25ac8e8}}, {"TPA", {0x828c43e9, 0xcbb8a8ed}},
    {"Barco", {0xe8d1a980, 0xd146f7ad}},  {"Crysta", {0x171ae295, 0x2e5c7664}},
    {"Viewsonic", {0x64d8c546, 0x4b24b4a7}}
};

// Emissive Matrix from user log
static const double MATRIX[3][3] = {
    {0.035814, -0.021980, 0.016668},
    {0.014015, 0.016946, 0.000451},
    {-0.000407, 0.000830, 0.078830}
};

int i1d3_open(const char *path) {
    char cmd[256];
    sprintf(cmd, "sudo chmod 666 %s 2>/dev/null", path);
    system(cmd); // Set Linux hidraw permissions

    int fd = open(path, O_RDWR);
    if (fd < 0) { perror("i1d3_open failed"); return -1; }
    return fd;
}

int i1d3_close(int fd) { return close(fd); }

int i1d3_send(int fd, uint8_t *buf, int len) {
    return write(fd, buf, len);
}

int i1d3_recv(int fd, uint8_t *buf, int maxlen) {
    return read(fd, buf, maxlen);
}

int i1d3_init_sequence(int fd) {
    uint8_t cmds[][2] = {{0x00, 0x01}, {0x00, 0x10}, {0x00, 0x11}, {0x00, 0x12}, {0x10, 0x00}, {0x00, 0x31}, {0x00, 0x13}, {0x00, 0x20}};
    uint8_t buf[64];
    for (int i = 0; i < 8; i++) {
        memset(buf, 0, 64);
        buf[0] = cmds[i][0]; buf[1] = cmds[i][1];
        i1d3_send(fd, buf, 64);
        usleep(150000); // 150ms delay
        i1d3_recv(fd, buf, 64);
    }
    return 0;
}

int i1d3_unlock(int fd, const uint32_t key[2]) {
    uint8_t buf[64] = {0};
    buf[0] = 0x99; buf[1] = 0x00; // Get Challenge
    i1d3_send(fd, buf, 64);
    if (i1d3_recv(fd, buf, 64) < 64 || buf[1] != 0x99) return -1;

    uint8_t c2 = buf[2], c3 = buf[3];
    uint8_t sc[8];
    for (int i = 0; i < 8; i++) sc[i] = c3 ^ buf[35 + i];

    uint32_t ci0 = (sc[3] << 24) | (sc[0] << 16) | (sc[4] << 8) | sc[6];
    uint32_t ci1 = (sc[1] << 24) | (sc[7] << 16) | (sc[2] << 8) | sc[5];
    uint32_t nK0 = ~key[0] + 1, nK1 = ~key[1] + 1; // 2's Complement

    uint32_t co[4] = {nK0 - ci1, nK1 - ci0, ci1 * nK0, ci0 * nK1};
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) sum += sc[i];
    auto keySum = [](uint32_t v) { return (v & 0xFF) + ((v >> 8) & 0xFF) + ((v >> 16) & 0xFF) + ((v >> 24) & 0xFF); };
    sum += keySum(nK0) + keySum(nK1);
    uint8_t s0 = sum & 0xFF, s1 = (sum >> 8) & 0xFF;

    uint8_t sr[16];
    sr[0] = ((co[0] >> 16) & 0xFF) + s0; sr[1] = ((co[2] >> 8) & 0xFF) - s1; sr[2] = (co[3] & 0xFF) + s1; sr[3] = ((co[1] >> 16) & 0xFF) + s0;
    sr[4] = ((co[2] >> 16) & 0xFF) - s1; sr[5] = ((co[3] >> 16) & 0xFF) - s0; sr[6] = ((co[1] >> 24) & 0xFF) - s0; sr[7] = (co[0] & 0xFF) - s1;
    sr[8] = ((co[3] >> 8) & 0xFF) + s0;  sr[9] = ((co[2] >> 24) & 0xFF) - s1; sr[10] = ((co[0] >> 8) & 0xFF) + s0; sr[11] = ((co[1] >> 8) & 0xFF) - s1;
    sr[12] = (co[1] & 0xFF) + s1;        sr[13] = ((co[3] >> 24) & 0xFF) + s1; sr[14] = (co[2] & 0xFF) + s0;      sr[15] = ((co[0] >> 24) & 0xFF) - s0;

    memset(buf, 0, 64);
    buf[0] = 0x9A; // Send Response
    for (int i = 0; i < 16; i++) buf[24 + i] = c2 ^ sr[i];
    i1d3_send(fd, buf, 64);
    i1d3_recv(fd, buf, 64);
    return (buf[2] == 0x77) ? 0 : -1; // Code 77 = Success
}

int i1d3_auto_find_unlock(int fd) {
    for (int i = 0; i < 11; i++) {
        printf("[INFO] Attempt %d/11: Testing %s...\n", i + 1, I1D3_CODES[i].name);
        if (i1d3_unlock(fd, I1D3_CODES[i].key) == 0) {
            printf("[SUCCESS] Instrument unlocked using %s keys.\n", I1D3_CODES[i].name);
            return 0;
        }
        usleep(400000);
    }
    return -1;
}

int i1d3_aio_measure(int fd, i1d3_color_results *res) {
    uint8_t buf[64] = {0x04, 0x00, 0x9F, 0x24, 0x00, 0x00, 0x07, 0xE8, 0x03}; // 0.2s measure
    i1d3_send(fd, buf, 64);
    usleep(500000);
    if (i1d3_recv(fd, buf, 64) < 64 || buf[1] != 0x04) return -1;

    uint32_t rCnt = *(uint32_t*)&buf[2], gCnt = *(uint32_t*)&buf[6], bCnt = *(uint32_t*)&buf[10];
    uint32_t rClk = *(uint32_t*)&buf[14], gClk = *(uint32_t*)&buf[18], bClk = *(uint32_t*)&buf[22];

    auto toHz = [](uint32_t cnt, uint32_t clk) { return (cnt <= 1) ? 0.0 : (cnt - 1) * 0.25 / (clk / 48000000.0); };
    double R = toHz(rCnt, rClk), G = toHz(gCnt, gClk), B = toHz(bCnt, bClk);

    res->X = MATRIX[0][0]*R + MATRIX[0][1]*G + MATRIX[0][2]*B;
    res->Y = MATRIX[1][0]*R + MATRIX[1][1]*G + MATRIX[1][2]*B;
    res->Z = MATRIX[2][0]*R + MATRIX[2][1]*G + MATRIX[2][2]*B;

    double sum = res->X + res->Y + res->Z;
    res->x = (sum > 0) ? res->X / sum : 0;
    res->y = (sum > 0) ? res->Y / sum : 0;

    double n = (res->x - 0.3320) / (0.1858 - res->y); // McCamy's
    res->CCT = 449.0*pow(n, 3) + 3525.0*pow(n, 2) + 6823.3*n + 5524.33;

    auto f = [](double t) { return (t > 0.008856) ? pow(t, 1.0/3.0) : (7.787 * t + 16.0/116.0); };
    double fX = f(res->X / 96.42), fY = f(res->Y / 100.0), fZ = f(res->Z / 82.49); // D50
    res->L = 116.0 * fY - 16.0; res->a = 500.0 * (fX - fY); res->b = 200.0 * (fY - fZ);
    return 0;
}
