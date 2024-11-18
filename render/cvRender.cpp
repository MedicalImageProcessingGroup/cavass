/*
  Copyright 1993-2016 Medical Image Processing Group
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


#include  "cavass.h"
#ifdef  Left
    #undef  Left
#endif
#ifdef  Right
    #undef  Right
#endif

#include "cvRender.h"
#include <ctype.h>


#define Malloc(ptr, type, num, bequest) \
{	ptr = (type *)malloc((num)*sizeof(type)); \
	if (ptr == NULL) \
	{	bequest; \
		return (1); \
	} \
}

extern "C" {
int VGetHeaderLength ( FILE* fp, int* hdrlen );
int VComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
	int* npoints );
}
void display_message(const char []);
void run_command(char cmnd[], bool fg);

static const int image_x=0, image_y=0, image2_x=0, image2_y=0, pixel_bytes=4;

cvRenderer::cvRenderer(char **file_list, int num_files, int icons)
{
	param_init();
	loadFiles(file_list, num_files, icons);
	set_colormap();
	mAux = NULL;
}


/*****************************************************************************
 * FUNCTION: param_init
 * DESCRIPTION: Loads files and initializes the global variables
 *    display, image_depth, pixel_bytes, ncolors, plane_normal,
 *    background, plane_transparency, mark_color,
 *    object_color_table and slice_color_table (allocated only),
 *    global_level, global_width, emission_power, t_shell_detail,
 *    surface_red_factor, surface_green_factor, surface_blue_factor,
 *    tissue_opacity, tissue_red, tissue_green, tissue_blue,
 *    surface_strength, inside_closeup, viewport_size, viewport_back,
 *    number_of_triangles.
 * PARAMETERS: None
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: project.cpp`triangle_table must be initialized.
 * RETURN VALUE: 0
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/26/06 by Dewey Odhner
 *    Modified: 2/26/07 to do no action if already called by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::param_init(void)
{
	object_list = NULL;
	slice_list = NULL;
	separate_piece1 = separate_piece2 = NULL;
	maximum_intensity_projection = 0;
	box = 0;
	line = 0;
	image_mode = WITH_ICON;
	image_valid = image2_valid = image3_valid = image4_valid = 0;
	icon_valid = 0;
	iw_valid = 0;
	colormap_valid = 0;
	marks = 0;
	gray_scale = 0;
	v_object_color_table = NULL;
	slice_color_table = NULL;
	scale = 0;
	depth_scale = 0;
	icon_scale = 0;
	glob_angle[0] = glob_angle[1] = glob_angle[2] = 0;
	glob_displacement[0] = glob_displacement[1] = glob_displacement[2] = 0;
	ambient.red = ambient.green = ambient.blue = 0;
	plane_normal[0] = plane_normal[1] = 0;
	plane_normal[2] = 1;
	plane_displacement = 0;
	line_angle[0] = line_angle[1] = 0;
	line_displacement[0] = line_displacement[1] = line_displacement[2] = 0;
	selected_object = 0;
	background.red = background.green = background.blue = 0;
	global_level = 50;
	global_width = 100;
	plane_transparency.red   = (unsigned short)(.8*65535);
	plane_transparency.green = (unsigned short)(.5*65535);
	plane_transparency.blue  = (unsigned short)(.8*65535);
	main_image = icon_image = image2 = image3 = image4 = NULL;
	mark_color.red = 65535;
	mark_color.green = mark_color.blue = 0;
	fade_edge = 0;
	surface_red_factor = surface_green_factor = surface_blue_factor =
		(float)MAX_SURFACE_FACTOR;
	tissue_opacity[0] = 0;
	tissue_opacity[1] = tissue_opacity[2] = .01f;
	tissue_opacity[3] = .4f;
	tissue_green[2] = tissue_blue[1] = tissue_blue[2] = 0;
	tissue_red[0] = tissue_green[0] =  tissue_blue[0] = tissue_red[1] =
		tissue_red[2] = tissue_red[3] = tissue_green[1] = tissue_green[3] =
		tissue_blue[3] = V_OBJECT_IMAGE_BACKGROUND-1;
	true_color = 0;
	surface_strength = 50;
	gray_interpolate = 1;
	label_slice = 0;
	emission_power = 1;
	surf_pct_power = 0;
	perspective = 0;
	axes = NULL;
	out_slice_spacing = 0;
	closeup_list = NULL;
	closeup_displacement[0] = closeup_displacement[1] =
		closeup_displacement[2] = 0;
	closeup_angle[0] = closeup_angle[2] = closeup_angle[1] = 0;
	closeup_scale = 0;
	icons_exist = 0;
	anti_alias = TRUE;
	plane = FALSE;
	inside_closeup = SWITCH_ON;
	viewport_size = .5;
	viewport_back = Z_BUFFER_LEVELS*2/3;
	glob_plane_displacement[0] = glob_plane_displacement[1] =
		glob_plane_displacement[2] = 0;
	slice_section_flag = 0;
	object_of_point = NULL;
	t_shell_detail = 7;
	count_triangles();
	static_slice_buffer = NULL;
	slice_buffer_size = 0;
	object_color_table = NULL;
	ncolors = 1;
	new_color();
	for (int j=0; j<4; j++)
	{
		old_materl_opacity[j] = -1;
		old_materl_red[j] = old_materl_green[j] = old_materl_blue[j] = 0;
		old_emis_power = 0;
		old_surf_ppower= -1;
		old_surf_strenth= -1;
		old_surf_red_factr = old_surf_green_factr = old_surf_blue_factr = 0;
		old_ambient_light.red = old_ambient_light.green =
			old_ambient_light.blue = 0;
	}
	no_interrupt = 0;
	interrupt_flag = &no_interrupt;
	file_list = NULL;
	file_list_size = 0;
	memset(&ximage, 0, sizeof(ximage));
	memset(&ximage2, 0, sizeof(ximage2));
	memset(&xicon, 0, sizeof(xicon));
	buffer_row_ptr = NULL;
	last_out_ptr = 0;
	this_ptr_ptr = NULL;
	end_ptr_ptr = NULL;
	this_slice = this_row = slice_points = 0;
	slice_point_list = NULL;
	memset(&mPrism, 0, sizeof(Prism));
	original_object = NULL;
	original_object1 = original_object2 = NULL;
	original_objects_set = FALSE;
	out_data = NULL;
	buffer_out_ptr = NULL;
	p_o_data = NULL;
	p_buff_out_ptr = NULL;
	inside_plane_G_code = outside_plane_G_code = 0;
	S_ptr_table = NULL;
	column_n_factor = slice_n_factor = row_n_factor = object_plane_distance= 0;
	new_ptr_table = NULL;
	clmns = rws = slcs = 0;
	inside = 0;
	out_buffer_size = 0;
	voxels_in_out_buffer = 0;
	adjacent_vois = NULL;
	S_row = NULL;
	p_o_buffer_size = 0;
	p_voxs_in_out_buffer = 0;
	color_table = NULL;
	old_scene[0] = old_scene[1] = 0;
	memset(scene, 0, sizeof(scene));
	last_slice = &null_slice;
	memset(&null_slice, 0, sizeof(null_slice));
	old_sl_data[0] = old_sl_data[1] = NULL;
	old_sl_width[0] = old_sl_width[1] = 0;
	memset(old_ends, 0, sizeof(old_ends));
	old_interpolate[0] = old_interpolate[1] = 0;
	error_flag = false;
	cut_count = 0;
	color_mode = PLANE_MODE;
	measurement_point= NULL;
	measurement_point_array_size = 0;
	nmeasurement_points = 0;
	new_mark_is_set  = false;
	mark_is_erased   = false;
	measurement_displayed = false;
	marked_object    = NULL;
	box_axis_label_loc[0].x = box_axis_label_loc[1].x = box_axis_label_loc[2].x
		= -1;
}



/*****************************************************************************
 * FUNCTION: show_icons
 * DESCRIPTION: Renders the icons, projecting them to the objects' icon image
 *    buffers, and combines the images into icon_image.
 * PARAMETERS:
 *    event_priority: Returns the priority of any event.  Takes no parameters.
 * SIDE EFFECTS: Any effects of event_priority may occur.
 *    The following global variables may be changed: current_window,
 *    object_list, icon_valid, iw_valid, message_displayed; if used with the
 *    Volume version of project, the global variable true_color and
 *    the static variables in pct_project.c may also be changed.
 *    An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init (if the Manipulate version of project
 *    is used) or vol_init (if the Volume version of project is used) must be
 *    made first.
 * RETURN VALUE: DONE, INTERRUPT3, or EROR.
 * EXIT CONDITIONS: Returns INTERRUPT3 if event_priority returns FIRST.
 *    If a memory allocation fails, EROR will be returned.
 *    Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 8/26/91 by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to display error from project() by Dewey Odhner
 *    Modified: 2/15/01 icons_exist checked before using icon by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::show_icons(Priority (*event_priority)(cvRenderer *))
{
	int image_status;
	Shell *object;

	if (!icons_exist)
		return (DONE);
	for (object=object_list; object!=NULL; object=object->next)
	{	if (object->O.on && object->O.icon.projected!=ICON)
		{	image_status = project(object, &object->icon_data, event_priority);
			switch (image_status)
			{	case 0:
					break;
				case 401:
					return (INTERRUPT3);
				default:
					display_error(image_status);
					return (EROR);
			}
		}
		if (object->reflection && object->reflection->on &&
				object->reflection->icon.projected!=ICON)
		{	image_status = render_reflection(object, &object->icon_data,
				event_priority);
			switch (image_status)
			{	case 0:
					break;
				case 401:
					return (INTERRUPT3);
				default:
					display_error(image_status);
					return (EROR);
			}
		}
	}
	return (make_image(icon_image, event_priority));
}

/*****************************************************************************
 * FUNCTION: main_image_ok
 * DESCRIPTION: Checks whether the main image projected flag of a virtual
 *    object is valid.
 * PARAMETERS:
 *    vobj: The virtual object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable anti_alias must be properly set.
 * RETURN VALUE: Non-zero if the main image projected flag is valid.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::main_image_ok(Virtual_object *vobj)
{
	return (
		anti_alias
		?	vobj->opacity!=.5
			?	vobj->main_image.projected==ANTI_ALIAS
			:	vobj->main_image.projected==ONE_TO_ONE
		:	vobj->main_image.projected==PIXEL_REPLICATE
	);
}

/*****************************************************************************
 * FUNCTION: show_objects
 * DESCRIPTION: Renders the main objects, projecting them to the objects'
 *    image buffers, and combines the images into main_image.
 * PARAMETERS:
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed: current_window,
 *    object_list, image_valid, image2_valid, iw_valid, message_displayed; if
 *    used with the Volume version of project, the global variable
 *    true_color and the static variables in pct_project.c may also be changed.
 *    An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init (if the Manipulate version of project
 *    is used) or vol_init (if the Volume version of project is used) must be
 *    made first.
 * RETURN VALUE: DONE, INTERRUPT3, or EROR.
 * EXIT CONDITIONS: Returns INTERRUPT3 if event_priority returns FIRST.
 *    If a memory allocation fails, EROR will be returned.
 *    Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to display error from project() by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::show_objects(Priority (*event_priority)(cvRenderer *))
{
	Function_status image_status;
	int error_code;
	Shell *object;

	for (object=object_list; object!=NULL; object=object->next)
	{	if (object->O.on && !main_image_ok(&object->O))
		{	error_code = project(object, &object->main_data, event_priority);
			switch (error_code)
			{	case 0:
					break;
				case 401:
					return (INTERRUPT3);
				default:
					display_error(error_code);
					return (EROR);
			}
		}
		if (object->reflection && object->reflection->on &&
				!main_image_ok(object->reflection))
		{	error_code = render_reflection(object, &object->main_data,
				event_priority);
			switch (error_code)
			{	case 0:
					break;
				case 401:
					return (INTERRUPT3);
				default:
					display_error(error_code);
					return (EROR);
			}
		}
	}
	image_status = make_image(main_image, event_priority);
	if (image_status == DONE)
		image_valid = TRUE;
	else
		return (image_status);
	if (image_mode==SEPARATE || image_mode==FUZZY_CONNECT)
	{	image_status = make_image(image2, event_priority);
		if (image_status == DONE)
			image2_valid = TRUE;
	}
	return (image_status);
}

/*****************************************************************************
 * FUNCTION: project
 * DESCRIPTION: Renders an object, projecting it to its image buffer.
 * PARAMETERS:
 *    object: The object to be projected.  If the image buffer is not big
 *       enough, it will be changed.  Memory must be allocated at
 *       _image.image, at _image.image[0], at _image.z_buffer, and at
 *       _image.z_buffer[0] for a _image.image_size square image, where
 *       _image is object->O.main_image if object_data is &object->data,
 *       object->O.icon if object_data is &object->icon_data.
 *       If it is BINARY, the object must be in memory; image depth is 8 bits.
 *    object_data: &object->data if the main object is to be projected;
 *       &object->icon_data if its icon is to be projected.
 *    event_priority: Returns the priority of any event; will be dereferenced
 *       only if non-null.  No parameters are passed to event_priority.
 * SIDE EFFECTS: Any effects of event_priority will occur.
 *    The following global variables may be changed: object_list, iw_valid.
 * ENTRY CONDITIONS: If event_priority is non-null, a call to manip_init must
 *    be made first.  The global variables anti_alias, scale, icon_scale,
 *    depth_scale, glob_angle, emission_power, surf_pct_power,
 *    t_shell_detail must be valid.
 * RETURN VALUE:
 *    0: successful
 *    1: memory allocation failure
 *    2: read error
 *    4: file opening error
 *    5: seek error
 *    401: event_priority returns FIRST
 * EXIT CONDITIONS: Returns 0 immediately if object->O is off.
 *    Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 12/4/91 by Dewey Odhner
 *    Modified: 2/22/94 event_priority called directly instead of
 *       manip_peek_event by Dewey Odhner
 *    Modified: 3/1/94 to return int by Dewey Odhner
 *    Modified: 3/16/94 to call VRender by Dewey Odhner
 *    Modified: 7/6/94 not to fade icons by Dewey Odhner
 *    Modified: 7/6/94 for maximum intensity projection by Dewey Odhner
 *    Modified: 4/25/96 emission_power, surf_pct_power added by Dewey Odhner
 *    Modified: 10/5/98 image size enlarged for margin patch by Dewey Odhner
 *    Modified: 11/19/98 perspective used by Dewey Odhner
 *    Modified: 10/3/03 t_shell_detail used by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::project(Shell *object, Shell_data *object_data,
	Priority (*event_priority)(cvRenderer *))
{
	double projection_offset[3], projection_matrix[3][3], center[3],
		rotation_matrix[3][3], dimin;
	int j, k, status, obj_pixel_bytes=0, *angle_shade;
	Display_mode mode;
	Object_image *object_image;
	Classification_type object_class;
	Rendering_parameters rend_params;

	rend_params.ambient = ambient;
	rend_params.fade_edge = object_data==&object->icon_data? FALSE: fade_edge;
	rend_params.surface_red_factor = surface_red_factor;
	rend_params.surface_green_factor = surface_green_factor;
	rend_params.surface_blue_factor = surface_blue_factor;
	for (j=0; j<4; j++)
	{	rend_params.tissue_opacity[j] = tissue_opacity[j];
		rend_params.tissue_red[j] = tissue_red[j];
		rend_params.tissue_green[j] = tissue_green[j];
		rend_params.tissue_blue[j] = tissue_blue[j];
	}
	rend_params.surface_strength = surface_strength;
	rend_params.emission_power = emission_power;
	rend_params.surf_pct_power = surf_pct_power;
	rend_params.maximum_intensity_projection = maximum_intensity_projection;
	rend_params.check_event = event_priority;
	object_class = st_cl(object_data);
	if (!object->O.on)
		return (0);
	if ((object_class==BINARY_B||object_class==T_SHELL) &&
			object->O.opacity!=.5)
		rend_params.tissue_opacity[0] = object->O.opacity;
	mode =
		object_data==&object->icon_data
		?	ICON
		:	anti_alias
			?	object->O.opacity!=.5
				?	ANTI_ALIAS
				:	ONE_TO_ONE
			:	PIXEL_REPLICATE;
	rend_params.perspective = mode==ICON? 0: perspective;
	rend_params.t_shell_detail = t_shell_detail;
	if (event_priority && event_priority(this)==FIRST)
		return (401);
	get_structure_transformation(projection_matrix, projection_offset,
		rotation_matrix, center, object, mode, FALSE);
	if (event_priority && event_priority(this)==FIRST)
		return (401);
	if (!object->O.shade_lut_computed)
	{	VComputeShadeLut(object->O.shade_lut, object->O.diffuse_exponent,
			object->O.diffuse_n, object->O.specular_fraction,
			object->O.specular_exponent, object->O.specular_n);
		object->O.shade_lut_computed = TRUE;
	}
	object_image = mode==ICON? &object->O.icon: &object->O.main_image;
	object_image->projected = BAD;
	j = (int)ceil(object->diameter*buffer_scale(mode))+1;
	if (mode==ANTI_ALIAS && j&1)
		j++;
	if (object_class!=BINARY_A &&
			(rend_params.fade_edge || need_patch(projection_matrix)))
		j += 2;
	if (object_image->image_size != j)
	{	if (object_image->image_size != 0)
		{	free(object_image->image[0]);
			free(object_image->z_buffer[0]);
			free(object_image->image);
			free(object_image->z_buffer);
			if (object_class!=BINARY_B && object_class!=BINARY_A &&
					object_class!=T_SHELL)
			{	free(object_image->opacity_buffer[0]);
				free(object_image->opacity_buffer);
				free(object_image->likelihood_buffer[0]);
				free(object_image->likelihood_buffer);
			}
			object_image->image_size = 0;
		}
		Malloc(object_image->image, char *, j+1, {})
		Malloc(object_image->z_buffer, int *, j+1, free(object_image->image))
		switch (object_class)
		{	case BINARY_A:
			case BINARY_B:
			case T_SHELL:
				obj_pixel_bytes = 1;
				break;
			case GRADIENT:
				obj_pixel_bytes = sizeof(Pixel_unit);
				object_image->pixel_units = 1;
				break;
			case PERCENT:
			case DIRECT:
				obj_pixel_bytes = 3*sizeof(Pixel_unit);
				object_image->pixel_units = 3;
				break;
		}
		Malloc(object_image->image[0], char, j*j*obj_pixel_bytes,
			free(object_image->image); free(object_image->z_buffer))
		Malloc(object_image->z_buffer[0], int, j*j,
			free(object_image->image[0]); free(object_image->image);
			free(object_image->z_buffer))
		if (object_class!=BINARY_B && object_class!=BINARY_A &&
				object_class!=T_SHELL)
		{	Malloc(object_image->opacity_buffer, unsigned char *, j+1,
				free(object_image->image[0]); free(object_image->image);
				free(object_image->z_buffer[0]); free(object_image->z_buffer))
			Malloc(object_image->opacity_buffer[0], unsigned char, j*j,
				free(object_image->image[0]); free(object_image->image);
				free(object_image->z_buffer[0]); free(object_image->z_buffer);
				free(object_image->opacity_buffer))
			Malloc(object_image->likelihood_buffer, unsigned char *, j+1,
				free(object_image->image[0]); free(object_image->image);
				free(object_image->z_buffer[0]); free(object_image->z_buffer);
				free(object_image->opacity_buffer[0]);
				free(object_image->opacity_buffer))
			Malloc(object_image->likelihood_buffer[0], unsigned char, j*j,
				free(object_image->image[0]); free(object_image->image);
				free(object_image->z_buffer[0]); free(object_image->z_buffer);
				free(object_image->opacity_buffer[0]);
				free(object_image->opacity_buffer);
				free(object_image->likelihood_buffer))
		}
		object_image->image_size = j;
		for (j=1; j<=object_image->image_size; j++)
		{	object_image->image[j] = object_image->image[0]+
				j*object_image->image_size*obj_pixel_bytes;
			object_image->z_buffer[j] =
				object_image->z_buffer[0]+j*object_image->image_size;
			if (object_class!=BINARY_B && object_class!=BINARY_A &&
					object_class!=T_SHELL)
			{	object_image->opacity_buffer[j] =
					object_image->opacity_buffer[0]+j*object_image->image_size;
				object_image->likelihood_buffer[j] =
					object_image->likelihood_buffer[0]+
					j*object_image->image_size;
			}
		}
	}
	if (object_class==BINARY_B || object_class==BINARY_A ||
			object_class==T_SHELL)
		for (j=0; j<object_image->image_size; j++)
		{	memset(object_image->image[j], OBJECT_IMAGE_BACKGROUND,
				object_image->image_size);
			memset(object_image->z_buffer[j], 0,
				object_image->image_size*sizeof(int));
		}
	else
		for (j=0; j<object_image->image_size; j++)
		{	for (k=0; k<object_image->image_size*object_image->pixel_units;k++)
				((Pixel_unit *)object_image->image[j])[k] =
					V_OBJECT_IMAGE_BACKGROUND;
			memset(object_image->z_buffer[j], 0,
				object_image->image_size*sizeof(int));
			memset(object_image->opacity_buffer[j], 0,
				object_image->image_size);
			memset(object_image->likelihood_buffer[j], 0,
				object_image->image_size);
		}
	dimin = (100-perspective)/
		(100-perspective*(.5+center[2]/Z_BUFFER_LEVELS));
	switch (mode)
	{
		case ANTI_ALIAS:
			object_image->image_location[0] = (int)(
				2*rint(.5*(dimin*center[0]-.5*object_image->image_size)) );
			object_image->image_location[1] = (int)(
				2*rint(.5*(dimin*center[1]-.5*object_image->image_size)) );
			break;
		case ONE_TO_ONE:
			object_image->image_location[0] = (int)(
				2*rint(dimin*center[0]-.5*object_image->image_size) );
			object_image->image_location[1] = (int)(
				2*rint(dimin*center[1]-.5*object_image->image_size) );
			break;
		case PIXEL_REPLICATE:
			object_image->image_location[0] = (int)(
				rint(dimin*center[0]-.5*object_image->image_size) );
			object_image->image_location[1] = (int)(
				rint(dimin*center[1]-.5*object_image->image_size) );
			break;
		case ICON:
			object_image->image_location[0] = (int)(
				rint(center[0]-.5*object_image->image_size) );
			object_image->image_location[1] = (int)(
				rint(center[1]-.5*object_image->image_size) );
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
	rend_params.shade_lut = object->O.shade_lut;
	VGetAngleShades(&angle_shade, rotation_matrix, &rend_params,
		SHADE_LUT_FLAG, object_class,
		(object_class==BINARY_B||object_class==T_SHELL)&& object->O.opacity<1);
	object_image->projected = mode;
	status = VRender(object_data, object_image,
				projection_matrix, projection_offset, angle_shade,
				&rend_params, AMBIENT_FLAG+FADE_EDGE_FLAG+
				SURFACE_RGB_FACTOR_FLAG+TISSUE_OPACITY_FLAG+TISSUE_RGB_FLAG+
				SURFACE_STRENGTH_FLAG+MIP_FLAG+CHECK_EVENT_FLAG+
				EMISSION_POWER_FLAG+SURF_PCT_POWER_FLAG+PERSPECTIVE_FLAG+
				T_SHELL_DETAIL_FLAG);
	check_true_color();
	if (status)
		object_image->projected = BAD;
	return (status);
}

/*****************************************************************************
 * FUNCTION: loadFile(const char filename[])
 * DESCRIPTION: Loads the .BS[02] or .SH0 or .IM0 file specified.
 * PARAMETERS:
 *    filename: Name of the file to be loaded
 * SIDE EFFECTS: Sets the global variables scale, depth_scale, icon_scale,
 *    display_area_width, display_area_height, display_area_x, display_area_y,
 *    image_x, image_y, icon_area_width, icon_area_height, icon_area_x,
 *    icon_area_y, icon_image_x, icon_image_y, image_mode, image2,
 *    display_area2, image_valid, image2_valid, icon_valid, selected_object,
 *    glob_displacement, icons_exist, file_list, file_list_size
 * ENTRY CONDITIONS: The global variables object_color_table, slice_color_table
 *    must point to ncolors Color_table_row's of allocated memory.
 *    The global variable object_list must point to a
 *    valid Shell or be NULL. (A valid Shell must have its 'next' point to a
 *    valid Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variable ncolors must be
 *    greater than each object color number, which must be at least 0.
 *    The variables file_list, file_list_size must be properly set.
 * RETURN VALUE: 0 if successful
 * EXIT CONDITIONS: If the file cannot be loaded, all files will be unloaded.
 *    Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 11/27/06 by Dewey Odhner
 *    Modified: 3/8/07 for multiple files by Dewey Odhner.
 *
 *****************************************************************************/
