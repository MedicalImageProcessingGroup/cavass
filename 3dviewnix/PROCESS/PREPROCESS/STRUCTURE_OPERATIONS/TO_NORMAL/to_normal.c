/*
  Copyright 1993-2014 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <math.h>
#include <string.h>
#include <limits.h>
#include <cv3dv.h>
#include "neighbors.h"

#define ROOT2 0.7071067811865475244008443
#define ROOT3 0.5773502691896257645091487

typedef struct { unsigned short c[2]; } Cord_with_Norm;
#define POSITIVE (unsigned short)(0x8000)
#define C_MASK (unsigned short)(0x7FFF)


/*
File  : marchingCubesGL.cpp
Author: George J. Grevera
Date  : 6/16/97
Desc. : Based on "Marching Cubes: A High Resolution 3D Surface Construction
        Algorithm," W.E. Lorensen, and H.E. Cline, Computer Graphics, 21(4),
        7/87, pp. 163-169.

        coordinate system:

             z
            +
           /
          /
         /
         ----- +x
        |
        |
        |
        +
        y

        vertices:

            v8-------v7
           /|        /|
          / |       / |
         /  |      /  |
        v4-------v3   |
        |   |     |   |
        |   v5----|--v6
        |  /      |  /
        | /       | /
        |/        |/
        v1-------v2

        edges:

            .----e7---.
           /|        /|
         e11|      e12|
         /  e8     /  e6
        .----e3---.   |
        |   |     |   |
        |   .---e5|---.
        e4 /      e2 /
        | e9      |e10
        |/        |/
        .----e1---.

//-----------------------------------------------------------------------
// each row corresponds to a configuration of a marching cube.  each column
// entry corresponds to an edge and will become the vertex of a triangle
// (once it has been interpolated along the edge).  then triples of these
// form the triangle. */

static const int triangle_edges[189][3] = {
{ 1,  2,  6},
{ 1,  2,  7},
{ 1,  2,  8},
{ 1,  2,  9},
{ 1,  2, 10},
{ 1,  2, 11},
{ 1,  2, 12},
{ 1,  3,  5},
{ 1,  3,  6},
{ 1,  3,  7},
{ 1,  3,  8},
{ 1,  3,  9},
{ 1,  3, 10},
{ 1,  3, 11},
{ 1,  3, 12},
{ 1,  4,  5},
{ 1,  4,  6},
{ 1,  4,  7},
{ 1,  4,  8},
{ 1,  4,  9},
{ 1,  4, 10},
{ 1,  4, 11},
{ 1,  4, 12},
{ 1,  5,  6},
{ 1,  5,  7},
{ 1,  5,  8},
{ 1,  5, 11},
{ 1,  5, 12},
{ 1,  6,  7},
{ 1,  6,  8},
{ 1,  6,  9},
{ 1,  6, 10},
{ 1,  6, 11},
{ 1,  6, 12},
{ 1,  7,  8},
{ 1,  7,  9},
{ 1,  7, 10},
{ 1,  7, 11},
{ 1,  7, 12},
{ 1,  8,  9},
{ 1,  8, 10},
{ 1,  8, 11},
{ 1,  8, 12},
{ 1,  9, 11},
{ 1,  9, 12},
{ 1, 10, 11},
{ 1, 10, 12},
{ 1, 11, 12},
{ 2,  3,  5},
{ 2,  3,  7},
{ 2,  3,  8},
{ 2,  3,  9},
{ 2,  3, 10},
{ 2,  3, 11},
{ 2,  3, 12},
{ 2,  4,  5},
{ 2,  4,  6},
{ 2,  4,  7},
{ 2,  4,  8},
{ 2,  4,  9},
{ 2,  4, 10},
{ 2,  4, 11},
{ 2,  4, 12},
{ 2,  5,  6},
{ 2,  5,  7},
{ 2,  5,  8},
{ 2,  5,  9},
{ 2,  5, 10},
{ 2,  5, 11},
{ 2,  5, 12},
{ 2,  6,  7},
{ 2,  6,  8},
{ 2,  6,  9},
{ 2,  6, 11},
{ 2,  7,  8},
{ 2,  7, 10},
{ 2,  7, 11},
{ 2,  7, 12},
{ 2,  8,  9},
{ 2,  8, 10},
{ 2,  8, 11},
{ 2,  8, 12},
{ 2,  9, 10},
{ 2,  9, 11},
{ 2,  9, 12},
{ 2, 10, 11},
{ 2, 11, 12},
{ 3,  4,  5},
{ 3,  4,  6},
{ 3,  4,  7},
{ 3,  4,  8},
{ 3,  4,  9},
{ 3,  4, 10},
{ 3,  4, 11},
{ 3,  4, 12},
{ 3,  5,  6},
{ 3,  5,  7},
{ 3,  5,  8},
{ 3,  5,  9},
{ 3,  5, 10},
{ 3,  5, 11},
{ 3,  5, 12},
{ 3,  6,  7},
{ 3,  6,  8},
{ 3,  6,  9},
{ 3,  6, 10},
{ 3,  6, 11},
{ 3,  6, 12},
{ 3,  7,  8},
{ 3,  7,  9},
{ 3,  7, 10},
{ 3,  8,  9},
{ 3,  8, 10},
{ 3,  8, 11},
{ 3,  8, 12},
{ 3,  9, 10},
{ 3,  9, 11},
{ 3,  9, 12},
{ 3, 10, 11},
{ 3, 10, 12},
{ 4,  5,  6},
{ 4,  5,  7},
{ 4,  5,  8},
{ 4,  5,  9},
{ 4,  5, 10},
{ 4,  5, 11},
{ 4,  5, 12},
{ 4,  6,  7},
{ 4,  6,  8},
{ 4,  6,  9},
{ 4,  6, 10},
{ 4,  6, 11},
{ 4,  6, 12},
{ 4,  7,  8},
{ 4,  7,  9},
{ 4,  7, 10},
{ 4,  7, 11},
{ 4,  7, 12},
{ 4,  8, 10},
{ 4,  8, 12},
{ 4,  9, 12},
{ 4, 10, 11},
{ 4, 10, 12},
{ 4, 11, 12},
{ 5,  6,  9},
{ 5,  6, 10},
{ 5,  6, 11},
{ 5,  6, 12},
{ 5,  7,  9},
{ 5,  7, 10},
{ 5,  7, 12},
{ 5,  8,  9},
{ 5,  8, 10},
{ 5,  8, 11},
{ 5,  8, 12},
{ 5,  9, 11},
{ 5,  9, 12},
{ 5, 10, 11},
{ 5, 10, 12},
{ 5, 11, 12},
{ 6,  7,  9},
{ 6,  7, 10},
{ 6,  7, 11},
{ 6,  7, 12},
{ 6,  8, 10},
{ 6,  8, 11},
{ 6,  8, 12},
{ 6,  9, 10},
{ 6,  9, 11},
{ 6,  9, 12},
{ 6, 10, 11},
{ 6, 11, 12},
{ 7,  8,  9},
{ 7,  8, 10},
{ 7,  8, 11},
{ 7,  8, 12},
{ 7,  9, 10},
{ 7,  9, 11},
{ 7,  9, 12},
{ 7, 10, 11},
{ 8,  9, 10},
{ 8,  9, 12},
{ 8, 10, 11},
{ 8, 10, 12},
{ 8, 11, 12},
{ 9, 10, 11},
{ 9, 10, 12},
{ 9, 11, 12},
{10, 11, 12}};

static const int triangle_table[255][6]={
{-1}, /* 0 */
{19, -1}, /* 1 */
{4, -1}, /* 2 */
{82, 59, -1}, /* 3 */
{54, -1}, /* 4 */
{19, 54, -1}, /* 5 */
{119, 12, -1}, /* 6 */
{91, 117, 186, -1}, /* 7 */
{93, -1}, /* 8 */
{43, 13, -1}, /* 9 */
{4, 93, -1}, /* 10 */
{53, 85, 185, -1}, /* 11 */
{62, 143, -1}, /* 12 */
{6, 44, 187, -1}, /* 13 */
{20, 141, 188, -1}, /* 14 */
{186, 187, -1}, /* 15 */
{151, -1}, /* 16 */
{15, 122, -1}, /* 17 */
{151, 4, -1}, /* 18 */
{67, 65, 58, -1}, /* 19 */
{151, 54, -1}, /* 20 */
{122, 15, 54, -1}, /* 21 */
{119, 12, 151, -1}, /* 22 */
{122, 126, 158, 94, -1}, /* 23 */
{151, 93, -1}, /* 24 */
{153, 100, 7, -1}, /* 25 */
{4, 151, 93, -1}, /* 26 */
{53, 85, 182, 152, -1}, /* 27 */
{62, 143, 151, -1}, /* 28 */
{25, 42, 184, 6, -1}, /* 29 */
{151, 45, 188, 21, -1}, /* 30 */
{153, 157, 188, -1}, /* 31 */
{145, -1}, /* 32 */
{145, 19, -1}, /* 33 */
{0, 23, -1}, /* 34 */
{144, 129, 56, -1}, /* 35 */
{145, 54, -1}, /* 36 */
{145, 19, 54, -1}, /* 37 */
{107, 95, 7, -1}, /* 38 */
{91, 98, 101, 147, -1}, /* 39 */
{145, 93, -1}, /* 40 */
{13, 43, 145, -1}, /* 41 */
{23, 0, 93, -1}, /* 42 */
{155, 68, 63, 53, -1}, /* 43 */
{143, 62, 145, -1}, /* 44 */
{145, 84, 187, 3, -1}, /* 45 */
{171, 32, 23, 21, -1}, /* 46 */
{144, 169, 187, -1}, /* 47 */
{180, 164, -1}, /* 48 */
{20, 130, 128, -1}, /* 49 */
{39, 2, 71, -1}, /* 50 */
{71, 58, -1}, /* 51 */
{180, 164, 54, -1}, /* 52 */
{54, 31, 16, 128, -1}, /* 53 */
{166, 42, 14, 39, -1}, /* 54 */
{107, 88, 128, -1}, /* 55 */
{164, 180, 93, -1}, /* 56 */
{12, 112, 164, 113, -1}, /* 57 */
{93, 3, 78, 71, -1}, /* 58 */
{53, 80, 71, -1}, /* 59 */
{86, 61, 164, 180, -1}, /* 60 */
{29, 31, 41, 6, 47, -1}, /* 61 */
{47, 21, 33, 39, 29, -1}, /* 62 */
{165, 171, -1}, /* 63 */
{163, -1}, /* 64 */
{163, 19, -1}, /* 65 */
{4, 163, -1}, /* 66 */
{59, 82, 163, -1}, /* 67 */
{70, 49, -1}, /* 68 */
{70, 49, 19, -1}, /* 69 */
{161, 36, 9, -1}, /* 70 */
{102, 104, 167, 91, -1}, /* 71 */
{163, 93, -1}, /* 72 */
{43, 13, 163, -1}, /* 73 */
{163, 4, 93, -1}, /* 74 */
{163, 52, 118, 185, -1}, /* 75 */
{136, 127, 56, -1}, /* 76 */
{0, 32, 43, 162, -1}, /* 77 */
{136, 127, 16, 31, -1}, /* 78 */
{161, 179, 185, -1}, /* 79 */
{163, 151, -1}, /* 80 */
{15, 122, 163, -1}, /* 81 */
{151, 4, 163, -1}, /* 82 */
{163, 152, 79, 58, -1}, /* 83 */
{49, 70, 151, -1}, /* 84 */
{25, 18, 70, 49, -1}, /* 85 */
{151, 31, 28, 9, -1}, /* 86 */
{138, 152, 92, 161, 110, -1}, /* 87 */
{151, 163, 93, -1}, /* 88 */
{163, 97, 7, 113, -1}, /* 89 */
{93, 4, 151, 163, -1}, /* 90 */
{52, 118, 157, 153, 163, -1}, /* 91 */
{151, 162, 131, 56, -1}, /* 92 */
{73, 162, 5, 153, 26, -1}, /* 93 */
{162, 131, 130, 20, 151, -1}, /* 94 */
{161, 179, 152, 182, -1}, /* 95 */
{150, 158, -1}, /* 96 */
{150, 158, 19, -1}, /* 97 */
{6, 38, 24, -1}, /* 98 */
{150, 126, 62, 123, -1}, /* 99 */
{67, 48, 96, -1}, /* 100 */
{19, 99, 96, 52, -1}, /* 101 */
{24, 9, -1}, /* 102 */
{91, 98, 96, -1}, /* 103 */
{158, 150, 93, -1}, /* 104 */
{11, 116, 158, 150, -1}, /* 105 */
{93, 77, 1, 24, -1}, /* 106 */
{64, 77, 66, 53, 83, -1}, /* 107 */
{60, 135, 149, 136, -1}, /* 108 */
{83, 3, 76, 67, 64, -1}, /* 109 */
{136, 17, 24, -1}, /* 110 */
{148, 177, -1}, /* 111 */
{175, 181, 186, -1}, /* 112 */
{46, 42, 18, 175, -1}, /* 113 */
{6, 44, 178, 172, -1}, /* 114 */
{175, 81, 58, -1}, /* 115 */
{180, 112, 108, 52, -1}, /* 116 */
{110, 52, 173, 20, 138, -1}, /* 117 */
{39, 34, 9, -1}, /* 118 */
{108, 90, -1}, /* 119 */
{93, 172, 178, 186, -1}, /* 120 */
{10, 113, 40, 175, 183, -1}, /* 121 */
{172, 178, 44, 6, 93, -1}, /* 122 */
{53, 80, 77, 74, -1}, /* 123 */
{176, 172, 75, 136, 57, -1}, /* 124 */
{4, 174, -1}, /* 125 */
{136, 17, 172, 35, -1}, /* 126 */
{174, -1}, /* 127 */
{174, -1}, /* 128 */
{174, 19, -1}, /* 129 */
{4, 174, -1}, /* 130 */
{82, 59, 174, -1}, /* 131 */
{54, 174, -1}, /* 132 */
{19, 174, 54, -1}, /* 133 */
{12, 119, 174, -1}, /* 134 */
{174, 140, 186, 94, -1}, /* 135 */
{108, 90, -1}, /* 136 */
{39, 34, 9, -1}, /* 137 */
{108, 90, 4, -1}, /* 138 */
{180, 112, 108, 52, -1}, /* 139 */
{175, 81, 58, -1}, /* 140 */
{6, 44, 178, 172, -1}, /* 141 */
{46, 42, 18, 175, -1}, /* 142 */
{175, 181, 186, -1}, /* 143 */
{148, 177, -1}, /* 144 */
{136, 17, 24, -1}, /* 145 */
{177, 148, 4, -1}, /* 146 */
{60, 135, 149, 136, -1}, /* 147 */
{148, 177, 54, -1}, /* 148 */
{54, 37, 24, 21, -1}, /* 149 */
{46, 14, 148, 177, -1}, /* 150 */
{142, 94, 124, 136, 121, -1}, /* 151 */
{91, 98, 96, -1}, /* 152 */
{24, 9, -1}, /* 153 */
{4, 123, 87, 96, -1}, /* 154 */
{67, 48, 96, -1}, /* 155 */
{150, 126, 62, 123, -1}, /* 156 */
{6, 38, 24, -1}, /* 157 */
{121, 123, 137, 20, 142, -1}, /* 158 */
{150, 158, -1}, /* 159 */
{145, 174, -1}, /* 160 */
{145, 174, 19, -1}, /* 161 */
{0, 23, 174, -1}, /* 162 */
{174, 120, 56, 123, -1}, /* 163 */
{145, 54, 174, -1}, /* 164 */
{145, 174, 19, 54, -1}, /* 165 */
{174, 147, 101, 7, -1}, /* 166 */
{147, 101, 98, 91, 174, -1}, /* 167 */
{90, 108, 145, -1}, /* 168 */
{145, 35, 9, 172, -1}, /* 169 */
{133, 89, 23, 0, -1}, /* 170 */
{109, 172, 51, 144, 72, -1}, /* 171 */
{145, 74, 58, 77, -1}, /* 172 */
{3, 84, 181, 175, 145, -1}, /* 173 */
{27, 147, 22, 175, 139, -1}, /* 174 */
{175, 181, 147, 156, -1}, /* 175 */
{161, 179, 185, -1}, /* 176 */
{136, 127, 16, 31, -1}, /* 177 */
{0, 32, 43, 162, -1}, /* 178 */
{136, 127, 56, -1}, /* 179 */
{54, 170, 185, 162, -1}, /* 180 */
{31, 16, 127, 136, 54, -1}, /* 181 */
{168, 162, 30, 107, 8, -1}, /* 182 */
{107, 88, 162, 131, -1}, /* 183 */
{102, 104, 167, 91, -1}, /* 184 */
{161, 36, 9, -1}, /* 185 */
{72, 3, 160, 91, 109, -1}, /* 186 */
{70, 49, -1}, /* 187 */
{57, 77, 134, 161, 176, -1}, /* 188 */
{6, 38, 31, 28, -1}, /* 189 */
{163, 19, -1}, /* 190 */
{163, -1}, /* 191 */
{165, 171, -1}, /* 192 */
{171, 165, 19, -1}, /* 193 */
{165, 171, 4, -1}, /* 194 */
{166, 184, 82, 59, -1}, /* 195 */
{53, 80, 71, -1}, /* 196 */
{19, 50, 71, 113, -1}, /* 197 */
{12, 112, 164, 113, -1}, /* 198 */
{103, 113, 105, 91, 115, -1}, /* 199 */
{107, 88, 128, -1}, /* 200 */
{166, 42, 14, 39, -1}, /* 201 */
{4, 132, 128, 94, -1}, /* 202 */
{115, 52, 111, 107, 103, -1}, /* 203 */
{71, 58, -1}, /* 204 */
{39, 2, 71, -1}, /* 205 */
{20, 130, 128, -1}, /* 206 */
{180, 164, -1}, /* 207 */
{144, 169, 187, -1}, /* 208 */
{171, 32, 23, 21, -1}, /* 209 */
{4, 147, 156, 187, -1}, /* 210 */
{159, 147, 125, 67, 55, -1}, /* 211 */
{155, 68, 63, 53, -1}, /* 212 */
{26, 21, 146, 53, 73, -1}, /* 213 */
{8, 31, 106, 144, 168, -1}, /* 214 */
{145, 93, -1}, /* 215 */
{91, 98, 101, 147, -1}, /* 216 */
{107, 95, 7, -1}, /* 217 */
{123, 87, 95, 107, 4, -1}, /* 218 */
{67, 48, 147, 101, -1}, /* 219 */
{144, 129, 56, -1}, /* 220 */
{0, 23, -1}, /* 221 */
{144, 129, 31, 16, -1}, /* 222 */
{145, -1}, /* 223 */
{153, 157, 188, -1}, /* 224 */
{19, 152, 182, 188, -1}, /* 225 */
{25, 42, 184, 6, -1}, /* 226 */
{55, 123, 69, 153, 159, -1}, /* 227 */
{53, 85, 182, 152, -1}, /* 228 */
{152, 182, 85, 53, 19, -1}, /* 229 */
{153, 100, 7, -1}, /* 230 */
{153, 100, 123, 87, -1}, /* 231 */
{122, 126, 158, 94, -1}, /* 232 */
{183, 152, 114, 39, 10, -1}, /* 233 */
{139, 94, 154, 6, 27, -1}, /* 234 */
{151, 54, -1}, /* 235 */
{67, 65, 58, -1}, /* 236 */
{39, 2, 152, 79, -1}, /* 237 */
{15, 122, -1}, /* 238 */
{151, -1}, /* 239 */
{186, 187, -1}, /* 240 */
{20, 141, 188, -1}, /* 241 */
{6, 44, 187, -1}, /* 242 */
{62, 143, -1}, /* 243 */
{53, 85, 185, -1}, /* 244 */
{20, 141, 52, 118, -1}, /* 245 */
{43, 13, -1}, /* 246 */
{93, -1}, /* 247 */
{91, 117, 186, -1}, /* 248 */
{119, 12, -1}, /* 249 */
{91, 117, 3, 84, -1}, /* 250 */
{54, -1}, /* 251 */
{82, 59, -1}, /* 252 */
{4, -1}, /* 253 */
{19, -1} /* 254 */
};

static const int edge_vertices[13][2]={{0,0},
    {1,2}, {3,2}, {4,3}, {4,1},
	{5,6}, {7,6}, {8,7}, {8,5},
	{1,5}, {2,6}, {4,8}, {3,7}};
static const int T_x[9]={0, 0, 1, 1, 0, 0, 1, 1, 0},
				 T_y[9]={0, 1, 1, 0, 0, 1, 1, 0, 0},
				 T_z[9]={0, 0, 0, 0, 0, 1, 1, 1, 1};

static int number_of_triangles[255];

int error;


char shell_name[100],image_name[100];
int max_structs,cur_struct,cur_volume,neighbors;


