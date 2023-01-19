/*
  Copyright 1993-2011, 2017 Medical Image Processing Group
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

#define OUT_BLOCK_SIZE 0x800 /* in short words */

bool polygon_is_clockwise(X_Point points[], int npoints);

static const int image_x=0, image_y=0, image2_x=0, image2_y=0;
static const Window display_area2=2;

/*****************************************************************************
 * FUNCTION: compute_z_extrema
 * DESCRIPTION: Computes prism->min_z_point and prism->max_z_point.
 * PARAMETERS:
 *    prism: A struct Prism with n and base_points initialized.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: prism->n and prism->base_points must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void compute_z_extrema(struct Prism *prism)
{
	int this_point;

	prism->min_z_point = prism->max_z_point = 0;
	for (this_point=0; this_point<prism->n; this_point++)
	{	if (prism->base_points[this_point][2]<
				prism->base_points[prism->min_z_point][2])
			prism->min_z_point = this_point;
		else if (prism->base_points[this_point][2]>
				prism->base_points[prism->max_z_point][2])
			prism->max_z_point = this_point;
	}
}

/*****************************************************************************
 * FUNCTION: get_prism
 * DESCRIPTION: Computes a prism whose base is in the viewing plane.
 *    The coordinates of the prism are in the strucure coordinates of
 *    the object with units of voxels.
 * PARAMETERS:
 *    points: The display_area coordinates of the vertices of the prism.
 *    npoints: The number of base vertices.
 *    depth: The altitude of the prism in mm not including a margin on the
 *       front end.
 *    z: The z-coordinate of the near base in depth-units before the margin is
 *       added on the front end.
 *    object, object_data: The object whose strucure coordinate system to use.
 *    draw_window: The display_area coordinates to use: display_area or
 *       display_area2.
 * SIDE EFFECTS: The memory at the address returned by this function will be
 *    reused or freed on the next call.  An error message may be issued.
 * ENTRY CONDITIONS: The memory at the address returned by this function must
 *    not be altered or freed and another call made to this function.
 *    A call to manip_init should be made first.
 * RETURN VALUE: The address of the computed prism.
 * EXIT CONDITIONS: Returns NULL if a memory allocation failure occurs.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/26/94 margin increased by Dewey Odhner
 *
 *****************************************************************************/
struct Prism *cvRenderer::get_prism(X_Point points[], int npoints,
		double depth, int z, Shell *object, Shell_data *object_data,
		Window draw_window)
{
	triple *point_ptr;
	X_Point *xpoint_ptr, *last_point;

	assert(npoints > 0);
	if (mPrism.base_points)
		free(mPrism.base_points);
	if ((mPrism.base_points=(triple *)malloc(npoints*sizeof(triple))) == NULL)
	{	report_malloc_error();
		return (NULL);
	}
	/* add margin */
	depth += Z_BUFFER_LEVELS/2/depth_scale;
	z += Z_BUFFER_LEVELS/2;

	mPrism.n = npoints;
	point_ptr = mPrism.base_points;
	last_point = points+npoints-1;
	for (xpoint_ptr=points; xpoint_ptr<=last_point; xpoint_ptr++)
		display_area_to_voxel_coords(*point_ptr++, xpoint_ptr->x,
			xpoint_ptr->y, z, object, object_data, draw_window);
	display_area_to_voxel_coords(mPrism.vector, points->x, points->y,
		(int)rint(z-depth*depth_scale), object, object_data, draw_window);
	vector_subtract(mPrism.vector, mPrism.vector, mPrism.base_points[0]);
	mPrism.clockwise =	mPrism.vector[2]>=0
						?	polygon_is_clockwise(points, npoints)
						:	!polygon_is_clockwise(points, npoints);
	compute_z_extrema(&mPrism);
	return(&mPrism);
}

