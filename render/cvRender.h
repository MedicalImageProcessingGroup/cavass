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

#ifndef __cvRender_h
#define __cvRender_h
 
#include <math.h>
#include "Viewnix.h"
#include "cv3dv.h"
#include <assert.h>
#include "3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/neighbors.h"
#include "patch.h"

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

class cvRenderer;

typedef enum Display_mode {BAD, ONE_TO_ONE, PIXEL_REPLICATE, ANTI_ALIAS, ICON
	} Display_mode;

typedef enum Priority {IGNOR, FIRST, SECOND} Priority;

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
	int bounds[3][2], threshold[6]; /* for scene data */
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
	int t_shell_detail; /* 1, 3, or 7; level of discretization */
	Priority (*check_event)(cvRenderer *); /* Function to check for interrupt,
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
#define T_SHELL_DETAIL_FLAG 0x80000

typedef enum Classification_type {BINARY_A, GRADIENT, PERCENT, BINARY_B,
	DIRECT, T_SHELL} Classification_type;

typedef unsigned short Pixel_unit; /* Must be big enough to hold the value
	V_GRAY_INDEX_OFFSET-1 */

#define Largest_y1(shell_data) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	(shell_data)->file->file_header.str.largest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number]: (shell_data)->bounds[0][1])

#define Smallest_y1(shell_data) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	(shell_data)->file->file_header.str.smallest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number]: (shell_data)->bounds[0][0])

#define Set_largest_y1(shell_data, v) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	((shell_data)->file->file_header.str.largest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number]=(float)(v)): \
	((shell_data)->bounds[0][1]=(int)(v)))

#define Set_smallest_y1(shell_data, v) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	((shell_data)->file->file_header.str.smallest_value[1+ \
	(shell_data)->file->file_header.str.num_of_components_in_TSE* \
	(shell_data)->shell_number]=(float)(v)): \
	((shell_data)->bounds[0][0]=(int)(v)))

#define Min_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	(shell_data)->file->file_header.str.min_max_coordinates[(coord)+ \
	6*(shell_data)->shell_number]: coord<2? (shell_data)->bounds[coord][0]* \
	(shell_data)->file->file_header.scn.xypixsz[coord]: \
	scn_slice_location(shell_data, (shell_data)->bounds[2][0]))

#define Max_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	(shell_data)->file->file_header.str.min_max_coordinates[(coord)+3+ \
	6*(shell_data)->shell_number]: coord<2? (shell_data)->bounds[coord][1]* \
	(shell_data)->file->file_header.scn.xypixsz[coord]: \
	scn_slice_location(shell_data, (shell_data)->bounds[2][1]))

#define Min_str_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.str.min_max_coordinates[(coord)+ \
	6*(shell_data)->shell_number])

#define Max_str_coordinate(shell_data, coord) \
	((shell_data)->file->file_header.str.min_max_coordinates[(coord)+3+ \
	6*(shell_data)->shell_number])

#define Slice_spacing(shell_data) \
	((shell_data)->file->file_header.gen.data_type!=IMAGE0? \
	((shell_data)->file->file_header.str.loc_of_samples[ \
	(shell_data)->file->file_header.str.num_of_samples[0]-1]- \
	(shell_data)->file->file_header.str.loc_of_samples[0])/ \
	((shell_data)->file->file_header.str.num_of_samples[0]-1): \
	(scn_slice_location(shell_data, (shell_data)->slices-1)- \
	scn_slice_location(shell_data, 0))/((shell_data)->slices-1))

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
	bool original;
} Shell;

typedef struct Manip_event {
	Priority priority;
	XEvent xevent;
} Manip_event;

typedef enum Function_status {
	EROR= -1,
	DONE,
	INTERRUPT1, /* usually left button press */
	INTERRUPT2, /* usually middle button press */
	INTERRUPT3 /* usually right button press */
} Function_status;

struct Rect_object {
	Virtual_object *vobj;
	Object_image *img;
};

struct Rect {
	short position[2], size[2], nobjects, nopaque_objects, plane;
	struct Rect *next;
	struct Rect_object object[1];
};

typedef union {
	int l;
	unsigned char c[sizeof(long)];
} Int_bytes, Color_table_row[COLOR_TABLE_COLUMNS];

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
	int inverted[2];
	struct Slice_image *next;
} Slice_image;

