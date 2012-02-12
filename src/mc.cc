/*
 * Made by Simon using Paul Bourke's tables
 * 
 * ISSUES:
 * memoization error on x.oy registers
 * 		offset: vec3i(-15, -15, -15)
 *		size: vec3i(30, 30, 30)
 *		density: x*x + 2*x + 17*y - z*z
 *
 * some triangles are generated with equal vertices
 *
 */

#include "mc.hh"
#include "mc_table.hh"
#include <stdlib.h>

vec3f middle (vec3f a, vec3f b) {
	return vec3f((a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2);
}

#define ISOLEVEL 0
vec3f linear_interpolation (unsigned char a, float va, unsigned char b, float vb) {
	const vec3f cube[8] = {
		vec3f(0, 0, 0),
		vec3f(1, 0, 0),
		vec3f(1, 0, 1),
		vec3f(0, 0, 1),
		vec3f(0, 1, 0),
		vec3f(1, 1, 0),
		vec3f(1, 1, 1),
		vec3f(0, 1, 1)
	};
	
	if (a != b)
		return cube[a] + (cube[b] - cube[a]) / (vb - va) * (ISOLEVEL - va);
	else 
		return cube[a];
}

// memo

float density (const vec3f p) {
	/*x -= p.x;
	y -= p.y;
	z -= p.z;*/
	return p.x * p.x + 2 * p.x + 17 * p.y - p.z * p.z; // TODO: take sphere position into account
	//return p.length() - 10;
}

// TODO: remove
struct edge_plane {
	int* oy; // vertical edges
	int* oz; // horizontal edges
};

// marching cube

void marching_cube (const vec3i offset, const vec3i size, // input
					std::vector<vec3f>& positions, std::vector<vec3s>& normals, std::vector<uint>& triangles) { // output
	const uchar edg[][2] = {
		{0, 1},
		{1, 2},
		{2, 3},
		{3, 0},
		{4, 5},
		{5, 6},
		{6, 7},
		{7, 4},
		{0, 4},
		{1, 5},
		{2, 6},
		{3, 7}
	};
	
	// temp		
	float grid[size.x][size.y][size.z];
	for (int i = 0; i < size.x; i++)  		 //x axis
		for (int j = 0; j < size.y; j++)		 //y axis
			for (int k = 0; k < size.z; k++) //z axis
				grid[i][j][k] = density(vec3f(offset.x + i, offset.y + j, offset.z + k));
	
	positions.clear(); // TODO rm?
	normals.clear();
	triangles.clear(); // TODO rm?

	std::vector<vec3f> normals_f;

	// X-axis memoization (YZ plane)
	edge_plane memo_x = {
		(int*)alloca((size.y) * size.z * sizeof(int)),
		(int*)alloca(size.y * (size.z) * sizeof(int))
	};

	std::memset(memo_x.oy, -1, (size.y) * size.z * sizeof(int));
	std::memset(memo_x.oz, -1, size.y * (size.z) * sizeof(int));
	
	// X-AXIS
	for (int i = 0; i < size.x - 1; i++) { 		//x axis
		// X-axis memoization (YZ plane)
		edge_plane next_x = {
			(int*)alloca((size.y) * size.z * sizeof(int)),
			(int*)alloca(size.y * (size.z) * sizeof(int))
		};

		std::memset(next_x.oy, -1, (size.y) * size.z * sizeof(int));
		std::memset(next_x.oz, -1, size.y * (size.z) * sizeof(int));
		
		// Y-axis memoization: indexes of the previous z-line
		int memo_y[3 * (size.y - 1) + 1]; // TODO: check if not 3 * Y + 1 ???
		std::memset(memo_y, -1, (3 * (size.y - 1) + 1) * sizeof(int));

		// Y-AXIS
		for (int j = 0; j < size.y - 1; j++) {
			// memoization: indexes of the previous z-line
			int next_y[3 * (size.y - 1) + 1];
			std::memset(next_y, -1, (3 * (size.y - 1) + 1) * sizeof(int));

			//z axis
			for (int k = 0; k < size.z - 1; k++) {
				float val[8] = { // fetch the value of the eight vertices of the cube
					grid[i    ][j    ][k    ],
					grid[i + 1][j    ][k    ],
					grid[i + 1][j    ][k + 1],
					grid[i    ][j    ][k + 1],
					grid[i    ][j + 1][k    ],
					grid[i + 1][j + 1][k    ],
					grid[i + 1][j + 1][k + 1],
					grid[i    ][j + 1][k + 1] 
				};
				
				// get the index representing the cube's vertices configuration
				uchar index = 0;
				for (int n = 0; n < 8; n++)
					if (val[n] <= ISOLEVEL) index |= (1 << n); // set nth bit to 1

				//check if the cube is completely inside or outside the volume
    			if (edge_table[index] == 0) continue; // || edge_table[index] ??

				// retrieve indexes in the vertices array of the previously built vertices from the memoization register
				int memo_cube[12] = {
					/*  0 */ memo_y[3 * k],
					/*  1 */ next_x.oz[j * size.z + k], //
					/*  2 */ memo_y[3 * k + 3],
					/*  3 */ memo_x.oz[j * size.z + k],
					/*  4 */ next_y[3 * k],
					/*  5 */ -1,
					/*  6 */ -1,
					/*  7 */ memo_x.oz[(j + 1) * size.z + k],
					/*  8 */ memo_x.oy[j * (size.z - 1) + k], // BUG
					/*  9 */ next_x.oy[j * (size.z - 1) + k], // BUG
					/* 10 */ -1,
					/* 11 */ memo_x.oy[j * (size.z - 1) + k + 1] // BUG
				};

				// get the origin corner of the cube
				vec3f origin (offset.x + i, offset.y + j, offset.z + k);

				// build the triangles using tri_table
				for (int n = 0; tri_table[index][n] != -1; n += 3) {
					uint v[3]; // 3 vertices of the contructed triangle, used for normals calculation
					
					// add the 3 vertices to the triangles array (create vertices if required)
					for (int m = n + 2; m >= n; m--) { // reverse indexes browsing for CW faces
						int e = tri_table[index][m]; // retrieve the edge's cube-index

						// check if the vertex has already been created
						// create it and save it to the register if not
						if (memo_cube[e] == -1) { // not memoized
							// construct the triangle's vertex and save it to the cube register
							memo_cube[e] = positions.size();
							positions.push_back(origin + linear_interpolation(edg[e][0], val[edg[e][0]], edg[e][1], val[edg[e][1]]));
							normals_f.push_back(vec3f(0, 0, 0));
							//v.specular = vec3ub(rand() % 255, rand() % 255, rand() % 255);
						}
				
						// add the vertex index to the element array
						v[m - n] = memo_cube[e];
						triangles.push_back(memo_cube[e]);
					}
					
					// add the normalized face normal to the vertices normals
					// TODO
					vec3f fn = cross(positions[v[1]] - positions[v[0]], positions[v[2]] - positions[v[0]]); // compute the triangle's normal
					if (fn != vec3f(0, 0, 0)) { // TODO / BUG
						fn.normalize();
						for (int m = 0; m < 3; m++)
							normals_f[v[m]] += fn;
					}
				}
				
				// save current cube into memoization registers
				/*  0 */ memo_y[3 * k] = memo_cube[0];
				/*  1 */ next_x.oz[j * size.z + k] = memo_cube[1];
				/*  2 */ memo_y[3 * k + 3] = memo_cube[2];
				/*  3 */ memo_x.oz[j * size.z + k] = memo_cube[3];
				/*  4 */ next_y[3 * k] = memo_cube[4];
				/*  5 */ next_x.oz[(j + 1) * size.z + k] = next_y[3 * k + 1] = memo_cube[5]; //
				/*  6 */ next_y[3 * k + 3] = memo_cube[6];
				/*  7 */ next_y[3 * k + 2] = memo_x.oz[(j + 1) * size.z + k] = memo_cube[7]; //
				/*  8 */ memo_x.oy[j * (size.z - 1) + k] = memo_cube[8];
				/*  9 */ next_x.oy[j * (size.z - 1) + k] = memo_cube[9];
				/* 10 */ next_x.oy[j * (size.z - 1) + k + 1] = memo_cube[10];
				/* 11 */ memo_x.oy[j * (size.z - 1) + k + 1] = memo_cube[11];
			} // end of Z-loop

			std::memcpy(memo_y, next_y, (3 * (size.y - 1) + 1) * sizeof(int));
		} // end of Y-loop

		std::memcpy(memo_x.oy, next_x.oy, size.y * size.z * sizeof(int));
		std::memcpy(memo_x.oz, next_x.oz, size.y * size.z * sizeof(int));
	} // end of X-loop
	
	// browse every vertices normals and normalize them
	// TODO
	for (uint i = 0; i < normals_f.size(); i++) {
		normals_f[i].normalize();
		normals.push_back(vec3s(normals_f[i].x * 32767, normals_f[i].y * 32767, normals_f[i].z * 32767));
		if (normals[i] == vec3s(0, 0, 0)) {
			outlog(normals_f[i]);
		}
	}
	
	assert(positions.size() == normals.size());
}