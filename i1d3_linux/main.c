/* ver:2026_01_13__10_00 - Updated for enhanced error handling */
#include <stdio.h>
#include <unistd.h>
#include "i1d3.h"

int main(void) {
    const char *dev = "/dev/hidraw0";
    int fd = i1d3_open(dev);
    if (fd < 0) {
        printf("[FATAL] Failed to open device %s: %s\n", dev, i1d3_error_string(fd));
        return 1;
    }

    printf("[SYS] Initializing sequence...\n");
    i1d3_error_t result = i1d3_init_sequence(fd);
    if (result != I1D3_SUCCESS) {
        printf("[FATAL] Initialization failed: %s\n", i1d3_error_string(result));
        i1d3_close(fd);
        return 1;
    }

    printf("[SYS] Starting Auto-Unlock...\n");
    result = i1d3_auto_find_unlock(fd);
    if (result != I1D3_SUCCESS) {
        printf("[FATAL] Auto-unlock failed: %s\n", i1d3_error_string(result));
        i1d3_close(fd);
        return 1;
    }

    printf("[SYS] Taking measurements (3 times)...\n");
    i1d3_color_results res;
    for (int i = 0; i < 3; i++) {
        result = i1d3_aio_measure(fd, &res);
        if (result == I1D3_SUCCESS) {
            printf("[%d] XYZ: %.2f, %.2f, %.2f | xy: %.4f, %.4f | CCT: %.0fK | Lab: %.1f, %.1f, %.1f\n", 
                   i+1, res.X, res.Y, res.Z, res.x, res.y, res.CCT, res.L, res.a, res.b);
        } else {
            printf("[%d] Measurement failed: %s\n", i+1, i1d3_error_string(result));
        }
        sleep(1);
    }

    i1d3_close(fd);
    printf("[SYS] Finished.\n");
    return 0;
}
