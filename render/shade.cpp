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


 
 
 
#include "cvRender.h"

#ifndef Abs
#define Abs(x) ((x)>=0? (x): -(x))
#endif

/*****************************************************************************
 * FUNCTION: stretch_cos
 * DESCRIPTION: Returns cos(acos(cos_theta)/n).
 * PARAMETERS:
 *    cos_theta: A number between or about -1.0 and 1.0
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: cos(acos(cos_theta)/n) if cos_theta is in [-1.0, 1.0].
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
static double stretch_cos(double cos_theta, double n)
{
	if (cos_theta > 1)
		cos_theta = 1;
	if (cos_theta < -1)
		cos_theta = -1;
	return (cos(acos(cos_theta)/n));
}

/*****************************************************************************
 * FUNCTION: spow
 * DESCRIPTION: Like pow, but returns a negative value if base is negative.
 * PARAMETERS:
 *    base: The number to be raised to a power.
 *    exponent: The exponent.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Like pow, but returns a negative value if base is negative.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
static double spow(double base, double exponent)
{
	return (base<0? -pow(-base, exponent): pow(base, exponent));
}


/*****************************************************************************
 * FUNCTION: VComputeShadeLut
 * DESCRIPTION: Initializes the shade look-up table of an object.  It must be
 *    called when the optical parameters of an object are set.
 * PARAMETERS:
 *    shade_lut: The shade lookup table to be initialized.
 *    diffuse_exponent: The exponent in the diffuse part of the shading
 *       formula.
 *    diffuse_n: The divisor in the diffuse part of the shading formula.
 *    specular_fraction: The weight applied to the specular part of the
 *       shading formula.
 *    specular_exponent: The exponent in the specular part of the shading
 *       formula.
 *    specular_n: The divisor in the specular part of the shading formula.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 3/12/90 by Dewey Odhner
 *    Modified: 2/17/93 to assign at least 1 by Dewey Odhner
 *    Modified: 3/18/94 parameters changed by Dewey Odhner
 *
 *****************************************************************************/
void VComputeShadeLut(int shade_lut[SHADE_LUT_SIZE], double diffuse_exponent,
	double diffuse_n, double specular_fraction, double specular_exponent,
	double specular_n)
{
	int i, j;
	double cos_theta;

	for (i=0; i<SHADE_LUT_SIZE; i++)
	{	cos_theta = 2*i/(double)(SHADE_LUT_SIZE-1)-1;
		j = (int)(MAX_ANGLE_SHADE*
			(	(1-specular_fraction)*
				spow(stretch_cos(cos_theta, diffuse_n),
					diffuse_exponent)+
				specular_fraction*
				spow(stretch_cos(cos_theta, specular_n),
					specular_exponent)
			));
		shade_lut[i] = j>0? j: 1;
	}
}


/*****************************************************************************
 * FUNCTION: get_gradient_table
 * DESCRIPTION: Initializes a look-up table which maps gradient codes to
 *    gradients normalized to unit magnitude.  This function is only called by
 *    VGetAngleShades.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The caller must not change the values in the table.
 * RETURN VALUE: The address of the gradient look-up table.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/20/90 by Dewey Odhner
 *
 *****************************************************************************/
static float (*get_gradient_table(void))[3]
{
	static int done;
	int i;
	double gx, gy, gz, norm_factor;
	static float g_table[G_CODES][3];

	if (done)
		return (g_table);
	for (i=0; i<G_CODES-1; i++)
	{	G_decode(gx, gy, gz, i);
		norm_factor=1/sqrt(gx*gx+gy*gy+gz*gz);
		g_table[i][0] = (float)(gx*norm_factor);
		g_table[i][1] = (float)(gy*norm_factor);
		g_table[i][2] = (float)(gz*norm_factor);
	}
	g_table[G_CODES-1][0] = g_table[G_CODES-1][1] = g_table[G_CODES-1][2] = 0;
	done=1;
	return (g_table);
}


/*****************************************************************************
 * FUNCTION: bget_gradient_table
 * DESCRIPTION: Initializes a look-up table which maps gradient codes to
 *    gradients normalized to unit magnitude.  This function is only called by
 *    VGetAngleShades.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The caller must not change the values in the table.
 * RETURN VALUE: The address of the gradient look-up table.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 5/16/01 by Dewey Odhner
 *
 *****************************************************************************/
static float (*bget_gradient_table(void))[3]
{
	static int done;
	int i;
	double gx, gy, gz, norm_factor;
	static float g_table[BG_CODES][3];

	if (done)
		return (g_table);
	for (i=0; i<BG_CODES-1; i++)
	{	BG_decode(gx, gy, gz, i);
		norm_factor=1/sqrt(gx*gx+gy*gy+gz*gz);
		g_table[i][0] = (float)(gx*norm_factor);
		g_table[i][1] = (float)(gy*norm_factor);
		g_table[i][2] = (float)(gz*norm_factor);
	}
	g_table[BG_CODES-1][0]= g_table[BG_CODES-1][1]= g_table[BG_CODES-1][2] = 0;
	done=1;
	return (g_table);
}


