/*
  Copyright 1993-2014, 2017, 2020 Medical Image Processing Group
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


/*
	This program converts IM0/BIM files to  Dicom format 

	
Author: T Iwanaga
Date  : 8/2006

*/
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include <Viewnix.h>
#include  "cv3dv.h"
#include <assert.h>


#define BIT_EQUALS_BYTE 255

/* use the same definitions as defined previously	*/

#define BI 0	/* 16 bits  	*/
#define BD 1	/* 32 bits	*/
#define AN 2	/* Ascii Numeric	*/
#define AT 3	/* Ascii text 	*/
#define OW 4	/* Special 	*/

#define TEXT_SIZE 2048


int put_dcm_data(FILE *, unsigned short, unsigned short, int, char *, char *, unsigned int);
int get_slices(int,  short *);
int get_volume(int, short *, int);
void check_IO(bool);

extern "C" {
int VDeleteBackgroundProcessInformation();
}

int BuildColorLookup(unsigned char ct[256][3], ViewnixHeader *vh);
int put_dcm_data(FILE *fp, unsigned short us1, unsigned short us2, int type,
	char *vr, char *str, unsigned int len);
int get_volume(int dim, short *list, int slice);
int get_slices(int dim, short *list);


void swap2bytes(unsigned char *data, int size)
{
	int i;
	unsigned char temp;

	for(i=0; i<size; )
	{
		temp = data[i];
		data[i] = data[i+1]; 
		i++;
		data[i++] = temp;
	}
}

#define BIGENDIAN	1
#define	LITTLEENDIAN	0

int bigendian(void)
{
  union
  {
    long l;
    char c[sizeof (long)];
  } u;

  u.l = 1;
  return (u.c[sizeof (long) - 1] == 1? BIGENDIAN: LITTLEENDIAN);
}


static int this_PC_type;
static int dcm_endian_type;
static int need_swap_bytes;
#ifdef BIT_EQUALS_BYTE
static unsigned char uc_data_0 = 0;
static unsigned char uc_data_1= BIT_EQUALS_BYTE;
#endif

