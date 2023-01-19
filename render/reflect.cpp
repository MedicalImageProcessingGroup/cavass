/*
  Copyright 1993-2011 Medical Image Processing Group
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

#define Malloc(ptr, type, num, bequest) \
{	ptr = (type *)malloc((num)*sizeof(type)); \
	if (ptr == NULL) \
	{	bequest; \
		return (1); \
	} \
}


/*****************************************************************************
 * FUNCTION: render_reflection
 * DESCRIPTION: Renders the relection of an object, projecting it to its
 *    reflection image buffer.
 * PARAMETERS:
 *    object: The object to be projected.  If the image buffer is not big
 *       enough, it will be changed.  Memory must be allocated at
 *       _image.image, at _image.image[0], at _image.z_buffer, and at
 *       _image.z_buffer[0] for a _image.image_size square image, where _image
 *       is object->reflection->main_image if object_data is &object->data,
 *       object->reflection->icon if object_data is &object->icon_data.
 *       The object must be in memory.  Image depth is 8 bits.
 *    object_data: &object->data if the main object is to be projected;
 *       &object->icon_data if its icon is to be projected.
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed:
 *    object_list, image_valid, icon_valid, iw_valid.
 * ENTRY CONDITIONS: If event_priority is non-null, a call to manip_init must
 *    be made first.  The global variables anti_alias, scale, icon_scale,
 *    depth_scale, glob_angle, plane_normal, plane_displacement must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Returns 0 immediately if object->reflection is off.
 *    Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 12/4/91 by Dewey Odhner
 *    Modified: 10/16/92 Changed first voxel by Dewey Odhner
 *    Modified: 4/30/93 fixed object_plane_distance by Dewey Odhner
 *    Modified: 10/20/93 to handle units by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/2/94 to return int by Dewey Odhner
 *    Modified: 11/20/98 perspective used by Dewey Odhner
 *    Modified: 12/16/98 perspective not used for icon by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::render_reflection(Shell *object, Shell_data *object_data,
	Priority (*event_priority)(cvRenderer *))
{
	double projection_offset[3], projection_matrix[3][3], center[3],
		rotation_matrix[3][3], object_plane_normal[3],
		object_plane_distance, xysize2, unit, dimin;
	int j, status, *angle_shade;
	Display_mode mode;
	Object_image *object_image;
	Rendering_parameters rend_params;

	if (object->reflection==NULL || !object->reflection->on)
		return (0);
	mode =
		object_data==&object->icon_data
		?	ICON
		:	anti_alias
			?	object->reflection->opacity!=.5
				?	ANTI_ALIAS
				:	ONE_TO_ONE
			:	PIXEL_REPLICATE;
	rend_params.perspective = mode==ICON? 0: perspective;
	if (event_priority && event_priority(this)==FIRST)
		return (401);
	/* transform plane to object coordinates from center */
	unit = unit_mm(object_data);
	AtoM(rotation_matrix, object->angle[0], object->angle[1],object->angle[2]);
	vector_matrix_multiply(object_plane_normal,plane_normal,rotation_matrix);
	object_plane_distance = plane_displacement-
		(object->displacement[0]*plane_normal[0]+
		 object->displacement[1]*plane_normal[1]+
		 object->displacement[2]*plane_normal[2]);
	/* transform plane to object coordinates from first voxel */
	object_plane_distance += .5*unit*(
		(Max_coordinate(object_data, 0)+Min_coordinate(object_data, 0))*
		object_plane_normal[0]+
		(Max_coordinate(object_data, 1)-Min_coordinate(object_data, 1))*
		object_plane_normal[1]+
		(Max_coordinate(object_data, 2)-Min_coordinate(object_data, 2))*
		object_plane_normal[2]);
	/* transform plane to object coordinates in voxel units */
	xysize2 = Slice_spacing(object_data);
	object_plane_normal[0] *=
		object_data->file->file_header.str.xysize[0]*unit;
	object_plane_normal[1] *=
		object_data->file->file_header.str.xysize[1]*unit;
	object_plane_normal[2] *= xysize2*unit;
	/* Note object_plane_normal is not unit length and
	   object_plane_distance is scaled by length of object_plane_normal.
	 */

	get_structure_transformation(projection_matrix, projection_offset,
		rotation_matrix, center, object, mode, TRUE);
	if (event_priority && event_priority(this)==FIRST)
		return (401);
	if (!object->reflection->shade_lut_computed)
	{	VComputeShadeLut(object->reflection->shade_lut,
			object->reflection->diffuse_exponent,
			object->reflection->diffuse_n,
			object->reflection->specular_fraction,
			object->reflection->specular_exponent,
			object->reflection->specular_n);
		object->reflection->shade_lut_computed = TRUE;
	}
	object_image =
		mode==ICON
		?	&object->reflection->icon
		:	&object->reflection->main_image;
	object_image->projected = BAD;
	j = (int)ceil(object->diameter*buffer_scale(mode))+1;
	if (mode==ANTI_ALIAS && j&1)
		j++;
	if (object_image->image_size != j)
	{	if (object_image->image_size != 0)
		{	free(object_image->image[0]);
			free(object_image->z_buffer[0]);
			free(object_image->image);
			free(object_image->z_buffer);
			object_image->image_size = 0;
		}
		Malloc(object_image->image, char *, j+1, {})
		Malloc(object_image->z_buffer, int *, j+1, free(object_image->image))
		Malloc(object_image->image[0], char, j*j, free(object_image->image);
			free(object_image->z_buffer))
		Malloc(object_image->z_buffer[0], int, j*j,
			free(object_image->image[0]); free(object_image->image);
			free(object_image->z_buffer))
		object_image->image_size = j;
		for (j=1; j<=object_image->image_size; j++)
		{	object_image->image[j] =
				object_image->image[0]+j*object_image->image_size;
			object_image->z_buffer[j] =
				object_image->z_buffer[0]+j*object_image->image_size;
		}
	}
	for (j=1; j<=object_image->image_size; j++)
	{	memset(object_image->image[j-1], OBJECT_IMAGE_BACKGROUND,
			object_image->image_size);
		memset(object_image->z_buffer[j-1], 0,
			object_image->image_size*sizeof(int));
	}
	dimin = (100-perspective)/
		(100-perspective*(.5+center[2]/Z_BUFFER_LEVELS));
	switch (mode)
	{
		case ANTI_ALIAS:
			object_image->image_location[0] =
				2*(int)rint(.5*(dimin*center[0]-.5*object_image->image_size));
			object_image->image_location[1] =
				2*(int)rint(.5*(dimin*center[1]-.5*object_image->image_size));
			break;
		case ONE_TO_ONE:
			object_image->image_location[0] =
				2*(int)rint(dimin*center[0]-.5*object_image->image_size);
			object_image->image_location[1] =
				2*(int)rint(dimin*center[1]-.5*object_image->image_size);
			break;
		case PIXEL_REPLICATE:
			object_image->image_location[0] =
				(int)rint(dimin*center[0]-.5*object_image->image_size);
			object_image->image_location[1] =
				(int)rint(dimin*center[1]-.5*object_image->image_size);
			break;
		case ICON:
			object_image->image_location[0] =
				(int)rint(center[0]-.5*object_image->image_size);
			object_image->image_location[1] =
				(int)rint(center[1]-.5*object_image->image_size);
			break;
		default:
			assert(FALSE);
	}
	if (mode == ONE_TO_ONE)
	{	projection_offset[0] -= object_image->image_location[0]/2;
		projection_offset[1] -= object_image->image_location[1]/2;
	}
	else
	{	projection_offset[0] -= object_image->image_location[0];
		projection_offset[1] -= object_image->image_location[1];
	}
	if (event_priority && event_priority(this)==FIRST)
		return (401);
	assert(mode!=ICON || voxel_spacing(object_data)*buffer_scale(mode)<1 ||
		voxel_spacing(object_data)>voxel_spacing(&object_list->icon_data));
	rend_params.shade_lut = object->reflection->shade_lut;
	VGetAngleShades(&angle_shade, rotation_matrix, &rend_params,
		SHADE_LUT_FLAG, st_cl(object_data), (st_cl(object_data)==BINARY_B||
		st_cl(object_data)==T_SHELL) && object->O.opacity<1);
	object_image->projected = mode;
	if (voxel_spacing(object_data)*buffer_scale(mode) < .99)
		if (perspective && mode!=ICON)
			status = perspec_project_r(object_data, object_image,
				projection_matrix, projection_offset, object_plane_normal,
				object_plane_distance, angle_shade, event_priority);
		else
			status = quick_project_r(object_data, object_image,
				projection_matrix, projection_offset,
				object_plane_normal, object_plane_distance, angle_shade,
				event_priority);
	else
		if (perspective && mode!=ICON)
			status = perspec_patch_project_r(object_data, object_image,
				projection_matrix, projection_offset, object_plane_normal,
				object_plane_distance, angle_shade, event_priority);
		else
			status = patch_project_r(object_data, object_image,
				projection_matrix, projection_offset,
				object_plane_normal, object_plane_distance, angle_shade,
				event_priority);
	if (status)
		object_image->projected = BAD;
	return (status);
}


