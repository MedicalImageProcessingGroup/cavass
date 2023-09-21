/*
  Copyright 1993-2004, 2023 Medical Image Processing Group
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
#include <stdio.h>
#include <cv3dv.h>
#include "neighbors.h"

void WriteBinarySlice(), InitHeader(), ReadStructure();

char *shell_name, *image_name;
int cur_struct;



typedef unsigned short TSE[2];


typedef struct {
  int count;
  TSE *tse;
} NTSE;
NTSE **ntse;


short *nt,width,height;

FILE *infp,*outfp;
static ViewnixHeader invh,outvh;


int min_x,min_y,min_z;

int error;

/************************************************************************
 *
 *      FUNCTION        : BS0_TO_BIM
 *
 *      DESCRIPTION     : This would take a BS0 (BSI) file and generate 
 *                        a BIM (binary) file. This assumes that the BS0 
 *                        always has a closed contour.
 *
 *      RETURN VALUE    : 0 - on succesfull completion.
 *
 *      PARAMETERS      : 
 *          argument 1 :  Name of this process.
 *          argument 2 :  Name of the BS0 file.
 *          argument 3 :  Strucutre number to be converted ( stating at 0)
 *          argument 4 :  Binary file name.
 *          argument 5 :  Background flag.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : file open or memory allocation error.
 *
 *      RELATED FUNCS   : ReadStructure(),InitHeader(),WriteBinarySlice()
 *
 *      History         : 07/12/1993 Supun Samarasekra                  
 *                        Modified: 8/29/02 usage message corrected
 *                           by Dewey Odhner.
 *
 ************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{
  char group[6],elem[6];
  int bg_flag,i,j;
  
  if (argc!=5) {
    printf("Usage:%s BS0_file struct_num BIM_file bg_flag\n",argv[0]);
    fflush(stdout);
    exit(-1);
  }


  shell_name = argv[1];
  image_name = argv[3];
  
  if (sscanf(argv[2],"%d",&cur_struct)!=1) {
    printf("Invalid structure number specified\n");
    fflush(stdout);
    exit(-1);
  }

  bg_flag=atoi(argv[4]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  if ((infp=fopen(shell_name,"rb"))==NULL) {
    fprintf(stderr,"Could not open %s\n",shell_name);
    fflush(stdout);
    exit(-1);
  }
  else {
    error=VReadHeader(infp,&invh,group,elem);
    if (error) printf("Read error %d group %s element %s\n",error,group,elem);
    fflush(stdout);
    if (error<=104) exit(-1);
  }

  if ((outfp=fopen(image_name,"wb+"))==NULL) {
    fprintf(stderr,"Could not open %s\n",image_name);
    fflush(stdout);
    exit(-1);
  }

  if (invh.str.num_of_structures<=cur_struct  || cur_struct<0) {
    printf("Invalid structure number specified (should be 0 - %d\n",outvh.str.num_of_structures-1);
    fflush(stdout);
    exit(-1);
  }
    
  ReadStructure();
  InitHeader();
  error=VWriteHeader(outfp,&outvh,group,elem);
  if (error && error!=106 && error!=107) {
    printf("Write error %d (group %s elem %s)\n",error,group,elem);
    fflush(stdout);
    exit(-1);
  }
  WriteBinarySlice();
  VCloseData(outfp);
  fclose(infp);



  if (bg_flag) 
    VDeleteBackgroundProcessInformation();
  return(0);
 


}



/************************************************************************
 *                                                                      *
 *      Function        : ReadStructure                                 *
 *                                                                      *
 *      Description     : This would read the input stucture file and   *
 *                        initialize the ntse,and nt array with the     *
 *                        vaules of cur_struct.                         *
 *                                                                      *
 *      Return Value    : None.                                         *
 *                                                                      *
 *      Parameters      : None.                                         *
 *                                                                      *
 *      Side effects    : On a read error this would exit the process.  *
 *                                                                      *
 *      Entry condition : infp should be opend for reading.             *
 *                                                                      *
 *      Related funcs   : None.                                         *
 *                                                                      *
 *      History         : 07/12/1993 Supun Samarasekera                 *
 *                                                                      *
 ************************************************************************/
