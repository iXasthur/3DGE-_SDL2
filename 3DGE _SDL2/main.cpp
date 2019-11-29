//
//  main.cpp
//  3DGE _SDL2
//
//  Created by Михаил Ковалевский on 19.10.2019.
//  Copyright © 2019 Mikhail Kavaleuski. All rights reserved.
//

#include <iostream>
#include <vector>
#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#include <SDL.h>
#elif __APPLE__
	//define something for MacOS
	#include <SDL2/SDL.h>
#endif


class Engine3D {
private:
	struct vec3 {
		float x, y, z;
	};

	struct Triangle {
		vec3 p[3];
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
	
	struct DrawList {
		std::vector<Mesh> obj;
	};

	Mesh MESH_CUBE;

	Matrix4 matProj;

	vec3 CameraPos = { 0, 0, 0 };


	const char title[4] = "^_^";
	const int FRAMES_PER_SECOND = 30;

	int WIDTH, HEIGHT;
	const float FOV = 90.0f;
	const float fNear = 0.1f;
	const float fFar = 1000.0f;

	bool isRunning = false;

	SDL_Window *window = NULL;


	bool check_window(SDL_Window *window) {
		if (window == NULL)
		{
			printf("Error creating window! SDL_Error: %s\n", SDL_GetError());
			return false;
		}
		else {
			printf("Created window(%dx%d)!\n", WIDTH, HEIGHT);
			return true;
		}
	}

	void MultiplyVectorByMatrix(vec3 &i, vec3 &o, Matrix4 &m) {

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
	}

	void updateScreenAndCameraProperties(SDL_Renderer *renderer) {
		// Gets real size of the window(Fix for MacOS/Resizing)
		SDL_GetRendererOutputSize(renderer, &WIDTH, &HEIGHT);

		const float fFovRad = 1.0f / tanf(FOV * 0.5f / 180.0f * 3.14159f);
		const float fAspectRatio = (float)HEIGHT / (float)WIDTH;

		
		matProj.m[0][0] = fAspectRatio * fFovRad;
		matProj.m[1][1] = fFovRad;
		matProj.m[2][2] = fFar / (fFar - fNear);
		matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matProj.m[2][3] = 1.0f;
		matProj.m[3][3] = 0.0f;

	}

	void DrawTriangle2D(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
		const SDL_Point points[4] = {
										{ x1,y1 },
										{ x2,y2 },
										{ x3,y3 },
										{ x1,y1 }
									};

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

	void DrawFlatTriangle(SDL_Renderer *renderer, SDL_Point *v) {

	}

	void DrawFilledTriangle2D(SDL_Renderer *renderer, Triangle2D tr) {
		// Points are Integers
		// p[0] need to have lowest y among points
		if (!(tr.p[0].y == tr.p[1].y == tr.p[2].y)) {
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
		Matrix4 matRotZ, matRotX;

		static float fTheta = 0;
		fTheta += 1/(float)FRAMES_PER_SECOND;

		// Rotation Z
		matRotZ.m[0][0] = cosf(fTheta);
		matRotZ.m[0][1] = sinf(fTheta);
		matRotZ.m[1][0] = -sinf(fTheta);
		matRotZ.m[1][1] = cosf(fTheta);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f);
		matRotX.m[1][2] = sinf(fTheta * 0.5f);
		matRotX.m[2][1] = -sinf(fTheta * 0.5f);
		matRotX.m[2][2] = cosf(fTheta * 0.5f);
		matRotX.m[3][3] = 1;


		for (Triangle tri: MESH_CUBE.polygons) {
			Triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;;

			// Rotate in Z-Axis
			MultiplyVectorByMatrix(tri.p[0], triRotatedZ.p[0], matRotZ);
			MultiplyVectorByMatrix(tri.p[1], triRotatedZ.p[1], matRotZ);
			MultiplyVectorByMatrix(tri.p[2], triRotatedZ.p[2], matRotZ);

			// Rotate in X-Axis
			MultiplyVectorByMatrix(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
			MultiplyVectorByMatrix(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
			MultiplyVectorByMatrix(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);

			// Offset into the screen
			triTranslated = triRotatedZX;
			triTranslated.p[0].z = triRotatedZX.p[0].z + 3.0f;
			triTranslated.p[1].z = triRotatedZX.p[1].z + 3.0f;
			triTranslated.p[2].z = triRotatedZX.p[2].z + 3.0f;

			// Calculate normal
			vec3 normal, line1, line2;
			line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
			line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
			line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

			line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
			line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
			line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			// Normalize normal
			float l = sqrtf(normal.x* normal.x + normal.y* normal.y + normal.z*normal.z);
			normal.x /= l;
			normal.y /= l;
			normal.z /= l;

			if (normal.x * (triTranslated.p[0].x - CameraPos.x) + 
				normal.y * (triTranslated.p[0].y - CameraPos.y) + 
				normal.z * (triTranslated.p[0].z - CameraPos.z) < 0.0f) {
				// 3D -> 2D
				MultiplyVectorByMatrix(triTranslated.p[0], triProjected.p[0], matProj);
				MultiplyVectorByMatrix(triTranslated.p[1], triProjected.p[1], matProj);
				MultiplyVectorByMatrix(triTranslated.p[2], triProjected.p[2], matProj);

				triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;
				triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
				triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;

				triProjected.p[0].x *= 0.5f * WIDTH;
				triProjected.p[0].y *= 0.5f * HEIGHT;
				triProjected.p[1].x *= 0.5f * WIDTH;
				triProjected.p[1].y *= 0.5f * HEIGHT;
				triProjected.p[2].x *= 0.5f * WIDTH;
				triProjected.p[2].y *= 0.5f * HEIGHT;

				SDL_Point points[3] = {
								{ triProjected.p[0].x,triProjected.p[0].y },
								{ triProjected.p[1].x,triProjected.p[1].y },
								{ triProjected.p[2].x,triProjected.p[2].y }
				};
				Triangle2D tr = {points[0], points[1], points[2]};
				/*DrawTriangle2D(renderer, triProjected.p[0].x, triProjected.p[0].y,
					triProjected.p[1].x, triProjected.p[1].y,
					triProjected.p[2].x, triProjected.p[2].y);*/
				DrawFilledTriangle2D(renderer, tr);
			}
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
				if (SDL_QUIT == windowEvent.type)
				{
					isRunning = false;
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

	int initEngine() {
		SDL_Init(SDL_INIT_VIDEO);


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
			return -100;
		};

		initMeshes();

		return 0;
	}
public:
	Engine3D(int _WIDTH, int _HEIGHT) {
		WIDTH = _WIDTH;
		HEIGHT = _HEIGHT;
		initEngine();
	}
	Engine3D() {
		WIDTH = 800;
		HEIGHT = 600;
		initEngine();
	}

	void startScene() {
		isRunning = true;
		StartRenderLoop();
	}

	void destroy() {
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
};






int main(int argc, char *argv[])
{
	Engine3D Engine(800, 600);
	Engine.startScene();
	Engine.destroy();

    return EXIT_SUCCESS;
}