int cvRenderer::loadFile(const char filename[])
{
	param_init();
	char **t_file_list = (char **)(file_list_size==0? malloc(sizeof(char *)):
		realloc(file_list, sizeof(char *)*(file_list_size+1)));
	if (t_file_list == NULL)
		return 1;
	t_file_list[file_list_size] =
		(char *)malloc(strlen(filename)<299? 300: strlen(filename)+1);
	if (t_file_list[file_list_size] == NULL)
		return 1;
	file_list = t_file_list;
	strcpy(file_list[file_list_size++], filename);
	while (object_list)
		remove_object(object_list);
	int er = loadFiles(file_list, file_list_size, FALSE);
	set_colormap();
	return er;
}

/*****************************************************************************
 * FUNCTION: unloadFiles(void)
 * DESCRIPTION: Unloads files.
 * PARAMETERS: None
 * SIDE EFFECTS: Sets object_list to NULL.
 * ENTRY CONDITIONS: The global variable object_list must point to a
 *    valid Shell or be NULL. (A valid Shell must have its 'next' point to a
 *    valid Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 3/8/07 by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::unloadFiles(void)
{
	if (file_list_size)
	{
		while (file_list_size--)
			free(file_list[file_list_size]);
		free(file_list);
	}
	while (object_list)
		remove_object(object_list);
}

/*****************************************************************************
 * FUNCTION: ~cvRenderer(void)
 * DESCRIPTION: Frees memory used by rendering functions.
 * PARAMETERS: None
 * SIDE EFFECTS: Sets object_list to NULL.
 *    The variables file_list_size may change.
 * ENTRY CONDITIONS: The global variables object_color_table, main_image->data
 *    must point to allocated memory.
 *    The global variable object_list must point to a
 *    valid Shell or be NULL. (A valid Shell must have its 'next' point to a
 *    valid Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)
 *    The variables file_list, file_list_size must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/28/06 by Dewey Odhner
 *    Modified: 3/8/07 for multiple files by Dewey Odhner.
 *
 *****************************************************************************/
cvRenderer::~cvRenderer(void)
{
	if (mAux != NULL) {
		cvRenderer*  tmp = mAux;
		mAux = NULL;
		delete tmp;
	}
	unloadFiles();
	if (main_image && main_image->data)
		free(main_image->data);
	main_image = NULL;
	if (icon_image && icon_image->data)
		free(icon_image->data);
	icon_image = NULL;
	if (image2 && image2->data)
		free(image2->data);
	image2 = NULL;
	if (object_color_table)
		free(object_color_table);
	object_color_table = NULL;
	if (old_scene[0])
		free(old_scene[0]);
	if (old_scene[1])
		free(old_scene[1]);
	if (measurement_point_array_size)
		free(measurement_point);
}

/*****************************************************************************
 * FUNCTION: cvCheckInterrupt
 * DESCRIPTION: Returns IGNOR unless *interrupt_flag is set.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: interrupt_flag must be set to a valid int address.
 * RETURN VALUE: IGNOR or FIRST
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/2/07 by Dewey Odhner
 *
 *****************************************************************************/
Priority cvRenderer::cvCheckInterrupt ( cvRenderer *ths ) {
	return (*ths->interrupt_flag? FIRST: IGNOR);
}

/*****************************************************************************
 * FUNCTION: render(int *interrupt_flg)
 * DESCRIPTION: Renders the main objects, projecting them to the objects'
 *    image buffers, and combines the images into main_image.
 * PARAMETERS:
 *    interrupt_flg: Variable to check if rendering is to be interrupted.
 * SIDE EFFECTS: The following global variables may be changed:
 *    object_list, image_valid, interrupt_flag.
 * ENTRY CONDITIONS: A call to loadFile(const char filename[]) must be
 *    made first.
 * RETURN VALUE: Reference to main_image
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/2/07 by Dewey Odhner
 *
 *****************************************************************************/
XImage& cvRenderer::render ( int *interrupt_flg ) {
	interrupt_flag = interrupt_flg;
	XImage & im = render();
	interrupt_flag = &no_interrupt;
	return im;
}

/*****************************************************************************
 * FUNCTION: render()
 * DESCRIPTION: Renders the main objects, projecting them to the objects'
 *    image buffers, and combines the images into main_image.
 * PARAMETERS: None
 * SIDE EFFECTS: The following global variables may be changed:
 *    object_list, image_valid.
 * ENTRY CONDITIONS: A call to loadFile(const char filename[]) must be
 *    made first.  interrupt_flag must be set to a valid int address.
 * RETURN VALUE: Reference to main_image
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 12/7/06 by Dewey Odhner
 *    Modified: 2/6/07 project return value checked by Dewey Odhner.
 *    Modified: 10/24/07 colormap set by Dewey Odhner
 *
 *****************************************************************************/
XImage& cvRenderer::render ( void ) {
	Shell *object;
	int error_code;

	image_valid = FALSE;
	error_code = 0;
	for (object=object_list; object!=NULL; object=object->next)
	{
		if (error_code==0 && object->O.on)
			error_code= project(object, &object->main_data, &cvRenderer::cvCheckInterrupt);
		if (error_code==0 && object->reflection && object->reflection->on)
			error_code = render_reflection(object, &object->main_data,
				&cvRenderer::cvCheckInterrupt);
	}
	if (!colormap_valid)
		set_colormap();
	if (error_code==0 && make_image(main_image, &cvRenderer::cvCheckInterrupt) == DONE)
		image_valid = TRUE;
	if (image_valid && st_cl(&object_list->main_data)!=PERCENT &&
			st_cl(&object_list->main_data)!=DIRECT)
	{
		image_valid = FALSE;
		unsigned int *j = (unsigned int *)main_image->data;
		for (int k=0; k<3*main_image->width*main_image->height; j++,k+=3)
		{
			unsigned int p = *j;
			main_image->data[k+0] = red_map[p&255];
			main_image->data[k+1] = green_map[(p>>8)&255];
			main_image->data[k+2] = blue_map[(p>>16)&255];
		}
	}
	return *main_image;
}

/*****************************************************************************
 * FUNCTION: get_recolored_image( int& w, int& h )
 * DESCRIPTION: Combines the main object images into main_image.
 * PARAMETERS:
      w, h: width and height of image in pixels will be assigned here.
 * SIDE EFFECTS: The following global variables may be changed:
 *    colormap_valid, image_valid.
 * ENTRY CONDITIONS: A call to loadFile(const char filename[]) must be
 *    made first.  interrupt_flag must be set to a valid int address.
 * RETURN VALUE: Pointer to main_image data
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 11/15/07 by Dewey Odhner
 *
 *****************************************************************************/
char* cvRenderer::get_recolored_image( int& w, int& h ) {
	image_valid = FALSE;
	if (!colormap_valid)
		set_colormap();
	if (make_image(main_image, &cvRenderer::cvCheckInterrupt) == DONE)
		image_valid = TRUE;
	if (image_valid && st_cl(&object_list->main_data)!=PERCENT &&
			st_cl(&object_list->main_data)!=DIRECT)
	{
		image_valid = FALSE;
		unsigned int *j = (unsigned int *)main_image->data;
		for (int k=0; k<3*main_image->width*main_image->height; j++,k+=3)
		{
			unsigned int p = *j;
			main_image->data[k+0] = red_map[p&255];
			main_image->data[k+1] = green_map[(p>>8)&255];
			main_image->data[k+2] = blue_map[(p>>16)&255];
		}
	}
	w = main_image->width;
	h = main_image->height;
	return main_image->data;
}

/*****************************************************************************
 * FUNCTION: get_recolored_image2( int& w, int& h )
 * DESCRIPTION: Combines the main object images into main_image.
 * PARAMETERS:
      w, h: width and height of image in pixels will be assigned here.
 * SIDE EFFECTS: The following global variables may be changed:
 *    colormap_valid, image_valid.
 * ENTRY CONDITIONS: A call to loadFile(const char filename[]) must be
 *    made first.  interrupt_flag must be set to a valid int address.
 * RETURN VALUE: Pointer to main_image data
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 5/7/09 by Dewey Odhner
 *
 *****************************************************************************/
char* cvRenderer::get_recolored_image2( int& w, int& h ) {
	image2_valid = FALSE;
	if (!colormap_valid)
		set_colormap();
	if (image_mode == SLICE)
	{
		do_plane_slice();
		Slice_image **image_ptr;
		for (image_ptr= &slice_list; (*image_ptr)->next;
				image_ptr=&(*image_ptr)->next)
			;
		w = (*image_ptr)->image->width;
		h = (*image_ptr)->image->height;
		image2_valid = FALSE;
		unsigned int *j = (unsigned int *)(*image_ptr)->image->data;
		if (true_color)
			for (int k=0; k<3*image2->width*image2->height; j++,k+=3)
			{
				unsigned int p = *j;
				image2->data[k+0] = p&255;
				image2->data[k+1] = (p>>8)&255;
				image2->data[k+2] = (p>>16)&255;
			}
		else
			for (int k=0; k<3*image2->width*image2->height; j++,k+=3)
			{
				unsigned int p = *j;
				image2->data[k+0] = red_map[p&255];
				image2->data[k+1] = green_map[(p>>8)&255];
				image2->data[k+2] = blue_map[(p>>16)&255];
			}
		return image2->data;
	}
	if (make_image(image2, &cvRenderer::cvCheckInterrupt) == DONE)
		image2_valid = TRUE;
	if (image2_valid && st_cl(&object_list->main_data)!=PERCENT &&
			st_cl(&object_list->main_data)!=DIRECT)
	{
		image2_valid = FALSE;
		unsigned int *j = (unsigned int *)image2->data;
		for (int k=0; k<3*image2->width*image2->height; j++,k+=3)
		{
			unsigned int p = *j;
			image2->data[k+0] = red_map[p&255];
			image2->data[k+1] = green_map[(p>>8)&255];
			image2->data[k+2] = blue_map[(p>>16)&255];
		}
	}
	w = image2->width;
	h = image2->height;
	return image2->data;
}

char* cvRenderer::render2 ( int& w, int& h, int& interrupt_flag ) {
    XImage  xi = render( &interrupt_flag );
    w = xi.width;
    h = xi.height;
    return xi.data;
}

/*****************************************************************************
 * FUNCTION: setScale(double sc)
 * DESCRIPTION: Resizes main_image
 * PARAMETERS:
 *    sc: scale of the main image in pixel/mm
 * SIDE EFFECTS: The following global variables may be changed: depth_scale,
 *    main_image, image2, image_x, image_y, image2_x, image2_y, iw_valid,
 *    image_valid, image2_valid, icon_valid, object_list (projected flags).
 * ENTRY CONDITIONS: The global variables object_list, scale, image_x, image_y,
 *    image2_x, image2_y, plane_displacement, plane_normal must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 12/8/06 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setScale(double sc)
{
	scale = sc;
	resize_image();
}

/*****************************************************************************
 * FUNCTION: getScale(void)
 * DESCRIPTION: Returns scale of the main image in pixel/mm
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable scale must be valid.
 * RETURN VALUE: Scale of the main image in pixel/mm
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 12/8/06 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getScale(void)
{
	return scale;
}

/*****************************************************************************
 * FUNCTION: Antialias(void)
 * DESCRIPTION: Sets the anti_alias flag to do anti-aliasing.
 * PARAMETERS: None
 * SIDE EFFECTS: Sets the anti_alias flag to do anti-aliasing.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/17/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::Antialias(void)
{
	anti_alias = TRUE;
}

/*****************************************************************************
 * FUNCTION: PixelReplicate(void)
 * DESCRIPTION: Sets the anti_alias flag not to do anti-aliasing.
 * PARAMETERS: None
 * SIDE EFFECTS: Sets the anti_alias flag not to do anti-aliasing.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/17/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::PixelReplicate(void)
{
	anti_alias = FALSE;
}

/*****************************************************************************
 * FUNCTION: setObjectColor(unsigned char red, unsigned char green,
 *    unsigned char blue, int n)
 * DESCRIPTION: Sets the color of an object.
 * PARAMETERS:
 *    red, green, blue: color components to be assigned to the object.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: The global variables ncolors, colormap_valid may be changed.
 * ENTRY CONDITIONS: The global variable object_list must be valid except for
 *    obj->color.  The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 1/17/07 by Dewey Odhner
 *    Modified: 10/24/07 colormap_valid cleared by Dewey Odhner.
 *
 *****************************************************************************/
int cvRenderer::setObjectColor(unsigned char red, unsigned char green, unsigned char blue,
	int n)
{
	Virtual_object *obj;

	obj = object_from_label(n);
	if (obj == NULL)
		return -1;
	obj->rgb.red = red*257;
	obj->rgb.green = green*257;
	obj->rgb.blue = blue*257;
	set_color_number(obj);
	colormap_valid = FALSE;
	return 0;
}

/*****************************************************************************
 * FUNCTION: getObjectCount
 * DESCRIPTION: Returns the number of virtual objects.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The number of virtual objects.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 2/5/07 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::getObjectCount(void)
{
	return number_of_objects();
}

/*****************************************************************************
 * FUNCTION: getAntialias(void)
 * DESCRIPTION: Returns the anti_alias flag.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: anti_alias should be properly set.
 * RETURN VALUE: Returns the anti_alias flag.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/5/07 by Dewey Odhner
 *
 *****************************************************************************/
bool cvRenderer::getAntialias ( void )
{
	if (anti_alias)
	    return true;
	return false;
}

