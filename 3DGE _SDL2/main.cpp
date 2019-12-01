//
//  main.cpp
//  3DGE _SDL2
//
//  Created by Михаил Ковалевский on 19.10.2019.
//  Copyright © 2019 Mikhail Kavaleuski. All rights reserved.
//

#include <iostream>
#include <vector>
#include <algorithm>
#include <list>
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#include <SDL.h>
#elif __APPLE__
	//define something for MacOS
	#include <SDL2/SDL.h>
#endif


class Engine3D {
private:
	enum class ERROR_CODES {
		ZERO, // No errors
		WINDOW_INIT_ERROR, // Error while window initialization
		SDL2_INIT_ERROR // Error while SDL2 initialization
	};
	ERROR_CODES GE_ERROR_CODE = ERROR_CODES::ZERO;

	enum class KEYBOARD_CONTROL_TYPES {
		ALLOW_SCENE_EDITING,
		ALLOW_OBJECT_EDITING,
		ALLOW_CAMERA_CONTROL
	};
	KEYBOARD_CONTROL_TYPES GE_CURRENT_KEYBOARD_CONTROL = KEYBOARD_CONTROL_TYPES::ALLOW_CAMERA_CONTROL;

	enum class RENDERING_STYLES {
		STD_SHADED,
		STD_POLY_SHADED,
		DEBUG_DRAW_ONLY_POLYGONS
	};
	RENDERING_STYLES GE_RENDERING_STYLE = RENDERING_STYLES::STD_POLY_SHADED;

	struct vec3 {
		float x, y, z;
	};

	struct Triangle {
		vec3 p[3];
		float R = 0.0f;
		float G = 0.0f;
		float B = 0.0f;
		float A = 255.0f;
	};

	struct Triangle2D {
		SDL_Point p[3];
	};

	struct Mesh {
		std::vector<Triangle> polygons;
	};

	struct Matrix4 {
		float m[4][4] = { 0 };
	};

	struct GEObject {
		vec3 position = { 0, 0, 0 };
		Mesh mesh;
	};
	
	struct DrawList {
		std::vector<GEObject> obj;
	};

	struct Camera {
		vec3 position = { 0, 2, 2 };
		vec3 lookDirection = { 0, 0, 1 };
		float fXRotation = 0;
		float fYRotation = 0;
		float fFOV = 90.0f;
		float fNear = 0.1f;
		float fFar = 1000.0f;
	};

	Mesh MESH_CUBE;
	Matrix4 matProj;
	Camera MainCamera;

	const char title[4] = "^_^";
	const int FRAMES_PER_SECOND = 120;

	int WIDTH, HEIGHT;

	bool isRunning = false;

	SDL_Window *window = NULL;

	bool check_window(SDL_Window *window) {
		if (window == NULL)
		{
			printf("Error creating window! SDL_Error: %s\n", SDL_GetError());
			return false;
		} else {
			printf("Created window(%dx%d)!\n", WIDTH, HEIGHT);
			return true;
		}
	}

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

	vec3 Vector_IntersectPlane(vec3 &plane_p, vec3 &plane_n, vec3 &lineStart, vec3 &lineEnd)
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

	int Triangle_ClipAgainstPlane(vec3 plane_p, vec3 plane_n, Triangle &in_tri, Triangle &out_tri1, Triangle &out_tri2)
	{
		// Make sure plane normal is indeed normal
		plane_n = Vector3_Normalize(plane_n);

		// Return signed shortest distance from point to plane, plane normal must be normalised
		auto dist = [&](vec3 &p)
		{
			vec3 n = Vector3_Normalize(p);
			return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector3_DotProduct(plane_n, plane_p));
		};

		// Create two temporary storage arrays to classify points either side of plane
		// If distance sign is positive, point lies on "inside" of plane
		vec3 *inside_points[3];  int nInsidePointCount = 0;
		vec3 *outside_points[3]; int nOutsidePointCount = 0;

		// Get signed distance of each point in triangle to plane
		float d0 = dist(in_tri.p[0]);
		float d1 = dist(in_tri.p[1]);
		float d2 = dist(in_tri.p[2]);

