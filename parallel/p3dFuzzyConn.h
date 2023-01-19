/*
  Copyright 1993-2009 Medical Image Processing Group
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

#define	DIETAG		1
#define STOPTAG         2
#define OBJINFO_WORKTAG		    3
#define SCALEINFO_WORKTAG		4
#define CHUNKINFO_WORKTAG		5
#define CHUNKDATA_WORKTAG		6
#define NUMSEEDS_WORKTAG        7
#define SEEDS_WORKTAG           8
#define SCALETAG        11
#define AFFNTAG         12
#define CONNTAG         13
#define RESULTTAG       21

#define MAX_OBJECTS     5
#define MAX_PROCESSOR   8
#define MAX_SEEDS       65535 /* temporary number */
#define CONN 4096

typedef struct {
  int   cols;
  int   rows;
  int   slices;
  int   begin_slice;
  int   end_slice;
  int   left_overlap;
  int   right_overlap;
  float pxsize_x;
  float pxsize_y;
  float thickness;
  int   num_of_bits;
  int   largest_intensity;
  char  affinity_flag;
  char  scale_flag;
  char  object_affn_flag;
} Chunk_str;


typedef struct {
  int     num_objects;
  int     mean_intensity[MAX_OBJECTS];
  double  sigma[MAX_OBJECTS];
  int     function[MAX_OBJECTS];
  int     function_mode[MAX_OBJECTS];
} Object_str;

typedef struct {
  int     max_scale;
  float   tolerance;
  double  sigma;
} Scale_str;


void     master(void);
void     slave(void);
void fuzzy_track();

void do_computation_work();
void scale_prepare();
void compute_feature_scale();
void compute_homogeneity();   
void compute_material();
void compute_affinity_feature_only();    
void compute_affinity();
void usage(int argc, char *argv[] );
