#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 데이터 구조체 정의
typedef struct { float matrix[3][3]; } GamutTable;
typedef struct { int entries[256]; } GammaTable;
typedef struct { double x, y, Y; } Measurement;

// ---------------------------------------------------------
// 보조 함수: 3x3 행렬 역산 (Cramer's Rule)
// ---------------------------------------------------------
int invert_matrix_3x3(float m[3][3], float inv[3][3]) {
    float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    if (fabs(det) < 1e-12) return 0; // 역행렬 존재 불가

    float invDet = 1.0f / det;
    inv[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
    inv[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invDet;
    inv[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;
    inv[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
    inv[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
    inv[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invDet;
    inv[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
    inv[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invDet;
    inv[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDet;
    return 1;
}

// ---------------------------------------------------------
// 1. Gamut 교정 함수: White Point Scaling 포함
// ---------------------------------------------------------
GamutTable set_tv_gamut(Measurement measured[4]) {
    GamutTable result = {0};
    float M_primaries[3][3], M_inv[3][3], S[3], M_curr[3][3];
    
    // [BT.709 Target Matrix 정의]
    float M_target[3][3] = {
        {0.4124f, 0.3576f, 0.1805f},
        {0.2126f, 0.7152f, 0.0722f},
        {0.0193f, 0.1192f, 0.9505f}
    };

    // (1) R, G, B 측정값으로 원색 방향 행렬 구성 (Y=1 정규화 방향)
    for (int i = 0; i < 3; i++) {
        M_primaries[0][i] = (float)(measured[i].x / measured[i].y);
        M_primaries[1][i] = 1.0f;
        M_primaries[2][i] = (float)((1.0 - measured[i].x - measured[i].y) / measured[i].y);
    }

    // (2) White 측정값을 XYZ로 변환
    float W_XYZ[3];
    W_XYZ[0] = (float)((measured[3].x * measured[3].Y) / measured[3].y);
    W_XYZ[1] = (float)measured[3].Y;
    W_XYZ[2] = (float)((1.0 - measured[3].x - measured[3].y) * measured[3].Y / measured[3].y);

    // (3) Scaling Factors S = inv(M_primaries) * W_XYZ 계산
    if (invert_matrix_3x3(M_primaries, M_inv)) {
        for (int i = 0; i < 3; i++) {
            S[i] = 0;
            for (int j = 0; j < 3; j++) S[i] += M_inv[i][j] * W_XYZ[j];
        }

        // (4) 최종 현재 패널 행렬 M_curr 구성
        for (int j = 0; j < 3; j++) {
            M_curr[0][j] = M_primaries[0][j] * S[j];
            M_curr[1][j] = M_primaries[1][j] * S[j];
            M_curr[2][j] = M_primaries[2][j] * S[j];
        }

        // (5) 교정 행렬 산출: M_final = M_target * inv(M_curr)
        float M_curr_inv[3][3];
        if (invert_matrix_3x3(M_curr, M_curr_inv)) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    for (int k = 0; k < 3; k++)
                        result.matrix[i][j] += M_target[i][k] * M_curr_inv[k][j];
                }
            }
        }
    }
    return result;
}

// ---------------------------------------------------------
// 2. Gamma 교정 함수: 11점 측정 기반 256 LUT 생성
// ---------------------------------------------------------
GammaTable set_tv_gamma(Measurement steps[11]) {
    GammaTable lut;
    double target_gamma = 2.2;
    double L_max = steps[10].Y; // 100% White 휘도

    for (int i = 0; i < 256; i++) {
        // (1) 해당 계조의 목표 휘도 계산
        double target_Y = pow((double)i / 255.0, target_gamma) * L_max;

        // (2) 측정 데이터 구간 찾기
        int seg = 0;
        for (int j = 0; j < 10; j++) {
            if (target_Y >= steps[j].Y && target_Y <= steps[j+1].Y) {
                seg = j;
                break;
            }
        }

        // (3) 선형 보간 (Linear Interpolation)
        // x축: 10% 단위 index (0, 25.5, 51 ... 255)
        double x0 = seg * 25.5;
        double x1 = (seg + 1) * 25.5;
        double y0 = steps[seg].Y;
        double y1 = steps[seg + 1].Y;

        double corrected_val;
        if (fabs(y1 - y0) < 1e-9) {
            corrected_val = x0;
        } else {
            corrected_val = x0 + (target_Y - y0) * (x1 - x0) / (y1 - y0);
        }

        // (4) 범위 제한 및 저장
        lut.entries[i] = (int)fmax(0, fmin(255, corrected_val));
    }
    return lut;
}

// ---------------------------------------------------------
// 메인 테스트 함수
// ---------------------------------------------------------
int main() {
    // 예시 데이터: 실제로는 센서 측정값이 들어감
    Measurement gamut_meas[4] = {
        {0.640, 0.330, 21.26}, // Red
        {0.300, 0.600, 71.52}, // Green
        {0.150, 0.060, 7.22},  // Blue
        {0.3127, 0.3290, 100.0} // White
    };

    Measurement gamma_meas[11];
    for(int i=0; i<=10; i++) {
        gamma_meas[i].x = 0.3127; gamma_meas[i].y = 0.3290;
        gamma_meas[i].Y = pow(i/10.0, 2.2) * 100.0; // 이미 완벽한 2.2 가정
    }

    // 함수 실행
    GamutTable gmt = set_tv_gamut(gamut_meas);
    GammaTable gma = set_tv_gamma(gamma_meas);

    // 결과 확인 (샘플)
    printf("Gamut Matrix [0][0]: %f\n", gmt.matrix[0][0]);
    printf("Gamma LUT [128]: %d\n", gma.entries[128]);

    return 0;
}
