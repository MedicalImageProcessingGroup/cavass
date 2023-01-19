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

#if defined (WIN32) || defined (_WIN32)
    #pragma  warning(disable:4996)  //necessary because bill's compiler deprecated stdio.h
#else
#include <unistd.h>
#endif
#include <wx/filename.h>
#include "cvRender.h"

bool ok_to_write(const char []);
void display_message(const char []);

static const Window display_area=1, display_area2=2;

/*****************************************************************************
 * FUNCTION: set_original_objects
 * DESCRIPTION: Set the global variables separate_piece1, separate_piece2,
 *    and the static variables original_object1, original_object2.
 * PARAMETERS:
 *    set_panel_switches: A function to be called upon completion.
 * SIDE EFFECTS: The global variables object_list, image2_valid and the static
 *    variable original_objects_set may be changed,
 *    and/or any effects of set_panel_switches will occur.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 11/24/92 to use selected_object label by Dewey Odhner
 *    Modified: 7/28/99 parameter set_panel_switches added by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::set_original_objects()
{
	Shell *obj;

	if (separate_piece1!=NULL && !separate_piece1->O.on)
		separate_piece1 = NULL;
	if (separate_piece2!=NULL && !separate_piece2->O.on)
		separate_piece2 = NULL;
	if (separate_piece1==NULL && separate_piece2==NULL)
	{	if (object_from_label(selected_object)->on)
			separate_piece1= actual_object(object_from_label(selected_object));
		else
			for (obj=object_list; obj; obj=obj->next)
				if (obj->O.on)
				{	separate_piece1 = obj;
					break;
				}
		image2_valid = FALSE;
	}
	if (!original_objects_set)
	{	original_object1 =
			separate_piece1&&separate_piece1->O.on
			?	separate_piece1
			:	NULL;
		original_object2 =
			separate_piece2&&separate_piece2->O.on
			?	separate_piece2
			:	NULL;
		original_objects_set = TRUE;
	}
}

/*****************************************************************************
 * FUNCTION: combine_object_data
 * DESCRIPTION: Merges two disjoint binary shells.
 * PARAMETERS:
 *    object_data1: One of the shells to be merged; this one will be replaced
 *       by the result.
 *    object_data2: The second of the shells to be merged; must have the same
 *       sampling scheme and class as the first and be disjoint.
 * SIDE EFFECTS: Events will be removed from the queue.
 * ENTRY CONDITIONS: Both shells must be in memory.  A call to VCreateColormap
 *    must be made first.  The global variable display must be properly set.
 * RETURN VALUE: DONE, EROR, or INTERRUPT3.
 * EXIT CONDITIONS: Returns INTERRUPT3 if a right button press event is found.
 *    Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/22/93 to allow for shells with different slices & rows
 *       by Dewey Odhner
 *    Modified: 5/5/94 for fuzzy shells by Dewey Odhner
 *    Modified: 5/31/94 to check ptr_table[0] by Dewey Odhner
 *    Modified: 6/7/94 to set object_data1->file->file_header.str.num_of_TSE
 *       by Dewey Odhner
 *    Modified: 5/18/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::combine_object_data(Shell_data *object_data1,
	Shell_data *object_data2)
{
	unsigned short **new_ptr_table, *in_ptr1, *in_ptr2, *out_ptr,
		*next_row_ptr, *next_slice_ptr, *this_row_end, *next_row_end,
		*next_slice_end;
	int this_row, this_slice, rows, slices, obj1_slice_offset,
		obj2_slice_offset, obj1_row_offset, obj2_row_offset,
		min_row, min_slice, list_size;
	Classification_type data_class;

	data_class = st_cl(object_data1);
	for (obj1_slice_offset=0; obj1_slice_offset<
			object_data1->file->file_header.str.num_of_samples[0];
			obj1_slice_offset++)
		if (object_data1->file->file_header.str.loc_of_samples[
				obj1_slice_offset] == Min_coordinate(object_data1, 2))
			break;
	if (obj1_slice_offset ==
			object_data1->file->file_header.str.num_of_samples[0])
		return (EROR);
	for (obj2_slice_offset=0; obj2_slice_offset<
			object_data2->file->file_header.str.num_of_samples[0];
			obj2_slice_offset++)
		if (object_data2->file->file_header.str.loc_of_samples[
				obj2_slice_offset] == Min_coordinate(object_data2, 2))
			break;
	if (obj2_slice_offset ==
			object_data2->file->file_header.str.num_of_samples[0])
		return (EROR);
	min_slice =
		obj1_slice_offset<obj2_slice_offset
		?	obj1_slice_offset
		:	obj2_slice_offset;
	obj1_slice_offset -= min_slice;
	obj2_slice_offset -= min_slice;
	obj1_row_offset = (int)(rint(Min_coordinate(object_data1, 1)/
		object_data1->file->file_header.str.xysize[1]));
	obj2_row_offset = (int)(rint(Min_coordinate(object_data2, 1)/
		object_data2->file->file_header.str.xysize[1]));
	min_row =
		obj1_row_offset<obj2_row_offset
		?	obj1_row_offset
		:	obj2_row_offset;
	obj1_row_offset -= min_row;
	obj2_row_offset -= min_row;
	rows =
		obj1_row_offset+object_data1->rows>obj2_row_offset+object_data2->rows
		?	obj1_row_offset+object_data1->rows
		:	obj2_row_offset+object_data2->rows;
	slices =
		obj1_slice_offset+object_data1->slices>
			obj2_slice_offset+object_data2->slices
		?	obj1_slice_offset+object_data1->slices
		:	obj2_slice_offset+object_data2->slices;
	new_ptr_table = (unsigned short **)malloc((rows*slices+1)*sizeof(short *));
	if (new_ptr_table == NULL)
		return (EROR);
	list_size = ((object_data1->ptr_table[object_data1->rows*
		object_data1->slices]-object_data1->ptr_table[0])+
		(object_data2->ptr_table[object_data2->rows*object_data2->slices]-
		object_data2->ptr_table[0]))*2;
	if (list_size)
	{	new_ptr_table[0] = (unsigned short *)malloc(list_size);
		if (new_ptr_table[0] == NULL)
		{	free(new_ptr_table);
			return (EROR);
		}
	}
	else
		new_ptr_table[0] = NULL;
	/* Merge shells */
	out_ptr = new_ptr_table[0];
	for (this_slice=0; this_slice<slices; this_slice++)
	{
#if 0
		if (manip_peek_event(NULL, ignore_unless_abort,NULL)->priority== FIRST)
		{	XEvent e;

			if (new_ptr_table[0])
				free(new_ptr_table[0]);
			free(new_ptr_table);
			manip_next_event(&e);
			return (INTERRUPT3);
		}
#endif
		for (this_row=0; this_row<rows; this_row++)
		{	if (this_slice>=obj1_slice_offset &&
					this_slice<obj1_slice_offset+object_data1->slices &&
					this_row>=obj1_row_offset &&
					this_row<obj1_row_offset+object_data1->rows)
				if (this_slice>=obj2_slice_offset &&
						this_slice<obj2_slice_offset+object_data2->slices &&
						this_row>=obj2_row_offset &&
						this_row<obj2_row_offset+object_data2->rows)
				{	in_ptr2 = object_data2->ptr_table[
						(this_slice-obj2_slice_offset)*object_data2->rows+
						this_row-obj2_row_offset];
					in_ptr1 = object_data1->ptr_table[
						(this_slice-obj1_slice_offset)*object_data1->rows+
						this_row-obj1_row_offset];
					for (;;)
						if (in_ptr2<object_data2->ptr_table[
								(this_slice-obj2_slice_offset)*
								object_data2->rows+this_row-obj2_row_offset+1])
							if (in_ptr1<object_data1->ptr_table[
									(this_slice-obj1_slice_offset)*
									object_data1->rows+this_row-
									obj1_row_offset+1])
							{	assert((in_ptr2[0]&1023)!=(in_ptr1[0]&1023) ||
									(data_class==BINARY_B&&
									(in_ptr2[1]&32768)!=(in_ptr1[1]&32768)));
								if ((in_ptr2[0]&1023) < (in_ptr1[0]&1023))
								{	out_ptr[0] = in_ptr2[0];
									out_ptr[1] = in_ptr2[1];
									if (data_class==BINARY_B ||
											data_class==BINARY_A)
									{	out_ptr += 2;
										in_ptr2 += 2;
									}
									else
									{	out_ptr[2] = in_ptr2[2];
										out_ptr += 3;
										in_ptr2 += 3;
									}
								}
								else
								{	out_ptr[0] = in_ptr1[0];
									out_ptr[1] = in_ptr1[1];
									if (data_class==BINARY_B ||
											data_class==BINARY_A)
									{	out_ptr += 2;
										in_ptr1 += 2;
									}
									else
									{	out_ptr[2] = in_ptr1[2];
										out_ptr += 3;
										in_ptr1 += 3;
									}
								}
							}
							else
							{	out_ptr[0] = in_ptr2[0];
								out_ptr[1] = in_ptr2[1];
								if (data_class==BINARY_B ||
										data_class==BINARY_A)
								{	out_ptr += 2;
									in_ptr2 += 2;
								}
								else
								{	out_ptr[2] = in_ptr2[2];
									out_ptr += 3;
									in_ptr2 += 3;
								}
							}
						else
							if (in_ptr1<object_data1->ptr_table[
									(this_slice-obj1_slice_offset)*
									object_data1->rows+this_row-
									obj1_row_offset+1])
							{	out_ptr[0] = in_ptr1[0];
								out_ptr[1] = in_ptr1[1];
								if (data_class==BINARY_B ||
										data_class==BINARY_A)
								{	out_ptr += 2;
									in_ptr1 += 2;
								}
								else
								{	out_ptr[2] = in_ptr1[2];
									out_ptr += 3;
									in_ptr1 += 3;
								}
							}
							else
								break;
				}
				else
					for (in_ptr1=object_data1->ptr_table[
							(this_slice-obj1_slice_offset)*
							object_data1->rows+
							this_row-obj1_row_offset];
							in_ptr1<object_data1->ptr_table[
							(this_slice-obj1_slice_offset)*
							object_data1->rows+
							this_row-obj1_row_offset+1];
							in_ptr1 += data_class==BINARY_B ||
								data_class==BINARY_A? 2: 3)
					{	out_ptr[0] = in_ptr1[0];
						out_ptr[1] = in_ptr1[1];
						if (data_class==BINARY_B ||
								data_class==BINARY_A)
							out_ptr += 2;
						else
						{	out_ptr[2] = in_ptr1[2];
							out_ptr += 3;
						}
					}
			else
				if (this_slice>=obj2_slice_offset &&
						this_slice<obj2_slice_offset+object_data2->slices &&
						this_row>=obj2_row_offset &&
						this_row<obj2_row_offset+object_data2->rows)
					for (in_ptr2=object_data2->ptr_table[
							(this_slice-obj2_slice_offset)*
							object_data2->rows+
							this_row-obj2_row_offset];
							in_ptr2<object_data2->ptr_table[
							(this_slice-obj2_slice_offset)*
							object_data2->rows+
							this_row-obj2_row_offset+1];
							in_ptr2 += data_class==BINARY_B ||
								data_class==BINARY_A? 2: 3)
					{	out_ptr[0] = in_ptr2[0];
						out_ptr[1] = in_ptr2[1];
						if (data_class==BINARY_B ||
								data_class==BINARY_A)
							out_ptr += 2;
						else
						{	out_ptr[2] = in_ptr2[2];
							out_ptr += 3;
						}
					}
			new_ptr_table[this_slice*rows+this_row+1] = out_ptr;
		}
	}
	/* Set neighbor codes.  Since the merged shells were disjoint, we only
	 * need to set neighbor codes for adjacent border voxels. */
	for (this_slice=0; this_slice<slices; this_slice++)
	{
#if 0
		if (manip_peek_event(NULL,ignore_unless_abort,NULL)->priority == FIRST)
		{	XEvent e;

			if (new_ptr_table[0])
				free(new_ptr_table[0]);
			free(new_ptr_table);
			manip_next_event(&e);
			return (INTERRUPT3);
		}
#endif
		for (this_row=0; this_row<rows; this_row++)
		{	out_ptr = new_ptr_table[this_slice*rows+this_row];
			this_row_end = new_ptr_table[this_slice*rows+this_row+1];
			if (this_row_end == out_ptr)
				continue;
			next_row_ptr = this_row_end;
			next_row_end =
				this_row<rows-1
				?	new_ptr_table[this_slice*rows+this_row+2]
				:	next_row_ptr;
			if (this_slice==slices-1)
				next_slice_ptr = next_slice_end = out_ptr;
			else
			{	next_slice_ptr = new_ptr_table[(this_slice+1)*rows+this_row];
				next_slice_end = new_ptr_table[(this_slice+1)*rows+this_row+1];
			}
			switch (data_class)
			{	case BINARY_A:
					for (; out_ptr<this_row_end; out_ptr+=2)
					{	if (out_ptr<this_row_end-2 &&
								(out_ptr[2]&1023)==(out_ptr[0]&1023)+1)
						{	out_ptr[0] |= PX;
							out_ptr[2] |= NX;
						}
						while (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)<(out_ptr[0]&1023))
							next_row_ptr += 2;
						if (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)==(out_ptr[0]&1023))
						{	out_ptr[0] |= PY;
							next_row_ptr[0] |= NY;
						}
						while (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)<(out_ptr[0]&1023))
							next_slice_ptr += 2;
						if (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)==(out_ptr[0]&1023))
						{	out_ptr[0] |= PZ;
							next_slice_ptr[0] |= NZ;
						}
					}
					break;
				case BINARY_B:
					for (; out_ptr<this_row_end; out_ptr+=2)
					{	if (out_ptr<this_row_end-2 &&
							   ((out_ptr[2]&1023)<<1|((out_ptr[3]&0x8000)!=0))==
							   ((out_ptr[0]&1023)<<1|((out_ptr[1]&0x8000)!=0))+1)
						{	out_ptr[0] |= PX;
							out_ptr[2] |= NX;
						}
						while (next_row_ptr<next_row_end && ((next_row_ptr[0]&
								1023)<<1|((next_row_ptr[1]&0x8000)!=0))<
								((out_ptr[0]&1023)<<1|((out_ptr[1]&0x8000)!=0)))
							next_row_ptr += 2;
						if (next_row_ptr<next_row_end && ((next_row_ptr[0]&
								1023)<<1|((next_row_ptr[1]&0x8000)!=0))==
								((out_ptr[0]&1023)<<1|((out_ptr[1]&0x8000)!=0)))
						{	out_ptr[0] |= PY;
							next_row_ptr[0] |= NY;
						}
						while (next_slice_ptr<next_slice_end &&
								((next_slice_ptr[0]&1023)<<1|
								((next_slice_ptr[1]&0x8000)!=0))<
								((out_ptr[0]&1023)<<1|((out_ptr[1]&0x8000)!=0)))
							next_slice_ptr += 2;
						if (next_slice_ptr<next_slice_end &&
								((next_slice_ptr[0]&1023)<<1|
								((next_slice_ptr[1]&0x8000)!=0))==
								((out_ptr[0]&1023)<<1|((out_ptr[1]&0x8000)!=0)))
						{	out_ptr[0] |= PZ;
							next_slice_ptr[0] |= NZ;
						}
					}
					break;
				case GRADIENT:
					for (; out_ptr<this_row_end; out_ptr+=3)
					{	if (out_ptr<this_row_end-3 &&
								(out_ptr[3]&1023)==(out_ptr[0]&1023)+1)
						{	if ((out_ptr[5]&0xffff) == 0xffff)
								out_ptr[0] |= PX;
							if ((out_ptr[2]&0xffff) == 0xffff)
								out_ptr[3] |= NX;
						}
						while (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)<(out_ptr[0]&1023))
							next_row_ptr += 3;
						if (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)==(out_ptr[0]&1023))
						{	if ((next_row_ptr[2]&0xffff) == 0xffff)
								out_ptr[0] |= PY;
							if ((out_ptr[2]&0xffff) == 0xffff)
								next_row_ptr[0] |= NY;
						}
						while (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)<(out_ptr[0]&1023))
							next_slice_ptr += 3;
						if (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)==(out_ptr[0]&1023))
						{	if ((next_slice_ptr[2]&0xffff) == 0xffff)
								out_ptr[0] |= PZ;
							if ((out_ptr[2]&0xffff) == 0xffff)
								next_slice_ptr[0] |= NZ;
						}
					}
					break;
				case PERCENT:
					for (; out_ptr<this_row_end; out_ptr+=3)
					{	if (out_ptr<this_row_end-3 &&
								(out_ptr[3]&1023)==(out_ptr[0]&1023)+1)
						{	if ((out_ptr[4]&7<<11)==7<<11 &&
									(out_ptr[5]&0xffff)==0xffff)
								out_ptr[0] |= PX;
							if ((out_ptr[1]&7<<11)==7<<11 &&
									(out_ptr[2]&0xffff)==0xffff)
								out_ptr[3] |= NX;
						}
						while (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)<(out_ptr[0]&1023))
							next_row_ptr += 3;
						if (next_row_ptr<next_row_end &&
								(next_row_ptr[0]&1023)==(out_ptr[0]&1023))
						{	if ((next_row_ptr[1]&7<<11)==7<<11 &&
									(next_row_ptr[2]&0xffff)==0xffff)
								out_ptr[0] |= PY;
							if ((out_ptr[1]&7<<11)==7<<11 &&
									(out_ptr[2]&0xffff)==0xffff)
								next_row_ptr[0] |= NY;
						}
						while (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)<(out_ptr[0]&1023))
							next_slice_ptr += 3;
						if (next_slice_ptr<next_slice_end &&
								(next_slice_ptr[0]&1023)==(out_ptr[0]&1023))
						{	if ((next_slice_ptr[1]&7<<11)==7<<11 &&
									(next_slice_ptr[2]&0xffff)==0xffff)
								out_ptr[0] |= PZ;
							if ((out_ptr[1]&7<<11)==7<<11 &&
									(out_ptr[2]&0xffff)==0xffff)
								next_slice_ptr[0] |= NZ;
						}
					}
					break;
				default:
					assert(false);
			}
		}
	}
	/* Discard interior voxels */
	out_ptr = this_row_end = new_ptr_table[0];
	for (this_slice=0; this_slice<slices; this_slice++)
	{
#if 0
		if (manip_peek_event(NULL,ignore_unless_abort,NULL)->priority == FIRST)
		{	XEvent e;

			if (new_ptr_table[0])
				free(new_ptr_table[0]);
			free(new_ptr_table);
			manip_next_event(&e);
			return (INTERRUPT3);
		}
#endif
		for (this_row=0; this_row<rows; this_row++)
		{	for (in_ptr1=this_row_end,
					this_row_end=new_ptr_table[this_slice*rows+this_row+1];
					in_ptr1<this_row_end;
					in_ptr1+=data_class==BINARY_B||data_class==BINARY_A? 2:3)
				if ((in_ptr1[0]&ALL_NEIGHBORS) != ALL_NEIGHBORS)
				{	out_ptr[0] = in_ptr1[0];
					out_ptr[1] = in_ptr1[1];
					if (data_class==BINARY_B || data_class==BINARY_A)
						out_ptr += 2;
					else
					{	out_ptr[2] = in_ptr1[2];
						out_ptr += 3;
					}
				}
			new_ptr_table[this_slice*rows+this_row+1] = out_ptr;
		}
	}

	if (object_data1->ptr_table[0])
		free(object_data1->ptr_table[0]);
	free(object_data1->ptr_table);
	object_data1->ptr_table = new_ptr_table;
	Min_str_coordinate(object_data1, 1) =
		min_row*object_data1->file->file_header.str.xysize[1];
	Max_str_coordinate(object_data1, 1) =
		(min_row+rows-1)*object_data1->file->file_header.str.xysize[1];
	Min_str_coordinate(object_data1, 2) =
		object_data1->file->file_header.str.loc_of_samples[min_slice];
	Max_str_coordinate(object_data1, 2) =
		object_data1->file->file_header.str.loc_of_samples[
		min_slice+slices-1];
	object_data1->rows = rows;
	object_data1->slices = slices;
	object_data1->file->file_header.str.num_of_TSE[object_data1->shell_number]=
		(object_data1->ptr_table[object_data1->rows*
		object_data1->slices]-object_data1->ptr_table[0])/
		(data_class==BINARY_B||data_class==BINARY_A? 2:3);
	object_data1->file->file_header.str.volume_valid = FALSE;
	object_data1->file->file_header.str.surface_area_valid = FALSE;
	return (DONE);
}

