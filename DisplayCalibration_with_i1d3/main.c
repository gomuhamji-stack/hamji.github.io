#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "i1d3_api.h"
#include "display_calibration_api.h"

// Global variables for sensor and calibrator state
static int i1d3_sensor_fd = -1;
static Calibrator cal;

// --- Helper Functions ---

// Function to clear input buffer after scanf
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Function to get a valid integer input from the user
int get_integer_input(const char *prompt) {
    int value;
    printf("%s", prompt);
    while (scanf("%d", &value) != 1) {
        printf("Invalid input. Please enter an integer: ");
        clear_input_buffer();
    }
    clear_input_buffer();
    return value;
}

// Function to display the debug menu
void display_menu() {
    printf("\n--- Debug Menu ---\n");
    printf("1. Initialize Sensor\n");
    printf("2. Read Sensor (Single Measurement)\n");
    printf("3. Change RGB Gain (Manual)\n");
    printf("4. Calibrate RGB Gain (Automatic)\n");
    printf("0. Exit\n");
    printf("------------------\n");
}

// --- Debug Menu Action Functions ---

void test_sensor_init() {
    const char *device_path = "/dev/hidraw0"; // Default i1d3 device path
    printf("[MENU] Initializing Sensor...\n");

    if (i1d3_sensor_fd != -1) {
        printf("[INFO] Sensor already open. Closing and re-opening.\n");
        i1d3_close(i1d3_sensor_fd);
        i1d3_sensor_fd = -1;
    }

    i1d3_sensor_fd = i1d3_open(device_path);
    if (i1d3_sensor_fd < 0) {
        fprintf(stderr, "[ERROR] Failed to open i1d3 device: %s (Error Code: %d)\n", i1d3_error_string(i1d3_sensor_fd), i1d3_sensor_fd);
        return;
    }
    printf("[INFO] i1d3 device opened (FD: %d).\n", i1d3_sensor_fd);

    i1d3_error_t err = i1d3_init_sequence(i1d3_sensor_fd);
    if (err != I1D3_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to initialize i1d3 sequence: %s\n", i1d3_error_string(err));
        i1d3_close(i1d3_sensor_fd);
        i1d3_sensor_fd = -1;
        return;
    }
    printf("[INFO] i1d3 initialization sequence complete.\n");

    err = i1d3_auto_find_unlock(i1d3_sensor_fd);
    if (err != I1D3_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to auto-unlock i1d3 device: %s\n", i1d3_error_string(err));
        i1d3_close(i1d3_sensor_fd);
        i1d3_sensor_fd = -1;
        return;
    }
    printf("[INFO] i1d3 device successfully unlocked.\n");

    // Initialize the calibrator with default target and gains
    Calibrator_init(&cal, 0.3127, 0.3290, 192, 192, 192);
    printf("[INFO] Calibrator initialized with target (x=%.4f, y=%.4f) and default gains (R%d, G%d, B%d).\n", 
           cal.target_x, cal.target_y, cal.current_gain[0], cal.current_gain[1], cal.current_gain[2]);
}

void test_sensor_read() {
    printf("[MENU] Performing single sensor measurement...\n");
    if (i1d3_sensor_fd < 0 || i1d3_get_state(i1d3_sensor_fd) != I1D3_STATE_UNLOCKED) {
        fprintf(stderr, "[ERROR] Sensor not initialized or unlocked. Please run '1. Initialize Sensor' first.\n");
        return;
    }

    CalibratedColorValue measured_color;
    if (Calibrator_get_current_color_from_sensor(i1d3_sensor_fd, &measured_color) != 0) {
        fprintf(stderr, "[ERROR] Failed to read color from sensor.\n");
        return;
    }

    printf("Measured Color: X=%.2f, Y=%.2f, Z=%.2f | x=%.4f, y=%.4f | CCT=%.0fK | L=%.1f, a=%.1f, b=%.1f\n",
           measured_color.X, measured_color.Y, measured_color.Z,
           measured_color.x, measured_color.y, // CCT, L, a, b are part of i1d3_color_results but not CalibratedColorValue
           0.0, 0.0, 0.0, 0.0); // Placeholder for now, as CalibratedColorValue only has xyY

    // To get CCT, L, a, b, we need to get the full i1d3_color_results and then copy relevant parts
    i1d3_color_results full_i1d3_res;
    if (i1d3_aio_measure(i1d3_sensor_fd, &full_i1d3_res) == I1D3_SUCCESS) {
         printf("Full Measurement: X=%.2f, Y=%.2f, Z=%.2f | x=%.4f, y=%.4f | CCT=%.0fK | L=%.1f, a=%.1f, b=%.1f\n",
           full_i1d3_res.X, full_i1d3_res.Y, full_i1d3_res.Z,
           full_i1d3_res.x, full_i1d3_res.y, full_i1d3_res.CCT, full_i1d3_res.L, full_i1d3_res.a, full_i1d3_res.b);
    }
}

