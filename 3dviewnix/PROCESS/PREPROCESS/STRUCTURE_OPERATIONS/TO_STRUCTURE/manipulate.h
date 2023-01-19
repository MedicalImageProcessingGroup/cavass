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
#include "Viewnix.h"
#include "assert.h"

#define GRAY_INDEX_OFFSET 128
#define OBJECT_IMAGE_BACKGROUND (GRAY_INDEX_OFFSET-2)
#define MARK_SHADE (GRAY_INDEX_OFFSET-1)
#define PLANE_INDEX_OFFSET (GRAY_INDEX_OFFSET*2)
#define Z_BUFFER_LEVELS 0x100000
#define MAX_ANGLE_SHADE 3840
#define Z_SUBLEVELS ((unsigned)256)
#define SHADE_LUT_SIZE 257
#define COLOR_TABLE_COLUMNS (PLANE_INDEX_OFFSET*2)
#define SHADE_SCALE_FACTOR \
	(MAX_ANGLE_SHADE*(Z_BUFFER_LEVELS/(OBJECT_IMAGE_BACKGROUND-1)))
#define MIDDLE_DEPTH (Z_BUFFER_LEVELS/2-0.5)

#define V_COLOR_TABLE_COLUMNS 65536
#define V_OBJECT_IMAGE_BACKGROUND (V_COLOR_TABLE_COLUMNS-2)
#define V_MARK_SHADE (V_COLOR_TABLE_COLUMNS-1)
#define V_SHADE_SCALE_FACTOR \
	(MAX_ANGLE_SHADE*(Z_BUFFER_LEVELS/(V_OBJECT_IMAGE_BACKGROUND-1)))
#define MAX_SURFACE_FACTOR \
	((V_OBJECT_IMAGE_BACKGROUND-1)/(256.*MAX_ANGLE_SHADE))

#define WINDOW_BORDER_WIDTH 1

#define SWITCH_OFF 0
#define SWITCH_ON 1
#define SWITCH_FULL 0
#define SWITCH_HALF 1
#undef PI
#define PI 3.1415926535897932384626433832795

#define Center_x (display_area_x+image_x+main_image->width*.5)
#define Center_y (display_area_y+image_y+main_image->height*.5)

typedef struct Patch {
	short top, bottom;
	struct Patch_line {
		short left, right;
		unsigned char *weight;
	} lines[1];
} Patch;

typedef enum Display_mode {BAD, ONE_TO_ONE, PIXEL_REPLICATE, ANTI_ALIAS, ICON
	} Display_mode;

typedef enum Priority {IGNORE, FIRST, SECOND} Priority;

typedef struct Object_image {
	Display_mode projected;
	char **image; /* poiter to array of pointers to rows of pixels */
	int image_size; /* Images will be square. */
	int pixel_units; /* number of components in each pixel of image */
	int image_location[2] /* pixels (before anti-aliasing or pixel replica-
		tion) of top left of this image from center of global image */;
	int **z_buffer;
	unsigned char **opacity_buffer, **likelihood_buffer;
} Object_image;

typedef struct Shell_data {
	int in_memory; /* flag that TSE's are in memory */
	int rows, slices; /* the number of rows per slice and slices in the shell*/
	int shell_number; /* which structure in the file, from zero */
	int volume_valid; /* flag that the volume for this structure
		(file->file_header.str.volume[shell_number]) is valid */
	unsigned short **ptr_table; /* Pointer to an array of rows*slices+1
		pointers to the TSE's for each row; if not in_memory, the row pointers
		give file positions from the beginning of the file when cast to int. */
	struct Shell_file *file; /* the file information, even if the shell has
		not been stored in a file */
} Shell_data;

typedef struct Shell_file {
	struct Shell_data **reference; /* list of shells referring to this file */
	int references; /* number of shells referring to this file */
	ViewnixHeader file_header; /* The file header information;
		file_header.gen.filename_valid indicates the file exists. */
} Shell_file;

typedef struct RGB {
	unsigned short red, green, blue;
} RGB;

