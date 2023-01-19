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

#include "cvRender.h"

#define SQRT2 1.414213562373095
#define SQRT3 1.732050807568877

int VReadData(void* data, int size, int items, FILE* fp, int* items_read)
{
	return VReadData((char *)data, size, items, fp, items_read);
}
int VWriteData(void* data, int size, int items, FILE* fp, int* items_written)
{
	return VWriteData((char *)data, size, items, fp, items_written);
}

/*****************************************************************************
 * FUNCTION: read_plan
 * DESCRIPTION: Reads a plan file and reports the information relevant to
 *    the manipulate program.
 * PARAMETERS:
 *    shell_file_name: The name of the structure system will go here.
 *    num_of_objects: The number of objects in the plan will go here.
 *    shell_numbers: A list with the shell number of each object goes here.
 *       The memory can be freed by the caller after use.
 *    rotations: A list with the rotation matrices giving the orientation of
 *       each object goes here.
 *       The memory can be freed by the caller after use.
 *    displacements: A list with the displacements of each object goes here.
 *       The memory can be freed by the caller after use.
 *    plan_file_name: The name of the plan file to read.
 *    read_bb: Flag to indicate the bounding box is to be read if it is there.
 *    num_of_axes: The number of sets of axes in the plan will go here.
 *    axis_rotations: A list with the rotation matrices giving the orientation
 *       of each set of axes goes here.
 *       The memory can be freed by the caller after use.
 *    axis_displacements: A list with the displacements of each set of axes
 *       goes here.  The memory can be freed by the caller after use.
 *    axis_shell_numbers: A list with the shell number of each set of axes
 *       goes here.  The memory can be freed by the caller after use.
 * SIDE EFFECTS: The global variable glob_displacement may be set.
 * ENTRY CONDITIONS: The plan file must be formatted according to the plan
 *    specification for a three-dimensional structure system.
 * RETURN VALUE:
 *    0: no error
 *    1: memory allocation failure
 *    2: read error
 *    4: can't open file
 *    100: plan file not formatted according to the plan specification for a
 *       three-dimensional structure system.
 *    235: White space was not found after string_size-1 characters.
 * EXIT CONDITIONS: If an error occurs, the values at the first five
 *    parameters are undefined; memory will not be allocated.  The function may
 *    not be able to distinguish between errors 2 and 100.
 * HISTORY:
 *    Created: 12/2/92 by Dewey Odhner.
 *    Modified: 6/17/93 to read the bounding box & set global variable
 *       glob_displacement by Dewey Odhner.
 *    Modified: 9/3/96 to read the axes by Dewey Odhner.
 *    Modified: 9/10/96 to set *num_of_axes to 0 by Dewey Odhner.
 *    Modified: 3/23/00 axis parameters checked for NULL by Dewey Odhner.
 *
 *****************************************************************************/
int cvRenderer::read_plan(char shell_file_name[300], int *num_of_objects,
	int **shell_numbers, triple (**rotations)[3], triple **displacements,
	char plan_file_name[], int read_bb, int *num_of_axes,
	triple (**axis_rotations)[3], triple **axis_displacements,
	int **axis_shell_numbers)
{
	FILE *plan_file;
	int level, *branch, j, k, error_code, obj_num, axisn;
	Plan_tree root, *node;
	char token[100];
	double *node_translation, min_x, min_y, min_z, max_x, max_y, max_z;
	triple *node_rotation, temp_translation, temp_rotation[3];

	typedef triple tri_triple[3];

	if (num_of_axes)
	{
		*axis_shell_numbers = NULL;
		*num_of_axes = 0;
	}
	plan_file = fopen(plan_file_name, "rb");
	if (plan_file == NULL)
		return (4);
	if (fgets(shell_file_name, 299, plan_file) == NULL)
	{	fclose(plan_file);
		return (2);
	}
	if (shell_file_name[strlen(shell_file_name)-1] == '\n')
		shell_file_name[strlen(shell_file_name)-1] = 0;
	token[0] = 0;
	if (read_bb)
	{	while (strcmp(token, "TREE") && (num_of_axes==NULL||
				strcmp(token, "AXIS")) && strcmp(token, "BBOX"))
		{	error_code = get_string(token, plan_file, 100);
			if (error_code)
			{	fclose(plan_file);
				return (error_code);
			}
		}
		if (strcmp(token, "BBOX") == 0)
		{	/* read bounding box */
			error_code = get_string(token, plan_file, 100);
			if (error_code)
			{	fclose(plan_file);
				return (error_code);
			}
			if (strcmp(token, "{"))
			{	fclose(plan_file);
				return (100);
			}
			if (fscanf(plan_file, " %lf %lf %lf %lf %lf %lf", &min_x, &max_x,
					&min_y, &max_y, &min_z, &max_z) != 6)
			{	fclose(plan_file);
				return (2);
			}
			glob_displacement[0] = -.5*(min_x+max_x);
			glob_displacement[1] = -.5*(min_y+max_y);
			glob_displacement[2] = -.5*(min_z+max_z);
		}
	}
	while (strcmp(token, "TREE") && strcmp(token, "AXIS"))
	{	error_code = get_string(token, plan_file, 100);
		if (error_code)
		{	fclose(plan_file);
			return (error_code);
		}
	}
	if (strcmp(token, "AXIS")==0 && num_of_axes)
	{	/* read axes */
		error_code = get_string(token, plan_file, 100);
		if (error_code)
		{	fclose(plan_file);
			return (error_code);
		}
		if (sscanf(token, "%d", num_of_axes) != 1)
		{	fclose(plan_file);
			return (100);
		}
		*axis_shell_numbers = (int *)malloc(*num_of_axes*sizeof(int));
		if (*axis_shell_numbers == NULL)
		{	fclose(plan_file);
			return (1);
		}
		*axis_displacements = (triple *)malloc(*num_of_axes*sizeof(triple));
		if (*axis_displacements == NULL)
		{	fclose(plan_file);
			free(*axis_shell_numbers);
			return (1);
		}
		*axis_rotations= (tri_triple *)malloc(*num_of_axes*sizeof(tri_triple));
		if (*axis_rotations == NULL)
		{	fclose(plan_file);
			free(*axis_shell_numbers);
			free(*axis_displacements);
			return (1);
		}
		error_code = get_string(token, plan_file, 100);
		if (error_code)
		{	fclose(plan_file);
			free(*axis_shell_numbers);
			free(*axis_displacements);
			free(*axis_rotations);
			return (error_code);
		}
		if (strcmp(token, "{"))
		{	fclose(plan_file);
			free(*axis_shell_numbers);
			free(*axis_displacements);
			free(*axis_rotations);
			return (100);
		}
		for (axisn=0; axisn<*num_of_axes; axisn++)
		{
			if (fscanf(plan_file, " %d", *axis_shell_numbers+axisn) != 1)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (2);
			}
			error_code = get_string(token, plan_file, 100);
			if (error_code)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (error_code);
			}
			if (strcmp(token, "{") != 0)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (100);
			}
			if (fscanf(plan_file, " %lf %lf %lf", (*axis_displacements)[axisn],
					(*axis_displacements)[axisn]+1,
					(*axis_displacements)[axisn]+2) != 3)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (2);
			}
			error_code = get_string(token, plan_file, 100);
			if (error_code)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (error_code);
			}
			if (strcmp(token, "}") != 0)
			{	fclose(plan_file);
				free(*axis_shell_numbers);
				free(*axis_displacements);
				free(*axis_rotations);
				return (100);
			}
			for (j=0; j<3; j++)
			{
				error_code = get_string(token, plan_file, 100);
				if (error_code)
				{	fclose(plan_file);
					free(*axis_shell_numbers);
					free(*axis_displacements);
					free(*axis_rotations);
					return (error_code);
				}
				if (strcmp(token, "{") != 0)
				{	fclose(plan_file);
					free(*axis_shell_numbers);
					free(*axis_displacements);
					free(*axis_rotations);
					return (100);
				}
				if (fscanf(plan_file, " %lf %lf %lf",
						(*axis_rotations)[axisn][0]+j,
						(*axis_rotations)[axisn][1]+j,
						(*axis_rotations)[axisn][2]+j) != 3)
				{	fclose(plan_file);
					free(*axis_shell_numbers);
					free(*axis_displacements);
					free(*axis_rotations);
					return (2);
				}
				error_code = get_string(token, plan_file, 100);
				if (error_code)
				{	fclose(plan_file);
					free(*axis_shell_numbers);
					free(*axis_displacements);
					free(*axis_rotations);
					return (error_code);
				}
				if (strcmp(token, "}") != 0)
				{	fclose(plan_file);
					free(*axis_shell_numbers);
					free(*axis_displacements);
					free(*axis_rotations);
					return (100);
				}
			}
		}
	}
	while (strcmp(token, "TREE"))
	{	error_code = get_string(token, plan_file, 100);
		if (error_code)
		{	fclose(plan_file);
			if (axis_shell_numbers && *axis_shell_numbers)
				free(*axis_shell_numbers),free(*axis_displacements),
					free(*axis_rotations);
			return (error_code);
		}
	}
	if (fscanf(plan_file, " %d", &level) != 1)
	{	fclose(plan_file);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),
				free(*axis_displacements),free(*axis_rotations);
		return (2);
	}
	error_code = get_string(token, plan_file, 100);
	if (error_code)
	{	fclose(plan_file);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (error_code);
	}
	if (strcmp(token, "{"))
	{	fclose(plan_file);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (100);
	}
	branch = (int *)malloc(level*sizeof(int));
	if (branch == NULL)
	{	fclose(plan_file);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (1);
	}
	root.child = (Plan_tree *)calloc(1, sizeof(Plan_tree));
	if (root.child == NULL)
	{	fclose(plan_file);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		free(branch);
		return (1);
	}
	root.level = level+1;
	root.children = 1;
	branch[0] = 0;
	*num_of_objects = 0;
	while (level >= 1)
	{	node = branch_pointer(&root, branch, level);
		if (fscanf(plan_file, " %d %d", &node->level,
				level==1? &node->obj_index: &node->children) != 2)
		{	free_children(&root);
			fclose(plan_file);
			free(branch);
			if (axis_shell_numbers && *axis_shell_numbers)
				free(*axis_shell_numbers),free(*axis_displacements),
					free(*axis_rotations);
			return (2);
		}
		error_code = get_string(token, plan_file, 100);
		if (error_code)
		{	fclose(plan_file);
			if (axis_shell_numbers && *axis_shell_numbers)
				free(*axis_shell_numbers),free(*axis_displacements),
					free(*axis_rotations);
			return (error_code);
		}
		if (strcmp(token, "{") == 0)
		{	for (j=0; j<4; j++)
				if (fscanf(plan_file, " %lf %lf %lf %lf", node->matrix[j],
						node->matrix[j]+1, node->matrix[j]+2,
						node->matrix[j]+3) != 4)
				{	node->children = 0;
					free_children(&root);
					fclose(plan_file);
					free(branch);
					if (axis_shell_numbers && *axis_shell_numbers)
						free(*axis_shell_numbers),free(*axis_displacements),
							free(*axis_rotations);
					return (2);
				}
				error_code = get_string(token, plan_file, 100);
				if (error_code==0 && strcmp(token, "}"))
					error_code = 100;
				if (error_code)
				{	fclose(plan_file);
					if (axis_shell_numbers && *axis_shell_numbers)
						free(*axis_shell_numbers),free(*axis_displacements),
							free(*axis_rotations);
					return (error_code);
				}
		}
		else if (strcmp(token, "ID") == 0)
			for (j=0; j<4; j++)
				for (k=0; k<4; k++)
					node->matrix[j][k] = j==k? 1: 0;
		else
		{	fclose(plan_file);
			if (axis_shell_numbers && *axis_shell_numbers)
				free(*axis_shell_numbers),free(*axis_displacements),
					free(*axis_rotations);
			return (100);
		}
		if (node->children)
		{	node->child =
				(Plan_tree *)calloc(node->children, sizeof(Plan_tree));
			if (node->child == NULL)
			{	node->children = 0;
				free_children(&root);
				fclose(plan_file);
				free(branch);
				if (axis_shell_numbers && *axis_shell_numbers)
					free(*axis_shell_numbers),free(*axis_displacements),
						free(*axis_rotations);
				return (1);
			}
		}
		else
			(*num_of_objects)++;
		breadth_next(&root, branch, &level);
	}
	fclose(plan_file);
	*shell_numbers = (int *)malloc(*num_of_objects*sizeof(int));
	if (*shell_numbers == NULL)
	{	free_children(&root);
		free(branch);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (1);
	}
	*rotations = (tri_triple *)malloc(*num_of_objects*sizeof(tri_triple));
	if (*rotations == NULL)
	{	free_children(&root);
		free(branch);
		free(*shell_numbers);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (1);
	}
	*displacements = (triple *)malloc(*num_of_objects*sizeof(triple));
	if (*displacements == NULL)
	{	free_children(&root);
		free(branch);
		free(*shell_numbers);
		free(*rotations);
		if (axis_shell_numbers && *axis_shell_numbers)
			free(*axis_shell_numbers),free(*axis_displacements),
				free(*axis_rotations);
		return (1);
	}
	for (obj_num=0; obj_num<*num_of_objects;
			j=1,breadth_next(&root, branch, &j),obj_num++)
	{	node_translation = (*displacements)[obj_num];
		node_rotation = (*rotations)[obj_num];
		for (j=0; j<3; j++)
		{	node_translation[j] = root.child[0].matrix[j][3];
			for (k=0; k<3; k++)
				node_rotation[j][k] = root.child[0].matrix[j][k];
		}
		for (level=root.level-2; level>0; level--)
		{	node = branch_pointer(&root, branch, level);
			for (j=0; j<3; j++)
			{	temp_translation[j] = node->matrix[j][3];
				for (k=0; k<3; k++)
					temp_rotation[j][k] = node->matrix[j][k];
			}
			matrix_vector_multiply(temp_translation, node_rotation,
				temp_translation);
			vector_add(node_translation, node_translation, temp_translation);
			matrix_multiply(node_rotation, temp_rotation, node_rotation);
		}
		(*shell_numbers)[obj_num]= branch_pointer(&root, branch, 1)->obj_index;
	}
	free_children(&root);
	free(branch);
	return (0);
}