/*****************************************************************************
 * FUNCTION: do_separation
 * DESCRIPTION: Cuts a part out of one of the separated pieces and merges it
 *    with the other by cutting with a polygonal prism.
 * PARAMETERS:
 *    vertices: The vertices of the polygon in display area coordinates;
 *       when connected in order by line segments and the last point to the
 *       first, must form a simple closed curve.
 *    nvertices: The number of vertices.
 *    inside: If inside, the part of the current object inside the polygonal
 *       prism will go to the other piece; otherwise the part outside the
 *       prism will go to the other piece.
 *    draw_window: The display area coordinates to use: display_area or
 *        display_area2.
 *    depth: the altitude of the polygonal prism, = distance of the far base
 *       beyond the closest voxel of the object within the polygon, in mm.
 * SIDE EFFECTS: The global variables object_list, separate_piece1,
 *    separate_piece2, image_valid, image2_valid, icon_valid may be changed.
 *    Events will be removed from the queue.  An error message may be issued.
 * ENTRY CONDITIONS: Calls to manip_init and set_separate_display_areas must
 *    be made first.
 *    The static variables original_object1, original_object2 must be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Returns if a right button press event is found.
 *    Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 11/20/92 to add objects to tail instead of head by Dewey Odhner
 *    Modified: 11/24/92 to use selected_object label by Dewey Odhner
 *    Modified: 1/5/93 to make old object immobile by Dewey Odhner
 *    Modified: 1/22/93 allow for shells with different slices & rows
 *       by Dewey Odhner
 *    Modified: 1/25/93 to transform marks to new coordinates by Dewey Odhner
 *    Modified: 7/13/93 not to make old object immobile by Dewey Odhner
 *    Modified: 10/20/93 to handle units by Dewey Odhner
 *    Modified: 5/19/94 to issue error message if not in memory by Dewey Odhner
 *    Modified: 5/8/95 to transform marks to new coordinates right
 *       by Dewey Odhner
 *    Modified: 2/15/01 icons_exist checked before using icon by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::do_separation(X_Point *vertices, int nvertices, int inside,
	Window draw_window, float depth)
{
	double shift[3], center_o[3], center_n[3], center_on[3], rotation[3][3],
		unit;
	int j, z, icon_flag;
	Shell *new_object1, *new_object2, *new_object_from, *new_object_to,
		*old_object_from, *old_object_to, *old_object_too, **tail;

	if (draw_window==display_area && separate_piece1!=NULL &&
			separate_piece1->O.on)
	{	old_object_from = separate_piece1;
		old_object_to = separate_piece2;
		if (get_curve_z(&z, vertices, nvertices, main_image,
				&separate_piece1->O.main_image))
			return;
	}
	else if (draw_window==display_area2 && separate_piece2!=NULL &&
			separate_piece2->O.on)
	{	old_object_from = separate_piece2;
		old_object_to = separate_piece1;
		if (get_curve_z(&z, vertices, nvertices, image2,
				&separate_piece2->O.main_image))
			return;
	}
	else
	{
		error_flag = true;
		return;
	}
	if (old_object_to!=NULL && !old_object_to->O.on)
		old_object_to = NULL;
	if (!old_object_from->main_data.in_memory ||
				(icons_exist && !old_object_from->icon_data.in_memory))
	{
		display_message("OBJECT IS NOT IN MEMORY.");
		return;
	}
	icon_valid = FALSE;
	new_object1 = (Shell *)calloc(1, sizeof(Shell));
	if (new_object1 == NULL)
	{	report_malloc_error();
		return;
	}
	new_object2 = (Shell *)calloc(1, sizeof(Shell));
	if (new_object2 == NULL)
	{	report_malloc_error();
		free(new_object1);
		return;
	}
	if (draw_window == display_area)
	{	new_object_from = new_object1;
		new_object_to = new_object2;
	}
	else
	{	new_object_from = new_object2;
		new_object_to = new_object1;
	}
	if (do_cut(old_object_from, icon_flag=FALSE, 2, new_object_from,
			new_object_to, vertices, nvertices, z, inside, depth, draw_window,
			NULL, NULL) != DONE)
	{	free(new_object1);
		free(new_object2);
		return;
	}
	if (icons_exist && do_cut(old_object_from, icon_flag=TRUE, 2,
			new_object_from, new_object_to, vertices, nvertices, z, inside,
			depth, draw_window, NULL, NULL) != DONE)
	{	destroy_object_data(&new_object1->main_data);
		free(new_object1);
		destroy_object_data(&new_object2->main_data);
		free(new_object2);
		return;
	}
	/* combine new_object_to with old old_object_to */
	if (old_object_to)
	{	unit = unit_mm(&old_object_to->main_data);
		for (j=0; j<3; j++)
		{	center_o[j]= .5*unit*(Min_coordinate(&old_object_to->main_data, j)+
				Max_coordinate(&old_object_to->main_data, j));
			center_on[j]= .5*unit*(Min_coordinate(&new_object_to->main_data,j)+
				Max_coordinate(&new_object_to->main_data, j));
		}
		if ((combine_object_data(&new_object_to->main_data,
				&old_object_to->main_data)!=DONE) ||
				(icons_exist && combine_object_data(&new_object_to->icon_data,
				&old_object_to->icon_data)!=DONE) ||
				combine_marks(new_object_to, old_object_to)!=DONE)
		{	destroy_object(new_object1);
			destroy_object(new_object2);
			return;
		}
		for (j=0; j<3; j++)
			center_n[j]= .5*unit*(Min_coordinate(&new_object_to->main_data, j)+
				Max_coordinate(&new_object_to->main_data, j));
		for (j=0; j<new_object_to->marks-old_object_to->marks; j++)
		{	vector_add(new_object_to->mark[j], new_object_to->mark[j],
				center_on);
			vector_subtract(new_object_to->mark[j], new_object_to->mark[j],
				center_n);
		}
		for (; j<new_object_to->marks; j++)
		{	vector_add(new_object_to->mark[j], new_object_to->mark[j],
				center_o);
			vector_subtract(new_object_to->mark[j], new_object_to->mark[j],
				center_n);
		}
		old_object_to->O.on = FALSE;
//@		if (&old_object_to->O == object_from_label(selected_object))
//@			display_error(VChangePanelItem("STATUS", "STATUS", SWITCH_OFF));
		old_object_too = old_object_to;
	}
	else
		old_object_too = old_object_from;
	/* add new cut objects to tail of object_list */
	for (tail= &object_list; *tail; tail= &(*tail)->next)
		;
	*tail = new_object1;
	new_object1->next = new_object2;
	separate_piece1 = new_object1;
	separate_piece2 = new_object2;
	new_object_from->angle[0] = old_object_from->angle[0];
	new_object_from->angle[1] = old_object_from->angle[1];
	new_object_from->angle[2] = old_object_from->angle[2];
	new_object_from->displacement[0] = old_object_from->displacement[0];
	new_object_from->displacement[1] = old_object_from->displacement[1];
	new_object_from->displacement[2] = old_object_from->displacement[2];
	new_object_from->plan_angle[0] = old_object_from->plan_angle[0];
	new_object_from->plan_angle[1] = old_object_from->plan_angle[1];
	new_object_from->plan_angle[2] = old_object_from->plan_angle[2];
	new_object_from->plan_displacement[0] =
		old_object_from->plan_displacement[0];
	new_object_from->plan_displacement[1] =
		old_object_from->plan_displacement[1];
	new_object_from->plan_displacement[2] =
		old_object_from->plan_displacement[2];
	new_object_from->mobile = old_object_from->mobile;
	new_object_from->O.on = TRUE;
	new_object_from->O.opacity = old_object_from->O.opacity;
	new_object_from->O.specular_fraction= old_object_from->O.specular_fraction;
	new_object_from->O.specular_exponent= old_object_from->O.specular_exponent;
	new_object_from->O.diffuse_exponent = old_object_from->O.diffuse_exponent;
	new_object_from->O.specular_n = old_object_from->O.specular_n;
	new_object_from->O.diffuse_n = old_object_from->O.diffuse_n;
	new_object_from->O.rgb = old_object_from->O.rgb;
	new_object_from->O.color = old_object_from->O.color;
	new_object_to->angle[0] = old_object_too->angle[0];
	new_object_to->angle[1] = old_object_too->angle[1];
	new_object_to->angle[2] = old_object_too->angle[2];
	new_object_to->displacement[0] = old_object_too->displacement[0];
	new_object_to->displacement[1] = old_object_too->displacement[1];
	new_object_to->displacement[2] = old_object_too->displacement[2];
	new_object_to->plan_angle[0] = old_object_too->plan_angle[0];
	new_object_to->plan_angle[1] = old_object_too->plan_angle[1];
	new_object_to->plan_angle[2] = old_object_too->plan_angle[2];
	new_object_to->plan_displacement[0] = old_object_too->plan_displacement[0];
	new_object_to->plan_displacement[1] = old_object_too->plan_displacement[1];
	new_object_to->plan_displacement[2] = old_object_too->plan_displacement[2];
	new_object_to->mobile = old_object_too->mobile;
	new_object_to->O.on = TRUE;
	new_object_to->O.opacity = old_object_too->O.opacity;
	new_object_to->O.specular_fraction = old_object_too->O.specular_fraction;
	new_object_to->O.specular_exponent = old_object_too->O.specular_exponent;
	new_object_to->O.diffuse_exponent = old_object_too->O.diffuse_exponent;
	new_object_to->O.specular_n = old_object_too->O.specular_n;
	new_object_to->O.diffuse_n = old_object_too->O.diffuse_n;
	new_object_to->O.rgb = old_object_too->O.rgb;
	new_object_to->O.color = old_object_too->O.color;
	if (old_object_to)
	{	AtoM(rotation, new_object_to->angle[0], new_object_to->angle[1],
			new_object_to->angle[2]);
		vector_subtract(shift, center_n, center_o);
		matrix_vector_multiply(shift, rotation, shift);
		vector_add(new_object_to->displacement, new_object_to->displacement,
			shift);
		AtoM(rotation, new_object_to->plan_angle[0],
			new_object_to->plan_angle[1], new_object_to->plan_angle[2]);
		vector_subtract(shift, center_n, center_o);
		matrix_vector_multiply(shift, rotation, shift);
		vector_add(new_object_to->plan_displacement,
			new_object_to->plan_displacement, shift);
		compute_diameter(new_object_to);
	}
	if (old_object_too==old_object_to && old_object_to!=original_object1 &&
			old_object_to!=original_object2)
		remove_object(old_object_to);
	if (old_object_from!=original_object1 && old_object_from!=original_object2)
		remove_object(old_object_from);
	else
	{	old_object_from->O.on = FALSE;
//@		if (&old_object_from->O == object_from_label(selected_object))
//@			display_error(VChangePanelItem("STATUS", "STATUS", SWITCH_OFF));
	}
	compactify_object(new_object_from, icons_exist);
	compactify_object(new_object_to, icons_exist);
}

