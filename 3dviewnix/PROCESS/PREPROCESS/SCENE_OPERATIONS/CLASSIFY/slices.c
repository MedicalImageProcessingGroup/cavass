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

 
 






#include "slices.h"

void compute_slices(vh, sl)
ViewnixHeader *vh;
SLICES *sl;
{

	int i,j,k;
	float spc;

    sl->sd = vh->scn.dimension;
 
	sl->width = vh->scn.xysize[0];
	sl->height = vh->scn.xysize[1];
 	sl->bits = vh->scn.num_of_bits;
    sl->volumes = (sl->sd==3) ? 1 : vh->scn.num_of_subscenes[0];
    sl->slices = (int *) malloc( sl->volumes * sizeof(int) );
	sl->max_slices = 0;
    sl->min_location3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->max_location3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->min_spacing3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->max_spacing3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->variable_spacing = (int *) malloc( sl->volumes * sizeof(int) );
    sl->fov3 = (float *) malloc( sl->volumes * sizeof(float) );
 
    /* read number of slices in each volume from the header */
    k = (sl->sd==4) ? 1 : 0;
    for(i=0; i<sl->volumes; i++)
    {
        sl->slices[i] = vh->scn.num_of_subscenes[k];
        k++;
    }
 
    /* Build location tree */
    sl->location4 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->location4[0] = 0.0;
    sl->location3 = (float **) malloc( sl->volumes * sizeof(float *) );
    sl->slice_index = (int **) malloc( sl->volumes * sizeof(int *) );
    k = (sl->sd==4) ? sl->volumes : 0;
    for(i=0; i<sl->volumes; i++)
    {
        if(sl->sd==4)
            sl->location4[i] = vh->scn.loc_of_subscenes[i];
 
        sl->location3[i] = (float *) malloc(sl->slices[i] * sizeof(float));
        sl->slice_index[i] = (int *) malloc(sl->slices[i] * sizeof(int));
        for(j=0; j<sl->slices[i]; j++)
        {
    		sl->slice_index[i][j] = (sl->sd==4) ? k - sl->volumes : k;
            sl->location3[i][j] = vh->scn.loc_of_subscenes[k];
            k++;
        }
    }

	
 
    /* Get Min and Max SPACING along each of the free dimensions */
    /* Get Min and Max LOCATIONS along each of the free dimensions */
	for(i=0; i<sl->volumes; i++)
	{
    	sl->min_spacing3[i] = 1000000;
    	sl->max_spacing3[i] = -10000000;
    	sl->min_location3[i] = 1000000;
    	sl->max_location3[i] = -10000000;
	}
    sl->min_spacing4 = sl->Min_location3 = sl->Min_spacing3 = 1000000;
    sl->max_spacing4 = sl->Max_location3 = sl->Max_spacing3 = -10000000;
    for(i=0; i<sl->volumes; i++)
    {
        if(i<sl->volumes-1)
        {
            spc = sl->location4[i+1] - sl->location4[i];
			if(spc<0) spc=-spc;
            if( spc < sl->min_spacing4 ) sl->min_spacing4 = spc;
            if( spc > sl->max_spacing4 ) sl->max_spacing4 = spc;
 
        }
 
        for(j=0; j<sl->slices[i]-1; j++)
        {
			/* SPACING */
            spc = sl->location3[i][j+1] - sl->location3[i][j];
			if(spc<0) spc=-spc;
            if( spc < sl->min_spacing3[i] ) sl->min_spacing3[i] = spc;
            if( spc > sl->max_spacing3[i] ) sl->max_spacing3[i] = spc;
            if( spc < sl->Min_spacing3 ) sl->Min_spacing3 = spc;
            if( spc > sl->Max_spacing3 ) sl->Max_spacing3 = spc;
 
			/* LOCATION */
            if(sl->location3[i][j] <sl->min_location3[i]) sl->min_location3[i]=sl->location3[i][j];
            if(sl->location3[i][j] >sl->max_location3[i]) sl->max_location3[i]=sl->location3[i][j];
            if(sl->location3[i][j] < sl->Min_location3) sl->Min_location3 = sl->location3[i][j];
            if(sl->location3[i][j] > sl->Max_location3) sl->Max_location3 = sl->location3[i][j];
        }
		/* LOCATION */
        if(sl->location3[i][j] <sl->min_location3[i]) sl->min_location3[i]=sl->location3[i][j];
        if(sl->location3[i][j] >sl->max_location3[i]) sl->max_location3[i]=sl->location3[i][j];
        if(sl->location3[i][j] < sl->Min_location3) sl->Min_location3 = sl->location3[i][j];
        if(sl->location3[i][j] > sl->Max_location3) sl->Max_location3 = sl->location3[i][j];
   
    }
 
    /* Get Global FOVs */
    sl->Fov3 = sl->Max_location3 - sl->Min_location3;
   


	/* For Each Volume */
	sl->total_slices = 0;
    for(i=0; i<sl->volumes; i++)
	{
		/* Maximum #of Slices */
		if( sl->slices[i] > sl->max_slices) sl->max_slices = sl->slices[i];

		/* Total Number of Slices */
		sl->total_slices += sl->slices[i];

		/* Field of View */
        sl->fov3[i] = (float)fabs( sl->location3[i][ sl->slices[i]-1 ]  -  sl->location3[i][0]  );

		/* Variable Spacing flag */
		if( sl->min_spacing3[i] == sl->max_spacing3[i])  sl->variable_spacing[i] = 0;
		else sl->variable_spacing[i] = 1;
	}
 
	/* Global Variable Spacing Flag */
	if(sl->Min_spacing3 == sl->Max_spacing3) sl->Variable_spacing = 0;
	else sl->Variable_spacing = 1;


}