/* Modified: 10/11/07 series UID concocted by Dewey Odhner. */
/* Modified: 1/4/10 slice range option added by Dewey Odhner. */
int main(int argc, char *argv[])
{
    FILE *fpin, *fpout;
    int i, j, k, ni=0, len, data_size, mode=0;
    int nslices, image_size, error, volume=0, suid_len;
    unsigned short num_bits, samples_per_pixel;
    char group[6], elem[6], *text;
    char *SeriesInstanceUID=NULL, *SOPInstanceUID=NULL;
    ViewnixHeader vh;
    unsigned short ushort_num;
    unsigned char *data, *out_data,*in_ptr,*out_ptr;
    unsigned char ctable[256][3];
    int first_slice=0, last_slice=999999;

    if (strncmp(argv[argc-1], "-SeriesInstanceUID=", 19) == 0)
    {
        SeriesInstanceUID = (char *)malloc(strlen(argv[argc-1])-17);
        strcpy(SeriesInstanceUID, argv[argc-1]+19);
        suid_len = (int)strlen(SeriesInstanceUID);
        SeriesInstanceUID[suid_len+1] = 0;
        argc--;
    }

    if(argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "mipg2dicom <input-file> <output-file> [<mode> [<first-slice> <last_slice>]] [-SeriesInstanceUID=<UID>]\n");
        fprintf(stderr, "input-file : BIM, IM0, or MV0 file name\n");
        fprintf(stderr, "output-file : output file name  with no extension \n");
        fprintf(stderr, "mode (optional):  0 = foreground 1=background\n");
        exit(1);
    }

    if (argc > 5)
        first_slice = atoi(argv[4]), last_slice = atoi(argv[5]);
    fpin=fopen(argv[1], "rb");
    if (fpin == NULL)
    {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(1);
    }

    text = (char *)malloc(TEXT_SIZE);
    memset(text, ' ', TEXT_SIZE);

    error=VReadHeader(fpin, &vh, group, elem);
    if (error <= 104) {
        fprintf(stderr,"Reading header erro (%d)\n", error);
                exit(-1);
    }

    if (argc == 4) {
        mode = atoi(argv[argc-1]);
#ifdef PROCESS_INTERFACE_USED
        if(mode == 1)
            VAddBackgroundProcessInformation("pacs2mipg");
#endif
    }

    if (vh.gen.study_valid && vh.gen.series_valid)
    {
        if (vh.gen.study[strlen(vh.gen.study)-1] == ' ')
            vh.gen.study[strlen(vh.gen.study)-1] = 0;
        if (vh.gen.series[strlen(vh.gen.series)-1] == ' ')
            vh.gen.series[strlen(vh.gen.series)-1] = 0;
    }

    if (vh.gen.data_type == IMAGE0) {
       nslices=get_slices(vh.scn.dimension,vh.scn.num_of_subscenes);
       num_bits =  vh.scn.num_of_bits;
       samples_per_pixel = 1;
          if (num_bits == 1)
             image_size = (vh.scn.xysize[0]*vh.scn.xysize[1]+7)/8;
          else if (num_bits == 8)
             image_size  = vh.scn.xysize[0]*vh.scn.xysize[1];
          else if (num_bits == 16)
             image_size = (vh.scn.xysize[0]*vh.scn.xysize[1])*2;
          else {
             fprintf(stderr, "image size cannot be determined\n");
             exit(1);
          }
    } else {
        num_bits =  vh.dsp.num_of_bits;
        nslices = vh.dsp.num_of_images;
        if (num_bits==8 && vh.dsp.num_of_elems==1) {
            samples_per_pixel = 1;
            image_size = vh.dsp.xysize[0]*vh.dsp.xysize[1];
            if (vh.gen.gray_descriptor_valid || vh.gen.red_descriptor_valid) {
                BuildColorLookup(ctable, &vh);
                samples_per_pixel = 3;
            }
        }
        else if (num_bits==24 && vh.dsp.num_of_elems==3) {
            image_size =vh.dsp.xysize[0]*vh.dsp.xysize[1]*3;
            samples_per_pixel = 3;
        }
        else {
              fprintf(stderr, "image size cannot be determined\n");
              exit(1);
        }
    }

    data = (unsigned char *)malloc(image_size);
    assert(data != NULL);
    this_PC_type = bigendian();

    for (i=0; i< nslices ; i++)
      if (i>=first_slice && i<= last_slice) {
        if (strlen(argv[2])+9 >= TEXT_SIZE)
        {
          fprintf(stderr, "File name is too long.\n");
          exit(1);
        }
        sprintf(text, "%s_%04d.dcm", argv[2], i+1);
        fpout=fopen(text, "wb");
        if (fpout == NULL)
        {
          fprintf(stderr, "Cannot open %s.\n", text);
          exit(1);
        }
#ifdef VERBOSE
        fflush(stderr);  printf("dicom file: %s\n", text);  fflush(stdout);
#endif
        fwrite(text, 1, 128, fpout);
        fwrite("DICM", 1, 4, fpout);
        strcpy(text, "1.2.840.10008.1.2.1 ");
        len = (int)strlen(text);
        put_dcm_data(fpout, 0x0002, 0x0010, AT, (char *)"UI", text, len); 
        dcm_endian_type = LITTLEENDIAN; 
        need_swap_bytes = (this_PC_type == BIGENDIAN) ? 1: 0;
        if (SeriesInstanceUID || (vh.gen.study_valid && vh.gen.series_valid))
        {
            if (SeriesInstanceUID == NULL)
            {
                SeriesInstanceUID = (char *)
                    malloc(strlen(vh.gen.study)+strlen(vh.gen.series)+8);
                sprintf(SeriesInstanceUID,"%s.%s", vh.gen.study,vh.gen.series);
                suid_len = (int)strlen(SeriesInstanceUID);
                if (vh.gen.data_type==IMAGE0 && vh.scn.dimension==4)
                {
                    sprintf(SeriesInstanceUID+suid_len,
                        ".%04d", volume+1);
                    suid_len = (int)strlen(SeriesInstanceUID);
                }
                SeriesInstanceUID[suid_len+1] = 0;
            }
            if (SOPInstanceUID == NULL)
                SOPInstanceUID = (char *)malloc(suid_len+7);
            strncpy(SOPInstanceUID, SeriesInstanceUID, suid_len);
            sprintf(SOPInstanceUID+suid_len, ".%d", i+1);
            put_dcm_data(fpout, 0x0008, 0x0018, AT, (char *)"UI",
                SOPInstanceUID, (int)strlen(SOPInstanceUID));
        }
        if (vh.gen.data_type == IMAGE0)
            volume = get_volume(vh.scn.dimension, vh.scn.num_of_subscenes, i);
        if (vh.gen.study_date_valid)
            put_dcm_data(fpout, 0x0008, 0x0020, AT, (char *)"SH", vh.gen.study_date,
                    (int)strlen(vh.gen.study_date));
        if (vh.gen.study_time_valid)
            put_dcm_data(fpout, 0x0008, 0x0030, AT, (char *)"SH", vh.gen.study_time,
                    (int)strlen(vh.gen.study_time));
        if (vh.gen.modality_valid)
            put_dcm_data(fpout, 0x0008, 0x0060, AT, (char *)"CS",
                vh.gen.modality, (int)strlen(vh.gen.modality));
        else
        {
            if (vh.gen.data_type==IMAGE0 && num_bits==1)
                sprintf(text, "%s", "SEG");
            else
                sprintf(text, "%s", "OT");
            put_dcm_data(fpout, 0x0008, 0x0060, AT, (char *)"CS",
                text, (int)strlen(text));
        }
		if (vh.scn.dimension==3 && vh.scn.num_of_subscenes[0]>1) {
			memset(text, 0, TEXT_SIZE);
			sprintf(text, "%e",
				vh.scn.loc_of_subscenes[1]-vh.scn.loc_of_subscenes[0]);
			put_dcm_data(fpout, 0x0018, 0x0050, AN, (char *)"DS", text, (int)strlen(text));
		}
        else if (vh.gen.slice_thickness_valid) {
            memset(text, 0, TEXT_SIZE);
            sprintf(text, "%e", vh.gen.slice_thickness);
            put_dcm_data(fpout, 0x0018, 0x0050, AN, (char *)"DS", text, (int)strlen(text));
        }

        if (vh.gen.kvp_valid) {
            sprintf(text, "%e", vh.gen.kvp[0]);
            put_dcm_data(fpout, 0x0018, 0x0060, AN, (char *)"DS", text, (int)strlen(text));
        }
        if (SeriesInstanceUID)
            put_dcm_data(fpout, 0x0020, 0x000e, AT, (char *)"UI",
                SeriesInstanceUID, suid_len);
        if (vh.gen.study_valid) {
            strcpy(text, vh.gen.study);
            put_dcm_data(fpout, 0x0020, 0x0010, AT, (char *)"SH", text, (int)strlen(text));
        }

        strcpy(text, "1 ");
        put_dcm_data(fpout, 0x0020, 0x0011, AT, (char *)"IS", text, (int)strlen(text));
        sprintf(text,"%d", i+1); /* image number        */
        put_dcm_data(fpout, 0x0020, 0x0012, AT, (char *)"IS", text, (int)strlen(text));
        sprintf(text,"%d", nslices-i); /* acquisition number        */
        put_dcm_data(fpout, 0x0020, 0x0013, AT, (char *)"IS", text, (int)strlen(text));
        if (vh.gen.data_type==IMAGE0)
        {
            sprintf(text, "%e", vh.scn.loc_of_subscenes[vh.scn.dimension==3?i:
                vh.scn.num_of_subscenes[0]+i]); /* slice location */
            put_dcm_data(fpout, 0x0020, 0x1041, AN, (char *)"DS", text, (int)strlen(text));
        }

        memcpy(text, &samples_per_pixel, 2);
        put_dcm_data(fpout, 0x0028, 0x0002, BI, (char *)"US", text, 2); 
        if (samples_per_pixel == 1) strcpy(text, "MONOCHROME2 ");
          else strcpy(text,"RGB ");
        put_dcm_data(fpout, 0x0028, 0x0004, AN, (char *)"CS", text, (int)strlen(text)); 

        ushort_num = 1;
        sprintf(text, "%d", ushort_num);
        put_dcm_data(fpout, 0x0028, 0x0008, AN, (char *)"IS", text, (int)strlen(text));

        if (vh.gen.data_type == IMAGE0) {
            memcpy(text,  &vh.scn.xysize[1], 2);
            put_dcm_data(fpout, 0x0028, 0x0010, BI, (char *)"US", text, 2);  /* row */
            memcpy(text, &vh.scn.xysize[0], 2);
            put_dcm_data(fpout, 0x0028, 0x0011, BI, (char *)"US", text, 2); /* column */
        } else {
            memcpy(text,  &vh.dsp.xysize[1], 2);
            put_dcm_data(fpout, 0x0028, 0x0010, BI, (char *)"US", text, 2);  /* row */
            memcpy(text, &vh.dsp.xysize[0], 2);
            put_dcm_data(fpout, 0x0028, 0x0011, BI, (char *)"US", text, 2); /* column */
        }

        if (vh.gen.data_type == IMAGE0) {
            sprintf(text,"%e\\%e", vh.scn.xypixsz[0], vh.scn.xypixsz[1]);
            put_dcm_data(fpout, 0x0028, 0x0030, AN, (char *)"DS", text, (int)strlen(text));
        } else {
            sprintf(text,"%e\\%e", vh.dsp.xypixsz[0], vh.dsp.xypixsz[1]);
            put_dcm_data(fpout, 0x0028, 0x0030, AN, (char *)"DS", text, (int)strlen(text));
        }

        if (vh.gen.data_type == IMAGE0) {
            memcpy(text, &num_bits, 2);
#ifdef BIT_EQUALS_BYTE
            if (num_bits == 1)
                ((unsigned short *)text)[0] = 8;
#endif
            put_dcm_data(fpout, 0x0028, 0x0100, BI, (char *)"US", text, 2);
            put_dcm_data(fpout, 0x0028, 0x0101, BI, (char *)"US", text, 2);
            ushort_num = (unsigned short) *vh.scn.largest_density_value;
            for (j=0; j < 16; j++)
                    if (ushort_num < (1 << j)) break;
            j -= 1;;
#ifdef VERBOSE
            printf("highest bits %d\n", j);
#endif
            ushort_num = (unsigned short)j;
            memcpy(text, &ushort_num, 2);
            put_dcm_data(fpout, 0x0028, 0x0102, BI, (char *)"US", text, 2);
        } else {
            ushort_num = 8;

            memcpy(text, &ushort_num, 2);
            put_dcm_data(fpout, 0x0028, 0x0100, BI, (char *)"US", text, 2);
            put_dcm_data(fpout, 0x0028, 0x0101, BI, (char *)"US", text, 2);
            for (j=0; j < 16; j++)
                    if (ushort_num < (1 << j)) break;
            j -= 1;;
            ushort_num = (unsigned short)j;
            memcpy(text, &ushort_num, 2);
            put_dcm_data(fpout, 0x0028, 0x0102, BI, (char *)"US", text, 2);
        }
        memset(text, 0, 2);
        put_dcm_data(fpout, 0x0028, 0x0103, BI, (char *)"SS", text, 2);
        if (vh.gen.data_type == IMAGE0) {
            if (vh.scn.smallest_density_value_valid) {
                ushort_num = (unsigned short) *vh.scn.smallest_density_value;
                memcpy(text, &ushort_num, 2); 
                put_dcm_data(fpout, 0x0028, 0x0108, BI, (char *)"US", text, 2);
            }
#ifdef BIT_EQUALS_BYTE
            if (num_bits == 1)
			{
				ushort_num = BIT_EQUALS_BYTE;
				memcpy(text, &ushort_num, 2);
				put_dcm_data(fpout, 0x0028, 0x0109, BD, (char *)"US", text, 2);
			}
			else
#endif
            if (vh.scn.largest_density_value_valid) {
                ushort_num = (unsigned short) *vh.scn.largest_density_value;
                memcpy(text, &ushort_num, 2); 
                put_dcm_data(fpout, 0x0028, 0x0109, BD, (char *)"US", text, 2);
            }

            if (vh.scn.axis_label_valid) {
                strcpy(text, (char *) vh.scn.axis_label);
                put_dcm_data(fpout, 0x0028, 0x1100, AN, (char *)"CS", text, (int)strlen(text));
            }
        }
        data_size = image_size;
#ifdef BIT_EQUALS_BYTE
        if (num_bits == 1) data_size = vh.scn.xysize[0]*vh.scn.xysize[1]; else
#endif
        if ((vh.gen.data_type == MOVIE0) && (num_bits == 8)) data_size *= 3;
         memcpy(text, &data_size, 4); 
        put_dcm_data(fpout, 0x7fe0, 0x0000, AN, (char *)"UL", text, 4);
         memcpy(text, &data_size, 4); 
        put_dcm_data(fpout, 0x7fe0, 0x0010, OW, (char *)"OW", text, 4); 
        VLSeekData(fpin, (double)i*image_size);
        VReadData((char *)data, 1, image_size, fpin, &j);
      
        if ((vh.gen.data_type == MOVIE0) &&  (num_bits == 8)) {
           data_size = image_size*3;
           out_data=(unsigned char *)malloc(data_size);
           assert (out_data != NULL );
           for(out_ptr=out_data,in_ptr=data,j=0;j <image_size;in_ptr++,j++, out_ptr +=3) {
             *out_ptr = ctable[*in_ptr][0];
             *(out_ptr+1) = ctable[*in_ptr][1];
             *(out_ptr+2) = ctable[*in_ptr][2];
           }
           ni = (int)fwrite (out_data, 1, data_size, fpout);
           check_IO (ni == data_size);
           free(out_data);
        }
        else {
#ifdef BIT_EQUALS_BYTE
          if (num_bits == 1 ) {
            for (j=0; j < image_size; j++) 
              for (k=0; k < 8; k++) { 
                if (8*j+k > data_size)
                  break;
                else if ( data[j] & (0x80 >> k) ) 
                  ni= (int)fwrite(&uc_data_1, 1, 1, fpout);
                else 
                  ni= (int)fwrite(&uc_data_0, 1, 1, fpout);
                check_IO (ni == 1); 
              }
          } else
#endif
          if (num_bits == 16) {
             swap2bytes(data, image_size);
             ni = (int)fwrite (data, 1, image_size, fpout);
             check_IO(ni == image_size);
          } else { 
             ni = (int)fwrite (data, 1, image_size, fpout);
             check_IO(ni == image_size) ; 
          }
#ifdef VERBOSE
          printf("%d bytes written\n", ni);
#endif
        }
      fclose( fpout );    fpout=NULL;
      fflush( stdout );    fflush( stderr );
    }
    if (mode == 1)
        VDeleteBackgroundProcessInformation();
    if (data)
    free(data);
    fclose(fpin);
    exit(0);
}

