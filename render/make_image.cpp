/*
  Copyright 1993-2015, 2021 Medical Image Processing Group
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

#define SQRT1_2 (1.414213562373095/2)


extern "C" {
int VComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
	int* npoints );
}

static void freerectlist(struct Rect *rectlist);

static const int image_x=0, image_y=0, image2_x=0, image2_y=0, pixel_bytes=4;
/*****************************************************************************
 * FUNCTION: make_image
 * DESCRIPTION: Combines images of objects into a single image ready to
 *    be displayed, and creates image of a plane if global variable plane,
 *    and puts marks on the image if global variable marks.
 * PARAMETERS:
 *    image: specifies main_image, image2, or icon_image.  Must be created as
 *        with XCreateImage and have sufficient memory allocated at
 *        image->data.
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: The combined image pixel values are stored at image->data.
 *    If image is main_image and the function completes, image_valid will be
 *    set. Any effects of event_priority will occur.
 * ENTRY CONDITIONS: The objects of object_list must have their images
 *    projected as with a successful call to project or be turned off.
 *    If event_priority so requires, the global variable display must describe
 *    a valid connection to an X server.  pixel_bytes must match the depth of
 *    image.  If anti_alias and image is main_image, the objects' image buffer
 *    sizes and positions must be multiples of 2.
 *    object_color_table must be allocated with ncolors rows and properly
 *    initialized.
 *    red_map, green_map, blue_map must be properly initialized.
 *    v_object_color_table must be allocated with one row and properly
 *    initialized if volume rendering.
 * RETURN VALUE: Why the function is returning.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 12/6/91 by Dewey Odhner.
 *    Modified 6/23/92 to use image2 by Dewey Odhner.
 *    Modified 8/7/92 to fix use of unallocated memory by Dewey Odhner.
 *    Modified 11/13/92 to correct the image subdivision algorithm
 *       by Dewey Odhner.
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/27/95 secondary objects counted instead of
 *       separate_piece1, separate_piece2 by Dewey Odhner.
 *    Modified 9/21/01 variable opacity handled by Dewey Odhner.
 *    Modified 1/31/06 colors remapped by Dewey Odhner.
 *
 *****************************************************************************/
Function_status cvRenderer::make_image(XImage *image,
	Priority (*event_priority)(cvRenderer *))
{
	int nobjects, image_size[2], j, need_plane_rect;
	Shell *this_object;
	Virtual_object *v_object, *only_object=NULL;
	Object_image *this_image;
	struct Rect *rectlistB, *rectlistC=NULL,
		recta={{0,0},{0,0},0,0,0,NULL,{{NULL,NULL}}}, *rectb=NULL, only;
	double rotation_matrix[3][3], plane_corners[4][3], normal_vector[3],
		plane_size, *plane_left, *plane_right, *plane_top_left=NULL,
		*plane_top_right=NULL, *plane_bottom_left=NULL, plane_shift;
	Display_mode mode;
	float obj_opacity=0;

/* rectlistB and rectlistC must be disjoint or identical. */
#define rect_alloc(ptr_var) \
{	ptr_var = (struct Rect *) \
		salloc(sizeof(struct Rect)+nobjects*sizeof(struct Rect_object)); \
	if (ptr_var==NULL) \
	{	report_malloc_error(); \
		freerectlist(rectlistB); \
		if (rectlistC != rectlistB) \
			freerectlist(rectlistC); \
		return (EROR); \
	} \
}

#define rect_free(ptr_var) \
{	if (ptr_var != &only) \
		freerectlist(ptr_var); \
}

	mode =
		image==icon_image
		?	ICON
		:	anti_alias
			?	ANTI_ALIAS
			:	PIXEL_REPLICATE;
	switch (mode)
	{	case PIXEL_REPLICATE:
			image_size[0] = image->width/2;
			image_size[1] = image->height/2;
			break;
		case ANTI_ALIAS:
			image_size[0] = image->width*2;
			image_size[1] = image->height*2;
			break;
		case ICON:
			image_size[0] = image->width/4;
			image_size[1] = image->height/4;
			break;
		default:
			assert(0);
	}
	assert((image_size[0]&1) == 0);
	assert((image_size[1]&1) == 0);

	/* Count objects */

	nobjects = 0;
	for (this_object=object_list; this_object!=NULL;
			this_object=this_object->next)
	{	assert(this_object->O.on ||
			this_object->reflection==NULL ||
			!this_object->reflection->on);
		if ((image_mode==SEPARATE &&
				((image==main_image && this_object==separate_piece2) ||
				(image==image2 && this_object!=separate_piece2))) ||
				(image_mode==FUZZY_CONNECT &&
				((image==main_image && this_object->secondary) ||
				(image==image2 && !this_object->secondary))))
			continue;
		for (j=1,v_object= &this_object->O; j<=2;
				j++,v_object=this_object->reflection)
		{	if (v_object == NULL)
				continue;
#ifndef NDEBUG
			this_image =
				mode==ICON
				?	&v_object->icon
				:	&v_object->main_image;
#endif
			if (v_object->on)
			{	assert(v_object->opacity==.5 ||
					this_image->projected!=ANTI_ALIAS ||
					((this_image->image_location[0]&1)==0 &&
					(this_image->image_location[1]&1)==0 &&
					(this_image->image_size&1)==0)
				);
				if (nobjects == 0)
					only_object = v_object;
				nobjects++;
				assert(this_image->projected ==
					(	mode==ANTI_ALIAS&&v_object->opacity==.5
						?	ONE_TO_ONE
						:	mode
					)
				);
			}
		}
	}

	if (event_priority && event_priority(this)==FIRST)
		return (INTERRUPT3);
	if (nobjects==1 && only_object->opacity!=.5)
	{
		obj_opacity = only_object->opacity;
		only_object->opacity = 1;
	}

	/* Build Rect list */
/* The next section of code divides the display space into rectangular regions
 * where in each region there is a set of objects whose images include those
 * pixels.  An outline of the algorithm: (^ means intersection)
 *
 *	Let A be the set of regions of the objects' images.
 *	Set B = {image domain}.
 *	For each region a in A do:
 *		set C = {};
 *		For each region b in B do:
 *			divide b U a into a set of disjoint rectangles F U G U {b ^ a}
 *				such that (U F) ^ a = {} and (U G) ^ b = {};
 *			set C = C U {b ^ a} U F;
 *		endfor;
 *		set B = C;
 *	endfor.
 *	Output B.
 */
	rect_alloc(rectlistB);
	rectlistB->position[0] = -image_size[0]/2;
	rectlistB->position[1] = -image_size[1]/2;
	rectlistB->size[0] = image_size[0];
	rectlistB->size[1] = image_size[1];
	rectlistB->nobjects = rectlistB->nopaque_objects = rectlistB->plane = 0;
	rectlistB->next = NULL;

	if (plane)
	{	int cornern;
		double p[3], q[3], norm;

		AtoM(rotation_matrix, glob_angle[0], glob_angle[1], glob_angle[2]);
		matrix_vector_multiply(normal_vector, rotation_matrix, plane_normal);
		plane_size = image_size[0]*SQRT1_2;
		p[0] = -normal_vector[2];
		p[1] = 0;
		p[2] = normal_vector[0];
		norm = p[0]*p[0]+p[2]*p[2];
		if (norm <= 0)
		{	p[0] = .5*plane_size;
			p[2] = 0;
		}
		else
		{	norm = .5*plane_size/sqrt(norm);
			p[0] *= norm;
			p[2] *= norm;
		}
		q[0] = normal_vector[1]*p[2];
		q[1] = normal_vector[2]*p[0]-normal_vector[0]*p[2];
		q[2] = -normal_vector[1]*p[0];
		/* p & q are orthogonal vectors parallel to the plane.
		   p is horizontal & q points up. */

		if (p[0] > 0)
		{	if (q[0] > 0)
			{	plane_left = plane_corners[2];
				plane_right = plane_corners[1];
			}
			else
			{	plane_left = plane_corners[0];
				plane_right = plane_corners[3];
			}
			plane_top_left = plane_corners[0];
			plane_top_right = plane_corners[1];
			plane_bottom_left = plane_corners[2];
		}
		else
		{	if (q[0] > 0)
			{	plane_left = plane_corners[3];
				plane_right = plane_corners[0];
			}
			else
			{	plane_left = plane_corners[1];
				plane_right = plane_corners[2];
			}
			plane_top_left = plane_corners[1];
			plane_top_right = plane_corners[0];
			plane_bottom_left = plane_corners[3];
		}
		plane_shift = plane_displacement*buffer_scale(mode);
		for (cornern=0; cornern<4; cornern++)
		{	plane_corners[cornern][0] = normal_vector[0]*plane_shift;
			plane_corners[cornern][1] = normal_vector[1]*plane_shift;
			plane_corners[cornern][2] = normal_vector[2]*plane_shift;
			if (cornern < 2)
			{	plane_corners[cornern][0] += q[0];
				plane_corners[cornern][1] += q[1];
				plane_corners[cornern][2] += q[2];
			}
			else
			{	plane_corners[cornern][0] -= q[0];
				plane_corners[cornern][1] -= q[1];
				plane_corners[cornern][2] -= q[2];
			}
			if (cornern & 1)
			{	plane_corners[cornern][0] += p[0];
				plane_corners[cornern][2] += p[2];
			}
			else
			{	plane_corners[cornern][0] -= p[0];
				plane_corners[cornern][2] -= p[2];
			}
			plane_corners[cornern][2] = MIDDLE_DEPTH-
				plane_corners[cornern][2]*depth_scale/buffer_scale(mode);
		}
		recta.position[0] = (short)floor(plane_left[0]);
		recta.position[1] = (short)floor(plane_top_left[1]);
		recta.size[0] = (short)ceil(plane_right[0])-recta.position[0];
		recta.size[1] = (short)ceil(plane_bottom_left[1])-recta.position[1];
		if (mode == ANTI_ALIAS)
		{	if (recta.position[0] & 1)
			{	recta.position[0] -= 1;
				recta.size[0] += 1;
			}
			if (recta.position[1] & 1)
			{	recta.position[1] -= 1;
				recta.size[1] += 1;
			}
			if (recta.size[0] & 1)
				recta.size[0] += 1;
			if (recta.size[1] & 1)
				recta.size[1] += 1;
		}
		recta.nobjects = 0;
		recta.nopaque_objects = 0;
		recta.plane = 1;
		recta.next = NULL;
		if (recta.position[0] < -image_size[0]/2)
		{	recta.size[0] -= -image_size[0]/2-recta.position[0];
			recta.position[0] = -image_size[0]/2;
		}
		if (recta.position[1] < -image_size[1]/2)
		{	recta.size[1] -= -image_size[1]/2-recta.position[1];
			recta.position[1] = -image_size[1]/2;
		}
		if (recta.size[0] > image_size[0]/2-recta.position[0])
			recta.size[0] = image_size[0]/2-recta.position[0];
		if (recta.size[1] > image_size[1]/2-recta.position[1])
			recta.size[1] = image_size[1]/2-recta.position[1];
		need_plane_rect = recta.size[0]>0 && recta.size[1]>0;
	}
	else
		need_plane_rect = FALSE;
	for (this_object=object_list; this_object!=NULL;
			this_object=this_object->next)
	{	if (!this_object->O.on ||
				(image_mode==SEPARATE &&
				((image==main_image && this_object==separate_piece2) ||
				(image==image2 && this_object!=separate_piece2))) ||
				(image_mode==FUZZY_CONNECT &&
				((image==main_image && this_object->secondary) ||
				(image==image2 && !this_object->secondary))))
			continue;
		for (j=1,v_object= &this_object->O; j<=2;
				need_plane_rect
				?	(need_plane_rect=FALSE)
				:	(v_object=this_object->reflection,j++))
		{	if (!need_plane_rect)
			{	if (v_object==NULL || !v_object->on)
					continue;
				this_image =
					mode==ICON
					?	&v_object->icon
					:	&v_object->main_image;
				if (mode==ANTI_ALIAS && v_object->opacity==.5)
					recta.size[0] = recta.size[1] = this_image->image_size*2;
				else
					recta.size[0] = recta.size[1] = this_image->image_size;
				recta.position[0] = this_image->image_location[0];
				recta.position[1] = this_image->image_location[1];
				recta.nobjects = 1;
				recta.nopaque_objects = v_object->opacity!=.5? 1: 0;
				recta.plane = 0;
				recta.next = NULL;
				recta.object[0].vobj = v_object;
				recta.object[0].img = this_image;
				if (recta.position[0] < -image_size[0]/2)
				{	recta.size[0] -= -recta.position[0]-image_size[0]/2;
					recta.position[0] = -image_size[0]/2;
				}
				if (recta.position[1] < -image_size[1]/2)
				{	recta.size[1] -= -recta.position[1]-image_size[1]/2;
					recta.position[1] = -image_size[1]/2;
				}
				if (recta.size[0] > image_size[0]/2-recta.position[0])
					recta.size[0] = image_size[0]/2-recta.position[0];
				if (recta.size[1] > image_size[1]/2-recta.position[1])
					recta.size[1] = image_size[1]/2-recta.position[1];
			}
			rectlistC = NULL;
			while (rectlistB)
			{	struct Rect *newrect;

				rectb = rectlistB;
				if (rectb->position[1]<recta.position[1]+recta.size[1] &&
						recta.position[1]<rectb->position[1]+rectb->size[1] &&
						rectb->position[0]<recta.position[0]+recta.size[0] &&
						recta.position[0]<rectb->position[0]+rectb->size[0])
				{

#define divide_in_1_dimension(I, o) \
			if (rectb->position[I]<recta.position[I]) \
			{	rect_alloc(newrect) \
				newrect->position[o] = rectb->position[o]; \
				newrect->size[o] = rectb->size[o]; \
				newrect->position[I] = rectb->position[I]; \
				newrect->size[I] = recta.position[I]-rectb->position[I]; \
				newrect->nobjects = rectb->nobjects; \
				newrect->nopaque_objects = rectb->nopaque_objects; \
				newrect->plane = rectb->plane; \
				newrect->next = rectlistC; \
				memcpy(newrect->object, rectb->object, \
					rectb->nobjects*sizeof(struct Rect_object)); \
				rectlistC = newrect; \
				rectb->position[I] += newrect->size[I]; \
				rectb->size[I] -= newrect->size[I]; \
			} \
			if (rectb->position[I]+rectb->size[I] > \
					recta.position[I]+recta.size[I]) \
			{	rect_alloc(newrect) \
				newrect->position[o] = rectb->position[o]; \
				newrect->size[o] = rectb->size[o]; \
				newrect->position[I] = recta.position[I]+recta.size[I]; \
				newrect->size[I] = rectb->position[I]+rectb->size[I]- \
					newrect->position[I]; \
				newrect->nobjects = rectb->nobjects; \
				newrect->nopaque_objects = rectb->nopaque_objects; \
				newrect->plane = rectb->plane; \
				newrect->next = rectlistC; \
				memcpy(newrect->object, rectb->object, \
					rectb->nobjects*sizeof(struct Rect_object)); \
				rectlistC = newrect; \
				rectb->size[I] -= newrect->size[I]; \
			} \

					divide_in_1_dimension(1, 0)
					divide_in_1_dimension(0, 1)
					if (recta.nobjects > 0)
					{	rectb->object[rectb->nobjects] =
							rectb->object[rectb->nopaque_objects];
						rectb->object[rectb->nopaque_objects]= recta.object[0];
						rectb->nopaque_objects += recta.nopaque_objects;
						rectb->nobjects += recta.nobjects;
					}
					rectb->plane |= recta.plane;
				}
				rectlistB = rectlistB->next;
				rectb->next = rectlistC;
				rectlistC = rectb;
			}
			rectlistB = rectlistC;
		}
	}

	/* Combine object images */

	for (rectb=rectlistB; rectb!=NULL; rectb=rectb->next)
	{	Function_status result;

		if (event_priority && event_priority(this)==FIRST)
		{	rect_free(rectlistB);
			if (nobjects==1 && only_object->opacity!=.5)
				only_object->opacity = obj_opacity;
			return (INTERRUPT3);
		}
		result =
			mode == ICON
			?	icon(image, rectb, plane_top_left,
					plane_top_right, plane_bottom_left)
			:	mode == PIXEL_REPLICATE
				?	pixel_replicate(image, rectb, plane_top_left,
						plane_top_right, plane_bottom_left)
				:	antialias(image, rectb, plane_top_left,
						plane_top_right, plane_bottom_left, event_priority);
		if (result != DONE)
		{	rect_free(rectlistB);
			if (nobjects==1 && only_object->opacity!=.5)
				only_object->opacity = obj_opacity;
			return (result);
		}
	}
	rect_free(rectlistB);
	if (nobjects==1 && only_object->opacity!=.5)
		only_object->opacity = obj_opacity;
	if (box)
		draw_box(image);
	return (display_marks(image, event_priority));
}