/*****************************************************************************
 * FUNCTION: free_children
 * DESCRIPTION: Free the sub-trees of a node of a Plan_tree.
 * PARAMETERS:
 *    node: Free the children of this node.
 * SIDE EFFECTS: None.
 * ENTRY CONDITIONS: The child list must be allocated if children is non-zero,
 *    for node and all descendants.
 * RETURN VALUE: None.
 * EXIT CONDITIONS: None.
 * HISTORY:
 *    Created: 11/4/92 by Dewey Odhner.
 *
 *****************************************************************************/
int free_children(Plan_tree *node)
{
	int j;

	for (j=0; j<node->children; j++)
		free_children(node->child+j);
	if (node->children)
		free(node->child);
	return 0;
}

/*****************************************************************************
 * FUNCTION: breadth_next
 * DESCRIPTION: Find the next node in breadth-first traversal of a Plan_tree.
 * PARAMETERS:
 *    root: the root of the tree
 *    branch: the branch to take at each level starting at the child of root
 *       to get to the current node.  There must be enough space in this
 *       array for root->level-1 elements.
 *    level: the level of the node.  level must be at least 1 and
 *       at most root->level.
 *    E.g., if root->level=5, branch={0, 1, 2}, level=2, root->children=1,
 *       root->child[0].children=2, and root->child[0].child[1].children=3,
 *       then the new values will be branch={0, 0, 0, 0}, level=1.
 * SIDE EFFECTS: None.
 * ENTRY CONDITIONS: The parameters must be consistent.
 * RETURN VALUE: None.
 * EXIT CONDITIONS: None.
 * HISTORY:
 *    Created: 11/4/92 by Dewey Odhner.
 *
 *****************************************************************************/
void breadth_next(Plan_tree *root, int branch[], int *level)
{
	if (*level == root->level)
		(*level)--;
	else if (branch_pointer(root, branch, *level+1)->children >
			branch[root->level-*level-1]+1)
		branch[root->level-*level-1]++;
	else
	{	(*level)++;
		breadth_next(root, branch, level);
		(*level)--;
		if (*level)
			branch[root->level-*level-1] = 0;
	}
}

/*****************************************************************************
 * FUNCTION: branch_pointer
 * DESCRIPTION: Return a pointer to a node in a Plan_tree.
 * PARAMETERS:
 *    root: the root of the tree
 *    branch: the branch to take at each level starting at the child of root.
 *    level: the level of the node to return.  level must be at least 1 and
 *       at most root->level.
 *    E.g., if root->level=5, branch={0, 1, 2}, and level=2, return
 *       &root->child[0].child[1].child[2]
 * SIDE EFFECTS: None.
 * ENTRY CONDITIONS: The parameters must be consistent.
 * RETURN VALUE: the pointer to the specified node
 * EXIT CONDITIONS: None.
 * HISTORY:
 *    Created: 11/4/92 by Dewey Odhner.
 *
 *****************************************************************************/
Plan_tree *branch_pointer(Plan_tree *root, int branch[], int level)
{
	return
	(	level==root->level
		?	root
		:	branch_pointer(root, branch, level+1)->child+
				branch[root->level-level-1]
	);
}

/*****************************************************************************
 * FUNCTION: get_string
 * DESCRIPTION: Reads a space-delimited string from a file.
 * PARAMETERS:
 *    string: Where the string will be put.
 *    file: The file to read from, must be opened for reading.
 *    string_size: The size of the array at string.
 * SIDE EFFECTS: The file position is advanced to the end of the string.
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE:
 *    0: no error
 *    2: read error
 *    235: White space was not found after string_size-1 characters.
 * EXIT CONDITIONS: If an error occurs, the file position will be left at the
 *    character where the error occurred.
 * HISTORY:
 *    Created: 11/2/92 by Dewey Odhner.
 *
 *****************************************************************************/
int get_string(char string[], FILE *file, int string_size)
{
	int n, c;

	for (c=getc(file);;c=getc(file))
	{	switch (c)
		{	case EOF:
				string[0] = 0;
				return (2);
			case ' ':
			case '\n':
			case '\t':
				continue;
		}
		break;
	}
	for (n=0; n<string_size; n++)
		switch (c)
		{	case EOF:
				string[n] = 0;
				return (2);
			case ' ':
			case '\n':
			case '\t':
				string[n] = 0;
				ungetc(c, file);
				return (0);
			default:
				string[n] = c;
				c = getc(file);
				continue;
		}
	string[n-1] = 0;
	ungetc(c, file);
	return (235);
}