Cord_with_Norm **TSE;
short sl,row;

FILE *infp,*infp1;
ViewnixHeader vh,vh1;

short *x_table,*y_table,*z_table;
float *x_dist,*y_dist,*z_dist;

char *sl_flag;
unsigned char **sl_ptr;


int scx_size,scy_size,scz_size,vol_skip;

void (*ReplaceNormals)();
void ReplaceBS1_26Normals_8Scene(),ReplaceBS1_26Normals_16Scene();
void ReplaceBS1_8Normals_8Scene(),ReplaceBS1_8Normals_16Scene();
void  ReplaceBS0_26Normals_8Scene(),ReplaceBS0_26Normals_16Scene();
void  ReplaceBS0_8Normals_8Scene(),ReplaceBS0_8Normals_16Scene();
void  ReplaceBS2_26Normals_8Scene(),ReplaceBS2_26Normals_16Scene();
void  ReplaceBS2_8Normals_8Scene(),ReplaceBS2_8Normals_16Scene();

void (*AllocateSlices)(),AllocateSlices8(),AllocateSlices16();
void (*InitTables)(),InitBS0Tables(),InitBS1Tables();

void FreeSpace(),
	UpdateFile(),
	ReadStructure();



/************************************************************************
 *                                                                      *
 *      Function        : main                                          *
 *                                                                      *
 *      Description     : This would process the command line argument  *
 *                        for create normals and call the necessary     *
 *                        functions to compute and update the normals   *
 *                                                                      *
 *      Return Value    : 0  = on normal completion.                    *
 *                        -1 = on an error condition.                   *
 *                                                                      *
 *      Parameters      : 7 parammeters will be passed from the command *
 *                        line.                                         *
 *            argv[0] : name of the this process.                       * 
 *            argv[1] : binary SHELL0 or SHELL1 file                    *
 *            argv[2] : object number ( >=1, <= max_objects in system)  *
 *                      ( -1 means all objects )                        *
 *            argv[3] : IM0 file for normals                            *
 *            argv[4] : current volume (if 3d this is ignored)          *
 *                      ( >= 0 and < max_volumes )                      *
 *            argv[5] : normal neighborhood (8 or 26)                   *
 *            argv[6] : bg_flag (0-foreground, 1-background)            *
 *                                                                      *
 *      Side effects    : This would modify the the input file          *
 *                        and replace its old normals with the new ones.*
 *                                                                      *
 *      Entry condition : SHELL file should have read write access.     *
 *                        image data should be 8 or 16 bits.            *
 *                                                                      *
 *      Related funcs   : ReplaceBS1_26Normals_8Scene(),                *
 *                    ReplaceBS1_26Normals_16Scene(),                   *
 *                    ReplaceBS1_8Normals_8Scene(),                     *
 *                    ReplaceBS1_8Normals_16Scene(),                    *
 *                    ReplaceBS0_26Normals_8Scene(),                    *
 *                    ReplaceBS0_26Normals_16Scene(),                   *
 *                    ReplaceBS0_8Normals_8Scene(),                     *
 *                    ReplaceBS0_8Normals_16Scene(),                    *
 *                    AllocateSlices8(),AllocateSlices16(),             *
 *                    InitBS0Tables(),InitBS1Tables()                   *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekera                 *
 *                                                                      *
 ************************************************************************/

int main(argc,argv)
int argc;
char *argv[];
{
  char group[6],elem[6];
  int bg_flag,i,j;
  
  if (argc!=7) {
    printf("Usage:%s <shell_file> struct_num <IM0_file> volume_num neighbors bg_flag\n",argv[0]);
    fflush(stdout);
    exit(-1);
  }


  strcpy(shell_name,argv[1]);
  strcpy(image_name,argv[3]);
  
  if (sscanf(argv[2],"%d",&cur_struct)!=1) {
    printf("Invalid structure number specified\n");
    fflush(stdout);
    exit(-1);
  }
  if (sscanf(argv[4],"%d",&cur_volume)!=1) {
    printf("Invalid volume number specified\n");
    fflush(stdout);
    exit(-1);
  }

  if (sscanf(argv[5],"%d",&neighbors)!=1 || (neighbors!=8 && neighbors!=26)) {
    printf("Invalid neighborhood for normals specified\n");
    fflush(stdout);
    exit(-1);
  }



  bg_flag=atoi(argv[6]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  if ((infp=fopen(shell_name,"r+b"))==NULL) {
    fprintf(stderr,"Could not open %s\n",shell_name);
    fflush(stdout);
    exit(-1);
  }
  else {
    error=VReadHeader(infp,&vh,group,elem);
    if (error) printf("Read error %d group %s element %s\n",error,group,elem);
    fflush(stdout);
    if (error<=104) exit(-1);
  }

  if ((infp1=fopen(image_name,"rb"))==NULL) {
    fprintf(stderr,"Could not open %s\n",image_name);
    fflush(stdout);
    exit(-1);
  }
  else {
    error=VReadHeader(infp1,&vh1,group,elem);
    if (error) printf("Read error %d group %s element %s\n",error,group,elem);
    if (error<=104) exit(-1);
  }

  if (vh1.gen.data_type!=IMAGE0 ||
      ( vh1.scn.num_of_bits!=8 && vh1.scn.num_of_bits!=16) ) {
    printf("The Scene should be 8 or 16 bit IMAGE0 data");
    fflush(stdout);
    exit(-1);
  }

  vol_skip = 0;
  for (j=0; j<cur_volume; j++)
    vol_skip += vh1.scn.num_of_subscenes[j+1];

  if (vh1.scn.num_of_bits==8) {
    if (vh.gen.data_type==SHELL0) {
      InitTables=InitBS0Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS0_8Normals_8Scene;
      else
	ReplaceNormals=  ReplaceBS0_26Normals_8Scene;

    }
    else if (vh.gen.data_type==SHELL1) {
      InitTables=InitBS1Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS1_8Normals_8Scene;
      else
	ReplaceNormals=  ReplaceBS1_26Normals_8Scene;

    }
    else {
      InitTables=InitBS0Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS2_8Normals_8Scene;
      else
	ReplaceNormals=  ReplaceBS2_26Normals_8Scene;

    }
    AllocateSlices=AllocateSlices8;
  }
  else if (vh1.scn.num_of_bits==16)  {
    if (vh.gen.data_type==SHELL0) {
      InitTables=InitBS0Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS0_8Normals_16Scene;
      else
	ReplaceNormals=  ReplaceBS0_26Normals_16Scene;

    }
    else if (vh.gen.data_type==SHELL1) {
      InitTables=InitBS1Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS1_8Normals_16Scene;
      else
	ReplaceNormals=  ReplaceBS1_26Normals_16Scene;
      
    }
    else {
      InitTables=InitBS0Tables;
      if (neighbors==8)
	ReplaceNormals=  ReplaceBS2_8Normals_16Scene;
      else
	ReplaceNormals=  ReplaceBS2_26Normals_16Scene;

    }
    AllocateSlices=AllocateSlices16;
  }
  else {
    printf("Error: Input scene should be 8 or 16 bits/pixel\n");
    fflush(stdout);
    exit(-1);
  }

  max_structs= vh.str.num_of_structures;

  if (cur_struct!= -1) {
    if (vh.str.num_of_structures<cur_struct  || cur_struct<=0) {
      printf("Invalid structure number specified (should be 1 - %d\n",vh.str.num_of_structures);
      fflush(stdout);
      exit(-1);
    }
    
    for(j=0;j<3;j++)
      for(i=0;i<3;i++) 
	if ( fabs(vh.str.domain[12*(cur_struct-1)+3*(j+1)+i]-
		  vh1.scn.domain[(j+1)*vh1.scn.dimension+i]) > 0.00001 ) {
	  printf("The Orientations of the structure and the scene are not the same\n");
	  fflush(stdout);
	  exit(-1);
	}
    
    
    ReadStructure();
    InitTables();
    ReplaceNormals();
    UpdateFile();
    FreeSpace();
    
  }
  else {
    for(cur_struct=1;cur_struct<=max_structs;cur_struct++) {
      if (vh.str.num_of_structures<cur_struct  || cur_struct<=0) {
	printf("Invalid structure number specified (should be 1 - %d\n",vh.str.num_of_structures);
	fflush(stdout);
	exit(-1);
      }
      
      for(j=0;j<3;j++)
	for(i=0;i<3;i++) 
	  if ( fabs(vh.str.domain[12*(cur_struct-1)+3*(j+1)+i]-
		    vh1.scn.domain[(j+1)*vh1.scn.dimension+i]) > 0.00001 ) {
	    printf("The Orientations of the structure and the scene are not the same\n");
	    fflush(stdout);
	    exit(-1);
	  }
      
      ReadStructure();
      InitTables();
      ReplaceNormals();
      UpdateFile();
      FreeSpace();
    }
  }

  if (bg_flag) 
    VDeleteBackgroundProcessInformation();
  return(0);
 


}



/************************************************************************
 *                                                                      *
 *      Function        : ReadStructure                                 *
 *                                                                      *
 *      Description     : This would read the input stucture file and   *
 *                        initialize the TSE array with the vaules of   *
 *                        cur_struct.                                   *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : number_of_triangles is initialized.
 *                        On a read error this would exit the process.  *
 *                                                                      *
 *      Entry condition : infp should be opend for reading.             *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekera                 *
 *                        Modified: 7/9/03 for SHELL2 by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReadStructure()
{

  int i,j,k,skip_size,num;
  short rtemp,cols;
  unsigned short *tcols;


  for (j=1; j<255; j++)
  {
    for (k=0; triangle_table[j][k]>=0; k++)
      ;
    number_of_triangles[j] = k;
  }

  skip_size=0;
  for(i=0;i<cur_struct-1;i++) {
    skip_size += (vh.str.num_of_NTSE[i]*vh.str.num_of_bits_in_NTSE)/8 +
      (vh.str.num_of_TSE[i]*vh.str.num_of_bits_in_TSE)/8 ;
  }
  error=VSeekData(infp,skip_size);
  if (error) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }

  if (VReadData((char *)&sl,2,1,infp,&num)) {
   printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }
  if (VReadData((char *)&row,2,1,infp,&num)) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }
  for(i=0;i<sl-1;i++)
    if (VReadData((char *)&rtemp,2,1,infp,&num) || rtemp!=row) {
      printf("Error: All slices should have equal number of rows\n");
      fflush(stdout);
      exit(-1);
    } 
  
  TSE=(Cord_with_Norm **)malloc( sizeof(Cord_with_Norm *) * (sl * row + 1) );
  if (TSE==NULL) {
    printf("Error: In memory allocation\n");
    fflush(stdout);
    exit(-1);
  }

  TSE[0]=(Cord_with_Norm *)malloc((vh.gen.data_type==SHELL2? 1:4)*vh.str.num_of_TSE[cur_struct-1]);
  if (TSE[0]==NULL) {
    printf("Error: In memory allocation\n");
    fflush(stdout);
    exit(-1);
  }

  if (vh.gen.data_type == SHELL2) {
    tcols = (unsigned short *)malloc(sl*row*2);
    if (tcols == NULL) {
      fprintf(stderr, "Error: In memory allocation\n");
      exit(-1);
    }
    if (VReadData((char *)tcols,2,sl*row,infp,&num) ||
        VReadData((char *)TSE[0],1,vh.str.num_of_TSE[cur_struct-1],infp,&num)) {
      fprintf(stderr, "Error in Reading Data\n");
      exit(-1);
    }
    for(i=0;i<sl*row;i++) {
      ((char **)TSE)[i+1] = ((char **)TSE)[i];
      for (j=0; j<tcols[i]; j++) {
        ((unsigned char **)TSE)[i+1] +=
          3+3*number_of_triangles[((unsigned char **)TSE)[i+1][0]];
      }
    }
    free(tcols);
  }
  else {
    for(i=0;i<sl*row;i++) {
      if (VReadData((char *)&cols,2,1,infp,&num)) {
        printf("Error in Reading Data\n");
        fflush(stdout);
        exit(-1);
      }
      TSE[i+1]=TSE[i]+cols;
    }
    if (VReadData((char *)TSE[0],2,vh.str.num_of_TSE[cur_struct-1]*2,infp,&num)) {
      printf("Error in Reading Data\n");
      fflush(stdout);
      exit(-1);
    }
  }
}






/************************************************************************
 *                                                                      *
 *      Function        : InitBS1Tables                                 *
 *                                                                      *
 *      Description     : This would build tables for interpolation and *
 *                        for mapping the stucutre coordinates to the   *
 *                        scene coordinates for BS1 files.              *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                        The following global variables are initialized*
 *                        x_table,y_table,z_table,x_dist,y_dist,z_dist, *
 *                        scx_size,scy_size,scz_size,sl_flag,sl_ptr     *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : Structure and scene data should be unfiromally*
 *                        spaced and the domains should be parallel.    *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                                                                      *
 ************************************************************************/
void InitBS1Tables()
{
  int i,skip,max_col;
  double scx_start,scy_start,scz_start;
  double x_incr,y_incr,z_incr,scx_incr,scy_incr,scz_incr,sc_dist;
  double cur,x_start,y_start,z_start;
  double M[3][3],t[3],orig_trans[3];

  /* Build 3 independent tables that would translate
     struct coords to scene cords */
  max_col=(int)rint(vh.str.min_max_coordinates[(cur_struct-1)*6+3]/vh.str.xysize[0]);

  
  

  x_table=(short *)malloc(sizeof(short)*(max_col+4))+1;
  y_table=(short *)malloc(sizeof(short)*(row+3))+1;
  z_table=(short *)malloc(sizeof(short)*(sl+3))+1;
  
  x_dist=(float *)malloc(sizeof(float)*(max_col+4))+1;
  y_dist=(float *)malloc(sizeof(float)*(row+3))+1;
  z_dist=(float *)malloc(sizeof(float)*(sl+3))+1;
  
  M[0][0]=vh.str.domain[(cur_struct-1)*12+3];
  M[0][1]=vh.str.domain[(cur_struct-1)*12+4];
  M[0][2]=vh.str.domain[(cur_struct-1)*12+5];
  M[1][0]=vh.str.domain[(cur_struct-1)*12+6];
  M[1][1]=vh.str.domain[(cur_struct-1)*12+7];
  M[1][2]=vh.str.domain[(cur_struct-1)*12+8];
  M[2][0]=vh.str.domain[(cur_struct-1)*12+9];
  M[2][1]=vh.str.domain[(cur_struct-1)*12+10];
  M[2][2]=vh.str.domain[(cur_struct-1)*12+11];

  t[0]= vh.str.domain[(cur_struct-1)*12  ]- vh1.scn.domain[0];
  t[1]= vh.str.domain[(cur_struct-1)*12+1]- vh1.scn.domain[1];
  t[2]= vh.str.domain[(cur_struct-1)*12+2]- vh1.scn.domain[2];
  orig_trans[0]= M[0][0]*t[0]+M[0][1]*t[1]+M[0][2]*t[2];
  orig_trans[1]= M[1][0]*t[0]+M[1][1]*t[1]+M[1][2]*t[2];
  orig_trans[2]= M[2][0]*t[0]+M[2][1]*t[1]+M[2][2]*t[2];

  x_incr=vh.str.xysize[0];
  x_start= orig_trans[0]- x_incr;

  y_incr=vh.str.xysize[1];
  y_start= orig_trans[1] + vh.str.min_max_coordinates[(cur_struct-1)*6 + 1] - 
    y_incr;
  
  z_incr=vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0];
  z_start=  orig_trans[2] + vh.str.min_max_coordinates[(cur_struct-1)*6 + 2] - 
    z_incr;
  
  scx_start= 0;
  scx_incr= vh1.scn.xypixsz[0];
  scx_size= vh1.scn.xysize[0];
  
  scy_start= 0;
  scy_incr= vh1.scn.xypixsz[1];
  scy_size= vh1.scn.xysize[1];

  if (vh1.scn.dimension==3) {
    scz_start= vh1.scn.loc_of_subscenes[0];
    scz_size = vh1.scn.num_of_subscenes[0];
	scz_incr = ((double)vh1.scn.loc_of_subscenes[scz_size-1] -
      vh1.scn.loc_of_subscenes[0])/(scz_size-1);
  }
  else {
    skip=0;
    for (i=0; i<=cur_volume; i++)
      skip += vh1.scn.num_of_subscenes[i];
    scz_size=vh1.scn.num_of_subscenes[cur_volume+1];
    scz_start= vh1.scn.loc_of_subscenes[skip];
    scz_incr = vh1.scn.loc_of_subscenes[skip+1]- vh1.scn.loc_of_subscenes[skip];
  }

  /* Initialize x table */
  for(cur=x_start-scx_start, i=0;i<max_col+3; cur += x_incr, i++) {
    if (cur > -scx_incr) {
      sc_dist= cur/scx_incr;
      x_table[i]= (short)floor(sc_dist);
      if (x_table[i] >= scx_size) {
	x_table[i]=scx_size-1; x_dist[i]=1.0;
      }
      else
	x_dist[i]= (float)(sc_dist - x_table[i]);
    }
    else {
      x_table[i]= -1;
      x_dist[i]=0.0;
    }
  }



  /* Initialize y table */
  for(cur=y_start-scy_start, i=0;i<row+2; cur += y_incr, i++) {
    if (cur > -scy_incr) {
      sc_dist= cur/scy_incr;
      y_table[i]= (short)floor(sc_dist);
      if (y_table[i] >= scy_size) {
	y_table[i]=scy_size-1; y_dist[i]=1.0;
      }
      else
	y_dist[i]= (float)(sc_dist - y_table[i]);
    }
    else {
      y_table[i]= -1;
      y_dist[i]=0.0;
    }
  }


  /* Initialize z table */
  for(cur=z_start-scz_start, i=0;i<sl+2; cur += z_incr, i++) {
    if (cur > -scz_incr) {
      sc_dist= cur/scz_incr;
      z_table[i]= (short)floor(sc_dist);
      if (z_table[i] >= scz_size) {
	z_table[i]=scz_size-1; z_dist[i]=1.0;
      }
      else
	z_dist[i]= (float)(sc_dist - z_table[i]);
    }
    else {
      z_table[i]= -1;
      z_dist[i]=0.0;
    }
  }
  
  
  sl_flag=(char *)calloc(scz_size+2,1);
  sl_ptr=(unsigned char **)malloc(sizeof(char *)*(scz_size+2));
  


}