#if 0
/*****************************************************************************
 * FUNCTION: do_split
 * DESCRIPTION: Splits the selected separated piece through marks.
 * PARAMETERS:
 *    set_panel_switches: A function to be called upon completion.
 * SIDE EFFECTS: The global variables object_list, separate_piece1,
 *    separate_piece2, image_valid, image2_valid, icon_valid may be changed,
 *    and/or any effects of set_panel_switches will occur.
 *    Events will be removed from the queue.  An error message may be issued.
 * ENTRY CONDITIONS: A call to manip_init must be made first.
 *    The selected object must be on, binary, in memory, and have marks.
 *    The static variables original_object1, original_object2 must be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Returns if a right button press event is found.
 *    Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 7/26/99 by Dewey Odhner
 *    Modified: 7/27/99 parameter set_panel_switches added by Dewey Odhner
 *    Modified: 8/27/99 slice coordinate corrected by Dewey Odhner
 *    Modified: 2/15/01 icons_exist checked before using icon by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::do_split(void (*set_panel_switches)())
{
	double shift[3], center_o[3], center_n[3], center_on[3], rotation[3][3],
		unit, this_dist, max_dist, this_comp, mm, (*mark_list)[3], deep,
		(*tmp_list)[3];
	int j, icon_flag, max_dist_point, k, m;
	Shell *new_object1, *new_object2, *new_object_from, *new_object_to,
		*old_object_from, *old_object_to, *old_object_too, **tail;
	struct Prism prism;
	char command[100];
	FILE *fp;

	set_button_action("", "", "ABORT", "", "", "ABORT");
	VDisplayStatus(1);
	XFlush(display);
	set_original_objects(set_panel_switches);
	old_object_from = actual_object(object_from_label(selected_object));
	if (old_object_from == separate_piece1)
		old_object_to = separate_piece2;
	else
		old_object_to = separate_piece1;
	if (old_object_to!=NULL && !old_object_to->O.on)
		old_object_to = NULL;
	image_valid = icon_valid = image2_valid = FALSE;
	new_object1 = (Shell *)calloc(1, sizeof(Shell));
	if (new_object1 == NULL)
	{	report_malloc_error();
		return;
	}
	new_object2 = (Shell *)calloc(1, sizeof(Shell));
	if (new_object2 == NULL)
	{	report_malloc_error();
		free(new_object1);
		return;
	}
	if (old_object_from == separate_piece1)
	{	new_object_from = new_object1;
		new_object_to = new_object2;
	}
	else
	{	new_object_from = new_object2;
		new_object_to = new_object1;
	}
	mark_list = (triple *)malloc(old_object_from->marks*sizeof(triple));
	if (mark_list == NULL)
	{
		free(new_object1);
		free(new_object2);
		return;
	}
	memcpy(mark_list, old_object_from->mark,
		old_object_from->marks*sizeof(triple));
	/* Change marks to voxel coordinates. */
	mm = 1/unit_mm(&old_object_from->main_data);
	for (j=0; j<old_object_from->marks; j++)
	{
		for (k=0; k<3; k++)
			mark_list[j][k] *= mm;
		mark_list[j][0] = mark_list[j][0]+
			.5*(Max_coordinate(&old_object_from->main_data, 0)+
			Min_coordinate(&old_object_from->main_data, 0));
		mark_list[j][1] = mark_list[j][1]+
			.5*(Max_coordinate(&old_object_from->main_data, 1)-
			Min_coordinate(&old_object_from->main_data, 1));
		mark_list[j][2] = mark_list[j][2]+
			.5*(Max_coordinate(&old_object_from->main_data, 2)-
			Min_coordinate(&old_object_from->main_data, 2));
		mark_list[j][0] /=
			old_object_from->main_data.file->file_header.str.xysize[0];
		mark_list[j][1] /=
			old_object_from->main_data.file->file_header.str.xysize[1];
		mark_list[j][2] /= Slice_spacing(&old_object_from->main_data);
	}
	deep = old_object_from->main_data.rows+old_object_from->main_data.slices+
		Largest_y1(&old_object_from->main_data);
	if (old_object_to)
		old_object_to->O.on = FALSE;
	j = output_plan_data("mnpspt_tmp_.BS0");
	if (j)
	{
		display_error(j);
		free(new_object1);
		free(new_object2);
		free(mark_list);
		return;
	}
	if (old_object_to)
		old_object_to->O.on = TRUE;
	if (system("BS0_TO_BIM mnpspt_tmp_.BS0 0 mnpspt_tmp_.BIM 0 >/dev/null"))
	{
		display_error(400);
		free(new_object1);
		free(new_object2);
		free(mark_list);
		return;
	}
	unlink("mnpspt_tmp_.BS0");
	if (system("distance3D mnpspt_tmp_.BIM mnpspt_tmp_.IM0 >/dev/null"))
	{
		display_error(400);
		free(new_object1);
		free(new_object2);
		free(mark_list);
		return;
	}
	unlink("mnpspt_tmp_.BIM");
	if (system(
			"local_maximumness mnpspt_tmp_.IM0 mnpspt_tmp__.IM0 0 >/dev/null"))
	{
		display_error(400);
		free(new_object1);
		free(new_object2);
		free(mark_list);
		return;
	}
	unlink("mnpspt_tmp_.IM0");
	prism.n = 5;
	prism.base_points = (triple *)malloc(sizeof(triple));
	if (prism.base_points == NULL)
	{
		report_malloc_error();
		free(new_object1);
		free(new_object2);
		free(mark_list);
		return;
	}
	memcpy(prism.base_points, mark_list, sizeof(triple));
	for (j=0; j<old_object_from->marks-1; j++)
	{
		sprintf(command, "findpath mnpspt_tmp__.IM0 %.0f %.0f %.0f %.0f %.0f %.0f mnpspt_tmp_.BIM mnpspt_tmp_.list 0 1 >/dev/null",
			mark_list[j][0]-Smallest_y1(&old_object_from->main_data),
			mark_list[j][1],
			mark_list[j][2]+Min_coordinate(&old_object_from->main_data, 2),
			mark_list[j+1][0]-Smallest_y1(&old_object_from->main_data),
			mark_list[j+1][1],
			mark_list[j+1][2]+Min_coordinate(&old_object_from->main_data, 2));
		if (system(command))
		{
			display_error(400);
			free(new_object1);
			free(new_object2);
			free(mark_list);
			free(prism.base_points);
			return;
		}
		unlink("mnpspt_tmp_.BIM");
		fp = fopen("mnpspt_tmp_.list", "rb");
		if (fp == NULL)
		{
			display_error(4);
			free(new_object1);
			free(new_object2);
			free(mark_list);
			free(prism.base_points);
			return;
		}
		if (fscanf(fp, "%d\n", &k) != 1)
		{
			display_error(2);
			free(new_object1);
			free(new_object2);
			free(mark_list);
			free(prism.base_points);
			fclose(fp);
			return;
		}
		k--;
		tmp_list = (triple *)realloc(prism.base_points,
			(prism.n+k)*sizeof(triple));
		if (tmp_list == NULL)
		{
			report_malloc_error();
			free(new_object1);
			free(new_object2);
			free(mark_list);
			free(prism.base_points);
			fclose(fp);
			return;
		}
		prism.base_points = tmp_list;
		for (m=k; m>0; m--)
		{
			if (fscanf(fp, "%lf %lf %lf\n", prism.base_points[prism.n-5+m],
				prism.base_points[prism.n-5+m]+1,
				prism.base_points[prism.n-5+m]+2) != 3)
			{
				display_error(2);
				free(new_object1);
				free(new_object2);
				free(mark_list);
				free(prism.base_points);
				fclose(fp);
				return;
			}
			prism.base_points[prism.n-5+m][0] +=
				Smallest_y1(&old_object_from->main_data);
			prism.base_points[prism.n-5+m][2] -=
				Min_coordinate(&old_object_from->main_data, 2);
		}
		prism.n += k;
		fclose(fp);
		unlink("mnpspt_tmp_.list");
	}
	unlink("mnpspt_tmp__.IM0");
	free(mark_list);
	max_dist_point = 0;
	max_dist = 0;
	vector_subtract(center_o, prism.base_points[prism.n-5],
		prism.base_points[0]);
	this_dist = 1/sqrt(center_o[0]*center_o[0]+
		center_o[1]*center_o[1]+center_o[2]*center_o[2]);
	for (k=0; k<3; k++)
		center_o[k] *= this_dist;
	/* center_o is now the direction of the last mark from the first. */
	for (j=1; j<prism.n-5; j++)
	{
		vector_subtract(shift, prism.base_points[j],
			prism.base_points[0]);
		this_dist = 0;
		for (k=0; k<3; k++)
		{
			this_comp = shift[(k+2)%3]*center_o[(k+1)%3]-
				shift[(k+1)%3]*center_o[(k+2)%3];
			this_dist += this_comp*this_comp;
		}
		if (this_dist > max_dist)
		{
			max_dist_point = j;
			max_dist = this_dist;
		}
	}
	if (max_dist == 0)
	{
		display_message("Can't split on a straight line!");
		free(new_object1);
		free(new_object2);
		return;
	}
	vector_subtract(shift, prism.base_points[max_dist_point],
			prism.base_points[0]);
	this_dist = shift[0]*center_o[0]+shift[1]*center_o[1]+shift[2]*center_o[2];
	for (k=0; k<3; k++)
		center_n[k] = shift[k]-center_o[k]*this_dist;
	this_dist = 1/sqrt(center_n[0]*center_n[0]+
		center_n[1]*center_n[1]+center_n[2]*center_n[2]);
	for (k=0; k<3; k++)
		center_n[k] *= this_dist;
	/* center_n is now a unit vector normal to the base of the desired prism.*/
	for (k=0; k<3; k++)
		prism.vector[k] = deep*2*center_n[k];
	for (j=1; j<prism.n-5; j++)
	{
		vector_subtract(shift, prism.base_points[j],
			prism.base_points[0]);
		this_dist =
			shift[0]*center_n[0]+shift[1]*center_n[1]+shift[2]*center_n[2];
		for (k=0; k<3; k++)
			prism.base_points[j][k]-= this_dist*center_n[k]+.5*prism.vector[k];
	}
	for (k=0; k<3; k++)
	{
		prism.base_points[prism.n-4][k] =
			prism.base_points[prism.n-5][k]+deep*center_o[k];
		prism.base_points[prism.n-1][k] =
			prism.base_points[0][k]-deep*center_o[k];
		prism.base_points[prism.n-3][k] =
			prism.base_points[prism.n-4][k]+
			deep*(center_n[(k+2)%3]*center_o[(k+1)%3]-
			center_n[(k+1)%3]*center_o[(k+2)%3]);
		prism.base_points[prism.n-2][k] =
			prism.base_points[prism.n-1][k]+
			deep*(center_n[(k+2)%3]*center_o[(k+1)%3]-
			center_n[(k+1)%3]*center_o[(k+2)%3]);
	}
	prism.clockwise = prism.vector[2]<0;

	/* Subsample */
	for (j=3,k=1; j<prism.n-4; j+=3,k++)
		memcpy(prism.base_points[k], prism.base_points[j], sizeof(triple));
	memcpy(prism.base_points[k],prism.base_points[prism.n-4],4*sizeof(triple));
	prism.n = k+4;

	compute_z_extrema(&prism);
	if (do_cut(old_object_from, icon_flag=FALSE, 2, new_object_from,
			new_object_to, NULL, 0, 0, TRUE, 0., None, NULL, &prism)
			!= DONE)
	{	free(new_object1);
		free(new_object2);
		free(prism.base_points);
		return;
	}

	if (icons_exist)
	{
		/* Change prism to icon voxel coordinates. */
		for (j=0; j<prism.n; j++)
		{
			prism.base_points[j][0] *=
				old_object_from->main_data.file->file_header.str.xysize[0];
			prism.base_points[j][1] *=
				old_object_from->main_data.file->file_header.str.xysize[1];
			prism.base_points[j][2] *=
				Slice_spacing(&old_object_from->main_data);
			prism.base_points[j][0] -=
				.5*(Max_coordinate(&old_object_from->main_data, 0)+
				Min_coordinate(&old_object_from->main_data, 0));
			prism.base_points[j][0] +=
				.5*(Max_coordinate(&old_object_from->icon_data, 0)+
				Min_coordinate(&old_object_from->icon_data, 0));
			prism.base_points[j][1] -=
				.5*(Max_coordinate(&old_object_from->main_data, 1)-
				Min_coordinate(&old_object_from->main_data, 1));
			prism.base_points[j][1] +=
				.5*(Max_coordinate(&old_object_from->icon_data, 1)-
				Min_coordinate(&old_object_from->icon_data, 1));
			prism.base_points[j][2] -=
				.5*(Max_coordinate(&old_object_from->main_data, 2)-
				Min_coordinate(&old_object_from->main_data, 2));
			prism.base_points[j][2] +=
				.5*(Max_coordinate(&old_object_from->icon_data, 2)-
				Min_coordinate(&old_object_from->icon_data, 2));
			prism.base_points[j][0] /=
				old_object_from->icon_data.file->file_header.str.xysize[0];
			prism.base_points[j][1] /=
				old_object_from->icon_data.file->file_header.str.xysize[1];
			prism.base_points[j][2] /=
				Slice_spacing(&old_object_from->icon_data);
		}
		prism.vector[0] *=
			old_object_from->main_data.file->file_header.str.xysize[0];
		prism.vector[1] *=
			old_object_from->main_data.file->file_header.str.xysize[1];
		prism.vector[2] *= Slice_spacing(&old_object_from->main_data);
		prism.vector[0] /=
			old_object_from->icon_data.file->file_header.str.xysize[0];
		prism.vector[1] /=
			old_object_from->icon_data.file->file_header.str.xysize[1];
		prism.vector[2] /= Slice_spacing(&old_object_from->icon_data);

		if (do_cut(old_object_from, icon_flag=TRUE, 2, new_object_from,
				new_object_to, NULL, 0, 0, TRUE, 0., None, NULL, &prism)
				!= DONE)
		{	destroy_object_data(&new_object1->main_data);
			free(new_object1);
			destroy_object_data(&new_object2->main_data);
			free(new_object2);
			free(prism.base_points);
			return;
		}
	}
	free(prism.base_points);
	/* combine new_object_to with old old_object_to */
	if (old_object_to)
	{	unit = unit_mm(&old_object_to->main_data);
		for (j=0; j<3; j++)
		{	center_o[j]= .5*unit*(Min_coordinate(&old_object_to->main_data, j)+
				Max_coordinate(&old_object_to->main_data, j));
			center_on[j]= .5*unit*(Min_coordinate(&new_object_to->main_data,j)+
				Max_coordinate(&new_object_to->main_data, j));
		}
		if (combine_object_data(&new_object_to->main_data,
				&old_object_to->main_data)!=DONE ||
				icons_exist && combine_object_data(&new_object_to->icon_data,
				&old_object_to->icon_data)!=DONE)
		{	destroy_object(new_object1);
			destroy_object(new_object2);
			return;
		}
		for (j=0; j<3; j++)
			center_n[j]= .5*unit*(Min_coordinate(&new_object_to->main_data, j)+
				Max_coordinate(&new_object_to->main_data, j));
		old_object_to->O.on = FALSE;
		if (&old_object_to->O == object_from_label(selected_object))
			display_error(VChangePanelItem("STATUS", "STATUS", SWITCH_OFF));
		old_object_too = old_object_to;
	}
	else
		old_object_too = old_object_from;
	/* add new cut objects to tail of object_list */
	for (tail= &object_list; *tail; tail= &(*tail)->next)
		;
	*tail = new_object1;
	new_object1->next = new_object2;
	separate_piece1 = new_object1;
	separate_piece2 = new_object2;
	new_object_from->angle[0] = old_object_from->angle[0];
	new_object_from->angle[1] = old_object_from->angle[1];
	new_object_from->angle[2] = old_object_from->angle[2];
	new_object_from->displacement[0] = old_object_from->displacement[0];
	new_object_from->displacement[1] = old_object_from->displacement[1];
	new_object_from->displacement[2] = old_object_from->displacement[2];
	new_object_from->plan_angle[0] = old_object_from->plan_angle[0];
	new_object_from->plan_angle[1] = old_object_from->plan_angle[1];
	new_object_from->plan_angle[2] = old_object_from->plan_angle[2];
	new_object_from->plan_displacement[0] =
		old_object_from->plan_displacement[0];
	new_object_from->plan_displacement[1] =
		old_object_from->plan_displacement[1];
	new_object_from->plan_displacement[2] =
		old_object_from->plan_displacement[2];
	new_object_from->mobile = old_object_from->mobile;
	new_object_from->O.on = TRUE;
	new_object_from->O.opacity = old_object_from->O.opacity;
	new_object_from->O.specular_fraction= old_object_from->O.specular_fraction;
	new_object_from->O.specular_exponent= old_object_from->O.specular_exponent;
	new_object_from->O.diffuse_exponent = old_object_from->O.diffuse_exponent;
	new_object_from->O.specular_n = old_object_from->O.specular_n;
	new_object_from->O.diffuse_n = old_object_from->O.diffuse_n;
	new_object_from->O.rgb = old_object_from->O.rgb;
	new_object_from->O.color = old_object_from->O.color;
	new_object_to->angle[0] = old_object_too->angle[0];
	new_object_to->angle[1] = old_object_too->angle[1];
	new_object_to->angle[2] = old_object_too->angle[2];
	new_object_to->displacement[0] = old_object_too->displacement[0];
	new_object_to->displacement[1] = old_object_too->displacement[1];
	new_object_to->displacement[2] = old_object_too->displacement[2];
	new_object_to->plan_angle[0] = old_object_too->plan_angle[0];
	new_object_to->plan_angle[1] = old_object_too->plan_angle[1];
	new_object_to->plan_angle[2] = old_object_too->plan_angle[2];
	new_object_to->plan_displacement[0] = old_object_too->plan_displacement[0];
	new_object_to->plan_displacement[1] = old_object_too->plan_displacement[1];
	new_object_to->plan_displacement[2] = old_object_too->plan_displacement[2];
	new_object_to->mobile = old_object_too->mobile;
	new_object_to->O.on = TRUE;
	new_object_to->O.opacity = old_object_too->O.opacity;
	new_object_to->O.specular_fraction = old_object_too->O.specular_fraction;
	new_object_to->O.specular_exponent = old_object_too->O.specular_exponent;
	new_object_to->O.diffuse_exponent = old_object_too->O.diffuse_exponent;
	new_object_to->O.specular_n = old_object_too->O.specular_n;
	new_object_to->O.diffuse_n = old_object_too->O.diffuse_n;
	new_object_to->O.rgb = old_object_too->O.rgb;
	new_object_to->O.color = old_object_too->O.color;
	if (old_object_to)
	{	AtoM(rotation, new_object_to->angle[0], new_object_to->angle[1],
			new_object_to->angle[2]);
		vector_subtract(shift, center_n, center_o);
		matrix_vector_multiply(shift, rotation, shift);
		vector_add(new_object_to->displacement, new_object_to->displacement,
			shift);
		AtoM(rotation, new_object_to->plan_angle[0],
			new_object_to->plan_angle[1], new_object_to->plan_angle[2]);
		vector_subtract(shift, center_n, center_o);
		matrix_vector_multiply(shift, rotation, shift);
		vector_add(new_object_to->plan_displacement,
			new_object_to->plan_displacement, shift);
		compute_diameter(new_object_to);
	}
	if (old_object_too==old_object_to && old_object_to!=original_object1 &&
			old_object_to!=original_object2)
		remove_object(old_object_to);
	if (old_object_from!=original_object1 && old_object_from!=original_object2)
		remove_object(old_object_from);
	else
	{	old_object_from->O.on = FALSE;
		if (&old_object_from->O == object_from_label(selected_object))
			display_error(VChangePanelItem("STATUS", "STATUS", SWITCH_OFF));
	}
	compactify_object(new_object_from);
	compactify_object(new_object_to);
	set_panel_switches();
	reset_common_switches();
}
#endif

