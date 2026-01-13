# i1d3_linux Toolchain Setup

## Overview
The `i1d3_linux/` component provides direct HID (Human Interface Device) communication with X-Rite i1Display3 color sensors on Linux systems. This is a pure C implementation with no external dependencies beyond standard system libraries.

## System Requirements

### Operating System
- **Linux** (Ubuntu 18.04+, CentOS 7+, or equivalent)
- **Kernel**: 4.15+ (for HIDRAW support)
- **Architecture**: x86_64 or ARM64

### Hardware Requirements
- X-Rite i1Display3 color sensor connected via USB
- Device appears as `/dev/hidraw*` (typically `/dev/hidraw0`)

## Build Environment Setup

### 1. Compiler Installation
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential gcc

# CentOS/RHEL/Fedora
sudo yum groupinstall "Development Tools"
# or
sudo dnf groupinstall "Development Tools"
```

### 2. Required System Headers
The code uses only standard POSIX headers:
- `<stdio.h>`, `<stdlib.h>`, `<string.h>` - Standard I/O and memory
- `<fcntl.h>`, `<unistd.h>` - File and system operations
- `<math.h>` - Mathematical functions

These are included with the base `build-essential` package.

## Compilation Instructions

### Simple Compilation
```bash
cd i1d3_linux/
gcc -o i1d3_test main.c 11d3.c -lm
```

### Optimized Build
```bash
gcc -O2 -Wall -Wextra -o i1d3_test main.c 11d3.c -lm
```

### Debug Build
```bash
gcc -g -O0 -Wall -Wextra -o i1d3_test main.c 11d3.c -lm
```

## Runtime Dependencies

### 1. Device Permissions
The application requires access to HID devices. Run with appropriate permissions:

```bash
# Option 1: Run as root (not recommended for production)
sudo ./i1d3_test

# Option 2: Add user to dialout group (recommended)
sudo usermod -a -G dialout $USER
# Logout and login again, then:
./i1d3_test

# Option 3: Use udev rules for persistent permissions
# Create /etc/udev/rules.d/99-i1d3.rules:
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0765", ATTRS{idProduct}=="5020", MODE="0666"
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 2. Device Detection
Verify device connection:
```bash
# List HID devices
ls /dev/hidraw*

# Check device details
udevadm info -a -n /dev/hidraw0 | grep -E "(idVendor|idProduct|manufacturer|product)"
```

Expected output for i1Display3:
```
ATTRS{idVendor}=="0765"
ATTRS{idProduct}=="5020"
ATTRS{manufacturer}=="X-Rite"
ATTRS{product}=="i1Display3"
```

## Build Verification

### Test Compilation
```bash
# Compile without linking to check syntax
gcc -c main.c 11d3.c -lm

# Check for warnings
gcc -Wall -Wextra -o i1d3_test main.c 11d3.c -lm
```

### Runtime Testing
```bash
# Run basic connectivity test
./i1d3_test

# Expected output:
[SYS] Initializing sequence...
[SYS] Starting Auto-Unlock...
[SUCCESS] Instrument unlocked using Retail keys.
[SYS] Taking measurements (3 times)...
[1] XYZ: 92.50, 97.10, 104.20 | xy: 0.3127, 0.3290 | CCT: 6504K | Lab: 95.0, 1.0, -1.0
```

## Troubleshooting

### Common Build Issues
1. **Missing math library**: Add `-lm` flag for `pow()`, `sqrt()` functions
2. **Permission denied**: Use `sudo` or configure udev rules
3. **Device not found**: Check USB connection and `/dev/hidraw*` devices

### Runtime Issues
1. **Unlock failed**: Try different USB port or power cycle device
2. **Invalid measurements**: Ensure proper device calibration
3. **Permission errors**: Verify user is in `dialout` group or use sudo

## Development Notes

### Code Structure
- `main.c`: Application entry point and measurement loop
- `11d3.c`: Core HID communication and sensor protocols
- `i1d3.h`: Public API definitions and data structures

### Key Functions
- `i1d3_open()`: Device initialization with permission setup
- `i1d3_init_sequence()`: 8-command initialization protocol
- `i1d3_auto_find_unlock()`: Brute-force unlock with 11 master keys
- `i1d3_aio_measure()`: RGB frequency measurement and XYZ/Lab conversion

### Security Considerations
- Uses `system("sudo chmod 666")` for device access (development only)
- Master unlock keys are embedded (from Argyll CMS project)
- No network communication or external dependencies