typedef enum Scale_set {NONE, COLOR, DIFFUSE, SPECULAR, BACKGROUND, AMBIENT,
	MAGNIFY, SPEED, OPACITY, SLICING, SRF_STRNTH, PERSPECTIVE, ZOOM} Scale_set;

typedef enum Command_state {NORMAL, SCROLL, SET_MARKS, ERASE,
	PREVIEW, ROTATE, OUTPUT_PAR} Command_state;

typedef enum Color_mode {OVERLAY_MODE, PLANE_MODE, MOVIE_MODE} Color_mode;

typedef enum Image_mode {WITH_ICON, SEPARATE, SLICE, FUZZY_CONNECT, CLOSEUP,
	WITHOUT_ICON, PROBE, SECTION} Image_mode;

struct Prism
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

typedef struct Plan_tree {
	int level, children, obj_index;
	struct Plan_tree *child;
	double matrix[4][4];
} Plan_tree;

typedef struct voi_struct
{	short x;
	unsigned short start_code, end_code;
} voi_struct;

typedef struct S_voxel_struct
{	short x;
	unsigned short neighbor_code;
} S_voxel_struct;

typedef double dpair[2];

typedef struct SCENE_INFO {
  int slices;
  unsigned long ptr;   /* position of the begining
                          of scene data set (header length) */
  char file_name[80];   /* File name                    */
  ViewnixHeader vh;     /* File header                  */
  FILE *fp;             /* File pointer                 */
  int width,height,     /* slice dimentions             */
      pix_size;         /* bits per pixel               */
  unsigned long min,max; /* cell values */
  unsigned long bytes_per_slice;
  unsigned char *byte_data;
  unsigned short *dbyte_data;
} Scene_Info;

typedef double POINT3D[3];

int get_patch(Patch **patch, double projection_matrix[3][3]);
int get_margin_patch(Patch **patch, double *top_margin, double *bottom_margin,
	double *left_margin, double *right_margin,
	double projection_matrix[3][3], double max_depth);
void trim_patch(Patch *patch);
int get_triangle_patch(Patch **patch, double projection_matrix[3][3],
	const int edge_vertices[13][2], const int triangle_edges[189][3],
	const int T_x[9], const int T_y[9], const int T_z[9], int v, int new_view,
	int tweak);
double unit_mm(Shell_data *shell_data);
double unit_size(int code);
void VComputeShadeLut(int shade_lut[SHADE_LUT_SIZE], double diffuse_exponent,
	double diffuse_n, double specular_fraction, double specular_exponent,
	double specular_n);
void VGetAngleShades(int **angle_shade, double rotation_matrix[3][3],
	Rendering_parameters *rend_params, int flags,
	Classification_type object_class, int show_back);
float scn_slice_location(Shell_data *, int);
Classification_type st_cl(Shell_data *);
void manipulate_abort(const char caller[],const char called[], int error_code);
void report_malloc_error(void);
Int_bytes intbytes(int colorcell);
double voxel_spacing(Shell_data *);
const char *unit_label(Shell_data *shell_data);
double object_volume(Shell_data *shell_data);
void matrix_multiply(double dest[3][3], double mleft[3][3],
	double mright[3][3]);
void matrix_vector_multiply(double dest[3], double matrix[3][3],
	double vector[3]);
void vector_matrix_multiply(double dest[3], double vector[3],
	double matrix[3][3]);
void vector_add(double dest[3], double vector1[3], double vector2[3]);
void vector_subtract(double dest[3], double vector1[3], double vector2[3]);
void axis_rot(double dest[3][3], int axis, double angle);
void transpose(double dest[3][3], double src[3][3]);
int VInvertMatrix(double Ainv[], double A[], int N);
void destroy_object_data(Shell_data *object_data);
void destroy_file_header(ViewnixHeader *file_header);
float scn_slice_location(Shell_data *shell_data, int slice);
int get_string(char string[], FILE *file, int string_size);
Plan_tree *branch_pointer(Plan_tree *root, int branch[], int level);
void breadth_next(Plan_tree *root, int branch[], int *level);
int free_children(Plan_tree *node);
void MtoA(double *A1, double *A2, double *A3, double M[3][3]);
void view_interpolate(double between[3], double pose1[3], double pose2[3],
	double partway);
