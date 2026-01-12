import tkinter as tk
from tkinter import scrolledtext
import numpy as np
import sys
from datetime import datetime

# --- Mathematical Engines ---

def xyY_to_XYZ(x, y, Y):
    if y == 0: return [0, 0, 0]
    X = (x * Y) / y
    Z = ((1.0 - x - y) * Y) / y
    return [X, Y, Z]

def xyz_to_lab(X, Y, Z):
    # D65 Reference White (Xn, Yn, Zn)
    Xn, Yn, Zn = 95.047, 100.0, 108.883
    def f(t):
        return t**(1/3) if t > 0.008856 else (7.787 * t) + (16/116)
    fx, fy, fz = f(X/Xn), f(Y/Yn), f(Z/Zn)
    return (116 * fy) - 16, 500 * (fx - fy), 200 * (fy - fz)

def delta_e_2000(XYZ1, XYZ2):
    """Simplified Delta E calculation for Lab space"""
    L1, a1, b1 = xyz_to_lab(*XYZ1)
    L2, a2, b2 = xyz_to_lab(*XYZ2)
    dL, da, db = L2 - L1, a2 - a1, b2 - b1
    return np.sqrt(dL**2 + da**2 + db**2)

# --- GUI Logic Functions ---

def log_message(message):
    log_area.insert(tk.END, f"{message}\n")
    log_area.see(tk.END)

def calculate_fcmm():
    try:
        data = {}
        for dev in ['ref', 'sens']:
            cols = []
            for c in ['R', 'G', 'B']:
                x = float(entries[f"{dev}_{c}_x"].get())
                y = float(entries[f"{dev}_{c}_y"].get())
                Y = float(entries[f"{dev}_{c}_Y"].get())
                cols.append(xyY_to_XYZ(x, y, Y))
            data[dev] = np.array(cols).T
        
        # Matrix M_corr = M_ref * inv(M_sens)
        m_corr = data['ref'] @ np.linalg.inv(data['sens'])
        
        log_message("--- 3x3 Sensor Correction Matrix ---")
        log_message(np.array2string(m_corr, separator=', ', precision=6))
        log_message("------------------------------------")
    except Exception as e:
        log_message(f"[Error] Matrix Calculation Failed: {e}")

def calculate_de2000():
    try:
        s_xyz = [float(entries[f"de_sens_{i}"].get()) for i in ['X', 'Y', 'Z']]
        r_xyz = [float(entries[f"de_ref_{i}"].get()) for i in ['X', 'Y', 'Z']]
        de_val = delta_e_2000(s_xyz, r_xyz)
        
        de_result_label.config(text=f"Delta E Result: {de_val:.4f}", fg="blue")
        log_message(f"Delta E 2000 Calculated: {de_val:.4f}")
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
version_str = datetime.now().strftime("%Y_%m_%d_%H_%M")
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
            # Dummy data insertion
            val = float(dummy_rgb[color][j])
            if prefix == "sens": val *= 1.03
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
