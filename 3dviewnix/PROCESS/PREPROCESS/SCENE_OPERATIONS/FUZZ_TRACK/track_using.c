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

int slice_from_input_index(int index, int volume);

/*****************************************************************************
 * FUNCTION: fuzzy_track
 * DESCRIPTION: Computes the connectedness values and stores them at out_data.
 * PARAMETERS:
 *    current_volume: volume number from zero in the subscene.
 * SIDE EFFECTS: Messages may be written to stdout.
 * ENTRY CONDITIONS: The variables feature_status, function_level,
 *    function_width, weight, function_selected, file_info, vh_in, in_data,
 *    bg_flag, out_affinity_flag, fuzzy_adjacency_flag, slice_spacing,
 *    points_filename must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 4/3/96 by Dewey Odhner
 *    Modified: 4/15/96 PushNeighbors fixed by Dewey Odhner
 *    Modified: 4/25/97 affinity image stored by Dewey Odhner
 *    Modified: 1/27/98 direction-specific affinity image stored
 *       by Dewey Odhner
 *    Modified: 2/24/98 points loaded from binary scene by Dewey Odhner
 *    Modified: 1/21/99 hashed heap used instead of queue by Dewey Odhner
 *    Modified: 2/1/99 connectivity used for hash value by Dewey Odhner
 *    Modified: 3/26/99 affinity computed during tracking by Dewey Odhner
 *
 *****************************************************************************/
