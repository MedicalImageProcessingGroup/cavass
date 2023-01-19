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


 
#include <ctype.h> 
 
#include "cvRender.h"


static const int image_x=0, image_y=0, image2_x=0, image2_y=0, pixel_bytes=4;
static const int display_area_x=0, display_area_y=0, display_area2_x=0, display_area2_y=0;

/*****************************************************************************
 * FUNCTION: unit_label
 * DESCRIPTION: Returns the abreviation for the unit of measurement in the Y0
 *    direction of a shell.
 * PARAMETERS:
 *    shell_data: The shell to get the unit from.  The unit must be at least
 *       0 (km) and not more than 4 (micron).
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The string at the address returned must not be changed.
 * RETURN VALUE: Returns the abreviation for the unit of measurement.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 10/19/93 by Dewey Odhner
 *    Modified: 7/6/01 DIRECT data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
const char *unit_label(Shell_data *shell_data)
{
	static const char label[5][8]={"km", "m", "cm", "mm", "microns"};

	return (label[shell_data->file->file_header.gen.data_type==IMAGE0?
		shell_data->file->file_header.scn.measurement_unit[0]:
		shell_data->file->file_header.str.measurement_unit[0]]);
}

/*****************************************************************************
 * FUNCTION: voxel_spacing
 * DESCRIPTION: Returns the diameter of a voxel.
 * PARAMETERS:
 *    obj_data: The Shell to get the voxel size from
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The parameter must be valid.
 * RETURN VALUE: The diameter of a voxel in mm.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/19/93 to handle units by Dewey Odhner
 *
 *****************************************************************************/
double voxel_spacing(Shell_data *obj_data)
{
	double sx, sy, sz, unit;

	unit = unit_mm(obj_data);
	if (obj_data->file->file_header.gen.data_type == IMAGE0)
	{
		sx = obj_data->file->file_header.scn.xypixsz[0]*unit;
		sy = obj_data->file->file_header.scn.xypixsz[1]*unit;
	}
	else
	{
		sx = obj_data->file->file_header.str.xysize[0]*unit;
		sy = obj_data->file->file_header.str.xysize[1]*unit;
	}
	sz = obj_data->slices<=1? sx:
		(Max_coordinate(obj_data, 2)-Min_coordinate(obj_data, 2))/
		(obj_data->slices-1)*unit;
	return (sqrt(sx*sx+sy*sy+sz*sz));
}

