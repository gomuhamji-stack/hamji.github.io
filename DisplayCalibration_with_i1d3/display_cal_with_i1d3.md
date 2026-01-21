# DisplayCalibration_with_i1d3

## 프로젝트 개요

`DisplayCalibration_with_i1d3`는 X-Rite i1Display3 색상 센서와 디스플레이 캘리브레이션 알고리즘을 통합한 프로젝트입니다. 실제 센서 측정값을 기반으로 TV/디스플레이의 RGB Gain을 자동으로 조정하여 목표 색온도(D65: x=0.3127, y=0.3290)를 달성합니다.

## 프로젝트 구조

```
DisplayCalibration_with_i1d3/
├── Makefile                           # 빌드 설정 파일
├── display_cal_with_i1d3.md          # 이 문서
├── main.c                             # 메인 프로그램 및 디버그 메뉴
├── i1d3_api.h                         # i1d3 센서 API 헤더
├── i1d3_api.c                         # i1d3 센서 API 구현
├── display_calibration_api.h          # 디스플레이 캘리브레이션 API 헤더
├── display_calibration_api.c          # 디스플레이 캘리브레이션 API 구현
└── display_cal_with_i1d3              # 컴파일된 실행파일 (51KB)
```

## 주요 컴포넌트

### 1. i1d3_api (센서 제어)
**목적**: X-Rite i1Display3 색상 센서와의 HID 통신을 담당합니다.

**주요 함수**:
- `i1d3_open()`: 센서 장치 열기
- `i1d3_init_sequence()`: 초기화 시퀀스 실행
- `i1d3_auto_find_unlock()`: 11개의 마스터 키를 순회하여 센서 언락
- `i1d3_aio_measure()`: 색상 측정 (XYZ, xy, CCT, Lab)

**데이터 구조**:
```c
typedef struct {
    double X, Y, Z;    // CIE XYZ 색상 좌표
    double x, y;       // CIE xy 색도 좌표
    double CCT;        // 색온도 (Kelvin)
    double L, a, b;    // CIE Lab 색상 좌표
} i1d3_color_results;
```

### 2. display_calibration_api (캘리브레이션 알고리즘)
**목적**: 측정된 색상값을 기반으로 RGB Gain을 계산하고 조정합니다.

**핵심 알고리즘**:
- **감도 측정**: R 및 G 채널의 실시간 감도 측정
- **예측 제어**: 현재 색도와 목표 색도 사이의 거리를 계산
- **적응형 학습률**: 거리에 따라 조정 강도를 동적으로 변경
- **최적값 추적**: 반복 과정에서 최소 거리 달성시 해당 Gain 값 저장

**주요 함수**:
- `Calibrator_init()`: 캘리브레이션 상태 초기화
- `Calibrator_check_sensitivity()`: 디스플레이 감도 측정
- `Calibrator_perform_calibration_step()`: 한 번의 캘리브레이션 단계 실행
- `Calibrator_get_best_gain()`: 최적의 RGB Gain 값 반환

### 3. main.c (디버그 메뉴)
**목적**: 대화형 디버그 메뉴를 통해 각 기능을 개별적으로 테스트합니다.

**디버그 메뉴 옵션**:
```
1. Initialize Sensor       - 센서 초기화 및 언락
2. Read Sensor             - 단일 색상 측정
3. Change RGB Gain         - 수동 RGB Gain 조정
4. Calibrate RGB Gain      - 자동 캘리브레이션 프로세스
0. Exit                    - 프로그램 종료
```

## 빌드 및 실행

### 빌드
```bash
cd /Users/leebongsu/hamji.github.io/DisplayCalibration_with_i1d3
make                        # 빌드
make clean                  # 빌드 결과물 제거
```

### 실행

**디버그 모드** (메뉴 기반 대화형 테스트):
```bash
./display_cal_with_i1d3 -dbg
```

**일반 모드** (미구현):
```bash
./display_cal_with_i1d3
```

## 사용 시나리오

### 시나리오 1: 센서 확인
```
메뉴 1 > 센서 초기화 > 메뉴 2 > 색상값 확인
```
- 센서가 정상적으로 동작하는지 확인
- 현재 디스플레이 색상값 측정

### 시나리오 2: 수동 Gain 조정
```
메뉴 1 > 센서 초기화 > 메뉴 3 > Gain 입력 > 메뉴 2 > 결과 확인
```
- 특정 RGB Gain 값에서의 색상 결과 확인
- 수동 디버깅 및 테스트

### 시나리오 3: 자동 캘리브레이션
```
메뉴 1 > 센서 초기화 > 메뉴 4 > 반복 횟수 입력 > 자동 실행 > 최적 Gain 출력
```
- 목표 색도에 도달하기 위한 자동 조정
- 최적 RGB Gain 값 찾기

## 색상 측정 흐름

```
센서 읽음 (RGB 주파수)
    ↓
보정 행렬 적용 (MATRIX)
    ↓
XYZ 색상 좌표 계산
    ↓
xy 색도 계산
    ↓
CCT (색온도) 계산 (McCamy's formula)
    ↓
Lab 색상 계산 (D50 화이트 포인트)
    ↓
i1d3_color_results 반환
```