void fuzzy_track(int current_volume)
{

    int j, k, slices_out, slice_size, counter;
	int toggle;
    int ei, x, y, z;
    Voxel cur;
	unsigned char *in_points_data;
	OutCellType *x_affinity, *y_affinity, *z_affinity, *affp[6];
    unsigned pmin, Pmax, afn;
	float x_adjacency, y_adjacency, z_adjacency, e_adjacency[6];
	FILE *fp_pts;

	toggle = 0;
	slices_out = 1+(file_info.max[0]-file_info.min[0])/file_info.incr[0];
    slice_size = vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
	x_adjacency = y_adjacency = z_adjacency = MAX_CONNECTIVITY;
	if (fuzzy_adjacency_flag)
	{
		if (vh_in.scn.xypixsz[0]<=vh_in.scn.xypixsz[1] &&
				vh_in.scn.xypixsz[0]<=slice_spacing)
		{
			y_adjacency *= vh_in.scn.xypixsz[0]/vh_in.scn.xypixsz[1];
			z_adjacency *= vh_in.scn.xypixsz[0]/slice_spacing;
		}
		else if (vh_in.scn.xypixsz[1]<=vh_in.scn.xypixsz[0] &&
				vh_in.scn.xypixsz[1]<=slice_spacing)
		{
			x_adjacency *= vh_in.scn.xypixsz[1]/vh_in.scn.xypixsz[0];
			z_adjacency *= vh_in.scn.xypixsz[1]/slice_spacing;
		}
		else
		{
			x_adjacency *= slice_spacing/vh_in.scn.xypixsz[0];
			y_adjacency *= slice_spacing/vh_in.scn.xypixsz[1];
		}
	}
	e_adjacency[0] = e_adjacency[3] = x_adjacency;
	e_adjacency[1] = e_adjacency[4] = y_adjacency;
	e_adjacency[2] = e_adjacency[5] = z_adjacency;
    H = Create_heap;
	if (H == NULL)
		handle_error(1);
    dimensions.xdim = vh_in.scn.xysize[0];
    dimensions.ydim = vh_in.scn.xysize[1];
    dimensions.zdim = 1+(file_info.max[0]-file_info.min[0])/file_info.incr[0];
    dimensions.slice_size = vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
	memset(out_data, 0, slices_out*slice_size*sizeof(OutCellType));
    for (j=0; j<num_points_picked; j++)
		if ((cur.z=slice_from_input_index(point_picked[j][2], current_volume))
				>= 0)
		{
	    	cur.x = point_picked[j][0];
	    	cur.y = point_picked[j][1];
	    	out_data[cur.z*slice_size+
				cur.y*vh_in.scn.xysize[0]+cur.x] = MAX_CONNECTIVITY;
        	if (Push_xyz(cur.x, cur.y, cur.z, MAX_CONNECTIVITY))
				handle_error(1);
    	}
	if (points_filename)
	{
		fp_pts = fopen(points_filename, "rb");
		if (fp_pts == NULL)
			handle_error(4);
		in_points_data = (unsigned char *)
			malloc((vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8);
		handle_error(VSeekData(fp_pts,
			(vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8*current_volume));
		for (cur.z=0; cur.z<slices_out; cur.z++)
		{
			handle_error(VReadData((char *)in_points_data, 1,
				(vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8, fp_pts, &j));
			counter = 0;
			j = 128;
			for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
				for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
				{
					if (in_points_data[counter] & j)
	    			{	out_data[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x]= MAX_CONNECTIVITY;
        				if (Push_xyz(cur.x, cur.y, cur.z,
								MAX_CONNECTIVITY))
							handle_error(1);
					}
					j >>= 1;
					if (j == 0)
					{
						j = 128;
						counter++;
					}
				}
		}
		free(in_points_data);
		fclose(fp_pts);
	}
	x_affinity =
		(OutCellType *)malloc(slices_out*slice_size*sizeof(OutCellType));
    y_affinity =
		(OutCellType *)malloc(slices_out*slice_size*sizeof(OutCellType));
	z_affinity =
		(OutCellType *)malloc((slices_out-1)*slice_size*sizeof(OutCellType));
	if (x_affinity==NULL || y_affinity==NULL || z_affinity==NULL)
	{
		if (x_affinity)
			free(x_affinity);
		if (y_affinity)
			free(y_affinity);
		if (z_affinity)
			free(z_affinity);
		if (!bg_flag)
		{
			printf(
			   "Unable to allocate affinity array; tracking on input data.\n");
			fflush(stdout);
		}
		if (affinity_type == 3)
		{
			fprintf(stderr, "Unable to allocate affinity array.\n");
			exit(1);
		}
		if (vh_in.scn.num_of_bits == 8)
		{
			while (!H_is_empty) {
	      		Pmax = out_data[Pop_xyz(&cur.x, &cur.y, &cur.z)];
      			for (ei = 0; ei < 6; ei++)
				{
                    x = cur.x + nbor[ei].x;
                    y = cur.y + nbor[ei].y;
                    z = cur.z + nbor[ei].z;
                    if (x >= 0 && x < dimensions.xdim &&
                        y >= 0 && y < dimensions.ydim &&
                        z >= 0 && z < dimensions.zdim)
                    {
                      j = cur.z * dimensions.slice_size +
					      cur.y * dimensions.xdim + cur.x;
					  k = z * dimensions.slice_size + y * dimensions.xdim + x;
					  afn= affinity(in_data_8[j],in_data_8[k],e_adjacency[ei],
					    cur.x, cur.y, cur.z, x, y, z, 0);
                      pmin = MIN(Pmax, afn);
                      if (pmin > out_data[k])
                      {
                        if (out_data[k] == 0)
                        {
				          if (Push_xyz(x, y, z, pmin))
					        handle_error(1);
                        }
				        else
                          Repush_xyz(x, y, z, pmin, out_data[k]);
                        out_data[k] = pmin;
                      }
                    }
				}
   	 		}
			if (num_points_picked==0 && points_filename==NULL)
			{
				if (out_affinity_flag[0])
				  for (cur.z=0; cur.z<slices_out; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]-1; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_8[j], in_data_8[j+1],
								x_adjacency, cur.x, cur.y, cur.z,
								cur.x+1, cur.y, cur.z, 0);
							out_data[j] = afn;
						}
				if (out_affinity_flag[1])
				  for (cur.z=0; cur.z<slices_out; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]-1; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_8[j],
								in_data_8[j+vh_in.scn.xysize[0]], y_adjacency,
								cur.x, cur.y, cur.z, cur.x, cur.y+1, cur.z, 0);
							if (afn > out_data[j])
							out_data[j] = afn;
						}
				if (out_affinity_flag[2])
				  for (cur.z=0; cur.z<slices_out-1; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_8[j],
								in_data_8[j+slice_size], z_adjacency,
								cur.x, cur.y, cur.z, cur.x, cur.y, cur.z+1, 0);
							if (afn > out_data[j])
							out_data[j] = afn;
						}
			}
		}
		else
		{
			while (!H_is_empty) {
	      		Pmax = out_data[Pop_xyz(&cur.x, &cur.y, &cur.z)];
      			for (ei = 0; ei < 6; ei++)
				{
                    x = cur.x + nbor[ei].x;
                    y = cur.y + nbor[ei].y;
                    z = cur.z + nbor[ei].z;
                    if (x >= 0 && x < dimensions.xdim &&
                        y >= 0 && y < dimensions.ydim &&
                        z >= 0 && z < dimensions.zdim)
                    {
                      j = cur.z * dimensions.slice_size +
					      cur.y * dimensions.xdim + cur.x;
					  k = z * dimensions.slice_size + y * dimensions.xdim + x;
					  afn = affinity(in_data_16[j], in_data_16[k],
					      e_adjacency[ei], cur.x, cur.y, cur.z, x, y, z, 0);
                      pmin = MIN(Pmax, afn);
                      if (pmin > out_data[k])
                      {
                        if (out_data[k] == 0)
                        {
				          if (Push_xyz(x, y, z, pmin))
					        handle_error(1);
                        }
				        else
                          Repush_xyz(x, y, z, pmin, out_data[k]);
                        out_data[k] = pmin;
                      }
                    }
				}
   	 		}
			if (num_points_picked==0 && points_filename==NULL)
			{
				if (out_affinity_flag[0])
				  for (cur.z=0; cur.z<slices_out; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]-1; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_16[j], in_data_16[j+1],
								x_adjacency, cur.x, cur.y, cur.z,
								cur.x+1, cur.y, cur.z, 0);
							out_data[j] = afn;
						}
				if (out_affinity_flag[1])
				  for (cur.z=0; cur.z<slices_out; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]-1; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_16[j],
								in_data_16[j+vh_in.scn.xysize[0]],y_adjacency,
								cur.x, cur.y, cur.z, cur.x, cur.y+1, cur.z, 0);
							if (afn > out_data[j])
							out_data[j] = afn;
						}
				if (out_affinity_flag[2])
				  for (cur.z=0; cur.z<slices_out-1; cur.z++)
					for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
						for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						{
							j=cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
							afn = affinity(in_data_16[j],
								in_data_16[j+slice_size], z_adjacency,
								cur.x, cur.y, cur.z, cur.x, cur.y, cur.z+1, 0);
							if (afn > out_data[j])
							out_data[j] = afn;
						}
			}
		}
	}
	else
	{
		memset(x_affinity, 255, slices_out*slice_size*sizeof(OutCellType));
		memset(y_affinity, 255, slices_out*slice_size*sizeof(OutCellType));
		memset(z_affinity, 255, (slices_out-1)*slice_size*sizeof(OutCellType));
		affp[0] = x_affinity;
		affp[1] = y_affinity;
		affp[2] = z_affinity;
		affp[3] = x_affinity-1;
		affp[4] = y_affinity-dimensions.xdim;
		affp[5] = z_affinity-dimensions.slice_size;
		counter = 0;
		while (!H_is_empty) {
			if (!bg_flag && counter--==0)
			{
				counter = 100000;
				if ((toggle = !toggle))
					printf("Tracking.\n");
				else
					printf("Tracking..\n");
				fflush(stdout);
			}
            Pmax = out_data[Pop_xyz(&cur.x, &cur.y, &cur.z)];
            for (ei = 0; ei < 6; ei++)
            {
              x = cur.x + nbor[ei].x;
              y = cur.y + nbor[ei].y;
              z = cur.z + nbor[ei].z;
              if (x >= 0 && x < dimensions.xdim &&
                  y >= 0 && y < dimensions.ydim &&
                  z >= 0 && z < dimensions.zdim)
              {
                j= cur.z*dimensions.slice_size + cur.y*dimensions.xdim + cur.x;
                k = z * dimensions.slice_size + y * dimensions.xdim + x;
                afn = affp[ei][j];
				if (afn == AFF_UNDEF)
				{
				  if (vh_in.scn.num_of_bits == 8)
				    afn = affinity(in_data_8[j],in_data_8[k], e_adjacency[ei],
					  cur.x, cur.y, cur.z, x, y, z, 0);
				  else
				    afn = affinity(in_data_16[j], in_data_16[k],
					      e_adjacency[ei], cur.x, cur.y, cur.z, x, y, z, 0);
				  affp[ei][j] = afn;
				}
                pmin = MIN(Pmax, afn);
                if (pmin > out_data[k])
                {
                  if (out_data[k] == 0)
                  {
				    if (Push_xyz(x, y, z, pmin))
					  handle_error(1);
                  }
				  else
                    Repush_xyz(x, y, z, pmin, out_data[k]);
                  out_data[k] = pmin;
                }
              }
            }
   	 	}
		if (num_points_picked==0 && points_filename==NULL)
		{
		  if (vh_in.scn.num_of_bits == 8)
		  {
			for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]-1; cur.x++)
						x_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_8[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_8[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x+1], x_adjacency,
							cur.x, cur.y, cur.z, cur.x+1, cur.y, cur.z, 0);
			for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]-1; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						y_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_8[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_8[cur.z*slice_size+
							(cur.y+1)*vh_in.scn.xysize[0]+cur.x], y_adjacency,
							cur.x, cur.y, cur.z, cur.x, cur.y+1, cur.z, 0);
			for (cur.z=0; cur.z<slices_out-1; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						z_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_8[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_8[(cur.z+1)*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x], z_adjacency,
							cur.x, cur.y, cur.z, cur.x, cur.y, cur.z+1, 0);
		  }
		  else
		  {
			if (out_affinity_flag[0])
			  for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]-1; cur.x++)
						x_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_16[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_16[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x+1], x_adjacency,
							cur.x, cur.y, cur.z, cur.x+1, cur.y, cur.z, 0);
			if (out_affinity_flag[1])
			  for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]-1; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						y_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_16[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_16[cur.z*slice_size+
							(cur.y+1)*vh_in.scn.xysize[0]+cur.x], y_adjacency,
							cur.x, cur.y, cur.z, cur.x, cur.y+1, cur.z, 0);
			if (out_affinity_flag[2])
			  for (cur.z=0; cur.z<slices_out-1; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
						z_affinity[cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+
							cur.x] = affinity(in_data_16[cur.z*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x],
							in_data_16[(cur.z+1)*slice_size+
							cur.y*vh_in.scn.xysize[0]+cur.x], z_adjacency,
							cur.x, cur.y, cur.z, cur.x, cur.y, cur.z+1, 0);
		  }
		  if (out_affinity_flag[0])
			  for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]-1; cur.x++)
					{
						j = cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
						out_data[j] = x_affinity[j];
					}
		  if (out_affinity_flag[1])
			  for (cur.z=0; cur.z<slices_out; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]-1; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
					{
						j = cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
						if (y_affinity[j] > out_data[j])
						out_data[j] = y_affinity[j];
					}
		  if (out_affinity_flag[2])
			  for (cur.z=0; cur.z<slices_out-1; cur.z++)
				for (cur.y=0; cur.y<vh_in.scn.xysize[1]; cur.y++)
					for (cur.x=0; cur.x<vh_in.scn.xysize[0]; cur.x++)
					{
						j = cur.z*slice_size+cur.y*vh_in.scn.xysize[0]+cur.x;
						if (z_affinity[j] > out_data[j])
						out_data[j] = z_affinity[j];
					}
		}
		free(x_affinity);
		free(y_affinity);
		free(z_affinity);
	}
}