/*****************************************************************************
 * FUNCTION: check_true_color
 * DESCRIPTION: Sets the global variable true_color for an object of class
 *    PERCENT.  The object must be the first in object_list.
 * PARAMETERS: None
 * SIDE EFFECTS: The following global variables may be changed: colormap_valid,
 *    image_valid, icon_valid.
 * ENTRY CONDITIONS: The global variables object_list, tissue_opacity,
 *    tissue_red, tissue_green, tissue_blue, visual, surface_red_factor,
 *    surface_green_factor, surface_blue_factor, background, surface_strength,
 *    ambient must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Returns immediately if the object is not PERCENT.
 * HISTORY:
 *    Created: 3/16/94 by Dewey Odhner
 *    Modified: 4/14/94 to check plane by Dewey Odhner
 *    Modified: 4/22/94 to correct ambient value by Dewey Odhner
 *    Modified: 6/9/94 to check tissue 4 by Dewey Odhner
 *    Modified: 5/12/97 check for differing ambient components by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::check_true_color(void)
{
	int old_true_color;

	if (st_cl(&object_list->main_data)!=PERCENT &&
			st_cl(&object_list->main_data)!=DIRECT)
		return;
	old_true_color = true_color;
	true_color = TRUE;
	if (true_color != old_true_color)
		colormap_valid = image_valid = icon_valid = FALSE;
}

/*****************************************************************************
 * FUNCTION: set_principal_view
 * DESCRIPTION: Sets the viewing angle to the specified principal orientation.
 * PARAMETERS:
 *    view_type: AXIAL, CORONAL (frontal), or SAGITTAL (lateral).
 * SIDE EFFECTS: The global variable glob_angle will be changed.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/18/95 by Dewey Odhner
 *    Modified: 9/4/02 changed default for missing axis labels by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::set_principal_view(Principal_plane view_type)
{
	Shell *obj;
	int j, k;
	StructureInfo *str;
	double glob_rotation[3][3], q;

	memset(glob_rotation, 0, sizeof(glob_rotation));
	obj = actual_object(object_from_label(selected_object));
	str = &obj->main_data.file->file_header.str;
	if (str->axis_label_valid)
		for (j=0; j<3; j++)
		{	str->axis_label[j][0] = toupper(str->axis_label[j][0]);
			unsigned m;
			for (m=1; m<strlen(str->axis_label[j]); m++)
				str->axis_label[j][m] = tolower(str->axis_label[j][m]);
			if (str->axis_label[j][m-1] == ' ')
				str->axis_label[j][m-1] = 0;
			switch (view_type)
			{	case AXIAL:
					if (strcmp(str->axis_label[j], "Head")==0)
						glob_rotation[2][j] = 1;
					else if (strcmp(str->axis_label[j], "Feet")==0)
						glob_rotation[2][j] = -1;
					else if (strcmp(str->axis_label[j], "Anterior")==0)
						glob_rotation[1][j] = -1;
					else if (strcmp(str->axis_label[j], "Posterior")==0)
						glob_rotation[1][j] = 1;
					else if (strcmp(str->axis_label[j], "Left")==0)
						glob_rotation[0][j] = 1;
					else if (strcmp(str->axis_label[j], "Right")==0)
						glob_rotation[0][j] = -1;
					break;
				case CORONAL:
					if (strcmp(str->axis_label[j], "Head")==0)
						glob_rotation[1][j] = -1;
					else if (strcmp(str->axis_label[j], "Feet")==0)
						glob_rotation[1][j] = 1;
					else if (strcmp(str->axis_label[j], "Anterior")==0)
						glob_rotation[2][j] = -1;
					else if (strcmp(str->axis_label[j], "Posterior")==0)
						glob_rotation[2][j] = 1;
					else if (strcmp(str->axis_label[j], "Left")==0)
						glob_rotation[0][j] = 1;
					else if (strcmp(str->axis_label[j], "Right")==0)
						glob_rotation[0][j] = -1;
					break;
				case SAGITTAL:
					if (strcmp(str->axis_label[j], "Head")==0)
						glob_rotation[1][j] = -1;
					else if (strcmp(str->axis_label[j], "Feet")==0)
						glob_rotation[1][j] = 1;
					else if (strcmp(str->axis_label[j], "Anterior")==0)
						glob_rotation[0][j] = -1;
					else if (strcmp(str->axis_label[j], "Posterior")==0)
						glob_rotation[0][j] = 1;
					else if (strcmp(str->axis_label[j], "Left")==0)
						glob_rotation[2][j] = -1;
					else if (strcmp(str->axis_label[j], "Right")==0)
						glob_rotation[2][j] = 1;
					break;
			}
		}
	q = 0;
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			q += glob_rotation[j][k]*glob_rotation[j][k];
	if (q != 3)
		switch (view_type)
		{
			case AXIAL:
				glob_rotation[0][0] = 1;
				glob_rotation[1][1] = 1;
				glob_rotation[2][2] = 1;
				break;
			case CORONAL:
				glob_rotation[0][0] = 1;
				glob_rotation[2][1] = 1;
				glob_rotation[1][2] = -1;
				break;
			case SAGITTAL:
				glob_rotation[2][0] = -1;
				glob_rotation[0][1] = 1;
				glob_rotation[1][2] = -1;
				break;
		}
#ifndef NDEBUG
	q = 0;
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			q += glob_rotation[j][k]*glob_rotation[j][k];
	assert(q == 3);
#endif
	MtoA(glob_angle, glob_angle+1, glob_angle+2, glob_rotation);
}

/*****************************************************************************
 * FUNCTION: invalidate_images
 * DESCRIPTION: Marks all the object images bad.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables object_list, image_valid, image2_valid,
 *    icon_valid, overlay_bad may be changed.
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::invalidate_images(void)
{
	Shell *object;

	image_valid = image2_valid = icon_valid = FALSE;
	for (object=object_list; object!=NULL; object=object->next)
	{	object->O.icon.projected =
			object->O.main_image.projected = BAD;
		if (object->reflection)
			object->reflection->icon.projected =
			object->reflection->main_image.projected = BAD;
	}
}

/*****************************************************************************
 * FUNCTION: set_color_number
 * DESCRIPTION: Sets vobj->color when vobj->rgb is given.
 * PARAMETERS:
 *    vobj: A virtual object whose color components have been assigned.
 * SIDE EFFECTS: The global variables ncolors, colormap_valid may be changed.
 * ENTRY CONDITIONS: The global variable object_list must be valid except for
 *    vobj->color.  The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 11/17/92 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::set_color_number(Virtual_object *vobj)
{
	int matching_object, unused_color;

	vobj->color = -1;
	matching_object = object_of_color(vobj->rgb);
	unused_color = unused_color_number();
	if (matching_object == 0)
	{	vobj->color =
			unused_color<0
			?	new_color()
			:	unused_color;
		colormap_valid = FALSE;
	}
	else
	{	vobj->color = object_from_number(matching_object)->color;
		if (color_mode!=MOVIE_MODE && unused_color>=0)
		{	assert(ncolors > 1);
			eliminate_color(unused_color);
		}
	}
}

/*****************************************************************************
 * FUNCTION: get_image_size
 * DESCRIPTION: Computes a size for the main image so that all the objects
 *    will fit in.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables object_list, scale,
 *    plane_displacement, plane_normal must be valid.
 * RETURN VALUE: Pixels on a side.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::get_image_size(void)
{
	int image_size;
	Shell *object;
	double distance, farthest_corner;

	farthest_corner = 0;
	for (object=object_list; object!=NULL; object=object->next)
	{	distance = sqrt(object->displacement[0]*object->displacement[0]+
			object->displacement[1]*object->displacement[1]+
			object->displacement[2]*object->displacement[2]);
		if (object->diameter/2+distance > farthest_corner)
			farthest_corner = object->diameter/2+distance;
		if (object->reflection)
		{	distance = sqrt(
			  (object->displacement[0]+2*plane_displacement*plane_normal[0])*
			  (object->displacement[0]+2*plane_displacement*plane_normal[0])+
			  (object->displacement[1]+2*plane_displacement*plane_normal[1])*
			  (object->displacement[1]+2*plane_displacement*plane_normal[1])+
			  (object->displacement[2]+2*plane_displacement*plane_normal[2])*
			  (object->displacement[2]+2*plane_displacement*plane_normal[2])
			);
			if (object->diameter/2+distance > farthest_corner)
				farthest_corner = object->diameter/2+distance;
		}
	}
	image_size = (int)ceil(2*farthest_corner*scale)+1;
	if (image_size % 4)
		image_size += 4-image_size%4;
	return (image_size);
}

/*****************************************************************************
 * FUNCTION: resize_image
 * DESCRIPTION: Resizes main_image, and image2 if it is for 3-d display.
 * PARAMETERS: None
 * SIDE EFFECTS: The following global variables may be changed: depth_scale,
 *    main_image, image2, image_x, image_y, image2_x, image2_y, iw_valid,
 *    image_valid, image2_valid, icon_valid, object_list (projected flags).
 *    Any error will be displayed in the dialog window.
 * ENTRY CONDITIONS: The global variables object_list, scale, image_x, image_y,
 *    image2_x, image2_y, plane_displacement, plane_normal must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/6/94 for truecolor images by Dewey Odhner
 *    Modified: 1/14/00 closeup image size not changed by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::resize_image(void)
{
	int image_size, data_bytes;

	destroy_slice_list();
	image_size = get_image_size();
	data_bytes = 4*image_size*image_size;
	if (main_image == NULL)
		main_image = &ximage;
	if (main_image->data)
		free (main_image->data);
	main_image->data = (char *)malloc(data_bytes);
	if (main_image->data == NULL)
	{	report_malloc_error();
		main_image->width = main_image->height = 0;
		main_image->bytes_per_line = 0;
	}
	else
	{	main_image->width = main_image->height = image_size;
		main_image->bytes_per_line = image_size*pixel_bytes;
	}
	if (image_mode!=PROBE&& image_mode!=CLOSEUP)
	{	if (image2 == NULL)
			image2 = &ximage2;
		if (image2->data)
			free (image2->data);
		image2->data = (char *)malloc(data_bytes);
		if (image2->data == NULL)
		{	report_malloc_error();
			image2->width = image2->height = 0;
			image2->bytes_per_line = 0;
		}
		else
		{	image2->width = image2->height = image_size;
			image2->bytes_per_line = image_size*pixel_bytes;
		}
	}
	set_depth_scale();
}

/*****************************************************************************
 * FUNCTION: closest_object
 * DESCRIPTION: Returns the object number (not label) of the virtual object
 *    pointed to.  (The object number is a positive number that may change if
 *    another virtual object is created or destroyed.)
 * PARAMETERS:
 *    x, y: The image window coordinates of a pixel where the virtual object
 *       is visible on main_image or image2.
 *    image_number: 1 (main_image) or 2 (image2)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: The object number (not label) of the closest virtual object
 *    if it exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 3/27/95 secondary objects counted instead of
 *       separate_piece1, separate_piece2 by Dewey Odhner.
 *    Modified: 7/13/09 image_number passed as parameter by Dewey Odhner.
 *
 *****************************************************************************/
