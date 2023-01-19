/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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

 

/*********************************************************************
* History:
*	1999 Oct. T.I  created   MV0_to_tiff.c
*********************************************************************/
 
/*****************************************************************************
 * FUNCTION: MV0_to_tiff
 * DESCRIPTION: Converts a MV0 image file into a tiff file.
 * PARAMETERS:
 *    1st: MV0 file name
 *    2nd: first frame number 
 *    3rd: last frame number 
 *    4th: output file name (program adds 'tif' extension name)
 * SIDE EFFECTS: 
 *	None
 * ENTRY CONDITIONS: 
 * RETURN VALUE: 
 *	0:	if successful
 *	non 0:	if failed to create a tiff ouput file
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: October 1998 by T.I.
 *    Modified: January 1999, documented by T.I.
 *    Modified: 7/25/01 for SCRN file conversion by Dewey Odhner.
 *****************************************************************************/
 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tiffio.h"

#include <cv3dv.h>

#undef fseek

ViewnixHeader vh;
FILE *fp,*outfp;
int error;

char copyright_notice[]=
"TIFF library functions \n\
 Copyright (c) 1988-1996 Sam Leffler \n\
 Copyright (c) 1991-1996 Silicon Graphics, Inc. \n\
 \n\
 Permission to use, copy, modify, distribute, and sell this software and  \n\
 its documentation for any purpose is hereby granted without fee, provided \n\
 that (i) the above copyright notices and this permission notice appear in \n\
 all copies of the software and related documentation, and (ii) the names of \n\n\
 Sam Leffler and Silicon Graphics may not be used in any advertising or \n\
 publicity relating to the software without the specific, prior written \n\
 permission of Sam Leffler and Silicon Graphics. \n\
\n\
 THE SOFTWARE IS PROVIDED \"AS-IS\" AND WITHOUT WARRANTY OF ANY KIND,  \n\
 EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY  \n\
 WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.   \n\
\n\
 IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR \n\
 ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, \n\
 OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, \n\
 WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF  \n\
 LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE  \n\
 OF THIS SOFTWARE.";


void BuildColorLookup();
int GetSaveScreenImage();
int WriteTiff();

