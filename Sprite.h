#pragma once
#include "math.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include <array>

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

private:
	SpriteCommon* spriteCommon = nullptr;

	struct VertexData
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	DirectXCommon* GetDxCommon()const { return dxCommon_; }
	DirectXCommon* dxCommon_;

#pragma region VertexBufferResourceを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = dxCommon_->CreateBufferResource(sizeof(VertexData) * 6);
#pragma endregion

#pragma region TransfomationMatrixSprite用のResourceを作る
	// Sprite用のTransfomationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite =dxCommon_-> CreateBufferResource(sizeof(TransformationMatrix));
#pragma endregion

#pragma region 平行光源をShderで使う
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightSprite =dxCommon_-> CreateBufferResource(sizeof(DirectionalLighting));
#pragma endregion

#pragma region Sprite用のリソース
	// Sprite用のマテリアルリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = dxCommon_->CreateBufferResource(sizeof(Material));
#pragma endregion

#pragma region Index用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indeResourceSprite = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 6);
#pragma endregion



	////vetexResourceSprite頂点バッファーを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{ };
	
};

