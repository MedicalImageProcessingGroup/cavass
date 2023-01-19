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

#ifndef  __aoto_register_h
#define  __aoto_register_h

#include "build_surf_tree.h"


typedef double KMPOINT[3];

typedef struct Reg_Seq {
  int obj_index;
  KMPOINT cent,axis[3], mu3;
  KMPOINT inst_axis,trans;
  double rot,eval[3];
  KMMATRIX M;
} REG_SEQ;

extern int Register_Intra(Surf_Info *InSurf, wxArrayString*  pstrOutPut, int *nInstancesNum);
extern int Register_Inter(Surf_Info *InSurf, wxArrayString*  pstrOutPut, int *nInstancesNum, int num_surf);
extern int SaveInterPlan(Surf_Info *InSurf, char *outfile, int timeInstances, int nRefFrame);
extern int SaveIntraPlan(Surf_Info *InSurf, char *outfile, int timeInstances, int nRefFrame );
extern void KinematicsInterRelease();
extern void KinematicsIntraRelease();

int CrossP(double vec1[], double vec2[], double res[] );
int GtEvec(  double mat[3][3], double eval[3], double evec[3][3] );
void GtEval( double mat[3][3], double eval[3] );
void SortEval(double e[3] );
void DoTransform(KMMATRIX mtNew, KMMATRIX prev, KMPOINT cent, KMPOINT axis,double angle, KMPOINT translate);       
void GetRotationAndTranslation();
void MakeAxisConsistant();
void MakeAxisConsistent(KMPOINT axis2[3], KMPOINT axis1[3], KMPOINT mu3_2, KMPOINT mu3_1 );
int get_isoshape_center(KMPOINT cent, char *description, int st, KMMATRIX M);
void InitInstance(REG_SEQ *inst, int sf, int st, KMMATRIX M);
int Load_Data(int sf, int st);
int ReleaseData(int sf,int st);
void Inv3Mat(  double in[3][3],double out[3][3] );

void DisplayListing(Surf_Info *s, int obj,int inst, wxArrayString*  pstrOutPut );
void DisplayInterListing(Surf_Info *s, int obj, int obj_real, int inst, wxArrayString*  pstrOutPut );

void ReleaseOutSequence();
int ComputeOutSequence();
void TransformVector(KMPOINT kmNew, KMMATRIX M, KMPOINT old);
void PrintLevel(int level, Surf_Tree *ptr, FILE *fp);
void print4mat(KMMATRIX M,FILE *fp);
double angle(double A[3], double C[3], double B[3] );
void Compute_Relative(int ob,REG_SEQ *rel_seq);
void Print_Txt(FILE *fp, int obj, REG_SEQ *rel_seq );

int StaticAnalyze(Surf_Info *InSurf, char DisplayString[NUM_FEATURE_LINES][MSG_WIDTH]);
int load_static_data();
int DisplaySurfaceStatistics(int obj_num, char DisplayStr[NUM_FEATURE_LINES][MSG_WIDTH]);
int GetSurfaceStatistics(char msg[NUM_FEATURE_LINES][MSG_WIDTH], int obj_num);
double get_axis_length_static(REG_SEQ * inst, Surf_Struct * str, int axis);
void output_stats(char* file_name);

#endif

////////////////////////////
