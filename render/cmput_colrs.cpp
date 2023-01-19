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

/*****************************************************************************
 * FUNCTION: intbytes
 * DESCRIPTION: Converts an int to Int_bytes.
 * PARAMETERS: colorcell is a pixel value for main_image or one of the same
 *    depth and byte order.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The encoded pixel value.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/5/06 by Dewey Odhner
 *
 *****************************************************************************/
Int_bytes intbytes(int colorcell)
{
	Int_bytes lb;

	lb.l = colorcell;
	return (lb);
}

/*****************************************************************************
 * FUNCTION: unused_color_number
 * DESCRIPTION: Finds the first unused color number less than ncolors.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)
 * RETURN VALUE: The first unused color number less than ncolors if it exists,
 *    -1 otherwise.
 * EXIT CONDITIONS: Undefined if entry condition is not fulfilled
 * HISTORY:
 *    Created: 6/18/92 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::unused_color_number(void)
{
	int n;
	Shell *obj;

	for (n=0; n<ncolors; n++)
	{	for (obj=object_list; obj!=NULL; obj=obj->next)
		{	if (obj->O.color == n)
				goto next_n;
			if (obj->reflection && obj->reflection->color==n)
				goto next_n;
		}
		return (n);
		next_n:;
	}
	return (-1);
}

/*****************************************************************************
 * FUNCTION: new_color
 * DESCRIPTION: Increments the number of object colors and returns a new color
 *    number.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables colormap_valid, image_valid, and
 *    icon_valid will be cleared.  The global variable ncolors will be
 *    incremented.  The memory at the global variables object_color_table,
 *    slice_color_table will be freed and new memory allocated.
 * ENTRY CONDITIONS: The global variables object_color_table,
 *    slice_color_table must point to dynamically allocated memory.
 *    There must be enough memory to allocate the new object_color_table and
 *    slice_color_table.
 *    The global variable ncolors must have a positive value.
 * RETURN VALUE: A new object color number.
 * EXIT CONDITIONS: If a memory allocation error occurs, the program will
 *    crash.  Otherwise undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1991 by Dewey Odhner
 *    Modified: 4/22/93 to invalidate slice_list by Dewey Odhner
 *    Modified: 10/15/93 to handle slice_color_table by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::new_color(void)
{
	colormap_valid = image_valid = image2_valid = icon_valid = FALSE;
	ncolors++;
	if (object_color_table)
		free(object_color_table);
	object_color_table = (Color_table_row *)
		malloc(ncolors*sizeof(Color_table_row));
	if (object_color_table == NULL)
		manipulate_abort("new_color", "malloc", 1);
	if (slice_color_table)
		free(slice_color_table);
	slice_color_table = (Color_table_row *)
		malloc(ncolors*sizeof(Color_table_row));
	if (slice_color_table == NULL)
		manipulate_abort("new_color", "malloc", 1);
	for (Slice_image *slice=slice_list; slice; slice=slice->next)
		slice->valid = FALSE;
	return (ncolors-1);
}

/*****************************************************************************
 * FUNCTION: eliminate_color
 * DESCRIPTION: Reduces the number of object colors by one.
 * PARAMETERS:
 *    unused_color: An unused color number.  It must be at least 0 and less
 *       than global variable ncolors, and must not be the color number of
 *       any Virtual_object in the object_list.
 * SIDE EFFECTS: If global variable color_mode has a value different from
 *    MOVIE_MODE, the global variable ncolors will be decremented, the valid
 *    flags for colormap and images will be cleared, and the number
 *    unused_color may acquire a new meaning.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 6/92 by Dewey Odhner
 *    Modified: 4/22/93 to invalidate slice_list by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::eliminate_color(int unused_color)
{
	Shell *obj;

	if (color_mode == MOVIE_MODE)
		return;
	ncolors--;
	colormap_valid = image_valid = image2_valid = icon_valid = FALSE;
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (obj->O.color == ncolors)
			obj->O.color = unused_color;
		if (obj->reflection && obj->reflection->color==ncolors)
			obj->reflection->color = unused_color;
	}
}

/*****************************************************************************
 * FUNCTION: eliminate_unused_colors
 * DESCRIPTION: Reduces the number of object colors by the number of unused
 *    object colors.
 * PARAMETERS: None
 * SIDE EFFECTS: If global variable color_mode has a value different from
 *    MOVIE_MODE and there are unused object colors, the global variable
 *    ncolors will be decremented, the valid flags for colormap and images
 *    will be cleared, and the number will be cleared, and the object color
 *    numbers may acquire new meanings.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variable ncolors must be
 *    greater than each object color number, which must be at least 0.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 6/92 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::eliminate_unused_colors(void)
{
	int n;

	n = unused_color_number();
	if (n>=0 && ncolors>1)
	{	eliminate_color(n);
		eliminate_unused_colors();
	}
}

/*****************************************************************************
 * FUNCTION: color_of_number
 * DESCRIPTION: Returns the brightest color for an object of color colorn
 * PARAMETERS:
 *    colorn: the color number that you want the color of
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell or be NULL. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)
 * RETURN VALUE: The address of a variable containing the brightest color for
 *    an object of color colorn; if colorn does not represent an object color,
 *    returns black.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1991 by Dewey Odhner
 *    Modified: 8/9/93 to return a pointer by Dewey Odhner
 *
 *****************************************************************************/
