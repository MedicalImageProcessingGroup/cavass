/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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


 
 
 
#ifdef INside

/* This part of the file is included from the other part; INside is defined
   in the other part, marked "Main Part of File". */

#if INside
#define WHICHside_insert_adjacent_vois inside_insert_adjacent_vois
#define WHICHside_x inside_x
#define WHICHside_is_start inside_is_start
#else
#define WHICHside_insert_adjacent_vois outside_insert_adjacent_vois
#define WHICHside_x outside_x
#define WHICHside_is_start outside_is_start
#endif

/*****************************************************************************
 * FUNCTION: inside_insert_adjacent_vois and outside_insert_adjacent_vois
 * DESCRIPTION: Insert voxels-of-intersection in adjacent rows
 *    into the voxel-of-intersection list.
 * PARAMETERS:
 *    offset: The offset from S_ptr_ptr to get the pointer to the neighboring
 *       row specified by row_code.  Must be 0, 1, -1, rws, or -rws.
 *    row_code: The row to consider neighbors in (it must exist):
 *       PY: the neighboring row in the +y direction
 *       PZ: the neighboring row in the +z direction
 *       NY: the neighboring row in the -y direction
 *       NZ: the neighboring row in the -z direction
 *       VHERE: the same row
 *    S_ptr_ptr: The pointer to the pointer to the current row of voxels-of-
 *       intersection for the secondary object S
 *    adjacent_vois_end: The pointer to the pointer to the end of the list of
 *       voxels-of-intersection, where the output goes; it will be incremented.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The static variables clmns, rws, data_class
 *    must be initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 12/17/92 by Dewey Odhner
 *    Modified: 7/20/93 assertions corrected by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::WHICHside_insert_adjacent_vois(int offset, int row_code,
		unsigned short **S_ptr_ptr, voi_struct **adjacent_vois_end)
{
	unsigned short  *S_inptr, *in_end_ptr;
	voi_struct *voi_ptr, *ptr2;
	int this_column;

	in_end_ptr = S_ptr_ptr[offset+1];
	for (S_inptr=S_ptr_ptr[offset]; S_inptr<in_end_ptr; S_inptr++)
	{	this_column = WHICHside_x(*S_inptr);
		assert(this_column <= clmns);
		for (voi_ptr=adjacent_vois; voi_ptr<*adjacent_vois_end; voi_ptr++)
			if (voi_ptr->x >= this_column)
				break;
		if (voi_ptr==*adjacent_vois_end || voi_ptr->x>this_column)
		{	for (ptr2=(*adjacent_vois_end)++; ptr2>voi_ptr; ptr2--)
			{	ptr2[0] = ptr2[-1];
				assert(ptr2 > adjacent_vois);
				assert(ptr2 < adjacent_vois+clmns+2);
			}
			voi_ptr->x = this_column;
			voi_ptr->start_code = voi_ptr->end_code = 0;
		}
		if (WHICHside_is_start(*S_inptr))
			voi_ptr->start_code |= row_code;
		else
			voi_ptr->end_code |= row_code;
		assert(voi_ptr >= adjacent_vois);
		assert(voi_ptr < adjacent_vois+clmns+2);
	}
}

#undef WHICHside_insert_adjacent_vois
#undef WHICHside_x
#undef WHICHside_is_start

#else

/* "Main Part of File" */
 
#include "cvRender.h"



#define PLANE_CUT_DISCARD 0
#define CUT_DISCARD 1
#define CUT_SAVE 2
#define VOI_LIST_CUT_DISCARD 3
#define OUT_BLOCK_SIZE 0x800 /* in 32-bit words */

#define VHERE PX
#define OFF_ROW_CODES (NZ|PZ|NY|PY)

#define Cat(a, b) a##b

#define inside_is_start(voxel) ((voxel)&PX)
#define outside_is_start(voxel) (((voxel)&PX)==0)
#define inside_x(voxel) ((voxel)&~PX)
#define outside_x(voxel) \
(	outside_is_start(voxel) \
	?	inside_x(voxel)+1 \
	:	inside_x(voxel)-1 \
)


static const int image_x=0, image_y=0, image2_x=0, image2_y=0, pixel_bytes=4;
static const int display_area_x=0, display_area_y=0, display_area2_x=0, display_area2_y=0;
static const Window display_area=1;


#define INside 1
#include "do_cut.cpp"
#undef INside
#define INside 0
#include "do_cut.cpp"
#undef INside


int is_in_polygon(int x, int y, X_Point vertices[], int nvertices);
bool polygon_is_clockwise(X_Point points[], int npoints);


/*****************************************************************************
 * FUNCTION: put_new_data
 * DESCRIPTION: Puts the data describing the newly cut out piece into the
 *    structure pointed to by object2 and allocates new memory
 *    at new_ptr_table.
 * PARAMETERS:
 *    object2: The data describing the newly cut out piece goes here.
 *    new_ptr_table, out_data, voxels_in_out_buffer: define the newly cut out
 *       piece.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: No error
 *    1: Memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/14/93 for new structure system protocol by Dewey Odhner
 *    Modified: 5/3/94 for fuzzy shells by Dewey Odhner
 *    Modified: 1/27/95 int changed to long by Dewey Odhner
 *
 *****************************************************************************/
int put_new_data(Shell_data *object2, size_t *&new_ptr_table,
	unsigned short *out_data, int &voxels_in_out_buffer)
{
	unsigned short **that_ptr_ptr, **end_ptr_ptr, *new_data, tse_size;

	tse_size = st_cl(object2)==BINARY_B||st_cl(object2)==BINARY_A? 2: 3;
	object2->ptr_table = (unsigned short **)new_ptr_table;
	new_data = (unsigned short *)malloc(voxels_in_out_buffer*tse_size*2);
	if (new_data == NULL)
		return (1);
	end_ptr_ptr = object2->ptr_table+object2->rows*object2->slices;
	for (that_ptr_ptr=object2->ptr_table; that_ptr_ptr<=end_ptr_ptr;
				that_ptr_ptr++)
		*that_ptr_ptr = new_data+(size_t)*that_ptr_ptr;
	memcpy(new_data, out_data, voxels_in_out_buffer*2*tse_size);
	new_ptr_table =
		(size_t *)malloc((object2->rows*object2->slices+1)*sizeof(size_t));
	if (new_ptr_table == NULL)
	{	free(new_data);
		new_ptr_table = (size_t *)(object2->ptr_table);
		object2->ptr_table = NULL;
		return (1);
	}
	object2->in_memory = TRUE;
	object2->file->file_header.str.num_of_NTSE[object2->shell_number] =
		1+object2->slices*(1+object2->rows);
	object2->file->file_header.str.num_of_TSE[object2->shell_number] =
		(unsigned)((object2->ptr_table[object2->rows*object2->slices]-
		object2->ptr_table[0])/tse_size);
	object2->file->file_header.str.volume_valid = FALSE;
	object2->file->file_header.str.surface_area_valid = FALSE;
	return (0);
}


/* 1/15/93 new_num must be >= old_num */
#define Copy_header_item(new, old, item, type, new_num, old_num) \
if (old->item) \
{	new->item = (type *) malloc((new_num)*sizeof(type)); \
	if (new->item == NULL) \
		return (1); \
	memcpy(new->item, old->item,(old_num)*sizeof(type));\
	new->Cat(item,_valid) = old->Cat(item,_valid); \
	if (new_num != old_num) \
		new->Cat(item,_valid) = False; \
}

#define Copy_header_string(new, old, item) \
	Copy_header_item(new, old, item, char, strlen(old->item)+1, \
		strlen(old->item)+1)

