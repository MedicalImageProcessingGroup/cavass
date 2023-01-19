/*
  Copyright 1993-2014, 2016, 2018-2020 Medical Image Processing Group
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
#include <stdlib.h>
#include <cv3dv.h>

#define SQR(x) ( (x)*(x) )
#define Floor(x) ((int)(x)<=(x)? (int)(x): (int)(x)-1)

int ResliceData(ViewnixHeader *vh, int out_slices, int plane_size[2],
    int out_bytes_per_slice, int slices,
    int bytes_per_slice, int vols, double ends[4][3], FILE *infp, FILE *outfp,
    int vhlen, int interp);

void Normalize(double vec[3]);
void InitHeader(char *outfile, ViewnixHeader *vh, ViewnixHeader *outvh,
    int vols, int out_slices, int plane_size[2], int ND_FLAG,
    double start_loc, double Px, double Py, double Pz, double ends[4][3],
    int force_domain, float domain[12]);
void WriteVolume(ViewnixHeader *vh, int vol, unsigned char *out_data,
    unsigned char *tmp_out_data, unsigned char *in_data,
    unsigned char *tmp_in_data, int bin_bytes_per_slice,
    int bin_out_bytes_per_slice, short *pl_slice_map, int plane_size[2],
    double cur_ends[3][3], int slices, int bytes_per_slice, FILE *infp,
    FILE *outfp, int vh_len, int interp);
void UnpackBitToByte(unsigned char *in, unsigned char *out, int size);


/*    Modified: 10/2/03 nearest neighbor option added by Dewey Odhner */
int main(int argc, char *argv[])
{

  int error,bg_flag, nearest_neighbor;
  FILE *infp,*outfp;
  ViewnixHeader vh,outvh;
  int vh_len;

  double ends[4][3],Px,Py,Pz,start_loc;
  float domain[12];
  int plane_size[2], ND_FLAG,out_slices;
  int vols,slices,bytes_per_slice,out_bytes_per_slice;





  int i,j;
  char grp[6],elem[6];

  nearest_neighbor = 0;
  for (j=1; j<argc; j++)
  {
    if (strcmp(argv[j], "-n") == 0)
      nearest_neighbor++;
    else
      argv[j-nearest_neighbor] = argv[j];
  }
  argc -= nearest_neighbor;
  switch (argc) {
   case 30:
   case 18:
    break;
   default:
    printf("Usage: %s infile outfile origin end_x end_y end_z plane_size out_slices bg_flag [domain] [-n]\n", argv[0]);
    fflush(stdout);
    printf("infile  - input scene (should have equal slice spacing)\n");
    printf("outfile - name of the reslice scene to write to \n");
    printf("origin  - [x y z] in pixels and slices of the input scene\n");
    printf("end_x   - location of the last pixel on the output x axis\n");
    printf("          relative to the original scene \n");
    printf("end_y   - same as above for the y-axis\n");
    printf("end_z   - same as above for the z_axis\n");
    printf("plane_size - size of the slice\n");
    printf("out_slices - output slices\n");
    printf("bg_flag - 1 is background, 0 is forground\n");
    printf("domain  - location & orientation of output scene domain\n");
    printf("-n      - nearest neighbor interpolation\n");
    exit(-1);
  }

  if (sscanf(argv[17],"%d",&bg_flag)!=1) {
    printf("Error in reading bg_flag\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);
  
  infp=fopen(argv[1],"r");
  outfp=fopen(argv[2],"w+");

  if (infp==NULL) {
    printf("Could not open the input file\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  
  if (outfp==NULL) {
    printf("Could not open the output file\n");
    fflush(stdout);
    exit(-1);
  }
  
  /* read the origin */
  if (sscanf(argv[3],"%lf",ends[0])!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[4],"%lf",ends[0]+1)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[5],"%lf",ends[0]+2)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }

  /* read the x axis */
  if (sscanf(argv[6],"%lf",ends[1])!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[7],"%lf",ends[1]+1)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[8],"%lf",ends[1]+2)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  

  /* read the y axis */
  if (sscanf(argv[9],"%lf",ends[2])!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[10],"%lf",ends[2]+1)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[11],"%lf",ends[2]+2)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }


  /* read the z axis */
  if (sscanf(argv[12],"%lf",ends[3])!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[13],"%lf",ends[3]+1)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }
  if (sscanf(argv[14],"%lf",ends[3]+2)!=1) {
    printf("Error in reading coordinates\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }


  if (sscanf(argv[15], "%dx%d", plane_size, plane_size+1)!=2)
  {
    if (sscanf(argv[15],"%d", plane_size)!=1) {
      printf("Error in reading plane_size\n");
      fflush(stdout);
      if (bg_flag) VDeleteBackgroundProcessInformation();
      exit(-1);
    }
    else
      plane_size[1] = plane_size[0];

  }
  if (sscanf(argv[16],"%d",&out_slices)!=1) {
    printf("Error in reading out_slices\n");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }

  for (j=18; j<argc; j++)
    if (sscanf(argv[j],"%f", domain+j-18) != 1) {
      printf("Error in reading domain\n");
      fflush(stdout);
      if (bg_flag) VDeleteBackgroundProcessInformation();
      exit(-1);
    }

  error=VReadHeader(infp,&vh,grp,elem);
  if (error) {
    if (error!=106 && error!=107) {
      printf("Read error  %d ( group:%s element:%s )\n",error,grp,elem);
      printf("Cannot recover from error\n");
      fflush(stdout);
      if (bg_flag) VDeleteBackgroundProcessInformation();
      exit(-1);
    }
  }
  VGetHeaderLength(infp,&vh_len);

  if (vh.gen.data_type!=IMAGE0) {
    printf("The Data file is not an IM0 file");
    fflush(stdout);
    if (bg_flag) VDeleteBackgroundProcessInformation();
    exit(-1);
  }

  Px=vh.scn.xypixsz[0];
  Py=vh.scn.xypixsz[1];
  
  if (vh.scn.dimension==4) {
    ND_FLAG=TRUE;
    if (argc == 30) {
      printf("3-D domain specified for 4-D scene\n");
      fflush(stdout);
      if (bg_flag) VDeleteBackgroundProcessInformation();
      exit(-1);
    }
    vols=vh.scn.num_of_subscenes[0];
    slices=vh.scn.num_of_subscenes[1];
    for(i=1;i<vols;i++) 
      if (slices!=vh.scn.num_of_subscenes[1+i]) {
        printf("Cannot handle 4D data with unequal number of slices\n");
        fflush(stdout);
        exit(-1);
      }
    Pz=fabs(vh.scn.loc_of_subscenes[vols+1]-vh.scn.loc_of_subscenes[vols]);
    start_loc=vh.scn.loc_of_subscenes[vols];
    for(i=0;i<vols;i++)
      for(j=0;j<slices-1;j++)
        if ( fabs(Pz- fabs(vh.scn.loc_of_subscenes[vols+i*slices+j+1] -
                           vh.scn.loc_of_subscenes[vols+i*slices+j])) > 0.05*Pz) {
          printf("Cannot handle data with unequal slices spacing\n");
          fflush(stdout);
          exit(-1);
        }
  }
  else {
    ND_FLAG=FALSE;
    vols=1;
    slices=vh.scn.num_of_subscenes[0];
    Pz=fabs(vh.scn.loc_of_subscenes[1]-vh.scn.loc_of_subscenes[0]);
    start_loc=vh.scn.loc_of_subscenes[0];
    for(j=0;j<slices-1;j++)
      if ( fabs(Pz- fabs(vh.scn.loc_of_subscenes[j+1] -
                         vh.scn.loc_of_subscenes[j])) > 0.05*Pz) {
        printf("Cannot handle data with unequal slices spacing\n");
        fflush(stdout);
        exit(-1);
      }
  }

  if (vh.scn.num_of_bits==16) {
    bytes_per_slice= 2*vh.scn.xysize[0]*vh.scn.xysize[1];
    out_bytes_per_slice= 2*plane_size[0]*plane_size[1];
  }
  else {
    bytes_per_slice= vh.scn.xysize[0]*vh.scn.xysize[1];
    out_bytes_per_slice= plane_size[0]*plane_size[1];
  }

  InitHeader(argv[2], &vh, &outvh, vols, out_slices, plane_size, ND_FLAG,
        start_loc, Px, Py, Pz, ends, argc==30, domain);
  
  
  error=VWriteHeader(outfp,&outvh,grp,elem);
  if (error) {
    if (error!=106 && error!=107) {
      printf("Write error  %d ( group:%s element:%s )\n",error,grp,elem);
      printf("Cannot revoer from error\n");
      fflush(stdout);
      if (bg_flag) VDeleteBackgroundProcessInformation();
      exit(-1);
    }
  }
  
  exit(ResliceData(&vh, out_slices, plane_size, out_bytes_per_slice, slices,
        bytes_per_slice, vols, ends, infp, outfp, vh_len, !nearest_neighbor));
    
    





}



void Normalize(double vec[3])
{

  double val;

  val= sqrt ( vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2] );
  vec[0]/=val;   vec[1]/=val;   vec[2]/=val; 

}


