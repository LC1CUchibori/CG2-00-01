#include "Particle.hlsli"


struct Material
{
	float32_t4 color;
	int32_t enableLighting;
	float32_t4x4 uvTransform;
};

struct DirectionalLight
{
	float32_t4 color;
	float32_t3 direction;
	float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSample : register(s0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
	float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor = gTexture.Sample(gSample, transformedUV.xy);
    
    // 透明度が0（完全に透明）ならピクセルを破棄
	if (textureColor.a <= 0.0f)
	{
		discard;
	}
    
    //float32_t4 textureColor = gTexture.Sample(gSample, input.texcoord);
	PixelShaderOutput output;
	//if (gMaterial.enableLighting != 0)
	//{
		float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
		float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
		output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
		output.color.a = gMaterial.color.a * textureColor.a;
	//}
	//else
	//{
		output.color = gMaterial.color * textureColor;
	//}
    
     // アルファ値が低い場合にピクセルを破棄（例えば、透明度が0.5以下の場合）
	if (output.color.a <= 0.0f)
	{
		discard;
	}
	return output;
}