/*****************************************************************************
 * FUNCTION: combine_marks
 * DESCRIPTION: Puts marks that are on an object onto another object in
 *    addition to the marks already there.
 * PARAMETERS:
 *    new_object: The object to receive additional marks.
 *    old_object: The object to get additional marks from.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: The global variables argv0, windows_open must be
 *    appropriately initialized.
 * RETURN VALUE: DONE or EROR.
 * EXIT CONDITIONS: Returns EROR memory allocation fails and entry condition
 *    is met.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 9/26/96 new_object->mark_array_size set by Dewey Odhner
 *
 *****************************************************************************/
Function_status cvRenderer::combine_marks(Shell *new_object, Shell *old_object)
{
	triple *new_mark;

	if (old_object->marks == 0)
		return (DONE);
	new_mark = (triple *)
	(	new_object->marks
		?	realloc(new_object->mark,
				(new_object->marks+old_object->marks)*sizeof(triple))
		:	malloc(old_object->marks*sizeof(triple))
	);
	if (new_mark == NULL)
	{	report_malloc_error();
		return (EROR);
	}
	new_object->mark = new_mark;
	new_object->mark_array_size = new_object->marks+old_object->marks;
	memcpy(new_object->mark+new_object->marks, old_object->mark,
		old_object->marks*sizeof(triple));
	new_object->marks += old_object->marks;
	return (DONE);
}

