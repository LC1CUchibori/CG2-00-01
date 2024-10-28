#include "WinApp.h"
#include <cstdint>

void WinApp::Initialize()
{
	HRESULT hr=CoInitializeEx(0, COINIT_MULTITHREADED);
#pragma region ウィンドウクラスの登録
	WNDCLASS wc{};

	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);
#pragma endregion

#pragma region ウィンドウサイズを決める
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
#pragma endregion

#pragma region ウィンドウ生成と表示
	HWND hwnd = CreateWindow(
		wc.lpszClassName,
		L"CG2",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);
#pragma endregion

	ShowWindow(hwnd, SW_SHOW);
}

void WinApp::Update()
{
}
