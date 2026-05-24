#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include "SafeQueue.h"
#include "../Packets/F125Packets.h"

// =================================================================
// 프로그램 전역 공유 상태 — extern 선언부
// 실제 메모리 정의는 main.cpp 에 위치
// =================================================================
extern std::atomic<bool>									g_running;
extern SafeQueue<std::vector<char>>			g_packetQueue;
extern std::atomic<uint32_t>							g_packetCount;

extern std::mutex							g_metricsMutex;
extern LiveDisplayMetrics			g_liveMetrics;   // 디버그 프리뷰용

// 전방 선언
void UdpReceiverThread(int port);