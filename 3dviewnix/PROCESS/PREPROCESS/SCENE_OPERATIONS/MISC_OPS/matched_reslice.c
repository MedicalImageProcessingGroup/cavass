/*
  Copyright 1993-2014, 2018 Medical Image Processing Group
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

#include <cv3dv.h>


typedef double MATRIX[4][4];
typedef double POINT[4];

void TransformPoint(POINT out, MATRIX M, POINT in);
void MultMat(MATRIX out, MATRIX mat1, MATRIX mat2);

/*
 *  Created: by Supun Samarasekera
 *  Modified: 9/22/94 to use registration matrix by Dewey Odhner
 *  Modified 6/2/99 to do stretching by Dewey Odhner.
 *  Modified: 9/30/99 to correct for slice location by Dewey Odhner
 *  Modified: 10/2/03 to pass -n flag to reslice_proc by Dewey Odhner
 *  Modified: 5/15/18 to handle single slice scenes by Dewey Odhner
 */
int main(argc,argv)
int argc;
char *argv[];
{
  int i, j, error, psargc, nstretchpoints=0, interp_flag=1;
  double Pz, vtmp[3], (*stretch_point)[2][3];
  FILE *in,*match;
  ViewnixHeader in_vh,match_vh;
  POINT p1[4],p2[4],p3[4];
  MATRIX MAT1,MAT2,TRANS, Mtmp;
  char grp[6],elem[6], *cmd;


  switch (argc) {
   case 4:
   case 16:
    psargc = argc;
    break;
   default:
    if (argc>4 && argc%6==5)
	{
	  if (strcmp(argv[4], "-l")==0 || strcmp(argv[4], "-n")==0)
	  {
	    nstretchpoints = (argc-5)/6;
		psargc = 4;
		interp_flag = strcmp(argv[4], "-n");
		break;
	  }
	  else if (argc>16 && (strcmp(argv[16], "-l")==0 ||
	      strcmp(argv[16], "-n")==0))
	  {
	    nstretchpoints = (argc-17)/6;
		psargc = 16;
		interp_flag = strcmp(argv[16], "-n");
	    break;
	  }
	}
    printf(
	  "Usage: %s <file_to_reslice> <file_to_match> <output_file> [matrix] [ [-l|-n] [landmark new_loc] ... ]\n",
	  argv[0]);
    printf("matrix: 4x3 rigid transformation from scanner coordinate system of <file_to_reslice> to <file_to_match>\n");
	printf("-l      - Use linear interpolation (default)\n");
	printf("-n      - Use nearest neighbor interpolation\n");
	printf("landmark- scanner coordinates of landmark in input scene to reslice\n");
	printf("new_loc - new location of landmark in scanner coordinate system\n");
	exit(-1);
  }

  if (nstretchpoints)
  {
    stretch_point = (void *)malloc(nstretchpoints*sizeof(*stretch_point));
	if (stretch_point == NULL)
	{
	  fprintf(stderr, "Out of memory.\n");
      exit(-1);
	}
	for (j=psargc+1; j<argc; j+=6)
	{
	  if (sscanf(argv[j], "%lf", stretch_point[(j-psargc)/6][0])!=1 ||
	      sscanf(argv[j+1], "%lf", stretch_point[(j-psargc)/6][0]+1)!=1 ||
	      sscanf(argv[j+2], "%lf", stretch_point[(j-psargc)/6][0]+2)!=1 ||
	      sscanf(argv[j+3], "%lf", stretch_point[(j-psargc)/6][1])!=1 ||
	      sscanf(argv[j+4], "%lf", stretch_point[(j-psargc)/6][1]+1)!=1 ||
	      sscanf(argv[j+5], "%lf", stretch_point[(j-psargc)/6][1]+2)!=1)
	  {
	    fprintf(stderr, "Bad coordinate.\n");
        exit(-1);
	  }
	}
  }

  in=fopen(argv[1],"r");
  if (in==NULL) {
    printf("Could not open the file to be resliced\n");
    exit(-1);
  }

  match=fopen(argv[2],"r");
  if (match==NULL) {
    printf("Could not open the file to be matched\n");
    exit(-1);
  }

  error=VReadHeader(in,&in_vh,grp,elem);
  if (error<=104) {
    printf("Error in reading input header\n");
    exit(-1);
  }


  error=VReadHeader(match,&match_vh,grp,elem);
  if (error<=104) {
    printf("Error in reading input header\n");
    exit(-1);
  }

  if (match_vh.gen.data_type!=IMAGE0 || in_vh.gen.data_type!=IMAGE0) {
    printf("Both input files should be IMAGE0 files\n");
    exit(-1);
  }

  if (match_vh.scn.dimension!=3 || in_vh.scn.dimension!=3) {
    printf("Both input files should be 3D\n");
    exit(-1);
  }

  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
	  if (4*i+j+4 < psargc) {
	    if (sscanf(argv[j==3? 4*i+7: 4*j+i+4], "%lf", TRANS[i]+j) != 1) {
          printf("Error in reading matrix\n");
          exit(-1);
		}
	  }
	  else
	    TRANS[i][j] = i==j;

  for (j=0; j<3; j++)
    vtmp[j] =
      TRANS[j][0]* -TRANS[0][3]+
      TRANS[j][1]* -TRANS[1][3]+
      TRANS[j][2]* -TRANS[2][3];
  for (j=0; j<3; j++)
    TRANS[j][3] = vtmp[j];
  
  MAT1[0][0]= match_vh.scn.domain[3];
  MAT1[1][0]= match_vh.scn.domain[4];
  MAT1[2][0]= match_vh.scn.domain[5];

  MAT1[0][1]= match_vh.scn.domain[6];
  MAT1[1][1]= match_vh.scn.domain[7];
  MAT1[2][1]= match_vh.scn.domain[8];

  MAT1[0][2]= match_vh.scn.domain[9];
  MAT1[1][2]= match_vh.scn.domain[10];
  MAT1[2][2]= match_vh.scn.domain[11];

  MAT1[0][3]= match_vh.scn.domain[0];
  MAT1[1][3]= match_vh.scn.domain[1];
  MAT1[2][3]= match_vh.scn.domain[2];
  MAT1[3][0]=MAT1[3][1]=MAT1[3][2]=0.0; MAT1[3][3]=1.0;

  MAT2[0][0]= in_vh.scn.domain[3];
  MAT2[0][1]= in_vh.scn.domain[4];
  MAT2[0][2]= in_vh.scn.domain[5];

  MAT2[1][0]= in_vh.scn.domain[6];
  MAT2[1][1]= in_vh.scn.domain[7];
  MAT2[1][2]= in_vh.scn.domain[8];

  MAT2[2][0]= in_vh.scn.domain[9];
  MAT2[2][1]= in_vh.scn.domain[10];
  MAT2[2][2]= in_vh.scn.domain[11];


  MAT2[0][3]= 
    MAT2[0][0]* -in_vh.scn.domain[0] +
    MAT2[0][1]* -in_vh.scn.domain[1] +
    MAT2[0][2]* -in_vh.scn.domain[2] ;
  MAT2[1][3]= 
    MAT2[1][0]* -in_vh.scn.domain[0] +
    MAT2[1][1]* -in_vh.scn.domain[1] +
    MAT2[1][2]* -in_vh.scn.domain[2] ;
  MAT2[2][3]= 
    MAT2[2][0]* -in_vh.scn.domain[0] +
    MAT2[2][1]* -in_vh.scn.domain[1] +
    MAT2[2][2]* -in_vh.scn.domain[2] ;
    

  MAT2[3][0]=MAT2[3][1]=MAT2[3][2]=0.0; MAT2[3][3]=1.0;


  MultMat(Mtmp,TRANS,MAT1);

  Pz = in_vh.scn.num_of_subscenes[0]>1?
    in_vh.scn.loc_of_subscenes[1]-in_vh.scn.loc_of_subscenes[0]:
    in_vh.gen.slice_thickness_valid? in_vh.gen.slice_thickness:
	match_vh.scn.num_of_subscenes[0]>1?
    match_vh.scn.loc_of_subscenes[1]-in_vh.scn.loc_of_subscenes[0]:
    match_vh.gen.slice_thickness_valid? match_vh.gen.slice_thickness: 1;
  for (j=0; j<nstretchpoints; j++)
  {
    p1[0][0] = stretch_point[j][0][0];
    p1[0][1] = stretch_point[j][0][1];
    p1[0][2] = stretch_point[j][0][2];
	TransformPoint(stretch_point[j][0], MAT2, p1[0]);
	TransformPoint(p1[0], TRANS, stretch_point[j][1]);
	TransformPoint(stretch_point[j][1], MAT2, p1[0]);
	stretch_point[j][0][0] /= in_vh.scn.xypixsz[0];
	stretch_point[j][0][1] /= in_vh.scn.xypixsz[1];
	stretch_point[j][0][2] =
	  (stretch_point[j][0][2]-in_vh.scn.loc_of_subscenes[0])/Pz;
	stretch_point[j][1][0] =
	  stretch_point[j][1][0]/in_vh.scn.xypixsz[0]-stretch_point[j][0][0];
	stretch_point[j][1][1] =
	  stretch_point[j][1][1]/in_vh.scn.xypixsz[1]-stretch_point[j][0][1];
	stretch_point[j][1][2] = (stretch_point[j][1][2]-
	  in_vh.scn.loc_of_subscenes[0])/Pz-stretch_point[j][0][2];
  }

  MultMat(TRANS,MAT2,Mtmp);

  p1[0][0]=0.0;  
  p1[0][1]=0.0;  
  p1[0][2]=match_vh.scn.loc_of_subscenes[0];
  
  p1[1][0]=(match_vh.scn.xysize[0]-1)*match_vh.scn.xypixsz[0];
  p1[1][1]=0.0;  
  p1[1][2]=match_vh.scn.loc_of_subscenes[0];
  
  p1[2][0]=0.0;
  p1[2][1]=(match_vh.scn.xysize[1]-1)*match_vh.scn.xypixsz[1];
  p1[2][2]=match_vh.scn.loc_of_subscenes[0];
  
  p1[3][0]=0.0;  
  p1[3][1]=0.0;  
  p1[3][2]=match_vh.scn.loc_of_subscenes[match_vh.scn.num_of_subscenes[0]-1];
  
  for(i=0;i<4;i++) {
    TransformPoint(p2[i],TRANS,p1[i]);
    p3[i][0]= p2[i][0]/in_vh.scn.xypixsz[0];
    p3[i][1]= p2[i][1]/in_vh.scn.xypixsz[1];
    p3[i][2]= (p2[i][2]-in_vh.scn.loc_of_subscenes[0])/Pz;
  }

  cmd = (char *)malloc(542+strlen(argv[1])+strlen(argv[3])+126*nstretchpoints);
  if (cmd == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
    exit(-1);
  }
  if (nstretchpoints)
  {
    sprintf(cmd,
     "stretch %s %s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %dx%d %d 0"
	  , argv[1],argv[3],
	  p3[0][0],p3[0][1],p3[0][2],
	  p3[1][0],p3[1][1],p3[1][2],
	  p3[2][0],p3[2][1],p3[2][2],
	  p3[3][0],p3[3][1],p3[3][2],
	  match_vh.scn.xysize[0], match_vh.scn.xysize[1],
	  match_vh.scn.num_of_subscenes[0]);
  }
  else
    sprintf(cmd,
"reslice_proc %s %s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %dx%d %d 0"
	 , argv[1],argv[3],
	 p3[0][0],p3[0][1],p3[0][2],
	 p3[1][0],p3[1][1],p3[1][2],
	 p3[2][0],p3[2][1],p3[2][2],
	 p3[3][0],p3[3][1],p3[3][2],
	 match_vh.scn.xysize[0], match_vh.scn.xysize[1],
	 match_vh.scn.num_of_subscenes[0]);
  if (psargc == 16)
    sprintf(cmd+strlen(cmd), " %f %f %f %f %f %f %f %f %f %f %f %f",
	  match_vh.scn.domain[0]+
	  match_vh.scn.domain[9]*match_vh.scn.loc_of_subscenes[0],
	  match_vh.scn.domain[1]+
	  match_vh.scn.domain[10]*match_vh.scn.loc_of_subscenes[0],
	  match_vh.scn.domain[2]+
	  match_vh.scn.domain[11]*match_vh.scn.loc_of_subscenes[0],
	  match_vh.scn.domain[3],
	  match_vh.scn.domain[4],
	  match_vh.scn.domain[5],
	  match_vh.scn.domain[6],
	  match_vh.scn.domain[7],
	  match_vh.scn.domain[8],
	  match_vh.scn.domain[9],
	  match_vh.scn.domain[10],
	  match_vh.scn.domain[11]);
  if (nstretchpoints)
    strcat(cmd, " -l");
  for (j=0; j<nstretchpoints; j++)
    sprintf(cmd+strlen(cmd), " %f %f %f %f %f %f",
	  stretch_point[j][0][0], stretch_point[j][0][1], stretch_point[j][0][2],
	  stretch_point[j][1][0], stretch_point[j][1][1], stretch_point[j][1][2]);
  if (!interp_flag)
    strcat(cmd, " -n");
  error=system(cmd);
  if (error) {
    printf("Could not execute the reslice process");
    exit(-1);
  }

  exit(0);
	 

}







void MultMat(MATRIX out, MATRIX mat1, MATRIX mat2)
{
  
  int i,j;
  
  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
      out[i][j]= mat1[i][0]*mat2[0][j] + mat1[i][1]*mat2[1][j]+
        mat1[i][2]*mat2[2][j]+mat1[i][3]*mat2[3][j];
}



 
void TransformPoint(POINT out, MATRIX M, POINT in)
{
 
  out[0]=M[0][0]*in[0]+M[0][1]*in[1]+M[0][2]*in[2]+M[0][3];
  out[1]=M[1][0]*in[0]+M[1][1]*in[1]+M[1][2]*in[2]+M[1][3];
  out[2]=M[2][0]*in[0]+M[2][1]*in[1]+M[2][2]*in[2]+M[2][3];
 
}
