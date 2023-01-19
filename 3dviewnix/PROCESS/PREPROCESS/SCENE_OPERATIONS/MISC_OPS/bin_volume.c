/*
  Copyright 1993-2016 Medical Image Processing Group
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
#include <cv3dv.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971694
#endif

int get_slices(int dim, short *list), bin_to_grey(unsigned char *bin_buffer,
    int length, unsigned char *grey_buffer, int min_value, int max_value);
void GtEval( double mat[3][3], double eval[3] );
int GtEvec(double mat[3][3], double eval[3], double evec[3][3]);
int CrossP(double vec1[3], double vec2[3], double res[3]);
void SortEval(double e[3] );

int main(argc,argv)
int argc;
char *argv[];
{

  double count=0;
  int bad_evec=0, slice_counts=0, threshold, find_thresh=FALSE, testbit;
  int i,j, size,size1,error,slices1,bytes, find_loc, rel_loc=0, find_bbox=0;
  ViewnixHeader vh1;
  FILE *in1, *outstream;
  unsigned char *data1, *d1_8;
  unsigned short *d1_16;
  char group[6],elem[6];
  double loc_sum[3];
  double pixel_volume;
  int bounds[6], best_thresh=65536;
  double root_lambda_1, eval[3], evec[3][3], ref_loc[3], ref_vol,
    root_lambda_sum, scale_measure, best_scale_measure=0;

  if (argc>2 && strcmp(argv[argc-2], "-o")==0)
  {
    outstream = fopen(argv[argc-1], "wb");
    argc -= 2;
  }
  else
    outstream = stdout;
  if (argc>2 && strcmp(argv[argc-1], "-s")==0)
  {
    slice_counts = 1;
	argc--;
  }
  if (argc>2 && strcmp(argv[argc-1], "-b")==0)
  {
    find_bbox = 1;
	argc--;
  }
  if (argc>6 && strcmp(argv[argc-5], "-r")==0)
  {
    rel_loc = find_loc = TRUE;
	argc -= 5;
	if (sscanf(argv[argc+1], "%lf", ref_loc)!=1 ||
	    sscanf(argv[argc+2], "%lf", ref_loc+1)!=1 ||
		sscanf(argv[argc+3], "%lf", ref_loc+2)!=1 ||
		sscanf(argv[argc+4], "%lf", &ref_vol)!=1)
	{
	  fprintf(stderr, "Bad numbers.\n");
	  exit(-1);
	}
  }

  if (argc!= 2 && (argc!=3 || strncmp(argv[2], "-l", 2))) {
    fprintf(stderr,
	  "Usage: bin_volume <BIM_file> [-l] [-r <x> <y> <z> <v>] [-b]\n");
    exit(-1);
  }
  find_loc = argc>2 || rel_loc;
  if (argc>2 && strlen(argv[2])>2 &&
      sscanf(argv[2]+2, "%d=%lf", &find_loc, &scale_measure) == 2)
  {
	find_thresh = TRUE;
	threshold = 32767;
  }
  memset(loc_sum, 0, sizeof(loc_sum));

  in1=fopen(argv[1],"rb");
  if (in1==NULL) {
    fprintf(stderr, "Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    fprintf(stderr, "Fatal error in reading header\n");
    exit(-1);
  }
  
  
  if (vh1.gen.data_type!=IMAGE0) {
    fprintf(stderr, "Input file should be IMAGE0\n");
    exit(-1);
  }
  if (vh1.scn.num_of_bits!=1 && !find_loc) {
    fprintf(stderr, "The Mask file should be binary\n");
    exit(-1);
  }

  slices1= get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);


  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  bytes = vh1.scn.num_of_bits==1? 1: vh1.scn.num_of_bits/8;
  pixel_volume= vh1.scn.xypixsz[0]*vh1.scn.xypixsz[1]*
    (vh1.scn.loc_of_subscenes[1]-vh1.scn.loc_of_subscenes[0]);

  size1 = (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  d1_8=(unsigned char *)(vh1.scn.num_of_bits==1? malloc(size): data1);
  d1_16 = (unsigned short *)d1_8;

  for (testbit=find_thresh?16384:1; testbit; testbit>>=1)
  {
   VSeekData(in1,0);

   bounds[0] = vh1.scn.xysize[0];
   bounds[2] = vh1.scn.xysize[1];
   bounds[4] = slices1;
   bounds[1] = bounds[3] = bounds[5] = 0;
   count=0;
   for(i=0;i<slices1;i++) {
    int x, y, c;
	double sc=0;
#ifdef VERBOSE
    fprintf(stderr, "Processing slice %d\r",i);
#endif
    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
	if (vh1.scn.num_of_bits == 1)
    {
	  count += c=bin_to_grey(data1,size,d1_8,0,1);
      if (find_loc || find_bbox)
      {
        for (y=0; y<vh1.scn.xysize[1]; y++)
        {
          int rc=0;
          for (x=0; x<vh1.scn.xysize[0]; x++)
            if (d1_8[y*vh1.scn.xysize[0]+x])
            {
              rc++;
              loc_sum[0] += x;
              if (x < bounds[0])
                bounds[0] = x;
              if (x > bounds[1])
                bounds[1] = x;
            }
          loc_sum[1] += rc*y;
          if (rc)
          {
            if (y < bounds[2])
              bounds[2] = y;
            if (y >bounds[3])
              bounds[3] = y;
          }
        }
        loc_sum[2] += c*vh1.scn.loc_of_subscenes[i];
        if (c)
        {
          if (i < bounds[4])
            bounds[4] = i;
          if (i >bounds[5])
            bounds[5] = i;
        }
      }
	  if (slice_counts)
	    fprintf(outstream, "%d\n", c);
    }
	else // gray scene
	{
	  for (y=0; y<vh1.scn.xysize[1]; y++)
	  {
        double rc=0;
		int pc;
        for (x=0; x<vh1.scn.xysize[0]; x++)
        {
		  pc = bytes>1? d1_16[y*vh1.scn.xysize[0]+x]:
		                 d1_8[y*vh1.scn.xysize[0]+x];
		  if (find_thresh)
		    pc = pc>=threshold;
		  if (pc)
          {
            rc += pc;
            loc_sum[0] += x*pc;
            if (x < bounds[0])
              bounds[0] = x;
            if (x > bounds[1])
              bounds[1] = x;
          }
        }
        if (rc)
        {
		  assert(rc*y >= 0);
          loc_sum[1] += rc*y;
		  sc += rc;
          if (y < bounds[2])
            bounds[2] = y;
          if (y >bounds[3])
            bounds[3] = y;
        }
	  }
      if (sc)
      {
        loc_sum[2] += sc*vh1.scn.loc_of_subscenes[i];
		count += sc;
        if (i < bounds[4])
          bounds[4] = i;
        if (i >bounds[5])
          bounds[5] = i;
      }
	}
	if (slice_counts && vh1.scn.num_of_bits>1)
	  fprintf(outstream, "%.0f\n", sc);
   }

   if (!rel_loc && !find_thresh)
   {
    fprintf(outstream, "pixel volume %f\n",pixel_volume);
    fprintf(outstream, "Total number of pixels  -> %.0f\n",count);
    fprintf(outstream, "Volume                  -> %f\n",
	   count*pixel_volume);
   }
   if (find_loc)
   {
    double loc[3], scene_center[3];
	loc[0] = loc_sum[0]*vh1.scn.xypixsz[0]/count;
	loc[1] = loc_sum[1]*vh1.scn.xypixsz[1]/count;
	loc[2] = loc_sum[2]/count;
	scene_center[0] = vh1.scn.xypixsz[0]*(bounds[0]+bounds[1])/2;
	scene_center[1] = vh1.scn.xypixsz[1]*(bounds[2]+bounds[3])/2;
	scene_center[2] = (vh1.scn.loc_of_subscenes[bounds[4]]+
	  vh1.scn.loc_of_subscenes[bounds[5]])/2;
	// do PCA
	{
	  double m[3][3], tpt[3];
	  int x, y, ii, jj;
	  memset(m, 0, sizeof(m));
	  VSeekData(in1,0);
	  for (i=0; i<slices1; i++)
	  {
	    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
		  fprintf(stderr, "Could not read data\n");
		  exit(-1);
		}
        if (vh1.scn.num_of_bits == 1)
		  bin_to_grey(data1,size,d1_8,0,1);
        tpt[2] = vh1.scn.loc_of_subscenes[i]-loc[2];
        for (y=0; y<vh1.scn.xysize[1]; y++)
        {
          tpt[1] = y*vh1.scn.xypixsz[1]-loc[1];
          for (x=0; x<vh1.scn.xysize[0]; x++)
          {
		    int pc = bytes>1? d1_16[y*vh1.scn.xysize[0]+x]:
			                   d1_8[y*vh1.scn.xysize[0]+x];
		    if (find_thresh)
		      pc = pc>=threshold;
			if (pc)
            {
              tpt[0] = x*vh1.scn.xypixsz[0]-loc[0];
              for (ii=0; ii<3; ii++)
                for (jj=0; jj<3; jj++)
                  m[ii][jj] += pc*tpt[ii]*tpt[jj];
            }
		  }
        }
	  }
	  for (ii=0; ii<3; ii++)
	    for (jj=0; jj<3; jj++)
		  m[ii][jj] /= count;
	  GtEval(m,eval);
	  SortEval(eval);
	  if (GtEvec(m,eval,evec))
	    bad_evec = 1;;
	  root_lambda_1 = sqrt(eval[0]);
	  root_lambda_sum = sqrt(eval[0]+eval[1]+eval[2]);
	}
	if (vh1.scn.domain_valid)
	{
	  memcpy(loc_sum, loc, sizeof(loc));

	  memcpy(loc, scene_center, sizeof(loc));
	  scene_center[0] = vh1.scn.domain[3]*loc[0]+vh1.scn.domain[6]*loc[1]+
	    vh1.scn.domain[9]*loc[2]+vh1.scn.domain[0];
	  scene_center[1] = vh1.scn.domain[4]*loc[0]+vh1.scn.domain[7]*loc[1]+
	    vh1.scn.domain[10]*loc[2]+vh1.scn.domain[1];
      scene_center[2] = vh1.scn.domain[5]*loc[0]+vh1.scn.domain[8]*loc[1]+
	    vh1.scn.domain[11]*loc[2]+vh1.scn.domain[2];

	  loc[0] = vh1.scn.domain[3]*loc_sum[0]+vh1.scn.domain[6]*loc_sum[1]+
	    vh1.scn.domain[9]*loc_sum[2]+vh1.scn.domain[0];
	  loc[1] = vh1.scn.domain[4]*loc_sum[0]+vh1.scn.domain[7]*loc_sum[1]+
	    vh1.scn.domain[10]*loc_sum[2]+vh1.scn.domain[1];
	  loc[2] = vh1.scn.domain[5]*loc_sum[0]+vh1.scn.domain[8]*loc_sum[1]+
	    vh1.scn.domain[11]*loc_sum[2]+vh1.scn.domain[2];

      for (j=0; j<3; j++)
	  {
	    memcpy(loc_sum, evec[j], sizeof(loc));
		evec[j][0] = vh1.scn.domain[3]*loc_sum[0]+vh1.scn.domain[6]*loc_sum[1]+
	      vh1.scn.domain[9]*loc_sum[2];
	    evec[j][1] = vh1.scn.domain[4]*loc_sum[0]+vh1.scn.domain[7]*loc_sum[1]+
	      vh1.scn.domain[10]*loc_sum[2];
	    evec[j][2] = vh1.scn.domain[5]*loc_sum[0]+vh1.scn.domain[8]*loc_sum[1]+
	      vh1.scn.domain[11]*loc_sum[2];
	  }
	}
	if (rel_loc)
	  fprintf(outstream, "%f %f %f %f\n",
	    loc[0]-ref_loc[0], loc[1]-ref_loc[1], loc[2]-ref_loc[2],
	    (find_loc==1? pow(count*pixel_volume, 1/3.):
	     find_loc==2? sqrt(
	      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])*
		  vh1.scn.xypixsz[0]*vh1.scn.xypixsz[0]+
		  (bounds[3]-bounds[2])*(bounds[3]-bounds[2])*
		  vh1.scn.xypixsz[1]*vh1.scn.xypixsz[1]+
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])*
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])):
         find_loc==3? root_lambda_1:
		 root_lambda_sum)/ref_vol);
	else if (find_thresh)
	{
	  double test_scale_measure=find_loc==1? pow(count*pixel_volume, 1/3.):
	    find_loc==2? sqrt(
	      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])*
		  vh1.scn.xypixsz[0]*vh1.scn.xypixsz[0]+
		  (bounds[3]-bounds[2])*(bounds[3]-bounds[2])*
		  vh1.scn.xypixsz[1]*vh1.scn.xypixsz[1]+
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])*
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])):
        find_loc==3? root_lambda_1:
		root_lambda_sum;
	  if (fabs(test_scale_measure-scale_measure) <
		  fabs(best_scale_measure-scale_measure))
	  {
		best_thresh = threshold;
		best_scale_measure = test_scale_measure;
	  }
	  if (test_scale_measure > scale_measure)
		threshold += testbit;
	  else
		threshold -= testbit;
	}
	else
	{
      fprintf(outstream, "Center                  -> (%f, %f, %f)\n",loc[0],loc[1],loc[2]);
	  fprintf(outstream, "Scale Measure           -> %f\n",
	    find_loc==1? pow(count*pixel_volume, 1/3.):
	    find_loc==2? sqrt(
	      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])*
		  vh1.scn.xypixsz[0]*vh1.scn.xypixsz[0]+
		  (bounds[3]-bounds[2])*(bounds[3]-bounds[2])*
		  vh1.scn.xypixsz[1]*vh1.scn.xypixsz[1]+
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])*
		  (vh1.scn.loc_of_subscenes[bounds[5]]-
		  vh1.scn.loc_of_subscenes[bounds[4]])):
        find_loc==3? root_lambda_1:
		root_lambda_sum);
      fprintf(outstream, "Eigenvalues             -> {%f, %f, %f}\n",
        eval[0], eval[1], eval[2]);
      fprintf(outstream, "Eigenvector 1           -> (%f, %f, %f)\n",
	    evec[0][0], evec[0][1], evec[0][2]);
      fprintf(outstream, "Eigenvector 2           -> (%f, %f, %f)\n",
	    evec[1][0], evec[1][1], evec[1][2]);
      fprintf(outstream, "Eigenvector 3           -> (%f, %f, %f)\n",
	    evec[2][0], evec[2][1], evec[2][2]);
	  fprintf(outstream, "Bounding Box Center     -> (%f, %f, %f)\n",
	    scene_center[0], scene_center[1], scene_center[2]);
	}
   }
  }
  if (find_thresh)
   fprintf(outstream, "Best threshold             -> %d\n", best_thresh);
  if (find_bbox)
  {
    float fb[6]; // Use scanner origin, but scene orientation.

	fb[0] = vh1.scn.xypixsz[0]*bounds[0];
	fb[1] = vh1.scn.xypixsz[0]*bounds[1];
	fb[2] = vh1.scn.xypixsz[1]*bounds[2];
	fb[3] = vh1.scn.xypixsz[1]*bounds[3];
	fb[4] = vh1.scn.loc_of_subscenes[bounds[4]];
	fb[5] = vh1.scn.loc_of_subscenes[bounds[5]];
	if (vh1.scn.domain_valid)
	  for (j=0; j<3; j++)
	  {
	    fb[2*j] += vh1.scn.domain[0]*vh1.scn.domain[3+3*j]+
		           vh1.scn.domain[1]*vh1.scn.domain[4+3*j]+
				   vh1.scn.domain[2]*vh1.scn.domain[5+3*j];
	    fb[2*j+1]+=vh1.scn.domain[0]*vh1.scn.domain[3+3*j]+
		           vh1.scn.domain[1]*vh1.scn.domain[4+3*j]+
				   vh1.scn.domain[2]*vh1.scn.domain[5+3*j];
	  }
	fprintf(outstream, "Bounding Box            -> [%f, %f] x [%f, %f] x [%f, %f]\n",
	  fb[0], fb[1], fb[2], fb[3], fb[4], fb[5]);
  }

  fclose(in1);
  exit(bad_evec);
}



int get_slices(int dim, short *list)
{
  int i,sm;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sm=0,i=0;i<list[0];i++)
      sm+= list[1+i];
    return(sm);
  }
  return(0);

}

int bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;
  int c;

  bin = bin_buffer;
  grey = grey_buffer;
  
  c=0;
  for(j=7,i=length; i>0; i--)    {
    if( (*bin & mask[j]) != 0)  {
      *grey = max_value;
      c++;
    }
    else 
      *grey = min_value;
    
    grey++;
    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
    
  }

  return c;

}

/************************************************************************
 *
 *      FUNCTION        : Det3
 *
 *      DESCRIPTION     : returns the determinant of a 3x3 martrix.
 *
 *      RETURN VALUE    : determinant of the matrix.
 *
 *      PARAMETERS      : m - input matrix
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : m should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
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

void SortEval(double e[3] )
{
  int i,j;
  double temp;

  for(i=0;i<2;i++)
    for(j=i+1;j<3;j++)
      if (e[i]<e[j])
	  {
	temp=e[j]; e[j]=e[i]; e[i]=temp;
      }
}

void GtEval( double mat[3][3],double eval[3] )
{       
  double  p,q,r,a,b,m,theta,delta;
  
  p = -mat[0][0]-mat[1][1]-mat[2][2];
  q  = -mat[0][1]*mat[1][0]-mat[0][2]*mat[2][0]-mat[1][2]*mat[2][1]
                +mat[0][0]*mat[1][1]+mat[0][0]*mat[2][2]+mat[1][1]*mat[2][2];
  r = -Det3(mat);

  a= q - p*p/3.0; b= p*p*p/13.5 - p*q/3.0 + r;
  m= 2.0*sqrt(a/-3.0);
  delta=b*b/4.0+a*a*a/27.0;
  if (delta >0.0 || a*m==0)
  {
	theta = M_PI/3.0;
	m = fabs(b);
  }
  else
    theta=(acos((3.0*b)/(a*m)))/3.0;
  eval[0]= p/-3.0 + m*cos(theta);
  eval[1]= p/-3.0 + m*cos(theta+ 2.0*M_PI/3.0);
  eval[2]= p/-3.0 + m*cos(theta+ 4.0*M_PI/3.0);

} 

/*------       Computation of the Eigenvectors of a 3 by 3 matrix       ------*/
int GtEvec(double mat[3][3], double eval[3], double evec[3][3])
{ 
  int     i,j, vn[3], cm1, cm2;
  double  vec0[3],vec1[3],vec2[3],outvec[3], mm[3];

  if (eval[0] == eval[2])
  {
    for (j=0; j<3; j++)
      for (i=0; i<3; i++)
        evec[j][i] = i==j;
    return(0);
  }

  if (eval[1]-eval[2] > eval[0]-eval[1])
  {
    vn[0] = 2;
	vn[1] = 0;
	vn[2] = 1;
  }
  else
  {
    vn[0] = 0;
	vn[1] = 1;
	vn[2] = 2;
  }
  for (i=0; i<3; i++)
  {
    vec0[i] = (i==0) ? mat[0][i]-eval[vn[0]] : mat[0][i];
    vec1[i] = (i==1) ? mat[1][i]-eval[vn[0]] : mat[1][i];
	vec2[i] = (i==2) ? mat[2][i]-eval[vn[0]] : mat[2][i];
  }
  mm[0] = vec0[0]*vec0[0]+vec0[1]*vec0[1]+vec0[2]*vec0[2];
  mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
  mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
  cm1 = mm[0]<mm[1]? 0:1;
  if (mm[2] < mm[cm1])
    cm1 = 2;
  switch (cm1)
  {
    case 0:
	  break;
	case 1:
	  memcpy(vec1, vec0, sizeof(vec0));
	  mm[1] = mm[0];
	  break;
	case 2:
	  memcpy(vec2, vec0, sizeof(vec0));
	  mm[2] = mm[0];
	  break;
  }
  if (CrossP(vec1, vec2, outvec) != 0) {
    fprintf(stderr, "Cannot compute Eigenvectors\n");
	return(-1);
  }
  for (i=0; i<3; i++)
    evec[vn[0]][i] = outvec[i];
  // One Eigenvector has been computed.

  if (eval[vn[1]] == eval[vn[2]])
  {
    if (CrossP(outvec, vec2, vec1) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  return(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = vec1[i];
  }
  else
  {
	int nm1=cm1==0?1:0, nm2=cm1==2?!nm1:2;
    for (i=0; i<3; i++)
	{
	  vec1[i] = (i==nm1) ? mat[nm1][i]-eval[vn[1]] : mat[nm1][i];
	  vec2[i] = (i==nm2) ? mat[nm2][i]-eval[vn[1]] : mat[nm2][i];
	}
	mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
	mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
	cm2 = mm[1]<mm[2]? nm1:nm2;
	if (cm2 == nm2)
	  memcpy(vec2, vec1, sizeof(vec0));
	for (i=0; i<3; i++)
	{
	  vec1[i] = evec[vn[0]][i];
	}
	if (CrossP(vec1, vec2, outvec) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  return(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = outvec[i];
  }
  // Two Eigenvectors have been computed.

  /* calculate the third Eigenvector as a cross product of the first ones */
  CrossP(evec[vn[0]], evec[vn[1]], outvec);
  for (i=0; i<3; i++)
    evec[vn[2]][i] = outvec[i];
  return(0);
}

int CrossP(double vec1[3], double vec2[3], double res[3])
{     
  double len;
  
  res[0] = vec1[1]*vec2[2]-vec1[2]*vec2[1];
  res[1] = vec1[2]*vec2[0]-vec1[0]*vec2[2];
  res[2] = vec1[0]*vec2[1]-vec1[1]*vec2[0];
  len = sqrt(pow(res[0],2.)+pow(res[1],2.)+pow(res[2],2.));
  if (len <= 0.0)
    return(-1);
  else {
    res[0] = res[0]/len;
    res[1] = res[1]/len;
    res[2] = res[2]/len;
  }
  return(0);
}       