int get_slices(int dim, short *list)
{
  int i,sum;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sum=0,i=0;i<list[0];i++)
      sum+= list[1+i];
    return(sum);
  }
  return(0);
}

/* 10/11/07 Dewey Odhner */
int get_volume(int dim, short *list, int slice)
{
	int i, sum;

	if (dim == 3)
		return 0;
	if (dim == 4) {
		for(sum=0,i=0; i<list[0]; i++)
		{
			sum+= list[1+i];
			if (sum > slice)
				return i;
		}
	}
	return(0);
}

int put_dcm_data(FILE *fp, unsigned short us1, unsigned short us2, int type,
	char *vr, char *str, unsigned int len)
{
  unsigned short short_len;

#ifdef VERBOSE
  printf("(group, element)=(%X, %X) (%d): ", us1, us2, len ); 
  for (int i=0; i<(int)len; i++) {
     if (isprint(str[i]))    printf("%c",   str[i] );
     else                    printf("(%d)", str[i] );
  }
  printf("\n" );  fflush( stdout );
#endif

  fwrite((void *)&us1, 1, 2, fp); /* Group */
  fwrite((void *)&us2, 1, 2, fp); /* Element  */
  fwrite( vr, 1, 2, fp);  /* VR  */

  if (len & 1) { /*  odd length.  */
	str[len++] = ' '; 
  }

  if (type == OW) {
    short_len = 0;
  }
  else {
	short_len = len;
  }
  fwrite((void *) &short_len, 1, 2, fp);

  fwrite((void *)str, 1, len, fp);
  return (0);
} 

int BuildColorLookup(unsigned char ct[256][3], ViewnixHeader *vh)
{
  int i,shift,len;

  if (vh->gen.red_descriptor_valid && vh->gen.green_descriptor_valid && vh->gen.blue_descriptor_valid) {

    len=vh->gen.red_descriptor[0]+vh->gen.red_descriptor[1];
    shift= vh->gen.red_descriptor[2] - 8;
    for(i=vh->gen.red_descriptor[1]; i< len; i++) {
      ct[i][0]   = vh->gen.red_lookup_data[i]   >>  shift;
      ct[i][1]   = vh->gen.green_lookup_data[i] >>  shift;
      ct[i][2]   = vh->gen.blue_lookup_data[i]  >>  shift;
    }
  }
  else {
    for(i=0;i<256;i++)
      ct[i][0]=ct[i][1]=ct[i][2]=i;
  }
  return (0);
}

void check_IO(bool c)
{
	if (!c)
	{
		fprintf(stderr, "I/O failure\n");
		exit(-1);
	}
}