/*****************************************************************************
 * FUNCTION: copy_shell_header
 * DESCRIPTION: Copies a file header structure of a SHELL0 structure.
 * PARAMETERS:
 *    new: The destination; relevant pointer members will have memory
 *       allocated; the caller can free it after use.
 *    old: The source; pointer members that are not NULL must have appropriate
 *       memory allocated.
 *    new_num_of_structures: The value for new->str.num_of_structures.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: Parameters must be valid.
 * RETURN VALUE:
 *    0: No error
 *    1: Memory allocation failure
 * EXIT CONDITIONS: If an error occurs, relevant pointer members will have
 *    memory allocated or be NULL;
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/15/93 for new structure system protocol by Dewey Odhner
 *    Modified: 4/15/93 for new structure system protocol by Dewey Odhner
 *    Modified: 4/26/93 to copy domain for more structures by Dewey Odhner
 *    Modified: 5/15/93 to copy scene file to new structures by Dewey Odhner
 *    Modified: 9/3/03 to copy stuff of more components of TSE by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::copy_shell_header(ViewnixHeader *newp, ViewnixHeader *oldp,
	int new_num_of_structures)
{
	int old_num_of_structures, j, c, k;

	old_num_of_structures =
		oldp->str.num_of_structures > new_num_of_structures
		?	new_num_of_structures
		:	oldp->str.num_of_structures;
	c = oldp->str.num_of_components_in_TSE;
	*newp = *oldp;
	newp->str.num_of_structures = new_num_of_structures;
	newp->gen.filename_valid = FALSE;
	newp->gen.description = NULL;
	newp->gen.comment = NULL;
	newp->gen.imaged_nucleus = NULL;
	newp->gen.gray_lookup_data = NULL;
	newp->gen.red_lookup_data = NULL;
	newp->gen.green_lookup_data = NULL;
	newp->gen.blue_lookup_data = NULL;
	newp->str.domain = NULL;
	newp->str.axis_label = NULL;
	newp->str.measurement_unit = NULL;
	newp->str.num_of_TSE = NULL;
	newp->str.num_of_NTSE = NULL;
	newp->str.NTSE_measurement_unit = NULL;
	newp->str.TSE_measurement_unit = NULL;
	newp->str.smallest_value = NULL;
	newp->str.largest_value = NULL;
	newp->str.signed_bits_in_TSE = NULL;
	newp->str.bit_fields_in_TSE = NULL;
	newp->str.signed_bits_in_NTSE = NULL;
	newp->str.bit_fields_in_NTSE = NULL;
	newp->str.num_of_samples = NULL;
	newp->str.loc_of_samples = NULL;
	newp->str.description_of_element = NULL;
	newp->str.parameter_vectors = NULL;
	newp->str.min_max_coordinates = NULL;
	newp->str.volume = NULL;
	newp->str.surface_area = NULL;
	newp->str.rate_of_change_volume = NULL;
	newp->str.description = NULL;
	newp->str.scene_file_valid = FALSE;
	Copy_header_string(newp, oldp, gen.description)
	Copy_header_string(newp, oldp, gen.comment)
	Copy_header_string(newp, oldp, gen.imaged_nucleus)
	Copy_header_item(newp, oldp, str.domain, float,
		12*new_num_of_structures, 12*old_num_of_structures)
	if (newp->str.domain)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
			memcpy(newp->str.domain+12*j, oldp->str.domain, 12*sizeof(float));
	Copy_header_item(newp, oldp, str.axis_label, Char30, 3, 3)
	Copy_header_item(newp, oldp, str.measurement_unit, short, 3, 3)
	Copy_header_item(newp, oldp, str.num_of_TSE, unsigned int,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(newp, oldp, str.num_of_NTSE, unsigned int,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(newp, oldp, str.NTSE_measurement_unit, short, 1, 1)
	Copy_header_item(newp, oldp, str.TSE_measurement_unit, short, c, c)
	Copy_header_item(newp, oldp, str.smallest_value, float,
		c*new_num_of_structures, c*old_num_of_structures)
	Copy_header_item(newp, oldp, str.largest_value, float,
		c*new_num_of_structures, c*old_num_of_structures)
	if (newp->str.smallest_value)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
		  for (k=0; k<c; k++)
			if (k != 1)
			{
				newp->str.smallest_value[c*j+k] = newp->str.smallest_value[k];
				newp->str.largest_value[c*j+k] = newp->str.largest_value[k];
			}
	Copy_header_item(newp, oldp, str.signed_bits_in_TSE, short, c, c)
	Copy_header_item(newp, oldp, str.bit_fields_in_TSE, short, 2*c, 2*c)
	Copy_header_item(newp, oldp, str.signed_bits_in_NTSE, short, 1, 1)
	Copy_header_item(newp, oldp, str.bit_fields_in_NTSE, short, 2, 2)
	Copy_header_item(newp, oldp, str.num_of_samples, short, 1, 1)
	Copy_header_item(newp, oldp, str.loc_of_samples, float,
		oldp->str.num_of_samples[0], oldp->str.num_of_samples[0])
	Copy_header_item(newp, oldp, str.description_of_element, short,
		oldp->str.num_of_elements, oldp->str.num_of_elements)
	Copy_header_item(newp, oldp, str.parameter_vectors, float,
		oldp->str.num_of_elements*new_num_of_structures,
		oldp->str.num_of_elements*old_num_of_structures)
	Copy_header_item(newp, oldp, str.min_max_coordinates, float,
		6*new_num_of_structures, 6*old_num_of_structures)
	Copy_header_item(newp, oldp, str.volume, float, new_num_of_structures,
		old_num_of_structures)
	Copy_header_item(newp, oldp, str.surface_area, float,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(newp, oldp, str.rate_of_change_volume, float,
		new_num_of_structures, old_num_of_structures)
	Copy_header_string(newp, oldp, str.description)
	Copy_header_item(newp, oldp, str.scene_file, Char30,
		new_num_of_structures, old_num_of_structures);
	if (newp->str.scene_file)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
			strcpy(newp->str.scene_file[j], oldp->str.scene_file[0]);
	return (0);
}

/*****************************************************************************
 * FUNCTION: do_cut
 * DESCRIPTION: Cuts the primary object by taking the intersection
 *	 with the secondary object represented by S.
 * PARAMETERS:
 *    primary_object: The object to be cut, must point to a valid
 *       Shell. (A valid Shell must have its 'main_data.file' and
 *       'icon_data.file' point to valid Shell_file structures.)
 *    icon_flag: If zero, main_data will be cut; otherwise icon_data.
 *    op_type: The type of operation to be done:
 *       0 (PLANE_CUT_DISCARD): The secondary object S is the half-space
 *          defined by the plane; only the intersection of S and the
 *          primary object is kept.
 *       1 (CUT_DISCARD): The secondary object S is the solid polygonal prism
 *          defined by points, npoints, z, depth, draw_window, or its
 *          complement; only the intersection of S and the
 *          primary object is kept.
 *       2 (CUT_SAVE): The secondary object S is the solid polygonal prism
 *          defined by points, npoints, z, depth, draw_window, or its
 *          complement; the intersection of S and the primary object is kept
 *          as well as the complement of S in the primary object.
 *       3 (VOI_LIST_CUT_DISCARD): The secondary object S is defined by a
 *          voxel-of-intersection list at s_ptr_table; only the intersection
 *          of S and the primary object is kept.
 *    out_object1: The intersection of S and the primary object will go in
 *       out_object1->main_data or ->icon_data depending on icon_flag.
 *    out_object2: If op_type is CUT_SAVE, the part of primary_object not
 *       in S will be put in out_object2->main_data or ->icon_data depending
 *       on icon_flag.
 *    points: List of vertices of polygon, the base of the prism S, in
 *       draw_window coordinates; not used if op_type is PLANE_CUT_DISCARD or
 *       VOI_LIST_CUT_DISCARD.
 *    npoints: Number of points in the list points.
 *    z: The z coordinate of the base of the prism defining S in the
 *       (left-handed) display coordinate system.
 *    inside_curve: Non-zero if S is the solid prism, zero if its complement.
 *    depth: The altitude of the prism in mm.
 *    draw_window: The window in which the curve was drawn, either
 *       display_area or display_area2.
 *    s_ptr_table: The voxel-of-intersection (voi) list defining S; points to
 *       an array of rws*slcs+1 pointers where
 *          clmns == Largest_y1(primary_object_data)+1;
 *          rws == primary_object_data->rows;
 *          slcs == primary_object_data->slices;
 *          primary_object_data == (icon_flag? &primary_object->icon_data:
 *             &primary_object->main_data);
 *       these pointers mark the beginning and end of the voi list for each
 *       row.  Each voi has the Y1 coordinate (0 to 1023) ORed with a flag
 *       PX for a starting voxel or 0 for an ending voxel.
 *       Memory for the voi list must be dynamically allocated at
 *       s_ptr_table[0] and memory for the pointer table must be dynamically
 *       allocated at s_ptr_table; this memory will be freed by this function.
 *    prismp: An alternate specification for the prism S, used only if
 *       npoints == 0.  The coordinates of the prism are in the strucure
 *       coordinates of the object with units of voxels.
 * SIDE EFFECTS: If an error occurs, a message is displayed.  Any events will
 *    be removed from the event queue up to the first right button press.
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables argv0, windows_open, dialog_window, glob_angle,
 *    scale, depth_scale, display_area, main_image, image_x, image_y,
 *    image2, image2_x, image2_y must be appropriately initialized.
 *    ButtonPress events should be selected to allow the user to abort.
 * RETURN VALUE: DONE, EROR, or INTERRUPT3 (right button press)
 * EXIT CONDITIONS: If return value is DONE, memory is allocated for the data
 *    attached to out_object1 (and out_object2 if op_type is CUT_SAVE) which
 *    can be freed by the caller after use; otherwise any memory allocated by
 *    this function is freed.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/14/93 for new structure system protocol by Dewey Odhner
 *    Modified: 1/28/93 to copy parameter vector to new structures
 *    Modified: 5/15/93 to validify domain and scene file in new structures
 *    Modified: 4/21/94 memory bug corrected by Dewey Odhner
 *    Modified: 5/9/94 for fuzzy shells by Dewey Odhner
 *    Modified: 1/27/95 int changed to long by Dewey Odhner
 *    Modified: 5/25/95 VOI_LIST_CUT_DISCARD mode added by Dewey Odhner
 *    Modified: 7/14/99 parameter prismp added by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::do_cut(Shell *primary_object, int icon_flag,
	int op_type, Shell *out_object1, Shell *out_object2, X_Point points[],
	int npoints, int z, int inside_curve, double depth, Window draw_window,
	unsigned short **s_ptr_table, struct Prism *prismp)
{
	Shell_data *primary_object_data, *out_object1_data, *out_object2_data=NULL;
	unsigned short **that_ptr_ptr, **end_ptr_ptr;
	Function_status status;
	int error_code;

	if (icon_flag)
	{	primary_object_data = &primary_object->icon_data;
		out_object1_data = &out_object1->icon_data;
		out_object1->O.icon.projected = BAD;
	}
	else
	{	primary_object_data = &primary_object->main_data;
		out_object1_data = &out_object1->main_data;
		out_object1->O.main_image.projected = BAD;
	}
	out_object1_data->in_memory = FALSE;
	out_object1_data->ptr_table = NULL;
	out_object1_data->rows = primary_object_data->rows;
	out_object1_data->slices = primary_object_data->slices;
	out_object1_data->shell_number = 0;
	out_object1_data->file = (Shell_file *)malloc(sizeof(Shell_file));
	if (out_object1_data->file == NULL)
	{	report_malloc_error();
		return (EROR);
	}
	out_object1_data->file->reference =
		(Shell_data **)malloc(sizeof(Shell_data *)*2/*in case of 2 objects*/);
	if (out_object1_data->file->reference == NULL)
	{	report_malloc_error();
		free(out_object1_data->file);
		return (EROR);
	}
	out_object1_data->file->reference[0] = out_object1_data;
	out_object1_data->file->references = 1;
	error_code = copy_shell_header(&out_object1_data->file->file_header,
		&primary_object_data->file->file_header, 2);
	if (error_code)
	{	display_error(error_code);
		destroy_object_data(out_object1_data);
		return (EROR);
	}
	out_object1_data->shell_number = 0;
	Set_largest_y1(out_object1_data, Largest_y1(primary_object_data));
	out_object1_data->file->file_header.str.largest_value_valid = 1;
	Set_smallest_y1(out_object1_data, Smallest_y1(primary_object_data));
	out_object1_data->file->file_header.str.smallest_value_valid = 1;
	Min_str_coordinate(out_object1_data, 0) =
		Min_str_coordinate(primary_object_data, 0);
	Max_str_coordinate(out_object1_data, 0) =
		Max_str_coordinate(primary_object_data, 0);
	Min_str_coordinate(out_object1_data, 1) =
		Min_str_coordinate(primary_object_data, 1);
	Max_str_coordinate(out_object1_data, 1) =
		Max_str_coordinate(primary_object_data, 1);
	Min_str_coordinate(out_object1_data, 2) =
		Min_str_coordinate(primary_object_data, 2);
	Max_str_coordinate(out_object1_data, 2) =
		Max_str_coordinate(primary_object_data, 2);
	out_object1_data->file->file_header.str.min_max_coordinates_valid = 1;
	out_object1_data->file->file_header.str.num_of_structures = 1;
	if (primary_object_data->file->file_header.str.parameter_vectors_valid)
	{	memcpy(out_object1_data->file->file_header.str.parameter_vectors,
			primary_object_data->file->file_header.str.parameter_vectors+
			primary_object_data->file->file_header.str.num_of_elements*
			primary_object_data->shell_number,
			primary_object_data->file->file_header.str.num_of_elements*
			sizeof(float));
		out_object1_data->file->file_header.str.parameter_vectors_valid = 1;
	}
	if (primary_object_data->file->file_header.str.domain_valid &&
			out_object1_data->file->file_header.str.domain)
		out_object1_data->file->file_header.str.domain_valid = 1;
	if (primary_object_data->file->file_header.str.scene_file_valid &&
			out_object1_data->file->file_header.str.scene_file)
		out_object1_data->file->file_header.str.scene_file_valid = 1;
	out_object1_data->file->file_header.str.volume_valid = 0;
	out_object1_data->file->file_header.str.rate_of_change_volume_valid = 0;
	slcs = primary_object_data->slices;
	rws = primary_object_data->rows;
	clmns = (int)Largest_y1(primary_object_data)+1;
	switch (op_type)
	{	case CUT_SAVE:
			out_object2_data =
				icon_flag
				?	&out_object2->icon_data
				:	&out_object2->main_data;
		case CUT_DISCARD:
			inside = inside_curve;
			if (!icon_flag && npoints)
			{	error_code = cut_marks(primary_object, out_object1, inside,
					points, npoints, z, depth, draw_window);
				if (error_code)
				{	display_error(error_code);
					destroy_object_data(out_object1_data);
					return (EROR);
				}
			}
			break;
		case PLANE_CUT_DISCARD:
			if (!icon_flag)
			{	error_code = plane_cut_marks(primary_object, out_object1);
				if (error_code)
				{	display_error(error_code);
					destroy_object_data(out_object1_data);
					return (EROR);
				}
			}
			plane_setup(primary_object, primary_object_data);
			break;
		case VOI_LIST_CUT_DISCARD:
			inside = TRUE;
			break;
	}
	if ((out_data=(unsigned short *)malloc(OUT_BLOCK_SIZE*6)) == NULL)
	{	report_malloc_error();
		destroy_object_data(out_object1_data);
		return (EROR);
	}
	out_buffer_size = OUT_BLOCK_SIZE;
	if (op_type == VOI_LIST_CUT_DISCARD)
		S_ptr_table = s_ptr_table;
	else
	{	S_ptr_table= (unsigned short **)malloc((rws*slcs+1)*sizeof(short*));
		if (S_ptr_table == NULL)
		{	report_malloc_error();
			destroy_object_data(out_object1_data);
			free(out_data);
			return (EROR);
		}
	}
	adjacent_vois = (voi_struct *)malloc((clmns+2)*sizeof(voi_struct));
	if (adjacent_vois == NULL)
	{	report_malloc_error();
		destroy_object_data(out_object1_data);
		free(out_data);
		free(S_ptr_table);
		return (EROR);
	}
	S_row = (S_voxel_struct *)malloc((clmns+2)*sizeof(S_voxel_struct));
	if (S_row == NULL)
	{	report_malloc_error();
		destroy_object_data(out_object1_data);
		free(out_data);
		free(S_ptr_table);
		free(adjacent_vois);
		return (EROR);
	}
	if ((new_ptr_table=(size_t *)malloc((rws*slcs+1)*sizeof(size_t))) == NULL)
	{	report_malloc_error();
		destroy_object_data(out_object1_data);
		free(out_data);
		free(S_ptr_table);
		free(adjacent_vois);
		free(S_row);
		return (EROR);
	}
	switch (op_type)
	{	case PLANE_CUT_DISCARD:
			*S_ptr_table = (unsigned short *)
				malloc(rws*slcs*2*sizeof(short));
			if (*S_ptr_table == NULL)
			{	report_malloc_error();
				destroy_object_data(out_object1_data);
				free(out_data);
				free(S_ptr_table);
				free(adjacent_vois);
				free(S_row);
				free(new_ptr_table);
				return (EROR);
			}
			get_plane_voi_list();
			break;
		case CUT_SAVE:
			out_object2_data->in_memory = FALSE;
			out_object2_data->ptr_table = NULL;
			out_object2_data->rows = primary_object_data->rows;
			out_object2_data->slices = primary_object_data->slices;
			out_object2_data->shell_number = 1;
			out_object2_data->file = out_object1_data->file;
			out_object2_data->file->reference[1] = out_object2_data;
			out_object2_data->file->references = 2;
			out_object2_data->file->file_header.str.num_of_structures = 2;
			Set_largest_y1(out_object2_data, Largest_y1(primary_object_data));
			Set_smallest_y1(out_object2_data,Smallest_y1(primary_object_data));
			Min_str_coordinate(out_object2_data, 0) =
				Min_str_coordinate(primary_object_data, 0);
			Max_str_coordinate(out_object2_data, 0) =
				Max_str_coordinate(primary_object_data, 0);
			Min_str_coordinate(out_object2_data, 1) =
				Min_str_coordinate(primary_object_data, 1);
			Max_str_coordinate(out_object2_data, 1) =
				Max_str_coordinate(primary_object_data, 1);
			Min_str_coordinate(out_object2_data, 2) =
				Min_str_coordinate(primary_object_data, 2);
			Max_str_coordinate(out_object2_data, 2) =
				Max_str_coordinate(primary_object_data, 2);
			if (primary_object_data->file->file_header.str.
					parameter_vectors_valid)
				memcpy(
					out_object2_data->file->file_header.str.parameter_vectors+
					out_object1_data->file->file_header.str.num_of_elements,
					out_object1_data->file->file_header.str.parameter_vectors,
					primary_object_data->file->file_header.str.num_of_elements*
					sizeof(float));
			if (!icon_flag && npoints)
			{	error_code = cut_marks(primary_object, out_object2, !inside,
					points, npoints, z, depth, draw_window);
				if (error_code)
				{	display_error(error_code);
					destroy_object_data(out_object1_data);
					free(out_data);
					free(S_ptr_table);
					free(adjacent_vois);
					free(S_row);
					free(new_ptr_table);
					return (EROR);
				}
			}
		case CUT_DISCARD:
			if (get_prism_voi_list(S_ptr_table, npoints?
				get_prism(points, npoints, depth, z, primary_object, icon_flag?
				&primary_object->icon_data: &primary_object->main_data,
				draw_window): prismp, icon_flag, draw_window, primary_object))
			{	destroy_object_data(out_object1_data);
				free(out_data);
				free(S_ptr_table);
				free(adjacent_vois);
				free(S_row);
				free(new_ptr_table);
				if (op_type == CUT_SAVE)
					destroy_object_data(out_object2_data);
				return (EROR);
			}
			break;
	}
	switch (op_type)
	{	case CUT_SAVE:
			inside=!inside;
			status = cut_rows(primary_object_data, FALSE,
				primary_object_data->file->file_header.str.bit_fields_in_TSE[6]
				==23);
			if (status != DONE)
			{	destroy_object_data(out_object1_data);
				free(out_data);
				free(S_ptr_table);
				free(adjacent_vois);
				free(S_row);
				free(new_ptr_table);
				return (status);
			}
			error_code = put_new_data(out_object2_data, new_ptr_table,
				out_data, voxels_in_out_buffer);
			if (error_code)
			{	display_error(error_code);
				destroy_object_data(out_object1_data);
				free(out_data);
				free(S_ptr_table);
				free(adjacent_vois);
				free(S_row);
				free(new_ptr_table);
				return (EROR);
			}
			inside=!inside;
	}
	status = cut_rows(primary_object_data, op_type==PLANE_CUT_DISCARD,
		primary_object_data->file->file_header.str.bit_fields_in_TSE[6]==23);
	free(*S_ptr_table);
	free(S_ptr_table);
	free(adjacent_vois);
	free(S_row);
	if (status != DONE)
	{	destroy_object_data(out_object1_data);
		free(out_data);
		free(new_ptr_table);
		return (status);
	}
	out_object1_data->ptr_table = (unsigned short **)new_ptr_table;
	end_ptr_ptr =
		out_object1_data->ptr_table+primary_object_data->rows*slcs;
	for (that_ptr_ptr=out_object1_data->ptr_table;
			that_ptr_ptr<=end_ptr_ptr; that_ptr_ptr++)
		*that_ptr_ptr = out_data+(size_t)*that_ptr_ptr;
	out_object1_data->in_memory = TRUE;
	out_object1_data->file->file_header.str.num_of_TSE[
		out_object1_data->shell_number] =
		(unsigned)((out_object1_data->ptr_table[out_object1_data->rows*
		out_object1_data->slices]-out_object1_data->ptr_table[0])/
		(st_cl(primary_object_data)==BINARY_B||
		st_cl(primary_object_data)==BINARY_A? 2:3));
	out_object1_data->file->file_header.str.volume_valid = FALSE;
	out_object1_data->file->file_header.str.surface_area_valid = FALSE;
	return DONE;
}

