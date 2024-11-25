
#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
//#include <d3d12.h>
//#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
//#include <dxcapi.h>
#include <vector>
#include <numbers>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "math.h"
//#include <wrl.h>
#include "Input.h"
#include "DirectXCommon.h"

//#include "externals/imgui/imgui.h"
//#include "externals/imgui/imgui_impl_dx12.h"
//#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")


#pragma region Resource作成の関数化(CreateBufferResource)
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};

	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;

	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}
#pragma endregion

/*#pragma region ConvertString
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
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
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}
#pragma endregion*/

//#pragma region 出力ウィンドウに文字を出す
//void Log(const std::string& message) {
//	OutputDebugStringA(message.c_str());
//}
//#pragma endregion

#pragma region CompilerShader関数
IDxcBlob* CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler
) {
#pragma region hlslファイルを読む
	Logger::Log(StringUtility::ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
#pragma endregion

#pragma region Compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr"
	};

	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);

	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region 警告・エラーが出ていないか確認する
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Logger::Log(shaderError->GetStringPointer());

		assert(false);
	}
#pragma endregion

#pragma region Compile結果を受け取って返す
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	Logger::Log(StringUtility::ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

	return shaderBlob;
}
#pragma endregion

#pragma region Transform変数
Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f } };
#pragma endregion

#pragma region cameraTransform変数
Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };
#pragma endregion

#pragma region spriteTransform変数
Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
#pragma endregion

#pragma region UVTransform変数
Transform uvTransformSprite{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform modelTrasform
{
	{1.0f,1.0f,1.0f},
	{1.0f,1.0f,1.0f},
	{1.0f,1.0f,1.0f},
};

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker()
	{
#pragma region ReportLiveObjects
		Microsoft::WRL::ComPtr <IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
#pragma endregion
	}
};


#pragma region DescriptorHeapの作成関数
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
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

#pragma region LoadTexture
DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミニマップ着きのデータを返す
	return mipImages;
}
#pragma endregion

#pragma region CreateTextureResource
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata) {

	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resouceDesc{ };
	resouceDesc.Width = UINT(metadata.width);                             // Textureの幅
	resouceDesc.Height = UINT(metadata.height);                           // Textureの高さ
	resouceDesc.MipLevels = UINT16(metadata.mipLevels);                   // mipmapの数
	resouceDesc.DepthOrArraySize = UINT16(metadata.arraySize);            // 奥行きor配Texturaの配列数
	resouceDesc.Format = metadata.format;                                 // Textureのフォーマット
	resouceDesc.SampleDesc.Count = 1;                                     // サンプリクト。１固定。
	resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは２次元

	// 利用するHeapの設定。非常に特殊な運用。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;                        // 細かい設定を行う
	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // writeBackポリシーでCPUアクセス可能
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // プロセッサの近くに配置

	// Resouceの作成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,                   // Heapの設定
		D3D12_HEAP_FLAG_NONE,              // Heapの特殊設定。特になし
		&resouceDesc,                      // Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,    // 初回のResourceState, Textureは基本読むだけ
		nullptr,                           // Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource));          // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}
#pragma endregion

#pragma region UploadTextureData関数
[[nodiscard]]
Microsoft::WRL::ComPtr <ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr <ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresouces;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresouces);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresouces.size()));
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresouces.size()), subresouces.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}
#pragma endregion  




#pragma region LoadMaterial関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	// 1.2.必要な変数の宣言とファイルを開く
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	// 3.ファイルを読み、MaterialDataを構築
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}
#pragma endregion

#pragma region LoadObjFile関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	// 1. 中で必要となる変数の宣言
	ModelData modelData; // 構築するModalData
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line; // ファイルから読んだ1行を格納するもの
	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	// 3. 実際のファイルを読み込み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む
		// identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			position.x *= -1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			// 面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndeices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndeices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値をを取得して頂点を構築する
				Vector4 position = positions[elementIndeices[0] - 1];
				Vector2 texcoord = texcoords[elementIndeices[1] - 1];
				Vector3 normal = normals[elementIndeices[2] - 1];
				VertexData vertex = { position,texcoord,normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };
			}
			// 頂点を逆順で登録することで、回り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			// mateialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}
#pragma endregion


