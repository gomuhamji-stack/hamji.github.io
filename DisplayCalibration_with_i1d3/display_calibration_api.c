#include "display_calibration_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// Private helper to clamp gain values
static int clamp_gain(int gain) {
    if (gain < 0) return 0;
    if (gain > 192) return 192;
    return gain;
}

void Calibrator_init(Calibrator *cal, double target_x, double target_y, int initial_r, int initial_g, int initial_b) {
    if (cal == NULL) return;

    cal->target_x = target_x;
    cal->target_y = target_y;
    
    cal->current_gain[0] = clamp_gain(initial_r);
    cal->current_gain[1] = clamp_gain(initial_g);
    cal->current_gain[2] = clamp_gain(initial_b);

    // Initialize best gain with current, and min_dist to a large value
    memcpy(cal->best_gain, cal->current_gain, sizeof(cal->current_gain));
    cal->min_dist = HUGE_VAL;

    // Initial sensitivity values (can be refined by Calibrator_check_sensitivity)
    cal->r_sens = 0.0006;
    cal->g_sens = 0.0005;
    cal->tv_fd = -1; // Initialize TV control handle as invalid
}

void Calibrator_set_tv_gain(int r, int g, int b) {
    // TODO: Implement actual hardware communication here (Serial/I2C/Network etc.)
    // For now, it's a printf for simulation.
    // In a real scenario, this might involve writing to a device file or sending commands.
    printf("[HW SIM] Set Gain: R=%d, G=%d, B=%d\n", r, g, b);
    // Example: write(cal->tv_fd, command_buffer, command_len);
    usleep(50000); // Simulate some delay for hardware response
}

int Calibrator_get_current_color_from_sensor(int fd, CalibratedColorValue *measured_color) {
    if (measured_color == NULL) {
        fprintf(stderr, "[ERROR] Calibrator_get_current_color_from_sensor: Null measured_color pointer.\n");
        return -1;
    }
    if (fd < 0) {
        fprintf(stderr, "[ERROR] Calibrator_get_current_color_from_sensor: Invalid i1d3 sensor file descriptor.\n");
        return -1;
    }

    i1d3_color_results i1d3_res;
    if (i1d3_aio_measure(fd, &i1d3_res) != 0) {
        fprintf(stderr, "[ERROR] Failed to get measurement from i1d3 sensor.\n");
        return -1;
    }

    measured_color->x = i1d3_res.x;
    measured_color->y = i1d3_res.y;
    measured_color->Y = i1d3_res.Y;
    measured_color->X = i1d3_res.X;
    measured_color->Z = i1d3_res.Z;

    return 0;
}

int Calibrator_check_sensitivity(Calibrator *cal, int sensor_fd) {
    if (cal == NULL) return -1;
    if (sensor_fd < 0) {
        fprintf(stderr, "[ERROR] Calibrator_check_sensitivity: Invalid sensor FD.\n");
        return -1;
    }

    printf(">>> Checking Display Sensitivity (collecting actual data)...\n");
    
    CalibratedColorValue base_cv;
    Calibrator_set_tv_gain(cal->current_gain[0], cal->current_gain[1], cal->current_gain[2]);
    if (Calibrator_get_current_color_from_sensor(sensor_fd, &base_cv) != 0) return -1;
    printf("Base Measurement: x=%.4f, y=%.4f\n", base_cv.x, base_cv.y);

    int test_step = 15; // Amount to change gain for sensitivity test
    int original_r = cal->current_gain[0];
    int original_g = cal->current_gain[1];

    // Measure R sensitivity
    Calibrator_set_tv_gain(clamp_gain(original_r - test_step), original_g, cal->current_gain[2]);
    CalibratedColorValue r_test_cv;
    if (Calibrator_get_current_color_from_sensor(sensor_fd, &r_test_cv) != 0) return -1;
    cal->r_sens = fabs(r_test_cv.x - base_cv.x) / test_step; // Sensitivity based on x change
    printf("R Test Measurement: x=%.4f, y=%.4f, dX=%.6f\n", r_test_cv.x, r_test_cv.y, fabs(r_test_cv.x - base_cv.x));

    // Measure G sensitivity
    Calibrator_set_tv_gain(original_r, clamp_gain(original_g - test_step), cal->current_gain[2]);
    CalibratedColorValue g_test_cv;
    if (Calibrator_get_current_color_from_sensor(sensor_fd, &g_test_cv) != 0) return -1;
    cal->g_sens = fabs(g_test_cv.y - base_cv.y) / test_step; // Sensitivity based on y change
    printf("G Test Measurement: x=%.4f, y=%.4f, dY=%.6f\n", g_test_cv.x, g_test_cv.y, fabs(g_test_cv.y - base_cv.y));

    // Restore original gain
    Calibrator_set_tv_gain(original_r, original_g, cal->current_gain[2]);
    usleep(100000); // Wait for TV to settle

    printf("Sensitivity analysis complete: R_Sens=%.6f, G_Sens=%.6f\n\n", cal->r_sens, cal->r_sens);
    if (cal->r_sens < 1e-7 || cal->g_sens < 1e-7) { // Prevent division by zero or very small sensitivity
        fprintf(stderr, "[WARNING] Sensitivity too low. Using default values.\n");
        cal->r_sens = 0.0006;
        cal->g_sens = 0.0005;
    }

    return 0;
}