int main(argc,argv)
int argc;
char *argv[];
{
  
  int i,incr,st_frame,end_frame,fr,in_size,out_size,width,height,bytes;
  int SCRN_flag, red_shift, green_shift, blue_shift, j;
  unsigned char *in_data,*out_data,*in,*out;
  char outfile[150],grp[6],elem[6];
  static unsigned char ctable[256][3];

  if (argc != 5) {
    printf("Usage:%s <MV0_file> 1st_frame last_frame  <tiff_without_extension> \n",argv[0]);
    exit(-1);
  }

  fp=fopen(argv[1], "rb");
  if (fp==NULL) {
    printf("Could not open input file\n");
    exit(-1);
  }

  SCRN_flag = strcmp(argv[1]+strlen(argv[1])-4, "SCRN")==0;
  if (SCRN_flag)
  {
    error=VReadData((char *)&fr, 4, 1, fp, &i);
	if (error)
	{
	  fprintf(stderr, "%s: Read Error\n", argv[0]);
	  exit(-1);
	}
	vh.dsp.num_of_images = fr;
  }
  else
  {
    error=VReadHeader(fp,&vh,grp,elem);
    if (error && error!=106 && error!=107) {
      printf("Read Error %d in group %s element %s\n",error,grp,elem);
      exit(-1);
    }

    if (vh.gen.data_type!= MOVIE0) {
      printf("You must specify a movie file to convert\n");
      exit(-1);
    }
  }
  if (sscanf(argv[2],"%d",&st_frame)!=1) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
  if (sscanf(argv[3],"%d",&end_frame)!=1) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
  
  if (st_frame==-1 && end_frame==-1) {
    st_frame=0;
    end_frame=vh.dsp.num_of_images-1;
  }	  
  if (st_frame < 0 ||
      st_frame >= vh.dsp.num_of_images ) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }

  if (end_frame < 0 ||
      end_frame >= vh.dsp.num_of_images ) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
    
  
  if (SCRN_flag)
  {
  }
  else if (vh.dsp.num_of_bits==8 && vh.dsp.num_of_elems==1)  {
    BuildColorLookup(ctable,&vh);
    bytes=1;
  }
  else if (vh.dsp.num_of_bits==24 && vh.dsp.num_of_elems==3) {
    bytes=3;
  }
  else {
    printf("Cannot handle this MOVIE format\n");
    exit(-1);
  }

  if (end_frame < st_frame) 
    incr= -1;
  else 
    incr=1;

  if (!SCRN_flag)
  {
    width=vh.dsp.xysize[0];
    height=vh.dsp.xysize[1];
    in_size =width*height*bytes;
    out_size=width*height*3;
    in_data=(unsigned char *)malloc(in_size);
    if (bytes==1) 
      out_data=(unsigned char *)calloc(out_size,1);
    else
      out_data=in_data;
    if (in_data==NULL || out_data == NULL) {
      printf("Could Not allocate space\n");
      exit(-1);
    }
  }

  for(fr=st_frame;fr<=end_frame; fr+=incr) {
    printf("Computing tiff file for slice %d\n",fr);
 
    if (SCRN_flag)
	{ XImage ximage;
	  ViewnixColor *colormap_colors;
	  short s;
	  unsigned *u;

	  /* Read Current 3dviewnix frame */
	  if (GetSaveScreenImage(argv[1], fr+1, &ximage, &colormap_colors, &s)) {
        printf("Error in reading data\n");
        exit(-1);
      }
	  width = ximage.width;
	  height = ximage.height;
	  in_size = height*ximage.bytes_per_line;
      out_size = width*height*3;
      out_data = (unsigned char *)malloc(out_size);
      if (out_data == NULL) {
        fprintf(stderr, "Could Not allocate space\n");
        exit(-1);
      }

      /* Convert data */
      if (ximage.depth == 8) {
        for (j=0; j<height; j++)
		  for (out=out_data+3*width*j,
              in=(unsigned char *)ximage.data+ximage.bytes_per_line*j,
		      i=width; i>0; in++,out+=3,i--) {
		    out[0] = colormap_colors[*in].red>>8;
		    out[1] = colormap_colors[*in].green>>8;
		    out[2] = colormap_colors[*in].blue>>8;
          }
      }
	  else if (ximage.bitmap_pad == 32)
	  {
		for (red_shift=0; (ximage.red_mask>>red_shift&1)==0; red_shift++)
          ;
        for (green_shift=0; (ximage.green_mask>>green_shift&1)==0;
		    green_shift++)
          ;
        for (blue_shift=0; (ximage.blue_mask>>blue_shift&1)==0; blue_shift++)
          ;
        for (out=out_data,u=(unsigned *)ximage.data,i=width*height; i>0;
		    u++,out+=3,i--) {
		  out[0] = colormap_colors[(*u&ximage.red_mask)>>red_shift].red>>8;
		  out[1]=colormap_colors[(*u&ximage.green_mask)>>green_shift].green>>8;
		  out[2] = colormap_colors[(*u&ximage.blue_mask)>>blue_shift].blue>>8;
        }
	  }
	  else {
        fprintf(stderr, "Cannot handle this SCRN format\n");
        exit(-1);
      }

	  free(ximage.data);
	  free(colormap_colors);
	}
	else
	{
	  /* Read Current 3dviewnix frame */
      VSeekData(fp,in_size*fr);
      if (VReadData((char *)in_data,1,in_size,fp,&i)) {
        printf("Error in reading data\n");
        exit(-1);
      }

      /* Convert data */
      if (vh.dsp.num_of_bits==8) {
        for(out=out_data,in=in_data,i=in_size;i>0;in++,out+=3,i--) {
		  *out     = ctable[*in][0];
		  *(out+1) = ctable[*in][1];
		  *(out+2) = ctable[*in][2];
        }
      }
	}
    sprintf(outfile,"%s_%d.tif",argv[4],fr);
    WriteTiff(outfile,vh.dsp.num_of_bits,(short)width,(short)height,out_data);
    if (SCRN_flag)
	  free(out_data);
  }
  printf("done writing tiff files for frames %d through %d\n",st_frame,end_frame);
  fclose(fp);
  exit(0);
}