int cvRenderer::closest_object(int x, int y, int image_number)
{
	Shell *obj;
	int ox, oy, sofar, n, closest_n, num_of_reflections;

	/* convert to input pixels */
	if (anti_alias)
	{	x *= 2;
		y *= 2;
	}
	else
	{	x /= 2;
		y /= 2;
	}

	sofar = closest_n = 0;
	num_of_reflections = number_of_reflections();
	n = 1;
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (obj->reflection)
		{	ox = x-obj->reflection->main_image.image_location[0];
			oy = y-obj->reflection->main_image.image_location[1];
			if (anti_alias && obj->reflection->opacity==.5)
			{	ox /= 2;
				oy /= 2;
			}
			if (obj->reflection->on &&
					ox>=0 && ox<obj->reflection->main_image.image_size &&
					oy>=0 && oy<obj->reflection->main_image.image_size &&
					obj->reflection->main_image.z_buffer[oy][ox]>sofar &&
					(image_mode!=SEPARATE ||
						((obj!=separate_piece2 || image_number==2) &&
						(obj!=separate_piece1 || image_number==1))) &&
					(image_mode!=FUZZY_CONNECT ||
						(obj->secondary && image_number==2) ||
						(!obj->secondary && image_number==1)))
			{	sofar = obj->reflection->main_image.z_buffer[oy][ox];
				closest_n = num_of_reflections+1-n;
			}
			n++;
		}
	}
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	ox = x-obj->O.main_image.image_location[0];
		oy = y-obj->O.main_image.image_location[1];
		if (anti_alias && obj->O.opacity==.5)
		{	ox /= 2;
			oy /= 2;
		}
		if (obj->O.on && ox>=0 && ox<obj->O.main_image.image_size &&
				oy>=0 && oy<obj->O.main_image.image_size &&
				obj->O.main_image.z_buffer[oy][ox]>sofar &&
				(image_mode!=SEPARATE ||
					((obj!=separate_piece2 || image_number==2) &&
					(obj!=separate_piece1 || image_number==1))) &&
				(image_mode!=FUZZY_CONNECT ||
					(obj->secondary && image_number==2) ||
					(!obj->secondary && image_number==1)))
		{	sofar = obj->O.main_image.z_buffer[oy][ox];
			closest_n = n;
		}
		n++;
	}
	return (closest_n);
}

