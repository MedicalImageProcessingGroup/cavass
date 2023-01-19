/*
  Copyright 1993-2008 Medical Image Processing Group
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

 
 



#include <math.h>

#include <Viewnix.h>
#include "assert.h"

#define COLOR_TABLE_COLUMNS (PLANE_INDEX_OFFSET*2)
#define SHADE_SCALE_FACTOR \
	(MAX_ANGLE_SHADE*(Z_BUFFER_LEVELS/(OBJECT_IMAGE_BACKGROUND+1)))
#define MIDDLE_DEPTH (Z_BUFFER_LEVELS/2-0.5)
#define SWITCH_OFF 0
#define SWITCH_ON 1
#define SWITCH_FULL 0
#define SWITCH_HALF 1
#define PI 3.14159265358979323846
/*  #define rint(x) floor(.5+(x))    */

#define Center_x (display_area_x+image_x+main_image->width*.5)
#define Center_y (display_area_y+image_y+main_image->height*.5)

typedef enum Priority {IGNORE, FIRST, SECOND} Priority;

typedef enum Display_mode {BAD, ONE_TO_ONE, PIXEL_REPLICATE, ANTI_ALIAS, ICON
	} Display_mode;

typedef enum Scale_set {NONE, COLOR, DIFFUSE, SPECULAR, BCKGND, AMBIENT,
	MAGNIFN, SPEED, OPACITY} Scale_set;

typedef enum Command_state {NORMAL, SCROLL, SET_MARKS, ERASE_MARKS,
	PREVIEW, ROTATE} Command_state;

typedef enum Color_mode {OVERLAY_MODE, PLANE_MODE, MOVIE_MODE} Color_mode;

typedef enum Image_mode {WITH_ICON, SEPARATE, SLICE} Image_mode;

typedef struct Manip_event {
	Priority priority;
	XEvent xevent;
} Manip_event;

typedef struct RGB {
	unsigned short red, green, blue;
} RGB;

typedef struct Object_image {
	Display_mode projected;
	char **image;
	int image_size; /* Images will be square. */
	int pixel_bytes;
	int image_location[2] /* pixels (before anti-aliasing or pixel replica-
		tion) of top left of this image from center of global image */;
	int **z_buffer;
	unsigned char **opacity_buffer, **likelihood_buffer;
} Object_image;

typedef struct Virtual_object {
	int on, opaque;
	Object_image main_image, icon;
	double specular_fraction, specular_exponent, diffuse_exponent,
		specular_n, diffuse_n;
	RGB rgb;
	int color;
	int shade_lut[SHADE_LUT_SIZE]; /* Entry i in shade_lut represents the shade
		(0 = dark, MAX_ANGLE_SHADE = bright) for angle
		acos(2*i/(SHADE_LUT_SIZE-1)-1) of normal from viewing direction. */
	int shade_lut_computed;
} Virtual_object;

typedef struct Shell_data {
	int in_memory, rows, slices, shell_number;
	unsigned short **ptr_table;
	struct Shell_file *file;
} Shell_data;

typedef struct Shell_file {
	struct Shell_data **reference;
	int references;
	ViewnixHeader file_header;
} Shell_file;

#define Largest_y1(shell_data) \
	((shell_data)->file->file_header.str.largest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number])

#define Smallest_y1(shell_data) \
	((shell_data)->file->file_header.str.smallest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number])

#define Min_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.str.min_max_coordinates[(coord)+ \
	6*(shell_data)->shell_number])

#define Max_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.str.min_max_coordinates[(coord)+3+ \
	6*(shell_data)->shell_number])

#define Slice_spacing(shell_data) \
	(((shell_data)->file->file_header.str.loc_of_samples[ \
	(shell_data)->file->file_header.str.num_of_samples[0]-1]- \
	(shell_data)->file->file_header.str.loc_of_samples[0])/ \
	((shell_data)->file->file_header.str.num_of_samples[0]-1))

typedef double triple[3];

typedef struct Shell {
	struct Shell *next;
	Shell_data main_data, icon_data;
	triple angle, displacement, plan_angle, plan_displacement;
	/* Displacements are mm of center of object from center of main image
		right and down, and from center of depth range away from viewer.
		Transformation is of points in structure coordinate system to
		plan coordinate system.  If the object is not loaded from a plan,
		plan_angle and plan_displacement take the object back to the
		scanner coordinate system. */

	int mobile;
	double diameter; /* mm */
	int marks, mark_array_size;
	triple *mark;
	Virtual_object O, *reflection;
} Shell;

typedef enum Function_status {
	ERROR= -1,
	DONE,
	INTERRUPT1, /* usually left button press */
	INTERRUPT2, /* usually middle button press */
	INTERRUPT3 /* usually right button press */
} Function_status;

typedef struct Patch {
	short top, bottom;
	struct Patch_line {short left, right;} lines[1];
} Patch;

struct Rect {
	short position[2], size[2], nobjects, nopaque_objects, plane;
	struct Rect *next;
	struct Rect_object {
		Virtual_object *vobj;
		Object_image *img;
	} object[1];
};

typedef union Long_bytes {
	long l;
	unsigned char c[sizeof(long)];
} Long_bytes, Color_table_row[COLOR_TABLE_COLUMNS];

#include "functions.h"
#include "glob_vars.h"