#if 0
/*****************************************************************************
 * FUNCTION: display_button_action
 * DESCRIPTION: Display the button action messages for the state of the
 *    Separate command.
 * PARAMETERS:
 *    command_state, iw_state, rot_state: The current command state, state
 *       associated with the image window, and rotational state under the
 *       Separate command.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable current_window should indicate the
 *    window in which the pointer is.  A successful call to VCreateColormap
 *    must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Errors are ignored.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/12/93 to use ROTATE button by Dewey Odhner
 *    Modified: 6/14/94 to use set_button_action by Dewey Odhner
 *    Modified: 7/18/95 rot state 7 added by Dewey Odhner
 *
 *****************************************************************************/
static void display_button_action(command_state, iw_state, rot_state)
	Command_state command_state;
	int iw_state, rot_state;
{
	switch (command_state)
	{	case SCROLL:
			set_button_action("GRAB", "RELEASE", "DONE", "", "", "DONE");
			break;
		case NORMAL:
			switch (iw_state)
			{	case 0:
					set_button_action("TRACE\nCURVE", "SELECT\nOBJECT",
						"DONE", "SELECT", "SELECT", "UPDATE");
					break;
				case 1:
					set_button_action("START\nTRACE", "START\nTRACE",
						"DONE", "SELECT", "SELECT", "UPDATE");
					break;
				case 3:
					set_button_action("SELECT\nSIDE", "SELECT\nSIDE",
						"DONE", "SELECT", "SELECT", "UPDATE");
					break;
				case 4:
					set_button_action("ENTIRE", "USE\nSCALE", "DONE",
						"SELECT", "SELECT", "UPDATE");
					break;
			}
			break;
		case ROTATE:
			switch (rot_state)
				{	case 1:
					set_button_action("ROTATE\n[X]/Y/Z",
						"RELEASE", "DONE", "", "", "DONE");
					break;
				case 2:
					set_button_action("ROTATE\nX/[Y]/Z",
						"RELEASE", "DONE", "", "", "DONE");
					break;
				case 3:
					set_button_action("ROTATE\nX/Y/[Z]",
						"RELEASE", "DONE", "", "", "DONE");
					break;
				case 4:
				case 5:
				case 6:
					set_button_action("ROTATE", "RESET", "DONE",
						"", "", "DONE");
					break;
				case 7:
					set_button_action("AXIAL", "FRONTAL", "LATERAL",
						"", "", "");
					break;
			}
			break;
	}
}
#endif

