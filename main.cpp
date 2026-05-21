#include "main.h"
#include "WinManager.h"

// =================================================================
// main.h에 extern으로 지정된 전역 변수들의 실제 유일한 메모리 공간 정의
// =================================================================
std::atomic<bool> g_running(true);								// 프로그램 실행 상태 플래그
SafeQueue<std::vector<char>> g_packetQueue;		// 수신된 패킷을 저장하는 스레드 안전 큐
std::atomic<uint32_t> g_packetCount(0);					// 수신된 패킷 수 카운터

std::mutex g_metricsMutex;
LiveDisplayMetrics g_liveMetrics;

// 1. 네트워크 수신 스레드 (생산자)
void UdpReceiverThread(int port) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "[Network] Winsock 초기화 실패\n";
		return;
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET) {
		std::cerr << "[Network] 소켓 생성 실패\n";
		WSACleanup();
		return;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "[Network] 포트 바인딩 실패 (" << port << ")\n";
		closesocket(sock);
		WSACleanup();
		return;
	}

	DWORD timeout = 500;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	std::vector<char> buffer(4096);
	sockaddr_in clientAddr{};
	int clientSize = sizeof(clientAddr);

	std::cout << "[Network] UDP 수신 스레 가동 완료 (Port: " << port << ")\n";

	while (g_running) {
		int bytesReceived = recvfrom(sock, buffer.data(), (int)buffer.size(), 0, (sockaddr*)&clientAddr, &clientSize);
		if (bytesReceived > 0) {
			std::vector<char> packetData(buffer.begin(), buffer.begin() + bytesReceived);
			g_packetQueue.push(packetData);
			g_packetCount++;
		}
	}

	closesocket(sock);
	WSACleanup();
	std::cout << "[Network] UDP 수신 스레드 종료.\n";
}

