#pragma once
#include "DirectXCommon.h"

// スプライト共通部分
class SpriteCommon
{
public:
	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 共通描画設定
	void CommonDraw();

	// ルートシグネチャの作成
	void RootSigunetureCreate();
	// グラフィックスパイプラインの生成
	void GraphicsPipelineGenerated();

	DirectXCommon* GetDxCommon()const { return dxCommon_; }

private:
	DirectXCommon* dxCommon_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
};