/* Modified: 12/17/03  s2->density_measurement_unit set by Dewey Odhner. */
/* Modified: 12/05/18 3-D axis transfomation transposed by Dewey Odhner. */
void InitHeader(char *outfile, ViewnixHeader *vh, ViewnixHeader *outvh,
    int vols, int out_slices, int plane_size[2], int ND_FLAG,
    double start_loc, double Px, double Py, double Pz, double ends[4][3],
    int force_domain, float domain[12])
{
  int i,j;
  SceneInfo *s1,*s2;
  double pix_z,orig[3],x_axis[3],y_axis[3],z_axis[3],temp,det;

  /* Set General Header */
  memcpy(&outvh->gen,&vh->gen,sizeof(GeneralInfo));
  strcpy(outvh->gen.filename1,outvh->gen.filename);
  strncpy(outvh->gen.filename,outfile,80);
  

  s1=&vh->scn;
  s2=&outvh->scn;
  /* Set Scene Header */
  s2->dimension=s1->dimension;
  s2->dimension_valid=1;

  x_axis[0]=Px*(ends[1][0]-ends[0][0]);
  x_axis[1]=Py*(ends[1][1]-ends[0][1]);
  x_axis[2]=Pz*(ends[1][2]-ends[0][2]);
  Normalize(x_axis);
  y_axis[0]=Px*(ends[2][0]-ends[0][0]);
  y_axis[1]=Py*(ends[2][1]-ends[0][1]);
  y_axis[2]=Pz*(ends[2][2]-ends[0][2]);
  Normalize(y_axis);
  z_axis[0]=Px*(ends[3][0]-ends[0][0]);
  z_axis[1]=Py*(ends[3][1]-ends[0][1]);
  z_axis[2]=Pz*(ends[3][2]-ends[0][2]);
  Normalize(z_axis);
  det=
    x_axis[0]*(y_axis[1]*z_axis[2]-y_axis[2]*z_axis[1]) -
    x_axis[1]*(y_axis[0]*z_axis[2]-y_axis[2]*z_axis[0]) +
    x_axis[2]*(y_axis[0]*z_axis[1]-y_axis[1]*z_axis[0]) ;
  if (det <0) {
    printf("Reslicing from last to first to maintain right handedness\n");
    for(i=0;i<3;i++) {
      temp=ends[0][i]; ends[0][i]=ends[3][i]; ends[3][i]=temp;
      ends[1][i] -= ends[3][i]-ends[0][i];
      ends[2][i] -= ends[3][i]-ends[0][i];
    }
    x_axis[0]=Px*(ends[1][0]-ends[0][0]);
    x_axis[1]=Py*(ends[1][1]-ends[0][1]);
    x_axis[2]=Pz*(ends[1][2]-ends[0][2]);
    Normalize(x_axis);
    y_axis[0]=Px*(ends[2][0]-ends[0][0]);
    y_axis[1]=Py*(ends[2][1]-ends[0][1]);
    y_axis[2]=Pz*(ends[2][2]-ends[0][2]);
    Normalize(y_axis);
    z_axis[0]=Px*(ends[3][0]-ends[0][0]);
    z_axis[1]=Py*(ends[3][1]-ends[0][1]);
    z_axis[2]=Pz*(ends[3][2]-ends[0][2]);
    Normalize(z_axis);
  }

  orig[0] = s1->domain[3]*(ends[0][0]*Px)+
            s1->domain[6]*(ends[0][1]*Py)+
            s1->domain[9]*(ends[0][2]*Pz + start_loc) + s1->domain[0];
  orig[1] = s1->domain[4]*(ends[0][0]*Px)+
            s1->domain[7]*(ends[0][1]*Py)+
            s1->domain[10]*(ends[0][2]*Pz + start_loc) + s1->domain[1];
  orig[2] = s1->domain[5]*(ends[0][0]*Px)+
            s1->domain[8]*(ends[0][1]*Py)+
            s1->domain[11]*(ends[0][2]*Pz + start_loc) + s1->domain[2];
  

  if (ND_FLAG) {

#if 1
    fprintf(stderr,"s2->domain not computed\n");
#else
    s2->domain=(float *)malloc(sizeof(float)*20);
    if (s2->domain==NULL) {
      fprintf(stderr,"Could Not Allocate domain\n");
      fflush(stdout);
      exit(-1);
    }
   
    /* tranform origin */
    s2->domain[0]= (float)(orig[0]*s1->domain[4]+orig[1]*s1->domain[5]+
      orig[2]*s1->domain[6] + s1->domain[0]);
    s2->domain[1]=(float)(orig[0]*s1->domain[8]+orig[1]*s1->domain[9]+
      orig[2]*s1->domain[10] + s1->domain[1]);
    s2->domain[2]=(float)(orig[0]*s1->domain[12]+orig[1]*s1->domain[13]+
      orig[2]*s1->domain[14] + s1->domain[2]);
    s2->domain[3]=s1->domain[3];

    /* tranform x_axis */
    s2->domain[4]= (float)(x_axis[0]*s1->domain[4]+x_axis[1]*s1->domain[5]+
      x_axis[2]*s1->domain[6]);
    s2->domain[5]= (float)(x_axis[0]*s1->domain[8]+x_axis[1]*s1->domain[9]+
      x_axis[2]*s1->domain[10]);
    s2->domain[6]= (float)(x_axis[0]*s1->domain[12]+x_axis[1]*s1->domain[13]+
      x_axis[2]*s1->domain[14]);
    s2->domain[7]=s1->domain[7];

    /* tranform y_axis */
    s2->domain[8]= (float)(y_axis[0]*s1->domain[4]+y_axis[1]*s1->domain[5]+
      y_axis[2]*s1->domain[6]);
    s2->domain[9]= (float)(y_axis[0]*s1->domain[8]+y_axis[1]*s1->domain[9]+
      y_axis[2]*s1->domain[10]);
    s2->domain[10]=(float)(y_axis[0]*s1->domain[12]+y_axis[1]*s1->domain[13]+
      y_axis[2]*s1->domain[14]);
    s2->domain[11]=s1->domain[11];

    /* tranform z_axis */
    s2->domain[12]= (float)(z_axis[0]*s1->domain[4]+z_axis[1]*s1->domain[5]+
      z_axis[2]*s1->domain[6]);
    s2->domain[13]= (float)(z_axis[0]*s1->domain[8]+z_axis[1]*s1->domain[9]+
      z_axis[2]*s1->domain[10]);
    s2->domain[14]=(float)(z_axis[0]*s1->domain[12]+z_axis[1]*s1->domain[13]+
      z_axis[2]*s1->domain[14]);
    s2->domain[15]=s1->domain[15];

    /* tranform t_axis */
    s2->domain[16]=s1->domain[16];
    s2->domain[17]=s1->domain[17];
    s2->domain[18]=s1->domain[18];
    s2->domain[19]=s1->domain[19];

#endif

  }
  else if (force_domain)
    s2->domain = domain;
  else {
    
    s2->domain=(float *)malloc(sizeof(float)*12);
    if (s2->domain==NULL) {
      fprintf(stderr,"Could Not Allocate domain\n");
      fflush(stdout);
      exit(-1);
    }

    /* store origin */
    s2->domain[0] = (float)orig[0];
    s2->domain[1] = (float)orig[1];
    s2->domain[2] = (float)orig[2];

    /* tranform x_axis */
    s2->domain[3]= (float)(x_axis[0]*s1->domain[3]+x_axis[1]*s1->domain[6]+
      x_axis[2]*s1->domain[9]);
    s2->domain[4]=(float)(x_axis[0]*s1->domain[4]+x_axis[1]*s1->domain[7]+
      x_axis[2]*s1->domain[10]);
    s2->domain[5]=(float)(x_axis[0]*s1->domain[5]+x_axis[1]*s1->domain[8]+
      x_axis[2]*s1->domain[11]);

    /* tranform y_axis */
    s2->domain[6]= (float)(y_axis[0]*s1->domain[3]+y_axis[1]*s1->domain[6]+
      y_axis[2]*s1->domain[9]);
    s2->domain[7]=(float)(y_axis[0]*s1->domain[4]+y_axis[1]*s1->domain[7]+
      y_axis[2]*s1->domain[10]);
    s2->domain[8]=(float)(y_axis[0]*s1->domain[5]+y_axis[1]*s1->domain[8]+
      y_axis[2]*s1->domain[11]);

    /* tranform z_axis */
    s2->domain[9]= (float)(z_axis[0]*s1->domain[3]+z_axis[1]*s1->domain[6]+
      z_axis[2]*s1->domain[9]);
    s2->domain[10]=(float)(z_axis[0]*s1->domain[4]+z_axis[1]*s1->domain[7]+
      z_axis[2]*s1->domain[10]);
    s2->domain[11]=(float)(z_axis[0]*s1->domain[5]+z_axis[1]*s1->domain[8]+
      z_axis[2]*s1->domain[11]);
    
  }

  s2->domain_valid = ND_FLAG==0;


  s2->axis_label_valid=0;

  s2->measurement_unit=(short *)malloc(s2->dimension*sizeof(short));
  if (s2->measurement_unit==NULL) {
    fprintf(stderr,"Could Not Allocate measurement_unit\n");
    fflush(stdout);
    exit(-1);
  }
  for(i=0;i<3;i++)
    s2->measurement_unit[i]=s1->measurement_unit[0];
  if (ND_FLAG) s2->measurement_unit[3]=s2->measurement_unit[3];
  s2->measurement_unit_valid=1;



  if (s1->num_of_density_values!=1) {
    printf("Cannot handle vector valued scenes\n");
    fflush(stdout);
    exit(-1);
  }
  else {
    s2->num_of_density_values=1;
    s2->num_of_density_values_valid=1;
  }
  
  s2->density_measurement_unit= s1->density_measurement_unit;
  s2->density_measurement_unit_valid= s1->density_measurement_unit_valid;
  
  s2->smallest_density_value=s1->smallest_density_value;
  s2->smallest_density_value_valid=s1->smallest_density_value_valid;
  if (s1->smallest_density_value_valid)
    s1->smallest_density_value[0] = 0;

  s2->largest_density_value=s1->largest_density_value;
  s2->largest_density_value_valid=s1->largest_density_value_valid;
  
  s2->num_of_integers=s1->num_of_integers;
  s2->num_of_integers_valid=s1->num_of_integers_valid;
  
  s2->signed_bits=s1->signed_bits;
  s2->signed_bits_valid=s1->signed_bits_valid;
  
  s2->num_of_bits=s1->num_of_bits;
  s2->num_of_bits_valid=s1->num_of_bits_valid;
  if (s2->num_of_bits!=1 && s2->num_of_bits!=8 && s2->num_of_bits!=16) {
    printf("The data has to be 8 or 16 bits\n");
    fflush(stdout);
    exit(-1);
  }

  s2->bit_fields=s1->bit_fields;
  s2->bit_fields_valid=s1->bit_fields_valid;


  s2->dimension_in_alignment=s1->dimension_in_alignment;
  s2->dimension_in_alignment_valid=s1->dimension_in_alignment_valid;

  s2->bytes_in_alignment=s1->bytes_in_alignment;
  s2->bytes_in_alignment_valid=s1->bytes_in_alignment_valid;


  s2->xysize[0]=plane_size[0];
  s2->xysize[1]=plane_size[1];
  s2->xysize_valid=1;


  if (ND_FLAG) {
    s2->num_of_subscenes=(short *)malloc(sizeof(short)*(1+vols));
    if (s2->num_of_subscenes==NULL) {
      fprintf(stderr,"Could Not Allocate num_of_subscenes\n");
      fflush(stdout);
      exit(-1);
    }
    s2->num_of_subscenes[0]=vols;
    for(i=0;i<vols;i++)
      s2->num_of_subscenes[i+1]=out_slices;
  }
  else {
    s2->num_of_subscenes=(short *)malloc(sizeof(short));
    if (s2->num_of_subscenes==NULL) {
      fprintf(stderr,"Could Not Allocate num_of_subscenes\n");
      fflush(stdout);
      exit(-1);
    }
    s2->num_of_subscenes[0]=out_slices;
  }
  s2->num_of_subscenes_valid=1;


  s2->xypixsz[0]= (float)(sqrt(SQR( Px*(ends[1][0]-ends[0][0])) +
                       SQR( Py*(ends[1][1]-ends[0][1])) +
                       SQR( Pz*(ends[1][2]-ends[0][2])) )/(plane_size[0]-1));
  s2->xypixsz[1]= (float)(sqrt(SQR( Px*(ends[2][0]-ends[0][0]) ) +
                       SQR( Py*(ends[2][1]-ends[0][1]) ) +
                       SQR( Pz*(ends[2][2]-ends[0][2]) ) )/(plane_size[1]-1));
  s2->xypixsz_valid=1;


  if (out_slices>1) 
    pix_z= sqrt(SQR( Px*(ends[3][0]-ends[0][0])) +
                SQR( Py*(ends[3][1]-ends[0][1])) +
                SQR( Pz*(ends[3][2]-ends[0][2])) )/(out_slices-1);
  else 
    pix_z=Pz;

  if (ND_FLAG) {
    s2->loc_of_subscenes=(float *)malloc(sizeof(float)*vols*(out_slices+1));
    if (s2->loc_of_subscenes==NULL) {
      fprintf(stderr,"Could Not Allocate loc_of_subscenes\n");
      fflush(stdout);
      exit(-1);
    }
    for(i=0;i<vols;i++) {
      s2->loc_of_subscenes[i]=s1->loc_of_subscenes[i];
      for(j=0;j<out_slices;j++)
        s2->loc_of_subscenes[vols+i*out_slices+j] = (float)(pix_z*j);
    }
  }
  else {
    s2->loc_of_subscenes=(float *)malloc(sizeof(float)*out_slices);
    if (s2->loc_of_subscenes==NULL) {
      fprintf(stderr,"Could Not Allocate loc_of_subscenes\n");
      fflush(stdout);
      exit(-1);
    }
    for(i=0;i<out_slices;i++)
      s2->loc_of_subscenes[i] = (float)(pix_z*i);
  }
  s2->loc_of_subscenes_valid=1;


  s2->description_valid=0;




}




