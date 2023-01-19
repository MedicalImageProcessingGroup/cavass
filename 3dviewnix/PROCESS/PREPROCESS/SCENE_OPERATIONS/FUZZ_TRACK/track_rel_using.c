/*
  Copyright 1993-2012 Medical Image Processing Group
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

 
 

/*****************************************************************************
 * FUNCTION: fuzzy_track
 * DESCRIPTION: Computes the connectedness values and stores them at out_data.
 * PARAMETERS:
 *    current_volume: volume number from zero in the subscene.
 * SIDE EFFECTS: Messages may be written to stdout.
 * ENTRY CONDITIONS: The variables feature_status, function_level,
 *    function_width, weight, function_selected, file_info, vh_in, in_data,
 *    out_affinity_flag, fuzzy_adjacency_flag, slice_spacing,
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
 *    Modified: 3/12/03 computes the connectedness value for specific object.
 *         affects gloable variables out_data,num_seeds,point_seeds,x-,y-,z-affinity
 *         by Ying Zhuge
 *****************************************************************************/
void fuzzy_track(int object)
{
    int j, k,counter, toggle=0;
    int ei, x, y, z;
    Voxel cur;
    OutCellType *affp[6];
    unsigned pmin, Pmax, afn;
    
    H = Create_heap;
    if (H == NULL)
      handle_error(1);

    memset(out_data[object], 0, pslice*slice_size*sizeof(OutCellType));
    
    for (j=0; j<num_seeds[object]; j++)
      {
	cur.x = point_seeds[object][j][0];
	cur.y = point_seeds[object][j][1];
	cur.z = point_seeds[object][j][2];

	out_data[object][cur.z * slice_size + cur.y * pcol + cur.x] = CONN;
	if (Push_xyz(cur.x, cur.y, cur.z, CONN))
	  handle_error(1);
      }
    
    affp[0] = x_affinity[object];
    affp[1] = y_affinity[object];
    affp[2] = z_affinity[object];
    affp[3] = x_affinity[object]-1;
    affp[4] = y_affinity[object]-pcol;
    affp[5] = z_affinity[object]-slice_size;

    counter = 0;
    while (!H_is_empty) {
      if (counter--==0)
	{
	  counter = 100000;
	  if ((toggle = !toggle))
	    printf("\rTracking.  ");
	  else
	    printf("\rTracking.. ");
	  fflush(stdout);
	}
      Pmax = out_data[object][Pop_xyz(&cur.x, &cur.y, &cur.z)];
      for (ei = 0; ei < 6; ei++)
	{
	  x = cur.x + nbor[ei].x;
	  y = cur.y + nbor[ei].y;
	  z = cur.z + nbor[ei].z;
	  if (x >= 0 && x < pcol &&
	      y >= 0 && y < prow &&
	      z >= 0 && z < pslice)
	    {
	      j= cur.z*slice_size + cur.y*pcol + cur.x;
	      k = z * slice_size + y * pcol + x;
	      afn = affp[ei][j];
	      pmin = MIN(Pmax, afn);
	      if (pmin > out_data[object][k])
                {
                  if (out_data[object][k] == 0)
		    {
		      if (Push_xyz(x, y, z, pmin))
			handle_error(1);
		    }
		  else
                    Repush_xyz(x, y, z, pmin, out_data[object][k]);
                  out_data[object][k] = pmin;
                }
	    }
	}
    }
    
    printf("\rTracking done.\n");
    fflush(stdout);
}