// 2. 데이터 가공 및 파일 출력 스레드 (소비자)
// 잠시 안쓰는 중
void DataProcessorThread() {
	std::ofstream csvFile("f1_25_telemetry_output.csv", std::ios::out | std::ios::trunc);
	// CSV 헤더 
	csvFile << "Frame,Time,Speed,Gear,RPM,Tyre_FL,Tyre_FR,Tyre_RL,Tyre_RR\n";

	std::vector<char> rawPacket;

	// 주파수(Hz) 측정용 타이머 변수
	auto lastHzCheckTime = std::chrono::steady_clock::now();
	uint32_t telemetryPacketCount = 0;
	float currentHz = 0.0f;

	// 콘솔 UI 렌더링 프레임 제한 타이머 (100ms = 10hz)
	auto lastUiUpdateTime = std::chrono::steady_clock::now();

	PacketHeader header{};

	while (g_running || g_packetQueue.size() > 0) {
		// 큐 팝 타임아웃을 1ms 로 하여 데이터 처리 스루풋 최대화
		if (g_packetQueue.pop(rawPacket, std::chrono::milliseconds(1))) {
			if (rawPacket.size() < sizeof(PacketHeader)) continue;

			// 1. 공통 헤더 안전 복사
			std::memcpy(&header, rawPacket.data(), sizeof(PacketHeader));

			// 2. F1 25 명세서 기반 Packet ID 필터링 (6: Car Telemetry 패킷)
			if (header.m_packetId == 6) {
				telemetryPacketCount++;

				// 실제 패킷 크기가 명세서 사양(1352 bytes)을 충족하는지 안전 검사
				if (rawPacket.size() >= sizeof(PacketCarTelemetryData)) {
					PacketCarTelemetryData packetData;
					std::memcpy(&packetData, rawPacket.data(), sizeof(PacketCarTelemetryData));

					// 3. 22대 차량 배열 중 플레이어 차량 고유 인덱스 데이터만 타겟팅 추출
					uint8_t playerIdx = header.m_playerCarIndex;
					if (playerIdx < 22) {
						CarTelemetryData playerCar = packetData.m_carTelemetryData[playerIdx];

						// 실시간 파일 스트리밍 출력 (I/O 병목이 없도록 수신 즉시 파일 쓰기)
						csvFile << header.m_frameIdentifier << ","
							<< header.m_sessionTime << ","
							<< playerCar.m_speed << ","
							<< (int)playerCar.m_gear << ","
							<< playerCar.m_engineRPM << ","
							<< playerCar.m_tyresPressure[0] << ","
							<< playerCar.m_tyresPressure[1] << ","
							<< playerCar.m_tyresPressure[2] << ","
							<< playerCar.m_tyresPressure[3] << "\n";

						// 콘솔 UI 스레드 공유용 가공 데이터 업데이트
						{
							std::lock_guard<std::mutex> lock(g_metricsMutex);
							g_liveMetrics.frameId = header.m_frameIdentifier;
							g_liveMetrics.sessionTime = header.m_sessionTime;
							g_liveMetrics.speed = playerCar.m_speed;
							g_liveMetrics.gear = playerCar.m_gear;
							g_liveMetrics.rpm = playerCar.m_engineRPM;
							for (int i = 0; i < 4; ++i) {
								g_liveMetrics.tyrePressure[i] = playerCar.m_tyresPressure[i];
							}
						}
					}
				}
			}
		}

		// 타이머 및 화면 출력 처리 
		auto now = std::chrono::steady_clock::now();

		// [Hz 계산] 1초 주기로 유효 수신 주파수 측정
		if (now - lastHzCheckTime >= std::chrono::seconds(1)) {
			currentHz = static_cast<float>(telemetryPacketCount);
			telemetryPacketCount = 0;
			lastHzCheckTime = now;
		}

		// [콘솔 UI 병목 픽스] 매 패킷마다 화면을 지우지 않고 10hz(100ms) 주기로만 갱신하여 CPU 점유율 최소화
		if (now - lastUiUpdateTime >= std::chrono::milliseconds(100)) {
			lastUiUpdateTime = now;

			LiveDisplayMetrics displayData;
			{
				std::lock_guard<std::mutex> lock(g_metricsMutex);
				displayData = g_liveMetrics;
			}

			// 화면 전체를 리셋하는 cls 대신 ANSI 이스케이프를 써서 스크롤 깜빡임 제거 (Win10 이상에서 지원)
			std::cout << "\x1b[H";
			std::cout << "======================================\n";
			std::cout << "  F1 25 TELEMETRY ENGINE PROTOTYPE (Debug Mode)\n";
			std::cout << "======================================\n";
			std::cout << "  [System Status]\tRunning...\n";
			std::cout << "  [Queue Size]\t\t" << g_packetQueue.size() << " packets pending\n";
			std::printf( "  [Data Frequency]\t%.1f Hz (Telemetry Filtered | Max: 60Hz)\n", currentHz);
			std::cout << "======================================\n";
			std::cout << "  [Live Telemetry Preview]\n";
			std::cout << "    - Frame ID:\t\t" << displayData.frameId << "\n";
			std::printf("    - Session Time:\t%.3f s\n", displayData.sessionTime);
			std::printf("    - Speed / Gear:\t%u km/h  |  Gear: %d\n", displayData.speed, (int)displayData.gear);
			std::printf("    - Engine RPM:\t%u RPM\n", displayData.rpm);
			std::printf("    - Tyre Press (FL/FR): %.2f psi / %.2f psi\n", displayData.tyrePressure[2], displayData.tyrePressure[3]); // 0,1번 배열 순서 매칭 확인 필
			std::printf("    - Tyre Press (RL/RR): %.2f psi / %.2f psi\n", displayData.tyrePressure[0], displayData.tyrePressure[1]);
			std::cout << "======================================\n";
			std::cout << "  [Output File]\tf1_25_telemetry_output.csv saved.\n";
			std::cout << "  Press [ENTER] to exit program.\n";
			std::cout << "======================================\n";
		}
	}
	csvFile.close();
	std::cout << "[Processor] 파일 저장 완료 및 스레드 종료.\n";
}

