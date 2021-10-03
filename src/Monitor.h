#pragma once

#include "Windows.h"

#include "MediaPack.h"

#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>


class Monitor {
public:
	Monitor() = delete;
	Monitor(int monitor_id, RECT rect, bool is_primary);

	~Monitor();

	Monitor(const Monitor& obj);
	Monitor& operator=(const Monitor& obj) = delete;
	Monitor(Monitor&& obj) noexcept;
	Monitor& operator=(Monitor&& obj) = delete;

	bool DrawFrame(Frame& frame);

	bool GetResolution(long& width, long& height);

	static bool Initialize();
	static void Finilize();

	static std::deque<Monitor> monitors;

	const int monitor_id;
	const bool is_primary;

private:
	// MonitorEnumProc callback function
	static BOOL WINAPI MonitorEnumProc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data);

	static bool is_initialized;
	static std::timed_mutex worker_lock;

	// ProgMan HWND
	static HWND progman_hwnd;

	// currently visible WorkerW params
	static HWND worker_hwnd;
	static HDC worker_hdc;
	static RECT worker_rect;

	// variables for drawing in WorkerW for each monitor
	HDC drawing_hdc = NULL;
	HBITMAP drawing_bitmap = NULL;
	BITMAPINFO bitmap_info;
	uint8_t* drawing_pixels = nullptr;
	size_t drawing_pixels_size = 0;

	// monitor params
	RECT monitor_rect;
	long monitor_width = 0, monitor_height = 0;
	long x_offset = LONG_MAX, y_offset = LONG_MAX;
};