/*****************************************************************************
 * FUNCTION: freerectlist
 * DESCRIPTION: Frees memory occupied by a Rect list.
 * PARAMETERS:
 *    rectlist: The address of the first item in a Rect list.
 * SIDE EFFECTS: Frees memory occupied by a the Rect list.
 * ENTRY CONDITIONS: All Rects must have been allocated by malloc.
 * RETURN VALUE: None.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: February 9, 1990 by Dewey Odhner.
 *
 *****************************************************************************/
static void freerectlist(struct Rect *rectlist)
{
	struct Rect *rect;

	while (rectlist != NULL)
	{	rect = rectlist;
		rectlist = rectlist->next;
		sfree(rect);
	}
}


/*****************************************************************************
 * FUNCTION: pixel_replicate
 * DESCRIPTION: Combines portions of images of objects into a rectangular
 *    region, and creates image of a plane if specified, for make_image in
 *    the case when mode is PIXEL_REPLICATE.
 * PARAMETERS:
 *    image: specifies the output image.  Must be created as
 *        with XCreateImage and have sufficient memory allocated at
 *        image->data.
 *    rect: specifies a rectangular region (in input pixels)
 *        and the objects to be displayed in it.
 *    plane_top_left, plane_top_right, plane_bottom_left: the coordinates
 *        of the plane corners with the first two coordinates in input
 *        pixels and the last in depth units.
 * SIDE EFFECTS: The combined image pixel values are stored at image->data.
 *    maximum_intensity_projection will be cleared if objects are BINARY.
 * ENTRY CONDITIONS: The objects of rect must have their images
 *    projected as with a successful call to project or be turned off,
 *    and the rect must correspond to a valid area in each object's
 *    image buffers.  pixel_bytes must match the depth of image.
 *    object_color_table must be allocated with ncolors rows and properly
 *    initialized.  object_list must be valid.
 *    v_object_color_table must be allocated with one row and properly
 *    initialized if volume rendering.
 * RETURN VALUE: Why the function is returning.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: December 2, 1991 by Dewey Odhner.
 *    Modified: 4/13/94 for volume rendered images by Dewey Odhner.
 *    Modified: 5/16/94 to correct color table use by Dewey Odhner
 *    Modified: 5/27/94 to show multiple PERCENT objects by Dewey Odhner
 *    Modified: 7/11/94 for maximum intensity projection by Dewey Odhner
 *    Modified: 12/12/94 v_object_color_table used with PERCENT objects
 *       by Dewey Odhner
 *    Modified: 9/25/01 to do variable opacity by Dewey Odhner
 *    Modified: 10/17/01 not to do variable opacity for MIP by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::pixel_replicate(XImage *image, struct Rect *rect,
	double plane_top_left[], double plane_top_right[],
	double plane_bottom_left[])
{
	int rown, thingn, **z_buffers, z_value, z_value2, k, i_color, i_color2,
		*plane_z_buffer1=NULL, *plane_z_buffer2=NULL, *plane_row0=NULL,
		*plane_row1=NULL,
		*plane_row2=NULL, pixel_byte, ii_value, ii_value2, plane_width=0,
		out_pixel_bytes, out_bytes_per_line, background_value, *z_order;
	Classification_type object_class;
	char *out_ptr1, *out_ptr2, *row_end;
	unsigned char **i_buffers, **o_buffers;
	Pixel_unit **p_buffers=NULL;
	double dz_dx, dz_dy=0, dx_dy=0;
	Int_bytes i_value;
	struct Rect_object *things;
	RGB pbg={0,0,0};

	object_class = st_cl(&object_list->main_data);
	things = rect->object;
	z_buffers = (int **)salloc(sizeof(int *)*rect->nobjects);
	if (z_buffers == NULL)
	{	report_malloc_error();
		return (EROR);
	}
	z_order = (int *)salloc(sizeof(int)*rect->nobjects);
	if (z_order == NULL)
	{	report_malloc_error();
		sfree(z_buffers);
		return (EROR);
	}
	if (object_class==PERCENT || object_class==DIRECT)
	{	out_pixel_bytes = 3;
		out_bytes_per_line = image->width*3;
		pbg.red = v_object_color_table[0][(int)(background.red*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
		pbg.green = v_object_color_table[0][(int)(background.green*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
		pbg.blue = v_object_color_table[0][(int)(background.blue*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
	}
	else
	{	out_pixel_bytes = pixel_bytes;
		out_bytes_per_line = image->bytes_per_line;
	}
	if (object_class==BINARY_B || object_class==BINARY_A || object_class==T_SHELL)
	{	i_buffers= (unsigned char **)salloc(sizeof(*i_buffers)*rect->nobjects);
		if (i_buffers == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			return (EROR);
		}
		background_value = OBJECT_IMAGE_BACKGROUND;
		maximum_intensity_projection = FALSE;
		o_buffers = NULL;
	}
	else
	{	p_buffers = (Pixel_unit **)salloc(sizeof(*p_buffers)*rect->nobjects);
		if (p_buffers == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			return (EROR);
		}
		i_buffers = (unsigned char **)p_buffers;
		background_value = V_OBJECT_IMAGE_BACKGROUND;
		o_buffers= (unsigned char **)salloc(sizeof(*o_buffers)*rect->nobjects);
		if (o_buffers == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			sfree(i_buffers);
			return (EROR);
		}
	}
	if (rect->plane)
	{	plane_row1 = (int *)salloc(sizeof(int)*2*rect->size[0]);
		if (plane_row1 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			sfree(i_buffers);
			if (o_buffers)
				sfree(o_buffers);
			return (EROR);
		}
		plane_row2 = (int *)salloc(sizeof(int)*2*rect->size[0]);
		if (plane_row2 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			sfree(i_buffers);
			if (o_buffers)
				sfree(o_buffers);
			sfree(plane_row1);
			return (EROR);
		}
		if (plane_top_right[0] > plane_top_left[0])
		{	dz_dx = (plane_top_right[2]-plane_top_left[2])/
				(2*(plane_top_right[0]-plane_top_left[0]));
			plane_width = (int)ceil(2*(plane_top_right[0]-plane_top_left[0]));
		}
		else
		{	dz_dx = 0;
			plane_width = 1;
		}
		plane_row0 = (int *)salloc(sizeof(int)*plane_width);
		if (plane_row0 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(z_order);
			sfree(i_buffers);
			if (o_buffers)
				sfree(o_buffers);
			sfree(plane_row1);
			sfree(plane_row2);
			return (EROR);
		}
		if (plane_bottom_left[1] > plane_top_left[1])
		{	dz_dy = (plane_bottom_left[2]-plane_top_left[2])/
				(2*(plane_bottom_left[1]-plane_top_left[1]));
			dx_dy = (plane_bottom_left[0]-plane_top_left[0])/
				(plane_bottom_left[1]-plane_top_left[1]);
		}
		else
			dz_dy = dx_dy = 0;
		for (k=0; k<plane_width; k++)
			plane_row0[k] = (int)(k*dz_dx);
	}
	for (rown=0; rown<rect->size[1]; rown++)
	{	for (thingn=0; thingn<rect->nobjects; thingn++)
		{	if (object_class!=BINARY_B && object_class!=BINARY_A && object_class!=T_SHELL)
				p_buffers[thingn] = (Pixel_unit *)
					things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					(rect->position[0]-things[thingn].img->image_location[0])*
					things[thingn].img->pixel_units;
			else
				i_buffers[thingn] = (unsigned char *)
					(things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
			if (o_buffers)
				o_buffers[thingn] = (unsigned char *)
					(things[thingn].img->opacity_buffer[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
			z_buffers[thingn] = things[thingn].img->z_buffer[rown+
				rect->position[1]-things[thingn].img->image_location[1]]+
				rect->position[0]-things[thingn].img->image_location[0];
		}

		if (rect->plane)
		{	int left_x1, left_x2, left_z1, left_z2, x;

			left_x1 = (int)rint(2*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+rect->position[1]-plane_top_left[1])));
			left_x2 = (int)rint(2*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+.5+rect->position[1]-plane_top_left[1])));
			left_z1 = (int)rint(plane_top_left[2]+
				dz_dy*(2*(rown+rect->position[1]-plane_top_left[1])));
			left_z2 = (int)rint(plane_top_left[2]+
				dz_dy*(2*(rown+rect->position[1]-plane_top_left[1])+1));
			for (x=0; x<rect->size[0]*2; x++)
			{	k = x-left_x1;
				plane_row1[x] =
					k>=0&&k<plane_width
					?	left_z1+plane_row0[k]
					:	0;
				k = x-left_x2;
				plane_row2[x] =
					k>=0&&k<plane_width
					?	left_z2+plane_row0[k]
					:	0;
			}
		}

		if (rect->plane)
		{	plane_z_buffer1 = plane_row1;
			plane_z_buffer2 = plane_row2;
		}
		out_ptr1 = image->data+
			out_bytes_per_line*
			(2*(rown+rect->position[1])+image->height/2)+
			out_pixel_bytes*(2*rect->position[0]+image->width/2);
		out_ptr2 = out_ptr1+out_bytes_per_line;
		row_end = out_ptr1+2*out_pixel_bytes*rect->size[0];
		switch (object_class)
		{	case BINARY_A:
			case BINARY_B:
			case GRADIENT:
			case T_SHELL:
				if (!rect->plane && rect->nopaque_objects==rect->nobjects)
					for (; out_ptr1<row_end; out_ptr1+=2*out_pixel_bytes,
							out_ptr2+=2*out_pixel_bytes)
					{	ii_value = background_value;
						i_color = z_value = 0;
						if (maximum_intensity_projection)
						{
						  for (thingn=0; thingn<rect->nopaque_objects;thingn++)
						  {	if (ii_value==background_value ||
										(*p_buffers[thingn]!=background_value&&
										*p_buffers[thingn]>ii_value))
							{	i_color = things[thingn].vobj->color;
								ii_value =
									object_class!=GRADIENT
									?	*i_buffers[thingn]
									:	*p_buffers[thingn];
								z_value = *z_buffers[thingn];
							}
							if (object_class!=GRADIENT)
								i_buffers[thingn]++;
							else
								p_buffers[thingn]++;
							z_buffers[thingn]++;
						  }
						}
						else
						{
						  for (thingn=0; thingn<rect->nopaque_objects;thingn++)
						  {
							for (k=thingn; k&&*z_buffers[thingn]>
									*z_buffers[z_order[k-1]]; k--)
								z_order[k] = z_order[k-1];
							z_order[k] = thingn;
						  }
						  for (k=0; k<thingn; k++)
						    if (*z_buffers[z_order[k]] == 0)
							  thingn = k;
							else if (o_buffers? *o_buffers[z_order[k]]==255:
								things[z_order[k]].vobj->opacity >= 1)
							{
							  thingn = k+1;
							  break;
							}
						  for (thingn--; thingn>=0; thingn--)
						  {
							i_color = things[z_order[thingn]].vobj->color;
							ii_value = (int)((ii_value==background_value? 0:
							  (1-(o_buffers? *o_buffers[z_order[thingn]]*
							  (1/255.):things[z_order[thingn]].vobj->opacity))*
							  ii_value)+
							  (o_buffers? *o_buffers[z_order[thingn]]*(1/255.):
							  things[z_order[thingn]].vobj->opacity)*(
								object_class!=GRADIENT
								?	*i_buffers[z_order[thingn]]
								:	*p_buffers[z_order[thingn]]));
							z_value = *z_buffers[z_order[thingn]];
						  }
						  for (thingn=0; thingn<rect->nopaque_objects;thingn++)
						  {
							if (object_class!=GRADIENT)
								i_buffers[z_order[thingn]]++;
							else
								p_buffers[z_order[thingn]]++;
							if (o_buffers)
								o_buffers[z_order[thingn]]++;
							z_buffers[z_order[thingn]]++;
						  }
					}
						i_value =
							object_class!=GRADIENT
							?	object_color_table[i_color][ii_value]
							:	object_color_table[i_color][
									v_object_color_table[0][ii_value]];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[pixel_byte] =
								out_ptr1[out_pixel_bytes+pixel_byte] =
									out_ptr2[pixel_byte] =
										out_ptr2[out_pixel_bytes+pixel_byte] =
											i_value.c[pixel_byte];
					}
				else
					for (; out_ptr1<row_end; out_ptr1+=2*out_pixel_bytes,
							out_ptr2+=2*out_pixel_bytes)
					{	ii_value = background_value;
						i_color = z_value = 0;
						for (thingn=0; thingn<rect->nopaque_objects; thingn++)
						{
							for (k=thingn; k&&*z_buffers[thingn]<
									*z_buffers[z_order[k-1]]; k--)
								z_order[k] = z_order[k-1];
							z_order[k] = thingn;
						}
						for (thingn=0; thingn<rect->nopaque_objects; thingn++)
						{	if (	maximum_intensity_projection
									?	ii_value==background_value ||
										(*p_buffers[z_order[thingn]]!=
										background_value&&
										*p_buffers[z_order[thingn]]>ii_value)
									:	*z_buffers[z_order[thingn]] > z_value)
							{	i_color = things[z_order[thingn]].vobj->color;
								ii_value= (int)((ii_value==background_value? 0:
								  (1-(o_buffers? *o_buffers[z_order[thingn]]*
								  (1/255.):
								  things[z_order[thingn]].vobj->opacity))*
								  ii_value)+(o_buffers?
								  *o_buffers[z_order[thingn]]*(1/255.):
								  things[z_order[thingn]].vobj->opacity)*(
									object_class!=GRADIENT
									?	*i_buffers[z_order[thingn]]
									:	*p_buffers[z_order[thingn]]));
								z_value = *z_buffers[z_order[thingn]];
							}
							if (object_class!=GRADIENT)
								i_buffers[z_order[thingn]]++;
							else
								p_buffers[z_order[thingn]]++;
							if (o_buffers)
								o_buffers[z_order[thingn]]++;
							z_buffers[z_order[thingn]]++;
						}
						ii_value2 = ii_value;
						i_color2 = i_color;
						z_value2 = z_value;
						for (; thingn<rect->nobjects; thingn++)
						{	if (	maximum_intensity_projection
									?	ii_value2==background_value ||
										(*p_buffers[thingn]!=background_value&&
										*p_buffers[thingn]>ii_value2)
									:	*z_buffers[thingn] > z_value2)
							{	i_color2 = things[thingn].vobj->color;
								ii_value2 =
									object_class!=GRADIENT
									?	*i_buffers[thingn]
									:	*p_buffers[thingn];
								z_value2 = *z_buffers[thingn];
							}
							if (object_class!=GRADIENT)
								i_buffers[thingn]++;
							else
								p_buffers[thingn]++;
							if (o_buffers)
								o_buffers[thingn]++;
							z_buffers[thingn]++;
						}
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[0]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer1[0]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[0]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer2[0]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[1]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer1[1]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[1]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer2[1]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						if (rect->plane)
						{	plane_z_buffer1 += 2;
							plane_z_buffer2 += 2;
						}
					}
				break;
			case PERCENT:
			case DIRECT:
				for (; out_ptr1<row_end; out_ptr1+=6, out_ptr2+=6)
				{	int p_value[3], pp_value[3];

					for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					{
						for (k=thingn; k&&*z_buffers[thingn]<
								*z_buffers[z_order[k-1]]; k--)
							z_order[k] = z_order[k-1];
						z_order[k] = thingn;
					}
					z_value = rect->nopaque_objects==0? 0:
						*z_buffers[z_order[rect->nopaque_objects-1]];
					if (z_value == 0)
					{	p_value[0] = pbg.red;
						p_value[1] = pbg.green;
						p_value[2] = pbg.blue;
					}
					else if (maximum_intensity_projection)
					{
						p_value[0] = *p_buffers[z_order[0]];
						p_value[1] = *p_buffers[z_order[0]];
						p_value[2] = *p_buffers[z_order[0]];
						for (thingn=1; thingn<rect->nopaque_objects; thingn++)
						{
							p_value[0] = p_buffers[z_order[thingn]][0];
							p_value[1] = p_buffers[z_order[thingn]][1];
							p_value[2] = p_buffers[z_order[thingn]][2];
						}
						p_value[0] = v_object_color_table[0][p_value[0]];
						p_value[1] = v_object_color_table[0][p_value[1]];
						p_value[2] = v_object_color_table[0][p_value[2]];
					}
					else
					{
						p_value[0] = (int)(*o_buffers[z_order[0]]*
							(1/255.)*p_buffers[z_order[0]][0]);
						p_value[1] = (int)(*o_buffers[z_order[0]]*
							(1/255.)*p_buffers[z_order[0]][1]);
						p_value[2] = (int)(*o_buffers[z_order[0]]*
							(1/255.)*p_buffers[z_order[0]][2]);
						for (thingn=1; thingn<rect->nopaque_objects; thingn++)
						{
							p_value[0] = (int)(
								(1-*o_buffers[z_order[thingn]]*(1/255.))*
								p_value[0]+*o_buffers[z_order[thingn]]*
								(1/255.)*p_buffers[z_order[thingn]][0]);
							p_value[1] = (int)(
								(1-*o_buffers[z_order[thingn]]*(1/255.))*
								p_value[1]+*o_buffers[z_order[thingn]]*
								(1/255.)*p_buffers[z_order[thingn]][1]);
							p_value[2] = (int)(
								(1-*o_buffers[z_order[thingn]]*(1/255.))*
								p_value[2]+*o_buffers[z_order[thingn]]*
								(1/255.)*p_buffers[z_order[thingn]][2]);
						}
						p_value[0] = v_object_color_table[0][p_value[0]];
						p_value[1] = v_object_color_table[0][p_value[1]];
						p_value[2] = v_object_color_table[0][p_value[2]];
					}
					if (rect->plane)
					{	pp_value[0] = (int)(p_value[0]*(1./65535)*
							plane_transparency.red);
						pp_value[1] = (int)(p_value[1]*(1./65535)*
							plane_transparency.green);
						pp_value[2] = (int)(p_value[2]*(1./65535)*
							plane_transparency.blue);
						if (plane_z_buffer1[0] > z_value)
						{	out_ptr1[0] = pp_value[0];
							out_ptr1[1] = pp_value[1];
							out_ptr1[2] = pp_value[2];
						}
						else
						{	out_ptr1[0] = p_value[0];
							out_ptr1[1] = p_value[1];
							out_ptr1[2] = p_value[2];
						}
						if (plane_z_buffer1[1] > z_value)
						{	out_ptr1[3] = pp_value[0];
							out_ptr1[4] = pp_value[1];
							out_ptr1[5] = pp_value[2];
						}
						else
						{	out_ptr1[3] = p_value[0];
							out_ptr1[4] = p_value[1];
							out_ptr1[5] = p_value[2];
						}
						if (plane_z_buffer2[0] > z_value)
						{	out_ptr2[0] = pp_value[0];
							out_ptr2[1] = pp_value[1];
							out_ptr2[2] = pp_value[2];
						}
						else
						{	out_ptr2[0] = p_value[0];
							out_ptr2[1] = p_value[1];
							out_ptr2[2] = p_value[2];
						}
						if (plane_z_buffer2[1] > z_value)
						{	out_ptr2[3] = pp_value[0];
							out_ptr2[4] = pp_value[1];
							out_ptr2[5] = pp_value[2];
						}
						else
						{	out_ptr2[3] = p_value[0];
							out_ptr2[4] = p_value[1];
							out_ptr2[5] = p_value[2];
						}
						plane_z_buffer1 += 2;
						plane_z_buffer2 += 2;
					}
					else
						for (pixel_byte=0; pixel_byte<3; pixel_byte++)
						{	out_ptr1[pixel_byte] = out_ptr2[3+pixel_byte] =
								out_ptr1[3+pixel_byte] = out_ptr2[pixel_byte] =
								p_value[pixel_byte];
						}
					for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					{	p_buffers[thingn] += 3;
						z_buffers[thingn]++;
						o_buffers[thingn]++;
					}
				}
				break;
		}
	}
	sfree(z_buffers);
	sfree(z_order);
	sfree(i_buffers);
	if (o_buffers)
		sfree(o_buffers);
	if (rect->plane)
	{	sfree(plane_row0);
		sfree(plane_row1);
		sfree(plane_row2);
	}
	return (DONE);
}


/*****************************************************************************
 * FUNCTION: icon
 * DESCRIPTION: Combines portions of images of objects into a rectangular
 *    region, and creates image of a plane if specified, for make_image in
 *    the case when mode is ICON.
 * PARAMETERS:
 *    image: specifies the output image.  Must be created as
 *        with XCreateImage and have sufficient memory allocated at
 *        image->data.
 *    rect: specifies a rectangular region (in input pixels)
 *        and the objects to be displayed in it.
 *    plane_top_left, plane_top_right, plane_bottom_left: the coordinates
 *        of the plane corners with the first two coordinates in input
 *        pixels and the last in depth units.
 * SIDE EFFECTS: The combined image pixel values are stored at image->data.
 *    maximum_intensity_projection will be cleared if objects are BINARY.
 * ENTRY CONDITIONS: The objects of rect must have their icon images
 *    projected as with a successful call to project or be turned off,
 *    and the rect must correspond to a valid area in each object's icon
 *    image buffers.  pixel_bytes must match the depth of image.
 *    object_color_table must be allocated with ncolors rows and properly
 *    initialized.  object_list must be valid.
 *    v_object_color_table must be allocated with one row and properly
 *    initialized if volume rendering.
 * RETURN VALUE: Why the function is returning.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: December 2, 1991 by Dewey Odhner.
 *    Modified: 4/13/94 for volume rendered images by Dewey Odhner.
 *    Modified: 5/16/94 to correct color table use by Dewey Odhner
 *    Modified: 6/8/94 to show multiple PERCENT objects by Dewey Odhner
 *    Modified: 7/11/94 for maximum intensity projection by Dewey Odhner
 *    Modified: 12/12/94 v_object_color_table used with PERCENT objects
 *       by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::icon(XImage *image, struct Rect *rect,
	double plane_top_left[],
	double plane_top_right[], double plane_bottom_left[])
{
	int rown, thingn, **z_buffers, z_value, z_value2, k, *plane_z_buffer1=NULL,
		*plane_z_buffer2=NULL, *plane_z_buffer3=NULL, *plane_z_buffer4=NULL,
		*plane_row0=NULL,
		*plane_row1=NULL, *plane_row2=NULL, *plane_row3=NULL, *plane_row4=NULL,
		pixel_byte, i_color, i_color2, ii_value, ii_value2, plane_width=0,
		out_pixel_bytes, out_bytes_per_line, background_value;
	Classification_type object_class;
	char *row_end;
	unsigned char **i_buffers;
	Pixel_unit **p_buffers;
	char *out_ptr1, *out_ptr2, *out_ptr3, *out_ptr4;
	double dz_dx, dz_dy=0, dx_dy=0;
	Int_bytes i_value;
	struct Rect_object *things;
	RGB pbg={0,0,0};

	object_class = st_cl(&object_list->main_data);
	things = rect->object;
	z_buffers = (int **)salloc(sizeof(int *)*rect->nobjects);
	if (z_buffers == NULL)
	{	report_malloc_error();
		return (EROR);
	}
	if (object_class==PERCENT || object_class==DIRECT)
	{	out_pixel_bytes = 3;
		out_bytes_per_line = image->width*3;
		pbg.red = v_object_color_table[0][(int)(background.red*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
		pbg.green = v_object_color_table[0][(int)(background.green*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
		pbg.blue = v_object_color_table[0][(int)(background.blue*
			((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
	}
	else
	{	out_pixel_bytes = pixel_bytes;
		out_bytes_per_line = image->bytes_per_line;
	}
	if (object_class==BINARY_B || object_class==BINARY_A || object_class==T_SHELL)
	{	i_buffers= (unsigned char **)salloc(sizeof(*i_buffers)*rect->nobjects);
		if (i_buffers == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			return (EROR);
		}
		p_buffers = (Pixel_unit **)i_buffers;
		background_value = OBJECT_IMAGE_BACKGROUND;
		maximum_intensity_projection = FALSE;
	}
	else
	{	p_buffers = (Pixel_unit **)salloc(sizeof(*p_buffers)*rect->nobjects);
		if (p_buffers == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			return (EROR);
		}
		i_buffers = (unsigned char **)p_buffers;
		background_value = V_OBJECT_IMAGE_BACKGROUND;
	}
	if (rect->plane)
	{	plane_row1 = (int *)salloc(sizeof(int)*4*rect->size[0]);
		if (plane_row1 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(i_buffers);
			return (EROR);
		}
		plane_row2 = (int *)salloc(sizeof(int)*4*rect->size[0]);
		if (plane_row2 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(i_buffers);
			sfree(plane_row1);
			return (EROR);
		}
		plane_row3 = (int *)salloc(sizeof(int)*4*rect->size[0]);
		if (plane_row3 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(i_buffers);
			sfree(plane_row1);
			sfree(plane_row2);
			return (EROR);
		}
		plane_row4 = (int *)salloc(sizeof(int)*4*rect->size[0]);
		if (plane_row4 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(i_buffers);
			sfree(plane_row1);
			sfree(plane_row2);
			sfree(plane_row3);
			return (EROR);
		}
		if (plane_top_right[0] > plane_top_left[0])
		{	dz_dx = (plane_top_right[2]-plane_top_left[2])/
				(4*(plane_top_right[0]-plane_top_left[0]));
			plane_width = (int)ceil(4*(plane_top_right[0]-plane_top_left[0]));
		}
		else
		{	dz_dx = 0;
			plane_width = 1;
		}
		plane_row0 = (int *)salloc(sizeof(int)*plane_width);
		if (plane_row0 == NULL)
		{	report_malloc_error();
			sfree(z_buffers);
			sfree(i_buffers);
			sfree(plane_row1);
			sfree(plane_row2);
			sfree(plane_row3);
			sfree(plane_row4);
			return (EROR);
		}
		if (plane_bottom_left[1] > plane_top_left[1])
		{	dz_dy = (plane_bottom_left[2]-plane_top_left[2])/
				(4*(plane_bottom_left[1]-plane_top_left[1]));
			dx_dy = (plane_bottom_left[0]-plane_top_left[0])/
				(plane_bottom_left[1]-plane_top_left[1]);
		}
		else
			dz_dy = dx_dy = 0;
		for (k=0; k<plane_width; k++)
			plane_row0[k] = (int)(k*dz_dx);
	}
	for (rown=0; rown<rect->size[1]; rown++)
	{	for (thingn=0; thingn<rect->nobjects; thingn++)
		{	if (object_class!=BINARY_B && object_class!=BINARY_A && object_class!=T_SHELL)
				p_buffers[thingn] = (Pixel_unit *)
					things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					(rect->position[0]-things[thingn].img->image_location[0])*
					things[thingn].img->pixel_units;
			else
				i_buffers[thingn] = (unsigned char *)
					(things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
			z_buffers[thingn] = things[thingn].img->z_buffer[rown+
				rect->position[1]-things[thingn].img->image_location[1]]+
				rect->position[0]-things[thingn].img->image_location[0];
		}

		if (rect->plane)
		{	int left_x1, left_x2, left_x3, left_x4, left_z1, left_z2,
				left_z3, left_z4, x;

			left_x1 = (int)rint(4*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+rect->position[1]-plane_top_left[1])));
			left_x2 = (int)rint(4*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+.25+rect->position[1]-plane_top_left[1])));
			left_x3 = (int)rint(4*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+.5+rect->position[1]-plane_top_left[1])));
			left_x4 = (int)rint(4*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+.75+rect->position[1]-plane_top_left[1])));
			left_z1 = (int)rint(plane_top_left[2]+
				dz_dy*(4*(rown+rect->position[1]-plane_top_left[1])));
			left_z2 = (int)rint(plane_top_left[2]+
				dz_dy*(4*(rown+rect->position[1]-plane_top_left[1])+1));
			left_z3 = (int)rint(plane_top_left[2]+
				dz_dy*(4*(rown+rect->position[1]-plane_top_left[1])+2));
			left_z4 = (int)rint(plane_top_left[2]+
				dz_dy*(4*(rown+rect->position[1]-plane_top_left[1])+3));
			for (x=0; x<rect->size[0]*4; x++)
			{	k = x-left_x1;
				plane_row1[x] =
					k>=0&&k<plane_width
					?	left_z1+plane_row0[k]
					:	0;
				k = x-left_x2;
				plane_row2[x] =
					k>=0&&k<plane_width
					?	left_z2+plane_row0[k]
					:	0;
				k = x-left_x3;
				plane_row3[x] =
					k>=0&&k<plane_width
					?	left_z3+plane_row0[k]
					:	0;
				k = x-left_x4;
				plane_row4[x] =
					k>=0&&k<plane_width
					?	left_z4+plane_row0[k]
					:	0;
			}
			plane_z_buffer1 = plane_row1;
			plane_z_buffer2 = plane_row2;
			plane_z_buffer3 = plane_row3;
			plane_z_buffer4 = plane_row4;
		}

		out_ptr1 = image->data+
			out_bytes_per_line*
			(4*(rown+rect->position[1])+image->height/2)+
			out_pixel_bytes*(4*rect->position[0]+image->width/2);
		out_ptr2 = out_ptr1+out_bytes_per_line;
		out_ptr3 = out_ptr2+out_bytes_per_line;
		out_ptr4 = out_ptr3+out_bytes_per_line;
		row_end = out_ptr1+4*out_pixel_bytes*rect->size[0];
		switch (object_class)
		{	case BINARY_A:
			case BINARY_B:
			case GRADIENT:
			case T_SHELL:
				if (rect->nopaque_objects==1 && rect->nobjects==1 &&
						out_pixel_bytes==1 && !rect->plane)
				 /* for speed */
				{	unsigned char *i_buffer;
					Pixel_unit *p_buffer;

					i_color = things[0].vobj->color;
					i_buffer = i_buffers[0];
					p_buffer = p_buffers[0];
					for (; out_ptr1<row_end;
							out_ptr1+=4, out_ptr2+=4, out_ptr3+=4, out_ptr4+=4)
					{	out_ptr1[0] = out_ptr1[1] = out_ptr1[2] = out_ptr1[3] =
						out_ptr2[0] = out_ptr2[1] = out_ptr2[2] = out_ptr2[3] =
						out_ptr3[0] = out_ptr3[1] = out_ptr3[2] = out_ptr3[3] =
						out_ptr4[0] = out_ptr4[1] = out_ptr4[2] = out_ptr4[3] =
							object_class!=GRADIENT
							?	object_color_table[i_color][*i_buffer++].c[0]
							:	object_color_table[i_color][
									v_object_color_table[0][*p_buffer++]].c[0];
					}
				}
				else
					for (; out_ptr1<row_end;
							out_ptr1+=4*out_pixel_bytes,
							out_ptr2+=4*out_pixel_bytes,
							out_ptr3+=4*out_pixel_bytes,
							out_ptr4+=4*out_pixel_bytes)
					{	ii_value = background_value;
						i_color = z_value = 0;
						for (thingn=0; thingn<rect->nopaque_objects; thingn++)
						{	if (	maximum_intensity_projection
									?	ii_value==background_value ||
										(*p_buffers[thingn]!=background_value&&
										*p_buffers[thingn]>ii_value)
									:	*z_buffers[thingn] > z_value)
							{	i_color = things[thingn].vobj->color;
								ii_value =
									object_class!=GRADIENT
									?	*i_buffers[thingn]
									:	*p_buffers[thingn];
								z_value = *z_buffers[thingn];
							}
							if (object_class!=GRADIENT)
								i_buffers[thingn]++;
							else
								p_buffers[thingn]++;
							z_buffers[thingn]++;
						}
						ii_value2 = ii_value;
						i_color2 = i_color;
						z_value2 = z_value;
						for (; thingn<rect->nobjects; thingn++)
						{	if (	maximum_intensity_projection
									?	ii_value2==background_value ||
										(*p_buffers[thingn]!=background_value&&
										*p_buffers[thingn]>ii_value2)
									:	*z_buffers[thingn] > z_value2)
							{	i_color2 = things[thingn].vobj->color;
								ii_value2 =
									object_class!=GRADIENT
									?	*i_buffers[thingn]
									:	*p_buffers[thingn];
								z_value2 = *z_buffers[thingn];
							}
							if (object_class!=GRADIENT)
								i_buffers[thingn]++;
							else
								p_buffers[thingn]++;
							z_buffers[thingn]++;
						}
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[0]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer1[0]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[0]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer2[0]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer3[0]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer3[0]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr3[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer4[0]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer4[0]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr4[pixel_byte] = i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[1]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer1[1]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[1]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer2[1]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer3[1]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer3[1]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr3[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer4[1]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer4[1]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr4[out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[2]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer1[2]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[2*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[2]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer2[2]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[2*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer3[2]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer3[2]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr3[2*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer4[2]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer4[2]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr4[2*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer1[3]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer1[3]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr1[3*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer2[3]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer2[3]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr2[3*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer3[3]>z_value
								?	ii_value+PLANE_INDEX_OFFSET
								:	ii_value
							:	v_object_color_table[0][ii_value]+(
								rect->plane && plane_z_buffer3[3]>z_value
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr3[3*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						i_value = object_color_table[i_color2][
							object_class!=GRADIENT
							?	rect->plane && plane_z_buffer4[3]>z_value2
								?	ii_value2+PLANE_INDEX_OFFSET
								:	ii_value2
							:	v_object_color_table[0][ii_value2]+(
								rect->plane && plane_z_buffer4[3]>z_value2
								?	PLANE_INDEX_OFFSET
								:	0)];
						for (pixel_byte=0; pixel_byte<out_pixel_bytes;
								pixel_byte++)
							out_ptr4[3*out_pixel_bytes+pixel_byte] =
								i_value.c[pixel_byte];
						if (rect->plane)
						{	plane_z_buffer1 += 4;
							plane_z_buffer2 += 4;
							plane_z_buffer3 += 4;
							plane_z_buffer4 += 4;
						}
					}
				break;
			case PERCENT:
			case DIRECT:
				for (; out_ptr1<row_end;
						out_ptr1+=12, out_ptr2+=12, out_ptr3+=12, out_ptr4+=12)
				{	int p_value[3], pp_value[3]={0,0,0}, p_value2[3],
						pp_value2[3], thing1, thing2;

					z_value = 0;
					thing1 = -1;
					for (thingn=0; thingn<rect->nopaque_objects; thingn++)
						if (*z_buffers[thingn] > z_value)
						{	thing1 = thingn;
							z_value = *z_buffers[thingn];
						}
					if (thing1 < 0)
					{	p_value[0] = pbg.red;
						p_value[1] = pbg.green;
						p_value[2] = pbg.blue;
						if (rect->plane)
						{	pp_value[0] = (int)(p_value[0]*(1./65535)*
								plane_transparency.red);
							pp_value[1] = (int)(p_value[1]*(1./65535)*
								plane_transparency.green);
							pp_value[2] = (int)(p_value[2]*(1./65535)*
								plane_transparency.blue);
						}
					}
					else
					{	p_value[0] =
							v_object_color_table[0][p_buffers[thing1][0]];
						p_value[1] =
								v_object_color_table[0][p_buffers[thing1][1]];
						p_value[2] =
								v_object_color_table[0][p_buffers[thing1][2]];
						if (rect->plane)
						{	pp_value[0] = (int)(p_value[0]*(1./65535)*
								plane_transparency.red);
							pp_value[1] = (int)(p_value[1]*(1./65535)*
								plane_transparency.green);
							pp_value[2] = (int)(p_value[2]*(1./65535)*
								plane_transparency.blue);
						}
					}
					z_value2 = z_value;
					thing2 = thing1;
					for (; thingn<rect->nobjects; thingn++)
						if (*z_buffers[thingn] > z_value2)
						{	thing2 = thingn;
							z_value2 = *z_buffers[thingn];
						}
					if (thing2 == thing1)
					{	p_value2[0] = p_value[0];
						p_value2[1] = p_value[1];
						p_value2[2] = p_value[2];
						if (rect->plane)
						{	pp_value2[0] = pp_value[0];
							pp_value2[1] = pp_value[1];
							pp_value2[2] = pp_value[2];
						}
					}
					else
					{	p_value2[0] =
							v_object_color_table[0][p_buffers[thing2][0]];
						p_value2[1] =
							v_object_color_table[0][p_buffers[thing2][1]];
						p_value2[2] =
							v_object_color_table[0][p_buffers[thing2][2]];
						if (rect->plane)
						{	pp_value2[0] = (int)(p_value2[0]*(1./65535)*
								plane_transparency.red);
							pp_value2[1] = (int)(p_value2[1]*(1./65535)*
								plane_transparency.green);
							pp_value2[2] = (int)(p_value2[2]*(1./65535)*
								plane_transparency.blue);
						}
					}
					if (rect->plane)
					{	if (plane_z_buffer1[0] > z_value2)
						{	out_ptr1[0] = pp_value2[0];
							out_ptr1[1] = pp_value2[1];
							out_ptr1[2] = pp_value2[2];
						}
						else
						{	out_ptr1[0] = p_value2[0];
							out_ptr1[1] = p_value2[1];
							out_ptr1[2] = p_value2[2];
						}
						if (plane_z_buffer1[1] > z_value)
						{	out_ptr1[3] = pp_value[0];
							out_ptr1[4] = pp_value[1];
							out_ptr1[5] = pp_value[2];
						}
						else
						{	out_ptr1[3] = p_value[0];
							out_ptr1[4] = p_value[1];
							out_ptr1[5] = p_value[2];
						}
						if (plane_z_buffer1[2] > z_value2)
						{	out_ptr1[6] = pp_value2[0];
							out_ptr1[7] = pp_value2[1];
							out_ptr1[8] = pp_value2[2];
						}
						else
						{	out_ptr1[6] = p_value2[0];
							out_ptr1[7] = p_value2[1];
							out_ptr1[8] = p_value2[2];
						}
						if (plane_z_buffer1[3] > z_value)
						{	out_ptr1[9] = pp_value[0];
							out_ptr1[10] = pp_value[1];
							out_ptr1[11] = pp_value[2];
						}
						else
						{	out_ptr1[9] = p_value[0];
							out_ptr1[10] = p_value[1];
							out_ptr1[11] = p_value[2];
						}
						if (plane_z_buffer2[0] > z_value)
						{	out_ptr2[0] = pp_value[0];
							out_ptr2[1] = pp_value[1];
							out_ptr2[2] = pp_value[2];
						}
						else
						{	out_ptr2[0] = p_value[0];
							out_ptr2[1] = p_value[1];
							out_ptr2[2] = p_value[2];
						}
						if (plane_z_buffer2[1] > z_value2)
						{	out_ptr2[3] = pp_value2[0];
							out_ptr2[4] = pp_value2[1];
							out_ptr2[5] = pp_value2[2];
						}
						else
						{	out_ptr2[3] = p_value2[0];
							out_ptr2[4] = p_value2[1];
							out_ptr2[5] = p_value2[2];
						}
						if (plane_z_buffer2[2] > z_value)
						{	out_ptr2[6] = pp_value[0];
							out_ptr2[7] = pp_value[1];
							out_ptr2[8] = pp_value[2];
						}
						else
						{	out_ptr2[6] = p_value[0];
							out_ptr2[7] = p_value[1];
							out_ptr2[8] = p_value[2];
						}
						if (plane_z_buffer2[3] > z_value2)
						{	out_ptr2[9] = pp_value2[0];
							out_ptr2[10] = pp_value2[1];
							out_ptr2[11] = pp_value2[2];
						}
						else
						{	out_ptr2[9] = p_value2[0];
							out_ptr2[10] = p_value2[1];
							out_ptr2[11] = p_value2[2];
						}
						if (plane_z_buffer3[0] > z_value2)
						{	out_ptr3[0] = pp_value2[0];
							out_ptr3[1] = pp_value2[1];
							out_ptr3[2] = pp_value2[2];
						}
						else
						{	out_ptr3[0] = p_value2[0];
							out_ptr3[1] = p_value2[1];
							out_ptr3[2] = p_value2[2];
						}
						if (plane_z_buffer3[1] > z_value)
						{	out_ptr3[3] = pp_value[0];
							out_ptr3[4] = pp_value[1];
							out_ptr3[5] = pp_value[2];
						}
						else
						{	out_ptr3[3] = p_value[0];
							out_ptr3[4] = p_value[1];
							out_ptr3[5] = p_value[2];
						}
						if (plane_z_buffer3[2] > z_value2)
						{	out_ptr3[6] = pp_value2[0];
							out_ptr3[7] = pp_value2[1];
							out_ptr3[8] = pp_value2[2];
						}
						else
						{	out_ptr3[6] = p_value2[0];
							out_ptr3[7] = p_value2[1];
							out_ptr3[8] = p_value2[2];
						}
						if (plane_z_buffer3[3] > z_value)
						{	out_ptr3[9] = pp_value[0];
							out_ptr3[10] = pp_value[1];
							out_ptr3[11] = pp_value[2];
						}
						else
						{	out_ptr3[9] = p_value[0];
							out_ptr3[10] = p_value[1];
							out_ptr3[11] = p_value[2];
						}
						if (plane_z_buffer4[0] > z_value)
						{	out_ptr4[0] = pp_value[0];
							out_ptr4[1] = pp_value[1];
							out_ptr4[2] = pp_value[2];
						}
						else
						{	out_ptr4[0] = p_value[0];
							out_ptr4[1] = p_value[1];
							out_ptr4[2] = p_value[2];
						}
						if (plane_z_buffer4[1] > z_value2)
						{	out_ptr4[3] = pp_value2[0];
							out_ptr4[4] = pp_value2[1];
							out_ptr4[5] = pp_value2[2];
						}
						else
						{	out_ptr4[3] = p_value2[0];
							out_ptr4[4] = p_value2[1];
							out_ptr4[5] = p_value2[2];
						}
						if (plane_z_buffer4[2] > z_value)
						{	out_ptr4[6] = pp_value[0];
							out_ptr4[7] = pp_value[1];
							out_ptr4[8] = pp_value[2];
						}
						else
						{	out_ptr4[6] = p_value[0];
							out_ptr4[7] = p_value[1];
							out_ptr4[8] = p_value[2];
						}
						if (plane_z_buffer4[3] > z_value2)
						{	out_ptr4[9] = pp_value2[0];
							out_ptr4[10] = pp_value2[1];
							out_ptr4[11] = pp_value2[2];
						}
						else
						{	out_ptr4[9] = p_value2[0];
							out_ptr4[10] = p_value2[1];
							out_ptr4[11] = p_value2[2];
						}
						plane_z_buffer1 += 4;
						plane_z_buffer2 += 4;
						plane_z_buffer3 += 4;
						plane_z_buffer4 += 4;
					}
					else
						for (pixel_byte=0; pixel_byte<3; pixel_byte++)
						{	out_ptr1[pixel_byte] = out_ptr1[6+pixel_byte] =
							out_ptr2[3+pixel_byte] = out_ptr2[9+pixel_byte] =
							out_ptr3[pixel_byte] = out_ptr3[6+pixel_byte] =
							out_ptr4[3+pixel_byte] = out_ptr4[9+pixel_byte] =
								p_value2[pixel_byte];
							out_ptr1[3+pixel_byte] = out_ptr1[9+pixel_byte] =
							out_ptr2[pixel_byte] = out_ptr2[6+pixel_byte] =
							out_ptr3[3+pixel_byte] = out_ptr3[9+pixel_byte] =
							out_ptr4[pixel_byte] = out_ptr4[6+pixel_byte] =
								p_value[pixel_byte];
						}
					for (thingn=0; thingn<rect->nobjects; thingn++)
					{	p_buffers[thingn] += 3;
						z_buffers[thingn]++;
					}
				}
				break;
		}
	}
	sfree(z_buffers);
	sfree(i_buffers);
	if (rect->plane)
	{	sfree(plane_row0);
		sfree(plane_row1);
		sfree(plane_row2);
		sfree(plane_row3);
		sfree(plane_row4);
	}
	return (DONE);
}


/*****************************************************************************
 * FUNCTION: antialias
 * DESCRIPTION: Combines portions of images of objects into a rectangular
 *    region, and creates image of a plane if specified, for make_image in
 *    the case when mode is ANTI_ALIAS.
 * PARAMETERS:
 *    image: specifies the output image.  Must be created as with
 *        XCreateImage and have sufficient memory allocated at image->data.
 *    rect: specifies a rectangular region (in input pixels)
 *        and the objects to be displayed in it.
 *    plane_top_left, plane_top_right, plane_bottom_left: the coordinates
 *        of the plane corners with the first two coordinates in input
 *        pixels and the last in depth units.
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: The combined image pixel values are stored at image->data.
 *    maximum_intensity_projection will be cleared if objects are BINARY.
 *    Any effects of event_priority will occur.
 * ENTRY CONDITIONS: The objects of rect must have their images
 *    projected as with a successful call to project or be turned off,
 *    and the rect must correspond to a valid area in each object's
 *    image buffers.  pixel_bytes must match the depth of image.
 *    object_color_table must be allocated with ncolors rows and properly
 *    initialized.  Input image sizes and positions must be multiples of two.
 *    object_list must be valid.
 *    v_object_color_table must be allocated with one row and properly
 *    initialized if volume rendering.
 * RETURN VALUE: Why the function is returning.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: December 2, 1991 by Dewey Odhner.
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 4/14/94 for volume rendered images by Dewey Odhner.
 *    Modified: 5/16/94 to correct color table use by Dewey Odhner
 *    Modified: 6/8/94 to show multiple PERCENT objects by Dewey Odhner
 *    Modified: 7/11/94 for maximum intensity projection by Dewey Odhner
 *    Modified: 12/12/94 v_object_color_table used with PERCENT objects
 *       by Dewey Odhner
 *    Modified: 9/25/01 to do variable opacity by Dewey Odhner
 *    Modified: 10/17/01 not to do variable opacity for MIP by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::antialias(XImage *image, struct Rect *rect,
	double plane_top_left[], double plane_top_right[],
	double plane_bottom_left[], Priority (*event_priority)(cvRenderer *))
{
	int rown, thingn, **z_buffers1, **z_buffers2, z_value1, z_value2,
		z_value3, z_value4, z_value5, *plane_z_buffer=NULL, *plane_row=NULL,
		pixel_byte, ii_value1, ii_value2, ii_value3, ii_value4, ii_value5,
		ii_values, color1, color2, color3, color4, color5, parity,
		*plane_row0=NULL, k, plane_width=0,
		ic_value[4][3]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
		out_pixel_bytes, out_bytes_per_line, background_value, *z_order;
	Classification_type object_class;
	char *out_ptr, *row_end;
	unsigned char **i_buffers1, **i_buffers2, **o_buffers1, **o_buffers2;
	Pixel_unit **p_buffers1=NULL, **p_buffers2=NULL;
	double dz_dx, dz_dy=0, dx_dy=0;
	Int_bytes i_value;
	struct Rect_object *things;
	float fplane_transparency[3]={0, 0, 0};
	RGB pbg={0, 0, 0};

	object_class = st_cl(&object_list->main_data);
	things = rect->object;
	z_buffers1 = (int **)salloc(sizeof(int *)*rect->nobjects);
	if (z_buffers1 == NULL)
	{	report_malloc_error();
		return (EROR);
	}
	z_buffers2 = (int **)salloc(sizeof(int *)*rect->nopaque_objects);
	if (z_buffers2 == NULL)
	{	sfree(z_buffers1);
		report_malloc_error();
		return (EROR);
	}
	z_order = (int *)salloc(sizeof(int)*rect->nobjects);
	if (z_order == NULL)
	{	report_malloc_error();
		sfree(z_buffers1);
		sfree(z_buffers2);
		return (EROR);
	}
	if (object_class==PERCENT || object_class==DIRECT)
	{	out_pixel_bytes = 3;
		out_bytes_per_line = image->width*3;
		pbg.red = (unsigned short)
			(background.red*((V_OBJECT_IMAGE_BACKGROUND-1)/65535.));
		pbg.green = (unsigned short)
			(background.green*((V_OBJECT_IMAGE_BACKGROUND-1)/65535.));
		pbg.blue = (unsigned short)
			(background.blue*((V_OBJECT_IMAGE_BACKGROUND-1)/65535.));
		fplane_transparency[0] = (float)
			(v_object_color_table[0][plane_transparency.red]*(1./65535));
		fplane_transparency[1] = (float)
			(v_object_color_table[0][plane_transparency.green]*(1./65535));
		fplane_transparency[2] = (float)
			(v_object_color_table[0][plane_transparency.blue]*(1./65535));
	}
	else
	{	out_pixel_bytes = pixel_bytes;
		out_bytes_per_line = image->bytes_per_line;
	}
	if (object_class==BINARY_B || object_class==BINARY_A || object_class==T_SHELL)
	{	i_buffers1 =
			(unsigned char **)salloc(sizeof(*i_buffers1)*rect->nobjects);
		if (i_buffers1 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			report_malloc_error();
			return (EROR);
		}
		i_buffers2 = (unsigned char **)
			salloc(sizeof(*i_buffers2)*rect->nopaque_objects);
		if (i_buffers2 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(i_buffers1);
			report_malloc_error();
			return (EROR);
		}
		background_value = OBJECT_IMAGE_BACKGROUND;
		assert(out_pixel_bytes == 4);
		maximum_intensity_projection = FALSE;
		o_buffers1 = o_buffers2 = NULL;
	}
	else
	{	p_buffers1 = (Pixel_unit **)salloc(sizeof(*p_buffers1)*rect->nobjects);
		if (p_buffers1 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			report_malloc_error();
			return (EROR);
		}
		p_buffers2 =
			(Pixel_unit **)salloc(sizeof(*p_buffers2)*rect->nopaque_objects);
		if (p_buffers2 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(p_buffers1);
			report_malloc_error();
			return (EROR);
		}
		i_buffers1 = (unsigned char **)p_buffers1;
		i_buffers2 = (unsigned char **)p_buffers2;
		background_value = V_OBJECT_IMAGE_BACKGROUND;
		o_buffers1 = (unsigned char **)
			salloc(sizeof(*o_buffers1)*rect->nobjects);
		if (o_buffers1 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(p_buffers1);
			sfree(p_buffers2);
			report_malloc_error();
			return (EROR);
		}
		o_buffers2 = (unsigned char **)
			salloc(sizeof(*o_buffers2)*rect->nobjects);
		if (o_buffers2 == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(p_buffers1);
			sfree(p_buffers2);
			sfree(o_buffers1);
			report_malloc_error();
			return (EROR);
		}
	}
	if (rect->plane)
	{	plane_row = (int *)salloc(sizeof(int)*rect->size[0]/2);
		if (plane_row == NULL)
		{	sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(i_buffers1);
			sfree(i_buffers2);
			if (o_buffers1)
			{
				sfree(o_buffers1);
				sfree(o_buffers2);
			}
			report_malloc_error();
			return (EROR);
		}
		if (plane_top_right[0] > plane_top_left[0])
		{	dz_dx = (plane_top_right[2]-plane_top_left[2])/
				(.5*(plane_top_right[0]-plane_top_left[0]));
			plane_width = (int)ceil(.5*(plane_top_right[0]-plane_top_left[0]));
		}
		else
		{	dz_dx = 0;
			plane_width = 1;
		}
		plane_row0 = (int *)salloc(sizeof(int)*plane_width);
		if (plane_row0 == NULL)
		{	report_malloc_error();
			sfree(z_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(i_buffers1);
			sfree(i_buffers2);
			if (o_buffers1)
			{
				sfree(o_buffers1);
				sfree(o_buffers2);
			}
			sfree(plane_row);
			return (EROR);
		}
		if (plane_bottom_left[1] > plane_top_left[1])
		{	dz_dy = (plane_bottom_left[2]-plane_top_left[2])/
				(.5*(plane_bottom_left[1]-plane_top_left[1]));
			dx_dy = (plane_bottom_left[0]-plane_top_left[0])/
				(plane_bottom_left[1]-plane_top_left[1]);
		}
		else
			dz_dy = dx_dy = 0;
		for (k=0; k<plane_width; k++)
			plane_row0[k] = (int)(k*dz_dx);
	}
	for (rown=0; rown<rect->size[1]; rown+=2)
	{	if (event_priority && event_priority(this)==FIRST)
		{	if (rect->plane)
				sfree(plane_row);
			sfree(i_buffers2);
			if (o_buffers1)
			{
				sfree(o_buffers1);
				sfree(o_buffers2);
			}
			sfree(i_buffers1);
			sfree(z_buffers2);
			sfree(z_order);
			sfree(z_buffers1);
			return (INTERRUPT3);
		}
		for (thingn=0; thingn<rect->nopaque_objects; thingn++)
		{	if (object_class==BINARY_B || object_class==BINARY_A || object_class==T_SHELL)
			{	i_buffers1[thingn] = (unsigned char *)
					(things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
				i_buffers2[thingn] = (unsigned char *)
					(things[thingn].img->image[rown+1+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
			}
			else
			{	p_buffers1[thingn] = (Pixel_unit *)
					things[thingn].img->image[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					(rect->position[0]-things[thingn].img->image_location[0])*
					things[thingn].img->pixel_units;
				p_buffers2[thingn] = (Pixel_unit *)
					things[thingn].img->image[rown+1+
					rect->position[1]-things[thingn].img->image_location[1]]+
					(rect->position[0]-things[thingn].img->image_location[0])*
					things[thingn].img->pixel_units;
				o_buffers1[thingn] = (unsigned char *)
					(things[thingn].img->opacity_buffer[rown+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
				o_buffers2[thingn] = (unsigned char *)
					(things[thingn].img->opacity_buffer[rown+1+
					rect->position[1]-things[thingn].img->image_location[1]]+
					rect->position[0]-things[thingn].img->image_location[0]);
			}
			z_buffers1[thingn] = things[thingn].img->z_buffer[rown+
				rect->position[1]-things[thingn].img->image_location[1]]+
				rect->position[0]-things[thingn].img->image_location[0];
			z_buffers2[thingn] = things[thingn].img->z_buffer[rown+1+
				rect->position[1]-things[thingn].img->image_location[1]]+
				rect->position[0]-things[thingn].img->image_location[0];
		}
		for (; thingn<rect->nobjects; thingn++)
		{	if (object_class==BINARY_B || object_class==BINARY_A || object_class==T_SHELL)
				i_buffers1[thingn] = (unsigned char *)
				(things[thingn].img->image[(rown+
				rect->position[1]-things[thingn].img->image_location[1])/2]+
				(rect->position[0]-things[thingn].img->image_location[0])/2);
			else
				p_buffers1[thingn] = (Pixel_unit *)
				things[thingn].img->image[(rown+
				rect->position[1]-things[thingn].img->image_location[1])/2]+
				(rect->position[0]-things[thingn].img->image_location[0])/2*
				things[thingn].img->pixel_units;
			z_buffers1[thingn] = things[thingn].img->z_buffer[(rown+
				rect->position[1]-things[thingn].img->image_location[1])/2]+
				(rect->position[0]-things[thingn].img->image_location[0])/2;
		}

		if (rect->plane)
		{	int left_x1, left_z1, x;

			left_x1 = (int)rint(.5*(plane_top_left[0]-rect->position[0]+
				dx_dy*(rown+rect->position[1]-plane_top_left[1])));
			left_z1 = (int)rint(plane_top_left[2]+
				dz_dy*(.5*(rown+rect->position[1]-plane_top_left[1])));
			for (x=0; x<rect->size[0]/2; x++)
			{	k = x-left_x1;
				plane_row[x] =
					k>=0&&k<plane_width
					?	left_z1+plane_row0[k]
					:	0;
			}
			plane_z_buffer = plane_row;
		}

		out_ptr = image->data+
			out_bytes_per_line*
			(rown+rect->position[1]+image->height)/2+
			out_pixel_bytes*(rect->position[0]+image->width)/2;
		parity = (rect->position[0]+rect->position[1]+rown)&2;
		row_end = out_ptr+out_pixel_bytes*rect->size[0]/2;
		switch (object_class)
		{	case BINARY_A:
			case BINARY_B:
			case GRADIENT:
			case T_SHELL:
				for (; out_ptr<row_end;
						out_ptr+=out_pixel_bytes, parity=!parity)
				{	ii_value1 = ii_value2 = ii_value3 = ii_value4 =
						background_value;
					z_value1 = z_value2 = z_value3 = z_value4 = 0;
					color1 = color2 = color3 = color4 = 0;
					if (maximum_intensity_projection)
					{
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {	if (ii_value1==background_value ||
								(p_buffers1[thingn][0]!=background_value&&
								p_buffers1[thingn][0]>ii_value1))
						{	ii_value1 =
								object_class!=GRADIENT
								?	i_buffers1[thingn][0]
								:	p_buffers1[thingn][0];
							z_value1 = z_buffers1[thingn][0];
							color1 = things[thingn].vobj->color;
						}
						if (ii_value2==background_value ||
								(p_buffers1[thingn][1]!=background_value&&
								p_buffers1[thingn][1]>ii_value2))
						{	ii_value2 =
								object_class!=GRADIENT
								?	i_buffers1[thingn][1]
								:	p_buffers1[thingn][1];
							z_value2 = z_buffers1[thingn][1];
							color2 = things[thingn].vobj->color;
						}
						if (ii_value3==background_value ||
								(p_buffers2[thingn][0]!=background_value&&
								p_buffers2[thingn][0]>ii_value3))
						{	ii_value3 =
								object_class!=GRADIENT
								?	i_buffers2[thingn][0]
								:	p_buffers2[thingn][0];
							z_value3 = z_buffers2[thingn][0];
							color3 = things[thingn].vobj->color;
						}
						if (ii_value4==background_value ||
								(p_buffers2[thingn][1]!=background_value&&
								p_buffers2[thingn][1]>ii_value4))
						{	ii_value4 =
								object_class!=GRADIENT
								?	i_buffers2[thingn][1]
								:	p_buffers2[thingn][1];
							z_value4 = z_buffers2[thingn][1];
							color4 = things[thingn].vobj->color;
						}
						if (object_class!=GRADIENT)
						{	i_buffers1[thingn] += 2;
							i_buffers2[thingn] += 2;
						}
						else
						{	p_buffers1[thingn] +=
								2*things[thingn].img->pixel_units;
							p_buffers2[thingn] +=
								2*things[thingn].img->pixel_units;
						}
						z_buffers1[thingn] += 2;
						z_buffers2[thingn] += 2;
					  }
					}
					else
					{
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {
						for (k=thingn; k&&z_buffers1[thingn][0]>
								z_buffers1[z_order[k-1]][0]; k--)
							z_order[k] = z_order[k-1];
						z_order[k] = thingn;
					  }
					  for (k=0; k<thingn; k++)
					    if (z_buffers1[z_order[k]][0] == 0)
						  thingn = k;
						else if (o_buffers1? *o_buffers1[z_order[k]]==255:
							things[z_order[k]].vobj->opacity >= 1)
						{
						  thingn = k+1;
						  break;
						}
					  for (thingn--; thingn>=0; thingn--)
					  {
						ii_value1 = (int)((ii_value1==background_value? 0:
						  (1-(o_buffers1?*o_buffers1[z_order[thingn]]*(1/255.):
						  things[z_order[thingn]].vobj->opacity))*ii_value1)+
						  (o_buffers1? *o_buffers1[z_order[thingn]]*(1/255.):
						  things[z_order[thingn]].vobj->opacity)*(
							object_class!=GRADIENT
							?	i_buffers1[z_order[thingn]][0]
							:	p_buffers1[z_order[thingn]][0]));
						z_value1 = z_buffers1[z_order[thingn]][0];
						color1 = things[z_order[thingn]].vobj->color;
					  }
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {
						for (k=thingn; k&&z_buffers1[thingn][1]>
								z_buffers1[z_order[k-1]][1]; k--)
							z_order[k] = z_order[k-1];
						z_order[k] = thingn;
					  }
					  for (k=0; k<thingn; k++)
					    if (z_buffers1[z_order[k]][1] == 0)
						  thingn = k;
						else if (o_buffers1? o_buffers1[z_order[k]][1]==255:
							things[z_order[k]].vobj->opacity >= 1)
						{
						  thingn = k+1;
						  break;
						}
					  for (thingn--; thingn>=0; thingn--)
					  {
						ii_value2 = (int)((ii_value2==background_value? 0: (1-
						  (o_buffers1? o_buffers1[z_order[thingn]][1]*(1/255.):
						  things[z_order[thingn]].vobj->opacity))*ii_value2)+
						  (o_buffers1? o_buffers1[z_order[thingn]][1]*(1/255.):
						  things[z_order[thingn]].vobj->opacity)*(
							object_class!=GRADIENT
							?	i_buffers1[z_order[thingn]][1]
							:	p_buffers1[z_order[thingn]][1]));
						z_value2 = z_buffers1[z_order[thingn]][1];
						color2 = things[z_order[thingn]].vobj->color;
					  }
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {
						for (k=thingn; k&&z_buffers2[thingn][0]>
								z_buffers2[z_order[k-1]][0]; k--)
							z_order[k] = z_order[k-1];
						z_order[k] = thingn;
					  }
					  for (k=0; k<thingn; k++)
					    if (z_buffers2[z_order[k]][0] == 0)
						  thingn = k;
						else if (o_buffers1? *o_buffers2[z_order[k]]==255:
							things[z_order[k]].vobj->opacity >= 1)
						{
						  thingn = k+1;
						  break;
						}
					  for (thingn--; thingn>=0; thingn--)
					  {
						ii_value3 = (int)((ii_value3==background_value? 0:
						  (1-(o_buffers1?*o_buffers2[z_order[thingn]]*(1/255.):
						  things[z_order[thingn]].vobj->opacity))*ii_value3)+
						  (o_buffers1? *o_buffers2[z_order[thingn]]*(1/255.):
						  things[z_order[thingn]].vobj->opacity)*(
							object_class!=GRADIENT
							?	i_buffers2[z_order[thingn]][0]
							:	p_buffers2[z_order[thingn]][0]));
						z_value3 = z_buffers2[z_order[thingn]][0];
						color3 = things[z_order[thingn]].vobj->color;
					  }
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {
						for (k=thingn; k&&z_buffers2[thingn][1]>
								z_buffers2[z_order[k-1]][1]; k--)
							z_order[k] = z_order[k-1];
						z_order[k] = thingn;
					  }
					  for (k=0; k<thingn; k++)
					    if (z_buffers2[z_order[k]][1] == 0)
						  thingn = k;
						else if (o_buffers1? o_buffers2[z_order[k]][1]==255:
							things[z_order[k]].vobj->opacity >= 1)
						{
						  thingn = k+1;
						  break;
						}
					  for (thingn--; thingn>=0; thingn--)
					  {
						ii_value4 = (int)((ii_value4==background_value? 0: (1-
						  (o_buffers1? o_buffers2[z_order[thingn]][1]*(1/255.):
						  things[z_order[thingn]].vobj->opacity))*ii_value4)+
						  (o_buffers1? o_buffers2[z_order[thingn]][1]*(1/255.):
						  things[z_order[thingn]].vobj->opacity)*(
							object_class!=GRADIENT
							?	i_buffers2[z_order[thingn]][1]
							:	p_buffers2[z_order[thingn]][1]));
						z_value4 = z_buffers2[z_order[thingn]][1];
						color4 = things[z_order[thingn]].vobj->color;
					  }
					  for (thingn=0; thingn<rect->nopaque_objects; thingn++)
					  {
						if (object_class!=GRADIENT)
						{
							i_buffers1[thingn] += 2;
							i_buffers2[thingn] += 2;
						}
						else
						{	p_buffers1[thingn] +=
								2*things[thingn].img->pixel_units;
							p_buffers2[thingn] +=
								2*things[thingn].img->pixel_units;
							o_buffers1[thingn] += 2;
							o_buffers2[thingn] += 2;
						}
						z_buffers1[thingn] += 2;
						z_buffers2[thingn] += 2;
					  }
					}
					z_value5 = z_value1;
					color5 = color1;
					if (z_value2 > z_value5)
					{	z_value5 = z_value2;
						color5 = color2;
					}
					if (z_value3 > z_value5)
					{	z_value5 = z_value3;
						color5 = color3;
					}
					if (z_value4 > z_value5)
					{	z_value5 = z_value4;
						color5 = color4;
					}
					ii_values = 0;
					ii_value5 = 0;
					if (ii_value1 != background_value)
					{	ii_values++;
						ii_value5 += ii_value1;
					}
					if (ii_value2 != background_value)
					{	ii_values++;
						ii_value5 += ii_value2;
					}
					if (ii_value3 != background_value)
					{	ii_values++;
						ii_value5 += ii_value3;
					}
					if (ii_value4 != background_value)
					{	ii_values++;
						ii_value5 += ii_value4;
					}
					switch (ii_values)
					{	case 0:
							ii_value5 = background_value;
							break;
						case 1:
							break;
						case 2:
							ii_value5 >>= 1;
							break;
						case 3:
							ii_value5 /= 3;
							break;
						case 4:
							ii_value5 >>= 2;
							break;
					}
					for (thingn=rect->nopaque_objects; thingn<rect->nobjects;
							thingn++)
					{	if (parity && (
								maximum_intensity_projection
								?	ii_value5==background_value ||
									(*p_buffers1[thingn]!=background_value&&
									*p_buffers1[thingn]>ii_value5)
								:	*z_buffers1[thingn]>z_value5))
						{	z_value5 = *z_buffers1[thingn];
							ii_value5 =
								object_class!=GRADIENT
								?	*i_buffers1[thingn]
								:	*p_buffers1[thingn];
							color5 = things[thingn].vobj->color;
						}
						z_buffers1[thingn]++;
						if (object_class!=GRADIENT)
							i_buffers1[thingn]++;
						else
							p_buffers1[thingn]++;
					}
					i_value = object_color_table[color5][
						object_class!=GRADIENT
						?	rect->plane && *plane_z_buffer>z_value5
							?	ii_value5+PLANE_INDEX_OFFSET
							:	ii_value5
						:	v_object_color_table[0][ii_value5]+(
							rect->plane && *plane_z_buffer>z_value5
							?	PLANE_INDEX_OFFSET
							:	0)];
					((int *)out_ptr)[0] = i_value.l;
					if (rect->plane)
						plane_z_buffer++;
				}
				break;
			case PERCENT:
			case DIRECT:
			  for (; out_ptr<row_end; out_ptr+=3)
			  {
				z_value1 = z_value2 = z_value3 = z_value4 = 0;

				for (thingn=0; thingn<rect->nopaque_objects; thingn++)
				{
					for (k=thingn; k&&z_buffers1[thingn][0]>
							z_buffers1[z_order[k-1]][0]; k--)
						z_order[k] = z_order[k-1];
					z_order[k] = thingn;
				}
				for (k=0; k<thingn; k++)
				    if (z_buffers1[z_order[k]][0] == 0)
					  thingn = k;
					else if (o_buffers1[z_order[k]][0] == 255)
					{
					  thingn = k+1;
					  break;
					}
				for (thingn--; thingn>=0; thingn--)
				{
				  for (pixel_byte=0; pixel_byte<3; pixel_byte++)
					ic_value[0][pixel_byte] = maximum_intensity_projection?
					  p_buffers1[z_order[thingn]][pixel_byte]:
					  (int)((z_value1==0? 0:
					  (1-o_buffers1[z_order[thingn]][0]*(1/255.))*
					  ic_value[0][pixel_byte])+
					  o_buffers1[z_order[thingn]][0]*(1/255.)*
					  p_buffers1[z_order[thingn]][pixel_byte]);
				  z_value1 = z_buffers1[z_order[thingn]][0];
				}

				for (thingn=0; thingn<rect->nopaque_objects; thingn++)
				{
					for (k=thingn; k&&z_buffers1[thingn][1]>
							z_buffers1[z_order[k-1]][1]; k--)
						z_order[k] = z_order[k-1];
					z_order[k] = thingn;
				}
				for (k=0; k<thingn; k++)
				    if (z_buffers1[z_order[k]][1] == 0)
					  thingn = k;
					else if (o_buffers1[z_order[k]][1] == 255)
					{
					  thingn = k+1;
					  break;
					}
				for (thingn--; thingn>=0; thingn--)
				{
				  for (pixel_byte=0; pixel_byte<3; pixel_byte++)
					ic_value[1][pixel_byte] = maximum_intensity_projection?
					  p_buffers1[z_order[thingn]][3+pixel_byte]:
					  (int)((z_value2==0? 0:
					  (1-o_buffers1[z_order[thingn]][1]*(1/255.))*
					  ic_value[1][pixel_byte])+
					  o_buffers1[z_order[thingn]][1]*(1/255.)*
					  p_buffers1[z_order[thingn]][3+pixel_byte]);
				  z_value2 = z_buffers1[z_order[thingn]][1];
				}

				for (thingn=0; thingn<rect->nopaque_objects; thingn++)
				{
					for (k=thingn; k&&z_buffers2[thingn][0]>
							z_buffers2[z_order[k-1]][0]; k--)
						z_order[k] = z_order[k-1];
					z_order[k] = thingn;
				}
				for (k=0; k<thingn; k++)
				    if (z_buffers2[z_order[k]][0] == 0)
					  thingn = k;
					else if (o_buffers2[z_order[k]][0] == 255)
					{
					  thingn = k+1;
					  break;
					}
				for (thingn--; thingn>=0; thingn--)
				{
				  for (pixel_byte=0; pixel_byte<3; pixel_byte++)
					ic_value[2][pixel_byte] = maximum_intensity_projection?
					  p_buffers2[z_order[thingn]][pixel_byte]:
					  (int)((z_value3==0? 0:
					  (1-o_buffers2[z_order[thingn]][0]*(1/255.))*
					  ic_value[2][pixel_byte])+
					  o_buffers2[z_order[thingn]][0]*(1/255.)*
					  p_buffers2[z_order[thingn]][pixel_byte]);
				  z_value3 = z_buffers2[z_order[thingn]][0];
				}

				for (thingn=0; thingn<rect->nopaque_objects; thingn++)
				{
					for (k=thingn; k&&z_buffers2[thingn][1]>
							z_buffers2[z_order[k-1]][1]; k--)
						z_order[k] = z_order[k-1];
					z_order[k] = thingn;
				}
				for (k=0; k<thingn; k++)
				    if (z_buffers2[z_order[k]][1] == 0)
					  thingn = k;
					else if (o_buffers2[z_order[k]][1] == 255)
					{
					  thingn = k+1;
					  break;
					}
				for (thingn--; thingn>=0; thingn--)
				{
				  for (pixel_byte=0; pixel_byte<3; pixel_byte++)
					ic_value[3][pixel_byte] = maximum_intensity_projection?
					  p_buffers2[z_order[thingn]][3+pixel_byte]:
					  (int)((z_value4==0? 0:
					  (1-o_buffers2[z_order[thingn]][1]*(1/255.))*
					  ic_value[3][pixel_byte])+
					  o_buffers2[z_order[thingn]][1]*(1/255.)*
					  p_buffers2[z_order[thingn]][3+pixel_byte]);
				  z_value4 = z_buffers2[z_order[thingn]][1];
				}

				z_value5 = z_value1>z_value2? z_value1:z_value2;
				if (z_value3 > z_value5)
					z_value5 = z_value3;
				if (z_value4 > z_value5)
					z_value5 = z_value4;
				for (pixel_byte=0; pixel_byte<3; pixel_byte++)
				{
					ii_values = 0;
					ii_value5 = 0;
					if (z_value1)
					{	ii_values++;
						ii_value5 += ic_value[0][pixel_byte];
					}
					if (z_value2)
					{	ii_values++;
						ii_value5 += ic_value[1][pixel_byte];
					}
					if (z_value3)
					{	ii_values++;
						ii_value5 += ic_value[2][pixel_byte];
					}
					if (z_value4)
					{	ii_values++;
						ii_value5 += ic_value[3][pixel_byte];
					}
					switch (pixel_byte)
					{	case 0:
							background_value = pbg.red;
							break;
						case 1:
							background_value = pbg.green;
							break;
						case 2:
							background_value = pbg.blue;
							break;
					}
					ii_value5 = (unsigned)
						(ii_value5+(4-ii_values)*background_value)/4;
					out_ptr[pixel_byte] =
						rect->plane && *plane_z_buffer>z_value5
						?	(char)(fplane_transparency[pixel_byte]*ii_value5)
						:	v_object_color_table[0][ii_value5];
				}
				for (thingn=0; thingn<rect->nopaque_objects; thingn++)
				{	p_buffers1[thingn] += 6;
					p_buffers2[thingn] += 6;
					z_buffers1[thingn] += 2;
					z_buffers2[thingn] += 2;
					o_buffers1[thingn] += 2;
					o_buffers2[thingn] += 2;
				}
				if (rect->plane)
					plane_z_buffer++;
			  }
			  break;
		}
	}
	if (rect->plane)
	{	sfree(plane_row);
		sfree(plane_row0);
	}
	sfree(i_buffers2);
	sfree(i_buffers1);
	if (o_buffers1)
	{
		sfree(o_buffers1);
		sfree(o_buffers2);
	}
	sfree(z_buffers2);
	sfree(z_order);
	sfree(z_buffers1);
	return (DONE);
}

/*****************************************************************************
 * FUNCTION: get_object_extent
 * DESCRIPTION: Computes the size of the bounding box of an object.
 * PARAMETERS:
 *    object_extent: The extent of the bounding box in plan space, but units of
 *       output pixels, in x, y, z, -x, -y, -z direction, goes here.
 *    object: The object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable scale must be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/26/93 to handle measurement units by Dewey Odhner
 *
 *****************************************************************************/
void get_object_extent(double object_extent[6], Shell *object, double scale)
{
	double corners[8][3], rotation[3][3], displacement_pixels[3],
		object_space_extent[3], length, width, depth;
	int j, k;

	AtoM(rotation, object->angle[0], object->angle[1], object->angle[2]);
	for (k=0; k<3; k++)
		displacement_pixels[k] = object->displacement[k]*scale;
	width =	(	Largest_y1(&object->main_data)-
				Smallest_y1(&object->main_data)+1
			)*object->main_data.file->file_header.str.xysize[0]*
			unit_mm(&object->main_data);
	length = object->main_data.rows*unit_mm(&object->main_data)*
		object->main_data.file->file_header.str.xysize[1];
	depth = object->main_data.slices*Slice_spacing(&object->main_data)*
		unit_mm(&object->main_data);
	object_space_extent[0] = width*scale/2;
	object_space_extent[1] = length*scale/2;
	object_space_extent[2] = depth*scale/2;
	for (j=0; j<8; j++)
	{	for (k=0; k<3; k++)
			corners[j][k] =
				j&1<<k
				?	object_space_extent[k]
				:	-object_space_extent[k];
		matrix_vector_multiply(corners[j], rotation, corners[j]);
		for (k=0; k<3; k++)
			corners[j][k] += displacement_pixels[k];
	}
	for (k=0; k<3; k++)
		object_extent[k] = object_extent[k+3] = corners[0][k];
	for (j=1; j<8; j++)
		for (k=0; k<3; k++)
		{	if (corners[j][k] > object_extent[k])
				object_extent[k] = corners[j][k];
			if (corners[j][k] < object_extent[k+3])
				object_extent[k+3] = corners[j][k];
		}
}

void cvRenderer::draw_box(XImage *image)
{
	if (!box || (image!=main_image && image!=image2))
		return;
	double rotation_matrix[3][3], box_corners[8][3], extent[6];
	int j, k, l, img_x, img_y, farthest_corner, arrowhead[3], arrowtail[3];

	if (image == main_image)
	{	img_x = image_x;
		img_y = image_y;
	}
	else
	{	img_x = image2_x;
		img_y = image2_y;
	}

	// get_box_extent(extent);
	double object_extent[6];
	Shell *object;
	int empty;
	extent[0] = extent[1] = extent[2] = main_image->width/2;
	extent[3] = extent[4] = extent[5] = -main_image->width/2;
	empty = TRUE;
	for (object=object_list; object!=NULL; object=object->next)
	{	get_object_extent(object_extent, object, scale);
		if (object_extent[3]>=object_extent[0] ||
				object_extent[4]>=object_extent[1] ||
				object_extent[5]>=object_extent[2])
			continue;
		if (empty)
		{	memcpy(extent, object_extent, sizeof(object_extent));
			empty = FALSE;
		}
		else
		{	if (object_extent[0] > extent[0])
				extent[0] = object_extent[0];
			if (object_extent[1] > extent[1])
				extent[1] = object_extent[1];
			if (object_extent[2] > extent[2])
				extent[2] = object_extent[2];
			if (object_extent[3] < extent[3])
				extent[3] = object_extent[3];
			if (object_extent[4] < extent[4])
				extent[4] = object_extent[4];
			if (object_extent[5] < extent[5])
				extent[5] = object_extent[5];
		}
	}

	AtoM(rotation_matrix, glob_angle[0], glob_angle[1], glob_angle[2]);
	farthest_corner = 0;
	for (j=0; j<8; j++)
	{	for (k=0; k<3; k++)
			box_corners[j][k] = extent[j&1<<k? k: k+3];
		matrix_vector_multiply(box_corners[j], rotation_matrix,
			box_corners[j]);
		box_corners[j][0] += image->width/2+img_x;
		box_corners[j][1] += image->height/2+img_y;
		box_corners[j][2] *= depth_scale/scale;
		if (box_corners[j][2] > box_corners[farthest_corner][2])
			farthest_corner = j;
	}
	switch (farthest_corner)
	{	case 0:
		case 7:
			arrowtail[0] = 4;
			arrowtail[1] = 1;
			arrowtail[2] = 2;
			break;
		case 3:
		case 4:
			arrowtail[0] = 0;
			arrowtail[1] = 0;
			arrowtail[2] = 1;
			break;
		case 6:
		case 1:
			arrowtail[0] = 2;
			arrowtail[1] = 0;
			arrowtail[2] = 0;
			break;
		case 5:
		case 2:
			arrowtail[0] = 0;
			arrowtail[1] = 4;
			arrowtail[2] = 0;
			break;
	}
	arrowhead[0] = arrowtail[0]+1;
	arrowhead[1] = arrowtail[1]+2;
	arrowhead[2] = arrowtail[2]+4;
	for (j=0; j<8; j++)
		for (k=1; k<8; k<<=1)
			if ((k&j) == 0)
			{
				draw_edge(box_corners[j], box_corners[j+k],
					box_corners[farthest_corner], image);
				for (l=0; l<3; l++)
					if (j+k==arrowhead[l] && j==arrowtail[l] && object_list->
							main_data.file->file_header.str.axis_label)
						label_axis(box_corners[j+k], box_corners[j], image,
							box_axis_label_loc+l);
			}
}

/*****************************************************************************
 * FUNCTION: label_axis
 * DESCRIPTION: Draws an arrowhead and axis label using the overlay in
 *    display_area or display_area2.
 * PARAMETERS:
 *   head, tail: The ends of the arrow to label the axis in display area
 *      coordinates.
 *   image: main_image or image2.  Specifies which display area to draw the
 *      line in.
 *   label: The label of the axis.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 11/1/93 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::label_axis(double head[3], double tail[3], XImage *image,
	X_Point *label_loc)
{
	const int space = 15;
	double u[2], norm;
	int x1, y1, x2, y2;
	if (head[0]==tail[0] && head[1]==tail[1])
	{
		label_loc->x = -1;
		return;
	}
	assert(image==main_image || image==image2);
	u[0] = head[0]-tail[0];
	u[1] = head[1]-tail[1];
	norm = 1/sqrt(u[0]*u[0]+u[1]*u[1]);
	u[0] *= norm;
	u[1] *= norm;
	x1 = (int)rint(head[0]-10*u[0]+5*u[1]);
	y1 = (int)rint(head[1]-10*u[1]-5*u[0]);
	x2 = (int)rint(head[0]);
	y2 = (int)rint(head[1]);
	if
	(	(x1>=0 || x2>=0) && (y1>=0 || y2>=0) &&
		(x1<image->width || x2<image->width) &&
		(y1<image->height || y2<image->height)
	)
		draw_line_segment(x1, y1, x2, y2, image);
	x1 = (int)rint(head[0]-10*u[0]-5*u[1]);
	y1 = (int)rint(head[1]-10*u[1]+5*u[0]);
	if
	(	(x1>=0 || x2>=0) && (y1>=0 || y2>=0) &&
		(x1<image->width || x2<image->width) &&
		(y1<image->height || y2<image->height)
	)
		draw_line_segment(x1, y1, x2, y2, image);
	x1 = (int)rint(head[0]+space*u[0]);
	y1 = (int)rint(head[1]+space*u[1]);
	if (x1>=0 && y1>=0 && x1<image->width && y1<image->height)
	{
		label_loc->x = x1;
		label_loc->y = y1;
	}
	else
		label_loc->x = -1;
}

/*****************************************************************************
 * FUNCTION: draw_edge
 * DESCRIPTION: Draws an edge of the bounding box using the overlay in
 *    display_area or display_area2.
 * PARAMETERS:
 *   p1, p2: The ends of the edge, display area coordinates.
 *   farthest_corner: If it is equal to p1 or p2, objects hide back line parts.
 *   image: main_image or image2.  Specifies which display area to draw the
 *       line in.
 * SIDE EFFECTS: The global variable overlay_clear may be changed.
 *    An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::draw_edge(double p1[3], double p2[3],
		double farthest_corner[3], XImage *image)
{
	int x1, y1, x2, y2;

	assert(image==main_image || image==image2);
	x1 = (int)rint(p1[0]);
	y1 = (int)rint(p1[1]);
	x2 = (int)rint(p2[0]);
	y2 = (int)rint(p2[1]);
	if
	(	(x1>=0 || x2>=0) && (y1>=0 || y2>=0) &&
		(x1<image->width || x2<image->width) &&
		(y1<image->height || y2<image->height)
	)
	{	if (p1==farthest_corner || p2==farthest_corner)
		/* Hide back line parts */
			draw_far_line(x1, y1, x2, y2, image);
		else
			draw_line_segment(x1, y1, x2, y2, image);
	}
}

/*****************************************************************************
 * FUNCTION: draw_far_line
 * DESCRIPTION: Draws a far edge of the bounding box using the overlay in
 *    display_area or display_area2, not drawing over objects.
 * PARAMETERS:
 *   x1, y1, x2, y2: The ends of the edge, display area coordinates.
 *   image: main_image or image2.  Specifies which display area to draw the
 *       line in.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::draw_far_line(int x1, int y1, int x2, int y2, XImage *image)
{
	X_Point *points;
	int npoints;

	assert(image==main_image || image==image2);
	if (VComputeLine(x1, y1, x2, y2, &points, &npoints) == 1)
	{	report_malloc_error();
		return;
	}
	Classification_type object_class = st_cl(&object_list->main_data);
	for (int j=0; j<npoints; j++)
		if (points[j].x>=0 && points[j].x<image->width &&
				points[j].y>=0 && points[j].y<image->height &&
				closest_z(points[j].x, points[j].y, image)==0)
		{
		  if (object_class==PERCENT || object_class==DIRECT)
		  {
			char *data = image->data+points[j].y*image->width*3+
				(points[j].x+image->xoffset)*3;
			data[0] = v_object_color_table[0][(int)(mark_color.red*
				((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
			data[1] = v_object_color_table[0][(int)(mark_color.green*
				((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
			data[2] = v_object_color_table[0][(int)(mark_color.blue*
				((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
		  }
		  else
			for (int pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
				image->data[points[j].y*image->bytes_per_line+
					(points[j].x+image->xoffset)*pixel_bytes+pixel_byte] =
					object_color_table[0][MARK_SHADE].c[pixel_byte];
		}
	free(points);
}

void cvRenderer::draw_line_segment(int x1, int y1, int x2, int y2,
		XImage *image)
{
	X_Point *points;
	int npoints, j;

	assert(image==main_image || image==image2);
	if (VComputeLine(x1, y1, x2, y2, &points, &npoints) == 1)
	{	report_malloc_error();
		return;
	}
	Classification_type object_class = st_cl(&object_list->main_data);
	for (j=0; j<npoints; j++)
		if (points[j].x>=0 && points[j].x<image->width &&
				points[j].y>=0 && points[j].y<image->height)
		{
			if (object_class==PERCENT || object_class==DIRECT)
			{
				char *data = image->data+points[j].y*image->width*3+
					(points[j].x+image->xoffset)*3;
				data[0] = v_object_color_table[0][(int)(mark_color.red*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
				data[1] = v_object_color_table[0][(int)(mark_color.green*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
				data[2] = v_object_color_table[0][(int)(mark_color.blue*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
			}
			else
				for (int pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
					image->data[points[j].y*image->bytes_per_line+
						(points[j].x+image->xoffset)*pixel_bytes+pixel_byte] =
						object_color_table[0][MARK_SHADE].c[pixel_byte];
		}
	free(points);
}

/*****************************************************************************
 * FUNCTION: display_marks
 * DESCRIPTION: Puts marks on main_image or image2 if global variable marks.
 * PARAMETERS:
 *    image: the image to put the marks on
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 * ENTRY CONDITIONS: image must be created as with XCreateImage and have
 *    sufficient memory allocated at image->data.
 *    The objects of object_list must have their images
 *    projected as with a successful call to project or be turned off.
 *    If event_priority so requires, display must describe a valid connection
 *    to an X server.  pixel_bytes must match the depth of image.
 *    object_color_table must be allocated and properly initialized.
 * RETURN VALUE: Why the function is returning.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 12/6/91 by Dewey Odhner.
 *    Modified 6/23/92 to use image2 by Dewey Odhner.
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::display_marks(XImage *image,
	Priority (*event_priority)(cvRenderer *))
{
	Classification_type object_class = st_cl(&object_list->main_data);
	if (image == main_image)
	{
	  for (int j=0; j<nmeasurement_points; j++)
	  {
		int x1, y1, z1;
		plan_to_display_area_coords(&x1, &y1, &z1, measurement_point[j]);
		if (x1<2 || x1>image->width-3 || y1<2 || y1>image->height-3 ||
				closest_z(x1, y1, image)>z1)
			continue;
		for (int d=1; d<3; d++)
			for (int x=x1-d; x<=x1+d; x+= 2*d)
				for (int y=y1-d; y<=y1+d; y+= 2*d)
					if (object_class==PERCENT || object_class==DIRECT)
					{
						char *data = image->data+y*image->width*3+x*3;
						data[0] =
							v_object_color_table[0][(int)(mark_color.red*
							((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
						data[1] =
							v_object_color_table[0][(int)(mark_color.green*
							((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
						data[2] =
							v_object_color_table[0][(int)(mark_color.blue*
							((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
					}
					else
						for (int pixel_byte=0; pixel_byte<pixel_bytes;
								pixel_byte++)
							image->data[y*image->bytes_per_line+x*pixel_bytes+
								pixel_byte] = object_color_table[0][MARK_SHADE]
								.c[pixel_byte];
	  }
	  for (int line=0; line<nmeasurement_points-1; line++)
	  {
		int visible_points;
		X_Point *xpoints;
		int x1, x2, y1, y2, z1, z2;
		plan_to_display_area_coords(&x1, &y1, &z1, measurement_point[line]);
		plan_to_display_area_coords(&x2, &y2, &z2, measurement_point[line+1]);
		get_3d_line(x1, y1, z1, x2, y2, z2, xpoints, visible_points);
		if (visible_points == 0)
			continue;
		for (int p=0; p<visible_points; p++)
		  if (xpoints[p].x>=0 && xpoints[p].x<image->width &&
		      xpoints[p].y>=0 && xpoints[p].y<image->height)
		  {
		    if (object_class==PERCENT || object_class==DIRECT)
			{
				char *data = image->data+xpoints[p].y*image->width*3+
					xpoints[p].x*3;
				data[0] = v_object_color_table[0][(int)(mark_color.red*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
				data[1] = v_object_color_table[0][(int)(mark_color.green*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
				data[2] = v_object_color_table[0][(int)(mark_color.blue*
					((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
			}
			else
				for (int pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
					image->data[xpoints[p].y*image->bytes_per_line+
						xpoints[p].x*pixel_bytes+pixel_byte] =
						object_color_table[0][MARK_SHADE].c[pixel_byte];
		  }
		free(xpoints);
	  }
	}
	Shell *object;
	int this_mark;

	if (marks && (image==main_image || image==image2))
		for (object=object_list; object; object=object->next)
			if (object->O.on)
				for (this_mark=0; this_mark<object->marks; this_mark++)
				{   if (event_priority && event_priority(this)==FIRST)
						return (INTERRUPT3);
					display_mark(object, object->mark[this_mark], image);
				}
	return (DONE);
}

/*****************************************************************************
 * FUNCTION: display_mark
 * DESCRIPTION: Puts a mark on main_image.
 * PARAMETERS:
 *    object: The object with the mark.
 *    mark: The object space coordinates of the mark.
 *    image: the image to put the marks on
 * SIDE EFFECTS: Puts a mark on main_image.
 * ENTRY CONDITIONS: image must be created as with XCreateImage and have
 *    sufficient memory allocated at image->data. The object must have
 *    its image projected as with a successful call to project.
 *    pixel_bytes must match the depth of image.
 *    object_list must be valid.
 *    object_color_table must be allocated and properly initialized.
 * RETURN VALUE: None.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 12/6/91 by Dewey Odhner.
 *    Modified 6/23/92 to use image2 by Dewey Odhner.
 *    Modified: 3/24/94 for volume rendered images by Dewey Odhner.
 *    Modified: 12/12/94 v_object_color_table used with PERCENT objects
 *       by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::display_mark(Shell *object, double mark[3], XImage *image)
{
	int x, y, x1, y1, pixel_byte;
	Classification_type object_class;
	char *data;

	if (!get_mark_x_y(&x, &y, object, mark, image))
		return;
	object_class = st_cl(&object_list->main_data);
	for (x1= -3; x1<=3; x1++)
	{	if (x+x1 < 0)
			continue;
		if (x+x1 >= image->width)
			break;
		for (y1= -3; y1<=3; y1++)
		{	if (y+y1 < 0)
				continue;
			if (y+y1 >= image->height)
				break;
			if ((x1+y1<2 && x1+y1>-2) || (x1-y1<2 && x1-y1>-2))
			{
				if (object_class==PERCENT || object_class==DIRECT)
				{	data = image->data+(y+y1)*image->width*3+
						(x+x1+image->xoffset)*3;
					data[0] = v_object_color_table[0][(int)(mark_color.red*
						((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
					data[1] = v_object_color_table[0][(int)(mark_color.green*
						((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
					data[2] = v_object_color_table[0][(int)(mark_color.blue*
						((V_OBJECT_IMAGE_BACKGROUND-1)/65535.))];
				}
				else
					for (pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
						image->data[(y+y1)*image->bytes_per_line+
							(x+x1+image->xoffset)*pixel_bytes+pixel_byte] =
							object_color_table[0][MARK_SHADE].c[pixel_byte];
			}
		}
	}
}

/*****************************************************************************
 * FUNCTION: get_z
 * DESCRIPTION: Returns the z-buffer value of the closest object or the
 *    closest opaque object at the pixel (or brightest object, if
 *    maximum_intensity_projection && class is GRADIENT).
 * PARAMETERS:
 *    x, y: The pixel to get the value at, display area coordinates.
 *    transparent: If non-zero, semi-transparent objects will be considered.
 *    image: main_image or image2.  Specifies which display area the pixel is
 *       in.
 * SIDE EFFECTS: object_of_point set
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: The z-buffer value of the closest object at the pixel.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 7/11/94 for maximum intensity projection by Dewey Odhner
 *    Modified: 3/27/95 secondary objects counted instead of
 *       separate_piece1, separate_piece2 by Dewey Odhner.
 *    Modified: 4/15/02 object_of_point set by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::get_z(int x, int y, int transparent, XImage *image)
{
	Shell *obj;
	Virtual_object *vobj;
	int ox, oy, sofar, vn, mip, isofar;

	assert(image==main_image || image==image2);
	/* convert from display area to image coordinates from center of image */
	if (image == main_image)
	{	x -= image->width/2+image_x;
		y -= image->height/2+image_y;
	}
	else
	{	x -= image->width/2+image2_x;
		y -= image->height/2+image2_y;
	}
	/* convert to input pixels */
	if (anti_alias)
	{	x *= 2;
		y *= 2;
	}
	else
	{	x /= 2;
		y /= 2;
	}

	mip = maximum_intensity_projection &&
		st_cl(&object_list->main_data)==GRADIENT;
	object_of_point = NULL;
	sofar = 0;
	isofar = V_OBJECT_IMAGE_BACKGROUND;
	for (obj=object_list; obj!=NULL; obj=obj->next)
		for (vn=1, vobj= &obj->O; vn<=2&&vobj!=NULL&&vobj->on;
				vn++, vobj=obj->reflection)
		{	if ((image_mode==SEPARATE && ((image==main_image &&
					obj==separate_piece2) ||
					(image==image2 && obj!=separate_piece2))) ||
					(image_mode==FUZZY_CONNECT &&
					((image==main_image && obj->secondary) ||
					(image==image2 && !obj->secondary))))
				continue;
			ox = x-vobj->main_image.image_location[0];
			oy = y-vobj->main_image.image_location[1];
			if (anti_alias && vobj->opacity==.5)
			{	ox /= 2;
				oy /= 2;
			}
			if (vobj->on && (transparent||vobj->opacity>=1) &&
					ox>=0 && ox<vobj->main_image.image_size &&
					oy>=0 && oy<vobj->main_image.image_size && (
					mip
					?	sofar==0 ||
						(vobj->main_image.z_buffer[oy][ox]>0 &&
						((Pixel_unit *)vobj->main_image.image[oy])[ox]>isofar)
					:	vobj->main_image.z_buffer[oy][ox]>sofar))
			{	object_of_point = vobj;
				sofar = vobj->main_image.z_buffer[oy][ox];
				if (mip)
					isofar = ((Pixel_unit *)vobj->main_image.image[oy])[ox];
			}
		}
	return (sofar);
}

/*****************************************************************************
 * FUNCTION: closest_z
 * DESCRIPTION: Returns the z-buffer value of the closest object at the pixel.
 * PARAMETERS:
 *    x, y: The pixel to get the value at, display area coordinates.
 *    image: main_image or image2.  Specifies which display area the pixel is
 *       in.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: The z-buffer value of the closest object at the pixel.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::closest_z(int x, int y, XImage *image)
{
	return(get_z(x, y, TRUE, image));
}

/*****************************************************************************
 * FUNCTION: get_mark_x_y
 * DESCRIPTION: Finds the pixel of a mark location if the mark is visible.
 * PARAMETERS:
 *    x, y: The display_area or display_area2 coordinates of the pixel.
 *    object: The object with the mark.
 *    mark: The mark
 *    image: main_image for display_area coordinates; image2 for
 *       display_area2 coordinates.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A call to manip_init should be made first.
 * RETURN VALUE: Non-zero if a mark was found.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 3/27/95 secondary objects counted instead of
 *       separate_piece1, separate_piece2 by Dewey Odhner.
 *    Modified: 4/19/00 returns TRUE if marks == 2 by Dewey Odhner.
 *
 *****************************************************************************/
int cvRenderer::get_mark_x_y(int *x, int *y, Shell *object, double mark[3], XImage *image)
{
	double mark_coords[3], rotation[3][3];
	int z;

	if ((image_mode==SEPARATE &&
			((image==main_image&& object==separate_piece2) ||
			(image==image2 && object!=separate_piece2))) ||
			(image_mode==FUZZY_CONNECT&&
			((image==main_image&& object->secondary)
			|| (image==image2 && !object->secondary))))
		return (FALSE);
	/* Convert from object to plan coordinates */
	AtoM(rotation, object->angle[0], object->angle[1], object->angle[2]);
	matrix_vector_multiply(mark_coords, rotation, mark);
	mark_coords[0] += object->displacement[0];
	mark_coords[1] += object->displacement[1];
	mark_coords[2] += object->displacement[2];
	/* Convert from plan to image coordinates from top left */
	AtoM(rotation, glob_angle[0], glob_angle[1], glob_angle[2]);
	matrix_vector_multiply(mark_coords, rotation, mark_coords);
	/* convert from physical units */
	mark_coords[0] *= scale;
	mark_coords[1] *= scale;
	z = (int)rint(MIDDLE_DEPTH-mark_coords[2]*depth_scale);
	*x = (int)rint(mark_coords[0]+image->width/2);
	*y = (int)rint(mark_coords[1]+image->height/2);
	if (marks == 2)
		return (TRUE);
	if (image == main_image)
		return (z+4000 >= get_z(*x+image_x, *y+image_y, FALSE, image));
	else
		return (z+4000 >= get_z(*x+image2_x, *y+image2_y, FALSE, image));
}
