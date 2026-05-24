#include "PlayerInputGraphView.h"

// [서브 APP] 플레이어 입력 그래프 뷰 구현
void PlayerInputGraphView::UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) {
	// 원형 쓰기, 세 버퍼 모두 동일 time 축 공유
	m_throttleBuf.AddPoint(sessionTime, telemetry.m_throttle);
	m_brakeBuf.AddPoint(sessionTime, telemetry.m_brake);
	m_steerBuf.AddPoint(sessionTime, telemetry.m_steer);

	// ImGui Docking Brangh -> 일반 ImGui::Begin 윈도우로 구현
	// 사용자가 창을 드래그하면 자동으로 OS 창으로 분리됨
	ImGui::SetNextWindowSize(ImVec2(800,350), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin(m_windowName.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	ImVec2 dynamicPlotSize = ImGui::GetContentRegionAvail();
	
	if (ImPlot::BeginPlot("##InputTracesPlot", dynamicPlotSize)) 
	{
		ImPlot::SetupAxes("Session Time (s)", "Value");
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1f, 1.1f, ImGuiCond_Always);

		const float X_AXIS_RANGE = 10.0f;
		float x_max = sessionTime;
		float x_min = (x_max > X_AXIS_RANGE) ? (x_max - X_AXIS_RANGE) : 0.0f;
		// 데이터가 아예 없는 초기 상태여도 축이 흔들리지 않고 미리 선언된 범위로 고정
		ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);

		ImPlotSpec spec;
		spec.LineColor= ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		spec.LineWeight = 2.0f;
		spec.Offset = m_throttleBuf.Offset;
		spec.Stride = sizeof(float);
		ImPlot::PlotLine("Throttle",
			m_throttleBuf.Xs.data(),
			m_throttleBuf.Ys.data(),
			m_throttleBuf.Size, spec);

		spec.LineColor= ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		spec.Offset = m_brakeBuf.Offset;
		ImPlot::PlotLine("Brake",
			m_brakeBuf.Xs.data(),
			m_brakeBuf.Ys.data(),
			m_brakeBuf.Size, spec);

		spec.LineColor= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		spec.LineWeight = 1.5f;
		spec.Offset = m_steerBuf.Offset;
		ImPlot::PlotLine("Steering",
			m_steerBuf.Xs.data(),
			m_steerBuf.Ys.data(),
			m_steerBuf.Size, spec);

		ImPlot::EndPlot();
	}
	ImGui::End();
}