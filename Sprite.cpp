#include "Sprite.h"
#include "SpriteCommon.h"

void Sprite::Initialize(SpriteCommon*spriteCommon)
{
	this->spriteCommon_ = spriteCommon;

// VertexBufferResourceを生成
	vertexResourceSprite = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 6);

// TransfomationMatrixSprite用のResourceを作る
	transformationMatrixResourceSprite =spriteCommon_->GetDxCommon()-> CreateBufferResource(sizeof(TransformationMatrix));

// 平行光源をShderで使う
    directionalLightSprite =spriteCommon_->GetDxCommon()-> CreateBufferResource(sizeof(DirectionalLighting));

// Sprite用のリソース
	materialResourceSprite = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

// Index用のリソース
	indeResourceSprite =spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

	textureSrvHandleGPU = spriteCommon_->GetDxCommon()->GetSRVGPUDescriptorHandle(1);
}

void Sprite::TransformationMatrixSprite()
{
	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::MaterialResourceSprite()
{
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = { Vector4(1.0f, 1.0f, 1.0f, 1.0f) };
	materialDataSprite->enableLighting = false;

	materialData->uvTransform = MakeIdentity4x4();
	materialDataSprite->uvTransform = MakeIdentity4x4();
}

void Sprite::DirectionLightSprite()
{
	directionalLightSprite->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;
}

void Sprite::IndexBufferViewSprite()
{
	
	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indeResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// インデックスリソースにデータを書き込む
	indeResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;
}

void Sprite::VertexBufferViewSprite()
{
	//リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//1頂点当たりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);
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
}

void Sprite::WPVMatrix()
{
#pragma region WVPMatrixを作って書き込む
	worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translata);
	viewMatrixSprite = MakeIdentity4x4();
	projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
	transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
	transformationMatrixDataSprite->World = worldMatrix;
}

void Sprite::UvTransformMatrix()
{
	uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeRatateZMatrix(uvTransformSprite.rotate.z));
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translata));
	materialDataSprite->uvTransform = uvTransformMatrix;
}

void Sprite::Draw()
{
	dxCommon_->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);
	dxCommon_->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);


#pragma region Spriteの描画
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, transformationMatrixResourceSprite->GetGPUVirtualAddress());
	//TransF_omationMatrixBufferの場所を設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
	//描画！
	dxCommon_->GetCommandList()->DrawInstanced(6, 1, 0, 0);
}