/************************************************************************
 *                                                                      *
 *      Function        : InitBS0Tables                                 *
 *                                                                      *
 *      Description     : This would build tables for interpolation and *
 *                        for mapping the stucutre coordinates to the   *
 *                        scene coordinates for BS0 files.              *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                        The following global variables are initialized*
 *                        x_table,y_table,z_table,x_dist,y_dist,z_dist, *
 *                        scx_size,scy_size,scz_size,sl_flag,sl_ptr     *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : Structure and scene data should be unfiromally*
 *                        spaced and the domains should be parallel.    *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 5/11/03 tables expanded by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void InitBS0Tables()
{
  int i,skip,max_col;
  double scx_start,scy_start,scz_start;
  double x_incr,y_incr,z_incr,scx_incr,scy_incr,scz_incr,sc_dist;
  double cur,x_start,y_start,z_start;
  double M[3][3],t[3],orig_trans[3];



  /* Build 3 independent tables that would translate
     struct coords to scene cords */
  max_col=(int)rint(vh.str.min_max_coordinates[(cur_struct-1)*6+3]/vh.str.xysize[0]);


  x_table=(short *)malloc(sizeof(short)*(2*max_col+7))+1;
  y_table=(short *)malloc(sizeof(short)*(2*row+5))+1;
  z_table=(short *)malloc(sizeof(short)*(2*sl+5))+1;
  
  x_dist=(float *)malloc(sizeof(float)*(2*max_col+7))+1;
  y_dist=(float *)malloc(sizeof(float)*(2*row+5))+1;
  z_dist=(float *)malloc(sizeof(float)*(2*sl+5))+1;
  
  M[0][0]=vh.str.domain[(cur_struct-1)*12+3];
  M[0][1]=vh.str.domain[(cur_struct-1)*12+4];
  M[0][2]=vh.str.domain[(cur_struct-1)*12+5];
  M[1][0]=vh.str.domain[(cur_struct-1)*12+6];
  M[1][1]=vh.str.domain[(cur_struct-1)*12+7];
  M[1][2]=vh.str.domain[(cur_struct-1)*12+8];
  M[2][0]=vh.str.domain[(cur_struct-1)*12+9];
  M[2][1]=vh.str.domain[(cur_struct-1)*12+10];
  M[2][2]=vh.str.domain[(cur_struct-1)*12+11];

  t[0]= vh.str.domain[(cur_struct-1)*12  ]- vh1.scn.domain[0];
  t[1]= vh.str.domain[(cur_struct-1)*12+1]- vh1.scn.domain[1];
  t[2]= vh.str.domain[(cur_struct-1)*12+2]- vh1.scn.domain[2];
  orig_trans[0]= M[0][0]*t[0]+M[0][1]*t[1]+M[0][2]*t[2];
  orig_trans[1]= M[1][0]*t[0]+M[1][1]*t[1]+M[1][2]*t[2];
  orig_trans[2]= M[2][0]*t[0]+M[2][1]*t[1]+M[2][2]*t[2];


  x_incr=vh.str.xysize[0]/2.0;
  x_start= orig_trans[0]- x_incr;
  
  y_incr=vh.str.xysize[1]/2.0;
  y_start= orig_trans[1] + vh.str.min_max_coordinates[(cur_struct-1)*6 + 1] - 
    y_incr;
  
  z_incr=(vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/2.0;
  z_start=  orig_trans[2] + vh.str.min_max_coordinates[(cur_struct-1)*6 + 2] - 
    z_incr;
  
  scx_incr= vh1.scn.xypixsz[0];
  scx_start= scx_incr/2.0;
  scx_size= vh1.scn.xysize[0];
  
  scy_incr= vh1.scn.xypixsz[1];
  scy_start= scy_incr/2.0;
  scy_size= vh1.scn.xysize[1];

  if (vh1.scn.dimension==3) {
    scz_size = vh1.scn.num_of_subscenes[0];
	scz_incr = ((double)vh1.scn.loc_of_subscenes[scz_size-1] -
      vh1.scn.loc_of_subscenes[0])/(scz_size-1);
    scz_start= vh1.scn.loc_of_subscenes[0]+scz_incr/2.0;
  }
  else {
    skip=0;
    for(i=0;i<cur_struct;i++)
      skip += vh1.scn.num_of_subscenes[i];
    scz_size=vh1.scn.num_of_subscenes[cur_struct];
    scz_incr= vh1.scn.loc_of_subscenes[skip+1]- vh1.scn.loc_of_subscenes[skip];
    scz_start= vh1.scn.loc_of_subscenes[skip]+scz_incr/2.0;
  }

  /* Initialize x table */
  for (cur=x_start-scx_start, i=-1; i<2*max_col+6;
      cur += x_incr, i++) {
    if (cur > -scx_incr) {
      sc_dist= cur/scx_incr;
      x_table[i]= (short)floor(sc_dist);
      if (x_table[i] >= scx_size) {
        x_table[i]=scx_size-1; x_dist[i]=1.0;
      }
      else
        x_dist[i]= (float)(sc_dist - x_table[i]);
    }
    else {
      x_table[i]= -1;
      x_dist[i]=0.0;
    }
  }



  /* Initialize y table */
  for (cur=y_start-scy_start, i=-1; i<2*row+4;
      cur += y_incr, i++) {
    if (cur > -scy_incr) {
      sc_dist= cur/scy_incr;
      y_table[i]= (short)floor(sc_dist);
      if (y_table[i] >= scy_size) {
        y_table[i]=scy_size-1; y_dist[i]=1.0;
      }
      else
        y_dist[i]= (float)(sc_dist - y_table[i]);
    }
    else {
      y_table[i]= -1;
      y_dist[i]=0.0;
    }
  }


  /* Initialize z table */
  for (cur=z_start-scz_start, i=-1; i<2*sl+4;
      cur += z_incr, i++) {
    if (cur > -scz_incr) {
      sc_dist= cur/scz_incr;
      z_table[i]= (short)floor(sc_dist);
      if (z_table[i] >= scz_size) {
        z_table[i]=scz_size-1; z_dist[i]=1.0;
      }
      else
        z_dist[i]= (float)(sc_dist - z_table[i]);
    }
    else {
      z_table[i]= -1;
      z_dist[i]=0.0;
    }
  }
  
  
  sl_flag=(char *)calloc(scz_size+2,1);
  sl_ptr=(unsigned char **)malloc(sizeof(char *)*(scz_size+2));
  


}



/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS1_26Normals_8Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS1     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 5/9/03 l12 contribution to x-
 *                           component corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS1_26Normals_8Scene()
{

  unsigned char *msl1,*msl2,*csl1,*csl2,*psl1,*psl2;
  static unsigned char *msl1_mr1,*msl2_mr1,*csl1_mr1,*csl2_mr1,*psl1_mr1,*psl2_mr1; 
  static unsigned char *msl1_mr2,*msl2_mr2,*csl1_mr2,*csl2_mr2,*psl1_mr2,*psl2_mr2; 
  static unsigned char *msl1_cr1,*msl2_cr1,*csl1_cr1,*csl2_cr1,*psl1_cr1,*psl2_cr1; 
  static unsigned char *msl1_cr2,*msl2_cr2,*csl1_cr2,*csl2_cr2,*psl1_cr2,*psl2_cr2; 
  static unsigned char *msl1_pr1,*msl2_pr1,*csl1_pr1,*csl2_pr1,*psl1_pr1,*psl2_pr1; 
  static unsigned char *msl1_pr2,*msl2_pr2,*csl1_pr2,*csl2_pr2,*psl1_pr2,*psl2_pr2; 
  static double mzdist,mydist,mxdist;
  static double czdist,cydist,cxdist;
  static double pzdist,pydist,pxdist;
  static double p000,p00c,p001,p0c0,p0cc,p0c1,p010,p01c,p011;
  static double pc00,pc0c,pc01,pcc0,     pcc1,pc10,pc1c,pc11;
  static double p100,p10c,p101,p1c0,p1cc,p1c1,p110,p11c,p111;
  static double l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,cur_slice,minus_slice,plus_row,cur_row,minus_row;
  int mc,cc,pc,sl_w,mult;

  
  sl_w=vh1.scn.xysize[0]+2;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[i];
    mzdist= z_dist[i];
    plus_slice= z_table[i+2];
    pzdist= z_dist[i+2];
    cur_slice= z_table[i+1];
    czdist= z_dist[i+1];


    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,cur_slice+1,cur_slice+2,-1,-1,-1,-1);

    msl1=sl_ptr[minus_slice+1];
    msl2=sl_ptr[minus_slice+2];
    csl1=sl_ptr[cur_slice+1];
    csl2=sl_ptr[cur_slice+2];
    psl1=sl_ptr[plus_slice+1];
    psl2=sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[j];
      mydist= y_dist[j];
      mult=sl_w*(minus_row+1);
      msl1_mr1= msl1 + mult;
      msl1_mr2= msl1_mr1 + sl_w;
      msl2_mr1= msl2 + mult;
      msl2_mr2= msl2_mr1 + sl_w;
      csl1_mr1= csl1 + mult;
      csl1_mr2= csl1_mr1 + sl_w;
      csl2_mr1= csl2 + mult;
      csl2_mr2= csl2_mr1 + sl_w;
      psl1_mr1= psl1 + mult;
      psl1_mr2= psl1_mr1 + sl_w;
      psl2_mr1= psl2 + mult;
      psl2_mr2= psl2_mr1 + sl_w;
      
      cur_row= y_table[j+1];
      cydist= y_dist[j+1];
      mult=sl_w*(cur_row+1);
      msl1_cr1= msl1 + mult;
      msl1_cr2= msl1_cr1 + sl_w;
      msl2_cr1= msl2 + mult;
      msl2_cr2= msl2_cr1 + sl_w;
      csl1_cr1= csl1 + mult;
      csl1_cr2= csl1_cr1 + sl_w;
      csl2_cr1= csl2 + mult;
      csl2_cr2= csl2_cr1 + sl_w;
      psl1_cr1= psl1 + mult;
      psl1_cr2= psl1_cr1 + sl_w;
      psl2_cr1= psl2 + mult;
      psl2_cr2= psl2_cr1 + sl_w;

      plus_row= y_table[j+2];
      pydist= y_dist[j+2];
      mult=sl_w*(plus_row+1);
      msl1_pr1= msl1 + mult;
      msl1_pr2= msl1_pr1 + sl_w;
      msl2_pr1= msl2 + mult;
      msl2_pr2= msl2_pr1 + sl_w;
      csl1_pr1= csl1 + mult;
      csl1_pr2= csl1_pr1 + sl_w;
      csl2_pr1= csl2 + mult;
      csl2_pr2= csl2_pr1 + sl_w;
      psl1_pr1= psl1 + mult;
      psl1_pr2= psl1_pr1 + sl_w;
      psl2_pr1= psl2 + mult;
      psl2_pr2= psl2_pr1 + sl_w;



#define MFUNC(P,W1,W2,W3,V1,V2,V3,V4,V5,V6,V7,V8) { \
						    float EXP1,EXP2,EXP3,EXP4,EXP5,EXP6; \
						    EXP1= (float)((1-W1)*V1 + W1*V2); \
						    EXP2= (float)((1-W1)*V3 + W1*V4); \
						    EXP3= (float)((1-W1)*V5 + W1*V6); \
						    EXP4= (float)((1-W1)*V7 + W1*V8); \
						    EXP5= (float)((1-W2)*EXP1+ W2*EXP2); \
						    EXP6= (float)((1-W2)*EXP3+ W2*EXP4); \
						    P= (1-(float)(W3))*EXP5+(float)(W3)*EXP6; \
						  }

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
	k= tse1->c[0] & C_MASK;
	mc= x_table[k]+1;
	mxdist= x_dist[k];
	
	cc= x_table[k+1]+1;
	cxdist= x_dist[k+1];
	
	pc= x_table[k+2]+1;
	pxdist= x_dist[k+2];
/*
	p000= 
	  (((1-mxdist)* msl1_mr1[mc] + mxdist* msl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* msl1_mr2[mc] + mxdist* msl1_mr2[mc+1])*  mydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_mr1[mc] + mxdist* msl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* msl2_mr2[mc] + mxdist* msl2_mr2[mc+1])*  mydist ) * mzdist ;
*/
	MFUNC(p000,mxdist,mydist,mzdist,
	      msl1_mr1[mc],msl1_mr1[mc+1],msl1_mr2[mc],msl1_mr2[mc+1],
	      msl2_mr1[mc],msl2_mr1[mc+1],msl2_mr2[mc],msl2_mr2[mc+1]);
	
	p00c= 
	  (((1-cxdist)* msl1_mr1[cc] + cxdist* msl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* msl1_mr2[cc] + cxdist* msl1_mr2[cc+1])*  mydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_mr1[cc] + cxdist* msl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* msl2_mr2[cc] + cxdist* msl2_mr2[cc+1])*  mydist ) * mzdist ;
	p001= 
	  (((1-pxdist)* msl1_mr1[pc] + pxdist* msl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* msl1_mr2[pc] + pxdist* msl1_mr2[pc+1])*  mydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_mr1[pc] + pxdist* msl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* msl2_mr2[pc] + pxdist* msl2_mr2[pc+1])*  mydist ) * mzdist ;

	p0c0= 
	  (((1-mxdist)* msl1_cr1[mc] + mxdist* msl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* msl1_cr2[mc] + mxdist* msl1_cr2[mc+1])*  cydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_cr1[mc] + mxdist* msl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* msl2_cr2[mc] + mxdist* msl2_cr2[mc+1])*  cydist ) * mzdist ;
	p0cc= 
	  (((1-cxdist)* msl1_cr1[cc] + cxdist* msl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* msl1_cr2[cc] + cxdist* msl1_cr2[cc+1])*  cydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_cr1[cc] + cxdist* msl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* msl2_cr2[cc] + cxdist* msl2_cr2[cc+1])*  cydist ) * mzdist ;
	p0c1= 
	  (((1-pxdist)* msl1_cr1[pc] + pxdist* msl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* msl1_cr2[pc] + pxdist* msl1_cr2[pc+1])*  cydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_cr1[pc] + pxdist* msl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* msl2_cr2[pc] + pxdist* msl2_cr2[pc+1])*  cydist ) * mzdist ;
	
	p010= 
	  (((1-mxdist)* msl1_pr1[mc] + mxdist* msl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* msl1_pr2[mc] + mxdist* msl1_pr2[mc+1])*  pydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_pr1[mc] + mxdist* msl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* msl2_pr2[mc] + mxdist* msl2_pr2[mc+1])*  pydist ) * mzdist ;
	p01c= 
	  (((1-cxdist)* msl1_pr1[cc] + cxdist* msl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* msl1_pr2[cc] + cxdist* msl1_pr2[cc+1])*  pydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_pr1[cc] + cxdist* msl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* msl2_pr2[cc] + cxdist* msl2_pr2[cc+1])*  pydist ) * mzdist ;
	p011= 
	  (((1-pxdist)* msl1_pr1[pc] + pxdist* msl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* msl1_pr2[pc] + pxdist* msl1_pr2[pc+1])*  pydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_pr1[pc] + pxdist* msl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* msl2_pr2[pc] + pxdist* msl2_pr2[pc+1])*  pydist ) * mzdist ;
	



	pc00= 
	  (((1-mxdist)* csl1_mr1[mc] + mxdist* csl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* csl1_mr2[mc] + mxdist* csl1_mr2[mc+1])*  mydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_mr1[mc] + mxdist* csl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* csl2_mr2[mc] + mxdist* csl2_mr2[mc+1])*  mydist ) * czdist ;
	pc0c= 
	  (((1-cxdist)* csl1_mr1[cc] + cxdist* csl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* csl1_mr2[cc] + cxdist* csl1_mr2[cc+1])*  mydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_mr1[cc] + cxdist* csl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* csl2_mr2[cc] + cxdist* csl2_mr2[cc+1])*  mydist ) * czdist ;
	pc01= 
	  (((1-pxdist)* csl1_mr1[pc] + pxdist* csl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* csl1_mr2[pc] + pxdist* csl1_mr2[pc+1])*  mydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_mr1[pc] + pxdist* csl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* csl2_mr2[pc] + pxdist* csl2_mr2[pc+1])*  mydist ) * czdist ;

	pcc0= 
	  (((1-mxdist)* csl1_cr1[mc] + mxdist* csl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* csl1_cr2[mc] + mxdist* csl1_cr2[mc+1])*  cydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_cr1[mc] + mxdist* csl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* csl2_cr2[mc] + mxdist* csl2_cr2[mc+1])*  cydist ) * czdist ;
/*	pccc= 
	  (((1-cxdist)* csl1_cr1[cc] + cxdist* csl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* csl1_cr2[cc] + cxdist* csl1_cr2[cc+1])*  cydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_cr1[cc] + cxdist* csl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* csl2_cr2[cc] + cxdist* csl2_cr2[cc+1])*  cydist ) * czdist ;
*/
	pcc1= 
	  (((1-pxdist)* csl1_cr1[pc] + pxdist* csl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* csl1_cr2[pc] + pxdist* csl1_cr2[pc+1])*  cydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_cr1[pc] + pxdist* csl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* csl2_cr2[pc] + pxdist* csl2_cr2[pc+1])*  cydist ) * czdist ;
	
	pc10= 
	  (((1-mxdist)* csl1_pr1[mc] + mxdist* csl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* csl1_pr2[mc] + mxdist* csl1_pr2[mc+1])*  pydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_pr1[mc] + mxdist* csl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* csl2_pr2[mc] + mxdist* csl2_pr2[mc+1])*  pydist ) * czdist ;
	pc1c= 
	  (((1-cxdist)* csl1_pr1[cc] + cxdist* csl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* csl1_pr2[cc] + cxdist* csl1_pr2[cc+1])*  pydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_pr1[cc] + cxdist* csl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* csl2_pr2[cc] + cxdist* csl2_pr2[cc+1])*  pydist ) * czdist ;
	pc11= 
	  (((1-pxdist)* csl1_pr1[pc] + pxdist* csl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* csl1_pr2[pc] + pxdist* csl1_pr2[pc+1])*  pydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_pr1[pc] + pxdist* csl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* csl2_pr2[pc] + pxdist* csl2_pr2[pc+1])*  pydist ) * czdist ;
	



	p100= 
	  (((1-mxdist)* psl1_mr1[mc] + mxdist* psl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* psl1_mr2[mc] + mxdist* psl1_mr2[mc+1])*  mydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_mr1[mc] + mxdist* psl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* psl2_mr2[mc] + mxdist* psl2_mr2[mc+1])*  mydist ) * pzdist ;
	p10c= 
	  (((1-cxdist)* psl1_mr1[cc] + cxdist* psl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* psl1_mr2[cc] + cxdist* psl1_mr2[cc+1])*  mydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_mr1[cc] + cxdist* psl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* psl2_mr2[cc] + cxdist* psl2_mr2[cc+1])*  mydist ) * pzdist ;
	p101= 
	  (((1-pxdist)* psl1_mr1[pc] + pxdist* psl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* psl1_mr2[pc] + pxdist* psl1_mr2[pc+1])*  mydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_mr1[pc] + pxdist* psl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* psl2_mr2[pc] + pxdist* psl2_mr2[pc+1])*  mydist ) * pzdist ;
	
	p1c0= 
	  (((1-mxdist)* psl1_cr1[mc] + mxdist* psl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* psl1_cr2[mc] + mxdist* psl1_cr2[mc+1])*  cydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_cr1[mc] + mxdist* psl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* psl2_cr2[mc] + mxdist* psl2_cr2[mc+1])*  cydist ) * pzdist ;
	p1cc= 
	  (((1-cxdist)* psl1_cr1[cc] + cxdist* psl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* psl1_cr2[cc] + cxdist* psl1_cr2[cc+1])*  cydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_cr1[cc] + cxdist* psl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* psl2_cr2[cc] + cxdist* psl2_cr2[cc+1])*  cydist ) * pzdist ;
	p1c1= 
	  (((1-pxdist)* psl1_cr1[pc] + pxdist* psl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* psl1_cr2[pc] + pxdist* psl1_cr2[pc+1])*  cydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_cr1[pc] + pxdist* psl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* psl2_cr2[pc] + pxdist* psl2_cr2[pc+1])*  cydist ) * pzdist ;
	
	p110= 
	  (((1-mxdist)* psl1_pr1[mc] + mxdist* psl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* psl1_pr2[mc] + mxdist* psl1_pr2[mc+1])*  pydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_pr1[mc] + mxdist* psl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* psl2_pr2[mc] + mxdist* psl2_pr2[mc+1])*  pydist ) * pzdist ;
	p11c= 
	  (((1-cxdist)* psl1_pr1[cc] + cxdist* psl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* psl1_pr2[cc] + cxdist* psl1_pr2[cc+1])*  pydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_pr1[cc] + cxdist* psl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* psl2_pr2[cc] + cxdist* psl2_pr2[cc+1])*  pydist ) * pzdist ;
	p111= 
	  (((1-pxdist)* psl1_pr1[pc] + pxdist* psl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* psl1_pr2[pc] + pxdist* psl1_pr2[pc+1])*  pydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_pr1[pc] + pxdist* psl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* psl2_pr2[pc] + pxdist* psl2_pr2[pc+1])*  pydist ) * pzdist ;
	
	     
	
	l1=p1cc-p0cc; l2=p11c-p00c; l3=p10c-p01c; l4=p1c1-p0c0;
	l5=p1c0-p0c1; l6=p111-p000; l7=p110-p001; l8=p101-p010;
	l9=p100-p011;l10=pc1c-pc0c;l11=pc11-pc00;l12=pc10-pc01;
	l13=pcc1-pcc0;
	tse1->c[1] = G_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9));

/*
	printf("%d %d %d nrm=%d  %f %f\n%f %f %f %f %f %f %f %f\n",i,j,k,tse1->c[1],pcc1,pcc0,
	       p1cc,p0cc,p11c,p00c,p10c,p01c,p1c1,p0c0);
	printf("%f %f %f %f %f %f %f %f\n",p1c0,p0c1,p111,p000,p110,p001,p101,p010);
	printf("%f %f %f %f %f %f %f %f\n",p100,p011,pc1c,pc0c,pc11,pc00,pc10,pc01);
*/
      }
    }
  }
  

}