/*****************************************************************************
 * FUNCTION: slice_line
 * DESCRIPTION: Adds to the slice_point_list the point where the line through
 *    a & b intersects the current slice.
 * PARAMETERS:
 *    a, b: The coordinates of two points that define a line.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables this_slice, slice_point_list,
 *    slice_points must be valid.  There must be enough memory at
 *    slice_point_list for another point.  b[2] and a[2] must differ.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::slice_line(triple a, triple b)
{
	double ratio;

	ratio = (this_slice-a[2])/(b[2]-a[2]);
	slice_point_list[slice_points][0] = a[0]+ratio*(b[0]-a[0]);
	slice_point_list[slice_points][1] = a[1]+ratio*(b[1]-a[1]);
	slice_points++;
}

/*****************************************************************************
 * FUNCTION: slice_rect
 * DESCRIPTION: The parameters to the function define a parallelogram (which
 *    will always be a rectangle in this program) whose corners are a, b, c, d,
 *    where b = a+vector, d = c+vector.  This function adds to the
 *    slice_point_list any point where one of the sides ac, ab, or bd
 *    intersects the current slice.  Where cd intersects the slice is handled
 *    with an adjacent rectangle.
 * PARAMETERS:
 *    vector: The vector to be added to the point a to get b, and to point c
 *       to get d.
 *    a, c: Two corners of a parallelogram.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables this_slice, slice_point_list,
 *    slice_points must be valid.  There must be enough memory at
 *    slice_point_list for two more points.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::slice_rect(triple vector, triple a, triple c)
{
	triple b, d;
	int i;

	for (i=0; i<3; i++)
	{	b[i] = a[i]+vector[i];
		d[i] = c[i]+vector[i];
	}
	if (a[2] > this_slice)
		if (b[2] > this_slice)
			if (c[2] > this_slice)
			{	if (d[2] <= this_slice)
					slice_line(b, d);
			}
			else
				if (d[2] > this_slice)
					slice_line(a, c);
				else
					if (vector[2] > 0)
					{	slice_line(a, c);
						slice_line(b, d);
					}
					else
					{	slice_line(b, d);
						slice_line(a, c);
					}
		else
			if (c[2] > this_slice)
				if (d[2] > this_slice)
				{	slice_line(a, b);
					slice_line(b, d);
				}
				else
					slice_line(a, b);
			else
			{	slice_line(a, b);
				slice_line(a, c);
			}
	else
		if (b[2] > this_slice)
			if (c[2] > this_slice)
			{	slice_line(a, b);
				slice_line(a, c);
			}
			else
				if (d[2] > this_slice)
					slice_line(a, b);
				else
				{	slice_line(a, b);
					slice_line(b, d);
				}
		else
			if (c[2] > this_slice)
				if (d[2] > this_slice)
					if (vector[2] > 0)
					{	slice_line(b, d);
						slice_line(a, c);
					}
					else
					{	slice_line(a, c);
						slice_line(b, d);
					}
				else
					slice_line(a, c);
			else
				if (d[2] > this_slice)
					slice_line(b, d);
}

/*****************************************************************************
 * FUNCTION: get_slice_points
 * DESCRIPTION: Constructs a list of points defining where prism intersects
 *    current slice.
 * PARAMETERS:
 *    prism: Defines the prism; all fields must be valid.
 * SIDE EFFECTS: The static variables min_row, max_row will be set.
 * ENTRY CONDITIONS: The static variables this_slice, slice_point_list,
 *    rws must be valid.  There must be enough memory at
 *    slice_point_list for all the points.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_slice_points(struct Prism *prism)
{
	int this_point;
	dpair *this_slice_point;

	slice_points = 0;
	if (prism->clockwise)
	{	for (this_point=0; this_point<prism->n-1; this_point++)
			slice_rect(prism->vector, prism->base_points[this_point],
					prism->base_points[this_point+1]);
		slice_rect(prism->vector, prism->base_points[prism->n-1],
				prism->base_points[0]);
	}
	else
	{	for (this_point=prism->n-1; this_point>0; this_point--)
			slice_rect(prism->vector, prism->base_points[this_point],
					prism->base_points[this_point-1]);
		slice_rect(prism->vector, prism->base_points[0],
				prism->base_points[prism->n-1]);
	}
	memcpy(slice_point_list+slice_points, slice_point_list, sizeof(dpair));
	max_row = 0;
	min_row = rws-1;
	for (this_slice_point=slice_point_list;
			this_slice_point<slice_point_list+slice_points; this_slice_point++)
	{	if (this_slice_point[0][1] > max_row)
			max_row = (int)ceil(this_slice_point[0][1]);
		if (this_slice_point[0][1] < min_row)
			min_row = (int)floor(this_slice_point[0][1]);
	}
	if (max_row >= rws)
		max_row = rws-1;
}

/* relative x coordinate of relevant edge */
#define Rel_x(voi) ((voi)&PX? (voi)-PX: (voi)+1)

