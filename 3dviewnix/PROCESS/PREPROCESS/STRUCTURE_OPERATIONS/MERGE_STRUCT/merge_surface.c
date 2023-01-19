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
#include <cv3dv.h>
#define MAX_BUFFER_SIZE 0x0FFF

int CheckConsistancy(ViewnixHeader *v1, ViewnixHeader *v2, ViewnixHeader *out);

unsigned char buffer[MAX_BUFFER_SIZE];


/************************************************************************
 *
 *      FUNCTION        : main
 *
 *      DESCRIPTION     : This merges two structures to create a new
 *                        structure system. The slice spacing and the
 *                        pixel size HAVE to be the same in additon to
 *                        have consistant information for other groups.
 *                        This writes the data in chunks of 
 *                        MAX_BUFFER_SIZE so that there is not much
 *                        overhead on memory.
 *
 *      RETURN VALUE    : 0  on normal completion.
 *                        -1 otherwise.
 *
 *      PARAMETERS      : 5 parameters specified.
 *             process_name : process containing this program.
 *             input_file1  : 1st structure system file.
 *             input_file2  : 2st structure system file.
 *             output_file  : Merged file.
 *             bg_flag      : 0 - running in the foreground.
 *                             1 - running in the background.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : Both input files should have consistant
 *                        information.
 *
 *      EXIT CONDITIONS : On error or apon completion.
 *
 *      RELATED FUNCS   : CheckConsistancy().
 *
 *      History         : 07/12/1993 Supun Samarasekra                  
 *
 ************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{
  FILE *infp1,*infp2,*outfp;
  static ViewnixHeader hd1,hd2,outhd;
  int i,file_type,bg_flag,error;
  unsigned int tse_size,ntse_size,tse_bytes,ntse_bytes,num_read;
  char grp[6],elem[6],*ext;
  float percent;

  if (argc!=5) {
    printf("Usage: %s <input_file1_with_extension> <input_file2_with_extension> <output_file_with_extension> bg_flag\n",argv[0]);
    fflush(stdout);
    printf("Input files should have the same extension.\n");
    printf("Extensions are BS1 BS0 BSI\n");
    printf("bg_flag - 1 - background process\n");
    printf("          0 - foreground process\n");
    exit(-1);
  }

  bg_flag=atoi(argv[4]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  ext=rindex(argv[1],'.');
  if (strcmp(ext,rindex(argv[2],'.'))) {
    printf("Both input files should have the same extension\n");
    fflush(stdout);
    exit(-1);
  }
  if (!strcmp(ext,".BS1")) 
    file_type=SHELL1;
  else if (!strcmp(ext,".BS0"))
    file_type=SHELL0;
  else if (!strcmp(ext,".BSI"))
    file_type=SHELL0;
  else {
    printf("Incorrect file extensions specified\n");
    fflush(stdout);
    exit(-1);
  }

  infp1=fopen(argv[1],"rb");
  infp2=fopen(argv[2],"rb");
  outfp=fopen(argv[3],"w+b");
  if (infp1==NULL || infp2==NULL) {
    printf("Could not open input files\n");
    fflush(stdout);
    exit(-1);
  }
  if (outfp==NULL) {
    printf("Could not open output file\n");
    fflush(stdout);
    exit(-1);
  }
    

  error=VReadHeader(infp1,&hd1,grp,elem);
  if (error) {
    printf("file1: Read error  %d ( group:%s element:%s )\n",error,grp,elem);
    if (error!=106 && error!=107) {
      printf("Cannot revoer from read error\n");
      fflush(stdout);
      exit(-1);
    }
  }

  error=VReadHeader(infp2,&hd2,grp,elem);
  if (error) {
    printf("file2: Read error  %d ( group:%s element:%s )\n",error,grp,elem);
    if (error!=106 && error!=107) {
      printf("Cannot revoer from read error\n");
      fflush(stdout);
      exit(-1);
    }
  }
   
  if (CheckConsistancy(&hd1,&hd2,&outhd)>=0) {
    error=VWriteHeader(outfp,&outhd,grp,elem);
    fflush(stdout);
    if (error) {
      printf("Write error  %d ( group:%s element:%s )\n",error,grp,elem);
      fflush(stdout);
      if (error!=106 && error!=107) {
	printf("Cannot revoer from error\n");
	fflush(stdout);
	exit(-1);
      }
    }
    VSeekData(infp1,0);
    VSeekData(infp2,0);
    VSeekData(outfp,0);
    /* Consistancy checks if it is byte aligned before */
    ntse_bytes= hd1.str.num_of_bits_in_NTSE/8;
    ntse_size= MAX_BUFFER_SIZE/ntse_bytes;
    tse_bytes=  hd1.str.num_of_bits_in_TSE/8;
    tse_size= MAX_BUFFER_SIZE/tse_bytes;
    for(i=0;i<hd1.str.num_of_structures;i++) {
      num_read=0;
      while (num_read+ntse_size <= hd1.str.num_of_NTSE[i]) {
	if (fread(buffer,ntse_bytes,ntse_size,infp1)!=ntse_size) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,ntse_bytes,ntse_size,outfp)!=ntse_size) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
	num_read += ntse_size;
      }
      if (num_read < hd1.str.num_of_NTSE[i]) {
	if (fread(buffer,ntse_bytes,hd1.str.num_of_NTSE[i]-num_read,infp1)!=
	    hd1.str.num_of_NTSE[i]-num_read) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,ntse_bytes,hd1.str.num_of_NTSE[i]-num_read,outfp)!=
	    hd1.str.num_of_NTSE[i]-num_read) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
      }
      
      num_read=0;
      while (num_read+tse_size <= hd1.str.num_of_TSE[i]) {
	if (fread(buffer,tse_bytes,tse_size,infp1)!=tse_size) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,tse_bytes,tse_size,outfp)!=tse_size) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
	num_read += tse_size;
	percent= (float)(100.0*num_read/hd1.str.num_of_TSE[i]);
	printf("%0.2f%% of surface %d in file 1 merged\n",percent,i+1); 
	fflush(stdout);
      }
      if (num_read < hd1.str.num_of_TSE[i]) {
	if (fread(buffer,tse_bytes,hd1.str.num_of_TSE[i]-num_read,infp1)!=
	    hd1.str.num_of_TSE[i]-num_read) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,tse_bytes,hd1.str.num_of_TSE[i]-num_read,outfp)!=
	    hd1.str.num_of_TSE[i]-num_read) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}
	printf("100.00%% of surface %d in file 2 merged\n",i+1); 
	fflush(stdout);	
      }
    }
    
    for(i=0;i<hd2.str.num_of_structures;i++) {
      /* HEADER 2 NTSE */
      num_read=0;
      while (num_read+ntse_size <= hd2.str.num_of_NTSE[i]) {
	if (fread(buffer,ntse_bytes,ntse_size,infp2)!=ntse_size) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,ntse_bytes,ntse_size,outfp)!=ntse_size) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
	num_read += ntse_size;
      }
      if (num_read < hd2.str.num_of_NTSE[i]) {
	if (fread(buffer,ntse_bytes,hd2.str.num_of_NTSE[i]-num_read,infp2)!=
	    hd2.str.num_of_NTSE[i]-num_read) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,ntse_bytes,hd2.str.num_of_NTSE[i]-num_read,outfp)!=
	    hd2.str.num_of_NTSE[i]-num_read) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
      }



      /* HEADER 2 TSE */
      num_read=0;
      while (num_read+tse_size <= hd2.str.num_of_TSE[i]) {
	if (fread(buffer,tse_bytes,tse_size,infp2)!=tse_size) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,tse_bytes,tse_size,outfp)!=tse_size) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
	num_read += tse_size;
	percent= (float)(100.0*num_read/hd2.str.num_of_TSE[i]);
	printf("%0.2f%% of surface %d in file 2 merged\n",percent,i+1); 
	fflush(stdout);
      }
      if (num_read < hd2.str.num_of_TSE[i]) {
	if (fread(buffer,tse_bytes,hd2.str.num_of_TSE[i]-num_read,infp2)!=
	    hd2.str.num_of_TSE[i]-num_read) {
	  printf("Could not read input file 1\n");
	  fflush(stdout);
	  exit(-1);
	}
	if (fwrite(buffer,tse_bytes,hd2.str.num_of_TSE[i]-num_read,outfp)!=
	    hd2.str.num_of_TSE[i]-num_read) {
	  printf("Could not write output file\n");
	  fflush(stdout);
	  exit(-1);
	}	
	printf("100.00%% of surface %d in file 2 merged\n",i+1); 
	fflush(stdout);
	
      }
    }
    
    
    
    
  }
  else {
    printf("The input files do not have consistant headers");
    fflush(stdout);
    exit(-1);
  }
  
  if (bg_flag) VDeleteBackgroundProcessInformation();
  return(0);
  
}

