#include "WinManager.h"
#include "SubApps/PlayerInputGraphView/PlayerInputGraphView.h"
#include "IconsFontAwesome6.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

//#include "imgui_internal.h"

// D3D11 전역 변수 (ImGui backend 가 내부적으로 사용)
static ID3D11Device*						g_pd3dDevice = nullptr;
static ID3D11DeviceContext*			g_pd3dDeviceContext =nullptr;
static IDXGISwapChain*					g_pSwapChain = nullptr;
static ID3D11RenderTargetView*	g_mainRenderTargetView=nullptr;
static UINT											g_ResizeWidth = 0;
static UINT											g_ResizeHeight = 0;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void SetupViewportTransparency(ImGuiViewport* viewport);
std::wstring GetWindowTitleW(HWND hWnd);

// WindowManager 구현
WindowManager::WindowManager() {}

void WindowManager::RegisterSubApp(const std::string& appName, SubAppDescriptor desc)
{
	m_subAppRegistry[appName] = std::move(desc);
	m_subAppToggleStates[appName] = false;	// 초기 OFF
	m_subAppLockStates[appName] = false;
}

void WindowManager::CreateMainWindow(const std::wstring& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandleA(nullptr);

	// 윈도우 클래스 등록
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"ImGuiDockingApp", nullptr };
	::RegisterClassExW(&wc);

	// 메인 윈도우 생성
	m_mainHwnd = ::CreateWindowExW(0, L"ImGuiDockingApp", title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, width, height, nullptr, nullptr, hInstance, nullptr);
	// NOTE: 디버그 출력
	if (!m_mainHwnd) {
		OutputDebugStringW(L"ERROR: CreateWindowExW failed!\n");
		return;
	}


	// D3D11 초기화
	if (!CreateDeviceD3D(m_mainHwnd)) {
		OutputDebugStringW(L"ERROR: CreateDeviceD3D failed!\n");
		CleanupDeviceD3D();
		::DestroyWindow(m_mainHwnd);
		return;
	}

	::ShowWindow(m_mainHwnd, SW_SHOWDEFAULT);
	::UpdateWindow(m_mainHwnd);

	// ImGui Docking Branch 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// ** Docking + Viewport 활성화
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// 도킹 가능
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;	// 멀티 뷰포트 (OS 창 자동 생성)

	// Viewport 설정
	io.ConfigViewportsNoAutoMerge = false;		// 창을 다시 메인으로 병합 허용
	io.ConfigViewportsNoTaskBarIcon = false;	// 각 뷰포트 작업 표시줄 아이콘 표시

	// 폰트 로드 (기본 폰트 + Font Awesome)

	// 1. 기본폰트 (한글 + ASCII)
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 2;
	fontConfig.OversampleV = 1;

	// 한글 범위 (on demand)
	static const ImWchar korean_ranges[] = {
		0x0020, 0x00FF,		// ASCII
		0xAC00, 0xD7A3,		// 한글 완성형
		0,
	};

	// 기본 폰트 로드 (한글 포함)
	// io.Fonts->AddFontDefault();   // -> Cannot use MergeMode with an explicit reference size when the destination font used an implicit reference size
	ImFontConfig baseConfig;
	io.Fonts->AddFontFromFileTTF("c:\\windows\\fonts\\arial.ttf", 16.0f, &baseConfig, korean_ranges);

	// Font Awesome 아이콘 병합
	ImFontConfig iconsConfig;
	iconsConfig.MergeMode = true;				// 기본 폰트에 병합
	iconsConfig.PixelSnapH = true;			
	iconsConfig.GlyphMinAdvanceX = 16.0f;		// 아이콘 최소 너비

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

	// Font Awesome OTF 파일 경로 (프로젝트에 포함 필요)
	io.Fonts->AddFontFromFileTTF(
		"Fonts/Font Awesome 6 Free-Solid-900.otf",		//  경로 내에 있어야 함
		16.0f,
		&iconsConfig,
		icons_ranges
	);

	// 폰트 아틀라스 빌드 (자동으로 처리됨)
	// io.Fonts->Build();

	ImGui::StyleColorsDark();

	// Platform/Renderer 백엔드 초기화
	ImGui_ImplWin32_Init(m_mainHwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	m_initialized = true;
	OutputDebugStringW(L"SUCCESS: WindowManager initialized\n");

	// 서브앱 등록 (팩토리 패턴)
	RegisterSubApp("Player Input Traces (60Hz)", {
		[]() -> std::unique_ptr<IInfoWindow> {
			return std::make_unique<PlayerInputGraphView>();
		}
	});

	// 추가 서브 앱 등록 시
	/*
	RegisterSubApp("SubAppName", {
		[]() {	return std::make_unique<SubAppClass>();
		}
	});
	*/
}

