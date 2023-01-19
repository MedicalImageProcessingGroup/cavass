/*
  Copyright 1993-2009 Medical Image Processing Group
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
 * FUNCTION: get_object_transformation
 * DESCRIPTION: Computes a rotation matrix and translation vector to transform
 *    from the coordinate system aligned with the structure coordinate system
 *    and centered on the object, to the image space coordinate system
 *    (both with physical units).
 * PARAMETERS:
 *    rotation: The rotation matrix to be computed.
 *    translation: The translation vector to be computed.
 *    object: The object
 *    mirror: Non-zero if the transformation is for the reflected object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables object_list, glob_angle,
 *    plane_normal, plane_displacement must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_object_transformation(double rotation[3][3],
	double translation[3], Shell *object, int mirror)
{
	double global_rotation[3][3], mirror_matrix[3][3], normal_translation;
	int j, k;

	AtoM(rotation, object->angle[0], object->angle[1], object->angle[2]);
	memcpy(translation, object->displacement,
		sizeof(object->displacement));
	if (mirror)
	{	normal_translation = translation[0]*plane_normal[0]+
			translation[1]*plane_normal[1]+translation[2]*plane_normal[2];
		for (j=0; j<3; j++)
		{	for (k=0; k<3; k++)
				mirror_matrix[j][k] = -2*plane_normal[j]*plane_normal[k];
			mirror_matrix[j][j] += 1;
			translation[j] +=
				2*plane_normal[j]*(plane_displacement-normal_translation);
		}
		matrix_multiply(rotation, mirror_matrix, rotation);
	}
	AtoM(global_rotation, glob_angle[0], glob_angle[1], glob_angle[2]);
	matrix_multiply(rotation, global_rotation, rotation);
	matrix_vector_multiply(translation, global_rotation, translation);
}

/*****************************************************************************
 * FUNCTION: buffer_scale
 * DESCRIPTION: Returns the scale in pixels per mm of an object's image buffer.
 * PARAMETERS:
 *    mode: Identifies how the buffer is to be mapped to output pixels; the
 *       value must be PIXEL_REPLICATE, ANTI_ALIAS, ICON, or ONE_TO_ONE.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables scale, icon_scale must be valid.
 * RETURN VALUE: The scale in pixels per mm of an object's image buffer.
 * EXIT CONDITIONS: Undefined if the parameter is not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
double cvRenderer::buffer_scale(Display_mode mode)
{
	switch (mode)
	{	default:
			assert(FALSE);
		case ICON:
			return (icon_scale*.25);
		case ONE_TO_ONE:
			return (scale);
		case PIXEL_REPLICATE:
			return (scale*0.5);
		case ANTI_ALIAS:
			return (scale*2);
	}
}

/*****************************************************************************
 * FUNCTION: get_structure_transformation
 * DESCRIPTION: Computes a transformation from the structure-aligned coordinate
 *    system with Y0 axis through center of first voxel in the structure's
 *    bounding box, Y0 = 0 plane through the origin of the structure coordinate
 *    system, and units of voxels, to the object's image buffer
 *    coordinate system with origin at center and units of pixels.
 * PARAMETERS:
 *    matrix: The rotation and scaling matrix to be computed.
 *    vector: The translation vector to be computed.
 *    rotation_matrix: The rotation matrix to be computed.
 *    center: The translation vector from the coordinate system centered on
 *       the structure's bounding box to be computed.
 *    object: The object
 *    mode: Identifies how the buffer is to be mapped to output pixels; the
 *       value must be PIXEL_REPLICATE, ANTI_ALIAS, ICON, or ONE_TO_ONE.
 *    mirror: Non-zero if the transformation is for the reflected object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variables object_list, glob_angle, scale,
 *    icon_scale, depth_scale, plane_normal, plane_displacement must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/21/93 for multiple structures by Dewey Odhner
 *    Modified: 10/19/93 to use different measurement units by Dewey Odhner
 *    Modified: 7/6/01 DIRECT data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::get_structure_transformation(double matrix[3][3],
	double vector[3], double rotation_matrix[3][3], double center[3],
	Shell *object, Display_mode mode, int mirror)
{
	double temp_matrix[3][3], unit;
	int j, k;
	Shell_data *obj_data;
	StructureInfo *str;

	obj_data = mode==ICON? &object->icon_data: &object->main_data;
	str = &obj_data->file->file_header.str;
	unit = unit_mm(obj_data);
	if (obj_data->file->file_header.gen.data_type == IMAGE0)
	{
		matrix[0][0] = obj_data->file->file_header.scn.xypixsz[0]*unit;
		matrix[1][1] = obj_data->file->file_header.scn.xypixsz[1]*unit;
	}
	else
	{
		matrix[0][0] = str->xysize[0]*unit;
		matrix[1][1] = str->xysize[1]*unit;
	}
	matrix[2][2] = Slice_spacing(obj_data)*unit;
	for (j=0; j<3; j++)
		for (k=0; k<j; k++)
			matrix[j][k] = matrix[k][j] = 0;
	vector[0] =
		-.5*unit*(Max_coordinate(obj_data, 0)+(Min_coordinate(obj_data, 0)));
	vector[1] =
		-.5*unit*(Max_coordinate(obj_data, 1)-(Min_coordinate(obj_data, 1)));
	vector[2] =
		-.5*unit*(Max_coordinate(obj_data, 2)-(Min_coordinate(obj_data, 2)));
	get_object_transformation(rotation_matrix, center, object, mirror);
	matrix_multiply(matrix, rotation_matrix, matrix);
	matrix_vector_multiply(vector, rotation_matrix, vector);
	vector_add(vector, center, vector);
	for (j=0; j<3; j++)
		for (k=0; k<j; k++)
			temp_matrix[j][k] = temp_matrix[k][j] = 0;
	temp_matrix[0][0] = temp_matrix[1][1] = buffer_scale(mode);
	temp_matrix[2][2] = -depth_scale;
	matrix_multiply(matrix, temp_matrix, matrix);
	matrix_vector_multiply(vector, temp_matrix, vector);
	matrix_vector_multiply(center, temp_matrix, center);
}

/*****************************************************************************
 * FUNCTION: unit_mm
 * DESCRIPTION: Returns the size in mm of the unit of measurement in the Y0
 *    direction of a shell.
 * PARAMETERS:
 *    shell_data: The shell to get the unit from.  The unit must be at least
 *       0 (km) and not more than 4 (micron).
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Returns the size in mm of the unit of measurement.
 * EXIT CONDITIONS: Undefined if shell_data is not valid.
 * HISTORY:
 *    Created: 10/19/93 by Dewey Odhner
 *    Modified: 7/6/01 DIRECT data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
double unit_mm(Shell_data *shell_data)
{
	return (unit_size(shell_data->file->file_header.gen.data_type==IMAGE0?
		shell_data->file->file_header.scn.measurement_unit[0]:
		shell_data->file->file_header.str.measurement_unit[0]));
}

/*****************************************************************************
 * FUNCTION: unit_size
 * DESCRIPTION: Returns the size in mm of the unit of measurement in the Y0
 *    direction from a code.
 * PARAMETERS:
 *    code:
 *       0: km
 *       1: m
 *       2: cm
 *       3: mm
 *       4: micron.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Returns the size in mm of the unit of measurement.
 * EXIT CONDITIONS: Undefined if code is not valid.
 * HISTORY:
 *    Created: 1/12/94 by Dewey Odhner
 *
 *****************************************************************************/
double unit_size(int code)
{
	static double mm[5]={1000000., 1000., 10., 1., .001};

	return (mm[code]);
}