/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS1_26Normals_16Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS1     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 16 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 16 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 5/9/03 l12 contribution to x-
 *                           component corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS1_26Normals_16Scene()
{

  static unsigned short *msl1,*msl2,*csl1,*csl2,*psl1,*psl2;
  static unsigned short *msl1_mr1,*msl2_mr1,*csl1_mr1,*csl2_mr1,*psl1_mr1,*psl2_mr1; 
  static unsigned short *msl1_mr2,*msl2_mr2,*csl1_mr2,*csl2_mr2,*psl1_mr2,*psl2_mr2; 
  static unsigned short *msl1_cr1,*msl2_cr1,*csl1_cr1,*csl2_cr1,*psl1_cr1,*psl2_cr1; 
  static unsigned short *msl1_cr2,*msl2_cr2,*csl1_cr2,*csl2_cr2,*psl1_cr2,*psl2_cr2; 
  static unsigned short *msl1_pr1,*msl2_pr1,*csl1_pr1,*csl2_pr1,*psl1_pr1,*psl2_pr1; 
  static unsigned short *msl1_pr2,*msl2_pr2,*csl1_pr2,*csl2_pr2,*psl1_pr2,*psl2_pr2; 
  static double mzdist,mydist,mxdist;
  static double czdist,cydist,cxdist;
  static double pzdist,pydist,pxdist;
  static double p000,p00c,p001,p0c0,p0cc,p0c1,p010,p01c,p011;
  static double pc00,pc0c,pc01,pcc0,     pcc1,pc10,pc1c,pc11;
  static double p100,p10c,p101,p1c0,p1cc,p1c1,p110,p11c,p111;
  double l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,cur_slice,minus_slice,plus_row,cur_row,minus_row;
  int mc,cc,pc,sl_w,mult;

  
  sl_w=vh1.scn.xysize[0]+2;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[i];
    mzdist= z_dist[i];
    plus_slice= z_table[i+2];
    pzdist= z_dist[i+2];
    cur_slice= z_table[i+1];
    czdist= z_dist[i+1];


    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,cur_slice+1,cur_slice+2,-1,-1,-1,-1);

    msl1=(unsigned short *)sl_ptr[minus_slice+1];
    msl2=(unsigned short *)sl_ptr[minus_slice+2];
    csl1=(unsigned short *)sl_ptr[cur_slice+1];
    csl2=(unsigned short *)sl_ptr[cur_slice+2];
    psl1=(unsigned short *)sl_ptr[plus_slice+1];
    psl2=(unsigned short *)sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[j];
      mydist= y_dist[j];
      mult=sl_w*(minus_row+1);
      msl1_mr1= msl1 + mult;
      msl1_mr2= msl1_mr1 + sl_w;
      msl2_mr1= msl2 + mult;
      msl2_mr2= msl2_mr1 + sl_w;
      csl1_mr1= csl1 + mult;
      csl1_mr2= csl1_mr1 + sl_w;
      csl2_mr1= csl2 + mult;
      csl2_mr2= csl2_mr1 + sl_w;
      psl1_mr1= psl1 + mult;
      psl1_mr2= psl1_mr1 + sl_w;
      psl2_mr1= psl2 + mult;
      psl2_mr2= psl2_mr1 + sl_w;
      
      cur_row= y_table[j+1];
      cydist= y_dist[j+1];
      mult=sl_w*(cur_row+1);
      msl1_cr1= msl1 + mult;
      msl1_cr2= msl1_cr1 + sl_w;
      msl2_cr1= msl2 + mult;
      msl2_cr2= msl2_cr1 + sl_w;
      csl1_cr1= csl1 + mult;
      csl1_cr2= csl1_cr1 + sl_w;
      csl2_cr1= csl2 + mult;
      csl2_cr2= csl2_cr1 + sl_w;
      psl1_cr1= psl1 + mult;
      psl1_cr2= psl1_cr1 + sl_w;
      psl2_cr1= psl2 + mult;
      psl2_cr2= psl2_cr1 + sl_w;

      plus_row= y_table[j+2];
      pydist= y_dist[j+2];
      mult=sl_w*(plus_row+1);
      msl1_pr1= msl1 + mult;
      msl1_pr2= msl1_pr1 + sl_w;
      msl2_pr1= msl2 + mult;
      msl2_pr2= msl2_pr1 + sl_w;
      csl1_pr1= csl1 + mult;
      csl1_pr2= csl1_pr1 + sl_w;
      csl2_pr1= csl2 + mult;
      csl2_pr2= csl2_pr1 + sl_w;
      psl1_pr1= psl1 + mult;
      psl1_pr2= psl1_pr1 + sl_w;
      psl2_pr1= psl2 + mult;
      psl2_pr2= psl2_pr1 + sl_w;


      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
	k= tse1->c[0] & C_MASK;
	mc= x_table[k]+1;
	mxdist= x_dist[k];
	
	cc= x_table[k+1]+1;
	cxdist= x_dist[k+1];
	
	pc= x_table[k+2]+1;
	pxdist= x_dist[k+2];

/*
	p000= 
	  (((1-mxdist)* msl1_mr1[mc] + mxdist* msl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* msl1_mr2[mc] + mxdist* msl1_mr2[mc+1])*  mydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_mr1[mc] + mxdist* msl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* msl2_mr2[mc] + mxdist* msl2_mr2[mc+1])*  mydist ) * mzdist ;
*/
	MFUNC(p000,mxdist,mydist,mzdist,
	      msl1_mr1[mc],msl1_mr1[mc+1],msl1_mr2[mc],msl1_mr2[mc+1],
	      msl2_mr1[mc],msl2_mr1[mc+1],msl2_mr2[mc],msl2_mr2[mc+1]);
	

	p00c= 
	  (((1-cxdist)* msl1_mr1[cc] + cxdist* msl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* msl1_mr2[cc] + cxdist* msl1_mr2[cc+1])*  mydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_mr1[cc] + cxdist* msl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* msl2_mr2[cc] + cxdist* msl2_mr2[cc+1])*  mydist ) * mzdist ;
	p001= 
	  (((1-pxdist)* msl1_mr1[pc] + pxdist* msl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* msl1_mr2[pc] + pxdist* msl1_mr2[pc+1])*  mydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_mr1[pc] + pxdist* msl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* msl2_mr2[pc] + pxdist* msl2_mr2[pc+1])*  mydist ) * mzdist ;

	p0c0= 
	  (((1-mxdist)* msl1_cr1[mc] + mxdist* msl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* msl1_cr2[mc] + mxdist* msl1_cr2[mc+1])*  cydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_cr1[mc] + mxdist* msl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* msl2_cr2[mc] + mxdist* msl2_cr2[mc+1])*  cydist ) * mzdist ;
	p0cc= 
	  (((1-cxdist)* msl1_cr1[cc] + cxdist* msl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* msl1_cr2[cc] + cxdist* msl1_cr2[cc+1])*  cydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_cr1[cc] + cxdist* msl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* msl2_cr2[cc] + cxdist* msl2_cr2[cc+1])*  cydist ) * mzdist ;
	p0c1= 
	  (((1-pxdist)* msl1_cr1[pc] + pxdist* msl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* msl1_cr2[pc] + pxdist* msl1_cr2[pc+1])*  cydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_cr1[pc] + pxdist* msl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* msl2_cr2[pc] + pxdist* msl2_cr2[pc+1])*  cydist ) * mzdist ;
	
	p010= 
	  (((1-mxdist)* msl1_pr1[mc] + mxdist* msl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* msl1_pr2[mc] + mxdist* msl1_pr2[mc+1])*  pydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_pr1[mc] + mxdist* msl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* msl2_pr2[mc] + mxdist* msl2_pr2[mc+1])*  pydist ) * mzdist ;
	p01c= 
	  (((1-cxdist)* msl1_pr1[cc] + cxdist* msl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* msl1_pr2[cc] + cxdist* msl1_pr2[cc+1])*  pydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_pr1[cc] + cxdist* msl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* msl2_pr2[cc] + cxdist* msl2_pr2[cc+1])*  pydist ) * mzdist ;
	p011= 
	  (((1-pxdist)* msl1_pr1[pc] + pxdist* msl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* msl1_pr2[pc] + pxdist* msl1_pr2[pc+1])*  pydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_pr1[pc] + pxdist* msl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* msl2_pr2[pc] + pxdist* msl2_pr2[pc+1])*  pydist ) * mzdist ;
	



	pc00= 
	  (((1-mxdist)* csl1_mr1[mc] + mxdist* csl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* csl1_mr2[mc] + mxdist* csl1_mr2[mc+1])*  mydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_mr1[mc] + mxdist* csl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* csl2_mr2[mc] + mxdist* csl2_mr2[mc+1])*  mydist ) * czdist ;
	pc0c= 
	  (((1-cxdist)* csl1_mr1[cc] + cxdist* csl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* csl1_mr2[cc] + cxdist* csl1_mr2[cc+1])*  mydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_mr1[cc] + cxdist* csl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* csl2_mr2[cc] + cxdist* csl2_mr2[cc+1])*  mydist ) * czdist ;
	pc01= 
	  (((1-pxdist)* csl1_mr1[pc] + pxdist* csl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* csl1_mr2[pc] + pxdist* csl1_mr2[pc+1])*  mydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_mr1[pc] + pxdist* csl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* csl2_mr2[pc] + pxdist* csl2_mr2[pc+1])*  mydist ) * czdist ;

	pcc0= 
	  (((1-mxdist)* csl1_cr1[mc] + mxdist* csl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* csl1_cr2[mc] + mxdist* csl1_cr2[mc+1])*  cydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_cr1[mc] + mxdist* csl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* csl2_cr2[mc] + mxdist* csl2_cr2[mc+1])*  cydist ) * czdist ;
/*	pccc= 
	  (((1-cxdist)* csl1_cr1[cc] + cxdist* csl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* csl1_cr2[cc] + cxdist* csl1_cr2[cc+1])*  cydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_cr1[cc] + cxdist* csl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* csl2_cr2[cc] + cxdist* csl2_cr2[cc+1])*  cydist ) * czdist ;
*/
	pcc1= 
	  (((1-pxdist)* csl1_cr1[pc] + pxdist* csl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* csl1_cr2[pc] + pxdist* csl1_cr2[pc+1])*  cydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_cr1[pc] + pxdist* csl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* csl2_cr2[pc] + pxdist* csl2_cr2[pc+1])*  cydist ) * czdist ;
	
	pc10= 
	  (((1-mxdist)* csl1_pr1[mc] + mxdist* csl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* csl1_pr2[mc] + mxdist* csl1_pr2[mc+1])*  pydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_pr1[mc] + mxdist* csl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* csl2_pr2[mc] + mxdist* csl2_pr2[mc+1])*  pydist ) * czdist ;
	pc1c= 
	  (((1-cxdist)* csl1_pr1[cc] + cxdist* csl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* csl1_pr2[cc] + cxdist* csl1_pr2[cc+1])*  pydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_pr1[cc] + cxdist* csl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* csl2_pr2[cc] + cxdist* csl2_pr2[cc+1])*  pydist ) * czdist ;
	pc11= 
	  (((1-pxdist)* csl1_pr1[pc] + pxdist* csl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* csl1_pr2[pc] + pxdist* csl1_pr2[pc+1])*  pydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_pr1[pc] + pxdist* csl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* csl2_pr2[pc] + pxdist* csl2_pr2[pc+1])*  pydist ) * czdist ;
	



	p100= 
	  (((1-mxdist)* psl1_mr1[mc] + mxdist* psl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* psl1_mr2[mc] + mxdist* psl1_mr2[mc+1])*  mydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_mr1[mc] + mxdist* psl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* psl2_mr2[mc] + mxdist* psl2_mr2[mc+1])*  mydist ) * pzdist ;
	p10c= 
	  (((1-cxdist)* psl1_mr1[cc] + cxdist* psl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* psl1_mr2[cc] + cxdist* psl1_mr2[cc+1])*  mydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_mr1[cc] + cxdist* psl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* psl2_mr2[cc] + cxdist* psl2_mr2[cc+1])*  mydist ) * pzdist ;
	p101= 
	  (((1-pxdist)* psl1_mr1[pc] + pxdist* psl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* psl1_mr2[pc] + pxdist* psl1_mr2[pc+1])*  mydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_mr1[pc] + pxdist* psl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* psl2_mr2[pc] + pxdist* psl2_mr2[pc+1])*  mydist ) * pzdist ;
	
	p1c0= 
	  (((1-mxdist)* psl1_cr1[mc] + mxdist* psl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* psl1_cr2[mc] + mxdist* psl1_cr2[mc+1])*  cydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_cr1[mc] + mxdist* psl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* psl2_cr2[mc] + mxdist* psl2_cr2[mc+1])*  cydist ) * pzdist ;
	p1cc= 
	  (((1-cxdist)* psl1_cr1[cc] + cxdist* psl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* psl1_cr2[cc] + cxdist* psl1_cr2[cc+1])*  cydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_cr1[cc] + cxdist* psl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* psl2_cr2[cc] + cxdist* psl2_cr2[cc+1])*  cydist ) * pzdist ;
	p1c1= 
	  (((1-pxdist)* psl1_cr1[pc] + pxdist* psl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* psl1_cr2[pc] + pxdist* psl1_cr2[pc+1])*  cydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_cr1[pc] + pxdist* psl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* psl2_cr2[pc] + pxdist* psl2_cr2[pc+1])*  cydist ) * pzdist ;
	
	p110= 
	  (((1-mxdist)* psl1_pr1[mc] + mxdist* psl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* psl1_pr2[mc] + mxdist* psl1_pr2[mc+1])*  pydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_pr1[mc] + mxdist* psl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* psl2_pr2[mc] + mxdist* psl2_pr2[mc+1])*  pydist ) * pzdist ;
	p11c= 
	  (((1-cxdist)* psl1_pr1[cc] + cxdist* psl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* psl1_pr2[cc] + cxdist* psl1_pr2[cc+1])*  pydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_pr1[cc] + cxdist* psl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* psl2_pr2[cc] + cxdist* psl2_pr2[cc+1])*  pydist ) * pzdist ;
	p111= 
	  (((1-pxdist)* psl1_pr1[pc] + pxdist* psl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* psl1_pr2[pc] + pxdist* psl1_pr2[pc+1])*  pydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_pr1[pc] + pxdist* psl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* psl2_pr2[pc] + pxdist* psl2_pr2[pc+1])*  pydist ) * pzdist ;
	
	     
	
	l1=p1cc-p0cc; l2=p11c-p00c; l3=p10c-p01c; l4=p1c1-p0c0;
	l5=p1c0-p0c1; l6=p111-p000; l7=p110-p001; l8=p101-p010;
	l9=p100-p011;l10=pc1c-pc0c;l11=pc11-pc00;l12=pc10-pc01;
	l13=pcc1-pcc0;
	tse1->c[1] = G_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9));


      }
    }
  }
  

}







/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS0_26Normals_8Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS0     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 5/23/01new bit fields accommodated
 *                           by Dewey Odhner.
 *                        Modified: 3/27/03 shortened by Dewey Odhner.
 *                        Modified: 5/9/03 l12 contribution to x-
 *                           component corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS0_26Normals_8Scene()
{

  Cord_with_Norm **tse,*tse1;
  static unsigned char *gp[3][2][3][2]; /* [mcp]sl[12]_[mcp]r[12] */
  static unsigned char *pp[3][2]; /* [mcp]sl[12] */
  static double gzdist[3],gydist[3],gxdist[3]; /* [mcp][zyx]dist */
  static double gv[3][3][3]; /* p[0c1][0c1][0c1] */
  static double l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  int i,j,k,plus_slice,cur_slice,minus_slice,plus_row,cur_row,minus_row;
  int gc[3], sl_w,mult, B, i0, i1, i2;


  sl_w=vh1.scn.xysize[0]+2;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[2*i];
    gzdist[0]= z_dist[2*i];
    plus_slice= z_table[2*i+2];
    gzdist[2]= z_dist[2*i+2];
    cur_slice= z_table[2*i+1];
    gzdist[1]= z_dist[2*i+1];


    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,cur_slice+1,cur_slice+2,-1,-1,-1,-1);

    pp[0][0]=sl_ptr[minus_slice+1];
    pp[0][1]=sl_ptr[minus_slice+2];
    pp[1][0]=sl_ptr[cur_slice+1];
    pp[1][1]=sl_ptr[cur_slice+2];
    pp[2][0]=sl_ptr[plus_slice+1];
    pp[2][1]=sl_ptr[plus_slice+2];

    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[2*j];
      gydist[0]= y_dist[2*j];
      mult=sl_w*(minus_row+1);
      gp[0][0][0][0]= pp[0][0] + mult;
      gp[0][0][0][1]= gp[0][0][0][0] + sl_w;
      gp[0][1][0][0]= pp[0][1] + mult;
      gp[0][1][0][1]= gp[0][1][0][0] + sl_w;
      gp[1][0][0][0]= pp[1][0] + mult;
      gp[1][0][0][1]= gp[1][0][0][0] + sl_w;
      gp[1][1][0][0]= pp[1][1] + mult;
      gp[1][1][0][1]= gp[1][1][0][0] + sl_w;
      gp[2][0][0][0]= pp[2][0] + mult;
      gp[2][0][0][1]= gp[2][0][0][0] + sl_w;
      gp[2][1][0][0]= pp[2][1] + mult;
      gp[2][1][0][1]= gp[2][1][0][0] + sl_w;

      cur_row= y_table[2*j+1];
      gydist[1]= y_dist[2*j+1];
      mult=sl_w*(cur_row+1);
      gp[0][0][1][0]= pp[0][0] + mult;
      gp[0][0][1][1]= gp[0][0][1][0] + sl_w;
      gp[0][1][1][0]= pp[0][1] + mult;
      gp[0][1][1][1]= gp[0][1][1][0] + sl_w;
      gp[1][0][1][0]= pp[1][0] + mult;
      gp[1][0][1][1]= gp[1][0][1][0] + sl_w;
      gp[1][1][1][0]= pp[1][1] + mult;
      gp[1][1][1][1]= gp[1][1][1][0] + sl_w;
      gp[2][0][1][0]= pp[2][0] + mult;
      gp[2][0][1][1]= gp[2][0][1][0] + sl_w;
      gp[2][1][1][0]= pp[2][1] + mult;
      gp[2][1][1][1]= gp[2][1][1][0] + sl_w;

      plus_row= y_table[2*j+2];
      gydist[2]= y_dist[2*j+2];
      mult=sl_w*(plus_row+1);
      gp[0][0][2][0]= pp[0][0] + mult;
      gp[0][0][2][1]= gp[0][0][2][0] + sl_w;
      gp[0][1][2][0]= pp[0][1] + mult;
      gp[0][1][2][1]= gp[0][1][2][0] + sl_w;
      gp[1][0][2][0]= pp[1][0] + mult;
      gp[1][0][2][1]= gp[1][0][2][0] + sl_w;
      gp[1][1][2][0]= pp[1][1] + mult;
      gp[1][1][2][1]= gp[1][1][2][0] + sl_w;
      gp[2][0][2][0]= pp[2][0] + mult;
      gp[2][0][2][1]= gp[2][0][2][0] + sl_w;
      gp[2][1][2][0]= pp[2][1] + mult;
      gp[2][1][2][1]= gp[2][1][2][0] + sl_w;


      B = vh.str.bit_fields_in_TSE[3] > 15;

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
         k= tse1->c[0] & XMASK;
         if (B)
             k = k<<1 | ((tse1->c[1]&0x8000)!=0);
         gc[0]= x_table[2*k]+1;
         gxdist[0]= x_dist[2*k];

         gc[1]= x_table[2*k+1]+1;
         gxdist[1]= x_dist[2*k+1];

         gc[2]= x_table[2*k+2]+1;
         gxdist[2]= x_dist[2*k+2];

         for (i0=0; i0<3; i0++)
           for (i1=0; i1<3; i1++)
             for (i2=0; i2<3; i2++)
		       MFUNC(gv[i0][i1][i2], gxdist[i2],gydist[i1],gzdist[i0],
                 gp[i0][0][i1][0][gc[i2]],gp[i0][0][i1][0][gc[i2]+1],
				 gp[i0][0][i1][1][gc[i2]],gp[i0][0][i1][1][gc[i2]+1],
                 gp[i0][1][i1][0][gc[i2]],gp[i0][1][i1][0][gc[i2]+1],
			     gp[i0][1][i1][1][gc[i2]],gp[i0][1][i1][1][gc[i2]+1]);



         l1=gv[2][1][1]-gv[0][1][1]; l2=gv[2][2][1]-gv[0][0][1];
		 l3=gv[2][0][1]-gv[0][2][1]; l4=gv[2][1][2]-gv[0][1][0];
         l5=gv[2][1][0]-gv[0][1][2]; l6=gv[2][2][2]-gv[0][0][0];
		 l7=gv[2][2][0]-gv[0][0][2]; l8=gv[2][0][2]-gv[0][2][0];
         l9=gv[2][0][0]-gv[0][2][2];l10=gv[1][2][1]-gv[1][0][1];
		 l11=gv[1][2][2]-gv[1][0][0];l12=gv[1][2][0]-gv[1][0][2];
         l13=gv[1][1][2]-gv[1][1][0];
         if (B)
             tse1->c[1] = BG_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9))|
			    (tse1->c[1]&0x8000);
         else
             tse1->c[1] = G_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9));

      }
    }
  }
}