void AtoM(double M[3][3], double A1, double A2, double A3);
void AtoV(double V[3], double A1, double A2);
void VtoA(double *A1, double *A2, double V[3]);

#if 0
char *salloc(unsigned long size);
void sfree(void *ptr);
#else
#define salloc(size) malloc(size)
#define sfree(ptr) free(ptr)
#endif

void destroy_object_image(Object_image *object_image);
void display_error ( int error_code );
int need_patch(double projection_matrix[3][3]);
int put_new_data(Shell_data *object2, size_t *&new_ptr_table,
	unsigned short *out_data, int &voxels_in_out_buffer);
void compactify_object(Shell *obj, int icons_exist);
void compactify_object_data(Shell_data *obj_data);
int is_this_side(triple point, triple plane_norml, double plane_disp);
void compute_diameter(Shell *obj, int icons_exist=0);
bool incompatible(Shell_data *shell_data1, Shell_data *shell_data2);

class cvRenderer {
public:
	cvRenderer(char **file_list, int num_files, int icons=FALSE);
	~cvRenderer(void);

	cvRenderer*  mAux;  ///< optional, extra renderer for stereo
	void setStereoTransform ( void );

	Shell *object_list; /* linked list of all the objects (shells) */
	Slice_image *slice_list; /* linked list of all the displayed slices */
	Shell *separate_piece1, *separate_piece2;
	int anti_alias; /* flag to indicate anti-aliasing is to be done on the
		main image; if FALSE pixel replication is done. */
	int maximum_intensity_projection; /* flag to indicate volume rendering
		is to be done by maximum intensity projection */
	int box; /* flag to indicate the box is to be shown */
	int plane; /* flag to indicate the cutting plane or mirror is
		to be shown */
	int line; /* flag to indicate the axis of motion is to be shown */
	Image_mode image_mode;
		/* WITH_ICON -- all objects that are on are to be
			shown in main image and icon image;
		   SEPARATE -- all but separate_piece2 in
			main_image and all but separate_piece1 in image2;
		   SLICE -- object that is
			on is to be shown in main image and slice through it in image2. */
	int image_valid; /* flag to indicate the main image is up to date */
	int image2_valid; /* flag to indicate image2 is up to date */
	int image3_valid; /* flag to indicate image3 is up to date */
	int image4_valid; /* flag to indicate image4 is up to date */
	int icon_valid; /* flag to indicate up to date icon has been displayed*/
	int iw_valid; /* flag to indicate the image window is up to date */
	int colormap_valid; /* flag to indicate the colormap is up to date */
	int marks; /* flag to indicate the marks are to be shown */
	int ncolors; /* the number of different colors of objects */
	int gray_scale; /* flag to indicate a section of the colormap is
		to be used for gray values */
	Color_table_row *object_color_table; /* lookup table that maps individual
		object image buffer pixel values to display colorcell values */
	V_color_table_row *v_object_color_table; /* lookup table that maps
		individual object image buffer pixel values to display colorcell
		values for volume rendering */
	Color_table_row *slice_color_table; /* lookup table that maps individual
		slice image buffer pixel values to display colorcell values */
	double scale; /* scale of the main image in pixel/mm */
	double depth_scale; /* scale of the z-buffers in units per mm */
	double icon_scale; /* scale of the icon image in pixel/mm */
	double glob_angle[3]; /* the angles of transformation from plan space
		to image space:
		glob_angle[0] -- the angle in radians between z-axis and rotation axis;
		glob_angle[1] -- the angle in radians of rotation axis projection on
			x-y plane;
		glob_angle[2] -- the rotation angle in radians. */
	double glob_displacement[3]; /* displacement of plan origin from center
	    of image space */
	RGB ambient; /* ambient light on a scale of 0 to 65535 */
	double plane_normal[3]; /* the normal vector of the cutting plane or
		mirror in plan space */
	double plane_displacement; /* displacement in millimeters of the
		cutting plane or mirror along plane_normal from center of plan space */
	double line_angle[2]; /* the direction of the axis of motion
		in plan space */
	double line_displacement[3]; /* the displacement of the axis of motion
		from the origin in plan space */
	int selected_object; /* the currently selected object label */
	int old_selected_object;
	RGB background; /* image background color */
	RGB plane_transparency; /* transparency to each color component on a
		scale of 0 to 65535 of the cutting plane or mirror */
	XImage *main_image, *icon_image, *image2, *image3, *image4;
	ViewnixColor mark_color; /* color of the marks */
	int fade_edge; /* flag to indicate voxel edges are to be faded to
		compensate for the crosshatch (partial area) effect */
	float surface_red_factor, surface_green_factor, surface_blue_factor;
		/* color of surface, 0 to MAX_SURFACE_FACTOR */
	float tissue_opacity[4]; /* opacity of each material, 0 to 1 */
	float tissue_red[4], tissue_green[4], tissue_blue[4]; /* color of each
		material, 0 to V_OBJECT_IMAGE_BACKGROUND-1 */
	int true_color; /* flag to indicate true color mode is being used */
	float surface_strength; /* brightness of surface in percentage
		rendering, 0 to 100 */
	float global_level, global_width; /* global intensity windowing,
		0 to 100; not valid when truecolor is on. */
	int gray_interpolate; /* flag to indicate linear interpolation is to be
		done for slice displays */
	int label_slice; /* flag to indicate slice orientation is to be shown */
	float emission_power; /* power of material percentage multiplying
		emission, >= 1 */
	float surf_pct_power; /* power of material percentage multiplying
		reflection, 0 to 1 */
	float perspective; /* amount of perspective (0 to 100.) */
	Shell *axes;
	float out_slice_spacing; /* mm */
	Object_closeup *closeup_list;
	double closeup_displacement[3], closeup_angle[3]; /* the transformation
		from main view to closeup view */
	double closeup_scale; /* scale of the closeup images in pixel/mm */
	int inside_closeup; /* flag to indicate closeups show inside view */
	float viewport_size; /* relative to the maximum */
	int icons_exist;
	int viewport_back; /* 0 to Z_BUFFER_LEVELS from back of view space */
	double glob_plane_displacement[3]; /* displacement in millimeters of the
		slicing plane from center of plan space */
	int slice_section_flag;
	Virtual_object *object_of_point;
	int t_shell_detail; /* 1, 3, or 7; level of discretization */