/*****************************************************************************
 * FUNCTION: load_file
 * DESCRIPTION: Loads the objects from a file.
 * PARAMETERS:
 *    last_shell: The address of the last pointer in the object list, unless
 *       icon_flag, then the last pointer in the object list after those with
 *       icons already loaded.
 *    icon_flag: Whether the file is to be loaded as icons; if so, the Shell
 *       structs must be already in the object_list.
 *    file_name: The file name
 *    from_plan: Whether the objects to be loaded are specified from a plan;
 *       if not, all objects in the file will be loaded.
 *    num_of_objects: The number of objects to be loaded if from_plan.
 *    shell_numbers: The objects to be loaded if from_plan. (0 = first object)
 * SIDE EFFECTS: The global variable colormap_valid will be cleared.
 *    An error message may be issued.
 * ENTRY CONDITIONS: The global variable object_list must be valid or NULL.
 * RETURN VALUE: Non-zero if unsuccessful.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 10/29/92 to use load_shell by Dewey Odhner
 *    Modified: 12/4/92 to use info from plan by Dewey Odhner
 *    Modified: 4/16/93 for new header structure by Dewey Odhner
 *    Modified: 10/19/93 to allow other units by Dewey Odhner
 *    Modified: 5/4/94 for fuzzy shells by Dewey Odhner
 *    Modified: 7/6/94 to clear colormap_valid by Dewey Odhner
 *    Modified: 5/18/01 BINARY_B data type accommodated by Dewey Odhner
 *    Modified: 7/6/01 DIRECT data type accommodated by Dewey Odhner
 *    Modified: 3/5/02 all trailing spaces removed from scene file name
 *       by Dewey Odhner
 *    Modified: 7/10/03 T_SHELL data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::load_file(Shell **last_shell, int icon_flag, char file_name[],
	int from_plan, int num_of_objects, int shell_numbers[])
{
	FILE *infile;
	int error_code, shell_number, j;
	char bad_group[5], bad_element[5];
	Shell_file *shell_file;
	Classification_type shell_class;

	if ((infile=fopen(file_name, "rb"))==NULL)
	{
		return (4);
	}
	shell_file = (Shell_file *)calloc(1, sizeof(Shell_file));
	if (shell_file == NULL)
	{	report_malloc_error();
		fclose(infile);
		return (1);
	}
	error_code = VReadHeader(infile, &shell_file->file_header, bad_group,
		bad_element);
	switch (error_code)
	{	case 0:
		case 107:
		case 106:
			break;
		default:
			fprintf(stderr, "Group %s element %s undefined in VReadHeader\n",
				bad_group, bad_element);
			fclose(infile);
			free(shell_file);
			return (error_code);
	}

	if (shell_file->file_header.gen.data_type == SHELL2)
		shell_class = T_SHELL;
	else if (shell_file->file_header.gen.data_type == IMAGE0)
		shell_class = DIRECT;
	else if (shell_file->file_header.str.bit_fields_in_TSE[5] >
			shell_file->file_header.str.bit_fields_in_TSE[4])
		shell_class = PERCENT;
	else if (shell_file->file_header.str.bit_fields_in_TSE[15] >
			shell_file->file_header.str.bit_fields_in_TSE[14])
		shell_class = GRADIENT;
	else if (shell_file->file_header.str.bit_fields_in_TSE[3] > 15)
		shell_class = BINARY_B;
	else
		shell_class = BINARY_A;
	/* check that all values are normal */
	if (shell_class==DIRECT?
			(shell_file->file_header.scn.dimension!=3&&
				shell_file->file_header.scn.dimension!=4) ||
			shell_file->file_header.scn.measurement_unit[0]<0 ||
			shell_file->file_header.scn.measurement_unit[0]>4 ||
			shell_file->file_header.scn.measurement_unit[1]!=
				shell_file->file_header.scn.measurement_unit[0] ||
			shell_file->file_header.scn.measurement_unit[2]!=
				shell_file->file_header.scn.measurement_unit[0] ||
			shell_file->file_header.scn.num_of_density_values!=1 ||
			shell_file->file_header.scn.num_of_integers!=1 ||
			shell_file->file_header.scn.signed_bits[0] ||
			(shell_file->file_header.scn.num_of_bits!=8&&
				shell_file->file_header.scn.num_of_bits!=16) ||
			shell_file->file_header.scn.bit_fields[0] ||
			shell_file->file_header.scn.bit_fields[1]!=
				shell_file->file_header.scn.num_of_bits-1 ||
			(	shell_file->file_header.scn.dimension==3?
				shell_file->file_header.scn.loc_of_subscenes[
				shell_file->file_header.scn.num_of_subscenes[0]-1]<=
				shell_file->file_header.scn.loc_of_subscenes[0]:
				shell_file->file_header.scn.loc_of_subscenes[
				shell_file->file_header.scn.num_of_subscenes[0]+
				shell_file->file_header.scn.num_of_subscenes[1]-1]<=
				shell_file->file_header.scn.loc_of_subscenes[
				shell_file->file_header.scn.num_of_subscenes[0]]
			):
			(shell_file->file_header.gen.data_type!=SHELL0 &&
			shell_file->file_header.gen.data_type!=SHELL2) ||
			shell_file->file_header.str.dimension!=3 ||
			shell_file->file_header.str.measurement_unit[0]<0 ||
			shell_file->file_header.str.measurement_unit[0]>4 ||
			shell_file->file_header.str.measurement_unit[1]!=
				shell_file->file_header.str.measurement_unit[0] ||
			shell_file->file_header.str.measurement_unit[2]!=
				shell_file->file_header.str.measurement_unit[0] ||
			shell_file->file_header.str.num_of_components_in_TSE!=
				(shell_class==T_SHELL? 32: 9) ||
			shell_file->file_header.str.num_of_components_in_NTSE!=1 ||
			shell_file->file_header.str.num_of_integers_in_TSE<6 ||
			shell_file->file_header.str.num_of_bits_in_TSE!=
				(shell_class==T_SHELL? 8:
				 shell_class==BINARY_B||shell_class==BINARY_A? 32: 48) ||
			shell_file->file_header.str.bit_fields_in_TSE[0] ||
			(shell_class!=T_SHELL&&
			(	shell_file->file_header.str.bit_fields_in_TSE[1]!=5 ||
				shell_file->file_header.str.bit_fields_in_TSE[2]!=6 ||
				shell_file->file_header.str.bit_fields_in_TSE[3]!=
					(shell_class==BINARY_B? 16:15) ||
				(shell_class!=PERCENT&&
					shell_file->file_header.str.bit_fields_in_TSE[5]>=
					shell_file->file_header.str.bit_fields_in_TSE[4])
			)) ||
			((shell_class==PERCENT&&
				(shell_file->file_header.str.bit_fields_in_TSE[4]!=16||
				shell_file->file_header.str.bit_fields_in_TSE[5]!=20)) ||
			(	shell_class==T_SHELL?
				shell_file->file_header.str.bit_fields_in_TSE[1]!=7 ||
				shell_file->file_header.str.bit_fields_in_TSE[2]!=8 ||
				shell_file->file_header.str.bit_fields_in_TSE[3]!=23 ||
				shell_file->file_header.str.bit_fields_in_TSE[4]!=24 ||
				shell_file->file_header.str.bit_fields_in_TSE[9]!=32 ||
				shell_file->file_header.str.bit_fields_in_TSE[10]!=33 ||
				shell_file->file_header.str.bit_fields_in_TSE[15]!=47 ||
				shell_file->file_header.str.bit_fields_in_TSE[16]!=48 ||
				shell_file->file_header.str.bit_fields_in_TSE[21]!=56 ||
				shell_file->file_header.str.bit_fields_in_TSE[22]!=57 ||
				shell_file->file_header.str.bit_fields_in_TSE[27]!=71 ||
				shell_file->file_header.str.bit_fields_in_TSE[28]!=72 ||
				shell_file->file_header.str.bit_fields_in_TSE[33]!=80 ||
				shell_file->file_header.str.bit_fields_in_TSE[34]!=81 ||
				shell_file->file_header.str.bit_fields_in_TSE[39]!=95 ||
				shell_file->file_header.str.bit_fields_in_TSE[40]!=96 ||
				shell_file->file_header.str.bit_fields_in_TSE[45]!=104 ||
				shell_file->file_header.str.bit_fields_in_TSE[46]!=105 ||
				shell_file->file_header.str.bit_fields_in_TSE[51]!=119 ||
				shell_file->file_header.str.bit_fields_in_TSE[52]!=120 ||
				shell_file->file_header.str.bit_fields_in_TSE[57]!=128 ||
				shell_file->file_header.str.bit_fields_in_TSE[58]!=129 ||
				shell_file->file_header.str.bit_fields_in_TSE[63]!=143:
				shell_class==BINARY_B?
				shell_file->file_header.str.bit_fields_in_TSE[6]!=17 ||
				shell_file->file_header.str.bit_fields_in_TSE[7]!=19 ||
				shell_file->file_header.str.bit_fields_in_TSE[8]!=20 ||
				shell_file->file_header.str.bit_fields_in_TSE[10]!=26 ||
				(shell_file->file_header.str.bit_fields_in_TSE[9]!=25||
					shell_file->file_header.str.bit_fields_in_TSE[11]!=31):
				shell_file->file_header.str.bit_fields_in_TSE[6]!=21 ||
				shell_file->file_header.str.bit_fields_in_TSE[7]!=23 ||
				shell_file->file_header.str.bit_fields_in_TSE[8]!=24 ||
				shell_file->file_header.str.bit_fields_in_TSE[10]!=28 ||
				((shell_file->file_header.str.bit_fields_in_TSE[9]!=27||
					shell_file->file_header.str.bit_fields_in_TSE[11]!=31) &&
				(shell_file->file_header.str.bit_fields_in_TSE[9]!=26||
					shell_file->file_header.str.bit_fields_in_TSE[11]!=30))
			)) ||
			((shell_class==BINARY_B||shell_class==BINARY_A) &&
			(	shell_file->file_header.str.bit_fields_in_TSE[13]>=
					shell_file->file_header.str.bit_fields_in_TSE[12] ||
				shell_file->file_header.str.bit_fields_in_TSE[15]>=
					shell_file->file_header.str.bit_fields_in_TSE[14] ||
				shell_file->file_header.str.bit_fields_in_TSE[17]>=
					shell_file->file_header.str.bit_fields_in_TSE[16]
			)) ||
			(shell_class==PERCENT&&
			(	shell_file->file_header.str.bit_fields_in_TSE[15]>=
					shell_file->file_header.str.bit_fields_in_TSE[14] ||
				shell_file->file_header.str.bit_fields_in_TSE[16]!=40 ||
				shell_file->file_header.str.bit_fields_in_TSE[17]!=47
			)) ||
			(shell_class==GRADIENT &&
			(	shell_file->file_header.str.bit_fields_in_TSE[14]!=40 ||
				shell_file->file_header.str.bit_fields_in_TSE[15]!=47 ||
				shell_file->file_header.str.bit_fields_in_TSE[17]>=
					shell_file->file_header.str.bit_fields_in_TSE[16]
			)) ||
			shell_file->file_header.str.num_of_integers_in_NTSE!=1 ||
			shell_file->file_header.str.num_of_bits_in_NTSE!=16 ||
			shell_file->file_header.str.min_max_coordinates_valid==0)
	{
		fclose(infile);
		destroy_file_header(&shell_file->file_header);
		free(shell_file);
		return (100);
	}

	strncpy(shell_file->file_header.gen.filename, file_name,
		sizeof(shell_file->file_header.gen.filename)-1);
	shell_file->file_header.gen.filename[
		sizeof(shell_file->file_header.gen.filename)-1] = 0;
	for (j=0; j<(int)sizeof(shell_file->file_header.gen.filename1); j++)
		;
	if (j < (int)sizeof(shell_file->file_header.gen.filename1))
		while ((j=(int)strlen(shell_file->file_header.gen.filename1)-1) &&
				shell_file->file_header.gen.filename1[j]==' ')
			shell_file->file_header.gen.filename1[j] = 0;
	if (shell_class!=DIRECT && shell_file->file_header.str.scene_file_valid)
		for (j=0; j<shell_file->file_header.str.num_of_structures; j++)
			while (shell_file->file_header.str.scene_file[j][0] &&
					shell_file->file_header.str.scene_file[j][
					strlen(shell_file->file_header.str.scene_file[j])-1] ==' ')
				shell_file->file_header.str.scene_file[j][
					strlen(shell_file->file_header.str.scene_file[j])-1] = 0;
	shell_file->reference = (Shell_data **)malloc(sizeof(Shell_data *)*
		(	from_plan
			?	num_of_objects
			:	shell_class==DIRECT
				? 1:shell_file->file_header.str.num_of_structures
		));
	if (shell_file->reference == NULL)
	{	report_malloc_error();
		fclose(infile);
		destroy_file_header(&shell_file->file_header);
		free(shell_file);
		return (1);
	}
	colormap_valid = FALSE;
	if (from_plan)
	{	shell_file->references = num_of_objects;
		for (shell_number=0; shell_number<num_of_objects; shell_number++)
		{	if (shell_numbers[shell_number] >=
					shell_file->file_header.str.num_of_structures)
			{
				fprintf(stderr, "Bad structure reference: %d\n",
					shell_numbers[shell_number]);
				fprintf(stderr, "%d structures in file.\n",
					shell_file->file_header.str.num_of_structures);
				fclose(infile);
				return (104);
			}
			error_code = load_shell(shell_file, infile, shell_number,
				shell_numbers[shell_number], last_shell, icon_flag);
			if (error_code)
			{
				fclose(infile);
				return (error_code);
			}
			last_shell = &(*last_shell)->next;
		}
	}
	else if (shell_class == DIRECT)
	{
		shell_file->references = shell_file->file_header.scn.dimension<4? 1:
			shell_file->file_header.scn.num_of_subscenes[0];
		for (shell_number=0; shell_number<shell_file->references;
				shell_number++)
		{	error_code = load_direct_data(shell_file, infile, shell_number,
				shell_number, last_shell, icon_flag);
			if (error_code)
			{
				fclose(infile);
				return (error_code);
			}
			last_shell = &(*last_shell)->next;
		}
	}
	else
	{	shell_file->references = shell_file->file_header.str.num_of_structures;
		for (shell_number=0; shell_number<shell_file->references;
				shell_number++)
		{	error_code = load_shell(shell_file, infile, shell_number,
				shell_number, last_shell, icon_flag);
			if (error_code)
			{
				fclose(infile);
				return (error_code);
			}
			last_shell = &(*last_shell)->next;
		}
	}
	fclose(infile);
	return (0);
}

/*****************************************************************************
 * FUNCTION: scn_slice_location
 * DESCRIPTION: Returns the location of a slice in a DIRECT object.
 * PARAMETERS:
 *    shell_data: The object data
 *    slice: The slice number (from 0)
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The parameters must be valid.
 * RETURN VALUE: The location of a slice in a DIRECT object.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 6/26/01 by Dewey Odhner
 *
 *****************************************************************************/
float scn_slice_location(Shell_data *shell_data, int slice)
{
	int s, v;

	if (shell_data->file->file_header.scn.dimension == 3)
	{
		assert(shell_data->shell_number == 0);
		return (shell_data->file->file_header.scn.loc_of_subscenes[slice]);
	}
	assert(shell_data->shell_number < 
		shell_data->file->file_header.scn.num_of_subscenes[0]);
	for (s=v=0; v<shell_data->shell_number; v++)
		s += shell_data->file->file_header.scn.num_of_subscenes[v+1];
	return (shell_data->file->file_header.scn.loc_of_subscenes[
		shell_data->file->file_header.scn.num_of_subscenes[0]+v+slice]);
}

