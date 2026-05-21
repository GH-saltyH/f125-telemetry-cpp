#include "WinManager.h"

// =================================================================
// WinManager.h에 명시된 하드웨어 컨텍스트 객체 정의 및 초기화 완료
// =================================================================
ID3D11Device*					g_pd3dDevice = nullptr;
ID3D11DeviceContext*		g_pd3dDeviceContext = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
UINT									g_ResizeWidth = 0;
UINT									g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// -----------------------------------------------------------------
// WindowManager 및 하위 그래픽 컴포넌트 멤버 함수 구현부
// -----------------------------------------------------------------
static WindowManager* g_WinManagerPtr = nullptr;  // WndProc 연동용 전역 포인터

void WindowManager::CreateNewAppWindow(const std::wstring& title, int width, int height, std::unique_ptr<IInfoWindow> view)
{
	g_WinManagerPtr = this;
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	OSWindowInstance winInstance;
	winInstance.uiView = std::move(view);

	WNDCLASSEXW wc = { sizeof(wc),
		CS_CLASSDC,
		WndProc,
		0L,
		0L,
		hInstance,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		L"IndependentAppSubWindowClass",
		nullptr };
	::RegisterClassExW(&wc);

	winInstance.hWnd = ::CreateWindowW(wc.lpszClassName,
		title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if (!winInstance.hWnd) return;

	// DX11 swap chain 독립 생성 및 매핑
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = winInstance.hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	IDXGIDevice* pDXGIDevice = nullptr;
	IDXGIAdapter* pDXGIAdapter = nullptr;
	IDXGIFactory* pIDXGIFactory = nullptr;
	m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));
	if (pDXGIDevice) {
		pDXGIDevice->GetAdapter(&pDXGIAdapter);
		if (pDXGIAdapter) {
			pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));
			pDXGIAdapter->Release();
		}
		pDXGIDevice->Release();
	}

	if (pIDXGIFactory) {
		pIDXGIFactory->CreateSwapChain(m_pd3dDevice, &sd, &winInstance.swapChain);
		pIDXGIFactory->Release();
	}

	// 렌더 타겟 뷰 초기화
	ID3D11Texture2D* pBackBuffer = nullptr;
	winInstance.swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if(pBackBuffer) {
		m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &winInstance.renderTargetView);
		pBackBuffer->Release();
	}

	// FIX1 : 폰트 아틀라스 충돌 및 빌더 에러 해결 (컨텍스트 초기화 수정)
	// 현재 메인 스레드의 기존 컨텍스트 백업
	ImGuiContext* backupContext = ImGui::GetCurrentContext();  

	// FIX1. 인자값으로 nullptr 을 전달하여 이 창만의 고유한 독립 폰트 아틀라스를 내부에서 생성
	winInstance.imguiContext = ImGui::CreateContext(nullptr); 

	ImGui::SetCurrentContext(winInstance.imguiContext);  // 새 컨텍스트 활성화

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 키보드 네비게이션 활성화
	ImGui::StyleColorsDark();

	// 독립 컨텍스트 환경 위에서 Win32 및 DX11 백엔드 초기화 수행
	ImGui_ImplWin32_Init(winInstance.hWnd);
	ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);

	// 초기화 종료 후 원래 메인 스레드가 사용하던 컨텍스트로 안전하게 복원
	ImGui::SetCurrentContext(backupContext);  

	::ShowWindow(winInstance.hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(winInstance.hWnd);

	m_osWindows.push_back(std::move(winInstance));
}

