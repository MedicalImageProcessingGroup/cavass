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

 
 



Patch *get_patch();
Function_status project(), render_reflection(), make_image(), show_icons(),
	show_objects(), do_cut();
XEvent manip_next_event();
Manip_event *manip_peek_event();
void compute_shade_lut(), update_background(), update_ambient();
double buffer_scale(), Atan2(), fraction_of_dw_available(), distance(),
	voxel_spacing(), angle();
int *get_angle_shades(), output_plan();
Virtual_object *object_from_number(), *object_from_label();
Shell *actual_object();
Priority ignore(), ignore_unless_abort(), image_motion_priority();
int VReadData(), VReadHeader(), VGetHeaderLength(), VWriteHeader(),
	VSeekData(), VCloseData();
