import tkinter as tk
from tkinter import scrolledtext
import numpy as np
import sys
from datetime import datetime

# --- Mathematical Engines ---

def xyY_to_XYZ(x, y, Y):
    """
    Convert xyY color coordinates to XYZ tristimulus values.
    
    Args:
        x, y: Chromaticity coordinates
        Y: Luminance
    
    Returns:
        [X, Y, Z]: Tristimulus values
    """
    if y == 0 or Y < 0:
        return [0.0, 0.0, 0.0]
    X = (x * Y) / y
    Z = ((1.0 - x - y) * Y) / y
    return [X, Y, Z]

def validate_xyY(x, y, Y):
    """
    Validate xyY color coordinates.
    
    Args:
        x, y: Chromaticity coordinates
        Y: Luminance
    
    Returns:
        bool: True if valid
    """
    return 0 <= x <= 1 and 0 <= y <= 1 and x + y <= 1 and Y >= 0

def xyz_to_lab(X, Y, Z):
    """
    Convert XYZ to CIE Lab color space using D65 reference white.
    
    Args:
        X, Y, Z: Tristimulus values
    
    Returns:
        (L*, a*, b*): Lab coordinates
    """
    # D65 Reference White (Xn, Yn, Zn)
    Xn, Yn, Zn = 95.047, 100.0, 108.883
    
    def f(t):
        return t**(1/3) if t > 0.008856 else (7.787 * t) + (16/116)
    
    xr, yr, zr = X/Xn, Y/Yn, Z/Zn
    fx, fy, fz = f(xr), f(yr), f(zr)
    
    L = 116 * fy - 16
    a = 500 * (fx - fy)
    b = 200 * (fy - fz)
    
    return L, a, b

def delta_e_2000(XYZ1, XYZ2):
    """
    Calculate CIE Delta E 2000 color difference.
    
    This implements the full CIEDE2000 formula with parametric weighting
    factors, hue rotation terms, and compensation for lightness differences.
    
    Args:
        XYZ1, XYZ2: Arrays of [X, Y, Z] tristimulus values
    
    Returns:
        float: Delta E 2000 value
    """
    Lab1 = xyz_to_lab(*XYZ1)
    Lab2 = xyz_to_lab(*XYZ2)
    
    L1, a1, b1 = Lab1
    L2, a2, b2 = Lab2
    
    # Parametric weighting factors
    kL = 1.0  # Lightness
    kC = 1.0  # Chroma
    kH = 1.0  # Hue
    
    # Calculate C (chroma) and h (hue angle)
    C1 = np.sqrt(a1**2 + b1**2)
    C2 = np.sqrt(a2**2 + b2**2)
    
    C_avg = (C1 + C2) / 2
    
    if C1 * C2 == 0:
        h1 = 0
        h2 = 0
    else:
        h1 = np.degrees(np.arctan2(b1, a1)) % 360
        h2 = np.degrees(np.arctan2(b2, a2)) % 360
    
    h_avg = (h1 + h2) / 2
    
    # Hue difference
    if abs(h1 - h2) <= 180:
        dh = h2 - h1
    else:
        dh = h2 - h1 + 360 if h2 <= h1 else h2 - h1 - 360
    
    dH = 2 * np.sqrt(C1 * C2) * np.sin(np.radians(dh) / 2)
    
    # Lightness difference
    dL = L2 - L1
    
    # Chroma difference
    dC = C2 - C1
    
    # Calculate CIEDE2000
    L_avg = (L1 + L2) / 2
    C_avg7 = C_avg**7
    G = 0.5 * (1 - np.sqrt(C_avg7 / (C_avg7 + 25**7)))
    
    a1_prime = a1 * (1 + G)
    a2_prime = a2 * (1 + G)
    
    C1_prime = np.sqrt(a1_prime**2 + b1**2)
    C2_prime = np.sqrt(a2_prime**2 + b2**2)
    C_avg_prime = (C1_prime + C2_prime) / 2
    
    if C1_prime * C2_prime == 0:
        h1_prime = 0
        h2_prime = 0
    else:
        h1_prime = np.degrees(np.arctan2(b1, a1_prime)) % 360
        h2_prime = np.degrees(np.arctan2(b2, a2_prime)) % 360
    
    h_avg_prime = (h1_prime + h2_prime) / 2
    
    if abs(h1_prime - h2_prime) <= 180:
        dh_prime = h2_prime - h1_prime
    else:
        dh_prime = h2_prime - h1_prime + 360 if h2_prime <= h1_prime else h2_prime - h1_prime - 360
    
    dH_prime = 2 * np.sqrt(C1_prime * C2_prime) * np.sin(np.radians(dh_prime) / 2)
    
    # Weighting functions
    T = (1 - 0.17 * np.cos(np.radians(h_avg_prime - 30)) + 
         0.24 * np.cos(np.radians(2 * h_avg_prime)) +
         0.32 * np.cos(np.radians(3 * h_avg_prime + 6)) -
         0.20 * np.cos(np.radians(4 * h_avg_prime - 63)))
    
    SL = 1 + (0.015 * (L_avg - 50)**2) / np.sqrt(20 + (L_avg - 50)**2)
    SC = 1 + 0.045 * C_avg_prime
    SH = 1 + 0.015 * C_avg_prime * T
    
    RT = (-np.sin(np.radians(2 * 60 * np.exp(-((h_avg_prime - 275) / 25)**2)))) * (2 * np.sqrt(C_avg_prime**7 / (C_avg_prime**7 + 25**7)))
    
    # Final calculation
    term_L = dL / (kL * SL)
    term_C = dC / (kC * SC)
    term_H = dH_prime / (kH * SH)
    
    delta_E = np.sqrt(term_L**2 + term_C**2 + term_H**2 + RT * term_C * term_H)
    
    return delta_E