/*****************************************************************************
 * FUNCTION: next_row
 * DESCRIPTION: Finishes up after finding the voxels-of-intersection in a row,
 *    and increments the row counter.
 * PARAMETERS: None
 * SIDE EFFECTS: The static variables p_buff_out_ptr, p_voxs_in_out_buffer,
 *    p_o_buffer_size, p_o_data, this_ptr_ptr, last_out_ptr, this_row,
 *    buffer_row_ptr may be changed.  An error message may be issued.
 * ENTRY CONDITIONS: The static variables buffer_row_ptr, p_buff_out_ptr,
 *    p_voxs_in_out_buffer, p_o_buffer_size, p_o_data, slice_points,
 *    this_ptr_ptr, last_out_ptr, this_row must be valid.
 * RETURN VALUE: 0 if successful.
 * EXIT CONDITIONS: Returns 1 if memory allocation fails.
 *    Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::next_row()
{
	unsigned short *in_ptr, *out_ptr, *seek_ptr, this_voxel, *temp_ptr;
	int in_count, old_in_count, this_x;

	/* sort_output */
	for (in_ptr=buffer_row_ptr; in_ptr<p_buff_out_ptr; in_ptr++)
	{	this_voxel = *in_ptr;
		this_x = Rel_x(this_voxel);
		for (seek_ptr=in_ptr; seek_ptr>buffer_row_ptr; seek_ptr--)
		{	if (Rel_x(seek_ptr[-1]) <= this_x)
				break;
			*seek_ptr = seek_ptr[-1];
		}
		*seek_ptr = this_voxel;
	}

	/* clean up overlaps */
	in_count = 0;
	for(in_ptr=out_ptr=buffer_row_ptr; in_ptr<p_buff_out_ptr; )
	{	this_voxel = *in_ptr;
		this_x = Rel_x(*in_ptr);
		old_in_count = in_count;
		for (; in_ptr<p_buff_out_ptr && Rel_x(*in_ptr)==this_x; in_ptr++)
			in_count += *in_ptr&PX? 1: -1;
		if (in_count>0 && old_in_count<=0)
			*out_ptr++ = this_x|PX;
		else if (in_count<=0 && old_in_count>0)
			*out_ptr++ = this_x-1;
	}
	assert(in_count == 0);
	p_voxs_in_out_buffer -= (int)(p_buff_out_ptr-out_ptr);
	p_buff_out_ptr = out_ptr;

	if (p_voxs_in_out_buffer >= p_o_buffer_size-slice_points-2)
	{	p_o_buffer_size += OUT_BLOCK_SIZE;
		temp_ptr = (unsigned short *)realloc(p_o_data, p_o_buffer_size*2);
		if (temp_ptr == NULL)
		{	report_malloc_error();
			free(p_o_data);
			return (1);
		}
		p_o_data = temp_ptr;
		p_buff_out_ptr = p_o_data+p_voxs_in_out_buffer;
	}

	*this_ptr_ptr++ = last_out_ptr;
	last_out_ptr = p_voxs_in_out_buffer;
	this_row++;
	buffer_row_ptr = p_buff_out_ptr;
	return (0);
}