	unsigned char red_map[256], green_map[256], blue_map[256];
	int V_num_overlays;
	XImage ximage, ximage2, xicon;
	bool error_flag;
	int cut_count;
	Slice_image null_slice;

	// from project.cpp:
	float one_minus_opacity_times[4][256], red_times_square_of[4][256],
		green_times_square_of[4][256], blue_times_square_of[4][256],
		sstrnth_table[4][3][256], sstrnth_ambient_table[4][3][256];
	unsigned short *slice_buffer; /* a buffer to hold one slice of shell
		data when the shell data are not in memory; must be set only by
		load_this_slice */
	int gray_flag;
	RGB ambient_light;
	int fade_flag, mip_flag, clip_flag;
	float surf_red_factr, surf_green_factr, surf_blue_factr;
	float materl_opacity[4];
	float materl_red[4], materl_green[4], materl_blue[4];
	float surf_strenth, emis_power, surf_ppower;
	float prspectiv;
	float surf_pct_table[256]; /* pow(i, surf_ppower) */
	float surf_opac_table[256]; /* pow(sstrenth, 1-surf_ppower) */
	Pixel_unit mip_lut[256]; /* lookup table to map opacity to intensity */
	int detail; /* level of t-shell discretization */
	int slice_buffer_size;
	unsigned short *static_slice_buffer;
	Color_mode color_mode;
	float old_materl_opacity[4],
		old_materl_red[4], old_materl_green[4], old_materl_blue[4],
		old_emis_power, old_surf_ppower, old_surf_strenth,
		old_surf_red_factr, old_surf_green_factr, old_surf_blue_factr;
	RGB old_ambient_light;

	// from do_cut.c
	unsigned short *out_data, *buffer_out_ptr,
		inside_plane_G_code, outside_plane_G_code, **S_ptr_table;
	double column_n_factor, slice_n_factor, row_n_factor,
		object_plane_distance;
	size_t *new_ptr_table;
	int clmns, rws, slcs,
		inside, out_buffer_size, voxels_in_out_buffer;
	voi_struct *adjacent_vois;
	S_voxel_struct *S_row;
	Classification_type data_class;