/*****************************************************************************
 * FUNCTION: VGetAngleShades
 * DESCRIPTION: Initializes a look-up table which maps gradient codes to
 *    shades, 0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 * PARAMETERS:
 *    angle_shade: The address of the shade look-up table will go here.
 *    rotation_matrix: Indicates the orientation of the object.
 *    rend_params: Parameters which have default values.  If a default value
 *       is used, it will be stored here.
 *       diffuse_exponent: Default is 1.
 *       diffuse_n: Default is 2.
 *       specular_fraction: Default is 0.2.
 *       specular_exponent: Default is 3.
 *       specular_n: Default is 1.
 *       light_direction: Default is {0, 0, -1}.  Currently only the
 *          default is implemented.
 *       shade_lut: May be NULL only if the default is used.  Default is to
 *          call VComputeShadeLut from within this function.
 *    flags: Indicates which fields in rend_params to use.  May be 0,
 *       SHADE_LUT_FLAG, or a bitwise or of one or more of
 *       DIFFUSE_EXPONENT_FLAG, DIFFUSE_N_FLAG, SPECULAR_FRACTION_FLAG,
 *       SPECULAR_EXPONENT_FLAG, SPECULAR_N_FLAG, LIGHT_DIRECTION_FLAG.
 *       If a flag bit is set, the value in the corresponding member of
 *       *rend_params will be used; otherwise the default value will be stored
 *       in *rend_params.
 *    object_class: Indicates which size of look-up table to initialize.
 *    show_back: Allows the back of a surface to be seen if non-zero.
 * SIDE EFFECTS: The table returned on previous calls will be overwritten and
 *    the same address will be returned again.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/20/90 by Dewey Odhner
 *    Modified: 2/17/93 to assign 0 for cos_theta < 0 by Dewey Odhner
 *    Modified: 7/21/93 parameters modified by Dewey Odhner
 *    Modified: 3/18/94 parameters modified by Dewey Odhner
 *    Modified: 5/16/01 parameter object_class added by Dewey Odhner
 *    Modified: 9/19/01 parameter show_back added by Dewey Odhner
 *
 *****************************************************************************/
void VGetAngleShades(int **angle_shade, double rotation_matrix[3][3],
	Rendering_parameters *rend_params, int flags,
	Classification_type object_class, int show_back)
{
	static int ang_shade[BG_CODES];
	int local_shade_lut[SHADE_LUT_SIZE], *shade_lut;
	unsigned short j;
	double cos_theta;
	float (*gradient_table)[3];

	*angle_shade = ang_shade;
	shade_lut = rend_params->shade_lut;
	if (flags&SHADE_LUT_FLAG)
	{
		assert(flags == SHADE_LUT_FLAG);
		assert(shade_lut);
	}
	else
	{	if (shade_lut == NULL)
			shade_lut = local_shade_lut;
		if ((flags&DIFFUSE_EXPONENT_FLAG) == 0)
			rend_params->diffuse_exponent = 1;
		if ((flags&DIFFUSE_N_FLAG) == 0)
			rend_params->diffuse_n = 2;
		if ((flags&SPECULAR_FRACTION_FLAG) == 0)
			rend_params->specular_fraction = (float).2;
		if ((flags&SPECULAR_EXPONENT_FLAG) == 0)
			rend_params->specular_exponent = 3;
		if ((flags&SPECULAR_N_FLAG) == 0)
			rend_params->specular_n = 1;
		VComputeShadeLut(shade_lut, rend_params->diffuse_exponent,
			rend_params->diffuse_n, rend_params->specular_fraction,
			rend_params->specular_exponent, rend_params->specular_n);
	}
	if (object_class==BINARY_B || object_class==T_SHELL)
	{
		gradient_table = bget_gradient_table();
		for (j=0; j<BG_CODES-1; j++)
		{	cos_theta =	gradient_table[j][0]*rotation_matrix[2][0]+
						gradient_table[j][1]*rotation_matrix[2][1]+
						gradient_table[j][2]*rotation_matrix[2][2];
			if (cos_theta < 0)
				cos_theta = show_back? -cos_theta: 0;
			ang_shade[j] =
				shade_lut[(int)rint((cos_theta+1)*(SHADE_LUT_SIZE-1)/2)];
		}
		ang_shade[BG_CODES-1] = shade_lut[0];
	}
	else
	{
		gradient_table = get_gradient_table();
		for (j=0; j<G_CODES-1; j++)
		{	cos_theta =	gradient_table[j][0]*rotation_matrix[2][0]+
						gradient_table[j][1]*rotation_matrix[2][1]+
						gradient_table[j][2]*rotation_matrix[2][2];
			if (cos_theta < 0)
				cos_theta = show_back? -cos_theta: 0;
			ang_shade[j] =
				shade_lut[(int)rint((cos_theta+1)*(SHADE_LUT_SIZE-1)/2)];
		}
		ang_shade[0x600] = shade_lut[0];
	}
}
