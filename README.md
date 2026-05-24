## F1 25 Telemetry Manager
<img height="354" alt="스크린샷 2026-05-24 152907" src="https://github.com/user-attachments/assets/b97f16b7-d5ee-4fb6-8616-41021c94286d" />
<img width="698" alt="스크린샷 2026-05-24 152902" src="https://github.com/user-attachments/assets/e56676f8-e0d9-49f1-b401-3cb57669f552" />



* **[Official] F1 25 Data Output spec**
*(https://forums.ea.com/t5/s/tghpe58374/attachments/tghpe58374/f1-games-game-info-hub-en/61/4/Data%20Output%20from%20F1%2025%20v3.pdf)*

## ⬛ 프로젝트 디렉토리 구조 (Directory Structure)

- **main.h**: F1 25 공식 텔레메트리 패킷 명세 규격에 맞춘 바이트 정렬 구조체 선언부 일체 포함.
- **main.cpp**: Winsock2 네트워크 수신기, 스레드 안전 큐 인프라, 데이터 파싱 및 실시간 콘솔 뷰어 구현부.
- **WinManager.h / .cpp**: ImGui Docking 및 멀티 뷰포트 기반 윈도우/서브앱 관리 시스템.
- **SubApps/**: 팩토리 패턴 기반의 확장 가능한 인포그래픽 윈도우 구현체 (예: `PlayerInputGraphView`).

## ⬛ 빌드 및 실행 요구 사양 (Requirements)

### 필수 환경
- **운영체제**: Windows 10 / 11
- **개발 환경**: Visual Studio 2022 (버전 17.0 이상 권장)
- **C++ 언어 표준**: ISO C++20 표준 (`/std:c++20`)
- **네트워크 종속성**: Windows 소켓 API (`ws2_32.lib`)
- **라이브러리**: Dear ImGui (Docking Branch), ImPlot, DirectX 11

### 빌드 절차
1. 본 저장소를 로컬 환경으로 클론합니다:
   ```bash
   git clone https://github.com/GH-saltyH/f125-telemetry-cpp.git
2. Visual Studio 2022를 실행하고 새 프로젝트 만들기 -> 비어 있는 C++ 프로젝트를 생성합니다. (또는 sln 열기)

3. 새 프로젝트인 경우 모든 h 와 cpp 를 수동으로 추가합니다.

4. 프로젝트 구성 속성을 설정합니다

   * 프로젝트 우클릭 -> 속성 -> 구성 속성 -> 일반 -> C++ 언어 표준 항목을 ISO C++20 표준 (/std:c++20)으로 변경합니다.

   * 상단 빌드 대상을 x64 플랫폼으로 지정합니다.

5. x64 플랫폼에서 Ctrl + F5를 눌러 컴파일 및 실행을 진행합니다.

## ⬛ 향후 개발 로드맵, 최신 아키텍처 (Development Roadmap)

현재 프로젝트는 *Dear ImGui Docking Branch* 를 채택하여 다음과 같은 아키텍처적 개선을 완료하였습니다.

● **완료**: 멀티 뷰포트 렌더링 시스템
  * `ImGuiConfigFlags_ViewportsEnable` 활성화로 OS 레벨의 자유로운 창 분리 및 병합 지원
  * 별도의 OS Window 인스턴스 관리 로직을 제거하고, ImGui 기반의 윈도우 생성/파괴 시스템으로 단순화
  * 서브앱(인포그래픽)은 팩토리 패턴을 통해 런타임에 동적으로 활성화 및 관리

● **예정**: 데이터 분석 엔진 및 고도화
  * 데이터 덤프 엔진: 기존 CSV 방식을 버퍼 최적화형 바이너리(.bin) 구조로 업그레이드하여 디스크 I/O 병목 최소화
  * 규칙 엔진(Rules Engine) 고도화
  * 시각화 패널: 종/횡 G-Force, 페이스 등 엔지니어링 급 실시간 차트 구현

## ⬛ 작동 유효성 검증 및 진단 (Diagnostics)
● **Docking 유연성**: 각 텔레메트리 창을 메인 창 밖으로 드래그 시 즉시 독립적인 OS 윈도우로 분리되어 듀얼모니터 환경 등에 최적화됨

● **데이터 무결성**

  * 최대 60Hz 주파수 동기화 확인

  * 패킷 시퀀스 누락 및 부동소수점 데이터 왜곡 여부 실시간 체크

  * 코너별 데이터 인덱스 매핑 검증

## ⬛ 라이선스 (License)
본 프로젝트는 MIT 라이선스에 따라 자유롭게 수정 및 배포할 수 있습니다.