RGB *cvRenderer::color_of_number(int colorn)
{
	Shell *obj;
	static RGB black;

	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (obj->O.color == colorn)
			return (&obj->O.rgb);
		if (obj->reflection && obj->reflection->color == colorn)
			return (&obj->reflection->rgb);
	}
	black.red = black.green = black.blue = 0;
	return (&black);
}

/*****************************************************************************
 * FUNCTION: set_colormap
 * DESCRIPTION: Sets the colormap, object_color_table, slice_color_table values
 *    according to the value of the global variables color_mode, true_color.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables colormap_valid, iw_valid, image_valid,
 *    icon_valid, overlay_clear, overlay_bad may be changed.
 * ENTRY CONDITIONS: The global variables object_color_table, slice_color_table
 *    must point to ncolors Color_table_row's of allocated memory.  The global
 *    variable gray_scale must be set if the gray-value portions of the table
 *    are to be initialized.  The global variable object_list must point to a
 *    valid Shell or be NULL. (A valid Shell must have its 'next' point to a
 *    valid Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variable ncolors must be
 *    greater than each object color number, which must be at least 0.
 *    A successful call to VCreateColormap must be made first.  The static
 *    variable from_scale should be FALSE for this function to complete
 *    uninterrupted.
 * RETURN VALUE: 0 if no error occurs.
 * EXIT CONDITIONS: Returns before completing the function, leaving
 *    colormap_valid FALSE, if from_scale is set and pointer motion in the
 *    scale occurs, or if an error occurs.  Undefined if entry conditions
 *    are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 3/5/93 to display status message by Dewey Odhner
 *    Modified: 3/25/94 to handle true color mode by Dewey Odhner
 *    Modified: 5/13/94 to clear display_area2 by Dewey Odhner
 *    Modified: 5/13/97 to bypass 3-3-2 color mode by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::set_colormap(void)
{
	switch (true_color)
	{
		case FALSE:
			break;
		default:
			return (enable_truecolor());
	}
	colormap_valid = FALSE;
	return (set_direct_colormap());
}

/*****************************************************************************
 * FUNCTION: enable_truecolor
 * DESCRIPTION: Turns on the true color option.
 * PARAMETERS:
 * SIDE EFFECTS: The global variables overlay_clear, overlay_bad,
 *    v_object_color_table, colormap_valid, icon_valid may be changed.
 * ENTRY CONDITIONS:
 *    The global variables ncolors,  must be set and slice_color_table
 *    must be allocated. The global variable object_list must point to a valid
 *    Shell or be NULL.
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *
 *****************************************************************************/
int cvRenderer::enable_truecolor(void)
{
	int shaden, oshade;

	if (colormap_valid)
		return (0);
	for (oshade=0; oshade<OBJECT_IMAGE_BACKGROUND; oshade++)
	{	shaden = oshade*256/OBJECT_IMAGE_BACKGROUND;
		object_color_table[0][oshade].c[0] = shaden;
		object_color_table[0][oshade].c[1] = shaden;
		object_color_table[0][oshade].c[2] = shaden;
	}
	object_color_table[0][OBJECT_IMAGE_BACKGROUND].c[0] = background.red/256;
	object_color_table[0][OBJECT_IMAGE_BACKGROUND].c[1]=background.green/256;
	object_color_table[0][OBJECT_IMAGE_BACKGROUND].c[2]= background.blue/256;
	return (0);
}

/*@ 2/1/06 */
int V_GetColorcellStatus(long colorcell, int *status)
{
	if ((colorcell&255) >= 244)
		return 3;
	return (colorcell&1? 2:1);
}

