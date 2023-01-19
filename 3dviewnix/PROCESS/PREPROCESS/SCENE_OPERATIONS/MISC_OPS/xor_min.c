/*
  Copyright 1993-2014, 2016 Medical Image Processing Group
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

#include <stdio.h>
#include <math.h>
#include <cv3dv.h>
#include <assert.h>

int get_slices(int dim, short *list);


float Imax, Imin;
int Range;

int main(argc,argv)
int argc;
char *argv[];
{
  int j, bytes1, *size1, size2, error, *slic1, slic2,
	*start_x_offset, *start_y_offset, *start_z_offset,
	end_x_offset, end_y_offset, end_z_offset, x1, y1, z1, x2, y2, z2,
	x_offset, y_offset, z_offset, x_start, y_start, z_start,
	x_stop, y_stop, z_stop, best_x_offset, best_y_offset, best_z_offset,
	best_on_boundary=FALSE, on_left_boundary,
	y_step=1, Start_y_offset, End_z_offset,
	End_y_offset, num_objects=1, cur_object, multi_flag=0;
  double total, best_total, bin_volume=0;
  ViewnixHeader *vh1, *vh2;
  FILE *in1,*in2, *outstream;
  unsigned char **data1, *data2;
  char group[6],elem[6];
  unsigned short ***data2_rle, **rle_buf;

#define Handle_error(message) \
{ \
  fprintf(stderr, "%s\n", message); \
  exit(1); \
}


  if (argc>2 && strcmp(argv[argc-2], "-o")==0)
  {
    outstream = fopen(argv[argc-1], "wb");
    argc -= 2;
  }
  else
    outstream = stdout;
  if (argc>9 && strcmp(argv[1], "-multiple")==0 &&
      sscanf(argv[2], "%d", &num_objects)==1)
  {
    multi_flag = 1;
  }
  if (argc>9 && strncmp(argv[argc-1], "-xor", 4)==0)
  {
	if (strcmp(argv[argc-1], "-xors") == 0)
	  y_step = 3;
	argc--;
  }
  start_x_offset = (int *)malloc(num_objects*sizeof(int));
  start_y_offset = (int *)malloc(num_objects*sizeof(int));
  start_z_offset = (int *)malloc(num_objects*sizeof(int));
  if (argc!=(multi_flag?6+5*num_objects:9) ||
      sscanf(argv[3], "%d", start_x_offset)!=1 ||
	  sscanf(argv[4], "%d", start_y_offset)!=1 ||
	  sscanf(argv[5], "%d", start_z_offset)!=1 ||
	  sscanf(argv[3+3*num_objects], "%d", &end_x_offset)!=1 ||
	  sscanf(argv[4+3*num_objects], "%d", &end_y_offset)!=1 ||
	  sscanf(argv[5+3*num_objects], "%d", &end_z_offset)!=1 ||
	  *start_x_offset>end_x_offset ||
	  *start_y_offset>end_y_offset ||
	  *start_z_offset>end_z_offset)
  {
    fprintf(stderr,
"Usage: xor_min [<IM0> <BIM> | -multiple <num_objects>] <start_x_offset1> <start_y_offset1> <start_z_offset1> [<start_x_offset2> <start_y_offset2> <start_z_offset2>] ... <end_x_offset> <end_y_offset> <end_z_offset> [<IM0> <BIM>] ... [-xors]\n");
    exit(-1);
  }
  for (cur_object=1; cur_object<num_objects; cur_object++)
    if (sscanf(argv[3+3*cur_object], "%d", start_x_offset+cur_object)!=1 ||
	    sscanf(argv[4+3*cur_object], "%d", start_y_offset+cur_object)!=1 ||
		sscanf(argv[5+3*cur_object], "%d", start_z_offset+cur_object)!=1)
	  Handle_error("Cannot parse starting offsets.");

  vh1 = (ViewnixHeader *)calloc(num_objects, sizeof(*vh1));
  vh2 = (ViewnixHeader *)calloc(num_objects, sizeof(*vh2));
  data1 = (unsigned char **)malloc(num_objects*sizeof(*data1));
  data2_rle = (unsigned short ***)malloc(num_objects*sizeof(*data2_rle));
  rle_buf = (unsigned short **)malloc(num_objects*sizeof(*rle_buf));
  size1 = (int *)malloc(num_objects*sizeof(int));
  slic1 = (int *)malloc(num_objects*sizeof(int));

  best_x_offset = start_x_offset[0];
  best_y_offset = start_y_offset[0];
  best_z_offset = start_z_offset[0];
  best_total = 0;


  for (cur_object=0; cur_object<num_objects; cur_object++)
  {
    in1 = fopen(argv[multi_flag? argc-2*(num_objects-cur_object): 1], "rb");
    if (in1==NULL )
      Handle_error("Error in opening the input file");
    in2 = fopen(argv[multi_flag? argc-2*(num_objects-cur_object)+1: 2], "rb");
    if (in2==NULL )
      Handle_error("Error in opening the input file");

    error=VReadHeader(in1,&vh1[cur_object],group,elem);
    if (error>0 && error<=104)
      Handle_error("Fatal error in reading header");

    if (vh1[cur_object].gen.data_type!=IMAGE0)
      Handle_error("This is not an IMAGE0 file");

    bytes1 = vh1[cur_object].scn.num_of_bits/8;
    slic1[cur_object] = get_slices(vh1[cur_object].scn.dimension,
      vh1[cur_object].scn.num_of_subscenes);
    size1[cur_object] = (int)vh1[cur_object].scn.xysize[0]*vh1[cur_object].scn.xysize[1];
    if (bytes1 != 2)
      Handle_error("xor requires first scene of 16 bits.");

    error=VReadHeader(in2,&vh2[cur_object],group,elem);
    if (error>0 && error<=104)
      Handle_error("Fatal error in reading header");

    if (cur_object == 0)
	{
	  Imin = vh2[0].scn.smallest_density_value[0];
      Imax = vh2[0].scn.largest_density_value[0];
      Range = (int)(Imax-Imin);

      size2 = (int)vh2[0].scn.xysize[0]*vh2[0].scn.xysize[1];
      slic2 = get_slices(vh2[0].scn.dimension, vh2[0].scn.num_of_subscenes);
      if (vh2[0].scn.num_of_bits != 1)
        Handle_error("Second input file must be binary.");
    }
	else
	  if (vh2[cur_object].scn.num_of_bits!=vh2->scn.num_of_bits ||
	      get_slices(vh2[cur_object].scn.dimension,
		  vh2[cur_object].scn.num_of_subscenes)!=slic2 ||
		  vh2[cur_object].scn.xysize[0]!=vh2->scn.xysize[0] ||
		  vh2[cur_object].scn.xysize[1]!=vh2->scn.xysize[1])
		Handle_error("Scenes do not match.");

    data1[cur_object] =
      (unsigned char *)malloc(bytes1*size1[cur_object]*slic1[cur_object]);
    if (data1[cur_object]==NULL)
      Handle_error("Could not allocate data. Aborting fuzz_ops");

    VSeekData(in1,0);
    VSeekData(in2,0);
    error = VReadData((char *)data1[cur_object], bytes1,
      size1[cur_object]*slic1[cur_object], in1, &j);
    if (error)
      Handle_error("Could not read data");
    fclose(in1);
    if (cur_object == 0)
	  data2= (unsigned char *)
	    malloc((size2*vh2[cur_object].scn.num_of_bits+7)/8*slic2+1);
    if (data2==NULL)
      Handle_error("Could not allocate data. Aborting fuzz_ops");
    data2[(size2*vh2[cur_object].scn.num_of_bits+7)/8*slic2] = 0;
    error = VReadData((char *)data2, (vh2[cur_object].scn.num_of_bits+7)/8,
      (vh2[cur_object].scn.num_of_bits==1?(size2+7)/8:size2)*slic2, in2, &j);
    if (error)
      Handle_error("Could not read data");
    fclose(in2);

    {
      best_total +=
        65534.*size2*slic2+65534.*size1[cur_object]*slic1[cur_object];
      data2_rle[cur_object] = (unsigned short **)malloc(
        vh2->scn.xysize[1]*slic2*sizeof(unsigned short *));
      rle_buf[cur_object] =
	    (unsigned short *)malloc((vh2->scn.xysize[0]+1)*sizeof(short));
      if (data2_rle[cur_object]==NULL || rle_buf[cur_object]==NULL)
        Handle_error("Out of memory.");
      for (z2=j=0; z2<slic2; z2++)
      {
        int xbyte=(size2+7)/8*z2, mask=128, slice_volume=0,
		  byteval=data2[xbyte];

        for (y2=0; y2<vh2->scn.xysize[1]; y2++,j++)
        {
          int runs=0;
          for (x2=0; x2<vh2->scn.xysize[0]; x2++)
          {
            if (byteval & mask)
            {
              slice_volume++;
              if ((runs&1) == 0)
                rle_buf[cur_object][runs++] = x2;
            }
            else
            {
              if (runs & 1)
                rle_buf[cur_object][runs++] = x2;
            }
            mask >>= 1;
            if (mask == 0)
            {
              mask = 128;
              xbyte++;
              byteval = data2[xbyte];
            }
          }
          if (runs & 1)
            rle_buf[cur_object][runs++] = vh2->scn.xysize[0];
          rle_buf[cur_object][runs++] = 65535;
          data2_rle[cur_object][j] =
		    (unsigned short *)malloc(runs*sizeof(short));
          if (data2_rle[cur_object][j] == NULL)
            Handle_error("Out of memory.");
          memcpy(data2_rle[cur_object][j], rle_buf[cur_object],
		    runs*sizeof(short));
        }
        bin_volume += slice_volume;
      }
    }
  }

  for (z_offset=start_z_offset[0]; z_offset<=end_z_offset; z_offset++)
  {
    for (y_offset=start_y_offset[0]; y_offset<=end_y_offset; y_offset+=y_step)
    {
      on_left_boundary = TRUE;
      for (x_offset=start_x_offset[0]; x_offset<=end_x_offset; x_offset++)
      {
    	{
          if (x_offset == start_x_offset[0])
          {
            total = 65534*bin_volume;
            for (cur_object=0; cur_object<num_objects; cur_object++)
              for (z1=0; z1<slic1[cur_object]; z1++)
                for (y1=0; y1<vh1[cur_object].scn.xysize[1]; y1++)
                  for (x1=0; x1<vh1[cur_object].scn.xysize[0]; )
                  {
                    if (z1+z_offset>=0 && z1+z_offset<slic2 &&
                        y1+y_offset>=0 && y1+y_offset<vh2->scn.xysize[1] &&
                        x1+x_offset>=0 && x1+x_offset<vh2->scn.xysize[0])
                    {
                      x1 = vh2->scn.xysize[0]-x_offset;
                      continue;
                    }
                    total += ((unsigned short *)data1[cur_object])
                      [(z1*vh1[cur_object].scn.xysize[1]+y1)*vh1[cur_object].scn.xysize[0]+x1];
                    x1++;
                  }
          }
        }
        for (cur_object=0; cur_object<num_objects; cur_object++)
        {
          int xoffst, yoffst, zoffst;
          zoffst = z_offset+start_z_offset[cur_object]-start_z_offset[0];
          z_start = zoffst<0? 0: zoffst;
          z_stop = slic1[cur_object]+zoffst<slic2?
            slic1[cur_object]+zoffst:slic2;
          yoffst = y_offset+start_y_offset[cur_object]-start_y_offset[0];
          y_start = yoffst<0? 0:yoffst;
          y_stop = vh1[cur_object].scn.xysize[1]+yoffst<vh2->scn.xysize[1]?
                   vh1[cur_object].scn.xysize[1]+yoffst:vh2->scn.xysize[1];
          xoffst = x_offset+start_x_offset[cur_object]-start_x_offset[0];
          x_start = xoffst<0? 0:xoffst;
          x_stop = vh1[cur_object].scn.xysize[0]+xoffst<vh2->scn.xysize[0]?
                   vh1[cur_object].scn.xysize[0]+xoffst:vh2->scn.xysize[0];

          for (z1=(z2=z_start)-zoffst; z2<z_stop; z2++,z1++)
          {
           j = z2*vh2->scn.xysize[1]+y_start;
           for (y1=(y2=y_start)-yoffst; y2<y_stop; y2++,y1++,j++)
           {
             int row_total=0, runs=0, isin;
             if (xoffst == start_x_offset[cur_object])
             {
               for (isin=0; data2_rle[cur_object][j][runs]<x_start; isin=!isin)
                 runs++;
               for (x1=(x2=x_start)-xoffst; x2<x_stop; x2++,x1++)
               {
                 int val1=((unsigned short *)data1[cur_object])
                   [(z1*vh1[cur_object].scn.xysize[1]+y1)*
                   vh1[cur_object].scn.xysize[0]+x1];
                 if (data2_rle[cur_object][j][runs] == x2)
                 {
                   runs++;
                   isin = !isin;
                 }
                 if (val1)
                   row_total += isin? -val1: val1;
               }
             }
             else
             {
               for (isin=1; (x2=data2_rle[cur_object][j][runs]) < x_stop;
                   runs++,isin= !isin)
               {
                 int val1;
                 if (x2 < x_start)
                   continue;
                 x1 = x2-xoffst;
                 val1 = ((unsigned short *)data1[cur_object])
                   [(z1*vh1[cur_object].scn.xysize[1]+y1)*
                   vh1[cur_object].scn.xysize[0]+x1];
                 if (val1)
                   row_total += 2*(isin? -val1: val1);
               }
             }
             total += row_total;
           }
          }
        }
        if (total < best_total)
        {
          best_total = total;
          best_x_offset = x_offset;
          best_y_offset = y_offset;
          best_z_offset = z_offset;
        }
        on_left_boundary = FALSE;
      }
    }
    if (y_step==3 && z_offset>=end_z_offset)
    {
      Start_y_offset = start_y_offset[0];
      End_z_offset = end_z_offset;
      End_y_offset = end_y_offset;
      z_offset = best_z_offset-1;
      end_z_offset = best_z_offset;
      start_y_offset[0] = best_y_offset-1;
      end_y_offset = best_y_offset+1;
      y_step = 2;
    }
  }
  if (y_step > 1)
  {
    start_y_offset[0] = Start_y_offset;
    end_z_offset = End_z_offset;
    end_y_offset = End_y_offset;
  }

  if (best_z_offset==start_z_offset[0] || best_z_offset==end_z_offset-1 ||
      best_y_offset==start_y_offset[0] || best_y_offset==end_y_offset-1 ||
      best_x_offset==start_x_offset[0] || best_x_offset==end_x_offset-1 ||
      best_on_boundary)
  {
    fprintf(stderr, "Optimum lies on search region boundary!\n");
    fprintf(stderr, "range: [%d, %d] x [%d, %d] x [%d, %d]\n",
      start_x_offset[0], end_x_offset,
      start_y_offset[0], end_y_offset,
      start_z_offset[0], end_z_offset);
  }

  fprintf(outstream,"%d %d %d\n", best_x_offset, best_y_offset, best_z_offset);
  fprintf(outstream, "%f\n", best_total);
  exit(0);
}

/*****************************************************************************
 * FUNCTION: get_slices
 * DESCRIPTION: Returns total number of slices in a scene.
 * PARAMETERS:
 *    dim: The dimension of the scene.
 *    list: The list containing the number of subscenes in the file.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The total number of slices in the file.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: unknown
 *
 *****************************************************************************/
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