	// from poly_cut.c
	unsigned short *p_o_data, *p_buff_out_ptr, *buffer_row_ptr;
	size_t last_out_ptr, *this_ptr_ptr, *end_ptr_ptr;
	int p_o_buffer_size, p_voxs_in_out_buffer, this_slice, this_row,
		slice_points;
	dpair *slice_point_list;
	struct Prism mPrism;
	int min_slice, max_slice, min_row, max_row;
		/* Slice and row values start at 0. */

	// from move.c
	int points_selected;
	double selected_points[2][3];

	Virtual_object *original_object;
	Shell *original_object1, *original_object2;
	int original_objects_set;
	ViewnixColor *color_table;
	int shades_per_color;
	Scene_Info scene[2];
	double        ( *measurement_point )[3];
	int              nmeasurement_points;
	int              measurement_point_array_size;
	bool             new_mark_is_set;
	bool             mark_is_erased;
	bool             measurement_displayed;
	double           erased_mark[3];
	Shell           *marked_object /* the last object marked */;
	X_Point          box_axis_label_loc[3]; /* Label text is at
                   object_list->main_data.file->file_header.str.axis_label[] */

	void inside_insert_adjacent_vois(int offset, int row_code,
		unsigned short **S_ptr_ptr, voi_struct **adjacent_vois_end);
	void outside_insert_adjacent_vois(int offset, int row_code,
		unsigned short **S_ptr_ptr, voi_struct **adjacent_vois_end);
	int copy_shell_header(ViewnixHeader *newp, ViewnixHeader *oldp,
		int new_num_of_structures);
	Function_status do_cut(Shell *primary_object, int icon_flag, int op_type,
		Shell *out_object1, Shell *out_object2, X_Point points[], int npoints,
		int z, int inside_curve, double depth, Window draw_window,
		unsigned short **s_ptr_table, struct Prism *prismp);
	int cut_marks(Shell *in_object, Shell *out_object, int inside_curve,
		X_Point points[], int npoints, int z,double depth, Window draw_window);
	int mark_is_in_curve(Shell *object, int this_mark, X_Point points[],
		int npoints, int z, double depth, Window draw_window);
	int plane_cut_marks(Shell *in_object, Shell *out_object);
	void plane_setup(Shell *primary_object, Shell_data *primary_object_data);
	Function_status cut_rows(Shell_data *primary_object_data, int cut_is_plane,
		int restrict_normal);
	void get_plane_voi_list();
	Function_status cut_rows();

