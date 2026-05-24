#include "main.h"
#include "WinManager.h"

// =================================================================
// GlobalState.h 에 extern 선언된 전역 변수들의 실제 메모리 공간 정의
// =================================================================
std::atomic<bool> g_running(true);
SafeQueue<std::vector<char>> g_packetQueue;
std::atomic<uint32_t> g_packetCount(0);
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

int main() {
	// WinSock 기반 UDP 백엔드 수신 스레드 가동
	int targetPort = 20778;  // F1 25 기본 값은 20777
	std::thread receiver(UdpReceiverThread, targetPort);

	// WindowManager 가 D3D 초기화 전체를 소유한다
	WindowManager winManager;
	winManager.CreateMainWindow(L"F1 25 Telemetry Manager", 900, 520);

	std::vector<char> rawPacket;
	PacketHeader header{};
	CarTelemetryData playerTelemetry{};

	// 메인 윈도우의 실행 여부에 종속하여 프로그램 수명 제어
	while (winManager.IsMainWindowActive()) {
		MSG msg;
		while(::PeekMessage(&msg,nullptr,0U,0U,PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)break;
		}

		// 패킷 파싱 처리 연동 (최신 데이터로 플레이어 텔레메트리 정보 업데이트)
		while(g_packetQueue.pop(rawPacket, std::chrono::milliseconds(0))) {
			if (rawPacket.size() < sizeof(PacketHeader)) continue;
			std::memcpy(&header, rawPacket.data(), sizeof(PacketHeader));

			if (header.m_packetId == 6 && rawPacket.size() >= sizeof(PacketCarTelemetryData)) {
				PacketCarTelemetryData fullPacket;
				std::memcpy(&fullPacket, rawPacket.data(), sizeof(PacketCarTelemetryData));
				if (header.m_playerCarIndex < 22) 
					playerTelemetry = fullPacket.m_carTelemetryData[header.m_playerCarIndex];
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
		winManager.RenderAllWindows(playerTelemetry, currentSessionTime);

		// Docking Branch Multi-Viewport 지원
		// 모든 플랫폼 윈도우(뷰포트) 자동 업데이트 및 렌더링
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		// Present(1,0) Vsync 에 의해 메인 루프가 자연스럽게 throttle 됨
		// Vsync 가 꺼진 환경에서만 16ms (60Hz) sleep 으로 풀백
	}

	// 클린업 및 메모리 소멸 단계
	g_running = false;
	if (receiver.joinable()) receiver.join();

	winManager.CleanupAll();

	// ImGui/ImPlot 컨텍스트 소멸은 WindowManager::CleanupAll() 에서 수행한다

	return 0;
}