/*****************************************************************************
 * FUNCTION: setAntialias(bool state)
 * DESCRIPTION: Sets the anti_alias flag to do anti-aliasing or not.
 * PARAMETERS:
 *    state: value to assign to the anti_alias flag
 * SIDE EFFECTS: Sets the anti_alias flag.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/5/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAntialias ( bool state )
{
	anti_alias = state;
}

/*****************************************************************************
 * FUNCTION: getObjectColor(unsigned char & red, unsigned char & green,
 *    unsigned char & blue, int n)
 * DESCRIPTION: Gets the color of an object.
 * PARAMETERS:
 *    red, green, blue: color components assigned to the object.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/5/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::getObjectColor(
	unsigned char & red, unsigned char & green, unsigned char & blue, int n)
{
	Virtual_object *obj;

	obj = object_from_label(n);
	assert(obj != NULL);
	red = (unsigned char)(obj->rgb.red/256);
	green = (unsigned char)(obj->rgb.green/256);
	blue = (unsigned char)(obj->rgb.blue/256);
}

/*****************************************************************************
 * FUNCTION: getObjectSpecularFraction(int n)
 * DESCRIPTION: Gets the specular fraction of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: the specular fraction of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectSpecularFraction(int n)
{
	return object_from_label(n)->specular_fraction;
}

/*****************************************************************************
 * FUNCTION: setObjectSpecularFraction(double v, int n)
 * DESCRIPTION: Sets the specular fraction of an object.
 * PARAMETERS:
 *    v: The specular fraction to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *    Modified: 11/19/07 shade_lut_computed flag cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setObjectSpecularFraction(double v, int n)
{
	object_from_label(n)->specular_fraction = v;
	object_from_label(n)->shade_lut_computed = FALSE;
}

/*****************************************************************************
 * FUNCTION: setAllSpecularFraction(double v)
 * DESCRIPTION: Sets the specular fraction of an object.
 * PARAMETERS:
 *    v: The specular fraction to assign to object n.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/28/12 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAllSpecularFraction(double v)
{
	Shell *object;
	for (object=object_list; object!=NULL; object=object->next)
	{
		object->O.specular_fraction = v;
		object->O.shade_lut_computed = FALSE;
		if (object->reflection)
		{
			object->reflection->specular_fraction = v;
			object->reflection->shade_lut_computed = FALSE;
		}
	}
}

/*****************************************************************************
 * FUNCTION: getObjectSpecularExponent(int n)
 * DESCRIPTION: Gets the specular exponent of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: the specular exponent of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectSpecularExponent(int n)
{
	return object_from_label(n)->specular_exponent;
}

/*****************************************************************************
 * FUNCTION: setObjectSpecularExponent(double v, int n)
 * DESCRIPTION: Sets the specular exponent of an object.
 * PARAMETERS:
 *    v: The specular exponent to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *    Modified: 11/19/07 shade_lut_computed flag cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setObjectSpecularExponent(double v, int n)
{
	object_from_label(n)->specular_exponent = v;
	object_from_label(n)->shade_lut_computed = FALSE;
}

/*****************************************************************************
 * FUNCTION: setAllSpecularExponent(double v)
 * DESCRIPTION: Sets the specular exponent of an object.
 * PARAMETERS:
 *    v: The specular exponent to assign to object n.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/28/12 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAllSpecularExponent(double v)
{
	Shell *object;
	for (object=object_list; object!=NULL; object=object->next)
	{
		object->O.specular_exponent = v;
		object->O.shade_lut_computed = FALSE;
		if (object->reflection)
		{
			object->reflection->specular_exponent = v;
			object->reflection->shade_lut_computed = FALSE;
		}
	}
}

/*****************************************************************************
 * FUNCTION: getObjectSpecularDivisor(int n)
 * DESCRIPTION: Gets the specular divisor of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: the specular divisor of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectSpecularDivisor(int n)
{
	return object_from_label(n)->specular_n;
}

/*****************************************************************************
 * FUNCTION: setObjectSpecularDivisor(double v, int n)
 * DESCRIPTION: Sets the specular divisor of an object.
 * PARAMETERS:
 *    v: The specular divisor to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *    Modified: 11/19/07 shade_lut_computed flag cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setObjectSpecularDivisor(double v, int n)
{
	object_from_label(n)->specular_n = v;
	object_from_label(n)->shade_lut_computed = FALSE;
}

/*****************************************************************************
 * FUNCTION: setAllSpecularDivisor(double v)
 * DESCRIPTION: Sets the specular divisor of an object.
 * PARAMETERS:
 *    v: The specular divisor to assign to object n.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/28/12 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAllSpecularDivisor(double v)
{
	Shell *object;
	for (object=object_list; object!=NULL; object=object->next)
	{
		object->O.specular_n = v;
		object->O.shade_lut_computed = FALSE;
		if (object->reflection)
		{
			object->reflection->specular_n = v;
			object->reflection->shade_lut_computed = FALSE;
		}
	}
}

/*****************************************************************************
 * FUNCTION: getObjectDiffuseExponent(int n)
 * DESCRIPTION: Gets the diffuse exponent of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: the diffuse exponent of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectDiffuseExponent(int n)
{
	return object_from_label(n)->diffuse_exponent;
}

/*****************************************************************************
 * FUNCTION: setObjectDiffuseExponent(double v, int n)
 * DESCRIPTION: Sets the diffuse exponent of an object.
 * PARAMETERS:
 *    v: The diffuse exponent to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *    Modified: 11/19/07 shade_lut_computed flag cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setObjectDiffuseExponent(double v, int n)
{
	object_from_label(n)->diffuse_exponent = v;
	object_from_label(n)->shade_lut_computed = FALSE;
}

/*****************************************************************************
 * FUNCTION: setAllDiffuseExponent(double v)
 * DESCRIPTION: Sets the specular exponent of an object.
 * PARAMETERS:
 *    v: The specular exponent to assign to object n.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/28/12 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAllDiffuseExponent(double v)
{
	Shell *object;
	for (object=object_list; object!=NULL; object=object->next)
	{
		object->O.diffuse_exponent = v;
		object->O.shade_lut_computed = FALSE;
		if (object->reflection)
		{
			object->reflection->diffuse_exponent = v;
			object->reflection->shade_lut_computed = FALSE;
		}
	}
}

/*****************************************************************************
 * FUNCTION: getObjectDiffuseDivisor(int n)
 * DESCRIPTION: Gets the diffuse divisor of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: the diffuse divisor of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectDiffuseDivisor(int n)
{
	return object_from_label(n)->diffuse_n;
}

/*****************************************************************************
 * FUNCTION: setObjectDiffuseDivisor(double v, int n)
 * DESCRIPTION: Sets the diffuse divisor of an object.
 * PARAMETERS:
 *    v: The diffuse divisor to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 *    The global variable ncolors must be greater than each
 *    object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/7/07 by Dewey Odhner
 *    Modified: 11/19/07 shade_lut_computed flag cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setObjectDiffuseDivisor(double v, int n)
{
	object_from_label(n)->diffuse_n = v;
	object_from_label(n)->shade_lut_computed = FALSE;
}

/*****************************************************************************
 * FUNCTION: setAllDiffuseDivisor(double v)
 * DESCRIPTION: Sets the specular divisor of an object.
 * PARAMETERS:
 *    v: The specular divisor to assign to object n.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 6/28/12 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setAllDiffuseDivisor(double v)
{
	Shell *object;
	for (object=object_list; object!=NULL; object=object->next)
	{
		object->O.diffuse_n = v;
		object->O.shade_lut_computed = FALSE;
		if (object->reflection)
		{
			object->reflection->diffuse_n = v;
			object->reflection->shade_lut_computed = FALSE;
		}
	}
}

/*****************************************************************************
 * FUNCTION: getObjectOpacity(int n)
 * DESCRIPTION: Gets the opacity of an object.
 * PARAMETERS:
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: the opacity of object
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/22/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getObjectOpacity(int n)
{
	return object_from_label(n)->opacity;
}

/*****************************************************************************
 * FUNCTION: setObjectOpacity(double v, int n)
 * DESCRIPTION: Sets the opacity of an object.
 * PARAMETERS:
 *    v: The opacity to assign to object n.
 *    n: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/22/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setObjectOpacity(double v, int n)
{
	object_from_label(n)->opacity = (float)v;
}

/*****************************************************************************
 * FUNCTION: getMaterialOpacity(int n)
 * DESCRIPTION: Gets the opacity of a material.
 * PARAMETERS:
 *    n: The material number
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable tissue_opacity must be valid.
 * RETURN VALUE: the opacity of material n
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/22/07 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::getMaterialOpacity(int n)
{
	assert(n < 4);
	return tissue_opacity[n];
}

/*****************************************************************************
 * FUNCTION: setMaterialOpacity(double v, int n)
 * DESCRIPTION: Sets the opacity of a material.
 * PARAMETERS:
 *    v: The opacity to assign to material n.
 *    n: The material number.
 * SIDE EFFECTS: tissue_opacity[n] is set.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/23/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setMaterialOpacity(double v, int n)
{
	assert(n < 4);
	tissue_opacity[n] = (float)v;
}

/*****************************************************************************
 * FUNCTION: setMIP(bool state)
 * DESCRIPTION: Sets the MIP flag.
 * PARAMETERS:
 *    state: whether maximum intensity projection will be done
 * SIDE EFFECTS: maximum_intensity_projection is set.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/30/15 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::setMIP(bool state)
{
	maximum_intensity_projection = state;
}

/*****************************************************************************
 * FUNCTION: getBackgroundColor(unsigned char & red, unsigned char & green,
 *    unsigned char & blue)
 * DESCRIPTION: Gets the color of the background.
 * PARAMETERS:
 *    red, green, blue: color components assigned to the background.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable background must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/23/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::getBackgroundColor(
	unsigned char & red, unsigned char & green, unsigned char & blue)
{
	red = (unsigned char)(background.red/256);
	green = (unsigned char)(background.green/256);
	blue = (unsigned char)(background.blue/256);
}

/*****************************************************************************
 * FUNCTION: setBackgroundColor(unsigned char red, unsigned char green,
 *    unsigned char blue)
 * DESCRIPTION: Sets the color of the background.
 * PARAMETERS:
 *    red, green, blue: color components to be assigned to the background.
 * SIDE EFFECTS: The global variable background may be changed.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/23/07 by Dewey Odhner
 *    Modified: 10/24/07 colormap_valid cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setBackgroundColor(
	unsigned char red, unsigned char green, unsigned char blue)
{
	background.red = red*257;
	background.green = green*257;
	background.blue = blue*257;
	colormap_valid = FALSE;
}

/*****************************************************************************
 * FUNCTION: getAmbientLight(unsigned char & red, unsigned char & green,
 *    unsigned char & blue)
 * DESCRIPTION: Gets the color components of the ambient light.
 * PARAMETERS:
 *    red, green, blue: color components assigned to the ambient light.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable ambient must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/23/07 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::getAmbientLight(
	unsigned char & red, unsigned char & green, unsigned char & blue)
{
	red = (unsigned char)(ambient.red/256);
	green = (unsigned char)(ambient.green/256);
	blue = (unsigned char)(ambient.blue/256);
}

/*****************************************************************************
 * FUNCTION: setAmbientLight(unsigned char red, unsigned char green,
 *    unsigned char blue)
 * DESCRIPTION: Sets the color components of the ambient light.
 * PARAMETERS:
 *    red, green, blue: color components to be assigned to the ambient light.
 * SIDE EFFECTS: The global variable ambient may be changed.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/23/07 by Dewey Odhner
 *    Modified: 10/24/07 colormap_valid cleared by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::setAmbientLight(
	unsigned char red, unsigned char green, unsigned char blue)
{
	ambient.red = red*257;
	ambient.green = green*257;
	ambient.blue = blue*257;
	colormap_valid = FALSE;
}


#ifdef MARKS_IMPLEMENTED
@
/*****************************************************************************
 * FUNCTION: redisplay_slices
 * DESCRIPTION: Displays the slices.
 * PARAMETERS: None
 * SIDE EFFECTS: An error message may be displayed.
 *    The colormap may be changed.
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables slice_list, display_area2, display_area2_x,
 *    display_area2_y, display_area2_width, display_area2_height, argv0,
 *    windows_open, dialog_window, true_color
 *    must be appropriately initialized.
 *    All image_data in slice list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/1/93 by Dewey Odhner
 *    Modified: 6/7/93 to display scale on slices by Dewey Odhner
 *    Modified: 4/5/95 to display slice orientation labels by Dewey Odhner
 *    Modified: 7/8/99 to display marks by Dewey Odhner
 *    Modified: 5/2/00 colormap changed in true color mode by Dewey Odhner
 *    Modified: 4/13/04 VDisplayColorImage called by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::redisplay_slices()
{
	int img_xloc, img_yloc, win_xloc, win_yloc, width, height, j, m, mx, my;
	int (*temp_slice_mark)[3];
	Slice_image *this_slice;
	double sl_scale, rotation[3][3], mark_coords[3];
	long *avail_colorcells;

	for (this_slice=slice_list; this_slice; this_slice=this_slice->next)
	{	if (!this_slice->valid)
			display_slice(this_slice);
		if (this_slice->x > 0)
		{	img_xloc = 0;
			win_xloc = this_slice->x;
			width =
				display_area2_width>this_slice->x+this_slice->image->width
				?	this_slice->image->width
				:	display_area2_width-this_slice->x;
		}
		else
		{	img_xloc = -this_slice->x;
			win_xloc = 0;
			width =
				display_area2_width>this_slice->x+this_slice->image->width
				?	this_slice->x+this_slice->image->width
				:	display_area2_width;
		}
		if (this_slice->y > 0)
		{	img_yloc = 0;
			win_yloc = this_slice->y;
			height =
				display_area2_height>this_slice->y+this_slice->image->height
				?	this_slice->image->height
				:	display_area2_height-this_slice->y;
		}
		else
		{	img_yloc = -this_slice->y;
			win_yloc = 0;
			height =
				display_area2_height>this_slice->y+this_slice->image->height
				?	this_slice->y+this_slice->image->height
				:	display_area2_height;
		}
		if (this_slice->image->height>0 && this_slice->image->width>0)
		{
			for (j=0; j<2; j++)
				if (this_slice->on[j])
				{
					for (m=0; m<this_slice->object[j]->marks; m++)
					{
						/* convert from object coordinates to plan
						    coordinates */
						AtoM(rotation, this_slice->object[j]->angle[0],
							this_slice->object[j]->angle[1],
							this_slice->object[j]->angle[2]);
						matrix_vector_multiply(mark_coords, rotation,
							this_slice->object[j]->mark[m]);
						mark_coords[0] +=
							this_slice->object[j]->displacement[0];
						mark_coords[1] +=
							this_slice->object[j]->displacement[1];
						mark_coords[2] +=
							this_slice->object[j]->displacement[2];

						if (plan_to_slice_area_coords(&mx, &my,
								mark_coords, this_slice) == 0)
							if (mx>=win_xloc && mx<win_xloc+width &&
									my>=win_yloc && my<win_yloc+height)
								display_slice_mark(mx, my);
					}
				}
		}
	}
}

/*****************************************************************************
 * FUNCTION: display_slice_mark
 * DESCRIPTION: Puts a mark on display_area_2.
 * PARAMETERS:
 *    x, y: coordinates of the center of the mark.
 * SIDE EFFECTS: Puts a mark on display_area_2.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 *    The variables marks, object_color_table,
 *    display_area2, display_area2_width, display_area2_height must be valid.
 * RETURN VALUE: None.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 6/29/99 by Dewey Odhner.
 *
 *****************************************************************************/
display_slice_mark(x, y)
{
	int x1, y1, npoints;
	X_Point points[33];

	if (!marks)
		return;
	npoints = 0;
	for (x1= -3; x1<=3; x1++)
	{	if (x+x1 < 0)
			continue;
		if (x+x1 >= display_area2_width)
			break;
		for (y1= -3; y1<=3; y1++)
		{	if (y+y1 < 0)
				continue;
			if (y+y1 >= display_area2_height)
				break;
			if (x1+y1<2 && x1+y1>-2 || x1-y1<2 && x1-y1>-2)
			{
				points[npoints].x = x+x1;
				points[npoints].y = y+y1;
				npoints++;
			}
		}
	}
	XDrawPoints(display, display_area2, gc, points, npoints, CoordModeOrigin);
}
#endif


#define Left 1
#define Right -1
#define Posterior 2
#define Anterior -2
#define Head 3
#define Feet -3

#define Sl_Sc scene[sl_scene]
#define Floor(x) ((int)(x)<=(x)? (int)(x): (int)(x)-1)


/*****************************************************************************
 * FUNCTION: do_plane_slice
 * DESCRIPTION: Computes the slice image for the last slice in the slice list
 *    if it is not already valid.
 * PARAMETERS: None
 * SIDE EFFECTS: The static variables scene, old_scene,
 *    select_slice.c`gray_map_scales_displayed and the global variables
 *    message_displayed, slice_list may be changed.
 * ENTRY CONDITIONS: The global variables panel_command, panel_commands,
 *    object_list, slice_list, slice_color_table, gray_level,
 *    gray_width, pixel_bytes, and the static variable scene must be
 *    appropriately set.
 *    There must be a "SCENE..." switch or button; its switch_item_displayed
 *    indicates the selected scene.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 11/24/92 to use selected_object label by Dewey Odhner
 *    Modified: 4/1/93 to use slice_list by Dewey Odhner
 *    Modified: 4/19/93 to use panel_command by Dewey Odhner
 *    Modified: 6/11/93 to search for scene command by Dewey Odhner
 *    Modified: 8/30/93 to use selected_object instead of scene command
 *       by Dewey Odhner
 *    Modified: 2/16/04 two sl_data pointers kept by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::do_plane_slice()
{
	Shell *object;
	int sl_scene, j, k, pixel_byte;
	char *out_ptr;
	POINT3D ends[3]; /* 3 corners of plane in voxel coords */
	Slice_image *sl_image;

	if (slice_list==NULL)
	{
		if (image2==NULL || !create_slice_image(image2->width, 0, 0))
			return;
		memcpy(slice_list->plane_normal, plane_normal, sizeof(plane_normal));
		slice_list->plane_displacement = plane_displacement;
		slice_list->valid = slice_list->data_valid[0] =
			slice_list->data_valid[1] = FALSE;
		slice_list->on[0] = slice_list->on[1] = FALSE;
		object = actual_object(object_from_label(selected_object));
		if (object->O.on)
		{
			slice_list->object[0] = object;
			slice_list->on[0] = TRUE;
			sl_scene = 1;
		}
		else
			sl_scene = 0;
		for (object=object_list; object&&sl_scene<2; object=object->next)
			if (object->O.on && (sl_scene==0||object!=slice_list->object[0]))
			{
				slice_list->object[sl_scene] = object;
				slice_list->on[sl_scene] = TRUE;
				sl_scene++;
			}
	}
	for (sl_image=slice_list; sl_image->next; sl_image=sl_image->next)
		;
	if (!sl_image->on[0] && !sl_image->on[1])
	{	if (!sl_image->valid)
			/* Blank out image */
			for (j=0; j<sl_image->image->height; j++)
			{
			  out_ptr =
				sl_image->image->data+j*sl_image->image->bytes_per_line;
			  if (true_color)
			    memset(out_ptr, 0, sl_image->image->bytes_per_line);
			  else
			    for (k=0; k<sl_image->image->width; k++, out_ptr+=pixel_bytes)
					for (pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
						out_ptr[pixel_byte] = slice_color_table[0][
							OBJECT_IMAGE_BACKGROUND].c[pixel_byte];
			}
		sl_image->valid = TRUE;
		return;
	}
	for (sl_scene=0; sl_scene<=1; sl_scene++)
	{	if (!sl_image->on[sl_scene])
			continue;
		if (load_scene(sl_image->object[sl_scene], Sl_Sc.file_name,
				&Sl_Sc.fp, &Sl_Sc.vh, &Sl_Sc.slices, &Sl_Sc.width,
				&Sl_Sc.height, &Sl_Sc.ptr, &Sl_Sc.pix_size, &Sl_Sc.min,
				&Sl_Sc.max, &Sl_Sc.bytes_per_slice, &Sl_Sc.byte_data,
				&Sl_Sc.dbyte_data))
			return;
		unsigned short *sl_data=NULL;
		get_plane_ends(sl_image->object[sl_scene], ends,
				sl_image->image->width, sl_image->image->height,
				&Sl_Sc.vh.scn, sl_image->plane_displacement,
				sl_image->plane_displacement, sl_image->plane_normal,
				sl_image->x_axis, sl_image->y_axis, &sl_image->plane_type);
		if (get_oblique_slice(&sl_data, sl_image->image->width,
					sl_image->image->height, ends, sl_image->interpolated,
					sl_scene) != DONE)
		{	destroy_scene(&Sl_Sc.vh, &Sl_Sc.fp, &Sl_Sc.byte_data,
					&Sl_Sc.dbyte_data);
				return;
		}
		if (old_scene[sl_scene])
				free(old_scene[sl_scene]);
		old_scene[sl_scene] = (char *)malloc(strlen(Sl_Sc.file_name)+1);
		strcpy(old_scene[sl_scene], Sl_Sc.file_name);
		if (!sl_image->gray_window_valid[sl_scene])
		{	sl_image->gray_min[sl_scene] = (float)Sl_Sc.min;
			sl_image->gray_max[sl_scene] = (float)Sl_Sc.max;
			sl_image->gray_level[sl_scene] = (float)((Sl_Sc.max+Sl_Sc.min)/2.);
			sl_image->gray_width[sl_scene] = (float)(Sl_Sc.max-Sl_Sc.min);
			sl_image->gray_window_valid[sl_scene] = TRUE;
		}
		if (!sl_image->data_valid[sl_scene])
			display_error(map_slice(sl_data, sl_image, sl_scene));
		free(sl_data);
	}
	display_slice(sl_image);
}

/*****************************************************************************
 * FUNCTION: map_slice
 * DESCRIPTION: Copies gray values of a slice image to sl_image->
 *    image_data[sl_scene].
 * PARAMETERS:
 *    sl_data: The input slice data, must have same width & height as
 *       sl_image->image.
 *    sl_image: The structure where the output pixel values are stored; also
 *       gives the width & height of the image.  sl_image->image_data[sl_scene]
 *       must have enough space allocated for the output or be NULL; if NULL
 *       it will be allocated.
 *    sl_scene: 0 or 1, specifies which image data will be computed.
 * SIDE EFFECTS: sl_image->data_valid[sl_scene] will be set.
 * ENTRY CONDITIONS: The global variables slice_color_table, gray_level,
 *    gray_width, pixel_bytes, and the static variable scene must be
 *    appropriately set.
 * RETURN VALUE:
 *    0: Successful
 *    1: Memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/23/93 by Dewey Odhner
 *    Modified: 2/3/94 gray windowing moved to display_slice by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::map_slice(unsigned short *sl_data, Slice_image *sl_image,
	int sl_scene)
{
	unsigned char *out_ptr8;
	int j, k;
	unsigned short *in_ptr, *out_ptr16;

	if (sl_image->image_data[sl_scene].c == NULL)
		sl_image->image_data[sl_scene].c = (unsigned char *)
			malloc(sl_image->image->height*sl_image->image->width*
				(sl_image->gray_max[sl_scene]>255? sizeof(short): 1));
	if (sl_image->image_data[sl_scene].c == NULL)
		return (1);

	/* prepare image */
	for (j=0; j<sl_image->image->height; j++)
	{	in_ptr = sl_data+j*sl_image->image->width;
		if (sl_image->gray_max[sl_scene] <= 255)
		{	out_ptr8 =
				sl_image->image_data[sl_scene].c+j*sl_image->image->width;
			for (k=0; k<sl_image->image->width; k++, in_ptr++, out_ptr8++)
				*out_ptr8 = (unsigned char)*in_ptr;
		}
		else
		{	out_ptr16 =
				sl_image->image_data[sl_scene].s+j*sl_image->image->width;
			for (k=0; k<sl_image->image->width; k++, in_ptr++, out_ptr16++)
				*out_ptr16 = *in_ptr;
		}
	}
	sl_image->data_valid[sl_scene] = TRUE;
	return (0);
}