int main(int, char**) {
	// WinSock 기반 UDP 백엔드 수신 스레드 가동
	int targetPort = 20778;  // F1 25 기본 값은 20777
	std::thread receiver(UdpReceiverThread, targetPort);

	// FIX : 보이지 않는 더미 윈도우를 활용하여 전역 하드웨어 디바이스 초기화 (최초 1회)
	HWND dummyHWnd = ::CreateWindowExW(0,
		L"STATIC",
		L"D3DInitDeviceHolder",
		0,
		0,
		0,
		0,
		0,
		nullptr,
		nullptr,
		nullptr,
		nullptr);

	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Device* pd3dDevice = nullptr;
	ID3D11DeviceContext* pd3dDeviceContext = nullptr;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	HRESULT hr = D3D11CreateDevice(nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevelArray,
		2,
		D3D11_SDK_VERSION,
		&pd3dDevice,
		&featureLevel,
		&pd3dDeviceContext);
	if (FAILED(hr)) {
		g_running = false;
		if (receiver.joinable()) receiver.join();
		return 1;
	}

	// ImGui 공유 폰트 내부 아틀라스 시스템 구동 준비
	IMGUI_CHECKVERSION();
	ImGuiContext* baseContext = ImGui::CreateContext();
	ImPlot::CreateContext();

	// 윈도우 매니저 기동 및 독립 앱 창 개별 생성 명령 submit
	WindowManager winManager(pd3dDevice, pd3dDeviceContext);

	// 독립 창 1호 생성 요청 (서브 모니터 등에 각각 독립 배치 가능)
	winManager.CreateNewAppWindow(L"F1 25 Player Input Graph", 1024, 450, std::make_unique<PlayerInputGraphView>());

	std::vector<char> rawPacket;
	PacketHeader header{};
	CarTelemetryData playerTelemetry{};

	// 전역 루프 가동 시작
	while (winManager.HasActiveWindows()) {
		MSG msg;
		while(::PeekMessage(&msg,nullptr,0U,0U,PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)break;
		}

		// 패킷 파싱 처리 연동 (최신 데이터로 플레이어 텔레메트리 정보 업데이트)
		while (g_packetQueue.size() > 0) {
			if (g_packetQueue.pop(rawPacket, std::chrono::milliseconds(0))) {
				if (rawPacket.size() < sizeof(PacketHeader)) continue;
				std::memcpy(&header, rawPacket.data(), sizeof(PacketHeader));

				if (header.m_packetId == 6 && rawPacket.size() >= sizeof(PacketCarTelemetryData)) {
					PacketCarTelemetryData fullPacket;
					std::memcpy(&fullPacket, rawPacket.data(), sizeof(PacketCarTelemetryData));
					if (header.m_playerCarIndex < 22) {
						playerTelemetry = fullPacket.m_carTelemetryData[header.m_playerCarIndex];
					}
				}
			}
		}

		// 패킷 단절 시 시뮬레이션 코드
		float currentSessionTime = header.m_sessionTime;
		if (currentSessionTime <= 0.0f) {
			static float dummyTime = 0.0f;
			dummyTime += 0.016f;
			currentSessionTime = dummyTime;
			playerTelemetry.m_throttle = (sinf(dummyTime) + 1.0f) * 0.5f;
			playerTelemetry.m_brake = (cosf(dummyTime * 1.5f) > 0.3f) ? (cosf(dummyTime * 1.5f)) : 0.0f;
			playerTelemetry.m_steer = sinf(dummyTime * 0.8f);
		}

		// 생성된 모든 독립 윈도우 인스턴스들의 메시지 스왑 및 렌더링 총괄 처리
		winManager.RenderAllIndependentWindows(playerTelemetry, currentSessionTime);

		std::this_thread::sleep_for(std::chrono::milliseconds(8)); //  약 120hz 동기화 속도
	}

	// 클린업 및 메모리 소멸 단계
	g_running = false;
	if (receiver.joinable()) receiver.join();

	winManager.CleanupAll();
	ImPlot::DestroyContext();
	ImGui::DestroyContext(baseContext);

	if (pd3dDeviceContext) pd3dDeviceContext->Release();
	if (pd3dDevice) pd3dDevice->Release();
	::DestroyWindow(dummyHWnd);

	return 0;
}