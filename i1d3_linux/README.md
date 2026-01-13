# i1d3_linux - X-Rite i1Display3 Linux Driver

A pure C library for communicating with X-Rite i1Display3 color sensors on Linux systems.

## Features

- **Pure C Implementation**: No external dependencies beyond standard POSIX libraries
- **HID Communication**: Direct access to HIDRAW devices for low-latency operation
- **Color Science**: Full color space conversions (XYZ, xy, CCT, Lab)
- **Error Handling**: Comprehensive error reporting and state management
- **Auto-Unlock**: Automatic detection and application of manufacturer unlock keys

## Quick Start

### Build
```bash
cd i1d3_linux/
make
```

### Usage
```c
#include "i1d3.h"

int main(void) {
    // Open device
    int fd = i1d3_open("/dev/hidraw0");
    if (fd < 0) {
        printf("Failed to open device: %s\n", i1d3_error_string(fd));
        return 1;
    }

    // Initialize and unlock
    i1d3_init_sequence(fd);
    i1d3_auto_find_unlock(fd);

    // Take measurement
    i1d3_color_results result;
    if (i1d3_aio_measure(fd, &result) == I1D3_SUCCESS) {
        printf("XYZ: %.2f, %.2f, %.2f\n", result.X, result.Y, result.Z);
        printf("CCT: %.0fK\n", result.CCT);
    }

    i1d3_close(fd);
    return 0;
}
```

## API Reference

### Device Management

#### `int i1d3_open(const char *path)`
Opens a connection to the i1Display3 device at the specified HIDRAW path.

**Parameters:**
- `path`: Device path (e.g., "/dev/hidraw0")

**Returns:** File descriptor on success, negative error code on failure

#### `int i1d3_close(int fd)`
Closes the device connection.

**Parameters:**
- `fd`: File descriptor from `i1d3_open()`

**Returns:** 0 on success, negative error code on failure

### Device Control

#### `i1d3_error_t i1d3_init_sequence(int fd)`
Initializes the device with the required command sequence.

**Parameters:**
- `fd`: File descriptor

**Returns:** `I1D3_SUCCESS` on success, error code on failure

#### `i1d3_error_t i1d3_auto_find_unlock(int fd)`
Automatically finds and applies the correct manufacturer unlock key.

**Parameters:**
- `fd`: File descriptor

**Returns:** `I1D3_SUCCESS` on success, error code on failure

### Measurements

#### `i1d3_error_t i1d3_aio_measure(int fd, i1d3_color_results *res)`
Performs a color measurement and returns results in multiple color spaces.

**Parameters:**
- `fd`: File descriptor
- `res`: Pointer to result structure

**Returns:** `I1D3_SUCCESS` on success, error code on failure

**Result Structure:**
```c
typedef struct {
    double X, Y, Z;    // CIE XYZ coordinates
    double x, y;       // CIE xy chromaticity
    double CCT;        // Correlated Color Temperature (K)
    double L, a, b;    // CIE Lab coordinates
} i1d3_color_results;
```

### Error Handling

#### `const char* i1d3_error_string(i1d3_error_t error)`
Converts an error code to a human-readable string.

#### `i1d3_state_t i1d3_get_state(int fd)`
Returns the current state of the device connection.

## Supported Manufacturers

The library includes unlock keys for:
- Retail
- Munki
- OEM
- NEC
- Quato
- HP
- Wacom
- TPA
- Barco
- Crysta
- Viewsonic

## System Requirements

- **Linux Kernel**: 4.15+ (HIDRAW support)
- **Compiler**: GCC 7+ or compatible C99 compiler
- **Permissions**: Access to `/dev/hidraw*` devices

## Troubleshooting

### Permission Denied
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Or set permissions manually
sudo chmod 666 /dev/hidraw0
```

### Device Not Found
- Ensure the i1Display3 is connected
- Check `ls /dev/hidraw*` to find the correct device
- Verify the device is not claimed by another driver

### Unlock Failed
- Confirm the device is an authentic i1Display3
- Some custom firmware may require different keys
- Check device connection stability

## Color Science

### Color Spaces
- **XYZ**: CIE 1931 color coordinates
- **xy**: Chromaticity coordinates (x = X/(X+Y+Z), y = Y/(X+Y+Z))
- **CCT**: Calculated using McCamy's approximation
- **Lab**: CIE Lab with D50 white point

### Measurement Process
1. Send measurement command with 200ms integration time
2. Receive raw sensor data (RGB counts and clock values)
3. Convert to frequency domain using calibration matrix
4. Transform to XYZ color space
5. Calculate derived color coordinates

## Version History

- **v1.0.0**: Initial release with error handling and state management
- Comprehensive API documentation
- Build system improvements

## License

This software is provided for engineering and research purposes. See project license for details.