void WindowManager::RenderAllWindows(const CarTelemetryData& telemetry, float sessionTime)
{
	if (!m_initialized) return;

	// ImGui 새 프레임 시작
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 메인 컨트롤 패널 (서브앱 토글 버튼)
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

	ImGui::Begin("F1 25 Telemetry Manager");
	ImGui::Text("Telemetry Dashboard");
	ImGui::Separator();

	for (auto& [appName, isActive] : m_subAppToggleStates) {
		ImGui::PushID(appName.c_str());

		// On/Off 토글버튼
		if (isActive) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
			if (ImGui::Button("ON")) {
				isActive = false;
			}
			ImGui::PopStyleColor();
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button ("OFF")) {
				isActive = true;
			}
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();

		// Lock / Unlock 버튼
		bool& isLocked = m_subAppLockStates[appName];

		if (isLocked) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
			if (ImGui::Button(ICON_FA_LOCK)) {
				isLocked = false;
			}
			ImGui::PopStyleColor();
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button(ICON_FA_UNLOCK)) {
				isLocked	= true;
			}
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();
		ImGui::TextUnformatted(appName.c_str());
		ImGui::PopID();
	}
	ImGui::End();

	// 서브앱 생성 파괴
	for (auto& [appName, isActive] : m_subAppToggleStates) {
		if (isActive && m_activeSubApps.find(appName) == m_activeSubApps.end()) {
			// 생성 (팩토리에서 인스턴스 생성)
			auto it = m_subAppRegistry.find(appName);
			if (it != m_subAppRegistry.end()) {
				m_activeSubApps[appName] = it->second.factory();
			}
		}
		else if (!isActive && m_activeSubApps.find(appName) != m_activeSubApps.end()) {
			// 파괴: unique_ptr 자동 해제
			m_activeSubApps.erase(appName);
		}
	}

	// 활성 서브앱 렌더링 (각 앱은 독립 ImGui::Begin 윈도우임)
	// 사용자가 창을 드래그하면 ImGui 가 자동으로 OS 창 생성
	for (auto& [appName, subApp] : m_activeSubApps) {
		if (subApp && subApp->m_isOpen) {
			
			// Lock 상태 전달 (서브앱이 자신의 상태를 알 수 있도록)
			auto LockIt = m_subAppLockStates.find(appName);
			if (LockIt != m_subAppLockStates.end()) {
				subApp->m_isLocked = LockIt->second;
			}
			else {
				subApp->m_isLocked = false;
			}

			// 서브앱 스타일 적용
			SubAppStyle style = subApp->GetStyle();

			// 윈도우 배경색 설정
			if (style.useWindowBackground) {
				ImGui::PushStyleColor(ImGuiCol_WindowBg, style.windowBg);
			}

			// Viewport 배경 처리 (OS 창으로 분리될 때)
			// ImGui 는 현재 윈도우가 어느 뷰포트에 속하는지 자동 추적
			ImGuiViewport* viewport = ImGui::FindViewportByID(ImGui::GetID(appName.c_str()));
			if (viewport && viewport->PlatformUserData) {
				// 분리된 viewport 의 clear color 설정
				// NOTE: ImGui 내부 API 이기 때문에 렌더링 전에 설정해야 함
			}
			subApp->UpdateAndRender(telemetry, sessionTime);

			// 스타일 복원
			if (style.useWindowBackground) {
				ImGui::PopStyleColor();
			}
		}
	}

	// ImGui 렌더링 제출
	ImGui::Render();

	// 메인 윈도우 렌더 타겟 설정
	const float clear_color[4] = { 0.20f, 0.25f, 0.30f, 1.00f };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// 메인 윈도우 Present
	g_pSwapChain->Present(1, 0);	// Vsync ON

	// 멀티 뷰포트 업데이트 및 렌더링 
	// ImGui 가 자동으로 모든 OS 창 관리
	// NOTE: main.cpp 에서 호출
}

