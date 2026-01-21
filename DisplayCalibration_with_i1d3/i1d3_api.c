/* ver:2026_01_13__10_00 - Enhanced with error handling and state management */
#include "i1d3_api.h" // Changed from "i1d3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

// Device state tracking (per file descriptor)
static i1d3_state_t device_states[256] = {I1D3_STATE_DISCONNECTED};

// Timeout configuration (in microseconds)
#define I1D3_TIMEOUT_INIT 150000
#define I1D3_TIMEOUT_UNLOCK 400000
#define I1D3_TIMEOUT_MEASURE 500000
#define I1D3_MAX_RETRIES 3

// Helper functions for calculations
static uint8_t keySum(uint32_t v) {
    return (v & 0xFF) + ((v >> 8) & 0xFF) + ((v >> 16) & 0xFF) + ((v >> 24) & 0xFF);
}

static double toHz(uint32_t cnt, uint32_t clk) {
    return (cnt <= 1) ? 0.0 : (cnt - 1) * 0.25 / (clk / 48000000.0);
}

static double labFunction(double t) {
    return (t > 0.008856) ? pow(t, 1.0/3.0) : (7.787 * t + 16.0/116.0);
}

// Error string mapping
const char* i1d3_error_string(i1d3_error_t error) {
    switch (error) {
        case I1D3_SUCCESS: return "Success";
        case I1D3_ERROR_OPEN_FAILED: return "Failed to open device";
        case I1D3_ERROR_PERMISSION_DENIED: return "Permission denied";
        case I1D3_ERROR_DEVICE_NOT_FOUND: return "Device not found";
        case I1D3_ERROR_INVALID_RESPONSE: return "Invalid response from device";
        case I1D3_ERROR_TIMEOUT: return "Operation timeout";
        case I1D3_ERROR_UNLOCK_FAILED: return "Unlock failed";
        case I1D3_ERROR_MEASUREMENT_FAILED: return "Measurement failed";
        case I1D3_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case I1D3_ERROR_NOT_INITIALIZED: return "Device not initialized";
        default: return "Unknown error";
    }
}

// State management
i1d3_state_t i1d3_get_state(int fd) {
    if (fd < 0 || fd >= 256) return I1D3_STATE_DISCONNECTED;
    return device_states[fd];
}

static void i1d3_set_state(int fd, i1d3_state_t state) {
    if (fd >= 0 && fd < 256) {
        device_states[fd] = state;
    }
}

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
// NOTE: This matrix must be calibrated for each sensor unit using i1d3_sensor_calibration.py
// The calibration procedure requires simultaneous measurements with a reference standard sensor.
// Update these values with the FCMM (Forward Color Matrix Model) output from the Python calibration tool.
// See README.md "Sensor Calibration Matrix" section for detailed instructions.
static const double MATRIX[3][3] = {
    {0.035814, -0.021980, 0.016668},
    {0.014015, 0.016946, 0.000451},
    {-0.000407, 0.000830, 0.078830}
};

int i1d3_open(const char *path) {
    if (!path) return I1D3_ERROR_INVALID_PARAMETER;

    // Try to set permissions first
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo chmod 666 %s 2>/dev/null", path);
    system(cmd);

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        switch (errno) {
            case ENOENT: return I1D3_ERROR_DEVICE_NOT_FOUND;
            case EACCES: return I1D3_ERROR_PERMISSION_DENIED;
            default: return I1D3_ERROR_OPEN_FAILED;
        }
    }

    // Initialize device state
    i1d3_set_state(fd, I1D3_STATE_CONNECTED);
    return fd;
}

int i1d3_close(int fd) {
    if (fd < 0) return I1D3_ERROR_INVALID_PARAMETER;

    int result = close(fd);
    if (result == 0) {
        i1d3_set_state(fd, I1D3_STATE_DISCONNECTED);
    }
    return result;
}

int i1d3_send(int fd, uint8_t *buf, int len) {
    if (fd < 0 || !buf || len <= 0) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) == I1D3_STATE_DISCONNECTED) return I1D3_ERROR_NOT_INITIALIZED;

    ssize_t written = write(fd, buf, len);
    if (written != len) {
        return I1D3_ERROR_OPEN_FAILED; // Could be more specific
    }
    return I1D3_SUCCESS;
}

int i1d3_recv(int fd, uint8_t *buf, int maxlen) {
    if (fd < 0 || !buf || maxlen <= 0) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) == I1D3_STATE_DISCONNECTED) return I1D3_ERROR_NOT_INITIALIZED;

    ssize_t received = read(fd, buf, maxlen);
    if (received < 0) {
        return I1D3_ERROR_OPEN_FAILED;
    }
    return (int)received;
}

i1d3_error_t i1d3_init_sequence(int fd) {
    if (fd < 0) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) != I1D3_STATE_CONNECTED) return I1D3_ERROR_NOT_INITIALIZED;

    uint8_t cmds[][2] = {{0x00, 0x01}, {0x00, 0x10}, {0x00, 0x11}, {0x00, 0x12}, {0x10, 0x00}, {0x00, 0x31}, {0x00, 0x13}, {0x00, 0x20}};
    uint8_t buf[64];

    for (int i = 0; i < 8; i++) {
        memset(buf, 0, 64);
        buf[0] = cmds[i][0];
        buf[1] = cmds[i][1];

        if (i1d3_send(fd, buf, 64) != I1D3_SUCCESS) {
            return I1D3_ERROR_OPEN_FAILED;
        }

        usleep(I1D3_TIMEOUT_INIT);

        int received = i1d3_recv(fd, buf, 64);
        if (received < 64) {
            return I1D3_ERROR_INVALID_RESPONSE;
        }
    }

    i1d3_set_state(fd, I1D3_STATE_INITIALIZED);
    return I1D3_SUCCESS;
}

