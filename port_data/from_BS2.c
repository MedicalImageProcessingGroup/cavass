/*
  Copyright 1993-2014, 2016-2017, 2019 Medical Image Processing Group
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
#include "3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/neighbors.h"

#define ROOT2 0.7071067811865475244008443
#define ROOT3 0.5773502691896257645091487

typedef struct { unsigned short c[2]; } Cord_with_Norm;
#define POSITIVE (unsigned short)(0x8000)
#define C_MASK (unsigned short)(0x7FFF)

int Inv3Mat(double in[3][3], double out[3][3]);
double Det3(double m[3][3]);
void write_stl(), ReadStructure();

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

*----------------------------------------------------------------------*
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


char *image_name;
int max_structs,cur_struct,cur_volume,neighbors;


Cord_with_Norm **TSE;
short sl,row;

FILE *infp,*outfp;
ViewnixHeader vh;


int vol_skip;



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
  int bg_flag, j;
  
  if (argc!=5) {
    fprintf(stderr,"Usage:%s <shell_file> <struct_num> <STL_file> <bg_flag>\n",
		argv[0]);
    exit(-1);
  }


  image_name = argv[3];
  
  if (sscanf(argv[2],"%d",&cur_struct)!=1) {
    fprintf(stderr, "Invalid structure number specified\n");
    exit(-1);
  }

  bg_flag=atoi(argv[4]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  if ((infp=fopen(argv[1],"rb"))==NULL) {
    fprintf(stderr,"Could not open %s\n",argv[1]);
    exit(-1);
  }
  else {
    error=VReadHeader(infp,&vh,group,elem);
    if (error && error!=106 && error!=107) {
	  fprintf(stderr, "Read error %d group %s element %s\n",error,group,elem);
      exit(-1);
	}
  }

  if (strcmp(image_name,"-c")!=0 && (outfp=fopen(image_name,"w"))==NULL) {
    fprintf(stderr,"Could not open %s\n",image_name);
    exit(-1);
  }

  for (j=(int)strlen(image_name)-2; j>=0; j--)
    if (image_name[j]=='/' || image_name[j]=='\\')
	{
	  image_name += j+1;
	  break;
	}

  if (vh.gen.data_type != SHELL2) {
	fprintf(stderr, "Implemented for BS2 only.\n");
	exit(-1);
  }

  max_structs= vh.str.num_of_structures;

  if (cur_struct!= -1) {
    if (vh.str.num_of_structures<cur_struct  || cur_struct<=0) {
      printf("Invalid structure number specified (should be 1 - %d\n",vh.str.num_of_structures);
      fflush(stdout);
      exit(-1);
    }
    
    
    ReadStructure();
    write_stl();
    
  }
  else {
    for(cur_struct=1;cur_struct<=max_structs;cur_struct++) {
      if (vh.str.num_of_structures<cur_struct  || cur_struct<=0) {
	printf("Invalid structure number specified (should be 1 - %d\n",vh.str.num_of_structures);
	fflush(stdout);
	exit(-1);
      }

      ReadStructure();
      write_stl();
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


/*****************************************************************************
 * FUNCTION: write_stl
 * DESCRIPTION: Writes t-shell structure in STL format.
 * PARAMETERS: None
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: The global variables outfp, image_name, vh, TSE,
 *    number_of_triangles, triangle_table, triangle_edges, edge_vertices,
 *    T_x, T_y, T_z must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs, will exit.
 * HISTORY:
 *    Created: 10/6/05 by Dewey Odhner
 *    Modified: 2/20/07 prints count of triangles if outfp is NULL
 *       by Dewey Odhner.
 *
 *****************************************************************************/