/*****************************************************************************
 * FUNCTION: unseparate
 * DESCRIPTION: Undoes the separation.
 * PARAMETERS: None
 * SIDE EFFECTS: The following global variables may be changed: object_list,
 *    image_valid, image2_valid, icon_valid, colormap_valid,
 *    overlay_clear, ncolors, object_color_table, selected_object.
 *    The static variables separate_cmds, separate_cmds2, original_object1,
 *    original_object2, original_objects_set may be changed.
 * ENTRY CONDITIONS: A call to manip_init must be made first.  The static
 *    variables separate_cmds or separate_cmds2, original_object1,
 *    original_object2 must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 7/28/99 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::unseparate()
{
	if (separate_piece1 != original_object1)
	{	assert(separate_piece1 != NULL);
		if (original_object2!=NULL || original_object1!=NULL)
		{	remove_object(separate_piece1);
			separate_piece1 = original_object1;
			image_valid = FALSE;
		}
		if (original_object1 != NULL)
			original_object1->O.on = TRUE;
	}
	if (separate_piece2 != original_object2)
	{	assert(separate_piece2 != NULL);
		if (original_object2!=NULL || original_object1!=NULL)
		{	remove_object(separate_piece2);
			separate_piece2 = original_object2;
			image_valid = FALSE;
		}
		if (original_object2 != NULL)
			original_object2->O.on = TRUE;
	}
}






/*****************************************************************************
 * FUNCTION: write_plan
 * DESCRIPTION: Creates files containing the objects that are on
 *    and a plan file with their orientations.
 * PARAMETERS:
 *    plan_name: The file name of the plan to be written.
 *    objects_on: The number of objects turned on.
 * SIDE EFFECTS: An error message may be displayed.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL, and must have its 'main_data.file' and
 *    'icon_data.file' point to valid Shell_file structures.)
 *    All objects that are on must be compatible.
 *    A successful call to VCreateColormap must be made first.  The global
 *    variables argv0, windows_open, dialog_window must be appropriately
 *    initialized.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: If an error occurs, an error message will be displayed.
 * HISTORY:
 *    Created: 9/30/93 by Dewey Odhner
 *    Modified: 10/20/93 to handle units by Dewey Odhner
 *    Modified: 5/10/94 for fuzzy shells by Dewey Odhner
 *    Modified: 9/11/96 displacement in scanner coordinate system corrected
 *       by Dewey Odhner
 *    Modified: 2/16/01 icons_exist checked before using icon by Dewey Odhner
 *    Modified: 9/2/03 BS2 plan written by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::write_plan(const char plan_name[], int objects_on)
{
	Shell *obj;
	int error_code, objectn, j;
	char *shell_name;
	FILE *planfile;
	triple displacement, rotation[3], orientation[3], origin;
	float *domain;
	double unit;

	shell_name = (char *)malloc(strlen(plan_name)+5);
	strcpy(shell_name, plan_name);
	strcpy(shell_name+strlen(plan_name)-3, st_cl(&object_list->main_data)==
		BINARY_B||st_cl(&object_list->main_data)==BINARY_A? "BS0":
		st_cl(&object_list->main_data)==T_SHELL? "BS2": "SH0");
	if (!ok_to_write(shell_name))
	{
		free(shell_name);
		return (401);
	}
	iw_valid = FALSE;
	error_code = output_plan_data(shell_name);
	if (error_code)
	{
		free(shell_name);
		return (error_code);
	}
	if (icons_exist)
	{
		shell_name[strlen(shell_name)-1] = 'I';
		error_code = output_plan_data(shell_name);
		shell_name[strlen(shell_name)-1] =
			st_cl(&object_list->main_data)==T_SHELL? '2': '0';
		if (error_code)
		{
			unlink(shell_name);
			free(shell_name);
			return (error_code);
		}
	}
	planfile = fopen(plan_name, "wb");
	if (planfile == NULL)
	{
		unlink(shell_name);
		shell_name[strlen(shell_name)-1] = 'I';
		unlink(shell_name);
		free(shell_name);
		return (error_code);
	}
	wxFileName fname(shell_name);
	if (fprintf(planfile,
	"%s\n\nBBOX {\n%.9E %.9E\n%.9E %.9E\n%.9E %.9E\n}\n\nTREE 2 {\n2 %d ID\n",
			(const char *)fname.GetFullName().c_str(),
			-glob_displacement[0]-main_image->width/2/scale,
			-glob_displacement[0]+main_image->width/2/scale,
			-glob_displacement[1]-main_image->width/2/scale,
			-glob_displacement[1]+main_image->width/2/scale,
			-glob_displacement[2]-main_image->width/2/scale,
			-glob_displacement[2]+main_image->width/2/scale, objects_on) <
			112+(int)fname.GetFullName().Len())
	{
		unlink(shell_name);
		shell_name[strlen(shell_name)-1] = 'I';
		unlink(shell_name);
		fclose(planfile);
		unlink(plan_name);
		free(shell_name);
		return (3);
	}
	objectn = 0;
	for (obj=object_list; obj; obj=obj->next)
		if (obj->O.on)
		{	if (fprintf(planfile, "1 %d\n\t{\n", objectn) < 7)
			{
				unlink(shell_name);
				shell_name[strlen(shell_name)-1] = 'I';
				unlink(shell_name);
				fclose(planfile);
				unlink(plan_name);
				free(shell_name);
				return (3);
			}
			AtoM(rotation, obj->angle[0], obj->angle[1], obj->angle[2]);
			unit = unit_mm(&obj->main_data);
			displacement[0] = -.5*unit*(Min_coordinate(&obj->main_data, 0)+
				Max_coordinate(&obj->main_data, 0));
			displacement[1] = -.5*unit*(Min_coordinate(&obj->main_data, 1)+
				Max_coordinate(&obj->main_data, 1));
			displacement[2] = -.5*unit*(Min_coordinate(&obj->main_data, 2)+
				Max_coordinate(&obj->main_data, 2));
			matrix_vector_multiply(displacement, rotation, displacement);
			vector_add(displacement, displacement, obj->displacement);
			vector_subtract(displacement, displacement, glob_displacement);
			if (obj->main_data.file->file_header.str.domain_valid)
			{	domain = obj->main_data.file->file_header.str.domain+
					12*obj->main_data.shell_number;
				origin[0] = unit*domain[0];
				origin[1] = unit*domain[1];
				origin[2] = unit*domain[2];
				orientation[0][0] = domain[3];
				orientation[0][1] = domain[4];
				orientation[0][2] = domain[5];
				orientation[1][0] = domain[6];
				orientation[1][1] = domain[7];
				orientation[1][2] = domain[8];
				orientation[2][0] = domain[9];
				orientation[2][1] = domain[10];
				orientation[2][2] = domain[11];
				matrix_multiply(rotation, rotation, orientation);
				matrix_vector_multiply(origin, rotation, origin);
				vector_subtract(displacement, displacement, origin);
			}
			for (j=0; j<3; j++)
				if (fprintf(planfile, "\t %.9E %.9E %.9E %.9E\n",
						rotation[j][0], rotation[j][1], rotation[j][2],
						displacement[j]) < 37)
				{
					unlink(shell_name);
					shell_name[strlen(shell_name)-1] = 'I';
					unlink(shell_name);
					fclose(planfile);
					unlink(plan_name);
					free(shell_name);
					return (3);
				}
			if (fprintf(planfile, "\t 0.0 0.0 0.0 1.0\n\t}\n") < 21)
			{
				unlink(shell_name);
				shell_name[strlen(shell_name)-1] = 'I';
				unlink(shell_name);
				fclose(planfile);
				unlink(plan_name);
				free(shell_name);
				return (3);
			}
			objectn++;
		}
	fprintf(planfile, "}\n");
	fclose(planfile);
	free(shell_name);
	return (0);
}

/*****************************************************************************
 * FUNCTION: incompatible
 * DESCRIPTION: Determines whether two sets of SHELL0 data are incompatible
 *    for storing together in a file.
 * PARAMETERS:
 *    shell_data1, shell_data2: The data to compare
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE: true if the shells are incompatible.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/19/93 by Dewey Odhner
 *
 *****************************************************************************/
