#pragma once
#include "math.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include <array>
#include "DirectXCommon.h"

#include <cassert>
#include <dxcapi.h>
#include <chrono>

#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include <format>
#include "externals/DirectXTex/DirectXTex.h"



class SpriteCommon;

// スプライト
class Sprite
{
public:
	// 初期化
	void Initialize(SpriteCommon*spriteCommon);

	void TransformationMatrixSprite();

	void MaterialResourceSprite();

	void DirectionLightSprite();

	void IndexBufferViewSprite();

	void VertexBufferViewSprite();

	void WPVMatrix();

	void UvTransformMatrix();

	void Draw();

	DirectXCommon* GetDxCommon()const { return dxCommon_; }

private:
	SpriteCommon* spriteCommon_ = nullptr;

	DirectXCommon* dxCommon_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightSprite;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite;

	Microsoft::WRL::ComPtr<ID3D12Resource> indeResourceSprite;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	////vetexResourceSprite頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{ };

	// データを書き込む
	TransformationMatrix* transformationMatrixDataSprite = nullptr;

	// 単位行列を書き込んでおく
	TransformationMatrix* transformationMatrixData = nullptr;

	Material* materialDataSprite = nullptr;

	Material* materialData = nullptr;

	DirectionalLighting* directionalLightData = nullptr;

	uint32_t* indexDataSprite = nullptr;

	//頂点リソースにデータを書き込む
	VertexData* vertexDataSprite = nullptr;

	
#pragma region spriteTransform変数
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
#pragma endregion

#pragma region UVTransform変数
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};

	Matrix4x4 worldMatrix;
	Matrix4x4 cameraMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;

	Matrix4x4 worldProjectionMatrix;
	Matrix4x4 worldMatrixSprite;
	Matrix4x4 viewMatrixSprite;
	Matrix4x4 projectionMatrixSprite;
	Matrix4x4 worldViewProjectionMatrixSprite;

	Matrix4x4 uvTransformMatrix;

	Matrix4x4 worldMatrixModel;
	Matrix4x4 cameraMatrixModel;
	Matrix4x4 viewMatrixModel;
	Matrix4x4 projectionMatrixModel;
	Matrix4x4 worldProjectionMatrixModel;
};

