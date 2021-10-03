#include "WinHelper.h"

HWND GetProgmanHWND()
{
	HWND progman_hwnd = FindWindowW(L"ProgMan", NULL);
	if (!progman_hwnd)
		return NULL;

	// Send 0x052C to Progman. This message directs Progman to spawn a 
	// WorkerW behind the desktop icons. If it is already there, nothing 
	// happens
	SendMessageTimeoutW(progman_hwnd, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

	return progman_hwnd;
}

BOOL CALLBACK WorkerSearcherCallback(HWND hwnd, LPARAM lParam)
{
	HWND* ret_hwnd = (HWND*)lParam;
	HWND def_view_hwnd = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);

	if (def_view_hwnd)
	{
		// Gets the WorkerW Window after the current one
		*ret_hwnd = FindWindowExW(NULL, hwnd, L"WorkerW", NULL);
	}

	return TRUE;
}

HWND GetWorkerHWND()
{
	HWND worker_hwnd = NULL;

	// get the last workerw window which have "SHELLDLL_DefView" child window
	EnumWindows(WorkerSearcherCallback, (LPARAM)& worker_hwnd);

	return worker_hwnd;
}