		if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[0]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[0]; }
		if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[1]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[1]; }
		if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[2]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[2]; }

		// Now classify triangle points, and break the input triangle into 
		// smaller output triangles if required. There are four possible
		// outcomes...

		if (nInsidePointCount == 0)
		{
			// All points lie on the outside of plane, so clip whole triangle
			// It ceases to exist

			return 0; // No returned triangles are valid
		}

		if (nInsidePointCount == 3)
		{
			// All points lie on the inside of plane, so do nothing
			// and allow the triangle to simply pass through
			out_tri1 = in_tri;

			return 1; // Just the one returned original triangle is valid
		}

		if (nInsidePointCount == 1 && nOutsidePointCount == 2)
		{
			// Triangle should be clipped. As two points lie outside
			// the plane, the triangle simply becomes a smaller triangle

			// Copy appearance info to new triangle
			/*out_tri1.col = in_tri.col;
			out_tri1.sym = in_tri.sym;*/
			out_tri1.R = in_tri.R;
			out_tri1.G = in_tri.G;
			out_tri1.B = in_tri.B;

			// The inside point is valid, so keep that...
			out_tri1.p[0] = *inside_points[0];

			// but the two new points are at the locations where the 
			// original sides of the triangle (lines) intersect with the plane
			out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

			return 1; // Return the newly formed single triangle
		}

		if (nInsidePointCount == 2 && nOutsidePointCount == 1)
		{
			// Triangle should be clipped. As two points lie inside the plane,
			// the clipped triangle becomes a "quad". Fortunately, we can
			// represent a quad with two new triangles

			// Copy appearance info to new triangles
			/*out_tri1.col = in_tri.col;
			out_tri1.sym = in_tri.sym;

			out_tri2.col = in_tri.col;
			out_tri2.sym = in_tri.sym;*/
			out_tri1.R = in_tri.R;
			out_tri1.G = in_tri.G;
			out_tri1.B = in_tri.B;

			out_tri2.R = in_tri.R;
			out_tri2.G = in_tri.G;
			out_tri2.B = in_tri.B;

			// The first triangle consists of the two inside points and a new
			// point determined by the location where one side of the triangle
			// intersects with the plane
			out_tri1.p[0] = *inside_points[0];
			out_tri1.p[1] = *inside_points[1];
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

			// The second triangle is composed of one of he inside points, a
			// new point determined by the intersection of the other side of the 
			// triangle and the plane, and the newly created point above
			out_tri2.p[0] = *inside_points[1];
			out_tri2.p[1] = out_tri1.p[2];
			out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

			return 2; // Return two newly formed triangles which form a quad
		}
	}

	vec3 Matrix4_MultiplyVector(vec3 &i, Matrix4 &m) {
		vec3 o;
		o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
		o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
		o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];

		const float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];
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

	Matrix4 Matrix_QuickInverse(Matrix4 &m) // Only for Rotation/Translation Matrixes
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



	void updateScreenAndCameraProperties(SDL_Renderer *renderer) {
		// Gets real size of the window(Fix for MacOS/Resizing)
		SDL_GetRendererOutputSize(renderer, &WIDTH, &HEIGHT);
		const float fAspectRatio = (float)HEIGHT / (float)WIDTH;
		matProj = Matrix4_MakeProjection(MainCamera.fFOV, fAspectRatio, MainCamera.fNear, MainCamera.fFar);
	}

	void DrawTriangle2D(SDL_Renderer *renderer, Triangle2D tr) {
		SDL_Point points[4] = { tr.p[0], tr.p[1], tr.p[2], tr.p[0] };
		SDL_RenderDrawLines(renderer, points, 4);
	}

	void DrawTopFlatTriangle(SDL_Renderer *renderer, SDL_Point *v)
	{
		/*
		  0 ---------- 1
			\	     /
			 \      /
			  \	   /
			   \  /
			    \/
				 2
		*/
		float dx0 = (float)(v[0].x - v[2].x) / (float)(v[2].y - v[0].y);
		float dx1 = (float)(v[1].x - v[2].x) / (float)(v[2].y - v[1].y);

		float xOffset0 = v[2].x;
		float xOffset1 = v[2].x;

		for (int scanlineY = v[2].y; scanlineY >= v[0].y; scanlineY--)
		{
			SDL_RenderDrawLine(renderer, xOffset0, scanlineY, xOffset1, scanlineY);
			xOffset0 += dx0;
			xOffset1 += dx1;
		}
	}

	void DrawBottomFlatTriangle(SDL_Renderer *renderer, SDL_Point *v)
	{
		/*
		        0
		        /\ 
			   /  \
		      /    \
		     /      \
			/        \
		  1 ---------- 2
		*/
		float dx0 = (float)(v[1].x - v[0].x) / (float)(v[1].y - v[0].y);
		float dx1 = (float)(v[2].x - v[0].x) / (float)(v[2].y - v[0].y);

		float xOffset0 = v[0].x;
		float xOffset1 = v[0].x;

		for (int scanlineY = v[0].y; scanlineY <= v[1].y; scanlineY++)
		{
			SDL_RenderDrawLine(renderer, xOffset0, scanlineY, xOffset1, scanlineY);
			xOffset0 += dx0;
			xOffset1 += dx1;
		}
	}

	void DrawFilledTriangle2D(SDL_Renderer *renderer, Triangle2D tr) {
		// Points are Integers
		// p[0] need to have lowest y among points
		if (!(tr.p[0].y == tr.p[1].y && tr.p[1].y == tr.p[2].y)) {
			bool sorted = false;
			while (!sorted)
			{
				sorted = true;
				for (int i = 0; i < 2; i++) {
					if (tr.p[i].y > tr.p[i + 1].y)
					{
						sorted = false;
						SDL_Point buffPoint = tr.p[i];
						tr.p[i] = tr.p[i + 1];
						tr.p[i + 1] = buffPoint;
					}
				}
			}

			if (tr.p[1].y == tr.p[2].y) {
				DrawBottomFlatTriangle(renderer, tr.p);
			} else
				if (tr.p[0].y == tr.p[1].y) {
					DrawTopFlatTriangle(renderer, tr.p);
				} else {
					SDL_Point splitPoint;
					splitPoint.x = tr.p[0].x + ((float)(tr.p[1].y - tr.p[0].y) / (float)(tr.p[2].y - tr.p[0].y)) * (tr.p[2].x - tr.p[0].x);
					splitPoint.y = tr.p[1].y;
					SDL_Point points[3];
					points[0] = tr.p[0];
					points[1] = tr.p[1];
					points[2] = splitPoint;
					DrawBottomFlatTriangle(renderer, points);
					points[0] = tr.p[1];
					points[1] = splitPoint;
					points[2] = tr.p[2];
					DrawTopFlatTriangle(renderer, points);
				}
			}
		
	}
	
	void DrawSceneObjects(SDL_Renderer *renderer) {
		Matrix4 matRotX, matRotY, matRotZ;

		/*static float theta = 0;
		theta += 1.0f / 30.0f;*/

		// Rotation X
		matRotX = Matrix4_MakeRotationX(0);

		// Rotation Y
		matRotY = Matrix4_MakeRotationY(0);

		// Rotation Z
		matRotZ = Matrix4_MakeRotationZ(0);

		Matrix4 matTrans;
		matTrans = Matrix4_MakeTranslation(0.0f, 0.0f, 5.0f);

		Matrix4 matWorld;
		matWorld = Matrix4_MakeIdentity();	// Form World Matrix
		matWorld = Matrix4_MultiplyMatrix(matRotX, matRotY); // Transform by rotation by X and Y
		matWorld = Matrix4_MultiplyMatrix(matWorld, matRotZ); // Transform by rotation by Y
		matWorld = Matrix4_MultiplyMatrix(matWorld, matTrans); // Transform by translation

		vec3 upVector = { 0, 1, 0 };
		vec3 targetVector = { 0, -1, 1 };
		Matrix4 matCameraRot = Matrix4_MultiplyMatrix(Matrix4_MakeRotationX(MainCamera.fXRotation), Matrix4_MakeRotationY(MainCamera.fYRotation));
		MainCamera.lookDirection = Matrix4_MultiplyVector(targetVector, matCameraRot);
		targetVector = Vector3_Add(MainCamera.position, MainCamera.lookDirection);
		Matrix4 matCamera = Matrix4_PointAt(MainCamera.position, targetVector, upVector);

		Matrix4 matView = Matrix_QuickInverse(matCamera);

		// Store triagles for rastering later
		std::vector<Triangle> vecTrianglesToRaster;

		for (Triangle tri: MESH_CUBE.polygons) {
			Triangle triProjected, triTransformed, triViewed;

			// World Matrix Transform
			triTransformed.p[0] = Matrix4_MultiplyVector(tri.p[0], matWorld);
			triTransformed.p[1] = Matrix4_MultiplyVector(tri.p[1], matWorld);
			triTransformed.p[2] = Matrix4_MultiplyVector(tri.p[2], matWorld);

			// Calculate triangle Normal
			vec3 normal, line1, line2;

			// Get lines either side of triangle
			line1 = Vector3_Sub(triTransformed.p[1], triTransformed.p[0]);
			line2 = Vector3_Sub(triTransformed.p[2], triTransformed.p[0]);

			// Take cross product of lines to get normal to triangle surface
			normal = Vector3_CrossProduct(line1, line2);

			// You normally need to normalise a normal!
			normal = Vector3_Normalize(normal);

			// Get Ray from triangle to camera
			vec3 vCameraRay = Vector3_Sub(triTransformed.p[0], MainCamera.position);

			if (Vector3_DotProduct(normal, vCameraRay) < 0.0f) {

				// Illumination
				vec3 LightDirection = { 1.0f, 1.0f, -1.0f }; // LIGHT ORIGIN
				LightDirection = Vector3_Normalize(LightDirection);

				// How similar is normal to light direction
				float dp = normal.x * LightDirection.x + normal.y * LightDirection.y + normal.z * LightDirection.z;
				if (dp < 0.1f) {
					dp = 0.1f;
				}
				triTransformed.R = dp * 255.0f;
				triTransformed.G = dp * 255.0f;
				triTransformed.B = dp * 255.0f;

				// Convert World Space --> View Space
				triViewed.p[0] = Matrix4_MultiplyVector(triTransformed.p[0], matView);
				triViewed.p[1] = Matrix4_MultiplyVector(triTransformed.p[1], matView);
				triViewed.p[2] = Matrix4_MultiplyVector(triTransformed.p[2], matView);
				triViewed.R = triTransformed.R;
				triViewed.G = triTransformed.G;
				triViewed.B = triTransformed.B;

				int nClippedTriangles = 0;
				Triangle clipped[2];
				nClippedTriangles = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);

				for (int n = 0; n < nClippedTriangles; n++)
				{
					// Project triangles from 3D --> 2D
					triProjected.p[0] = Matrix4_MultiplyVector(clipped[n].p[0], matProj);
					triProjected.p[1] = Matrix4_MultiplyVector(clipped[n].p[1], matProj);
					triProjected.p[2] = Matrix4_MultiplyVector(clipped[n].p[2], matProj);
					triProjected.R = clipped[n].R;
					triProjected.G = clipped[n].G;
					triProjected.B = clipped[n].B;

					// X/Y are inverted so put them back
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					// Offset verts into visible normalised space
					vec3 vOffsetView = { 1,1,0 };
					triProjected.p[0] = Vector3_Add(triProjected.p[0], vOffsetView);
					triProjected.p[1] = Vector3_Add(triProjected.p[1], vOffsetView);
					triProjected.p[2] = Vector3_Add(triProjected.p[2], vOffsetView);
					triProjected.p[0].x *= 0.5f * WIDTH;
					triProjected.p[0].y *= 0.5f * HEIGHT;
					triProjected.p[1].x *= 0.5f * WIDTH;
					triProjected.p[1].y *= 0.5f * HEIGHT;
					triProjected.p[2].x *= 0.5f * WIDTH;
					triProjected.p[2].y *= 0.5f * HEIGHT;

					// Store triangle for sorting
					vecTrianglesToRaster.push_back(triProjected);
				}
				
				sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle &t1, Triangle &t2)
					{
						float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
						float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
						return z1 > z2;
					});

				// Legacy rendering code
				/*for (Triangle rTri : vecTrianglesToRaster) {
					SDL_Point points[3] = {
									{ rTri.p[0].x,rTri.p[0].y },
									{ rTri.p[1].x,rTri.p[1].y },
									{ rTri.p[2].x,rTri.p[2].y }
					};
					Triangle2D tr = { points[0], points[1], points[2] };
					SDL_SetRenderDrawColor(renderer, dp * 255.0f, dp * 255.0f, dp * 255.0f, 255.0f);
					DrawFilledTriangle2D(renderer, tr);

					if (DEBUG_DRAW_POLYGONS) {
						SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
						DrawTriangle2D(renderer, tr);
					}
				}*/

				for (auto &triToRaster : vecTrianglesToRaster)
				{
					// Clip triangles against all four screen edges, this could yield
					// a bunch of triangles, so create a queue that we traverse to 
					//  ensure we only test new triangles generated against planes
					Triangle clipped[2];
					std::list<Triangle> listTriangles;

					// Add initial triangle
					listTriangles.push_back(triToRaster);
					int nNewTriangles = 1;

					for (int p = 0; p < 4; p++)
					{
						int nTrisToAdd = 0;
						while (nNewTriangles > 0)
						{
							// Take triangle from front of queue
							Triangle test = listTriangles.front();
							listTriangles.pop_front();
							nNewTriangles--;

							// Clip it against a plane. We only need to test each 
							// subsequent plane, against subsequent new triangles
							// as all triangles after a plane clip are guaranteed
							// to lie on the inside of the plane. I like how this
							// comment is almost completely and utterly justified
							switch (p)
							{
							case 0:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
							case 1:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)HEIGHT - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
							case 2:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
							case 3:	nTrisToAdd = Triangle_ClipAgainstPlane({ (float)WIDTH - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
							}

							// Clipping may yield a variable number of triangles, so
							// add these new ones to the back of the queue for subsequent
							// clipping against next planes
							for (int w = 0; w < nTrisToAdd; w++) {
								listTriangles.push_back(clipped[w]);
							}

						}
						nNewTriangles = listTriangles.size();
					}


					// Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
					for (Triangle &t : listTriangles)
					{
						SDL_Point points[3] = {
									{ t.p[0].x,t.p[0].y },
									{ t.p[1].x,t.p[1].y },
									{ t.p[2].x,t.p[2].y }
						};
						Triangle2D tr = { points[0], points[1], points[2] };

						switch (GE_RENDERING_STYLE)
						{
						case Engine3D::RENDERING_STYLES::STD_SHADED:
							SDL_SetRenderDrawColor(renderer, t.R, t.G, t.B, 255.0f);
							DrawFilledTriangle2D(renderer, tr);
							break;
						case Engine3D::RENDERING_STYLES::STD_POLY_SHADED:
							SDL_SetRenderDrawColor(renderer, t.R, t.G, t.B, 255.0f);
							DrawFilledTriangle2D(renderer, tr);
							SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
							DrawTriangle2D(renderer, tr);
							break;
						case Engine3D::RENDERING_STYLES::DEBUG_DRAW_ONLY_POLYGONS:
							SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
							DrawTriangle2D(renderer, tr);
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}

	void SceneEditingHandle(SDL_Scancode scancode) {
		printf("SceneEditingHandle->");
		switch (scancode) {
		case SDL_SCANCODE_SPACE: {
			printf("Tapped SDL_SCANCODE_SPACE\n");
			break;
		}
		case SDL_SCANCODE_UP: {
			printf("Tapped SDL_SCANCODE_UP\n");
			break;
		}
		case SDL_SCANCODE_DOWN: {
			printf("Tapped SDL_SCANCODE_DOWN\n");
			break;
		}
		case SDL_SCANCODE_LEFT: {
			printf("Tapped SDL_SCANCODE_LEFT\n");
			break;
		}
		case SDL_SCANCODE_RIGHT: {
			printf("Tapped SDL_SCANCODE_RIGHT\n");
			break;
		}
		default:
			printf("Tapped untreated key: %d\n", scancode);
			break;
		}
	}

	void CameraMovementHandle(SDL_Scancode scancode) {
		float offset = 10.0f / (float)FRAMES_PER_SECOND;
		printf("CameraMovementHandle->");
		switch (scancode) {
			// WASD
		case SDL_SCANCODE_W: {
			printf("Tapped SDL_SCANCODE_W\n");
			MainCamera.position.z += offset;
			break;
		}
		case SDL_SCANCODE_A: {
			printf("Tapped SDL_SCANCODE_A\n");
			MainCamera.position.x += offset;
			break;
		}
		case SDL_SCANCODE_S: {
			printf("Tapped SDL_SCANCODE_S\n");
			MainCamera.position.z -= offset;
			break;
		}
		case SDL_SCANCODE_D: {
			printf("Tapped SDL_SCANCODE_D\n");
			MainCamera.position.x -= offset;
			break;
		}
		case SDL_SCANCODE_SPACE: {
			printf("Tapped SDL_SCANCODE_SPACE\n");
			MainCamera.position.y += offset;
			break;
		}
		case SDL_SCANCODE_X: {
			printf("Tapped SDL_SCANCODE_X\n");
			MainCamera.position.y -= offset;
			break;
		}
		case SDL_SCANCODE_UP: {
			printf("Tapped SDL_SCANCODE_UP\n");
			MainCamera.fXRotation -= offset;
			break;
		}
		case SDL_SCANCODE_DOWN: {
			printf("Tapped SDL_SCANCODE_DOWN\n");
			MainCamera.fXRotation += offset;
			break;
		}
		case SDL_SCANCODE_LEFT: {
			printf("Tapped SDL_SCANCODE_LEFT\n");
			MainCamera.fYRotation -= offset;
			break;
		}
		case SDL_SCANCODE_RIGHT: {
			printf("Tapped SDL_SCANCODE_RIGHT\n");
			MainCamera.fYRotation += offset;
			break;
		}
		case SDL_SCANCODE_R: {
			printf("Tapped SDL_SCANCODE_R\n");
			MainCamera.fXRotation = 0;
			MainCamera.fYRotation = 0;
			MainCamera.position.x = 0;
			MainCamera.position.y = 2;
			MainCamera.position.z = 2;
			break;
		}
		default:
			printf("Tapped untreated key: %d\n", scancode);
			break;
		}
	}

	void StartRenderLoop() {
		SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
		Uint32 start;
		SDL_Event windowEvent;
		while (isRunning)
		{
			if (SDL_PollEvent(&windowEvent))
			{
				if (windowEvent.type == SDL_QUIT)
				{
					isRunning = false;
				}

				if (windowEvent.type == SDL_KEYDOWN) {
					switch (GE_CURRENT_KEYBOARD_CONTROL)
					{
					case Engine3D::KEYBOARD_CONTROL_TYPES::ALLOW_SCENE_EDITING:
						SceneEditingHandle(windowEvent.key.keysym.scancode);
						break;
					case Engine3D::KEYBOARD_CONTROL_TYPES::ALLOW_OBJECT_EDITING:
						break;
					case Engine3D::KEYBOARD_CONTROL_TYPES::ALLOW_CAMERA_CONTROL:
						CameraMovementHandle(windowEvent.key.keysym.scancode);
						break;
					default:
						break;
					}
				}
			}
			start = SDL_GetTicks();

			//Updates properties of the screen and camera
			updateScreenAndCameraProperties(renderer);

			//Background(Clears with color)
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			//Draws scene
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			DrawSceneObjects(renderer);

			// Renders window
			SDL_RenderPresent(renderer);
			Uint32 ticks = SDL_GetTicks();
			if (1000 / FRAMES_PER_SECOND > ticks - start) {
				SDL_Delay(1000 / FRAMES_PER_SECOND - (ticks - start));
			}
		}
		SDL_DestroyRenderer(renderer);
	}
	
	void initMeshes() {
		MESH_CUBE.polygons = {

			// CLOCKWISE DIRECTION

			// SOUTH
			{ 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

			// EAST                                                      
			{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

			// NORTH                                                     
			{ 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
			{ 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

			// WEST                                                      
			{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

			// TOP                                                       
			{ 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

			// BOTTOM                                                    
			{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
			{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

		};
	}

	ERROR_CODES initEngine() {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			return ERROR_CODES::SDL2_INIT_ERROR;
		}

		window = SDL_CreateWindow(
									title,
									SDL_WINDOWPOS_UNDEFINED,
									SDL_WINDOWPOS_UNDEFINED,
									WIDTH,
									HEIGHT,
									SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
								 );

		// Check that the window was successfully created
		if (!check_window(window)) {
			return ERROR_CODES::WINDOW_INIT_ERROR;
		};

		initMeshes();
		printf("Initialized meshes!\n");

		return ERROR_CODES::ZERO;
	}

	void Destroy() {
		SDL_DestroyWindow(window);
		SDL_Quit();
		printf("Destroyed 3DGE!\n");
	}

public:
	Engine3D(int _WIDTH, int _HEIGHT) {
		WIDTH = _WIDTH;
		HEIGHT = _HEIGHT;
		GE_ERROR_CODE = initEngine();
	}
	Engine3D() {
		WIDTH = 800;
		HEIGHT = 600;
		GE_ERROR_CODE = initEngine();
	}

	~Engine3D() {
		Destroy();
	}

	void startScene() {
		if (GE_ERROR_CODE != ERROR_CODES::ZERO) {
			printf("Unable to start scene! ERROR: %d\n", GE_ERROR_CODE);
			return;
		}
		isRunning = true;
		StartRenderLoop();
	}
};






int main(int argc, char *argv[])
{
	Engine3D Engine(800, 600);
	Engine.startScene();
    return 0;
}