# --- GUI Logic Functions ---

def log_message(message):
    log_area.insert(tk.END, f"{message}\n")
    log_area.see(tk.END)

def calculate_fcmm():
    """
    Calculate Forward Color Matrix Model (FCMM) correction matrix.
    Validates input data and computes 3x3 correction matrix.
    """
    try:
        data = {}
        for dev in ['ref', 'sens']:
            cols = []
            for c in ['R', 'G', 'B']:
                x = float(entries[f"{dev}_{c}_x"].get())
                y = float(entries[f"{dev}_{c}_y"].get())
                Y = float(entries[f"{dev}_{c}_Y"].get())
                
                # Validate xyY values
                if not validate_xyY(x, y, Y):
                    log_message(f"[Error] Invalid xyY values for {dev} {c}: x={x}, y={y}, Y={Y}")
                    return
                
                cols.append(xyY_to_XYZ(x, y, Y))
            data[dev] = np.array(cols).T
        
        # Matrix M_corr = M_ref * inv(M_sens)
        m_corr = data['ref'] @ np.linalg.inv(data['sens'])
        
        log_message("--- 3x3 Sensor Correction Matrix ---")
        log_message(np.array2string(m_corr, separator=', ', precision=6))
        log_message("------------------------------------")
    except ValueError as e:
        log_message(f"[Error] Invalid input values: {e}")
    except np.linalg.LinAlgError as e:
        log_message(f"[Error] Matrix inversion failed: {e}")
    except Exception as e:
        log_message(f"[Error] Matrix Calculation Failed: {e}")

def calculate_de2000():
    """
    Calculate CIE Delta E 2000 color difference between two XYZ measurements.
    Includes input validation for XYZ tristimulus values.
    """
    try:
        s_xyz = [float(entries[f"de_sens_{i}"].get()) for i in ['X', 'Y', 'Z']]
        r_xyz = [float(entries[f"de_ref_{i}"].get()) for i in ['X', 'Y', 'Z']]
        
        # Basic validation for XYZ (should be non-negative)
        if any(x < 0 for x in s_xyz + r_xyz):
            log_message("[Error] XYZ values must be non-negative")
            return
        
        de_val = delta_e_2000(s_xyz, r_xyz)
        
        de_result_label.config(text=f"Delta E 2000 Result: {de_val:.4f}", fg="blue")
        log_message(f"CIE Delta E 2000 Calculated: {de_val:.4f}")
    except ValueError as e:
        log_message(f"[Error] Invalid XYZ input values: {e}")
    except Exception as e:
        log_message(f"[Error] Delta E Calculation Failed: {e}")

def exit_app():
    root.destroy()
    sys.exit(0)

# --- UI Layout Construction ---

