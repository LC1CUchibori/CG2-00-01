#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"

// DirectX基盤
class DirectXCommon
{
public: // メンバ関数
	// 初期化
	void Initialize(WinApp*winApp);
	// デバイスの初期化
	void DeviceInitialize();
	// コマンド関連の初期化
	void CommandInitialize();
	// スワップチェーンの生成
	void SwapChainGenerate();
	// 深度バッファの生成
	void DepthBufferGenerate();



private:
	// DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	// DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// CommandQueue
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	// CommandAllocator
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	// CommandList
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	// SwapChain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;

	// WindowsAPI
	WinApp* winApp = nullptr;
};