## 캘리브레이션 알고리즘 흐름

```
1. 감도 측정
   - R/G Gain을 변경하여 센서 응답 측정
   - 각 채널의 감도(Sensitivity) 계산

2. 반복 루프 (정해진 횟수만큼)
   a) 현재 색도 측정
   b) 목표 색도와의 거리 계산
   c) 최소 거리 기록
   d) 예측 제어로 새로운 Gain 계산
   e) TV에 새로운 Gain 적용
   f) 안정화 대기 (100ms)

3. 결과
   - 최적 Gain 값 출력
   - 최소 색도 거리 출력
```

## 주요 파라미터

### 색상 공간 참조값
```
D65 화이트 포인트 (i1d3_api.c):
- Xn = 95.047
- Yn = 100.0
- Zn = 108.883

목표 색도 (D65):
- x = 0.3127
- y = 0.3290
```

### 캘리브레이션 파라미터
```
RGB Gain 범위: 0 ~ 192
학습률:
  - 거리 > 0.005: 0.8
  - 거리 ≤ 0.005: 0.4
Blue Gain 보조: 거리 > 0.01일 때 활성화
```

### 센서 보정 행렬 (MATRIX)
```c
{0.035814, -0.021980, 0.016668},
{0.014015, 0.016946, 0.000451},
{-0.000407, 0.000830, 0.078830}
```

**주의**: 이 행렬은 i1d3_sensor_calibration.py를 사용하여 참조 센서(CA-210)와의 동시 측정으로 캘리브레이션되어야 합니다.

## 하드웨어 요구사항

### 필수 장비
- **i1Display3 센서**: USB로 연결, /dev/hidraw0에 마운트됨
- **Linux 시스템**: 커널 4.15 이상 (HIDRAW 지원)

### 권장 장비
- **참조 디스플레이** (CA-210 등): 센서 보정 행렬 생성 시 필요

### 권한 설정
```bash
# udev 규칙으로 영구 설정 (권장)
sudo vi /etc/udev/rules.d/99-i1d3.rules

# 다음 내용 추가:
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0765", ATTRS{idProduct}=="5020", MODE="0666"

# 규칙 적용
sudo udevadm control --reload-rules
sudo udevadm trigger
```

또는

```bash
# 임시 권한 설정 (한 번만)
sudo chmod 666 /dev/hidraw0
```

## 컴파일 설정

### Makefile
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = -lm

SRCS = main.c i1d3_api.c display_calibration_api.c
OBJS = $(SRCS:.c=.o)
TARGET = display_cal_with_i1d3
```

### 컴파일러 요구사항
- GCC 7.0 이상
- 표준 C99 이상

## 향후 개선 계획

### Phase 1: 안정성 강화
- [ ] 더 강력한 에러 처리
- [ ] 장치 재연결 로직
- [ ] 구간별 타임아웃 처리

### Phase 2: 성능 최적화
- [ ] 측정 시간 단축
- [ ] 캘리브레이션 수렴 속도 개선
- [ ] 메모리 최적화

### Phase 3: 기능 확장
- [ ] 다중 센서 지원
- [ ] 배치 측정 모드
- [ ] 실시간 스트리밍
- [ ] 설정 파일 지원

### Phase 4: 인터페이스 개선
- [ ] Python 바인딩
- [ ] 웹 기반 제어 인터페이스
- [ ] GUI 개발

## 문제 해결

### 센서 감지 안 됨
```bash
# 1. USB 연결 확인
lsusb | grep "X-Rite"

# 2. HID 장치 확인
ls -la /dev/hidraw*

# 3. 권한 설정
sudo chmod 666 /dev/hidraw0
```

### 언락 실패
- 센서가 이미 언락 상태인지 확인
- USB 포트 변경 후 재시도
- 센서 전원 재부팅

### 측정값이 이상함
- 센서 보정 행렬 확인 (i1d3_api.c의 MATRIX)
- 참조 센서(CA-210)로 재검증
- i1d3_sensor_calibration.py로 재캘리브레이션

## 라이선스 및 참고

- **i1d3 마스터 키**: Argyll CMS 프로젝트에서 제공
- **색상 과학**: CIE XYZ, Lab, CCT 표준 준수
- **하드웨어**: X-Rite i1Display3 공식 문서 참고

## 개발자 노트

### 코드 구조
- **i1d3_api**: 순수 C 구현, 외부 의존성 없음
- **display_calibration_api**: 센서 데이터 기반 실제 알고리즘
- **main.c**: 디버그 및 테스트 인터페이스

### 수정 후 주의사항
- 색상 계산 로직 수정 시 수학적 정확성 검증 필수
- 센서 통신 프로토콜 변경 시 i1d3_api.c 주석 참고
- 새로운 기능 추가 시 모듈 독립성 유지

---

**최종 수정**: 2026년 1월 21일  
**버전**: 1.0
