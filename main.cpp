#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include<dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include<dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include<cmath>
#include "math.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"




extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};




// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	switch (msg) {




	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}
	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}
	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &result[0], sizeNeeded, NULL, NULL);
	return result;
}

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

#pragma region リソース作成の関数化

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

#pragma endregion

IDxcBlob* CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {
	Log(ConvertString(std::format(L"Begin CompileShader, path: {}, profile: {}\n", filePath, profile)));
	IDxcBlobEncoding* shaderSources = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSources);
	assert(SUCCEEDED(hr));
	DxcBuffer shaderSourcesBuffer;
	shaderSourcesBuffer.Ptr = shaderSources->GetBufferPointer();
	shaderSourcesBuffer.Size = shaderSources->GetBufferSize();
	shaderSourcesBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourcesBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));

	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConvertString(std::format(L"Compile Succeeded, path: {}, profile: {}\n", filePath, profile)));
	shaderSources->Release();
	shaderResult->Release();
	return shaderBlob;
}

#pragma region Transform変数を作る
Transform transform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
#pragma endregion

#pragma region DIscriptorHEap関数




ID3D12DescriptorHeap* CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}
#pragma endregion


#pragma region ImGuiネオン風
void SetGamingStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.WindowRounding = 5.3f;
	style.FrameRounding = 2.3f;
	style.ScrollbarRounding = 0;

	colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.7f, 0.7f, 0.7f, 0.65f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1f, 0.1f, 0.1f, 0.51f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.97f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.8f, 0.8f, 0.8f, 0.31f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.9f, 0.9f, 0.9f, 0.78f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.78f, 0.69f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.4f, 0.4f, 0.76f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.8f, 0.8f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.0f, 0.48f, 1.0f, 0.65f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.0f, 0.48f, 1.0f, 0.76f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.48f, 1.0f, 0.86f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.48f, 1.0f, 0.65f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.48f, 1.0f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.60f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.60f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.35f);
}

#pragma endregion

#pragma region TextureData読み込む



DirectX::ScratchImage LoadTexture(const std::string& filePath)
{
	// テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}

#pragma endregion

#pragma region テクスチャ作る
ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1; // サンプルカウント、固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元

	// 利用するHeapの設定。詳細に特殊な運用、02_04exで一般的なケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

	// Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読みだけ
		nullptr, // Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}
#pragma endregion

#pragma region TextureResourceにデータを転送する

void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages)
{
	// Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		// MipMapレベルを指定して各Imageを取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel), // 全領域へコピー
			nullptr, // データアドレス
			img->pixels, // ピクセルデータ
			UINT(img->rowPitch), // 行サイズ
			UINT(img->slicePitch)); // 1枚サイズ
		assert(SUCCEEDED(hr));
	}
}

#pragma endregion



int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);
#pragma region Windowの作成
	WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

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
	ShowWindow(hwnd, SW_SHOW);
#pragma endregion

#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

#pragma region Factoryの作成
	IDXGIFactory7* dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region Adapterの作成
	IDXGIAdapter4* useAdapter = nullptr;
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Log(ConvertString(std::format(L"Use Adapter: {}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);
#pragma endregion

#pragma region Deviceの作成
	ID3D12Device* device = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::format("Feature Level: {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");
#pragma endregion

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
		infoQueue->Release();
	}
#endif

#pragma region CommandQueue
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region CommandAllocater
	ID3D12CommandAllocator* commandAllocater = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocater));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region CommandListの作成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocater, nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));
#pragma endregion


#pragma region SwapChainの作成
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;
	swapChainDesc.Height = kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region DescriptorHeapの作成
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	// SRV用のヒープディスクリプタの数は128。SRVはShader内で読み取るものなので、ShaderVisibleはtrue
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);


	assert(SUCCEEDED(hr));
#pragma endregion








#pragma region SwapChainResource
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
#pragma endregion

#pragma region dxcCompiler

	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

#pragma region シグネチャの作成

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

#pragma region DescriptorRange

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算


#pragma endregion



#pragma region ルートパラメータの作成

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

#pragma endregion



#pragma region Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

#pragma endregion






	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion

#pragma region BlendState
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
#pragma endregion

#pragma region RasterizerStateの設定

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#pragma endregion

#pragma region Shaderをコンパイルする

	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);
#pragma endregion

#pragma region PSO生成する

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region VertexResource

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeof(VertexData) * 3;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertexResource = nullptr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region Material用のリソースを作る
	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Vector4));

	Vector4* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	*materialData = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
#pragma endregion

#pragma region WVPresourceを作る

	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
	Matrix4x4* wvpData = nullptr;
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	*wvpData = MakeIdentity4x4();
#pragma endregion





#pragma region VertexBufferViewを作成

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 3;
	vertexBufferView.StrideInBytes = sizeof(VertexData);
#pragma endregion



#pragma region 頂点データの更新
	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 左下
	vertexData[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[0].texcoord = { 0.0f, 1.0f };

	// 上
	vertexData[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertexData[1].texcoord = { 0.5f, 0.0f };

	// 右下
	vertexData[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[2].texcoord = { 1.0f, 1.0f };

#pragma endregion







#pragma region ViewportとScissor
	D3D12_VIEWPORT viewport{};
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;
#pragma endregion

#pragma region Imgui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	SetGamingStyle();  // ゲーミング風スタイルを適用
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

#pragma endregion



#pragma region Textureを読んで転送する
	// Textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	UploadTextureData(textureResource, mipImages);
#pragma endregion


#pragma region ShaderResourceView
	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// 後ほどImGuiが使うのでその次を使う
	textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// SRVの生成
	device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);

#pragma endregion




#pragma region メインループ
	MSG msg{};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("Color Picker");
			ImGui::ColorEdit4("Text Color With Flags", &materialData->x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			ImGui::End();

			// ImGuiウィンドウの作成
			ImGui::Begin("Transform Controls");
			ImGui::SliderFloat3("Position", &transform.translata.x, -5.0f, 5.0f);
			ImGui::SliderFloat3("Rotation", &transform.rotate.x, -180.0f, 180.0f);
			ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 2.0f);
			ImGui::End();

			ImGui::Render();



			/*transform.rotate.y += 0.03f;*/

			Transform cameraTransform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };
			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translata);
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translata);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			*wvpData = worldViewProjectionMatrix;

			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();



			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;


			commandList->ResourceBarrier(1, &barrier);

			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);



			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicsPipelineState);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
			// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。

			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);



			commandList->DrawInstanced(3, 1, 0, 0);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			// 実際のcommandListのImGuiの描画コマンドを描画
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);

			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			swapChain->Present(1, 0);

			fenceValue++;
			commandQueue->Signal(fence, fenceValue);
			if (fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			hr = commandAllocater->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocater, nullptr);
			assert(SUCCEEDED(hr));

		}
	}
#pragma endregion


	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#pragma region 解放処理

	textureResource->Release();
	srvDescriptorHeap->Release();
	wvpResource->Release();
	materialResource->Release();
	vertexResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescriptorHeap->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocater->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();
#ifdef _DEBUG
	debugController->Release();
#endif
	CloseWindow(hwnd);

	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}
	Log("Hello DirectX!\n");
	CoUninitialize();
	return 0;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* bufferResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferResource)
	);
	if (FAILED(hr)) {
		Log("Failed to create buffer resource\n");
		assert(SUCCEEDED(hr));
	}
	return bufferResource;
}