/*****************************************************************************
 * FUNCTION: patch_project_r
 * DESCRIPTION: Renders the relection of an object, projecting it to its
 *    relection image buffer using a patch of pixels for each voxel.
 * PARAMETERS:
 *    object_data: The object data to be projected.  The object must be
 *       in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *       Image depth is 8 bits.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer.
 *    object_plane_normal: The normal vector in voxel units to the relection
 *       (clipping) plane.
 *    object_plane_distance: The displacement of the plane from the first
 *       voxel along the normal vector.  Note object_plane_normal is not unit
 *       length and object_plane_distance is scaled by length of
 *       object_plane_normal.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed:
 *    object_list, image_valid, icon_valid, iw_valid.
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/2/94 to return int by Dewey Odhner
 *    Modified: 8/28/01 to handle BINARY_B shells by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::patch_project_r(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	double object_plane_normal[3], double object_plane_distance,
	int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *))
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
		this_slice_x, this_slice_y, this_slice_z, error_code, B;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double this_slice_distance, this_row_distance;

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
	B = st_cl(object_data)==BINARY_B;
	this_slice_x = (int)(0x10000*(projection_offset[0]+.25));
	this_slice_y = (int)(0x10000*(projection_offset[1]+.25));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	this_slice_distance = object_plane_distance;
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (event_priority && event_priority(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		this_row_distance = this_slice_distance;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column = B? (*this_ptr&0x3ff)<<1|
						((this_ptr[1]&0x8000)!=0): *this_ptr&0x3ff;
					if (this_row_distance-
							this_column*object_plane_normal[0] < 0)
						continue;
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
			this_row_distance -= object_plane_normal[1];
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
		this_slice_distance -= object_plane_normal[2];
	}
	free(patch);
	return (0);
}


/*****************************************************************************
 * FUNCTION: quick_project_r
 * DESCRIPTION: Renders the relection of an object, projecting it to its
 *    relection image buffer using one pixel for each voxel.  (If the scale is
 *    too large, there will be holes.)
 * PARAMETERS:
 *    object_data: The object data to be projected.  The object must be
 *       in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *       Image depth is 8 bits.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer.
 *    object_plane_normal: The normal vector in voxel units to the relection
 *       (clipping) plane.
 *    object_plane_distance: The displacement of the plane from the first
 *       voxel along the normal vector.  Note object_plane_normal is not unit
 *       length and object_plane_distance is scaled by length of
 *       object_plane_normal.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed:
 *    object_list, image_valid, icon_valid, iw_valid.
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/2/94 to return int by Dewey Odhner
 *    Modified: 8/28/01 to handle BINARY_B shells by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::quick_project_r(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	double object_plane_normal[3], double object_plane_distance,
	int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade,
		this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor, jx, jy, jz, coln,
		column_x_table[1024], column_y_table[1024], column_z_table[1024],
		this_x, this_y, this_z,
		this_slice_x, this_slice_y, this_slice_z, B;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double this_slice_distance, this_row_distance;

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
	B = st_cl(object_data)==BINARY_B;
	this_slice_x = (int)(0x10000*(projection_offset[0]+.5));
	this_slice_y = (int)(0x10000*(projection_offset[1]+.5));
	this_slice_z = (int)(Z_SUBLEVELS*projection_offset[2]);
	this_slice_distance = object_plane_distance;
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (event_priority && event_priority(this)==FIRST)
			return (401);
		next_ptr_ptr = object_data->ptr_table+
			this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		this_row_distance = this_slice_distance;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr;
					this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column = B? (*this_ptr&0x3ff)<<1|
						((this_ptr[1]&0x8000)!=0): *this_ptr&0x3ff;
					if (this_row_distance-
							this_column*object_plane_normal[0] < 0)
						continue;
					this_x= (this_row_x+column_x_table[this_column])/
						(unsigned)0x10000;
					this_y= (this_row_y+column_y_table[this_column])/
						(unsigned)0x10000;
					this_z= (this_row_z+column_z_table[this_column])/
						Z_SUBLEVELS;
					if (this_z< object_image->z_buffer[this_y][this_x])
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
			this_row_distance -= object_plane_normal[1];
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
		this_slice_distance -= object_plane_normal[2];
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: perspec_patch_project_r
 * DESCRIPTION: Renders the relection of a binary SHELL0 object, by perspective
 *    projection to its relection image buffer using a patch of pixels for
 *    each voxel.
 * PARAMETERS:
 *    object_data: The object data to be projected.  The object must be
 *       in memory.
 *    object_image: The object image to be projected.  The image buffer must
 *       be big enough for the projection of the object.
 *       Image depth is 8 bits.
 *    projection_matrix: The matrix which transforms a vector in structure
 *       space in units of voxels to image space in units of pixels.
 *    projection_offset: The offset in pixels of the center of the first voxel
 *       of the first row of the first slice from the top left corner of the
 *       object image buffer.
 *    object_plane_normal: The normal vector in voxel units to the relection
 *       (clipping) plane.
 *    object_plane_distance: The displacement of the plane from the first
 *       voxel along the normal vector.  Note object_plane_normal is not unit
 *       length and object_plane_distance is scaled by length of
 *       object_plane_normal.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed:
 *    object_list, image_valid, icon_valid, iw_valid.
 * ENTRY CONDITIONS: The variables perspective must be properly set.
 *    Any entry conditions of event_priority must be met.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 11/20/98 by Dewey Odhner
 *    Modified: 8/28/01 to handle BINARY_B shells by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::perspec_patch_project_r(Shell_data *object_data,
	Object_image *object_image, double projection_matrix[3][3],
	double projection_offset[3], double object_plane_normal[3],
	double object_plane_distance, int angle_shade[G_CODES],
	Priority (*event_priority)(cvRenderer *))
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
	int this_line, patch_line, this_shade, error_code, center_loc[2], B;
	char *this_pixel, *line_end;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double this_slice_distance, this_row_distance;

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
	B = st_cl(object_data)==BINARY_B;
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
	this_slice_distance = object_plane_distance;
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (event_priority && event_priority(this)==FIRST)
		{
			free(patch);
			return (401);
		}
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		this_row_distance = this_slice_distance;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column = B? (*this_ptr&0x3ff)<<1|
						((this_ptr[1]&0x8000)!=0): *this_ptr&0x3ff;
					if (this_row_distance-
							this_column*object_plane_normal[0] < 0)
						continue;
					this_z = (int)(this_row_z+column_z_table[this_column]);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-perspective)/
						(100*Z_BUFFER_LEVELS-perspective*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-perspective)/
						(100*Z_BUFFER_LEVELS-perspective*this_z)*
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
			this_row_distance -= object_plane_normal[1];
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
		this_slice_distance -= object_plane_normal[2];
	}
	free(patch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: perspec_project_r
 * DESCRIPTION: Renders the relection of a binary SHELL0 object, by perspective
 *    projection using one pixel for each voxel.  (If the scale is too large,
 *    there will be holes.)
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
 *    object_plane_normal: The normal vector in voxel units to the relection
 *       (clipping) plane.
 *    object_plane_distance: The displacement of the plane from the first
 *       voxel along the normal vector.  Note object_plane_normal is not unit
 *       length and object_plane_distance is scaled by length of
 *       object_plane_normal.
 *    angle_shade: A look-up table which maps gradient codes to shades;
 *       0 (not projected) or 1 (dark) to MAX_ANGLE_SHADE (bright).
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 * ENTRY CONDITIONS: Any entry conditions of event_priority must be met.
 * RETURN VALUE:
 *    0: successful
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Undefined if parameters are not valid.
 * HISTORY:
 *    Created: 11/20/98 by Dewey Odhner
 *    Modified: 8/28/01 to handle BINARY_B shells by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::perspec_project_r(Shell_data *object_data, Object_image *object_image,
	double projection_matrix[3][3], double projection_offset[3],
	double object_plane_normal[3], double object_plane_distance,
	int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *))
{
	int this_column, this_row, this_slice, this_angle_shade;
	double this_row_x, this_row_y, this_row_z,
		column_x_factor, slice_x_factor, row_x_factor, column_y_factor,
		slice_y_factor, row_y_factor, column_z_factor, slice_z_factor,
		row_z_factor;
	int this_x, this_y, this_z, coln, center_loc[2], B;
	double column_x_table[1024], column_y_table[1024], column_z_table[1024];
	double this_slice_x, this_slice_y, this_slice_z;
	unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
	double this_slice_distance, this_row_distance;

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
	B = st_cl(object_data)==BINARY_B;
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
	this_slice_distance = object_plane_distance;
	for (this_slice=0; this_slice<object_data->slices; this_slice++)
	{	if (event_priority && event_priority(this)==FIRST)
			return (401);
		next_ptr_ptr = object_data->ptr_table+this_slice*object_data->rows+1;
		this_ptr = next_ptr_ptr[-1];
		this_row_x = this_slice_x;
		this_row_y = this_slice_y;
		this_row_z = this_slice_z;
		this_row_distance = this_slice_distance;
		for (this_row=0; this_row<object_data->rows; this_row++)
		{	for (next_ptr= *next_ptr_ptr; this_ptr<next_ptr; this_ptr+=2)
			{	if ((this_angle_shade=angle_shade[this_ptr[1]&0x7fff]))
				{	this_column = B? (*this_ptr&0x3ff)<<1|
						((this_ptr[1]&0x8000)!=0): *this_ptr&0x3ff;
					if (this_row_distance-
							this_column*object_plane_normal[0] < 0)
						continue;
					this_z = (int)(this_row_z+column_z_table[this_column]+.5);
					assert(this_z < Z_BUFFER_LEVELS);
					this_x = -center_loc[0]+
						(int)(Z_BUFFER_LEVELS*(100-perspective)/
						(100*Z_BUFFER_LEVELS-perspective*this_z)*
						(this_row_x+column_x_table[this_column]+
						center_loc[0])+.5);
					this_y = -center_loc[1]+
						(int)(Z_BUFFER_LEVELS*(100-perspective)/
						(100*Z_BUFFER_LEVELS-perspective*this_z)*
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
			this_row_distance -= object_plane_normal[1];
		}
		this_slice_x += slice_x_factor;
		this_slice_y += slice_y_factor;
		this_slice_z += slice_z_factor;
		this_slice_distance -= object_plane_normal[2];
	}
	return (0);
}
