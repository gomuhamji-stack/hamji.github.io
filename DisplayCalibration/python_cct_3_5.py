

# 필요 모듈 설치 가이드:
# pip install matplotlib numpy
# macOS 사용 시 Tkinter 이슈 발생 시: brew install python-tk@3.12


import tkinter as tk
import matplotlib
matplotlib.use('TkAgg')
from tkinter import scrolledtext
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
import sys

class OptimizedTVAdjuster:
    def __init__(self, target_xy):
        self.target_x, self.target_y = target_xy
        self.history = []
        self.current_gain = [192, 192, 192] 
        self.best_gain = (192, 192, 192)
        self.min_dist = float('inf')
        self.best_idx = 0

    def get_color_value(self):
        """실제 물리 엔진에 가까운 시뮬레이션 (Gain과 좌표의 비례 관계 반영)"""
        r, g, b = self.current_gain
        # 실제 패널은 Gain에 따라 선형/비선형적으로 좌표가 이동함
        # 아래 수식은 R, G, B가 각각 x, y에 주는 영향력(Sensitivity)을 모사함
        x = 0.25 + (r * 0.0004) + (g * 0.0001) + (b * 0.00005)
        y = 0.23 + (r * 0.0001) + (g * 0.0005) + (b * 0.0001)
        return [x + np.random.normal(0, 0.0001), y + np.random.normal(0, 0.0001), 100.0]

class CalibrationUI:
    def __init__(self, root):
        self.root = root
        self.root.title("최적 예측 제어 캘리브레이터 (Jacobian)")
        self.target_xy = (0.3127, 0.3290)
        self.adj = OptimizedTVAdjuster(self.target_xy)
        self.current_step = 0
        self.max_steps = 20
        self.is_running = False

        # GUI 구성 (생략 - 이전과 동일)
        self.fig, self.ax = plt.subplots(figsize=(5, 4))
        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.log_widget = scrolledtext.ScrolledText(root, height=10)
        self.log_widget.pack(fill=tk.X)
        
        self.btn_start = tk.Button(root, text="최적화 프로세스 시작", command=self.start_calibration)
        self.btn_start.pack(pady=5)
        self.btn_exit = tk.Button(root, text="종료", command=sys.exit)
        self.btn_exit.pack(pady=5)

    def start_calibration(self):
        self.is_running = True
        self.run_step()

    def run_step(self):
        if self.current_step >= self.max_steps or not self.is_running:
            return

        r, g, b = self.adj.current_gain
        x, y, z = self.adj.get_color_value()
        self.adj.history.append([x, y, z])
        
        dx = self.target_xy[0] - x
        dy = self.target_xy[1] - y
        dist = np.sqrt(dx**2 + dy**2)

        if dist < self.adj.min_dist:
            self.adj.min_dist, self.adj.best_gain, self.adj.best_idx = dist, (r, g, b), self.current_step

        # 로그 출력
        log_msg = f"[{self.current_step+1:02d}] R:{int(r)} G:{int(g)} B:{int(b)} | x:{x:.4f} y:{y:.4f} | Dist:{dist:.5f}\n"
        self.log_widget.insert(tk.END, log_msg); self.log_widget.see(tk.END)

        # --- 최적 알고리즘: 비례 제어 + 예측 가중치 ---
        # Gain 변화량 당 좌표 변화 예상치 (Sensitivity Table)
        # 패널마다 다르지만 보통 R은 x에, G는 y에 큰 영향을 줌
        r_sens, g_sens, b_sens = 0.0006, 0.0005, 0.0004 
        
        # 오차에 감도를 나누어 필요한 Gain 변화량을 계산 (P-Control 기반)
        # 192를 넘지 않게 하면서 오차의 80%씩 빠르게 접근 (Over-shooting 방지)
        adj_r = (dx / r_sens) * 0.8
        adj_g = (dy / g_sens) * 0.8
        
        new_r = np.clip(r + adj_r, 0, 192)
        new_g = np.clip(g + adj_g, 0, 192)
        
        # B는 x, y를 동시에 고려하여 보조적으로 사용
        # x, y 오차가 둘다 남았을 때 휘도를 위해 조절
        new_b = b # 기본 유지
        if dist > 0.01:
            new_b = np.clip(b + (dx+dy)*50, 0, 192)

        self.adj.current_gain = [new_r, new_g, new_b]
        
        self.update_plot()
        self.current_step += 1
        self.root.after(500, self.run_step) # 수렴이 빠르므로 간격 단축

    def update_plot(self):
        self.ax.clear()
        self.ax.set_xlim(0.28, 0.38); self.ax.set_ylim(0.28, 0.38)
        self.ax.plot(self.target_xy[0], self.target_xy[1], 'rx')
        if self.adj.history:
            h = np.array(self.adj.history)
            self.ax.plot(h[:, 0], h[:, 1], 'g-', alpha=0.5)
            self.ax.scatter(h[-1, 0], h[-1, 1], c='blue')
        self.canvas.draw_idle()

if __name__ == "__main__":
    root = tk.Tk(); app = CalibrationUI(root); root.mainloop()
