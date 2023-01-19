/*
  Copyright 1993-2013, 2016-2017 Medical Image Processing Group
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

#define MAX_OPACITY 240

#define SL_FW 4
#define RO_FW 2
#define CO_FW 1
#define SL_BW 0
#define RO_BW 0
#define CO_BW 0

#define Malloc(ptr, type, num, bequest) \
{	ptr = (type *)malloc((num)*sizeof(type)); \
	if (ptr == NULL) \
	{	bequest; \
		return (1); \
	} \
}

#define Num_neighbors(x) \
(	(((x)&PX)!=0)+ \
	(((x)&PY)!=0)+ \
	(((x)&PZ)!=0)+ \
	(((x)&NX)!=0)+ \
	(((x)&NY)!=0)+ \
	(((x)&NZ)!=0) \
)


int number_of_triangles[255]; /* in each t-shell configuration */


/*****************************************************************************
 * FUNCTION: VRender
 * DESCRIPTION: Renders a Shell0 object by parallel projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  If binary, it must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *                         BINARY_A    GRADIENT    PERCENT    BINARY_B
 *       total # of bits   32          48          48         32
 *       ncode             0 to 5      0 to 5      0 to 5     0 to 5
 *       y1                6 to 15     6 to 15     6 to 15    6 to 16
 *       tt                not stored  not stored  16 to 20   not stored
 *       n1                21 to 23    21 to 23    21 to 23   17 to 19
 *       n2                24 to 27    24 to 27    24 to 27   20 to 25
 *       n3                28 to 31    28 to 31    28 to 31   26 to 31
 *       gm                not stored  32 to 39    32 to 39   not stored
 *       op                not stored  40 to 47    not stored not stored
 *       pg                not stored  not stored  40 to 47   not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  For binary shells,
 *       the image is 1 byte per pixel; for GRADIENT shells, 2 bytes per
 *       pixel, and for PERCENT, 6 bytes per pixel with the first unsigned
 *       short being the red component, next green, then blue.
 *       The image values are 0 (black) to OBJECT_IMAGE_BACKGROUND-1 (bright),
 *       OBJECT_IMAGE_BACKGROUND for background pixels for binary shells;
 *       0 (black) to V_OBJECT_IMAGE_BACKGROUND-1 (bright),
 *       V_OBJECT_IMAGE_BACKGROUND for background pixels for fuzzy shells.
 *       The z-buffer values are from back to front, i.e. in a left-handed
 *       coordinate system.  For binary shells, opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       For all shells, projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    rend_params: Parameters which have default values.  If a default value
 *       is used, it will be stored here.
 *       ambient: Applies only to PERCENT renderings; for others, ambient
 *          light can be added by remapping the pixel values after rendering.
 *          The default value is 0.
 *       fade_edge: Applies to GRADIENT and PERCENT.  Default is FALSE.
 *       surface_red_factor, surface_green_factor, surface_blue_factor:
 *          Apply only to PERCENT renderings.  Default is MAX_SURFACE_FACTOR.
 *       tissue_opacity: Applies only to PERCENT renderings.  Default is
 *          {0, 0, 0, 1}.
 *       tissue_red, tissue_green, tissue_blue: Apply only to PERCENT
 *          renderings.  Defaults are {0, max, max, max}, {0, max, 0, max},
 *          {0, 0, 0, max}, max = V_OBJECT_IMAGE_BACKGROUND-1.
 *       surface_strength: Applies only to PERCENT renderings.  Default is 50.
 *       emission_power: Applies only to PERCENT renderings.  Default is 2.
 *       surf_pct_power: Applies only to PERCENT renderings.  Default is 1.
 *       perspective: Applies only to BINARY_A renderings.  Default is 0.
 *       maximum_intensity_projection: Applies to GRADIENT and PERCENT.
 *          Default is FALSE.
 *       check_event:  Default is NULL.  NULL is acceptable, it will not be
 *          dereferenced if NULL.
 *    flags: A bitwise or of zero or more of AMBIENT_FLAG, FADE_EDGE_FLAG,
 *       SURFACE_RGB_FACTOR_FLAG, TISSUE_OPACITY_FLAG, TISSUE_RGB_FLAG,
 *       SURFACE_STRENGTH_FLAG, MIP_FLAG, CHECK_EVENT_FLAG,
 *       EMISSION_POWER_FLAG, SURF_PCT_POWER_FLAG, PERSPECTIVE_FLAG, CLIP_FLAG.
 *       If a flag bit is
 *       set, the value in the corresponding member of *rend_params will be
 *       used; otherwise the default value will be stored in *rend_params.
 * SIDE EFFECTS: Any effects of rend_params->check_event() will occur.
 * ENTRY CONDITIONS: Any entry conditions of rend_params->check_event must be
 *    met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: rend_params->check_event FIRST
 * EXIT CONDITIONS: Returns 401 if rend_params->check_event returns FIRST
 * HISTORY:
 *    Created: 2/16/94 by Dewey Odhner
 *    Modified: 6/30/94 to do maximum intensity projection by Dewey Odhner
 *    Modified: 4/25/96 emission_power, surf_pct_power added by Dewey Odhner
 *    Modified: 11/2/98 perspective added by Dewey Odhner
 *    Modified: 1/4/00 clip_flag set by Dewey Odhner
 *    Modified: 5/16/01 BINARY_B shell accommodated by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::VRender(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Rendering_parameters *rend_params, int flags)
{
	int j;

	if ((flags&AMBIENT_FLAG) == 0)
		rend_params->ambient.red = rend_params->ambient.green =
			rend_params->ambient.blue = 0;
	ambient_light = rend_params->ambient;
	if ((flags&FADE_EDGE_FLAG) == 0)
		rend_params->fade_edge = FALSE;
	fade_flag = rend_params->fade_edge;
	if ((flags&SURFACE_RGB_FACTOR_FLAG) == 0)
		rend_params->surface_red_factor = rend_params->surface_green_factor =
			rend_params->surface_blue_factor = (float)MAX_SURFACE_FACTOR;
	surf_red_factr = rend_params->surface_red_factor;
	surf_green_factr = rend_params->surface_green_factor;
	surf_blue_factr = rend_params->surface_blue_factor;
	if ((flags&TISSUE_OPACITY_FLAG) == 0)
	{	rend_params->tissue_opacity[0] = rend_params->tissue_opacity[1] =
			rend_params->tissue_opacity[2] = 0;
		rend_params->tissue_opacity[3] = 1;
	}
	if ((flags&TISSUE_RGB_FLAG) == 0)
	{	rend_params->tissue_red[0] = 0;
		rend_params->tissue_green[0] = 0;
		rend_params->tissue_blue[0] = 0;
		rend_params->tissue_red[1] = 1;
		rend_params->tissue_green[1] = 1;
		rend_params->tissue_blue[1] = 0;
		rend_params->tissue_red[2] = 1;
		rend_params->tissue_green[2] = 0;
		rend_params->tissue_blue[2] = 0;
		rend_params->tissue_red[3] = 1;
		rend_params->tissue_green[3] = 1;
		rend_params->tissue_blue[3] = 1;
	}
	for (j=0; j<4; j++)
	{	materl_opacity[j] = rend_params->tissue_opacity[j];
		materl_red[j] = rend_params->tissue_red[j];
		materl_green[j] = rend_params->tissue_green[j];
		materl_blue[j] = rend_params->tissue_blue[j];
	}
	if ((flags&SURFACE_STRENGTH_FLAG) == 0)
		rend_params->surface_strength = 50;
	surf_strenth = rend_params->surface_strength;
	if ((flags&MIP_FLAG) == 0)
		rend_params->maximum_intensity_projection = FALSE;
	mip_flag = rend_params->maximum_intensity_projection;
	if ((flags&EMISSION_POWER_FLAG) == 0)
		rend_params->emission_power = 2;
	emis_power = rend_params->emission_power;
	if ((flags&SURF_PCT_POWER_FLAG) == 0)
		rend_params->surf_pct_power = 1;
	surf_ppower = rend_params->surf_pct_power;
	if ((flags&CHECK_EVENT_FLAG) == 0)
		rend_params->check_event = NULL;
	if ((flags&PERSPECTIVE_FLAG) == 0)
		rend_params->perspective = 0;
	prspectiv = rend_params->perspective;
	clip_flag = flags&CLIP_FLAG;
	if ((flags&T_SHELL_DETAIL_FLAG) == 0)
		rend_params->t_shell_detail = 1;
	detail = rend_params->t_shell_detail;
	switch (st_cl(object_data))
	{
		case BINARY_A:
			return (bin_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
		case GRADIENT:
			if (mip_flag)
				return (gr_mip_project(object_data, object_image,
					projection_matrix, projection_offset,
					rend_params->check_event));
			return (gr_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
		case PERCENT:
			if (mip_flag)
				return (pct_mip_project(object_data, object_image,
					projection_matrix, projection_offset,
					rend_params->check_event));
			return (pct_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
		case BINARY_B:
			return (bbin_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
		case T_SHELL:
			return (ts_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
		case DIRECT:
			if (mip_flag)
				return (d_mip_project(object_data, object_image,
					projection_matrix, projection_offset,
					rend_params->check_event));
			return (d_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				rend_params->check_event));
	}
	VPrintFatalError("VRender", "Internal error.", st_cl(object_data));
	return (-1);
}

/*****************************************************************************
 * FUNCTION: need_patch
 * DESCRIPTION: Checks whether a patch is needed for rendering a voxel.
 * PARAMETERS:
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if a patch is needed.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/14/94 by Dewey Odhner
 *
 *****************************************************************************/
int need_patch(double projection_matrix[3][3])
{
	double width, height, diameter_sq;

	/* Check whether voxel size is more than one pixel */
	width = projection_matrix[0][0]+projection_matrix[0][1]+
		projection_matrix[0][2];
	height = projection_matrix[1][0]+projection_matrix[1][1]+
		projection_matrix[1][2];
	diameter_sq = width*width+height*height;
	width = projection_matrix[0][0]+projection_matrix[0][1]-
		projection_matrix[0][2];
	height = projection_matrix[1][0]+projection_matrix[1][1]-
		projection_matrix[1][2];
	if (width*width+height*height > diameter_sq)
		diameter_sq = width*width+height*height;
	width = projection_matrix[0][0]-projection_matrix[0][1]+
		projection_matrix[0][2];
	height = projection_matrix[1][0]-projection_matrix[1][1]+
		projection_matrix[1][2];
	if (width*width+height*height > diameter_sq)
		diameter_sq = width*width+height*height;
	width = projection_matrix[0][0]-projection_matrix[0][1]-
		projection_matrix[0][2];
	height = projection_matrix[1][0]-projection_matrix[1][1]-
		projection_matrix[1][2];
	if (width*width+height*height > diameter_sq)
		diameter_sq = width*width+height*height;
	return (diameter_sq > .99);
}

/*****************************************************************************
 * FUNCTION: bin_project
 * DESCRIPTION: Renders a binary SHELL0 object by parallel projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The variables prspectiv, clip_flag must be properly set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/14/94 by Dewey Odhner
 *    Modified: 11/2/98 perspective added by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bin_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *))
{
	if (clip_flag)
		return (clip_patch_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
	else
		if (need_patch(projection_matrix))
			if (prspectiv)
				return (perspec_patch_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else
				return (patch_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
		else
			if (prspectiv)
				return (perspec_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else
				return (quick_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
}

/*****************************************************************************
 * FUNCTION: patch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 12/16/91 by Dewey Odhner
 *    Modified: 2/22/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::patch_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln, *z_pixel,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade,
		this_slice_x, this_slice_y, this_slice_z, error_code;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
	this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]]))
				{	this_column = (int)(*this_ptr&0x3ff);
					this_x = (unsigned)
						(this_row_x+column_x_table[this_column])/0x10000;
					this_y = (unsigned)
						(this_row_y+column_y_table[this_column])/0x10000;
					this_z = (unsigned)
						(this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					patch_bottom = this_y+patch->bottom;
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						right_end = this_x+patch->lines[patch_line].right;
						this_pixel = object_image->image[this_line]+left_end;
						line_end = object_image->image[this_line]+right_end;
						z_pixel = object_image->z_buffer[this_line]+left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: clip_patch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The variable viewport_back must be properly set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1/12/00 by Dewey Odhner
 *    Modified: 3/7/00 limits computed on number of rows to be projected
 *       by Dewey Odhner
 *    Modified: 3/17/00 clipping done in z-direction by Dewey Odhner
 *    Modified: 4/26/00 filament unshading removed by Dewey Odhner
 *    Modified: 10/10/00 clipping done at viewport_back by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::clip_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln, *z_pixel,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade,
		this_slice_x, this_slice_y, this_slice_z, error_code,
		first_slice, last_slice, first_row, last_row;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	first_slice = 0;
	last_slice = object_data->slices;
	this_slice_x =
		(int)(0x10000*(projection_offset[0]+.25)+first_slice*slice_x_factor);
	this_slice_y =
		(int)(0x10000*(projection_offset[1]+.25)+first_slice*slice_y_factor);
	this_slice_z =
		(int)(Z_SUBLEVELS*projection_offset[2]+first_slice*slice_z_factor);
	for (this_slice=first_slice; this_slice<last_slice; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}

		if ((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor == 0)
		{
			if (column_x_factor>0
					? column_y_factor<0
						? column_y_factor*(double)this_slice_x<=column_x_factor*(double)this_slice_y && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)>=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size)
						: column_y_factor*(double)this_slice_x>=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size) && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)<=column_x_factor*(double)this_slice_y
					: column_y_factor>0
						? column_y_factor*(double)this_slice_x>=column_x_factor*(double)this_slice_y && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)<=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size)
						: column_y_factor*(double)this_slice_x<=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size) && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)>=column_x_factor*(double)this_slice_y)
			{
				first_row = 0;
				last_row = object_data->rows;
			}
			else
			{
				first_row = object_data->rows;
				last_row = 0;
			}
		}
		else
		{
			if ((column_x_factor>0) != (column_y_factor>0))
			{
				first_row = (int)((column_y_factor*(double)this_slice_x-column_x_factor*(double)this_slice_y)/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
				last_row = (int)((column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)-column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size))/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
			}
			else
			{
				first_row = (int)((column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)-column_x_factor*(double)this_slice_y)/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
				last_row = (int)((column_y_factor*(double)this_slice_x-column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size))/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
			}
			if (first_row > last_row)
			{
				jy = first_row;
				first_row = last_row;
				last_row = jy;
			}
			last_row += 1;
			if (first_row < 0)
				first_row = 0;
			if (last_row > object_data->rows)
				last_row = object_data->rows;
		}
		if (first_row<last_row && (row_x_factor>0
				? (row_y_factor<0
					? (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor>0
						? row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
						: row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
					: (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor>0
						? row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor)
						: row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor)) 
				: (row_y_factor>0
					? (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor<0
						? row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
						: row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
					: (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor<0
						? row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor)
						: row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor))))
		{
			first_row = object_data->rows;
			last_row = 0;
		}
		if (first_row < last_row)
		{
			this_row_x = this_slice_x+first_row*row_x_factor;
			this_row_y = this_slice_y+first_row*row_y_factor;
			this_row_z = this_slice_z+first_row*row_z_factor;
			next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+first_row+1;
			this_ptr = next_ptr_ptr[-1];
		}
		for (this_row=first_row; this_row<last_row; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	this_angle_shade = angle_shade[this_ptr[1]];
				if (this_angle_shade)
				{	this_column = (int)(*this_ptr&0x3ff);
					if (this_row_z+column_z_table[this_column] < 0)
						continue;
					this_z = (unsigned)
						(this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
					if (this_z>=Z_BUFFER_LEVELS || this_z<viewport_back)
						continue;
					this_x = (unsigned)
						(this_row_x+column_x_table[this_column])/0x10000;
					this_y = (unsigned)
						(this_row_y+column_y_table[this_column])/0x10000;
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					if (patch_top < 0)
					{
						patch_line = -patch_top;
						patch_top = 0;
					}
					patch_bottom = this_y+patch->bottom;
					if (patch_bottom > object_image->image_size)
						patch_bottom = object_image->image_size;
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						if (left_end < 0)
							left_end = 0;
						right_end = this_x+patch->lines[patch_line].right;
						if (right_end > object_image->image_size)
							right_end = object_image->image_size;
						if (right_end <= left_end)
							continue;
						this_pixel = object_image->image[this_line]+left_end;
						line_end = object_image->image[this_line]+right_end;
						z_pixel = object_image->z_buffer[this_line]+left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: perspec_patch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by perspective projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: The variables prspectiv must be properly set.
 *    Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 11/19/98 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::perspec_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade;
	double this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor;
	int coln, *z_pixel;
	double column_x_table[1024], column_y_table[1024], column_z_table[1024];
	int patch_top, patch_bottom, left_end, right_end, this_x, this_y, this_z;
	double this_slice_x, this_slice_y, this_slice_z;
	int this_line, patch_line, this_shade, error_code, center_loc[2];
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = projection_matrix[0][0];
	row_x_factor = projection_matrix[0][1];
	slice_x_factor = projection_matrix[0][2];
	column_y_factor = projection_matrix[1][0];
	row_y_factor = projection_matrix[1][1];
	slice_y_factor = projection_matrix[1][2];
	column_z_factor = projection_matrix[2][0];
	row_z_factor = projection_matrix[2][1];
	slice_z_factor = projection_matrix[2][2];
	/* Use coordinates with units of 1. pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = MIDDLE_DEPTH+coln*column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	this_slice_x = (projection_offset[0]+.25);
	this_slice_y = (projection_offset[1]+.25);
	this_slice_z = projection_offset[2];
	if (object_image->projected == ONE_TO_ONE)
	{
		center_loc[0] = object_image->image_location[0]/2;
		center_loc[1] = object_image->image_location[1]/2;
	}
	else
	{
		center_loc[0] = object_image->image_location[0];
		center_loc[1] = object_image->image_location[1];
	}
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7ff]))
				{	this_column = (int)(*this_ptr&0x3ff);
					this_z = (int)(this_row_z+column_z_table[this_column]);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_y+column_y_table[this_column]+
						center_loc[1])+.5);
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					patch_bottom = this_y+patch->bottom;
					assert(patch_top >= 0);
					assert(patch_bottom <= object_image->image_size);
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						right_end = this_x+patch->lines[patch_line].right;
						assert(left_end >= 0);
						assert(right_end <= object_image->image_size);
						this_pixel =
							object_image->image[this_line]+(int)left_end;
						line_end=object_image->image[this_line]+(int)right_end;
						z_pixel =
							object_image->z_buffer[this_line]+(int)left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: perspec_project
 * DESCRIPTION: Renders a binary SHELL0 object, by perspective projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: The variables prspectiv must be properly set.
 *    Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 11/20/98 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::perspec_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade;
	double this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor;
	int this_x, this_y, this_z, coln, center_loc[2];
	double column_x_table[1024], column_y_table[1024], column_z_table[1024];
	double this_slice_x, this_slice_y, this_slice_z;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = projection_matrix[0][0];
	row_x_factor = projection_matrix[0][1];
	slice_x_factor = projection_matrix[0][2];
	column_y_factor = projection_matrix[1][0];
	row_y_factor = projection_matrix[1][1];
	slice_y_factor = projection_matrix[1][2];
	column_z_factor = projection_matrix[2][0];
	row_z_factor = projection_matrix[2][1];
	slice_z_factor = projection_matrix[2][2];
	/* Use coordinates with units of 1. pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = MIDDLE_DEPTH+coln*column_z_factor;
	}
	this_slice_x = projection_offset[0];
	this_slice_y = projection_offset[1];
	this_slice_z = projection_offset[2];
	if (object_image->projected == ONE_TO_ONE)
	{
		center_loc[0] = object_image->image_location[0]/2;
		center_loc[1] = object_image->image_location[1]/2;
	}
	else
	{
		center_loc[0] = object_image->image_location[0];
		center_loc[1] = object_image->image_location[1];
	}
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
			return (401);
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7ff]))
				{	this_column = (int)(*this_ptr&0x3ff);
					this_z = (int)(this_row_z+column_z_table[this_column]+.5);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_y+column_y_table[this_column]+
						center_loc[1])+.5);
					if (this_z < object_image->z_buffer[this_y][this_x])
						continue;
					object_image->image[this_y][this_x] = (unsigned)
						this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					object_image->z_buffer[this_y][this_x] = this_z;
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: quick_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 12/11/91 by Dewey Odhner
 *    Modified: 2/22/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, *row_y_table,
		column_z_factor, slice_z_factor, row_z_factor, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_x, this_y, this_z,
		this_slice_x, this_slice_y, this_slice_z;
	double slice_y_factor = 0x10000*projection_matrix[1][2],
		row_y_factor = 0x10000*projection_matrix[1][1],
		column_y_factor = 0x10000*projection_matrix[1][0];
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	row_y_table = (int *)malloc(object_data->rows*sizeof(int));
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (this_row=0; this_row<object_data->rows; this_row++)
		row_y_table[this_row] = (int)rint(this_row*row_y_factor);
	for (coln=0; coln<= Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = (int)rint(coln*column_y_factor);
		column_z_table[coln] =
			(int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(row_y_table);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
			this_slice*slice_y_factor);
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{
			this_row_y = this_slice_y+row_y_table[this_row];
			for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]]))
				{	this_column = (int)(*this_ptr&0x3ff);
					this_x = (this_row_x+column_x_table[this_column])/
						(unsigned)0x10000;
					this_y = (this_row_y+column_y_table[this_column])/
						(unsigned)0x10000;
					this_z = (this_row_z+column_z_table[this_column])/
						Z_SUBLEVELS;
					if (this_z < object_image->z_buffer[this_y][this_x])
						continue;
					object_image->image[this_y][this_x] = (unsigned)
						this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					object_image->z_buffer[this_y][this_x] = this_z;
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_z += slice_z_factor;
	}
	free(row_y_table);
	return (0);
}

/*****************************************************************************
 * FUNCTION: ts_project
 * DESCRIPTION: Renders a binary SHELL2 object by parallel projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The variables detail, must be properly set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 10/3/03 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::ts_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	if ((fade_flag && materl_opacity[0]<1) || need_patch(projection_matrix))
		if (materl_opacity[0]>0 && materl_opacity[0]<1)
			return (ts_tpatch_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				check_event));
		else
			return (ts_patch_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				check_event));
	else
		if (materl_opacity[0]>0 && materl_opacity[0]<1)
			return (ts_tpatch_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				check_event));
		else
			return (ts_quick_project(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				check_event));
}

/*
File  : marchingCubesGL.cpp
Author: George J. Grevera
Date  : 6/16/97
Desc. : Based on "Marching Cubes: A High Resolution 3D Surface Construction
        Algorithm," W.E. Lorensen, and H.E. Cline, Computer Graphics, 21(4),
        7/87, pp. 163-169.

        coordinate system:

             z
            +
           /
          /
         /
         ----- +x
        |
        |
        |
        +
        y

        vertices:

            v8-------v7
           /|        /|
          / |       / |
         /  |      /  |
        v4-------v3   |
        |   |     |   |
        |   v5----|--v6
        |  /      |  /
        | /       | /
        |/        |/
        v1-------v2

        edges:

            .----e7---.
           /|        /|
         e11|      e12|
         /  e8     /  e6
        .----e3---.   |
        |   |     |   |
        |   .---e5|---.
        e4 /      e2 /
        | e9      |e10
        |/        |/
        .----e1---.

//----------------------------------------------------------------------
// each row corresponds to a configuration of a marching cube.  each column
// entry corresponds to an edge and will become the vertex of a triangle
// (once it has been interpolated along the edge).  then triples of these
// form the triangle.
*/