void ReadStructure()
{

  int i,j,count,skip_size,num;
  short rtemp,cols;
  TSE *ts,*ptr;


  skip_size=0;
  for(i=0;i<cur_struct;i++) {
    skip_size += (invh.str.num_of_NTSE[i]*invh.str.num_of_bits_in_NTSE)/8 +
      (invh.str.num_of_TSE[i]*invh.str.num_of_bits_in_TSE)/8 ;
  }
  error=VSeekData(infp,skip_size);
  if (error) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }


  nt=(short *)malloc(sizeof(short)*invh.str.num_of_NTSE[cur_struct]);
  if (nt==NULL) {
    printf("Memory allocation error when reading NTSE's\n");
    exit(-1);
  }

  ts=(TSE *)malloc(sizeof(TSE)*invh.str.num_of_TSE[cur_struct]);
  if (ts==NULL) {
    printf("Memory allocation error when reading TSE's\n");
    exit(-1);
  }
  

  if (VReadData((char *)nt,2,invh.str.num_of_NTSE[cur_struct],infp,&num)) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }


  if (VReadData((char *)ts,2,2*invh.str.num_of_TSE[cur_struct],infp,&num)) {
    printf("Error in Reading Data\n");
    fflush(stdout);
    exit(-1);
  }
  

  ntse=(NTSE **)malloc(sizeof(NTSE *)*nt[0]);
  if (ntse==NULL) {
    printf("Memory allocation error when reading TSE's\n");
    exit(-1);
  }
 
  count=1+nt[0];
  ptr=ts;
  for(i=0;i<nt[0];i++)  {
    if (nt[i+1])  {
      ntse[i]=(NTSE *)malloc(sizeof(NTSE)*nt[i+1]); /* one for each contour */
      if (ntse[i]==NULL) {
	printf("Memory allocation error when reading TSE's\n");
	exit(-1);
      }
      for(j=0;j<nt[i+1];j++) {
	ntse[i][j].count=nt[count];
	ntse[i][j].tse=ptr;
	ptr += nt[count];
	count++;
      }
    }
    else 
      ntse[i]=NULL;
  }
  

}



/************************************************************************
 *
 *      FUNCTION        : InitHeader()
 *
 *      DESCRIPTION     : Initialize the output header. This looks at the
 *                        Input header and initializes the output header 
 *                        so as the fit the structure axactly. The domain
 *                        of the binary image is moved to accommodate 
 *                        this.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : None.
 *
 *      SIDE EFFECTS    : This initializes the global variables
 *                        ntse,nt,min_x,min_y,min_z,width,height.
 *
 *      ENTRY CONDITION : The input header should be read.
 *
 *      EXIT CONDITIONS : Memory allocation error.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *
 ************************************************************************/
void InitHeader()
{
  int i;
  SceneInfo *sc;
  StructureInfo *st;
  double max_diff;

  /* The General Header should be the same */
  memcpy(&outvh.gen,&invh.gen,sizeof(GeneralInfo));
  outvh.gen.data_type=IMAGE0;
  strncpy(outvh.gen.filename,image_name, sizeof(outvh.gen.filename)-1);
  outvh.gen.filename_valid=1;




  sc=&outvh.scn;
  st=&invh.str;


  min_x=rint(st->min_max_coordinates[6*cur_struct]/st->xysize[0]);
  min_y=rint(st->min_max_coordinates[6*cur_struct+1]/st->xysize[1]);
  min_z=0;
  for(max_diff=0.1,i=0;i<st->num_of_samples[0];i++) 
    if (max_diff > (st->loc_of_samples[i]-st->min_max_coordinates[6*cur_struct+2]) &&
	fabs(st->loc_of_samples[i]-st->min_max_coordinates[6*cur_struct+2]) < 0.00001) {
      min_z=i;
      max_diff=(st->loc_of_samples[i]-st->min_max_coordinates[6*cur_struct+2]);
    }
  



  sc->dimension=3;
  sc->dimension_valid=1;

  if (st->domain_valid) {
    sc->domain= st->domain + 12*cur_struct;
    sc->domain_valid=1;
    /* transleate the domain to min_x,min_y */
    sc->domain[0] += (sc->domain[3] *st->min_max_coordinates[6*cur_struct  ] +
		      sc->domain[6] *st->min_max_coordinates[6*cur_struct+1] );
    sc->domain[1] += (sc->domain[4] *st->min_max_coordinates[6*cur_struct  ] +
		      sc->domain[7] *st->min_max_coordinates[6*cur_struct+1] );
    sc->domain[2] += (sc->domain[5] *st->min_max_coordinates[6*cur_struct  ] +
		      sc->domain[8] *st->min_max_coordinates[6*cur_struct+1] );
  }


  if (st->axis_label_valid) {
    sc->axis_label= st->axis_label;
    sc->axis_label_valid=1;
  }

  sc->measurement_unit=st->measurement_unit;
  sc->measurement_unit_valid=st->measurement_unit_valid;
  
  sc->num_of_density_values=1;
  sc->num_of_density_values_valid=1;
  

  sc->smallest_density_value=(float *)malloc(sizeof(float));
  sc->largest_density_value=(float *)malloc(sizeof(float));
  if (sc->smallest_density_value) {
    sc->smallest_density_value[0]=0.0;
    sc->smallest_density_value_valid=1;
  }
  if (sc->largest_density_value) {
    sc->largest_density_value[0]=1.0;
    sc->largest_density_value_valid=1;
  }

  sc->num_of_integers=1;
  sc->num_of_integers_valid=1;
  
  sc->signed_bits=(short *)malloc(sizeof(short));
  if (sc->signed_bits) {
    sc->signed_bits[0]=0;
    sc->signed_bits_valid=1;
  }
  
  sc->num_of_bits=1;
  sc->num_of_bits_valid=1;

  sc->bit_fields=(short *)malloc(sizeof(short)*2);
  if (sc->bit_fields) {
    sc->bit_fields_valid=1;
    sc->bit_fields[0]=sc->bit_fields[1]=0;
  }

  sc->xysize[0]=width=  1+
    rint((st->min_max_coordinates[6*cur_struct+3] - st->min_max_coordinates[6*cur_struct])/
	 st->xysize[0]);
  sc->xysize[1]=height= 1+
    rint((st->min_max_coordinates[6*cur_struct+4] - st->min_max_coordinates[6*cur_struct+1])/
	 st->xysize[1]);
  sc->xysize_valid=1;

  sc->num_of_subscenes=st->num_of_samples;
  sc->num_of_subscenes_valid=st->num_of_samples_valid;

  sc->loc_of_subscenes=st->loc_of_samples;
  sc->loc_of_subscenes_valid=st->loc_of_samples_valid;

  sc->xypixsz[0]=st->xysize[0];
  sc->xypixsz[1]=st->xysize[1];
  sc->xypixsz_valid=st->xysize_valid;




}






