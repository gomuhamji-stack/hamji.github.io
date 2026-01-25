# 📅 GoodHabit Tracker

**GoodHabit Tracker**는 로컬 PC에 데이터를 영구 저장하며 개인의 습관을 기록하고 시각화하는 가벼운 웹 애플리케이션입니다. 외부 클라우드 서버를 사용하지 않아 프라이버시가 보호되며, 임베디드 개발 환경과 유사한 독립적인 구조를 지향합니다.

## 🚀 주요 기능

- **실시간 습관 기록:** 매일 3가지 습관(성공/실패/미지정)과 한 줄 일기를 기록합니다.
- **자동 저장 (Auto-save):** 별도의 저장 버튼 없이 입력 즉시 로컬 SQLite DB에 반영됩니다.
- **시각적 대시보드:**
  - **Radar Chart (월간 밸런스):** 습관별 달성률과 지속성을 오각형 그래프로 시각화합니다.
  - **Color-coded Mini Calendar:** 습관 달성 개수에 따라 색상(회색/노랑/연두/진녹색)이 변하는 달력을 제공합니다.
- **월간 아카이브:** 사이드바를 통해 이전 달의 기록을 간편하게 조회할 수 있습니다.
- **월말 회고 (Monthly Wrap-up):** 매달 마지막 날 기록 시 총평 모달이 활성화되며, 저장된 총평은 아카이브 하단에 표시됩니다.
- **사용자 맞춤형:** 습관 이름을 자유롭게 수정할 수 있으며, 오늘 날짜 자동 포커스 및 하이라이트 기능을 지원합니다.
- **반응형 UI:** 브라우저 크기에 따라 레이아웃이 유연하게 조절됩니다 (Clean Dashboard 테마).

## 🛠 기술 스택

- **Backend:** Python 3, Flask
- **Database:** SQLite (파일 기반 로컬 DB)
- **Frontend:** Vanilla JS, CSS3, HTML5, [Chart.js](https://www.chartjs.org/)

## 📂 프로젝트 구조

```text
GoodHabit/
├── app.py              # Flask 서버 및 API 로직
├── habit_data.db       # SQLite 데이터베이스 (개인 기록 저장)
├── templates/          # HTML 템플릿
│   └── index.html      # UI 디자인 및 프론트엔드 로직
├── requirements.txt    # 필요 라이브러리 목록
└── .gitignore          # Git 추적 제외 설정 (DB 파일 등 포함)