/*****************************************************************************
 * FUNCTION: display_slice
 * DESCRIPTION: Combines the valid image data in sl_image by gray screen door
 *    dithering into sl_image->image->data.
 * PARAMETERS:
 *    sl_image: The structure in which image data will be combined.
 *       sl_image->image->data must have enough space allocated for the output.
 *       sl_image->on must have at least one non-zero value, and the
 *       corresponding sl_image->image_data must be valid.
 * SIDE EFFECTS: sl_image->valid will be set.
 *    An error message may be displayed.
 * ENTRY CONDITIONS: The global variables slice_color_table, pixel_bytes,
 *    true_color must be appropriately set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/23/93 by Dewey Odhner
 *    Modified: 2/3/94 gray windowing moved from map_slice by Dewey Odhner
 *    Modified: 2/16/04 gray_lut allocation corrected by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::display_slice(Slice_image *sl_image)
{
	int j, k, pixel_byte, sl_scene[2], parity;
	char *out_ptr;
	unsigned char *gray_lut[2], *in_ptr8[2];
	double v, f, vmin, vmax;
	unsigned short *in_ptr16[2];

	/* prepare image */
	assert (sl_image->image->xoffset == 0);
	sl_scene[0] = !sl_image->on[0];
	sl_scene[1] = sl_image->on[1];
	gray_lut[0] =
		(unsigned char *)malloc((int)sl_image->gray_max[sl_scene[0]]+1);
	if (gray_lut[0] == NULL)
	{	report_malloc_error();
		return;
	}
	vmin = 0;
	vmax = true_color? 255: OBJECT_IMAGE_BACKGROUND;
	f = (vmax-vmin)/
		(	sl_image->gray_width[sl_scene[0]]>0
			?	sl_image->gray_width[sl_scene[0]]
			:	1
		);
	for (j=0; j<=sl_image->gray_max[sl_scene[0]]; j++)
	{	v = vmin+(j-(sl_image->gray_level[sl_scene[0]]-
			sl_image->gray_width[sl_scene[0]]/2))*f;
		if (v < vmin)
			v = vmin;
		if (v >= vmax)
			v = vmax-1;
		gray_lut[0][j] = (unsigned char)(sl_image->inverted[0]? vmax-1-v: v);
	}
	if (sl_image->gray_max[sl_scene[1]]==sl_image->gray_max[sl_scene[0]] &&
			sl_image->gray_width[sl_scene[1]]==
			sl_image->gray_width[sl_scene[0]] &&
			sl_image->gray_level[sl_scene[1]]==
			sl_image->gray_level[sl_scene[0]])
		gray_lut[1] = gray_lut[0];
	else
	{	gray_lut[1] =
			(unsigned char *)malloc((int)sl_image->gray_max[sl_scene[1]]+1);
		if (gray_lut[1] == NULL)
		{
			report_malloc_error();
			free(gray_lut[0]);
			return;
		}
		f = (vmax-vmin)/
			(	sl_image->gray_width[sl_scene[1]]>0
				?	sl_image->gray_width[sl_scene[1]]
				:	1
			);
		for (j=0; j<=sl_image->gray_max[sl_scene[1]]; j++)
		{	v = vmin+(j-(sl_image->gray_level[sl_scene[1]]-
				sl_image->gray_width[sl_scene[1]]/2))*f;
			if (v < vmin)
				v = vmin;
			if (v >= vmax)
				v = vmax-1;
			gray_lut[1][j] = (unsigned char)(sl_image->inverted[1]? vmax-1-v: v);
		}
	}
	for (j=0; j<sl_image->image->height; j++)
	{	parity = j&1;
		if (sl_image->gray_max[sl_scene[0]]>255)
		{	in_ptr16[0] =
				sl_image->image_data[sl_scene[0]].s+j*sl_image->image->width;
			in_ptr8[0] = NULL;
		}
		else
			in_ptr8[0] =
				sl_image->image_data[sl_scene[0]].c+j*sl_image->image->width;
		if (sl_image->gray_max[sl_scene[1]]>255)
		{	in_ptr16[1] =
				sl_image->image_data[sl_scene[1]].s+j*sl_image->image->width;
			in_ptr8[1] = NULL;
		}
		else
			in_ptr8[1] =
				sl_image->image_data[sl_scene[1]].c+j*sl_image->image->width;
		out_ptr =
			sl_image->image->data+j*sl_image->image->bytes_per_line;
		for (k=0; k<sl_image->image->width; k++)
		{
			if (true_color)
			{
				out_ptr[0] = gray_lut[parity][
							in_ptr8[parity]
							?	*in_ptr8[parity]
							:	*in_ptr16[parity]];
				out_ptr[2] = out_ptr[1] = out_ptr[0];
			}
			else
				for (pixel_byte=0; pixel_byte<pixel_bytes; pixel_byte++)
					out_ptr[pixel_byte] = slice_color_table[sl_image->object[
						sl_scene[parity]]->O.color][gray_lut[parity][
							in_ptr8[parity]
							?	*in_ptr8[parity]
							:	*in_ptr16[parity]
						]].c[pixel_byte];
			parity = !parity;
			if (in_ptr8[0])
				in_ptr8[0]++;
			else
				in_ptr16[0]++;
			if (in_ptr8[1])
				in_ptr8[1]++;
			else
				in_ptr16[1]++;
			out_ptr += pixel_bytes;
		}
	}
	sl_image->valid = TRUE;
	free(gray_lut[0]);
	if (gray_lut[sl_scene[1]] != gray_lut[sl_scene[0]])
		free(gray_lut[1]);
}

/*****************************************************************************
 * FUNCTION: get_plane_ends
 * DESCRIPTION: Computes the coordinates of corners of slices in units of
 *    voxels of the original scene.
 * PARAMETERS:
 *    object: Used to obtain the location and orientation of the scanner
 *       coordinate system with respect to the plan coordinate system.
 *    ends: The result goes here:
 *       ends[0]: the origin of the first slice
 *       ends[1]: the (max, 0, 0) corner of the first slice
 *       ends[2]: the (0, max, 0) corner of the first slice
 *       ends[3]: the origin of the last slice; nothing will be stored here
 *          if last_loc == first_loc.
 *    sl_width, sl_height: The size of the slice in pixels
 *    scn: Used to obtain the scene coordinate system with respect to the
 *       scanner coordinate system.
 *    first_loc, last_loc: The locations of the first and last slices along
 *       the direction of sl_normal with respect to the center of image space.
 *    sl_normal: The unit normal vector to the slices with respect to the plan
 *       coordinate system.
 *    x_axis, y_axis: Vectors of one pixel along slice axes, relative to plan
 *       space will be stored here.
 *    plane_type: The nearest principal plane to the slice plane.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables glob_angle, scale must
 *    be appropriately set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions or parameters are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/1/93 to use sl_image by Dewey Odhner
 *    Modified: 4/16/93 for new header structure by Dewey Odhner
 *    Modified: 4/29/93 to use orientations of domains by Dewey Odhner
 *    Modified: 5/12/93 to pass scn by Dewey Odhner
 *    Modified: 5/13/93 to get point for second plane by Dewey Odhner
 *    Modified: 10/21/93 to handle units by Dewey Odhner
 *    Modified: 10/27/93 to compute x_axis, y_axis by Dewey Odhner
 *    Modified: 1/12/94 to handle orientations of domains correctly
 *       by Dewey Odhner
 *    Modified: 9/6/94 to handle location of first scene slice by Dewey Odhner
 *    Modified: 4/7/95 slice orientation changed and parameter plane_type
 *       added by Dewey Odhner
 *    Modified: 7/17/95 trailing space removed from axis label by Dewey Odhner
 *    Modified: 8/30/99 sagittal slice vectors corrected by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_plane_ends(Shell *object, POINT3D ends[4], int sl_width,
	int sl_height, SceneInfo *scn, double first_loc, double last_loc,
	double sl_normal[3], double x_axis[3], double y_axis[3],
	Principal_plane *plane_type)
{
	double rotation_matrix[3][3], plane_corners[4][3], normal_vector[3],
		slice_spacing, *plane_top_left, *plane_top_right, *plane_bottom_left;
	int cornern, scene_axis[3], scanner_axis[3]={0,0,0}, j, k, normal_axis;
	double p[3], q[3], norm, translation[3], sl_scale, temp_q[3];
	StructureInfo *str;
	float *domain;

	if (scn->axis_label_valid)
		for (j=0; j<3; j++)
		{	scn->axis_label[j][0] = toupper(scn->axis_label[j][0]);
			for (k=1; k<(int)strlen(scn->axis_label[j]); k++)
				scn->axis_label[j][k] = tolower(scn->axis_label[j][k]);
			if (scn->axis_label[j][k-1] == ' ')
				scn->axis_label[j][k-1] = 0;
			if (strcmp(scn->axis_label[j], "Left") == 0)
				scene_axis[j] = Left;
			else if (strcmp(scn->axis_label[j], "Right") == 0)
				scene_axis[j] = Right;
			else if (strcmp(scn->axis_label[j], "Posterior") == 0)
				scene_axis[j] = Posterior;
			else if (strcmp(scn->axis_label[j], "Anterior") == 0)
				scene_axis[j] = Anterior;
			else if (strcmp(scn->axis_label[j], "Head") == 0)
				scene_axis[j] = Head;
			else if (strcmp(scn->axis_label[j], "Feet") == 0)
				scene_axis[j] = Feet;
			else
				scene_axis[j] = j==0? Left: j==1? Posterior: Head;
		}
	else
	{	scene_axis[0] = Left;
		scene_axis[1] = Posterior;
		scene_axis[2] = Head;
	}
	if (scn->domain_valid)
	{	j = 0;
		for (k=0, norm=0; k<3; k++)
			if (scn->domain[3+j+3*k] > norm)
			{	norm = scn->domain[3+j+3*k];
				scanner_axis[j] = scene_axis[k];
			}
			else if (-scn->domain[3+j+3*k] > norm)
			{	norm = -scn->domain[3+j+3*k];
				scanner_axis[j] = -scene_axis[k];
			}
		j = 1;
		for (k=0, norm=0; k<3; k++)
			if (scene_axis[k]!=scanner_axis[0] &&
					-scene_axis[k]!=scanner_axis[0])
			{
				if (scn->domain[3+j+3*k]>norm)
				{	norm = scn->domain[3+j+3*k];
					scanner_axis[j] = scene_axis[k];
				}
				else if (-scn->domain[3+j+3*k] > norm)
				{	norm = -scn->domain[3+j+3*k];
					scanner_axis[j] = -scene_axis[k];
				}
			}
		j = 2;
		for (k=0, norm=0; k<3; k++)
			if (scene_axis[k]!=scanner_axis[0] &&
					-scene_axis[k]!=scanner_axis[0] &&
					scene_axis[k]!=scanner_axis[1] &&
					-scene_axis[k]!=scanner_axis[1])
			{
				if (scn->domain[3+j+3*k] > norm)
				{	norm = scn->domain[3+j+3*k];
					scanner_axis[j] = scene_axis[k];
				}
				else if (-scn->domain[3+j+3*k] > norm)
				{	norm = -scn->domain[3+j+3*k];
					scanner_axis[j] = -scene_axis[k];
				}
			}
	}
	else
		memcpy(scanner_axis, scene_axis, sizeof(scanner_axis));
	if (fabs(sl_normal[2]) >= fabs(sl_normal[1]) &&
			fabs(sl_normal[2]) >= fabs(sl_normal[0])) /* axial */
	{	if (sl_normal[2] >= 0)
		{	p[0] = (1+sl_normal[2]-sl_normal[0]*sl_normal[0])/(1+sl_normal[2]);
			p[1] = -sl_normal[0]*sl_normal[1]/(1+sl_normal[2]);
			p[2] = -sl_normal[0];
			q[0] = -p[1];
			q[1]= -(1+sl_normal[2]-sl_normal[1]*sl_normal[1])/(1+sl_normal[2]);
			q[2] = sl_normal[1];
		}
		else
		{	p[0] = (1-sl_normal[2]-sl_normal[0]*sl_normal[0])/(1-sl_normal[2]);
			p[1] = -sl_normal[0]*sl_normal[1]/(1-sl_normal[2]);
			p[2] = sl_normal[0];
			q[0] = -p[1];
			q[1]= -(1-sl_normal[2]-sl_normal[1]*sl_normal[1])/(1-sl_normal[2]);
			q[2] = -sl_normal[1];
		}
		normal_axis = 2;
	}
	else if (fabs(sl_normal[1]) >= fabs(sl_normal[0])) /* coronal */
	{	if (sl_normal[1] >= 0)
		{	q[2] = (1+sl_normal[1]-sl_normal[2]*sl_normal[2])/(1+sl_normal[1]);
			q[0] = -sl_normal[2]*sl_normal[0]/(1+sl_normal[1]);
			q[1] = -sl_normal[2];
			p[2] = q[0];
			p[0] = (1+sl_normal[1]-sl_normal[0]*sl_normal[0])/(1+sl_normal[1]);
			p[1] = -sl_normal[0];
		}
		else
		{	q[2] = (1-sl_normal[1]-sl_normal[2]*sl_normal[2])/(1-sl_normal[1]);
			q[0] = -sl_normal[2]*sl_normal[0]/(1-sl_normal[1]);
			q[1] = sl_normal[2];
			p[2] = q[0];
			p[0] = (1-sl_normal[1]-sl_normal[0]*sl_normal[0])/(1-sl_normal[1]);
			p[1] = sl_normal[0];
		}
		normal_axis = 1;
	}
	else /* sagittal */
	{	if (sl_normal[0] >= 0)
		{	p[1] = (1+sl_normal[0]-sl_normal[1]*sl_normal[1])/(1+sl_normal[0]);
			p[2] = -sl_normal[1]*sl_normal[2]/(1+sl_normal[0]);
			p[0] = -sl_normal[1];
			q[1] = p[2];
			q[2] = (1+sl_normal[0]-sl_normal[2]*sl_normal[2])/(1+sl_normal[0]);
			q[0] = -sl_normal[2];
		}
		else
		{	p[1] = (1-sl_normal[0]-sl_normal[1]*sl_normal[1])/(1-sl_normal[0]);
			p[2] = -sl_normal[1]*sl_normal[2]/(1-sl_normal[0]);
			p[0] = sl_normal[1];
			q[1] = p[2];
			q[2] = (1-sl_normal[0]-sl_normal[2]*sl_normal[2])/(1-sl_normal[0]);
			q[0] = sl_normal[2];
		}
		normal_axis = 0;
	}
	switch (scanner_axis[normal_axis])
	{	case Anterior:
		case Posterior:
			*plane_type = CORONAL;
			break;
		case Feet:
		case Head:
			*plane_type = AXIAL;
			break;
		case Left:
		case Right:
			*plane_type = SAGITTAL;
			break;
	}
	memcpy(temp_q, q, sizeof(q));
	if (scanner_axis[normal_axis!=0? 0:1] ==
			(*plane_type==SAGITTAL? Anterior: Right))
	{	p[0] = -p[0];
		p[1] = -p[1];
		p[2] = -p[2];
	}
	else if (scanner_axis[normal_axis!=0? 0:1] ==
			(*plane_type==AXIAL? Anterior: Head))
		memcpy(q, p, sizeof(p));
	else if (scanner_axis[normal_axis!=0? 0:1] ==
			(*plane_type==AXIAL? Posterior: Feet))
	{	q[0] = -p[0];
		q[1] = -p[1];
		q[2] = -p[2];
	}
	if ((normal_axis==2? -scanner_axis[1]: scanner_axis[2]) ==
			(*plane_type==AXIAL? Posterior: Feet))
	{	q[0] = -temp_q[0];
		q[1] = -temp_q[1];
		q[2] = -temp_q[2];
	}
	else if ((normal_axis==2? -scanner_axis[1]: scanner_axis[2]) ==
			(*plane_type==SAGITTAL? Posterior: Left))
		memcpy(p, temp_q, sizeof(p));
	else if ((normal_axis==2? -scanner_axis[1]: scanner_axis[2]) ==
			(*plane_type==SAGITTAL? Anterior: Right))
	{	p[0] = -temp_q[0];
		p[1] = -temp_q[1];
		p[2] = -temp_q[2];
	}
	AtoM(rotation_matrix, glob_angle[0], glob_angle[1], glob_angle[2]);
	matrix_vector_multiply(normal_vector, rotation_matrix, sl_normal);
	sl_scale = scale*sl_width/main_image->width;
	norm = .5*sl_width/sl_scale;
	p[0] *= norm;
	p[1] *= norm;
	p[2] *= norm;
	q[0] *= norm;
	q[1] *= norm;
	q[2] *= norm;
	matrix_vector_multiply(p, rotation_matrix, p);
	matrix_vector_multiply(q, rotation_matrix, q);
	/* p & q are orthogonal vectors parallel to the plane. */

	plane_top_left = plane_corners[0];
	plane_top_right = plane_corners[1];
	plane_bottom_left = plane_corners[2];
	for (cornern=0; cornern<4; cornern++)
	{	plane_corners[cornern][0] = normal_vector[0]*first_loc;
		plane_corners[cornern][1] = normal_vector[1]*first_loc;
		plane_corners[cornern][2] = normal_vector[2]*first_loc;
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
			plane_corners[cornern][1] += p[1];
			plane_corners[cornern][2] += p[2];
		}
		else
		{	plane_corners[cornern][0] -= p[0];
			plane_corners[cornern][1] -= p[1];
			plane_corners[cornern][2] -= p[2];
		}
	}
	ends[0][0] = plane_top_left[0];
	ends[0][1] = plane_top_left[1];
	ends[0][2] = plane_top_left[2];
	ends[1][0] = plane_top_right[0];
	ends[1][1] = plane_top_right[1];
	ends[1][2] = plane_top_right[2];
	ends[2][0] = plane_bottom_left[0];
	ends[2][1] = plane_bottom_left[1];
	ends[2][2] = plane_bottom_left[2];
	if (last_loc != first_loc)
	{	ends[3][0] = normal_vector[0]*last_loc+q[0]-p[0];
		ends[3][1] = normal_vector[1]*last_loc+q[1]-p[1];
		ends[3][2] = normal_vector[2]*last_loc+q[2]-p[2];
	}
	x_axis[0] = (plane_top_right[0]-plane_top_left[0])/sl_width;
	x_axis[1] = (plane_top_right[1]-plane_top_left[1])/sl_width;
	x_axis[2] = (plane_top_right[2]-plane_top_left[2])/sl_width;
	y_axis[0] = (plane_bottom_left[0]-plane_top_left[0])/sl_width;
	y_axis[1] = (plane_bottom_left[1]-plane_top_left[1])/sl_width;
	y_axis[2] = (plane_bottom_left[2]-plane_top_left[2])/sl_width;
	vector_matrix_multiply(x_axis, x_axis, rotation_matrix);
	vector_matrix_multiply(y_axis, y_axis, rotation_matrix);

	/* convert from image coordinates to object coordinates */
	get_object_transformation(rotation_matrix, translation, object, FALSE);
	vector_subtract(ends[0], ends[0], translation);
	vector_subtract(ends[1], ends[1], translation);
	vector_subtract(ends[2], ends[2], translation);
	vector_matrix_multiply(ends[0], ends[0], rotation_matrix);
	vector_matrix_multiply(ends[1], ends[1], rotation_matrix);
	vector_matrix_multiply(ends[2], ends[2], rotation_matrix);
	if (last_loc != first_loc)
	{	vector_subtract(ends[3], ends[3], translation);
		vector_matrix_multiply(ends[3], ends[3], rotation_matrix);
	}

	str = &object->main_data.file->file_header.str;
	translation[0] = unit_size(str->measurement_unit[0])*
		.5*(str->min_max_coordinates[6*object->main_data.shell_number+3]+
		str->min_max_coordinates[6*object->main_data.shell_number]);
	translation[1] = unit_size(str->measurement_unit[1])*
		.5*(str->min_max_coordinates[6*object->main_data.shell_number+4]+
		str->min_max_coordinates[6*object->main_data.shell_number+1]);
	translation[2] = unit_size(str->measurement_unit[2])*
		.5*(str->min_max_coordinates[6*object->main_data.shell_number+5]+
		str->min_max_coordinates[6*object->main_data.shell_number+2]);
	/* go from structure to scene coordinates */
	domain = str->domain+12*object->main_data.shell_number;
	/* convert to scanner orientation */
	rotation_matrix[0][0] = domain[3];
	rotation_matrix[1][0] = domain[4];
	rotation_matrix[2][0] = domain[5];
	rotation_matrix[0][1] = domain[6];
	rotation_matrix[1][1] = domain[7];
	rotation_matrix[2][1] = domain[8];
	rotation_matrix[0][2] = domain[9];
	rotation_matrix[1][2] = domain[10];
	rotation_matrix[2][2] = domain[11];
	matrix_vector_multiply(translation, rotation_matrix, translation);
	matrix_vector_multiply(ends[0], rotation_matrix, ends[0]);
	matrix_vector_multiply(ends[1], rotation_matrix, ends[1]);
	matrix_vector_multiply(ends[2], rotation_matrix, ends[2]);
	if (last_loc != first_loc)
		matrix_vector_multiply(ends[3], rotation_matrix, ends[3]);
	/* shift origin */
	translation[0] += domain[0]*unit_size(str->measurement_unit[0])-
		scn->domain[0]*unit_size(scn->measurement_unit[0]);
	translation[1] += domain[1]*unit_size(str->measurement_unit[1])-
		scn->domain[1]*unit_size(scn->measurement_unit[1]);
	translation[2] += domain[2]*unit_size(str->measurement_unit[2])-
		scn->domain[2]*unit_size(scn->measurement_unit[2]);
	vector_add(ends[0], ends[0], translation);
	vector_add(ends[1], ends[1], translation);
	vector_add(ends[2], ends[2], translation);
	if (last_loc != first_loc)
		vector_add(ends[3], ends[3], translation);
	/* convert to scene orientation */
	rotation_matrix[0][0] = scn->domain[3];
	rotation_matrix[0][1] = scn->domain[4];
	rotation_matrix[0][2] = scn->domain[5];
	rotation_matrix[1][0] = scn->domain[6];
	rotation_matrix[1][1] = scn->domain[7];
	rotation_matrix[1][2] = scn->domain[8];
	rotation_matrix[2][0] = scn->domain[9];
	rotation_matrix[2][1] = scn->domain[10];
	rotation_matrix[2][2] = scn->domain[11];
	matrix_vector_multiply(ends[0], rotation_matrix, ends[0]);
	matrix_vector_multiply(ends[1], rotation_matrix, ends[1]);
	matrix_vector_multiply(ends[2], rotation_matrix, ends[2]);
	if (last_loc != first_loc)
		matrix_vector_multiply(ends[3], rotation_matrix, ends[3]);

	/* shift to first slice */
	ends[0][2] -= scn->loc_of_subscenes[0]*unit_size(scn->measurement_unit[2]);
	ends[1][2] -= scn->loc_of_subscenes[0]*unit_size(scn->measurement_unit[2]);
	ends[2][2] -= scn->loc_of_subscenes[0]*unit_size(scn->measurement_unit[2]);
	if (last_loc != first_loc)
		ends[3][2] -=
			scn->loc_of_subscenes[0]*unit_size(scn->measurement_unit[2]);

	/* convert from physical coordinates to voxel coordinates */
	ends[0][0] /= (scn->xypixsz[0]*unit_size(scn->measurement_unit[0]));
	ends[1][0] /= (scn->xypixsz[0]*unit_size(scn->measurement_unit[0]));
	ends[2][0] /= (scn->xypixsz[0]*unit_size(scn->measurement_unit[0]));
	ends[0][1] /= (scn->xypixsz[1]*unit_size(scn->measurement_unit[1]));
	ends[1][1] /= (scn->xypixsz[1]*unit_size(scn->measurement_unit[1]));
	ends[2][1] /= (scn->xypixsz[1]*unit_size(scn->measurement_unit[1]));
	slice_spacing = scn->num_of_subscenes[0]<=1? scn->xypixsz[0]:
		(scn->loc_of_subscenes[scn->num_of_subscenes[0]-1]-
		 scn->loc_of_subscenes[0])/(scn->num_of_subscenes[0]-1);
	ends[0][2] /= slice_spacing*unit_size(scn->measurement_unit[2]);
	ends[1][2] /= slice_spacing*unit_size(scn->measurement_unit[2]);
	ends[2][2] /= slice_spacing*unit_size(scn->measurement_unit[2]);
	if (last_loc != first_loc)
	{	ends[3][0] /= (scn->xypixsz[0]*unit_size(scn->measurement_unit[0]));
		ends[3][1] /= (scn->xypixsz[1]*unit_size(scn->measurement_unit[1]));
		ends[3][2] /= slice_spacing*unit_size(scn->measurement_unit[2]);
	}
}