	int VRender(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Rendering_parameters *rend_params,
		int flags);
	Function_status show_icons(Priority (*event_priority)(cvRenderer *));
	int main_image_ok(Virtual_object *vobj);
	Function_status show_objects(Priority (*event_priority)(cvRenderer *));
	int project(Shell *object, Shell_data *object_data,
		Priority (*event_priority)(cvRenderer *));
	int loadFile(const char filename[]);
	void unloadFiles(void);
	static Priority cvCheckInterrupt ( cvRenderer * );
	XImage& render ( int *interrupt_flg );
	XImage& render ( void );
	char* get_recolored_image( int& w, int& h );
	char* get_recolored_image2( int& w, int& h );
	char* render2 ( int& w, int& h, int& interrupt_flag );
	void setScale(double sc);
	double getScale(void);
	void Antialias(void);
	void PixelReplicate(void);
	int setObjectColor(unsigned char red, unsigned char green,
		unsigned char blue, int n);
	int getObjectCount(void);
	bool getAntialias ( void );
	void setAntialias ( bool state );
	void getObjectColor(unsigned char & red, unsigned char & green,
		unsigned char & blue, int n);
	double getObjectSpecularFraction(int n);
	void setObjectSpecularFraction(double v, int n);
	double getObjectSpecularExponent(int n);
	void setObjectSpecularExponent(double v, int n);
	double getObjectSpecularDivisor(int n);
	void setObjectSpecularDivisor(double v, int n);
	double getObjectDiffuseExponent(int n);
	void setObjectDiffuseExponent(double v, int n);
	double getObjectDiffuseDivisor(int n);
	void setObjectDiffuseDivisor(double v, int n);
	void setAllSpecularFraction(double v);
	void setAllSpecularExponent(double v);
	void setAllSpecularDivisor(double v);
	void setAllDiffuseExponent(double v);
	void setAllDiffuseDivisor(double v);
	double getObjectOpacity(int n);
	void setObjectOpacity(double v, int n);
	double getMaterialOpacity(int n);
	void setMaterialOpacity(double v, int n);
	void setMIP(bool state);
	bool getMIP(void);
	void getBackgroundColor(unsigned char & red, unsigned char & green,
		unsigned char & blue);
	void setBackgroundColor(unsigned char red, unsigned char green,
		unsigned char blue);
	void getAmbientLight(unsigned char & red, unsigned char & green,
		unsigned char & blue);
	void setAmbientLight(unsigned char red, unsigned char green,
		unsigned char blue);
	int object_number(Virtual_object *vobj);
	int number_of_objects(void);
	void check_true_color(void);
	void set_principal_view(Principal_plane view_type);
	void invalidate_images(void);
	void invalidate_main_images(void);
	void set_color_number(Virtual_object *vobj);
	int closest_object(int x, int y, int image_number);
	RGB *color_of_number(int colorn);
	int set_direct_colormap(void);
	void V_PutColormap(ViewnixColor *color_table, int nentries);
	void compute_v_object_color_table(void);
	void destroy_virtual_object(Virtual_object *object);
	void destroy_object(Shell *object);
	int load_file(Shell **last_shell, int icon_flag, char file_name[],
		int from_plan, int num_of_objects, int shell_numbers[]);
	void do_curved_cut(X_Point *vertices, int nvertices, int inside,
		float depth);
	int get_curve_z(int *z, X_Point *points, int npoints, XImage *img,
		Object_image *ob_im);
	void set_original_objects();
	Function_status combine_object_data(Shell_data *object_data1,
		Shell_data *object_data2);
	void do_separation(X_Point *vertices, int nvertices, int inside,
		Window draw_window, float depth);
	void do_split(void (*set_panel_switches)());
	Function_status combine_marks(Shell *new_object, Shell *old_object);
	void unseparate();
	int write_plan(const char plan_name[], int objects_on);
	int output_plan_data(const char filename[]);
	void do_plane_slice(void);
	bool objects_need_be_turned_off(void);
	void destroy_slice_list(void);
	void set_principal_plane(Principal_plane plane_type);
	void do_slice_output(const char out_file_name[], double first_loc,
		double last_loc, double out_pixel_size, bool matched_output);
	Function_status get_object_roi_stats(double *total_density,
		double *mean_density, double *standard_deviation, double *min,
		double *max, Priority (*event_priority)(cvRenderer *));
	void do_plane_cut();
	void draw_box(XImage *image);
	void label_axis(double head[3], double tail[3], XImage *image,
		X_Point *label_loc);
	void draw_edge(double p1[3], double p2[3], double farthest_corner[3],
		XImage *image);
	void draw_far_line(int x1, int y1, int x2, int y2, XImage *image);
	void draw_line_segment(int x1, int y1, int x2, int y2, XImage *image);

protected:
	int no_interrupt, *interrupt_flag;
	char **file_list;
	int file_list_size;
	char *old_scene[2];
	Slice_image *last_slice;
	unsigned short *old_sl_data[2];
	int old_sl_width[2], old_sl_height[2];
	POINT3D old_ends[2][3];
	int old_interpolate[2];