/*****************************************************************************
 * FUNCTION: load_shell
 * DESCRIPTION: Loads one object from a file.
 * PARAMETERS:
 *    shell_file: The file information for the shell
 *    infile: The file pointer, must be open for reading.
 *    ref_number: The reference number from shell_file to this object
 *    shell_number: The object to be loaded. (0 = first object)
 *    last_shell: The address of the last pointer in the object list, unless
 *       icon_flag, then the last pointer in the object list after those with
 *       icons already loaded.
 *    icon_flag: Whether the shell is to be loaded as an icon.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: The global variable number_of_triangles must be
 *    initialized.  The global variable object_list must be valid or NULL.
 * RETURN VALUE: Non-zero if unsuccessful.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 10/29/92 by Dewey Odhner
 *    Modified: 12/4/92 to work with plan by Dewey Odhner
 *    Modified: 1/20/93 to handle bad max Y2 coordinate by Dewey Odhner
 *    Modified: 4/13/93 to use new data interface functions by Dewey Odhner
 *    Modified: 5/4/94 for fuzzy shells by Dewey Odhner
 *    Modified: 5/31/94 to fix negative rows assignment by Dewey Odhner
 *    Modified: 11/21/94 obj_data->ptr_table = NULL when freed by Dewey Odhner
 *    Modified: 1/17/95 to handle bad min Y2 coordinate by Dewey Odhner
 *    Modified: 10/18/00 items_read changed to int by Dewey Odhner
 *    Modified: 6/4/01 BINARY_B shell accommodated by Dewey Odhner
 *    Modified: 7/10/03 T_SHELL data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::load_shell(Shell_file *shell_file, FILE *infile,
	int ref_number, int shell_number, Shell **last_shell, int icon_flag)
{
	Shell_data *obj_data;
	short *ntse;
	double data_offset;
	int items_read, error_code, ptrs_in_table, j, num_of_TSE, k;
	unsigned short n1, n2, n3;

	if (*last_shell == NULL)
	{	*last_shell = (Shell *)calloc(1, sizeof(Shell));
		if (*last_shell == NULL)
		{	destroy_file_header(&shell_file->file_header);
			free(shell_file->reference);
			free(shell_file);
			return (1);
		}
		(*last_shell)->mobile = TRUE;
	}
	obj_data =
		icon_flag
		?	&(*last_shell)->icon_data
		:	&(*last_shell)->main_data;
	obj_data->shell_number = shell_number;
	obj_data->file = shell_file;
	shell_file->reference[ref_number] = obj_data;
	data_offset = 0;
	for (j=0; j<shell_number; j++)
		data_offset += 2*shell_file->file_header.str.num_of_NTSE[j]+
			shell_file->file_header.str.num_of_bits_in_TSE/8*
				(double)shell_file->file_header.str.num_of_TSE[j];
	for (j=0; j<shell_file->file_header.str.num_of_samples[0]; j++)
		if (shell_file->file_header.str.loc_of_samples[j] ==
				Min_coordinate(obj_data, 2))
			break;
	if (j >= shell_file->file_header.str.num_of_samples[0])
		return (104);
	for (obj_data->slices=1;
			obj_data->slices<=shell_file->file_header.str.num_of_samples[0];
			obj_data->slices++)
		if (shell_file->file_header.str.loc_of_samples[j+obj_data->slices-1] ==
				Max_coordinate(obj_data, 2))
			break;
	if (obj_data->slices > shell_file->file_header.str.num_of_samples[0])
		return (104);
	obj_data->rows = (int)rint((Max_coordinate(obj_data, 1)-
		Min_coordinate(obj_data, 1))/shell_file->file_header.str.xysize[1]+1);
	if (obj_data->rows < 0)
		obj_data->rows = 0;
	ptrs_in_table = obj_data->slices*obj_data->rows+1;
	obj_data->ptr_table =
		(unsigned short **)malloc(ptrs_in_table*sizeof(unsigned short *));
	if (obj_data->ptr_table == NULL)
		return (1);
	if (shell_file->file_header.str.num_of_NTSE[shell_number])
		ntse = (short *)malloc(sizeof(short)*
			shell_file->file_header.str.num_of_NTSE[shell_number]);
	else
	{	ntse = (short *)malloc(1);
		fprintf(stderr, "EMPTY SHELL!\n");
	}
	if (ntse == NULL)
	{	free(obj_data->ptr_table);
		obj_data->ptr_table = NULL;
		return (1);
	}
	error_code = VLSeekData(infile, data_offset);
	if (error_code)
	{	free(ntse);
		free(obj_data->ptr_table);
		obj_data->ptr_table = NULL;
		return (error_code);
	}
	error_code = VReadData(ntse, 2,
		shell_file->file_header.str.num_of_NTSE[shell_number], infile,
		&items_read);
	if (error_code==0 && items_read!=
			(int)shell_file->file_header.str.num_of_NTSE[shell_number])
		error_code = 2;
	if (error_code)
	{	free(ntse);
		free(obj_data->ptr_table);
		obj_data->ptr_table = NULL;
		return (error_code);
	}
	if (ntse[0] != obj_data->slices)
	{	free(ntse);
		free(obj_data->ptr_table);
		obj_data->ptr_table = NULL;
		return (100);
	}
	for (j=1; j<=obj_data->slices; j++)
		if (ntse[j] != obj_data->rows)
		{	free(ntse);
			free(obj_data->ptr_table);
			obj_data->ptr_table = NULL;
			return (100);
		}
	num_of_TSE = shell_file->file_header.str.num_of_TSE[shell_number];
	if (num_of_TSE == 0)
	{	obj_data->ptr_table[0] = NULL;
		obj_data->in_memory = TRUE;
	}
	else
	{	obj_data->ptr_table[0] = (unsigned short *)malloc(
			shell_file->file_header.str.num_of_bits_in_TSE/8*num_of_TSE);
		if (obj_data->ptr_table[0] == NULL)
			if (shell_file->file_header.str.num_of_bits_in_TSE==32 ||
					shell_file->file_header.gen.data_type==SHELL2 ||
					shell_file->file_header.str.bit_fields_in_TSE[9]==26)
			{	free(ntse);
				return (1);
			}
			else
			{	obj_data->ptr_table[0]=(unsigned short *)(size_t)ftell(infile);
				for (j=1; j<ptrs_in_table; j++)
					obj_data->ptr_table[j] =
						obj_data->ptr_table[j-1]+ntse[obj_data->slices+j]*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
				free(ntse);
				obj_data->in_memory = FALSE;
			}
		else if (shell_file->file_header.gen.data_type == SHELL2)
		{
			error_code = VReadData(obj_data->ptr_table[0], 1, num_of_TSE,
				infile, &items_read);
			if (error_code)
			{	free(obj_data->ptr_table[0]);
				free(obj_data->ptr_table);
				obj_data->ptr_table = NULL;
				return (error_code);
			}
			for (j=1; j<ptrs_in_table; j++)
			{
				obj_data->ptr_table[j] = obj_data->ptr_table[j-1];
				for (k=0; k<ntse[obj_data->slices+j]; k++)
					((unsigned char **)obj_data->ptr_table)[j] +=
						3+3*number_of_triangles[
							((unsigned char **)obj_data->ptr_table)[j][0]];
			}
			free(ntse);
			obj_data->in_memory = TRUE;
		}
		else
		{	for (j=1; j<ptrs_in_table; j++)
				obj_data->ptr_table[j] =
					obj_data->ptr_table[j-1]+ntse[obj_data->slices+j]*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
			free(ntse);
			error_code = VReadData(obj_data->ptr_table[0], 2,
				shell_file->file_header.str.num_of_bits_in_TSE/16*num_of_TSE,
				infile, &items_read);
			if (error_code==0 && items_read!=
					shell_file->file_header.str.num_of_bits_in_TSE/16*num_of_TSE)
				error_code = 2;
			if (error_code)
			{	free(obj_data->ptr_table[0]);
				free(obj_data->ptr_table);
				obj_data->ptr_table = NULL;
				return (error_code);
			}
			/* map normal codes for icon data */
			if (shell_file->file_header.str.bit_fields_in_TSE[6] == 23
					/* otherwise 21 */)
				for (j=1; j<num_of_TSE*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
						j+=shell_file->file_header.str.num_of_bits_in_TSE/16)
				{	n1 = obj_data->ptr_table[0][j]>>6;
					n2 = obj_data->ptr_table[0][j]>>3 & 7;
					n3 = obj_data->ptr_table[0][j] & 7;
					obj_data->ptr_table[0][j] = n1<<8 | n2<<5 | n3<<1;
				}
			else if (shell_file->file_header.str.bit_fields_in_TSE[11] < 31)
				for (j=1; j<num_of_TSE*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
						j+=shell_file->file_header.str.num_of_bits_in_TSE/16)
					obj_data->ptr_table[0][j] &= 0xffee;
			obj_data->in_memory = TRUE;
		}
	}
	return (0);
}

