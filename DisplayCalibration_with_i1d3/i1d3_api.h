/* ver:2026_01_13__10_00 - Enhanced with error handling and comprehensive documentation */
#ifndef I1D3_API_H
#define I1D3_API_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Error codes returned by i1d3 functions
 */
typedef enum {
    I1D3_SUCCESS = 0,                    /**< Operation completed successfully */
    I1D3_ERROR_OPEN_FAILED = -1,         /**< Failed to open device */
    I1D3_ERROR_PERMISSION_DENIED = -2,   /**< Permission denied accessing device */
    I1D3_ERROR_DEVICE_NOT_FOUND = -3,    /**< Device not found at specified path */
    I1D3_ERROR_INVALID_RESPONSE = -4,    /**< Invalid response received from device */
    I1D3_ERROR_TIMEOUT = -5,             /**< Operation timed out */
    I1D3_ERROR_UNLOCK_FAILED = -6,       /**< Failed to unlock device */
    I1D3_ERROR_MEASUREMENT_FAILED = -7,  /**< Color measurement failed */
    I1D3_ERROR_INVALID_PARAMETER = -8,   /**< Invalid function parameter */
    I1D3_ERROR_NOT_INITIALIZED = -9      /**< Device not in correct state for operation */
} i1d3_error_t;

/**
 * @brief Device connection states
 */
typedef enum {
    I1D3_STATE_DISCONNECTED = 0,  /**< Device is not connected */
    I1D3_STATE_CONNECTED = 1,     /**< Device is connected but not initialized */
    I1D3_STATE_INITIALIZED = 2,   /**< Device is initialized but not unlocked */
    I1D3_STATE_UNLOCKED = 3       /**< Device is fully ready for measurements */
} i1d3_state_t;

/**
 * @brief Color measurement results in multiple color spaces
 */
typedef struct {
    double X, Y, Z;    /**< CIE XYZ color coordinates */
    double x, y;       /**< CIE xy chromaticity coordinates */
    double CCT;        /**< Correlated Color Temperature in Kelvin */
    double L, a, b;    /**< CIE Lab color coordinates */
} i1d3_color_results;

/**
 * @brief Open a connection to an i1Display3 device
 *
 * This function opens the HID device at the specified path and attempts to set
 * appropriate permissions for access. The device must be physically connected
 * and accessible.
 *
 * @param path Path to the HID device (e.g., "/dev/hidraw0")
 * @return File descriptor on success, negative error code on failure
 */
int i1d3_open(const char *path);

/**
 * @brief Close the connection to an i1Display3 device
 *
 * @param fd File descriptor returned by i1d3_open()
 * @return 0 on success, negative error code on failure
 */
int i1d3_close(int fd);

/**
 * @brief Send data to the i1Display3 device
 *
 * @param fd File descriptor
 * @param buf Buffer containing data to send
 * @param len Length of data to send
 * @return Number of bytes sent on success, negative error code on failure
 */
int i1d3_send(int fd, uint8_t *buf, int len);

/**
 * @brief Receive data from the i1Display3 device
 *
 * @param fd File descriptor
 * @param buf Buffer to store received data
 * @param maxlen Maximum number of bytes to receive
 * @return Number of bytes received on success, negative error code on failure
 */
int i1d3_recv(int fd, uint8_t *buf, int maxlen);

/**
 * @brief Initialize the i1Display3 device
 *
 * Sends the required initialization sequence to prepare the device for operation.
 * Must be called after opening the device and before attempting to unlock it.
 *
 * @param fd File descriptor
 * @return I1D3_SUCCESS on success, error code on failure
 */
i1d3_error_t i1d3_init_sequence(int fd);

/**
 * @brief Unlock the device using specific manufacturer keys
 *
 * Attempts to unlock the device using the provided 64-bit key pair.
 * This is required before taking measurements.
 *
 * @param fd File descriptor
 * @param key Array of two 32-bit integers representing the unlock key
 * @return I1D3_SUCCESS on success, error code on failure
 */
i1d3_error_t i1d3_unlock(int fd, const uint32_t key[2]);

/**
 * @brief Attempt to automatically unlock the device using a predefined list of master keys
 *
 * Iterates through a list of known master keys and attempts to unlock the device.
 * This simplifies the unlock process for the user.
 *
 * @param fd File descriptor
 * @return I1D3_SUCCESS on success, error code if all keys fail
 */
i1d3_error_t i1d3_auto_find_unlock(int fd);

/**
 * @brief Perform an All-In-One color measurement
 *
 * Initiates a color measurement, retrieves the raw sensor data, and converts
 * it into XYZ, xy, CCT, and Lab color spaces. The results are stored in the
 * provided i1d3_color_results structure.
 *
 * @param fd File descriptor
 * @param res Pointer to an i1d3_color_results structure to store the measurement data
 * @return I1D3_SUCCESS on success, error code on failure
 */
i1d3_error_t i1d3_aio_measure(int fd, i1d3_color_results *res);

/**
 * @brief Get the current state of an i1d3 device
 *
 * @param fd File descriptor
 * @return Current device state
 */
i1d3_state_t i1d3_get_state(int fd);

/**
 * @brief Get a human-readable error message for an error code
 *
 * @param error The error code
 * @return Pointer to error message string
 */
const char* i1d3_error_string(i1d3_error_t error);

#endif // I1D3_API_H