/*@ 2/1/06 */
int V_GetFreeColorcells(long **avail_colorcells, long *num_avail_colorcells)
{
	int j;

	*num_avail_colorcells = 244;
	*avail_colorcells = (long *)malloc(*num_avail_colorcells*sizeof(long));
	if (*avail_colorcells == NULL)
		return 1;
	for (j=0; j<*num_avail_colorcells; j++)
		(*avail_colorcells)[j] = 0x010101*j;
	return 0;
}

/*@ 1/31/06 */
void cvRenderer::V_PutColormap(ViewnixColor *color_table, int nentries)
{
	int j;

	assert(nentries <= 256);
	for (j=0; j<nentries; j++)
	{
		red_map[color_table[j].pixel&255] = color_table[j].red>>8;
		green_map[(color_table[j].pixel>>8)&255] = color_table[j].green>>8;
		blue_map[color_table[j].pixel>>16] = color_table[j].blue>>8;
	}
}

/*****************************************************************************
 * FUNCTION: set_direct_colormap
 * DESCRIPTION: Sets the object_color_table, slice_color_table values for
 *    DirectColor visual class.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variable colormap_valid will be set if the call
 *    is successful.
 * ENTRY CONDITIONS: The global variables object_color_table, slice_color_table
 *    must point to ncolors Color_table_row's of allocated memory.  The global
 *    variable gray_scale must be set if the gray-value portions of the table
 *    are to be initialized.  The global variable object_list must point to a
 *    valid Shell or be NULL. (A valid Shell must have its 'next' point to a
 *    valid Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variable ncolors must be
 *    greater than each object color number, which must be at least 0.
 *    The global variables global_level, global_width must be properly set.
 *    A successful call to VCreateColormap must be made first.  The static
 *    variable from_scale should be FALSE for this function to complete
 *    uninterrupted.
 * RETURN VALUE: 0 if no error occurs.
 * EXIT CONDITIONS: Returns before completing the function, leaving
 *    colormap_valid FALSE, if from_scale is set and pointer motion in the
 *    scale occurs, or if an error occurs.  Undefined if entry conditions
 *    are not fulfilled.
 * HISTORY:
 *    Created: 2/1/93 by Dewey Odhner
 *    Modified: 10/15/93 to handle slice_color_table by Dewey Odhner
 *    Modified: 8/12/94 to use global_level, global_width by Dewey Odhner
 *    Modified: 11/11/94 to use global_level, global_width only for
 *       binary shells by Dewey Odhner
 *    Modified: 7/1/97 num_avail_colorcells made long by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::set_direct_colormap(void)
{
	long *avail_colorcells, num_avail_colorcells;
	int colorn, shaden, oshade, error_code;
	RGB rgb;
	float base, slope, windowed_shade;

	if (color_table == NULL)
	{
		error_code =
			V_GetFreeColorcells(&avail_colorcells,&num_avail_colorcells);
		if (error_code)
			return (error_code);
		shades_per_color = num_avail_colorcells-2;
		if (shades_per_color > OBJECT_IMAGE_BACKGROUND)
			shades_per_color = OBJECT_IMAGE_BACKGROUND;
		color_table = (ViewnixColor *)
			malloc((shades_per_color+2)*sizeof(ViewnixColor));
		if (color_table == NULL)
		{	free(avail_colorcells);
			return(1);
		}
		for (shaden=0; shaden<=shades_per_color+1; shaden++)
			color_table[shaden].pixel = avail_colorcells[shaden];
		free(avail_colorcells);
	}
	if (object_list && (st_cl(&object_list->main_data)==BINARY_B ||
			st_cl(&object_list->main_data)==BINARY_A ||
			st_cl(&object_list->main_data)==T_SHELL))
	{	base = (float)(655.35*(global_level-.5*global_width));
		slope = 100/global_width;
	}
	else
	{	base = 0;
		slope = 1;
	}
	for (shaden=0; shaden<shades_per_color; shaden++)
	{	windowed_shade = (shaden*65535/(shades_per_color-1)-base)*slope;
		if (windowed_shade < 0)
			windowed_shade = 0;
		if (windowed_shade > 65535)
			windowed_shade = 65535;
		color_table[shaden].red =
			color_table[shaden].green =
			color_table[shaden].blue =
			(unsigned short)windowed_shade;
	}
	color_table[shades_per_color].red = background.red;
	color_table[shades_per_color].green = background.green;
	color_table[shades_per_color].blue = background.blue;
	color_table[shades_per_color+1].red =
		(unsigned short)(plane_transparency.red/65535.*background.red);
	color_table[shades_per_color+1].green =
		(unsigned short)(plane_transparency.green/65535.*background.green);
	color_table[shades_per_color+1].blue =
		(unsigned short)(plane_transparency.blue/65535.*background.blue);
	for (colorn=0; colorn<ncolors; colorn++)
	{
		for (oshade=0; oshade<OBJECT_IMAGE_BACKGROUND; oshade++)
		{	shaden = oshade*shades_per_color/OBJECT_IMAGE_BACKGROUND;
			rgb = *color_of_number(colorn);
			object_color_table[colorn][oshade] =
				intbytes(
				(color_table[(int)(rgb.red/65535.*(shaden*(1-ambient.red/65535.)+
				(shades_per_color-1)*(ambient.red/65535.)))].pixel&
				255)|
				(color_table[(int)(rgb.green/65535.*(shaden*(1-ambient.green/65535.)+
				(shades_per_color-1)*(ambient.green/65535.)))].pixel&
				(255<<8))|
				(color_table[(int)(rgb.blue/65535.*(shaden*(1-ambient.blue/65535.)+
				(shades_per_color-1)*(ambient.blue/65535.)))].pixel&
				(255<<16)));
			object_color_table[colorn][oshade+PLANE_INDEX_OFFSET] =
				intbytes(
				(color_table[(int)(rgb.red/(65535*65535.)*(shaden*
				(1-ambient.red/65535.)+(shades_per_color-1)*(ambient.red/65535.))*
				plane_transparency.red)].pixel&255)|
				(color_table[(int)(rgb.green/(65535*65535.)*(shaden*
				(1-ambient.green/65535.)+(shades_per_color-1)*(ambient.green/65535.))*
				plane_transparency.green)].pixel&(255<<8))|
				(color_table[(int)(rgb.blue/(65535*65535.)*(shaden*
				(1-ambient.blue/65535.)+(shades_per_color-1)*(ambient.blue/65535.))*
				plane_transparency.blue)].pixel&(255<<16)));
			slice_color_table[colorn][oshade] =
				intbytes((color_table[(int)(rgb.red/65535.*shaden)].pixel&255)|
				(color_table[(int)(rgb.green/65535.*shaden)].pixel&(255<<8))|
				(color_table[(int)(rgb.blue/65535.*shaden)].pixel&(255<<16)));
			if (gray_scale)
			{	object_color_table[colorn][oshade+GRAY_INDEX_OFFSET] =
					intbytes(color_table[shaden].pixel);
				object_color_table[colorn][oshade+GRAY_INDEX_OFFSET+
					PLANE_INDEX_OFFSET] =
					intbytes(
					(color_table[(int)(shaden*plane_transparency.red/65535.)].
					pixel&255)|(
					color_table[(int)(shaden*plane_transparency.green/65535.)].
					pixel&(255<<8))|
					(color_table[(int)(shaden*plane_transparency.blue/65535.)].
					pixel&(255<<16)));
			}
		}
		object_color_table[colorn][OBJECT_IMAGE_BACKGROUND] =
			intbytes(color_table[shades_per_color].pixel);
		object_color_table[colorn][
			OBJECT_IMAGE_BACKGROUND+PLANE_INDEX_OFFSET] =
			intbytes(color_table[shades_per_color+1].pixel);
		object_color_table[colorn][MARK_SHADE] =
			intbytes((
				color_table[(int)(mark_color.red/65535.*(shades_per_color-1))].
				pixel&255)|(
				color_table[(int)(mark_color.green/65535.*(shades_per_color-1))].
				pixel&(255<<8))|(
				color_table[(int)(mark_color.blue/65535.*(shades_per_color-1))].
				pixel&(255<<16)));
		object_color_table[colorn][MARK_SHADE+PLANE_INDEX_OFFSET] =
			intbytes(
			(color_table[(int)(mark_color.red/(65535*65535.)*
			plane_transparency.red*(shades_per_color-1))].pixel&255)|
			(color_table[(int)(mark_color.green/(65535*65535.)*
			plane_transparency.green*(shades_per_color-1))].pixel&(255<<8))|
			(color_table[(int)(mark_color.blue/(65535*65535.)*
			plane_transparency.blue*(shades_per_color-1))].pixel&(255<<16)));
	}
	V_PutColormap(color_table, shades_per_color+2);
	return (0);
}