/*****************************************************************************
 * FUNCTION: load_scene
 * DESCRIPTION: Loads information from the scene file from which the object
 *    was derived.
 * PARAMETERS:
 *    object: Identifies the scene file.
 *    scene_file_name: The scene file name will be put here.
 *    fp: The file pointer open for reading will be put here.
 *    vh: The header information from the file will be put here.
 *    slices: The number of slices in the scene will be put here.
 *    width, height: The width and height of slices in pixels will be put here.
 *    hdrlen: Position of the begining of scene data in the file will go here.
 *    bits: Bits per cell value will be put here.
 *    min, max: Range of cell values will be put here.
 *    bytes_per_slice: Bytes per slice will be put here.
 *    byte_data, dbyte_data: The scene data will be loaded at one of these
 *       addresses if memory permits.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 * RETURN VALUE: Non-zero if unsuccessful.
 * EXIT CONDITIONS: Returns a non-zero value if the scene header cannot be
 *    read or contains unexpected values.
 *    Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/16/93 for new header structure by Dewey Odhner
 *    Modified: 10/21/93 to allow different units by Dewey Odhner
 *    Modified: 1/10/93 return parameters added by Dewey Odhner
 *    Modified: 7/22/94 to load data into memory by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::load_scene(Shell *object, char scene_file_name[], FILE **fp,
	ViewnixHeader *vh, int *slices, int *width, int *height,
	unsigned long *hdrlen, int *bits, unsigned long *min, unsigned long *max,
	unsigned long *bytes_per_slice, unsigned char **byte_data,
	unsigned short **dbyte_data)
{
	int error_code, items_read;
	char bad_group[5], bad_element[5], *file_name;

	file_name =
		object->main_data.file->file_header.str.scene_file_valid
		?	object->main_data.file->file_header.str.scene_file[
				object->main_data.shell_number]
		:	object->main_data.file->file_header.gen.filename1;
	if (file_name == NULL)
	{	display_message("SCENE FILE NOT KNOWN.");
		return (-1);
	}
	if (strcmp(scene_file_name, file_name))
		strcpy(scene_file_name, file_name);
	else if (*fp != NULL)
		return (0);
	destroy_scene(vh, fp, byte_data, dbyte_data);
	*fp = fopen(scene_file_name, "rb");
	if (*fp == NULL)
	{	display_message("CAN'T OPEN SCENE FILE.");
		return (4);
	}
	error_code = VReadHeader(*fp, vh, bad_group, bad_element);
	switch (error_code)
	{	case 0:
		case 107:
		case 106:
			break;
		default:
			display_error(error_code);
			fprintf(stderr, "Group %s element %s undefined in VReadHeader\n",
				bad_group, bad_element);
			return (error_code);
	}
	if (vh->scn.dimension != 3)
	{	display_message("SCENE FILE NOT THREE-DIMENSIONAL.");
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (-1);
	}
	if (vh->scn.measurement_unit[0]<0 ||
			vh->scn.measurement_unit[0]>4 ||
			vh->scn.measurement_unit[1]<0 ||
			vh->scn.measurement_unit[1]>4 ||
			vh->scn.measurement_unit[2]<0 ||
			vh->scn.measurement_unit[2]>4)
	{	display_message("UNEXPECTED UNIT OF MEASUREMENT IN SCENE FILE.");
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (-1);
	}
	if (vh->scn.num_of_density_values != 1)
	{	display_message("MULTIPLE-VALUED DATA IN SCENE FILE.");
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (-1);
	}
	if (vh->scn.num_of_integers != 1)
	{	display_message("NON-INTEGER DATA IN SCENE FILE.");
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (-1);
	}
	switch (vh->scn.num_of_bits)
	{	case 16:
		case 8:
			if (vh->scn.bit_fields[0]!=0 ||
					vh->scn.bit_fields[1]!=vh->scn.num_of_bits-1)
			{	display_message("UNEXPECTED BIT FIELD IN SCENE FILE.");
				destroy_scene(vh, fp, byte_data, dbyte_data);
				return (-1);
			}
			break;
		default:
			display_message("UNEXPECTED BITS PER CELL IN SCENE FILE.");
			destroy_scene(vh, fp, byte_data, dbyte_data);
			return (-1);
	}
	if (vh->scn.dimension_in_alignment!=0 &&
			vh->scn.bytes_in_alignment>vh->scn.num_of_bits/8)
	{	display_message("UNEXPECTED ALIGNMENT IN SCENE FILE.");
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (-1);
	}
	*slices = vh->scn.num_of_subscenes[0];
	*width = vh->scn.xysize[0];
	*height = vh->scn.xysize[1];
	int headerlength;
	error_code = VGetHeaderLength(*fp, &headerlength);
	if (error_code)
	{	display_error(error_code);
		destroy_scene(vh, fp, byte_data, dbyte_data);
		return (error_code);
	}
	*hdrlen = (unsigned long)headerlength;
	*bits = vh->scn.num_of_bits;
	*min = (unsigned long)vh->scn.smallest_density_value[0];
	*max = (unsigned long)vh->scn.largest_density_value[0];
	*bytes_per_slice = *width**height*(*bits/8);
	if (*bits == 8)
	{	*byte_data = (unsigned char *)malloc(*slices**bytes_per_slice);
		if (*byte_data)
		{	error_code = VSeekData(*fp, 0);
			if (error_code)
			{	display_error(error_code);
				destroy_scene(vh, fp, byte_data, dbyte_data);
				return (error_code);
			}
			error_code = VReadData((char *)*byte_data, 1,
				*slices**bytes_per_slice, *fp, &items_read);
			if (error_code==0 && items_read!=*slices*(int)*bytes_per_slice)
				error_code = 2;
			if (error_code)
			{	display_error(error_code);
				destroy_scene(vh, fp, byte_data, dbyte_data);
				return (error_code);
			}
		}
	}
	else
	{	*dbyte_data = (unsigned short *)malloc(*slices**bytes_per_slice);
		if (*dbyte_data)
		{	error_code = VSeekData(*fp, 0);
			if (error_code)
			{	display_error(error_code);
				destroy_scene(vh, fp, byte_data, dbyte_data);
				return (error_code);
			}
			error_code = VReadData((char *)*dbyte_data, 2,
				*slices**width**height, *fp, &items_read);
			if (error_code==0 && items_read!=*slices**width**height)
				error_code = 2;
			if (error_code)
			{	display_error(error_code);
				destroy_scene(vh, fp, byte_data, dbyte_data);
				return (error_code);
			}
		}
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: destroy_scene_header
 * DESCRIPTION: Frees the memory referenced by the pointer fields of a scene
 *    file header.
 * PARAMETERS:
 *    vh: The scene file header.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The pointer fields of vh must point to allocated memory or
 *    be NULL.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/11/93 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_scene_header(ViewnixHeader *vh)
{
	if (vh->gen.description)
		free(vh->gen.description);
	if (vh->gen.comment)
		free(vh->gen.comment);
	if (vh->gen.imaged_nucleus)
		free(vh->gen.imaged_nucleus);
	if (vh->gen.gray_lookup_data)
		free(vh->gen.gray_lookup_data);
	if (vh->gen.red_lookup_data)
		free(vh->gen.red_lookup_data);
	if (vh->gen.green_lookup_data)
		free(vh->gen.green_lookup_data);
	if (vh->gen.blue_lookup_data)
		free(vh->gen.blue_lookup_data);
	if (vh->scn.domain)
		free(vh->scn.domain);
	if (vh->scn.axis_label)
		free(vh->scn.axis_label);
	if (vh->scn.measurement_unit)
		free(vh->scn.measurement_unit);
	if (vh->scn.density_measurement_unit)
		free(vh->scn.density_measurement_unit);
	if (vh->scn.smallest_density_value)
		free(vh->scn.smallest_density_value);
	if (vh->scn.largest_density_value)
		free(vh->scn.largest_density_value);
	if (vh->scn.signed_bits)
		free(vh->scn.signed_bits);
	if (vh->scn.bit_fields)
		free(vh->scn.bit_fields);
	if (vh->scn.num_of_subscenes)
		free(vh->scn.num_of_subscenes);
	if (vh->scn.loc_of_subscenes)
		free(vh->scn.loc_of_subscenes);
	if (vh->scn.description)
		free(vh->scn.description);
	memset(vh, 0, sizeof(*vh));
}

/*****************************************************************************
 * FUNCTION: destroy_scene
 * DESCRIPTION: Frees the memory referenced by the pointer fields of vh, and
 *    closes the file if *fp is not NULL.
 * PARAMETERS:
 *    vh: The scene header to be cleared.
 *    fp: The file pointer to be cleared.
 * SIDE EFFECTS: Clears the pointers.
 * ENTRY CONDITIONS: The pointer fields of vh must point to allocated memory or
 *    be NULL.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/29/93 to free stuff in vh by Dewey Odhner
 *    Modified: 1/10/94 parameters added by Dewey Odhner
 *    Modified: 7/19/94 to free scene data memory by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_scene(ViewnixHeader *vh, FILE **fp,
	unsigned char **byte_data, unsigned short **dbyte_data)
{
	destroy_scene_header(vh);
	if (*fp)
	{	fclose(*fp);
		*fp = NULL;
	}
	if (*byte_data)
	{	free(*byte_data);
		*byte_data = NULL;
	}
	if (*dbyte_data)
	{	free(*dbyte_data);
		*dbyte_data = NULL;
	}
}

/*****************************************************************************
 * FUNCTION: get_oblique_slice
 * DESCRIPTION: Computes a data slice from a scene.
 * PARAMETERS:
 *    sl_data: The address to put the data slice; if not NULL, must point to
 *       allocated memory.
 *    sl_width, sl_height: The size of the slice in pixels.
 *    ends: The coordinates of corners of the slice in units of voxels of the
 *       original scene.
 *       ends[0]: the origin of the slice
 *       ends[1]: the (max, 0, 0) corner of the slice
 *       ends[2]: the (0, max, 0) corner of the slice
 *    interpolate: Flag specifying linear interpolation
 *    sl_scene: which static data set to use
 * SIDE EFFECTS: Extraneous events will be removed from the queue.
 *    An error message may be issued.
 * ENTRY CONDITIONS: The static variable scene must be appropriately set.
 *    The global variables display, argv0, windows_open, dialog_window
 *    must be appropriately initialized.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: DONE, EROR, or INTERRUPT3.
 * EXIT CONDITIONS: If parameters are the same as at the last successful call,
 *    this function will return immediately.  If a right button press is
 *    detected or an error occurs, *sl_data will be freed and set to NULL and
 *    this function will return.
 *    Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner (adapted from Supun Samarasekera)
 *    Modified: 6/14/93 to make scene_data local by Dewey Odhner
 *    Modified: 7/20/94 to use data in memory by Dewey Odhner
 *    Modified: 7/25/94 to use data VReadData by Dewey Odhner
 *    Modified: 7/26/94 to go through only range of slices by Dewey Odhner
 *    Modified: 8/11/94 to go faster with data in memory by Dewey Odhner
 *    Modified: 1/23/95 interpolate parameter added by Dewey Odhner
 *    Modified: 1/25/95 multiplication tables added by Dewey Odhner
 *    Modified: 2/10/04 sl_scene parameter added by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::get_oblique_slice(unsigned short **sl_data,
	int sl_width,int sl_height, POINT3D ends[3], int interpolate, int sl_scene)
{
  unsigned short *pl_slice_map=NULL;
  POINT3D u,v,point; /* vectors along the x,y directions on the plane in vol
   coords */
  double slice, dx,dy,dz;
  int i,j,n, x,y,z, min_slice=0, max_slice=0, *x_table[3]={NULL,NULL,NULL};
  unsigned short *map, *img_ptr, *ptr, *pt1, *pt2, *ptr1=NULL, *ptr2, *tmp_ptr;
  unsigned char *pt1_8, *pt2_8, *ptr1_8=NULL, *ptr2_8, *tmp_ptr_8;
  char *scene_data; /* space for two slices */
  unsigned *slice_table=NULL, *row_table=NULL;


  /* If parameters are the same as before, don't repeat the computation */
  if (old_sl_data[sl_scene] != NULL &&
  		*sl_data == old_sl_data[sl_scene] &&
		sl_width == old_sl_width[sl_scene] &&
		sl_height == old_sl_height[sl_scene] &&
		ends[0][0] == old_ends[sl_scene][0][0] &&
		ends[0][1] == old_ends[sl_scene][0][1] &&
		ends[0][2] == old_ends[sl_scene][0][2] &&
		ends[1][0] == old_ends[sl_scene][1][0] &&
		ends[1][1] == old_ends[sl_scene][1][1] &&
		ends[1][2] == old_ends[sl_scene][1][2] &&
		ends[2][0] == old_ends[sl_scene][2][0] &&
		ends[2][1] == old_ends[sl_scene][2][1] &&
		ends[2][2] == old_ends[sl_scene][2][2] &&
		interpolate == old_interpolate[sl_scene] &&
		strcmp(Sl_Sc.file_name, old_scene[sl_scene]) == 0)
	return (DONE);
  old_sl_data[sl_scene] = NULL;

  if (Sl_Sc.byte_data || Sl_Sc.dbyte_data)
    scene_data = NULL;
  else
  { scene_data = (char *)malloc(2*Sl_Sc.bytes_per_slice);
    if (scene_data == NULL)
    { report_malloc_error();
      return (EROR);
    }

    /* Allocate space for the buffer that stores the lower slice numbrer */
    pl_slice_map = (unsigned short *)malloc(sizeof(short)*sl_width*sl_height);
    if (pl_slice_map == NULL) {
      report_malloc_error();
	  free(scene_data);
      return (EROR);
    }
  }
 
  /* Unit vector in the horz. dir along the plane in volumes coords */
  u[0]=(ends[1][0]-ends[0][0])/sl_width;
  u[1]=(ends[1][1]-ends[0][1])/sl_width;
  u[2]=(ends[1][2]-ends[0][2])/sl_width;

  /* Unit vector in the vert. dir along the plane in volumes coords */
  v[0]=(ends[2][0]-ends[0][0])/sl_height;
  v[1]=(ends[2][1]-ends[0][1])/sl_height;
  v[2]=(ends[2][2]-ends[0][2])/sl_height;


  /* Initialize the lower slice index which gives rise to each pixel in
      the plane */ 
  if (scene_data) {
    min_slice = Sl_Sc.slices;
    max_slice = 0;
    for(ptr=pl_slice_map,i=0;i<sl_height;i++)
      for(j=0;j<sl_width;j++,ptr++) {
        slice= ends[0][2] + i*v[2] + j*u[2];
	    if (slice >=0 && slice <Sl_Sc.slices-1) 
	    { *ptr = Floor(slice);
	      if (*ptr < min_slice)
		    min_slice = *ptr;
		  if (*ptr > max_slice)
		    max_slice = *ptr;
	    }
	    else 
	      *ptr = Sl_Sc.slices;
      }
  }

  /* allocate space for the image buffer */
  if (*sl_data==NULL || sl_width!=old_sl_width[sl_scene] ||
      sl_height!=old_sl_height[sl_scene]) {
    if (*sl_data != NULL)
      free(*sl_data);
    *sl_data = (unsigned short *)malloc(2*sl_width*sl_height);
    if (*sl_data == NULL) {
      report_malloc_error();
	  if (scene_data) {
	    free(scene_data);
        free(pl_slice_map);
      }
      return (EROR);
    }
  }

  /* allocate x_table */
  if (!interpolate)
  {	x_table[0] = (int *)malloc(sl_width*sizeof(int));
  	x_table[1] = (int *)malloc(sl_width*sizeof(int));
	x_table[2] = (int *)malloc(sl_width*sizeof(int));
	slice_table = (unsigned *)malloc(Sl_Sc.slices*sizeof(int));
	row_table = (unsigned *)malloc(Sl_Sc.height*sizeof(int));
  	if (x_table[0]==NULL || x_table[1]==NULL || x_table[2]==NULL ||
			slice_table==NULL || row_table==NULL)
	{	report_malloc_error();
		if (scene_data) {
	    	free(scene_data);
        	free(pl_slice_map);
			free(*sl_data);
        	*sl_data=NULL;
    	}
		if (x_table[0])
			free(x_table[0]);
		if (x_table[1])
			free(x_table[1]);
		if (x_table[2])
			free(x_table[2]);
		if (slice_table)
			free(slice_table);
		if (row_table)
			free(row_table);
    	return (EROR);
	}
	for (i=0; i<sl_width; i++)
		for (j=0; j<3; j++)
			x_table[j][i] = (int)rint(i*u[j]);
	for (i=j=0; i<Sl_Sc.height; i++, j+=Sl_Sc.width)
		row_table[i] = j;
	for (i=j=0; i<Sl_Sc.slices; i++, j+=Sl_Sc.width*Sl_Sc.height)
		slice_table[i] = j;
  }

  if (Sl_Sc.pix_size==16) {  /* if the data is 16 bits */
    /* ptr1 always points to the fist slice and ptr2 to the second */
    if (scene_data && max_slice>=min_slice)
	{ ptr1=(unsigned short *)scene_data;
      ptr2=(unsigned short *)(scene_data + Sl_Sc.bytes_per_slice);
      if (fseek(Sl_Sc.fp, Sl_Sc.ptr+min_slice*Sl_Sc.bytes_per_slice, 0)) {
	    display_error(5);
	    free(scene_data);
	    free(pl_slice_map);
		free(*sl_data);
        *sl_data=NULL;
		if (!interpolate)
		{ free(x_table[0]);
		  free(x_table[1]);
		  free(x_table[2]);
		  free(slice_table);
		  free(row_table);
		}
	    return (EROR);
	  }
      /* Read the Next slice */
	  j = VReadData((char *)ptr2, 2, Sl_Sc.width*Sl_Sc.height, Sl_Sc.fp, &n);
      if (j) {
        display_error(j);
	    free(scene_data);
	    free(pl_slice_map);
		free(*sl_data);
        *sl_data=NULL;
		if (!interpolate)
		{ free(x_table[0]);
		  free(x_table[1]);
		  free(x_table[2]);
		  free(slice_table);
		  free(row_table);
		}
        return (EROR);
      }
	}
	else
	  ptr2 = Sl_Sc.dbyte_data;
    memset(*sl_data, 0, 2*sl_width*sl_height);
    /* go through each slice and perform tri-linear interpolation */
    if (scene_data)
	  for(n=min_slice; n<=max_slice; n++) {
        /* swap so the first slice is in ptr1 and second is in ptr2 */
        tmp_ptr=ptr1; ptr1=ptr2; ptr2=tmp_ptr;
	    /* Read the Next slice */
	    j = VReadData((char *)ptr2, 2, Sl_Sc.width*Sl_Sc.height, Sl_Sc.fp, &i);
        if (j) {
          display_error(j);
		  free(scene_data);
		  free(pl_slice_map);
		  free(*sl_data);
          *sl_data=NULL;
		  if (!interpolate)
		  { free(x_table[0]);
		    free(x_table[1]);
		    free(x_table[2]);
		  	free(slice_table);
		  	free(row_table);
		  }
          return (EROR);
        }
        img_ptr = *sl_data;
		map = pl_slice_map;
		if (interpolate)
        { for(i=0;i<sl_height;i++)
	    	for(j=0;j<sl_width;j++,map++,img_ptr++) 
	    	if (*map==n) { /* if the lower slice index matches n */
	    	  point[0]= ends[0][0] + i*v[0] + j*u[0];
	    	  x=Floor(point[0]);
			  if (x>=0 && x<Sl_Sc.width-1) {
	    	    point[1]= ends[0][1] + i*v[1] + j*u[1];
	     	   y=Floor(point[1]);
	     	   if (y>=0 && y<Sl_Sc.height-1) {
	     	     point[2]= ends[0][2] + i*v[2] + j*u[2];
	     	     z=n;
	     	     dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
         	     /* if within bounds init locations in each slice */
	     	     pt1= (unsigned short *)ptr1 + y*Sl_Sc.width+x;
         	     pt2= (unsigned short *)ptr2 + y*Sl_Sc.width+x;
	     	     *img_ptr= (unsigned short)(.5+ 
		 	       (1-dz)*
		 		     ((1-dy)*((1-dx)* *pt1 +
					              dx* *(pt1+1)) +
			            dy  *((1-dx)* *(pt1+Sl_Sc.width) +
						          dx* *(pt1+Sl_Sc.width+1))
			         )+
			        dz *
			         ((1-dy)*((1-dx)* *pt2 +
					              dx* *(pt2+1)) +
			            dy  *((1-dx)* *(pt2+Sl_Sc.width) +
						          dx* *(pt2+Sl_Sc.width+1))
				     ));
			    }
	    	  }
	    	}
		}
		else /* !interpolate */
		  for(i=0;i<sl_height;i++)
	      {	x=(int)rint(ends[0][0] + i*v[0]);
	     	y=(int)rint(ends[0][1] + i*v[1]);
	    	for(j=0;j<sl_width;j++,map++,img_ptr++) 
	    	  if (*map==n /* if the slice index matches n */ &&
			  		x+x_table[0][j]>=0 && x+x_table[0][j]<Sl_Sc.width-1 &&
	     	  		y+x_table[1][j]>=0 && y+x_table[1][j]<Sl_Sc.height-1) {
	     	     pt1= (unsigned short *)ptr1 +
				 	row_table[y+x_table[1][j]]+x+x_table[0][j];
	     	     *img_ptr= *pt1;
			  }
		  }
      } 
	else /* scene_data == NULL */
	{
      img_ptr = *sl_data;
      for(i=0;i<sl_height;i++)
	    if (interpolate)
	      for(j=0;j<sl_width;j++,img_ptr++) 
	      { point[2]= ends[0][2] + i*v[2] + j*u[2];
	        if (point[2]<0 || point[2]>=Sl_Sc.slices-1)
		      continue;
		    point[0]= ends[0][0] + i*v[0] + j*u[0];
	        x=Floor(point[0]);
		    if (x>=0 && x<Sl_Sc.width-1) {
	          point[1]= ends[0][1] + i*v[1] + j*u[1];
	          y=Floor(point[1]);
	          if (y>=0 && y<Sl_Sc.height-1) {
	            z = Floor(point[2]);
                ptr1 = Sl_Sc.dbyte_data+Sl_Sc.width*Sl_Sc.height*z;
                ptr2 = ptr1+Sl_Sc.width*Sl_Sc.height;
	            dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                /* if within bounds init locations in each slice */
	            pt1= (unsigned short *)ptr1 + y*Sl_Sc.width+x;
                pt2= (unsigned short *)ptr2 + y*Sl_Sc.width+x;
	            *img_ptr= (unsigned short)(.5+ 
		          (1-dz)*
		           ((1-dy)*((1-dx)* *pt1 +
				                dx* *(pt1+1)) +
		              dy  *((1-dx)* *(pt1+Sl_Sc.width) +
					            dx* *(pt1+Sl_Sc.width+1))
		           )+
		          dz *
		           ((1-dy)*((1-dx)* *pt2 +
				                dx* *(pt2+1)) +
		              dy  *((1-dx)* *(pt2+Sl_Sc.width) +
					            dx* *(pt2+Sl_Sc.width+1))
			       ));
		      }
	        }
          }
		else /* !interpolate */
	    { x=(int)rint(ends[0][0] + i*v[0]);
	      y=(int)rint(ends[0][1] + i*v[1]);
		  z=(int)rint(ends[0][2] + i*v[2]);
	      for(j=0;j<sl_width;j++,img_ptr++)
	      { if (z+x_table[2][j]>=0 && z+x_table[2][j]<Sl_Sc.slices-1 &&
		    	x+x_table[0][j]>=0 && x+x_table[0][j]<Sl_Sc.width-1 &&
	        	y+x_table[1][j]>=0 && y+x_table[1][j]<Sl_Sc.height-1) {
              ptr1 = Sl_Sc.dbyte_data+
					slice_table[z+x_table[2][j]];
	          pt1= (unsigned short *)ptr1 +
			  	row_table[y+x_table[1][j]]+x+x_table[0][j];
	          *img_ptr= *pt1;
	        }
          }
		}
    }
  }
  else {
    /* ptr1 always points to the fist slice and ptr2 to the second */
    if (scene_data && max_slice>=min_slice)
	{ ptr1_8=(unsigned char *)scene_data;
      ptr2_8=(unsigned char *)(scene_data + Sl_Sc.bytes_per_slice);
	  if (fseek(Sl_Sc.fp, Sl_Sc.ptr+min_slice*Sl_Sc.bytes_per_slice, 0)) {
	    display_error(5);
	    free(pl_slice_map);
	    free(scene_data);
        free(*sl_data);
        *sl_data=NULL;
		if (!interpolate)
		{ free(x_table[0]);
		  free(x_table[1]);
		  free(x_table[2]);
		  free(slice_table);
		  free(row_table);
		}
	    return (EROR);
	  }
      /* Read the Next slice */
      if (fread(ptr2_8, 1, Sl_Sc.bytes_per_slice, Sl_Sc.fp) !=
			Sl_Sc.bytes_per_slice) {
        display_error(2);
	    free(pl_slice_map);
        free(scene_data);
        free(*sl_data);
        *sl_data=NULL;
		if (!interpolate)
		{ free(x_table[0]);
		  free(x_table[1]);
		  free(x_table[2]);
		  free(slice_table);
		  free(row_table);
		}
        return (EROR);
      }
	}
	else
	  ptr2_8 = Sl_Sc.byte_data;
    memset(*sl_data, 0, 2*sl_width*sl_height);
    /* go through each slice and perform tri-linear interpolation */
    if (scene_data)
	  for(n=min_slice; n<=max_slice; n++) {
        /* swap so the first slice is in ptr1 and second is in ptr2 */
        tmp_ptr_8=ptr1_8; ptr1_8=ptr2_8; ptr2_8=tmp_ptr_8;
        /* Read the Next slice */
        if (fread(ptr2_8, 1, Sl_Sc.bytes_per_slice, Sl_Sc.fp) !=
			Sl_Sc.bytes_per_slice) {
          display_error(2);
		  free(pl_slice_map);
		  free(scene_data);
          free(*sl_data);
          *sl_data=NULL;
		  if (!interpolate)
		  { free(x_table[0]);
		    free(x_table[1]);
		    free(x_table[2]);
		  	free(slice_table);
		  	free(row_table);
		  }
          return (EROR);
        }
        img_ptr = *sl_data;
		map = pl_slice_map;
		if (interpolate)
        { for(i=0;i<sl_height;i++)
	    	for(j=0;j<sl_width;j++,map++,img_ptr++) 
	    	  if (*map==n) { /* if the lower slice index matches n */
	            point[0]= ends[0][0] + i*v[0] + j*u[0];
	            point[1]= ends[0][1] + i*v[1] + j*u[1];
	            x=Floor(point[0]);
		        if (x>=0 && x<Sl_Sc.width-1) {
	              point[2]= ends[0][2] + i*v[2] + j*u[2];
	              y=Floor(point[1]);
	              if (y>=0 && y<Sl_Sc.height-1) {
	                z=n;
	                dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                    /* if within bounds init locations in each slice */
	                pt1_8= (unsigned char *)ptr1_8 + y*Sl_Sc.width+x;
                    pt2_8= (unsigned char *)ptr2_8 + y*Sl_Sc.width+x;
	                *img_ptr= (unsigned short)(.5+
                      (1-dz)*
		               ((1-dy)*((1-dx)* *pt1_8 +
				                    dx* *(pt1_8+1)) +
		                  dy  *((1-dx)* *(pt1_8+Sl_Sc.width)+
					                dx* *(pt1_8+Sl_Sc.width+1))
		               )+
		              dz *
		               ((1-dy)*((1-dx)* *pt2_8 +
				                    dx* *(pt2_8+1)) +
			               dy *((1-dx)* *(pt2_8+Sl_Sc.width) +
					                dx* *(pt2_8+Sl_Sc.width+1))
		               ));
		          }
	            }
			  }
		}
		else
          for(i=0;i<sl_height;i++)
	      {	x=(int)rint(ends[0][0] + i*v[0]);
	     	y=(int)rint(ends[0][1] + i*v[1]);
		    for(j=0;j<sl_width;j++,map++,img_ptr++) 
	    	  if (*map==n /* if the slice index matches n */ &&
			  		x+x_table[0][j]>=0 && x+x_table[0][j]<Sl_Sc.width-1 &&
	     	  		y+x_table[1][j]>=0 && y+x_table[1][j]<Sl_Sc.height-1) {
	             pt1_8= (unsigned char *)ptr1_8 +
				 	row_table[y+x_table[1][j]]+x+x_table[0][j];
	             *img_ptr= *pt1_8;
		      }
		  }
    }
	else {
      img_ptr = *sl_data;
      for(i=0;i<sl_height;i++)
	    if (interpolate)
	      for(j=0;j<sl_width;j++,img_ptr++) 
	      { point[2]= ends[0][2] + i*v[2] + j*u[2];
	        if (point[2]<0 || point[2]>=Sl_Sc.slices-1)
		      continue;
		    point[0]= ends[0][0] + i*v[0] + j*u[0];
	        x=Floor(point[0]);
		    if (x>=0 && x<Sl_Sc.width-1) {
	          point[1]= ends[0][1] + i*v[1] + j*u[1];
	          y=Floor(point[1]);
	          if (y>=0 && y<Sl_Sc.height-1) {
	            z = Floor(point[2]);
                ptr1_8 = Sl_Sc.byte_data+Sl_Sc.bytes_per_slice*z;
                ptr2_8 = ptr1_8+Sl_Sc.bytes_per_slice;
	            dx=point[0]-x; dy=point[1]-y; dz=point[2]-z;
                /* if within bounds init locations in each slice */
	            pt1_8= (unsigned char *)ptr1_8 + y*Sl_Sc.width+x;
                pt2_8= (unsigned char *)ptr2_8 + y*Sl_Sc.width+x;
	            *img_ptr= (unsigned short)(.5+
                  (1-dz)*
		           ((1-dy)*((1-dx)* *pt1_8 +
				                dx* *(pt1_8+1)) +
		                dy*((1-dx)* *(pt1_8+Sl_Sc.width) +
					            dx* *(pt1_8+Sl_Sc.width+1))
		           )+
		          dz *
		           ((1-dy)*((1-dx)* *pt2_8 +
				                dx* *(pt2_8+1)) +
			            dy*((1-dx)* *(pt2_8+Sl_Sc.width) +
					            dx* *(pt2_8+Sl_Sc.width+1))
		           ));
		      }
	        }
	      }
		else
	    { x=(int)rint(ends[0][0] + i*v[0]);
	      y=(int)rint(ends[0][1] + i*v[1]);
		  z=(int)rint(ends[0][2] + i*v[2]);
	      for(j=0;j<sl_width;j++,img_ptr++) 
	      { if (z+x_table[2][j]>=0 && z+x_table[2][j]<Sl_Sc.slices-1 &&
		    	x+x_table[0][j]>=0 && x+x_table[0][j]<Sl_Sc.width-1 &&
	        	y+x_table[1][j]>=0 && y+x_table[1][j]<Sl_Sc.height-1) {
              ptr1_8 = Sl_Sc.byte_data+
			    slice_table[z+x_table[2][j]];
	          pt1_8= (unsigned char *)ptr1_8 +
			    row_table[y+x_table[1][j]]+x+x_table[0][j];
	          *img_ptr= *pt1_8;
	        }
	      }
		}
	}
  }

  if (scene_data) {
    free(scene_data);
    free(pl_slice_map);
  }
  if (!interpolate)
  { free(x_table[0]);
	free(x_table[1]);
	free(x_table[2]);
	free(slice_table);
	free(row_table);
  }
  old_sl_data[sl_scene] = *sl_data;
  old_sl_width[sl_scene] = sl_width;
  old_sl_height[sl_scene] = sl_height;
  old_ends[sl_scene][0][0] = ends[0][0];
  old_ends[sl_scene][0][1] = ends[0][1];
  old_ends[sl_scene][0][2] = ends[0][2];
  old_ends[sl_scene][1][0] = ends[1][0];
  old_ends[sl_scene][1][1] = ends[1][1];
  old_ends[sl_scene][1][2] = ends[1][2];
  old_ends[sl_scene][2][0] = ends[2][0];
  old_ends[sl_scene][2][1] = ends[2][1];
  old_ends[sl_scene][2][2] = ends[2][2];
  old_interpolate[sl_scene] = interpolate;
  return (DONE);
}

