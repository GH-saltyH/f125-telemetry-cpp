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

// Win32 전역 WndProc 메시지 처리기 인터페이스 선언부
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

// OS 창 핸들 및 DX11 렌더 타깃 뷰포트 관리를 위한 구조체
struct OSWindowInstance {
	HWND hWnd = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;
	ImGuiContext* imguiContext = nullptr;    // 창 마다 가질 독립적 UI 상태 컨텍스트
	std::unique_ptr<IInfoWindow> uiView = nullptr;
	UINT resizeWidth = 0;
	UINT resizeHeight = 0;
};;

class WindowManager {
private:
	// FIX: 단일 뷰포트 벡터에서 실제 OS 창 인스턴스 배열로 변경
	std::vector<OSWindowInstance> m_osWindows;
	ID3D11Device* m_pd3dDevice= nullptr;
	ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;
public:
	WindowManager(ID3D11Device* device, ID3D11DeviceContext* context)
		: m_pd3dDevice(device), m_pd3dDeviceContext(context) {
	}
	~WindowManager() { CleanupAll(); }

	// FIX: 새로운 독립 창을 OS에 등록하고 띄우는 메서드로 고도화
	void CreateNewAppWindow(const std::wstring& title, int width, int height, std::unique_ptr<IInfoWindow> view);

	// 독립 창들을 순회하며 이벤트 메시지, ImGui 컨텍스트 스왑, 렌더 submit 총괄 처리하는 메서드로 고도화
	void RenderAllIndependentWindows(const CarTelemetryData& telemetry, float sessionTime);

	void CleanupAll();
	bool HasActiveWindows() const { return !m_osWindows.empty(); }

	// 유틸리티: WinProc 에서 개별 창 인스턴스를 찾을 수 있도록 포인터 반환
	OSWindowInstance* FindInstanceByHWND(HWND hWnd);
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