/*****************************************************************************
 * FUNCTION: compactify_object
 * DESCRIPTION: Removes slices and rows that are outside the bounding box of
 *    the object's main shell and its icon.
 * PARAMETERS:
 *    obj: The object to be compactified
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: obj must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 1/20/93 by Dewey Odhner
 *    Modified: 1/22/93 to shift plan_displacement by Dewey Odhner
 *    Modified: 1/25/93 to transform marks to new coordinates by Dewey Odhner
 *    Modified: 10/20/93 to handle units by Dewey Odhner
 *    Modified: 2/14/01 icons_exist checked before doing icon by Dewey Odhner
 *
 *****************************************************************************/
void compactify_object(Shell *obj, int icons_exist)
{
	double shift[3], center[3], rotation[3][3], unit;
	int j;

	unit = unit_mm(&obj->main_data);
	for (j=0; j<3; j++)
		center[j] = -.5*unit*(Min_coordinate(&obj->main_data, j)+
			Max_coordinate(&obj->main_data, j));
	compactify_object_data(&obj->main_data);
	if (icons_exist)
		compactify_object_data(&obj->icon_data);
	for (j=0; j<3; j++)
		center[j] += .5*unit*(Min_coordinate(&obj->main_data, j)+
			Max_coordinate(&obj->main_data, j));
	for (j=0; j<obj->marks; j++)
		vector_subtract(obj->mark[j], obj->mark[j], center);
	AtoM(rotation, obj->angle[0], obj->angle[1], obj->angle[2]);
	matrix_vector_multiply(shift, rotation, center);
	vector_add(obj->displacement, obj->displacement, shift);
	AtoM(rotation, obj->plan_angle[0], obj->plan_angle[1], obj->plan_angle[2]);
	matrix_vector_multiply(shift, rotation, center);
	vector_add(obj->plan_displacement, obj->plan_displacement, shift);
	compute_diameter(obj);
}