/*****************************************************************************
 * FUNCTION: ResliceData
 * DESCRIPTION: Computes a resliced scene from a scene and writes it
 *    to a file.
 * PARAMETERS:
 *    vh: The header information of the input scene file.  The scene must be
 *       one, eight, or 16 bits per cell, scalar, packed, aligned by slice,
 *       have equal slice spacing and equal slices per volume.
 *    out_slices: The number of slices per volume in the output scene.
 *    plane_size: The size of the slices in the output scene in pixels.
 *    out_bytes_per_slice: The output slice size in bytes unpacked; i.e., bytes
 *       for 8 & 16 bit data, pixels for binary scenes.
 *    slices: the slices per volume in the input scene.
 *    bytes_per_slice: The input slice size in bytes unpacked; i.e., bytes
 *       for 8 & 16 bit data, pixels for binary scenes.
 *    vols: The number of volumes to be resliced.
 *    ends: The coordinates of corners of the output volume in units of voxels
 *       of the original scene.
 *       ends[0]: the origin of the slice
 *       ends[1]: the (max, 0, 0) corner of the slice
 *       ends[2]: the (0, max, 0) corner of the slice
 *    infp: The input scene file.
 *    outfp: The output scene file, must be positioned for writing the data.
 *    vhlen: The length of the input file header in bytes.
 *    interp: Non-zero for trilinear interpolation, otherwise nearest neighbor.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs, a message will be written to stdout
 *    and the process exited.
 * HISTORY:
 *    Created: by Supun Samarasekera
 *    Modified: 9/15/94 to use parameters, not global variables by Dewey Odhner
 *    Modified: 9/15/94 to make non-square slices by Dewey Odhner
 *    Modified: 9/15/94 to go faster with data in memory by Dewey Odhner
 *    Modified: 10/2/03 nearest neighbor option added by Dewey Odhner
 *
 *****************************************************************************/