bool incompatible(Shell_data *shell_data1, Shell_data *shell_data2)
{
	int j;

	if (shell_data1->file->file_header.gen.data_type !=
			shell_data2->file->file_header.gen.data_type ||
			shell_data1->file->file_header.str.num_of_samples[0] !=
			shell_data2->file->file_header.str.num_of_samples[0])
		return (true);
	for (j=0; j<shell_data1->file->file_header.str.num_of_samples[0]; j++)
		if (shell_data1->file->file_header.str.loc_of_samples[j] !=
				shell_data2->file->file_header.str.loc_of_samples[j])
			return (true);
	if (shell_data1->file->file_header.str.num_of_elements_valid !=
			 shell_data2->file->file_header.str.num_of_elements_valid)
		return (true);
	if (shell_data1->file->file_header.str.num_of_elements_valid &&
			shell_data1->file->file_header.str.num_of_elements !=
			shell_data2->file->file_header.str.num_of_elements)
		return (true);
	if (shell_data1->file->file_header.str.xysize[0]!=
			shell_data2->file->file_header.str.xysize[0] ||
			shell_data1->file->file_header.str.xysize[1]!=
			shell_data2->file->file_header.str.xysize[1])
		return (true);
	if (shell_data1->file->file_header.str.description_of_element &&
			shell_data2->file->file_header.str.description_of_element)
		for (j=0; j<shell_data1->file->file_header.str.num_of_elements; j++)
			if (shell_data1->file->file_header.str.description_of_element[j] !=
				  shell_data2->file->file_header.str.description_of_element[j])
				return (true);
	return (false);
}

