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

 



extern Window image_window, dialog_window, button_window;
extern Window current_window; /* one of the preceding */
extern Window display_area, icon_area, display_area2; /* subwindows of the
	image window. display_area is where the main image is displayed. */
extern Window grab_window; /* input-only window for confining the cursor */
extern Shell *object_list; /* linked list of all the objects (shells) */
extern Shell *separate_piece1, *separate_piece2;
extern Display *display;
extern Visual *visual;
extern int ready_cursor; /* The cursor type for the image window if not busy */
extern int display_area_x, display_area_y, display_area_width,
	display_area_height; /* position & size of display_area */
extern int image_x, image_y; /* position of main image in display_area */
extern int display_area2_x, display_area2_y, display_area2_width,
	display_area2_height; /* position & size of display_area2 */
extern int image2_x, image2_y; /* position of image2 in display_area2 */
extern int icon_area_x, icon_area_y, icon_area_width, icon_area_height;
	/* position & size of icon_area */
extern int icon_image_x,icon_image_y; /* position of icon image in icon_area */
extern int grab_window_right, grab_window_bottom; /* coordinates of the last
	pixel in grab_window */
extern int anti_alias; /* flag to indicate anti-aliasing is to be done on the
	main image; if FALSE pixel replication is done. */
extern int box; /* flag to indicate the box is to be shown */
extern int plane; /* flag to indicate the cutting plane or mirror is
	to be shown */
extern int line; /* flag to indicate the axis of motion is to be shown */
extern Image_mode image_mode; /* WITH_ICON -- all objects that are on are to be
	shown in main image and icon image; SEPARATE -- all but separate_piece2 in
	main_image and all but separate_piece1 in image2; SLICE -- object that is
	on is to be shown in main image and slice through it in image2. */
extern int image_valid; /* flag to indicate the main image is up to date */
extern int image2_valid; /* flag to indicate image2 is up to date */
extern int icon_valid; /* flag to indicate up to date icon has been displayed*/
extern int iw_valid; /* flag to indicate the image window is up to date */
extern int windows_open; /* flag to indicate the windows are mapped */
extern int colormap_valid; /* flag to indicate the colormap is up to date */
extern int overlay_on; /* flag to indicate the overlay is turned on */
extern int overlay_bad; /* 1 -- the overlay needs to be erased on display_area
	and/or display_area2;
	2 -- the overlay needs to be erased on display_area and icon_area */
extern int marks; /* flag to indicate the marks are to be shown */
extern int overlay_clear; /* flag to indicate the overlay 1 bit in
	display_area (and display_area2 if it exists) is clear */
extern Color_mode color_mode;
extern int pixel_bytes; /* the number of bytes per pixel in the images */
extern int image_depth; /* the depth of the images */
extern int ncolors; /* the number of different colors of objects */
extern int gray_scale; /* flag to indicate a section of the colormap is
	to be used for gray values */
extern int wait_time; /* time in milliseconds that we keep starting new icon
	views since the last completed view if new motion events keep coming */
extern int panel_commands; /* the number of panel buttons and switches */
extern PanelCmdInfo *panel_command; /* the array where the current panel
	commands are */
extern Color_table_row *object_color_table; /* lookup table that maps
	individual object image buffer pixel values to display colorcell values */
extern double scale; /* scale of the main image in pixel/mm */
extern double depth_scale; /* scale of the z-buffers in units per mm */
extern double icon_scale; /* scale of the icon image in pixel/mm */
extern double glob_angle[3]; /* the angles of transformation from plan space
	to image space */
extern float speed; /* sensitivity to mouse motion, 1.0 = normal */
extern float gray_level, gray_width;
extern unsigned short ambient; /* ambient light on a scale of 0 to 65535 */
extern double plane_normal[3]; /* the normal vector of the cutting plane or
	mirror in plan space */
extern double plane_displacement; /* displacement in millimeters of the
	cutting plane or mirror along plane_normal from center of plan space */
extern double line_angle[2]; /* the direction of the axis of motion
	in plan space */
extern double line_displacement[3]; /* the displacement of the axis of motion
	from the origin in plan space */
extern int selected_object; /* the currently selected object label */
extern RGB background; /* image background color */
extern RGB plane_transparency; /* transparency to each color component on a
	scale of 0 to 65535 of the cutting plane or mirror */
extern XImage *main_image, *icon_image, *image2;
extern char *argv0; /* the program name */
extern ViewnixColor mark_color; /* color of the marks */
extern char default_name[MAX_DEFAULT_FILES][MAX_DEFAULT_CHAR]; /* default file
	name for VDisplayOutputFilePrompt */
