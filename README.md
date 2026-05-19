
<img width="2121" height="993" alt="samples" src="https://github.com/user-attachments/assets/0dc69fee-8c00-48b4-a7f2-804aeaa197b4" />

* **[Official] F1 25 Data Output spec**
*(https://forums.ea.com/t5/s/tghpe58374/attachments/tghpe58374/f1-games-game-info-hub-en/61/4/Data%20Output%20from%20F1%2025%20v3.pdf)*

## ⬛ 프로젝트 디렉토리 구조 (Directory Structure)

- **main.h**: F1 25 공식 텔레메트리 패킷 명세 규격에 맞춘 바이트 정렬 구조체 선언부 일체 포함.
- **main.cpp**: Winsock2 네트워크 수신기, 스레드 안전 큐 인프라, 데이터 파싱 및 실시간 콘솔 뷰어 구현부.

## ⬛ 빌드 및 실행 요구 사양 (Requirements)

### 필수 환경
- **운영체제**: Windows 10 / 11
- **개발 환경**: Visual Studio 2022 (버전 17.0 이상 권장)
- **C++ 언어 표준**: ISO C++20 표준 (`/std:c++20`)
- **네트워크 종속성**: Windows 소켓 API (`ws2_32.lib`)

### 빌드 절차
1. 본 저장소를 로컬 환경으로 클론합니다:
   ```bash
   git clone [https://github.com/GH-saltyH/f125-telemetry-cpp.git](https://github.com/GH-saltyH/f125-telemetry-cpp.git)
2. Visual Studio 2022를 실행하고 새 프로젝트 만들기 -> 비어 있는 C++ 프로젝트를 생성합니다.

3. 프로젝트 구성에 main.h 파일과 main.cpp 파일을 추가합니다.

4. 프로젝트 구성 속성을 설정합니다


   * 프로젝트 우클릭 -> 속성 -> 구성 속성 -> 일반 -> C++ 언어 표준 항목을 ISO C++20 표준 (/std:c++20)으로 변경합니다.

   * 상단 빌드 대상을 x64 플랫폼으로 지정합니다.

5. Ctrl + F5를 눌러 컴파일 및 실행을 진행합니다.

## ⬛ 향후 개발 로드맵 (Development Roadmap)
본 프로토타입 백엔드 파이프라인을 기반으로 향후 다음과 같은 전문 분석 컴포넌트 확장을 예정하고 있습니다.

● **1단계**: 고속 고집적 데이터 덤프 시스템 엔진

  * 기존 텍스트 기반의 CSV 파일 출력을 버퍼 최적화형 바이너리 파일 구조(.bin)로 업그레이드하여 디스크 쓰기 병목 제로화.

  * 세션 고유 ID(m_sessionUID)를 감지하여 덮어쓰기 없이 세션별 파일 분할 자동 기록 기능 구현.

● **2단계**: 그래픽 대시보드 UI 및 인포그래픽 레이어 연동

  * Dear ImGui 또는 Qt GUI 레이어를 통합하여 실제 레이싱 엔지니어링 환경을 모방한 실시간 선형 롤링 그래프 구축.

  * 종/횡 가속도 G-Force 차트, 스티어링 각도, 스로틀 및 브레이크 트레이스 데이터 시각화 패널 구현.

● **3단계**: 실시간 데이터 검증 및 분석 검사 엔진 (Rules Engine)

  * 인게임 타이어 압력(F: 22.5~29.5 psi / R: 20.5~26.5 psi) 및 서스펜션 작동 한계 규격 실시간 모니터링 모듈 추가.

  * 셋업 최적 범위를 벗어나거나 코너링 중 차량 하부 바닥이 노면에 닿는 현상(Bottoming out) 발생 시 실시간 디버그 경고 및 오버레이 알림 제공.

## ⬛ 작동 유효성 검증 및 진단 (Diagnostics)
레이싱 세션 구동 시 콘솔 창을 통해 데이터 가공 파이프라인의 실시간 유효성을 즉시 검증할 수 있습니다.

● **Data Frequency**: 특정 Packet ID 필터링 처리가 정상 작동하는지 계측합니다. 게임 내 전송 설정율과 정확히 동기화된 주파수(최대 60Hz)를 유지하면서도, 처리되지 못하고 밀려 있는 잔여 큐 크기가 항상 0 근처를 마크하는지 검증합니다.

● **Telemetry Preview**: 바이너리 캐스팅 정밀도를 체크하기 위한 체크포인트를 실시간 노출합니다:

  * 프레임 시퀀스 누락 여부 (Frame ID)

  * 부동소수점 데이터 왜곡 여부 (Session Time이 정상 초 단위 float로 유지되는지)

  * 네 바퀴 코너별 배열 인덱스 매핑 무결성 (PSI 타이어 압력 데이터 전수 검증)

## ⬛ 라이선스 (License)
본 프로젝트는 MIT 라이선스에 따라 자유롭게 수정 및 배포할 수 있습니다.