/*****************************************************************************
 * FUNCTION: compactify_object_data
 * DESCRIPTION: Removes slices and rows that are outside the bounding box of
 *    the shell.
 * PARAMETERS:
 *    obj_data: The object to be compactified, must be a binary SHELL0.
 * SIDE EFFECTS: An error message may be displayed if obj_data is corrupt.
 * ENTRY CONDITIONS: obj_data must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 1/20/93 by Dewey Odhner
 *    Modified: 5/3/94 for fuzzy shells by Dewey Odhner
 *    Modified: 6/1/94 bounds error corrected by Dewey Odhner
 *    Modified: 5/18/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
void compactify_object_data(Shell_data *obj_data)
{
	int this_slice, this_row, min_slice, max_slice, min_row, max_row,
		min_column, max_column, old_first_slice, tse_size, this_column;
	unsigned short *this_ptr;
	const Classification_type data_class = st_cl(obj_data);

	tse_size = data_class==BINARY_B||data_class==BINARY_A? 2: 3;
	/* find correct values */
	min_slice = obj_data->slices-1;
	max_slice = 0;
	min_row = obj_data->rows-1;
	max_row = 0;
	min_column = (int)Largest_y1(obj_data)+1;
	max_column = 0;
	for (this_slice=0; this_slice<obj_data->slices; this_slice++)
		for (this_row=0; this_row<obj_data->rows; this_row++)
			for (this_ptr=
					obj_data->ptr_table[this_slice*obj_data->rows+this_row];
					this_ptr<
					obj_data->ptr_table[this_slice*obj_data->rows+this_row+1];
					this_ptr+=tse_size)
			{	if (this_slice < min_slice)
					min_slice = this_slice;
				if (this_slice > max_slice)
					max_slice = this_slice;
				if (this_row < min_row)
					min_row = this_row;
				if (this_row > max_row)
					max_row = this_row;
				this_column = data_class==BINARY_B? ((*this_ptr&XMASK)<<1|
					((this_ptr[1]&0x8000)!=0)): *this_ptr&XMASK;
				if (this_column < min_column)
					min_column = this_column;
				if (this_column > max_column)
					max_column = this_column;
			}
	if (min_column == Largest_y1(obj_data)+1)
		min_slice = 0;
	for (old_first_slice=0;
			old_first_slice<obj_data->file->file_header.str.num_of_samples[0];
			old_first_slice++)
		if (obj_data->file->file_header.str.loc_of_samples[old_first_slice] ==
				Min_coordinate(obj_data, 2))
			break;
	if (old_first_slice == obj_data->file->file_header.str.num_of_samples[0])
	{	display_error(104);
		return;
	}

	/* compactify pointer array */
	for (this_slice=min_slice; this_slice<=max_slice; this_slice++)
		for (this_row=min_row; this_row<=max_row; this_row++)
			obj_data->ptr_table[(this_slice-min_slice)*(max_row-min_row+1)+
				this_row-min_row] =
				obj_data->ptr_table[this_slice*obj_data->rows+this_row];
	obj_data->ptr_table[(max_slice-min_slice+1)*(max_row-min_row+1)] =
		obj_data->ptr_table[obj_data->slices*obj_data->rows];

	/* set new values */
	Set_smallest_y1(obj_data, min_column);
	Set_largest_y1(obj_data, max_column);
	Min_str_coordinate(obj_data, 0) =
		min_column*obj_data->file->file_header.str.xysize[0];
	Max_str_coordinate(obj_data, 0) =
		max_column*obj_data->file->file_header.str.xysize[0];
	Min_str_coordinate(obj_data, 1) +=
		min_row*obj_data->file->file_header.str.xysize[1];
	Max_str_coordinate(obj_data, 1) -=
		(obj_data->rows-1-max_row)*obj_data->file->file_header.str.xysize[1];
	Min_str_coordinate(obj_data, 2) = obj_data->file->file_header.str.
		loc_of_samples[old_first_slice+min_slice];
	Max_str_coordinate(obj_data, 2) = obj_data->file->file_header.str.
		loc_of_samples[old_first_slice+max_slice];
	obj_data->slices = max_slice-min_slice+1;
	obj_data->rows = max_row-min_row+1;
	if (obj_data->rows < 0)
		obj_data->rows = 0;
	obj_data->file->file_header.str.num_of_NTSE[obj_data->shell_number] =
		1+obj_data->slices*(1+obj_data->rows);
}

/*****************************************************************************
 * FUNCTION: cut_marks
 * DESCRIPTION: Copies the marks on one object that fall within S, a polygonal
 *    prism or its complement, to a new object.
 * PARAMETERS:
 *    in_object: The object to copy the marks from
 *    out_object: The object to copy the marks to, must have no marks already.
 *       Memory allocated at out_object->mark can be freed by the caller
 *       after use.
 *    inside_curve: Non-zero if S is the solid prism, zero if its complement.
 *    points: List of vertices of polygon, the base of the prism S, in
 *       draw_window coordinates.
 *    npoints: Number of points in the list points.
 *    z: The z coordinate of the base of the prism defining S in the
 *       (left-handed) display coordinate system.
 *    depth: The altitude of the prism in mm.
 *    draw_window: The window in which the curve was drawn, either
 *       display_area or display_area2.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables argv0, windows_open, dialog_window, glob_angle,
 *    scale, depth_scale, display_area, main_image, image_x, image_y,
 *    image2, image2_x, image2_y must be appropriately initialized.
 * RETURN VALUE:
 *    0: no error
 *    1: memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/25/93 to transform marks to new coordinates by Dewey Odhner
 *    Modified: 9/26/96 out_object->mark_array_size set by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::cut_marks(Shell *in_object, Shell*out_object, int inside_curve,
	X_Point points[], int npoints, int z, double depth, Window draw_window)
{
	int this_mark, nmarks, j;
	double translation[3];

	assert(out_object->marks == 0);
	/* count marks on this side of curve */
	nmarks = 0;
	for (this_mark=0; this_mark<in_object->marks; this_mark++)
		if (mark_is_in_curve(in_object, this_mark, points, npoints, z, depth,
				draw_window) == (inside_curve!=0))
			nmarks++;
	if (nmarks == 0)
		return (0);
	/* copy marks */
	out_object->mark = (triple *)malloc(nmarks*sizeof(triple));
	if (out_object->mark == NULL)
		return (1);
	out_object->mark_array_size = nmarks;
	for (j=0; j<3; j++)
		translation[j] = .5*(Min_coordinate(&in_object->main_data, j)+
			Max_coordinate(&in_object->main_data, j)-
			Min_coordinate(&out_object->main_data, j)-
			Max_coordinate(&out_object->main_data, j));
	for (this_mark=0; this_mark<in_object->marks; this_mark++)
		if (mark_is_in_curve(in_object, this_mark, points, npoints, z, depth,
				draw_window) == (inside_curve!=0))
		{	vector_add(out_object->mark[out_object->marks],
				in_object->mark[this_mark], translation);
			out_object->marks++;
		}
	return (0);
}

