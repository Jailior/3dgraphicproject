/*
* 3D Graphics Engine
* 
* Author: Ali Osman
* 
* Last Updated: 09/11/2024
*/

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "linear.h"
#include "meshType.h"

#include <fstream>
#include <strstream>
#include <algorithm>
#include <string>

using namespace std;
using namespace linear;

class olcEngine3D : public olc::PixelGameEngine
{
public:
	olcEngine3D()
	{
		sAppName = "3D Graphics Engine";
	}

private:
	meshType::mesh meshCube;
	mat4x4 matProj;

	vec3d vCamera;
	vec3d vLookDir;

	float fYaw;

	float fTheta;

	olc::Sprite *sprTex1;

	/*
		Returns an olc::Pixel of luminosity, lum.
		lum must be between 0 and 1, inclusive.
	*/
	olc::Pixel GetColour(float lum) {
		int nValue = (int)(std::max(lum, 0.20f) * 255.0f);
		return olc::Pixel(nValue, nValue, nValue);
	}

	float* pDepthBuffer = nullptr;

public:
	bool OnUserCreate() override
	{
		pDepthBuffer = new float[ScreenWidth() * ScreenHeight()];
		
		/*Loads obj file from assets*/
		meshCube.LoadFromObjectFile("./assets/spyro2.obj", true);

		/*Loads texture file from assets*/
		sprTex1 = new olc::Sprite("./assets/terrain2.png");

		matProj = linear::Matrix_MakeProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		/*
		* Camera controls:
		* W - move forward
		* A - turn left
		* S - move backward
		* D - turn right
		* 
		* Up - move upwards
		* Down - move downwards
		*/

		if (GetKey(olc::Key::UP).bHeld)
			vCamera.y += 8.0f * fElapsedTime;

		if (GetKey(olc::Key::DOWN).bHeld)
			vCamera.y -= 8.0f * fElapsedTime;

		vec3d vForward = linear::Vector_Mul(vLookDir, 8.0f * fElapsedTime);

		if (GetKey(olc::Key::W).bHeld)
			vCamera = linear::Vector_Add(vCamera, vForward);

		if (GetKey(olc::Key::S).bHeld)
			vCamera = linear::Vector_Sub(vCamera, vForward);

		if (GetKey(olc::Key::A).bHeld)
			fYaw -= 2.0f * fElapsedTime;

		if (GetKey(olc::Key::D).bHeld)
			fYaw += 2.0f * fElapsedTime;


		mat4x4 matRotZ, matRotX;

		matRotZ = linear::Matrix_MakeRotationZ(fTheta * 0.5f);
		matRotX = linear::Matrix_MakeRotationX(fTheta);

		mat4x4 matTrans;
		/*Camera starting position (x, y, z)*/
		matTrans = linear::Matrix_MakeTranslation(0.0f, -5.0f, 5.0f);

		mat4x4 matWorld;
		matWorld = linear::Matrix_MakeIdentity();
		matWorld = linear::Matrix_MultiplyMatrix(matRotZ, matRotX);
		matWorld = linear::Matrix_MultiplyMatrix(matWorld, matTrans);

		vec3d vUp = { 0, 1, 0 };
		vec3d vTarget = { 0, 0, 1 };
		mat4x4 matCameraRot = linear::Matrix_MakeRotationY(fYaw);
		vLookDir = linear::Matrix_MultiplyVector(matCameraRot, vTarget);
		vTarget = linear::Vector_Add(vCamera, vLookDir);

		mat4x4 matCamera = linear::Matrix_PointAt(vCamera, vTarget, vUp);

		mat4x4 matView = linear::Matrix_QuickInverse(matCamera);

		vector<triangle> vecTrianglesToRaster;

		/*Draw triangles*/
		for (auto tri : meshCube.tris)
		{
			triangle triProjected, triTransformed, triViewed;

			triTransformed.p[0] = linear::Matrix_MultiplyVector(matWorld, tri.p[0]);
			triTransformed.p[1] = linear::Matrix_MultiplyVector(matWorld, tri.p[1]);
			triTransformed.p[2] = linear::Matrix_MultiplyVector(matWorld, tri.p[2]);
			triTransformed.t[0] = tri.t[0];
			triTransformed.t[1] = tri.t[1];
			triTransformed.t[2] = tri.t[2];

			vec3d normal, line1, line2;
			
			line1 = linear::Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
			line2 = linear::Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

			normal = linear::Vector_CrossProduct(line1, line2);

			normal = linear::Vector_Normalize(normal);

			vec3d vCameraRay = linear::Vector_Sub(triTransformed.p[0], vCamera);


			if(linear::Vector_DotProduct(normal, vCameraRay) < 0.0f)
			{
				/*Illumination*/
				vec3d light_direction = { 0.0f, 1.0f, 1.0f };
				light_direction = linear::Vector_Normalize(light_direction);

				float dp = max(0.1f, linear::Vector_DotProduct(light_direction, normal));
				
				triTransformed.col = GetColour(dp);

				triViewed.p[0] = linear::Matrix_MultiplyVector(matView, triTransformed.p[0]);
				triViewed.p[1] = linear::Matrix_MultiplyVector(matView, triTransformed.p[1]);
				triViewed.p[2] = linear::Matrix_MultiplyVector(matView, triTransformed.p[2]);
				triViewed.col = triTransformed.col;
				triViewed.t[0] = triTransformed.t[0];
				triViewed.t[1] = triTransformed.t[1];
				triViewed.t[2] = triTransformed.t[2];

				int nClippedTriangles = 0;
				triangle clipped[2];
				nClippedTriangles = linear::Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);

				for (int n = 0; n < nClippedTriangles; n++)
				{
					triProjected.p[0] = linear::Matrix_MultiplyVector(matProj, clipped[n].p[0]);
					triProjected.p[1] = linear::Matrix_MultiplyVector(matProj, clipped[n].p[1]);
					triProjected.p[2] = linear::Matrix_MultiplyVector(matProj, clipped[n].p[2]);
					triProjected.col = clipped[n].col;
					triProjected.t[0] = clipped[n].t[0];
					triProjected.t[1] = clipped[n].t[1];
					triProjected.t[2] = clipped[n].t[2];

					triProjected.t[0].u = triProjected.t[0].u / triProjected.p[0].w;
					triProjected.t[1].u = triProjected.t[1].u / triProjected.p[1].w;
					triProjected.t[2].u = triProjected.t[2].u / triProjected.p[2].w;

					triProjected.t[0].v = triProjected.t[0].v / triProjected.p[0].w;
					triProjected.t[1].v = triProjected.t[1].v / triProjected.p[1].w;
					triProjected.t[2].v = triProjected.t[2].v / triProjected.p[2].w;

					triProjected.t[0].w = 1.0f / triProjected.p[0].w;
					triProjected.t[1].w = 1.0f / triProjected.p[1].w;
					triProjected.t[2].w = 1.0f / triProjected.p[2].w;

					triProjected.p[0] = linear::Vector_Div(triProjected.p[0], triProjected.p[0].w);
					triProjected.p[1] = linear::Vector_Div(triProjected.p[1], triProjected.p[1].w);
					triProjected.p[2] = linear::Vector_Div(triProjected.p[2], triProjected.p[2].w);

					// X/Y are inverted so un-invert them
					triProjected.p[0].x *= -1.0f;
					triProjected.p[1].x *= -1.0f;
					triProjected.p[2].x *= -1.0f;
					triProjected.p[0].y *= -1.0f;
					triProjected.p[1].y *= -1.0f;
					triProjected.p[2].y *= -1.0f;

					// Scale into view
					vec3d vOffsetView = { 1, 1, 0 };
					triProjected.p[0] = linear::Vector_Add(triProjected.p[0], vOffsetView);
					triProjected.p[1] = linear::Vector_Add(triProjected.p[1], vOffsetView);
					triProjected.p[2] = linear::Vector_Add(triProjected.p[2], vOffsetView);


					triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
					triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
					triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
					triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

					vecTrianglesToRaster.push_back(triProjected);
				}
			}
		}