typedef struct Rendering_parameters {
	float diffuse_exponent; /* The exponent in the diffuse part of the shading
		formula. */
	float diffuse_n; /* The divisor in the diffuse part of the shading formula */
	float specular_fraction; /* The weight applied to the specular part of the
		shading formula. */
	float specular_exponent; /* The exponent in the specular part of the
		shading formula. */
	float specular_n; /* The divisor in the specular part of the shading
		formula. */
	float light_direction[3]; /* A unit vector giving the direction of the
		light source. */
	int *shade_lut; /* Points to a table of SHADE_LUT_SIZE values
		set by VComputeShadeLut. */
	RGB ambient; /* ambient light on a scale of 0 to 65535 */
	int fade_edge; /* flag to indicate voxel edges are to be faded to
		compensate for the crosshatch (partial area) effect */
	float surface_red_factor, surface_green_factor, surface_blue_factor;
		/* color of surface, 0 to MAX_SURFACE_FACTOR */
	float tissue_opacity[4]; /* opacity of each material, 0 to 1 */
	float tissue_red[4], tissue_green[4], tissue_blue[4]; /* color of each
		material, 0 to V_OBJECT_IMAGE_BACKGROUND-1 */
	float surface_strength; /* brightness of surface in percentage
		rendering, 0 to 100 */
	float emission_power; /* power of material percentage multiplying
		emission, >= 1 */
	float surf_pct_power; /* power of material percentage multiplying
		reflection, 0 to 1 */
	float perspective; /* amount of perspective */
	int maximum_intensity_projection; /* flag to indicate volume rendering is
		to be done by maximum intensity projection. */
	Priority (*check_event)(); /* Function to check for interrupt,
		takes no parameters. */
} Rendering_parameters;

#define AMBIENT_FLAG 1
#define FADE_EDGE_FLAG 2
#define SURFACE_RGB_FACTOR_FLAG 4
#define TISSUE_OPACITY_FLAG 8
#define TISSUE_RGB_FLAG 16
#define SURFACE_STRENGTH_FLAG 32
#define CHECK_EVENT_FLAG 64
#define DIFFUSE_EXPONENT_FLAG 128
#define DIFFUSE_N_FLAG 256
#define SPECULAR_FRACTION_FLAG 512
#define SPECULAR_EXPONENT_FLAG 1024
#define SPECULAR_N_FLAG 2048
#define LIGHT_DIRECTION_FLAG 4096
#define SHADE_LUT_FLAG 8192
#define MIP_FLAG 16384
#define EMISSION_POWER_FLAG 32768
#define SURF_PCT_POWER_FLAG 65536
#define PERSPECTIVE_FLAG 0x20000
#define CLIP_FLAG 0x40000

typedef enum Classification_type {BINARY, GRADIENT, PERCENT, BINARY_B, DIRECT}
	Classification_type;

typedef unsigned short Pixel_unit; /* Must be big enough to hold the value
	V_GRAY_INDEX_OFFSET-1 */

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

typedef struct Virtual_object {
	int on;
	float opacity;
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
	int secondary;
	double diameter; /* mm */
	int marks, mark_array_size;
	triple *mark;
	Virtual_object O, *reflection;
} Shell;

typedef struct Manip_event {
	Priority priority;
	XEvent xevent;
} Manip_event;

typedef enum Function_status {
	ERROR= -1,
	DONE,
	INTERRUPT1, /* usually left button press */
	INTERRUPT2, /* usually middle button press */
	INTERRUPT3 /* usually right button press */
} Function_status;

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

typedef unsigned char V_color_table_row[V_COLOR_TABLE_COLUMNS];

typedef enum Principal_plane {AXIAL, CORONAL, SAGITTAL} Principal_plane;

typedef struct Slice_image {
	XImage *image;
	union {unsigned char *c; unsigned short *s;} image_data[2];
	int x, y;
	double plane_normal[3], plane_displacement;
	Principal_plane plane_type;
	double x_axis[3], y_axis[3]; /* vectors of one pixel along slice axes,
		relative to plan space */
	Shell *object[2];
	int on[2];
	float gray_level[2], gray_width[2]; /* current */
	float gray_min[2], gray_max[2]; /* original */
	int gray_window_valid[2];
	int data_valid[2];
	int valid;
	int interpolated;
	struct Slice_image *next;
} Slice_image;

typedef enum Scale_set {NONE, COLOR, DIFFUSE, SPECULAR, BACKGROUND, AMBIENT,
	MAGNIFY, SPEED, OPACITY, SLICING, SRF_STRNTH, PERSPECTIVE, ZOOM} Scale_set;

typedef enum Command_state {NORMAL, SCROLL, SET_MARKS, ERASE,
	PREVIEW, ROTATE, OUTPUT_PAR} Command_state;

typedef enum Color_mode {OVERLAY_MODE, PLANE_MODE, MOVIE_MODE} Color_mode;

typedef enum Image_mode {WITH_ICON, SEPARATE, SLICE, FUZZY_CONNECT, CLOSEUP,
	WITHOUT_ICON, PROBE, SECTION} Image_mode;

struct prism
{	int n; /* number of base vertices */
	triple *base_points /* coordinates of base vertices */,
		vector /* displacement of the other base from the base base */;
	int min_z_point /* index of the vertex with the smallest z coordinate */,
		max_z_point /* index of the vertex with the largest z coordinate */,
		clockwise /* Are the vertices in clockwise order? */;
};

typedef struct Object_closeup {
	Object_image image;
	Virtual_object *object;
	struct Object_closeup *next;
} Object_closeup;

typedef struct Viewpoint {
	double angle[3][3], displacement[3], length;
} Viewpoint;
