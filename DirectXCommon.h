#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include <array>

#include <cassert>
#include <dxcapi.h>

#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include <format>
#include "externals/DirectXTex/DirectXTex.h"

// DirectX基盤
class DirectXCommon
{
public: // メンバ関数
	//DirectXCommon(){};

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
	// 各種デスクリプタヒープの生成
	void DescriptorGenerate();
	// RTVの初期化
	void RTVInitialize();
	// 深度ステンシルビューの初期化
	void DSVInitialize();
	// フェンス生成
	void FenceInitialize();
	// ビューポート矩形の初期化
	void ViewPortIntialize();
	// シザリング矩形の初期化
	void ScissourInitialize();
	// DXCコンパイラの生成
	void DXCCompilerGenerate();
	// ImGuiの初期化
	void ImGuiInitialize();
	// 描画前処理
	void PreDraw();
	// 描画後処理
	void PostDraw();

	ID3D12Resource* CreateDepthStencilTexturResource(ID3D12Device* device, int32_t width, int32_t height);

	ID3D12Device* GetDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList()const { return commandList.Get(); }

	//ID3D12Resource* GetTexture() { return texture.Get(); }


	const D3D12_DEPTH_STENCIL_DESC& GetDepthStencilDesc() const {
		return depthStencilDesc;
	}

	ID3D12Resource* GetTextureResource3() const { return textureResources3; }

	ID3D12Resource* GetTextureResource2() const { return textureResources2; }

	ID3D12Resource* GetTextureResource() const { return textureResources; }
	
	

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath,const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipimages);

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

#pragma region DescriptorHeapの作成関数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType;
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
		assert(SUCCEEDED(hr));
		return descriptorHeap;
	}
#pragma endregion

#pragma region CPUとGPUの関数化
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descrptorHeap, uint32_t descriptorSize, uint32_t index)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descrptorHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += (descriptorSize * index);
		return handleGPU;
	}
#pragma endregion


private:

	uint32_t descriptorSizeSRV;
	uint32_t descriptorSizeRTV;
	uint32_t descriptorSizeDSV;

	HANDLE fenceEvent;
	uint64_t fenceValue = 0;

	// DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	//Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	// DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// infoQueue
	Microsoft::WRL::ComPtr<ID3D12InfoQueue>infoQueue;
	// CommandQueue
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	// CommandAllocator
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	// CommandList
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	// SwapChain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	// SwapChainResources
	//Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];
	// rtvDescriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	// srvDescriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	// dsvDescriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	// depthStencilResourece
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

	// fence
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	
	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter;

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource;


	IDxcIncludeHandler* includeHandler = nullptr;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};

	D3D12_VIEWPORT viewport{};

	D3D12_RECT scissorRect{};

	D3D12_HEAP_PROPERTIES heapProperties{  };

	D3D12_CLEAR_VALUE depthClerValue{};

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	D3D12_RESOURCE_DESC resourceDesc{};

	D3D12_INFO_QUEUE_FILTER filter{};

	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;

	ID3D12Resource* textureResources3;
	ID3D12Resource* textureResources2;
	ID3D12Resource* textureResources;

	// SRV
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);
	// RTV
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);
	// DSV
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

	// WindowsAPI
	WinApp* winApp = nullptr;
};