/*****************************************************************************
 * MACRO: output_voxel
 * DESCRIPTION: Adds a voxel-of-intersection to the voxel-of-intersection list.
 * PARAMETERS:
 *    rfn: The rounding function to be used (ceil or floor).
 *    or: PX or 0; PX marks a left-voi.
 * SIDE EFFECTS: The static variables p_buff_out_ptr, p_voxs_in_out_buffer,
 *    slice_point_list may be changed.
 * ENTRY CONDITIONS: The static variables p_buff_out_ptr, p_voxs_in_out_buffer,
 *    slice_point_list, clmns, this_row must be valid.
 *    The int variable this_point is the index into slice_point_list of the
 *    voxel to be added to the voxel-of-intersection list.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
#define output_voxel(rfn, Or) \
{	int x; \
\
	x = \
	(int) rfn(slice_point_list[this_point][0]+ \
		(this_row-slice_point_list[this_point][1])* \
		(	slice_point_list[this_point+1][0]- \
			slice_point_list[this_point][0] \
		)/ \
		(	slice_point_list[this_point+1][1]- \
			slice_point_list[this_point][1] \
		) \
	); \
	if (x < 0) \
		x = 0; \
	if (x >= clmns) \
		x = clmns-1; \
	*p_buff_out_ptr++ = x | Or; \
	p_voxs_in_out_buffer++; \
}
#define output_left_voxel output_voxel(ceil, PX)
#define output_right_voxel output_voxel(floor, 0)

/*****************************************************************************
 * FUNCTION: get_prism_voi_list
 * DESCRIPTION: Constructs a voxel-of-intersection list for a prism.
 *    The coordinates of the prism are in the
 *    strucure coordinates of the object with units of voxels.
 * PARAMETERS:
 *    ptr_table: The pointer table of the voxel-of-intersection list will be
 *       stored here.  There must be sufficient space.
 *    prism: A struct Prism with n and base_points initialized.
 *    icon_flag: Non-zero if the icon is to be used.
 *    draw_window: The display_area coordinates to use: display_area or
 *       display_area2.
 *    primary_object: The object whose strucure coordinate system to use.
 * SIDE EFFECTS: The static variables rws, clmns, p_o_data, p_o_buffer_size,
 *    slice_point_list, p_buff_out_ptr, buffer_row_ptr, last_out_ptr,
 *    p_voxs_in_out_buffer, this_ptr_ptr, min_slice, max_slice, this_slice,
 *    this_row, min_row, max_row, end_ptr_ptr will be set.
 *    An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/27/95 int changed to long *ptr_table by Dewey Odhner
 *    Modified: 7/14/99 prism not constrained by viewing direction
 *       by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::get_prism_voi_list(unsigned short **ptr_table, struct Prism *prism,
	int icon_flag, Window draw_window, Shell *primary_object)
{
	int error_code;
	Shell_data *object_data;

	object_data =
		icon_flag? &primary_object->icon_data: &primary_object->main_data;
	if (prism == NULL)
		return (1);
	rws = object_data->rows;
	clmns = (int)Largest_y1(object_data)+1;
	if((p_o_data=(unsigned short *)malloc(OUT_BLOCK_SIZE*2)) == NULL)
	{	report_malloc_error();
		return (1);
	}
	if ((slice_point_list=(dpair *)malloc(2*(prism->n+1)*sizeof(dpair))) == NULL)
	{	report_malloc_error();
		free(p_o_data);
		return (1);
	}
	p_o_buffer_size = OUT_BLOCK_SIZE;
	p_buff_out_ptr = buffer_row_ptr = p_o_data;
	last_out_ptr = p_voxs_in_out_buffer = 0;
	this_ptr_ptr = (size_t *)ptr_table;
	min_slice = (int)(
		prism->vector[2]<0
		?	rint(prism->base_points[prism->min_z_point][2]+prism->vector[2])
		:	rint(prism->base_points[prism->min_z_point][2]));
	if (min_slice >= object_data->slices)
		min_slice = object_data->slices-1;
	max_slice = (int)(
		prism->vector[2]>0
		?	rint(prism->base_points[prism->max_z_point][2]+prism->vector[2])
		:	rint(prism->base_points[prism->max_z_point][2]));
	if (max_slice >= object_data->slices)
		max_slice = object_data->slices-1;
	for (this_slice=0; this_slice<min_slice; this_slice++)
		for (this_row=0; this_row<rws;)
		{	error_code = next_row();
			if (error_code)
			{	free(p_o_data);
				free(slice_point_list);
				return (error_code);
			}
		}
	for (; this_slice<=max_slice; this_slice++)
	{	get_slice_points(prism);
		for (this_row=0; this_row<min_row;)
		{	error_code = next_row();
			if (error_code)
			{	free(p_o_data);
				free(slice_point_list);
				return (error_code);
			}
		}
		while (this_row <= max_row)
		{	int this_point;

			for (this_point=0; this_point<slice_points; this_point++)
			{	if (slice_point_list[this_point][1]>this_row)
				{	if (slice_point_list[this_point+1][1]<=this_row)
						output_left_voxel
				}
				else
					if (slice_point_list[this_point+1][1]>this_row)
						output_right_voxel
			}
			error_code = next_row();
			if (error_code)
			{	free(p_o_data);
				free(slice_point_list);
				return (error_code);
			}
		}
		while (this_row < rws)
		{	error_code = next_row();
			if (error_code)
			{	free(p_o_data);
				free(slice_point_list);
				return (error_code);
			}
		}
	}
	for (; this_slice<object_data->slices; this_slice++)
		for (this_row=0; this_row<rws;)
		{	error_code = next_row();
			if (error_code)
			{	free(p_o_data);
				free(slice_point_list);
				return (error_code);
			}
		}
	free(slice_point_list);
	error_code = next_row();
	if (error_code)
	{	free(p_o_data);
		return (error_code);
	}
	end_ptr_ptr = (size_t *)ptr_table+rws*object_data->slices;
	int n = 0;
	for (this_ptr_ptr=(size_t *)ptr_table; this_ptr_ptr<=end_ptr_ptr;
			this_ptr_ptr++,n++)
		ptr_table[n] = p_o_data+*this_ptr_ptr;
	return (0);
}

/*****************************************************************************
 * FUNCTION: polygon_is_clockwise
 * DESCRIPTION: Determines whether the vertices of a polygon are in clockwise
 *    order.
 * PARAMETERS:
 *    points: The coordinates of the vertices of a polygon; when connected in
 *       order by line segments and the last point to the first, must form
 *       a simple closed curve.
 *    npoints: The number of vertices.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if the vertices of a polygon are in clockwise order.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
bool polygon_is_clockwise(X_Point points[], int npoints)
{
	int top_point, next_point, last_point;

	top_point = 0;
	for (next_point=1; next_point<npoints; next_point++)
		if (points[next_point].y < points[top_point].y)
			top_point = next_point;
	last_point = top_point-1;
	if (last_point < 0)
		last_point += npoints;
	next_point = (top_point+1)%npoints;
	return (
		points[next_point].y==points[top_point].y
		?	points[next_point].x>points[top_point].x
		:	points[top_point].y==points[last_point].y
			?	points[top_point].x>points[last_point].x
			:	(points[next_point].x-points[top_point].x)*
				(points[last_point].y-points[top_point].y)>
				(points[last_point].x-points[top_point].x)*
				(points[next_point].y-points[top_point].y)
	);
}

/*****************************************************************************
 * FUNCTION: display_area_to_voxel_coords
 * DESCRIPTION: Converts display area coordinates to structure coordinates
 *    in units of voxels.
 * PARAMETERS:
 *    voxel_coords: The structure coordinates go here.
 *    x, y, z: The (left-handed) display_area coordinates.
 *    object, obj_data: The object whose strucure coordinate system to use.
 *    window: The display_area coordinates to use: display_area or
 *       display_area2.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/26/93 to work with updated structure protocol by Dewey Odhner
 *    Modified: 10/20/93 to work with units by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::display_area_to_voxel_coords(double voxel_coords[3], int x,
	int y, int z, Shell *object, Shell_data *obj_data, Window window)
{
	double rotation[3][3], translation[3], mm;

	assert(obj_data==&object->icon_data || obj_data==&object->main_data);
	/* convert from display area to image coordinates from center of image */
	if ((image_mode==SEPARATE || image_mode==FUZZY_CONNECT) &&
			window==display_area2)
	{	voxel_coords[0] = x-image2->width/2-image2_x;
		voxel_coords[1] = y-image2->height/2-image2_y;
	}
	else
	{	voxel_coords[0] = x-main_image->width/2-image_x;
		voxel_coords[1] = y-main_image->height/2-image_y;
	}
	voxel_coords[2] = z;
	/* convert to physical units */
	voxel_coords[0] /= scale;
	voxel_coords[1] /= scale;
	voxel_coords[2] = (MIDDLE_DEPTH-voxel_coords[2])/depth_scale;
	/* convert from image coordinates to object coordinates */
	get_object_transformation(rotation, translation, object, FALSE);
	vector_subtract(voxel_coords, voxel_coords, translation);
	vector_matrix_multiply(voxel_coords, voxel_coords, rotation);
	/* convert from physical coordinates to voxel coordinates */
	mm = 1/unit_mm(obj_data);
	voxel_coords[0] *= mm;
	voxel_coords[1] *= mm;
	voxel_coords[2] *= mm;
	voxel_coords[0] +=
		.5*(Max_coordinate(obj_data, 0)+Min_coordinate(obj_data, 0));
	voxel_coords[1] +=
		.5*(Max_coordinate(obj_data, 1)-Min_coordinate(obj_data, 1));
	voxel_coords[2] +=
		.5*(Max_coordinate(obj_data, 2)-Min_coordinate(obj_data, 2));
	voxel_coords[0] /= obj_data->file->file_header.str.xysize[0];
	voxel_coords[1] /= obj_data->file->file_header.str.xysize[1];
	voxel_coords[2] /= Slice_spacing(obj_data);
}

