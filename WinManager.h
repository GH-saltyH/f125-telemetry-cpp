#pragma once

#include "main.h"

// 중복 선언 방지를 위해 전역 하드웨어 디바이스 포인터 객체들을 extern 전방 선언
// =================================================================
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern UINT g_ResizeWidth;
extern UINT g_ResizeHeight;
extern ID3D11RenderTargetView* g_mainRenderTargetView;

// Win32 / DX11 핵심 인프라 함수 원형 선언 (본문 제거 완료)
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 독립 인포그래픽 창 관리를 위한 인터페이스 및 관리자 클래스
class IInfoWindow {
public:
	bool m_isOpen = true;
	std::string m_windowName;

	IInfoWindow(const std::string& name) : m_windowName(name) {}
	virtual ~IInfoWindow() = default;

	// 각 창이 독립적으로 가질 렌더링 및 가공 루프
	virtual void UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) = 0;
};

class WindowManager {
private:
	std::vector<std::unique_ptr<IInfoWindow>> m_windows;
public:
	void AddWindow(std::unique_ptr<IInfoWindow> window) {
		m_windows.push_back(std::move(window));
	}

	void RenderAllWindows(const CarTelemetryData& telemetry, float sessionTime);
};

// 플레이어 패달/휠 조작 입출력 플로팅 뷰 클래스
class PlayerInputGraphView : public IInfoWindow {
private:
	static const int MAX_DATA_POINTS = 600;
	std::vector<float> m_timeBuffer;
	std::vector<float> m_throttleBuffer;
	std::vector<float> m_brakeBuffer;
	std::vector<float> m_steerBuffer;

public:
	PlayerInputGraphView() : IInfoWindow("Player Input Traces (60Hz)") {}
	void UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) override;
};


//
//void CleanupRenderTarget() {
//	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
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
//	ID3D11Texture2D* pBackBuffer;
//	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
//	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
//	pBackBuffer->Release();
//}
//
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
//
//// -----------------------------------------------------------------
//// 3. Win32 및 DX11 보일러플레이트 지원 함수 구현부
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