/*****************************************************************************
 * FUNCTION: mark_is_in_curve
 * DESCRIPTION: Determines whether a mark falls within a polygonal prism.
 * PARAMETERS:
 *    object: The object that the mark is on
 *    this_mark: The mark in question
 *    points: List of vertices of polygon, the base of the prism in
 *       draw_window coordinates.
 *    npoints: Number of points in the list points.
 *    z: The z coordinate of the base of the prism in the
 *       (left-handed) display coordinate system.
 *    depth: The altitude of the prism in mm.
 *    draw_window: The window in which the curve was drawn, either
 *       display_area or display_area2.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables argv0, windows_open, dialog_window, glob_angle,
 *    scale, depth_scale, display_area, main_image, image_x, image_y,
 *    image2, image2_x, image2_y must be appropriately initialized.
 * RETURN VALUE:
 *    0: The mark falls outside of the prism.
 *    1: The mark falls inside of the prism.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::mark_is_in_curve(Shell *object, int this_mark,
	X_Point points[], int npoints, int z, double depth, Window draw_window)
{
	double mark_coords[3], rotation[3][3], real_z;

	/* transform plane mark to display area coordinates */
	/* Convert from object to plan coordinates */
	AtoM(rotation, object->angle[0], object->angle[1], object->angle[2]);
	matrix_vector_multiply(mark_coords, rotation, object->mark[this_mark]);
	mark_coords[0] += object->displacement[0];
	mark_coords[1] += object->displacement[1];
	mark_coords[2] += object->displacement[2];
	/* Convert from plan to image coordinates from top left */
	AtoM(rotation, glob_angle[0], glob_angle[1], glob_angle[2]);
	matrix_vector_multiply(mark_coords, rotation, mark_coords);

	real_z = (MIDDLE_DEPTH-z)/depth_scale;
	if (mark_coords[2]<real_z || mark_coords[2]>real_z+depth)
		return (FALSE);
	/* convert from physical units */
	mark_coords[0] *= scale;
	mark_coords[1] *= scale;
	/* convert to display area coordinates */
	if (image_mode!=SEPARATE || draw_window==display_area)
	{	mark_coords[0] = rint(mark_coords[0]+main_image->width/2+image_x);
		mark_coords[1] = rint(mark_coords[1]+main_image->height/2+image_y);
	}
	else
	{	mark_coords[0] = rint(mark_coords[0]+image2->width/2+image2_x);
		mark_coords[1] = rint(mark_coords[1]+image2->height/2+image2_y);
	}

	return (is_in_polygon((int)mark_coords[0], (int)mark_coords[1],
		points, npoints));
}

/*****************************************************************************
 * FUNCTION: plane_cut_marks
 * DESCRIPTION: Copies the marks on one object that fall on this side of the
 *    plane to a new object.
 * PARAMETERS:
 *    in_object: The object to copy the marks from
 *    out_object: The object to copy the marks to, must have no marks already.
 *       Memory allocated at out_object->mark can be freed by the caller
 *       after use.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables argv0, windows_open, dialog_window, plane_normal,
 *    plane_displacement must be appropriately initialized.
 * RETURN VALUE:
 *    0: no error
 *    1: memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/26/92 to correct plane displacement by Dewey Odhner
 *    Modified: 9/26/96 out_object->mark_array_size set by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::plane_cut_marks(Shell *in_object, Shell *out_object)
{
	double rotation_matrix[3][3], plane_norml[3], plane_disp, translation[3];
	int this_mark, nmarks, j;

	/* transform plane to object coordinates from center */
	AtoM(rotation_matrix, in_object->angle[0], in_object->angle[1],
		in_object->angle[2]);
	vector_matrix_multiply(plane_norml, plane_normal, rotation_matrix);
	plane_disp = plane_displacement-
		in_object->displacement[0]*plane_normal[0]-
		in_object->displacement[1]*plane_normal[1]-
		in_object->displacement[2]*plane_normal[2];
	/* count marks on this side of plane */
	nmarks = 0;
	for (this_mark=0; this_mark<in_object->marks; this_mark++)
		if (is_this_side(in_object->mark[this_mark], plane_norml, plane_disp))
			nmarks++;
	/* copy marks */
	out_object->mark = (triple *)malloc(nmarks*sizeof(triple));
	if (out_object->mark == NULL)
		return(1);
	out_object->mark_array_size = nmarks;
	assert(out_object->marks == 0);
	for (j=0; j<3; j++)
		translation[j] = .5*(Min_coordinate(&in_object->main_data, j)+
			Max_coordinate(&in_object->main_data, j)-
			Min_coordinate(&out_object->main_data, j)-
			Max_coordinate(&out_object->main_data, j));
	for (this_mark=0; this_mark<in_object->marks; this_mark++)
		if (is_this_side(in_object->mark[this_mark], plane_norml, plane_disp))
		{	vector_add(out_object->mark[out_object->marks],
				in_object->mark[this_mark], translation);
			out_object->marks++;
		}
	return (0);
}

/*****************************************************************************
 * FUNCTION: is_this_side
 * DESCRIPTION: Determines whether a point is on "this" side of a plane
 *    (the normal vector points to the other side).
 * PARAMETERS:
 *    point: The coordinates of the point
 *    plane_norml: The normal vector to the plane
 *    plane_disp: The displacement of the plane from the origin along the
 *       direction of the normal vector
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if point is on this side
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int is_this_side(triple point, triple plane_norml,
	double plane_disp)
{
	return (point[0]*plane_norml[0]+point[1]*plane_norml[1]+
		point[2]*plane_norml[2] < plane_disp);
}

/*****************************************************************************
 * FUNCTION: plane_setup
 * DESCRIPTION: Initializes the static variables inside, column_n_factor,
 *     row_n_factor, slice_n_factor,
 *     inside_plane_G_code, outside_plane_G_code for doing the plane cut.
 * PARAMETERS:
 *    primary_object: The object to be cut
 *    primary_object_data: The object data to be cut
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables plane_normal, plane_displacement
 *    must be properly initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if parameters or entry conditions are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/16/92 first voxel by Dewey Odhner
 *    Modified: 7/21/93 to correct plane G-codes by Dewey Odhner
 *    Modified: 10/20/93 to handle units by Dewey Odhner
 *    Modified: 5/24/01 to handle BINARY_B data by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::plane_setup(Shell *primary_object,
	Shell_data *primary_object_data)
{
	StructureInfo *str;
	double rotation_matrix[3][3], object_plane_normal[3], xysize2, unit;

	/* transform plane to object coordinates from center */
	str = &primary_object_data->file->file_header.str;
	AtoM(rotation_matrix, primary_object->angle[0], primary_object->angle[1],
		primary_object->angle[2]);
	vector_matrix_multiply(object_plane_normal,plane_normal,rotation_matrix);
	object_plane_distance = -plane_displacement+
		primary_object->displacement[0]*plane_normal[0]+
		primary_object->displacement[1]*plane_normal[1]+
		primary_object->displacement[2]*plane_normal[2];
	/* transform plane to object coordinates from first voxel */
	unit = unit_mm(primary_object_data);
	object_plane_distance -= .5*unit*(
		(Max_coordinate(primary_object_data, 0)+
		 Min_coordinate(primary_object_data, 0))*object_plane_normal[0]+
		(Max_coordinate(primary_object_data, 1)-
		 Min_coordinate(primary_object_data, 1))*object_plane_normal[1]+
		(Max_coordinate(primary_object_data, 2)-
		 Min_coordinate(primary_object_data, 2))*object_plane_normal[2]);
	/* transform plane to object coordinates in voxel units */
	xysize2 =	primary_object_data->slices<=1
				?	str->xysize[0]
				:	(Max_coordinate(primary_object_data, 2)-
						Min_coordinate(primary_object_data, 2))/
						(primary_object_data->slices-1);
	column_n_factor = -str->xysize[0]*object_plane_normal[0]*unit;
	row_n_factor = -str->xysize[1]*object_plane_normal[1]*unit;
	slice_n_factor = -xysize2*object_plane_normal[2]*unit;

	inside = 1;
	if (str->bit_fields_in_TSE[3] > 15)
	{
		inside_plane_G_code = BG_code(-object_plane_normal[0],
			-object_plane_normal[1], -object_plane_normal[2]);
		outside_plane_G_code = BG_code(object_plane_normal[0],
			object_plane_normal[1], object_plane_normal[2]);
	}
	else
	{
		inside_plane_G_code = G_code(-object_plane_normal[0],
			-object_plane_normal[1], -object_plane_normal[2]);
		outside_plane_G_code = G_code(object_plane_normal[0],
			object_plane_normal[1], object_plane_normal[2]);
		if (str->bit_fields_in_TSE[9] <= 26)
		{	inside_plane_G_code &= 0xffee;
			outside_plane_G_code &= 0xffee;
		}
	}
}

