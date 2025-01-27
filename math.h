#include<cmath>
#include <vector>
#include<string>

struct Vector4
{
	float x;
	float y;
	float z;
	float w;
};

struct Vector3
{
	float x;
	float y;
	float z;
};

struct Vector2
{
	float x;
	float y;
};

struct Matrix3x3 {
	float m[3][3];
};

struct Matrix4x4 {
	float m[4][4];
};

struct Transform{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translata;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLighting {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct MaterialData{
	std::string textureFilePath;
};

struct ModelData{
	std::vector<VertexData> vertices;
	MaterialData material;
};


Matrix4x4 MakeIdentity4x4();

Matrix4x4 MakeScaleMatrix(const Vector3& scale);

Matrix4x4 MakeRotateXMatrix(const float radian);

Matrix4x4 MakeRatateYMatrix(const float radian);

Matrix4x4 MakeRatateZMatrix(const float radian);

Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

Matrix4x4 Inverse(const Matrix4x4& m);

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