	void slice_line(triple a, triple b);
	void slice_rect(triple vector, triple a, triple c);
	void get_slice_points(struct Prism *prism);
	int next_row();
	int get_prism_voi_list(unsigned short **ptr_table, struct Prism *prism,
		int icon_flag, Window draw_window, Shell *primary_object);
	void display_area_to_voxel_coords(double voxel_coords[3], int x, int y,
		int z, Shell *object, Shell_data *obj_data, Window window);
	void redisplay_slices(void);
	void display_slice(Slice_image *sl_image);
	int map_slice(unsigned short *sl_data, Slice_image*sl_image, int sl_scene);
	void get_plane_ends(Shell *object, POINT3D ends[4], int sl_width,
		int sl_height, SceneInfo *scn, double first_loc, double last_loc,
		double sl_normal[3], double x_axis[3], double y_axis[3],
		Principal_plane *plane_type);
	int load_scene(Shell *object, char scene_file_name[], FILE **fp,
		ViewnixHeader *vh, int *slices, int *width, int *height,
		unsigned long *hdrlen, int *bits, unsigned long *min,
		unsigned long *max, unsigned long *bytes_per_slice,
		unsigned char **byte_data, unsigned short **dbyte_data);
	void destroy_scene_header(ViewnixHeader *vh);
	void destroy_scene(ViewnixHeader *vh, FILE **fp, unsigned char **byte_data,
		unsigned short **dbyte_data);
	Function_status get_oblique_slice(unsigned short **sl_data, int sl_width,
		int sl_height, POINT3D ends[3], int interpolate, int sl_scene);

	Function_status pixel_replicate(XImage *image, struct Rect *rect,
		double plane_top_left[], double plane_top_right[],
		double plane_bottom_left[]);
	Function_status antialias(XImage *image, struct Rect *rect,
		double plane_top_left[], double plane_top_right[],
		double plane_bottom_left[], Priority (*event_priority)(cvRenderer *));
	Function_status icon(XImage *image, struct Rect *rect,
		double plane_top_left[],
		double plane_top_right[], double plane_bottom_left[]);
	Function_status display_marks(XImage *image,
		Priority (*event_priority)(cvRenderer *));
	void display_mark(Shell *object, double mark[3], XImage *image);
	void set_last_slice_plane(void);
	int create_slice_image(int size, int x, int y);
	void destroy_slice_image(void);
	void do_slice_output(void);

	static void count_triangles(void);
	void initialize_times_tables(void);
	int bin_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *));
	int perspec_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int perspec_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int load_this_slice(Shell_data *object_data, int this_slice,
		unsigned short **this_slice_ptr);
	int bbin_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int ts_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int gr_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *));
	int gr_mip_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		Priority (*check_event)(cvRenderer *));
	int pct_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int pct_mip_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int d_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int d_mip_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));

	int patch_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[G_CODES], Priority (*check_event)(cvRenderer *));
	int quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int bperspec_patch_project(Shell_data *object_data,
		Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int bperspec_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int bpatch_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int bquick_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int gr_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int gr_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	void gr_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024], int itop_margin,
		int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image);
	void gr_mip_patch_one_voxel(int this_row_z, int this_row_x,
		int this_row_y, int column_x_table[1024], int column_y_table[1024],
		int column_z_table[1024], int itop_margin, int ibottom_margin,
		int ileft_margin,  int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image);
	int pct_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int pct_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	void pct_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024], int itop_margin,
		int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image);
	void pct_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024],
		unsigned short *this_ptr, Object_image *object_image);
	void pct_mip_patch_one_voxel(int this_row_z, int this_row_x,
		int this_row_y, int column_x_table[1024], int column_y_table[1024],
		int column_z_table[1024], int itop_margin,  int ibottom_margin,
		int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image);
	void pct_mip_paint_one_voxel(int this_row_z, int this_row_x,
		int this_row_y, int column_x_table[1024], int column_y_table[1024],
		int column_z_table[1024], unsigned short *this_ptr,
		Object_image *object_image);
	int gr_mip_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int gr_mip_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int pct_mip_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int pct_mip_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int clip_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int bclip_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int d_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	int d_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], int angle_shade[G_CODES],
		Priority (*check_event)(cvRenderer *));
	void d_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024], int itop_margin,
		int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image,
		int threshold[6]);
	void d_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024],
		unsigned short *this_ptr, Object_image *object_image,
		int threshold[6]);
	void d_mip_patch_one_voxel(int this_row_z, int this_row_x,
		int this_row_y, int column_x_table[1024], int column_y_table[1024],
		int column_z_table[1024], int itop_margin, int ibottom_margin,
		int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image,
		int threshold[6]);
	void d_mip_paint_one_voxel(int this_row_z, int this_row_x,
		int this_row_y, int column_x_table[1024], int column_y_table[1024],
		int column_z_table[1024], unsigned short *this_ptr,
		Object_image *object_image, int threshold[6]);
	int d_mip_patch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int d_mip_quick_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], Priority (*check_event)(cvRenderer *));
	int tpatch_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int tquick_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	void t_patch_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[1024],
		int column_y_table[1024], int column_z_table[1024], int itop_margin,
		int ibottom_margin, int ileft_margin, int iright_margin, Patch *patch,
		unsigned short *this_ptr, Object_image *object_image);
	void t_paint_one_voxel(int this_row_z, int angle_shade[G_CODES],
		int this_row_x, int this_row_y, int column_x_table[2048],
		int column_y_table[2048], int column_z_table[2048],
		unsigned short *this_ptr, Object_image *object_image);
	int ts_quick_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int ts_patch_project(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));
	int ts_tpatch_project(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3],
		int angle_shade[BG_CODES], Priority (*check_event)(cvRenderer *));

	// from reflect.cpp:
	int patch_project_r(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		double object_plane_normal[3], double object_plane_distance,
		int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *));
	int quick_project_r(Shell_data *object_data, Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		double object_plane_normal[3], double object_plane_distance,
		int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *));
	int perspec_patch_project_r(Shell_data *object_data,
		Object_image *object_image, double projection_matrix[3][3],
		double projection_offset[3], double object_plane_normal[3],
		double object_plane_distance, int angle_shade[G_CODES],
		Priority (*event_priority)(cvRenderer *));
	int perspec_project_r(Shell_data *object_data,
		Object_image *object_image,
		double projection_matrix[3][3], double projection_offset[3],
		double object_plane_normal[3], double object_plane_distance,
		int angle_shade[G_CODES], Priority (*event_priority)(cvRenderer *));
