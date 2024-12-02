#pragma once
#include <wtypes.h>
#include <Windows.h>
#include <cstdint>


class WinApp
{
public:

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数

	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 終了
	void Finalize();

	bool ProcessMessage();

public:

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	HWND GetHwnd() const { return hwnd; }

	HINSTANCE GetHInstance()const { return wc.hInstance; }

private:

	// ウィンドウハンドル
	HWND hwnd = nullptr;
	// ウィンドウクラスの設定
	WNDCLASS wc{};
};

