/* ver:2026_01_13__10_00 - Enhanced with error handling and comprehensive documentation */
#ifndef I1D3_H
#define I1D3_H

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
 * @brief Automatically find and apply the correct unlock key
 *
 * Tries all known manufacturer keys in sequence until one works.
 * This is the recommended way to unlock devices.
 *
 * @param fd File descriptor
 * @return I1D3_SUCCESS on success, error code on failure
 */
i1d3_error_t i1d3_auto_find_unlock(int fd);

/**
 * @brief Perform a color measurement
 *
 * Takes a color measurement using the device's sensor and returns results
 * in XYZ, xy, CCT, and Lab color spaces. The device must be unlocked first.
 *
 * @param fd File descriptor
 * @param res Pointer to structure to store measurement results
 * @return I1D3_SUCCESS on success, error code on failure
 */
i1d3_error_t i1d3_aio_measure(int fd, i1d3_color_results *res);

/**
 * @brief Get the current state of the device
 *
 * @param fd File descriptor
 * @return Current device state
 */
i1d3_state_t i1d3_get_state(int fd);

/**
 * @brief Convert error code to human-readable string
 *
 * @param error Error code
 * @return String description of the error
 */
const char* i1d3_error_string(i1d3_error_t error);

#endif