void test_change_rgb_gain() {
    printf("[MENU] Manually changing RGB Gain...\n");
    if (i1d3_sensor_fd < 0 || i1d3_get_state(i1d3_sensor_fd) != I1D3_STATE_UNLOCKED) {
        fprintf(stderr, "[ERROR] Sensor not initialized or unlocked. Please run '1. Initialize Sensor' first.\n");
        return;
    }

    int r_gain = get_integer_input("Enter Red Gain (0-192): ");
    int g_gain = get_integer_input("Enter Green Gain (0-192): ");
    int b_gain = get_integer_input("Enter Blue Gain (0-192): ");

    cal.current_gain[0] = r_gain;
    cal.current_gain[1] = g_gain;
    cal.current_gain[2] = b_gain;

    Calibrator_set_tv_gain(cal.current_gain[0], cal.current_gain[1], cal.current_gain[2]);
    printf("[INFO] TV Gain set to R=%d, G=%d, B=%d.\n", r_gain, g_gain, b_gain);
}

void test_calibration_rgb_gain() {
    printf("[MENU] Starting automatic RGB Gain calibration...\n");
    if (i1d3_sensor_fd < 0 || i1d3_get_state(i1d3_sensor_fd) != I1D3_STATE_UNLOCKED) {
        fprintf(stderr, "[ERROR] Sensor not initialized or unlocked. Please run '1. Initialize Sensor' first.\n");
        return;
    }

    int num_steps = get_integer_input("Enter number of calibration steps: ");
    if (num_steps <= 0) {
        fprintf(stderr, "[ERROR] Number of steps must be positive.\n");
        return;
    }

    printf("Initializing calibrator for automatic calibration...\n");
    // Re-initialize calibrator to reset its state for a fresh calibration run
    Calibrator_init(&cal, 0.3127, 0.3290, cal.current_gain[0], cal.current_gain[1], cal.current_gain[2]);

    if (Calibrator_check_sensitivity(&cal, i1d3_sensor_fd) != 0) {
        fprintf(stderr, "[ERROR] Failed to check display sensitivity. Aborting calibration.\n");
        return;
    }

    printf("Starting %d calibration steps...\n", num_steps);
    for (int i = 1; i <= num_steps; i++) {
        if (Calibrator_perform_calibration_step(&cal, i1d3_sensor_fd, i) != 0) {
            fprintf(stderr, "[ERROR] Calibration step %d failed. Aborting.\n", i);
            break;
        }
    }

    int best_r, best_g, best_b;
    Calibrator_get_best_gain(&cal, &best_r, &best_g, &best_b);
    printf("\n--- Calibration Finished ---\n");
    printf("Best Gain Found: R=%d, G=%d, B=%d (Minimum Distance: %.6f)\n", best_r, best_g, best_b, cal.min_dist);

    // Apply the best gain at the end of calibration
    Calibrator_set_tv_gain(best_r, best_g, best_b);
    cal.current_gain[0] = best_r;
    cal.current_gain[1] = best_g;
    cal.current_gain[2] = best_b;
}

// --- Main Function ---

int main(int argc, char *argv[]) {
    int debug_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-dbg") == 0) {
            debug_mode = 1;
            break;
        }
    }

    if (debug_mode) {
        int choice;
        do {
            display_menu();
            choice = get_integer_input("Enter your choice: ");

            switch (choice) {
                case 1: test_sensor_init(); break;
                case 2: test_sensor_read(); break;
                case 3: test_change_rgb_gain(); break;
                case 4: test_calibration_rgb_gain(); break;
                case 0: printf("Exiting debug menu.\n"); break;
                default: printf("Invalid choice. Please try again.\n"); break;
            }
            printf("\n");

        } while (choice != 0);

        if (i1d3_sensor_fd != -1) {
            i1d3_close(i1d3_sensor_fd);
            printf("[INFO] i1d3 device closed.\n");
        }
    } else {
        printf("Running in normal operation mode. Use '-dbg' for debug menu.\n");
        printf("Normal operation not yet implemented.\n");
        // TODO: Implement normal operational flow here
    }

    return 0;
}