bool useMonsterBall = true;


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakCheck;


	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
	}





	Input* input_ = nullptr;
	WinApp* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;

	winApp = new WinApp();
	winApp->Initialize();
	input_ = new Input();
	input_->Initialize(winApp);
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);


	//#pragma region DescriptorHeapの生成
	//	// ディスクリプタヒープの生成
	//	// RTV用のヒープでディスクリプタの数は2。RTVはShader内で読むものではないので、ShaderVisibleはfalse
	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//	// SRV用のヒープでディスクリプタの数は128。SRVはShader内で読むものなので、ShaderVisibleはtrue
	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//	// DVS用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	//
	//	// ディスクリプタヒープが作れなかったので起動できない
	//	assert(SUCCEEDED(hr));
	//#pragma endregion

		/*const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);*/

		//GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);

#pragma region RootSignatureを生成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
#pragma endregion

#pragma region RootParameter
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;


	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
#pragma endregion

#pragma region Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
#pragma endregion

#pragma region InputLayoutの設定
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region InputLayoutの設定
	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);
#pragma endregion

#pragma region BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
#pragma endregion

#pragma region RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#pragma endregion

	//#pragma region ShaderをCompileする
	//	IDxcBlob* vertexShaderBlob = CompileShader(L"Resources/shaders/Object3d.VS.hlsl",
	//		L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	//	assert(vertexShaderBlob != nullptr);
	//
	//	IDxcBlob* pixelShaderBlob = CompileShader(L"Resources/shaders/Object3d.PS.hlsl",
	//		L"ps_6_0", dxcUtils, dxcCompiler, includeHandler)
	//
	//		;
	//	assert(pixelShaderBlob != nullptr);
	//#pragma endregion
	//
	//#pragma region PSOを生成する
	//	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	//	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	//	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	//		vertexShaderBlob->GetBufferSize() };
	//	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	//		pixelShaderBlob->GetBufferSize() };
	//	graphicsPipelineStateDesc.BlendState = blendDesc;
	//	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	//	// 書き込むRTVの情報
	//	graphicsPipelineStateDesc.NumRenderTar= 1;
	//	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	// 利用するトポロジ（形状）のタイプ。三角形
	//	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//	// どのように画面に色を打ち込むかの設定（気にしなくてよい）
	//	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	//	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//	// DepthStencilの設定
	//	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//	// 実際に生成
	//	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	//	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
	//		IID_PPV_ARGS(&graphicsPipelineState));
	//	assert(SUCCEEDED(hr));
	//#pragma endregion


#pragma region Resource
	const uint32_t kSubdivision = 16;

#pragma region VertexResourceを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(dxCommon->GetDevice(), sizeof(VertexData) * kSubdivision * kSubdivision * 6);
#pragma endregion

	//#pragma region DepthStencilTextureを作成
	//	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTexturResource(device, WinApp::kClientWidth, WinApp::kClientHeight);
	//#pragma endregion

#pragma region VertexBufferResourceを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(VertexData) * 6);
#pragma endregion



#pragma region Material用のResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = CreateBufferResource(dxCommon->GetDevice(), sizeof(Material));
	Material* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) };
	materialData->enableLighting = true;
#pragma endregion

#pragma region TransformationMatrix用のResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = CreateBufferResource(dxCommon->GetDevice(), sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixData = nullptr;
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
#pragma endregion


#pragma region TransfomationMatrixSprite用のResourceを作る
	// Sprite用のTransfomationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* transformationMatrixDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	// 単位行列を書き込んでおく
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
#pragma endregion


#pragma region Sprite用のリソース
	// Sprite用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(Material));
	Material* materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) };
	materialDataSprite->enableLighting = false;
#pragma endregion

	materialData->uvTransform = MakeIdentity4x4();
	materialDataSprite->uvTransform = MakeIdentity4x4();

	//////vertexResource頂点バッファーを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{ };
	////リソースの先頭のアドレスから使う
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	////使用するリソースのサイズは頂点分のサイズ
	//vertexBufferView.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 6;
	////1頂点当たりのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData);
	////頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	////書き込むためのアドレスを取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

#pragma region ModelDataを使う
	// モデルデータ読み込み
	ModelData modelData = LoadObjFile("resources", "axis.obj");
	// 頂点リソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexModelResource = CreateBufferResource(dxCommon->GetDevice(), sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // リソース先頭のアドレスから得る
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData); // 頂点あたりのサイズ
	// 頂点バッファリソースデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size()); // 頂点データをリソースにコピー

