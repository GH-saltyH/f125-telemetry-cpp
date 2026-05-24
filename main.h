#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <fstream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <map>

#include <memory>
#include <string>
#include <d3d11.h>
#include <tchar.h>

// Dear ImGui / ImPlot
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "d3d11.lib")       

// 분리된 코어 모듈
#include "Packets/F125Packets.h"
#include "Core/SafeQueue.h"	
#include "Core/GlobalState.h"

