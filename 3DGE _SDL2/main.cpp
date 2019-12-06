//
//  main.cpp
//  3DGE _SDL2
//
//  Created by Михаил Ковалевский on 19.10.2019.
//  Copyright © 2019 Mikhail Kavaleuski. All rights reserved.
//

#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include "GE_3DMath.h"
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
	KEYBOARD_CONTROL_TYPES GE_CURRENT_KEYBOARD_CONTROL = KEYBOARD_CONTROL_TYPES::ALLOW_SCENE_EDITING;

	enum class RENDERING_STYLES {
		STD_SHADED,
		STD_POLY_SHADED,
		DEBUG_DRAW_ONLY_POLYGONS
	};
	RENDERING_STYLES GE_RENDERING_STYLE = RENDERING_STYLES::STD_SHADED;


	struct Triangle {
		vec3 p[3];
		float R = 255.0f;
		float G = 255.0f;
		float B = 255.0f;
	};

	struct Triangle2D {
		SDL_Point p[3];
	};

	struct Mesh {
		std::vector<Triangle> polygons; // First is always considered as a selector
	};

	struct GE_Object {
	private:
		vec3 position = { 0, 0, 0 };
	public:
		Mesh mesh;

		vec3 getPosition() {
			return position;
		}

		void moveBy(vec3 v) {
			position = Vector3_Add(position, v);
			for (Triangle &tri : mesh.polygons) {
				for (vec3 &p : tri.p) {
					p = Vector3_Add(p, v);
				}
			}
			printf("Moved block to %.2f %.2f %.2f\n", position.x, position.y, position.z);
		}

		void moveTo(vec3 v) {
			vec3 offset = Vector3_Sub(v, position);
			moveBy(offset);
			//printf("Moved block to %.2f %.2f %.2f\n", position.x, position.y, position.z);
		}

		void scaleBy(float k) {
			for (Triangle &tri : mesh.polygons) {
				for (vec3 &p : tri.p) {
					p = Vector3_Mul(p, k);
				}
			}
			printf("Scaled block by %.2f\n", k);
		}

		void setColor(float R, float G, float B) {
			for (Triangle &tri : mesh.polygons) {
				tri.R = R;
				tri.G = G;
				tri.B = B;
			}
			printf("Changed color to %.2f %.2f %.2f\n", R, G, B);
		}
	};
	
	struct DrawList {
		GE_Object selectorBox;
		std::vector<GE_Object> obj;
	};
	DrawList GE_DRAW_LIST;

	struct GE_Camera {
		// pi -3.14159f
		vec3 position = { 3, 4, 0 };
		vec3 lookDirection = { 0, 0, 1 };
		float fXRotation = 3.14159f / 6.0f; //3.14159f / 1000.0f;
		float fYRotation = 3.14159f / 8.0f; // 3.14159f / 4.0f;
		float fFOV = 90.0f;
		float fNear = 0.1f;
		float fFar = 1000.0f;
	};

	struct GE_STD_MESHES {
		Mesh MESH_CUBE;
	};
	GE_STD_MESHES GE_MESHES;

	Matrix4 matProj;
	GE_Camera MainCamera;

	// Illumination
	vec3 LightDirection = { 0.5f, 0.75f, -1.0f }; // LIGHT ORIGIN

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

	int Triangle_ClipAgainstPlane(vec3 plane_p, vec3 plane_n, Triangle &in_tri, Triangle &out_tri1, Triangle &out_tri2)
	{
		// Make sure plane normal is indeed normal
		plane_n = Vector3_Normalize(plane_n);

		// Return signed shortest distance from point to plane, plane normal must be normalised
		auto dist = [&](vec3 &p)
		{
			//vec3 n = Vector3_Normalize(p);
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
			out_tri1.p[1] = Vector3_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
			out_tri1.p[2] = Vector3_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

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
			out_tri1.p[2] = Vector3_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

			// The second triangle is composed of one of he inside points, a
			// new point determined by the intersection of the other side of the 
			// triangle and the plane, and the newly created point above
			out_tri2.p[0] = *inside_points[1];
			out_tri2.p[1] = out_tri1.p[2];
			out_tri2.p[2] = Vector3_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

			return 2; // Return two newly formed triangles which form a quad
		}
        
        return -1;
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

	void FillTrianglesToRasterVector(std::vector<Triangle> &vecTrianglesToRaster, Triangle &tri, Matrix4 &matWorld, Matrix4 &matView) {
		Triangle triProjected, triTransformed, triViewed;

		// World Matrix Transform
		triTransformed.p[0] = Matrix4_MultiplyVector(tri.p[0], matWorld);
		triTransformed.p[1] = Matrix4_MultiplyVector(tri.p[1], matWorld);
		triTransformed.p[2] = Matrix4_MultiplyVector(tri.p[2], matWorld);
		triTransformed.R = tri.R;
		triTransformed.G = tri.G;
		triTransformed.B = tri.B;

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

			// How similar is normal to light direction
			float dp = normal.x * LightDirection.x + normal.y * LightDirection.y + normal.z * LightDirection.z;
			if (dp < 0.1f) {
				dp = 0.1f;
			}
			triTransformed.R = dp * triTransformed.R;
			triTransformed.G = dp * triTransformed.G;
			triTransformed.B = dp * triTransformed.B;

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
		vec3 targetVector = { 0, 0, 1 };
        Matrix4 m1 = Matrix4_MakeRotationX(MainCamera.fXRotation);
        Matrix4 m2 = Matrix4_MakeRotationY(MainCamera.fYRotation);
		Matrix4 matCameraRot = Matrix4_MultiplyMatrix(m1, m2);
		MainCamera.lookDirection = Matrix4_MultiplyVector(targetVector, matCameraRot);
		targetVector = Vector3_Add(MainCamera.position, MainCamera.lookDirection);
		Matrix4 matCamera = Matrix4_PointAt(MainCamera.position, targetVector, upVector);

		Matrix4 matView = Matrix4_QuickInverse(matCamera);

		// Store triagles for rastering later
		std::vector<Triangle> vecTrianglesToRaster;

		for (GE_Object &obj : GE_DRAW_LIST.obj) {
			if (!Vector3_Equals(obj.getPosition(), GE_DRAW_LIST.selectorBox.getPosition())) {
				for (Triangle &tri : obj.mesh.polygons) {
					FillTrianglesToRasterVector(vecTrianglesToRaster, tri, matWorld, matView);
				}
			}
		}

		for (Triangle &tri : GE_DRAW_LIST.selectorBox.mesh.polygons) {
			FillTrianglesToRasterVector(vecTrianglesToRaster, tri, matWorld, matView);
		}

		sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](Triangle &t1, Triangle &t2)
			{
				float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
				float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
				return z1 > z2;
			});

		for (Triangle &triToRaster : vecTrianglesToRaster)
		{
			// Clip triangles against all four screen edges, this could yield
			// a bunch of triangles, so create a queue that we traverse to 
			//  ensure we only test new triangles generated against planes
			Triangle clipped[2];
			std::list<Triangle> listTriangles;

			// Add initial triangle
			listTriangles.push_back(triToRaster);
			unsigned long nNewTriangles = 1;

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
					case 1:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)HEIGHT - 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 2:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 3:	nTrisToAdd = Triangle_ClipAgainstPlane({ (float)WIDTH - 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
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
							{ (int)t.p[0].x,(int)t.p[0].y },
							{ (int)t.p[1].x,(int)t.p[1].y },
							{ (int)t.p[2].x,(int)t.p[2].y }
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

	bool CheckIfBlockExists(vec3 pos) {
		for (GE_Object &obj : GE_DRAW_LIST.obj) {
			if (Vector3_Length(Vector3_Sub(obj.getPosition(),pos)) == 0) {
				return true;
			}
		}
		return false;
	}

	void CreateBlockAtSelectorPosition() {
		vec3 pos = GE_DRAW_LIST.selectorBox.getPosition();
		if (!CheckIfBlockExists(pos)){
			GE_Object sBox;
			sBox.mesh = GE_MESHES.MESH_CUBE;
			sBox.setColor(255.0f, 255.0f, 255.0f);
			sBox.moveTo(pos);
			GE_DRAW_LIST.obj.push_back(sBox);
			printf("Created block at %.2f %.2f %.2f\n", pos.x, pos.y, pos.z);
		} else {
			printf("Aborted to created block at %.2f %.2f %.2f\n", pos.x, pos.y, pos.z);
		}
		
	}

	void RemoveBlockAtSelectorPosition() {
		vec3 pos = GE_DRAW_LIST.selectorBox.getPosition();
		int index = -1;
		int i = 0;
		for (GE_Object &obj : GE_DRAW_LIST.obj) {
			if (Vector3_Equals(obj.getPosition(), pos)) {
				index = i;
				break;
			}
			i++;
		}
		if (index >= 0)	{
			GE_DRAW_LIST.obj.erase(GE_DRAW_LIST.obj.begin() + index);
			printf("Removed block at %.2f %.2f %.2f\n", pos.x, pos.y, pos.z);
		} else {
			printf("Unable to remove block at %.2f %.2f %.2f\n", pos.x, pos.y, pos.z);
		}
	}

	void SceneEditingHandle(SDL_Scancode scancode) {
		//printf("SceneEditingHandle->");
		switch (scancode) {
		case SDL_SCANCODE_UP: {
			//printf("Tapped SDL_SCANCODE_UP\n");
			GE_DRAW_LIST.selectorBox.moveBy({ 0, 0, 1 });
			break;
		}
		case SDL_SCANCODE_DOWN: {
			//printf("Tapped SDL_SCANCODE_DOWN\n");
			GE_DRAW_LIST.selectorBox.moveBy({ 0, 0, -1 });
			break;
		}
		case SDL_SCANCODE_LEFT: {
			//printf("Tapped SDL_SCANCODE_LEFT\n");
			GE_DRAW_LIST.selectorBox.moveBy({ 1, 0, 0 });
			break;
		}
		case SDL_SCANCODE_RIGHT: {
			//printf("Tapped SDL_SCANCODE_RIGHT\n");
			GE_DRAW_LIST.selectorBox.moveBy({ -1, 0, 0 });
			break;
		}
		case SDL_SCANCODE_SPACE: {
			//printf("Tapped SDL_SCANCODE_SPACE\n");
			GE_DRAW_LIST.selectorBox.moveBy({ 0, 1, 0 });
			break;
		}
		case SDL_SCANCODE_X: {
			//printf("Tapped SDL_SCANCODE_X\n");
			GE_DRAW_LIST.selectorBox.moveBy({ 0, -1, 0 });
			break;
		}
		case SDL_SCANCODE_F: {
			//printf("Tapped SDL_SCANCODE_F\n");
			CreateBlockAtSelectorPosition();
			break;
		}
		case SDL_SCANCODE_R: {
			//printf("Tapped SDL_SCANCODE_R\n");
			RemoveBlockAtSelectorPosition();
			break;
		}
		case SDL_SCANCODE_GRAVE: {
			//printf("Tapped SDL_SCANCODE_GRAVE\n");
			GE_CURRENT_KEYBOARD_CONTROL = Engine3D::KEYBOARD_CONTROL_TYPES::ALLOW_CAMERA_CONTROL;
		}
		default:
			printf("Tapped untreated key: %d\n", scancode);
			break;
		}
	}

	void CameraMovementHandle(SDL_Scancode scancode) {
		float offset = 20.0f / (float)FRAMES_PER_SECOND;
		//printf("CameraMovementHandle->");
		switch (scancode) {
			// WASD
		case SDL_SCANCODE_W: {
			//printf("Tapped SDL_SCANCODE_W\n");
			MainCamera.position.z += offset;
			break;
		}
		case SDL_SCANCODE_A: {
			//printf("Tapped SDL_SCANCODE_A\n");
			MainCamera.position.x += offset;
			break;
		}
		case SDL_SCANCODE_S: {
			//printf("Tapped SDL_SCANCODE_S\n");
			MainCamera.position.z -= offset;
			break;
		}
		case SDL_SCANCODE_D: {
			//printf("Tapped SDL_SCANCODE_D\n");
			MainCamera.position.x -= offset;
			break;
		}
		case SDL_SCANCODE_SPACE: {
			//printf("Tapped SDL_SCANCODE_SPACE\n");
			MainCamera.position.y += offset;
			break;
		}
		case SDL_SCANCODE_X: {
			//printf("Tapped SDL_SCANCODE_X\n");
			MainCamera.position.y -= offset;
			break;
		}
		case SDL_SCANCODE_UP: {
			//printf("Tapped SDL_SCANCODE_UP\n");
			MainCamera.fXRotation -= offset;
			break;
		}
		case SDL_SCANCODE_DOWN: {
			//printf("Tapped SDL_SCANCODE_DOWN\n");
			MainCamera.fXRotation += offset;
			break;
		}
		case SDL_SCANCODE_LEFT: {
			//printf("Tapped SDL_SCANCODE_LEFT\n");
			MainCamera.fYRotation -= offset;
			break;
		}
		case SDL_SCANCODE_RIGHT: {
			//printf("Tapped SDL_SCANCODE_RIGHT\n");
			MainCamera.fYRotation += offset;
			break;
		}
		case SDL_SCANCODE_LEFTBRACKET: {
			//printf("Tapped SDL_SCANCODE_RIGHT\n");
			if (MainCamera.fFOV > 0 + 1)
			{
				MainCamera.fFOV--;
			}
			printf("%f\n", MainCamera.fFOV);
			break;
		}
		case SDL_SCANCODE_RIGHTBRACKET: {
			//printf("Tapped SDL_SCANCODE_RIGHT\n");
			if (MainCamera.fFOV < 180 - 1)
			{
				MainCamera.fFOV++;
			}
			printf("%f\n", MainCamera.fFOV);
			break;
		}
		case SDL_SCANCODE_R: {
			//printf("Tapped SDL_SCANCODE_R\n");
			MainCamera.fXRotation = 0;
			MainCamera.fYRotation = 0;
			MainCamera.position.x = 0;
			MainCamera.position.y = 2;
			MainCamera.position.z = 2;
			break;
		}
		case SDL_SCANCODE_GRAVE: {
			//printf("Tapped SDL_SCANCODE_GRAVE\n");
			GE_CURRENT_KEYBOARD_CONTROL = Engine3D::KEYBOARD_CONTROL_TYPES::ALLOW_SCENE_EDITING;
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
					switch (GE_CURRENT_KEYBOARD_CONTROL) {
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
		GE_MESHES.MESH_CUBE.polygons = {
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

	void initSelectorBox() {
		GE_Object sBox;
		sBox.mesh = GE_MESHES.MESH_CUBE;
		sBox.setColor(250.0f, 211.0f, 0.0f);
		GE_DRAW_LIST.selectorBox = sBox;

		//GE_Object sBox2;
		//sBox2.mesh = GE_MESHES.MESH_CUBE;
		//GE_DRAW_LIST.obj.push_back(sBox2);
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

		initSelectorBox();

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






