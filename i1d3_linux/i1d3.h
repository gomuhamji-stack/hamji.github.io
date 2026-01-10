/* ver:2026_01_10__09_07 */
#ifndef I1D3_H
#define I1D3_H

#include <stdint.h>

typedef struct {
    double X, Y, Z;
    double x, y;
    double CCT;
    double L, a, b;
} i1d3_color_results;

// Core communication functions
int i1d3_open(const char *path);
int i1d3_close(int fd);
int i1d3_send(int fd, uint8_t *buf, int len);
int i1d3_recv(int fd, uint8_t *buf, int maxlen);

// Logic sequences
int i1d3_init_sequence(int fd);
int i1d3_unlock(int fd, const uint32_t key[2]);
int i1d3_auto_find_unlock(int fd);
int i1d3_aio_measure(int fd, i1d3_color_results *res);

#endif
