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

#define MAX_NUM_FEATURES 7 
#define MAX_NUM_TRANSFORMS 6 
#define OFF 0
#define ON 1

#define ABS(a) ((a) < 0? -(a): (a))

struct FeatureList {
	
	int status;
	int transform;
	float weight;
	float rmin;
	float rmax;
	float rmean;
	float rstddev;
	  /* r-prefix ==> ratio & not actual value */

};

int get_slices(int dim, short *list);
unsigned short edge_cost(int a, int b, int c, int d, int e, int f);
void LoadFeatureList(const char *feature_def_file, int object_number);
float fld_edge_cost(int a, int b, int c, int d, int e, int f);


struct FeatureList temp_list[MAX_NUM_FEATURES];
float Imax, Imin;
int Range;
int fld_flag;
float fld_weight[3];

int main(argc,argv)
int argc;
char *argv[];
{
  double penalty=65535, loc_penalty=0;
  int j, bytes1, slices, size1,size2, error, slic1, slic2,
	start_x_offset, start_y_offset, start_z_offset,
	end_x_offset, end_y_offset, end_z_offset, x1, y1, z1, x2, y2, z2,
	x_offset, y_offset, z_offset, x_start, y_start, z_start,
	x_stop, y_stop, z_stop, best_x_offset, best_y_offset, best_z_offset,
	object_number=1, edge_count, orientation='/', first_pass=TRUE,
	best_on_boundary=FALSE, last_was_best, on_left_boundary, tot_nedges=0,
	all_edges=FALSE, xor_flag=FALSE, y_step=1, Start_y_offset, End_z_offset,
	End_y_offset, inout_flag=FALSE, num_objects=1, cur_object, multi_flag=0,
	options;
  double total, best_total, deviation, bin_volume=0, inout_total, outin_total;
  double xsharp=1, ysharp=1, zsharp=1;
  ViewnixHeader *vh1, *vh2;
  FILE *in1,*in2, *outstream;
  unsigned char **data1, *data2;
  char group[6],elem[6];
  unsigned short ***h_edge_cost, ***v_edge_cost;
  unsigned short **h_nedges, **v_nedges, ***h_edge, ***v_edge, *temp_edge;
  unsigned short **t_nedges, **l_nedges, ***t_edge, ***l_edge, *temp_edge2;
  unsigned short **b_nedges, **r_nedges, ***b_edge, ***r_edge;
  char ***h_edge_dir, ***v_edge_dir;
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
  if (argc>9 && strncmp(argv[argc-1], "-inout", 4)==0)
  {
    inout_flag = TRUE;
	argc--;
  }
  if (argc>9 && strncmp(argv[argc-1], "-xor", 4)==0)
  {
    xor_flag = TRUE;
	if (strcmp(argv[argc-1], "-xors") == 0)
	  y_step = 3;
	argc--;
  }
  if (argc>9 && strcmp(argv[argc-1], "-a")==0)
  {
    all_edges = TRUE;
	argc--;
  }
  options = argc-9-(multi_flag? 2*num_objects: 0);
  if (argc<9 ||
      sscanf(argv[3], "%d", &start_x_offset)!=1 ||
	  sscanf(argv[4], "%d", &start_y_offset)!=1 ||
	  sscanf(argv[5], "%d", &start_z_offset)!=1 ||
	  sscanf(argv[6], "%d", &end_x_offset)!=1 ||
	  sscanf(argv[7], "%d", &end_y_offset)!=1 ||
	  sscanf(argv[8], "%d", &end_z_offset)!=1 ||
	  start_x_offset>end_x_offset ||
	  start_y_offset>end_y_offset ||
	  start_z_offset>end_z_offset ||
	  (options && sscanf(argv[10], "%d", &object_number)!=1) ||
	  (options>=2 && sscanf(argv[12], "%lf", &loc_penalty)!=1) || options>2)
  {
    fprintf(stderr,
"Usage: overlap_peak [<IM0> <BIM> | -multiple <num_objects>] <start_x_offset> <start_y_offset> <start_z_offset> <end_x_offset> <end_y_offset> <end_z_offset> [<feature_file> [<object_number> [[+|-|/]<penalty> [<loc_penalty>]]]] [<IM0> <BIM>] ... [-a] [-xor] [-inout] [-o <os>]\n");
    exit(-1);
  }

  if (!multi_flag && argc>=12)
  {
    if (object_number == 0)
	  orientation = '+';
	else
    {
      orientation = argv[11][0];
	  sscanf(argv[11]+1, "%lf", &penalty);
    }
  }
  if (options)
    LoadFeatureList(argv[9], object_number);

  vh1 = (ViewnixHeader *)calloc(num_objects, sizeof(*vh1));
  vh2 = (ViewnixHeader *)calloc(num_objects, sizeof(*vh2));
  data1 = (unsigned char **)malloc(num_objects*sizeof(*data1));
  data2_rle = (unsigned short ***)malloc(num_objects*sizeof(*data2_rle));
  rle_buf = (unsigned short **)malloc(num_objects*sizeof(*rle_buf));

  best_x_offset = start_x_offset;
  best_y_offset = start_y_offset;
  best_z_offset = start_z_offset;
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

    if (cur_object == 0)
	{
      bytes1 = vh1->scn.num_of_bits/8;
      slices = slic1= get_slices(vh1->scn.dimension,vh1->scn.num_of_subscenes);
      size1 = (int)vh1->scn.xysize[0]*vh1->scn.xysize[1];
      if (vh1->scn.num_of_bits==1 || (options && vh1->scn.num_of_bits!=16))
        Handle_error("First input file must not be binary.");
      if ((xor_flag||inout_flag) && bytes1!=2)
        Handle_error("xor requires first scene of 16 bits.");
    }
	else
	  if (vh1[cur_object].scn.num_of_bits!=vh1->scn.num_of_bits ||
	      get_slices(vh1[cur_object].scn.dimension,
		  vh1[cur_object].scn.num_of_subscenes)!=slic1 ||
		  vh1[cur_object].scn.xysize[0]!=vh1->scn.xysize[0] ||
		  vh1[cur_object].scn.xysize[1]!=vh1->scn.xysize[1])
	    Handle_error("Scenes do not match.");

    error=VReadHeader(in2,&vh2[cur_object],group,elem);
    if (error>0 && error<=104)
      Handle_error("Fatal error in reading header");

    if (cur_object == 0)
	{
	  Imin = vh2[0].scn.smallest_density_value[0];
      Imax = vh2[0].scn.largest_density_value[0];
      Range = (int)(Imax-Imin);

      size2 = (int)vh2[0].scn.xysize[0]*vh2[0].scn.xysize[1];
      if ((slic2=get_slices(vh2[0].scn.dimension,vh2[0].scn.num_of_subscenes))> slices)
        slices = slic2;
      if ((vh2[0].scn.num_of_bits==1) != !options)
        Handle_error("Second input file must be binary.");
    }
	else
	  if (vh2[cur_object].scn.num_of_bits!=vh2->scn.num_of_bits ||
	      get_slices(vh2[cur_object].scn.dimension,
		  vh2[cur_object].scn.num_of_subscenes)!=slic2 ||
		  vh2[cur_object].scn.xysize[0]!=vh2->scn.xysize[0] ||
		  vh2[cur_object].scn.xysize[1]!=vh2->scn.xysize[1])
		Handle_error("Scenes do not match.");

    data1[cur_object]= (unsigned char *)malloc(bytes1*size1*slic1);
    if (data1[cur_object]==NULL)
      Handle_error("Could not allocate data. Aborting fuzz_ops");

    if (options)
    {
      if (!fld_flag)
      {
        h_edge_cost = (unsigned short ***)malloc(slic2*sizeof(short **));
        v_edge_cost = (unsigned short ***)malloc(slic2*sizeof(short **));
        for (z2=0; z2<slic2; z2++)
        {
          h_edge_cost[z2] =
            (unsigned short **)malloc((vh2[cur_object].scn.xysize[1]-1)*sizeof(short *));
          for (y2=0; y2<vh2[cur_object].scn.xysize[1]-1; y2++)
            if ((h_edge_cost[z2][y2] = (unsigned short *)
                calloc(vh2[cur_object].scn.xysize[0]-2, sizeof(short))) == NULL)
              Handle_error("Out of memory.");
          v_edge_cost[z2] =
            (unsigned short **)malloc((vh2[cur_object].scn.xysize[1]-2)*sizeof(short *));
          for (y2=0; y2<vh2[cur_object].scn.xysize[1]-2; y2++)
            if ((v_edge_cost[z2][y2] = (unsigned short *)
                calloc(vh2[cur_object].scn.xysize[0]-1, sizeof(short))) == NULL)
              Handle_error("Out of memory.");
        }
      }
      if (orientation != '/')
      {
        if (!fld_flag)
        {
         h_edge_dir = (char ***)malloc(slic2*sizeof(char **));
         v_edge_dir = (char ***)malloc(slic2*sizeof(char **));
         for (z2=0; z2<slic2; z2++)
         {
          h_edge_dir[z2]=(char **)malloc((vh2[cur_object].scn.xysize[1]-1)*sizeof(char *));
          for (y2=0; y2<vh2[cur_object].scn.xysize[1]-1; y2++)
            if ((h_edge_dir[z2][y2] = (char *)
                calloc(vh2[cur_object].scn.xysize[0]-2, 1)) == NULL)
              Handle_error("Out of memory.");
          v_edge_dir[z2]=(char **)malloc((vh2[cur_object].scn.xysize[1]-2)*sizeof(char *));
          for (y2=0; y2<vh2[cur_object].scn.xysize[1]-2; y2++)
            if ((v_edge_dir[z2][y2] = (char *)
                calloc(vh2[cur_object].scn.xysize[0]-1, 1)) == NULL)
              Handle_error("Out of memory.");
         }
        }
        t_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        l_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        t_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        l_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        b_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        r_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        b_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        r_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        for (z1=0; z1<slic1; z1++)
        {
          t_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short));
          t_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short *));
          l_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short));
          l_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short *));
          b_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short));
          b_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short *));
          r_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short));
          r_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short *));
        }
        temp_edge2 = (unsigned short *)
		  malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short));
      }
      else // fld_flag
      {
        h_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        v_nedges = (unsigned short **)malloc(slic1*sizeof(short *));
        h_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        v_edge = (unsigned short ***)malloc(slic1*sizeof(short *));
        for (z1=0; z1<slic1; z1++)
        {
          h_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short));
          h_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short *));
          v_nedges[z1] =
            (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short));
          v_edge[z1] =
            (unsigned short **)malloc((vh1[cur_object].scn.xysize[1]-2)*sizeof(short *));
        }
      }
      temp_edge= (unsigned short *)malloc((vh1[cur_object].scn.xysize[1]-1)*sizeof(short));
    }

    VSeekData(in1,0);
    VSeekData(in2,0);
    error = VReadData((char *)data1[cur_object], bytes1, size1*slic1, in1, &j);
    if (error)
      Handle_error("Could not read data");
    fclose(in1);

    if (options)
    {
      for (z1=0; z1<slic1; z1++)
      {
        for (y1=0; y1<vh1[cur_object].scn.xysize[1]-1; y1++)
          switch (orientation)
          {
            case '/':
              h_nedges[z1][y1] = 0;
              for (x1=0; x1<vh1[cur_object].scn.xysize[0]-2; x1++)
                if ((((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)!=
                    (((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)
					)
                  temp_edge[h_nedges[z1][y1]++] = x1;
              if (h_nedges[z1][y1])
              {
                h_edge[z1][y1] =
                  (unsigned short *)malloc(h_nedges[z1][y1]*sizeof(short));
                memcpy(h_edge[z1][y1], temp_edge,
				  h_nedges[z1][y1]*sizeof(short));
                tot_nedges += h_nedges[z1][y1];
              }
              break;
            case '+':
            case '-':
              t_nedges[z1][y1] = b_nedges[z1][y1] = 0;
              for (x1=0; x1<vh1[cur_object].scn.xysize[0]-2; x1++)
                if ((((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)!=
                    (((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)
					)
                {
                  if (((unsigned short *)data1[cur_object])[(z1*vh1[cur_object].scn.xysize[1]+y1+1)*
				      vh1[cur_object].scn.xysize[0]+x1+1] > 32767)
                    temp_edge[t_nedges[z1][y1]++] = x1;
                  else
                    temp_edge2[b_nedges[z1][y1]++] = x1;
                }
              if (t_nedges[z1][y1])
              {
                t_edge[z1][y1] =
                  (unsigned short *)malloc(t_nedges[z1][y1]*sizeof(short));
                memcpy(t_edge[z1][y1], temp_edge,
				  t_nedges[z1][y1]*sizeof(short));
                tot_nedges += t_nedges[z1][y1];
              }
              if (b_nedges[z1][y1])
              {
                b_edge[z1][y1] =
                  (unsigned short *)malloc(b_nedges[z1][y1]*sizeof(short));
                memcpy(b_edge[z1][y1], temp_edge2,
				  b_nedges[z1][y1]*sizeof(short));
                tot_nedges += b_nedges[z1][y1];
              }
              break;
          }
        for (y1=0; y1<vh1[cur_object].scn.xysize[1]-2; y1++)
          switch (orientation)
          {
            case '/':
              v_nedges[z1][y1] = 0;
              for (x1=0; x1<vh1[cur_object].scn.xysize[0]-1; x1++)
                if ((((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1]>32767)!=
                    (((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)
					)
                  temp_edge[v_nedges[z1][y1]++] = x1;
              if (v_nedges[z1][y1])
              {
                v_edge[z1][y1] =
                  (unsigned short *)malloc(v_nedges[z1][y1]*sizeof(short));
                memcpy(v_edge[z1][y1], temp_edge,
				  v_nedges[z1][y1]*sizeof(short));
                tot_nedges += v_nedges[z1][y1];
              }
              break;
            case '+':
            case '-':
              l_nedges[z1][y1] = r_nedges[z1][y1] = 0;
              for (x1=0; x1<vh1[cur_object].scn.xysize[0]-1; x1++)
                if ((((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1]>32767)!=
                    (((unsigned short *)data1[cur_object])
                    [(z1*vh1[cur_object].scn.xysize[1]+y1+1)*vh1[cur_object].scn.xysize[0]+x1+1]>32767)
					)
                {
                  if (((unsigned short *)data1[cur_object])[(z1*vh1[cur_object].scn.xysize[1]+y1+1)*
				      vh1[cur_object].scn.xysize[0]+x1+1] > 32767)
                    temp_edge[l_nedges[z1][y1]++] = x1;
                  else
                    temp_edge2[r_nedges[z1][y1]++] = x1;
                }
              if (l_nedges[z1][y1])
              {
                l_edge[z1][y1] =
                  (unsigned short *)malloc(l_nedges[z1][y1]*sizeof(short));
                memcpy(l_edge[z1][y1], temp_edge,
				  l_nedges[z1][y1]*sizeof(short));
                tot_nedges += l_nedges[z1][y1];
              }
              if (r_nedges[z1][y1])
              {
                r_edge[z1][y1] =
                  (unsigned short *)malloc(r_nedges[z1][y1]*sizeof(short));
                memcpy(r_edge[z1][y1], temp_edge2,
				  r_nedges[z1][y1]*sizeof(short));
                tot_nedges += r_nedges[z1][y1];
              }
              break;
          }
      }
      free(data1[cur_object]);
      // cull edges
      if (!all_edges && tot_nedges>2050)
      {
        tot_nedges = (int)(RAND_MAX*2000./tot_nedges);
        for (z1=0; z1<slic1; z1++)
        {
          for (y1=0; y1<vh1[cur_object].scn.xysize[1]-1; y1++)
            switch (orientation)
            {
              case '/':
                if (h_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<h_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      h_edge[z1][y1][x2] = h_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(h_edge[z1][y1]);
                  h_nedges[z1][y1] = x2;
                }
                break;
              case '+':
              case '-':
                if (t_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<t_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      t_edge[z1][y1][x2] = t_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(t_edge[z1][y1]);
                  t_nedges[z1][y1] = x2;
                }
                if (b_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<b_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      b_edge[z1][y1][x2] = b_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(b_edge[z1][y1]);
                  b_nedges[z1][y1] = x2;
                }
                break;
            }
          for (y1=0; y1<vh1[cur_object].scn.xysize[1]-2; y1++)
            switch (orientation)
            {
              case '/':
                if (v_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<v_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      v_edge[z1][y1][x2] = v_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(v_edge[z1][y1]);
                  v_nedges[z1][y1] = x2;
                }
                break;
              case '+':
              case '-':
                if (l_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<l_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      l_edge[z1][y1][x2] = l_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(l_edge[z1][y1]);
                  l_nedges[z1][y1] = x2;
                }
                if (r_nedges[z1][y1])
                {
                  for (x1=x2=0; x1<r_nedges[z1][y1]; x1++)
                    if (rand() < tot_nedges)
                    {
                      r_edge[z1][y1][x2] = r_edge[z1][y1][x1];
                      x2++;
                    }
                  if (x2 == 0)
                    free(r_edge[z1][y1]);
                  r_nedges[z1][y1] = x2;
                }
                break;
            }
        }
      }
      else
        fprintf(stderr, "Using all %d edges\n", tot_nedges);
    }
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

    if (xor_flag || inout_flag)
    {
      best_total += 65534.*size2*slic2+65534.*size1*slic1;
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

  if (end_x_offset > start_x_offset)
    xsharp = 4./((end_x_offset-start_x_offset)*
                 (end_x_offset-start_x_offset));
  if (end_y_offset > start_y_offset)
    ysharp = 4./((end_y_offset-start_y_offset)*
                 (end_y_offset-start_y_offset));
  if (end_z_offset > start_z_offset)
    zsharp = 4./((end_z_offset-start_z_offset)*
                 (end_z_offset-start_z_offset));
  for (z_offset=start_z_offset; z_offset<=end_z_offset; z_offset++)
  {
    z_start = z_offset<0? 0:z_offset;
    z_stop = slic1+z_offset<slic2? slic1+z_offset:slic2;
    for (y_offset=start_y_offset; y_offset<=end_y_offset; y_offset+=y_step)
    {
      y_start = y_offset<0? 0:y_offset;
      y_stop = vh1->scn.xysize[1]+y_offset<vh2->scn.xysize[1]?
               vh1->scn.xysize[1]+y_offset:vh2->scn.xysize[1];
      on_left_boundary = TRUE;
      for (x_offset=start_x_offset; x_offset<=end_x_offset; x_offset++)
      {
    	x_start = x_offset<0? 0:x_offset;
    	x_stop = vh1->scn.xysize[0]+x_offset<vh2->scn.xysize[0]?
    	         vh1->scn.xysize[0]+x_offset:vh2->scn.xysize[0];
        if (xor_flag)
    	{
          if (x_offset == start_x_offset)
          {
            total = 65534*bin_volume;
            for (cur_object=0; cur_object<num_objects; cur_object++)
              for (z1=0; z1<slic1; z1++)
                for (y1=0; y1<vh1->scn.xysize[1]; y1++)
                  for (x1=0; x1<vh1->scn.xysize[0]; )
                  {
                    if (z1+z_offset>=0 && z1+z_offset<slic2 &&
                        y1+y_offset>=0 && y1+y_offset<vh2->scn.xysize[1] &&
                        x1+x_offset>=0 && x1+x_offset<vh2->scn.xysize[0])
                    {
                      x1 = vh2->scn.xysize[0]-x_offset;
                      continue;
                    }
                    total += ((unsigned short *)data1[cur_object])
                      [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                    x1++;
                  }
          }
        }
        else if (inout_flag)
        {
          if (x_offset == start_x_offset)
          {
            inout_total = 0;
            for (cur_object=0; cur_object<num_objects; cur_object++)
              for (z1=0; z1<slic1; z1++)
                for (y1=0; y1<vh1->scn.xysize[1]; y1++)
                  for (x1=0; x1<vh1->scn.xysize[0]; )
                  {
                    if (z1+z_offset>=0 && z1+z_offset<slic2 &&
                        y1+y_offset>=0 && y1+y_offset<vh2->scn.xysize[1] &&
                        x1+x_offset>=0 && x1+x_offset<vh2->scn.xysize[0])
                    {
                      x1 = vh2->scn.xysize[0]-x_offset;
                      continue;
                    }
                    inout_total += ((unsigned short *)data1[cur_object])
                      [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                    x1++;
                  }
            outin_total = 65534*bin_volume;
          }
        }
        else
          total = 0;
        edge_count = 0;
        for (z1=(z2=z_start)-z_offset; z2<z_stop; z2++,z1++)
         if (options)
         {
          deviation =
            (x_offset-.5*(start_x_offset+end_x_offset))*
            (x_offset-.5*(start_x_offset+end_x_offset))*xsharp+
            (y_offset-.5*(start_y_offset+end_y_offset))*
            (y_offset-.5*(start_y_offset+end_y_offset))*ysharp+
            (z_offset-.5*(start_z_offset+end_z_offset))*
            (z_offset-.5*(start_z_offset+end_z_offset))*zsharp;
          if (deviation > 1)
          {
            if (last_was_best)
              best_on_boundary = TRUE;
            last_was_best = FALSE;
            continue;
          }
          if (orientation == '/')
          {
           // horizontal edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-1; y2++,y1++)
            for (j=0; j<h_nedges[z1][y1]; j++)
            {
              x1 = h_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-2-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total += (h_edge_cost[z2][y2][x2]? h_edge_cost[z2][y2][x2]:
                (h_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2])
                  ));
            }

           // vertical edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-2; y2++,y1++)
            for (j=0; j<v_nedges[z1][y1]; j++)
            {
              x1 = v_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-1-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total += (v_edge_cost[z2][y2][x2]? v_edge_cost[z2][y2][x2]:
                (v_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1])
                  ));
            }

          }
          else if (fld_flag)
          {

           // horizontal edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-1; y2++,y1++)
           {
            for (j=0; j<t_nedges[z1][y1]; j++)
            {
              x1 = t_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-2-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total +=
                  vh2->scn.num_of_bits==16?
                  fld_edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2]):
                  fld_edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2]);
            }
            for (j=0; j<b_nedges[z1][y1]; j++)
            {
              x1 = b_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-2-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total +=
                  vh2->scn.num_of_bits==16?
                  fld_edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2]):
                  fld_edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2]);
            }
           }

           // vertical edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-2; y2++,y1++)
           {
            for (j=0; j<l_nedges[z1][y1]; j++)
            {
              x1 = l_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-1-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total +=
                  vh2->scn.num_of_bits==16?
                  fld_edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1]):
                  fld_edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1]);
            }
            for (j=0; j<r_nedges[z1][y1]; j++)
            {
              x1 = r_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-1-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              total +=
                  vh2->scn.num_of_bits==16?
                  fld_edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2]):
                  fld_edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2]);
            }
           }

          }
          else // orientation != '/' && !fld_flag
          {
           unsigned short tmp_cost;
           int tmp_dir;

           // horizontal edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-1; y2++,y1++)
           {
            for (j=0; j<t_nedges[z1][y1]; j++)
            {
              x1 = t_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-2-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              if (h_edge_dir[z2][y2][x2])
              {
               tmp_cost = h_edge_cost[z2][y2][x2];
               tmp_dir = h_edge_dir[z2][y2][x2];
              }
              else
              {
               tmp_cost =
                (h_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2])
                  );
               h_edge_dir[z2][y2][x2] = (char)(tmp_dir =
                 (vh2->scn.num_of_bits==16?
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1] :
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1])
                 ? '+': '-');
              }
              total += tmp_dir==orientation? tmp_cost: penalty;
            }
            for (j=0; j<b_nedges[z1][y1]; j++)
            {
              x1 = b_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-2-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              if (h_edge_dir[z2][y2][x2])
              {
               tmp_cost = h_edge_cost[z2][y2][x2];
               tmp_dir = h_edge_dir[z2][y2][x2];
              }
              else
              {
               tmp_cost =
                (h_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+2])
                  );
               h_edge_dir[z2][y2][x2] = (char)(tmp_dir =
                 (vh2->scn.num_of_bits==16?
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1] :
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1])
                 ? '+': '-');
              }
              total += tmp_dir!=orientation? tmp_cost: penalty;

            }
           }

           // vertical edges
           for (y1=(y2=y_start)-y_offset; y2<y_stop-2; y2++,y1++)
           {
            for (j=0; j<l_nedges[z1][y1]; j++)
            {
              x1 = l_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-1-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              if (v_edge_dir[z2][y2][x2])
              {
               tmp_cost = v_edge_cost[z2][y2][x2];
               tmp_dir = v_edge_dir[z2][y2][x2];
              }
              else
              {
               tmp_cost =
                (v_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1])
                  );
               v_edge_dir[z2][y2][x2] = (char)(tmp_dir =
                 (vh2->scn.num_of_bits==16?
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2] :
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2])
                 ? '+': '-');
              }
              total += tmp_dir==orientation? tmp_cost: penalty;
            }
            for (j=0; j<r_nedges[z1][y1]; j++)
            {
              x1 = r_edge[z1][y1][j];
              if (x1 < x_start-x_offset)
                continue;
              if (x1 >= x_stop-1-x_offset)
                break;
              x2 = x1+x_offset;
              edge_count++;
              if (v_edge_dir[z2][y2][x2])
              {
               tmp_cost = v_edge_cost[z2][y2][x2];
               tmp_dir = v_edge_dir[z2][y2][x2];
              }
              else
              {
               tmp_cost =
                (v_edge_cost[z2][y2][x2]=
                  vh2->scn.num_of_bits==16?
                  edge_cost(
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    ((unsigned short *)data2)
                    [(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1]):
                  edge_cost(
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2],
                    data2[(z2*vh2->scn.xysize[1]+y2)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1],
                    data2[(z2*vh2->scn.xysize[1]+y2+2)*vh2->scn.xysize[0]+x2+1])
                  );
               v_edge_dir[z2][y2][x2] = (char)(tmp_dir =
                 (vh2->scn.num_of_bits==16?
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 ((unsigned short *)data2)
                 [(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2] :
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2+1] >
                 data2[(z2*vh2->scn.xysize[1]+y2+1)*vh2->scn.xysize[0]+x2])
                 ? '+': '-');
              }
              total += tmp_dir!=orientation? tmp_cost: penalty;
            }
           }
          }

         }
         else // !options
         if (xor_flag)
         {
		  for (cur_object=0; cur_object<num_objects; cur_object++)
		  {
           j = z2*vh2->scn.xysize[1]+y_start;
           for (y1=(y2=y_start)-y_offset; y2<y_stop; y2++,y1++,j++)
           {
             int row_total=0, runs=0, isin;
             if (x_offset == start_x_offset)
             {
               for (isin=0; data2_rle[cur_object][j][runs]<x_start; isin= !isin)
                 runs++;
               for (x1=(x2=x_start)-x_offset; x2<x_stop; x2++,x1++)
               {
                 int val1=((unsigned short *)data1[cur_object])
                   [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
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
               for (isin=1; (x2=data2_rle[cur_object][j][runs]) < x_stop; runs++,isin=
                   !isin)
               {
                 int val1;
                 if (x2 < x_start)
                   continue;
                 x1 = x2-x_offset;
                 val1 = ((unsigned short *)data1[cur_object])
                   [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                 if (val1)
                   row_total += 2*(isin? -val1: val1);
               }
             }
             total += row_total;
           }
		  }
         }
         else // !xor_flag
         if (inout_flag)
         {
		  for (cur_object=0; cur_object<num_objects; cur_object++)
		  {
           j = z2*vh2->scn.xysize[1]+y_start;
           for (y1=(y2=y_start)-y_offset; y2<y_stop; y2++,y1++,j++)
           {
             int row_inout=0, row_outin=0, runs=0, isin;
             if (x_offset == start_x_offset)
             {
               for (isin=0; data2_rle[cur_object][j][runs]<x_start; isin= !isin)
                 runs++;
               for (x1=(x2=x_start)-x_offset; x2<x_stop; x2++,x1++)
               {
                 int val1=((unsigned short *)data1[cur_object])
                   [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                 if (data2_rle[cur_object][j][runs] == x2)
                 {
                   runs++;
                   isin = !isin;
                 }
                 if (val1)
                 {
                   if (isin)
                     row_outin -= val1;
                   else
                     row_inout += val1;
                 }
               }
             }
             else
             {
               for (isin=1; (x2=data2_rle[cur_object][j][runs]) < x_stop; runs++,isin=
                   !isin)
               {
                 int val1;
                 if (x2 < x_start)
                   continue;
                 x1 = x2-x_offset;
                 val1 = ((unsigned short *)data1[cur_object])
                   [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                 if (val1)
                 {
                   if (isin)
                     row_outin -= 2*val1;
                   else
                     row_inout += 2*val1;
                 }
               }
             }
             inout_total += row_inout;
             outin_total += row_outin;
           }
		  }
         }
         else // !xor_flag && !inout_flag
         {
          int xbyte=(size2+7)/8*z2+(vh2->scn.xysize[0]*y_start+x_start)/8;
          int mask=128>>((vh2->scn.xysize[0]*y_start+x_start)%8);
          int byte2val=data2[xbyte];
          for (y1=(y2=y_start)-y_offset; y2<y_stop; y2++,y1++)
          {
            int row_total=0;
            for (x1=(x2=x_start)-x_offset; x2<x_stop; x2++,x1++)
            {
              if (byte2val & mask)
              {
                if (bytes1 == 2)
                  row_total += ((unsigned short *)data1[0])
                    [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
                else
                  row_total += data1[0]
                    [(z1*vh1->scn.xysize[1]+y1)*vh1->scn.xysize[0]+x1];
              }
              mask >>= 1;
              if (mask == 0)
              {
                  mask = 128;
                xbyte++;
                byte2val = data2[xbyte];
              }
            }
            total += row_total;
          }
         }
        if (options)
        {
          if (edge_count && (first_pass || (total>0? (1+loc_penalty*deviation):
              1/(1+loc_penalty*deviation))*total/edge_count<best_total))
          {
            best_total = (total>0? (1+loc_penalty*deviation):
              1/(1+loc_penalty*deviation))*total/edge_count;
            best_x_offset = x_offset;
            best_y_offset = y_offset;
            best_z_offset = z_offset;
            first_pass = FALSE;
            best_on_boundary = on_left_boundary;
            last_was_best = TRUE;
          }
          else
            last_was_best = FALSE;
        }
        else
          if (xor_flag? total<best_total: inout_flag? inout_total<best_total &&
              outin_total<best_total: total>best_total)
          {
            best_total = inout_flag? inout_total>outin_total? inout_total:
              outin_total: total;
            best_x_offset = x_offset;
            best_y_offset = y_offset;
            best_z_offset = z_offset;
          }
        on_left_boundary = FALSE;
      }
    }
    if (y_step==3 && z_offset>=end_z_offset)
    {
      Start_y_offset = start_y_offset;
      End_z_offset = end_z_offset;
      End_y_offset = end_y_offset;
      z_offset = best_z_offset-1;
      end_z_offset = best_z_offset;
      start_y_offset = best_y_offset-1;
      end_y_offset = best_y_offset+1;
      y_step = 2;
    }
  }
  if (y_step > 1)
  {
    start_y_offset = Start_y_offset;
    end_z_offset = End_z_offset;
    end_y_offset = End_y_offset;
  }

#ifdef VERBOSE
  if (best_z_offset==start_z_offset || best_z_offset==end_z_offset-1 ||
      best_y_offset==start_y_offset || best_y_offset==end_y_offset-1 ||
      best_x_offset==start_x_offset || best_x_offset==end_x_offset-1 ||
      best_on_boundary)
  {
    fprintf(stderr, "Optimum lies on search region boundary!\n");
    fprintf(stderr, "range: [%d, %d] x [%d, %d] x [%d, %d]\n",
      start_x_offset, end_x_offset,
      start_y_offset, end_y_offset,
      start_z_offset, end_z_offset);
  }
#endif

  fprintf(outstream, "%d %d %d\n", best_x_offset, best_y_offset, best_z_offset);
  if (inout_flag)
    fprintf(outstream, "%f %f\n%f\n", inout_total, outin_total,
      inout_total>outin_total? inout_total: outin_total);
  else
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



/******************************************************************************
*******************************************************************************
*                                                                             *
*                                                                             *
*                                                                             *
*                                                                             *
*                             FEATURES                                        *
*                                                                             *
*                                                                             *
*                                                                             *
*                                                                             *
*******************************************************************************
*******************************************************************************/




/*****************************************************************************
 * FUNCTION: AbsGrad1
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 2-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |b - e|
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int AbsGrad1(int a, int b, int c, int d, int e, int f)
{
	return abs(b - e);
}
 
 
 



/*****************************************************************************
 * FUNCTION: AbsGrad2
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |(a+b+c) - (d+e+f)| * 1/3
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int AbsGrad2(int a, int b, int c, int d, int e, int f)
{
	return (int)((1.0/3) *  abs((a+b+c) - (d+e+f)));
}
 





/*****************************************************************************
 * FUNCTION: AbsGrad3
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |(0.5a+b+0.5c) - (0.5d+e+0.5f)|/2
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int AbsGrad3(int a, int b, int c, int d, int e, int f)
{
    return abs( (a+c-d-f)/2+(b-e) )/2;
}








/*****************************************************************************
 * FUNCTION: AbsGrad4
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = ( |a-e| + |b-d| + |b-f| + |c-e| )/4
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int AbsGrad4(int a, int b, int c, int d, int e, int f)
{
    return abs( (a-e) + abs(b-d) + abs(b-f) + abs(c-e) )/4;
}
 

 






/*****************************************************************************
 * FUNCTION: Density1
 * DESCRIPTION: Finds the higher of the densities on either side of an edge
 *              (viz., head of the gradient vector), and assigns that value
 *              to the edge feature.
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = 1/3 * [((a+b+c) > (d+e+f)) ? (a+b+c) : (d+e+f)]
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int Density1(int a, int b, int c, int d, int e, int f)
{
	return (a+b+c > d+e+f ? a+b+c : d+e+f)/3;
}









/*****************************************************************************
 * FUNCTION: Density2
 * DESCRIPTION: Finds the lower of the densities on either side of an edge
 *              (viz., tail of the gradient vector), and assigns that value
 *              to the edge feature.
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = 1/3 * [((a+b+c) < (d+e+f)) ? (a+b+c) : (d+e+f)]
 *
 * PARAMETERS:
 *     a, b, c, d, e, f: pixel values
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: the feature value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int Density2(int a, int b, int c, int d, int e, int f)
{
	return (a+b+c < d+e+f ? a+b+c : d+e+f)/3;
}



int Feature(int which, int a, int b, int c, int d, int e, int f)
{
	switch (which)
	{
		case 0:
			return Density1(a, b, c, d, e, f);
		case 1:
			return Density2(a, b, c, d, e, f);
		case 2:
			return AbsGrad1(a, b, c, d, e, f);
		case 3:
			return AbsGrad2(a, b, c, d, e, f);
		case 4:
			return AbsGrad3(a, b, c, d, e, f);
		case 5:
			return AbsGrad4(a, b, c, d, e, f);
		default:
			assert(FALSE);
			return -1;
	}
}

/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                                                                           **
**                                                                           **
**                            TRANSFORMS                                     **
**                                                                           **
**                                                                           **
**                                                                           **
**                                                                           **
*******************************************************************************
*******************************************************************************/
 
 
/*****************************************************************************
 * FUNCTION: InvLinearTransform
 * DESCRIPTION: Applies an InverseLinear Transform to convert Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |   +    
 *         COST |    +  
 *              |     +
 *              |      +
 *              |       +
 *              _________________
 *                  FEATURE
 *
 *       If x < fmin,   cost(x) =  Imax-Imin
 *       If x > fmax,   cost(x) =  Imax-Imin
 *       Else           cost(x) =  slope*(x-fmax)
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    featr_val: the feature value
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: temp_list, Imax, Imin, Range must be initialized.
 * RETURN VALUE: the edge cost value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int InvLinearTransform(int featr, int featr_val)
{
  float slope, Fmax, Fmin;

  slope = (float)(1.0/(temp_list[featr].rmin-temp_list[featr].rmax));
  
  if( featr==0 || featr==1 )
    {
      Fmin = Imin + (temp_list[featr].rmin*Range);
      Fmax = Imin + (temp_list[featr].rmax*Range);
    }
  else
    {
      Fmin = (temp_list[featr].rmin*Range);
      Fmax = (temp_list[featr].rmax*Range);
    }

  if (featr_val < Fmin )
    return (int)(Imax-Imin);
  else
    if(featr_val > Fmax )
      return (int)(Imax-Imin);
    else
      return (int)(slope*(featr_val-Fmax));
}






/*****************************************************************************
 * FUNCTION: InvGaussianTransform
 * DESCRIPTION: Applies an Inverse Gaussian to transform Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |   --             --
 *              |      \          /
 *              |       \        /
 *         COST |         \    /
 *              |           --
 *              ________________________
 *                     FEATURE
 *
 *        cost(x) = Imax - (Imax-Imin)* e**(-(x-mean)**2/(2*stddev**2) )
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    featr_val: the feature value
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: temp_list, Imax, Imin, Range must be initialized.
 * RETURN VALUE: the edge cost value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/4/10 for overlap_peak by Dewey Odhner.
 *
 *****************************************************************************/
int InvGaussianTransform(int featr, int featr_val)
{
  double square, fvar;
  float fmean;

  /** Setup these constants for the comptation **/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;

  fvar = -2.0*(temp_list[featr].rstddev*Range)*(temp_list[featr].rstddev*Range);

  /*** Use inverse Gaussian transform to compute Costs **/
  square= (featr_val-fmean)*(featr_val-fmean);
  return (int)rint(Range*(1.0 - exp((double)square/fvar)));
}







/*****************************************************************************
 * FUNCTION: LinearTransform
 * DESCRIPTION: Applies a Linear transform to convert Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |       +     
 *         COST |      +        
 *              |     +           
 *              |    +             
 *              |   +                
 *              _________________
 *                  FEATURE
 *
 *       If x < fmin,   cost(x) =  Imax-Imin
 *       If x > fmax,   cost(x) =  Imax-Imin
 *       Else           cost(x) =  slope*(x-fmin)
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    featr_val: the feature value
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: temp_list, Imax, Imin, Range must be initialized.
 * RETURN VALUE: the edge cost value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/3/10 for overlap_peak by Dewey Odhner
 *
 *****************************************************************************/
int LinearTransform(int featr, int featr_val)
{
    float slope, Fmax, Fmin;

    slope = (float)(1.0/(temp_list[featr].rmax-temp_list[featr].rmin));
 
	if( featr==0 || featr==1 )
	{
		Fmin = Imin + (temp_list[featr].rmin*Range);
		Fmax = Imin + (temp_list[featr].rmax*Range);
	}
	else
	{
        Fmin = (temp_list[featr].rmin*Range);
        Fmax = (temp_list[featr].rmax*Range);
    }

	if (featr_val < Fmin )
        return (int)(Imax-Imin);
    else
        if(featr_val > Fmax )
            return (int)(Imax-Imin);
        else
            return (int)(slope*(featr_val-Fmin));
}


/*****************************************************************************
 * FUNCTION: GaussianTransform
 * DESCRIPTION: Applies an Upright Gaussian to transform Edge Features
 *              into Edge Costs.
 *               
 *              |
 *              |           --
 *         COST |         /    \
 *              |       /        \
 *              |      /          \
 *              |   --             --
 *              ________________________
 *                     FEATURE
 *
 *        cost(x) = Imin + (Imax-Imin)* e**(-(x-mean)**2/(2*stddev**2) )
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    featr_val: the feature value
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: temp_list, Imax, Imin, Range must be initialized.
 * RETURN VALUE: the edge cost value
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 11/4/10 for overlap_peak by Dewey Odhner.
 *
 *****************************************************************************/
int GaussianTransform(int featr, int featr_val)
{
  double square, fvar;
  float fmean;

  /*** set up constants to speed up computation ***/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  fvar = -2.0*(temp_list[featr].rstddev*Range)*(temp_list[featr].rstddev*Range);

  /*** Use Gaussian transform to compute Costs **/
  square= (featr_val-fmean)*(featr_val-fmean);
  return (int)rint(Range*exp((double)square/fvar));
}


/* Modified: 5/23/08 HWHM set to specified stddev by Dewey Odhner.
 *    Modified: 11/4/10 for overlap_peak by Dewey Odhner.
 */
int HyperTransform(int featr, int featr_val)
{
  double Abx, K;
  double fmean;

  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  K = 1.0/(Range*temp_list[featr].rstddev+.01);
  Abx = fabs(featr_val-fmean);
  return (int)rint(Range/(K*Abx+1));
}
 
 
 

/* Modified: 5/23/08 HWHM set to specified stddev by Dewey Odhner.
 *    Modified: 11/4/10 for overlap_peak by Dewey Odhner.
 */
int InvHyperTransform(int featr, int featr_val)
{
  double Abx, K;
  double fmean;

  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  K = 1.0/(Range*temp_list[featr].rstddev+.01);
  Abx = fabs(featr_val-fmean);
  return (int)rint(Range*( 1.0 - 1/(K*Abx+1)));
}

int Transform(int featr, int featr_val)
{
	switch (temp_list[featr].transform)
	{
		case 0:
			return LinearTransform(featr, featr_val);
		case 1:
			return GaussianTransform(featr, featr_val);
		case 2:
			return InvLinearTransform(featr, featr_val);
		case 3:
			return InvGaussianTransform(featr, featr_val);
		case 4:
			return HyperTransform(featr, featr_val);
		case 5:
			return InvHyperTransform(featr, featr_val);
		default:
			assert(FALSE);
			return -1;
	}
}




#define LOAD_FAIL \
  { \
	fclose(fp); \
	fprintf(stderr, "Failed loading feature file.\n"); \
    return; \
  }

/* Modified: 4/11/97 file closed by Dewey Odhner. */
void LoadFeatureList(const char *feature_def_file, int object_number)
{
  int i, j;
  FILE *fp;

  fp=fopen(feature_def_file,"rb");
  fld_flag = object_number==0;
  if (fld_flag)
  {
    if (fscanf(fp, "%f %f %f", fld_weight, fld_weight+1, fld_weight+2) != 3)
	  LOAD_FAIL
  }
  else
  {
    if (fscanf(fp,"%d",&i)!=1 || i!=MAX_NUM_FEATURES)
      LOAD_FAIL
    for (j=0; j<object_number; j++)
    {
     for(i=0;i<MAX_NUM_FEATURES;i++) {
      if (fscanf(fp,"%d %d %f %f %f %f %f",
                 &temp_list[i].status,&temp_list[i].transform,
                 &temp_list[i].weight,&temp_list[i].rmin,
                 &temp_list[i].rmax,&temp_list[i].rmean,
                 &temp_list[i].rstddev)!=7)
      {
        fclose(fp);
        LOAD_FAIL
      }
     }
     if (MAX_NUM_FEATURES>=7)
       temp_list[6].status=OFF;
    }
  }
  fclose(fp);
}

float fld_edge_cost(int a, int b, int c, int d, int e, int f)
{
	float feature_val[3] /* hi, low, gradient */, grad2;
	int reverse;

	if (e > b)
	{
		feature_val[0] = (float)e;
		feature_val[1] = (float)b;
	}
	else
	{
		feature_val[0] = (float)b;
		feature_val[1] = (float)e;
	}
	grad2 = (float)(d+e+f-a-b-c);
	reverse = grad2<0;
	if (grad2 < 0)
		grad2 = -grad2;
	feature_val[2] = 1/(float)6*grad2+
		(float).1767767*(ABS(a-e)+ABS(b-d)+ABS(b-f)+ABS(c-e));
	if (reverse)
		feature_val[2] = -feature_val[2];
	return fld_weight[0]*feature_val[0]+
	       fld_weight[1]*feature_val[1]+
		   fld_weight[2]*feature_val[2];
}

unsigned short edge_cost(int a, int b, int c, int d, int e, int f)
{
	int j;
	double k=0;

	for (j=0; j<6; j++)
		if (temp_list[j].status)
			k+= temp_list[j].weight*Transform(j, Feature(j, a, b, c, d, e, f));
	return (unsigned short)k;
}