i1d3_error_t i1d3_unlock(int fd, const uint32_t key[2]) {
    if (fd < 0 || !key) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) != I1D3_STATE_INITIALIZED) return I1D3_ERROR_NOT_INITIALIZED;

    uint8_t buf[64] = {0};
    buf[0] = 0x99; buf[1] = 0x00; // Get Challenge

    if (i1d3_send(fd, buf, 64) != I1D3_SUCCESS) {
        return I1D3_ERROR_OPEN_FAILED;
    }

    int received = i1d3_recv(fd, buf, 64);
    if (received < 64 || buf[1] != 0x99) {
        return I1D3_ERROR_INVALID_RESPONSE;
    }

    uint8_t c2 = buf[2], c3 = buf[3];
    uint8_t sc[8];
    for (int i = 0; i < 8; i++) sc[i] = c3 ^ buf[35 + i];

    uint32_t ci0 = (sc[3] << 24) | (sc[0] << 16) | (sc[4] << 8) | sc[6];
    uint32_t ci1 = (sc[1] << 24) | (sc[7] << 16) | (sc[2] << 8) | sc[5];
    uint32_t nK0 = ~key[0] + 1, nK1 = ~key[1] + 1; // 2's Complement

    uint32_t co[4] = {nK0 - ci1, nK1 - ci0, ci1 * nK0, ci0 * nK1};
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) sum += sc[i];
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

    if (i1d3_send(fd, buf, 64) != I1D3_SUCCESS) {
        return I1D3_ERROR_OPEN_FAILED;
    }

    received = i1d3_recv(fd, buf, 64);
    if (received < 64) {
        return I1D3_ERROR_INVALID_RESPONSE;
    }

    if (buf[2] == 0x77) { // Code 77 = Success
        i1d3_set_state(fd, I1D3_STATE_UNLOCKED);
        return I1D3_SUCCESS;
    }

    return I1D3_ERROR_UNLOCK_FAILED;
}

i1d3_error_t i1d3_auto_find_unlock(int fd) {
    if (fd < 0) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) != I1D3_STATE_INITIALIZED) return I1D3_ERROR_NOT_INITIALIZED;

    for (int i = 0; i < 11; i++) {
        printf("[INFO] Attempt %d/11: Testing %s...\n", i + 1, I1D3_CODES[i].name);
        i1d3_error_t result = i1d3_unlock(fd, I1D3_CODES[i].key);
        if (result == I1D3_SUCCESS) {
            printf("[SUCCESS] Instrument unlocked using %s keys.\n", I1D3_CODES[i].name);
            return I1D3_SUCCESS;
        }
        usleep(I1D3_TIMEOUT_UNLOCK);
    }
    printf("[ERROR] All unlock keys failed.\n");
    return I1D3_ERROR_UNLOCK_FAILED;
}

i1d3_error_t i1d3_aio_measure(int fd, i1d3_color_results *res) {
    if (fd < 0 || !res) return I1D3_ERROR_INVALID_PARAMETER;
    if (i1d3_get_state(fd) != I1D3_STATE_UNLOCKED) return I1D3_ERROR_NOT_INITIALIZED;

    uint8_t buf[64] = {0x04, 0x00, 0x9F, 0x24, 0x00, 0x00, 0x07, 0xE8, 0x03}; // 0.2s measure

    if (i1d3_send(fd, buf, 64) != I1D3_SUCCESS) {
        return I1D3_ERROR_OPEN_FAILED;
    }

    usleep(I1D3_TIMEOUT_MEASURE);

    int received = i1d3_recv(fd, buf, 64);
    if (received < 64 || buf[1] != 0x04) {
        return I1D3_ERROR_INVALID_RESPONSE;
    }

    uint32_t rCnt = *(uint32_t*)&buf[2], gCnt = *(uint32_t*)&buf[6];
    uint32_t rClk = *(uint32_t*)&buf[14], gClk = *(uint32_t*)&buf[18], bClk = *(uint32_t*)&buf[22];

    double R = toHz(rCnt, rClk), G = toHz(gCnt, gClk), B = toHz(0, bClk);

    res->X = MATRIX[0][0]*R + MATRIX[0][1]*G + MATRIX[0][2]*B;
    res->Y = MATRIX[1][0]*R + MATRIX[1][1]*G + MATRIX[1][2]*B;
    res->Z = MATRIX[2][0]*R + MATRIX[2][1]*G + MATRIX[2][2]*B;

    double sum = res->X + res->Y + res->Z;
    res->x = (sum > 0) ? res->X / sum : 0;
    res->y = (sum > 0) ? res->Y / sum : 0;

    double n = (res->x - 0.3320) / (0.1858 - res->y); // McCamy's
    res->CCT = 449.0*pow(n, 3) + 3525.0*pow(n, 2) + 6823.3*n + 5524.33;

    double fX = labFunction(res->X / 96.42), fY = labFunction(res->Y / 100.0), fZ = labFunction(res->Z / 82.49); // D50
    res->L = 116.0 * fY - 16.0; res->a = 500.0 * (fX - fY); res->b = 200.0 * (fY - fZ);
    return I1D3_SUCCESS;
}
