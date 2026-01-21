#ifndef DISPLAY_CALIBRATION_API_H
#define DISPLAY_CALIBRATION_API_H

#include "i1d3_api.h" // For i1d3_color_results

// Structure to hold color values (from sensor)
typedef struct {
    double x, y, Y; // xyY color space
    double X, Z; // XYZ values might also be useful
} CalibratedColorValue;

// Structure to manage the calibration state
typedef struct {
    double target_x, target_y; // Target chromaticity
    int current_gain[3];      // Current R, G, B gain values (0-192)
    int best_gain[3];         // Best R, G, B gain found during calibration
    double min_dist;          // Minimum distance to target chromaticity
    double r_sens;            // R channel sensitivity
    double g_sens;            // G channel sensitivity
    int tv_fd;                // File descriptor or handle for TV control (if applicable)
} Calibrator;

// --- API Functions ---

/**
 * @brief Initializes the Calibrator structure with target chromaticity and default gains.
 * @param cal Pointer to the Calibrator structure to initialize.
 * @param target_x Target x chromaticity.
 * @param target_y Target y chromaticity.
 * @param initial_r Initial Red gain (0-192).
 * @param initial_g Initial Green gain (0-192).
 * @param initial_b Initial Blue gain (0-192).
 */
void Calibrator_init(Calibrator *cal, double target_x, double target_y, int initial_r, int initial_g, int initial_b);

/**
 * @brief Sets the TV's RGB gain. This is a placeholder and needs actual hardware implementation.
 * @param r Red gain (0-192).
 * @param g Green gain (0-192).
 * @param b Blue gain (0-192).
 */
void Calibrator_set_tv_gain(int r, int g, int b);

/**
 * @brief Measures the current color values using the i1d3 sensor.
 *        This function internally calls i1d3_aio_measure.
 * @param fd The file descriptor for the i1d3 sensor.
 * @param measured_color Pointer to a CalibratedColorValue struct to store the results.
 * @return 0 on success, -1 on failure.
 */
int Calibrator_get_current_color_from_sensor(int fd, CalibratedColorValue *measured_color);


/**
 * @brief Checks the display's sensitivity for Red and Green channels.
 *        This involves taking multiple sensor measurements with modified gains.
 * @param cal Pointer to the Calibrator structure.
 * @param sensor_fd The file descriptor for the i1d3 sensor.
 * @return 0 on success, -1 on failure.
 */
int Calibrator_check_sensitivity(Calibrator *cal, int sensor_fd);

/**
 * @brief Performs a single step of the CCT calibration algorithm.
 *        It reads the sensor, calculates adjustments, and updates gains.
 * @param cal Pointer to the Calibrator structure.
 * @param sensor_fd The file descriptor for the i1d3 sensor.
 * @param step_num The current calibration step number (for logging).
 * @return 0 on success, -1 on failure.
 */
int Calibrator_perform_calibration_step(Calibrator *cal, int sensor_fd, int step_num);

/**
 * @brief Retrieves the best RGB gain values found during calibration.
 * @param cal Pointer to the Calibrator structure.
 * @param r Pointer to store the best Red gain.
 * @param g Pointer to store the best Green gain.
 * @param b Pointer to store the best Blue gain.
 */
void Calibrator_get_best_gain(Calibrator *cal, int *r, int *g, int *b);

/**
 * @brief Prints the current status of the calibrator (e.g., current gains, measured xy, distance).
 * @param cal Pointer to the Calibrator structure.
 * @param step_num Current step number.
 * @param measured_color The latest measured color values.
 */
void Calibrator_print_status(Calibrator *cal, int step_num, CalibratedColorValue *measured_color);


#endif // DISPLAY_CALIBRATION_API_H