#pragma endregion


#pragma region 平行光源をShderで使う
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(DirectionalLighting));

	DirectionalLighting* directionalLightData = nullptr;

	directionalLightSprite->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;
#pragma endregion

#pragma region Index用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indeResourceSprite = CreateBufferResource(dxCommon->GetDevice(), sizeof(uint32_t) * 6);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indeResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
#pragma endregion

	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;
	indeResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;

	////経度分割1つ分の経度φd
	//const float kLonEvery = 2 * std::numbers::pi_v<float> / (float)kSubdivision;
	////緯度分割１つ分の緯度Θd
	//const float kLatEvery = std::numbers::pi_v<float> / (float)kSubdivision;
	////緯度方向に分割しながら線を描く
	//const float w = 2.0f;
	//for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
	//	float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;//θ
	//	for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
	//		//テクスチャ用のTexcood

	//		//書き込む最初の場所
	//		uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
	//		float lon = lonIndex * kLonEvery;//∮
	//		//基準点a
	//		vertexData[start].position.x = std::cosf(lat) * std::cosf(lon);
	//		vertexData[start].position.y = std::sinf(lat);
	//		vertexData[start].position.z = std::cosf(lat) * std::sinf(lon);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;

	//		//基準点b
	//		start++;
	//		vertexData[start].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon);
	//		vertexData[start].position.y = std::sinf(lat + kLatEvery);
	//		vertexData[start].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex + 1.0f) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;

	//		//基準点c
	//		start++;
	//		vertexData[start].position.x = std::cosf(lat) * std::cosf(lon + kLonEvery);
	//		vertexData[start].position.y = std::sinf(lat);
	//		vertexData[start].position.z = std::cosf(lat) * std::sinf(lon + kLonEvery);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex + 1.0f) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;

	//		//基準点c
	//		start++;
	//		vertexData[start].position.x = std::cosf(lat) * std::cosf(lon + kLonEvery);
	//		vertexData[start].position.y = std::sinf(lat);
	//		vertexData[start].position.z = std::cosf(lat) * std::sinf(lon + kLonEvery);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex + 1.0f) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;
	//		
	//		//基準点b
	//		start++;
	//		vertexData[start].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon);
	//		vertexData[start].position.y = std::sinf(lat + kLatEvery);
	//		vertexData[start].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex + 1.0f) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;
	//		
	//		//基準点d
	//		start++;
	//		vertexData[start].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery);
	//		vertexData[start].position.y = std::sinf(lat + kLatEvery);
	//		vertexData[start].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery);
	//		vertexData[start].position.w = w;
	//		vertexData[start].texcoord = { float(lonIndex + 1) / float(kSubdivision), 1.0f - float(latIndex + 1) / float(kSubdivision) };
	//		vertexData[start].normal.x = vertexData[start].position.x;
	//		vertexData[start].normal.y = vertexData[start].position.y;
	//		vertexData[start].normal.z = vertexData[start].position.z;
	//	}
	//}



	////vetexResourceSprite頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{ };
	//リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);
	//頂点リソースにデータを書き込む
	VertexData* vertexDataSprite = nullptr;
	//書き込むためのアドレスを取得
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//一個目
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };//左した
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };//左上
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };//右下
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	//二個目
	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };//左した
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };//右上
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	vertexDataSprite[4].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };//右下
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };
	vertexDataSprite[5].normal = { 0.0f,0.0f,-1.0f };