int ResliceData(ViewnixHeader *vh, int out_slices, int plane_size[2],
    int out_bytes_per_slice, int slices,
    int bytes_per_slice, int vols, double ends[4][3], FILE *infp, FILE *outfp,
    int vhlen, int interp)
{
  short *pl_slice_map;
  int i, j, bin_bytes_per_slice, bin_out_bytes_per_slice, num, slice;
  double cur_ends[3][3], diff[3];
  unsigned char *tmp_out_data, *out_data, *in_data, *tmp_in_data;

  in_data = (unsigned char  *)malloc((slices+2)*bytes_per_slice);
  if (in_data) {
    pl_slice_map = NULL;
    memset(in_data, 0, bytes_per_slice);
    memset(in_data+(slices+1)*bytes_per_slice, 0, bytes_per_slice);
  }
  else {
    /* Allocate space for the buffer that stores the lower slice numbrer */
    pl_slice_map= (short *)malloc(sizeof(short)*plane_size[0]*plane_size[1]);
    if (pl_slice_map==NULL) {
      fprintf(stderr,"Could Not Allocate output buffers\n");
      fflush(stdout);
      return(-1);
    }
    if ((in_data=(unsigned char  *)malloc(2*bytes_per_slice))==NULL) {
      fprintf(stderr,"Could Not Allocate output buffers\n");
      fflush(stdout);
      return(-1);
    }
  }
  if (vh->scn.num_of_bits==1) {
    bin_bytes_per_slice= (bytes_per_slice+7)/8;
    if ((tmp_in_data=(unsigned char  *)malloc(bin_bytes_per_slice))==NULL) {
      fprintf(stderr,"Could Not Allocate output buffers\n");
      fflush(stdout);
      return(-1);
    } 
  }

  /* allocate space for the image buffer */
  if ((out_data=(unsigned char *)malloc(out_bytes_per_slice))==NULL) {
    printf("Could not allocate data for the output buffer\n");
    fflush(stdout);
    exit(-1);
  }
  if (vh->scn.num_of_bits==1) {
    bin_out_bytes_per_slice = (out_bytes_per_slice+7)/8;
    if ((tmp_out_data=(unsigned char *)malloc(bin_out_bytes_per_slice))==NULL){
      printf("Could not allocate data for the output buffer\n");
      fflush(stdout);
      exit(-1);
    }
  }
  if (out_slices>1) {
    diff[0]= (ends[3][0]-ends[0][0])/(out_slices-1);
    diff[1]= (ends[3][1]-ends[0][1])/(out_slices-1);
    diff[2]= (ends[3][2]-ends[0][2])/(out_slices-1);
  }
  else {
    diff[0]=diff[1]=diff[2]=0.0;
  }
  
  for(i=0;i<vols;i++) {
    if (pl_slice_map == NULL)
    {
      if (vh->scn.num_of_bits == 1) {
        if (fseek(infp, vhlen+i*slices*bin_bytes_per_slice, 0L)) {
          printf("Seek error on input file");
          fflush(stdout);
          exit(-1);
        }
        for (slice=1; slice<=slices; slice++) {
          if (VReadData((char*)tmp_in_data, 1,bin_bytes_per_slice,infp,&num)) {
            printf("Could not read original data set");
            fflush(stdout);
            exit(-1);
          }
          UnpackBitToByte(tmp_in_data, in_data+slice*bytes_per_slice,
            bytes_per_slice);
        }
      }
      else {
        if (fseek(infp, vhlen+i*slices*bytes_per_slice, 0L)) {
          printf("Seek error on input file");
          fflush(stdout);
          exit(-1);
        }
        if (VReadData((char*)in_data+bytes_per_slice,(vh->scn.num_of_bits+7)/8,
            slices*bytes_per_slice/((vh->scn.num_of_bits+7)/8),infp,&num)) {
          printf("Could not read original data set");
          fflush(stdout);
          exit(-1);
        }
      }
    }
    for(j=0;j<out_slices;j++) {
      //printf("Writing slice %d of Volume %d\n", j+1, i+1);
      //fflush(stdout);
      cur_ends[0][0] = ends[0][0] + diff[0]*j;
      cur_ends[0][1] = ends[0][1] + diff[1]*j;
      cur_ends[0][2] = ends[0][2] + diff[2]*j;
      
      cur_ends[1][0] = ends[1][0] + diff[0]*j;
      cur_ends[1][1] = ends[1][1] + diff[1]*j;
      cur_ends[1][2] = ends[1][2] + diff[2]*j;
      
      cur_ends[2][0] = ends[2][0] + diff[0]*j;
      cur_ends[2][1] = ends[2][1] + diff[1]*j;
      cur_ends[2][2] = ends[2][2] + diff[2]*j;

      WriteVolume(vh, i, out_data, tmp_out_data, in_data, tmp_in_data,
        bin_bytes_per_slice, bin_out_bytes_per_slice, pl_slice_map,
        plane_size, cur_ends, slices, bytes_per_slice, infp, outfp, vhlen,
        interp);
    }      

  }
  
  if (pl_slice_map)
    free(pl_slice_map);
  free(in_data);
  if (vh->scn.num_of_bits == 1) {
    free(tmp_in_data);
    free(tmp_out_data);
  }
  free(out_data);
  return(0);

}



