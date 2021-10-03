#include "Monitor.h"
#include "WinHelper.h"


std::deque<Monitor> Monitor::monitors;
bool Monitor::is_initialized = false;

HWND Monitor::progman_hwnd = NULL;
HWND Monitor::worker_hwnd = NULL;
HDC Monitor::worker_hdc = NULL;
RECT Monitor::worker_rect = {0, 0, 0, 0};
std::timed_mutex Monitor::worker_lock;

Monitor::Monitor(int monitor_id, RECT rect, bool is_primary) :
	monitor_id(monitor_id), is_primary(is_primary)
{
	if (!is_initialized)
		throw std::exception("WorkerW isn't initialized.");

	if (rect.right - rect.left <= 0 || rect.bottom - rect.top <= 0)
		throw std::exception("Wrong monitor coordinates.");


	monitor_rect = rect;
	monitor_width = rect.right - rect.left;
	monitor_height = rect.bottom - rect.top;

	// calculate offset to draw frame correctly
	x_offset = monitor_rect.left - worker_rect.left;
	y_offset = monitor_rect.top - worker_rect.top;

	drawing_hdc = CreateCompatibleDC(worker_hdc);
	if (!drawing_hdc)
		throw std::exception("Can't create compatible DC.");

	// initialize bitmap and allocate memory to directly write pixels to
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biWidth = monitor_width;
	bitmap_info.bmiHeader.biHeight = -monitor_height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 24;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	bitmap_info.bmiHeader.biSizeImage = monitor_width * monitor_height * 3;

	// drawing_pixels will be freed when DeleteObject will be called
	drawing_bitmap = CreateDIBSection(drawing_hdc, &bitmap_info, DIB_PAL_COLORS, reinterpret_cast<void**>(&drawing_pixels), NULL, 0);
	if (!drawing_bitmap) {
		DeleteDC(drawing_hdc);
		throw std::exception("Error while allocating DIB section.");
	}

	drawing_pixels_size = (size_t)monitor_width * monitor_height * 3;
}

Monitor::~Monitor()
{
	if (drawing_bitmap)
		DeleteObject(drawing_bitmap);

	if (drawing_hdc)
		DeleteDC(drawing_hdc);
}

Monitor::Monitor(const Monitor& obj) : 
	bitmap_info(obj.bitmap_info), monitor_rect(obj.monitor_rect), monitor_id(obj.monitor_id), is_primary(obj.is_primary)
{
	drawing_hdc = CreateCompatibleDC(worker_hdc);
	if (!drawing_hdc)
		throw std::exception("Can't create compatible DC.");

	drawing_bitmap = CreateDIBSection(drawing_hdc, &bitmap_info, DIB_PAL_COLORS, (void**)& drawing_pixels, NULL, 0);
	if (!drawing_bitmap) {
		DeleteDC(drawing_hdc);
		throw std::exception("Error while allocating DIB section.");
	}
}

Monitor::Monitor(Monitor&& obj) noexcept :
	bitmap_info(obj.bitmap_info), monitor_rect(obj.monitor_rect), monitor_id(obj.monitor_id), is_primary(obj.is_primary)
{
	drawing_hdc = obj.drawing_hdc;
	obj.drawing_hdc = NULL;

	drawing_bitmap = obj.drawing_bitmap;
	obj.drawing_bitmap = NULL;

	drawing_pixels = obj.drawing_pixels;
	obj.drawing_pixels = nullptr;
}

bool Monitor::DrawFrame(Frame& frame)
{
	if (frame.frame_buf == nullptr || frame.crop_width == 0 || frame.crop_height == 0)
		return false;

	// try to lock mutex for 5 sec
	if (!worker_lock.try_lock_for(std::chrono::seconds(5)))
		return false;

	std::lock_guard<std::timed_mutex> locker(worker_lock, std::adopt_lock_t());


	// change bitmap to our
	HGDIOBJ old_bitmap = SelectObject(drawing_hdc, drawing_bitmap);
	if (!old_bitmap)
		return false;


	// copy frame pixels to DIB
	int copy_code = 0;
	for (long f = frame.y_offset, t = 0; f < (frame.y_offset + frame.crop_height); ++f, ++t) {
		copy_code = memcpy_s(
			drawing_pixels + monitor_width * t * 3, drawing_pixels_size - monitor_width * t * 3,
			frame.frame_buf + frame.linesize * f + frame.x_offset * 3, frame.crop_width * 3
		);

		if (copy_code) {
			SelectObject(drawing_hdc, old_bitmap);
			return false;
		}
	}

	// stretching the original frame to monitor
	copy_code = StretchBlt(
		worker_hdc,										// destination HDC
		x_offset, y_offset,								// upper left corner of the destination rectangle
		monitor_width, monitor_height,					// width and heigh of the destination rectangle
		drawing_hdc,									// source HDC
		0, 0,	 										// upper left corner of the source rectangle
		frame.crop_width, frame.crop_height,	 		// width and heigh of the source rectangle
		SRCCOPY
	);

	if (!copy_code) {
		SelectObject(drawing_hdc, old_bitmap);
		return false;
	}

	SelectObject(drawing_hdc, old_bitmap);

	return true;
}

bool Monitor::GetResolution(long& width, long& height)
{
	if (monitor_width == 0 || monitor_height == 0)
		return false;

	width = monitor_width;
	height = monitor_height;

	return true;
}

bool Monitor::Initialize()
{
	BOOL ret_code;

	// get all worker's params we need

	progman_hwnd = GetProgmanHWND();
	if (!progman_hwnd)
		return false;

	worker_hwnd = GetWorkerHWND();
	if (!worker_hwnd)
		return false;

	worker_hdc = GetDC(worker_hwnd);
	if (!worker_hdc)
		return false;

	ret_code = GetWindowRect(worker_hwnd, &worker_rect);
	if (!ret_code)
		return false;

	is_initialized = true;

	// get the list of all monitors
	HDC common_dc = CreateDCW(L"DISPLAYS", NULL, NULL, NULL);
	BOOL ret = EnumDisplayMonitors(common_dc, NULL, (MONITORENUMPROC)MonitorEnumProc, NULL);
	DeleteDC(common_dc);

	return true;
}

void Monitor::Finilize()
{
	if (!is_initialized)
		return;

	if (worker_hdc)
		ReleaseDC(worker_hwnd, worker_hdc);

	is_initialized = false;

	return;
}

BOOL Monitor::MonitorEnumProc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data)
{
	MONITORINFOEX monitor_info;
	monitor_info.cbSize = sizeof(MONITORINFOEX);
	BOOL code = GetMonitorInfoW(monitor, &monitor_info);
	if (!code)
		return TRUE;

	try {
		monitors.emplace_back(monitors.size(), monitor_info.rcMonitor, monitor_info.dwFlags & MONITORINFOF_PRIMARY);
	}
	catch (std::exception& exception) {
		std::cout << exception.what() << std::endl;
		return TRUE;
	}

	return TRUE;
}
