#pragma once

#define DIRECTINPUT_VERSION  0x0800
#include <dinput.h>
#include <wrl.h>
#include "WinApp.h"

// 入力
class Input
{
public: // メンバ関数
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
    // 初期化
	void Initialize(WinApp*winApp);
	// 更新
	void Update();

	bool PushKey(BYTE keyNumber);

	bool TriggerKey(BYTE keyNumber);
private: // メンバ変数
// キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard;

	ComPtr<IDirectInput8> directInput;

	BYTE key[256] = {};
	BYTE keyPre[256] = {};

	// WindowsAPI
	WinApp* winApp = nullptr;
};