int Calibrator_perform_calibration_step(Calibrator *cal, int sensor_fd, int step_num) {
    if (cal == NULL) return -1;

    CalibratedColorValue current_measured_color;
    // 1. Measure current state
    Calibrator_set_tv_gain(cal->current_gain[0], cal->current_gain[1], cal->current_gain[2]);
    if (Calibrator_get_current_color_from_sensor(sensor_fd, &current_measured_color) != 0) {
        fprintf(stderr, "[ERROR] Failed to get sensor data during calibration step.\n");
        return -1;
    }

    // 2. Calculate distance to target
    double dx = cal->target_x - current_measured_color.x;
    double dy = cal->target_y - current_measured_color.y;
    double dist = sqrt(dx * dx + dy * dy);

    // 3. Update best gain if current distance is better
    if (dist < cal->min_dist) {
        cal->min_dist = dist;
        memcpy(cal->best_gain, cal->current_gain, sizeof(cal->current_gain));
    }
    
    Calibrator_print_status(cal, step_num, &current_measured_color);

    // 4. Calculate predictive control based Gain adjustment
    double learning_rate = (dist > 0.005) ? 0.8 : 0.4;
    double adj_r = (dx / (cal->r_sens > 1e-7 ? cal->r_sens : 1e-7)) * learning_rate; // Avoid division by zero
    double adj_g = (dy / (cal->g_sens > 1e-7 ? cal->g_sens : 1e-7)) * learning_rate; // Avoid division by zero

    // 5. Apply new Gain (with clamping)
    cal->current_gain[0] = clamp_gain(cal->current_gain[0] + (int)round(adj_r));
    cal->current_gain[1] = clamp_gain(cal->current_gain[1] + (int)round(adj_g));
    
    // Blue Gain auxiliary logic (from original CCT code)
    if (dist > 0.01) {
        cal->current_gain[2] = clamp_gain(cal->current_gain[2] + (int)round((dx + dy) * 40));
    }

    // 6. Apply to actual hardware (simulated)
    Calibrator_set_tv_gain(cal->current_gain[0], cal->current_gain[1], cal->current_gain[2]);

    usleep(100000); // Pause for 100ms for TV to stabilize
    return 0;
}

void Calibrator_get_best_gain(Calibrator *cal, int *r, int *g, int *b) {
    if (cal == NULL || r == NULL || g == NULL || b == NULL) return;
    *r = cal->best_gain[0];
    *g = cal->best_gain[1];
    *b = cal->best_gain[2];
}

void Calibrator_print_status(Calibrator *cal, int step_num, CalibratedColorValue *measured_color) {
    if (cal == NULL || measured_color == NULL) return;
    printf("[%02d] R:%d G:%d B:%d | x:%.4f y:%.4f Y:%.2f | Dist:%.5f | R_sens:%.6f G_sens:%.6f\n", 
           step_num, cal->current_gain[0], cal->current_gain[1], cal->current_gain[2],
           measured_color->x, measured_color->y, measured_color->Y, cal->min_dist,
           cal->r_sens, cal->g_sens);
}