/*****************************************************************************
 * FUNCTION: set_principal_plane
 * DESCRIPTION: Sets the cutting plane to the specified principal orientation.
 * PARAMETERS:
 *    plane_type: AXIAL, CORONAL, or SAGITTAL.
 * SIDE EFFECTS: The global variables plane_normal, plane_displacement,
 *    will be changed.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 4/10/95 by Dewey Odhner
 *    Modified: 7/17/95 trailing space removed from axis label by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::set_principal_plane(Principal_plane plane_type)
{
	Shell *obj;
	unsigned int j, k;
	StructureInfo *str;
	double rotation[3][3];
	float *domain;

	obj = actual_object(object_from_label(selected_object));
	str = &obj->main_data.file->file_header.str;
	if (str->axis_label_valid)
		for (j=0; j<3; j++)
		{	str->axis_label[j][0] = toupper(str->axis_label[j][0]);
			for (k=1; k<strlen(str->axis_label[j]); k++)
				str->axis_label[j][k] = tolower(str->axis_label[j][k]);
			if (str->axis_label[j][k-1] == ' ')
				str->axis_label[j][k-1] = 0;
			switch (plane_type)
			{	case AXIAL:
					if (strcmp(str->axis_label[j], "Head")==0 ||
							strcmp(str->axis_label[j], "Feet")==0)
						goto found;
					break;
				case CORONAL:
					if (strcmp(str->axis_label[j], "Anterior")==0 ||
							strcmp(str->axis_label[j], "Posterior")==0)
						goto found;
					break;
				case SAGITTAL:
					if (strcmp(str->axis_label[j], "Left")==0 ||
							strcmp(str->axis_label[j], "Right")==0)
						goto found;
					break;
			}
		}
	j = plane_type==AXIAL? 2: plane_type==CORONAL? 1: 0;
	found: ;
	for (k=0; k<3; k++)
		plane_normal[k] = j==k;
	domain = str->domain+12*obj->main_data.shell_number;
	/* convert to scanner orientation */
	rotation[0][0] = domain[3];
	rotation[1][0] = domain[4];
	rotation[2][0] = domain[5];
	rotation[0][1] = domain[6];
	rotation[1][1] = domain[7];
	rotation[2][1] = domain[8];
	rotation[0][2] = domain[9];
	rotation[1][2] = domain[10];
	rotation[2][2] = domain[11];
	matrix_vector_multiply(plane_normal, rotation, plane_normal);
	plane_displacement = 0;
}

/*****************************************************************************
 * FUNCTION: create_slice_image
 * DESCRIPTION: Adds a slice image structure to the tail of the slice list and
 *    displays the slice image.
 * PARAMETERS:
 *    size: The size in pixels of the image to be added (size x size square).
 *    x, y: The position of the top left of the image in display_area2
 * SIDE EFFECTS: The static variable last_slice is set to the new slice.
 *    The global variable message_displayed may be changed.
 *    An error message may be displayed in the dialog window.
 * ENTRY CONDITIONS: The global variable slice_list must be a valid linked
 *    list.  The global variables display, image_depth, true_color
 *    must be appropriately initialized.
 *    A successful call to manip_init must be made first.
 * RETURN VALUE: Non-zero if successful.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 3/31/93 by Dewey Odhner
 *    Modified: 6/28/93 parameters x, y added by Dewey Odhner
 *    Modified: 1/19/95 new slice ->interpolated flag set by Dewey Odhner
 *    Modified: 4/12/04 memory for 24-bit data allocated by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::create_slice_image(int size, int x, int y)
{
	Slice_image **image_ptr;

	assert(image_mode == SLICE);
	resize_image();
	for (image_ptr= &slice_list; *image_ptr; image_ptr= &(*image_ptr)->next)
		;
	*image_ptr = (Slice_image *)calloc(1, sizeof(Slice_image));
	if (*image_ptr == NULL)
	{	report_malloc_error();
		return (FALSE);
	}
	(*image_ptr)->image = &ximage2;
	memcpy((*image_ptr)->plane_normal, plane_normal, sizeof(plane_normal));
	(*image_ptr)->plane_displacement = plane_displacement;
	(*image_ptr)->interpolated = gray_interpolate;
	(*image_ptr)->object[0] = last_slice->object[0];
	(*image_ptr)->object[1] = last_slice->object[1];
	(*image_ptr)->on[0] = last_slice->on[0];
	(*image_ptr)->on[1] = last_slice->on[1];
	(*image_ptr)->gray_level[0] = last_slice->gray_level[0];
	(*image_ptr)->gray_width[0] = last_slice->gray_width[0];
	(*image_ptr)->inverted[0] = last_slice->inverted[0];
	(*image_ptr)->gray_min[0] = last_slice->gray_min[0];
	(*image_ptr)->gray_max[0] = last_slice->gray_max[0];
	(*image_ptr)->gray_window_valid[0] = last_slice->gray_window_valid[0];
	(*image_ptr)->gray_level[1] = last_slice->gray_level[1];
	(*image_ptr)->gray_width[1] = last_slice->gray_width[1];
	(*image_ptr)->inverted[1] = last_slice->inverted[1];
	(*image_ptr)->gray_min[1] = last_slice->gray_min[1];
	(*image_ptr)->gray_max[1] = last_slice->gray_max[1];
	(*image_ptr)->gray_window_valid[1] = last_slice->gray_window_valid[1];
	last_slice = *image_ptr;
	last_slice->x = x;
	last_slice->y = y;
	return (TRUE);
}

/*****************************************************************************
 * FUNCTION: destroy_slice_image
 * DESCRIPTION: Removes a slice image structure from the tail of the slice
 *    list.
 * PARAMETERS: None
 * SIDE EFFECTS: The static variable last_slice will be changed.
 * ENTRY CONDITIONS: The global variable slice_list and the static variable
 *    last_slice must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/2/93 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_slice_image(void)
{
	Slice_image **tail;

	if (slice_list == NULL)
		return;
	for (tail= &slice_list; *tail!=last_slice; tail= &(*tail)->next)
		;
	*tail = NULL;
	if (last_slice->valid)
		iw_valid = FALSE;
	if (last_slice->image_data[0].c)
		free(last_slice->image_data[0].c);
	if (last_slice->image_data[1].c)
		free(last_slice->image_data[1].c);
	if (last_slice->image->data)
		free(last_slice->image->data);
	last_slice->image->data = NULL;
	free(last_slice);
	if (slice_list)
		for (last_slice=slice_list; last_slice->next;
				last_slice=last_slice->next)
			;
	else
		last_slice = &null_slice;
}

/*****************************************************************************
 * FUNCTION: destroy_slice_list
 * DESCRIPTION: Removes the slice image structures from the slice list.
 * PARAMETERS: None
 * SIDE EFFECTS: The static variables last_slice and null_slice will be
 *    changed.  The gray map scales will be removed.
 * ENTRY CONDITIONS: The global variable slice_list and the static variable
 *    last_slice must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/21/93 by Dewey Odhner
 *    Modified: 5/10/93 to remove gray map scales by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_slice_list(void)
{
	while (slice_list)
		destroy_slice_image();
	null_slice.object[0] = NULL;
	null_slice.on[0] = FALSE;

}

/*****************************************************************************
 * FUNCTION: do_slice_output
 * DESCRIPTION: Saves the output of the Select Slice command.
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: Calls to VDisplayRunModeCommand and
 *    VDisplayOutputFilePrompt must be made first.
 *    The static variables last_slice, pixel_size,, out_slice_spacing,
 *    first_loc, last_loc, and the global variables object_list, dialog_window,
 *    main_image, scale, plane_normal, matched_output, gray_interpolate
 *    must be appropriately initialized.
 *    ButtonPress events should be selected.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If the output file already exists, the user will be asked
 *    whether to overwrite it, and if he/she says no, the function will return
 *    without saving.  Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/13/93 by Dewey Odhner
 *    Modified: 9/30/94 to set do_slice_output_called by Dewey Odhner
 *    Modified: 4/1/98 matched output done by Dewey Odhner
 *    Modified: 10/3/03 -n passed to reslice_proc by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::do_slice_output(const char out_file_name[], double first_loc,
	double last_loc, double out_pixel_size, bool matched_output)
{
	char *in_file_name, bad_group[5], bad_element[5],
		*command_line, *matching_file_name;
	int choice=0, error_code, run_mode=0, image_size;
	Shell *object, *matching_object;
	FILE *scene_file;
	ViewnixHeader vh;
	double ends[4][3], dx[3], dy[3], dz[3], temp_point[3], temp_rotation[3][3],
		rotation[3][3];
	Principal_plane plane_type;
	StructureInfo *str;
	float *domain;
	Slice_image *last_slice;

	if (slice_list)
		for (last_slice=slice_list; last_slice->next;
				last_slice=last_slice->next)
			;
	else
		last_slice = &null_slice;
	if (!last_slice->on[0])
		choice = 1;
	object = last_slice->object[choice];
	in_file_name =
		object->main_data.file->file_header.str.scene_file_valid
		?	object->main_data.file->file_header.str.scene_file[
				object->main_data.shell_number]
		:	object->main_data.file->file_header.gen.filename1;
	if (in_file_name == NULL)
	{
		display_message("GREY DATA UNKNOWN.");
		return;
	}
	command_line =
		(char *)malloc(268+strlen(in_file_name)+strlen(out_file_name)+
		Preferences::getHome().Len());
	if (matched_output)
	{
		matching_object = last_slice->on[!choice]? last_slice->object[!choice]:
			actual_object(object_from_label(selected_object));
		matching_file_name =
			matching_object->main_data.file->file_header.str.scene_file_valid
			?	matching_object->main_data.file->file_header.str.scene_file[
					matching_object->main_data.shell_number]
			:	matching_object->main_data.file->file_header.gen.filename1;
		if (strcmp(matching_file_name, in_file_name) == 0)
		{
			display_message("Specify another scene to match with.");
			free(command_line);
			return;
		}

		/* find transformation from object's scanner coordinate system to
			matching_object's scanner coordinate system */
		/* object's scanner coordinate system to object's structure
			coordinate system */
		str = &object->main_data.file->file_header.str;
		domain = str->domain+12*object->main_data.shell_number;
		dx[0] = -unit_size(str->measurement_unit[0])*domain[0];
		dx[1] = -unit_size(str->measurement_unit[1])*domain[1];
		dx[2] = -unit_size(str->measurement_unit[2])*domain[2];
		rotation[0][0] = domain[3];
		rotation[0][1] = domain[4];
		rotation[0][2] = domain[5];
		rotation[1][0] = domain[6];
		rotation[1][1] = domain[7];
		rotation[1][2] = domain[8];
		rotation[2][0] = domain[9];
		rotation[2][1] = domain[10];
		rotation[2][2] = domain[11];
		matrix_vector_multiply(dx, rotation, dx);

		/* object's structure coordinate system to object's center */
		dx[0] -= unit_size(str->measurement_unit[0])*
			.5*(str->min_max_coordinates[6*object->main_data.shell_number+3]+
			str->min_max_coordinates[6*object->main_data.shell_number]);
		dx[1] -= unit_size(str->measurement_unit[1])*
			.5*(str->min_max_coordinates[6*object->main_data.shell_number+4]+
			str->min_max_coordinates[6*object->main_data.shell_number+1]);
		dx[2] -= unit_size(str->measurement_unit[2])*
			.5*(str->min_max_coordinates[6*object->main_data.shell_number+5]+
			str->min_max_coordinates[6*object->main_data.shell_number+2]);

		/* object's center to image space */
		get_object_transformation(temp_rotation, dy, object, FALSE);
		matrix_multiply(rotation, temp_rotation, rotation);
		matrix_vector_multiply(dx, temp_rotation, dx);
		vector_add(dx, dx, dy);

		/* image space to matching_object's center */
		str = &matching_object->main_data.file->file_header.str;
		get_object_transformation(temp_rotation, dy, matching_object, FALSE);
		transpose(temp_rotation, temp_rotation);
		vector_subtract(dx, dx, dy);
		matrix_vector_multiply(dx, temp_rotation, dx);
		matrix_multiply(rotation, temp_rotation, rotation);

		/* matching_object's center to
			matching_object's structure coordinate system */
		dx[0] += unit_size(str->measurement_unit[0])*.5*(str->
			min_max_coordinates[6*matching_object->main_data.shell_number+3]+
			str->min_max_coordinates[
			6*matching_object->main_data.shell_number]);
		dx[1] += unit_size(str->measurement_unit[1])*.5*(str->
			min_max_coordinates[6*matching_object->main_data.shell_number+4]+
			str->min_max_coordinates[
			6*matching_object->main_data.shell_number+1]);
		dx[2] += unit_size(str->measurement_unit[2])*.5*(str->
			min_max_coordinates[6*matching_object->main_data.shell_number+5]+
			str->min_max_coordinates[
			6*matching_object->main_data.shell_number+2]);

		/* matching_object's structure coordinate system to
			matching_object's scanner coordinate system */
		domain = str->domain+12*matching_object->main_data.shell_number;
		temp_rotation[0][0] = domain[3];
		temp_rotation[1][0] = domain[4];
		temp_rotation[2][0] = domain[5];
		temp_rotation[0][1] = domain[6];
		temp_rotation[1][1] = domain[7];
		temp_rotation[2][1] = domain[8];
		temp_rotation[0][2] = domain[9];
		temp_rotation[1][2] = domain[10];
		temp_rotation[2][2] = domain[11];
		matrix_multiply(rotation, temp_rotation, rotation);
		matrix_vector_multiply(dx, temp_rotation, dx);
		dx[0] += unit_size(str->measurement_unit[0])*domain[0];
		dx[1] += unit_size(str->measurement_unit[1])*domain[1];
		dx[2] += unit_size(str->measurement_unit[2])*domain[2];

		sprintf(command_line,
		   "\"%s/matched_reslice\" %s %s %s.IM0 %f %f %f %f %f %f %f %f %f %f %f %f%s",
			(const char *)Preferences::getHome().c_str(), in_file_name,
			matching_file_name, out_file_name,
			rotation[0][0], rotation[0][1], rotation[0][2], dx[0],
			rotation[1][0], rotation[1][1], rotation[1][2], dx[1],
			rotation[2][0], rotation[2][1], rotation[2][2], dx[2],
			gray_interpolate? "": " -n");
	}
	else
	{
		scene_file = fopen(in_file_name, "rb");
		if (scene_file == NULL)
		{
			display_message("Failed to open input file.");
			free(command_line);
			return;
		}
		error_code = VReadHeader(scene_file, &vh, bad_group, bad_element);
		fclose(scene_file);
		switch (error_code)
		{	case 0:
			case 107:
			case 106:
				break;
			default:
				display_message("Operation failed.");
				fprintf(stderr,"Group %s element %s undefined in VReadHeader\n"
					, bad_group, bad_element);
				free(command_line);
				return;
		}
		image_size = (int)ceil(main_image->width/(scale*out_pixel_size));
		get_plane_ends(object, ends, image_size,
			image_size, &vh.scn, first_loc, last_loc, plane_normal, temp_point,
			temp_point, &plane_type);
		destroy_scene_header(&vh);

		if (first_loc == last_loc)
			memcpy(ends[3], ends[0], sizeof(ends[3]));
		else
		{	/* make the system right-handed */
			vector_subtract(dx, ends[1], ends[0]);
			vector_subtract(dy, ends[2], ends[0]);
			vector_subtract(dz, ends[3], ends[0]);
			if ((dx[1]*dy[2]-dx[2]*dy[1])*dz[0]+(dx[2]*dy[0]-dx[0]*dy[2])*dz[1]
					+(dx[0]*dy[1]-dx[1]*dy[0])*dz[2] < 0)
			{	memcpy(temp_point, ends[0], sizeof(temp_point));
				memcpy(ends[0], ends[1], sizeof(ends[0]));
				memcpy(ends[1], temp_point, sizeof(ends[1]));
				vector_add(ends[2], ends[2], dx);
				vector_add(ends[3], ends[3], dx);
			}
		}

		sprintf(command_line,
			"\"%s/reslice_proc\" \"%s\" \"%s\" %f %f %f %f %f %f %f %f %f %f %f %f %d %d %d%s",
			(const char *)Preferences::getHome().c_str(), in_file_name, out_file_name, ends[0][0], ends[0][1], ends[0][2],
			ends[1][0], ends[1][1], ends[1][2],
			ends[2][0], ends[2][1], ends[2][2],
			ends[3][0], ends[3][1], ends[3][2], image_size,
			(int)fabs((last_loc-first_loc)/out_slice_spacing)+1, run_mode,
			gray_interpolate? "": " -n");
	}

	run_command(command_line, run_mode==0);
}