/************************************************************************
 *
 *      FUNCTION        : WriteBinarySlice
 *
 *      DESCRIPTION     : This first converts the structure into a
 *                        edge arrary containing the edges along the X
 *                        direction. THen this is converted into a binary
 *                        image.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : None.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : The global varibles nt,ntse,min_x,min_y,min_z
 *                        width,height, invh should be initialized. outfp 
 *                        should be opend for read/write.
 *
 *      EXIT CONDITIONS : Read or Write error.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        6/6/01 new bit fields used by Dewey Odhner.
 *
 ************************************************************************/
void WriteBinarySlice()
{
  unsigned char *dat_slice,*edge_slice,*bin_slice,*ed,*dt;
  int i,j,k,bin_size,ptr,type;
  TSE *tse;
  int  x;

  dat_slice=(unsigned char *)malloc(width*height);
  edge_slice=(unsigned char *)malloc((width+1)*height);
  bin_size= (width*height+7)/8;
  bin_slice=(unsigned char *)calloc(bin_size,1);
  

  VSeekData(outfp,0);
  /* Write Blank Slices */
  for(i=0;i<min_z;i++)  {
    printf("processing slice %d/%d\n",i+1,outvh.scn.num_of_subscenes[0]);
    fflush(stdout);
    if (fwrite(bin_slice,1,bin_size,outfp)!=bin_size) {
      printf("Error in writing data\n");
      fflush(stdout);
      exit(-1);
    }
  }

  /* Write the contour data */
  for(i=0;i<nt[0];i++)  {
    printf("processing slice %d/%d\n",min_z+i+1,outvh.scn.num_of_subscenes[0]);
    fflush(stdout);
    memset(edge_slice,0,(width+1)*height);
    if (ntse[i]) {
      for(j=0;j<nt[i+1];j++) 
	for(tse=ntse[i][j].tse,k=0;k<ntse[i][j].count;tse++,k++) {
	  if (invh.str.bit_fields_in_TSE[3] > 15)
	    x= (int)((tse[0][0]&XMASK)<<1|(tse[0][1]&0x8000)!=0) - min_x;
	  else
	    x= (int)(tse[0][0]&XMASK) - min_x;
	  ptr= (width+1)*j + x;
	  if ((tse[0][0] & NX) == 0) {
	    edge_slice[ptr]++;
	  }
	  if ((tse[0][0] & PX) == 0) {
	    edge_slice[ptr+1]++;
	  }	  
	}
    }
    for(ed=edge_slice,dt=dat_slice,j=0;j<height;ed++,j++) {
      type=0;
      for(k=0;k<width;ed++,dt++,k++) {
	type= (type+ *ed)%2;
	*dt = (unsigned char)type;
      }
    }
    VPackByteToBit(dat_slice,width*height,bin_slice);
    if (fwrite(bin_slice,1,bin_size,outfp)!=bin_size) {
      printf("Error in writing data\n");
      fflush(stdout);
      exit(-1);
    }
    
  }
  
  /* Write Blank Slices */
  memset(bin_slice,0,bin_size);
  for(i=min_z+nt[0];i<outvh.scn.num_of_subscenes[0];i++)  {
    printf("processing slice %d/%d\n",i+1,outvh.scn.num_of_subscenes[0]);
    fflush(stdout);
    if (fwrite(bin_slice,1,bin_size,outfp)!=bin_size) {
      printf("Error in writing data\n");
      fflush(stdout);
      exit(-1);
    }
  }
  
  free(dat_slice);
  free(bin_slice);
  free(edge_slice);
  
}