void WindowManager::RenderAllIndependentWindows(const CarTelemetryData& telemetry, float sessionTime)
{
	ImGuiContext* originalContext = ImGui::GetCurrentContext();  // 글로벌 컨텍스트 백업

	for (auto it = m_osWindows.begin(); it != m_osWindows.end();) {
		// 사용자가 UI 창을 닫았거나 배후에서 HWND 가 파괴된 경우 청소
		if (!it->uiView->m_isOpen || !::IsWindow(it->hWnd)) {
			ImGui::SetCurrentContext(it->imguiContext);
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext(it->imguiContext);

			if (it->renderTargetView) it->renderTargetView->Release();
			if (it->swapChain) it->swapChain->Release();
			::DestroyWindow(it->hWnd);

			it = m_osWindows.erase(it);
			continue;
		}

		// 창 크기 변경 메시지가 들어온 경우의 렌더 타깃 유연 갱신 처리
		if (it->resizeWidth != 0 && it->resizeHeight != 0) {
			if (it->renderTargetView) {
				it->renderTargetView->Release();
				it->renderTargetView = nullptr;
			}
			it->swapChain->ResizeBuffers(0, it->resizeWidth, it->resizeHeight, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* pBackBuffer = nullptr;
			it->swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
			if (pBackBuffer) {
				m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &it->renderTargetView);
				pBackBuffer->Release();
			}
			it->resizeWidth = it->resizeHeight = 0;
		}

		// 창에 매핑된 독립 ImGuiContext 로 현재 스레드 전역 상태 전향전환
		ImGui::SetCurrentContext(it->imguiContext);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 고정 축 그래프 출력 뷰 가동
		it->uiView->UpdateAndRender(telemetry, sessionTime);

		ImGui::Render();
		const float clear_color[4] = { 0.08f, 0.08f,0.10f,1.0f };
		m_pd3dDeviceContext->OMSetRenderTargets(1, &it->renderTargetView, nullptr);
		m_pd3dDeviceContext->ClearRenderTargetView(it->renderTargetView, clear_color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		it->swapChain->Present(1, 0);   // 각 독립 앱 화면 백버퍼 갱신 출력

		++it;
	}

	ImGui::SetCurrentContext(originalContext);  // 글로벌 컨텍스트로 복귀
}

void WindowManager::CleanupAll()
{
	for (auto& win : m_osWindows) {
		if (win.imguiContext) {
			ImGui::SetCurrentContext(win.imguiContext);
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext(win.imguiContext);
		}
		if (win.renderTargetView) win.renderTargetView->Release();
		if (win.swapChain) win.swapChain->Release();
		if (win.hWnd) ::DestroyWindow(win.hWnd);
	}
	m_osWindows.clear();
}

OSWindowInstance* WindowManager::FindInstanceByHWND(HWND hWnd)
{
	for (auto& win : m_osWindows) {
		if (win.hWnd == hWnd) return &win;
	}
	return nullptr;
}

void PlayerInputGraphView::UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) {
	if (m_timeBuffer.size() >= MAX_DATA_POINTS) {
		m_timeBuffer.erase(m_timeBuffer.begin());
		m_throttleBuffer.erase(m_throttleBuffer.begin());
		m_brakeBuffer.erase(m_brakeBuffer.begin());
		m_steerBuffer.erase(m_steerBuffer.begin());
	}

	m_timeBuffer.push_back(sessionTime);
	m_throttleBuffer.push_back(telemetry.m_throttle);
	m_brakeBuffer.push_back(telemetry.m_brake);
	m_steerBuffer.push_back(telemetry.m_steer);
	// int8_t 형태의 Steering 데이터를 플로팅 가능한 정규화 float(-1.0 ~ 1.0)으로 변환 적용
	//m_steerBuffer.push_back(static_cast<float>(telemetry.m_steer) / 100.0f);
	
	// FIX: 전체 윈도우 꽉 차도록 ImGui 배치
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);

	ImGui::Begin(m_windowName.c_str(), &m_isOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
	ImGui::Text("Pedal / Steer");
	ImGui::Separator();

	if (ImPlot::BeginPlot("##InputTracesPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Session Time (s)", "Value", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f, ImGuiCond_Always);

		// 2026-05-21 Fix : 데이터 존재 여부와 무관하게 가로축 표현 한계치 고정 (링버퍼 크기에 맞춰 고정 가능)
		const float X_AXIS_RANGE = 10.0f;
		float x_max = sessionTime;
		float x_min = x_max = X_AXIS_RANGE;

		// 데이터가 아예 없는 초기 상태여도 축이 흔들리지 않고 미리 선언된 범위로 고정
		ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);

		// 내부 버퍼가 비어있지 않을 때만 드로우 호출 (축은 바깥에서 항상 수행하도록)
		if (!m_timeBuffer.empty()) {
			ImPlot::SetupAxisLimits(ImAxis_X1, m_timeBuffer.front(), m_timeBuffer.back(), ImGuiCond_Always);

			// implot.h 사양에 따른 인라인 사양 강제 매핑 및 U32 컬러 형변환 완료
			ImPlot::PlotLine("Throttle", m_timeBuffer.data(), m_throttleBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)),
				ImPlotProp_LineWeight, 2.5f
				});

			ImPlot::PlotLine("Brake", m_timeBuffer.data(), m_brakeBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)),
				ImPlotProp_LineWeight, 2.5f
				});

			ImPlot::PlotLine("Steering", m_timeBuffer.data(), m_steerBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)),
				ImPlotProp_LineWeight, 1.5f
				});
		}
		ImPlot::EndPlot();
	}
	ImGui::End();
}

// -----------------------------------------------------------------
// DirectX 11 장치 인프라 저수준 보일러플레이트 실체 정의부 
// -----------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED)
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK) return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D() {
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer) {
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void CleanupRenderTarget() {
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// 다중 네이티브 윈도우 라우팅 메시지 핸들러 구현부
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (g_WinManagerPtr) {
		OSWindowInstance* instance = g_WinManagerPtr->FindInstanceByHWND(hWnd);
		if (instance && instance->imguiContext) {
			ImGui::SetCurrentContext(instance->imguiContext);  // 메시지 처리 시 해당 창의 ImGuiContext 로 전환
			if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true; // ImGui 가 메시지를 처리한 경우 추가 처리를 막고 바로 반환

			switch (msg) {
			case WM_SIZE:
				if (wParam != SIZE_MINIMIZED) {
					instance->resizeWidth = (UINT)LOWORD(lParam);
					instance->resizeHeight = (UINT)HIWORD(lParam);
				}
				return 0;
			case WM_CLOSE:
				instance->uiView->m_isOpen = false; // 플래그 오프를 통한 렌더링 루프 내 창 종료 처리
				return 0;
			}
		}
	}

	if (msg == WM_DESTROY && !g_WinManagerPtr->HasActiveWindows()) {
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