/*****************************************************************************
 * FUNCTION: WriteVolume
 * DESCRIPTION: Computes a resliced data slice from a scene and writes it
 *    to a file.
 * PARAMETERS:
 *    vh: The header information of the input scene file.  The scene must be
 *       one, eight, or 16 bits per cell, scalar, packed, aligned by slice,
 *       have equal slice spacing and equal slices per volume.
 *    vol: The number from zero of the volume to be resliced.
 *    out_data: A buffer of size plane_size[0]*plane_size[1] bytes if data is
 *       one or eight bits per cell, or short words if 16 bits.
 *    tmp_out_data: A buffer of size bin_out_bytes_per_slice if data is one
 *       bit per cell; otherwise will not be dereferenced.
 *    in_data: If pl_slice_map is NULL, the input scene data with an empty
 *       slice before and after, unpacked if binary; otherwise a buffer of
 *       size bytes_per_slice.
 *    tmp_in_data: A buffer of size tmp_in_bytes_per_slice if data is one
 *       bit per cell and pl_slice_map is non-zero; otherwise will not be
 *       dereferenced.
 *    bin_bytes_per_slice: The number of bytes per slice in the input scene
 *       if it is binary.
 *    bin_out_bytes_per_slice: The number of bytes per slice in the output
 *       scene if it is binary.
 *    pl_slice_map: NULL if the input volume is loaded at in_data; otherwise
 *       a buffer of size plane_size[0]*plane_size[1] short words.
 *    plane_size: The size of the slices in the output scene in pixels.
 *    cur_ends: The coordinates of corners of the output slice in units of
 *       voxels of the original scene.
 *       cur_ends[0]: the origin of the slice
 *       cur_ends[1]: the (max, 0, 0) corner of the slice
 *       cur_ends[2]: the (0, max, 0) corner of the slice
 *    slices: the slices per volume in the input scene.
 *    bytes_per_slice: The input slice size in bytes unpacked; i.e., bytes
 *       for 8 & 16 bit data, pixels for binary scenes.
 *    infp: The input scene file.
 *    outfp: The output scene file, must be positioned for writing the
 *       current slice data.
 *    vh_len: The length of the input file header in bytes.
 *    interp: Non-zero for trilinear interpolation, otherwise nearest neighbor.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs, a message will be written to stdout
 *    and the process exited.
 * HISTORY:
 *    Created: by Supun Samarasekera
 *    Modified: 9/15/94 to use parameters, not global variables by Dewey Odhner
 *    Modified: 9/15/94 to make non-square slices by Dewey Odhner
 *    Modified: 9/15/94 to go faster with data in memory by Dewey Odhner
 *    Modified: 10/2/03 nearest neighbor option added by Dewey Odhner
 *
 *****************************************************************************/