/*****************************************************************************
 * FUNCTION: cut_rows
 * DESCRIPTION: The function constructs the Shell0 representation
 *    of the new object (intersection) from the Shell0 representation of
 *    primary_object and voxel-of-intersection representation of the secondary
 *    object.
 * PARAMETERS:
 *    primary_object_data: The primary object
 *    cut_is_plane: Non-zero if the secondary object is the half-space defined
 *       by the plane; zero if it is the polygonal prism.
 *    restrict_normal: Non-zero if the normal codes of the new object are to
 *       be restricted to 3 bits each for n2 and n3.
 * SIDE EFFECTS: The static variables buffer_out_ptr, voxels_in_out_buffer,
 *    out_data, out_buffer_size, data_class may be changed.
 * ENTRY CONDITIONS: The following static variables must be properly set:
 *    clmns, rws, slcs should be initialized as
 *       clmns = Largest_y1(primary_object_data)+1;
 *       rws = primary_object_data->rows;
 *       slcs = primary_object_data->slices;
 *    new_ptr_table must point to (rws*slcs+1)*4 bytes of memory;
 *    out_data must point to out_buffer_size TSE's of allocated memory;
 *    S_ptr_table must point to the pointer table to the voxel-of-intersection
 *    list of the secondary object;
 *    S_row must point to a (clmns+2)-element array;
 *    inside indicates the secondary object is the region between voxels-of-
 *    intersection rather than the complement;
 *    adjacent_vois must point to a (clmns)-element array;
 *    plane_setup and get_plane_voi_list should be called if cut_is_plane,
 *    otherwise inside_plane_G_code, outside_plane_G_code need not be set but
 *    get_prism_voi_list should be called.
 *    The secondary object must have the same number of clmns, rws, and
 *    slcs as the primary object. (Columns are counted from Y1=0,
 *    not from Smallest_y1(primary_object_data).)
 * RETURN VALUE: DONE, EROR, or INTERRUPT3
 * EXIT CONDITIONS: Returns INTERRUPT3 if a right button press is detected;
 *    EROR if a memory allocation failure occurs.  Undefined if parameters
 *    and/or static variables are inconsistent.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/3/94 for fuzzy shells by Dewey Odhner
 *    Modified: 1/27/95 int changed to long by Dewey Odhner
 *    Modified: 5/24/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::cut_rows(Shell_data *primary_object_data,
	int cut_is_plane, int restrict_normal)
{
	unsigned short **in_ptr_ptr, **S_ptr_ptr, *in_ptr, *in_end_ptr,
		neighbor_code, *out_ptr1, *row_out_ptr, *new_out_data;
	voi_struct *adjacent_vois_end, *voi_ptr;
	S_voxel_struct *S_row_end, *S_in_ptr;
	size_t *this_ptr_ptr, last_out_ptr;
	int this_row, this_slice, this_column, in_segment;

	data_class = st_cl(primary_object_data);
	this_ptr_ptr = new_ptr_table;
	buffer_out_ptr = out_data;
	last_out_ptr = voxels_in_out_buffer = 0;
	in_ptr_ptr = primary_object_data->ptr_table;
	S_ptr_ptr = S_ptr_table;
	for (this_slice=0; this_slice<slcs; this_slice++)
	{	for (this_row=0; this_row<rws; this_row++)
		{
			/* Make sure there's enough room in out_buffer */
			if (voxels_in_out_buffer >= out_buffer_size-clmns)
			{	out_buffer_size += OUT_BLOCK_SIZE;
				new_out_data =
					(unsigned short *)realloc(out_data,out_buffer_size*
					(data_class==BINARY_B||data_class==BINARY_A?4:6));
				if (new_out_data == NULL)
				{	report_malloc_error();
					return (EROR);
				}
				out_data = new_out_data;
				buffer_out_ptr = out_data+(data_class==BINARY_B||
					data_class==BINARY_A? 2: 3)*voxels_in_out_buffer;
			}

			/* Get voxels-of-intresection defining secondary object for this
				row and adjacent rows */
			if (inside)
			{	adjacent_vois_end=adjacent_vois;
				if (this_slice>0)
					inside_insert_adjacent_vois(-rws, NZ, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_row>0)
					inside_insert_adjacent_vois(-1, NY, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_row<rws-1)
					inside_insert_adjacent_vois(1, PY, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_slice<slcs-1)
					inside_insert_adjacent_vois(rws, PZ, S_ptr_ptr,
						&adjacent_vois_end);
				inside_insert_adjacent_vois(0, VHERE, S_ptr_ptr,
					&adjacent_vois_end);
			}
			else
			{	adjacent_vois_end=adjacent_vois;
				if (this_slice>0)
					outside_insert_adjacent_vois(-rws, NZ, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_row>0)
					outside_insert_adjacent_vois(-1, NY, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_row<rws-1)
					outside_insert_adjacent_vois(1, PY, S_ptr_ptr,
						&adjacent_vois_end);
				if (this_slice<slcs-1)
					outside_insert_adjacent_vois(rws, PZ, S_ptr_ptr,
						&adjacent_vois_end);
				outside_insert_adjacent_vois(0, VHERE, S_ptr_ptr,
					&adjacent_vois_end);
			}

			/* Find border voxels & neighbor codes for S row */
			S_row_end = S_row;
			if (inside)
			{	neighbor_code = 0;
				in_segment = FALSE;
			}
			else
			{	neighbor_code = ALL_NEIGHBORS;
				in_segment = TRUE;
			}
			for (voi_ptr=adjacent_vois; voi_ptr<adjacent_vois_end; voi_ptr++)
			{	neighbor_code |= voi_ptr->start_code&OFF_ROW_CODES;
				if (voi_ptr->start_code & VHERE)
				{	in_segment = TRUE;
					neighbor_code |= PX;
				}
				if (in_segment)
				{	if (voi_ptr->end_code & VHERE)
						neighbor_code &= ~PX;
					this_column = voi_ptr->x;
					assert(S_row_end >= S_row);
					assert(S_row_end < S_row+clmns+2);
					if (neighbor_code != ALL_NEIGHBORS) /* boundary voxel */
					{	S_row_end->neighbor_code = neighbor_code;
						S_row_end++->x = this_column;
					}
				}
				neighbor_code &= ~(voi_ptr->end_code&OFF_ROW_CODES);
				if (in_segment)
				{	if (neighbor_code & PX)
					{	neighbor_code |= NX;
						assert(voi_ptr < adjacent_vois_end);
						if (neighbor_code != ALL_NEIGHBORS)
							while(++this_column < voi_ptr[1].x)
							{	assert(S_row_end >= S_row);
								assert(S_row_end < S_row+clmns+2);
								S_row_end->neighbor_code = neighbor_code;
								S_row_end++->x = this_column;
							}
					}
					else
					{	neighbor_code &= ~NX;
						in_segment = FALSE;
					}
				}
			}

			/* Find intersection of S & P (primary_object) row */
			/* First do border voxels of S that are in P */
			row_out_ptr = buffer_out_ptr;
			in_end_ptr = in_ptr_ptr[1];
			for (S_in_ptr=S_row; S_in_ptr<S_row_end; S_in_ptr++)
			{	this_column = S_in_ptr->x;
				for (in_ptr= *in_ptr_ptr; in_ptr<in_end_ptr; in_ptr+=
						(data_class==BINARY_B||data_class==BINARY_A? 2: 3))
					if ((data_class==BINARY_B? (*in_ptr&XMASK)<<1|((in_ptr[1]&
							0x8000)!=0):*in_ptr&XMASK) > this_column)
						break;
				if (in_ptr > *in_ptr_ptr) /* There is a voxel in P in same row
						with y1 <= this_column */
				{	in_ptr -= (data_class==BINARY_B||data_class==BINARY_A? 2: 3);
					if ((data_class==BINARY_B? (*in_ptr&XMASK)<<1|((in_ptr[1]&
							0x8000)!=0):*in_ptr&XMASK) == this_column) /* This
							voxel is in the shell domain of P */
					{	assert(buffer_out_ptr >= out_data);
						assert(buffer_out_ptr < out_data+
							out_buffer_size*(data_class==BINARY_A? 2: 3));
						if (data_class == BINARY_B)
						{
							*buffer_out_ptr = (*in_ptr&S_in_ptr->neighbor_code)
								|(this_column>>1);
							buffer_out_ptr[1] =
								(in_ptr[1]&0x7fff)|(this_column&1?0x8000:0);
						}
						else
						{
							*buffer_out_ptr =
								(*in_ptr&S_in_ptr->neighbor_code)|this_column;
							buffer_out_ptr[1] = in_ptr[1]; /* normal code */
						}
						if (data_class==BINARY_B || data_class==BINARY_A)
							buffer_out_ptr += 2;
						else
						{	buffer_out_ptr[2] = in_ptr[2]; /* fuzzy info */
							buffer_out_ptr += 3;
						}
						voxels_in_out_buffer++;
					}
					else if (*in_ptr & PX) /* This voxel is in
							the interior of P */
					{	assert(buffer_out_ptr >= out_data);
						assert(buffer_out_ptr < out_data+out_buffer_size*
							(data_class==BINARY_B||data_class==BINARY_A? 2: 3));
						*buffer_out_ptr = S_in_ptr->neighbor_code|
							S_in_ptr->x>>(data_class==BINARY_B?1:0);
						if (cut_is_plane)
							buffer_out_ptr[1] =
								inside
								?	inside_plane_G_code
								:	outside_plane_G_code;
						else
						{	double gx, gy, gz;
							unsigned short neighbors;

							neighbors = S_in_ptr->neighbor_code;
							gx = ((neighbors&PX)!=0)-((neighbors&NX)!=0);
							gy = ((neighbors&PY)!=0)-((neighbors&NY)!=0);
							gz = ((neighbors&PZ)!=0)-((neighbors&NZ)!=0);
							buffer_out_ptr[1] = data_class==BINARY_B?
								BG_code(gx, gy, gz): G_code(gx, gy, gz);
							/* Restrict if object has small normal codes. */
							if (restrict_normal)
								buffer_out_ptr[1] &= 0xffee;
						}
						switch (data_class)
						{	case BINARY_B:
								if (S_in_ptr->x & 1)
									buffer_out_ptr[1] |= 0x8000;
							case BINARY_A:
								buffer_out_ptr += 2;
								break;
							case PERCENT:
								buffer_out_ptr[1] |= 7<<11;
							case GRADIENT:
								/* interpolate gradient values */
								buffer_out_ptr[2] =
									((((in_ptr[3]&XMASK)-this_column)*
									  (in_ptr[2]>>8)+
									  (this_column-(*in_ptr&XMASK))*
									  (in_ptr[5]>>8)
									 )/((in_ptr[3]&XMASK)-(*in_ptr&XMASK))
									)<<8 | 0xff;
								buffer_out_ptr += 3;
								break;
							default:
								assert(false);
						}
						voxels_in_out_buffer++;
					}
				}
			}
			/* Now border voxels of P that are in S (& not border of S)*/
			for (in_ptr= *in_ptr_ptr; in_ptr<in_end_ptr;
					in_ptr+=(data_class==BINARY_B||data_class==BINARY_A? 2: 3))
			{	this_column = *in_ptr&XMASK;
				if (data_class == BINARY_B)
					this_column = this_column<<1|((in_ptr[1]&0x8000)!=0);
				for (S_in_ptr=S_row; S_in_ptr<S_row_end; S_in_ptr++)
					if ((S_in_ptr->x) > this_column)
						break;
				if
				(	(S_in_ptr==S_row && !inside) ||
					(S_in_ptr>S_row &&
					(--S_in_ptr)->x<this_column &&
					S_in_ptr->neighbor_code&PX)
				)
				{	if (data_class == BINARY_B)
						for (out_ptr1=buffer_out_ptr;
								out_ptr1>row_out_ptr &&
									((out_ptr1[-2]&XMASK)<<1|
									((out_ptr1[-1]&0x8000)!=0))>=this_column;
								out_ptr1-=2)
						{	assert(((out_ptr1[-2]&XMASK)<<1|
									((out_ptr1[-1]&0x8000)!=0))!= this_column);
							out_ptr1[0] = out_ptr1[-2];
							out_ptr1[1] = out_ptr1[-1];
							assert(out_ptr1 >= row_out_ptr+2);
							assert(row_out_ptr >= out_data);
							assert(out_ptr1+1 < out_data+2*out_buffer_size);
						}
					else if (data_class == BINARY_A)
						for (out_ptr1=buffer_out_ptr;
								out_ptr1>row_out_ptr &&
									(out_ptr1[-2]&XMASK)>=this_column;
								out_ptr1-=2)
						{	assert((out_ptr1[-2]&XMASK) != this_column);
							out_ptr1[0] = out_ptr1[-2];
							out_ptr1[1] = out_ptr1[-1];
							assert(out_ptr1 >= row_out_ptr+2);
							assert(row_out_ptr >= out_data);
							assert(out_ptr1+1 < out_data+2*out_buffer_size);
						}
					else
					{	for (out_ptr1=buffer_out_ptr;
								out_ptr1>row_out_ptr &&
									(out_ptr1[-3]&XMASK)>=this_column;
								out_ptr1-=3)
						{	assert((out_ptr1[-3]&XMASK) != this_column);
							out_ptr1[0] = out_ptr1[-3];
							out_ptr1[1] = out_ptr1[-2];
							out_ptr1[2] = out_ptr1[-1];
							assert(out_ptr1 >= row_out_ptr+3);
							assert(row_out_ptr >= out_data);
							assert(out_ptr1+1 < out_data+3*out_buffer_size);
						}
						out_ptr1[2] = in_ptr[2];
					}
					out_ptr1[0] = in_ptr[0];
					out_ptr1[1] = in_ptr[1];
					assert(out_ptr1 >= out_data);
					assert(out_ptr1+1 < out_data+(data_class==BINARY_B||
						data_class==BINARY_A? 2: 3)*out_buffer_size);
					buffer_out_ptr +=
						(data_class==BINARY_B||data_class==BINARY_A? 2: 3);
					voxels_in_out_buffer++;
				}
			}

			in_ptr_ptr++;
			assert(this_ptr_ptr >= new_ptr_table);
			assert(this_ptr_ptr <= new_ptr_table+rws*slcs);
			*this_ptr_ptr++ = last_out_ptr;
			last_out_ptr = (data_class==BINARY_B||data_class==BINARY_A? 2: 3)*
				voxels_in_out_buffer;
			S_ptr_ptr++;
		}
