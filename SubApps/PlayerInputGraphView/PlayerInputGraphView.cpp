#include "PlayerInputGraphView.h"
#include "imgui_internal.h"

// [서브 APP] 플레이어 입력 그래프 뷰 구현
void PlayerInputGraphView::UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) {
	// 원형 쓰기, 세 버퍼 모두 동일 time 축 공유
	m_throttleBuf.AddPoint(sessionTime, telemetry.m_throttle);
	m_brakeBuf.AddPoint(sessionTime, telemetry.m_brake);
	m_steerBuf.AddPoint(sessionTime, telemetry.m_steer);

	// ══════════════════════════
	// 윈도우 설정
	// ══════════════════════════
	SubAppStyle style = GetStyle();

	ImGui::SetNextWindowSize(ImVec2(800,350), ImGuiCond_FirstUseEver);

	// 윈도우 플래그 조합
	ImGuiWindowFlags flags = style.windowFlags;

	// Lock 상태 : 이동 불가
	if (m_isLocked) {
		flags |= ImGuiWindowFlags_NoMove;
	}
	else
	{
		flags &= ~ImGuiWindowFlags_NoMove;
	}

	if (!style.useWindowBackground) {
		flags |= ImGuiWindowFlags_NoBackground;		// 윈도우 배경 제거
	}
	if (style.hideTitleBar) {
		flags |= ImGuiWindowFlags_NoTitleBar;
	}

	// WindowManager 가 스타일을 적용했으므로 플래그만 처리함
	if (!ImGui::Begin(m_windowName.c_str(), &m_isOpen, flags))
	{
		ImGui::End();
		return;
	}

	// ══════════════════════════
	// Drag 이동 처리 (Unlock) - 전체 창 영역에서 드래그 이벤트 수행
	//  ImPlot 보다 먼저 그려야 플로팅 요소보다 위에서 점유 할 수 있음
	// ══════════════════════════
	if (!m_isLocked) {
		// 창 위치/크기
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();;

		ImRect bb(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y));
		ImGuiID id = ImGui::GetID("##drag_full_window");

		if (ImGui::ItemAdd(bb, id)) {
			bool hovered, held;
			bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_MouseButtonLeft);

			if (held && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				OutputDebugStringW(L"OnDrag\n");
				ImVec2 delta = ImGui::GetIO().MouseDelta;
				ImGui::SetWindowPos(ImVec2(winPos.x + delta.x, winPos.y + delta.y));
			}
		}
	}

	// ══════════════════════════
	// 그래프 플로팅
	// ══════════════════════════
	
	// Plot 색상 설정
	ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.05f, 0.05f, 0.05f, 0.8f));
	ImPlot::PushStyleColor(ImPlotCol_PlotBorder, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

	ImVec2 dynamicPlotSize = ImGui::GetContentRegionAvail();

	if (ImPlot::BeginPlot("##InputTracesPlot", dynamicPlotSize,
		ImPlotFlags_NoTitle)) 
	{
		// Y 축 설정 (0-1), 노 데코
		//ImPlot::SetupAxes("Session Time (s)", "Value");
		ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoGridLines);
		ImPlot::SetupAxisLimits(ImAxis_Y1, -0.03f, 1.03f, ImGuiCond_Always);

		// X 축 설정 (0-1), 10초 슬라이딩 윈도우
		const float X_AXIS_RANGE = 10.0f;
		float x_max = (sessionTime > X_AXIS_RANGE) ? sessionTime : X_AXIS_RANGE;
		float x_min = (x_max > X_AXIS_RANGE) ? (x_max - X_AXIS_RANGE) : 0.0f;

		ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoGridLines);
		ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);

		// ══════════════════════════
		// 라인 플로팅
		// ══════════════════════════
		ImPlotSpec spec;

		// Throttle (녹색)
		spec.LineColor= ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		spec.LineWeight = 2.0f;
		spec.Offset = m_throttleBuf.Offset;
		spec.Stride = sizeof(float);
		ImPlot::PlotLine("Throttle",
			m_throttleBuf.Xs.data(),
			m_throttleBuf.Ys.data(),
			m_throttleBuf.Size, spec);

		// Brake (적색)
		spec.LineColor= ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		spec.Offset = m_brakeBuf.Offset;
		ImPlot::PlotLine("Brake",
			m_brakeBuf.Xs.data(),
			m_brakeBuf.Ys.data(),
			m_brakeBuf.Size, spec);

		// 스티어 Y 축 정규화 (0-1)
		if (m_steerNormalizedCache.size() != m_steerBuf.Capacity) {
			m_steerNormalizedCache.resize(m_steerBuf.Capacity);
		}
		for (int i = 0; i < m_steerBuf.Capacity; i++)
		{
			m_steerNormalizedCache[i] = (m_steerBuf.Ys[i] + 1.0f) * 0.5;
		}

		// Steering (흰색)
		spec.LineColor= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		spec.LineWeight = 1.5f;
		spec.Offset = m_steerBuf.Offset;
		ImPlot::PlotLine("Steering",
			m_steerBuf.Xs.data(),
			m_steerNormalizedCache.data(),
			m_steerBuf.Size, spec);

		ImPlot::EndPlot();
	}

	// 스타일 복구 (pop)
	ImPlot::PopStyleColor(2);


	// ══════════════════════════
	// Drag 상태 오버레이 (어두운 음영 + 이동가능  메시지 출력부)
	// 순서 중요:  플로팅 보다 나중에 그려야 함
	// ══════════════════════════

	if (!m_isLocked) {
		// 창 위치/크기
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// 1. 반투명 오버레이 (RGB 0.1, 0.1, 0.1) (0.5 alpha)
		//	Plot 위를 덮는 전체 창 오버레이
		drawList->AddRectFilled(
			winPos,
			ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
			IM_COL32(25, 25, 25, 128)
		);

		// 2. "드래그하여 이동" 텍스트 ( 창 중앙에)
		const char* dragText = "Click to Drag";
		ImVec2 textSize = ImGui::CalcTextSize(dragText);
		ImVec2 textPos = ImVec2(
			winPos.x + (winSize.x - textSize.x) * 0.5f,
			winPos.y + (winSize.y - textSize.y) * 0.5
		);
		drawList->AddText(textPos, IM_COL32(255, 255, 255, 204), dragText);		// Alpha 0.8
	}


	ImGui::End();
}
