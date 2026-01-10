# TV Color Temperature Auto-Calibration Simulator (v3.5)

이 프로젝트는 TV 디스플레이의 색온도를 목표치(D65)에 맞추기 위해 R, G, B Gain을 자동으로 조정하는 알고리즘 시뮬레이터입니다. 감도 행렬(Jacobian) 기반의 예측 제어 로직을 사용하여 최소한의 측정 횟수로 최적의 색좌표를 찾아냅니다.

## 주요 기능
- **Jacobian 기반 예측 제어**: Gain 변화에 따른 색좌표 변화량을 계산하여 목표치로 빠르게 수렴.
- **Adaptive Step Size**: 타겟과의 거리에 따라 조정폭을 동적으로 변경하여 정밀도 향상.
- **물리적 제약 조건 준수**: Gain 값이 192(Bypass)를 초과하여 화질이 왜곡되는 현상 방지.
- **실시간 시각화**: CIE 1931 xy 차트상에 조정 경로를 실시간으로 표시.

## 설치 방법 (macOS / Python 3.12 기준)

### 1. 필수 라이브러리 설치
```bash
pip install matplotlib numpy
