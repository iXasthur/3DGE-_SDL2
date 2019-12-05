#ifndef GE_3DMATH_H
#define GE_3DMATH_H

#include <math.h>

struct vec3 {
	float x, y, z;
};

struct Matrix4 {
	float m[4][4] = { 0 };
};

vec3 Vector3_Add(vec3 &v1, vec3 &v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

vec3 Vector3_Sub(vec3 &v1, vec3 &v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

vec3 Vector3_Mul(vec3 &v1, float k) {
	return { v1.x * k, v1.y * k, v1.z * k };
}

vec3 Vector3_Div(vec3 &v1, float k) {
	return { v1.x / k, v1.y / k, v1.z / k };
}

float Vector3_DotProduct(vec3 &v1, vec3 &v2) {
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

float Vector3_Length(vec3 &v)
{
	return sqrtf(Vector3_DotProduct(v, v));
}

bool Vector3_Equals(vec3 &v1, vec3 &v2) {
	if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z){
		return true;
	}
	return false;
}

vec3 Vector3_Normalize(vec3 &v) {
	float l = Vector3_Length(v);
	return { v.x / l, v.y / l, v.z / l };
}

vec3 Vector3_CrossProduct(vec3 &v1, vec3 &v2)
{
	vec3 v;
	v.x = v1.y * v2.z - v1.z * v2.y;
	v.y = v1.z * v2.x - v1.x * v2.z;
	v.z = v1.x * v2.y - v1.y * v2.x;
	return v;
}

vec3 Vector3_IntersectPlane(vec3 &plane_p, vec3 &plane_n, vec3 &lineStart, vec3 &lineEnd)
{
	plane_n = Vector3_Normalize(plane_n);
	float plane_d = -Vector3_DotProduct(plane_n, plane_p);
	float ad = Vector3_DotProduct(lineStart, plane_n);
	float bd = Vector3_DotProduct(lineEnd, plane_n);
	float t = (-plane_d - ad) / (bd - ad);
	vec3 lineStartToEnd = Vector3_Sub(lineEnd, lineStart);
	vec3 lineToIntersect = Vector3_Mul(lineStartToEnd, t);
	return Vector3_Add(lineStart, lineToIntersect);
}

vec3 Matrix4_MultiplyVector(vec3 &i, Matrix4 &m) {
	vec3 o;
	o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
	o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
	o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];

	float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];
	if (w != 0.0f)
	{
		o.x /= w;
		o.y /= w;
		o.z /= w;
	}
	return o;
}

Matrix4 Matrix4_MakeIdentity() {
	Matrix4 matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4 Matrix4_MakeRotationX(float fAngleRad)
{
	Matrix4 matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[1][2] = sinf(fAngleRad);
	matrix.m[2][1] = -sinf(fAngleRad);
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4 Matrix4_MakeRotationY(float fAngleRad)
{
	Matrix4 matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][2] = sinf(fAngleRad);
	matrix.m[2][0] = -sinf(fAngleRad);
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4 Matrix4_MakeRotationZ(float fAngleRad)
{
	Matrix4 matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][1] = sinf(fAngleRad);
	matrix.m[1][0] = -sinf(fAngleRad);
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4 Matrix4_MakeTranslation(float x, float y, float z)
{
	Matrix4 matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	matrix.m[3][0] = x;
	matrix.m[3][1] = y;
	matrix.m[3][2] = z;
	return matrix;
}

Matrix4 Matrix4_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
{
	float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
	Matrix4 matrix;
	matrix.m[0][0] = fAspectRatio * fFovRad;
	matrix.m[1][1] = fFovRad;
	matrix.m[2][2] = fFar / (fFar - fNear);
	matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	matrix.m[2][3] = 1.0f;
	matrix.m[3][3] = 0.0f;
	return matrix;
}

Matrix4 Matrix4_MultiplyMatrix(Matrix4 &m1, Matrix4 &m2)
{
	Matrix4 matrix;
	for (int c = 0; c < 4; c++) {
		for (int r = 0; r < 4; r++) {
			matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
		}
	}
	return matrix;
}

Matrix4 Matrix4_PointAt(vec3 &pos, vec3 &target, vec3 &up)
{
	// Calculate new forward direction
	vec3 newForward = Vector3_Sub(target, pos);
	newForward = Vector3_Normalize(newForward);

	// Calculate new Up direction
	vec3 a = Vector3_Mul(newForward, Vector3_DotProduct(up, newForward));
	vec3 newUp = Vector3_Sub(up, a);
	newUp = Vector3_Normalize(newUp);

	// New Right direction is easy, its just cross product
	vec3 newRight = Vector3_CrossProduct(newUp, newForward);

	// Construct Dimensioning and Translation Matrix	
	Matrix4 matrix;
	matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
	return matrix;

}

Matrix4 Matrix4_QuickInverse(Matrix4 &m) // Only for Rotation/Translation Matrixes
{
	Matrix4 matrix;
	matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
	matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
	matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
	matrix.m[3][3] = 1.0f;
	return matrix;
}

#endif