/*****************************************************************************
 * FUNCTION: objects_need_be_turned_off
 * DESCRIPTION: Determines whether more than one object is on for operations
 *    that require only one object.
 * PARAMETERS: None
 * SIDE EFFECTS: Displays a message if objects need to be turned off.
 *    Sets error_flag if objects need to be turned off.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  A successful call to VCreateColormap must
 *    be made first.  The global variables argv0, windows_open, dialog_window,
 *    separate_piece1, separate_piece2, icons_exist
 *    must be appropriately initialized.
 * RETURN VALUE: TRUE if objects need to be turned off.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 3/27/95 secondary objects counted instead of
 *       separate_piece1, separate_piece2 by Dewey Odhner.
 *    Modified: 2/10/04 corrected check for t-shell by Dewey Odhner.
 *    Modified: 7/16/09 error_flag set by Dewey Odhner.
 *
 *****************************************************************************/
bool cvRenderer::objects_need_be_turned_off()
{
	Shell *obj;
	int num_of_objects, num_of_secondary_objects;

	num_of_objects = num_of_secondary_objects = 0;
	for (obj=object_list; obj; obj=obj->next)
		if (obj->O.on)
		{	num_of_objects++;
			if (st_cl(&obj->main_data)==T_SHELL ||
					(icons_exist && st_cl(&obj->icon_data)==T_SHELL))
			{
				display_message("Cannot do T-SHELL structures.");
				error_flag = true;
				return (true);
			}
			if (obj->secondary)
				num_of_secondary_objects++;
			if (obj->reflection!=NULL && obj->reflection->on)
			{	num_of_objects++;
				if (obj->secondary)
					num_of_secondary_objects++;
			}
			if (num_of_objects>2 || (num_of_objects==2 &&
					(image_mode!=SEPARATE ||
					separate_piece1==NULL || !separate_piece1->O.on ||
					separate_piece2==NULL || !separate_piece2->O.on) &&
					(image_mode!=FUZZY_CONNECT|| num_of_secondary_objects!=1)))
			{	display_message("Turn off all but one object.");
				error_flag = true;
				return (true);
			}
		}
	return (false);
}

/*****************************************************************************
 * FUNCTION: display_3d_line
 * DESCRIPTION: Displays the parts of a line in display_area that are not
 *    hidden by parts of objects.
 * PARAMETERS:
 *    x, y, z: The display_area coordinates of one end of the line.
 *    last_x, last_y, last_z: The display_area coordinates of the other
 *       end of the line.
 * SIDE EFFECTS: May issue an error message.
 * ENTRY CONDITIONS: A call to manip_init should be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Returns without displaying the line if both ends are on
 *    the same pixel.  Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/4/00 overlay bypassed when needed by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_3d_line(int x, int y, int z, int last_x, int last_y,
	int last_z, X_Point *&xpoints, int &visible_points)
{
	int nxpoints, this_point, this_z, delta_x, delta_y;
	double inv_sqr;

	visible_points = 0;
	if (VComputeLine(x, y, last_x, last_y, &xpoints, &nxpoints) == 1)
	{
		report_malloc_error();
		return;
	}
	if (nxpoints == 1)
	{
		free(xpoints);
		return;
	}
	delta_x = last_x-x;
	delta_y = last_y-y;
	inv_sqr = 1.0/(delta_x*delta_x+delta_y*delta_y);
	for (this_point=0; this_point<nxpoints; this_point++)
	{
		this_z = z+(int)(
				((xpoints[this_point].x-x)*delta_x+
				 (xpoints[this_point].y-y)*delta_y
				)*inv_sqr*(last_z-z)
			);
		if (this_z >= closest_z(xpoints[this_point].x, xpoints[this_point].y,
				main_image))
			xpoints[visible_points++] = xpoints[this_point];
	}
}

/*****************************************************************************
 * FUNCTION: draw_line
 * DESCRIPTION: Draws the axis of movement using the overlay in display_area.
 * PARAMETERS:
 *    interrupt_priority: Assigns a priority to an event; events of priority
 *       FIRST will cause draw_box to return unfinished; events of priority
 *       SECOND will remain in the queue; events of priority IGNORE will
 *       be discarded.  No parameters are passed to interrupt_priority.
 * SIDE EFFECTS: The following global variables may be changed: current_window,
 *    overlay_bad, overlay_clear, message_displayed.
 *    The static variables waiting, line_drawn may be changed.
 *    Extraneous events will be removed from the queue.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If call is interrupted, will return leaving the
 *    static variable line_drawn unset.
 * HISTORY:
 *    Created: 2/23/93 by Dewey Odhner
 *    Modified: 3/31/94 to check for true_color by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_line(X_Point *&xpoints, int &visible_points)
{
	double line_ends[2][3], line_direction[3];
	int x1, x2, y1, y2, z1, z2;

	AtoV(line_direction, line_angle[0], line_angle[1]);
	line_ends[0][0] = main_image->width/scale*line_direction[0];
	line_ends[0][1] = main_image->width/scale*line_direction[1];
	line_ends[0][2] = main_image->width/scale*line_direction[2];
	line_ends[1][0] = -line_ends[0][0];
	line_ends[1][1] = -line_ends[0][1];
	line_ends[1][2] = -line_ends[0][2];
	line_ends[0][0] += line_displacement[0];
	line_ends[0][1] += line_displacement[1];
	line_ends[0][2] += line_displacement[2];
	line_ends[1][0] += line_displacement[0];
	line_ends[1][1] += line_displacement[1];
	line_ends[1][2] += line_displacement[2];
	plan_to_display_area_coords(&x1, &y1, &z1, line_ends[0]);
	plan_to_display_area_coords(&x2, &y2, &z2, line_ends[1]);
	get_3d_line(x1, y1, z1, x2, y2, z2, xpoints, visible_points);
}

/*****************************************************************************
 * FUNCTION: plan_to_display_area_coords
 * DESCRIPTION: Converts plan coordinates of a point to display_area
 *    coordinates.
 * PARAMETERS:
 *    x, y, z: The display_area coordinates of the point go here.
 *    plan_coords: The plan coordinates of the point.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables glob_angle, scale, depth_scale,
 *    main_image, image_x, image_y must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::plan_to_display_area_coords(int *x, int *y, int *z,
	double plan_coords[3])
{
	double rotation[3][3], image_coords[3];

	AtoM(rotation, glob_angle[0], glob_angle[1], glob_angle[2]);
	matrix_vector_multiply(image_coords, rotation, plan_coords);
	/* convert from physical units */
	image_coords[0] *= scale;
	image_coords[1] *= scale;
	*z = (int)rint(MIDDLE_DEPTH-image_coords[2]*depth_scale);
	/* convert to display area coordinates */
	*x = (int)rint(image_coords[0]+main_image->width/2+image_x);
	*y = (int)rint(image_coords[1]+main_image->height/2+image_y);
}

#define SL_FW 4
#define RO_FW 2
#define CO_FW 1
#define SL_BW 0
#define RO_BW 0
#define CO_BW 0

/*****************************************************************************
 * MACRO: Intersect_voxel
 * DESCRIPTION: Computes the intersection points of a voxel of the selected
 *    object with the rows of the selected scene.  Output will be in an array
 *    of linked lists at unsigned *intersection_table, with indexes into
 *    Row_intersection *intersection_buffer.  Intersection points with smaller
 *    scene x-coordinates are added first; they are added to the head of the
 *    list.
 * PARAMETERS: None
 * SIDE EFFECTS: The following variables may be changed: min_row, max_row,
 *    min_slice, max_slice, face, i, j, k, scn_slice, buffer_size,
 *    intersection_buffer, intersection_table, buffer_used.
 *    An error message may be displayed.
 * ENTRY CONDITIONS: The following variables must be properly set:
 *    FILE *fp=NULL: pointer to the scene file;
 *    ViewnixHeader vh: header information from the scene file;
 *    int slices, width, height: the number of slices, width and height in
 *       pixels of each slice in the scene;
 *    double matrix[3][3], vector[3]: the transformation from the structure
 *       coordinates, units of voxels relative to first voxel of first slice,
 *       to the scene coordinate system, units of voxels;
 *    unsigned *intersection_table: the pointer to a table that has a linked
 *       list of object intersections for each row of each slice of the scene;
 *       the value is an index into the array at intersection_buffer, or
 *       0x7fffffff for an empty list;
 *    Row_intersection *intersection_buffer: the array containing the object-
 *       row inersection entries; must be dynamically allocated;
 *    unsigned long buffer_size, buffer_used: the number of entries that can
 *       be, and actually are, respectively, in intersection_buffer;
 *    int obj_row, obj_slice: the current row and slice of the object;
 *    int is_start[6]: a table which tells whether each voxel face faces the
 *       -x direction of the scene;
 *    unsigned short *this_ptr: points to the current voxel in the object;
 *    double g[3], ecoeff[3], qcoeff[3][2]:
 *       g[k] = .5*(matrix[2][j]*matrix[1][i]-matrix[1][j]*matrix[2][i]),
 *       ecoeff[i] = -(g[j]*matrix[0][k]+g[k]*matrix[0][j])/(4*g[i])-
 *          .5*matrix[0][i],
 *       qcoeff[i][0] =
 *          (matrix[2][k]*matrix[0][k]-matrix[2][j]*matrix[0][j])/(4*g[i]),
 *       qcoeff[i][1] =
 *          (matrix[1][j]*matrix[0][j]-matrix[1][k]*matrix[0][k])/(4*g[i]),
 *       where j = (i+1)%3, k = (i+2)%3;
 *    double voxel_range[2]: the deviation in scene-voxel y and z coordinates
 *       of the corner of an object-voxel from its center;
 *    static itable[8]={0, 1, 2, 0, 1, 2, 0, 1};
 *    static nmask[6]={PX, PY, PZ, NX, NY, NZ}.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None; may return ERROR from the calling function.
 * EXIT CONDITIONS: May return ERROR from the calling function.
 * HISTORY:
 *    Created: 2/10/94 by Dewey Odhner
 *
 *****************************************************************************/
#define Intersect_voxel \
{	double vc[3], q[3], r; \
	int min_row, max_row, min_slice, max_slice, start_faces, \
		scn_row, scn_slice; \
\
	vc[0] = data_class==BINARY_B? \
		((*this_ptr&0x3ff)<<1|((this_ptr[1]&0x8000)!=0)): *this_ptr&0x3ff; \
	vc[1] = obj_row; \
	vc[2] = obj_slice; \
	matrix_vector_multiply(vc, matrix, vc); \
	vector_add(vc, vc, vector); \
	min_row = (int)ceil(vc[1]-voxel_range[0]); \
	max_row = (int)floor(vc[1]+voxel_range[0]); \
	min_slice = (int)ceil(vc[2]-voxel_range[1]); \
	max_slice = (int)floor(vc[2]+voxel_range[1]); \
	if (min_row < 0) \
		min_row = 0; \
	if (max_row >= height) \
		max_row = height-1; \
	if (min_slice < 0) \
		min_slice = 0; \
	if (max_slice >= slices) \
		max_slice = slices-1; \
	for (start_faces=0; start_faces<=1; start_faces++) \
		for (face=0; face<6; face++) \
			if (is_start[face] == start_faces) \
			{	if (*this_ptr&nmask[face]) \
					continue; \
				i = itable[face]; \
				j = jtable[face]; \
				k = ktable[face]; \
				for (scn_slice=min_slice; scn_slice<=max_slice; scn_slice++) \
				{	q[2] = scn_slice-vc[2]; \
					for (scn_row=min_row; scn_row<=max_row; scn_row++) \
					{	q[1] = scn_row-vc[1]; \
						r = matrix[2][j]*q[1]-matrix[1][j]*q[2]+Etimes(g[k]); \
						if (r<-absg[i] || r>absg[i]) \
							continue; \
						r = matrix[2][k]*q[1]-matrix[1][k]*q[2]-Etimes(g[j]); \
						if (r<-absg[i] || r>absg[i]) \
							continue; \
						if (buffer_used == buffer_size) \
						{	Row_intersection *tmp_buf; \
\
							buffer_size += Buffer_block; \
							tmp_buf = (Row_intersection *) \
								realloc(intersection_buffer, \
								buffer_size*sizeof(Row_intersection)); \
							if (tmp_buf == NULL) \
							{	report_malloc_error(); \
								free(intersection_buffer); \
								free(intersection_table); \
								destroy_scene(&vh, &fp, &byte_data, \
									&dbyte_data); \
								return (EROR); \
							} \
							intersection_buffer = tmp_buf; \
						} \
						intersection_buffer[buffer_used].next = \
							intersection_table[scn_slice*height+scn_row]; \
						intersection_table[scn_slice*height+scn_row] = \
							buffer_used; \
						intersection_buffer[buffer_used].is_start=start_faces;\
						intersection_buffer[buffer_used].x = \
							qcoeff[i][0]*q[1]+qcoeff[i][1]*q[2]+ \
							Etimes(ecoeff[i])+vc[0]; \
						buffer_used++; \
					} \
				} \
			} \
}