/*****************************************************************************
 * FUNCTION: load_direct_data
 * DESCRIPTION: Loads one object from a scene file.
 * PARAMETERS:
 *    shell_file: The file information for the shell
 *    infile: The file pointer, must be open for reading.
 *    ref_number: The reference number from shell_file to this object
 *    shell_number: The object to be loaded. (0 = first object)
 *    last_shell: The address of the last pointer in the object list, unless
 *       icon_flag, then the last pointer in the object list after those with
 *       icons already loaded.
 *    icon_flag: Whether the shell is to be loaded as an icon.
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: The global variable object_list must be valid or NULL.
 *    The global variable windows_open must be appropriately initialized.
 * RETURN VALUE: Non-zero if unsuccessful.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 7/11/01 by Dewey Odhner
 *    Modified: 9/3/02 check for abort by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::load_direct_data(Shell_file *shell_file, FILE *infile,
	int ref_number, int shell_number, Shell **last_shell, int icon_flag)
{
	Shell_data *obj_data;
	double data_offset;
	unsigned long volume_size;
	int items_read, error_code, ptrs_in_table, j, num_of_TSE, m, k;
	unsigned short n1, *data_buffer16=NULL;
	unsigned histogram[65536];
	unsigned char *data_buffer8;
	double max_g, gx, gy, gz, slice_spacing, gm;


#define Data16(x, y, z) data_buffer16[(obj_data->rows*(z)+(y))* \
	shell_file->file_header.scn.xysize[0]+(x)]

#define Data8(x, y, z) data_buffer8[(obj_data->rows*(z)+(y))* \
	shell_file->file_header.scn.xysize[0]+(x)]


	if (*last_shell == NULL)
	{	*last_shell = (Shell *)calloc(1, sizeof(Shell));
		if (*last_shell == NULL)
		{	destroy_file_header(&shell_file->file_header);
			free(shell_file->reference);
			free(shell_file);
			return (1);
		}
		(*last_shell)->mobile = TRUE;
	}
	obj_data =
		icon_flag
		?	&(*last_shell)->icon_data
		:	&(*last_shell)->main_data;
	obj_data->shell_number = shell_number;
	obj_data->file = shell_file;
	obj_data->rows = shell_file->file_header.scn.xysize[1];
	shell_file->reference[ref_number] = obj_data;
	data_offset = 0;
	for (j=0; j<shell_number; j++)
	{
		assert(shell_file->file_header.scn.dimension == 4);
		data_offset += shell_file->file_header.scn.num_of_bits/8*
			shell_file->file_header.scn.xysize[0]*obj_data->rows*
			(double)shell_file->file_header.scn.num_of_subscenes[j+1];
	}
	obj_data->slices = shell_file->file_header.scn.num_of_subscenes[
		shell_file->file_header.scn.dimension==4? shell_number+1:0];
	volume_size =
		shell_file->file_header.scn.xysize[0]*obj_data->rows*obj_data->slices;
	data_buffer8 = (unsigned char *)
		malloc(shell_file->file_header.scn.num_of_bits/8*volume_size);
	if (data_buffer8 == NULL)
		return (1);
	error_code = VLSeekData(infile, data_offset);
	if (error_code)
	{
		free(data_buffer8);
		return (error_code);
	}
	error_code = VReadData(data_buffer8, shell_file->file_header.scn.
		num_of_bits/8, volume_size, infile, &items_read);
	if (error_code)
	{
		free(data_buffer8);
		return (error_code);
	}
	memset(histogram, 0, sizeof(histogram));
	memset(obj_data->threshold, 0, sizeof(obj_data->threshold));
	if (shell_file->file_header.scn.num_of_bits == 16)
	{
		data_buffer16 = (unsigned short *)data_buffer8;
		for (j=0; (unsigned)j<volume_size; j++)
			histogram[data_buffer16[j]]++;
	}
	else
		for (j=0; (unsigned)j<volume_size; j++)
			histogram[data_buffer8[j]]++;
	for (k=m=j=0; (unsigned)m<volume_size; j++)
	{
		m += histogram[j];
		while (k<6 && obj_data->threshold[k]==0 &&
				(unsigned)m>=volume_size/10*(k+5))
			obj_data->threshold[k++] = j;
	}
	if (!shell_file->file_header.scn.largest_density_value_valid)
		shell_file->file_header.scn.largest_density_value =
			(float *)malloc(sizeof(float));
	if (shell_file->file_header.scn.largest_density_value == NULL)
	{
		free(data_buffer8);
		return (1);
	}
	shell_file->file_header.scn.largest_density_value[0] =
		(float)obj_data->threshold[5];
	shell_file->file_header.scn.largest_density_value_valid = 1;
	max_g =
		.4*(1+2*SQRT2+4/SQRT3)*(obj_data->threshold[5]-obj_data->threshold[0]);
	slice_spacing = Slice_spacing(obj_data);
	num_of_TSE = 0;
	for (k=obj_data->threshold[0]; k<j; k++)
		num_of_TSE += histogram[k];
	ptrs_in_table = obj_data->slices*obj_data->rows+1;
	obj_data->ptr_table =
		(unsigned short **)malloc(ptrs_in_table*sizeof(unsigned short *));
	if (obj_data->ptr_table == NULL)
	{
		free(data_buffer8);
		return (1);
	}
	obj_data->ptr_table[0] = (unsigned short *)malloc(6*num_of_TSE);
	if (obj_data->ptr_table[0] == NULL)
	{
		free(obj_data->ptr_table);
		obj_data->ptr_table = NULL;
		free(data_buffer8);
		return (1);
	}
	obj_data->bounds[0][0] = shell_file->file_header.scn.xysize[0];
	obj_data->bounds[1][0] = obj_data->rows;
	obj_data->bounds[2][0] = obj_data->slices;
	obj_data->bounds[0][1]= obj_data->bounds[1][1]= obj_data->bounds[2][1]= -1;
	if (shell_file->file_header.scn.num_of_bits == 16)
		for (m=0; m<obj_data->slices; m++)
		{
			for (k=0; k<obj_data->rows; k++)
			{
				obj_data->ptr_table[obj_data->rows*m+k+1] =
					obj_data->ptr_table[obj_data->rows*m+k];
				for (j=0; j<shell_file->file_header.scn.xysize[0]; j++)
				{
					if (Data16(j, k, m) >= obj_data->threshold[0])
					{
					  if (j > obj_data->bounds[0][1])
					    obj_data->bounds[0][1] = j;
					  if (j < obj_data->bounds[0][0])
					    obj_data->bounds[0][0] = j;
					  if (k > obj_data->bounds[1][1])
					    obj_data->bounds[1][1] = k;
					  if (k < obj_data->bounds[1][0])
					    obj_data->bounds[1][0] = k;
					  if (m > obj_data->bounds[2][1])
					    obj_data->bounds[2][1] = m;
					  if (m < obj_data->bounds[2][0])
					    obj_data->bounds[2][0] = m;
					  obj_data->ptr_table[obj_data->rows*m+k+1][2] =
						Data16(j, k, m);
					  if (m == 0)
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k, m)+
							Data16(j+1, k, m+1)-Data16(j, k+1, m)+
							Data16(j+1, k+1, m)-Data16(j, k, m+1)+
							Data16(j+1, k, m)-Data16(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j+1, k, m)+
							Data16(j+1, k+1, m)-Data16(j, k, m+1)+
							Data16(j, k+1, m)-Data16(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j+1, k, m)+
							Data16(j+1, k, m+1)-Data16(j, k+1, m)+
							Data16(j, k, m+1)-Data16(j+1, k+1, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j, k, m+1)-Data16(j-1, k+1, m)+
							Data16(j, k+1, m)-Data16(j-1, k, m+1)+
							Data16(j, k, m)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j-1, k, m+1)+
							Data16(j-1, k+1, m)-Data16(j, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j-1, k+1, m)+
							Data16(j-1, k, m+1)-Data16(j, k+1, m)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k+1, m)+
							Data16(j+1, k+1, m)-Data16(j-1, k, m+1)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k, m)+
							Data16(j+1, k+1, m)-Data16(j-1, k, m+1)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k+1, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k+1, m)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k-1, m+1)+
							Data16(j+1, k-1, m)-Data16(j, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k, m)-Data16(j, k-1, m+1)+
							Data16(j, k, m)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j, k, m)+
							Data16(j, k-1, m+1)-Data16(j+1, k, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k-1, m+1)+
							Data16(j, k-1, m)-Data16(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k, m)-Data16(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k-1, m+1)-Data16(j, k, m)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(Data16(j+1, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k-1, m+1)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k, m)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k-1, m+1)-Data16(j+1, k, m)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j, k+1, m)+
							Data16(j+1, k+1, m)-Data16(j, k-1, m+1)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k+1, m)-Data16(j, k-1, m+1)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k, m)+
							Data16(j, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j, k+1, m)+
							Data16(j, k-1, m+1)-Data16(j+1, k+1, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j-1, k+1, m)+
							Data16(j, k+1, m)-Data16(j-1, k-1, m+1)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(Data16(j, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j-1, k+1, m)+
							Data16(j-1, k-1, m+1)-Data16(j, k+1, m)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k+1, m)+
							Data16(j+1, k+1, m)-Data16(j-1, k-1, m+1)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k+1, m)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m+1)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k, m)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m))+
							1/SQRT3*(4*(Data16(j, k, m+1)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k-1, m)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k+1, m)+
							Data16(j-1, k-1, m+1)-Data16(j+1, k+1, m)));
						  }
					  else if (m == obj_data->slices-1)
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m-1)+
							Data16(j+1, k, m)-Data16(j, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k, m)+
							Data16(j+1, k, m-1)-Data16(j, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m)-Data16(j+1, k, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k, m)+
							Data16(j, k+1, m-1)-Data16(j+1, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m)-Data16(j+1, k, m-1)+
							Data16(j+1, k, m)-Data16(j, k+1, m-1)+
							Data16(j, k, m)-Data16(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j, k, m)-Data16(j-1, k+1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k, m)+
							Data16(j, k, m-1)-Data16(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m-1)-Data16(j, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k, m)-Data16(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(Data16(j+1, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k, m)+
							Data16(j+1, k, m-1)-Data16(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m-1)-Data16(j+1, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k+1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m-1)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k+1, m-1)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m-1)-Data16(j, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k-1, m)+
							Data16(j, k, m-1)-Data16(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j, k, m-1)+
							Data16(j, k-1, m)-Data16(j+1, k, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m-1)-Data16(j-1, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m-1)-Data16(j, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k-1, m)-Data16(j, k, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(Data16(j+1, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m-1)-Data16(j-1, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m-1)-Data16(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k-1, m)-Data16(j+1, k, m-1)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m-1)-Data16(j, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(Data16(j+1, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k-1, m)+
							Data16(j, k+1, m-1)-Data16(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j, k, m-1)+
							Data16(j, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m-1)+
							Data16(j, k-1, m)-Data16(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m-1)-Data16(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(Data16(j, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m-1)-Data16(j, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k-1, m)-Data16(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m))+
							1/SQRT3*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m-1)-Data16(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m))+
							1/SQRT3*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m-1)-Data16(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data16(j, k, m)-Data16(j, k, m-1))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k, m)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m)-Data16(j, k+1, m-1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k, m-1))+
							Data16(j+1, k+1, m)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k-1, m)-Data16(j+1, k+1, m-1)));
						  }
					  else
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j+1, k, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j+1, k, m+1)-Data16(j, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k, m+1)+
							Data16(j+1, k, m-1)-Data16(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j+1, k, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k, m+1)+
							Data16(j, k+1, m-1)-Data16(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j+1, k, m-1)+
							Data16(j+1, k, m+1)-Data16(j, k+1, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j, k, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j, k, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k, m+1)+
							Data16(j, k, m-1)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k, m+1)+
							Data16(j-1, k+1, m-1)-Data16(j, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*(Data16(j, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k, m+1)+
							Data16(j+1, k, m-1)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k+1, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j-1, k, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k, m)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j, k+1, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k, m+1)+
							Data16(j-1, k+1, m-1)-Data16(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k, m-1)+
							Data16(j+1, k, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k+1, m-1)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k-1, m+1)+
							Data16(j+1, k-1, m-1)-Data16(j, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k-1, m+1)+
							Data16(j, k, m-1)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k, m-1))+
							1/SQRT3*(Data16(j+1, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j, k, m-1)+
							Data16(j, k-1, m+1)-Data16(j+1, k, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j, k-1, m-1)-Data16(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k, m-1)-Data16(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k, m-1))+
							1/SQRT3*(Data16(j, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k-1, m+1)-Data16(j, k, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*(Data16(j+1, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j+1, k-1, m-1)-Data16(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data16(j, k, m)-Data16(j, k-1, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k, m)-Data16(j+1, k-1, m)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j, k-1, m))+
							Data16(j+1, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k, m-1)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k, m-1))+
							1/SQRT3*(Data16(j+1, k, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k-1, m+1)-Data16(j+1, k, m-1)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data16(j+1, k, m)-Data16(j, k, m))+
							1/SQRT2*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j, k, m+1))+
							1/SQRT3*(4*(Data16(j+1, k, m)-Data16(j, k, m))+
							Data16(j+1, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k-1, m+1)+
							Data16(j+1, k-1, m-1)-Data16(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j, k-1, m+1)+
							Data16(j, k+1, m-1)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*(Data16(j+1, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j, k+1, m-1)+
							Data16(j, k-1, m+1)-Data16(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data16(j, k, m)-Data16(j-1, k, m))+
							1/SQRT2*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*(4*(Data16(j, k, m)-Data16(j-1, k, m))+
							Data16(j, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j, k-1, m-1)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*
							(Data16(j, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k+1, m-1)-Data16(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*(Data16(j, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k-1, m+1)-Data16(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data16(j+1, k, m)-Data16(j-1, k, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j+1, k-1, m)-Data16(j-1, k+1, m)+
							Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j+1, k, m-1)-Data16(j-1, k, m+1))+
							1/SQRT3*
							(Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j+1, k-1, m-1)-Data16(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data16(j, k+1, m)-Data16(j, k-1, m)+
							1/SQRT2*(Data16(j+1, k+1, m)-Data16(j-1, k-1, m)+
							Data16(j-1, k+1, m)-Data16(j+1, k-1, m)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k+1, m-1)-Data16(j, k-1, m+1))+
							1/SQRT3*
							(Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k+1, m-1)-Data16(j-1, k-1, m+1)+
							Data16(j-1, k+1, m-1)-Data16(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data16(j, k, m+1)-Data16(j, k, m-1)+
							1/SQRT2*(Data16(j+1, k, m+1)-Data16(j-1, k, m-1)+
							Data16(j-1, k, m+1)-Data16(j+1, k, m-1)+
							Data16(j, k+1, m+1)-Data16(j, k-1, m-1)+
							Data16(j, k-1, m+1)-Data16(j, k+1, m-1))+
							1/SQRT3*
							(Data16(j+1, k+1, m+1)-Data16(j-1, k-1, m-1)+
							Data16(j-1, k+1, m+1)-Data16(j+1, k-1, m-1)+
							Data16(j+1, k-1, m+1)-Data16(j-1, k+1, m-1)+
							Data16(j-1, k-1, m+1)-Data16(j+1, k+1, m-1)));
						  }
					  gm = sqrt(gx*gx+gy*gy+gz*gz);
					  if (gm > max_g)
					    gm = max_g;
					  n1 = (unsigned short)rint(255/max_g*gm);
					  obj_data->ptr_table[obj_data->rows*m+k+1][1] =
					    G_code(gx, gy, gz) | (n1&7)<<13;
					  obj_data->ptr_table[obj_data->rows*m+k+1][0] =
					    (n1&0xf8)<<8 | j;
					  obj_data->ptr_table[obj_data->rows*m+k+1] += 3;
					}
				}
			}
		}
	else /* if (shell_file->file_header.scn.num_of_bits == 8) */
		for (m=0; m<obj_data->slices; m++)
		{
			for (k=0; k<obj_data->rows; k++)
			{
				obj_data->ptr_table[obj_data->rows*m+k+1] =
					obj_data->ptr_table[obj_data->rows*m+k];
				for (j=0; j<shell_file->file_header.scn.xysize[0]; j++)
				{
					if (Data8(j, k, m) >= obj_data->threshold[0])
					{
					  if (j > obj_data->bounds[0][1])
					    obj_data->bounds[0][1] = j;
					  if (j < obj_data->bounds[0][0])
					    obj_data->bounds[0][0] = j;
					  if (k > obj_data->bounds[1][1])
					    obj_data->bounds[1][1] = k;
					  if (k < obj_data->bounds[1][0])
					    obj_data->bounds[1][0] = k;
					  if (m > obj_data->bounds[2][1])
					    obj_data->bounds[2][1] = m;
					  if (m < obj_data->bounds[2][0])
					    obj_data->bounds[2][0] = m;
					  obj_data->ptr_table[obj_data->rows*m+k+1][2] =
						Data8(j, k, m);
					  if (m == 0)
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k, m)+
							Data8(j+1, k, m+1)-Data8(j, k+1, m)+
							Data8(j+1, k+1, m)-Data8(j, k, m+1)+
							Data8(j+1, k, m)-Data8(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j+1, k, m)+
							Data8(j+1, k+1, m)-Data8(j, k, m+1)+
							Data8(j, k+1, m)-Data8(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j+1, k, m)+
							Data8(j+1, k, m+1)-Data8(j, k+1, m)+
							Data8(j, k, m+1)-Data8(j+1, k+1, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j, k, m+1)-Data8(j-1, k+1, m)+
							Data8(j, k+1, m)-Data8(j-1, k, m+1)+
							Data8(j, k, m)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j-1, k, m+1)+
							Data8(j-1, k+1, m)-Data8(j, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j-1, k+1, m)+
							Data8(j-1, k, m+1)-Data8(j, k+1, m)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k+1, m)+
							Data8(j+1, k+1, m)-Data8(j-1, k, m+1)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k, m)+
							Data8(j+1, k+1, m)-Data8(j-1, k, m+1)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k+1, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k+1, m)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k-1, m+1)+
							Data8(j+1, k-1, m)-Data8(j, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k, m)-Data8(j, k-1, m+1)+
							Data8(j, k, m)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j, k, m)+
							Data8(j, k-1, m+1)-Data8(j+1, k, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k-1, m+1)+
							Data8(j, k-1, m)-Data8(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k, m)-Data8(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k-1, m+1)-Data8(j, k, m)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(Data8(j+1, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k-1, m+1)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k, m)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k-1, m+1)-Data8(j+1, k, m)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j, k+1, m)+
							Data8(j+1, k+1, m)-Data8(j, k-1, m+1)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k+1, m)-Data8(j, k-1, m+1)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k, m)+
							Data8(j, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j, k+1, m)+
							Data8(j, k-1, m+1)-Data8(j+1, k+1, m)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j-1, k+1, m)+
							Data8(j, k+1, m)-Data8(j-1, k-1, m+1)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(Data8(j, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j-1, k+1, m)+
							Data8(j-1, k-1, m+1)-Data8(j, k+1, m)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k+1, m)+
							Data8(j+1, k+1, m)-Data8(j-1, k-1, m+1)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k+1, m)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m+1)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k, m)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m))+
							1/SQRT3*(4*(Data8(j, k, m+1)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k-1, m)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k+1, m)+
							Data8(j-1, k-1, m+1)-Data8(j+1, k+1, m)));
						  }
					  else if (m == obj_data->slices-1)
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m-1)+
							Data8(j+1, k, m)-Data8(j, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k, m)+
							Data8(j+1, k, m-1)-Data8(j, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m)-Data8(j+1, k, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k, m)+
							Data8(j, k+1, m-1)-Data8(j+1, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m)-Data8(j+1, k, m-1)+
							Data8(j+1, k, m)-Data8(j, k+1, m-1)+
							Data8(j, k, m)-Data8(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j, k, m)-Data8(j-1, k+1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k, m)+
							Data8(j, k, m-1)-Data8(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m-1)-Data8(j, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k, m)-Data8(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(Data8(j+1, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k, m)+
							Data8(j+1, k, m-1)-Data8(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m-1)-Data8(j+1, k, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k+1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m-1)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k+1, m-1)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m-1)-Data8(j, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k-1, m)+
							Data8(j, k, m-1)-Data8(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j, k, m-1)+
							Data8(j, k-1, m)-Data8(j+1, k, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m-1)-Data8(j-1, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m-1)-Data8(j, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k-1, m)-Data8(j, k, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(Data8(j+1, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m-1)-Data8(j-1, k, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m-1)-Data8(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k-1, m)-Data8(j+1, k, m-1)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m-1)-Data8(j, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(Data8(j+1, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k-1, m)+
							Data8(j, k+1, m-1)-Data8(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j, k, m-1)+
							Data8(j, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m-1)+
							Data8(j, k-1, m)-Data8(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m-1)-Data8(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(Data8(j, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m-1)-Data8(j, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k-1, m)-Data8(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m))+
							1/SQRT3*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m-1)-Data8(j-1, k+1, m));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m))+
							1/SQRT3*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m-1)-Data8(j+1, k-1, m)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							2*(Data8(j, k, m)-Data8(j, k, m-1))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k, m)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m)-Data8(j, k+1, m-1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k, m-1))+
							Data8(j+1, k+1, m)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k-1, m)-Data8(j+1, k+1, m-1)));
						  }
					  else
						if (k == 0)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j+1, k, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j+1, k, m+1)-Data8(j, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k, m+1)+
							Data8(j+1, k, m-1)-Data8(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j+1, k, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k, m+1)+
							Data8(j, k+1, m-1)-Data8(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j+1, k, m-1)+
							Data8(j+1, k, m+1)-Data8(j, k+1, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j, k, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j, k, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k, m+1)+
							Data8(j, k, m-1)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k, m+1)+
							Data8(j-1, k+1, m-1)-Data8(j, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*(Data8(j, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k, m+1)+
							Data8(j+1, k, m-1)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k+1, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j-1, k, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k, m)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j, k+1, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k, m+1)+
							Data8(j-1, k+1, m-1)-Data8(j+1, k, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k, m-1)+
							Data8(j+1, k, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k+1, m-1)));
						  }
						else if (k == obj_data->rows-1)
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k-1, m+1)+
							Data8(j+1, k-1, m-1)-Data8(j, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k-1, m+1)+
							Data8(j, k, m-1)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k, m-1))+
							1/SQRT3*(Data8(j+1, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j, k, m-1)+
							Data8(j, k-1, m+1)-Data8(j+1, k, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j, k-1, m-1)-Data8(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k, m-1)-Data8(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k, m-1))+
							1/SQRT3*(Data8(j, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k-1, m+1)-Data8(j, k, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*(Data8(j+1, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j+1, k-1, m-1)-Data8(j-1, k, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							2*(Data8(j, k, m)-Data8(j, k-1, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k, m)-Data8(j+1, k-1, m)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j, k-1, m))+
							Data8(j+1, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k, m-1)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k, m-1))+
							1/SQRT3*(Data8(j+1, k, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k-1, m+1)-Data8(j+1, k, m-1)));
						  }
						else
						  if (j == 0)
						  {
						   gx= 2*(Data8(j+1, k, m)-Data8(j, k, m))+
							1/SQRT2*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j, k, m+1))+
							1/SQRT3*(4*(Data8(j+1, k, m)-Data8(j, k, m))+
							Data8(j+1, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k-1, m+1)+
							Data8(j+1, k-1, m-1)-Data8(j, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j, k-1, m+1)+
							Data8(j, k+1, m-1)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*(Data8(j+1, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j, k+1, m-1)+
							Data8(j, k-1, m+1)-Data8(j+1, k+1, m-1)));
						  }
						  else if (j== shell_file->file_header.scn.xysize[0]-1)
						  {
						   gx= 2*(Data8(j, k, m)-Data8(j-1, k, m))+
							1/SQRT2*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*(4*(Data8(j, k, m)-Data8(j-1, k, m))+
							Data8(j, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j, k-1, m-1)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*
							(Data8(j, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k+1, m-1)-Data8(j, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*(Data8(j, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k-1, m+1)-Data8(j, k+1, m-1)));
						  }
						  else
						  {
						   gx= Data8(j+1, k, m)-Data8(j-1, k, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j+1, k-1, m)-Data8(j-1, k+1, m)+
							Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j+1, k, m-1)-Data8(j-1, k, m+1))+
							1/SQRT3*
							(Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j+1, k-1, m-1)-Data8(j-1, k+1, m+1));
						   gy= (shell_file->file_header.scn.xypixsz[0]/
						    shell_file->file_header.scn.xypixsz[1])*(
							Data8(j, k+1, m)-Data8(j, k-1, m)+
							1/SQRT2*(Data8(j+1, k+1, m)-Data8(j-1, k-1, m)+
							Data8(j-1, k+1, m)-Data8(j+1, k-1, m)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k+1, m-1)-Data8(j, k-1, m+1))+
							1/SQRT3*
							(Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k+1, m-1)-Data8(j-1, k-1, m+1)+
							Data8(j-1, k+1, m-1)-Data8(j+1, k-1, m+1)));
						   gz= (shell_file->file_header.scn.xypixsz[0]/
						    slice_spacing)*(
							Data8(j, k, m+1)-Data8(j, k, m-1)+
							1/SQRT2*(Data8(j+1, k, m+1)-Data8(j-1, k, m-1)+
							Data8(j-1, k, m+1)-Data8(j+1, k, m-1)+
							Data8(j, k+1, m+1)-Data8(j, k-1, m-1)+
							Data8(j, k-1, m+1)-Data8(j, k+1, m-1))+
							1/SQRT3*
							(Data8(j+1, k+1, m+1)-Data8(j-1, k-1, m-1)+
							Data8(j-1, k+1, m+1)-Data8(j+1, k-1, m-1)+
							Data8(j+1, k-1, m+1)-Data8(j-1, k+1, m-1)+
							Data8(j-1, k-1, m+1)-Data8(j+1, k+1, m-1)));
						  }
					  gm = sqrt(gx*gx+gy*gy+gz*gz);
					  if (gm > max_g)
					    gm = max_g;
					  n1 = (unsigned short)rint(255/max_g*gm);
					  obj_data->ptr_table[obj_data->rows*m+k+1][1] =
					    G_code(gx, gy, gz) | (n1&7)<<13;
					  obj_data->ptr_table[obj_data->rows*m+k+1][0] =
					    (n1&0xf8)<<8 | j;
					  obj_data->ptr_table[obj_data->rows*m+k+1] += 3;
					}
				}
			}
		}
	free(data_buffer8);
	obj_data->in_memory = TRUE;
	return (0);
}

