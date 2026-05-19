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
	int targetPort = 20777;  // F1 25 기본 값은 20777
	std::thread receiver(UdpReceiverThread, targetPort);

	// Win32 윈도우 클래스 등록 및 창 생성
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"F1 25 Telemetry Window", nullptr };
	::RegisterClassExW(&wc);
	HWND hWnd = ::CreateWindow(wc.lpszClassName, L"F1 25 High-Speed Graphics Dashboard Pro", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	// DX11 디바이스 초기화
	if (!CreateDeviceD3D(hWnd)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		g_running = false;
		if (receiver.joinable ()) receiver.join();
		return 1;
	}

	::ShowWindow(hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(hWnd);


	// 4. ImGui 및 ImPlot 컨텍스트 바인딩
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// 대시보드 시스템 초기화 및 독립 프로토타입 창 생성 등록
	WindowManager winManager;
	winManager.AddWindow(std::make_unique<PlayerInputGraphView>());

	// 무한 루프 플로우 통제 변수
	bool done = false;
	std::vector<char> rawPacket;
	PacketHeader header{};
	CarTelemetryData playerTelemetry{};

	while (!done) {
		// Win32 이벤트 메시지 루프 강제 펌핑
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) done = true;
		}
		if (done) break;

		// 창 크기가 변경되었을 때 렌더 타깃 리사이징 버퍼 갱신 처리
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// 7. 생산자-소비자 패킷 큐 스루풋 파싱 (스레드 세이프 보장 최신화)
		while (g_packetQueue.size() > 0) {
			if (g_packetQueue.pop(rawPacket, std::chrono::milliseconds(0))) {
				if (rawPacket.size() < sizeof(PacketHeader)) continue;
				std::memcpy(&header, rawPacket.data(), sizeof(PacketHeader));

				// Packet ID 6 (Car Telemetry) 데이터 필터 패킹
				if (header.m_packetId == 6 && rawPacket.size() >= sizeof(PacketCarTelemetryData)) {
					PacketCarTelemetryData fullPacket;
					std::memcpy(&fullPacket, rawPacket.data(), sizeof(PacketCarTelemetryData));
					if (header.m_playerCarIndex < 22) {
						playerTelemetry = fullPacket.m_carTelemetryData[header.m_playerCarIndex];
					}
				}
			}
		}

		// 안전 장치: 세션 타임 가비지 예외 필터 처리
		float currentSessionTime = header.m_sessionTime;
		if (currentSessionTime < 0.0f || currentSessionTime > 50000.0f) {
			// 아직 게임 연결 전이거나 가비지 값일 경우 흐르는 시간 임시 보정용 가상 틱 생성
			static float dummyTime = 0.0f;
			dummyTime += 0.016f;
			currentSessionTime = dummyTime;

			// 디버그 테스트용 더미 인풋 시뮬레이터 (사인/코사인 궤적 구동)
			playerTelemetry.m_throttle = (sinf(dummyTime) + 1.0f) * 0.5f;
			playerTelemetry.m_brake = (cosf(dummyTime * 1.5f) > 0.3f) ? (cosf(dummyTime * 1.5f)) : 0.0f;
			playerTelemetry.m_steer = sinf(dummyTime * 0.8f);
		}

		// 8. ImGui/ImPlot 프레임 그리기 시작
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 외부 ImGui 라이브러리 정상 연동 확인용 공식 데모 도킹 창 출력 (X 버튼으로 탈착 가능)
		ImGui::ShowDemoWindow();
		ImPlot::ShowDemoWindow();

		// 9. 독립 관리 창 총괄 렌더 가동
		winManager.RenderAllWindows(playerTelemetry, currentSessionTime);

		// 10. 백버퍼 스왑 및 화면 GPU 출력 서브밋
		ImGui::Render();
		const float clear_color_with_alpha[4] = { 0.10f, 0.10f, 0.12f, 1.0f }; //Moody Gray 백그라운드 색감
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // VSync 걸어 60fps 고정으로 CPU/GPU 오버헤드 억제
	}

	// 11. 인프라 클린업 및 메모리 소멸 단계
	g_running = false;
	if (receiver.joinable()) receiver.join();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hWnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}