#pragma endregion

	//#pragma region Texture3を読む
	//	DirectX::ScratchImage mipImages3 = LoadTexture("Resources/monsterBall.png");
	//	const DirectX::TexMetadata& metadata3 = mipImages3.GetMetadata();
	//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResources3 = CreateTextureResource(device, metadata3);
	//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource3 = UploadTextureData(textureResources3, mipImages3, device, commandList);
	//#pragma endregion<
	//
	//#pragma region Texture2を読む
	//	//DirectX::ScratchImage mipImages2 = LoadTexture("Resources/uvChecker.png");
	//	DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
	//	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResources2 = CreateTextureResource(device, metadata2);
	//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResources2, mipImages2, device, commandList);
	//#pragma endregion
	//
	//#pragma region Texturを読む
	//	// Textureを読んで転送する
	//	DirectX::ScratchImage mipImages = LoadTexture("Resources/uvChecker.png");
	//	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device, metadata);
	//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);
	//#pragma endregion 
	//
	//#pragma region ShaderResourceView
	//	// metaDataを基にSRVの設定
	//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{  };
	//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	//
	//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//	srvDesc2.Format = metadata2.format;
	//	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);
	//
	//	// SRVを作成するDescriptHeap	の場所を決める
	//	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	//
	//	// 2枚目のSRVをつくる
	//	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	//	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	//
	//	// 先頭はImGuiが使っているのでその次を使う
	//	textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	// SRVの設定
	//	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);
	//
	//	device->CreateShaderResourceView(textureResources2.Get(), &srvDesc2, textureSrvHandleCPU2);
	//#pragma endregion 


		//ウィンドウのｘボタンが押されるまでループ
	MSG msg{};



	while (true) {
		// Windowsのメッセージ処理
		if (winApp->ProcessMessage()) {
			// ゲームループを抜ける
			break;
		}

		input_->Update();
		// 座標操作
		if (input_->PushKey(DIK_UP) || input_->PushKey(DIK_DOWN) || input_->PushKey(DIK_RIGHT) || input_->PushKey(DIK_LEFT))
		{
			if (input_->PushKey(DIK_UP)) {
				transformSprite.translata.y -= 1.0f;
			}
			else if (input_->PushKey(DIK_DOWN)) {
				transformSprite.translata.y += 1.0f;
			}
			if (input_->PushKey(DIK_RIGHT)) {
				transformSprite.translata.x += 1.0f;
			}
			else if (input_->PushKey(DIK_LEFT)) {
				transformSprite.translata.x -= 1.0f;
			}
		}

		if (input_->TriggerKey(DIK_UP) || input_->TriggerKey(DIK_DOWN) || input_->TriggerKey(DIK_RIGHT) || input_->TriggerKey(DIK_LEFT))
		{
			if (input_->TriggerKey(DIK_UP)) {
				transformSprite.translata.y -= 1.0f;
			}
			else if (input_->TriggerKey(DIK_DOWN)) {
				transformSprite.translata.y += 1.0f;
			}
			if (input_->TriggerKey(DIK_RIGHT)) {
				transformSprite.translata.x += 1.0f;
			}
			else if (input_->TriggerKey(DIK_LEFT)) {
				transformSprite.translata.x -= 1.0f;
			}
		}

		//ゲームの処理
#pragma region Transformを使ってCBufferを更新する
			//transform.rotate.y += 0.03f;
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translata);
		Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translata);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		Matrix4x4 worldProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		transformationMatrixData->WVP = worldProjectionMatrix;
		transformationMatrixData->World = worldMatrix;
#pragma endregion

#pragma region WVPMatrixを作って書き込む
		Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translata);
		Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
		Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
		Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
		transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
		transformationMatrixDataSprite->World = worldMatrix;
