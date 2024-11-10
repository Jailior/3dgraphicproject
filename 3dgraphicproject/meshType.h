/*
* meshType.h
* 
* Author: Ali Osman
* 
* Used for loading in obj files as a
* mesh of triangles.
* 
* Can be used to create other meshes.
*/


#pragma once

#include <strstream>

#include "linear.h"
using namespace std;
using namespace linear;

namespace meshType
{
	/*A grouping of triangles*/
	struct mesh
	{
		vector<triangle> tris;

		bool LoadFromObjectFile(string sFilename, bool bHasTexture = false)
		{
			ifstream f(sFilename);
			if (!f.is_open())
			{
				return false;
			}

			/*Local cache*/
			vector<vec3d> verts;
			vector<vec2d> texs;

			while (!f.eof())
			{
				char line[128];
				f.getline(line, 128);

				strstream s;
				s << line;

				char junk;

				if (line[0] == 'v')
				{
					if (line[1] == 't')
					{
						vec2d v;
						s >> junk >> junk >> v.u >> v.v;
						v.u = 1.0f - v.u;
						v.v = 1.0f - v.v;
						texs.push_back(v);
					}
					else
					{
						vec3d v;
						s >> junk >> v.x >> v.y >> v.z;
						verts.push_back(v);
					}
				}
				if (!bHasTexture)
				{
					if (line[0] == 'f')
					{
						int f[3];
						s >> junk >> f[0] >> f[1] >> f[2];
						tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
					}
				}
				else
				{
					if (line[0] == 'f')
					{
						s >> junk;

						string tokens[6];
						int nTokenCount = -1;


						while (!s.eof())
						{
							char c = s.get();
							if (c == ' ' || c == '/') {
								nTokenCount++;
							}
							else {
								tokens[nTokenCount].append(1, c);
							}
						}

						// I forgot what this does... Commenting it out doesn't seem to break anything.
						//tokens[nTokenCount].pop_back();

						tris.push_back({ verts[stoi(tokens[0]) - 1], verts[stoi(tokens[2]) - 1], verts[stoi(tokens[4]) - 1],
							texs[stoi(tokens[1]) - 1], texs[stoi(tokens[3]) - 1], texs[stoi(tokens[5]) - 1] });

					}
				}
			}
			return true;
		}
		
		/*Loads a simple sphere at position 0,0,0*/
		bool LoadSphere() {
			double radius = 1.0;
			int segments = 20;
			int rings = 20;

			vector<vec3d> verts;

			for (int i = 0; i <= rings; ++i) {
				double theta = i * 3.14159265359 / rings;
				double sinTheta = sin(theta);
				double cosTheta = cos(theta);

				for (int j = 0; j <= segments; ++j) {
					double phi = j * 2.0 * 3.14159265359 / segments;
					double sinPhi = sin(phi);
					double cosPhi = cos(phi);

					double x = radius * cosPhi * sinTheta;
					double y = radius * sinPhi * sinTheta;
					double z = radius * cosTheta;

					vec3d v;
					v.x = x;
					v.y = y;
					v.z = z;

					verts.push_back(v);
				}
			}

			// Generate triangles from vertices
			for (int i = 0; i < rings; ++i) {
				for (int j = 0; j < segments; ++j) {
					int first = i * (segments + 1) + j;
					int second = first + segments + 1;

					// Create two triangles for each face (quad split into 2 triangles)
					tris.push_back({ verts[first], verts[second], verts[first + 1] });
					tris.push_back({ verts[second], verts[second + 1], verts[first + 1] });
				}
			}
			return true;
		}
	};
}