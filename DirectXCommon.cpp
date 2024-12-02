#include "DirectXCommon.h"


#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp*winApp)
{
	// NULL検出
	assert(winApp);

	// メンバ変数に記録
	this->winApp = winApp;

	DeviceInitialize();
	CommandInitialize();
	SwapChainGenerate();
	DepthBufferGenerate();
	DescriptorGenerate();
	RTVInitialize();
	DSVInitialize();
	FenceInitialize();
	ViewPortIntialize();
	ScissourInitialize();
	DXCCompilerGenerate();
	ImGuiInitialize();
}

void DirectXCommon::DeviceInitialize()
{
	HRESULT hr;

#ifdef _DEBUG

#pragma region DebugLayer
	debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#pragma endregion

#endif


#pragma region DXGIFactoryの生成
	//Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory = nullptr;

	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region 使用するアダプタを決定
	useAdapter = nullptr;

	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Logger::Log(StringUtility::ConvertString((L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}

	assert(useAdapter != nullptr);
#pragma endregion

#pragma region D3D12Deviceの生成
	//Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {
			Logger::Log(("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}

	assert(device != nullptr);
	Logger::Log("Complete create D3D12Device!!!\n");
#pragma endregion


#ifdef _DEBUG

#pragma region エラー・警告時に停止
	infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

#pragma region エラーと警告の抑制
		D3D12_MESSAGE_ID denyIds[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		infoQueue->PushStorageFilter(&filter);
#pragma endregion

	}
#endif
#pragma endregion
}

void DirectXCommon::CommandInitialize()
{
	HRESULT hr;

#pragma region コマンドキューの生成
	 commandQueue = nullptr;
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region コマンドアロケータの生成
	commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));

	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region コマンドリストの生成
	commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr,
		IID_PPV_ARGS(&commandList));

	assert(SUCCEEDED(hr));
#pragma endregion
}

void DirectXCommon::SwapChainGenerate()
{
	HRESULT hr;

#pragma region SwapChainの生成
	swapChain = nullptr;
	swapChainDesc.Width = WinApp::kClientWidth;
	swapChainDesc.Height = WinApp::kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region SwapChainからResourceを引っ張ってくる
	//swapChainResources[2] = { nullptr };

	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));
#pragma endregion
}

void DirectXCommon::DepthBufferGenerate()
{
		resourceDesc.Width = WinApp::kClientWidth;//Textureの幅
		resourceDesc.Height = WinApp::kClientHeight;//Textureの高さ
		resourceDesc.MipLevels = 1;//mipmapの数
		resourceDesc.DepthOrArraySize = 1;//奥行きor配列Texturの配列数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DetpthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1;//サンプリング。１固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthSrencilとして使う通知

		//利用するheapの設定
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る
		//深度値のクリア設定
		depthClerValue.DepthStencil.Depth = 1.0f;//最大値
		depthClerValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーアット。Resource合わせる
		//Resourceの生成
		resource = nullptr;
		HRESULT hr = device->CreateCommittedResource(
			&heapProperties,//Heapの設定
			D3D12_HEAP_FLAG_NONE,//Heapの特殊設定。特になし
			&resourceDesc,//Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度値を書き込む状態のしておく
			&depthClerValue,//Clear最適値
			IID_PPV_ARGS(&depthStencilResource));//作成するResourceポインタへのポインタ
		assert(SUCCEEDED(hr));

		// DepthStencilStateの設定
		// Deothの機能を有効化する
		depthStencilDesc.DepthEnable = true;
		// 書き込みします
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		// 比較関数はLessEqual
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

}

void DirectXCommon::DescriptorGenerate()
{
	uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


#pragma region DescriptorHeapの生成
	// ディスクリプタヒープの生成
	// RTV用のヒープでディスクリプタの数は2。RTVはShader内で読むものではないので、ShaderVisibleはfalse
	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
#pragma endregion
}

void DirectXCommon::RTVInitialize()
{
#pragma region RTVを作る
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < 2; ++i)
	{
		rtvHandles[0] = rtvStartHandle;
		device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
		
		rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
	}
#pragma endregion
}

void DirectXCommon::DSVInitialize()
{
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//Format
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dtexture
	//DSHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::FenceInitialize()
{
	HRESULT hr;
	fence = nullptr;
	
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::ViewPortIntialize()
{
	viewport.Width = WinApp::kClientWidth;
	viewport.Height = WinApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXCommon::ScissourInitialize()
{
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;
}

void DirectXCommon::DXCCompilerGenerate()
{
	HRESULT hr;
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::ImGuiInitialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void DirectXCommon::PreDraw()
{
#pragma region バックバッファの番号取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
#pragma endregion

#pragma region TransitionBarrierを貼る
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	commandList->ResourceBarrier(1, &barrier);

	//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
;
#pragma endregion

#pragma region RTVとDSVを設定する
	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
#pragma endregion
	
#pragma region 画面全体の色をクリア
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
#pragma endregion

#pragma region 画面全体の深度をクリア
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#pragma endregion

#pragma region SRV用のデスクリプタヒープを指定
	//描画用のDescriptHeap
	ID3D12DescriptorHeap *descriptorHepes[] = { srvDescriptorHeap.Get()};
	commandList->SetDescriptorHeaps(1, descriptorHepes);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#pragma endregion

#pragma region ビューポート領域の設定
	commandList->RSSetViewports(1, &viewport);
#pragma endregion

#pragma region シザー矩形の設定
	commandList->RSSetScissorRects(1, &scissorRect);
#pragma endregion
}

void DirectXCommon::PostDraw()
{
#pragma region バックバッファの番号取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
#pragma endregion

#pragma region リソースバリアで表示状態に変更
	D3D12_RESOURCE_BARRIER barrier{};

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	commandList->ResourceBarrier(1, &barrier);
#pragma endregion

#pragma region グラフィックスコマンドをクローズ
	hr = commandList->Close();

	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region GPUコマンドの実行
	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());
#pragma endregion

#pragma region GPU画面の交換を通知
	swapChain->Present(1, 0);
#pragma endregion

#pragma region Fenceの値を更新
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);

		WaitForSingleObject(fenceEvent, INFINITE);
	}
#pragma endregion

#pragma region コマンドキューにシグナルに送る
	commandQueue->Signal(fence.Get(), fenceValue);
#pragma endregion

#pragma region コマンド完了待ち
	fenceValue++;
#pragma endregion

#pragma region コマンドアロケーターのリセット
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region コマンドリストのリセット
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
#pragma endregion
}


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeRTV, index);
}
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeRTV, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeDSV, index);
}
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeDSV, index);
}