void write_stl()
{
  double w[3], xyz[3][3];
  unsigned char **tse,*tse1;
  int i,j,k, i0, q, total_triangles;
  double voxsz[3], normal[3], g;
  float *domain;


  if (vh.str.domain_valid)
  {
    domain = vh.str.domain+12*(cur_struct-1);
  }
  else
  {
    domain = vh.str.domain = (float *)calloc(12, sizeof(float));
	domain[3] = domain[7] = domain[11] = 1;
  }
  if (outfp && fprintf(outfp, "solid %s\n", image_name) < 8) {
    fprintf(stderr, "Write failure.\n");
	exit(1);
  }
  voxsz[0] = vh.str.xysize[0];
  voxsz[1] = vh.str.xysize[1];
  voxsz[2] = vh.str.loc_of_samples[1]-vh.str.loc_of_samples[0];
  total_triangles = 0;
  for(tse=(unsigned char **)TSE,i=0; i<sl; i++)
    for(j=0;j<row;j++,tse++)
	  for(tse1= *tse; tse1<tse[1]; tse1+=3+3*number_of_triangles[*tse1]) {
        k = (int)(tse1[1])<<8|tse1[2];
		if (outfp == NULL)
		 total_triangles += number_of_triangles[*tse1];
		else
	     for (i0=0; i0<number_of_triangles[*tse1]; i0++) {

	      q = tse1[3+3*i0]>>5;
	      w[0] = .5+1./7*(q&4? q|~3: q&3);
	      q = tse1[3+3*i0]>>2;
	      w[1] = .5+1./7*(q&4? q|~3: q&3);
	      q = (tse1[3+3*i0]<<1) | ((tse1[4+3*i0]&1<<7)!=0);
	      w[2] = .5+1./7*(q&4? q|~3: q&3);
		  for (q=0; q<3; q++) {
		    xyz[q][0] = w[q]*T_x[edge_vertices[triangle_edges[
			            triangle_table[tse1[0]][i0]][q]][1]]+
					(1-w[q])*T_x[edge_vertices[triangle_edges[
					    triangle_table[tse1[0]][i0]][q]][0]];
		    xyz[q][0] *= voxsz[0];
			xyz[q][1] = w[q]*T_y[edge_vertices[triangle_edges[
			            triangle_table[tse1[0]][i0]][q]][1]]+
					(1-w[q])*T_y[edge_vertices[triangle_edges[
					    triangle_table[tse1[0]][i0]][q]][0]];
		    xyz[q][1] *= voxsz[1];
			xyz[q][2] = w[q]*T_z[edge_vertices[triangle_edges[
			            triangle_table[tse1[0]][i0]][q]][1]]+
					(1-w[q])*T_z[edge_vertices[triangle_edges[
					    triangle_table[tse1[0]][i0]][q]][0]];
		    xyz[q][2] *= voxsz[2];
		  }
		  BG_decode(normal[0], normal[1], normal[2],
		    ((int)tse1[4+3*i0]<<8&0x7fff)|tse1[5+3*i0]);
		  g = normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2];
		  if (g)
		    g = -1/sqrt(g);
		  for (q=0; q<3; q++)
		    normal[q] *= g;
		  if (fprintf(outfp, "facet normal %f %f %f\n outer loop\n",
		      normal[0]*domain[3]+normal[1]*domain[6]+normal[2]*domain[9],
			  normal[0]*domain[4]+normal[1]*domain[7]+normal[2]*domain[10],
			  normal[0]*domain[5]+normal[1]*domain[8]+normal[2]*domain[11]
			  ) < 36) {
			fprintf(stderr, "Write failure.\n");
			exit(1);
		  }
		  if (((xyz[1][1]-xyz[0][1])*(xyz[2][2]-xyz[1][2])-
		       (xyz[1][2]-xyz[0][2])*(xyz[2][1]-xyz[1][1]))*normal[0]+
			  ((xyz[1][2]-xyz[0][2])*(xyz[2][0]-xyz[1][0])-
			   (xyz[1][0]-xyz[0][0])*(xyz[2][2]-xyz[1][2]))*normal[1]+
			  ((xyz[1][0]-xyz[0][0])*(xyz[2][1]-xyz[1][1])-
		       (xyz[1][1]-xyz[0][1])*(xyz[2][0]-xyz[1][0]))*normal[2] < 0)
			for (q=2; q>=0; q--) {
			  if (fprintf(outfp, "    vertex %f %f %f\n",
				  domain[0]+((xyz[q][0]+k*voxsz[0]))*domain[3]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[6]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[9],
				  domain[1]+((xyz[q][0]+k*voxsz[0]))*domain[4]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[7]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[10],
				  domain[2]+((xyz[q][0]+k*voxsz[0]))*domain[5]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[8]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[11]
				  ) < 27) {
				fprintf(stderr, "Write failure.\n");
				exit(1);
			  }
			}
		  else
			for (q=0; q<3; q++) {
			  if (fprintf(outfp, "    vertex %f %f %f\n",
				  domain[0]+((xyz[q][0]+k*voxsz[0]))*domain[3]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[6]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[9],
				  domain[1]+((xyz[q][0]+k*voxsz[0]))*domain[4]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[7]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[10],
				  domain[2]+((xyz[q][0]+k*voxsz[0]))*domain[5]
				           +((xyz[q][1]+j*voxsz[1])+
						     vh.str.min_max_coordinates[1])*domain[8]
						   +((xyz[q][2]+vh.str.loc_of_samples[i])+
						     vh.str.min_max_coordinates[2])*domain[11]
				  ) < 27) {
				fprintf(stderr, "Write failure.\n");
				exit(1);
			  }
			}
		  if (fprintf(outfp, " endloop\nendfacet\n") < 18) {
		    fprintf(stderr, "Write failure.\n");
			exit(1);
		  }
         }
      }
  if (outfp == NULL)
   exit(printf("%d\n", total_triangles) < 2);
  else
   if (fprintf(outfp, "endsolid %s\n", image_name) < 11) {
    fprintf(stderr, "Write failure.\n");
	exit(1);
   }
}

      
int Inv3Mat(double in[3][3], double out[3][3])
{
  double t;

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

double Det3(double m[3][3])
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}
