#pragma once
#include "../../WinManager.h"

// =====================================
// 원형 버퍼  ImPlot PlotLine offset 파라미터 활용
// erase(begin()) O(N) 시프트 → 인덱스 래핑으로 대체
// =====================================
struct ScrollingBuffer {
	int			Capacity;	
	int			Size;				// 실제 채워진 원소 수 (Capacity 도달 전 성장 구간)
	int			Offset;			// 다음 쓰기 위치
	std::vector<float> Xs;
	std::vector<float> Ys;

	explicit ScrollingBuffer(int capacity = 600)
		: Capacity(capacity), Size(0), Offset(0)
	{
		Xs.resize(capacity, 0.0f);
		Ys.resize(capacity, 0.0f);
	}

	void AddPoint(float x, float y) {
		Xs[Offset] = x;
		Ys[Offset] = y;
		Offset = (Offset + 1) % Capacity;
		if (Size < Capacity)++Size;
	}

	void Clear() { Size = 0; Offset = 0; }

	// ImPlot::PlotLine 에 넘길 offset 인자
	// 버퍼가 꽉 찬 이후에는 가장 오래된 원소의 위치가 현재 offset
	int PlotOffset() const { return (Size < Capacity) ? 0 : Offset; }
};

// 플레이어 패달/휠 조작 입출력 플로팅 뷰 클래스
class PlayerInputGraphView : public IInfoWindow {
private:
	static const int MAX_DATA_POINTS = 600;
	ScrollingBuffer m_throttleBuf{ MAX_DATA_POINTS };
	ScrollingBuffer m_brakeBuf{ MAX_DATA_POINTS };
	ScrollingBuffer m_steerBuf{ MAX_DATA_POINTS };

public:
	PlayerInputGraphView() : IInfoWindow("Player Input Traces (60Hz)") {}
	void UpdateAndRender(const CarTelemetryData& telemetry, float sessionTime) override;
};