/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS0_26Normals_16Scene                  *
 *                                                                      *
 *      Description     : This would replace the normals of the BS0     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 16 bit input scene.                  *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 16 bits.                        *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 6/1/01new bit fields accommodated
 *                           by Dewey Odhner.
 *                        Modified: 5/9/03 l12 contribution to x-
 *                           component corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS0_26Normals_16Scene()
{

  static unsigned short *msl1,*msl2,*csl1,*csl2,*psl1,*psl2;
  static unsigned short *msl1_mr1,*msl2_mr1,*csl1_mr1,*csl2_mr1,*psl1_mr1,*psl2_mr1; 
  static unsigned short *msl1_mr2,*msl2_mr2,*csl1_mr2,*csl2_mr2,*psl1_mr2,*psl2_mr2; 
  static unsigned short *msl1_cr1,*msl2_cr1,*csl1_cr1,*csl2_cr1,*psl1_cr1,*psl2_cr1; 
  static unsigned short *msl1_cr2,*msl2_cr2,*csl1_cr2,*csl2_cr2,*psl1_cr2,*psl2_cr2; 
  static unsigned short *msl1_pr1,*msl2_pr1,*csl1_pr1,*csl2_pr1,*psl1_pr1,*psl2_pr1; 
  static unsigned short *msl1_pr2,*msl2_pr2,*csl1_pr2,*csl2_pr2,*psl1_pr2,*psl2_pr2; 
  static double mzdist,mydist,mxdist;
  static double czdist,cydist,cxdist;
  static double pzdist,pydist,pxdist;
  static double p000,p00c,p001,p0c0,p0cc,p0c1,p010,p01c,p011;
  static double pc00,pc0c,pc01,pcc0,     pcc1,pc10,pc1c,pc11;
  static double p100,p10c,p101,p1c0,p1cc,p1c1,p110,p11c,p111;
  static double l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,cur_slice,minus_slice,plus_row,cur_row,minus_row;
  int mc,cc,pc,sl_w,mult, B;

  
  sl_w=vh1.scn.xysize[0]+2;
  B = vh.str.bit_fields_in_TSE[3] > 15;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[2*i];
    mzdist= z_dist[2*i];
    plus_slice= z_table[2*i+2];
    pzdist= z_dist[2*i+2];
    cur_slice= z_table[2*i+1];
    czdist= z_dist[2*i+1];


    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,cur_slice+1,cur_slice+2,-1,-1,-1,-1);

    msl1=(unsigned short *)sl_ptr[minus_slice+1];
    msl2=(unsigned short *)sl_ptr[minus_slice+2];
    csl1=(unsigned short *)sl_ptr[cur_slice+1];
    csl2=(unsigned short *)sl_ptr[cur_slice+2];
    psl1=(unsigned short *)sl_ptr[plus_slice+1];
    psl2=(unsigned short *)sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[2*j];
      mydist= y_dist[2*j];
      mult=sl_w*(minus_row+1);
      msl1_mr1= msl1 + mult;
      msl1_mr2= msl1_mr1 + sl_w;
      msl2_mr1= msl2 + mult;
      msl2_mr2= msl2_mr1 + sl_w;
      csl1_mr1= csl1 + mult;
      csl1_mr2= csl1_mr1 + sl_w;
      csl2_mr1= csl2 + mult;
      csl2_mr2= csl2_mr1 + sl_w;
      psl1_mr1= psl1 + mult;
      psl1_mr2= psl1_mr1 + sl_w;
      psl2_mr1= psl2 + mult;
      psl2_mr2= psl2_mr1 + sl_w;
      
      cur_row= y_table[2*j+1];
      cydist= y_dist[2*j+1];
      mult=sl_w*(cur_row+1);
      msl1_cr1= msl1 + mult;
      msl1_cr2= msl1_cr1 + sl_w;
      msl2_cr1= msl2 + mult;
      msl2_cr2= msl2_cr1 + sl_w;
      csl1_cr1= csl1 + mult;
      csl1_cr2= csl1_cr1 + sl_w;
      csl2_cr1= csl2 + mult;
      csl2_cr2= csl2_cr1 + sl_w;
      psl1_cr1= psl1 + mult;
      psl1_cr2= psl1_cr1 + sl_w;
      psl2_cr1= psl2 + mult;
      psl2_cr2= psl2_cr1 + sl_w;

      plus_row= y_table[2*j+2];
      pydist= y_dist[2*j+2];
      mult=sl_w*(plus_row+1);
      msl1_pr1= msl1 + mult;
      msl1_pr2= msl1_pr1 + sl_w;
      msl2_pr1= msl2 + mult;
      msl2_pr2= msl2_pr1 + sl_w;
      csl1_pr1= csl1 + mult;
      csl1_pr2= csl1_pr1 + sl_w;
      csl2_pr1= csl2 + mult;
      csl2_pr2= csl2_pr1 + sl_w;
      psl1_pr1= psl1 + mult;
      psl1_pr2= psl1_pr1 + sl_w;
      psl2_pr1= psl2 + mult;
      psl2_pr2= psl2_pr1 + sl_w;


      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
	k= tse1->c[0] & XMASK;
	if (B)
	    k = k<<1 | ((tse1->c[1]&0x8000)!=0);
	mc= x_table[2*k]+1;
	mxdist= x_dist[2*k];
	
	cc= x_table[2*k+1]+1;
	cxdist= x_dist[2*k+1];
	
	pc= x_table[2*k+2]+1;
	pxdist= x_dist[2*k+2];

	MFUNC(p000,mxdist,mydist,mzdist,
	      msl1_mr1[mc],msl1_mr1[mc+1],msl1_mr2[mc],msl1_mr2[mc+1],
	      msl2_mr1[mc],msl2_mr1[mc+1],msl2_mr2[mc],msl2_mr2[mc+1]);
	p00c= 
	  (((1-cxdist)* msl1_mr1[cc] + cxdist* msl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* msl1_mr2[cc] + cxdist* msl1_mr2[cc+1])*  mydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_mr1[cc] + cxdist* msl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* msl2_mr2[cc] + cxdist* msl2_mr2[cc+1])*  mydist ) * mzdist ;
	p001= 
	  (((1-pxdist)* msl1_mr1[pc] + pxdist* msl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* msl1_mr2[pc] + pxdist* msl1_mr2[pc+1])*  mydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_mr1[pc] + pxdist* msl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* msl2_mr2[pc] + pxdist* msl2_mr2[pc+1])*  mydist ) * mzdist ;

	p0c0= 
	  (((1-mxdist)* msl1_cr1[mc] + mxdist* msl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* msl1_cr2[mc] + mxdist* msl1_cr2[mc+1])*  cydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_cr1[mc] + mxdist* msl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* msl2_cr2[mc] + mxdist* msl2_cr2[mc+1])*  cydist ) * mzdist ;
	p0cc= 
	  (((1-cxdist)* msl1_cr1[cc] + cxdist* msl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* msl1_cr2[cc] + cxdist* msl1_cr2[cc+1])*  cydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_cr1[cc] + cxdist* msl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* msl2_cr2[cc] + cxdist* msl2_cr2[cc+1])*  cydist ) * mzdist ;
	p0c1= 
	  (((1-pxdist)* msl1_cr1[pc] + pxdist* msl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* msl1_cr2[pc] + pxdist* msl1_cr2[pc+1])*  cydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_cr1[pc] + pxdist* msl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* msl2_cr2[pc] + pxdist* msl2_cr2[pc+1])*  cydist ) * mzdist ;
	
	p010= 
	  (((1-mxdist)* msl1_pr1[mc] + mxdist* msl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* msl1_pr2[mc] + mxdist* msl1_pr2[mc+1])*  pydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_pr1[mc] + mxdist* msl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* msl2_pr2[mc] + mxdist* msl2_pr2[mc+1])*  pydist ) * mzdist ;
	p01c= 
	  (((1-cxdist)* msl1_pr1[cc] + cxdist* msl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* msl1_pr2[cc] + cxdist* msl1_pr2[cc+1])*  pydist ) * (1-mzdist) +
	     (((1-cxdist)* msl2_pr1[cc] + cxdist* msl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* msl2_pr2[cc] + cxdist* msl2_pr2[cc+1])*  pydist ) * mzdist ;
	p011= 
	  (((1-pxdist)* msl1_pr1[pc] + pxdist* msl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* msl1_pr2[pc] + pxdist* msl1_pr2[pc+1])*  pydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_pr1[pc] + pxdist* msl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* msl2_pr2[pc] + pxdist* msl2_pr2[pc+1])*  pydist ) * mzdist ;
	



	pc00= 
	  (((1-mxdist)* csl1_mr1[mc] + mxdist* csl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* csl1_mr2[mc] + mxdist* csl1_mr2[mc+1])*  mydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_mr1[mc] + mxdist* csl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* csl2_mr2[mc] + mxdist* csl2_mr2[mc+1])*  mydist ) * czdist ;
	pc0c= 
	  (((1-cxdist)* csl1_mr1[cc] + cxdist* csl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* csl1_mr2[cc] + cxdist* csl1_mr2[cc+1])*  mydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_mr1[cc] + cxdist* csl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* csl2_mr2[cc] + cxdist* csl2_mr2[cc+1])*  mydist ) * czdist ;
	pc01= 
	  (((1-pxdist)* csl1_mr1[pc] + pxdist* csl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* csl1_mr2[pc] + pxdist* csl1_mr2[pc+1])*  mydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_mr1[pc] + pxdist* csl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* csl2_mr2[pc] + pxdist* csl2_mr2[pc+1])*  mydist ) * czdist ;

	pcc0= 
	  (((1-mxdist)* csl1_cr1[mc] + mxdist* csl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* csl1_cr2[mc] + mxdist* csl1_cr2[mc+1])*  cydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_cr1[mc] + mxdist* csl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* csl2_cr2[mc] + mxdist* csl2_cr2[mc+1])*  cydist ) * czdist ;
	pcc1= 
	  (((1-pxdist)* csl1_cr1[pc] + pxdist* csl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* csl1_cr2[pc] + pxdist* csl1_cr2[pc+1])*  cydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_cr1[pc] + pxdist* csl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* csl2_cr2[pc] + pxdist* csl2_cr2[pc+1])*  cydist ) * czdist ;
	
	pc10= 
	  (((1-mxdist)* csl1_pr1[mc] + mxdist* csl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* csl1_pr2[mc] + mxdist* csl1_pr2[mc+1])*  pydist ) * (1-czdist) +
	     (((1-mxdist)* csl2_pr1[mc] + mxdist* csl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* csl2_pr2[mc] + mxdist* csl2_pr2[mc+1])*  pydist ) * czdist ;
	pc1c= 
	  (((1-cxdist)* csl1_pr1[cc] + cxdist* csl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* csl1_pr2[cc] + cxdist* csl1_pr2[cc+1])*  pydist ) * (1-czdist) +
	     (((1-cxdist)* csl2_pr1[cc] + cxdist* csl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* csl2_pr2[cc] + cxdist* csl2_pr2[cc+1])*  pydist ) * czdist ;
	pc11= 
	  (((1-pxdist)* csl1_pr1[pc] + pxdist* csl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* csl1_pr2[pc] + pxdist* csl1_pr2[pc+1])*  pydist ) * (1-czdist) +
	     (((1-pxdist)* csl2_pr1[pc] + pxdist* csl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* csl2_pr2[pc] + pxdist* csl2_pr2[pc+1])*  pydist ) * czdist ;
	



	p100= 
	  (((1-mxdist)* psl1_mr1[mc] + mxdist* psl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* psl1_mr2[mc] + mxdist* psl1_mr2[mc+1])*  mydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_mr1[mc] + mxdist* psl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* psl2_mr2[mc] + mxdist* psl2_mr2[mc+1])*  mydist ) * pzdist ;
	p10c= 
	  (((1-cxdist)* psl1_mr1[cc] + cxdist* psl1_mr1[cc+1])*(1-mydist) +
	   ((1-cxdist)* psl1_mr2[cc] + cxdist* psl1_mr2[cc+1])*  mydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_mr1[cc] + cxdist* psl2_mr1[cc+1])*(1-mydist) +
	      ((1-cxdist)* psl2_mr2[cc] + cxdist* psl2_mr2[cc+1])*  mydist ) * pzdist ;
	p101= 
	  (((1-pxdist)* psl1_mr1[pc] + pxdist* psl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* psl1_mr2[pc] + pxdist* psl1_mr2[pc+1])*  mydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_mr1[pc] + pxdist* psl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* psl2_mr2[pc] + pxdist* psl2_mr2[pc+1])*  mydist ) * pzdist ;
	
	p1c0= 
	  (((1-mxdist)* psl1_cr1[mc] + mxdist* psl1_cr1[mc+1])*(1-cydist) +
	   ((1-mxdist)* psl1_cr2[mc] + mxdist* psl1_cr2[mc+1])*  cydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_cr1[mc] + mxdist* psl2_cr1[mc+1])*(1-cydist) +
	      ((1-mxdist)* psl2_cr2[mc] + mxdist* psl2_cr2[mc+1])*  cydist ) * pzdist ;
	p1cc= 
	  (((1-cxdist)* psl1_cr1[cc] + cxdist* psl1_cr1[cc+1])*(1-cydist) +
	   ((1-cxdist)* psl1_cr2[cc] + cxdist* psl1_cr2[cc+1])*  cydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_cr1[cc] + cxdist* psl2_cr1[cc+1])*(1-cydist) +
	      ((1-cxdist)* psl2_cr2[cc] + cxdist* psl2_cr2[cc+1])*  cydist ) * pzdist ;
	p1c1= 
	  (((1-pxdist)* psl1_cr1[pc] + pxdist* psl1_cr1[pc+1])*(1-cydist) +
	   ((1-pxdist)* psl1_cr2[pc] + pxdist* psl1_cr2[pc+1])*  cydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_cr1[pc] + pxdist* psl2_cr1[pc+1])*(1-cydist) +
	      ((1-pxdist)* psl2_cr2[pc] + pxdist* psl2_cr2[pc+1])*  cydist ) * pzdist ;
	
	p110= 
	  (((1-mxdist)* psl1_pr1[mc] + mxdist* psl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* psl1_pr2[mc] + mxdist* psl1_pr2[mc+1])*  pydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_pr1[mc] + mxdist* psl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* psl2_pr2[mc] + mxdist* psl2_pr2[mc+1])*  pydist ) * pzdist ;
	p11c= 
	  (((1-cxdist)* psl1_pr1[cc] + cxdist* psl1_pr1[cc+1])*(1-pydist) +
	   ((1-cxdist)* psl1_pr2[cc] + cxdist* psl1_pr2[cc+1])*  pydist ) * (1-pzdist) +
	     (((1-cxdist)* psl2_pr1[cc] + cxdist* psl2_pr1[cc+1])*(1-pydist) +
	      ((1-cxdist)* psl2_pr2[cc] + cxdist* psl2_pr2[cc+1])*  pydist ) * pzdist ;
	p111= 
	  (((1-pxdist)* psl1_pr1[pc] + pxdist* psl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* psl1_pr2[pc] + pxdist* psl1_pr2[pc+1])*  pydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_pr1[pc] + pxdist* psl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* psl2_pr2[pc] + pxdist* psl2_pr2[pc+1])*  pydist ) * pzdist ;
	
	     
	
	l1=p1cc-p0cc; l2=p11c-p00c; l3=p10c-p01c; l4=p1c1-p0c0;
	l5=p1c0-p0c1; l6=p111-p000; l7=p110-p001; l8=p101-p010;
	l9=p100-p011;l10=pc1c-pc0c;l11=pc11-pc00;l12=pc10-pc01;
	l13=pcc1-pcc0;
	if (B)
	    tse1->c[1] = BG_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9))|
			    (tse1->c[1]&0x8000);
	else
	    tse1->c[1] = G_code(l13 + ROOT2*(l11-l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
			    l10 + ROOT2*(l11+l12+l2-l3) + ROOT3*(l6+l7-l8-l9),
			    l1  + ROOT2*(l2 +l3 +l4+l5) + ROOT3*(l6+l7+l8+l9));


      }
    }
  }
  

}

