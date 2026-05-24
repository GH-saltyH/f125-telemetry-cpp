#pragma once

#include "main.h"
#include <functional>

extern UINT										g_ResizeWidth;
extern UINT										g_ResizeHeight;

// Win32 / DX11 핵심 인프라 함수 원형 선언 (본문 제거 완료)
// Win32 전역 WndProc 메시지 처리기 인터페이스 선언부
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 독립 인포그래픽 창 관리를 위한 인터페이스 및 관리자 클래스
class IInfoWindow {
public:
	bool m_isOpen = true;
	std::string m_windowName;

	IInfoWindow(const std::string& name) : m_windowName(name) {}
	virtual ~IInfoWindow() = default;

	virtual void UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) = 0;
};

// 서브앱 등록 디스크립터 (팩토리 패턴)
struct SubAppDescriptor {
	std::function<std::unique_ptr<IInfoWindow>()> factory;
};

class WindowManager {
private:
	HWND	m_mainHwnd = nullptr;
	bool			m_initialized = false;

	std::map<std::string, bool>								m_subAppToggleStates;			// ON/OFF 상태
	std::map<std::string, SubAppDescriptor>		m_subAppRegistry;					// 팩토리
	std::map<std::string, std::unique_ptr<IInfoWindow>> m_activeSubApps;	// 활성 서브앱 인스턴스

public:
	WindowManager();
	~WindowManager() { CleanupAll(); }

	void RegisterSubApp(const std::string& appName, SubAppDescriptor desc);
	void CreateMainWindow(const std::wstring& title, int width, int height);
	void RenderAllWindows(const CarTelemetryData& telemetry, float sessionTime);
	void CleanupAll();
	bool IsMainWindowActive() const;
};

