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
void WindowManager::RenderAllWindows(const CarTelemetryData& telemetry, float sessionTime) {
	for (auto it = m_windows.begin(); it != m_windows.end();) {
		if (!(*it)->m_isOpen) {
			it = m_windows.erase(it);
		}
		else {
			(*it)->UpdateAndRender(telemetry, sessionTime);
			++it;
		}
	}
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

	// int8_t 형태의 Steering 데이터를 플로팅 가능한 정규화 float(-1.0 ~ 1.0)으로 변환 적용
	m_steerBuffer.push_back(static_cast<float>(telemetry.m_steer) / 100.0f);

	ImGui::Begin(m_windowName.c_str(), &m_isOpen);
	ImGui::Text("F1 25 실시간 인풋 레이아웃 스크롤러");
	ImGui::Separator();

	if (ImPlot::BeginPlot("##InputTracesPlot", ImVec2(-1, 350))) {
		ImPlot::SetupAxes("Session Time (s)", "Value", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f, ImGuiCond_Always);

		if (!m_timeBuffer.empty()) {
			ImPlot::SetupAxisLimits(ImAxis_X1, m_timeBuffer.front(), m_timeBuffer.back(), ImGuiCond_Always);

			// implot.h 사양에 따른 인라인 사양 강제 매핑 및 U32 컬러 형변환 완료
			ImPlot::PlotLine("Throttle", m_timeBuffer.data(), m_throttleBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)),
				ImPlotProp_LineWeight, 2.0f
				});

			ImPlot::PlotLine("Brake", m_timeBuffer.data(), m_brakeBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)),
				ImPlotProp_LineWeight, 2.0f
				});

			ImPlot::PlotLine("Steering", m_timeBuffer.data(), m_steerBuffer.data(), (int)m_timeBuffer.size(), {
				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.6f, 1.0f, 1.0f)),
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
			g_ResizeWidth = (UINT)LOWORD(lParam);
			g_ResizeHeight = (UINT)HIWORD(lParam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


//
//#include "WinManager.h"
//
//// =================================================================
//// 🛠️ WinManager.h에 명시된 하드웨어 컨텍스트 객체 정의 및 초기화 완료
//// =================================================================
//ID3D11Device* g_pd3dDevice = nullptr;
//ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
//IDXGISwapChain* g_pSwapChain = nullptr;
//UINT                     g_ResizeWidth = 0;
//UINT                     g_ResizeHeight = 0;
//ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
//
//// -----------------------------------------------------------------
//// WindowManager 및 하위 그래픽 컴포넌트 멤버 함수 구현부
//// -----------------------------------------------------------------
//void WindowManager::RenderAllWindows(const CarTelemetryData& telemetry, float sessionTime) {
//	for (auto it = m_windows.begin(); it != m_windows.end();) {
//		if (!(*it)->m_isOpen) {
//			it = m_windows.erase(it);
//		}
//		else {
//			(*it)->UpdateAndRender(telemetry, sessionTime);
//			++it;
//		}
//	}
//}
//
//void PlayerInputGraphView::UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) {
//	if (m_timeBuffer.size() >= MAX_DATA_POINTS) {
//		m_timeBuffer.erase(m_timeBuffer.begin());
//		m_throttleBuffer.erase(m_throttleBuffer.begin());
//		m_brakeBuffer.erase(m_brakeBuffer.begin());
//		m_steerBuffer.erase(m_steerBuffer.begin());
//	}
//
//	m_timeBuffer.push_back(sessionTime);
//	m_throttleBuffer.push_back(telemetry.m_throttle);
//	m_brakeBuffer.push_back(telemetry.m_brake);
//	
//	// int8_t 형태의 Steering 데이터를 플로팅 가능한 정규화 float(-1.0 ~ 1.0)으로 변환 적용
//	m_steerBuffer.push_back(static_cast<float>(telemetry.m_steer) / 100.0f);
//
//	ImGui::Begin(m_windowName.c_str(), &m_isOpen);
//	ImGui::Text("F1 25 실시간 인풋 레이아웃 스크롤러");
//	ImGui::Separator();
//
//	if (ImPlot::BeginPlot("##InputTracesPlot", ImVec2(-1, 350))) {
//		ImPlot::SetupAxes("Session Time (s)", "Value", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
//		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f, ImGuiCond_Always);
//
//		if (!m_timeBuffer.empty()) {
//			ImPlot::SetupAxisLimits(ImAxis_X1, m_timeBuffer.front(), m_timeBuffer.back(), ImGuiCond_Always);
//
//			// implot.h 사양에 따른 인라인 사양 강제 매핑 및 U32 컬러 형변환 완료
//			ImPlot::PlotLine("Throttle", m_timeBuffer.data(), m_throttleBuffer.data(), (int)m_timeBuffer.size(), {
//				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)),
//				ImPlotProp_LineWeight, 2.0f
//			});
//
//			ImPlot::PlotLine("Brake", m_timeBuffer.data(), m_brakeBuffer.data(), (int)m_timeBuffer.size(), {
//				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)),
//				ImPlotProp_LineWeight, 2.0f
//			});
//
//			ImPlot::PlotLine("Steering", m_timeBuffer.data(), m_steerBuffer.data(), (int)m_timeBuffer.size(), {
//				ImPlotProp_LineColor, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.6f, 1.0f, 1.0f)),
//				ImPlotProp_LineWeight, 1.5f
//			});
//		}
//		ImPlot::EndPlot();
//	}
//	ImGui::End();
//}
//
//// -----------------------------------------------------------------
//// DirectX 11 장치 인프라 저수준 보일러플레이트 실체 정의부 (LNK2005 완전 제거)
//// -----------------------------------------------------------------
//bool CreateDeviceD3D(HWND hWnd) {
//	DXGI_SWAP_CHAIN_DESC sd;
//	ZeroMemory(&sd, sizeof(sd));
//	sd.BufferCount = 2;
//	sd.BufferDesc.Width = 0;
//	sd.BufferDesc.Height = 0;
//	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	sd.BufferDesc.RefreshRate.Numerator = 60;
//	sd.BufferDesc.RefreshRate.Denominator = 1;
//	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	sd.OutputWindow = hWnd;
//	sd.SampleDesc.Count = 1;
//	sd.SampleDesc.Quality = 0;
//	sd.Windowed = TRUE;
//	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//
//	UINT createDeviceFlags = 0;
//	D3D_FEATURE_LEVEL featureLevel;
//	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
//	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//	if (res == DXGI_ERROR_UNSUPPORTED)
//		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//	if (res != S_OK) return false;
//
//	CreateRenderTarget();
//	return true;
//}
//
//void CleanupDeviceD3D() {
//	CleanupRenderTarget();
//	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
//	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
//	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
//}
//
//void CreateRenderTarget() {
//	ID3D11Texture2D* pBackBuffer = nullptr;
//	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
//	if (pBackBuffer) {
//		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
//		pBackBuffer->Release();
//	}
//}
//
//void CleanupRenderTarget() {
//	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
//}
//
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
//	switch (msg) {
//	case WM_SIZE:
//		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
//			g_ResizeWidth = (UINT)LOWORD(lParam);
//			g_ResizeHeight = (UINT)HIWORD(lParam);
//		}
//		return 0;
//	case WM_SYSCOMMAND:
//		if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
//		break;
//	case WM_DESTROY:
//		::PostQuitMessage(0);
//		return 0;
//	}
//	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
//}