/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS2_26Normals_8Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS2     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : sl_ptr, are updated.                          *
 *                                                                      *
 *      Entry condition : vh, cur_struct, sl_flag, @@ must be initialized.
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : Created: 7/3/03 by Dewey Odhner.
 *                        Modified: 7/14/03 equivalent triangles identified
 *                           by Dewey Odhner.
 *                        Modified: 8/29/03 anisotropic structure accounted for
 *                           by Dewey Odhner.
 *                        Modified: 10/4/05 initialization for each triangle
 *                           corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS2_26Normals_8Scene()
{

  unsigned char **tse,*tse1;
  static unsigned char *gp[9][3][2][3][2]; /* [mcp]sl[12]_[mcp]r[12] */
  unsigned char *msl1[9],*msl2[9],*csl1[9],*csl2[9],*psl1[9],*psl2[9];
  static double gzdist[9][3],gydist[9][3],gxdist[9][3]; /* [mcp][zyx]dist */
  static double gv[3][3][3]; /* p[0c1][0c1][0c1] */
  static double l1[9],l2[9],l3[9],l4[9],l5[9],l6[9],l7[9],l8[9],l9[9],l10[9],
    l11[9],l12[9],l13[9], w[3];
  int i,j,k,plus_slice[9],cur_slice[9],minus_slice[9],
    plus_row,cur_row,minus_row;
  int gc[3], sl_w,mult, i0, i1, i2, q;
  unsigned short new_gcode;
  double aspect[2];


  sl_w=vh1.scn.xysize[0]+2;
  aspect[0] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[0];
  aspect[1] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[1];
  for(tse=(unsigned char **)TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    for (q=1; q<=8; q++) {
	  i0 = 2*(i+T_z[q])-1;
      minus_slice[q]= z_table[i0];
      gzdist[q][0]= z_dist[i0];
	  i0 = 2*(i+T_z[q])+1;
      plus_slice[q]= z_table[i0];
      gzdist[q][2]= z_dist[i0];
	  i0 = 2*(i+T_z[q]);
      cur_slice[q]= z_table[i0];
      gzdist[q][1]= z_dist[i0];
	}
    AllocateSlices(minus_slice[1]+1,minus_slice[1]+2,plus_slice[1]+1,
	  plus_slice[1]+2,cur_slice[1]+1,cur_slice[1]+2,plus_slice[5]+1,
	  plus_slice[5]+2,cur_slice[5]+1,cur_slice[5]+2);
    for (q=1; q<=8; q++) {
      msl1[q]=sl_ptr[minus_slice[q]+1];
      msl2[q]=sl_ptr[minus_slice[q]+2];
      csl1[q]=sl_ptr[cur_slice[q]+1];
      csl2[q]=sl_ptr[cur_slice[q]+2];
      psl1[q]=sl_ptr[plus_slice[q]+1];
      psl2[q]=sl_ptr[plus_slice[q]+2];
	}

    for(j=0;j<row;j++,tse++) {

      for (q=1; q<=8; q++) {
		i1 = 2*(j+T_y[q])-1;
		minus_row= y_table[i1];
        gydist[q][0]= y_dist[i1];
        mult=sl_w*(minus_row+1);
        gp[q][0][0][0][0]= msl1[q] + mult;
        gp[q][0][0][0][1]= gp[q][0][0][0][0] + sl_w;
        gp[q][0][1][0][0]= msl2[q] + mult;
        gp[q][0][1][0][1]= gp[q][0][1][0][0] + sl_w;
        gp[q][1][0][0][0]= csl1[q] + mult;
        gp[q][1][0][0][1]= gp[q][1][0][0][0] + sl_w;
        gp[q][1][1][0][0]= csl2[q] + mult;
        gp[q][1][1][0][1]= gp[q][1][1][0][0] + sl_w;
        gp[q][2][0][0][0]= psl1[q] + mult;
        gp[q][2][0][0][1]= gp[q][2][0][0][0] + sl_w;
        gp[q][2][1][0][0]= psl2[q] + mult;
        gp[q][2][1][0][1]= gp[q][2][1][0][0] + sl_w;

		i1 = 2*(j+T_y[q]);
        cur_row= y_table[i1];
        gydist[q][1]= y_dist[i1];
        mult=sl_w*(cur_row+1);
        gp[q][0][0][1][0]= msl1[q] + mult;
        gp[q][0][0][1][1]= gp[q][0][0][1][0] + sl_w;
        gp[q][0][1][1][0]= msl2[q] + mult;
        gp[q][0][1][1][1]= gp[q][0][1][1][0] + sl_w;
        gp[q][1][0][1][0]= csl1[q] + mult;
        gp[q][1][0][1][1]= gp[q][1][0][1][0] + sl_w;
        gp[q][1][1][1][0]= csl2[q] + mult;
        gp[q][1][1][1][1]= gp[q][1][1][1][0] + sl_w;
        gp[q][2][0][1][0]= psl1[q] + mult;
        gp[q][2][0][1][1]= gp[q][2][0][1][0] + sl_w;
        gp[q][2][1][1][0]= psl2[q] + mult;
        gp[q][2][1][1][1]= gp[q][2][1][1][0] + sl_w;

		i1 = 2*(j+T_y[q])+1;
        plus_row= y_table[i1];
        gydist[q][2]= y_dist[i1];
        mult=sl_w*(plus_row+1);
        gp[q][0][0][2][0]= msl1[q] + mult;
        gp[q][0][0][2][1]= gp[q][0][0][2][0] + sl_w;
        gp[q][0][1][2][0]= msl2[q] + mult;
        gp[q][0][1][2][1]= gp[q][0][1][2][0] + sl_w;
        gp[q][1][0][2][0]= csl1[q] + mult;
        gp[q][1][0][2][1]= gp[q][1][0][2][0] + sl_w;
        gp[q][1][1][2][0]= csl2[q] + mult;
        gp[q][1][1][2][1]= gp[q][1][1][2][0] + sl_w;
        gp[q][2][0][2][0]= psl1[q] + mult;
        gp[q][2][0][2][1]= gp[q][2][0][2][0] + sl_w;
        gp[q][2][1][2][0]= psl2[q] + mult;
        gp[q][2][1][2][1]= gp[q][2][1][2][0] + sl_w;
	  }

      for(tse1= *tse; tse1 < *(tse+1) ; tse1+=3+3*number_of_triangles[*tse1]) {
       k = (int)(tse1[1])<<8|tse1[2];
       for (q=1; q<=8; q++) {
		 i2 = 2*(k+T_x[q])-1;
		 gc[0] = x_table[i2]+1;
         gxdist[q][0] = x_dist[i2];
         i2 = 2*(k+T_x[q]);
         gc[1] = x_table[i2]+1;
         gxdist[q][1] = x_dist[i2];
         i2 = 2*(k+T_x[q])+1;
         gc[2] = x_table[i2]+1;
         gxdist[q][2] = x_dist[i2];
         for (i0=0; i0<3; i0++)
          for (i1=0; i1<3; i1++)
           for (i2=0; i2<3; i2++)
		    MFUNC(gv[i0][i1][i2], gxdist[q][i2],gydist[q][i1],gzdist[q][i0],
                gp[q][i0][0][i1][0][gc[i2]],gp[q][i0][0][i1][0][gc[i2]+1],
				gp[q][i0][0][i1][1][gc[i2]],gp[q][i0][0][i1][1][gc[i2]+1],
                gp[q][i0][1][i1][0][gc[i2]],gp[q][i0][1][i1][0][gc[i2]+1],
			    gp[q][i0][1][i1][1][gc[i2]],gp[q][i0][1][i1][1][gc[i2]+1]);



         l1[q]= gv[2][1][1]-gv[0][1][1]; l2[q]=gv[2][2][1]-gv[0][0][1];
		 l3[q]= gv[2][0][1]-gv[0][2][1]; l4[q]=gv[2][1][2]-gv[0][1][0];
         l5[q]= gv[2][1][0]-gv[0][1][2]; l6[q]=gv[2][2][2]-gv[0][0][0];
		 l7[q]= gv[2][2][0]-gv[0][0][2]; l8[q]=gv[2][0][2]-gv[0][2][0];
         l9[q]= gv[2][0][0]-gv[0][2][2];l10[q]=gv[1][2][1]-gv[1][0][1];
		 l11[q]=gv[1][2][2]-gv[1][0][0];l12[q]=gv[1][2][0]-gv[1][0][2];
         l13[q]=gv[1][1][2]-gv[1][1][0];
	   }
	   for (i0=0; i0<number_of_triangles[*tse1]; i0++) {
	     l1[0] = l2[0] = l3[0] = l4[0] = l5[0] = l6[0] = l7[0] = l8[0] =
	       l9[0] = l10[0] = l11[0] = l12[0] = l13[0] = 0;
	     q = tse1[3+3*i0]>>5;
	     w[0] = .5+1./7*(q&4? q|~3: q&3);
	     q = tse1[3+3*i0]>>2;
	     w[1] = .5+1./7*(q&4? q|~3: q&3);
	     q = tse1[3+3*i0]<<1 | ((tse1[4+3*i0]&1<<7)!=0);
	     w[2] = .5+1./7*(q&4? q|~3: q&3);
	     for (q=0; q<3; q++) {
	        l1[0] += w[q]*l1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l2[0] += w[q]*l2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l3[0] += w[q]*l3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l4[0] += w[q]*l4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l5[0] += w[q]*l5[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l5[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l6[0] += w[q]*l6[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l6[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l7[0] += w[q]*l7[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l7[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l8[0] += w[q]*l8[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l8[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l9[0] += w[q]*l9[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l9[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l10[0]+= w[q]*l10[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l10[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l11[0]+= w[q]*l11[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l11[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l12[0]+= w[q]*l12[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l12[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l13[0]+= w[q]*l13[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l13[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	     }
		 new_gcode = BG_code((l13[0] + ROOT2*(l11[0]-l12[0]+l4[0]-l5[0]) +
	            ROOT3*(l6[0]-l7[0]+l8[0]-l9[0]))*aspect[0],
			    (l10[0] + ROOT2*(l11[0]+l12[0]+l2[0]-l3[0]) +
				 ROOT3*(l6[0]+l7[0]-l8[0]-l9[0]))*aspect[1],
			    l1[0]  + ROOT2*(l2[0] +l3[0] +l4[0]+l5[0]) +
				 ROOT3*(l6[0]+l7[0]+l8[0]+l9[0]));
		 tse1[4+3*i0] = (tse1[4+3*i0]&1<<7) | (new_gcode>>8);
		 tse1[5+3*i0] = new_gcode&255;
       }
	  }
    }
  }

}






/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS2_26Normals_16Scene                  *
 *                                                                      *
 *      Description     : This would replace the normals of the BS2     *
 *                        surface with a 26 neighbor normal computed    *
 *                        from the 16 bit input scene.                  *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : sl_ptr, are updated.                          *
 *                                                                      *
 *      Entry condition : vh, cur_struct, sl_flag, number_of_triangles,
 *                           @@ must be initialized.
 *                        Input data is 16 bits.                        *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : Created: 7/3/03 by Dewey Odhner.
 *                        Modified: 7/14/03 equivalent triangles identified
 *                           by Dewey Odhner.
 *                        Modified: 8/29/03 anisotropic structure accounted for
 *                           by Dewey Odhner.
 *                        Modified: 10/4/05 initialization for each triangle
 *                           corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS2_26Normals_16Scene()
{

  unsigned char **tse,*tse1;
  static unsigned short *gp[9][3][2][3][2]; /* [mcp]sl[12]_[mcp]r[12] */
  unsigned short *msl1[9],*msl2[9],*csl1[9],*csl2[9],*psl1[9],*psl2[9];
  static double gzdist[9][3],gydist[9][3],gxdist[9][3]; /* [mcp][zyx]dist */
  static double gv[3][3][3]; /* p[0c1][0c1][0c1] */
  static double l1[9],l2[9],l3[9],l4[9],l5[9],l6[9],l7[9],l8[9],l9[9],l10[9],
    l11[9],l12[9],l13[9], w[3];
  int i,j,k,plus_slice[9],cur_slice[9],minus_slice[9],
    plus_row,cur_row,minus_row;
  int gc[3], sl_w,mult, i0, i1, i2, q;
  unsigned short new_gcode;
  double aspect[2];


  sl_w=vh1.scn.xysize[0]+2;
  aspect[0] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[0];
  aspect[1] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[1];
  for(tse=(unsigned char **)TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    for (q=1; q<=8; q++) {
	  i0 = 2*(i+T_z[q])-1;
      minus_slice[q]= z_table[i0];
      gzdist[q][0]= z_dist[i0];
	  i0 = 2*(i+T_z[q])+1;
      plus_slice[q]= z_table[i0];
      gzdist[q][2]= z_dist[i0];
	  i0 = 2*(i+T_z[q]);
      cur_slice[q]= z_table[i0];
      gzdist[q][1]= z_dist[i0];
	}
    AllocateSlices(minus_slice[1]+1,minus_slice[1]+2,plus_slice[1]+1,
	  plus_slice[1]+2,cur_slice[1]+1,cur_slice[1]+2,plus_slice[5]+1,
	  plus_slice[5]+2,cur_slice[5]+1,cur_slice[5]+2);
    for (q=1; q<=8; q++) {
      msl1[q]= ((unsigned short **)sl_ptr)[minus_slice[q]+1];
      msl2[q]= ((unsigned short **)sl_ptr)[minus_slice[q]+2];
      csl1[q]= ((unsigned short **)sl_ptr)[cur_slice[q]+1];
      csl2[q]= ((unsigned short **)sl_ptr)[cur_slice[q]+2];
      psl1[q]= ((unsigned short **)sl_ptr)[plus_slice[q]+1];
      psl2[q]= ((unsigned short **)sl_ptr)[plus_slice[q]+2];
	}

    for(j=0;j<row;j++,tse++) {

      for (q=1; q<=8; q++) {
		i1 = 2*(j+T_y[q])-1;
		minus_row= y_table[i1];
        gydist[q][0]= y_dist[i1];
        mult=sl_w*(minus_row+1);
        gp[q][0][0][0][0]= msl1[q] + mult;
        gp[q][0][0][0][1]= gp[q][0][0][0][0] + sl_w;
        gp[q][0][1][0][0]= msl2[q] + mult;
        gp[q][0][1][0][1]= gp[q][0][1][0][0] + sl_w;
        gp[q][1][0][0][0]= csl1[q] + mult;
        gp[q][1][0][0][1]= gp[q][1][0][0][0] + sl_w;
        gp[q][1][1][0][0]= csl2[q] + mult;
        gp[q][1][1][0][1]= gp[q][1][1][0][0] + sl_w;
        gp[q][2][0][0][0]= psl1[q] + mult;
        gp[q][2][0][0][1]= gp[q][2][0][0][0] + sl_w;
        gp[q][2][1][0][0]= psl2[q] + mult;
        gp[q][2][1][0][1]= gp[q][2][1][0][0] + sl_w;

		i1 = 2*(j+T_y[q]);
        cur_row= y_table[i1];
        gydist[q][1]= y_dist[i1];
        mult=sl_w*(cur_row+1);
        gp[q][0][0][1][0]= msl1[q] + mult;
        gp[q][0][0][1][1]= gp[q][0][0][1][0] + sl_w;
        gp[q][0][1][1][0]= msl2[q] + mult;
        gp[q][0][1][1][1]= gp[q][0][1][1][0] + sl_w;
        gp[q][1][0][1][0]= csl1[q] + mult;
        gp[q][1][0][1][1]= gp[q][1][0][1][0] + sl_w;
        gp[q][1][1][1][0]= csl2[q] + mult;
        gp[q][1][1][1][1]= gp[q][1][1][1][0] + sl_w;
        gp[q][2][0][1][0]= psl1[q] + mult;
        gp[q][2][0][1][1]= gp[q][2][0][1][0] + sl_w;
        gp[q][2][1][1][0]= psl2[q] + mult;
        gp[q][2][1][1][1]= gp[q][2][1][1][0] + sl_w;

		i1 = 2*(j+T_y[q])+1;
        plus_row= y_table[i1];
        gydist[q][2]= y_dist[i1];
        mult=sl_w*(plus_row+1);
        gp[q][0][0][2][0]= msl1[q] + mult;
        gp[q][0][0][2][1]= gp[q][0][0][2][0] + sl_w;
        gp[q][0][1][2][0]= msl2[q] + mult;
        gp[q][0][1][2][1]= gp[q][0][1][2][0] + sl_w;
        gp[q][1][0][2][0]= csl1[q] + mult;
        gp[q][1][0][2][1]= gp[q][1][0][2][0] + sl_w;
        gp[q][1][1][2][0]= csl2[q] + mult;
        gp[q][1][1][2][1]= gp[q][1][1][2][0] + sl_w;
        gp[q][2][0][2][0]= psl1[q] + mult;
        gp[q][2][0][2][1]= gp[q][2][0][2][0] + sl_w;
        gp[q][2][1][2][0]= psl2[q] + mult;
        gp[q][2][1][2][1]= gp[q][2][1][2][0] + sl_w;
	  }

      for(tse1= *tse; tse1 < *(tse+1) ; tse1+=3+3*number_of_triangles[*tse1]) {
       k = (int)(tse1[1])<<8|tse1[2];
       for (q=1; q<=8; q++) {
		 i2 = 2*(k+T_x[q])-1;
		 gc[0] = x_table[i2]+1;
         gxdist[q][0] = x_dist[i2];
         i2 = 2*(k+T_x[q]);
         gc[1] = x_table[i2]+1;
         gxdist[q][1] = x_dist[i2];
         i2 = 2*(k+T_x[q])+1;
         gc[2] = x_table[i2]+1;
         gxdist[q][2] = x_dist[i2];
         for (i0=0; i0<3; i0++)
          for (i1=0; i1<3; i1++)
           for (i2=0; i2<3; i2++)
		    MFUNC(gv[i0][i1][i2], gxdist[q][i2],gydist[q][i1],gzdist[q][i0],
                gp[q][i0][0][i1][0][gc[i2]],gp[q][i0][0][i1][0][gc[i2]+1],
				gp[q][i0][0][i1][1][gc[i2]],gp[q][i0][0][i1][1][gc[i2]+1],
                gp[q][i0][1][i1][0][gc[i2]],gp[q][i0][1][i1][0][gc[i2]+1],
			    gp[q][i0][1][i1][1][gc[i2]],gp[q][i0][1][i1][1][gc[i2]+1]);



         l1[q]= gv[2][1][1]-gv[0][1][1]; l2[q]=gv[2][2][1]-gv[0][0][1];
		 l3[q]= gv[2][0][1]-gv[0][2][1]; l4[q]=gv[2][1][2]-gv[0][1][0];
         l5[q]= gv[2][1][0]-gv[0][1][2]; l6[q]=gv[2][2][2]-gv[0][0][0];
		 l7[q]= gv[2][2][0]-gv[0][0][2]; l8[q]=gv[2][0][2]-gv[0][2][0];
         l9[q]= gv[2][0][0]-gv[0][2][2];l10[q]=gv[1][2][1]-gv[1][0][1];
		 l11[q]=gv[1][2][2]-gv[1][0][0];l12[q]=gv[1][2][0]-gv[1][0][2];
         l13[q]=gv[1][1][2]-gv[1][1][0];
	   }
	   for (i0=0; i0<number_of_triangles[*tse1]; i0++) {
	     l1[0] = l2[0] = l3[0] = l4[0] = l5[0] = l6[0] = l7[0] = l8[0] =
	       l9[0] = l10[0] = l11[0] = l12[0] = l13[0] = 0;
	     q = tse1[3+3*i0]>>5;
	     w[0] = .5+1./7*(q&4? q|~3: q&3);
	     q = tse1[3+3*i0]>>2;
	     w[1] = .5+1./7*(q&4? q|~3: q&3);
	     q = tse1[3+3*i0]<<1 | ((tse1[4+3*i0]&1<<7)!=0);
	     w[2] = .5+1./7*(q&4? q|~3: q&3);
	     for (q=0; q<3; q++) {
	        l1[0] += w[q]*l1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l2[0] += w[q]*l2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l3[0] += w[q]*l3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l4[0] += w[q]*l4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l5[0] += w[q]*l5[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l5[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l6[0] += w[q]*l6[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l6[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l7[0] += w[q]*l7[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l7[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l8[0] += w[q]*l8[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l8[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l9[0] += w[q]*l9[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l9[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l10[0]+= w[q]*l10[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l10[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l11[0]+= w[q]*l11[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l11[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l12[0]+= w[q]*l12[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l12[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	        l13[0]+= w[q]*l13[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
	             (1-w[q])*l13[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
	     }
		 new_gcode = BG_code((l13[0] + ROOT2*(l11[0]-l12[0]+l4[0]-l5[0]) +
	            ROOT3*(l6[0]-l7[0]+l8[0]-l9[0]))*aspect[0],
			    (l10[0] + ROOT2*(l11[0]+l12[0]+l2[0]-l3[0]) +
				 ROOT3*(l6[0]+l7[0]-l8[0]-l9[0]))*aspect[1],
			    l1[0]  + ROOT2*(l2[0] +l3[0] +l4[0]+l5[0]) +
				 ROOT3*(l6[0]+l7[0]+l8[0]+l9[0]));
		 tse1[4+3*i0] = (tse1[4+3*i0]&1<<7) | (new_gcode>>8);
		 tse1[5+3*i0] = new_gcode&255;
       }
	  }
    }
  }

}



/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS1_8Normals_8Scene                    *
 *                                                                      *
 *      Description     : This would replace the normals of the BS1     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 3/27/03 shortened by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS1_8Normals_8Scene()
{

  unsigned char *msl1,*msl2,*psl1,*psl2;
  unsigned char *gp[2][2][2][2]; /* [mp]sl[12]_[mp]r[12] */
  double gzdist[2], gydist[2], gxdist[2]; /* [mp][zyx]dist */
  double gv[2][2][2];
  double v1,v2,v3,v4;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,minus_slice,plus_row,minus_row;
  int gc[2], sl_w,mult, i0, i1, i2;

  
  sl_w=vh1.scn.xysize[0]+2;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[i];
    gzdist[0]= z_dist[i];
    plus_slice= z_table[i+2];
    gzdist[1]= z_dist[i+2];
    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,-1,-1,-1,-1,-1,-1);


    msl1=sl_ptr[minus_slice+1];
    msl2=sl_ptr[minus_slice+2];
    psl1=sl_ptr[plus_slice+1];
    psl2=sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[j];
      gydist[0]= y_dist[j];
      mult=sl_w*(minus_row+1);
      gp[0][0][0][0]= msl1 + mult;
      gp[0][0][0][1]= gp[0][0][0][0] + sl_w;
      gp[0][1][0][0]= msl2 + mult;
      gp[0][1][0][1]= gp[0][1][0][0] + sl_w;
      gp[1][0][0][0]= psl1 + mult;
      gp[1][0][0][1]= gp[1][0][0][0] + sl_w;
      gp[1][1][0][0]= psl2 + mult;
      gp[1][1][0][1]= gp[1][1][0][0] + sl_w;
      
      plus_row= y_table[j+2];
      gydist[1]= y_dist[j+2];
      mult=sl_w*(plus_row+1);
      gp[0][0][1][0]= msl1 + mult;
      gp[0][0][1][1]= gp[0][0][1][0] + sl_w;
      gp[0][1][1][0]= msl2 + mult;
      gp[0][1][1][1]= gp[0][1][1][0] + sl_w;
      gp[1][0][1][0]= psl1 + mult;
      gp[1][0][1][1]= gp[1][0][1][0] + sl_w;
      gp[1][1][1][0]= psl2 + mult;
      gp[1][1][1][1]= gp[1][1][1][0] + sl_w;

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
        k= tse1->c[0] & C_MASK;
        gc[0]= x_table[k]+1;
        gxdist[0]= x_dist[k];
	
        gc[1]= x_table[k+2]+1;
        gxdist[1]= x_dist[k+2];

        for (i0=0; i0<2; i0++)
          for (i1=0; i1<2; i1++)
            for (i2=0; i2<2; i2++)
              gv[i0][i1][i2] =
                (((1-gxdist[i2])* gp[i0][0][i1][0][gc[i2]] +
				 gxdist[i2]* gp[i0][0][i1][0][gc[i2]+1])*(1-gydist[i1]) +
                 ((1-gxdist[i2])* gp[i0][0][i1][1][gc[i2]] +
				  gxdist[i2]* gp[i0][0][i1][1][gc[i2]+1])*  gydist[i1] ) *
				   (1-gzdist[i0]) +
                   (((1-gxdist[i2])* gp[i0][1][i1][0][gc[i2]] +
				    gxdist[i2]* gp[i0][1][i1][0][gc[i2]+1])*(1-gydist[i1]) +
                    ((1-gxdist[i2])* gp[i0][1][i1][1][gc[i2]] +
					 gxdist[i2]* gp[i0][1][i1][1][gc[i2]+1])*  gydist[i1] ) *
					  gzdist[i0] ;

        v1= gv[1][1][1]-gv[0][0][0];
        v2= gv[0][1][1]-gv[1][0][0];
        v3= gv[0][0][1]-gv[1][1][0];
        v4= gv[1][0][1]-gv[0][1][0];
        tse1->c[1] = G_code(v1+v2+v3+v4,v1+v2-v3-v4,v1-v2-v3+v4);

      }
    }
  }
  

}


/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS1_8Normals_16Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS1     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 16 bit input scene.                  *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 16 bits.                        *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                                                                      *
 ************************************************************************/
void ReplaceBS1_8Normals_16Scene()
{

  unsigned short *msl1,*msl2,*psl1,*psl2;
  unsigned short *msl1_mr1,*msl2_mr1,*psl1_mr1,*psl2_mr1; 
  unsigned short *msl1_mr2,*msl2_mr2,*psl1_mr2,*psl2_mr2; 
  unsigned short *msl1_pr1,*msl2_pr1,*psl1_pr1,*psl2_pr1; 
  unsigned short *msl1_pr2,*msl2_pr2,*psl1_pr2,*psl2_pr2; 
  double mzdist,mydist,mxdist,pzdist,pydist,pxdist;
  double p000,p001,p010,p011,p100,p101,p110,p111;
  double v1,v2,v3,v4;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,minus_slice,plus_row,minus_row;
  int mc,pc,sl_w,mult;

  
  sl_w=vh1.scn.xysize[0]+2;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[i];
    mzdist= z_dist[i];
    plus_slice= z_table[i+2];
    pzdist= z_dist[i+2];
    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,-1,-1,-1,-1,-1,-1);


    msl1=(unsigned short *)sl_ptr[minus_slice+1];
    msl2=(unsigned short *)sl_ptr[minus_slice+2];
    psl1=(unsigned short *)sl_ptr[plus_slice+1];
    psl2=(unsigned short *)sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[j];
      mydist= y_dist[j];
      mult=sl_w*(minus_row+1);
      msl1_mr1= msl1 + mult;
      msl1_mr2= msl1_mr1 + sl_w;
      msl2_mr1= msl2 + mult;
      msl2_mr2= msl2_mr1 + sl_w;
      psl1_mr1= psl1 + mult;
      psl1_mr2= psl1_mr1 + sl_w;
      psl2_mr1= psl2 + mult;
      psl2_mr2= psl2_mr1 + sl_w;
      
      plus_row= y_table[j+2];
      pydist= y_dist[j+2];
      mult=sl_w*(plus_row+1);
      msl1_pr1= msl1 + mult;
      msl1_pr2= msl1_pr1 + sl_w;
      msl2_pr1= msl2 + mult;
      msl2_pr2= msl2_pr1 + sl_w;
      psl1_pr1= psl1 + mult;
      psl1_pr2= psl1_pr1 + sl_w;
      psl2_pr1= psl2 + mult;
      psl2_pr2= psl2_pr1 + sl_w;

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
	k= tse1->c[0] & C_MASK;
	mc= x_table[k]+1;
	mxdist= x_dist[k];
	
	pc= x_table[k+2]+1;
	pxdist= x_dist[k+2];

	p000= 
	  (((1-mxdist)* msl1_mr1[mc] + mxdist* msl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* msl1_mr2[mc] + mxdist* msl1_mr2[mc+1])*  mydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_mr1[mc] + mxdist* msl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* msl2_mr2[mc] + mxdist* msl2_mr2[mc+1])*  mydist ) * mzdist ;
	p001= 
	  (((1-pxdist)* msl1_mr1[pc] + pxdist* msl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* msl1_mr2[pc] + pxdist* msl1_mr2[pc+1])*  mydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_mr1[pc] + pxdist* msl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* msl2_mr2[pc] + pxdist* msl2_mr2[pc+1])*  mydist ) * mzdist ;
	
	p010= 
	  (((1-mxdist)* msl1_pr1[mc] + mxdist* msl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* msl1_pr2[mc] + mxdist* msl1_pr2[mc+1])*  pydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_pr1[mc] + mxdist* msl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* msl2_pr2[mc] + mxdist* msl2_pr2[mc+1])*  pydist ) * mzdist ;
	p011= 
	  (((1-pxdist)* msl1_pr1[pc] + pxdist* msl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* msl1_pr2[pc] + pxdist* msl1_pr2[pc+1])*  pydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_pr1[pc] + pxdist* msl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* msl2_pr2[pc] + pxdist* msl2_pr2[pc+1])*  pydist ) * mzdist ;
	
	p100= 
	  (((1-mxdist)* psl1_mr1[mc] + mxdist* psl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* psl1_mr2[mc] + mxdist* psl1_mr2[mc+1])*  mydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_mr1[mc] + mxdist* psl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* psl2_mr2[mc] + mxdist* psl2_mr2[mc+1])*  mydist ) * pzdist ;
	p101= 
	  (((1-pxdist)* psl1_mr1[pc] + pxdist* psl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* psl1_mr2[pc] + pxdist* psl1_mr2[pc+1])*  mydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_mr1[pc] + pxdist* psl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* psl2_mr2[pc] + pxdist* psl2_mr2[pc+1])*  mydist ) * pzdist ;
	
	p110= 
	  (((1-mxdist)* psl1_pr1[mc] + mxdist* psl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* psl1_pr2[mc] + mxdist* psl1_pr2[mc+1])*  pydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_pr1[mc] + mxdist* psl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* psl2_pr2[mc] + mxdist* psl2_pr2[mc+1])*  pydist ) * pzdist ;
	p111= 
	  (((1-pxdist)* psl1_pr1[pc] + pxdist* psl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* psl1_pr2[pc] + pxdist* psl1_pr2[pc+1])*  pydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_pr1[pc] + pxdist* psl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* psl2_pr2[pc] + pxdist* psl2_pr2[pc+1])*  pydist ) * pzdist ;
	
	     
	
	v1= p111-p000; v2=p011-p100; v3=p001-p110; v4=p101-p010;
	tse1->c[1] = G_code(v1+v2+v3+v4,v1+v2-v3-v4,v1-v2-v3+v4);




      }
    }
  }
  

}







/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS0_8Normals_8Scene                    *
 *                                                                      *
 *      Description     : This would replace the normals of the BS0     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 6/1/01new bit fields accommodated
 *                           by Dewey Odhner.
 *                        Modified: 3/28/03 shortened by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS0_8Normals_8Scene()
{

  unsigned char *msl1,*msl2,*psl1,*psl2;
  unsigned char *gp[2][2][2][2]; /* [mp]sl[12]_[mp]r[12] */
  double gzdist[2], gydist[2], gxdist[2]; /* [mp][zyx]dist */
  double gv[2][2][2];
  double v1,v2,v3,v4;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,minus_slice,plus_row,minus_row;
  int gc[2], sl_w,mult, B, i0, i1, i2;

  
  sl_w=vh1.scn.xysize[0]+2;
  B = vh.str.bit_fields_in_TSE[3] > 15;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[2*i];
    gzdist[0]= z_dist[2*i];
    plus_slice= z_table[2*i+2];
    gzdist[1]= z_dist[2*i+2];
    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,-1,-1,-1,-1,-1,-1);


    msl1=sl_ptr[minus_slice+1];
    msl2=sl_ptr[minus_slice+2];
    psl1=sl_ptr[plus_slice+1];
    psl2=sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[2*j];
      gydist[0]= y_dist[2*j];
      mult=sl_w*(minus_row+1);
      gp[0][0][0][0]= msl1 + mult;
      gp[0][0][0][1]= gp[0][0][0][0] + sl_w;
      gp[0][1][0][0]= msl2 + mult;
      gp[0][1][0][1]= gp[0][1][0][0] + sl_w;
      gp[1][0][0][0]= psl1 + mult;
      gp[1][0][0][1]= gp[1][0][0][0] + sl_w;
      gp[1][1][0][0]= psl2 + mult;
      gp[1][1][0][1]= gp[1][1][0][0] + sl_w;
      
      plus_row= y_table[2*j+2];
      gydist[1]= y_dist[2*j+2];
      mult=sl_w*(plus_row+1);
      gp[0][0][1][0]= msl1 + mult;
      gp[0][0][1][1]= gp[0][0][1][0] + sl_w;
      gp[0][1][1][0]= msl2 + mult;
      gp[0][1][1][1]= gp[0][1][1][0] + sl_w;
      gp[1][0][1][0]= psl1 + mult;
      gp[1][0][1][1]= gp[1][0][1][0] + sl_w;
      gp[1][1][1][0]= psl2 + mult;
      gp[1][1][1][1]= gp[1][1][1][0] + sl_w;

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
        k= tse1->c[0] & XMASK;
        if (B)
            k = k<<1 | ((tse1->c[1]&0x8000)!=0);
        gc[0]= x_table[2*k]+1;
        gxdist[0]= x_dist[2*k];

        gc[1]= x_table[2*k+2]+1;
        gxdist[1]= x_dist[2*k+2];

        for (i0=0; i0<2; i0++)
          for (i1=0; i1<2; i1++)
            for (i2=0; i2<2; i2++)
              gv[i0][i1][i2] =
                (((1-gxdist[i2])* gp[i0][0][i1][0][gc[i2]] +
				 gxdist[i2]* gp[i0][0][i1][0][gc[i2]+1])*(1-gydist[i1]) +
                 ((1-gxdist[i2])* gp[i0][0][i1][1][gc[i2]] +
				  gxdist[i2]* gp[i0][0][i1][1][gc[i2]+1])*  gydist[i1] ) *
				   (1-gzdist[i0]) +
                   (((1-gxdist[i2])* gp[i0][1][i1][0][gc[i2]] +
				    gxdist[i2]* gp[i0][1][i1][0][gc[i2]+1])*(1-gydist[i1]) +
                    ((1-gxdist[i2])* gp[i0][1][i1][1][gc[i2]] +
					 gxdist[i2]* gp[i0][1][i1][1][gc[i2]+1])*  gydist[i1] ) *
					  gzdist[i0] ;

        v1= gv[1][1][1]-gv[0][0][0];
        v2= gv[0][1][1]-gv[1][0][0];
        v3= gv[0][0][1]-gv[1][1][0];
        v4= gv[1][0][1]-gv[0][1][0];
        if (B)
          tse1->c[1] = BG_code(v1+v2+v3+v4, v1+v2-v3-v4, v1-v2-v3+v4)|
            (tse1->c[1]&0x8000);
        else
          tse1->c[1] = G_code(v1+v2+v3+v4, v1+v2-v3-v4, v1-v2-v3+v4);


      }
    }
  }
  

}


/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS0_8Normals_16Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS0     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 16 bit input scene.                  *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : Normals are updated.                          *
 *                                                                      *
 *      Entry condition : All necessary tables are initialized.         *
 *                        Input data is 16 bits.                        *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 6/4/01 new bit fields accommodated
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS0_8Normals_16Scene()
{

  unsigned short *msl1,*msl2,*psl1,*psl2;
  unsigned short *msl1_mr1,*msl2_mr1,*psl1_mr1,*psl2_mr1; 
  unsigned short *msl1_mr2,*msl2_mr2,*psl1_mr2,*psl2_mr2; 
  unsigned short *msl1_pr1,*msl2_pr1,*psl1_pr1,*psl2_pr1; 
  unsigned short *msl1_pr2,*msl2_pr2,*psl1_pr2,*psl2_pr2; 
  double mzdist,mydist,mxdist,pzdist,pydist,pxdist;
  double p000,p001,p010,p011,p100,p101,p110,p111;
  double v1,v2,v3,v4;
  Cord_with_Norm **tse,*tse1;
  int i,j,k,plus_slice,minus_slice,plus_row,minus_row;
  int mc,pc,sl_w,mult, B;

  
  sl_w=vh1.scn.xysize[0]+2;
  B = vh.str.bit_fields_in_TSE[3] > 15;
  for(tse=TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    minus_slice= z_table[2*i];
    mzdist= z_dist[2*i];
    plus_slice= z_table[2*i+2];
    pzdist= z_dist[2*i+2];
    AllocateSlices(minus_slice+1,minus_slice+2,plus_slice+1,plus_slice+2,-1,-1,-1,-1,-1,-1);


    msl1=(unsigned short *)sl_ptr[minus_slice+1];
    msl2=(unsigned short *)sl_ptr[minus_slice+2];
    psl1=(unsigned short *)sl_ptr[plus_slice+1];
    psl2=(unsigned short *)sl_ptr[plus_slice+2];
    
    for(j=0;j<row;j++,tse++) {

      minus_row= y_table[2*j];
      mydist= y_dist[2*j];
      mult=sl_w*(minus_row+1);
      msl1_mr1= msl1 + mult;
      msl1_mr2= msl1_mr1 + sl_w;
      msl2_mr1= msl2 + mult;
      msl2_mr2= msl2_mr1 + sl_w;
      psl1_mr1= psl1 + mult;
      psl1_mr2= psl1_mr1 + sl_w;
      psl2_mr1= psl2 + mult;
      psl2_mr2= psl2_mr1 + sl_w;
      
      plus_row= y_table[2*j+2];
      pydist= y_dist[2*j+2];
      mult=sl_w*(plus_row+1);
      msl1_pr1= msl1 + mult;
      msl1_pr2= msl1_pr1 + sl_w;
      msl2_pr1= msl2 + mult;
      msl2_pr2= msl2_pr1 + sl_w;
      psl1_pr1= psl1 + mult;
      psl1_pr2= psl1_pr1 + sl_w;
      psl2_pr1= psl2 + mult;
      psl2_pr2= psl2_pr1 + sl_w;

      for(tse1= *tse; tse1 < *(tse+1) ; tse1++) {
	k= tse1->c[0] & XMASK;
	if (B)
	    k = k<<1 | ((tse1->c[1]&0x8000)!=0);
	mc= x_table[2*k]+1;
	mxdist= x_dist[2*k];
	
	pc= x_table[2*k+2]+1;
	pxdist= x_dist[2*k+2];

	p000= 
	  (((1-mxdist)* msl1_mr1[mc] + mxdist* msl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* msl1_mr2[mc] + mxdist* msl1_mr2[mc+1])*  mydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_mr1[mc] + mxdist* msl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* msl2_mr2[mc] + mxdist* msl2_mr2[mc+1])*  mydist ) * mzdist ;
	p001= 
	  (((1-pxdist)* msl1_mr1[pc] + pxdist* msl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* msl1_mr2[pc] + pxdist* msl1_mr2[pc+1])*  mydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_mr1[pc] + pxdist* msl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* msl2_mr2[pc] + pxdist* msl2_mr2[pc+1])*  mydist ) * mzdist ;
	
	p010= 
	  (((1-mxdist)* msl1_pr1[mc] + mxdist* msl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* msl1_pr2[mc] + mxdist* msl1_pr2[mc+1])*  pydist ) * (1-mzdist) +
	     (((1-mxdist)* msl2_pr1[mc] + mxdist* msl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* msl2_pr2[mc] + mxdist* msl2_pr2[mc+1])*  pydist ) * mzdist ;
	p011= 
	  (((1-pxdist)* msl1_pr1[pc] + pxdist* msl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* msl1_pr2[pc] + pxdist* msl1_pr2[pc+1])*  pydist ) * (1-mzdist) +
	     (((1-pxdist)* msl2_pr1[pc] + pxdist* msl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* msl2_pr2[pc] + pxdist* msl2_pr2[pc+1])*  pydist ) * mzdist ;
	
	p100= 
	  (((1-mxdist)* psl1_mr1[mc] + mxdist* psl1_mr1[mc+1])*(1-mydist) +
	   ((1-mxdist)* psl1_mr2[mc] + mxdist* psl1_mr2[mc+1])*  mydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_mr1[mc] + mxdist* psl2_mr1[mc+1])*(1-mydist) +
	      ((1-mxdist)* psl2_mr2[mc] + mxdist* psl2_mr2[mc+1])*  mydist ) * pzdist ;
	p101= 
	  (((1-pxdist)* psl1_mr1[pc] + pxdist* psl1_mr1[pc+1])*(1-mydist) +
	   ((1-pxdist)* psl1_mr2[pc] + pxdist* psl1_mr2[pc+1])*  mydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_mr1[pc] + pxdist* psl2_mr1[pc+1])*(1-mydist) +
	      ((1-pxdist)* psl2_mr2[pc] + pxdist* psl2_mr2[pc+1])*  mydist ) * pzdist ;
	
	p110= 
	  (((1-mxdist)* psl1_pr1[mc] + mxdist* psl1_pr1[mc+1])*(1-pydist) +
	   ((1-mxdist)* psl1_pr2[mc] + mxdist* psl1_pr2[mc+1])*  pydist ) * (1-pzdist) +
	     (((1-mxdist)* psl2_pr1[mc] + mxdist* psl2_pr1[mc+1])*(1-pydist) +
	      ((1-mxdist)* psl2_pr2[mc] + mxdist* psl2_pr2[mc+1])*  pydist ) * pzdist ;
	p111= 
	  (((1-pxdist)* psl1_pr1[pc] + pxdist* psl1_pr1[pc+1])*(1-pydist) +
	   ((1-pxdist)* psl1_pr2[pc] + pxdist* psl1_pr2[pc+1])*  pydist ) * (1-pzdist) +
	     (((1-pxdist)* psl2_pr1[pc] + pxdist* psl2_pr1[pc+1])*(1-pydist) +
	      ((1-pxdist)* psl2_pr2[pc] + pxdist* psl2_pr2[pc+1])*  pydist ) * pzdist ;
	
	     
	
	v1= p111-p000; v2=p011-p100; v3=p001-p110; v4=p101-p010;
	if (B)
	    tse1->c[1] = BG_code(v1+v2+v3+v4, v1+v2-v3-v4, v1-v2-v3+v4)|
			    (tse1->c[1]&0x8000);
	else
	    tse1->c[1] = G_code(v1+v2+v3+v4, v1+v2-v3-v4, v1-v2-v3+v4);




      }
    }
  }
  

}

/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS2_8Normals_8Scene                    *
 *                                                                      *
 *      Description     : This would replace the normals of the BS2     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 8 bit input scene.                   *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : sl_ptr, are updated.
 *                                                                      *
 *      Entry condition : vh, cur_struct, sl_flag, number_of_triangles,
 *                           @@ are initialized.
 *                        Input data is 8 bits.                         *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : Created: 7/7/03 by Dewey Odhner.
 *                        Modified: 7/14/03 equivalent triangles identified
 *                           by Dewey Odhner.
 *                        Modified: 8/29/03 anisotropic structure accounted for
 *                           by Dewey Odhner.
 *                        Modified: 10/4/05 initialization for each triangle
 *                           corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS2_8Normals_8Scene()
{

  unsigned char *msl1[9],*msl2[9],*psl1[9],*psl2[9];
  unsigned char *gp[9][2][2][2][2]; /* [mp]sl[12]_[mp]r[12] */
  double gzdist[9][2], gydist[9][2], gxdist[9][2]; /* [mp][zyx]dist */
  double gv[2][2][2];
  double v1[9],v2[9],v3[9],v4[9], w[3];
  unsigned char **tse,*tse1;
  int i,j,k, plus_slice[9], minus_slice[9], plus_row,minus_row;
  int gc[2], sl_w,mult, i0, i1, i2, q;
  unsigned short new_gcode;
  double aspect[2];


  sl_w=vh1.scn.xysize[0]+2;
  aspect[0] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[0];
  aspect[1] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[1];
  for(tse=(unsigned char **)TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    for (q=1; q<=8; q++) {
	  i0 = 2*(i+T_z[q])-1;
	  minus_slice[q] = z_table[i0];
	  gzdist[q][0] = z_dist[i0];
	  i0 = 2*(i+T_z[q])+1;
	  plus_slice[q] = z_table[i0];
	  gzdist[q][1] = z_dist[i0];
	}
    AllocateSlices(minus_slice[1]+1, minus_slice[1]+2, plus_slice[1]+1,
	  plus_slice[1]+2, plus_slice[5]+1, plus_slice[5]+2,-1,-1,-1,-1);
    for (q=1; q<=8; q++) {
	  msl1[q] = sl_ptr[minus_slice[q]+1];
      msl2[q] = sl_ptr[minus_slice[q]+2];
      psl1[q] = sl_ptr[plus_slice[q]+1];
      psl2[q] = sl_ptr[plus_slice[q]+2];
	}

    for(j=0;j<row;j++,tse++) {

      for (q=1; q<=8; q++) {
		i1 = 2*(j+T_y[q])-1;
		minus_row= y_table[i1];
        gydist[q][0]= y_dist[i1];
        mult=sl_w*(minus_row+1);
        gp[q][0][0][0][0]= msl1[q] + mult;
        gp[q][0][0][0][1]= gp[q][0][0][0][0] + sl_w;
        gp[q][0][1][0][0]= msl2[q] + mult;
        gp[q][0][1][0][1]= gp[q][0][1][0][0] + sl_w;
        gp[q][1][0][0][0]= psl1[q] + mult;
        gp[q][1][0][0][1]= gp[q][1][0][0][0] + sl_w;
        gp[q][1][1][0][0]= psl2[q] + mult;
        gp[q][1][1][0][1]= gp[q][1][1][0][0] + sl_w;
      
		i1 = 2*(j+T_y[q])+1;
        plus_row= y_table[i1];
        gydist[q][1]= y_dist[i1];
        mult=sl_w*(plus_row+1);
        gp[q][0][0][1][0]= msl1[q] + mult;
        gp[q][0][0][1][1]= gp[q][0][0][1][0] + sl_w;
        gp[q][0][1][1][0]= msl2[q] + mult;
        gp[q][0][1][1][1]= gp[q][0][1][1][0] + sl_w;
        gp[q][1][0][1][0]= psl1[q] + mult;
        gp[q][1][0][1][1]= gp[q][1][0][1][0] + sl_w;
        gp[q][1][1][1][0]= psl2[q] + mult;
        gp[q][1][1][1][1]= gp[q][1][1][1][0] + sl_w;
	  }

	  for(tse1= *tse; tse1 < *(tse+1) ; tse1+=3+3*number_of_triangles[*tse1]) {
        k = (int)(tse1[1])<<8|tse1[2];
        for (q=1; q<=8; q++) {
		  i2 = 2*(k+T_x[q])-1;
		  gc[0] = x_table[i2]+1;
          gxdist[q][0] = x_dist[i2];
          i2 = 2*(k+T_x[q])+1;
          gc[1] = x_table[i2]+1;
          gxdist[q][1] = x_dist[i2];

          for (i0=0; i0<2; i0++)
           for (i1=0; i1<2; i1++)
            for (i2=0; i2<2; i2++)
              gv[i0][i1][i2] =
                (((1-gxdist[q][i2]) * gp[q][i0][0][i1][0][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][0][i1][0][gc[i2]+1])*
				 (1-gydist[q][i1]) +
                 ((1-gxdist[q][i2]) * gp[q][i0][0][i1][1][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][0][i1][1][gc[i2]+1])*
				    gydist[q][i1] ) * (1-gzdist[q][i0]) +
                 (((1-gxdist[q][i2])* gp[q][i0][1][i1][0][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][1][i1][0][gc[i2]+1])*
				  (1-gydist[q][i1]) +
                  ((1-gxdist[q][i2])* gp[q][i0][1][i1][1][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][1][i1][1][gc[i2]+1])*
					gydist[q][i1] ) * gzdist[q][i0] ;

          v1[q]= gv[1][1][1]-gv[0][0][0];
          v2[q]= gv[0][1][1]-gv[1][0][0];
          v3[q]= gv[0][0][1]-gv[1][1][0];
          v4[q]= gv[1][0][1]-gv[0][1][0];
		}
	    for (i0=0; i0<number_of_triangles[*tse1]; i0++) {
		  v1[0] = v2[0] = v3[0] = v4[0] = 0;
	      q = tse1[3+3*i0]>>5;
	      w[0] = .5+1./7*(q&4? q|~3: q&3);
	      q = tse1[3+3*i0]>>2;
	      w[1] = .5+1./7*(q&4? q|~3: q&3);
	      q = tse1[3+3*i0]<<1 | ((tse1[4+3*i0]&1<<7)!=0);
	      w[2] = .5+1./7*(q&4? q|~3: q&3);
		  for (q=0; q<3; q++) {
		    v1[0] += w[q]*v1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v2[0] += w[q]*v2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v3[0] += w[q]*v3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v4[0] += w[q]*v4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		  }
		  new_gcode = BG_code((v1[0]+v2[0]+v3[0]+v4[0])*aspect[0],
		    (v1[0]+v2[0]-v3[0]-v4[0])*aspect[1], v1[0]-v2[0]-v3[0]+v4[0]);
		  tse1[4+3*i0] = (tse1[4+3*i0]&1<<7) | (new_gcode>>8);
		  tse1[5+3*i0] = new_gcode&255;
        }
      }
    }
  }

}


/************************************************************************
 *                                                                      *
 *      Function        : ReplaceBS2_8Normals_16Scene                   *
 *                                                                      *
 *      Description     : This would replace the normals of the BS2     *
 *                        surface with a  8 neighbor normal computed    *
 *                        from the 16 bit input scene.                  *
 *                        Note: The slice read in from the scene is     *
 *                        padded around with a row of pixels.           *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : sl_ptr, are updated.                          *
 *                                                                      *
 *      Entry condition : vh, cur_struct, sl_flag, number_of_triangles,
 *                           @@ must be initialized.
 *                        Input data is 16 bits.                        *
 *                                                                      *
 *      Related funcs   : AllocateSlices().                             *
 *                                                                      *
 *      History         : 7/7/03 by Dewey Odhner.
 *                        Modified: 7/14/03 equivalent triangles identified
 *                           by Dewey Odhner.
 *                        Modified: 8/29/03 anisotropic structure accounted for
 *                           by Dewey Odhner.
 *                        Modified: 10/4/05 initialization for each triangle
 *                           corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ReplaceBS2_8Normals_16Scene()
{

  unsigned short *msl1[9],*msl2[9],*psl1[9],*psl2[9];
  unsigned short *gp[9][2][2][2][2]; /* [mp]sl[12]_[mp]r[12] */
  double gzdist[9][2], gydist[9][2], gxdist[9][2]; /* [mp][zyx]dist */
  double gv[2][2][2];
  double v1[9],v2[9],v3[9],v4[9], w[3];
  unsigned char **tse,*tse1;
  int i,j,k, plus_slice[9], minus_slice[9], plus_row,minus_row;
  int gc[2], sl_w,mult, i0, i1, i2, q;
  unsigned short new_gcode;
  double aspect[2];


  sl_w=vh1.scn.xysize[0]+2;
  aspect[0] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[0];
  aspect[1] =
    (vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0])/vh.str.xysize[1];
  for(tse=(unsigned char **)TSE,i=0;i<sl;i++) {

    printf("Normals for object %d/%d slice %d/%d\n",cur_struct,max_structs,i+1,sl);
    fflush(stdout);

    for (q=1; q<=8; q++) {
	  i0 = 2*(i+T_z[q])-1;
	  minus_slice[q] = z_table[i0];
	  gzdist[q][0] = z_dist[i0];
	  i0 = 2*(i+T_z[q])+1;
	  plus_slice[q] = z_table[i0];
	  gzdist[q][1] = z_dist[i0];
	}
    AllocateSlices(minus_slice[1]+1, minus_slice[1]+2, plus_slice[1]+1,
	  plus_slice[1]+2, plus_slice[5]+1, plus_slice[5]+2,-1,-1,-1,-1);
    for (q=1; q<=8; q++) {
	  msl1[q] = (unsigned short *)sl_ptr[minus_slice[q]+1];
      msl2[q] = (unsigned short *)sl_ptr[minus_slice[q]+2];
      psl1[q] = (unsigned short *)sl_ptr[plus_slice[q]+1];
      psl2[q] = (unsigned short *)sl_ptr[plus_slice[q]+2];
	}

    for(j=0;j<row;j++,tse++) {

      for (q=1; q<=8; q++) {
		i1 = 2*(j+T_y[q])-1;
		minus_row= y_table[i1];
        gydist[q][0]= y_dist[i1];
        mult=sl_w*(minus_row+1);
        gp[q][0][0][0][0]= msl1[q] + mult;
        gp[q][0][0][0][1]= gp[q][0][0][0][0] + sl_w;
        gp[q][0][1][0][0]= msl2[q] + mult;
        gp[q][0][1][0][1]= gp[q][0][1][0][0] + sl_w;
        gp[q][1][0][0][0]= psl1[q] + mult;
        gp[q][1][0][0][1]= gp[q][1][0][0][0] + sl_w;
        gp[q][1][1][0][0]= psl2[q] + mult;
        gp[q][1][1][0][1]= gp[q][1][1][0][0] + sl_w;
      
		i1 = 2*(j+T_y[q])+1;
        plus_row= y_table[i1];
        gydist[q][1]= y_dist[i1];
        mult=sl_w*(plus_row+1);
        gp[q][0][0][1][0]= msl1[q] + mult;
        gp[q][0][0][1][1]= gp[q][0][0][1][0] + sl_w;
        gp[q][0][1][1][0]= msl2[q] + mult;
        gp[q][0][1][1][1]= gp[q][0][1][1][0] + sl_w;
        gp[q][1][0][1][0]= psl1[q] + mult;
        gp[q][1][0][1][1]= gp[q][1][0][1][0] + sl_w;
        gp[q][1][1][1][0]= psl2[q] + mult;
        gp[q][1][1][1][1]= gp[q][1][1][1][0] + sl_w;
	  }

	  for(tse1= *tse; tse1 < *(tse+1) ; tse1+=3+3*number_of_triangles[*tse1]) {
        k = (int)(tse1[1])<<8|tse1[2];
        for (q=1; q<=8; q++) {
		  i2 = 2*(k+T_x[q])-1;
		  gc[0] = x_table[i2]+1;
          gxdist[q][0] = x_dist[i2];
          i2 = 2*(k+T_x[q])+1;
          gc[1] = x_table[i2]+1;
          gxdist[q][1] = x_dist[i2];

          for (i0=0; i0<2; i0++)
           for (i1=0; i1<2; i1++)
            for (i2=0; i2<2; i2++)
              gv[i0][i1][i2] =
                (((1-gxdist[q][i2]) * gp[q][i0][0][i1][0][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][0][i1][0][gc[i2]+1])*
				 (1-gydist[q][i1]) +
                 ((1-gxdist[q][i2]) * gp[q][i0][0][i1][1][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][0][i1][1][gc[i2]+1])*
				    gydist[q][i1] ) * (1-gzdist[q][i0]) +
                 (((1-gxdist[q][i2])* gp[q][i0][1][i1][0][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][1][i1][0][gc[i2]+1])*
				  (1-gydist[q][i1]) +
                  ((1-gxdist[q][i2])* gp[q][i0][1][i1][1][gc[i2]] +
				  gxdist[q][i2]* gp[q][i0][1][i1][1][gc[i2]+1])*
					gydist[q][i1] ) * gzdist[q][i0] ;

          v1[q]= gv[1][1][1]-gv[0][0][0];
          v2[q]= gv[0][1][1]-gv[1][0][0];
          v3[q]= gv[0][0][1]-gv[1][1][0];
          v4[q]= gv[1][0][1]-gv[0][1][0];
		}
	    for (i0=0; i0<number_of_triangles[*tse1]; i0++) {
		  v1[0] = v2[0] = v3[0] = v4[0] = 0;
	      q = tse1[3+3*i0]>>5;
	      w[0] = .5+1./7*(q&4? q|~3: q&3);
	      q = tse1[3+3*i0]>>2;
	      w[1] = .5+1./7*(q&4? q|~3: q&3);
	      q = tse1[3+3*i0]<<1 | ((tse1[4+3*i0]&1<<7)!=0);
	      w[2] = .5+1./7*(q&4? q|~3: q&3);
		  for (q=0; q<3; q++) {
		    v1[0] += w[q]*v1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v1[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v2[0] += w[q]*v2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v2[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v3[0] += w[q]*v3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v3[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		    v4[0] += w[q]*v4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][1]]+
		         (1-w[q])*v4[edge_vertices[triangle_edges[
					triangle_table[tse1[0]][i0]][q]][0]];
		  }
		  new_gcode = BG_code((v1[0]+v2[0]+v3[0]+v4[0])*aspect[0],
		    (v1[0]+v2[0]-v3[0]-v4[0])*aspect[1], v1[0]-v2[0]-v3[0]+v4[0]);
		  tse1[4+3*i0] = (tse1[4+3*i0]&1<<7) | (new_gcode>>8);
		  tse1[5+3*i0] = new_gcode&255;
        }
      }
    }
  }

}





/************************************************************************
 *                                                                      *
 *      Function        : UpdateFile()                                  *
 *                                                                      *
 *      Description     : Update the structure file with the updated    *
 *                        TSE values.                                   *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : TSE's should be updated.                      *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 7/9/03 for SHELL2 by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void UpdateFile()
{

  int i,skip_size,rtemp;

  
  skip_size=0;
  for(i=0;i<cur_struct-1;i++) {
    skip_size += (vh.str.num_of_NTSE[i]*vh.str.num_of_bits_in_NTSE)/8 +
      (vh.str.num_of_TSE[i]*vh.str.num_of_bits_in_TSE)/8 ;
  }
  skip_size += (vh.str.num_of_NTSE[cur_struct-1]*vh.str.num_of_bits_in_NTSE)/8;
  error=VSeekData(infp,skip_size);
  if (error) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }


  if (vh.gen.data_type==SHELL2?
      VWriteData((char *)TSE[0],1,vh.str.num_of_TSE[cur_struct-1],infp,&rtemp):
      VWriteData((char *)TSE[0],2,vh.str.num_of_TSE[cur_struct-1]*2,infp,&rtemp)) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }
    

}



/************************************************************************
 *                                                                      *
 *      Function        : AllocateSlices8                               *
 *                                                                      *
 *      Description     : Allocate slices s1 through s6 and read the    *
 *                        data from a 8 bit scene. if sn is 0 or greater*
 *                        than the max possible slice it would generate *
 *                        empty slices. Pad the slice with a row of     *
 *                        pixels all around.                            *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : s1 through s10. The slices to be read.         *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : if the sl_flag is set for any slice do not    *
 *                        read the slice as it is already in memory.    *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 4/8/03 to 10 slices by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void AllocateSlices8(s1,s2,s3,s4,s5,s6, s7, s8, s9, s10)
int s1,s2,s3,s4,s5,s6, s7, s8, s9, s10;
{
  static unsigned long hdr_len;
  static unsigned char *in_array;
  unsigned char *in,*out;
  int i,j,alloc_size,tmp;

  if (hdr_len==0)
  {
    VGetHeaderLength(infp1, &j);
	hdr_len = (unsigned long)j;
  }
  if (in_array==NULL) 
    in_array=(unsigned char *)malloc(scx_size*scy_size);

  alloc_size= (scx_size+2)*(scy_size+2);
  for(i=0;i<s1;i++)
    if (sl_flag[i]!=0) {
      free(sl_ptr[i]); 
      sl_ptr[i]=NULL; 
      sl_flag[i]=0;
    }

  if (sl_flag[s1]==0) {
    sl_ptr[s1]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s1]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }
    if (s1 && s1 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s1-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s1]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s1]=1;
  }

  if (sl_flag[s2]==0) {
    sl_ptr[s2]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s2]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }
    if (s2 && s2 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s2-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s2]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s2]=1;
  }

  if (sl_flag[s3]==0) {
    sl_ptr[s3]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s3]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s3 && s3 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s3-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s3]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s3]=1;
  }

  if (sl_flag[s4]==0) {
    sl_ptr[s4]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s4]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s4 && s4 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s4-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s4]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s4]=1;
  }


  if (s5>-1 && sl_flag[s5]==0) {
    sl_ptr[s5]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s5]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s5 && s5 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s5-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s5]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s5]=1;
  }

  if (s6> -1 && sl_flag[s6]==0) {
    sl_ptr[s6]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s6]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s6 && s6 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s6-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s6]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s6]=1;
  }

  if (s7> -1 && sl_flag[s7]==0) {
    sl_ptr[s7]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s7]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s7 && s7 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s7-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s7]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s7]=1;
  }

  if (s8> -1 && sl_flag[s8]==0) {
    sl_ptr[s8]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s8]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s8 && s8 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s8-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s8]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s8]=1;
  }

  if (s9> -1 && sl_flag[s9]==0) {
    sl_ptr[s9]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s9]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s9 && s9 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s9-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s9]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s9]=1;
  }

  if (s10> -1 && sl_flag[s10]==0) {
    sl_ptr[s10]=(unsigned char *)calloc(alloc_size,1);
    if (sl_ptr[s10]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s10 && s10 <= scz_size ) {
      fseek(infp1,hdr_len+ scx_size*scy_size*(vol_skip+s10-1),0);
      if (VReadData((char *)in_array,1,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=sl_ptr[s10]+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s10]=1;
  }


}





/************************************************************************
 *                                                                      *
 *      Function        : AllocateSlices16                              *
 *                                                                      *
 *      Description     : Allocate slices s1 through s6 and read the    *
 *                        data from a 16bit scene. If sn is 0 or greater*
 *                        than the max possible slice it would generate *
 *                        empty slices. Pad the slice with a row of     *
 *                        pixels all around.                            *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : s1 through s10. The slices to be read.         *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : if the sl_flag is set for any slice do not    *
 *                        read the slice as it is already in memory.    *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 4/8/03 to 10 slices by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void AllocateSlices16(s1,s2,s3,s4,s5,s6, s7, s8, s9, s10)
int s1,s2,s3,s4,s5,s6, s7, s8, s9, s10;
{
  static unsigned long hdr_len;
  static unsigned short *in_array;
  unsigned short *in,*out;
  int i,j,alloc_size,tmp;

  if (hdr_len==0)
  {
    VGetHeaderLength(infp1, &j);
	hdr_len = (unsigned long)j;
  }
  if (in_array==NULL) 
    in_array=(unsigned short *)malloc(2*scx_size*scy_size);

  alloc_size= (scx_size+2)*(scy_size+2);
  for(i=0;i<s1;i++)
    if (sl_flag[i]!=0) {
      free(sl_ptr[i]); 
      sl_ptr[i]=NULL; 
      sl_flag[i]=0;
    }

  if (sl_flag[s1]==0) {
    sl_ptr[s1]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s1]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }
    if (s1 && s1 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s1-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s1])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s1]=1;
  }

  if (sl_flag[s2]==0) {
    sl_ptr[s2]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s2]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }
    if (s2 && s2 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s2-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s2])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s2]=1;
  }

  if (sl_flag[s3]==0) {
    sl_ptr[s3]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s3]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s3 && s3 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s3-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s3])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s3]=1;
  }

  if (sl_flag[s4]==0) {
    sl_ptr[s4]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s4]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s4 && s4 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s4-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s4])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s4]=1;
  }


  if (s5>-1 && sl_flag[s5]==0) {
    sl_ptr[s5]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s5]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s5 && s5 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s5-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s5])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s5]=1;
  }


  if (s6>-1 && sl_flag[s6]==0) {
    sl_ptr[s6]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s6]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s6 && s6 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s6-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s6])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s6]=1;
  }

  if (s7>-1 && sl_flag[s7]==0) {
    sl_ptr[s7]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s7]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s7 && s7 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s7-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s7])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s7]=1;
  }

  if (s8>-1 && sl_flag[s8]==0) {
    sl_ptr[s8]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s8]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s8 && s8 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s8-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s8])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s8]=1;
  }

  if (s9>-1 && sl_flag[s9]==0) {
    sl_ptr[s9]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s9]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s9 && s9 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s9-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s9])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s9]=1;
  }

  if (s10>-1 && sl_flag[s10]==0) {
    sl_ptr[s10]=(unsigned char *)calloc(alloc_size,2);
    if (sl_ptr[s10]==NULL) {
      printf("Memory Allocation fault. Cannot Continue\n");
      fflush(stdout);
      exit(-1);
    }

    if (s10 && s10 <= scz_size ) {
      fseek(infp1,hdr_len+ 2*scx_size*scy_size*(vol_skip+s10-1),0);
      if (VReadData((char *)in_array,2,scx_size*scy_size,infp1,&tmp)) {
	printf("Could not read scene data\n");
	fflush(stdout);
	exit(-1);
      }
      for(i=0,out=((unsigned short *)sl_ptr[s10])+scx_size+3,in=in_array;i<scy_size;i++,out+=2)
	for(j=0;j<scx_size;j++,in++,out++)
	  *out= *in;
    }
    sl_flag[s10]=1;
  }


}









/************************************************************************
 *                                                                      *
 *      Function        : FreeSpace()                                   *
 *                                                                      *
 *      Description     : This frees allocated space for the tabels and *
 *                        the TSE's                                     *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : None.                                         *
 *                                                                      *
 *      Entry condition : None.                                         *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                                                                      *
 ************************************************************************/
void FreeSpace()
{
  int i;

  if (TSE[0]) free(TSE[0]);
  if (TSE) free(TSE);

  if (x_table) free(x_table-1);
  if (y_table) free(y_table-1);
  if (z_table) free(z_table-1);
  

  if (x_dist) free(x_dist-1);
  if (y_dist) free(y_dist-1);
  if (z_dist) free(z_dist-1);


  for(i=0;i<scz_size+2;i++)
    if (sl_flag[i] && sl_ptr[i]) free(sl_ptr[i]);
  if (sl_flag) free(sl_flag);
  if (sl_ptr) free(sl_ptr);
  

}




      
int Inv3Mat(in,out)
  double in[3][3],out[3][3];
{
  double t,Det3(double m[3][3]);

  t=Det3(in);
  if (t) {
    out[0][0]=(in[1][1]*in[2][2]-in[1][2]*in[2][1])/t;
    out[0][1]=(in[0][2]*in[2][1]-in[0][1]*in[2][2])/t;
    out[0][2]=(in[0][1]*in[1][2]-in[0][2]*in[1][1])/t;
    out[1][0]=(in[1][2]*in[2][0]-in[1][0]*in[2][2])/t;
    out[1][1]=(in[0][0]*in[2][2]-in[0][2]*in[2][0])/t;
    out[1][2]=(in[0][2]*in[1][0]-in[0][0]*in[1][2])/t;
    out[2][0]=(in[1][0]*in[2][1]-in[1][1]*in[2][0])/t;
    out[2][1]=(in[0][1]*in[2][0]-in[0][0]*in[2][1])/t;
    out[2][2]=(in[0][0]*in[1][1]-in[0][1]*in[1][0])/t;
    return(1);
  } 
  else 
    return(0);
}

double Det3(m)
     double m[3][3];
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}