static const int triangle_edges[189][3] = {
{ 1,  2,  6},
{ 1,  2,  7},
{ 1,  2,  8},
{ 1,  2,  9},
{ 1,  2, 10},
{ 1,  2, 11},
{ 1,  2, 12},
{ 1,  3,  5},
{ 1,  3,  6},
{ 1,  3,  7},
{ 1,  3,  8},
{ 1,  3,  9},
{ 1,  3, 10},
{ 1,  3, 11},
{ 1,  3, 12},
{ 1,  4,  5},
{ 1,  4,  6},
{ 1,  4,  7},
{ 1,  4,  8},
{ 1,  4,  9},
{ 1,  4, 10},
{ 1,  4, 11},
{ 1,  4, 12},
{ 1,  5,  6},
{ 1,  5,  7},
{ 1,  5,  8},
{ 1,  5, 11},
{ 1,  5, 12},
{ 1,  6,  7},
{ 1,  6,  8},
{ 1,  6,  9},
{ 1,  6, 10},
{ 1,  6, 11},
{ 1,  6, 12},
{ 1,  7,  8},
{ 1,  7,  9},
{ 1,  7, 10},
{ 1,  7, 11},
{ 1,  7, 12},
{ 1,  8,  9},
{ 1,  8, 10},
{ 1,  8, 11},
{ 1,  8, 12},
{ 1,  9, 11},
{ 1,  9, 12},
{ 1, 10, 11},
{ 1, 10, 12},
{ 1, 11, 12},
{ 2,  3,  5},
{ 2,  3,  7},
{ 2,  3,  8},
{ 2,  3,  9},
{ 2,  3, 10},
{ 2,  3, 11},
{ 2,  3, 12},
{ 2,  4,  5},
{ 2,  4,  6},
{ 2,  4,  7},
{ 2,  4,  8},
{ 2,  4,  9},
{ 2,  4, 10},
{ 2,  4, 11},
{ 2,  4, 12},
{ 2,  5,  6},
{ 2,  5,  7},
{ 2,  5,  8},
{ 2,  5,  9},
{ 2,  5, 10},
{ 2,  5, 11},
{ 2,  5, 12},
{ 2,  6,  7},
{ 2,  6,  8},
{ 2,  6,  9},
{ 2,  6, 11},
{ 2,  7,  8},
{ 2,  7, 10},
{ 2,  7, 11},
{ 2,  7, 12},
{ 2,  8,  9},
{ 2,  8, 10},
{ 2,  8, 11},
{ 2,  8, 12},
{ 2,  9, 10},
{ 2,  9, 11},
{ 2,  9, 12},
{ 2, 10, 11},
{ 2, 11, 12},
{ 3,  4,  5},
{ 3,  4,  6},
{ 3,  4,  7},
{ 3,  4,  8},
{ 3,  4,  9},
{ 3,  4, 10},
{ 3,  4, 11},
{ 3,  4, 12},
{ 3,  5,  6},
{ 3,  5,  7},
{ 3,  5,  8},
{ 3,  5,  9},
{ 3,  5, 10},
{ 3,  5, 11},
{ 3,  5, 12},
{ 3,  6,  7},
{ 3,  6,  8},
{ 3,  6,  9},
{ 3,  6, 10},
{ 3,  6, 11},
{ 3,  6, 12},
{ 3,  7,  8},
{ 3,  7,  9},
{ 3,  7, 10},
{ 3,  8,  9},
{ 3,  8, 10},
{ 3,  8, 11},
{ 3,  8, 12},
{ 3,  9, 10},
{ 3,  9, 11},
{ 3,  9, 12},
{ 3, 10, 11},
{ 3, 10, 12},
{ 4,  5,  6},
{ 4,  5,  7},
{ 4,  5,  8},
{ 4,  5,  9},
{ 4,  5, 10},
{ 4,  5, 11},
{ 4,  5, 12},
{ 4,  6,  7},
{ 4,  6,  8},
{ 4,  6,  9},
{ 4,  6, 10},
{ 4,  6, 11},
{ 4,  6, 12},
{ 4,  7,  8},
{ 4,  7,  9},
{ 4,  7, 10},
{ 4,  7, 11},
{ 4,  7, 12},
{ 4,  8, 10},
{ 4,  8, 12},
{ 4,  9, 12},
{ 4, 10, 11},
{ 4, 10, 12},
{ 4, 11, 12},
{ 5,  6,  9},
{ 5,  6, 10},
{ 5,  6, 11},
{ 5,  6, 12},
{ 5,  7,  9},
{ 5,  7, 10},
{ 5,  7, 12},
{ 5,  8,  9},
{ 5,  8, 10},
{ 5,  8, 11},
{ 5,  8, 12},
{ 5,  9, 11},
{ 5,  9, 12},
{ 5, 10, 11},
{ 5, 10, 12},
{ 5, 11, 12},
{ 6,  7,  9},
{ 6,  7, 10},
{ 6,  7, 11},
{ 6,  7, 12},
{ 6,  8, 10},
{ 6,  8, 11},
{ 6,  8, 12},
{ 6,  9, 10},
{ 6,  9, 11},
{ 6,  9, 12},
{ 6, 10, 11},
{ 6, 11, 12},
{ 7,  8,  9},
{ 7,  8, 10},
{ 7,  8, 11},
{ 7,  8, 12},
{ 7,  9, 10},
{ 7,  9, 11},
{ 7,  9, 12},
{ 7, 10, 11},
{ 8,  9, 10},
{ 8,  9, 12},
{ 8, 10, 11},
{ 8, 10, 12},
{ 8, 11, 12},
{ 9, 10, 11},
{ 9, 10, 12},
{ 9, 11, 12},
{10, 11, 12}};

static const int triangle_table[255][6]={
{-1}, /* 0 */
{19, -1}, /* 1 */
{4, -1}, /* 2 */
{82, 59, -1}, /* 3 */
{54, -1}, /* 4 */
{19, 54, -1}, /* 5 */
{119, 12, -1}, /* 6 */
{91, 117, 186, -1}, /* 7 */
{93, -1}, /* 8 */
{43, 13, -1}, /* 9 */
{4, 93, -1}, /* 10 */
{53, 85, 185, -1}, /* 11 */
{62, 143, -1}, /* 12 */
{6, 44, 187, -1}, /* 13 */
{20, 141, 188, -1}, /* 14 */
{186, 187, -1}, /* 15 */
{151, -1}, /* 16 */
{15, 122, -1}, /* 17 */
{151, 4, -1}, /* 18 */
{67, 65, 58, -1}, /* 19 */
{151, 54, -1}, /* 20 */
{122, 15, 54, -1}, /* 21 */
{119, 12, 151, -1}, /* 22 */
{122, 126, 158, 94, -1}, /* 23 */
{151, 93, -1}, /* 24 */
{153, 100, 7, -1}, /* 25 */
{4, 151, 93, -1}, /* 26 */
{53, 85, 182, 152, -1}, /* 27 */
{62, 143, 151, -1}, /* 28 */
{25, 42, 184, 6, -1}, /* 29 */
{151, 45, 188, 21, -1}, /* 30 */
{153, 157, 188, -1}, /* 31 */
{145, -1}, /* 32 */
{145, 19, -1}, /* 33 */
{0, 23, -1}, /* 34 */
{144, 129, 56, -1}, /* 35 */
{145, 54, -1}, /* 36 */
{145, 19, 54, -1}, /* 37 */
{107, 95, 7, -1}, /* 38 */
{91, 98, 101, 147, -1}, /* 39 */
{145, 93, -1}, /* 40 */
{13, 43, 145, -1}, /* 41 */
{23, 0, 93, -1}, /* 42 */
{155, 68, 63, 53, -1}, /* 43 */
{143, 62, 145, -1}, /* 44 */
{145, 84, 187, 3, -1}, /* 45 */
{171, 32, 23, 21, -1}, /* 46 */
{144, 169, 187, -1}, /* 47 */
{180, 164, -1}, /* 48 */
{20, 130, 128, -1}, /* 49 */
{39, 2, 71, -1}, /* 50 */
{71, 58, -1}, /* 51 */
{180, 164, 54, -1}, /* 52 */
{54, 31, 16, 128, -1}, /* 53 */
{166, 42, 14, 39, -1}, /* 54 */
{107, 88, 128, -1}, /* 55 */
{164, 180, 93, -1}, /* 56 */
{12, 112, 164, 113, -1}, /* 57 */
{93, 3, 78, 71, -1}, /* 58 */
{53, 80, 71, -1}, /* 59 */
{86, 61, 164, 180, -1}, /* 60 */
{29, 31, 41, 6, 47, -1}, /* 61 */
{47, 21, 33, 39, 29, -1}, /* 62 */
{165, 171, -1}, /* 63 */
{163, -1}, /* 64 */
{163, 19, -1}, /* 65 */
{4, 163, -1}, /* 66 */
{59, 82, 163, -1}, /* 67 */
{70, 49, -1}, /* 68 */
{70, 49, 19, -1}, /* 69 */
{161, 36, 9, -1}, /* 70 */
{102, 104, 167, 91, -1}, /* 71 */
{163, 93, -1}, /* 72 */
{43, 13, 163, -1}, /* 73 */
{163, 4, 93, -1}, /* 74 */
{163, 52, 118, 185, -1}, /* 75 */
{136, 127, 56, -1}, /* 76 */
{0, 32, 43, 162, -1}, /* 77 */
{136, 127, 16, 31, -1}, /* 78 */
{161, 179, 185, -1}, /* 79 */
{163, 151, -1}, /* 80 */
{15, 122, 163, -1}, /* 81 */
{151, 4, 163, -1}, /* 82 */
{163, 152, 79, 58, -1}, /* 83 */
{49, 70, 151, -1}, /* 84 */
{25, 18, 70, 49, -1}, /* 85 */
{151, 31, 28, 9, -1}, /* 86 */
{138, 152, 92, 161, 110, -1}, /* 87 */
{151, 163, 93, -1}, /* 88 */
{163, 97, 7, 113, -1}, /* 89 */
{93, 4, 151, 163, -1}, /* 90 */
{52, 118, 157, 153, 163, -1}, /* 91 */
{151, 162, 131, 56, -1}, /* 92 */
{73, 162, 5, 153, 26, -1}, /* 93 */
{162, 131, 130, 20, 151, -1}, /* 94 */
{161, 179, 152, 182, -1}, /* 95 */
{150, 158, -1}, /* 96 */
{150, 158, 19, -1}, /* 97 */
{6, 38, 24, -1}, /* 98 */
{150, 126, 62, 123, -1}, /* 99 */
{67, 48, 96, -1}, /* 100 */
{19, 99, 96, 52, -1}, /* 101 */
{24, 9, -1}, /* 102 */
{91, 98, 96, -1}, /* 103 */
{158, 150, 93, -1}, /* 104 */
{11, 116, 158, 150, -1}, /* 105 */
{93, 77, 1, 24, -1}, /* 106 */
{64, 77, 66, 53, 83, -1}, /* 107 */
{60, 135, 149, 136, -1}, /* 108 */
{83, 3, 76, 67, 64, -1}, /* 109 */
{136, 17, 24, -1}, /* 110 */
{148, 177, -1}, /* 111 */
{175, 181, 186, -1}, /* 112 */
{46, 42, 18, 175, -1}, /* 113 */
{6, 44, 178, 172, -1}, /* 114 */
{175, 81, 58, -1}, /* 115 */
{180, 112, 108, 52, -1}, /* 116 */
{110, 52, 173, 20, 138, -1}, /* 117 */
{39, 34, 9, -1}, /* 118 */
{108, 90, -1}, /* 119 */
{93, 172, 178, 186, -1}, /* 120 */
{10, 113, 40, 175, 183, -1}, /* 121 */
{172, 178, 44, 6, 93, -1}, /* 122 */
{53, 80, 77, 74, -1}, /* 123 */
{176, 172, 75, 136, 57, -1}, /* 124 */
{4, 174, -1}, /* 125 */
{136, 17, 172, 35, -1}, /* 126 */
{174, -1}, /* 127 */
{174, -1}, /* 128 */
{174, 19, -1}, /* 129 */
{4, 174, -1}, /* 130 */
{82, 59, 174, -1}, /* 131 */
{54, 174, -1}, /* 132 */
{19, 174, 54, -1}, /* 133 */
{12, 119, 174, -1}, /* 134 */
{174, 140, 186, 94, -1}, /* 135 */
{108, 90, -1}, /* 136 */
{39, 34, 9, -1}, /* 137 */
{108, 90, 4, -1}, /* 138 */
{180, 112, 108, 52, -1}, /* 139 */
{175, 81, 58, -1}, /* 140 */
{6, 44, 178, 172, -1}, /* 141 */
{46, 42, 18, 175, -1}, /* 142 */
{175, 181, 186, -1}, /* 143 */
{148, 177, -1}, /* 144 */
{136, 17, 24, -1}, /* 145 */
{177, 148, 4, -1}, /* 146 */
{60, 135, 149, 136, -1}, /* 147 */
{148, 177, 54, -1}, /* 148 */
{54, 37, 24, 21, -1}, /* 149 */
{46, 14, 148, 177, -1}, /* 150 */
{142, 94, 124, 136, 121, -1}, /* 151 */
{91, 98, 96, -1}, /* 152 */
{24, 9, -1}, /* 153 */
{4, 123, 87, 96, -1}, /* 154 */
{67, 48, 96, -1}, /* 155 */
{150, 126, 62, 123, -1}, /* 156 */
{6, 38, 24, -1}, /* 157 */
{121, 123, 137, 20, 142, -1}, /* 158 */
{150, 158, -1}, /* 159 */
{145, 174, -1}, /* 160 */
{145, 174, 19, -1}, /* 161 */
{0, 23, 174, -1}, /* 162 */
{174, 120, 56, 123, -1}, /* 163 */
{145, 54, 174, -1}, /* 164 */
{145, 174, 19, 54, -1}, /* 165 */
{174, 147, 101, 7, -1}, /* 166 */
{147, 101, 98, 91, 174, -1}, /* 167 */
{90, 108, 145, -1}, /* 168 */
{145, 35, 9, 172, -1}, /* 169 */
{133, 89, 23, 0, -1}, /* 170 */
{109, 172, 51, 144, 72, -1}, /* 171 */
{145, 74, 58, 77, -1}, /* 172 */
{3, 84, 181, 175, 145, -1}, /* 173 */
{27, 147, 22, 175, 139, -1}, /* 174 */
{175, 181, 147, 156, -1}, /* 175 */
{161, 179, 185, -1}, /* 176 */
{136, 127, 16, 31, -1}, /* 177 */
{0, 32, 43, 162, -1}, /* 178 */
{136, 127, 56, -1}, /* 179 */
{54, 170, 185, 162, -1}, /* 180 */
{31, 16, 127, 136, 54, -1}, /* 181 */
{168, 162, 30, 107, 8, -1}, /* 182 */
{107, 88, 162, 131, -1}, /* 183 */
{102, 104, 167, 91, -1}, /* 184 */
{161, 36, 9, -1}, /* 185 */
{72, 3, 160, 91, 109, -1}, /* 186 */
{70, 49, -1}, /* 187 */
{57, 77, 134, 161, 176, -1}, /* 188 */
{6, 38, 31, 28, -1}, /* 189 */
{163, 19, -1}, /* 190 */
{163, -1}, /* 191 */
{165, 171, -1}, /* 192 */
{171, 165, 19, -1}, /* 193 */
{165, 171, 4, -1}, /* 194 */
{166, 184, 82, 59, -1}, /* 195 */
{53, 80, 71, -1}, /* 196 */
{19, 50, 71, 113, -1}, /* 197 */
{12, 112, 164, 113, -1}, /* 198 */
{103, 113, 105, 91, 115, -1}, /* 199 */
{107, 88, 128, -1}, /* 200 */
{166, 42, 14, 39, -1}, /* 201 */
{4, 132, 128, 94, -1}, /* 202 */
{115, 52, 111, 107, 103, -1}, /* 203 */
{71, 58, -1}, /* 204 */
{39, 2, 71, -1}, /* 205 */
{20, 130, 128, -1}, /* 206 */
{180, 164, -1}, /* 207 */
{144, 169, 187, -1}, /* 208 */
{171, 32, 23, 21, -1}, /* 209 */
{4, 147, 156, 187, -1}, /* 210 */
{159, 147, 125, 67, 55, -1}, /* 211 */
{155, 68, 63, 53, -1}, /* 212 */
{26, 21, 146, 53, 73, -1}, /* 213 */
{8, 31, 106, 144, 168, -1}, /* 214 */
{145, 93, -1}, /* 215 */
{91, 98, 101, 147, -1}, /* 216 */
{107, 95, 7, -1}, /* 217 */
{123, 87, 95, 107, 4, -1}, /* 218 */
{67, 48, 147, 101, -1}, /* 219 */
{144, 129, 56, -1}, /* 220 */
{0, 23, -1}, /* 221 */
{144, 129, 31, 16, -1}, /* 222 */
{145, -1}, /* 223 */
{153, 157, 188, -1}, /* 224 */
{19, 152, 182, 188, -1}, /* 225 */
{25, 42, 184, 6, -1}, /* 226 */
{55, 123, 69, 153, 159, -1}, /* 227 */
{53, 85, 182, 152, -1}, /* 228 */
{152, 182, 85, 53, 19, -1}, /* 229 */
{153, 100, 7, -1}, /* 230 */
{153, 100, 123, 87, -1}, /* 231 */
{122, 126, 158, 94, -1}, /* 232 */
{183, 152, 114, 39, 10, -1}, /* 233 */
{139, 94, 154, 6, 27, -1}, /* 234 */
{151, 54, -1}, /* 235 */
{67, 65, 58, -1}, /* 236 */
{39, 2, 152, 79, -1}, /* 237 */
{15, 122, -1}, /* 238 */
{151, -1}, /* 239 */
{186, 187, -1}, /* 240 */
{20, 141, 188, -1}, /* 241 */
{6, 44, 187, -1}, /* 242 */
{62, 143, -1}, /* 243 */
{53, 85, 185, -1}, /* 244 */
{20, 141, 52, 118, -1}, /* 245 */
{43, 13, -1}, /* 246 */
{93, -1}, /* 247 */
{91, 117, 186, -1}, /* 248 */
{119, 12, -1}, /* 249 */
{91, 117, 3, 84, -1}, /* 250 */
{54, -1}, /* 251 */
{82, 59, -1}, /* 252 */
{4, -1}, /* 253 */
{19, -1} /* 254 */
};

static const int edge_vertices[13][2]={{0,0},
    {1,2}, {3,2}, {4,3}, {4,1},
	{5,6}, {7,6}, {8,7}, {8,5},
	{1,5}, {2,6}, {4,8}, {3,7}};
static const int T_x[9]={0, -1,  1,  1, -1, -1,  1,  1, -1},
				 T_y[9]={0,  1,  1, -1, -1,  1,  1, -1, -1},
				 T_z[9]={0, -1, -1, -1, -1,  1,  1,  1,  1};