/*****************************************************************************
 * FUNCTION: is_in_polygon
 * DESCRIPTION: Test whether point is in polygon
 * PARAMETERS:
 *    x, y: the coordinates of the point
 *    vertices: The vertices of a polygon;
 *       when connected in order by line segments and the last point to the
 *       first, must form a simple closed curve.
 *    nvertices: the number of vertices
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if the point is inside the polygon.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int is_in_polygon(int x, int y, X_Point vertices[], int nvertices)
{
	int in_poly, this_point, next_point, last_point, line_x;

	in_poly = FALSE;
	for (this_point=0; this_point<nvertices; this_point++)
	{	next_point = this_point+1;
		if (next_point >= nvertices)
			next_point = 0;
		if ((y>=vertices[this_point].y && y<=vertices[next_point].y) ||
				(y>=vertices[next_point].y && y<=vertices[this_point].y))
		{	if (vertices[this_point].y==vertices[next_point].y) /* horizontal*/
			{	if ((x>=vertices[this_point].x && x<=vertices[next_point].x) ||
					  (x<=vertices[this_point].x && x>=vertices[next_point].x))
					return(TRUE);
			}
			else if
			(	x >=
				(	line_x=vertices[this_point].x+(int)
					(	(y-vertices[this_point].y)*
						(vertices[next_point].x-vertices[this_point].x)/
						(double)
						(vertices[next_point].y-vertices[this_point].y)+.5
					)
				)
			)
			{	if (x == line_x)
					return (TRUE);
				if (y == vertices[this_point].y) /* starting point */
				{	for (last_point=this_point; vertices[last_point].y==y;
							last_point=(last_point+nvertices-1)%nvertices)
						if (last_point == (this_point+1)%nvertices)
							return (FALSE);
					if ((vertices[last_point].y>y) !=
							(vertices[next_point].y>y))
						in_poly = !in_poly;
				}
				else if (y != vertices[next_point].y) /* not ending point */
					in_poly = !in_poly;
			}
		}
	}
	return (in_poly);
}
