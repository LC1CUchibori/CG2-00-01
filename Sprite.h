#pragma once


class SpriteCommon;

// スプライト
class Sprite
{
public:
	// 初期化
	void Initialize(SpriteCommon*spriteCommon);
private:
	SpriteCommon* spriteCommon = nullptr;

	struct VertexData
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};


	
};

