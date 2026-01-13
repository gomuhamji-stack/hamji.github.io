# TV Gamut and Gamma Calibration

## Overview
This C program implements TV display calibration algorithms for gamut and gamma correction. It calculates correction matrices based on measured color primaries and gamma response to achieve accurate color reproduction according to BT.709 standards.

## Target Environment
- **Operating System**: Linux
- **Architecture**: Optimized for embedded Linux environments on TV systems
- **Language**: C (ANSI C compatible)

## Dependencies
- GCC compiler
- Math library (`libm`)

## Build Instructions
```bash
gcc -o tv_calibration TV_gamut_gamma_calibration.c -lm
```

## Usage
```bash
./tv_calibration
```

The program runs with sample test data and outputs calibration results to stdout.

## Code Structure

### Data Structures
- `GamutTable`: 3x3 matrix for color gamut correction
- `GammaTable`: 256-entry lookup table for gamma correction
- `Measurement`: Structure containing xyY color coordinates

### Core Functions

#### `invert_matrix_3x3(float m[3][3], float inv[3][3])`
Calculates the inverse of a 3x3 matrix using Cramer's rule.
- **Input**: Source matrix `m`
- **Output**: Inverse matrix `inv`
- **Return**: 1 on success, 0 if matrix is singular

#### `set_tv_gamut(Measurement measured[4])`
Computes gamut correction matrix from measured color primaries.
- **Input**: Array of 4 measurements (R, G, B primaries + white point)
- **Output**: GamutTable with correction matrix
- **Algorithm**: 
  - Constructs primaries matrix from xyY measurements
  - Calculates scaling factors using white point
  - Computes final correction matrix: M_final = M_target × inv(M_current)

#### `set_tv_gamma(Measurement steps[11])`
Generates gamma correction LUT from 11-point measurement data.
- **Input**: Array of 11 measurements at 10% intervals
- **Output**: GammaTable with 256-entry LUT
- **Algorithm**: Linear interpolation between measurement points targeting gamma 2.2

### Main Function
Contains sample measurement data for testing. In production, this would be replaced with actual sensor readings.

## Mathematical Background

### Gamut Correction
- Converts measured display primaries to BT.709 color space
- Uses XYZ color space transformations
- Handles white point scaling for accurate color temperature

### Gamma Correction
- Compensates for display's non-linear luminance response
- Targets standard gamma 2.2 curve
- Uses piecewise linear interpolation for LUT generation

## Example Output
```
Gamut Matrix [0][0]: 0.010000
Gamma LUT [128]: 127
```

## Integration Notes
- Designed for integration with color measurement sensors (e.g., i1d3)
- Matrices can be applied in TV firmware for real-time color correction
- LUT can be loaded into display processing pipeline

## Limitations
- Assumes BT.709 target color space
- Fixed gamma target of 2.2
- No error handling for invalid measurement data

## Future Enhancements
- Support for additional color spaces (DCI-P3, BT.2020)
- Dynamic gamma adjustment
- Integration with CIE color difference calculations</content>
<parameter name="filePath">/Users/leebongsu/hamji.github.io/TV_gamut_gamma_calibration.md