#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ( (x) > (y) ? (x) : (y) )


/************************************************************************
 *
 *      FUNCTION        : CheckConsistancy()
 *
 *      DESCRIPTION     : This checks if the files have consistant 
 *                        information and builds a new header for 
 *                        the new structure from the old ones.
 *                        Most type 1D,2,2D,3 fields will be set to
 *                        invalid if either input file has it 
 *                        invalidated.
 *
 *      RETURN VALUE    : >= 0 means cosistant structures.
 *                        inconsistant otherwise.
 *
 *      PARAMETERS      :
 *                     vh1 : input header 1.
 *                     vh2 : input header 2.
 *                     out : output header.
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : v1 and v2 should ve initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *                        Modified: 11/14/02 description of extraction method
 *                           copied to new header by Dewey Odhner.
 *                        Modified: 11/8/02 slice spacing tolerance changed
 *                           by Dewey Odhner.
 *                        Modified: 2/18/05 scene_file assignment corrected
 *                           by Dewey Odhner.
 *
 ************************************************************************/
int CheckConsistancy(ViewnixHeader *v1, ViewnixHeader *v2, ViewnixHeader *out)
{
  int i,j,slices;
  double pz;
  StructureInfo *s1,*s2,*os;


  /* Initialize the general header of thr output file to be the same
     as the 1st input files */
  memcpy(&(out->gen),&(v1->gen),sizeof(GeneralInfo));
  
  s1=&v1->str;
  s2=&v2->str;
  os=&out->str;
  
  /* DIMENSION */
  if (s1->dimension!=3 || s2->dimension!=3) {
    printf("Dimension of the structures should be 3\n");
    fflush(stdout);
    return(-1);
  }
  os->dimension=3; os->dimension_valid=1;

  /* NUMBER OF STRUCUTRES */
  os->num_of_structures= s1->num_of_structures+s2->num_of_structures;
  os->num_of_structures_valid=1;



  /* DOMAIN */
  os->domain=(float *)malloc(sizeof(float)*12*os->num_of_structures);
  for(j=0;j<s1->num_of_structures;j++) {
    for(i=0;i<3;i++)
      os->domain[12*j+i]=s1->domain[12*j+i] + s1->domain[j*12+9+i]*s1->loc_of_samples[0];
    for(i=3;i<12;i++) 
      os->domain[12*j+i]=s1->domain[12*j+i];
  }
  for(j=0;j<s2->num_of_structures;j++) {
    for(i=0;i<3;i++)
      os->domain[12*(j+s1->num_of_structures)+i]=
	s2->domain[12*j+i] + s2->domain[j*12+9+i]*s2->loc_of_samples[0];
    for(i=3;i<12;i++) 
      os->domain[12*(j+s1->num_of_structures)+i]=s2->domain[12*j+i];
  }
  
  os->domain_valid=1;
  
  
  
  /* AXIS LABELS */
  if (s1->axis_label_valid && s2->axis_label_valid) {
    os->axis_label=s1->axis_label;
    os->axis_label_valid=1;
  }



  /* MEASUREMENT_UNIT */
  if (s1->measurement_unit_valid && s2->measurement_unit_valid) {
    for(i=0;i<3;i++)
      if (s1->measurement_unit[i]!=s2->measurement_unit[i]) {
	printf("The structures should have the same measurement units\n");
	fflush(stdout);
	return(-1);
      }
    os->measurement_unit=s1->measurement_unit;
    os->measurement_unit_valid=1;
  }
  else
    os->measurement_unit_valid=0;
  
  if (s1->scene_file_valid && s2->scene_file_valid ) {
    os->scene_file=(Char30 *)malloc(sizeof(Char30)*os->num_of_structures);
    for(i=0;i<s1->num_of_structures;i++)
      strcpy(os->scene_file[i],
	     s1->scene_file[i]);
    for(i=0;i<s2->num_of_structures;i++)
      strcpy(os->scene_file[s1->num_of_structures+i],
	     s2->scene_file[i]);
    os->scene_file_valid=1;
  }
  else
    os->scene_file_valid=0;



  /* NUMBER OF TSE'S AND NTSE'S */
  os->num_of_TSE=(unsigned int *)malloc(sizeof(int)*os->num_of_structures);
  for(i=0;i<s1->num_of_structures;i++)
    os->num_of_TSE[i]=s1->num_of_TSE[i];
  for(i=0;i<s2->num_of_structures;i++)
    os->num_of_TSE[i+s1->num_of_structures]=s2->num_of_TSE[i];
  os->num_of_TSE_valid=1;

  os->num_of_NTSE=(unsigned int *)malloc(sizeof(int)*os->num_of_structures);
  for(i=0;i<s1->num_of_structures;i++)
    os->num_of_NTSE[i]=s1->num_of_NTSE[i];
  for(i=0;i<s2->num_of_structures;i++)
    os->num_of_NTSE[i+s1->num_of_structures]=s2->num_of_NTSE[i];
  os->num_of_NTSE_valid=1;
  

  


  /* NUMBER OF COMPONENTS OF TSE'S AND NTSE'S */
  if (s1->num_of_components_in_TSE!=s2->num_of_components_in_TSE) {
    printf("The structures dont have the same num_of_components_in_TSE\n");
    return(-1);
  }
  os->num_of_components_in_TSE=s1->num_of_components_in_TSE;
  os->num_of_components_in_TSE_valid=1;

  if (s1->num_of_components_in_TSE<2) {
    printf ("The structures should at least have the ncode and y1 fileds\n");
    fflush(stdout);
    return(-1);
  }

  if (s1->num_of_components_in_NTSE!=s2->num_of_components_in_NTSE) {
    printf("The structures dont have the same num_of_components_in_NTSE\n");
    fflush(stdout);
    return(-1);
  }
  os->num_of_components_in_NTSE=s1->num_of_components_in_NTSE;
  os->num_of_components_in_NTSE_valid=1;





  /* SMALLEST AND THE LARGEST VALUES */
  os->smallest_value=
    (float *)malloc(sizeof(float)*os->num_of_structures*os->num_of_components_in_TSE);
  for(i=0;i<s1->num_of_structures*s1->num_of_components_in_TSE;i++)
    os->smallest_value[i]=s1->smallest_value[i];
  for(i=0;i<s2->num_of_structures*s1->num_of_components_in_TSE;i++)
    os->smallest_value[i+s1->num_of_structures*s1->num_of_components_in_TSE]=
      s2->smallest_value[i];
  
  
  os->largest_value=
    (float *)malloc(sizeof(float)*os->num_of_structures*os->num_of_components_in_TSE);
  for(i=0;i<s1->num_of_structures*s1->num_of_components_in_TSE;i++)
    os->largest_value[i]=s1->largest_value[i];
  for(i=0;i<s2->num_of_structures*s1->num_of_components_in_TSE;i++)
    os->largest_value[i+s1->num_of_structures*s1->num_of_components_in_TSE]=
      s2->largest_value[i];
  
  if (s1->smallest_value_valid && s2->smallest_value_valid)
    os->smallest_value_valid=1;
  if (s1->largest_value_valid && s2->largest_value_valid)
    os->largest_value_valid=1;
  
  
  
  
  
  
  if (s1->num_of_integers_in_TSE!=s2->num_of_integers_in_TSE) {
    printf("The structures dont have the same num_of_integers_in_TSE\n");
    fflush(stdout);
    return(-1);
  }
  os->num_of_integers_in_TSE=s1->num_of_integers_in_TSE;
  os->num_of_integers_in_TSE_valid=1;
  
  


  for(i=0;i<s1->num_of_components_in_TSE;i++) 
    if (s1->signed_bits_in_TSE[i]!=s2->signed_bits_in_TSE[i]) {
      printf("The TSE do not have the same sign\n");
      fflush(stdout);
      return(-1);
    }
  os->signed_bits_in_TSE=s1->signed_bits_in_TSE;
  if (s1->signed_bits_in_TSE_valid && s2->signed_bits_in_TSE_valid)
    os->signed_bits_in_TSE_valid=1;
  else 
    os->signed_bits_in_TSE_valid=0;
  
  
  if (s1->num_of_bits_in_TSE!=s2->num_of_bits_in_TSE) {
    printf("The structures dont have the same num_of_bits_in_TSE\n");
    fflush(stdout);
    return(-1);
  }
  os->num_of_bits_in_TSE=s1->num_of_bits_in_TSE;
  os->num_of_bits_in_TSE_valid=1;
  
  if (s1->num_of_bits_in_TSE%8 !=0) {
    printf("Cannot handle TSE's that are not byte aligned\n");
    fflush(stdout);
    return(-1);
  }
  



  for(i=0;i<s1->num_of_components_in_TSE;i++)
    if (s1->bit_fields_in_TSE[2*i] <= s1->bit_fields_in_TSE[2*i+1] &&
	(s1->bit_fields_in_TSE[2*i] != s2->bit_fields_in_TSE[2*i]  ||
	 s1->bit_fields_in_TSE[2*i+1] != s2->bit_fields_in_TSE[2*i+1])) {
      printf("The bit fields in the TSE are inconsistant\n");
      fflush(stdout);
      return(-1);
    }
  
  
  os->bit_fields_in_TSE=s1->bit_fields_in_TSE;
  os->bit_fields_in_TSE_valid=1;
  
  
  
  
  
  
  if (s1->num_of_integers_in_NTSE!=s2->num_of_integers_in_NTSE) {
    printf("The structures dont have the same num_of_integers_in_NTSE\n");
    fflush(stdout);
    return(-1);
  }
  os->num_of_integers_in_NTSE=s1->num_of_integers_in_NTSE;
  os->num_of_integers_in_NTSE_valid=1;
  
  
  
  
  for(i=0;i<s1->num_of_components_in_NTSE;i++)
    if (s1->signed_bits_in_NTSE[i]!=s2->signed_bits_in_NTSE[i]) {
      printf("The NTSE do not have the same sign\n");
      fflush(stdout);
      return(-1);
    }
  os->signed_bits_in_NTSE=s1->signed_bits_in_NTSE;
  if (s1->signed_bits_in_NTSE_valid && s2->signed_bits_in_NTSE_valid)
    os->signed_bits_in_NTSE_valid=1;
  else 
    os->signed_bits_in_NTSE_valid=0;
  
  
  
  
  if (s1->num_of_bits_in_NTSE!=s2->num_of_bits_in_NTSE) {
    printf("The structures dont have the same num_of_bits_in_TSE\n");
    fflush(stdout);
    return(-1);
  }
  os->num_of_bits_in_NTSE=s1->num_of_bits_in_NTSE;
  os->num_of_bits_in_NTSE_valid=1;
  
  if (s1->num_of_bits_in_NTSE%8 !=0) {
    printf("Cannot handle NTSE's that are not byte aligned\n");
    fflush(stdout);
    return(-1);
  }
  
  
  
  
  for(i=0;i<s1->num_of_components_in_NTSE;i++)
    if (s1->bit_fields_in_NTSE[2*i] <= s1->bit_fields_in_TSE[2*i+1] &&
	(s1->bit_fields_in_TSE[2*i] != s2->bit_fields_in_TSE[2*i]  ||
	 s1->bit_fields_in_TSE[2*i+1] != s2->bit_fields_in_TSE[2*i+1])) {
      printf("The bit fields in the TSE are inconsistant\n");
      fflush(stdout);
      return(-1);
    }
  os->bit_fields_in_NTSE=s1->bit_fields_in_NTSE;
  os->bit_fields_in_NTSE_valid=1;
  
  if (fabs(s1->xysize[0]-s2->xysize[0]) > 0.0001 ||
      fabs(s1->xysize[1]-s2->xysize[1]) > 0.0001) {
    printf("Pixel size of the scenes are different\n");
    fflush(stdout);
    return(-1);
  }
  os->xysize[0]=s1->xysize[0];
  os->xysize[1]=s1->xysize[1];
  os->xysize_valid=1;
  
  
  
  




  /* NUMBER AND LOCATION OF EACH OF THE SAMPLES */
  pz=fabs(s1->loc_of_samples[1]-s1->loc_of_samples[0]);
  for(i=0;i<s1->num_of_samples[0]-1;i++) {
    if (fabs(pz-fabs((double)s1->loc_of_samples[i+1]-s1->loc_of_samples[i])) > 0.01*pz) {
      printf("The slices should be equally spaced in the scenes\n");
      fflush(stdout);
      return(-1);
    }
  }
  for(i=0;i<s2->num_of_samples[0]-1;i++) 
    if (fabs(pz-fabs((double)s2->loc_of_samples[i+1]-s2->loc_of_samples[i])) > 0.01*pz) {
      printf("The slices should be equally spaced in both scenes\n");
      fflush(stdout);
      return(-1);
    }

  slices=MAX(s1->num_of_samples[0],s2->num_of_samples[0]);
  os->num_of_samples=(short *)malloc(sizeof(short));
  os->num_of_samples[0]=slices;
  os->loc_of_samples=(float *)malloc(sizeof(float)*slices);

  for(i=0;i<slices;i++)
    os->loc_of_samples[i] = (float)(i*pz);

  os->num_of_samples_valid=1;
  os->loc_of_samples_valid=1;






  /* PARAMETER VECTOR */
  if (s1->num_of_elements_valid && s2->num_of_elements_valid) {
    if (s1->num_of_elements!=s2->num_of_elements) {
      printf("The structures dont have the same num_of_elements\n");
      fflush(stdout);
      return(-1);
    }
    os->num_of_elements=s1->num_of_elements;
    os->num_of_elements_valid=1;
  }
  else 
    os->num_of_elements_valid=0;

  if (s1->description_of_element_valid && s2->description_of_element_valid) {
    for(i=0;i<s1->num_of_elements;i++)
      if (s1->description_of_element[i]!=s2->description_of_element[i]) {
	printf("Parameter vectors dont have the same description for fields\n");
	fflush(stdout);
	return(-1);
      }
    os->description_of_element=s1->description_of_element;
    os->description_of_element_valid=1;
  }
  else 
    os->description_of_element_valid=0;
  

  if (s1->parameter_vectors_valid && s2->parameter_vectors_valid) {
    os->parameter_vectors=
      (float *)malloc(sizeof(float)*(s2->num_of_structures*s2->num_of_elements+
				     s1->num_of_structures*s1->num_of_elements));
    for(i=0;i<s1->num_of_structures*s1->num_of_elements;i++)
      os->parameter_vectors[i]=s1->parameter_vectors[i];
    for(i=0;i<s2->num_of_structures*s2->num_of_elements;i++)
      os->parameter_vectors[s1->num_of_structures*s1->num_of_elements+i]=s2->parameter_vectors[i];
    os->parameter_vectors_valid=1;
  }
  else

    os->parameter_vectors_valid=0;
  
		      
    



  /* MIN MAX COORDINATES */
  os->min_max_coordinates=
    (float *)malloc(sizeof(float)*os->num_of_structures*6);
  if (s1->volume_valid && s2->volume_valid) {
    os->volume= (float *)malloc(sizeof(float)*os->num_of_structures);
    os->volume_valid=1;
  }
  if (s1->surface_area_valid && s2->surface_area_valid) {
    os->surface_area= (float *)malloc(sizeof(float)*os->num_of_structures);
    os->surface_area_valid=1;
  }
  if (s1->rate_of_change_volume_valid && s2->rate_of_change_volume_valid) {
    os->rate_of_change_volume= (float *)malloc(sizeof(float)*os->num_of_structures);
    os->rate_of_change_volume_valid=1;
  }
  for(i=0;i<s1->num_of_structures;i++) {
    os->min_max_coordinates[i*6]  =s1->min_max_coordinates[i*6]  ;
    os->min_max_coordinates[i*6+1]=s1->min_max_coordinates[i*6+1];
    os->min_max_coordinates[i*6+2]=s1->min_max_coordinates[i*6+2]-s1->loc_of_samples[0];
    os->min_max_coordinates[i*6+3]=s1->min_max_coordinates[i*6+3];
    os->min_max_coordinates[i*6+4]=s1->min_max_coordinates[i*6+4];
    os->min_max_coordinates[i*6+5]=s1->min_max_coordinates[i*6+5]-s1->loc_of_samples[0];
    if (s1->volume_valid && s2->volume_valid) 
      os->volume[i]=s1->volume[i];
    if (s1->surface_area_valid && s2->surface_area_valid)
      os->surface_area[i]=s1->surface_area[i];
    if (s1->rate_of_change_volume_valid && s2->rate_of_change_volume_valid) 
      os->rate_of_change_volume[i]=s1->rate_of_change_volume[i];
  }
  j=s1->num_of_structures*6;
  for(i=0;i<s2->num_of_structures;i++) {
    os->min_max_coordinates[j+i*6]  =s2->min_max_coordinates[i*6]  ;
    os->min_max_coordinates[j+i*6+1]=s2->min_max_coordinates[i*6+1];
    os->min_max_coordinates[j+i*6+2]=s2->min_max_coordinates[i*6+2]-s2->loc_of_samples[0];
    os->min_max_coordinates[j+i*6+3]=s2->min_max_coordinates[i*6+3];
    os->min_max_coordinates[j+i*6+4]=s2->min_max_coordinates[i*6+4];
    os->min_max_coordinates[j+i*6+5]=s2->min_max_coordinates[i*6+5]-s2->loc_of_samples[0];
    if (s1->volume_valid && s2->volume_valid) 
      os->volume[s1->num_of_structures+i]=s2->volume[i];
    if (s1->surface_area_valid && s2->surface_area_valid)
      os->surface_area[s1->num_of_structures+i]=s2->surface_area[i];
    if (s1->rate_of_change_volume_valid && s2->rate_of_change_volume_valid) 
      os->rate_of_change_volume[s1->num_of_structures+i]=s2->rate_of_change_volume[i];
  }
  os->min_max_coordinates_valid=1;
  

  /* EXTRACTION METHOD */
  if (s1->description_valid || s2->description_valid)
  {
    if (s1->description_valid == 0)
	  s1->description = "";
    if (s2->description_valid == 0)
	  s2->description = "";
    os->description = malloc(strlen(s1->description)+strlen(s2->description)+
      s1->num_of_structures);
    if (os->description == NULL)
    {
      fprintf(stderr, "Out of memory.\n");
	  exit(1);
    }
    for (i=j=0; s1->description[i]; i++)
    {
      os->description[i] = s1->description[i];
	  if (s1->description[i] == '\\')
	    j++;
    }
    while (j < s1->num_of_structures)
    {
      os->description[i++] = '\\';
	  j++;
    }
    strcpy(os->description+i, s2->description);
	os->description_valid = 1;
  }

  return(0);
  
}
    
	  
      