/*****************************************************************************
 * FUNCTION: output_plan_data
 * DESCRIPTION: Creates a file containing a structure system from the
 *    objects that are on.
 * PARAMETERS:
 *    filename: The output file name; if it ends with 'I', icon data will be
 *       stored rather than main data.  Must be a non-empty string.
 * SIDE EFFECTS: If error 106 occurs in VWriteHeader or an object is not
 *    in memory, a message is displayed.
 * ENTRY CONDITIONS: All the objects that are on must be compatible
 *    The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL, and must have its 'main_data.file' and
 *    'icon_data.file' point to valid Shell_file structures.)
 *    A successful call to VCreateColormap must be made first.
 *    The global variables argv0, windows_open, dialog_window,
 *    number_of_triangles must be appropriately initialized.
 * RETURN VALUE: Non-zero if an error occurs other than 106
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 12/7/92 by Dewey Odhner
 *    Modified: 1/11/93 to handle parameter vectors by Dewey Odhner
 *    Modified: 1/15/93 to handle new structure system protocol by Dewey Odhner
 *    Modified: 4/16/93 for new structure system protocol by Dewey Odhner
 *    Modified: 5/19/94 for fuzzy shells by Dewey Odhner
 *    Modified: 11/24/99 fclose call after VCloseData removed by Dewey Odhner
 *    Modified: 10/19/00 items_written changed to int by Dewey Odhner
 *    Modified: 2/16/01 icons_exist checked before using icon by Dewey Odhner
 *    Modified: 9/3/03 for T-shells by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::output_plan_data(const char filename[])
{
	int tree_size, j, k, error_code, shell_number, num_of_NTSE, num_of_shells,
		icon_flag, binary;
	short *tree_data;
	unsigned short *this_ptr;
	FILE *shell_file;
	int items_written;
	char bad_group[5], bad_element[5];
	Shell *obj, *first_obj;
	Shell_data *obj_data;
	ViewnixHeader file_header;
	unsigned char *c_ptr;

	num_of_shells = 0;
	first_obj = NULL;
	for (obj=object_list; obj; obj=obj->next)
		if (obj->O.on)
		{	if (first_obj == NULL)
				first_obj = obj;
			num_of_shells++;
			if (!obj->main_data.in_memory ||
					(icons_exist && !obj->icon_data.in_memory))
				return (1);
		}
	if (first_obj == NULL)
		return (0);
	shell_file = fopen(filename, "wb+");
	if (shell_file == NULL)
		return (4);
	icon_flag = filename[strlen(filename)-1]=='I';
	obj_data = icon_flag? &first_obj->icon_data: &first_obj->main_data;
	binary = st_cl(obj_data)==BINARY_B||st_cl(obj_data)==BINARY_A;
	if (st_cl(obj_data) == T_SHELL)
		binary = 2;
	error_code = copy_shell_header(&file_header, &obj_data->file->file_header,
		num_of_shells);
	if (error_code)
	{	destroy_file_header(&file_header);
		fclose(shell_file);
		return (error_code);
	}
	wxFileName fname(filename);
	strncpy(file_header.gen.filename,(const char *)fname.GetFullName().c_str(),
		sizeof(file_header.gen.filename)-1);
	file_header.gen.filename[sizeof(file_header.gen.filename)-1] = 0;
	file_header.gen.filename_valid = TRUE;
	file_header.str.volume_valid = TRUE;
	file_header.str.surface_area_valid = TRUE;
	file_header.str.rate_of_change_volume_valid = TRUE;
	shell_number = 0;
	for (obj=object_list; obj; obj=obj->next)
		if (obj->O.on)
		{	obj_data = icon_flag? &obj->icon_data: &obj->main_data;
			if (file_header.str.num_of_elements_valid &&
					file_header.str.num_of_elements>0)
				memcpy(file_header.str.parameter_vectors+
					shell_number*file_header.str.num_of_elements,
					obj_data->file->file_header.str.parameter_vectors+
					file_header.str.num_of_elements*
					obj->main_data.shell_number,
					file_header.str.num_of_elements*sizeof(float));
			file_header.str.num_of_TSE[shell_number] =
				obj_data->file->file_header.str.num_of_TSE[
				obj->main_data.shell_number];
			file_header.str.num_of_NTSE[shell_number] =
				obj_data->file->file_header.str.num_of_NTSE[
				obj->main_data.shell_number];
			file_header.str.smallest_value[
				file_header.str.num_of_components_in_TSE*shell_number+1] =
					Smallest_y1(obj_data);
			file_header.str.largest_value[
				file_header.str.num_of_components_in_TSE*shell_number+1] =
					Largest_y1(obj_data);
			memcpy(file_header.str.min_max_coordinates+6*shell_number,
				obj_data->file->file_header.str.min_max_coordinates+
				6*obj->main_data.shell_number, 6*sizeof(float));
			memcpy(file_header.str.domain+12*shell_number,
				obj_data->file->file_header.str.domain+
				12*obj->main_data.shell_number, 12*sizeof(float));
			if (file_header.str.scene_file)
			{
				if (obj_data->file->file_header.str.scene_file_valid)
					strcpy(file_header.str.scene_file[shell_number],
						obj_data->file->file_header.str.scene_file[
						obj->main_data.shell_number]);
				else
				{	free(file_header.str.scene_file);
					file_header.str.scene_file = NULL;
				}
			}
			if (obj_data->volume_valid)
				file_header.str.volume[shell_number] =
					obj_data->file->file_header.str.volume[
					obj->main_data.shell_number];
			else
				file_header.str.volume_valid = FALSE;
			if (obj_data->file->file_header.str.surface_area_valid)
				file_header.str.surface_area[shell_number] =
					obj_data->file->file_header.str.surface_area[
					obj->main_data.shell_number];
			else
				file_header.str.surface_area_valid = FALSE;
			if (obj_data->file->file_header.str.rate_of_change_volume_valid)
				file_header.str.rate_of_change_volume[shell_number] =
					obj_data->file->file_header.str.rate_of_change_volume[
					obj->main_data.shell_number];
			else
				file_header.str.rate_of_change_volume_valid = FALSE;
			shell_number++;
		}
	file_header.str.scene_file_valid = file_header.str.scene_file!=NULL;
	file_header.str.num_of_structures = num_of_shells;
	file_header.str.num_of_structures_valid = 1;
	file_header.str.domain_valid = 1;
	file_header.str.num_of_TSE_valid = 1;
	file_header.str.num_of_NTSE_valid = 1;
	file_header.str.smallest_value_valid = 1;
	file_header.str.largest_value_valid = 1;
	file_header.str.min_max_coordinates_valid = 1;
	file_header.str.surface_area_valid = 0;
	file_header.str.rate_of_change_volume_valid = 0;
	error_code= VWriteHeader(shell_file, &file_header, bad_group, bad_element);
	switch (error_code)
	{	case 0:
		case 107:
		case 106:
			break;
		default:
			fprintf(stderr, "Group %s element %s undefined in VWriteHeader\n",
				bad_group, bad_element);
			destroy_file_header(&file_header);
			fclose(shell_file);
			unlink(filename);
			return (error_code);
	}
	error_code = VSeekData(shell_file, 0);
	if (error_code)
	{	destroy_file_header(&file_header);
		fclose(shell_file);
		unlink(filename);
		return (error_code);
	}
	for (obj=object_list; obj; obj=obj->next)
		if (obj->O.on)
		{	obj_data = icon_flag? &obj->icon_data: &obj->main_data;
			num_of_NTSE = 1+obj_data->slices*(1+obj_data->rows);
			if (binary == 2)
			{
			  tree_data = (short *)malloc(2*num_of_NTSE);
			  if (tree_data == NULL)
			  {	destroy_file_header(&file_header);
				fclose(shell_file);
				unlink(filename);
				return (1);
			  }
			  tree_data[0] = obj_data->slices;
			  for (j=1; j<=obj_data->slices; j++)
				tree_data[j] = obj_data->rows;
			  for (j=0; j<obj_data->rows*obj_data->slices; j++)
			  {
			    for (c_ptr=(unsigned char *)obj_data->ptr_table[j],k=0;
					c_ptr<(unsigned char *)obj_data->ptr_table[j+1];
					c_ptr+=3+3*number_of_triangles[*c_ptr])
				  k++;
				tree_data[1+obj_data->slices+j] = k;
			  }
			  error_code = VWriteData((char *)tree_data, 2, num_of_NTSE,
				shell_file, &items_written);
			  tree_size =
				((char *)obj_data->ptr_table[obj_data->rows*obj_data->slices]-
				(char *)obj_data->ptr_table[0]);
			  if (error_code == 0)
			    error_code = VWriteData((char *)obj_data->ptr_table[0], 1,
				  tree_size, shell_file, &items_written);
			}
			else
			{
			  tree_size = num_of_NTSE+
				(obj_data->ptr_table[obj_data->rows*obj_data->slices]-
				obj_data->ptr_table[0]);
			  tree_data = (short *)malloc(tree_size*2);
			  if (tree_data == NULL)
			  {	destroy_file_header(&file_header);
				fclose(shell_file);
				unlink(filename);
				return (1);
			  }
			  tree_data[0] = obj_data->slices;
			  for (j=1; j<=obj_data->slices; j++)
				tree_data[j] = obj_data->rows;
			  for (j=0; j<obj_data->rows*obj_data->slices; j++)
				tree_data[1+obj_data->slices+j] =
					(obj_data->ptr_table[j+1]-obj_data->ptr_table[j])/
						(binary? 2:3);
			  for (this_ptr=obj_data->ptr_table[0]; this_ptr<
					obj_data->ptr_table[obj_data->rows*obj_data->slices];
					this_ptr+=(binary? 2:3))
			  {	tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])] =
					this_ptr[0];
				if (obj_data->file->file_header.str.bit_fields_in_TSE[6] != 23)
					tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])+1]=
						this_ptr[1];
				else /* map normal codes for obsolete icon data */
				{	int n1, n2, n3;

					n1 = this_ptr[1]>>8;
					n2 = this_ptr[1]>>5 & 7;
					n3 = this_ptr[1]>>1 & 7;
					tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])+1]=
						n1<<6 | n2<<3 | n3;
				}
				if (!binary)
					tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])+2]=
						this_ptr[2];
			  }
			  error_code =
				VWriteData((char *)tree_data, 2, tree_size, shell_file,
					&items_written);
			}
			if (error_code==0 && items_written!=tree_size)
				error_code = 3;
			if (error_code)
			{	free(tree_data);
				fclose(shell_file);
				unlink(filename);
				destroy_file_header(&file_header);
				return (error_code);
			}
			free(tree_data);
		}
	error_code = VCloseData(shell_file);
	if (error_code)
		unlink(filename);
	destroy_file_header(&file_header);
	return (error_code);
}