/*****************************************************************************
 * FUNCTION: get_object_roi_stats
 * DESCRIPTION: Computes the statistics in the region of the selected object.
 * PARAMETERS:
 *    total_density, mean_density, standard_deviation, min, max: The total
 *       density, mean density, standard deviation, minimum, and maximum
 *       values of pixels within the region are returned here.
 *    event_priority: Returns the priority of any event.
 * SIDE EFFECTS: The global variable stats_valid will be set to indicate
 *    success.  An error message may be displayed.  Events of priority IGNORE
 *    will be removed from the queue.
 * ENTRY CONDITIONS: The global variables selected_object, object_list,
 *    and the static variable selected_scene must be appropriately set.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: DONE, INTERRUPT3, or EROR.
 * EXIT CONDITIONS: Returns INTERRUPT3 if a priority FIRST event is found.
 * HISTORY:
 *    Created: 2/10/94 by Dewey Odhner
 *    Modified: 5/11/94 to return no statistics for fuzzy shell by Dewey Odhner
 *    Modified: 5/4/95 crash fixed by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::get_object_roi_stats(double *total_density,
	double *mean_density, double *standard_deviation, double *min, double *max,
	Priority (*event_priority)(cvRenderer *))
{
	int j, k;
	double sum_squares;
	unsigned long N;
	FILE *fp=NULL;
	ViewnixHeader vh;
	Shell *scene_obj, *selected_obj;
	char file_name[80];
	int slices, width, height, bits;
	unsigned long hdrlen, scene_min, scene_max, bytes_per_slice;
	double matrix[3][3], vector[3], diag_matrix[3][3], unit;
	double rotation[3][3];
	Shell_data *obj_data;
	StructureInfo *str;
	SceneInfo *scn;
	float *domain;
	unsigned *intersection_table;
	typedef struct Row_intersection {
		double x;
		unsigned int is_start:1;
		unsigned int next:31;/* index into buffer; 0x7fffffff terminates list*/
	} Row_intersection;
	Row_intersection *intersection_buffer;
	unsigned char *byte_data=NULL;
	unsigned short *dbyte_data=NULL;
	Classification_type data_class;

	selected_obj = actual_object(object_from_label(selected_object));
	scene_obj = selected_obj;
	memset(&vh, 0, sizeof(vh));
	if (load_scene(scene_obj, file_name, &fp, &vh, &slices, &width, &height,
			&hdrlen, &bits, &scene_min, &scene_max, &bytes_per_slice,
			&byte_data, &dbyte_data))
		return (EROR);
	obj_data = &selected_obj->main_data;
	data_class = st_cl(obj_data);
	if (data_class!=BINARY_B && data_class!=BINARY_A)
		return (EROR);
	str = &obj_data->file->file_header.str;
	unit = unit_mm(obj_data);
	diag_matrix[0][0] = str->xysize[0]*unit;
	diag_matrix[1][1] = str->xysize[1]*unit;
	diag_matrix[2][2] = Slice_spacing(obj_data)*unit;
	for (j=0; j<3; j++)
		for (k=0; k<j; k++)
			diag_matrix[j][k] = diag_matrix[k][j] = 0;
	vector[0] =
		-.5*unit*(Max_coordinate(obj_data, 0)+(Min_coordinate(obj_data, 0)));
	vector[1] =
		-.5*unit*(Max_coordinate(obj_data, 1)-(Min_coordinate(obj_data, 1)));
	vector[2] =
		-.5*unit*(Max_coordinate(obj_data, 2)-(Min_coordinate(obj_data, 2)));
	AtoM(rotation, selected_obj->angle[0], selected_obj->angle[1],
		selected_obj->angle[2]);
	matrix_multiply(matrix, rotation, diag_matrix);
	matrix_vector_multiply(vector, rotation, vector);
	vector_add(vector, selected_obj->displacement, vector);
	/* Now we have transformation from structure to plan coordinates
		in matrix, vector. */
	/* Now go to scene coordinates. */
	obj_data = &scene_obj->main_data;
	unit = unit_mm(obj_data);
	str = &obj_data->file->file_header.str;
	scn = &vh.scn;
	vector_subtract(vector, vector, scene_obj->displacement);
	AtoM(rotation, scene_obj->angle[0], scene_obj->angle[1],
		scene_obj->angle[2]);
	transpose(rotation, rotation);
	matrix_vector_multiply(vector, rotation, vector);
	matrix_multiply(matrix, rotation, matrix);
	vector[0] +=
		.5*unit*(Max_coordinate(obj_data, 0)+(Min_coordinate(obj_data, 0)));
	vector[1] +=
		.5*unit*(Max_coordinate(obj_data, 1)+(Min_coordinate(obj_data, 1)));
	vector[2] +=
		.5*unit*(Max_coordinate(obj_data, 2)+(Min_coordinate(obj_data, 2)));
	/* Now we have transformation to structure coordinate system of the
		object derived from the scene. */
	domain = str->domain+12*scene_obj->main_data.shell_number;
	/* convert to scanner orientation */
	rotation[0][0] = domain[3];
	rotation[1][0] = domain[4];
	rotation[2][0] = domain[5];
	rotation[0][1] = domain[6];
	rotation[1][1] = domain[7];
	rotation[2][1] = domain[8];
	rotation[0][2] = domain[9];
	rotation[1][2] = domain[10];
	rotation[2][2] = domain[11];
	matrix_vector_multiply(vector, rotation, vector);
	matrix_multiply(matrix, rotation, matrix);
	/* shift origin */
	vector[0] += domain[0]*unit_size(str->measurement_unit[0])-
		scn->domain[0]*unit_size(scn->measurement_unit[0]);
	vector[1] += domain[1]*unit_size(str->measurement_unit[1])-
		scn->domain[1]*unit_size(scn->measurement_unit[1]);
	vector[2] += domain[2]*unit_size(str->measurement_unit[2])-
		scn->domain[2]*unit_size(scn->measurement_unit[2]);
	/* convert to scene orientation */
	rotation[0][0] = scn->domain[3];
	rotation[0][1] = scn->domain[4];
	rotation[0][2] = scn->domain[5];
	rotation[1][0] = scn->domain[6];
	rotation[1][1] = scn->domain[7];
	rotation[1][2] = scn->domain[8];
	rotation[2][0] = scn->domain[9];
	rotation[2][1] = scn->domain[10];
	rotation[2][2] = scn->domain[11];
	matrix_vector_multiply(vector, rotation, vector);
	matrix_multiply(matrix, rotation, matrix);
	vector[2] -= scn->loc_of_subscenes[0];
	/* convert from physical coordinates to voxel coordinates */
	diag_matrix[0][0]= 1/(scn->xypixsz[0]*unit_size(scn->measurement_unit[0]));
	diag_matrix[1][1]= 1/(scn->xypixsz[1]*unit_size(scn->measurement_unit[1]));
	diag_matrix[2][2] = 1/(unit_size(scn->measurement_unit[2])*
		(scn->num_of_subscenes[0]<=1? scn->xypixsz[0]:
		(scn->loc_of_subscenes[scn->num_of_subscenes[0]-1]-
		 scn->loc_of_subscenes[0])/(scn->num_of_subscenes[0]-1)));
	matrix_vector_multiply(vector, diag_matrix, vector);
	matrix_multiply(matrix, diag_matrix, matrix);

	/* Compute intersection of object with scene domain */
	{	unsigned long buffer_size, buffer_used;
		int face, i, order, obj_row, obj_slice, is_start[6];
		unsigned short *this_ptr, **next_ptr_ptr, *next_ptr;
		double g[3], ecoeff[3], qcoeff[3][2] /* Note: qcoeff[i][0] is coeffi-
			cient of q[1] in x[i]; qcoeff[i][1] is coefficient of q[2] */;
		double voxel_range[2], absg[3];
		static int itable[8]={0, 1, 2, 0, 1, 2, 0, 1},
			nmask[6]={PX, PY, PZ, NX, NY, NZ};
#define jtable (itable+1)
#define ktable (itable+2)

#define Etimes(x) (face<3? -(x): (x))

#define Buffer_block 0x1000

		if (event_priority(this) == FIRST)
		{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
			return (INTERRUPT3);
		}
		obj_data = &selected_obj->main_data;
		intersection_buffer = (Row_intersection *)malloc(Buffer_block*
			sizeof(Row_intersection));
		if (intersection_buffer == NULL)
		{	report_malloc_error();
			destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
			return (EROR);
		}
		buffer_size = Buffer_block;
		intersection_table = (unsigned *)malloc(slices*height*sizeof(int));
		if (intersection_table == NULL)
		{	report_malloc_error();
			destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
			free(intersection_buffer);
			return (EROR);
		}
		for (j=0; j<slices*height; j++)
			intersection_table[j] = 0x7fffffff;
		buffer_used = 0;
		voxel_range[0] = voxel_range[1] = 0;
		for (i=0; i<3; i++)
		{	j = jtable[i];
			k = ktable[i];
			g[k] = .5*(matrix[2][j]*matrix[1][i]-matrix[1][j]*matrix[2][i]);
			absg[k] = g[k]<0? -g[k]: g[k];
			voxel_range[0] += matrix[1][i]>0? matrix[1][i]: -matrix[1][i];
			voxel_range[1] += matrix[2][i]>0? matrix[2][i]: -matrix[2][i];
		}
		voxel_range[0] *= .5;
		voxel_range[1] *= .5;
		for (i=0; i<3; i++)
		{	j = jtable[i];
			k = ktable[i];
			if (g[i])
			{	qcoeff[i][0] = (matrix[2][k]*matrix[0][k]-
					matrix[2][j]*matrix[0][j])/(4*g[i]);
				qcoeff[i][1] = (matrix[1][j]*matrix[0][j]-
					matrix[1][k]*matrix[0][k])/(4*g[i]);
				ecoeff[i] = -(g[j]*matrix[0][k]+g[k]*matrix[0][j])/(4*g[i])-
					.5*matrix[0][i];
			}
			is_start[i] = matrix[0][i]<0;
			is_start[i+3] = !is_start[i];
		}
		order = (is_start[0]? CO_FW: CO_BW) +
		        (is_start[1]? RO_FW: RO_BW) +
		        (is_start[2]? SL_FW: SL_BW);
		switch (order)
		{
			case CO_BW+RO_BW+SL_BW:
				for (obj_slice=obj_data->slices-1; obj_slice>=0; obj_slice--)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+(obj_slice+1)*obj_data->rows-1;
					this_ptr = next_ptr_ptr[1]-2;
					for (obj_row=obj_data->rows-1; obj_row>=0; obj_row--)
					{	for (next_ptr= *next_ptr_ptr;
								this_ptr>=next_ptr; this_ptr-=2)
						Intersect_voxel
						next_ptr_ptr--;
					}
				}
				break;
			case CO_FW+RO_BW+SL_BW:
				for (obj_slice=obj_data->slices-1; obj_slice>=0; obj_slice--)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+(obj_slice+1)*obj_data->rows;
					for (obj_row=obj_data->rows-1; obj_row>=0; obj_row--)
					{	this_ptr = next_ptr_ptr[-1];
						for (next_ptr= *next_ptr_ptr;
								this_ptr<next_ptr; this_ptr+=2)
						Intersect_voxel
						next_ptr_ptr--;
					}
				}
				break;
			case CO_BW+RO_FW+SL_BW:
				for (obj_slice=obj_data->slices-1; obj_slice>=0; obj_slice--)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+obj_slice*obj_data->rows+1;
					for (obj_row=0; obj_row<obj_data->rows; obj_row++)
					{	this_ptr = *next_ptr_ptr-2;
						for (next_ptr=next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=2)
						Intersect_voxel
						next_ptr_ptr++;
					}
				}
				break;
			case CO_FW+RO_FW+SL_BW:
				for (obj_slice=obj_data->slices-1; obj_slice>=0; obj_slice--)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+obj_slice*obj_data->rows+1;
					this_ptr = next_ptr_ptr[-1];
					for (obj_row=0; obj_row<obj_data->rows; obj_row++)
					{	for (next_ptr= *next_ptr_ptr;
								this_ptr<next_ptr; this_ptr+=2)
						Intersect_voxel
						next_ptr_ptr++;
					}
				}
				break;
			case CO_BW+RO_BW+SL_FW:
				for (obj_slice=0; obj_slice<obj_data->slices; obj_slice++)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+(obj_slice+1)*obj_data->rows-1;
					this_ptr = next_ptr_ptr[1]-2;
					for (obj_row=obj_data->rows-1; obj_row>=0; obj_row--)
					{	for (next_ptr=*next_ptr_ptr;
								this_ptr>=next_ptr; this_ptr-=2)
						Intersect_voxel
						next_ptr_ptr--;
					}
				}
				break;
			case CO_FW+RO_BW+SL_FW:
				for (obj_slice=0; obj_slice<obj_data->slices; obj_slice++)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+(obj_slice+1)*obj_data->rows;
					for (obj_row=obj_data->rows-1; obj_row>=0; obj_row--)
					{	this_ptr = next_ptr_ptr[-1];
						for (next_ptr= *next_ptr_ptr;
								this_ptr<next_ptr; this_ptr+=2)
						Intersect_voxel
						next_ptr_ptr--;
					}
				}
				break;
			case CO_BW+RO_FW+SL_FW:
				for (obj_slice=0; obj_slice<obj_data->slices; obj_slice++)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+obj_slice*obj_data->rows+1;
					for (obj_row=0; obj_row<obj_data->rows; obj_row++)
					{	this_ptr = *next_ptr_ptr-2;
						for (next_ptr=next_ptr_ptr[-1];
								this_ptr>=next_ptr; this_ptr-=2)
						Intersect_voxel
						next_ptr_ptr++;
					}
				}
				break;
			case CO_FW+RO_FW+SL_FW:
				for (obj_slice=0; obj_slice<obj_data->slices; obj_slice++)
				{	if (event_priority(this) == FIRST)
					{	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
						free(intersection_buffer);
						free(intersection_table);
						return (INTERRUPT3);
					}
					next_ptr_ptr =
						obj_data->ptr_table+obj_slice*obj_data->rows+1;
					this_ptr = next_ptr_ptr[-1];
					for (obj_row=0; obj_row<obj_data->rows; obj_row++)
					{	for (next_ptr= *next_ptr_ptr;
								this_ptr<next_ptr; this_ptr+=2)
						Intersect_voxel
						next_ptr_ptr++;
					}
				}
				break;
		}
	}
	*total_density = 0;
	*min = scene_max;
	*max = scene_min;
	sum_squares = N = 0;
	switch (bits)
	{	int scn_row, scn_slice, scn_column, in, error_code, items_read;
		unsigned char *slice_buf8;
		unsigned short *slice_buf16;
		unsigned next, val;
		double startx, endx;
		double slice_density, slice_squares;

/*@ later use scene data already loaded */

		case 8:
			slice_buf8 = (unsigned char *)malloc(width*height);
			if (slice_buf8 == NULL)
			{	report_malloc_error();
				free(intersection_buffer);
				free(intersection_table);
				destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
				return (EROR);
			}
			for (scn_slice=0; scn_slice<slices; scn_slice++)
			{	if (event_priority(this) == FIRST)
				{	free(intersection_buffer);
					free(intersection_table);
					destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
					return (INTERRUPT3);
				}
				in = FALSE;
				for (scn_row=0; scn_row<height; scn_row++)
					if (intersection_table[scn_slice*height+scn_row] !=
							0x7fffffff)
					{	in = TRUE;
						break;
					}
				if (!in)
					continue;
				error_code =
					fseek(fp, hdrlen+width*height*scn_slice, 0)
					?	5
					:	VReadData((char *)slice_buf8, 1, width*height, fp,
							&items_read);
				if (error_code==0 && items_read!=width*height)
					error_code = 2;
				if (error_code)
				{	display_error(error_code);
					free(intersection_buffer);
					free(intersection_table);
					destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
					free(slice_buf8);
					return (EROR);
				}
				slice_density = slice_squares = 0;
				for (scn_row=0; scn_row<height; scn_row++)
				{	startx = width;
					endx = 0;
					for (next=intersection_table[scn_slice*height+scn_row];
							next!=0x7fffffff;
							next=intersection_buffer[next].next)
						if (intersection_buffer[next].is_start)
						{	startx = intersection_buffer[next].x;
							if (startx < 0)
								startx = 0;
							assert(startx >= endx);
						}
						else
						{	startx = ceil(startx);
							endx = floor(intersection_buffer[next].x);
							if (endx > width-1)
								endx = width-1;
							for (scn_column=(int)startx; scn_column<=endx;
									scn_column++)
							{	val = slice_buf8[scn_row*width+scn_column];
								slice_density += val;
								if (val < *min)
									*min = val;
								if (val > *max)
									*max = val;
								slice_squares += val*val;
								N++;
							}
						}
				}
				*total_density += slice_density;
				sum_squares += slice_squares;
			}
			free(slice_buf8);
			break;
		case 16:
			slice_buf16 = (unsigned short *)malloc(width*height*2);
			if (slice_buf16 == NULL)
			{	report_malloc_error();
				free(intersection_buffer);
				free(intersection_table);
				destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
				return (EROR);
			}
			for (scn_slice=0; scn_slice<slices; scn_slice++)
			{	if (event_priority(this) == FIRST)
				{	free(intersection_buffer);
					free(intersection_table);
					destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
					return (INTERRUPT3);
				}
				in = FALSE;
				for (scn_row=0; scn_row<height; scn_row++)
					if (intersection_table[scn_slice*height+scn_row] !=
							0x7fffffff)
					{	in = TRUE;
						break;
					}
				if (!in)
					continue;
				error_code =
					fseek(fp, hdrlen+width*height*scn_slice*2, 0)
					?	5
					:	VReadData((char *)slice_buf16, 2, width*height, fp,
							&items_read);
				if (error_code==0 && items_read!=width*height)
					error_code = 2;
				if (error_code)
				{	display_error(error_code);
					free(intersection_buffer);
					free(intersection_table);
					destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
					free(slice_buf16);
					return (EROR);
				}
				slice_density = slice_squares = 0;
				for (scn_row=0; scn_row<height; scn_row++)
				{	startx = width;
					endx = 0;
					for (next=intersection_table[scn_slice*height+scn_row];
							next!=0x7fffffff;
							next=intersection_buffer[next].next)
						if (intersection_buffer[next].is_start)
						{	startx = intersection_buffer[next].x;
							if (startx < 0)
								startx = 0;
							assert(startx >= endx);
						}
						else
						{	startx = ceil(startx);
							endx = floor(intersection_buffer[next].x);
							if (endx > width-1)
								endx = width-1;
							for (scn_column=(int)startx; scn_column<=endx;
									scn_column++)
							{	val = slice_buf16[scn_row*width+scn_column];
								slice_density += val;
								if (val < *min)
									*min = val;
								if (val > *max)
									*max = val;
								slice_squares += val*val;
								N++;
							}
						}
				}
				*total_density += slice_density;
				sum_squares += slice_squares;
			}
			free(slice_buf16);
			break;
		default:
			assert(FALSE);
	}
	destroy_scene(&vh, &fp, &byte_data, &dbyte_data);
	free(intersection_buffer);
	free(intersection_table);
	if (N == 0)
		return (DONE);
	*mean_density = *total_density/N;
	*standard_deviation = sqrt((sum_squares-*total_density**mean_density)/N);
	return (DONE);
}

/*****************************************************************************
 * FUNCTION: do_plane_cut
 * DESCRIPTION: Produces a new object by cutting the current object with
 *    a plane.
 * PARAMETERS: None
 * SIDE EFFECTS: The old selected_object will be turned off.  The new
 *    object will become the selected_object.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variables scale, icon_scale,
 *    depth_scale, main_image, icon_image, selected_object, display_area_x,
 *    display_area_y, display_area_width, display_area_height, image_x,
 *    image_y, icons_exist must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs or the user aborts the operation, the
 *    object list should be restored to what it was before the call.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 11/20/92 to have new object at head instead of tail.
 *    Modified: 11/24/92 to use selected_object label.
 *    Modified: 1/5/93 to make old object immobile by Dewey Odhner.
 *    Modified: 7/8/93 not to make old object immobile by Dewey Odhner.
 *    Modified: 4/19/94 not to destroy object until after use by Dewey Odhner.
 *    Modified: 5/19/94 to issue error message if not in memory by Dewey Odhner
 *    Modified: 2/14/01 icons_exist checked before cutting icon by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::do_plane_cut()
{
	Shell *new_object, *old_object, **tail;
	int icon_flag;

	old_object = actual_object(object_from_label(selected_object));
	if (!old_object->main_data.in_memory ||
			(icons_exist && !old_object->icon_data.in_memory))
	{
		return;
	}
	new_object = (Shell *)calloc(1, sizeof(Shell));
	if (new_object == NULL)
	{	report_malloc_error();
		return;
	}
	if (do_cut(old_object, icon_flag=FALSE, 0, new_object, NULL, NULL, 0, 0, 0,
			0., None, NULL, NULL) != DONE)
	{	free(new_object);
		return;
	}
	if (icons_exist && do_cut(old_object, icon_flag=TRUE, 0, new_object, NULL,
			NULL, 0, 0, 0, 0., None, NULL, NULL) != DONE)
	{	destroy_object_data(&new_object->main_data);
		free(new_object);
		return;
	}
	new_object->angle[0] = old_object->angle[0];
	new_object->angle[1] = old_object->angle[1];
	new_object->angle[2] = old_object->angle[2];
	new_object->displacement[0] = old_object->displacement[0];
	new_object->displacement[1] = old_object->displacement[1];
	new_object->displacement[2] = old_object->displacement[2];
	new_object->plan_angle[0] = old_object->plan_angle[0];
	new_object->plan_angle[1] = old_object->plan_angle[1];
	new_object->plan_angle[2] = old_object->plan_angle[2];
	new_object->plan_displacement[0] = old_object->plan_displacement[0];
	new_object->plan_displacement[1] = old_object->plan_displacement[1];
	new_object->plan_displacement[2] = old_object->plan_displacement[2];
	new_object->diameter = old_object->diameter;
	new_object->mobile = old_object->mobile;
	new_object->O.on = TRUE;
	new_object->O.opacity = old_object->O.opacity;
	new_object->O.specular_fraction = old_object->O.specular_fraction;
	new_object->O.specular_exponent = old_object->O.specular_exponent;
	new_object->O.diffuse_exponent = old_object->O.diffuse_exponent;
	new_object->O.specular_n = old_object->O.specular_n;
	new_object->O.diffuse_n = old_object->O.diffuse_n;
	new_object->O.rgb = old_object->O.rgb;
	new_object->O.color = old_object->O.color;
	old_object->O.on = FALSE;
	if (original_object == NULL)
	{
		old_object->original = true;
		original_object = object_from_label(selected_object);
		/* add new cut object to tail of object_list */
		for (tail= &object_list; *tail; tail= &(*tail)->next)
			;
		*tail = new_object;
	}
	else
	{	/* replace object at tail of object_list with new cut object */
		for (tail= &object_list; (*tail)->next; tail= &(*tail)->next)
			;
		assert(object_from_label(selected_object) == &(*tail)->O);
		*tail = new_object;
		destroy_object(old_object);
	}
	selected_object = object_label(&new_object->O);
	compactify_object(new_object, icons_exist);
	cut_count++;
}
/*****************************************************************************/
/**
 * \brief this function sets the transform of the aux stereo renderer
 */
void cvRenderer::setStereoTransform ( void ) {
	assert( this->mAux != NULL );

    int  rot_axis = 1;  //x is 0, y is 1, and z is 2
    double  whichAngleValue = Preferences::getStereoAngle() * M_PI / 180.0;  //convert degrees to radians
    //handle change in viewing angle
    double  from_matrix[3][3];
    //::AtoM( from_matrix, mAux->glob_angle[0], mAux->glob_angle[1], mAux->glob_angle[2] );
    ::AtoM( from_matrix, this->glob_angle[0], this->glob_angle[1], this->glob_angle[2] );
    double  rot_matrix[3][3];
    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
    ::matrix_multiply( rot_matrix, rot_matrix, from_matrix );
    ::MtoA( mAux->glob_angle, mAux->glob_angle+1, mAux->glob_angle+2, rot_matrix );
}
/*****************************************************************************/
