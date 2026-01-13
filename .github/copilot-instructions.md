# AI Coding Assistant Instructions for hamji.github.io

> **Disclaimer**: This repository is an **engineering-side research and experimentation toolkit**. 
> It is **not production-grade firmware** and is not intended for end-customer delivery.

## 1. Project Purpose & Scope
- This project is for **internal development, experimentation, and technical demos**.
- Focus on **functional correctness** and **engineering utility** rather than code elegance or UI/UX design.
- The primary goal is to analyze, validate, and generate calibration parameters for external TV firmware.

## 2. Operating Environment (Critical)
AI must ensure code portability according to these targets:
- **TV-side Logic (C Language)**: Must be optimized for **Linux** environments.
- **Local PC Tools (Python/GUI)**: Must guarantee compatibility with **Windows** and **macOS**.
- **Experimental Interfaces**: Vanilla JavaScript for browser-based testing.

## 3. Mathematical Integrity & Precision
- **No Refactoring on Math**: Once a mathematical formula or matrix calculation (Jacobian, FCMM, etc.) is marked as "verified," **do not change the logic** even if it looks redundant.
- **Precision**: Always use `double` (C) or `float64` (Python) to avoid rounding errors in color space conversions (xyY, XYZ, Lab).
- **Edge Cases**: Always handle divide-by-zero (e.g., when Y=0 in xyY) or singular matrices during inversion.

## 4. GUI & UI Implementation Patterns
- **Simplicity First**: Implement only essential features for internal demos.
- **Tkinter/Vanilla JS**: Prefer standard libraries to minimize environment setup.
- **Engineering Feedback**: Use clear logging (ScrolledText) and timestamps for traceability instead of complex UI notifications.

## 5. Toolchain & Documentation Policy
- If the AI identifies a lack of explanation for a specific toolchain, build process, or environment setup:
  - **Task**: Automatically suggest or generate a separate `TOOLCHAIN_NAME.md` file to provide necessary documentation.
- Keep documentation technical, concise, and focused on the engineering workflow.

## 6. Key Components Reference
- `i1d3_sensor_calibration.py`: Host-side PC tool (Win/Mac) for sensor matrix and Delta E analysis.
- `TV_gamut_gamma_calibration.c`: Linux-compatible core logic for display correction.
- `i1d3_linux/`: C-based sensor communication (HID) specifically for Linux environments.

## 7. AI Assistant Command Summary
- **Do not assume production constraints**: Ignore memory-saving or real-time safety patterns unless requested.
- **Prefer explicit logic**: Clear, step-by-step math is better than highly abstracted helper layers.
- **Ask before structural changes**: If a suggestion involves changing the core calibration flow, verify with the user first.