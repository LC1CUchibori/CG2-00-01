
#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <vector>
#include <numbers>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <wrl.h>
#include "Input.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"

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

	WinApp* winApp = nullptr;
	winApp = new WinApp();
	winApp->Initialize();

	Input* input_ = nullptr;
	input_ = new Input();
	input_->Initialize(winApp);

	DirectXCommon* dxCommon = nullptr;
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	SpriteCommon* spriteCommon = nullptr;
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	Sprite* sprite = nullptr;
	sprite = new Sprite();
	sprite->Initialize(spriteCommon);


	#pragma region DescriptorHeapの生成
		// ディスクリプタヒープの生成
		// RTV用のヒープでディスクリプタの数は2。RTVはShader内で読むものではないので、ShaderVisibleはfalse
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(dxCommon->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
		// SRV用のヒープでディスクリプタの数は128。SRVはShader内で読むものなので、ShaderVisibleはtrue
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(dxCommon->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
		// DVS用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(dxCommon->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	
		// ディスクリプタヒープが作れなかったので起動できない
		//assert(SUCCEEDED(hr));
	#pragma endregion

		const uint32_t descriptorSizeSRV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const uint32_t descriptorSizeRTV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		const uint32_t descriptorSizeDSV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dxCommon->GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 0);







#pragma region Resource
	const uint32_t kSubdivision = 16;

#pragma region VertexResourceを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource =dxCommon->CreateBufferResource(sizeof(VertexData) * kSubdivision * kSubdivision * 6);
#pragma endregion

	#pragma region DepthStencilTextureを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource =dxCommon->CreateDepthStencilTexturResource(dxCommon->GetDevice(),WinApp::kClientWidth, WinApp::kClientHeight);
	#pragma endregion




#pragma region Material用のResourceを作る
	Material* materialData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) };
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();
#pragma endregion

#pragma region TransformationMatrix用のResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource =dxCommon-> CreateBufferResource(sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixData = nullptr;
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
#pragma endregion

#pragma region 平行光源をShderで使う
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightSprite =dxCommon-> CreateBufferResource(sizeof(DirectionalLighting));
#pragma endregion
	DirectionalLighting* directionalLightData = nullptr;

	directionalLightSprite->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;




	////vertexResource頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{ };
	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 6;
	//1頂点当たりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

#pragma region ModelDataを使う
	// モデルデータ読み込み
	ModelData modelData = LoadObjFile("resources", "axis.obj");
	// 頂点リソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexModelResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファビューを作成する
	vertexBufferView;
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // リソース先頭のアドレスから得る
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData); // 頂点あたりのサイズ
	// 頂点バッファリソースデータを書き込む
	 vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size()); // 頂点データをリソースにコピー

#pragma endregion
	

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

	//		//基準点
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



	

	#pragma region Texture3を読む
		DirectX::ScratchImage mipImages3 =dxCommon->LoadTexture("Resources/monsterBall.png");
		const DirectX::TexMetadata& metadata3 = mipImages3.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResources3 = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata3);
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource3 = dxCommon->UploadTextureData(textureResources3.Get(), mipImages3);
	#pragma endregion<
	
	#pragma region Texture2を読む
		//DirectX::ScratchImage mipImages2 = LoadTexture("Resources/uvChecker.png");
		DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(modelData.material.textureFilePath);
		const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResources2 = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata2);
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = dxCommon->UploadTextureData(textureResources2.Get(), mipImages2);
	#pragma endregion
	
	#pragma region Texturを読む
		// Textureを読んで転送する
		DirectX::ScratchImage mipImages = dxCommon->LoadTexture("Resources/uvChecker.png");
		const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata);
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =dxCommon->UploadTextureData(textureResource.Get(), mipImages);
	#pragma endregion 
	
	#pragma region ShaderResourceView
		// metaDataを基にSRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{  };
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
		srvDesc2.Format = metadata2.format;
		srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);
	
		// SRVを作成するDescriptHeap	の場所を決める
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(1);
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(1);
	
		// 2枚目のSRVをつくる
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =dxCommon->GetSRVCPUDescriptorHandle(2);
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =dxCommon->GetSRVGPUDescriptorHandle(2);
	
		// 先頭はImGuiが使っているのでその次を使う
		textureSrvHandleCPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		textureSrvHandleGPU.ptr += dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// SRVの設定
		dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);
	
		dxCommon->GetDevice()->CreateShaderResourceView(textureResources2.Get(), &srvDesc2, textureSrvHandleCPU2);
	#pragma endregion 


		//ウィンドウのｘボタンが押されるまでループ
	MSG msg{};

	Matrix4x4 worldMatrix;
	Matrix4x4 cameraMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;

	Matrix4x4 worldProjectionMatrix;
	/*Matrix4x4 worldMatrixSprite;
	Matrix4x4 viewMatrixSprite;
	Matrix4x4 projectionMatrixSprite;
	Matrix4x4 worldViewProjectionMatrixSprite;

	Matrix4x4 uvTransformMatrix;*/
		
	Matrix4x4 worldMatrixModel;
	Matrix4x4 cameraMatrixModel;
	Matrix4x4 viewMatrixModel;
	Matrix4x4 projectionMatrixModel;
	Matrix4x4 worldProjectionMatrixModel;

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
		worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translata);
		cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translata);
		viewMatrix = Inverse(cameraMatrix);
		projectionMatrix = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		worldProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		transformationMatrixData->WVP = worldProjectionMatrix;
		transformationMatrixData->World = worldMatrix;
#pragma endregion

		worldMatrixModel = MakeAffineMatrix(modelTrasform.scale,modelTrasform.rotate,modelTrasform.translata);
		cameraMatrixModel = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translata);
		viewMatrixModel = Inverse(cameraMatrixModel);
		projectionMatrixModel = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		worldProjectionMatrixModel = Multiply(worldMatrixModel, Multiply(viewMatrixModel, projectionMatrixModel));
		input_->Update();
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGuiウィンドウの作成
		DirectionalLighting* directionalLightData = nullptr;
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


		dxCommon->PreDraw();

		spriteCommon->CommonDraw();

		
		#pragma region 三角形の描画
					dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
					//現状を設定。POSに設定しているものとはまた別。おなじ物を設定すると考えておけばいい
					dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
					//wvp用のCBufferの場所を設定
					dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
					dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
					dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightSprite->GetGPUVirtualAddress());
					//描画！
					//commandList->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);
					// ModelDataの描画
					dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
		#pragma endregion
		
				



		dxCommon->PostDraw();


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
	delete spriteCommon;
	delete sprite;
#pragma endregion



	return 0;


}