/*****************************************************************************
 * FUNCTION: destroy_file_header
 * DESCRIPTION: Frees the memory occupied by a Shell data file header.
 * PARAMETERS:
 *    file_header: The file header to be destroyed, must point to a valid
 *       ViewnixHeader for a SHELL0 file.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/15/93 for new header changes by Dewey Odhner
 *    Modified: 7/5/94 to free str.scene_file by Dewey Odhner
 *    Modified: 7/5/01 to free file_header->scn fields by Dewey Odhner
 *
 *****************************************************************************/
void destroy_file_header(ViewnixHeader *file_header)
{
	if (file_header->gen.description)
		free(file_header->gen.description);
	if (file_header->gen.comment)
		free(file_header->gen.comment);
	if (file_header->gen.imaged_nucleus)
		free(file_header->gen.imaged_nucleus);
	if (file_header->gen.gray_lookup_data)
		free(file_header->gen.gray_lookup_data);
	if (file_header->gen.red_lookup_data)
		free(file_header->gen.red_lookup_data);
	if (file_header->gen.green_lookup_data)
		free(file_header->gen.green_lookup_data);
	if (file_header->gen.blue_lookup_data)
		free(file_header->gen.blue_lookup_data);
	if (file_header->gen.data_type == IMAGE0)
	{
		if (file_header->scn.domain)
			free(file_header->scn.domain);
		if (file_header->scn.axis_label)
			free(file_header->scn.axis_label);
		if (file_header->scn.measurement_unit)
			free(file_header->scn.measurement_unit);
		if (file_header->scn.density_measurement_unit)
			free(file_header->scn.density_measurement_unit);
		if (file_header->scn.smallest_density_value)
			free(file_header->scn.smallest_density_value);
		if (file_header->scn.largest_density_value)
			free(file_header->scn.largest_density_value);
		if (file_header->scn.signed_bits)
			free(file_header->scn.signed_bits);
		if (file_header->scn.bit_fields)
			free(file_header->scn.bit_fields);
		if (file_header->scn.num_of_subscenes)
			free(file_header->scn.num_of_subscenes);
		if (file_header->scn.loc_of_subscenes)
			free(file_header->scn.loc_of_subscenes);
		if (file_header->scn.description)
			free(file_header->scn.description);
	}
	else
	{
		if (file_header->str.domain)
			free(file_header->str.domain);
		if (file_header->str.axis_label)
			free(file_header->str.axis_label);
		if (file_header->str.measurement_unit)
			free(file_header->str.measurement_unit);
		if (file_header->str.num_of_TSE)
			free(file_header->str.num_of_TSE);
		if (file_header->str.num_of_NTSE)
			free(file_header->str.num_of_NTSE);
		if (file_header->str.NTSE_measurement_unit)
			free(file_header->str.NTSE_measurement_unit);
		if (file_header->str.TSE_measurement_unit)
			free(file_header->str.TSE_measurement_unit);
		if (file_header->str.smallest_value)
			free(file_header->str.smallest_value);
		if (file_header->str.largest_value)
			free(file_header->str.largest_value);
		if (file_header->str.signed_bits_in_TSE)
			free(file_header->str.signed_bits_in_TSE);
		if (file_header->str.bit_fields_in_TSE)
			free(file_header->str.bit_fields_in_TSE);
		if (file_header->str.signed_bits_in_NTSE)
			free(file_header->str.signed_bits_in_NTSE);
		if (file_header->str.bit_fields_in_NTSE)
			free(file_header->str.bit_fields_in_NTSE);
		if (file_header->str.num_of_samples)
			free(file_header->str.num_of_samples);
		if (file_header->str.loc_of_samples)
			free(file_header->str.loc_of_samples);
		if (file_header->str.description_of_element)
			free(file_header->str.description_of_element);
		if (file_header->str.parameter_vectors)
			free(file_header->str.parameter_vectors);
		if (file_header->str.min_max_coordinates)
			free(file_header->str.min_max_coordinates);
		if (file_header->str.volume)
			free(file_header->str.volume);
		if (file_header->str.surface_area)
			free(file_header->str.surface_area);
		if (file_header->str.rate_of_change_volume)
			free(file_header->str.rate_of_change_volume);
		if (file_header->str.description)
			free(file_header->str.description);
		if (file_header->str.scene_file)
			free(file_header->str.scene_file);
	}
}