		/*Sort triangles back to front*/
		sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
			{
				float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
				float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
				return z1 > z2;
			});

		// Clear Screen
		FillRect(0, 0, ScreenWidth() - 1, ScreenHeight(), olc::DARK_CYAN);

		// Rest depth buffer
		for (int i = 0; i < ScreenWidth() * ScreenHeight(); i++)
			pDepthBuffer[i] = 0.0f;

		for (auto &triToRaster : vecTrianglesToRaster)
		{
			triangle clipped[2];
			list<triangle> listTriangles;

			// Add initial triangle
			listTriangles.push_back(triToRaster);
			int nNewTriangles = 1;

			for (int p = 0; p < 4; p++)
			{
				int nTrisToAdd = 0;
				while (nNewTriangles > 0)
				{
					// Take triangle from front of queue
					triangle test = listTriangles.front();
					listTriangles.pop_front();
					nNewTriangles--;

					switch (p)
					{
						case 0:	nTrisToAdd = linear::Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
						case 1:	nTrisToAdd = linear::Triangle_ClipAgainstPlane({ 0.0f, (float)ScreenHeight() - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
						case 2:	nTrisToAdd = linear::Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
						case 3:	nTrisToAdd = linear::Triangle_ClipAgainstPlane({ (float)ScreenWidth() - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					}


					for (int w = 0; w < nTrisToAdd; w++)
						listTriangles.push_back(clipped[w]);
				}
				nNewTriangles = listTriangles.size();
			}


			/*Draw textured triangles*/
			for (auto &t: listTriangles)
			{
				TexturedTriangle(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
					t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
					t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w, sprTex1);

				/*Use to fill all triangles with set color*/
				//FillTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, t.col);
				
				/*Use to draw outlines of triangles with set color*/
				//DrawTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, olc::WHITE);
			}
		}

		return true;
	}


	/*
	* Draw a textured triangle given 3 points 
	* and texture information for all 3 points.
	*/
	void TexturedTriangle(int x1, int y1, float u1, float v1, float w1,
		int x2, int y2, float u2, float v2, float w2,
		int x3, int y3, float u3, float v3, float w3,
		olc::Sprite *tex)
	{
		if (y2 < y1)
		{
			swap(y1, y2);
			swap(x1, x2);
			swap(u1, u2);
			swap(v1, v2);
			swap(w1, w2);
		}

		if (y3 < y1)
		{
			swap(y1, y3);
			swap(x1, x3);
			swap(u1, u3);
			swap(v1, v3);
			swap(w1, w3);
		}

		if (y3 < y2)
		{
			swap(y2, y3);
			swap(x2, x3);
			swap(u2, u3);
			swap(v2, v3);
			swap(w2, w3);
		}

		int dy1 = y2 - y1;
		int dx1 = x2 - x1;
		float dv1 = v2 - v1;
		float du1 = u2 - u1;
		float dw1 = w2 - w1;

		int dy2 = y3 - y1;
		int dx2 = x3 - x1;
		float dv2 = v3 - v1;
		float du2 = u3 - u1;
		float dw2 = w3 - w1;

		float tex_u, tex_v, tex_w;

		float dax_step = 0, dbx_step = 0,
			du1_step = 0, dv1_step = 0,
			du2_step = 0, dv2_step = 0,
			dw1_step = 0, dw2_step = 0;

		if (dy1) dax_step = dx1 / (float)abs(dy1);
		if (dy2) dbx_step = dx2 / (float)abs(dy2);

		if (dy1) du1_step = du1 / (float)abs(dy1);
		if (dy1) dv1_step = dv1 / (float)abs(dy1);
		if (dy1) dw1_step = dw1 / (float)abs(dy1);

		if (dy2) du2_step = du2 / (float)abs(dy2);
		if (dy2) dv2_step = dv2 / (float)abs(dy2);
		if (dy2) dw2_step = dw2 / (float)abs(dy2);

		if (dy1)
		{
			for (int i = y1; i <= y2; i++)
			{
				int ax = x1 + (float)(i - y1) * dax_step;
				int bx = x1 + (float)(i - y1) * dbx_step;

				float tex_su = u1 + (float)(i - y1) * du1_step;
				float tex_sv = v1 + (float)(i - y1) * dv1_step;
				float tex_sw = w1 + (float)(i - y1) * dw1_step;

				float tex_eu = u1 + (float)(i - y1) * du2_step;
				float tex_ev = v1 + (float)(i - y1) * dv2_step;
				float tex_ew = w1 + (float)(i - y1) * dw2_step;

				if (ax > bx)
				{
					swap(ax, bx);
					swap(tex_su, tex_eu);
					swap(tex_sv, tex_ev);
					swap(tex_sw, tex_ew);
				}

				tex_u = tex_su;
				tex_v = tex_sv;
				tex_w = tex_sw;

				float tstep = 1.0f / ((float)(bx - ax));
				float t = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					tex_u = (1.0f - t) * tex_su + t * tex_eu;
					tex_v = (1.0f - t) * tex_sv + t * tex_ev;
					tex_w = (1.0f - t) * tex_sw + t * tex_ew;

					if (tex_w > pDepthBuffer[i * ScreenWidth() + j]) {
						Draw(j, i, tex->Sample(tex_u / tex_w, tex_v / tex_w));
						pDepthBuffer[i * ScreenWidth() + j] = tex_w;
					}

					t += tstep;
				}

			}
		}

		dy1 = y3 - y2;
		dx1 = x3 - x2;
		dv1 = v3 - v2;
		du1 = u3 - u2;
		dw1 = w3 - w2;

		if (dy1) dax_step = dx1 / (float)abs(dy1);
		if (dy2) dbx_step = dx2 / (float)abs(dy2);

		du1_step = 0, dv1_step = 0;
		if (dy1) du1_step = du1 / (float)abs(dy1);
		if (dy1) dv1_step = dv1 / (float)abs(dy1);
		if (dy1) dw1_step = dw1 / (float)abs(dy1);

		if (dy1)
		{
			for (int i = y2; i <= y3; i++)
			{
				int ax = x2 + (float)(i - y2) * dax_step;
				int bx = x1 + (float)(i - y1) * dbx_step;

				float tex_su = u2 + (float)(i - y2) * du1_step;
				float tex_sv = v2 + (float)(i - y2) * dv1_step;
				float tex_sw = w2 + (float)(i - y2) * dw1_step;

				float tex_eu = u1 + (float)(i - y1) * du2_step;
				float tex_ev = v1 + (float)(i - y1) * dv2_step;
				float tex_ew = w1 + (float)(i - y1) * dw2_step;

				if (ax > bx)
				{
					swap(ax, bx);
					swap(tex_su, tex_eu);
					swap(tex_sv, tex_ev);
					swap(tex_sw, tex_ew);
				}

				tex_u = tex_su;
				tex_v = tex_sv;
				tex_w = tex_sw;

				float tstep = 1.0f / ((float)(bx - ax));
				float t = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					tex_u = (1.0f - t) * tex_su + t * tex_eu;
					tex_v = (1.0f - t) * tex_sv + t * tex_ev;
					tex_w = (1.0f - t) * tex_sw + t * tex_ew;

					if (tex_w > pDepthBuffer[i * ScreenWidth() + j]) {
						Draw(j, i, tex->Sample(tex_u / tex_w, tex_v / tex_w));
						pDepthBuffer[i * ScreenWidth() + j] = tex_w;
					}

					t += tstep;
				}

			}

		}
	}
};

int main() {

	// Create screen
	olcEngine3D demo;

	// Construct(width, height, pixel width, pixel height)
	// Control Resolution
	if (demo.Construct(256*4, 240*4, 1, 1))
		demo.Start();

	return 0;
}