#if 0
@
		if (manip_peek_event(NULL, ignore_unless_abort, NULL)->priority ==
				FIRST)
		{	XEvent event;

			manip_next_event(&event);
			return (INTERRUPT3);
		}
#endif
	}
	*this_ptr_ptr = last_out_ptr;
	assert(this_ptr_ptr >= new_ptr_table);
	assert(this_ptr_ptr <= new_ptr_table+rws*slcs);
	return (DONE);
}

/*****************************************************************************
 * FUNCTION: get_plane_voi_list
 * DESCRIPTION: Computes the voxel-of-intersection representation of the
 *    secondary object for the plane cut.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The following static variables must be properly set:
 *    clmns, rws, slcs should be initialized as
 *       clmns = Largest_y1(primary_object_data)+1;
 *       rws = primary_object_data->rows;
 *       slcs = primary_object_data->slices;
 *    S_ptr_table must point to (rws*slcs+1)*4 bytes of memory;
 *    S_ptr_table[0] must point to rws*slcs*4 bytes of memory;
 *    plane_setup should be called first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 2/23/09 roundoff error handled by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::get_plane_voi_list()
{
	unsigned short **this_ptr_ptr;
	int this_row, this_slice;
	double this_row_n, end_row_n;

	this_ptr_ptr = S_ptr_table;
	for (this_slice=0; this_slice<slcs; this_slice++)
	{	this_row_n = this_slice*slice_n_factor-object_plane_distance;
		end_row_n = this_row_n+column_n_factor*clmns;
		for (this_row=0; this_row<rws; this_row++)
		{	if (this_row_n >= 0)
				if (end_row_n >= 0)
				{	this_ptr_ptr[0][0] = PX;
					this_ptr_ptr[0][1] = clmns-1;
					this_ptr_ptr[1] = this_ptr_ptr[0]+2;
				}
				else
				{	this_ptr_ptr[0][0] = PX;
					this_ptr_ptr[0][1] =
						(unsigned short)(-this_row_n/column_n_factor);
					if (this_ptr_ptr[0][1] >= clmns)
						this_ptr_ptr[0][1] = clmns-1;
					this_ptr_ptr[1] = this_ptr_ptr[0]+2;
				}
			else
				if (end_row_n >= 0)
				{	this_ptr_ptr[0][0] =
						(unsigned short)ceil(-this_row_n/column_n_factor);
					if (this_ptr_ptr[0][0] >= clmns)
						this_ptr_ptr[1] = this_ptr_ptr[0];
					else
					{
						this_ptr_ptr[0][0] |= PX;
						this_ptr_ptr[0][1] = clmns-1;
						this_ptr_ptr[1] = this_ptr_ptr[0]+2;
					}
				}
				else
					this_ptr_ptr[1] = this_ptr_ptr[0];
			this_row_n += row_n_factor;
			end_row_n += row_n_factor;
			this_ptr_ptr++;
		}
	}
}


/*****************************************************************************
 * FUNCTION: do_curved_cut
 * DESCRIPTION: Produces a new object by cutting the current object with
 *    a polygonal prism.
 * PARAMETERS:
 *    vertices: The vertices of a polygon in display_area coordinates;
 *       when connected in order by line segments and the last point to the
 *       first, must form a simple closed curve.
 *    nvertices: the number of vertices
 *    inside: If inside, the part of the current object inside the polygonal
 *       prism will form the new object; otherwise the part outside the
 *       prism will form the new object.
 *    depth: the altitude of the polygonal prism, = distance of the far base
 *       beyond the closest voxel of the object within the polygon, in mm.
 * SIDE EFFECTS: The old selected_object will be turned off.  The new
 *    object will become the selected_object.  Events will be removed from
 *    the queue.
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
 *    Modified: 4/25/94 not to destroy object until after use by Dewey Odhner.
 *    Modified: 5/19/94 to issue error message if not in memory by Dewey Odhner
 *    Modified: 5/9/95 to clear valid-flags after get_curve_z by Dewey Odhner
 *    Modified: 2/14/01 icons_exist checked before cutting icon by Dewey Odhner
 *    Modified: 7/16/09 to cut object which is on by Dewey Odhner.
 *    Modified: 7/21/09 old object not assumed to be at tail of list
 *       by Dewey Odhner.
 *
 *****************************************************************************/