void BuildColorLookup(ct,vh)
unsigned char ct[256][3];
ViewnixHeader *vh;
{
  int i,shift, len;
  
  /* Generate Color Map */
  if (vh->gen.red_descriptor_valid &&
      vh->gen.green_descriptor_valid &&
      vh->gen.blue_descriptor_valid) {
    
    len=(vh->gen.red_descriptor[0]+vh->gen.red_descriptor[1]);
    shift= vh->gen.red_descriptor[2] - 8;
    for(i=vh->gen.red_descriptor[1]; i< len;
	i++) {
      ct[i][0]   = vh->gen.red_lookup_data[i]   >>  shift;
      ct[i][1]   = vh->gen.green_lookup_data[i] >>  shift;
      ct[i][2]   = vh->gen.blue_lookup_data[i]  >>  shift;
    }
  }
  else {
    for(i=0;i<256;i++)
      ct[i][0]=ct[i][1]=ct[i][2]=i;
  }
}

/*****************************************************************************
 * FUNCTION: WriteTiff
 * DESCRIPTION: Write a tiff output file
 * PARAMETERS:
 *    1st: tiff output file name
 *    2nd: number of bits used per pixel
 *    3rd: width of the image
 *    4th: height of the image
 *    5th: pointer to image data 
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: 
 * RETURN VALUE: 
 *	0:      if a tiff output file successfully written
 *	non 0:  if failed to produce a tiff output file
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: October 1998 by T.I.
 *    Modified: February 1999, documented by T.I.
 *
 *****************************************************************************/

int WriteTiff(outname,bits,xsize,ysize,data)
  char *outname;
  short bits;
  short xsize, ysize;
    unsigned char *data;
{
  uint16 photometric;
  uint32 rowsperstrip = (uint32) -1;
  double resolution = -1;
  unsigned char *buf;
  uint32 row;
  tsize_t linebytes;
  uint16 spp=3;
  TIFF *out;
  uint32 w, h;
  uint16 compression = COMPRESSION_LZW;


    w=xsize; h=ysize;
    photometric = PHOTOMETRIC_RGB;
    out =TIFFOpen(outname, "w");
    compression = COMPRESSION_PACKBITS;	/* default */
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH,  w);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
    linebytes = spp * w;
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
        TIFFDefaultStripSize(out, rowsperstrip));
    if (resolution > 0) {
    	TIFFSetField(out, TIFFTAG_XRESOLUTION, resolution);
    	TIFFSetField(out, TIFFTAG_YRESOLUTION, resolution);
    	TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    }
    buf = data;
    for (row = 0; row < h; row++) {
    	if (TIFFWriteScanline(out, buf, row, 0) < 0)
    		break;
        buf += linebytes;
    }
    (void) TIFFClose(out);
    return (0);
}

/*****************************************************************************
 * FUNCTION: GetSaveScreenImage
 * DESCRIPTION: Loads an image from a SCRN file.
 * PARAMETERS:
 *    scrn_file: The file name.
 *    frame: The frame number from 1 to be loaded.
 *    ximage: The image is loaded here.
 *    colormap_colors: The colormap is loaded here.
 *    entries: The number of colormap entries is loaded here.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The environment variable VIEWNIX_ENV must be properly set.
 * RETURN VALUE: 0, normal completion
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/26/99 by Dewey Odhner
 *
 *****************************************************************************/