/*****************************************************************************
 * FUNCTION: count_triangles
 * DESCRIPTION: Initializes the global array number_of_triangles.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: T_table must be initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 7/10/03 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::count_triangles(void)
{
	int j, k;

	for (j=1; j<255; j++)
	{
		for (k=0; triangle_table[j][k]>=0; k++)
			;
		number_of_triangles[j] = k;
	}
}

/*****************************************************************************
 * FUNCTION: ts_quick_project
 * DESCRIPTION: Renders a binary SHELL2 object, by parallel projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The arrays edge_vertices, triangle_edges, number_of_triangles,
 *    T_x, T_y, T_z must be initialized.
 * RETURN VALUE:
 *    0: successful
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 7/15/03 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::ts_quick_project(Shell_data *object_data, Object_image *object_image
	, double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, *row_y_table,
		column_z_factor, slice_z_factor, row_z_factor, coln, tn,
		column_x_table[65536], column_y_table[65536], column_z_table[65536],
		this_x, this_y, this_z,
		this_slice_x, this_slice_y, this_slice_z,
		triangle_x[189], triangle_y[189], triangle_z[189];
	double slice_y_factor = 0x10000*projection_matrix[1][2],
		row_y_factor = 0x10000*projection_matrix[1][1],
		column_y_factor = 0x10000*projection_matrix[1][0];
	unsigned char *this_ptr, **next_ptr_ptr, *next_ptr;

	row_y_table = (int *)malloc(object_data->rows*sizeof(int));
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (this_row=0; this_row<object_data->rows; this_row++)
		row_y_table[this_row] = (int)rint(this_row*row_y_factor);
	for (coln=0; coln<= Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = (int)rint(coln*column_y_factor);
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	for (tn=0; tn<189; tn++)
	{
		triangle_x[tn] = (
			(T_x[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][1]])*column_x_factor+
			(T_y[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][1]])*row_x_factor+
			(T_z[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][1]])*slice_x_factor)/12;
		triangle_y[tn] = (int)rint(
			(T_x[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][1]])*column_y_factor+
			(T_y[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][1]])*row_y_factor+
			(T_z[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][1]])*slice_y_factor)/12;
		triangle_z[tn] = (
			(T_x[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][1]])*column_z_factor+
			(T_y[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][1]])*row_z_factor+
			(T_z[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][1]])*slice_z_factor)/12;
	}
	this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(row_y_table);
			return (401);
		}
		next_ptr_ptr = (unsigned char **)object_data->ptr_table+
			this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
			this_slice*slice_y_factor);
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{
			this_row_y = this_slice_y+row_y_table[this_row];
			for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr;
					this_ptr+=3+3*number_of_triangles[*this_ptr])
			{
			  this_column = (int)this_ptr[1]<<8 | this_ptr[2];
			  for (tn=0; tn<number_of_triangles[*this_ptr]; tn++)
			  {
			  	if ((this_angle_shade = angle_shade[
				        ((int)this_ptr[4+3*tn]<<8&0x7fff)|this_ptr[5+3*tn]]))
				{
					this_x = (this_row_x+column_x_table[this_column]+
						triangle_x[triangle_table[*this_ptr][tn]])/
						(unsigned)0x10000;
					this_y = (this_row_y+column_y_table[this_column]+
						triangle_y[triangle_table[*this_ptr][tn]])/
						(unsigned)0x10000;
					this_z = (this_row_z+column_z_table[this_column]+
						triangle_z[triangle_table[*this_ptr][tn]])/
						Z_SUBLEVELS;
					if (this_z < object_image->z_buffer[this_y][this_x])
						continue;
					object_image->image[this_y][this_x] = (unsigned)
						this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					object_image->z_buffer[this_y][this_x] = this_z;
				}
			  }
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_z += slice_z_factor;
	}
	free(row_y_table);
	return (0);
}

/*****************************************************************************
 * FUNCTION: ts_patch_project
 * DESCRIPTION: Renders a binary SHELL2 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The arrays edge_vertices, triangle_edges, number_of_triangles,
 *    T_x, T_y, T_z, and the variables detail, must be initialized.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 9/2/03 by Dewey Odhner
 *    Modified: 10/3/03 variable "detail" used by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::ts_patch_project(Shell_data *object_data, Object_image *object_image
	, double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch **patch;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, tn, v, v_table0[256], v_table1[256], *z_pixel,
		column_x_table[65536], column_y_table[65536], column_z_table[65536],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade, new_view, tweak,
		this_slice_x, this_slice_y, this_slice_z, triangle_z[189], error_code;
	char *this_pixel, *line_end;
	unsigned char *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<= Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	patch = (Patch **)calloc(64827, sizeof(Patch *));
	tweak = detail==3;
	for (v=0; v<343; v++)
	{
		tn = detail>3? v: (v/49<2? 49: v/49>4? 5*49: 3*49)+
			(v/7%7<2? 7: v/7%7>4? 35: 21)+(v%7<2? 1: v%7>4? 5: 3);
		if (v%7 & 1)
			v_table0[(((v/49-3)&7)<<5)|((((v/7%7)-3)&7)<<2)|((((v%7)-3)&6)>>1)] = tn;
		else
			v_table1[(((v/49-3)&7)<<5)|((((v/7%7)-3)&7)<<2)|((((v%7)-3)&6)>>1)] = tn;
	}
	for (tn=0; tn<189; tn++)
		triangle_z[tn] = (
			(T_x[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][1]])*column_z_factor+
			(T_y[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][1]])*row_z_factor+
			(T_z[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][1]])*slice_z_factor)/12;
	this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
	this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	new_view = TRUE;
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			for (tn=0; tn<64827; tn++)
				if (patch[tn])
					free(patch[tn]);
			free(patch);
			return (401);
		}
		next_ptr_ptr = (unsigned char **)object_data->ptr_table+
			this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr;
					this_ptr+=3+3*number_of_triangles[*this_ptr])
			{
			  this_column = (int)this_ptr[1]<<8 | this_ptr[2];
			  this_x =
			    (unsigned)(this_row_x+column_x_table[this_column])/0x10000;
			  this_y =
			    (unsigned)(this_row_y+column_y_table[this_column])/0x10000;
			  for (tn=0; tn<number_of_triangles[*this_ptr]; tn++)
			  {
			  	if ((this_angle_shade = angle_shade[
				        ((int)this_ptr[4+3*tn]<<8&0x7fff)|this_ptr[5+3*tn]]))
				{
					v = (detail?
						(this_ptr[4+3*tn]&0x80? v_table1[this_ptr[3+3*tn]]:
						v_table0[this_ptr[3+3*tn]])*189: v_table0[0]*189)+
						triangle_table[*this_ptr][tn];
					if (patch[v] == NULL)
					{
						error_code =
							get_triangle_patch(patch+v, projection_matrix,
							edge_vertices, triangle_edges, T_x, T_y, T_z, v,
							new_view, tweak);
						if (error_code)
						{
							for (tn=0; tn<64827; tn++)
								if (patch[tn])
									free(patch[tn]);
							free(patch);
							return (error_code);
						}
						new_view = FALSE;
					}
					this_z = (this_row_z+column_z_table[this_column]+
						triangle_z[triangle_table[*this_ptr][tn]])/
						Z_SUBLEVELS;
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch[v]->top;
					patch_bottom = this_y+patch[v]->bottom;
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch[v]->lines[patch_line].left;
						right_end = this_x+patch[v]->lines[patch_line].right;
						this_pixel = object_image->image[this_line]+left_end;
						line_end = object_image->image[this_line]+right_end;
						z_pixel = object_image->z_buffer[this_line]+left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			  }
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	for (tn=0; tn<64827; tn++)
		if (patch[tn])
			free(patch[tn]);
	free(patch);
	return (0);
}

#define ts_tpatch_one_voxel \
{ \
	unsigned char old_shade; \
 \
    this_column = ((int)this_ptr[1]<<8) | this_ptr[2]; \
    this_x = \
      (unsigned)(this_row_x+column_x_table[this_column])/0x10000; \
    this_y = \
      (unsigned)(this_row_y+column_y_table[this_column])/0x10000; \
    for (tn=0; tn<number_of_triangles[*this_ptr]; tn++) \
    { \
      if ((this_angle_shade = angle_shade[ \
              ((int)this_ptr[4+3*tn]<<8&0x7fff)|this_ptr[5+3*tn]])) \
      { \
          v = (detail? \
              (this_ptr[4+3*tn]&0x80? v_table1[this_ptr[3+3*tn]]: \
              v_table0[this_ptr[3+3*tn]])*189: v_table0[0]*189)+ \
              triangle_table[*this_ptr][tn]; \
          if (patch[v] == NULL) \
          { \
              error_code = \
                  get_triangle_patch(patch+v, projection_matrix, \
                  edge_vertices, triangle_edges, T_x, T_y, T_z, v, \
                  new_view, tweak); \
              if (error_code) \
              { \
                  for (tn=0; tn<64827; tn++) \
                      if (patch[tn]) \
                          free(patch[tn]); \
                  if ((order&CO_FW) == 0) \
                      free(voxel_list); \
                  return (error_code); \
              } \
              new_view = FALSE; \
          } \
          this_z = (int)((this_row_z+column_z_table[this_column]+ \
              triangle_z[triangle_table[*this_ptr][tn]])/ \
              Z_SUBLEVELS); \
          this_shade = (int)(((unsigned)this_angle_shade*this_z/ \
		      SHADE_SCALE_FACTOR)*materl_opacity[0]); \
          patch_line = 0; \
          patch_top = this_y+patch[v]->top; \
          patch_bottom = this_y+patch[v]->bottom; \
          for (this_line=patch_top; this_line<patch_bottom; \
                  this_line++, patch_line++) \
          {   left_end = this_x+patch[v]->lines[patch_line].left; \
              right_end = this_x+patch[v]->lines[patch_line].right; \
              this_pixel = object_image->image[this_line]+left_end; \
              line_end = object_image->image[this_line]+right_end; \
              z_pixel = object_image->z_buffer[this_line]+left_end; \
              for (; this_pixel<line_end; this_pixel++, z_pixel++) \
                  if (this_z > *z_pixel) \
                  { \
				      old_shade = *this_pixel; \
					  *this_pixel = \
					      old_shade == OBJECT_IMAGE_BACKGROUND \
						  ?	this_shade \
						  : (unsigned char)((1-materl_opacity[0])* \
						  	old_shade)+this_shade; \
				      *z_pixel = this_z; \
                  } \
          } \
      } \
    } \
}

/*****************************************************************************
 * FUNCTION: ts_tpatch_project
 * DESCRIPTION: Renders a binary SHELL2 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The arrays edge_vertices, triangle_edges, number_of_triangles,
 *    T_x, T_y, T_z, and the variables detail, materl_opacity[0],
 *    must be initialized.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 2/20/07 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::ts_tpatch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch **patch;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, tn, v, v_table0[256], v_table1[256], *z_pixel,
		column_x_table[65536], column_y_table[65536], column_z_table[65536],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade, new_view, tweak, order,
		this_slice_x, this_slice_y, this_slice_z, triangle_z[189], error_code;
	char *this_pixel, *line_end;
	unsigned char *this_ptr, **next_ptr_ptr, *next_ptr, **voxel_list=NULL,
	    **vl_end;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<= Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	patch = (Patch **)calloc(64827, sizeof(Patch *));
	tweak = detail==3;
	for (v=0; v<343; v++)
	{
		tn = detail>3? v: (v/49<2? 49: v/49>4? 5*49: 3*49)+
			(v/7%7<2? 7: v/7%7>4? 35: 21)+(v%7<2? 1: v%7>4? 5: 3);
		if (v%7 & 1)
			v_table0[(((v/49-3)&7)<<5)|((((v/7%7)-3)&7)<<2)|((((v%7)-3)&6)>>1)] = tn;
		else
			v_table1[(((v/49-3)&7)<<5)|((((v/7%7)-3)&7)<<2)|((((v%7)-3)&6)>>1)] = tn;
	}
	for (tn=0; tn<189; tn++)
		triangle_z[tn] = (
			(T_x[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_x[edge_vertices[triangle_edges[tn][2]][1]])*column_z_factor+
			(T_y[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_y[edge_vertices[triangle_edges[tn][2]][1]])*row_z_factor+
			(T_z[edge_vertices[triangle_edges[tn][0]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][0]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][1]][1]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][0]]+
			 T_z[edge_vertices[triangle_edges[tn][2]][1]])*slice_z_factor)/12;
	order =	(projection_matrix[2][0]>=0? CO_FW: CO_BW) +
			(projection_matrix[2][1]>=0? RO_FW: RO_BW) +
			(projection_matrix[2][2]>=0? SL_FW: SL_BW);
	if ((order&CO_FW) == 0)
	{
		voxel_list = (unsigned char **)malloc(((int)Largest_y1(object_data)+1)*
			sizeof(unsigned char *));
		if (voxel_list == NULL)
		{
			free(patch);
			return 1;
		}
	}
	new_view = TRUE;
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(voxel_list);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_ptr = next_ptr_ptr[-1];
					next_ptr = *next_ptr_ptr;
					vl_end = voxel_list;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						*vl_end++ = this_ptr;
					while (vl_end > voxel_list)
					{
						this_ptr = *--vl_end;
						ts_tpatch_one_voxel
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_ptr = next_ptr_ptr[-1];
					next_ptr = *next_ptr_ptr;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						ts_tpatch_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(voxel_list);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					next_ptr = *next_ptr_ptr;
					vl_end = voxel_list;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						*vl_end++ = this_ptr;
					while (vl_end > voxel_list)
					{
						this_ptr = *--vl_end;
						ts_tpatch_one_voxel
					}
					this_ptr = next_ptr;
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					next_ptr = *next_ptr_ptr;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						ts_tpatch_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(voxel_list);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_ptr = next_ptr_ptr[-1];
					next_ptr = *next_ptr_ptr;
					vl_end = voxel_list;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						*vl_end++ = this_ptr;
					while (vl_end > voxel_list)
					{
						this_ptr = *--vl_end;
						ts_tpatch_one_voxel
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_ptr = next_ptr_ptr[-1];
					next_ptr = *next_ptr_ptr;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						ts_tpatch_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(voxel_list);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					next_ptr = *next_ptr_ptr;
					vl_end = voxel_list;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						*vl_end++ = this_ptr;
					while (vl_end > voxel_list)
					{
						this_ptr = *--vl_end;
						ts_tpatch_one_voxel
					}
					this_ptr = next_ptr;
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					for (tn=0; tn<64827; tn++)
						if (patch[tn])
							free(patch[tn]);
					free(patch);
					return (401);
				}
				next_ptr_ptr = (unsigned char **)
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					next_ptr = *next_ptr_ptr;
					for (; this_ptr<next_ptr;
							this_ptr+=3+3*number_of_triangles[*this_ptr])
						ts_tpatch_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	for (tn=0; tn<64827; tn++)
		if (patch[tn])
			free(patch[tn]);
	if ((order&CO_FW) == 0)
		free(voxel_list);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: bbin_project
 * DESCRIPTION: Renders a binary SHELL0 object by parallel projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The variables prspectiv, clip_flag must be properly set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 5/17/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bbin_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	if (clip_flag)
		return (bclip_patch_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
	else
		if ((fade_flag&& materl_opacity[0]<1) || need_patch(projection_matrix))
			if (prspectiv)
				return (bperspec_patch_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else if (materl_opacity[0]>0 && materl_opacity[0]<1)
				return (tpatch_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else
				return (bpatch_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
		else
			if (prspectiv)
				return (bperspec_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else if (materl_opacity[0]>0 && materl_opacity[0]<1)
				return (tquick_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
			else
				return (bquick_project(object_data, object_image,
					projection_matrix, projection_offset, angle_shade,
					check_event));
}

/*****************************************************************************
 * FUNCTION: bclip_patch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The variable viewport_back must be properly set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 5/17/01 by Dewey Odhner
 *    Modified: 3/1/02 this_column calculation corrected by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bclip_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln, *z_pixel,
		column_x_table[2048], column_y_table[2048], column_z_table[2048],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade,
		this_slice_x, this_slice_y, this_slice_z, error_code,
		first_slice, last_slice, first_row, last_row;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	first_slice = 0;
	last_slice = object_data->slices;
	this_slice_x =
		(int)(0x10000*(projection_offset[0]+.25)+first_slice*slice_x_factor);
	this_slice_y =
		(int)(0x10000*(projection_offset[1]+.25)+first_slice*slice_y_factor);
	this_slice_z =
		(int)(Z_SUBLEVELS*projection_offset[2]+first_slice*slice_z_factor);
	for (this_slice=first_slice; this_slice<last_slice; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}

		if ((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor == 0)
		{
			if (column_x_factor>0
					? column_y_factor<0
						? column_y_factor*(double)this_slice_x<=column_x_factor*(double)this_slice_y && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)>=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size)
						: column_y_factor*(double)this_slice_x>=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size) && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)<=column_x_factor*(double)this_slice_y
					: column_y_factor>0
						? column_y_factor*(double)this_slice_x>=column_x_factor*(double)this_slice_y && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)<=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size)
						: column_y_factor*(double)this_slice_x<=column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size) && column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)>=column_x_factor*(double)this_slice_y)
			{
				first_row = 0;
				last_row = object_data->rows;
			}
			else
			{
				first_row = object_data->rows;
				last_row = 0;
			}
		}
		else
		{
			if ((column_x_factor>0) != (column_y_factor>0))
			{
				first_row = (int)((column_y_factor*(double)this_slice_x-column_x_factor*(double)this_slice_y)/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
				last_row = (int)((column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)-column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size))/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
			}
			else
			{
				first_row = (int)((column_y_factor*(double)(this_slice_x-0x10000*object_image->image_size)-column_x_factor*(double)this_slice_y)/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
				last_row = (int)((column_y_factor*(double)this_slice_x-column_x_factor*(double)(this_slice_y-0x10000*object_image->image_size))/((double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor));
			}
			if (first_row > last_row)
			{
				jy = first_row;
				first_row = last_row;
				last_row = jy;
			}
			last_row += 1;
			if (first_row < 0)
				first_row = 0;
			if (last_row > object_data->rows)
				last_row = object_data->rows;
		}
		if (first_row<last_row && (row_x_factor>0
				? (row_y_factor<0
					? (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor>0
						? row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
						: row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
					: (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor>0
						? row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor)
						: row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor)) 
				: (row_y_factor>0
					? (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor<0
						? row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
						: row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size)
					: (double)column_x_factor*row_y_factor-(double)column_y_factor*row_x_factor<0
						? row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor)
						: row_y_factor*(double)(this_slice_x+Smallest_y1(object_data)*column_x_factor)>row_x_factor*(double)(this_slice_y+Smallest_y1(object_data)*column_y_factor-0x10000*object_image->image_size) || row_y_factor*(double)(this_slice_x+Largest_y1(object_data)*column_x_factor-0x10000*object_image->image_size)<row_x_factor*(double)(this_slice_y+Largest_y1(object_data)*column_y_factor))))
		{
			first_row = object_data->rows;
			last_row = 0;
		}
		if (first_row < last_row)
		{
			this_row_x = this_slice_x+first_row*row_x_factor;
			this_row_y = this_slice_y+first_row*row_y_factor;
			this_row_z = this_slice_z+first_row*row_z_factor;
			next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+first_row+1;
			this_ptr = next_ptr_ptr[-1];
		}
		for (this_row=first_row; this_row<last_row; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	this_angle_shade = angle_shade[this_ptr[1]&0x7fff];
				if (this_angle_shade)
				{	this_column =
						(int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
					if (this_row_z+column_z_table[this_column] < 0)
						continue;
					this_z = (unsigned)
						(this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
					if (this_z>=Z_BUFFER_LEVELS || this_z<viewport_back)
						continue;
					this_x = (unsigned)
						(this_row_x+column_x_table[this_column])/0x10000;
					this_y = (unsigned)
						(this_row_y+column_y_table[this_column])/0x10000;
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					if (patch_top < 0)
					{
						patch_line = -patch_top;
						patch_top = 0;
					}
					patch_bottom = this_y+patch->bottom;
					if (patch_bottom > object_image->image_size)
						patch_bottom = object_image->image_size;
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						if (left_end < 0)
							left_end = 0;
						right_end = this_x+patch->lines[patch_line].right;
						if (right_end > object_image->image_size)
							right_end = object_image->image_size;
						if (right_end <= left_end)
							continue;
						this_pixel = object_image->image[this_line]+left_end;
						line_end = object_image->image[this_line]+right_end;
						z_pixel = object_image->z_buffer[this_line]+left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: bpatch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 5/17/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bpatch_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln, *z_pixel,
		column_x_table[2048], column_y_table[2048], column_z_table[2048],
		this_line, patch_top, patch_bottom, left_end, right_end, patch_line,
		this_x, this_y, this_z, this_shade,
		this_slice_x, this_slice_y, this_slice_z, error_code;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
	this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column =
						(int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
					this_x = (unsigned)
						(this_row_x+column_x_table[this_column])/0x10000;
					this_y = (unsigned)
						(this_row_y+column_y_table[this_column])/0x10000;
					this_z = (unsigned)
						(this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					patch_bottom = this_y+patch->bottom;
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						right_end = this_x+patch->lines[patch_line].right;
						this_pixel = object_image->image[this_line]+left_end;
						line_end = object_image->image[this_line]+right_end;
						z_pixel = object_image->z_buffer[this_line]+left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: tpatch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    materl_opacity[0] must be set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 9/14/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::tpatch_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[2048], column_y_table[2048], column_z_table[2048],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double top_margin, bottom_margin, left_margin, right_margin, voxel_depth,
		temp_matrix[3][3], temp_vector[3];

	order =	(projection_matrix[2][0]>=0? CO_FW: CO_BW) +
			(projection_matrix[2][1]>=0? RO_FW: RO_BW) +
			(projection_matrix[2][2]>=0? SL_FW: SL_BW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	if (fade_flag == 2)
	{
		error_code = VInvertMatrix(temp_matrix[0], projection_matrix[0], 3);
		if (error_code)
			return (error_code);
		temp_vector[0] = temp_vector[1] = 0;
		temp_vector[2] = 1;
		matrix_vector_multiply(temp_vector, temp_matrix, temp_vector);
		voxel_depth = sqrt((object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data))/
			(object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]*
			temp_vector[0]*temp_vector[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]*
			temp_vector[1]*temp_vector[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data)*
			temp_vector[2]*temp_vector[2]));
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, voxel_depth);
		if (error_code)
			return (error_code);
		trim_patch(patch);
	}
	else
	{
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, 1.);
		if (error_code)
			return (error_code);
	}
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[1]-2;
					next_ptr = *next_ptr_ptr;
					for (; this_ptr>=next_ptr; this_ptr-=2)
						t_patch_one_voxel(this_row_z, angle_shade,
							this_row_x, this_row_y,
							column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	this_ptr= *next_ptr_ptr-2;
					for (next_ptr=next_ptr_ptr[-1];
							this_ptr>=next_ptr; this_ptr-=2)
						t_patch_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[1]-2;
					next_ptr = *next_ptr_ptr;
					for (; this_ptr>=next_ptr; this_ptr-=2)
						t_patch_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	this_ptr= *next_ptr_ptr-2;
					for (next_ptr=next_ptr_ptr[-1];
							this_ptr>=next_ptr; this_ptr-=2)
						t_patch_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;this_ptr<next_ptr; this_ptr+=2)
						t_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: t_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class BINARY_B to an
 *    image buffer using a patch of pixels.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, materl_opacity
 *    must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 9/18/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::t_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024], int itop_margin,
	int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image)
{
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,
		this_angle_shade, shade_computed;
	unsigned char this_shade=0, *this_pixel, *line_start, *line_end;

	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
	{
		this_column = (int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
		icoord = this_row_x+column_x_table[this_column];
		this_x = icoord/0x10000;
		icoord -= this_x*0x10000+0x8000;
		left_margin_fpart = ileft_margin+icoord;
		left_margin_ipart = left_margin_fpart/0x10000;
		left_margin_fpart -= left_margin_ipart*0x10000;
		right_margin_fpart = iright_margin-icoord;
		right_margin_ipart = right_margin_fpart/0x10000;
		right_margin_fpart -= right_margin_ipart*0x10000;
		icoord = this_row_y+column_y_table[this_column];
		this_y = icoord/0x10000;
		icoord -= this_y*0x10000+0x8000;
		top_margin_fpart = itop_margin+icoord;
		top_margin_ipart = top_margin_fpart/0x10000;
		top_margin_fpart -= top_margin_ipart*0x10000;
		bottom_margin_fpart = ibottom_margin-icoord;
		bottom_margin_ipart = bottom_margin_fpart/0x10000;
		bottom_margin_fpart -= bottom_margin_ipart*0x10000;
		if (fade_flag == 2)
			top_margin_ipart = bottom_margin_ipart =
				left_margin_ipart = right_margin_ipart = 0;
		shade_computed = FALSE;
		patch_line = top_margin_ipart;
		patch_top = this_y+patch->top+patch_line;
		patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
		for (this_line=patch_top; this_line<patch_bottom;
				this_line++, patch_line++)
		{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
			left_end = this_x+patch_column;
			right_end =
				this_x+patch->lines[patch_line].right-right_margin_ipart;
			line_start =
				(unsigned char *)object_image->image[this_line]+left_end;
			z_pixel = object_image->z_buffer[this_line]+left_end;
			line_end =
				(unsigned char *)object_image->image[this_line]+right_end;
			for (this_pixel=line_start; this_pixel<line_end;
					this_pixel++, z_pixel++, patch_column++)
			{
				float new_opacity;
				unsigned char old_shade, pixel_shade;

				if (patch->lines[patch_line-top_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line-top_margin_ipart].right>
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].right>
							patch_column)
				{	if (!shade_computed)
					{	this_z = (this_row_z+column_z_table[this_column])/
							Z_SUBLEVELS;
						this_shade= (unsigned char)(((unsigned)this_angle_shade
							*this_z/SHADE_SCALE_FACTOR)*materl_opacity[0]);
						shade_computed = TRUE;
					}
					if (fade_flag == 1)
					{	if ((patch_line==top_margin_ipart ||
								patch->lines[patch_line-top_margin_ipart-1].
									left>patch_column ||
								patch->lines[patch_line-top_margin_ipart-1].
									right<=patch_column))
							new_opacity =
								(0x10000-top_margin_fpart)*((float)1/0x10000);
						else
							new_opacity = 1;
						if (patch_line==patch->bottom-patch->top-
								bottom_margin_ipart-1||
								patch->lines[patch_line+bottom_margin_ipart+1].
									left>patch_column ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									right<=patch_column)
							new_opacity *= (0x10000-bottom_margin_fpart)*
								((float)1/0x10000);
						if (this_pixel == line_start)
							new_opacity *=
								(0x10000-left_margin_fpart)*((float)1/0x10000);
						if (this_pixel == line_end-1)
							new_opacity *= (0x10000-right_margin_fpart)*
								((float)1/0x10000);
						pixel_shade = (unsigned char)(new_opacity*this_shade);
					}
					else if (fade_flag == 2)
					{
						new_opacity = (float)1/255*
							patch->lines[patch_line].weight[
							patch_column-patch->lines[patch_line].left];
						pixel_shade = (unsigned char)(new_opacity*this_shade);
					}
					else
					{	assert(fade_flag == 0);
						new_opacity = 1;
						pixel_shade = this_shade;
					}
					old_shade = *this_pixel;
					*this_pixel =
						old_shade == OBJECT_IMAGE_BACKGROUND
						?	pixel_shade
						:	(unsigned char)((1-new_opacity*materl_opacity[0])*
								old_shade)+pixel_shade;
					*z_pixel = this_z;
				}
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: tquick_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    materl_opacity[0] must be set.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 9/19/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::tquick_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, *row_y_table,
		column_z_factor, slice_z_factor, row_z_factor, jx, jy, jz, coln,
		column_x_table[2048], column_y_table[2048], column_z_table[2048],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	double slice_y_factor = 0x10000*projection_matrix[1][2],
		row_y_factor = 0x10000*projection_matrix[1][1],
		column_y_factor = 0x10000*projection_matrix[1][0];
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double top_margin, bottom_margin, left_margin, right_margin, voxel_depth,
		temp_matrix[3][3], temp_vector[3];

	row_y_table = (int *)malloc(object_data->rows*sizeof(int));
	order =	(projection_matrix[2][0]>=0? CO_FW: CO_BW) +
			(projection_matrix[2][1]>=0? RO_FW: RO_BW) +
			(projection_matrix[2][2]>=0? SL_FW: SL_BW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	for (this_row=0; this_row<object_data->rows; this_row++)
		row_y_table[this_row] = (int)rint(this_row*row_y_factor);
	jx = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{
		jy = (int)rint(coln*column_y_factor);
		column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jz += column_z_factor;
	}
	if (fade_flag == 2)
	{
		error_code = VInvertMatrix(temp_matrix[0], projection_matrix[0], 3);
		if (error_code)
		{
			free(row_y_table);
			return (error_code);
		}
		temp_vector[0] = temp_vector[1] = 0;
		temp_vector[2] = 1;
		matrix_vector_multiply(temp_vector, temp_matrix, temp_vector);
		voxel_depth = sqrt((object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data))/
			(object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]*
			temp_vector[0]*temp_vector[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]*
			temp_vector[1]*temp_vector[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data)*
			temp_vector[2]*temp_vector[2]));
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, voxel_depth);
		if (error_code)
		{
			free(row_y_table);
			return (error_code);
		}
		trim_patch(patch);
	}
	else
	{
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, 1.);
		if (error_code)
		{
			free(row_y_table);
			return (error_code);
		}
	}
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = next_ptr_ptr[1]-2;
					next_ptr = *next_ptr_ptr;
					for (; this_ptr>=next_ptr; this_ptr-=2)
						t_paint_one_voxel(this_row_z, angle_shade,
							this_row_x, this_row_y,
							column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_paint_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr= *next_ptr_ptr-2;
					for (next_ptr=next_ptr_ptr[-1];
							this_ptr>=next_ptr; this_ptr-=2)
						t_paint_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_paint_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = next_ptr_ptr[1]-2;
					next_ptr = *next_ptr_ptr;
					for (; this_ptr>=next_ptr; this_ptr-=2)
						t_paint_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=2)
						t_paint_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr= *next_ptr_ptr-2;
					for (next_ptr=next_ptr_ptr[-1];
							this_ptr>=next_ptr; this_ptr-=2)
						t_paint_one_voxel(this_row_z, angle_shade,
							this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					for (next_ptr=*next_ptr_ptr;this_ptr<next_ptr; this_ptr+=2)
						t_paint_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(row_y_table);
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: t_paint_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class BINARY_B to an
 *    image buffer using a single pixel.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables materl_opacity,
 *    must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 9/20/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::t_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[2048],
	int column_y_table[2048], int column_z_table[2048],
	unsigned short *this_ptr, Object_image *object_image)
{
	int this_x, this_y, this_z, *z_pixel, this_column, this_angle_shade;
	unsigned char this_shade, *this_pixel, old_shade;

	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
	{
		this_column = (int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
		this_x = (this_row_x+column_x_table[this_column])/0x10000;
		this_y = (this_row_y+column_y_table[this_column])/0x10000;
		z_pixel = object_image->z_buffer[this_y]+this_x;
		this_pixel = (unsigned char *)object_image->image[this_y]+this_x;
		this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
		this_shade = (unsigned char)(((unsigned)this_angle_shade*this_z/
			SHADE_SCALE_FACTOR)*materl_opacity[0]);
		old_shade = *this_pixel;
		*this_pixel =
			old_shade == OBJECT_IMAGE_BACKGROUND
			?	this_shade
			:	(unsigned char)((1-materl_opacity[0])*old_shade)+this_shade;
		*z_pixel = this_z;
	}
}

/*****************************************************************************
 * FUNCTION: bperspec_patch_project
 * DESCRIPTION: Renders a binary SHELL0 object, by perspective projection
 *    using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: The variables prspectiv must be properly set.
 *    Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 5/16/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bperspec_patch_project(Shell_data *object_data,
	Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_column, this_row, this_slice, this_angle_shade;
	double this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor;
	int coln, *z_pixel;
	double column_x_table[2048], column_y_table[2048], column_z_table[2048];
	int patch_top, patch_bottom, left_end, right_end, this_x, this_y, this_z;
	double this_slice_x, this_slice_y, this_slice_z;
	int this_line, patch_line, this_shade, error_code, center_loc[2];
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = projection_matrix[0][0];
	row_x_factor = projection_matrix[0][1];
	slice_x_factor = projection_matrix[0][2];
	column_y_factor = projection_matrix[1][0];
	row_y_factor = projection_matrix[1][1];
	slice_y_factor = projection_matrix[1][2];
	column_z_factor = projection_matrix[2][0];
	row_z_factor = projection_matrix[2][1];
	slice_z_factor = projection_matrix[2][2];
	/* Use coordinates with units of 1. pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = MIDDLE_DEPTH+coln*column_z_factor;
	}
	error_code = get_patch(&patch, projection_matrix);
	if (error_code)
		return (error_code);
	this_slice_x = (projection_offset[0]+.25);
	this_slice_y = (projection_offset[1]+.25);
	this_slice_z = projection_offset[2];
	if (object_image->projected == ONE_TO_ONE)
	{
		center_loc[0] = object_image->image_location[0]/2;
		center_loc[1] = object_image->image_location[1]/2;
	}
	else
	{
		center_loc[0] = object_image->image_location[0];
		center_loc[1] = object_image->image_location[1];
	}
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column =
						(int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
					this_z = (int)(this_row_z+column_z_table[this_column]);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_y+column_y_table[this_column]+
						center_loc[1])+.5);
					this_shade =
						(unsigned)this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					patch_line = 0;
					patch_top = this_y+patch->top;
					patch_bottom = this_y+patch->bottom;
					assert(patch_top >= 0);
					assert(patch_bottom <= object_image->image_size);
					for (this_line=patch_top; this_line<patch_bottom;
							this_line++, patch_line++)
					{	left_end = this_x+patch->lines[patch_line].left;
						right_end = this_x+patch->lines[patch_line].right;
						assert(left_end >= 0);
						assert(right_end <= object_image->image_size);
						this_pixel =
							object_image->image[this_line]+(int)left_end;
						line_end=object_image->image[this_line]+(int)right_end;
						z_pixel =
							object_image->z_buffer[this_line]+(int)left_end;
						for (; this_pixel<line_end; this_pixel++, z_pixel++)
							if (this_z > *z_pixel)
							{	*z_pixel = this_z;
								*this_pixel = this_shade;
							}
					}
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: bperspec_project
 * DESCRIPTION: Renders a binary SHELL0 object, by perspective projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: The variables prspectiv must be properly set.
 *    Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 5/16/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bperspec_project(Shell_data *object_data, Object_image *object_image
	, double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade;
	double this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor;
	int this_x, this_y, this_z, coln, center_loc[2];
	double column_x_table[2048], column_y_table[2048], column_z_table[2048];
	double this_slice_x, this_slice_y, this_slice_z;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	column_x_factor = projection_matrix[0][0];
	row_x_factor = projection_matrix[0][1];
	slice_x_factor = projection_matrix[0][2];
	column_y_factor = projection_matrix[1][0];
	row_y_factor = projection_matrix[1][1];
	slice_y_factor = projection_matrix[1][2];
	column_z_factor = projection_matrix[2][0];
	row_z_factor = projection_matrix[2][1];
	slice_z_factor = projection_matrix[2][2];
	/* Use coordinates with units of 1. pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = MIDDLE_DEPTH+coln*column_z_factor;
	}
	this_slice_x = projection_offset[0];
	this_slice_y = projection_offset[1];
	this_slice_z = projection_offset[2];
	if (object_image->projected == ONE_TO_ONE)
	{
		center_loc[0] = object_image->image_location[0]/2;
		center_loc[1] = object_image->image_location[1]/2;
	}
	else
	{
		center_loc[0] = object_image->image_location[0];
		center_loc[1] = object_image->image_location[1];
	}
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
			return (401);
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column =
						(int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
					this_z = (int)(this_row_z+column_z_table[this_column]+.5);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-prspectiv)/
						(100*Z_BUFFER_LEVELS-prspectiv*this_z)*
						(this_row_y+column_y_table[this_column]+
						center_loc[1])+.5);
					if (this_z < object_image->z_buffer[this_y][this_x])
						continue;
					object_image->image[this_y][this_x] = (unsigned)
						this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					object_image->z_buffer[this_y][this_x] = this_z;
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
			this_row_y += row_y_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: bquick_project
 * DESCRIPTION: Renders a binary SHELL0 object, by parallel projection using
 *    one pixel for each voxel.  (If the scale is too large, there will be
 *    holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.  The data must be in memory.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   32
 *       ncode             0 to 5
 *       y1                6 to 16
 *       tt                not stored
 *       n1                17 to 19
 *       n2                20 to 25
 *       n3                26 to 31
 *       gm, op, pg        not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 1 byte
 *       per pixel.  The image values are 0 (black) to
 *       OBJECT_IMAGE_BACKGROUND-1 (bright), OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.  opacity_buffer and
 *       likelihood_buffer are not used and need not be allocated.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 * RETURN VALUE:
 *    0: successful
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 5/17/01 by Dewey Odhner
 *    Modified: 12/26/16 y factors refined by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::bquick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[BG_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, *row_y_table,
		column_z_factor, slice_z_factor, row_z_factor, coln,
		column_x_table[2048], column_y_table[2048], column_z_table[2048],
		this_x, this_y, this_z,
		this_slice_x, this_slice_y, this_slice_z;
	double slice_y_factor = 0x10000*projection_matrix[1][2],
		row_y_factor = 0x10000*projection_matrix[1][1],
		column_y_factor = 0x10000*projection_matrix[1][0];
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;

	row_y_table = (int *)malloc(object_data->rows*sizeof(int));
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (this_row=0; this_row<object_data->rows; this_row++)
		row_y_table[this_row] = (int)rint(this_row*row_y_factor);
	for (coln=0; coln<= Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = (int)rint(coln*column_y_factor);
		column_z_table[coln] =
			(int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (check_event && check_event(this)==FIRST)
		{
			free(row_y_table);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
			this_slice*slice_y_factor);
		this_row_z = this_slice_z;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{
			this_row_y = this_slice_y+row_y_table[this_row];
			for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column =
						(int)(*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0);
					this_x = (this_row_x+column_x_table[this_column])/
						(unsigned)0x10000;
					this_y = (this_row_y+column_y_table[this_column])/
						(unsigned)0x10000;
					this_z = (this_row_z+column_z_table[this_column])/
						Z_SUBLEVELS;
					if (this_z < object_image->z_buffer[this_y][this_x])
						continue;
					object_image->image[this_y][this_x] = (unsigned)
						this_angle_shade*this_z/SHADE_SCALE_FACTOR;
					object_image->z_buffer[this_y][this_x] = this_z;
				}
			}
			next_ptr_ptr++;
			this_row_x += row_x_factor;
			this_row_z += row_z_factor;
		}
		this_slice_x += slice_x_factor;
		this_slice_z += slice_z_factor;
	}
	free(row_y_table);
	return (0);
}

/*****************************************************************************
 * FUNCTION: gr_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 3/14/94 parameters changed by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *))
{
	if (fade_flag || need_patch(projection_matrix))
		return (gr_patch_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
	else
		return (gr_quick_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
}


/*****************************************************************************
 * FUNCTION: gr_patch_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 5/17/94 cleaned up pointer arithmetic by Dewey Odhner
 *    Modified: 10/20/98 weighted patch used by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin, voxel_depth,
		temp_matrix[3][3], temp_vector[3];

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}

#define Get_this_slice \
{	error_code = \
		load_this_slice(object_data, this_slice, &this_slice_ptr); \
	if (error_code) \
		return (error_code); \
}

	if (fade_flag == 2)
	{
		error_code = VInvertMatrix(temp_matrix[0], projection_matrix[0], 3);
		if (error_code)
			return (error_code);
		temp_vector[0] = temp_vector[1] = 0;
		temp_vector[2] = 1;
		matrix_vector_multiply(temp_vector, temp_matrix, temp_vector);
		voxel_depth = sqrt((object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data))/
			(object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]*
			temp_vector[0]*temp_vector[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]*
			temp_vector[1]*temp_vector[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data)*
			temp_vector[2]*temp_vector[2]));
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, voxel_depth);
		if (error_code)
			return (error_code);
		trim_patch(patch);
	}
	else
	{
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, 1.);
		if (error_code)
			return (error_code);
	}
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	ncode = this_ptr[0]>>10
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[2]>>8
 *	op = this_ptr[2]&255
 */

						{	gr_patch_one_voxel(this_row_z, angle_shade,
								this_row_x, this_row_y,
								column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	gr_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	gr_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	gr_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_patch_one_voxel(this_row_z, angle_shade, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: load_this_slice
 * DESCRIPTION: Loads a slice of a shell of class GRADIENT or PERCENT and sets
 *    the global variable slice_buffer to point to that data.
 * PARAMETERS:
 *    object_data: The shell for which a slice is to be loaded.  The shell
 *       data must be in object_data->file->file_header.gen.filename.
 *    this_slice: The slice to be loaded, numbered from 0.
 *    this_slice_ptr: The value in object_data->ptr_table for the beginning
 *       of this slice will be stored here.
 * SIDE EFFECTS: Memory at slice_buffer may be overwritten or freed.
 * ENTRY CONDITIONS: The global variable slice_buffer must not be set outside
 *    this function.
 * RETURN VALUE: One of the 3DVIEWNIX error codes.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/5/93 to use VReadData by Dewey Odhner
 *    Modified: 4/21/94 to set slice_buffer if object_data->in_memory
 *       by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::load_this_slice(Shell_data *object_data, int this_slice,
	unsigned short **this_slice_ptr)
{
	int slice_size, error_code, items_read;
	FILE *object_file;

	if (object_data->in_memory)
	{	*this_slice_ptr = slice_buffer =
			object_data->ptr_table[this_slice*object_data->rows];
		return (0);
	}
	object_file = fopen(object_data->file->file_header.gen.filename, "rb");
	if (object_file == NULL)
		return (4);
	if (fseek(object_file, (long)
			(size_t)object_data->ptr_table[this_slice*object_data->rows], 0))
	{	fclose(object_file);
		return (5);
	}
	*this_slice_ptr = object_data->ptr_table[this_slice*object_data->rows];
	slice_size= (int)(object_data->ptr_table[(this_slice+1)*object_data->rows]-
		*this_slice_ptr);
	if (slice_size > slice_buffer_size)
	{	if (static_slice_buffer)
			free(static_slice_buffer);
		static_slice_buffer = (unsigned short *)malloc(slice_size*2);
		if (static_slice_buffer == NULL)
		{	slice_buffer_size = 0;
			fclose(object_file);
			return (1);
		}
		slice_buffer_size = slice_size;
	}
	else if (slice_buffer_size == 0)
	{	static_slice_buffer = (unsigned short *)malloc(2);
		if (static_slice_buffer == NULL)
		{	slice_buffer_size = 0;
			fclose(object_file);
			return (1);
		}
		slice_buffer_size = 1;
	}
	error_code =
		VReadData((char*)static_slice_buffer, 2, slice_size, object_file,&items_read);
	if (error_code==0 && items_read!=slice_size)
		error_code = 2;
	fclose(object_file);
	slice_buffer = static_slice_buffer;
	return (error_code);
}

/*****************************************************************************
 * FUNCTION: gr_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class GRADIENT to an
 *    image buffer using a patch of pixels.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variable fade_flag must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *    Modified: 2/17/94 shade_computed flag added by Dewey Odhner
 *    Modified: 7/8/94 efficiency improved by Dewey Odhner
 *    Modified: 10/27/98 weights used if fade_flag == 2 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::gr_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024], int itop_margin,
	int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image)
{
	unsigned char opacity, *opacity_ptr, likelihood, *likelihood_ptr;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,
		this_angle_shade, shade_computed;
	Pixel_unit this_shade=0, *this_pixel, *line_start, *line_end;

	if ((this_angle_shade=angle_shade[this_ptr[1]]))
	{	opacity = this_ptr[2]&255;
		likelihood = this_ptr[2]>>8;
		this_column = (int)(*this_ptr&0x3ff);
		icoord = this_row_x+column_x_table[this_column];
		this_x = icoord/0x10000;
		icoord -= this_x*0x10000+0x8000;
		left_margin_fpart = ileft_margin+icoord;
		left_margin_ipart = left_margin_fpart/0x10000;
		left_margin_fpart -= left_margin_ipart*0x10000;
		right_margin_fpart = iright_margin-icoord;
		right_margin_ipart = right_margin_fpart/0x10000;
		right_margin_fpart -= right_margin_ipart*0x10000;
		icoord = this_row_y+column_y_table[this_column];
		this_y = icoord/0x10000;
		icoord -= this_y*0x10000+0x8000;
		top_margin_fpart = itop_margin+icoord;
		top_margin_ipart = top_margin_fpart/0x10000;
		top_margin_fpart -= top_margin_ipart*0x10000;
		bottom_margin_fpart = ibottom_margin-icoord;
		bottom_margin_ipart = bottom_margin_fpart/0x10000;
		bottom_margin_fpart -= bottom_margin_ipart*0x10000;
		if (fade_flag == 2)
			top_margin_ipart = bottom_margin_ipart =
				left_margin_ipart = right_margin_ipart = 0;
		shade_computed = FALSE;
		patch_line = top_margin_ipart;
		patch_top = this_y+patch->top+patch_line;
		patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
		for (this_line=patch_top; this_line<patch_bottom;
				this_line++, patch_line++)
		{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
			left_end = this_x+patch_column;
			right_end =
				this_x+patch->lines[patch_line].right-right_margin_ipart;
			line_start = (Pixel_unit *)object_image->image[this_line]+left_end;
			opacity_ptr = object_image->opacity_buffer[this_line]+left_end;
			z_pixel = object_image->z_buffer[this_line]+left_end;
			likelihood_ptr =
				object_image->likelihood_buffer[this_line]+left_end;
			line_end = (Pixel_unit *)object_image->image[this_line]+right_end;
			for (this_pixel=line_start; this_pixel<line_end;
					this_pixel++, z_pixel++, opacity_ptr++,
						likelihood_ptr++, patch_column++)
			{	unsigned char old_opacity, pixel_likelihood;
				int old_transparency;
				float new_opacity;
				Pixel_unit old_shade, pixel_shade;

				old_opacity = *opacity_ptr;
				if (old_opacity<MAX_OPACITY &&
						patch->lines[patch_line-top_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line-top_margin_ipart].right>
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].right>
							patch_column)
				{	if (!shade_computed)
					{	this_z = (this_row_z+column_z_table[this_column])/
							Z_SUBLEVELS;
						this_shade = ((unsigned)this_angle_shade*this_z/
							V_SHADE_SCALE_FACTOR)*opacity/255;
						shade_computed = TRUE;
					}
					old_transparency = 255-old_opacity;
					if (fade_flag == 1)
					{	if ((patch_line==top_margin_ipart ||
								patch->lines[patch_line-top_margin_ipart-1].
									left>patch_column ||
								patch->lines[patch_line-top_margin_ipart-1].
									right<=patch_column))
							new_opacity =
								(0x10000-top_margin_fpart)*((float)1/0x10000);
						else
							new_opacity = 1;
						if (patch_line==patch->bottom-patch->top-
								bottom_margin_ipart-1||
								patch->lines[patch_line+bottom_margin_ipart+1].
									left>patch_column ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									right<=patch_column)
							new_opacity *= (0x10000-bottom_margin_fpart)*
								((float)1/0x10000);
						if (this_pixel == line_start)
							new_opacity *=
								(0x10000-left_margin_fpart)*((float)1/0x10000);
						if (this_pixel == line_end-1)
							new_opacity *= (0x10000-right_margin_fpart)*
								((float)1/0x10000);
						pixel_likelihood =
							(unsigned char)(likelihood*new_opacity);
						pixel_shade = (Pixel_unit)(new_opacity*this_shade);
						*opacity_ptr += 255-(int)
							(255-old_transparency*new_opacity*opacity/255);
					}
					else if (fade_flag == 2)
					{
						new_opacity = (float)1/255*
							patch->lines[patch_line].weight[
							patch_column-patch->lines[patch_line].left];
						pixel_likelihood =
							(unsigned char)(likelihood*new_opacity);
						pixel_shade = (Pixel_unit)(new_opacity*this_shade);
						*opacity_ptr += 255-(int)
							(255-old_transparency*new_opacity*opacity/255);
					}
					else
					{	assert(fade_flag == 0);
						pixel_likelihood = likelihood;
						pixel_shade = this_shade;
						*opacity_ptr += old_transparency*opacity/255;
					}
					old_shade = *this_pixel;
					*this_pixel =
						old_shade == V_OBJECT_IMAGE_BACKGROUND
						?	pixel_shade
						:	old_shade+old_transparency*pixel_shade/255;
					if (pixel_likelihood >= *likelihood_ptr)
					{	*likelihood_ptr = pixel_likelihood;
						*z_pixel = this_z;
					}
				}
			}
		}
	}
}


/*****************************************************************************
 * MACRO: Paint_one_voxel
 * DESCRIPTION: Projects one voxel of a SHELL0 of class GRADIENT to one pixel
 *    of an object image buffer.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The following variables must be valid:
 *    int this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per
 *       depth unit in the left-handed image coordinate system.
 *    int angle_shade[G_CODES]: A look-up table which maps gradient codes to
 *       shades; 0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    int this_row_x, this_row_y: The object image buffer coordinates of the
 *       row in 0x10000 units per pixel.
 *    int column_x_table[1024], column_y_table[1024], column_z_table[1024]:
 *       Lookup tables giving the coordinates of a voxel relative to the row,
 *       same units as row coordinates, indexed by y1 value of the voxel.
 *    unsigned short *this_ptr: Address of the TSE for the voxel to be
 *       projected.
 *    Object_image *object_image: The object image to be projected.  The image
 *       buffer must be big enough for the projection of the object.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *
 *****************************************************************************/
#define Paint_one_voxel \
{	int this_angle_shade, this_column, this_x, this_y, this_z, *z_pixel; \
	unsigned char opacity, *opacity_ptr, transparency, \
		likelihood, *likelihood_ptr; \
	Pixel_unit this_shade, *this_pixel; \
\
	if ((this_angle_shade=angle_shade[this_ptr[1]])) \
	{	Pixel_unit old_shade; \
\
		this_column = (int)(*this_ptr&0x3ff); \
		this_x = (this_row_x+column_x_table[this_column])/0x10000; \
		this_y = (this_row_y+column_y_table[this_column])/0x10000; \
		this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS; \
		opacity = this_ptr[2]&255; \
		likelihood = this_ptr[2]>>8; \
		this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x; \
		opacity_ptr = object_image->opacity_buffer[this_y]+this_x; \
		likelihood_ptr = object_image->likelihood_buffer[this_y]+this_x; \
		transparency = 255-*opacity_ptr; \
		z_pixel = object_image->z_buffer[this_y]+this_x; \
		if (transparency > 255-MAX_OPACITY) \
		{ \
			this_shade = ((unsigned)this_angle_shade*this_z/ \
				V_SHADE_SCALE_FACTOR)*opacity/255; \
			*opacity_ptr += (int)transparency*opacity/255; \
			old_shade = *this_pixel; \
			*this_pixel = \
				old_shade == V_OBJECT_IMAGE_BACKGROUND \
				?	this_shade \
				:	old_shade+transparency*this_shade/255; \
			if (likelihood >= *likelihood_ptr) \
			{	*likelihood_ptr = likelihood; \
				*z_pixel = this_z; \
			} \
		} \
	} \
}


/*****************************************************************************
 * FUNCTION: gr_quick_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    projection using one pixel for each voxel.  (If the scale is too large,
 *    there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 5/17/94 cleaned up pointer arithmetic by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, *row_y_table,
		column_z_factor, slice_z_factor, row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, error_code;
	double slice_y_factor = 0x10000*projection_matrix[1][2],
		row_y_factor = 0x10000*projection_matrix[1][1],
		column_y_factor = 0x10000*projection_matrix[1][0];
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	row_y_table = (int *)malloc(object_data->rows*sizeof(int));
	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (this_row=0; this_row<object_data->rows; this_row++)
		row_y_table[this_row] = (int)rint(this_row*row_y_factor);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = (int)rint(coln*column_y_factor);
		column_z_table[coln] = 
			(int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	Paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Paint_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{
						this_row_y = this_slice_y+row_y_table[this_row];
						this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	Paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Paint_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{
						this_row_y = this_slice_y+row_y_table[this_row];
						this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	Paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Paint_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{
						this_row_y = this_slice_y+row_y_table[this_row];
						this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	Paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(row_y_table);
					return (401);
				}
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_slice_y = (int)rint(0x10000*(projection_offset[1]/*+*/)+
					this_slice*slice_y_factor);
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{
					this_row_y = this_slice_y+row_y_table[this_row];
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Paint_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(row_y_table);
	return (0);
}

/*****************************************************************************
 * FUNCTION: gr_mip_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    maximum intensity projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.  The static variable
 *    mip_lut will be initialized.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/30/94 by Dewey Odhner
 *    Modified: 7/8/94 to initialize mip_lut by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_mip_project(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	Priority (*check_event)(cvRenderer *))
{
	int j;

	if (mip_lut[1] == 0)
		for (j=0; j<256; j++)
			mip_lut[j] = j*(V_OBJECT_IMAGE_BACKGROUND-1)/255;
	if (fade_flag || need_patch(projection_matrix))
		return (gr_mip_patch_project(object_data, object_image,
			projection_matrix, projection_offset, check_event));
	else
		return (gr_mip_quick_project(object_data, object_image,
			projection_matrix, projection_offset, check_event));
}


/*****************************************************************************
 * FUNCTION: gr_mip_patch_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    maximum intensity projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 6/30/94 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_mip_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
		&left_margin, &right_margin, projection_matrix, 1.);
	if (error_code)
		return (error_code);
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	ncode = this_ptr[0]>>10
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[2]>>8
 *	op = this_ptr[2]&255
 */

						{	gr_mip_patch_one_voxel(this_row_z,
								this_row_x, this_row_y,
								column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_mip_patch_one_voxel(this_row_z, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	gr_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_mip_patch_one_voxel(this_row_z, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	gr_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_mip_patch_one_voxel(this_row_z, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	gr_mip_patch_one_voxel(this_row_z, this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
			this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						gr_mip_patch_one_voxel(this_row_z, this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: gr_mip_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class GRADIENT to an
 *    image buffer using a patch of pixels, maximum intensity projection.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, mip_lut must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 7/1/94 by Dewey Odhner
 *    Modified: 7/8/94 to use mip_lut, & store MI z-value by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::gr_mip_patch_one_voxel(int this_row_z, int this_row_x,
	int this_row_y, int column_x_table[1024], int column_y_table[1024],
	int column_z_table[1024], int itop_margin, int ibottom_margin,
	int ileft_margin,  int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image)
{
	unsigned char opacity;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,
		z_computed;
	Pixel_unit this_shade, *this_pixel, *line_start, *line_end, pixel_shade;

	opacity = this_ptr[2]&255;
	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord -= this_x*0x10000+0x8000;
	left_margin_fpart = ileft_margin+icoord;
	left_margin_ipart = left_margin_fpart/0x10000;
	left_margin_fpart -= left_margin_ipart*0x10000;
	right_margin_fpart = iright_margin-icoord;
	right_margin_ipart = right_margin_fpart/0x10000;
	right_margin_fpart -= right_margin_ipart*0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	icoord -= this_y*0x10000+0x8000;
	top_margin_fpart = itop_margin+icoord;
	top_margin_ipart = top_margin_fpart/0x10000;
	top_margin_fpart -= top_margin_ipart*0x10000;
	bottom_margin_fpart = ibottom_margin-icoord;
	bottom_margin_ipart = bottom_margin_fpart/0x10000;
	bottom_margin_fpart -= bottom_margin_ipart*0x10000;
	this_shade = mip_lut[opacity];
	z_computed = FALSE;
	patch_line = top_margin_ipart;
	patch_top = this_y+patch->top+patch_line;
	patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
	for (this_line=patch_top; this_line<patch_bottom;
			this_line++, patch_line++)
	{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
		left_end = this_x+patch_column;
		right_end =
			this_x+patch->lines[patch_line].right-right_margin_ipart;
		line_start = (Pixel_unit *)object_image->image[this_line]+left_end;
		z_pixel = object_image->z_buffer[this_line]+left_end;
		line_end = (Pixel_unit *)object_image->image[this_line]+right_end;
		for (this_pixel=line_start; this_pixel<line_end;
				this_pixel++, z_pixel++, patch_column++)
		{	if (patch->lines[patch_line-top_margin_ipart].left<=patch_column &&
					patch->lines[patch_line-top_margin_ipart].right>
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].left<=
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].right>
						patch_column)
			{	if (fade_flag)
				{	float new_opacity;

					if ((patch_line==top_margin_ipart ||
							patch->lines[patch_line-top_margin_ipart-1].
								left>patch_column ||
							patch->lines[patch_line-top_margin_ipart-1].
								right<=patch_column))
						new_opacity =
							(0x10000-top_margin_fpart)*((float)1/0x10000);
					else
						new_opacity = 1;
					if (patch_line==patch->bottom-patch->top-
							bottom_margin_ipart-1||
							patch->lines[patch_line+bottom_margin_ipart+1].
								left>patch_column ||
							patch->lines[patch_line+bottom_margin_ipart+1].
								right<=patch_column)
						new_opacity *= (0x10000-bottom_margin_fpart)*
							((float)1/0x10000);
					if (this_pixel == line_start)
						new_opacity *=
							(0x10000-left_margin_fpart)*((float)1/0x10000);
					if (this_pixel == line_end-1)
						new_opacity *= (0x10000-right_margin_fpart)*
							((float)1/0x10000);
					pixel_shade = (Pixel_unit)(new_opacity*this_shade);
				}
				else
					pixel_shade = this_shade;
				if (*this_pixel==V_OBJECT_IMAGE_BACKGROUND ||
						pixel_shade>*this_pixel)
				{	*this_pixel = pixel_shade;
					if (!z_computed)
					{	this_z = (this_row_z+column_z_table[this_column])/
							Z_SUBLEVELS;
						z_computed = TRUE;
					}
					*z_pixel = this_z;
				}
			}
		}
	}
}


/*****************************************************************************
 * MACRO: Mip_paint_one_voxel
 * DESCRIPTION: Projects one voxel of a SHELL0 of class GRADIENT to one pixel
 *    of an object image buffer, maximum intensity projection.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The following variables must be valid:
 *    int this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per
 *       depth unit in the left-handed image coordinate system.
 *    int this_row_x, this_row_y: The object image buffer coordinates of the
 *       row in 0x10000 units per pixel.
 *    int column_x_table[1024], column_y_table[1024], column_z_table[1024]:
 *       Lookup tables giving the coordinates of a voxel relative to the row,
 *       same units as row coordinates, indexed by y1 value of the voxel.
 *    unsigned short *this_ptr: Address of the TSE for the voxel to be
 *       projected.
 *    Object_image *object_image: The object image to be projected.  The image
 *       buffer must be big enough for the projection of the object.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/1/94 by Dewey Odhner
 *    Modified: 7/8/94 to use mip_lut, & store MI z-value by Dewey Odhner
 *
 *****************************************************************************/
#define Mip_paint_one_voxel \
{	int this_column, this_x, this_y; \
	Pixel_unit *this_pixel, this_shade; \
\
	this_column = (int)(*this_ptr&0x3ff); \
	this_x = (this_row_x+column_x_table[this_column])/0x10000; \
	this_y = (this_row_y+column_y_table[this_column])/0x10000; \
	this_shade = mip_lut[this_ptr[2]&255]; \
	this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x; \
	if (*this_pixel==V_OBJECT_IMAGE_BACKGROUND || \
			this_shade>*this_pixel) \
	{	*this_pixel = this_shade; \
		object_image->z_buffer[this_y][this_x] = \
			(this_row_z+column_z_table[this_column])/Z_SUBLEVELS; \
	} \
}


/*****************************************************************************
 * FUNCTION: gr_mip_quick_project
 * DESCRIPTION: Renders a SHELL0 object of class GRADIENT by parallel
 *    maximum intensity projection using one pixel for each voxel.
 *    (If the scale is too large,  there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                40 to 47
 *       pg                not stored
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 2 bytes
 *       per pixel.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables slice_buffer, fade_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 7/1/94 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::gr_mip_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	Mip_paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Mip_paint_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	Mip_paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Mip_paint_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	Mip_paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Mip_paint_one_voxel
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	Mip_paint_one_voxel
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						Mip_paint_one_voxel
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: initialize_times_tables
 * DESCRIPTION: Initializes the static variables one_minus_opacity_times,
 *    red_times_square_of, green_times_square_of, blue_times_square_of,
 *    sstrnth_table, sstrnth_ambient_table, gray_flag.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables materl_opacity, materl_red,
 *    materl_green, materl_blue, visual, surf_red_factr,
 *    surf_green_factr, surf_blue_factr, background, surf_strenth,
 *    ambient, emis_power, surf_ppower must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 6/19/93 to initialize sstrnth_table, sstrnth_ambient_table
 *       by Dewey Odhner
 *    Modified: 9/1/93 to use ambient components by Dewey Odhner
 *    Modified: 9/24/93 to check windows_open by Dewey Odhner
 *    Modified: 3/15/94 to eliminate side effects Dewey Odhner
 *    Modified: 4/5/94 put surface factors in table by Dewey Odhner
 *    Modified: 4/25/96 square tables made general by Dewey Odhner
 *    Modified: 4/26/96 surf_pct_table, surf_opac_table set by Dewey Odhner
 *    Modified: 5/1/96 surfaces given material colors if surface color
 *       factors are zero by Dewey Odhner
 *    Modified: 5/3/96 surfaces scaled up given material colors if surface
 *       color factors are zero by Dewey Odhner
 *    Modified: 10/6/98 material colors checked if surface color
 *       factors are zero by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::initialize_times_tables(void)
{
#define Pow(x, y) ((y)==0? 1.0: pow((double)(x), (double)(y)))

	int materl, ip, color;
	float p=0, p_pow, surface_color[4][3];

	for (materl=0; materl<4; materl++)
		if (materl_opacity[materl]!=old_materl_opacity[materl] ||
				materl_red[materl]!=old_materl_red[materl] ||
				materl_green[materl]!=old_materl_green[materl] ||
				materl_blue[materl]!=old_materl_blue[materl] ||
				emis_power!=old_emis_power)
			for (ip=0; ip<256; ip++)
			{	p = (float)(ip/255.);
				p_pow = (float)Pow(p, emis_power);
				one_minus_opacity_times[materl][ip] =
					1-materl_opacity[materl]*p;
				red_times_square_of[materl][ip] =
					materl_opacity[materl]*materl_red[materl]*p_pow;
				green_times_square_of[materl][ip] =
					materl_opacity[materl]*materl_green[materl]*p_pow;
				blue_times_square_of[materl][ip] =
					materl_opacity[materl]*materl_blue[materl]*p_pow;
			}
	if (surf_ppower != old_surf_ppower)
		for (ip=0; ip<256; ip++)
			surf_pct_table[ip] = (float)Pow(ip/255., surf_ppower);
	gray_flag =
		(materl_opacity[0]==0||
			(materl_red[0]==materl_green[0]&&materl_red[0]==materl_blue[0])) &&
		(materl_opacity[1]==0||
			(materl_red[1]==materl_green[1]&&materl_red[1]==materl_blue[1])) &&
		(materl_opacity[2]==0||
			(materl_red[2]==materl_green[2]&&materl_red[2]==materl_blue[2])) &&
		(materl_opacity[3]==0||
			(materl_red[3]==materl_green[3]&&materl_red[3]==materl_blue[3])) &&
		surf_red_factr==surf_green_factr &&
		surf_green_factr==surf_blue_factr &&
		ambient_light.red==ambient_light.green &&
		ambient_light.green==ambient_light.blue;
	if (surf_strenth!=old_surf_strenth || surf_ppower!=old_surf_ppower ||
			surf_red_factr!=old_surf_red_factr ||
			surf_green_factr!=old_surf_green_factr ||
			surf_blue_factr!=old_surf_blue_factr ||
			ambient_light.red!=old_ambient_light.red ||
			ambient_light.green!=old_ambient_light.green ||
			ambient_light.blue!=old_ambient_light.blue ||
			(surf_red_factr==0&&surf_green_factr==0&&surf_blue_factr==0&&p))
	{
		if (surf_red_factr==0 && surf_green_factr==0 && surf_blue_factr==0)
		{	for (materl=1; materl<4; materl++)
			{	p = materl_green[materl]>materl_red[materl]?
					materl_green[materl]:materl_red[materl];
				if (materl_blue[materl] > p)
					p = materl_blue[materl];
				surface_color[materl][0] = (float)(
					((V_OBJECT_IMAGE_BACKGROUND-1)/256.)
					*(p==0? 1:materl_red[materl]/p));
				surface_color[materl][1] = (float)(
					((V_OBJECT_IMAGE_BACKGROUND-1)/256.)
					*(p==0? 1:materl_green[materl]/p));
				surface_color[materl][2] = (float)(
					((V_OBJECT_IMAGE_BACKGROUND-1)/256.)
					*(p==0? 1:materl_blue[materl]/p));
			}
			for (ip=0; ip<256; ip++)
			{		sstrnth_table[0][0][ip] = (float)(
					surf_strenth<=50
					?	ip*(surf_strenth*.02)
					:	surf_strenth>=100
						?	255
						:	ip/(2-surf_strenth*.02));
				if (sstrnth_table[0][0][ip] > 255)
					sstrnth_table[0][0][ip] = 255;
				surf_opac_table[ip] =
					(float)Pow(sstrnth_table[0][0][ip]/255, 1-surf_ppower);
				for (materl=1; materl<4; materl++)
					sstrnth_table[materl][0][ip] = sstrnth_table[0][0][ip];
				for (materl=0; materl<4; materl++)
				{	sstrnth_ambient_table[materl][0][ip] =
						surface_color[materl][0]*surf_opac_table[ip]*
						sstrnth_table[materl][0][ip]*ambient_light.red;
					sstrnth_ambient_table[materl][1][ip] =
						surface_color[materl][1]*surf_opac_table[ip]*
						sstrnth_table[materl][0][ip]*ambient_light.green;
					sstrnth_ambient_table[materl][2][ip] =
						surface_color[materl][2]*surf_opac_table[ip]*
						sstrnth_table[materl][0][ip]*ambient_light.blue;
					sstrnth_table[materl][2][ip] = (float)(
						surface_color[materl][2]*sstrnth_table[materl][0][ip]*
						(65535-ambient_light.blue)*(1./MAX_ANGLE_SHADE));
					sstrnth_table[materl][1][ip] = (float)(
						surface_color[materl][1]*sstrnth_table[materl][0][ip]*
						(65535-ambient_light.green)*(1./MAX_ANGLE_SHADE));
					sstrnth_table[materl][0][ip] = (float)(
						surface_color[materl][0]*sstrnth_table[materl][0][ip]*
						(65535-ambient_light.red)*(1./MAX_ANGLE_SHADE));
				}
			}
		}
		else
			for (ip=0; ip<256; ip++)
			{	sstrnth_table[0][0][ip] = (float)(
					surf_strenth<=50
					?	ip*(surf_strenth*.02)
					:	surf_strenth>=100
						?	255
						:	ip/(2-surf_strenth*.02));
				if (sstrnth_table[0][0][ip] > 255)
					sstrnth_table[0][0][ip] = 255;
				surf_opac_table[ip] =
					(float)Pow(sstrnth_table[0][0][ip]/255, 1-surf_ppower);
				sstrnth_ambient_table[0][0][ip] =
					surf_red_factr*surf_opac_table[ip]*
					sstrnth_table[0][0][ip]*ambient_light.red*MAX_ANGLE_SHADE;
				sstrnth_ambient_table[0][1][ip] = surf_green_factr*
					surf_opac_table[ip]*sstrnth_table[0][0][ip]*
					ambient_light.green*MAX_ANGLE_SHADE;
				sstrnth_ambient_table[0][2][ip] =
					surf_blue_factr*surf_opac_table[ip]*
					sstrnth_table[0][0][ip]*ambient_light.blue*MAX_ANGLE_SHADE;
				sstrnth_table[0][2][ip] = surf_blue_factr*
					sstrnth_table[0][0][ip]*(65535-ambient_light.blue);
				sstrnth_table[0][1][ip] = surf_green_factr*
					sstrnth_table[0][0][ip]*(65535-ambient_light.green);
				sstrnth_table[0][0][ip] *=
					surf_red_factr*(65535-ambient_light.red);
				for (materl=1; materl<4; materl++)
					for (color=0; color<3; color++)
					{	sstrnth_table[materl][color][ip] =
							sstrnth_table[0][color][ip];
						sstrnth_ambient_table[materl][color][ip] =
							sstrnth_ambient_table[0][color][ip];
					}
			}
	}
	for (materl=0; materl<4; materl++)
	{
		old_materl_opacity[materl] = materl_opacity[materl];
		old_materl_red[materl] = materl_red[materl];
		old_materl_green[materl] = materl_green[materl];
		old_materl_blue[materl] = materl_blue[materl];
	}
	old_emis_power = emis_power;
	old_surf_ppower = surf_ppower;
	old_surf_strenth = surf_strenth;
	old_surf_red_factr = surf_red_factr;
	old_surf_green_factr = surf_green_factr;
	old_surf_blue_factr = surf_blue_factr;
	old_ambient_light = ambient_light;
}


/*****************************************************************************
 * FUNCTION: pct_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variables one_minus_opacity_times, red_times_square_of,
 *    green_times_square_of, blue_times_square_of, sstrnth_table,
 *    sstrnth_ambient_table, gray_flag, slice_buffer may be changed.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables ambient_light, fade_flag, surf_red_factr,
 *    surf_green_factr, surf_blue_factr, materl_opacity, materl_red,
 *    materl_green, materl_blue, surf_strenth must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 3/14/94 parameters changed by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	initialize_times_tables();
	if (fade_flag || need_patch(projection_matrix))
		return (pct_patch_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
	else
		return (pct_quick_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
}


/*****************************************************************************
 * FUNCTION: pct_patch_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variable slice_buffer may be changed.
 * ENTRY CONDITIONS: The static variables ambient_light, fade_flag,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 5/17/94 cleaned up pointer arithmetic by Dewey Odhner
 *    Modified: 10/20/98 weighted patch used by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin, voxel_depth,
		temp_matrix[3][3], temp_vector[3];

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	if (fade_flag == 2)
	{
		error_code = VInvertMatrix(temp_matrix[0], projection_matrix[0], 3);
		if (error_code)
			return (error_code);
		temp_vector[0] = temp_vector[1] = 0;
		temp_vector[2] = 1;
		matrix_vector_multiply(temp_vector, temp_matrix, temp_vector);
		voxel_depth = sqrt((object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data))/
			(object_data->file->file_header.str.xysize[0]*
			object_data->file->file_header.str.xysize[0]*
			temp_vector[0]*temp_vector[0]+
			object_data->file->file_header.str.xysize[1]*
			object_data->file->file_header.str.xysize[1]*
			temp_vector[1]*temp_vector[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data)*
			temp_vector[2]*temp_vector[2]));
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, voxel_depth);
		if (error_code)
			return (error_code);
		trim_patch(patch);
	}
	else
	{
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, 1.);
		if (error_code)
			return (error_code);
	}
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	ncode = this_ptr[0]>>10
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[2]>>8
 *	op = this_ptr[2]&255
 */

						{	pct_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	pct_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: pct_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class PERCENT to an
 *    image buffer using a patch of pixels.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, one_minus_opacity_times,
 *    red_times_square_of, green_times_square_of, blue_times_square_of,
 *    sstrnth_table, sstrnth_ambient_table, gray_flag, surf_pct_table,
 *    surf_opac_table must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *    Modified: 6/19/93 to use sstrnth_table, sstrnth_ambient_table
 *       by Dewey Odhner
 *    Modified: 9/1/93 to use ambient components by Dewey Odhner
 *    Modified: 2/16/94 color_computed flag added by Dewey Odhner
 *    Modified: 4/5/94 for surface factors in table by Dewey Odhner
 *    Modified: 4/29/96 surf_pct_table, surf_opac_table used by Dewey Odhner
 *    Modified: 5/1/96 surfaces colored by material by Dewey Odhner
 *    Modified: 6/10/96 back material contribution corrected by Dewey Odhner
 *    Modified: 10/27/98 weights used if fade_flag == 2 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::pct_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024], int itop_margin,
	int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image)
{
	unsigned char *opacity_ptr, *likelihood_pixel;
	unsigned front_materl=0, back_materl, this_voxel, pb, pf, likelihood;
	float this_angle_shade, this_red=0, this_green=0,
		this_blue=0, opacity=0, transparency, as, ass;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,color_computed;
	Pixel_unit *this_pixel, *line_start, *line_end;

	this_voxel = this_ptr[1];
	this_angle_shade = (float)angle_shade[this_voxel&0x7ff];
	if (this_angle_shade > 0)
	{	likelihood = this_ptr[2]>>8;
		this_column = (int)(*this_ptr&0x3ff);
		icoord = this_row_x+column_x_table[this_column];
		this_x = icoord/0x10000;
		icoord -= this_x*0x10000+0x8000;
		left_margin_fpart = ileft_margin+icoord;
		left_margin_ipart = left_margin_fpart/0x10000;
		left_margin_fpart -= left_margin_ipart*0x10000;
		right_margin_fpart = iright_margin-icoord;
		right_margin_ipart = right_margin_fpart/0x10000;
		right_margin_fpart -= right_margin_ipart*0x10000;
		icoord = this_row_y+column_y_table[this_column];
		this_y = icoord/0x10000;
		icoord -= this_y*0x10000+0x8000;
		top_margin_fpart = itop_margin+icoord;
		top_margin_ipart = top_margin_fpart/0x10000;
		top_margin_fpart -= top_margin_ipart*0x10000;
		bottom_margin_fpart = ibottom_margin-icoord;
		bottom_margin_ipart = bottom_margin_fpart/0x10000;
		bottom_margin_fpart -= bottom_margin_ipart*0x10000;
		if (fade_flag == 2)
			top_margin_ipart = bottom_margin_ipart =
				left_margin_ipart = right_margin_ipart = 0;
		patch_line = top_margin_ipart;
		patch_top = this_y+patch->top+patch_line;
		patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
		color_computed = FALSE;
		for (this_line=patch_top; this_line<patch_bottom;
				this_line++, patch_line++)
		{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
			left_end = this_x+patch_column;
			right_end =
				this_x+patch->lines[patch_line].right-right_margin_ipart;
			line_start= (Pixel_unit*)object_image->image[this_line]+left_end*3;
			opacity_ptr = object_image->opacity_buffer[this_line]+left_end;
			z_pixel = object_image->z_buffer[this_line]+left_end;
			likelihood_pixel =
				object_image->likelihood_buffer[this_line]+left_end;
			line_end = (Pixel_unit*)object_image->image[this_line]+right_end*3;
			for (this_pixel=line_start; this_pixel<line_end;
					this_pixel+=3, opacity_ptr++, z_pixel++,
						likelihood_pixel++, patch_column++)
			{	unsigned char old_opacity, old_transparency, pixel_likelihood;
				float new_opacity;

				old_opacity = *opacity_ptr;
				if (old_opacity<MAX_OPACITY && 
						patch->lines[patch_line-top_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line-top_margin_ipart].right>
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].right>
							patch_column)
				{	if (!color_computed)
					{	back_materl = this_voxel>>12;
						switch (back_materl)
						{	case 0:
							case 1:
								front_materl = 0;
								break;
							case 2:
								front_materl = 1;
								break;
							case 3:
								front_materl = 2;
						}
						pb = this_ptr[2]&255;
						pf = 255-pb;
						this_red= red_times_square_of[front_materl][pf];
						if (!gray_flag)
						{	this_green=green_times_square_of[front_materl][pf];
							this_blue = blue_times_square_of[front_materl][pf];
						}
						as = materl_opacity[back_materl]*surf_pct_table[pb];
						transparency= one_minus_opacity_times[back_materl][pb]*
							(1-as)*one_minus_opacity_times[front_materl][pf];
						opacity = 1-transparency;
						ass = as*(float)(1/65535.);
						this_red +=
							one_minus_opacity_times[front_materl][pf]*
							(ass*(this_angle_shade*
							 sstrnth_table[back_materl][0][likelihood]+
							 sstrnth_ambient_table[back_materl][0][likelihood])
							 +(1-as)*red_times_square_of[back_materl][pb]);
						if (!gray_flag)
						{	this_green +=
							  one_minus_opacity_times[front_materl][pf]*
							  (ass*(this_angle_shade*
							   sstrnth_table[back_materl][1][likelihood]+
							   sstrnth_ambient_table[back_materl][1][likelihood])+
								(1-as)*green_times_square_of[back_materl][pb]);
							this_blue +=
							   one_minus_opacity_times[front_materl][pf]*
							   (ass*(this_angle_shade*
							    sstrnth_table[back_materl][2][likelihood]+
							    sstrnth_ambient_table[back_materl][2][likelihood])+
								(1-as)*blue_times_square_of[back_materl][pb]);
						}
						this_z = (this_row_z+column_z_table[this_column])/
							Z_SUBLEVELS;
						color_computed = TRUE;
					}
					if (fade_flag==1 && (patch_line==top_margin_ipart ||
							patch->lines[patch_line-top_margin_ipart-1]. 
								left>patch_column ||
							patch->lines[patch_line-top_margin_ipart-1].
								right<=patch_column))
						new_opacity =
							(float)(0x10000-top_margin_fpart)/0x10000;
					else if (fade_flag == 2)
						new_opacity = (float)1/255*
							patch->lines[patch_line].weight[
							patch_column-patch->lines[patch_line].left];
					else
						new_opacity = 1;
					if (fade_flag == 1)
					{	if (patch_line==patch->bottom-patch->top-
								bottom_margin_ipart-1 ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									left>patch_column ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									right<=patch_column)
							new_opacity *=
								(float)(0x10000-bottom_margin_fpart)/0x10000;
						if (this_pixel == line_start)
							new_opacity *=
								(float)(0x10000-left_margin_fpart)/0x10000;
						if (this_pixel == line_end-3)
							new_opacity *=
								(float)(0x10000-right_margin_fpart)/0x10000;
					}
					old_transparency = 255-old_opacity;
					*opacity_ptr += 255-
						(int)(255-old_transparency*new_opacity*opacity);
					if (old_opacity == 0)
					{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
						if (gray_flag)
							this_pixel[1] = this_pixel[2] = this_pixel[0];
						else
						{	this_pixel[1] =
								(unsigned short)(new_opacity*this_green);
							this_pixel[2] =
								(unsigned short)(new_opacity*this_blue);
						}
					}
					else
					{	this_pixel[0] += (unsigned short)
							(old_transparency*new_opacity*this_red/255);
						if (gray_flag)
							this_pixel[1] = this_pixel[2] = this_pixel[0];
						else
						{	this_pixel[1]+= (unsigned short)
								(old_transparency*new_opacity*this_green/255);
							this_pixel[2] += (unsigned short)
								(old_transparency*new_opacity*this_blue/255);
						}
					}
					pixel_likelihood = (unsigned char)(likelihood*new_opacity);
					if (pixel_likelihood > *likelihood_pixel)
					{	*likelihood_pixel = pixel_likelihood;
						*z_pixel = this_z;
					}
				}
			}
		}
	}
}


/*****************************************************************************
 * FUNCTION: pct_quick_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    projection using one pixel for each voxel.  (If the scale is too large,
 *    there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variable slice_buffer may be changed.
 * ENTRY CONDITIONS: The static variables ambient_light,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/25/94 check_event called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 5/17/94 cleaned up pointer arithmetic by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
							pct_paint_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
						next_ptr_ptr--;
						this_row_x -= row_x_factor;
						this_row_z -= row_z_factor;
						this_row_y -= row_y_factor;
					}
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_paint_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	pct_paint_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_paint_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: pct_paint_one_voxel
 * DESCRIPTION: Projects one voxel of a SHELL0 of class PERCENT to one pixel
 *    of an object image buffer.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables one_minus_opacity_times,
 *    red_times_square_of, green_times_square_of, blue_times_square_of,
 *    sstrnth_table, sstrnth_ambient_table, gray_flag, surf_pct_table,
 *    surf_opac_table must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/2/92 for more than one byte i-pixels by Dewey Odhner
 *    Modified: 6/19/93 to use sstrnth_table, sstrnth_ambient_table
 *       by Dewey Odhner
 *    Modified: 9/1/93 to use ambient components by Dewey Odhner
 *    Modified: 4/5/94 for surface factors in table by Dewey Odhner
 *    Modified: 4/26/96 surf_pct_table, surf_opac_table used by Dewey Odhner
 *    Modified: 5/1/96 surfaces colored by material by Dewey Odhner
 *    Modified: 6/10/96 back material contribution corrected by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::pct_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024],
	unsigned short *this_ptr, Object_image *object_image)
{
	unsigned char *opacity_ptr, *likelihood_pixel, old_transparency;
	unsigned icoord,front_materl=0,back_materl, this_voxel, pb, pf, likelihood;
	float this_angle_shade, this_red, this_green=0,
		this_blue=0, opacity, transparency, as, ass;
	int this_x, this_y, this_z, *z_pixel, this_column;
	Pixel_unit *this_pixel;

	this_voxel = this_ptr[1];
	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	opacity_ptr = object_image->opacity_buffer[this_y]+this_x;
	if (*opacity_ptr<MAX_OPACITY)
	{	this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
		this_angle_shade = (float)angle_shade[this_voxel&0x7ff];
		if (this_angle_shade > 0)
		{	back_materl = this_voxel>>12;
			switch (back_materl)
			{	case 0:
				case 1:
					front_materl = 0;
					break;
				case 2:
					front_materl = 1;
					break;
				case 3:
					front_materl = 2;
			}
			pb = this_ptr[2]&255;
			pf = 255-pb;
			this_red = red_times_square_of[front_materl][pf];
			if (!gray_flag)
			{	this_green = green_times_square_of[front_materl][pf];
				this_blue = blue_times_square_of[front_materl][pf];
			}
			likelihood = this_ptr[2]>>8;
			as = materl_opacity[back_materl]*surf_pct_table[pb];
			transparency = one_minus_opacity_times[back_materl][pb]*
				(1-as)*one_minus_opacity_times[front_materl][pf];
			opacity = 1-transparency;
			ass = as*(float)(1/65535.);
			this_red +=
				one_minus_opacity_times[front_materl][pf]*
				(ass*(this_angle_shade*
				 sstrnth_table[back_materl][0][likelihood]+
				 sstrnth_ambient_table[back_materl][0][likelihood])+
				 (1-as)*red_times_square_of[back_materl][pb]);
			if (!gray_flag)
			{	this_green +=
				  one_minus_opacity_times[front_materl][pf]*
				  (ass*(this_angle_shade*
				   sstrnth_table[back_materl][1][likelihood]+
				   sstrnth_ambient_table[back_materl][1][likelihood])+
					(1-as)*green_times_square_of[back_materl][pb]);
				this_blue +=
				   one_minus_opacity_times[front_materl][pf]*
				   (ass*(this_angle_shade*
				    sstrnth_table[back_materl][2][likelihood]+
				    sstrnth_ambient_table[back_materl][2][likelihood])+
					(1-as)*blue_times_square_of[back_materl][pb]);
			}
			z_pixel = object_image->z_buffer[this_y]+this_x;
			likelihood_pixel = object_image->likelihood_buffer[this_y]+this_x;
			this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x*3;
			old_transparency = 255-*opacity_ptr;
			if (*opacity_ptr == 0)
			{	this_pixel[0] = (unsigned short)this_red;
				if (gray_flag)
					this_pixel[1] = this_pixel[2] = this_pixel[0];
				else
				{	this_pixel[1] = (unsigned short)this_green;
					this_pixel[2] = (unsigned short)this_blue;
				}
			}
			else
			{	this_pixel[0] +=
					(unsigned short)(old_transparency*this_red/255);
				if (gray_flag)
					this_pixel[1] = this_pixel[2] = this_pixel[0];
				else
				{	this_pixel[1] +=
						(unsigned short)(old_transparency*this_green/255);
					this_pixel[2] +=
						(unsigned short)(old_transparency*this_blue/255);
				}
			}
			*opacity_ptr += 255-(int)(255-old_transparency*opacity);
			if (likelihood > *likelihood_pixel)
			{	*likelihood_pixel = likelihood;
				*z_pixel = this_z;
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: pct_mip_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    maximum intensity projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variables one_minus_opacity_times, red_times_square_of,
 *    green_times_square_of, blue_times_square_of, sstrnth_table,
 *    sstrnth_ambient_table, gray_flag, slice_buffer may be changed.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables ambient_light, fade_flag, surf_red_factr,
 *    surf_green_factr, surf_blue_factr, materl_opacity, materl_red,
 *    materl_green, materl_blue, surf_strenth must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/5/94 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_mip_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	if (fade_flag || need_patch(projection_matrix))
		return (pct_mip_patch_project(object_data, object_image,
			projection_matrix, projection_offset,
			check_event));
	else
		return (pct_mip_quick_project(object_data, object_image,
			projection_matrix, projection_offset,
			check_event));
}


/*****************************************************************************
 * FUNCTION: pct_mip_patch_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    maximum intensity projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variable slice_buffer may be changed.
 * ENTRY CONDITIONS: The static variables ambient_light, fade_flag,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/5/94 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_mip_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
		&left_margin, &right_margin, projection_matrix, 1.);
	if (error_code)
		return (error_code);
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	ncode = this_ptr[0]>>10
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[2]>>8
 *	op = this_ptr[2]&255
 */

						{	pct_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr = slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				Get_this_slice
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: pct_mip_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of a SHELL0 of class PERCENT to an
 *    image buffer using a patch of pixels, maximum intensity projection.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, gray_flag must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 7/5/94 by Dewey Odhner
 *    Modified: 7/7/94 voxel color assignment corrected by Dewey Odhner
 *    Modified: 4/9/97 to store red MI z-value by Dewey Odhner
 *    Modified: 7/9/97 to store initial MI z-value by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::pct_mip_patch_one_voxel(int this_row_z, int this_row_x,
	int this_row_y, int column_x_table[1024], int column_y_table[1024],
	int column_z_table[1024], int itop_margin,  int ibottom_margin,
	int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image)
{
	unsigned char *likelihood_pixel;
	unsigned front_materl=0, back_materl, this_voxel, pb, pf, likelihood;
	float this_red=0, this_green=0, this_blue=0;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,color_computed;
	Pixel_unit *this_pixel, *line_start, *line_end;

	this_voxel = this_ptr[1];
	likelihood = this_ptr[2]>>8;
	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord -= this_x*0x10000+0x8000;
	left_margin_fpart = ileft_margin+icoord;
	left_margin_ipart = left_margin_fpart/0x10000;
	left_margin_fpart -= left_margin_ipart*0x10000;
	right_margin_fpart = iright_margin-icoord;
	right_margin_ipart = right_margin_fpart/0x10000;
	right_margin_fpart -= right_margin_ipart*0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	icoord -= this_y*0x10000+0x8000;
	top_margin_fpart = itop_margin+icoord;
	top_margin_ipart = top_margin_fpart/0x10000;
	top_margin_fpart -= top_margin_ipart*0x10000;
	bottom_margin_fpart = ibottom_margin-icoord;
	bottom_margin_ipart = bottom_margin_fpart/0x10000;
	bottom_margin_fpart -= bottom_margin_ipart*0x10000;
	patch_line = top_margin_ipart;
	patch_top = this_y+patch->top+patch_line;
	patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
	color_computed = FALSE;
	for (this_line=patch_top; this_line<patch_bottom;
			this_line++, patch_line++)
	{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
		left_end = this_x+patch_column;
		right_end =
			this_x+patch->lines[patch_line].right-right_margin_ipart;
		line_start= (Pixel_unit*)object_image->image[this_line]+left_end*3;
		z_pixel = object_image->z_buffer[this_line]+left_end;
		likelihood_pixel =
			object_image->likelihood_buffer[this_line]+left_end;
		line_end = (Pixel_unit*)object_image->image[this_line]+right_end*3;
		for (this_pixel=line_start; this_pixel<line_end;
				this_pixel+=3, z_pixel++, likelihood_pixel++, patch_column++)
		{	unsigned char pixel_likelihood;
			float new_opacity;

			if (patch->lines[patch_line-top_margin_ipart].left<=patch_column &&
					patch->lines[patch_line-top_margin_ipart].right>
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].left<=
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].right>
						patch_column)
			{	if (!color_computed)
				{	back_materl = this_voxel>>12;
					switch (back_materl)
					{	case 0:
						case 1:
							front_materl = 0;
							break;
						case 2:
							front_materl = 1;
							break;
						case 3:
							front_materl = 2;
					}
					pb = this_ptr[2]&255;
					pf = 255-pb;
					this_red = (float)((pf*materl_opacity[front_materl]*
						materl_red[front_materl]+
						pb*materl_opacity[back_materl]*
						materl_red[back_materl])*(1./255));
					if (!gray_flag)
					{	this_green = (float)((pf*materl_opacity[front_materl]*
							materl_green[front_materl]+
							pb*materl_opacity[back_materl]*
							materl_green[back_materl])*(1./255));
						this_blue = (float)((pf*materl_opacity[front_materl]*
							materl_blue[front_materl]+
							pb*materl_opacity[back_materl]*
							materl_blue[back_materl])*(1./255));
					}
					this_z = (this_row_z+column_z_table[this_column])/
						Z_SUBLEVELS;
					color_computed = TRUE;
				}
				if (fade_flag && (patch_line==top_margin_ipart ||
						patch->lines[patch_line-top_margin_ipart-1]. 
							left>patch_column ||
						patch->lines[patch_line-top_margin_ipart-1].
							right<=patch_column))
					new_opacity =
						(float)(0x10000-top_margin_fpart)/0x10000;
				else
					new_opacity = 1;
				if (fade_flag)
				{	if (patch_line==patch->bottom-patch->top-
							bottom_margin_ipart-1 ||
							patch->lines[patch_line+bottom_margin_ipart+1].
								left>patch_column ||
							patch->lines[patch_line+bottom_margin_ipart+1].
								right<=patch_column)
						new_opacity *=
							(float)(0x10000-bottom_margin_fpart)/0x10000;
					if (this_pixel == line_start)
						new_opacity *=
							(float)(0x10000-left_margin_fpart)/0x10000;
					if (this_pixel == line_end-3)
						new_opacity *=
							(float)(0x10000-right_margin_fpart)/0x10000;
				}
				pixel_likelihood = (unsigned char)(likelihood*new_opacity);
				if (this_pixel[0] == V_OBJECT_IMAGE_BACKGROUND)
				{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
					if (gray_flag)
						this_pixel[1] = this_pixel[2] = this_pixel[0];
					else
					{	this_pixel[1] =
							(unsigned short)(new_opacity*this_green);
						this_pixel[2] =
							(unsigned short)(new_opacity*this_blue);
					}
					*likelihood_pixel = pixel_likelihood;
					*z_pixel = this_z;
				}
				else
				{	if ((unsigned short)(new_opacity*this_red) > this_pixel[0])
					{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
						*likelihood_pixel = pixel_likelihood;
						*z_pixel = this_z;
					}
					if (gray_flag)
						this_pixel[1] = this_pixel[2] = this_pixel[0];
					else
					{	if ((unsigned short)(new_opacity*this_green) >
								this_pixel[1])
							this_pixel[1] =
								(unsigned short)(new_opacity*this_green);
						if ((unsigned short)(new_opacity*this_blue) >
								this_pixel[2])
							this_pixel[2] =
								(unsigned short)(new_opacity*this_blue);
					}
				}
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: pct_mip_quick_project
 * DESCRIPTION: Renders a SHELL0 object of class PERCENT by parallel
 *    maximum intensity projection using one pixel for each voxel.
 *    (If the scale is too large, there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             0 to 5
 *       y1                6 to 15
 *       tt                16 to 20
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                32 to 39
 *       op                not stored
 *       pg                40 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variable slice_buffer may be changed.
 * ENTRY CONDITIONS: The static variables materl_opacity,
 *    materl_red, materl_green, materl_blue, gray_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/5/94 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::pct_mip_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
							pct_mip_paint_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
						next_ptr_ptr--;
						this_row_x -= row_x_factor;
						this_row_z -= row_z_factor;
						this_row_y -= row_y_factor;
					}
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_paint_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr =
							slice_buffer+(next_ptr_ptr[1]-3-this_slice_ptr);
						next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_paint_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
					for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr=slice_buffer+(*next_ptr_ptr-3-this_slice_ptr);
						for (next_ptr=
								slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
								this_ptr>=next_ptr; this_ptr-=3)
						{	pct_mip_paint_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, this_ptr, object_image);
							if (this_ptr == slice_buffer)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				Get_this_slice;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = slice_buffer+(next_ptr_ptr[-1]-this_slice_ptr);
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=slice_buffer+(*next_ptr_ptr-this_slice_ptr);
							this_ptr<next_ptr; this_ptr+=3)
						pct_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: pct_mip_paint_one_voxel
 * DESCRIPTION: Projects one voxel of a SHELL0 of class PERCENT to one pixel
 *    of an object image buffer, maximum intensity projection.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables materl_opacity, materl_red,
 *    materl_green, materl_blue, gray_flag must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 7/5/94 by Dewey Odhner
 *    Modified: 7/7/94 voxel color assignment corrected by Dewey Odhner
 *    Modified: 4/9/97 to store red MI z-value by Dewey Odhner
 *    Modified: 7/9/97 to store initial MI z-value by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::pct_mip_paint_one_voxel(int this_row_z, int this_row_x,
	int this_row_y, int column_x_table[1024], int column_y_table[1024],
	int column_z_table[1024], unsigned short *this_ptr,
	Object_image *object_image)
{
	unsigned char *likelihood_pixel;
	unsigned icoord,front_materl=0,back_materl, this_voxel, pb, pf, likelihood;
	float this_red, this_green=0, this_blue=0;
	int this_x, this_y, this_z, *z_pixel, this_column;
	Pixel_unit *this_pixel;

	this_voxel = this_ptr[1];
	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
	back_materl = this_voxel>>12;
	switch (back_materl)
	{	case 0:
		case 1:
			front_materl = 0;
			break;
		case 2:
			front_materl = 1;
			break;
		case 3:
			front_materl = 2;
	}
	pb = this_ptr[2]&255;
	pf = 255-pb;
	this_red = (float)((pf*materl_opacity[front_materl]*
		materl_red[front_materl]+
		pb*materl_opacity[back_materl]*
		materl_red[back_materl])*(1./255));
	if (!gray_flag)
	{	this_green = (float)((pf*materl_opacity[front_materl]*
			materl_green[front_materl]+
			pb*materl_opacity[back_materl]*
			materl_green[back_materl])*(1./255));
		this_blue = (float)((pf*materl_opacity[front_materl]*
			materl_blue[front_materl]+
			pb*materl_opacity[back_materl]*
			materl_blue[back_materl])*(1./255));
	}
	likelihood = this_ptr[2]>>8;
	z_pixel = object_image->z_buffer[this_y]+this_x;
	likelihood_pixel = object_image->likelihood_buffer[this_y]+this_x;
	this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x*3;
	if (this_pixel[0] == V_OBJECT_IMAGE_BACKGROUND)
	{	this_pixel[0] = (unsigned short)this_red;
		if (gray_flag)
			this_pixel[1] = this_pixel[2] = this_pixel[0];
		else
		{	this_pixel[1] = (unsigned short)this_green;
			this_pixel[2] = (unsigned short)this_blue;
		}
		*likelihood_pixel = likelihood;
		*z_pixel = this_z;
	}
	else
	{	if ((unsigned short)this_red > this_pixel[0])
		{	this_pixel[0] = (unsigned short)this_red;
			*likelihood_pixel = likelihood;
			*z_pixel = this_z;
		}
		if (gray_flag)
			this_pixel[1] = this_pixel[2] = this_pixel[0];
		else
		{	if ((unsigned short)this_green > this_pixel[1])
				this_pixel[1] = (unsigned short)this_green;
			if ((unsigned short)this_blue > this_pixel[2])
				this_pixel[2] = (unsigned short)this_blue;
		}
	}
}


/*****************************************************************************
 * FUNCTION: d_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variables one_minus_opacity_times, red_times_square_of,
 *    green_times_square_of, blue_times_square_of, sstrnth_table,
 *    sstrnth_ambient_table, gray_flag, slice_buffer may be changed.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables ambient_light, fade_flag, surf_red_factr,
 *    surf_green_factr, surf_blue_factr, materl_opacity, materl_red,
 *    materl_green, materl_blue, surf_strenth must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/22/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	initialize_times_tables();
	if (fade_flag || need_patch(projection_matrix))
		return (d_patch_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
	else
		return (d_quick_project(object_data, object_image,
			projection_matrix, projection_offset, angle_shade,
			check_event));
}


/*****************************************************************************
 * FUNCTION: d_patch_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variable slice_buffer may be changed.
 * ENTRY CONDITIONS: The static variables ambient_light, fade_flag,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin, voxel_depth,
		temp_matrix[3][3], temp_vector[3];

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	if (fade_flag == 2)
	{
		error_code = VInvertMatrix(temp_matrix[0], projection_matrix[0], 3);
		if (error_code)
			return (error_code);
		temp_vector[0] = temp_vector[1] = 0;
		temp_vector[2] = 1;
		matrix_vector_multiply(temp_vector, temp_matrix, temp_vector);
		voxel_depth = sqrt((object_data->file->file_header.scn.xypixsz[0]*
			object_data->file->file_header.scn.xypixsz[0]+
			object_data->file->file_header.scn.xypixsz[1]*
			object_data->file->file_header.scn.xypixsz[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data))/
			(object_data->file->file_header.scn.xypixsz[0]*
			object_data->file->file_header.scn.xypixsz[0]*
			temp_vector[0]*temp_vector[0]+
			object_data->file->file_header.scn.xypixsz[1]*
			object_data->file->file_header.scn.xypixsz[1]*
			temp_vector[1]*temp_vector[1]+
			Slice_spacing(object_data)*Slice_spacing(object_data)*
			temp_vector[2]*temp_vector[2]));
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, voxel_depth);
		if (error_code)
			return (error_code);
		trim_patch(patch);
	}
	else
	{
		error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
			&left_margin, &right_margin, projection_matrix, 1.);
		if (error_code)
			return (error_code);
	}
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8 & 0x7
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[0]>>8&0xf8 | this_ptr[1]>>13
 *	intensity = this_ptr[2]
 */

						{	d_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	d_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_patch_one_voxel(this_row_z, angle_shade,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_patch_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: d_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of an object of class DIRECT to an
 *    image buffer using a patch of pixels.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *    threshold: Intensity levels defining the different materials.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, one_minus_opacity_times,
 *    red_times_square_of, green_times_square_of, blue_times_square_of,
 *    sstrnth_table, sstrnth_ambient_table, gray_flag, surf_pct_table,
 *    surf_opac_table must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::d_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024], int itop_margin,
	int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image, int threshold[6])
{
	unsigned char *opacity_ptr, *likelihood_pixel;
	unsigned front_materl, back_materl, pb, pf, likelihood;
	float this_angle_shade, this_red=0, this_green=0,
		this_blue=0, opacity=0, transparency, as, ass;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,color_computed;
	Pixel_unit *this_pixel, *line_start, *line_end;

	this_angle_shade = (float)(angle_shade[this_ptr[1]&0x7ff]);
	if (this_angle_shade > 0)
	{
		likelihood = (this_ptr[0]>>8&0xf8) | (this_ptr[1]>>13);
		this_column = (int)(*this_ptr&0x3ff);
		icoord = this_row_x+column_x_table[this_column];
		this_x = icoord/0x10000;
		icoord -= this_x*0x10000+0x8000;
		left_margin_fpart = ileft_margin+icoord;
		left_margin_ipart = left_margin_fpart/0x10000;
		left_margin_fpart -= left_margin_ipart*0x10000;
		right_margin_fpart = iright_margin-icoord;
		right_margin_ipart = right_margin_fpart/0x10000;
		right_margin_fpart -= right_margin_ipart*0x10000;
		icoord = this_row_y+column_y_table[this_column];
		this_y = icoord/0x10000;
		icoord -= this_y*0x10000+0x8000;
		top_margin_fpart = itop_margin+icoord;
		top_margin_ipart = top_margin_fpart/0x10000;
		top_margin_fpart -= top_margin_ipart*0x10000;
		bottom_margin_fpart = ibottom_margin-icoord;
		bottom_margin_ipart = bottom_margin_fpart/0x10000;
		bottom_margin_fpart -= bottom_margin_ipart*0x10000;
		if (fade_flag == 2)
			top_margin_ipart = bottom_margin_ipart =
				left_margin_ipart = right_margin_ipart = 0;
		patch_line = top_margin_ipart;
		patch_top = this_y+patch->top+patch_line;
		patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
		color_computed = FALSE;
		for (this_line=patch_top; this_line<patch_bottom;
				this_line++, patch_line++)
		{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
			left_end = this_x+patch_column;
			right_end =
				this_x+patch->lines[patch_line].right-right_margin_ipart;
			line_start= (Pixel_unit*)object_image->image[this_line]+left_end*3;
			opacity_ptr = object_image->opacity_buffer[this_line]+left_end;
			z_pixel = object_image->z_buffer[this_line]+left_end;
			likelihood_pixel =
				object_image->likelihood_buffer[this_line]+left_end;
			line_end = (Pixel_unit*)object_image->image[this_line]+right_end*3;
			for (this_pixel=line_start; this_pixel<line_end;
					this_pixel+=3, opacity_ptr++, z_pixel++,
						likelihood_pixel++, patch_column++)
			{	unsigned char old_opacity, old_transparency, pixel_likelihood;
				float new_opacity;

				old_opacity = *opacity_ptr;
				if (old_opacity<MAX_OPACITY && 
						patch->lines[patch_line-top_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line-top_margin_ipart].right>
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].left<=
							patch_column &&
						patch->lines[patch_line+bottom_margin_ipart].right>
							patch_column)
				{	if (!color_computed)
					{
						for(back_materl=3; back_materl; back_materl--)
							if (this_ptr[2] >= threshold[(back_materl-1)*2])
								break;
						switch (back_materl)
						{	case 0:
							case 1:
								front_materl = 0;
								break;
							case 2:
								front_materl = 1;
								break;
							case 3:
								front_materl = 2;
						}
						pb = back_materl==0? 0:
							this_ptr[2]>=threshold[back_materl*2-1]? 255:
							(int)(255.*(this_ptr[2]-threshold[back_materl*2-2])
							/(threshold[back_materl*2-1]-
							threshold[back_materl*2-2]));
						pf = 255-pb;
						this_red= red_times_square_of[front_materl][pf];
						if (!gray_flag)
						{	this_green=green_times_square_of[front_materl][pf];
							this_blue = blue_times_square_of[front_materl][pf];
						}
						as = materl_opacity[back_materl]*surf_pct_table[pb];
						transparency= one_minus_opacity_times[back_materl][pb]*
							(1-as)*one_minus_opacity_times[front_materl][pf];
						opacity = 1-transparency;
						ass = as*(float)(1/65535.);
						this_red +=
							one_minus_opacity_times[front_materl][pf]*
							(ass*(this_angle_shade*
							 sstrnth_table[back_materl][0][likelihood]+
							 sstrnth_ambient_table[back_materl][0][likelihood])
							 +(1-as)*red_times_square_of[back_materl][pb]);
						if (!gray_flag)
						{	this_green +=
							  one_minus_opacity_times[front_materl][pf]*
							  (ass*(this_angle_shade*
							   sstrnth_table[back_materl][1][likelihood]+
							   sstrnth_ambient_table[back_materl][1][likelihood])+
								(1-as)*green_times_square_of[back_materl][pb]);
							this_blue +=
							   one_minus_opacity_times[front_materl][pf]*
							   (ass*(this_angle_shade*
							    sstrnth_table[back_materl][2][likelihood]+
							    sstrnth_ambient_table[back_materl][2][likelihood])+
								(1-as)*blue_times_square_of[back_materl][pb]);
						}
						this_z = (this_row_z+column_z_table[this_column])/
							Z_SUBLEVELS;
						color_computed = TRUE;
					}
					if (fade_flag==1 && (patch_line==top_margin_ipart ||
							patch->lines[patch_line-top_margin_ipart-1]. 
								left>patch_column ||
							patch->lines[patch_line-top_margin_ipart-1].
								right<=patch_column))
						new_opacity =
							(float)(0x10000-top_margin_fpart)/0x10000;
					else if (fade_flag == 2)
						new_opacity = (float)1/255*
							patch->lines[patch_line].weight[
							patch_column-patch->lines[patch_line].left];
					else
						new_opacity = 1;
					if (fade_flag == 1)
					{	if (patch_line==patch->bottom-patch->top-
								bottom_margin_ipart-1 ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									left>patch_column ||
								patch->lines[patch_line+bottom_margin_ipart+1].
									right<=patch_column)
							new_opacity *=
								(float)(0x10000-bottom_margin_fpart)/0x10000;
						if (this_pixel == line_start)
							new_opacity *=
								(float)(0x10000-left_margin_fpart)/0x10000;
						if (this_pixel == line_end-3)
							new_opacity *=
								(float)(0x10000-right_margin_fpart)/0x10000;
					}
					old_transparency = 255-old_opacity;
					*opacity_ptr += 255-
						(int)(255-old_transparency*new_opacity*opacity);
					if (old_opacity == 0)
					{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
						if (gray_flag)
							this_pixel[1] = this_pixel[2] = this_pixel[0];
						else
						{	this_pixel[1] = (unsigned short)(new_opacity*this_green);
							this_pixel[2] = (unsigned short)(new_opacity*this_blue);
						}
					}
					else
					{	this_pixel[0] += (unsigned short)
							(old_transparency*new_opacity*this_red/255);
						if (gray_flag)
							this_pixel[1] = this_pixel[2] = this_pixel[0];
						else
						{	this_pixel[1]+=
								(unsigned short)(old_transparency*new_opacity*this_green/255);
							this_pixel[2] +=
								(unsigned short)(old_transparency*new_opacity*this_blue/255);
						}
					}
					pixel_likelihood = (unsigned char)(likelihood*new_opacity);
					if (pixel_likelihood > *likelihood_pixel)
					{	*likelihood_pixel = pixel_likelihood;
						*z_pixel = this_z;
					}
				}
			}
		}
	}
}


/*****************************************************************************
 * FUNCTION: d_quick_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    projection using one pixel for each voxel.  (If the scale is too large,
 *    there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: The static variables ambient_light,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], int angle_shade[G_CODES],
	Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
							d_paint_one_voxel(this_row_z, angle_shade,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
						next_ptr_ptr--;
						this_row_x -= row_x_factor;
						this_row_z -= row_z_factor;
						this_row_y -= row_y_factor;
					}
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_paint_one_voxel(this_row_z, angle_shade,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	d_paint_one_voxel(this_row_z, angle_shade,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_paint_one_voxel(this_row_z, angle_shade,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_paint_one_voxel(this_row_z, angle_shade,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: d_paint_one_voxel
 * DESCRIPTION: Projects one voxel of an object of class DIRECT to one pixel
 *    of an object image buffer.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *    threshold: Intensity levels defining the different materials.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables one_minus_opacity_times,
 *    red_times_square_of, green_times_square_of, blue_times_square_of,
 *    sstrnth_table, sstrnth_ambient_table, gray_flag, surf_pct_table,
 *    surf_opac_table must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::d_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
	int this_row_x, int this_row_y, int column_x_table[1024],
	int column_y_table[1024], int column_z_table[1024],
	unsigned short *this_ptr, Object_image *object_image, int threshold[6])
{
	unsigned char *opacity_ptr, *likelihood_pixel, old_transparency;
	unsigned icoord,front_materl, back_materl, pb, pf, likelihood;
	float this_angle_shade, this_red, this_green=0,
		this_blue=0, opacity, transparency, as, ass;
	int this_x, this_y, this_z, *z_pixel, this_column;
	Pixel_unit *this_pixel;

	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	opacity_ptr = object_image->opacity_buffer[this_y]+this_x;
	if (*opacity_ptr<MAX_OPACITY)
	{	this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
		this_angle_shade = (float)(angle_shade[this_ptr[1]&0x7ff]);
		if (this_angle_shade > 0)
		{
			for(back_materl=3; back_materl; back_materl--)
				if (this_ptr[2] >= threshold[(back_materl-1)*2])
					break;
			switch (back_materl)
			{	case 0:
				case 1:
					front_materl = 0;
					break;
				case 2:
					front_materl = 1;
					break;
				case 3:
					front_materl = 2;
			}
			pb = back_materl==0? 0: this_ptr[2]>=threshold[back_materl*2-1]?
				255: (int)(255.*(this_ptr[2]-threshold[back_materl*2-2])/
				(threshold[back_materl*2-1]-threshold[back_materl*2-2]));
			pf = 255-pb;
			this_red = red_times_square_of[front_materl][pf];
			if (!gray_flag)
			{	this_green = green_times_square_of[front_materl][pf];
				this_blue = blue_times_square_of[front_materl][pf];
			}
			likelihood = (this_ptr[0]>>8&0xf8) | (this_ptr[1]>>13);
			as = materl_opacity[back_materl]*surf_pct_table[pb];
			transparency = one_minus_opacity_times[back_materl][pb]*
				(1-as)*one_minus_opacity_times[front_materl][pf];
			opacity = 1-transparency;
			ass = as*(float)(1/65535.);
			this_red +=
				one_minus_opacity_times[front_materl][pf]*
				(ass*(this_angle_shade*
				 sstrnth_table[back_materl][0][likelihood]+
				 sstrnth_ambient_table[back_materl][0][likelihood])+
				 (1-as)*red_times_square_of[back_materl][pb]);
			if (!gray_flag)
			{	this_green +=
				  one_minus_opacity_times[front_materl][pf]*
				  (ass*(this_angle_shade*
				   sstrnth_table[back_materl][1][likelihood]+
				   sstrnth_ambient_table[back_materl][1][likelihood])+
					(1-as)*green_times_square_of[back_materl][pb]);
				this_blue +=
				   one_minus_opacity_times[front_materl][pf]*
				   (ass*(this_angle_shade*
				    sstrnth_table[back_materl][2][likelihood]+
				    sstrnth_ambient_table[back_materl][2][likelihood])+
					(1-as)*blue_times_square_of[back_materl][pb]);
			}
			z_pixel = object_image->z_buffer[this_y]+this_x;
			likelihood_pixel = object_image->likelihood_buffer[this_y]+this_x;
			this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x*3;
			old_transparency = 255-*opacity_ptr;
			if (*opacity_ptr == 0)
			{	this_pixel[0] = (unsigned short)this_red;
				if (gray_flag)
					this_pixel[1] = this_pixel[2] = this_pixel[0];
				else
				{	this_pixel[1] = (unsigned short)this_green;
					this_pixel[2] = (unsigned short)this_blue;
				}
			}
			else
			{	this_pixel[0]+=(unsigned short)(old_transparency*this_red/255);
				if (gray_flag)
					this_pixel[1] = this_pixel[2] = this_pixel[0];
				else
				{	this_pixel[1] +=
						(unsigned short)(old_transparency*this_green/255);
					this_pixel[2] +=
						(unsigned short)(old_transparency*this_blue/255);
				}
			}
			*opacity_ptr += 255-(int)(255-old_transparency*opacity);
			if (likelihood > *likelihood_pixel)
			{	*likelihood_pixel = likelihood;
				*z_pixel = this_z;
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: d_mip_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    maximum intensity projection.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 *    The static variables one_minus_opacity_times, red_times_square_of,
 *    green_times_square_of, blue_times_square_of, sstrnth_table,
 *    sstrnth_ambient_table, gray_flag may be changed.
 * ENTRY CONDITIONS: Any entry conditions of check_event must be met.
 *    The static variables ambient_light, fade_flag, surf_red_factr,
 *    surf_green_factr, surf_blue_factr, materl_opacity, materl_red,
 *    materl_green, materl_blue, surf_strenth must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_mip_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	if (fade_flag || need_patch(projection_matrix))
		return (d_mip_patch_project(object_data, object_image,
			projection_matrix, projection_offset,
			check_event));
	else
		return (d_mip_quick_project(object_data, object_image,
			projection_matrix, projection_offset,
			check_event));
}


/*****************************************************************************
 * FUNCTION: d_mip_patch_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    maximum intensity projection using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: The static variables ambient_light, fade_flag,
 *    surf_red_factr, surf_green_factr, surf_blue_factr, materl_opacity,
 *    materl_red, materl_green, materl_blue, surf_strenth,
 *    one_minus_opacity_times, red_times_square_of, green_times_square_of,
 *    blue_times_square_of, sstrnth_table, sstrnth_ambient_table, gray_flag
 *    must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_mip_patch_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	Patch *patch=NULL;
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z, order,
		itop_margin, ibottom_margin, ileft_margin, iright_margin, error_code;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;
	double top_margin, bottom_margin, left_margin, right_margin;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left pixel of the object's image buffer. */
	/* Remember, the patch represents a voxel centered .25 pixel from the
		corner of pixel 0. */
	jx = jy = 0;
	jz = (int)(Z_SUBLEVELS*MIDDLE_DEPTH);
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = jx;
		column_y_table[coln] = jy;
		column_z_table[coln] = jz;
		jx += column_x_factor;
		jy += column_y_factor;
		jz += column_z_factor;
	}
	error_code = get_margin_patch(&patch, &top_margin, &bottom_margin,
		&left_margin, &right_margin, projection_matrix, 1.);
	if (error_code)
		return (error_code);
	itop_margin = (int)rint(0x10000*top_margin);
	ibottom_margin = (int)rint(0x10000*bottom_margin);
	ileft_margin = (int)rint(0x10000*left_margin);
	iright_margin = (int)rint(0x10000*right_margin);
	switch (order)
	{
		case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
/* this_ptr points to the voxel information that will be in the TSE of
 * the SHELL structure.
 *	y1 = this_ptr[0]&0x3ff
 *	n1 = this_ptr[1]>>8 & 0x7
 *	n2 = this_ptr[1]>>4 & 0xf
 *	n3 = this_ptr[1]&0xf
 *	gm = this_ptr[0]>>8&0xf8 | this_ptr[1]>>13
 *	intensity = this_ptr[2]
 */

						{	d_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_patch_one_voxel(this_row_z,
								this_row_x,
								this_row_y, column_x_table, column_y_table,
								column_z_table, itop_margin,ibottom_margin,
								ileft_margin, iright_margin, patch, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
				{
					free(patch->lines[0].weight);
					free(patch);
					return (401);
				}
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_patch_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, itop_margin,ibottom_margin,
							ileft_margin, iright_margin, patch, this_ptr,
							object_image, object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	free(patch->lines[0].weight);
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: d_mip_patch_one_voxel
 * DESCRIPTION: Scan-converts one voxel of an object of class DIRECT to an
 *    image buffer using a patch of pixels, maximum intensity projection.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    itop_margin, ibottom_margin, ileft_margin, iright_margin: Estimates of
 *       the average overlap or margin that the patch extends beyond the
 *       projection of the voxel in 0x10000 units per pixel.  These are used
 *       to fade the edges when the fade option is on.
 *    patch: A patch describing the projection of a voxel, from the function
 *       get_margin_patch.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *    threshold: Intensity levels defining the different materials.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables fade_flag, gray_flag must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::d_mip_patch_one_voxel(int this_row_z, int this_row_x,
	int this_row_y, int column_x_table[1024], int column_y_table[1024],
	int column_z_table[1024], int itop_margin, int ibottom_margin,
	int ileft_margin, int iright_margin, Patch *patch,
	unsigned short *this_ptr, Object_image *object_image, int threshold[6])
{
	unsigned char *likelihood_pixel;
	unsigned front_materl, back_materl, pb, pf, likelihood;
	float this_red=0, this_green=0, this_blue=0;
	int top_margin_fpart, bottom_margin_fpart, left_margin_fpart,
		right_margin_fpart, top_margin_ipart, bottom_margin_ipart,
		left_margin_ipart, right_margin_ipart, icoord, patch_column,
		this_x, this_y, this_z=0, patch_line, *z_pixel, this_column,
		this_line, patch_top, patch_bottom, left_end, right_end,color_computed;
	Pixel_unit *this_pixel, *line_start, *line_end;

	likelihood = (this_ptr[0]>>8&0xf8) | (this_ptr[1]>>13);
	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord -= this_x*0x10000+0x8000;
	left_margin_fpart = ileft_margin+icoord;
	left_margin_ipart = left_margin_fpart/0x10000;
	left_margin_fpart -= left_margin_ipart*0x10000;
	right_margin_fpart = iright_margin-icoord;
	right_margin_ipart = right_margin_fpart/0x10000;
	right_margin_fpart -= right_margin_ipart*0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	icoord -= this_y*0x10000+0x8000;
	top_margin_fpart = itop_margin+icoord;
	top_margin_ipart = top_margin_fpart/0x10000;
	top_margin_fpart -= top_margin_ipart*0x10000;
	bottom_margin_fpart = ibottom_margin-icoord;
	bottom_margin_ipart = bottom_margin_fpart/0x10000;
	bottom_margin_fpart -= bottom_margin_ipart*0x10000;
	patch_line = top_margin_ipart;
	patch_top = this_y+patch->top+patch_line;
	patch_bottom = this_y+patch->bottom-bottom_margin_ipart;
	color_computed = FALSE;
	for (this_line=patch_top; this_line<patch_bottom;
			this_line++, patch_line++)
	{	patch_column = patch->lines[patch_line].left+left_margin_ipart;
		left_end = this_x+patch_column;
		right_end =
			this_x+patch->lines[patch_line].right-right_margin_ipart;
		line_start= (Pixel_unit*)object_image->image[this_line]+left_end*3;
		z_pixel = object_image->z_buffer[this_line]+left_end;
		likelihood_pixel =
			object_image->likelihood_buffer[this_line]+left_end;
		line_end = (Pixel_unit*)object_image->image[this_line]+right_end*3;
		for (this_pixel=line_start; this_pixel<line_end;
				this_pixel+=3, z_pixel++, likelihood_pixel++, patch_column++)
		{	unsigned char pixel_likelihood;
			float new_opacity;

			if (patch->lines[patch_line-top_margin_ipart].left<=patch_column &&
					patch->lines[patch_line-top_margin_ipart].right>
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].left<=
						patch_column &&
					patch->lines[patch_line+bottom_margin_ipart].right>
						patch_column)
			{	if (!color_computed)
				{
					for(back_materl=3; back_materl; back_materl--)
						if (this_ptr[2] >= threshold[(back_materl-1)*2])
							break;
					switch (back_materl)
					{	case 0:
						case 1:
							front_materl = 0;
							break;
						case 2:
							front_materl = 1;
							break;
						case 3:
							front_materl = 2;
					}
					pb = back_materl==0? 0:
						this_ptr[2]>=threshold[back_materl*2-1]? 255:
						(int)(255.*(this_ptr[2]-threshold[back_materl*2-2])/
						(threshold[back_materl*2-1]-
						threshold[back_materl*2-2]));
					pf = 255-pb;
					this_red = (float)((pf*materl_opacity[front_materl]*
						materl_red[front_materl]+
						pb*materl_opacity[back_materl]*
						materl_red[back_materl])*(1./255));
					if (!gray_flag)
					{	this_green = (float)((pf*materl_opacity[front_materl]*
							materl_green[front_materl]+
							pb*materl_opacity[back_materl]*
							materl_green[back_materl])*(1./255));
						this_blue = (float)((pf*materl_opacity[front_materl]*
							materl_blue[front_materl]+
							pb*materl_opacity[back_materl]*
							materl_blue[back_materl])*(1./255));
					}
					this_z = (this_row_z+column_z_table[this_column])/
						Z_SUBLEVELS;
					color_computed = TRUE;
				}
				if (fade_flag && (patch_line==top_margin_ipart ||
						patch->lines[patch_line-top_margin_ipart-1]. 
							left>patch_column ||
						patch->lines[patch_line-top_margin_ipart-1].
							right<=patch_column))
					new_opacity =
						(float)(0x10000-top_margin_fpart)/0x10000;
				else
					new_opacity = 1;
				if (fade_flag)
				{	if (patch_line==patch->bottom-patch->top-
							bottom_margin_ipart-1 ||
							patch->lines[patch_line+bottom_margin_ipart+1].
								left>patch_column ||
							patch->lines[patch_line+bottom_margin_ipart+1].
								right<=patch_column)
						new_opacity *=
							(float)(0x10000-bottom_margin_fpart)/0x10000;
					if (this_pixel == line_start)
						new_opacity *=
							(float)(0x10000-left_margin_fpart)/0x10000;
					if (this_pixel == line_end-3)
						new_opacity *=
							(float)(0x10000-right_margin_fpart)/0x10000;
				}
				pixel_likelihood = (unsigned char)(likelihood*new_opacity);
				if (this_pixel[0] == V_OBJECT_IMAGE_BACKGROUND)
				{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
					if (gray_flag)
						this_pixel[1] = this_pixel[2] = this_pixel[0];
					else
					{	this_pixel[1] = (unsigned short)(new_opacity*this_green);
						this_pixel[2] = (unsigned short)(new_opacity*this_blue);
					}
					*likelihood_pixel = pixel_likelihood;
					*z_pixel = this_z;
				}
				else
				{	if ((unsigned short)(new_opacity*this_red) > this_pixel[0])
					{	this_pixel[0] = (unsigned short)(new_opacity*this_red);
						*likelihood_pixel = pixel_likelihood;
						*z_pixel = this_z;
					}
					if (gray_flag)
						this_pixel[1] = this_pixel[2] = this_pixel[0];
					else
					{	if ((unsigned short)(new_opacity*this_green) > this_pixel[1])
							this_pixel[1] = (unsigned short)(new_opacity*this_green);
						if ((unsigned short)(new_opacity*this_blue) > this_pixel[2])
							this_pixel[2] = (unsigned short)(new_opacity*this_blue);
					}
				}
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: d_mip_quick_project
 * DESCRIPTION: Renders an object of class DIRECT by parallel
 *    maximum intensity projection using one pixel for each voxel.
 *    (If the scale is too large, there will be holes.)
 * PARAMETERS:
 *    object_data: The shell to be rendered.
 *       The bit-fields in the TSE's must be as follows:
 *       total # of bits   48
 *       ncode             not stored
 *       y1                6 to 15
 *       tt                not stored
 *       n1                21 to 23
 *       n2                24 to 27
 *       n3                28 to 31
 *       gm                0 to 5 and 16 to 18
 *       density           32 to 47
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.  The image is 6 bytes
 *       per pixel with the first unsigned short being the red component,
 *       next green, then blue.  The image values are 0 (black) to
 *       V_OBJECT_IMAGE_BACKGROUND-1 (bright), V_OBJECT_IMAGE_BACKGROUND for
 *       background pixels.  The z-buffer values are from back to front,
 *       i.e. in a left-handed coordinate system.
 *       projected, pixel_units, and image_location are unused.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *       This would have a negative determinant, since it is going from a
 *       right-handed to a left-handed coordinate system.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer and from MIDDLE_DEPTH in the z-direction.
 *       This MUST put the object entirely within the image buffer space.
 *    check_event: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to check_event.
 * SIDE EFFECTS: Any effects of check_event will occur.
 * ENTRY CONDITIONS: The static variables materl_opacity,
 *    materl_red, materl_green, materl_blue, gray_flag must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: check_event returns FIRST
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::d_mip_quick_project(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], Priority (*check_event)(cvRenderer *))
{
	int this_row, this_slice, this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, coln, order,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_slice_x, this_slice_y, this_slice_z;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr, *this_slice_ptr;

	order =	(projection_matrix[2][0]>=0? CO_BW: CO_FW) +
			(projection_matrix[2][1]>=0? RO_BW: RO_FW) +
			(projection_matrix[2][2]>=0? SL_BW: SL_FW);
	column_x_factor = (int)rint(0x10000*projection_matrix[0][0]);
	row_x_factor = (int)rint(0x10000*projection_matrix[0][1]);
	slice_x_factor = (int)rint(0x10000*projection_matrix[0][2]);
	column_y_factor = (int)rint(0x10000*projection_matrix[1][0]);
	row_y_factor = (int)rint(0x10000*projection_matrix[1][1]);
	slice_y_factor = (int)rint(0x10000*projection_matrix[1][2]);
	column_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][0]);
	row_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][1]);
	slice_z_factor = (int)rint(Z_SUBLEVELS*projection_matrix[2][2]);
	/* Use coordinates with units of 1./0x10000 pixel spacing with origin at
		top left corner of top left of the object's image buffer. */
	for (coln=0; coln<=Largest_y1(object_data); coln++)
	{	column_x_table[coln] = coln*column_x_factor;
		column_y_table[coln] = coln*column_y_factor;
		column_z_table[coln] = (int)(Z_SUBLEVELS*MIDDLE_DEPTH+coln*column_z_factor);
	}
	switch (order)
	{	case CO_BW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
							d_mip_paint_one_voxel(this_row_z,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
						next_ptr_ptr--;
						this_row_x -= row_x_factor;
						this_row_z -= row_z_factor;
						this_row_y -= row_y_factor;
					}
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_paint_one_voxel(this_row_z,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_BW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->slices-1)*slice_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->slices-1)*slice_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->slices-1)*slice_z_factor);
			for (this_slice=object_data->slices-1; this_slice>=0; this_slice--)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x -= slice_x_factor;
				this_slice_y -= slice_y_factor;
				this_slice_z -= slice_z_factor;
			}
			break;
		case CO_BW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows-1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	if (next_ptr_ptr[1]-3-this_slice_ptr >= 0)
					{	this_ptr = next_ptr_ptr[1]-3;
						next_ptr = *next_ptr_ptr;
						for (; this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_paint_one_voxel(this_row_z,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_BW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/)+
				(object_data->rows-1)*row_x_factor);
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/)+
				(object_data->rows-1)*row_y_factor);
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]+
				(object_data->rows-1)*row_z_factor);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+(this_slice+1)*object_data->rows;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=object_data->rows-1; this_row>=0; this_row--)
				{	this_ptr = next_ptr_ptr[-1];
					for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr--;
					this_row_x -= row_x_factor;
					this_row_z -= row_z_factor;
					this_row_y -= row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_BW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	if (*next_ptr_ptr-3-this_slice_ptr >= 0)
					{	this_ptr = *next_ptr_ptr-3;
						for (next_ptr=
								next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=3)
						{	d_mip_paint_one_voxel(this_row_z,
								this_row_x, this_row_y, column_x_table,
								column_y_table, column_z_table, this_ptr,
								object_image, object_data->threshold);
							if (this_ptr == this_slice_ptr)
								break;
						}
					}
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
		case CO_FW+RO_FW+SL_FW:
			this_slice_x = (int)(0x10000*(projection_offset[0]/*+.5*/));
			this_slice_y = (int)(0x10000*(projection_offset[1]/*+.5*/));
			this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
			for (this_slice=0; this_slice<object_data->slices; this_slice++)
			{	if (check_event && check_event(this)==FIRST)
					return (401);
				this_slice_ptr =
					object_data->ptr_table[this_slice*object_data->rows];;
				next_ptr_ptr =
					object_data->ptr_table+this_slice*object_data->rows+1;
				this_ptr = next_ptr_ptr[-1];
				this_row_x = this_slice_x;
				this_row_y = this_slice_y;
				this_row_z = this_slice_z;
				for (this_row=0; this_row<object_data->rows; this_row++)
				{	for (next_ptr=*next_ptr_ptr;
							this_ptr<next_ptr; this_ptr+=3)
						d_mip_paint_one_voxel(this_row_z,this_row_x,
							this_row_y, column_x_table, column_y_table,
							column_z_table, this_ptr, object_image,
							object_data->threshold);
					next_ptr_ptr++;
					this_row_x += row_x_factor;
					this_row_z += row_z_factor;
					this_row_y += row_y_factor;
				}
				this_slice_x += slice_x_factor;
				this_slice_y += slice_y_factor;
				this_slice_z += slice_z_factor;
			}
			break;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: d_mip_paint_one_voxel
 * DESCRIPTION: Projects one voxel of an object of class DIRECT to one pixel
 *    of an object image buffer, maximum intensity projection.
 * PARAMETERS:
 *    this_row_z: The z-coordinate of the row in Z_SUBLEVELS units per depth
 *       unit in the left-handed image coordinate system.
 *    this_row_x, this_row_y: The object image buffer coordinates of the row
 *       in 0x10000 units per pixel.
 *    column_x_table, column_y_table, column_z_table: Lookup tables giving the
 *       coordinates of a voxel relative to the row, same units as row
 *       coordinates, indexed by y1 value of the voxel.
 *    this_ptr: Address of the TSE for the voxel to be projected.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *    threshold: Intensity levels defining the different materials.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables materl_opacity, materl_red,
 *    materl_green, materl_blue, gray_flag must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 6/25/01 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::d_mip_paint_one_voxel(int this_row_z, int this_row_x,
	int this_row_y, int column_x_table[1024], int column_y_table[1024],
	int column_z_table[1024], unsigned short *this_ptr,
	Object_image *object_image, int threshold[6])
{
	unsigned char *likelihood_pixel;
	unsigned icoord,front_materl, back_materl, pb, pf, likelihood;
	float this_red, this_green=0, this_blue=0;
	int this_x, this_y, this_z, *z_pixel, this_column;
	Pixel_unit *this_pixel;

	this_column = (int)(*this_ptr&0x3ff);
	icoord = this_row_x+column_x_table[this_column];
	this_x = icoord/0x10000;
	icoord = this_row_y+column_y_table[this_column];
	this_y = icoord/0x10000;
	this_z = (this_row_z+column_z_table[this_column])/Z_SUBLEVELS;
	for(back_materl=3; back_materl; back_materl--)
		if (this_ptr[2] >= threshold[(back_materl-1)*2])
			break;
	switch (back_materl)
	{	case 0:
		case 1:
			front_materl = 0;
			break;
		case 2:
			front_materl = 1;
			break;
		case 3:
			front_materl = 2;
	}
	pb = back_materl==0? 0: this_ptr[2]>=threshold[back_materl*2-1]? 255:
		(int)(255.*(this_ptr[2]-threshold[back_materl*2-2])/
		(threshold[back_materl*2-1]-threshold[back_materl*2-2]));
	pf = 255-pb;
	this_red = (float)((pf*materl_opacity[front_materl]*
		materl_red[front_materl]+
		pb*materl_opacity[back_materl]*
		materl_red[back_materl])*(1./255));
	if (!gray_flag)
	{	this_green = (float)((pf*materl_opacity[front_materl]*
			materl_green[front_materl]+
			pb*materl_opacity[back_materl]*
			materl_green[back_materl])*(1./255));
		this_blue = (float)((pf*materl_opacity[front_materl]*
			materl_blue[front_materl]+
			pb*materl_opacity[back_materl]*
			materl_blue[back_materl])*(1./255));
	}
	likelihood = (this_ptr[0]>>8&0xf8) | (this_ptr[1]>>13);
	z_pixel = object_image->z_buffer[this_y]+this_x;
	likelihood_pixel = object_image->likelihood_buffer[this_y]+this_x;
	this_pixel = (Pixel_unit *)object_image->image[this_y]+this_x*3;
	if (this_pixel[0] == V_OBJECT_IMAGE_BACKGROUND)
	{	this_pixel[0] = (unsigned short)this_red;
		if (gray_flag)
			this_pixel[1] = this_pixel[2] = this_pixel[0];
		else
		{	this_pixel[1] = (unsigned short)this_green;
			this_pixel[2] = (unsigned short)this_blue;
		}
		*likelihood_pixel = likelihood;
		*z_pixel = this_z;
	}
	else
	{	if ((unsigned short)this_red > this_pixel[0])
		{	this_pixel[0] = (unsigned short)this_red;
			*likelihood_pixel = likelihood;
			*z_pixel = this_z;
		}
		if (gray_flag)
			this_pixel[1] = this_pixel[2] = this_pixel[0];
		else
		{	if ((unsigned short)this_green > this_pixel[1])
				this_pixel[1] = (unsigned short)this_green;
			if ((unsigned short)this_blue > this_pixel[2])
				this_pixel[2] = (unsigned short)this_blue;
		}
	}
}