void WindowManager::CleanupAll()
{
	if (!m_initialized) return;

	m_activeSubApps.clear();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	if (m_mainHwnd && ::IsWindow(m_mainHwnd))
		::DestroyWindow(m_mainHwnd);

	m_initialized = false;
}

bool WindowManager::IsMainWindowActive() const
{
	return m_mainHwnd && ::IsWindow(m_mainHwnd);
}

void WindowManager::ApplyDWMTransparency(HWND hwnd)
{
	if (!hwnd) return;

	// 창 제목으로 서브앱 찾기
	std::wstring wtitle = GetWindowTitleW(hwnd);
	int size = ::WideCharToMultiByte(CP_UTF8, 0, wtitle.c_str(), -1,
		nullptr, 0, nullptr, nullptr);
	if (size == 0)return;

	std::string utf8Title(size, 0);
	::WideCharToMultiByte(CP_UTF8, 0, wtitle.c_str(), -1,
		&utf8Title[0], size, nullptr, nullptr);
	utf8Title.resize(size - 1);

	auto it = m_activeSubApps.find(utf8Title);
	if (it == m_activeSubApps.end()) return;

	SubAppStyle style = it->second->GetStyle();

	// Extended Window Style 조합
	LONG exStyle = ::GetWindowLongW(hwnd, GWL_EXSTYLE);

	// 레이어드 윈도우 
	exStyle |= WS_EX_LAYERED;

	// 오버레이 모드 설정
	if (style.overlayMode.enabled) {
		if (style.overlayMode.alwaysOnTop) {
			exStyle |= WS_EX_TOPMOST;
		}
		if (style.overlayMode.clickThrough) {
			exStyle |= WS_EX_TRANSPARENT;	// 마우스 이벤트 통과
		}
		if (style.overlayMode.noActivate) {
			exStyle |= WS_EX_NOACTIVATE;		// 포커스 X
		}
	}

	// DWM Blur Behind
	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.fEnable = TRUE;
	bb.hRgnBlur = ::CreateRectRgn(0, 0, -1, -1);

	HRESULT hr = ::DwmEnableBlurBehindWindow(hwnd, &bb);
	if (bb.hRgnBlur) ::DeleteObject(bb.hRgnBlur);

	// DWM 프레임 확장
	MARGINS margins = { -1, -1, -1, -1 };
	hr = ::DwmExtendFrameIntoClientArea(hwnd, &margins);

	// 레이어드 윈도우 투명도 설정
	BYTE alpha = (BYTE)style.overlayMode.opacity;
	::SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

	// 항상 위 설정 
	if (style.overlayMode.enabled && style.overlayMode.alwaysOnTop) {
		::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	::InvalidateRect(hwnd, nullptr, TRUE);
	::UpdateWindow(hwnd);

	std::cout << "[DWM] 오버레이 활성화: " << utf8Title
		<< "   (투명도=" << (int)alpha << ")\n";
}

// D3D11 Helper 
bool CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator= 1;
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
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 
		nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, 
		&sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

	if (hr == DXGI_ERROR_UNSUPPORTED) {
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
			createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION,
			&sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	}

	if (hr != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (g_pd3dDeviceContext) {
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = nullptr;
	}
	if (g_pd3dDevice) {
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer)
	{
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

// 완전 투명 배경 쓸 때 -> DWM 은 성능 비용이 있음 (Windows 전용)
void SetupViewportTransparency(ImGuiViewport* viewport)
{
	HWND hWnd = (HWND)viewport->PlatformHandle;

	// DWM 투명도 활성화
	MARGINS margins = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(hWnd, &margins);

	// 윈도우 스타일 변경
	LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
}

std::wstring GetWindowTitleW(HWND hWnd)
{
	if (!hWnd) return L"";

	wchar_t title[256] = { 0 };
	int len = ::GetWindowTextW(hWnd, title, 256);

	return (len > 0) ? std::wstring(title) : L"";
}

// Win32 WndProc
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOLORCHANGE:
		if ((wParam & 0xFFF0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

