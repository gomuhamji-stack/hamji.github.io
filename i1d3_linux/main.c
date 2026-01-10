/* ver:2026_01_10__09_07 */
#include <stdio.h>
#include <unistd.h>
#include "i1d3.h"

int main() {
    const char *dev = "/dev/hidraw0";
    int fd = i1d3_open(dev);
    if (fd < 0) return 1;

    printf("[SYS] Initializing sequence...\n");
    i1d3_init_sequence(fd);

    printf("[SYS] Starting Auto-Unlock...\n");
    if (i1d3_auto_find_unlock(fd) != 0) {
        printf("[FATAL] All keys failed. Check device connection.\n");
        i1d3_close(fd); return 1;
    }

    printf("[SYS] Taking measurements (3 times)...\n");
    i1d3_color_results res;
    for (int i = 0; i < 3; i++) {
        if (i1d3_aio_measure(fd, &res) == 0) {
            printf("[%d] XYZ: %.2f, %.2f, %.2f | xy: %.4f, %.4f | CCT: %.0fK | Lab: %.1f, %.1f, %.1f\n", 
                   i+1, res.X, res.Y, res.Z, res.x, res.y, res.CCT, res.L, res.a, res.b);
        }
        sleep(1);
    }

    i1d3_close(fd);
    printf("[SYS] Finished.\n");
    return 0;
}