int GetSaveScreenImage(scrn_file, frame, ximage, colormap_colors, entries)
char *scrn_file;
int frame;
XImage *ximage;
ViewnixColor **colormap_colors;
short *entries;
{
    FILE *fp;
    int totbytes, error, total_frame, i, items_read;
    unsigned long pixel;
    char flags;

    fp = fopen(scrn_file, "rb");
    if (fp == NULL) return(4);
    error = VReadData((char *)&total_frame,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    if (frame < 1 || frame > total_frame) {
      fclose(fp);
      return(209);
    }
    for (i=0; i<frame-1; i++) {
      /* Added by supun - to make it machine independent */
      error = VReadData((char *)&ximage->width,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->height,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->xoffset,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->format,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->byte_order,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->bitmap_unit,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error=VReadData((char *)&ximage->bitmap_bit_order,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->bitmap_pad,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->depth,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->bytes_per_line,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->bits_per_pixel,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->red_mask,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->green_mask,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = VReadData((char *)&ximage->blue_mask,sizeof(int),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }

        
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      totbytes = ximage->bytes_per_line*ximage->height;

      error = fseek(fp,totbytes,1L);
      if (error == -1) {
        fclose(fp);
        return(5);
      }

      error = VReadData((char *)entries,sizeof(short),1,fp, &items_read);
      if (items_read == 0) {
        fclose(fp);
        return(2);
      }
      error = fseek(fp,11**entries,1L);
      if (error == -1) {
        fclose(fp);
        return(5);
      }
    }
    /* Added by supun - to make it machine independent */
    error = VReadData((char *)&ximage->width,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->height,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->xoffset,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->format,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->byte_order,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->bitmap_unit,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->bitmap_bit_order,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->bitmap_pad,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->depth,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->bytes_per_line,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->bits_per_pixel,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->red_mask,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->green_mask,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    error = VReadData((char *)&ximage->blue_mask,sizeof(int),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      return(2);
    }
    
    
    totbytes = ximage->bytes_per_line*ximage->height;

    ximage->data = (char *)malloc(totbytes*sizeof(char));
    if (ximage->data == NULL) {
      fclose(fp);
      return(1);
    }
    error = (int)fread(ximage->data,sizeof(char),totbytes,fp);
    if (error == 0) {
      fclose(fp);
      free(ximage->data);
      return(2);
    }

    error = VReadData((char *)entries,sizeof(short),1,fp, &items_read);
    if (items_read == 0) {
      fclose(fp);
      free(ximage->data);
      return(2);
    }
    *colormap_colors = (ViewnixColor *)malloc(*entries*sizeof(ViewnixColor));
    if (*colormap_colors == NULL) {
      fclose(fp);
      free(ximage->data);
      return (1);
    }
    for(i=0;i<*entries;i++) {
      error = VReadData((char *)&pixel,sizeof(int),1,fp, &items_read);
      if (items_read != 1) {
        fclose(fp);
        free(ximage->data);
        free(*colormap_colors);
        return(2);
      }
      (*colormap_colors)[pixel&255].pixel =
        pixel;
      error = VReadData((char *)
        &(*colormap_colors)[pixel&255].red,
        sizeof(short),1,fp, &items_read);
      if (items_read != 1) {
        fclose(fp);
        free(ximage->data);
        free(*colormap_colors);
        return(2);
      }
      error = VReadData((char *)
        &(*colormap_colors)[pixel&255].green,
        sizeof(short),1,fp, &items_read);
      if (items_read != 1) {
        fclose(fp);
        free(ximage->data);
        free(*colormap_colors);
        return(2);
      }
      error = VReadData((char *)
        &(*colormap_colors)[pixel&255].blue,
        sizeof(short),1,fp, &items_read);
      if (items_read != 1) {
        fclose(fp);
        free(ximage->data);
        free(*colormap_colors);
        return(2);
      }
      error = VReadData((char *)
        &flags,
        sizeof(char),1,fp, &items_read);
      if (items_read != 1) {
        fclose(fp);
        free(ximage->data);
        free(*colormap_colors);
        return(2);
      }
    }
    fclose(fp);
    return (0);
}
