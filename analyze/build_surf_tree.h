/*
  Copyright 1993-2011 Medical Image Processing Group
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

#ifndef  __build_surf_tree_h
#define  __build_surf_tree_h

#include  "KinematicsCommon.h"

typedef struct QLIST {
  Surf_Tree *pt;
  struct QLIST *next;
} QList;


extern int BuildSurfTree(Surf_Info *srf);
extern int GetBoxFromSurf(Surf_Info *s);
extern int get_token( FILE *file, char string[], int string_size);
extern int InitPlan(Surf_Info *s);
extern int init_defaults();
extern int write_defaults();

Surf_Tree *PopQ(QList **Q);
void PushQ(QList **Q, Surf_Tree *pt);
int GetTree(FILE *fp, Surf_Info *s);
int GetAxis(FILE *fp, Surf_Info *s);
int GetBBox(FILE *fp, Surf_Info *s);
int GetBoxFromPlan(Surf_Info *s );

void MultMat(KMMATRIX out, KMMATRIX mat1, KMMATRIX mat2);     
void MakeID(KMMATRIX m);
int GetPrevStructureIndex(Surf_Info *srf, short *index);
int FindPrevStructureIndex(Surf_Info *srf,short * index, char *ilock);
int GetNextStructureIndex(Surf_Info *srf, short *index);
int FindNextStructureIndex(Surf_Info *srf, short *index, char *ilock);
Surf_Tree *GetPt(Surf_Tree *tr,short *index, KMMATRIX M);
Surf_Tree *GetSurfPointer(Surf_Info *srf, short *ind, KMMATRIX M);
void BuildOrder(Surf_Tree *parent, Surf_Tree **tree, float * sorted_list, int dim, int st, int end, int level);
void FreeTree(Surf_Tree *t);     
void ReleaseAllStructs();
int GetNextToken(FILE *fp, char *str);
double Det4(KMMATRIX m);
double Det3(double m[3][3]);
int Inv4Mat(KMMATRIX inmat, KMMATRIX outmat);
int GetMatrix(FILE *fp, KMMATRIX M);
int GetPoint(FILE *fp, double p[3]);
void UpdateLimits(Surf_Info *s, KMMATRIX D, KMPOINT in);
void TransformPoint(KMPOINT KmNew, KMMATRIX M, KMPOINT old);


#endif

/////////////////////////////////