void WriteVolume(ViewnixHeader *vh, int vol, unsigned char *out_data,
    unsigned char *tmp_out_data, unsigned char *in_data,
    unsigned char *tmp_in_data, int bin_bytes_per_slice,
    int bin_out_bytes_per_slice, short *pl_slice_map, int plane_size[2],
    double cur_ends[3][3], int slices, int bytes_per_slice, FILE *infp,
    FILE *outfp, int vh_len, int interp)
{

  double u[3],v[3],point[3]; /* vectors along the x,y directions on the plane in vol coords */
  double slice,dx,dy,dz;
  int i, j, n, x, y, z, min_slice, max_slice, num;
  short *map,*ptr;
  unsigned short *img_ptr,*pt1,*pt2,*ptr1,*ptr2,*tmp_ptr;
  unsigned char *img_ptr_8,*pt1_8,*pt2_8,*ptr1_8,*ptr2_8,*tmp_ptr_8;


 
  if (plane_size[0]>1) {
    /* Unit vector in the horz. dir along the plane in volumes coords */ 
    u[0]=(cur_ends[1][0]-cur_ends[0][0])/(plane_size[0]-1);
    u[1]=(cur_ends[1][1]-cur_ends[0][1])/(plane_size[0]-1);
    u[2]=(cur_ends[1][2]-cur_ends[0][2])/(plane_size[0]-1);
  }
  else 
    u[0]=u[1]=u[2]=0.0;
  if (plane_size[1]>1) {
    /* Unit vector in the vert. dir along the plane in volumes coords */ 
    v[0]=(cur_ends[2][0]-cur_ends[0][0])/(plane_size[1]-1);
    v[1]=(cur_ends[2][1]-cur_ends[0][1])/(plane_size[1]-1);
    v[2]=(cur_ends[2][2]-cur_ends[0][2])/(plane_size[1]-1);
  }
  else 
    v[0]=v[1]=v[2]=0.0;


  /* Initialize the lower slice index which gives rise to each pixel
     in the plane */  
  if (pl_slice_map) {
    min_slice=slices;
    max_slice=-2;
    for(ptr=pl_slice_map,i=0;i<plane_size[1];i++)
      for(j=0;j<plane_size[0];j++,ptr++) {
        slice= cur_ends[0][2] + i*v[2] + j*u[2] ;
        if (slice >=-1 && slice <slices)  {
          *ptr = Floor(slice);
          if (*ptr < min_slice) min_slice=*ptr;
          if (*ptr > max_slice) max_slice=*ptr;
        }
        else 
          *ptr = slices;
      
      }
  }
  
  
 
  if (vh->scn.num_of_bits==16) {  /* if the data is 16 bits */
    memset(out_data,0,2*plane_size[0]*plane_size[1]);

    /* ptr1 always points to the fist slice and ptr2 to the second */
    if (pl_slice_map) {
      ptr1=(unsigned short *)in_data;
      ptr2=(unsigned short *)(in_data + bytes_per_slice);
      /* Read the Next slice */
      if (min_slice>=0 && min_slice<slices) {
        fseek(infp,(long)(vh_len+(vol*slices+min_slice)*bytes_per_slice),0L);
        if (VReadData((char *)ptr2,2,bytes_per_slice/2,infp,&num)) {
          printf("Could not read original data set");
          fflush(stdout);
          exit(-1);
        }
      }
      else 
        memset(ptr2,0,bytes_per_slice);
      /* go through each slice and perform tri-linear interpolation */
      for(n=min_slice;n<=max_slice;n++) {
        /* swap so the first slice is in ptr1 and second is in ptr2 */
        tmp_ptr=ptr1; ptr1=ptr2; ptr2=tmp_ptr;
        if (n < slices-1) {
          fseek(infp,(long)(vh_len+(vol*slices+n+1)*bytes_per_slice),0L);
          if (VReadData((char *)ptr2,2,bytes_per_slice/2,infp,&num)) {
            printf("Could not read original data set");
            fflush(stdout);
            exit(-1);
          } 
        }
        else
          memset(ptr2,0,bytes_per_slice);
        img_ptr=(unsigned short *)out_data; map=pl_slice_map;
        for(i=0;i<plane_size[1];i++)
          for(j=0;j<plane_size[0];j++,map++,img_ptr++) 
            if (*map==n) { /* if the lower slice index matches n */
              point[0]= cur_ends[0][0] + i*v[0] + j*u[0] ;
              x=Floor(point[0]);
              if (x>= -1 && x<vh->scn.xysize[0]) {
                point[1]= cur_ends[0][1] + i*v[1] + j*u[1] ;
                y=Floor(point[1]);
                if (y>= -1 && y<vh->scn.xysize[1]) {
                  point[2]= cur_ends[0][2] + i*v[2] + j*u[2] ;
                  z=n;
                  dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                  /* if within bounds init locations in each slice */
                  pt1= (unsigned short *)ptr1 + y*vh->scn.xysize[0]+x;
                  pt2= (unsigned short *)ptr2 + y*vh->scn.xysize[0]+x;
                  *img_ptr= (unsigned short)(interp? .5+ 
                    (1-dz)*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt1
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt1+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    ) +
                    dz*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt2
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt2+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    )
                   :
                    dz<.5?
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt1
                        : x==vh->scn.xysize[0]-1? 0: *(pt1+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt1+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt1+vh->scn.xysize[0]+1)
                    :
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt2
                        : x==vh->scn.xysize[0]-1? 0: *(pt2+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt2+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt2+vh->scn.xysize[0]+1));
                }
              }
            }
       }
    }
    else {
      img_ptr=(unsigned short *)out_data;
      for(i=0;i<plane_size[1];i++)
        for(j=0;j<plane_size[0];j++,img_ptr++) {
          point[2]= cur_ends[0][2] + i*v[2] + j*u[2] ;
          if (point[2]<-1 || point[2]>=slices)
            continue;
          point[0]= cur_ends[0][0] + i*v[0] + j*u[0] ;
          x=Floor(point[0]);
          if (x>= -1 && x<vh->scn.xysize[0]) {
            point[1]= cur_ends[0][1] + i*v[1] + j*u[1] ;
            y=Floor(point[1]);
            if (y>= -1 && y<vh->scn.xysize[1]) {
              z=Floor(point[2]);
              ptr1 = (unsigned short *)(in_data+(z+1)*bytes_per_slice);
              ptr2 = (unsigned short *)(in_data+(z+2)*bytes_per_slice);
              dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
              /* if within bounds init locations in each slice */
              pt1= (unsigned short *)ptr1 + y*vh->scn.xysize[0]+x;
              pt2= (unsigned short *)ptr2 + y*vh->scn.xysize[0]+x;
              *img_ptr= (unsigned short)(interp? .5+ 
                (1-dz)*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt1
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt1+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                ) +
                dz*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt2
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt2+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                )
               :
                dz<.5?
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt1
                    : x==vh->scn.xysize[0]-1? 0: *(pt1+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt1+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt1+vh->scn.xysize[0]+1)
                :
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt2
                    : x==vh->scn.xysize[0]-1? 0: *(pt2+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt2+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt2+vh->scn.xysize[0]+1));
            }
          }
        }
    }

    if (VWriteData((char*)out_data,2,plane_size[0]*plane_size[1],outfp,&num)) {
      printf("Could not write output data\n");
      fflush(stdout);
      exit(-1);
    }

  }
  else if (vh->scn.num_of_bits==8) {
    memset(out_data,0,plane_size[0]*plane_size[1]);

    /* ptr1 always points to the fist slice and ptr2 to the second */
    if (pl_slice_map) {
      ptr1_8=(unsigned char *)in_data;
      ptr2_8=(unsigned char *)(in_data + bytes_per_slice);

      if (min_slice>=0 && min_slice<slices) {
        fseek(infp,(long)(vh_len+(vol*slices+min_slice)*bytes_per_slice),0L);
        if (fread(ptr2_8,1,bytes_per_slice,infp)!=bytes_per_slice) {
          printf("Could not read original data set");
          fflush(stdout);
          exit(-1);
        }
      }
      else 
        memset(ptr2_8,0,bytes_per_slice);
      /* go through each slice and perform tri-linear interpolation */
      for(n=min_slice;n<=max_slice;n++) {
        /* swap so the first slice is in ptr1 and second is in ptr2 */
        tmp_ptr_8=ptr1_8; ptr1_8=ptr2_8; ptr2_8=tmp_ptr_8;
        if (n < slices-1) {
          fseek(infp,(long)(vh_len+(vol*slices+n+1)*bytes_per_slice),0L);
          if (fread(ptr2_8,1,bytes_per_slice,infp)!=bytes_per_slice) {
            printf("Could not read original data set");
            fflush(stdout);
            exit(-1);
          }
        }
        else
          memset(ptr2_8,0,bytes_per_slice);
        img_ptr_8=(unsigned char *)out_data; map=pl_slice_map;
        for(i=0;i<plane_size[1];i++)
          for(j=0;j<plane_size[0];j++,map++,img_ptr_8++) 
            if (*map==n) { /* if the lower slice index matches n */
              point[0]= cur_ends[0][0] + i*v[0] + j*u[0];
              x=Floor(point[0]);
              if (x>= -1 && x<vh->scn.xysize[0]) {
                point[1]= cur_ends[0][1] + i*v[1] + j*u[1];
                y=Floor(point[1]);
                if (y>= -1 && y<vh->scn.xysize[1]) {
                  point[2]= cur_ends[0][2] + i*v[2] + j*u[2];
                  z=n;
                  dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                  /* if within bounds init locations in each slice */
                  pt1_8= (unsigned char *)ptr1_8 + y*vh->scn.xysize[0]+x;
                  pt2_8= (unsigned char *)ptr2_8 + y*vh->scn.xysize[0]+x;
                  *img_ptr_8= (unsigned char)(interp? .5+ 
                    (1-dz)*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt1_8
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1_8+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt1_8+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1_8+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    ) +
                    dz*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt2_8
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2_8+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt2_8+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2_8+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    )
                   :
                    dz<.5?
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt1_8
                        : x==vh->scn.xysize[0]-1? 0: *(pt1_8+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt1_8+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt1_8+vh->scn.xysize[0]+1)
                    :
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt2_8
                        : x==vh->scn.xysize[0]-1? 0: *(pt2_8+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt2_8+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt2_8+vh->scn.xysize[0]+1));
                }
              }
            }
      } 
    }
    else {
      img_ptr_8=(unsigned char *)out_data;
      for(i=0;i<plane_size[1];i++)
        for(j=0;j<plane_size[0];j++,img_ptr_8++) {
          point[2]= cur_ends[0][2] + i*v[2] + j*u[2];
          if (point[2]<-1 || point[2]>=slices)
            continue;
          point[0]= cur_ends[0][0] + i*v[0] + j*u[0];
          x=Floor(point[0]);
          if (x>= -1 && x<vh->scn.xysize[0]) {
            point[1]= cur_ends[0][1] + i*v[1] + j*u[1];
            y=Floor(point[1]);
            if (y>= -1 && y<vh->scn.xysize[1]) {
              z=Floor(point[2]);
              ptr1_8 = in_data+(z+1)*bytes_per_slice;
              ptr2_8 = in_data+(z+2)*bytes_per_slice;
              dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
              /* if within bounds init locations in each slice */
              pt1_8= (unsigned char *)ptr1_8 + y*vh->scn.xysize[0]+x;
              pt2_8= (unsigned char *)ptr2_8 + y*vh->scn.xysize[0]+x;
              *img_ptr_8= (unsigned char)(interp? .5+
                (1-dz)*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt1_8
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1_8+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt1_8+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1_8+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                ) +
                dz*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt2_8
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2_8+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt2_8+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2_8+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                )
               :
                dz<.5?
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt1_8
                    : x==vh->scn.xysize[0]-1? 0: *(pt1_8+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt1_8+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt1_8+vh->scn.xysize[0]+1)
                :
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt2_8
                    : x==vh->scn.xysize[0]-1? 0: *(pt2_8+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt2_8+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt2_8+vh->scn.xysize[0]+1));
            }
          }
        }
    }
    if (fwrite(out_data,1,plane_size[0]*plane_size[1],outfp)!=
            plane_size[0]*plane_size[1]) {
      printf("Could not write output data\n");
      fflush(stdout);
      exit(-1);
    }
  }
  else {
    memset(out_data,0,plane_size[0]*plane_size[1]);

    /* ptr1 always points to the fist slice and ptr2 to the second */
    if (pl_slice_map) {
      ptr1_8=(unsigned char *)in_data;
      ptr2_8=(unsigned char *)(in_data + bytes_per_slice);

      if (min_slice>=0 && min_slice<slices) {
        fseek(infp,(long)(vh_len+(vol*slices+min_slice)*bin_bytes_per_slice),
          0L);
        if (fread(tmp_in_data,1,bin_bytes_per_slice,infp)!=bin_bytes_per_slice)
        { printf("Could not read original data set");
          fflush(stdout);
          exit(-1);
        }
        UnpackBitToByte(tmp_in_data,ptr2_8,bytes_per_slice);
      }
      else 
        memset(ptr2_8,0,bytes_per_slice);
      /* go through each slice and perform tri-linear interpolation */
      for(n=min_slice;n<=max_slice;n++) {
        /* swap so the first slice is in ptr1 and second is in ptr2 */
        tmp_ptr_8=ptr1_8; ptr1_8=ptr2_8; ptr2_8=tmp_ptr_8;
        if (n < slices-1) {
          fseek(infp,(long)(vh_len+(vol*slices+n+1)*bin_bytes_per_slice),0L);
          if (fread(tmp_in_data,1,bin_bytes_per_slice,infp) !=
              bin_bytes_per_slice) {
            printf("Could not read original data set");
            fflush(stdout);
            exit(-1);
          }
          UnpackBitToByte(tmp_in_data,ptr2_8,bytes_per_slice);
        }
        else
          memset(ptr2_8,0,bytes_per_slice);
        img_ptr_8=(unsigned char *)out_data; map=pl_slice_map;
        for(i=0;i<plane_size[1];i++)
          for(j=0;j<plane_size[0];j++,map++,img_ptr_8++) 
            if (*map==n) { /* if the lower slice index matches n */
              point[0]= cur_ends[0][0] + i*v[0] + j*u[0];
              x=Floor(point[0]);
              if (x>= -1 && x<vh->scn.xysize[0]) {
                point[1]= cur_ends[0][1] + i*v[1] + j*u[1];
                y=Floor(point[1]);
                if (y>= -1 && y<vh->scn.xysize[1]) {
                  point[2]= cur_ends[0][2] + i*v[2] + j*u[2];
                  z=n;
                  dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                  /* if within bounds init locations in each slice */
                  pt1_8= (unsigned char *)ptr1_8 + y*vh->scn.xysize[0]+x;
                  pt2_8= (unsigned char *)ptr2_8 + y*vh->scn.xysize[0]+x;
                  *img_ptr_8= interp?
                    (1-dz)*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt1_8
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1_8+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt1_8+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt1_8+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    ) +
                    dz*
                    ( (y==-1? 0: (1-dy)*
                        ( (x==-1? 0:
                            (1-dx)* *pt2_8
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2_8+1)
                          )
                        )
                      ) +
                      (y==vh->scn.xysize[1]-1? 0: dy*
                        ( (x==-1? 0:
                            (1-dx)* *(pt2_8+vh->scn.xysize[0])
                          ) +
                          (x==vh->scn.xysize[0]-1? 0:
                            dx*     *(pt2_8+vh->scn.xysize[0]+1)
                          )
                        )
                      )
                    ) >= 127.5
                   :
                    dz<.5?
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt1_8
                        : x==vh->scn.xysize[0]-1? 0: *(pt1_8+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt1_8+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt1_8+vh->scn.xysize[0]+1)
                    :
                      dy<.5? y==-1? 0:
                        dx<.5? x==-1? 0:             *pt2_8
                        : x==vh->scn.xysize[0]-1? 0: *(pt2_8+1)
                      : y==vh->scn.xysize[1]-1? 0:
                        dx<.5? x==-1? 0:             *(pt2_8+vh->scn.xysize[0])
                        : x==vh->scn.xysize[0]-1? 0: *(pt2_8+vh->scn.xysize[0]+1);
                }
              }
            }
      } 
    }
    else {
      img_ptr_8=(unsigned char *)out_data;
      for(i=0;i<plane_size[1];i++)
        for(j=0;j<plane_size[0];j++,img_ptr_8++) {
          point[2]= cur_ends[0][2] + i*v[2] + j*u[2];
          if (point[2]<-1 || point[2]>=slices)
            continue;
          point[0]= cur_ends[0][0] + i*v[0] + j*u[0];
          x=Floor(point[0]);
          if (x>= -1 && x<vh->scn.xysize[0]) {
            point[1]= cur_ends[0][1] + i*v[1] + j*u[1];
            y=Floor(point[1]);
            if (y>= -1 && y<vh->scn.xysize[1]) {
              z=Floor(point[2]);
              ptr1_8 = in_data+(z+1)*bytes_per_slice;
              ptr2_8 = in_data+(z+2)*bytes_per_slice;
              dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
              /* if within bounds init locations in each slice */
              pt1_8= (unsigned char *)ptr1_8 + y*vh->scn.xysize[0]+x;
              pt2_8= (unsigned char *)ptr2_8 + y*vh->scn.xysize[0]+x;
              *img_ptr_8= interp?
                (1-dz)*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt1_8
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1_8+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt1_8+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt1_8+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                ) +
                dz*
                ( (y==-1? 0: (1-dy)*
                    ( (x==-1? 0:
                        (1-dx)* *pt2_8
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2_8+1)
                      )
                    )
                  ) +
                  (y==vh->scn.xysize[1]-1? 0: dy*
                    ( (x==-1? 0:
                        (1-dx)* *(pt2_8+vh->scn.xysize[0])
                      ) +
                      (x==vh->scn.xysize[0]-1? 0:
                        dx*     *(pt2_8+vh->scn.xysize[0]+1)
                      )
                    )
                  )
                ) >= 127.5
               :
                dz<.5?
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt1_8
                    : x==vh->scn.xysize[0]-1? 0: *(pt1_8+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt1_8+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt1_8+vh->scn.xysize[0]+1)
                :
                  dy<.5? y==-1? 0:
                    dx<.5? x==-1? 0:             *pt2_8
                    : x==vh->scn.xysize[0]-1? 0: *(pt2_8+1)
                  : y==vh->scn.xysize[1]-1? 0:
                    dx<.5? x==-1? 0:             *(pt2_8+vh->scn.xysize[0])
                    : x==vh->scn.xysize[0]-1? 0: *(pt2_8+vh->scn.xysize[0]+1);
            }
          }
        }
    }
    VPackByteToBit(out_data,plane_size[0]*plane_size[1],tmp_out_data);
    if (fwrite(tmp_out_data,1,bin_out_bytes_per_slice,outfp) !=
        bin_out_bytes_per_slice) {
      printf("Could not write output data\n");
      fflush(stdout);
      exit(-1);
    }
  }


}




void UnpackBitToByte(unsigned char *in, unsigned char *out, int size)
{
  int i,j;
  static unsigned char mask[8]= {1,2,4,8,16,32,64,128};
  unsigned char *p1,*p2;
  
  i=0; j=7; p1=in; p2=out;
  while (i<size) {
    if ((*p1) & mask[j]) {
      *p2= 255;
    }
    else
      *p2= 0;
    p2++;
    if (j==0) {
      j=7;
      p1++;
    }
    else 
      j--;
    i++;
  }
}