/*****************************************************************************
 * FUNCTION: invalidate_main_images
 * DESCRIPTION: Marks all the main object images bad.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables object_list, image_valid, image2_valid,
 *    may be changed.
 * ENTRY CONDITIONS: The global variable object_list must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::invalidate_main_images(void)
{
	Shell *object;

	image_valid = image2_valid = FALSE;
	for (object=object_list; object!=NULL; object=object->next)
	{	object->O.main_image.projected = BAD;
		if (object->reflection)
			object->reflection->main_image.projected = BAD;
	}
}

/*****************************************************************************
 * FUNCTION: compute_diameter
 * DESCRIPTION: Computes the diameter of (the bounding box of) an object.
 * PARAMETERS:
 *    obj: The object whose diameter to compute
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: obj must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/22/93 by Dewey Odhner
 *    Modified: 3/3/93 to take icon into account by Dewey Odhner
 *    Modified: 9/28/93 to ignore icon if not present by Dewey Odhner
 *    Modified: 10/19/93 to handle units by Dewey Odhner
 *    Modified: 5/31/94 to correct negative rows or slices by Dewey Odhner
 *    Modified: 7/5/01 for DIRECT data by Dewey Odhner
 *
 *****************************************************************************/
void compute_diameter(Shell *obj, int icons_exist)
{
	double width, length, depth, icon_diameter, unit;

	unit = unit_mm(&obj->main_data);
	if (obj->main_data.file->file_header.gen.data_type == IMAGE0)
	{
		width = (obj->main_data.bounds[0][1]-obj->main_data.bounds[0][0]+1)*
			obj->main_data.file->file_header.scn.xypixsz[0]*unit;
		length = obj->main_data.rows*
			obj->main_data.file->file_header.scn.xypixsz[1]*unit;
		depth = obj->main_data.slices*Slice_spacing(&obj->main_data)*unit;
	}
	else
	{
		width = (Largest_y1(&obj->main_data)-Smallest_y1(&obj->main_data)+1)*
			obj->main_data.file->file_header.str.xysize[0]*unit;
		length = obj->main_data.rows*
			obj->main_data.file->file_header.str.xysize[1]*unit;
		depth = obj->main_data.slices*Slice_spacing(&obj->main_data)*unit;
	}
	obj->diameter = sqrt(length*length+width*width+depth*depth);
	if (obj->main_data.rows<=0 || obj->main_data.slices<=0)
		obj->diameter = 0;
	if (!icons_exist || obj->icon_data.file==NULL)
		return;
	unit = unit_mm(&obj->icon_data);
	if (obj->icon_data.file->file_header.gen.data_type == IMAGE0)
	{
		width = (obj->icon_data.bounds[0][1]-obj->icon_data.bounds[0][0]+1)*
			obj->icon_data.file->file_header.scn.xypixsz[0]*unit;
		length = obj->icon_data.rows*
			obj->icon_data.file->file_header.scn.xypixsz[1]*unit;
		depth = obj->icon_data.slices*Slice_spacing(&obj->icon_data)*unit;
	}
	else
	{
		width = (Largest_y1(&obj->icon_data)-Smallest_y1(&obj->icon_data)+1)*
			obj->icon_data.file->file_header.str.xysize[0]*unit;
		length = obj->icon_data.rows*
			obj->icon_data.file->file_header.str.xysize[1]*unit;
		depth = obj->icon_data.slices*Slice_spacing(&obj->icon_data)*unit;
	}
	icon_diameter = sqrt(length*length+width*width+depth*depth);
	if (obj->icon_data.rows<=0 || obj->icon_data.slices<=0)
		icon_diameter = 0;
	if (icon_diameter > obj->diameter)
		obj->diameter = icon_diameter;
}

/*****************************************************************************
 * FUNCTION: destroy_object
 * DESCRIPTION: Frees the memory occupied by an object.
 * PARAMETERS:
 *    object: The object to be destroyed, must point to a valid Shell except
 *       that the 'next' field is ignored. (A valid Shell must have its
 *       'reflection' point to a valid Virtual_object or be NULL, and must have
 *       its 'main_data.file' and 'icon_data.file' point to valid Shell_file
 *       structures.)
 * SIDE EFFECTS: May alter the file information shared with another object.
 *    If the object was on, the global variables image_valid, icon_valid,
 *    image2_valid will be cleared.  overlay_bad may be changed.
 * ENTRY CONDITIONS: The global variables box, overlay_bad should be
 *    properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 4/26/94 to set overlay_bad by Dewey Odhner
 *    Modified: 2/28/97 object->mark freed by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_object(Shell *object)
{
	destroy_object_data(&object->main_data);
	destroy_object_data(&object->icon_data);
	destroy_virtual_object(&object->O);
	if (object->reflection)
	{	destroy_virtual_object(object->reflection);
		free(object->reflection);
	}
	if (object->mark)
		free(object->mark);
	free(object);
}

/*****************************************************************************
 * FUNCTION: destroy_object_data
 * DESCRIPTION: Frees the memory occupied by a Shell data structure.
 * PARAMETERS:
 *    object_data: The object data to be destroyed, must point to a valid
 *       Shell_data structure.
 * SIDE EFFECTS: May alter the file information shared with another object.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/31/94 to check object_data->ptr_table[0] by Dewey Odhner
 *
 *****************************************************************************/
void destroy_object_data(Shell_data *object_data)
{
	int j;

	if (object_data->in_memory && object_data->ptr_table[0])
		free(object_data->ptr_table[0]);
	if (object_data->ptr_table)
		free(object_data->ptr_table);
	if (object_data->file)
	{	object_data->file->references--;
		object_data->file->file_header.gen.filename_valid = FALSE;
		if (object_data->file->references > 0)
		{	for (j=0; object_data->file->reference[j]!=object_data; j++)
				;
			for (; j<object_data->file->references; j++)
				object_data->file->reference[j] =
					object_data->file->reference[j+1];
		}
		else
		{	destroy_file_header(&object_data->file->file_header);
			free(object_data->file->reference);
			free(object_data->file);
		}
	}
}

/*****************************************************************************
 * FUNCTION: destroy_virtual_object
 * DESCRIPTION: Frees the memory occupied by a Virtual_object.
 * PARAMETERS:
 *    object: The virtual object to be destroyed, must point to a valid
 *       Virtual_object.
 * SIDE EFFECTS: If the virtual object was on, the global variables
 *    image_valid, icon_valid, image2_valid will be cleared.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::destroy_virtual_object(Virtual_object *object)
{
	if (object->on)
		image_valid = icon_valid = image2_valid = FALSE;
	destroy_object_image(&object->main_image);
	destroy_object_image(&object->icon);
}

/*****************************************************************************
 * FUNCTION: compute_v_object_color_table
 * DESCRIPTION: Initializes the global varible v_object_color_table if
 *    objects are non-binary.
 * PARAMETERS: None
 * SIDE EFFECTS: The global variables image_valid, image2_valid, icon_valid
 *    may be cleared.  An error message may be displayed.
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The global variables global_level, global_width, argv0, windows_open,
 *    object_list must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 11/14/94 by Dewey Odhner
 *    Modified: 6/11/96 max changed for percent class by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::compute_v_object_color_table(void)
{
	int shaden, oshade, max;
	float base, slope;

	if (v_object_color_table==NULL && st_cl(&object_list->main_data)!=BINARY_A &&
			 st_cl(&object_list->main_data)!=BINARY_B)
	{	v_object_color_table =
			(V_color_table_row *)malloc(sizeof(V_color_table_row));
		if (v_object_color_table == NULL)
		{	report_malloc_error();
			return;
		}
	}
	max=st_cl(&object_list->main_data)==PERCENT||
		st_cl(&object_list->main_data)==DIRECT? 255:OBJECT_IMAGE_BACKGROUND-1;
	if (v_object_color_table == NULL)
		return;
	base = (float)
		((V_OBJECT_IMAGE_BACKGROUND-1)*.01*(global_level-.5*global_width));
	slope = (float)((max*100./(V_OBJECT_IMAGE_BACKGROUND-1))/global_width);
	for (oshade=0; oshade<V_OBJECT_IMAGE_BACKGROUND; oshade++)
	{	shaden = (int)((oshade-base)*slope);
		if (shaden < 0)
			shaden = 0;
		if (shaden > max)
			shaden = max;
		v_object_color_table[0][oshade] = shaden;
	}
	v_object_color_table[0][V_OBJECT_IMAGE_BACKGROUND] =
		OBJECT_IMAGE_BACKGROUND;
	image_valid = image2_valid = icon_valid = FALSE;
}

/*****************************************************************************
 * FUNCTION: st_cl
 * DESCRIPTION: Returns the classification type of a shell data structure.
 * PARAMETERS:
 *    object_data: The shell data structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: BINARY_A, BINARY_B, GRADIENT, or PERCENT
 * EXIT CONDITIONS: Undefined if parameter is not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 5/16/01 BINARY_B returned by Dewey Odhner
 *    Modified: 6/22/01 DIRECT returned by Dewey Odhner
 *
 *****************************************************************************/
Classification_type st_cl(Shell_data *object_data)
{
	if (object_data->file->file_header.gen.data_type != SHELL0)
		return (object_data->file->file_header.gen.data_type==SHELL2? T_SHELL:
			DIRECT);
	if (object_data->file->file_header.str.bit_fields_in_TSE[5] >
			object_data->file->file_header.str.bit_fields_in_TSE[4])
		return (PERCENT);
	if (object_data->file->file_header.str.bit_fields_in_TSE[15] >
			object_data->file->file_header.str.bit_fields_in_TSE[14])
		return (GRADIENT);
	if (object_data->file->file_header.str.bit_fields_in_TSE[3] > 15)
		return (BINARY_B);
	return (BINARY_A);
}

/*****************************************************************************
 * FUNCTION: loadFiles
 * DESCRIPTION: Loads the .BS0 files or one .SH0 file specified.
 * PARAMETERS:
 *    file_list: Names of the files to be loaded
 *    input_files: Number of the files to be loaded
 *    icons: Non-zero if icons are to be used
 * SIDE EFFECTS: Sets the global variables scale, depth_scale, icon_scale,
 *    display_area_width, display_area_height, display_area_x, display_area_y,
 *    image_x, image_y, icon_area_width, icon_area_height, icon_area_x,
 *    icon_area_y, icon_image_x, icon_image_y, image_mode, image2,
 *    display_area2, image_valid, image2_valid, icon_valid, selected_object,
 *    glob_displacement, icons_exist
 * ENTRY CONDITIONS: For each .BS0 file there must be a .BSI (icon) file with
 *    the rest of the name the same if icons.
 *    The global variable object_list must be valid or NULL.
 * RETURN VALUE: 0 if successful
 * EXIT CONDITIONS: If any file cannot be loaded, all files will be unloaded.
 *    Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 10/31/06 by Dewey Odhner
 *
 *****************************************************************************/