public:
	int render_reflection(Shell *object, Shell_data *object_data,
		Priority (*event_priority)(cvRenderer *));

	void param_init();
	int loadFiles(char **file_list, int input_files, int icons);
	int set_colormap();
	int enable_truecolor(void);
	Function_status make_image(XImage *, Priority (*)(cvRenderer *));
	Function_status show_closeups(Priority (*)(cvRenderer *), XImage *, int*);
	double buffer_scale(Display_mode);
	double closeup_buffer_scale(Display_mode);
	int output_plan(void);
	int output_shell(void);
	Virtual_object *object_from_number(int);
	Virtual_object *object_from_label(int);
	Shell *actual_object(Virtual_object *);
	struct Prism *get_prism(X_Point [], int, double, int, Shell *,
		Shell_data *, Window);
	void resize_image(void);
	void remove_object(Shell *bad_obj);
	int object_label(Virtual_object *vobj);
	void set_depth_scale(void);
	void eliminate_unused_colors(void);
	int get_image_size(void);
	int get_mark_x_y(int *x, int *y, Shell *object, double mark[3],
		XImage *image);
	int number_of_reflections(void);
	int object_of_color(RGB rgb);
	int unused_color_number(void);
	int new_color(void);
	void eliminate_color(int unused_color);
	void get_object_transformation(double rotation[3][3],
		double translation[3], Shell *object, int mirror);
	void get_structure_transformation(double matrix[3][3], double vector[3],
		double rotation_matrix[3][3], double center[3], Shell *object,
		Display_mode mode, int mirror);
	int get_z(int x, int y, int transparent, XImage *image);
	int closest_z(int x, int y, XImage *image);
	int load_shell(Shell_file *shell_file, FILE *infile, int ref_number,
		int shell_number, Shell **last_shell, int icon_flag);
	int load_direct_data(Shell_file *shell_file, FILE *infile, int ref_number,
		int shell_number, Shell **last_shell, int icon_flag);
	int read_plan(char shell_file_name[300], int *num_of_objects,
		int **shell_numbers, triple (**rotations)[3], triple **displacements,
		char plan_file_name[], int read_bb, int *num_of_axes,
		triple (**axis_rotations)[3], triple **axis_displacements,
		int **axis_shell_numbers);
	void get_3d_line(int x, int y, int z, int last_x, int last_y, int last_z,
		X_Point *&xpoints, int &visible_points);
	void get_line(X_Point *&xpoints, int &visible_points);
	void plan_to_display_area_coords(int *x, int *y, int *z,
		double plan_coords[3]);
};

extern int number_of_triangles[255]; /* in each t-shell configuration */

#endif