void cvRenderer::do_curved_cut(X_Point *vertices, int nvertices, int inside, float depth)
{
	int z, icon_flag;
	Shell *new_object, *old_object, **tail;

	image_valid = image2_valid = icon_valid = FALSE;
	for(old_object=object_list; !old_object->O.on; old_object=old_object->next)
		;
	if (!old_object->main_data.in_memory ||
			(icons_exist && !old_object->icon_data.in_memory))
	{
		fprintf(stderr, "OBJECT IS NOT IN MEMORY.\n");
		error_flag = true;
		return;
	}
	new_object = (Shell *)calloc(1, sizeof(Shell));
	if (new_object == NULL)
	{	report_malloc_error();
		error_flag = true;
		return;
	}
	if (get_curve_z(&z, vertices, nvertices, main_image,
			&old_object->O.main_image))
	{	free(new_object);
		error_flag = true;
		return;
	}
	image_valid = image2_valid = icon_valid = FALSE;
	if (do_cut(old_object, icon_flag=FALSE, 1 , new_object, NULL,
			vertices, nvertices, z, inside, depth, None, NULL, NULL) != DONE)
	{	free(new_object);
		error_flag = true;
		return;
	}
	if (icons_exist && do_cut(old_object, icon_flag=TRUE, 1, new_object, NULL,
			vertices, nvertices, z, inside, depth, None, NULL, NULL) != DONE)
	{	destroy_object_data(&new_object->main_data);
		free(new_object);
		error_flag = true;
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

#define MAX_LINES 500

/*****************************************************************************
 * FUNCTION: get_curve_z
 * DESCRIPTION: Reports the largest (closest to viewer) z-buffer value in
 *    a polygonal region of an Object_image.
 * PARAMETERS:
 *    z: The result goes here.
 *    points: The vertices of the polygon in display_area coordinates if
 *       img is main_image, display_area2 coordinates if img is image2;
 *       when connected in order by line segments and the last point to the
 *       first, must form a simple closed curve.
 *    npoints: the number of vertices
 *    img: main_image or  image2
 *    ob_im: The image from which to get the z-buffer value; must be a valid
 *       projected image as with a successful call to project.
 * SIDE EFFECTS: Effects of update (in update.c) will occur.
 * ENTRY CONDITIONS: A call to manip_init or vol_init must be made first.
 * RETURN VALUE: 0 if no error occurs.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 6/29/94 memory violation corrected by Dewey Odhner
 *    Modified: 5/8/95 to return if object image not projected by Dewey Odhner
 *    Modified: 5/9/95 update called by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::get_curve_z(int *z, X_Point *points, int npoints, XImage *img, Object_image *ob_im)
{
	int x1, y1, x2=0, this_point, that_point, min_y_drawn, max_y_drawn;
	int in_count, clockwise, img_x, img_y, this_x, this_y;
	int *this_pixel, *next_row_pixel, *end_pixel;
	struct poi
	{	short x;
		enum {LEFT, RIGHT} side;
	} *poi_list, *end_poi, *in_poi, *seek_ptr;

	assert(img==main_image || img==image2);
//@	update(FALSE);
	if (img == main_image)
	{	img_x = image_x;
		img_y = image_y;
	}
	else
	{	img_x = image2_x;
		img_y = image2_y;
	}
	clockwise = polygon_is_clockwise(points, npoints);
	poi_list = (struct poi *)malloc(npoints*sizeof(struct poi));
	if (poi_list == NULL)
	{	report_malloc_error();
		return(1);
	}
	*z = 0;
	min_y_drawn = max_y_drawn = points[0].y;
	for (this_point=1; this_point<npoints; this_point++)
	{	if (points[this_point].y > max_y_drawn)
			max_y_drawn = points[this_point].y;
		if (points[this_point].y < min_y_drawn)
			min_y_drawn = points[this_point].y;
	}
	for (y1=min_y_drawn; y1<=max_y_drawn; y1++)
	{	/* build pixel-of-intersection list */
		end_poi = poi_list;
		for (this_point=0; this_point<npoints; this_point++)
		{	that_point = this_point+1;
			if (that_point >= npoints)
				that_point = 0;
			if (points[this_point].y>y1)
			{	if (points[that_point].y<=y1)
				{	end_poi->x =
						(short)rint(points[this_point].x+
							(y1-points[this_point].y)*
							(points[that_point].x-points[this_point].x)/
							(double)(points[that_point].y-points[this_point].y)
						);
					end_poi->side = clockwise? poi::LEFT: poi::RIGHT;
					end_poi++;
				}
			}
			else
				if (points[that_point].y>y1)
				{	end_poi->x =
						(short)rint(points[this_point].x+
							(y1-points[this_point].y)*
							(points[that_point].x-points[this_point].x)/
							(double)(points[that_point].y-points[this_point].y)
						);
					end_poi->side = clockwise? poi::RIGHT: poi::LEFT;
					end_poi++;
				}
				else if (points[this_point].y==y1 && points[that_point].y==y1)
				{
					if (points[that_point].x >= points[this_point].x)
					{	end_poi->x = points[this_point].x;
						end_poi->side = poi::LEFT;
						end_poi++;
						end_poi->x = points[that_point].x;
						end_poi->side = poi::RIGHT;
						end_poi++;
					}
					else
					{	end_poi->x = points[that_point].x;
						end_poi->side = poi::LEFT;
						end_poi++;
						end_poi->x = points[this_point].x;
						end_poi->side = poi::RIGHT;
						end_poi++;
					}
				}
		}

		/* sort pixels-of-intersection */
		for (in_poi=poi_list; in_poi<end_poi; in_poi++)
		{	struct poi this_poi;
			this_poi = *in_poi;
			if (this_poi.side == poi::LEFT)
				for (seek_ptr=in_poi; seek_ptr>poi_list; seek_ptr--)
				{	if (seek_ptr[-1].x < this_poi.x)
						break;
					*seek_ptr = seek_ptr[-1];
				}
			else
				for (seek_ptr=in_poi; seek_ptr>poi_list; seek_ptr--)
				{	if (seek_ptr[-1].x <= this_poi.x)
						break;
					*seek_ptr = seek_ptr[-1];
				}
			*seek_ptr = this_poi;
		}

		in_count = 0;
		for (in_poi=poi_list; in_poi<end_poi; )
		{	struct poi this_poi;

			this_poi = *in_poi;
			if (this_poi.side == poi::LEFT)
				for (; in_poi<end_poi && in_poi->x==this_poi.x &&
						in_poi->side==poi::LEFT; in_poi++)
				{	if (++in_count == 1)
						x2 = this_poi.x;
				}
			else
				for (; in_poi<end_poi && in_poi->x==this_poi.x &&
						in_poi->side==poi::RIGHT; in_poi++)
				{	if (--in_count == 0)
					{	x1 = this_poi.x+1;
						switch (ob_im->projected)
						{	case ANTI_ALIAS:
								this_y = 2*(y1-img_y)-img->height-
									ob_im->image_location[1];
								if (this_y<0 || this_y>=ob_im->image_size)
									break;
								this_x = 2*(x2-img_x)-img->width-
									ob_im->image_location[0];
								if (this_x < 0)
									this_x = 0;
								if (this_x >= ob_im->image_size)
									break;
								this_pixel = ob_im->z_buffer[this_y]+this_x;
								end_pixel =
									this_x+2*(x1-x2)>ob_im->image_size
									?	ob_im->z_buffer[this_y]+
											ob_im->image_size
									:	this_pixel+2*(x1-x2);
								next_row_pixel =
									ob_im->z_buffer[this_y+1]+this_x;
								while (this_pixel < end_pixel)
								{	if (*this_pixel > *z)
										*z = *this_pixel;
									this_pixel++;
									if (*this_pixel > *z)
										*z = *this_pixel;
									this_pixel++;
									if (*next_row_pixel > *z)
										*z = *next_row_pixel;
									next_row_pixel++;
									if (*next_row_pixel > *z)
										*z = *next_row_pixel;
									next_row_pixel++;
								}
								break;
							case PIXEL_REPLICATE:
								if ((y1&1) == 0)
								{	this_y = (y1-img_y-img->height/2)/2-
										ob_im->image_location[1];
									if (this_y<0 || this_y>=ob_im->image_size)
										break;
									this_x = (x2-img_x-img->width/2)/2-
										ob_im->image_location[0];
									if (this_x < 0)
										this_x = 0;
									if (this_x >= ob_im->image_size)
										break;
									this_pixel =
										ob_im->z_buffer[this_y]+this_x;
									end_pixel =
										this_x+(x1-x2)/2>ob_im->image_size
										?	ob_im->z_buffer[this_y]+
												ob_im->image_size
										:	this_pixel+(x1-x2)/2;
									while (this_pixel < end_pixel)
									{	if (*this_pixel > *z)
											*z = *this_pixel;
										this_pixel++;
									}
								}
								break;
							case ONE_TO_ONE:
								this_y = y1-img_y-img->height/2-
									ob_im->image_location[1]/2;
								if (this_y<0 || this_y>=ob_im->image_size)
									break;
								this_x = x2-img_x-img->width/2-
									ob_im->image_location[0]/2;
								if (this_x < 0)
									this_x = 0;
								if (this_x >= ob_im->image_size)
									break;
								this_pixel = ob_im->z_buffer[this_y]+this_x;
								end_pixel =
									this_x+x1-x2>ob_im->image_size
									?	ob_im->z_buffer[this_y]+
											ob_im->image_size
									:	this_pixel+x1-x2;
								while (this_pixel < end_pixel)
								{	if (*this_pixel > *z)
										*z = *this_pixel;
									this_pixel++;
								}
								break;
							default:
								free(poi_list);
								return (400);
						}
					}
				}
		}
	}
	free(poi_list);
	return(0);
}


#endif
