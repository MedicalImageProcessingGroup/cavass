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
 * FUNCTION: object_number
 * DESCRIPTION: Returns the object number (not label) of a virtual object.
 *    (The object number is a positive number that may change if another
 *    virtual object is created or destroyed.)
 * PARAMETERS:
 *    vobj: The virtual object
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The object number (not label) of a virtual object if it
 *    exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::object_number(Virtual_object *vobj)
{
	int n, num_of_reflections;
	Shell *obj;

	num_of_reflections = number_of_reflections();
	n = 1;
	for (obj=object_list; obj!=NULL; obj=obj->next)
		if (obj->reflection)
		{	if (vobj == obj->reflection)
				return (num_of_reflections+1-n);
			n++;
		}
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (vobj == &obj->O)
			return (n);
		n++;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: object_of_color
 * DESCRIPTION: Returns the object number (not label) of a virtual object of
 *    a particular color if one exists; otherwise zero.
 * PARAMETERS:
 *    rgb: The color components on a scale of 0 to 65535.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The object number (not label) of a virtual object of
 *    a particular color if one exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::object_of_color(RGB rgb)
{
	int n, num_of_reflections;
	Shell *obj;

	num_of_reflections = number_of_reflections();
	n = 1;
	for (obj=object_list; obj!=NULL; obj=obj->next)
		if (obj->reflection)
		{	if (obj->reflection->rgb.red==rgb.red &&
					obj->reflection->rgb.green==rgb.green &&
					obj->reflection->rgb.blue==rgb.blue &&
					obj->reflection->color>=0)
				return (num_of_reflections+1-n);
			n++;
		}
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (obj->O.rgb.red==rgb.red && obj->O.rgb.green==rgb.green &&
				obj->O.rgb.blue==rgb.blue && obj->O.color>=0)
			return (n);
		n++;
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: object_from_number
 * DESCRIPTION: Returns the address of a virtual object from the object number.
 * PARAMETERS:
 *    m: The object number (not label).
 *       (The object number is a positive number that may change if another
 *       virtual object is created or destroyed.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The address of the virtual object if it
 *    exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
Virtual_object *cvRenderer::object_from_number(int m)
{
	int n, num_of_reflections;
	Shell *obj;

	num_of_reflections = number_of_reflections();
	n = 1;
	for (obj=object_list; obj!=NULL; obj=obj->next)
		if (obj->reflection)
		{	if (m == num_of_reflections+1-n)
				return (obj->reflection);
			n++;
		}
	for (obj=object_list; obj!=NULL; obj=obj->next)
	{	if (m == n)
			return (&obj->O);
		n++;
	}
	return (NULL);
}

/*****************************************************************************
 * FUNCTION: actual_object
 * DESCRIPTION: Returns the address of the Shell to which a virtual object
 *    belongs.
 * PARAMETERS:
 *    vobj: The virtual object.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The address of the Shell to which a virtual object belongs
 *    if it exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
Shell *cvRenderer::actual_object(Virtual_object *vobj)
{
	Shell *obj;

	if (vobj == NULL)
		return (NULL);
	for (obj=object_list; obj!=NULL; obj=obj->next)
		if (vobj==&obj->O || vobj==obj->reflection)
			return (obj);
	return (NULL);
}

/*****************************************************************************
 * FUNCTION: number_of_objects
 * DESCRIPTION: Returns the number of virtual objects.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The number of virtual objects.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::number_of_objects(void)
{
	Shell *obj;
	int num_of_objects;

	num_of_objects = 0;
	for (obj=object_list; obj; obj=obj->next)
	{	num_of_objects++;
		if (obj->reflection != NULL)
			num_of_objects++;
	}
	return (num_of_objects);
}

/*****************************************************************************
 * FUNCTION: number_of_reflections
 * DESCRIPTION: Returns the number of reflections.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The number of reflections.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::number_of_reflections(void)
{
	Shell *obj;
	int num_of_reflections;

	num_of_reflections = 0;
	for (obj=object_list; obj; obj=obj->next)
		if (obj->reflection != NULL)
			num_of_reflections++;
	return (num_of_reflections);
}

/*****************************************************************************
 * FUNCTION: object_label
 * DESCRIPTION: Returns the object label of a virtual object.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * PARAMETERS:
 *    vobj: The virtual object
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The object label of a virtual object if it
 *    exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::object_label(Virtual_object *vobj)
{
	int num_of_reflections, j;
	Shell *obj;

	if (vobj == NULL)
		return (0);
	num_of_reflections = number_of_reflections();
	for (j=num_of_reflections,obj=object_list; obj; j++,obj=obj->next)
	{	if (vobj == &obj->O)
			return (j-num_of_reflections+1);
		if (vobj == obj->reflection)
			return (-(j-num_of_reflections+1));
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: object_from_label
 * DESCRIPTION: Returns the address of a virtual object from the object label.
 * PARAMETERS:
 *    label: The object label.
 *    (The object label is a positive number for a positive object, a negative
 *    number for a reflection, that may change if another
 *    object is destroyed.  It is not related to the parameter vector value.)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: The address of the virtual object if it
 *    exists; otherwise zero.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
Virtual_object *cvRenderer::object_from_label(int label)
{
	int num_of_reflections, j;
	Shell *obj;

	num_of_reflections = number_of_reflections();
	for (j=num_of_reflections,obj=object_list; obj; j++,obj=obj->next)
	{	if (label == j-num_of_reflections+1)
			return (&obj->O);
		if (label == -(j-num_of_reflections+1))
			return (obj->reflection);
	}
	return (NULL);
}