/*****************************************************************************
 * FUNCTION: object_volume
 * DESCRIPTION: Returns the volume of the shell.
 * PARAMETERS:
 *    shell_data: The shell to get the volume of; must be in memory.
 * SIDE EFFECTS: The volume will be stored with the object if it is not
 *    already.
 * ENTRY CONDITIONS: shell_data must be in memory.
 * RETURN VALUE: The volume of the shell
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/30/93 by Dewey Odhner
 *    Modified: 5/18/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
double object_volume(Shell_data *shell_data)
{
	unsigned long j;
	double volume;
	unsigned short *ptr1, *end_ptr, this_column;
	Classification_type data_class;

	if (!shell_data->volume_valid)
	{	data_class = st_cl(shell_data);
		if (data_class!=BINARY_B && data_class!=BINARY_A)
			return (0);

		/* compute volume */
		j = 0;
		end_ptr = shell_data->ptr_table[shell_data->slices*shell_data->rows];
		for (ptr1=shell_data->ptr_table[0]; ptr1<end_ptr; ptr1+=2)
		{
			this_column = data_class==BINARY_A? *ptr1&XMASK:
				((*ptr1&XMASK)<<1|(ptr1[1]&0x8000))!=0;
			while (*ptr1&PX)
				ptr1+=2;
			j += (data_class==BINARY_A? *ptr1&XMASK:
				((*ptr1&XMASK)<<1|(ptr1[1]&0x8000))!=0)-this_column+1;
		}
		volume = j*shell_data->file->file_header.str.xysize[0]*
			shell_data->file->file_header.str.xysize[1]*
			Slice_spacing(shell_data);

		if (shell_data->file->file_header.str.volume == NULL)
		{	shell_data->file->file_header.str.volume = (float *)malloc(
				shell_data->file->file_header.str.num_of_structures*
				sizeof(float));
			if (shell_data->file->file_header.str.volume == NULL)
				return (volume);
		}
		shell_data->file->file_header.str.volume[shell_data->shell_number] =
			(float)volume;
		shell_data->volume_valid = TRUE;
		if (shell_data->file->references ==
				shell_data->file->file_header.str.num_of_structures)
		{	shell_data->file->file_header.str.volume_valid = TRUE;
			for (int m=0; m<shell_data->file->references; m++)
				if (!shell_data->file->reference[m]->volume_valid)
					shell_data->file->file_header.str.volume_valid = FALSE;
		}
	}
	return shell_data->file->file_header.str.volume[shell_data->shell_number];
}