root = tk.Tk()
root.title("Display Calibration Tool")
root.geometry("550x850")

# 0. Version Info (Top Left)
version_str = "1.1.0"  # Static version for reproducibility
tk.Label(root, text=f"SW Version: {version_str}", font=('Arial', 8)).pack(anchor="nw", padx=10)

entries = {}
btn_style = {"bg": "#90EE90", "fg": "black", "font": ('Arial', 10, 'bold'), "relief": "raised"}

# 1. FCMM Section
fcmm_frame = tk.LabelFrame(root, text=" 1. Sensor Matrix Generation (FCMM) ", padx=10, pady=10, font=('Arial', 10, 'bold'))
fcmm_frame.pack(pady=10, fill="x", padx=15)

dummy_rgb = {
    "R": ["0.640", "0.330", "21.26"],
    "G": ["0.300", "0.600", "71.52"],
    "B": ["0.150", "0.060", "7.22"]
}

def create_input_table(parent, title, prefix, row_offset):
    tk.Label(parent, text=title, fg="navy", font=('Arial', 9, 'italic')).grid(row=row_offset, column=0, columnspan=4, sticky="w", pady=5)
    for i, color in enumerate(['R', 'G', 'B']):
        tk.Label(parent, text=color).grid(row=row_offset+1+i, column=0)
        for j, axis in enumerate(['x', 'y', 'Y']):
            key = f"{prefix}_{color}_{axis}"
            entries[key] = tk.Entry(parent, width=10)
            entries[key].grid(row=row_offset+1+i, column=j+1, padx=3, pady=2)
            # Insert realistic dummy data with typical sensor variations
            val = float(dummy_rgb[color][j])
            if prefix == "sens":
                # Simulate realistic sensor measurement variations (±2-5%)
                variation = np.random.uniform(0.98, 1.05) if axis == 'Y' else np.random.uniform(0.995, 1.008)
                val *= variation
            entries[key].insert(0, f"{val:.3f}")

create_input_table(fcmm_frame, "Reference Data (CA-210)", "ref", 0)
create_input_table(fcmm_frame, "Target Sensor Data (i1Display3)", "sens", 4)

tk.Button(fcmm_frame, text="GENERATE 3X3 MATRIX", command=calculate_fcmm, **btn_style).grid(row=8, column=0, columnspan=4, pady=10, sticky="ew")

# 2. Delta E 2000 Section
de_frame = tk.LabelFrame(root, text=" 2. Error Analysis (Delta E 2000) ", padx=10, pady=10, font=('Arial', 10, 'bold'))
de_frame.pack(pady=10, fill="x", padx=15)

de_dummies = {"sens": ["92.5", "97.1", "104.2"], "ref": ["95.0", "100.0", "108.9"]}

for i, dev in enumerate(['sens', 'ref']):
    label_text = "i1Display3 (X,Y,Z):" if dev == 'sens' else "Reference (X,Y,Z):"
    tk.Label(de_frame, text=label_text).grid(row=i, column=0, sticky="w")
    for j, axis in enumerate(['X', 'Y', 'Z']):
        key = f"de_{dev}_{axis}"
        entries[key] = tk.Entry(de_frame, width=10)
        entries[key].grid(row=i, column=j+1, padx=3, pady=5)
        entries[key].insert(0, de_dummies[dev][j])

tk.Button(de_frame, text="CALCULATE DELTA E", command=calculate_de2000, **btn_style).grid(row=2, column=0, columnspan=4, pady=10, sticky="ew")
de_result_label = tk.Label(de_frame, text="Waiting for measurement...", font=('Arial', 10, 'italic'))
de_result_label.grid(row=3, column=0, columnspan=4)

# 3. Log Area Section
log_frame = tk.LabelFrame(root, text=" System Log ", padx=5, pady=5)
log_frame.pack(pady=5, fill="both", expand=True, padx=15)
log_area = scrolledtext.ScrolledText(log_frame, height=10, font=('Consolas', 9))
log_area.pack(fill="both", expand=True)

# 4. Exit Button
tk.Button(root, text="EXIT PROGRAM", command=exit_app, bg="#FF3B30", fg="white", font=('Arial', 10, 'bold'), height=2).pack(pady=10, fill="x", padx=15)

root.protocol("WM_DELETE_WINDOW", exit_app)
root.mainloop()