#pragma endregion

		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		uvTransformMatrix = Multiply(uvTransformMatrix, MakeRatateZMatrix(uvTransformSprite.rotate.z));
		uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translata));
		materialDataSprite->uvTransform = uvTransformMatrix;


		/*Matrix4x4 worldMatrixModel = MakeAffineMatrix(modelTrasform.scale,modelTrasform.rotate,modelTrasform.translata);
		Matrix4x4 cameraMatrixModel = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translata);
		Matrix4x4 viewMatrixModel = Inverse(cameraMatrixModel);
		Matrix4x4 projectionMatrixModel = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		Matrix4x4 worldProjectionMatrixModel = Multiply(worldMatrixModel, Multiply(viewMatrixModel, projectionMatrixModel));*/
		input_->Update();
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGuiウィンドウの作成
		ImGui::Begin("Transform Controls");
		if (ImGui::CollapsingHeader("SphereTransform")) {
			ImGui::ColorEdit4("Text Color With Flags", &materialData->color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			ImGui::SliderFloat3("Position", &transform.translata.x, -5.0f, 5.0f);// 移動
			ImGui::SliderFloat3("Rotation", &transform.rotate.x, -180.0f, 180.0f);// 回転変更
			ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 2.0f);// 大きさ変更
		}
		if (ImGui::CollapsingHeader("Light")) {
			ImGui::ColorEdit3("LightColor", &directionalLightData->color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			ImGui::SliderFloat3("Direction", &directionalLightData->direction.x, -10.0f, 10.0f);
			ImGui::SliderFloat("Intensity", &directionalLightData->intensity, -10.0f, 10.0f);
		}
		if (ImGui::CollapsingHeader("UVTransform")) {
			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translata.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
		}
		// 切り替え
		ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		// リセット
		if (ImGui::Button("Delete")) {
			transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
			directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
			directionalLightData->direction = { 0.0f,-1.0f,0.0f };
			directionalLightData->intensity = 1.0f;
		}
		ImGui::End();

		ImGui::Render();


		//#pragma region コマンドを積み込み確定させる
		//			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
		//
		//#pragma region TransitionBarrierを貼る
		//			D3D12_RESOURCE_BARRIER barrier{};
		//
		//			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//
		//			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//
		//			barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		//
		//			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		//
		//			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//
		//			commandList->ResourceBarrier(1, &barrier);
		//#pragma endregion
		//
		//			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
		//
		//			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
		//			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
		//
		//			// 描画先のRTVとDSVを設定する
		//			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		//			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
		//			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		//
		//			//描画用のDescriptHeap
		//			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHepes[] = { srvDescriptorHeap };
		//			commandList->SetDescriptorHeaps(1, descriptorHepes->GetAddressOf());
		//			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
		//
		//#pragma region コマンドを積む
		//			commandList->RSSetViewports(1, &viewport);
		//			commandList->RSSetScissorRects(1, &scissorRect);
		//			//RootSignatureを設定。POSに設定しているけどベット設定が必要
		//			commandList->SetGraphicsRootSignature(rootSignature.Get());
		//			commandList->SetPipelineState(graphicsPipelineState.Get());
		//#pragma endregion
		//
		//
		//#pragma region 三角形の描画
		//			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		//			//現状を設定。POSに設定しているものとはまた別。おなじ物を設定すると考えておけばいい
		//			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//			//wvp用のCBufferの場所を設定
		//			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
		//			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
		//			commandList->SetGraphicsRootConstantBufferView(3, directionalLightSprite->GetGPUVirtualAddress());
		//			//描画！
		//			//commandList->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);
		//			// ModelDataの描画
		//			commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
		//#pragma endregion
		//
		//			commandList->IASetIndexBuffer(&indexBufferViewSprite);
		//			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
		//
		//
		//#pragma region Spriteの描画
		//			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
		//			commandList->SetGraphicsRootConstantBufferView(0, transformationMatrixResourceSprite->GetGPUVirtualAddress());
		//			//TransFomationMatrixBufferの場所を設定
		//			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
		//			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
		//			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
		//			//描画！
		//			commandList->DrawInstanced(6, 1, 0, 0);
		//#pragma endregion
		//
		//
		//#pragma region 画面表示をできるようにする
		//			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//
		//			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		//
		//			commandList->ResourceBarrier(1, &barrier);
		//#pragma endregion
		//			hr = commandList->Close();
		//
		//			assert(SUCCEEDED(hr));
		//#pragma endregion
		//
		//#pragma region コマンドをキックする
		//			Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList };
		//			commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());
		//
		//			swapChain->Present(1, 0);
		//
		//#pragma region GPUにSignalを送る
		//			fenceValue++;
		//
		//			commandQueue->Signal(fence.Get(), fenceValue);
		//
		//#pragma endregion
		//
		//#pragma region Fenceの値を確認してGPUを待つ
		//			if (fence->GetCompletedValue() < fenceValue) {
		//				fence->SetEventOnCompletion(fenceValue, fenceEvent);
		//
		//				WaitForSingleObject(fenceEvent, INFINITE);
		//			}
		//#pragma endregion
		//
		//			hr = commandAllocator->Reset();
		//			assert(SUCCEEDED(hr));
		//
		//			hr = commandList->Reset(commandAllocator.Get(), nullptr);
		//			assert(SUCCEEDED(hr));
		//#pragma endregion

	}


	std::string str0{ "STRING!!!" };

	std::string str1{ std::to_string(10) };

#pragma region 解放処理

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	winApp->Finalize();

	delete input_;
	delete winApp;
	delete dxCommon;
#pragma endregion



	return 0;


}