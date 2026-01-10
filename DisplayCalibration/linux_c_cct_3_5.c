#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

// 구조체 정의: 색좌표 및 기기 상태 관리
typedef struct {
    double x, y, z;
} ColorValue;

typedef struct {
    double target_x, target_y;
    int current_gain[3]; // R, G, B
    int best_gain[3];
    double min_dist;
    double r_sens;
    double g_sens;
} Calibrator;

// ---------------------------------------------------------
// [TODO] 하드웨어 제어 및 센서 인터페이스 (직접 구현 필요)
// ---------------------------------------------------------

/**
 * i1Display3 센서로부터 실제 X, Y, Z 값을 읽어오는 함수
 */
ColorValue get_color_value(int r, int g, int b) {
    ColorValue cv;
    
    // 시뮬레이션 모델: 실제 센서 SDK 호출 코드로 대체하세요.
    cv.x = 0.25 + (r * 0.0004) + (g * 0.0001) + (b * 0.00005);
    cv.y = 0.23 + (r * 0.0001) + (g * 0.0005) + (b * 0.0001);
    cv.z = 100.0;
    
    // 미세한 노이즈 추가 (시뮬레이션용)
    cv.x += ((double)rand() / RAND_MAX - 0.5) * 0.0002;
    cv.y += ((double)rand() / RAND_MAX - 0.5) * 0.0002;
    
    return cv;
}

/**
 * TV의 Gain 값을 실제로 변경하는 함수 (Serial/I2C/Network 등)
 */
void set_tv_gain(int r, int g, int b) {
    // printf("[HW] Set Gain: R=%d, G=%d, B=%d\n", r, g, b);
    // 여기에 실제 통신 프로토콜을 구현하세요.
}

// ---------------------------------------------------------
// 핵심 알고리즘 로직
// ---------------------------------------------------------

/**
 * 패널의 실시간 감도(Sensitivity)를 측정
 */
void check_sens(Calibrator *c) {
    printf(">>> 감도 체크 시작 (실측 데이터 수집 중...)\n");
    
    ColorValue base_cv = get_color_value(c->current_gain[0], c->current_gain[1], c->current_gain[2]);
    int test_step = 15;
    
    // R 감도 측정
    ColorValue r_test_cv = get_color_value(c->current_gain[0] - test_step, c->current_gain[1], c->current_gain[2]);
    c->r_sens = fabs(r_test_cv.x - base_cv.x) / test_step;
    
    // G 감도 측정
    ColorValue g_test_cv = get_color_value(c->current_gain[0], c->current_gain[1] - test_step, c->current_gain[2]);
    c->g_sens = fabs(g_test_cv.y - base_cv.y) / test_step;
    
    printf("실측 감도 분석 완료: R_Sens=%.6f, G_Sens=%.6f\n\n", c->r_sens, c->g_sens);
}

int main() {
    // 1. 초기화
    Calibrator cal = {
        .target_x = 0.3127,
        .target_y = 0.3290,
        .current_gain = {192, 192, 192},
        .min_dist = 999.0,
        .r_sens = 0.0006, // 기본값
        .g_sens = 0.0005
    };

    printf("Starting TV Color Temperature Calibration (20 steps)...\n");

    // 2. 실제 패널 특성 파악
    check_sens(&cal);

    // 3. 메인 조정 루프 (20회)
    for (int step = 1; step <= 20; step++) {
        // 현재 상태 측정
        ColorValue cv = get_color_value(cal.current_gain[0], cal.current_gain[1], cal.current_gain[2]);
        
        double dx = cal.target_x - cv.x;
        double dy = cal.target_y - cv.y;
        double dist = sqrt(dx * dx + dy * dy);

        // 최적값 기록
        if (dist < cal.min_dist) {
            cal.min_dist = dist;
            for(int i=0; i<3; i++) cal.best_gain[i] = cal.current_gain[i];
        }

        printf("[%02d] R:%d G:%d B:%d | x:%.4f y:%.4f | Dist:%.5f\n", 
               step, cal.current_gain[0], cal.current_gain[1], cal.current_gain[2], cv.x, cv.y, dist);

        // 예측 제어 기반 Gain 계산
        double learning_rate = (dist > 0.005) ? 0.8 : 0.4;
        double adj_r = (dx / cal.r_sens) * learning_rate;
        double adj_g = (dy / cal.g_sens) * learning_rate;

        // 새로운 Gain 설정 (0~192 범위 제한)
        cal.current_gain[0] = (int)fmax(0, fmin(192, cal.current_gain[0] + adj_r));
        cal.current_gain[1] = (int)fmax(0, fmin(192, cal.current_gain[1] + adj_g));
        
        // Blue Gain 보조 로직
        if (dist > 0.01) {
            cal.current_gain[2] = (int)fmax(0, fmin(192, cal.current_gain[2] + (dx + dy) * 40));
        }

        // 실제 하드웨어에 적용
        set_tv_gain(cal.current_gain[0], cal.current_gain[1], cal.current_gain[2]);

        // 처리 속도 조절 (리눅스 usleep: 100ms)
        usleep(100000); 
    }

    // 4. 결과 출력 및 종료
    printf("\n--- Calibration Finished ---\n");
    printf("Best Gain Found: R=%d, G=%d, B=%d\n", cal.best_gain[0], cal.best_gain[1], cal.best_gain[2]);
    printf("Minimum Distance: %.6f\n", cal.min_dist);

    return 0;
}
