#pragma once

#include "main.h"
#include <functional>

extern UINT										g_ResizeWidth;
extern UINT										g_ResizeHeight;

// Win32 / DX11 핵심 인프라 함수 원형 선언 (본문 제거 완료)
// Win32 전역 WndProc 메시지 처리기 인터페이스 선언부
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 오버레이 스타일
struct OverlayMode {
	bool enabled = false;					
	bool alwaysOnTop = true;			// HWND_TOPMOST  - 다른 창 위에 표시
	bool clickThrough = false;			// WS_EX_TRANSPARENT  - 마우스 이벤트 통과
	bool noActivate = true;				// WS_EX_NOACTIVATE  - 포커스 받지 않음
	int opacity = 255;						// 0투명 255 불투명
};

// 각 창의 윈도우 스타일 제어
struct SubAppStyle {
	// 윈도우 배경 설정
	ImVec4 windowBg = ImVec4(0.15f, 0.15f, 0.15f, 0.9f);		// 기본 약간 투명한 회색
	bool useWindowBackground = true;		// false 면 완전 투명 (ImGuiWindowFlags_NoBackground)
	bool hideTitleBar = false;	// ImGuiWindowFlags_NoTitleBar 

	// 윈도우 플래그
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

	// Viewport 설정 (OS 창으로 분리될 때)
	bool transparentViewport = false;		// true 면 DMW 합성 투명도 활성화 (Windows 전용임)
	ImVec4 viewportClearColor= ImVec4(0.0f, 0.0f, 0.0f, 0.0f);		// Viewport 배경색

	// 게임 오버레이 설정
	OverlayMode overlayMode;
	
	// 프리셋: 게임 오버레이 (클릭 X 항상 위)
	static SubAppStyle GameOverlay() {
		SubAppStyle s;
		s.windowBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		s.useWindowBackground = false;
		s.transparentViewport = true;
		s.windowFlags = ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground;

		// 오버레이 모드 활성화
		s.overlayMode.enabled = true;
		s.overlayMode.alwaysOnTop = true;
		s.overlayMode.clickThrough = true;		// 게임 조작 방해 X
		s.overlayMode.noActivate = true;
		s.overlayMode.opacity = 230;

		return s;
	}

	// 프리셋: 게임 오버레이 (클릭 가능)
	static SubAppStyle InteractiveOverl() {
		SubAppStyle s = GameOverlay();
		s.overlayMode.clickThrough = false;
		s.windowFlags &= ~ImGuiWindowFlags_NoMove;	// 이동 가능하게 제거
		return s;
	}

	// 기본 스타일 프리셋
	static SubAppStyle Opaque() {
		SubAppStyle s;
		s.windowBg = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
		s.useWindowBackground = true;
		s.transparentViewport = false;
		return s;
	}

	static SubAppStyle SemiTransparent() {
		SubAppStyle s;
		s.windowBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);		// 윈도우 배경 없음
		s.useWindowBackground = false;				// NoBackground 플래그 사용
		s.transparentViewport = false;
		s.viewportClearColor= ImVec4(0.1f, 0.1f, 0.1f, 0.5f);		// Viewport 만 반투명
		return s;
	}

	static SubAppStyle FullTransparent() {
		SubAppStyle s;
		s.windowBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);		// 윈도우 배경 없음
		s.useWindowBackground = false;				// NoBackground 플래그 사용
		s.transparentViewport = true;					// DWM 투명 (Windows 만)
		s.viewportClearColor= ImVec4(0.1f, 0.1f, 0.1f, 0.5f);		// Viewport 만 반투명
		return s;
	}
};

// 독립 인포그래픽 창 관리를 위한 인터페이스 및 관리자 클래스
class IInfoWindow {
public:
	bool m_isOpen = true;
	bool m_isLocked = false;

	std::string m_windowName;

	IInfoWindow(const std::string& name) : m_windowName(name) {}
	virtual ~IInfoWindow() = default;

	// 서브 클래스에서 오버라이드해서 스타일 독립화
	virtual SubAppStyle GetStyle() const { return SubAppStyle::Opaque(); }

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
	std::map<std::string, bool>								m_subAppLockStates;				// Lock / Unlock 상태
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

	void ApplyDWMTransparency(HWND hwnd);
};