int cvRenderer::loadFiles(char **file_list, int input_files, int icons)
{
	int error_code, file_number, num_of_objects, *shell_numbers, k;
	Shell *this_object, **tail_ptr;
	char *file_extension;
	triple (*rotations)[3], *displacements, orientation[3], center, center1;
	float *domain;
	double unit;

	icons_exist = icons;
	glob_displacement[0] = glob_displacement[1] = glob_displacement[2] = 0;
	tail_ptr = &object_list;
	for (file_number=0; file_number<input_files; file_number++)
	{	file_extension = strrchr(file_list[file_number], '.');
		if (strcmp(file_extension, ".PLN") == 0)
		{	error_code = read_plan(file_list[file_number],
				&num_of_objects, &shell_numbers, &rotations, &displacements,
				file_list[file_number], TRUE, NULL, NULL, NULL, NULL);
			if (error_code)
			{
				while (object_list)
					remove_object(object_list);
				return error_code;
			}
			if ((error_code= load_file(tail_ptr, FALSE, file_list[file_number],
					TRUE, num_of_objects, shell_numbers)))
			{	while (object_list)
					remove_object(object_list);
				free(shell_numbers);
				free(rotations);
				free(displacements);
				return error_code;
			}
			if (icons_exist)
			{
				file_list[file_number][strlen(file_list[file_number])-1] = 'I';
				if ((error_code=load_file(tail_ptr,TRUE,file_list[file_number],
						TRUE, num_of_objects, shell_numbers)))
				{	while (object_list)
						remove_object(object_list);
					free(shell_numbers);
					free(rotations);
					free(displacements);
					icons_exist = FALSE;
					return error_code;
				}
			}
			for (k=0; *tail_ptr; k++,tail_ptr= &(*tail_ptr)->next)
			{	domain = (*tail_ptr)->main_data.file->file_header.str.domain+
					12*(*tail_ptr)->main_data.shell_number;
				orientation[0][0] = domain[3];
				orientation[1][0] = domain[4];
				orientation[2][0] = domain[5];
				orientation[0][1] = domain[6];
				orientation[1][1] = domain[7];
				orientation[2][1] = domain[8];
				orientation[0][2] = domain[9];
				orientation[1][2] = domain[10];
				orientation[2][2] = domain[11];
				unit = unit_mm(&(*tail_ptr)->main_data);
				(*tail_ptr)->displacement[0] = unit*(.5*
					(Min_coordinate(&(*tail_ptr)->main_data, 0)+
					 Max_coordinate(&(*tail_ptr)->main_data, 0)));
				(*tail_ptr)->displacement[1] = unit*(.5*
					(Min_coordinate(&(*tail_ptr)->main_data, 1)+
					 Max_coordinate(&(*tail_ptr)->main_data, 1)));
				(*tail_ptr)->displacement[2] = unit*(.5*
					(Min_coordinate(&(*tail_ptr)->main_data, 2)+
					 Max_coordinate(&(*tail_ptr)->main_data, 2)));
				matrix_vector_multiply((*tail_ptr)->displacement,
					orientation, (*tail_ptr)->displacement);
				(*tail_ptr)->displacement[0] += unit*domain[0];
				(*tail_ptr)->displacement[1] += unit*domain[1];
				(*tail_ptr)->displacement[2] += unit*domain[2];
				matrix_vector_multiply((*tail_ptr)->displacement,
					rotations[k], (*tail_ptr)->displacement);
				vector_add((*tail_ptr)->displacement,
					(*tail_ptr)->displacement, displacements[k]);
				matrix_multiply(orientation, rotations[k], orientation);
				MtoA((*tail_ptr)->angle, (*tail_ptr)->angle+1,
					(*tail_ptr)->angle+2, orientation);
			}
			free(shell_numbers);
			free(rotations);
			free(displacements);
		}
		else
		{	if ((error_code= load_file(tail_ptr, FALSE, file_list[file_number],
					FALSE, 0, NULL)))
			{	while (object_list)
					remove_object(object_list);
				return error_code;
			}
			if (icons_exist)
			{
				file_list[file_number][strlen(file_list[file_number])-1] = 'I';
				if ((error_code=load_file(tail_ptr,TRUE,file_list[file_number],
						FALSE, 0, NULL)))
				{	while (object_list)
						remove_object(object_list);
					icons_exist = FALSE;
					return error_code;
				}
			}
			for (; *tail_ptr; tail_ptr= &(*tail_ptr)->next)
			{	domain= ((*tail_ptr)->main_data.file->file_header.gen.data_type
					==IMAGE0?
					(*tail_ptr)->main_data.file->file_header.scn.domain:
					(*tail_ptr)->main_data.file->file_header.str.domain)+
					12*(*tail_ptr)->main_data.shell_number;
				orientation[0][0] = domain[3];
				orientation[1][0] = domain[4];
				orientation[2][0] = domain[5];
				orientation[0][1] = domain[6];
				orientation[1][1] = domain[7];
				orientation[2][1] = domain[8];
				orientation[0][2] = domain[9];
				orientation[1][2] = domain[10];
				orientation[2][2] = domain[11];
				MtoA((*tail_ptr)->angle, (*tail_ptr)->angle+1,
					(*tail_ptr)->angle+2, orientation);
				unit = unit_mm(&(*tail_ptr)->main_data);
				center[0]= .5*unit*(Min_coordinate(&(*tail_ptr)->main_data, 0)+
					Max_coordinate(&(*tail_ptr)->main_data, 0));
				center[1]= .5*unit*(Min_coordinate(&(*tail_ptr)->main_data, 1)+
					Max_coordinate(&(*tail_ptr)->main_data, 1));
				center[2]= .5*unit*(Min_coordinate(&(*tail_ptr)->main_data, 2)+
					Max_coordinate(&(*tail_ptr)->main_data, 2));
				matrix_vector_multiply(center, orientation, center);
				center1[0] = -unit*domain[0];
				center1[1] = -unit*domain[1];
				center1[2] = -unit*domain[2];
				vector_subtract((*tail_ptr)->displacement, center, center1);
				if (tail_ptr == &object_list)
					vector_subtract(glob_displacement, center1, center);
			}
		}
	}
	selected_object = object_label(&object_list->O);
	for (this_object=object_list; this_object; this_object=this_object->next)
	{	compute_diameter(this_object, icons_exist);
		this_object->O.specular_fraction = 0.2;
		this_object->O.specular_exponent = 3.0;
		this_object->O.diffuse_exponent = 1;
		this_object->O.specular_n = 1;
		this_object->O.diffuse_n = 2;
		if (this_object==object_list)
		{
			this_object->O.rgb.red = 50000;
			this_object->O.rgb.green = 50000;
			this_object->O.rgb.blue = 65535;
			this_object->O.color = 0;
		}
		else
		{
			ncolors = 2;
			this_object->O.rgb.red = 65535;
			this_object->O.rgb.green = 65535;
			this_object->O.rgb.blue = 50000;
			this_object->O.color = 1;
		}
		this_object->O.on = TRUE;
		this_object->O.opacity = 1;
		vector_add(this_object->displacement,
			this_object->displacement, glob_displacement);
		memcpy(this_object->plan_displacement,
			this_object->displacement, sizeof(triple));
		memcpy(this_object->plan_angle, this_object->angle,
			sizeof(triple));
	}
	scale = 1.9798/voxel_spacing(&object_list->main_data);
	icon_scale = 3.9596/voxel_spacing(icons_exist? &object_list->icon_data:
		&object_list->main_data);
	set_depth_scale();
	viewport_back = (int)(Z_BUFFER_LEVELS-120*depth_scale);
	if (viewport_back < 0)
		viewport_back = 0;
	resize_image();
	if (st_cl(&object_list->main_data)!=BINARY_B &&
			st_cl(&object_list->main_data)!=BINARY_A &&
			st_cl(&object_list->main_data)!=T_SHELL)
	{	compute_v_object_color_table();
		if (v_object_color_table == NULL)
			while (object_list)
				remove_object(object_list);
	}
	if (st_cl(&object_list->main_data)==PERCENT ||
			st_cl(&object_list->main_data)==DIRECT)
		check_true_color();
	else
		true_color = FALSE;
	for (this_object=object_list; this_object; this_object=this_object->next)
		this_object->original = true;
	return 0;
}

/*****************************************************************************
 * FUNCTION: remove_object
 * DESCRIPTION: Removes an object from the object list.
 * PARAMETERS:
 *    bad_obj: pointer to the object to be removed
 * SIDE EFFECTS: The labels of objects beyond bad_obj will change. The global
 *    variables selected_object, ncolors, image_valid, image2_valid,
 *    icon_valid, separate_piece1, separate_piece2, colormap_valid,
 *    slice_list may change,
 *    and the object color numbers may acquire new meanings.
 * ENTRY CONDITIONS: The global variable object_list must point to a valid
 *    Shell. (A valid Shell must have its 'next' point to a valid
 *    Shell or be NULL, and must have its 'reflection' point to a valid
 *    Virtual_object or be NULL.)  The global variables selected_object,
 *    ncolors, color_mode must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1/5/93 by Dewey Odhner
 *    Modified: 1/26/93 to clear separate_piece1 or separate_piece2
 *       by Dewey Odhner
 *    Modified: 4/20/93 to remove slice_list by Dewey Odhner
 *    Modified: 11/17/94 freed memory reference fixed by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::remove_object(Shell *bad_obj)
{
	Shell *obj;
	int bad_obj_label;

	bad_obj_label = object_label(&bad_obj->O);
	if (bad_obj_label==selected_object || -bad_obj_label==selected_object)
		selected_object = bad_obj->next? bad_obj_label: bad_obj_label-1;
	else if (selected_object > bad_obj_label)
		selected_object--;
	else if (selected_object < -bad_obj_label)
		selected_object++;
	if (bad_obj == object_list)
		object_list = object_list->next;
	else
	{	for (obj=object_list; obj->next!=bad_obj; obj=obj->next)
			;
		obj->next = obj->next->next;
	}
	if (bad_obj == separate_piece1)
		separate_piece1 = NULL;
	if (bad_obj == separate_piece2)
		separate_piece2 = NULL;
	if (bad_obj->original)
		original_object = NULL;
	destroy_object(bad_obj);
	eliminate_unused_colors();
}

/*****************************************************************************
 * FUNCTION: set_depth_scale
 * DESCRIPTION: Sets the global variable depth_scale.
 * PARAMETERS: None
 * SIDE EFFECTS: Marks main object images bad.  The global variables
 *    image_valid, image2_valid, overlay_bad may be changed.
 * ENTRY CONDITIONS: The global variables object_list, scale,
 *    plane_displacement, plane_normal must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void cvRenderer::set_depth_scale(void)
{
	depth_scale = Z_BUFFER_LEVELS/get_image_size()*scale;
	invalidate_main_images();
}

/*****************************************************************************
 * FUNCTION: destroy_object_image
 * DESCRIPTION: Frees allocated memory associated with an object image.
 * PARAMETERS:
 *    object_image: The object image to be destroyed.  Buffers must be
 *       allocated if object_image->image_size is non-zero.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if object_image is not valid.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void destroy_object_image(Object_image *object_image)
{
	if (object_image->image_size)
	{	free(object_image->image[0]);
		free(object_image->image);
		free(object_image->z_buffer[0]);
		free(object_image->z_buffer);
		if (object_image->opacity_buffer)
		{	free(object_image->opacity_buffer[0]);
			free(object_image->opacity_buffer);
		}
		if (object_image->likelihood_buffer)
		{	free(object_image->likelihood_buffer[0]);
			free(object_image->likelihood_buffer);
		}
	}
}
