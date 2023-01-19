/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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

#include "neighbors.h"
#include "manipulate.h"
#include <cv3dv.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif

int load_file(Shell_file *shell_file, char file_name[]),
 copy_shell_header(ViewnixHeader *new, ViewnixHeader *old,
    int new_num_of_structures),
 output_shell_data(Shell_file *shell_file),
 load_shell(Shell_file *shell_file, FILE *infile, int ref_number,
    int shell_number);

char *argv0;

#ifndef  FALSE
#define  FALSE 0
#endif

#ifndef  False
#define  False 0
#endif
/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Creates an icon for a Shell0 file.
 * PARAMETERS:
 *    argc: The number of command line parameters (should be 2).
 *    argv: The command line parameters
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: The environment variable VIEWNIX_ENV
 *    must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits the process with code 1 on error.
 * HISTORY:
 *    Created: 4/16/98 by Dewey Odhner
 *    Modified: 6/8/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
int main(argc, argv)
	int argc;
	char *argv[];
{
	static Shell_file shell_file, icon_file;
	char icon_name[300];
	int shellno, lump_factor, cn, rn, sn, lc, lr, ls, tse_size,
		cells_found, ncode, tt, gm, op, min_sample, max_sample, B;
	unsigned short *sp, **stp;
	double gx, gy, gz, cgx, cgy, cgz;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s shell0_file\n", argv[0]);
		exit(1);
	}
	argv0 = argv[0];
	if (strlen(argv[1]) > 299)
	{
		fprintf(stderr, "File name is too long.\n");
		exit(1);
	}
	strcpy(icon_name, argv[1]);
	icon_name[strlen(icon_name)-1] = 'I';
	unlink(icon_name);
	load_file(&shell_file, argv[1]);
	lump_factor=1;
	for (shellno=0; shellno<shell_file.references; shellno++)
		for (; shell_file.reference[shellno]->slices*
				shell_file.reference[shellno]->rows*
				(Largest_y1(shell_file.reference[shellno])-
				Smallest_y1(shell_file.reference[shellno])+1)>
				64*64*64*lump_factor*lump_factor*lump_factor; lump_factor++)
			;
	if (copy_shell_header(&icon_file.file_header, &shell_file.file_header,
			shell_file.references))
	{
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	strncpy(icon_file.file_header.gen.filename, icon_name,
		sizeof(icon_file.file_header.gen.filename)-1);
	icon_file.file_header.gen.filename[
		sizeof(icon_file.file_header.gen.filename)-1] = 0;
	icon_file.file_header.str.xysize[0] *= lump_factor;
	icon_file.file_header.str.xysize[1] *= lump_factor;
	icon_file.references = shell_file.references;
	icon_file.reference =
		(Shell_data **)malloc(icon_file.references*sizeof(Shell_data *));
	if (icon_file.reference == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	tse_size = shell_file.file_header.str.num_of_bits_in_TSE/16;
	B = shell_file.file_header.str.bit_fields_in_TSE[3]>15;
	for (shellno=0; shellno<shell_file.references; shellno++)
	{
		for (sn=0; sn<shell_file.file_header.str.num_of_samples[0]; sn++)
		{
			if (shell_file.file_header.str.min_max_coordinates[shellno*6+2] ==
					shell_file.file_header.str.loc_of_samples[sn])
				min_sample = sn;
			if (shell_file.file_header.str.min_max_coordinates[shellno*6+5] ==
					shell_file.file_header.str.loc_of_samples[sn])
				max_sample = sn;
		}
		icon_file.file_header.str.num_of_samples[0] =
			(shell_file.file_header.str.num_of_samples[0]+lump_factor-1)/
			lump_factor;
		for (sn=0; sn<icon_file.file_header.str.num_of_samples[0]; sn++)
			icon_file.file_header.str.loc_of_samples[sn] =
				shell_file.file_header.str.loc_of_samples[sn*lump_factor];
		icon_file.file_header.str.min_max_coordinates[shellno*6+5] =
			icon_file.file_header.str.loc_of_samples[max_sample/lump_factor];
		icon_file.file_header.str.min_max_coordinates[shellno*6+2] =
			icon_file.file_header.str.loc_of_samples[min_sample/lump_factor];
		icon_file.reference[shellno] =
			(Shell_data *)malloc(sizeof(Shell_data));
		if (icon_file.reference[shellno] == NULL)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
		icon_file.reference[shellno]->file = &icon_file;
		icon_file.reference[shellno]->rows =
			(shell_file.reference[shellno]->rows+lump_factor-1)/lump_factor;
		icon_file.reference[shellno]->slices =
			max_sample/lump_factor-min_sample/lump_factor+1;
		icon_file.file_header.str.min_max_coordinates[shellno*6+4] =
			icon_file.file_header.str.min_max_coordinates[shellno*6+1]+
			(icon_file.reference[shellno]->rows-1)*
			icon_file.file_header.str.xysize[1];
		icon_file.file_header.str.min_max_coordinates[shellno*6+3] = (float)(
			icon_file.file_header.str.min_max_coordinates[shellno*6]+
			floor(rint((shell_file.file_header.
			str.min_max_coordinates[shellno*6+3]-shell_file.file_header.
			str.min_max_coordinates[shellno*6])/
			shell_file.file_header.str.xysize[0])/lump_factor)*
			icon_file.file_header.str.xysize[0]);
		icon_file.reference[shellno]->shell_number = shellno;
		icon_file.reference[shellno]->volume_valid = FALSE;
		Smallest_y1(icon_file.reference[shellno]) =
			Smallest_y1(shell_file.reference[shellno])/lump_factor;
		Largest_y1(icon_file.reference[shellno]) =
			Largest_y1(shell_file.reference[shellno])/lump_factor;
		icon_file.reference[shellno]->ptr_table = (unsigned short **)
			malloc((icon_file.reference[shellno]->rows*
			icon_file.reference[shellno]->slices+1)*sizeof(short *));
		if (icon_file.reference[shellno]->ptr_table == NULL)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
		icon_file.reference[shellno]->ptr_table[0] = (unsigned short *)malloc(
			(shell_file.reference[shellno]->ptr_table[shell_file.reference[
			shellno]->rows*shell_file.reference[shellno]->slices]-
			shell_file.reference[shellno]->ptr_table[0])*sizeof(short));
		if (icon_file.reference[shellno]->ptr_table[0] == NULL)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}

		/* create icon TSE's */
		for (sn=0; sn<icon_file.reference[shellno]->slices; sn++)
			for (rn=0; rn<icon_file.reference[shellno]->rows; rn++)
			{
				icon_file.reference[shellno]->ptr_table[sn*
					icon_file.reference[shellno]->rows+rn+1] =
					icon_file.reference[shellno]->ptr_table[sn*
					icon_file.reference[shellno]->rows+rn];
				for (cn=(int)Smallest_y1(icon_file.reference[shellno]);
						cn<=Largest_y1(icon_file.reference[shellno]); cn++)
				{
					cells_found = 0;
					cgx = cgy = cgz = 0;
					ncode = tt = gm = op = 0;
					for (ls=0; ls<lump_factor&&
						sn*lump_factor-min_sample%lump_factor+ls
						<shell_file.reference[shellno]->slices; ls++)
					{
					  if (sn*lump_factor-min_sample%lump_factor+ls < 0)
					    continue;
					  for (lr=0; lr<lump_factor&&rn*lump_factor+lr<
						  shell_file.reference[shellno]->rows; lr++)
						for (lc=0,stp=shell_file.reference[shellno]->ptr_table+
							(sn*lump_factor-min_sample%lump_factor+ls)*
							shell_file.reference[shellno]->rows
							+rn*lump_factor+lr,sp= *stp; lc<lump_factor; lc++)
						{
						  while (sp<stp[1] && (B? ((sp[0]&XMASK)<<1)|
						      ((sp[1]&0x8000)!=0):
							  sp[0]&XMASK)<cn*lump_factor+lc)
						    sp += tse_size;
						  if (sp >= stp[1])
						    continue;
						  if ((B? ((sp[0]&XMASK)<<1)|((sp[1]&0x8000)!=0):
						      sp[0]&XMASK) == cn*lump_factor+lc)
						  {
							if (lc==0 && sp[0]&NX)
							  ncode |= NX;
							if (lc==lump_factor-1 && sp[0]&PX)
							  ncode |= PX;
							if (lr==0 && sp[0]&NY)
							  ncode |= NY;
							if (lr==lump_factor-1 && sp[0]&PY)
							  ncode |= PY;
							if (ls==0 && sp[0]&NZ)
							  ncode |= NZ;
							if (ls==lump_factor-1 && sp[0]&PZ)
							  ncode |= PZ;
							if (B)
							  BG_decode(gx, gy, gz, sp[1]&0x7fff)
							else
							  G_decode(gx, gy, gz, sp[1]&2047);
							if (tse_size == 2)
							{
								cgx += gx;
								cgy += gy;
								cgz += gz;
							}
							else
							{
								cgx += gx*(sp[2]>>8);
								cgy += gy*(sp[2]>>8);
								cgz += gz*(sp[2]>>8);
								if (sp[2]>>8 > gm)
								{
									gm = sp[2]>>8;
									tt = sp[1]>>11;
									op = sp[2]&255;
								}
							}
							cells_found++;
							sp += tse_size;
						  }
						  else if ((B? ((sp[0]&XMASK)<<1)|((sp[1]&0x8000)!=0):
						      sp[0]&XMASK) >= (cn+1)*lump_factor)
						    break;
						}
					}
					if (cells_found)
					{
						icon_file.reference[shellno]->ptr_table[sn*
							icon_file.reference[shellno]->rows+rn+1][0] =
							ncode | cn;
						icon_file.reference[shellno]->ptr_table[sn*
							icon_file.reference[shellno]->rows+rn+1][1] =
							(tt<<11) | G_code(cgx, cgy, cgz);
						if (tse_size == 3)
							icon_file.reference[shellno]->ptr_table[sn*
								icon_file.reference[shellno]->rows+rn+1][2] =
								(gm<<8) | op;
						icon_file.reference[shellno]->ptr_table[sn*
							icon_file.reference[shellno]->rows+rn+1]+=tse_size;
					}
				}
			}

		icon_file.reference[shellno]->in_memory = TRUE;
		free(shell_file.reference[shellno]->ptr_table[0]);
		free(shell_file.reference[shellno]->ptr_table);
		icon_file.file_header.str.num_of_NTSE[shellno] = 1+icon_file.reference[
			shellno]->slices*(1+icon_file.reference[shellno]->rows);
		icon_file.file_header.str.num_of_TSE[shellno] =
			(unsigned)((icon_file.reference[
			shellno]->ptr_table[icon_file.reference[shellno]->rows*
			icon_file.reference[shellno]->slices]-
			icon_file.reference[shellno]->ptr_table[0])/tse_size);
	}
	icon_file.file_header.str.bit_fields_in_TSE[3] = 15;
	icon_file.file_header.str.bit_fields_in_TSE[6] = 21;
	icon_file.file_header.str.bit_fields_in_TSE[7] = 23;
	icon_file.file_header.str.bit_fields_in_TSE[8] = 24;
	icon_file.file_header.str.bit_fields_in_TSE[9] = 27;
	icon_file.file_header.str.bit_fields_in_TSE[10] = 28;
	output_shell_data(&icon_file);
	exit (0);
}

/*****************************************************************************
 * FUNCTION: load_file
 * DESCRIPTION: Loads the objects from a file.
 * PARAMETERS:
 *    shell_file: Data will be loaded here.
 *    file_name: The file name
 * SIDE EFFECTS: An error message may be issued.
 * ENTRY CONDITIONS: The variable argv0 and environment variable VIEWNIX_ENV
 *    must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits the process with code 1 on error.
 * HISTORY:
 *    Created: 4/2/98 by Dewey Odhner
 *    Modified: 6/7/01 BINARY_B data type accommodated by Dewey Odhner
 *
 *****************************************************************************/
int load_file(Shell_file *shell_file, char file_name[])
{
	FILE *infile;
	int error_code, shell_number, j;
	char bad_group[5], bad_element[5], msg[200];
	Classification_type shell_class;

	if ((infile=fopen(file_name, "rb"))==NULL)
	{
		fprintf(stderr, "Could not open file %s\n", file_name);
		exit(1);
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
			VDecodeError(argv0, "load_file", error_code, msg);
			fprintf(stderr, "%s\n", msg);
			exit(1);
	}

	if (shell_file->file_header.str.bit_fields_in_TSE[5] >
			shell_file->file_header.str.bit_fields_in_TSE[4])
		shell_class = PERCENT;
	else if (shell_file->file_header.str.bit_fields_in_TSE[15] >
			shell_file->file_header.str.bit_fields_in_TSE[14])
		shell_class = GRADIENT;
	else if (shell_file->file_header.str.bit_fields_in_TSE[3] > 15)
		shell_class = BINARY_B;
	else
		shell_class = BINARY;
	/* check that all values are normal */
	if (shell_file->file_header.gen.data_type!=SHELL0 ||
			shell_file->file_header.str.dimension!=3 ||
			shell_file->file_header.str.measurement_unit[0]<0 ||
			shell_file->file_header.str.measurement_unit[0]>4 ||
			shell_file->file_header.str.measurement_unit[1]!=
				shell_file->file_header.str.measurement_unit[0] ||
			shell_file->file_header.str.measurement_unit[2]!=
				shell_file->file_header.str.measurement_unit[0] ||
			shell_file->file_header.str.num_of_components_in_TSE!=9 ||
			shell_file->file_header.str.num_of_components_in_NTSE!=1 ||
			shell_file->file_header.str.num_of_integers_in_TSE<6 ||
			shell_file->file_header.str.num_of_bits_in_TSE!=
				(shell_class==BINARY_B||shell_class==BINARY? 32: 48) ||
			shell_file->file_header.str.bit_fields_in_TSE[0] ||
			shell_file->file_header.str.bit_fields_in_TSE[1]!=5 ||
			shell_file->file_header.str.bit_fields_in_TSE[2]!=6 ||
			shell_file->file_header.str.bit_fields_in_TSE[3]!=
				(shell_class==BINARY_B? 16:15) ||
			(shell_class!=PERCENT&&
				shell_file->file_header.str.bit_fields_in_TSE[5]>=
				shell_file->file_header.str.bit_fields_in_TSE[4]) ||
			((shell_class==PERCENT&&
				(shell_file->file_header.str.bit_fields_in_TSE[4]!=16||
				shell_file->file_header.str.bit_fields_in_TSE[5]!=20)) ||
			(	shell_class==BINARY_B?
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
			((shell_class==BINARY_B||shell_class==BINARY) &&
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
		fprintf(stderr, "UNEXPECTED VALUE IN DATA FILE.\n");
		exit(1);
	}

	strncpy(shell_file->file_header.gen.filename, file_name,
		sizeof(shell_file->file_header.gen.filename)-1);
	shell_file->file_header.gen.filename[
		sizeof(shell_file->file_header.gen.filename)-1] = 0;
	if (shell_file->file_header.gen.filename1[
			strlen(shell_file->file_header.gen.filename1)-1] == ' ')
		shell_file->file_header.gen.filename1[
			strlen(shell_file->file_header.gen.filename1)-1] = 0;
	if (shell_file->file_header.str.scene_file_valid)
		for (j=0; j<shell_file->file_header.str.num_of_structures; j++)
			if (shell_file->file_header.str.scene_file[j][
					strlen(shell_file->file_header.str.scene_file[j])-1] ==' ')
				shell_file->file_header.str.scene_file[j][
					strlen(shell_file->file_header.str.scene_file[j])-1] = 0;
	shell_file->reference = (Shell_data **)malloc(sizeof(Shell_data *)*
		shell_file->file_header.str.num_of_structures);
	if (shell_file->reference == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	shell_file->references = shell_file->file_header.str.num_of_structures;
	for (shell_number=0; shell_number<shell_file->references; shell_number++)
	{
		error_code= load_shell(shell_file, infile, shell_number, shell_number);
		if (error_code)
		{
			if (error_code == -1)
				strcpy(msg, "EMPTY SHELL IN INPUT FILE");
			else
				VDecodeError(argv0, "load_file", error_code, msg);
			fprintf(stderr, "%s\n", msg);
			exit(1);
		}
	}
	fclose(infile);
	return 0;
}

/*****************************************************************************
 * FUNCTION: load_shell
 * DESCRIPTION: Loads one object from a file.
 * PARAMETERS:
 *    shell_file: The file information for the shell
 *    infile: The file pointer, must be open for reading.
 *    ref_number: The reference number from shell_file to this object
 *    shell_number: The object to be loaded. (0 = first object)
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE: Non-zero if unsuccessful; -1 if shell is empty.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 4/6/98 by Dewey Odhner
 *    Modified: 10/18/00 items_read changed to int by Dewey Odhner
 *
 *****************************************************************************/
int load_shell(Shell_file *shell_file, FILE *infile, int ref_number,
 int shell_number)
{
	Shell_data *obj_data;
	short *ntse;
	unsigned long data_offset;
	int items_read, error_code, ptrs_in_table, j, num_of_TSE;
	unsigned short n1, n2, n3;

	shell_file->reference[ref_number] = obj_data =
		(Shell_data *)calloc(1, sizeof(Shell_data));
	if (obj_data == NULL)
		return (1);
	obj_data->shell_number = shell_number;
	obj_data->file = shell_file;
	data_offset = 0;
	for (j=0; j<shell_number; j++)
		data_offset += 2*shell_file->file_header.str.num_of_NTSE[j]+
			shell_file->file_header.str.num_of_bits_in_TSE/8*
				shell_file->file_header.str.num_of_TSE[j];
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
	{
		free(obj_data);
		return (1);
	}
	if (shell_file->file_header.str.num_of_NTSE[shell_number])
		ntse = (short *)malloc(sizeof(short)*
			shell_file->file_header.str.num_of_NTSE[shell_number]);
	else
	{
		free(obj_data);
		return (-1);
	}
	if (ntse == NULL)
	{	free(obj_data->ptr_table);
		free(obj_data);
		return (1);
	}
	error_code = VSeekData(infile, data_offset);
	if (error_code)
	{	free(ntse);
		free(obj_data->ptr_table);
		free(obj_data);
		return (error_code);
	}
	error_code = VReadData((char *)ntse, 2,
		shell_file->file_header.str.num_of_NTSE[shell_number], infile,
		&items_read);
	if (error_code==0 && items_read!=
			shell_file->file_header.str.num_of_NTSE[shell_number])
		error_code = 2;
	if (error_code)
	{	free(ntse);
		free(obj_data->ptr_table);
		free(obj_data);
		return (error_code);
	}
	if (ntse[0] != obj_data->slices)
	{	free(ntse);
		free(obj_data->ptr_table);
		free(obj_data);
		return (100);
	}
	for (j=1; j<=obj_data->slices; j++)
		if (ntse[j] != obj_data->rows)
		{	free(ntse);
			free(obj_data->ptr_table);
			free(obj_data);
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
		{
			free(ntse);
			free(obj_data->ptr_table);
			free(obj_data);
			return (1);
		}
		else
		{	for (j=1; j<ptrs_in_table; j++)
				obj_data->ptr_table[j] =
					obj_data->ptr_table[j-1]+ntse[obj_data->slices+j]*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
			free(ntse);
			error_code = VReadData((char *)obj_data->ptr_table[0], 2,
				shell_file->file_header.str.num_of_bits_in_TSE/16*num_of_TSE,
				infile, &items_read);
			if (error_code==0 && items_read!=shell_file->
					file_header.str.num_of_bits_in_TSE/16*num_of_TSE)
				error_code = 2;
			if (error_code)
			{	free(obj_data->ptr_table[0]);
				free(obj_data->ptr_table);
				free(obj_data);
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
			else if (shell_file->file_header.str.bit_fields_in_TSE[9] <= 26)
				for (j=1; j<num_of_TSE*
						shell_file->file_header.str.num_of_bits_in_TSE/16;
						j+=shell_file->file_header.str.num_of_bits_in_TSE/16)
					obj_data->ptr_table[0][j] &= 0xffee;
			obj_data->in_memory = TRUE;
		}
	}
	return (0);
}


#define _0x9999 0/**/x9999
#define x9999 *9999
#if _0x9999
#define Cat(a, b) a/**/b
#else
#define Cat(a, b) a##b
#endif
#undef x9999
#undef _0x9999

/* 1/15/93 new_num must be >= old_num */
#define Copy_header_item(new, old, item, type, new_num, old_num) \
if (old->item) \
{	new->item = (type *) malloc((new_num)*sizeof(type)); \
	if (new->item == NULL) \
		return (1); \
	memcpy(new->item, old->item,(old_num)*sizeof(type));\
	new->Cat(item,_valid) = old->Cat(item,_valid); \
	if (new_num != old_num) \
		new->Cat(item,_valid) = False; \
}

#define Copy_header_string(new, old, item) \
	Copy_header_item(new, old, item, char, strlen(old->item)+1, \
		strlen(old->item)+1)

/*****************************************************************************
 * FUNCTION: copy_shell_header
 * DESCRIPTION: Copies a file header structure of a SHELL0 structure.
 * PARAMETERS:
 *    new: The destination; relevant pointer members will have memory
 *       allocated; the caller can free it after use.
 *    old: The source; pointer members that are not NULL must have appropriate
 *       memory allocated.
 *    new_num_of_structures: The value for new->str.num_of_structures.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: Parameters must be valid.
 * RETURN VALUE:
 *    0: No error
 *    1: Memory allocation failure
 * EXIT CONDITIONS: If an error occurs, relevant pointer members will have
 *    memory allocated or be NULL;
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *    Modified: 1/15/93 for new structure system protocol by Dewey Odhner
 *    Modified: 4/15/93 for new structure system protocol by Dewey Odhner
 *    Modified: 4/26/93 to copy domain for more structures by Dewey Odhner
 *    Modified: 5/15/93 to copy scene file to new structures by Dewey Odhner
 *
 *****************************************************************************/
int copy_shell_header(ViewnixHeader *new, ViewnixHeader *old,
    int new_num_of_structures)
{
	int old_num_of_structures, j;

	old_num_of_structures =
		old->str.num_of_structures > new_num_of_structures
		?	new_num_of_structures
		:	old->str.num_of_structures;
	*new = *old;
	new->str.num_of_structures = new_num_of_structures;
	new->gen.filename_valid = FALSE;
	new->gen.description = NULL;
	new->gen.comment = NULL;
	new->gen.imaged_nucleus = NULL;
	new->gen.gray_lookup_data = NULL;
	new->gen.red_lookup_data = NULL;
	new->gen.green_lookup_data = NULL;
	new->gen.blue_lookup_data = NULL;
	new->str.domain = NULL;
	new->str.axis_label = NULL;
	new->str.measurement_unit = NULL;
	new->str.num_of_TSE = NULL;
	new->str.num_of_NTSE = NULL;
	new->str.NTSE_measurement_unit = NULL;
	new->str.TSE_measurement_unit = NULL;
	new->str.smallest_value = NULL;
	new->str.largest_value = NULL;
	new->str.signed_bits_in_TSE = NULL;
	new->str.bit_fields_in_TSE = NULL;
	new->str.signed_bits_in_NTSE = NULL;
	new->str.bit_fields_in_NTSE = NULL;
	new->str.num_of_samples = NULL;
	new->str.loc_of_samples = NULL;
	new->str.description_of_element = NULL;
	new->str.parameter_vectors = NULL;
	new->str.min_max_coordinates = NULL;
	new->str.volume = NULL;
	new->str.surface_area = NULL;
	new->str.rate_of_change_volume = NULL;
	new->str.description = NULL;
	new->str.scene_file_valid = FALSE;
	Copy_header_string(new, old, gen.description)
	Copy_header_string(new, old, gen.comment)
	Copy_header_string(new, old, gen.imaged_nucleus)
	Copy_header_item(new, old, str.domain, float,
		12*new_num_of_structures, 12*old_num_of_structures)
	if (new->str.domain)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
			memcpy(new->str.domain+12*j, old->str.domain, 12*sizeof(float));
	Copy_header_item(new, old, str.axis_label, Char30, 3, 3)
	Copy_header_item(new, old, str.measurement_unit, short, 3, 3)
	Copy_header_item(new, old, str.num_of_TSE, unsigned int,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(new, old, str.num_of_NTSE, unsigned int,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(new, old, str.NTSE_measurement_unit, short, 1, 1)
	Copy_header_item(new, old, str.TSE_measurement_unit, short, 9, 9)
	Copy_header_item(new, old, str.smallest_value, float,
		9*new_num_of_structures, 9*old_num_of_structures)
	Copy_header_item(new, old, str.largest_value, float,
		9*new_num_of_structures, 9*old_num_of_structures)
	if (new->str.smallest_value)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
		{	new->str.smallest_value[9*j] = 0;
			new->str.smallest_value[9*j+2] = 0;
			new->str.smallest_value[9*j+3] = 0;
			new->str.smallest_value[9*j+4] = 0;
			new->str.smallest_value[9*j+5] = 0;
			new->str.smallest_value[9*j+6] = 0;
			new->str.smallest_value[9*j+7] = 0;
			new->str.smallest_value[9*j+8] = 0;
			new->str.largest_value[9*j] = 63;
			new->str.largest_value[9*j+2] = 7;
			new->str.largest_value[9*j+3] = 6;
			new->str.largest_value[9*j+4] = new->str.largest_value[4];
			new->str.largest_value[9*j+5] = new->str.largest_value[4];
			new->str.largest_value[9*j+6] = 255;
			new->str.largest_value[9*j+7] = 255;
			new->str.largest_value[9*j+8] = 255;
		}
	Copy_header_item(new, old, str.signed_bits_in_TSE, short, 9, 9)
	Copy_header_item(new, old, str.bit_fields_in_TSE, short, 18, 18)
	Copy_header_item(new, old, str.signed_bits_in_NTSE, short, 1, 1)
	Copy_header_item(new, old, str.bit_fields_in_NTSE, short, 2, 2)
	Copy_header_item(new, old, str.num_of_samples, short, 1, 1)
	Copy_header_item(new, old, str.loc_of_samples, float,
		old->str.num_of_samples[0], old->str.num_of_samples[0])
	Copy_header_item(new, old, str.description_of_element, short,
		old->str.num_of_elements, old->str.num_of_elements)
	Copy_header_item(new, old, str.parameter_vectors, float,
		old->str.num_of_elements*new_num_of_structures,
		old->str.num_of_elements*old_num_of_structures)
	Copy_header_item(new, old, str.min_max_coordinates, float,
		6*new_num_of_structures, 6*old_num_of_structures)
	Copy_header_item(new, old, str.volume, float, new_num_of_structures,
		old_num_of_structures)
	Copy_header_item(new, old, str.surface_area, float,
		new_num_of_structures, old_num_of_structures)
	Copy_header_item(new, old, str.rate_of_change_volume, float,
		new_num_of_structures, old_num_of_structures)
	Copy_header_string(new, old, str.description)
	Copy_header_item(new, old, str.scene_file, Char30,
		new_num_of_structures, old_num_of_structures);
	if (new->str.scene_file)
		for (j=old_num_of_structures; j<new_num_of_structures; j++)
			strcpy(new->str.scene_file[j], old->str.scene_file[0]);
	return (0);
}

/*****************************************************************************
 * FUNCTION: output_shell_data
 * DESCRIPTION: Creates a file containing a structure system.
 * PARAMETERS:
 *    shell_file: Specifies the shell to be written.
 * SIDE EFFECTS: If error 106 occurs in VWriteHeader or an object is not
 *    in memory, a message is displayed.
 * ENTRY CONDITIONS: The global variables argv0, must be
 *    appropriately initialized.
 * RETURN VALUE: Non-zero if an error occurs other than 106
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 4/3/98 by Dewey Odhner
 *    Modified: 10/19/00 items_written changed to int by Dewey Odhner
 *    Modified: 3/19/01 extra fclose call deleted by Dewey Odhner
 *
 *****************************************************************************/
int output_shell_data(Shell_file *shell_file)
{
	int tree_size, j, error_code, shell_number, num_of_NTSE, num_of_shells,
		binary;
	short *tree_data;
	unsigned short *this_ptr;
	FILE *fp;
	int items_written;
	char bad_group[5], bad_element[5];
	Shell_data *obj_data;

	num_of_shells = shell_file->file_header.str.num_of_structures;
	fp = fopen(shell_file->file_header.gen.filename, "w+b");
	if (fp == NULL)
		return (4);
	binary = shell_file->file_header.str.num_of_bits_in_TSE==32;
	error_code =
		VWriteHeader(fp, &shell_file->file_header, bad_group, bad_element);
	switch (error_code)
	{	case 0:
		case 107:
		case 106:
			break;
		default:
			fprintf(stderr, "Group %s element %s undefined in VWriteHeader\n",
				bad_group, bad_element);
			fclose(fp);
			unlink(shell_file->file_header.gen.filename);
			return (error_code);
	}
	error_code = VSeekData(fp, 0);
	if (error_code)
	{
		fclose(fp);
		unlink(shell_file->file_header.gen.filename);
		return (error_code);
	}
	for (shell_number=0; shell_number<num_of_shells; shell_number++)
	{
		obj_data = shell_file->reference[shell_number];
		num_of_NTSE = 1+obj_data->slices*(1+obj_data->rows);
		tree_size = (int)(num_of_NTSE+
			(obj_data->ptr_table[obj_data->rows*obj_data->slices]-
			obj_data->ptr_table[0]));
		tree_data = (short *)malloc(tree_size*2);
		if (tree_data == NULL)
		{
			fclose(fp);
			unlink(shell_file->file_header.gen.filename);
			return (1);
		}
		tree_data[0] = obj_data->slices;
		for (j=1; j<=obj_data->slices; j++)
			tree_data[j] = obj_data->rows;
		for (j=0; j<obj_data->rows*obj_data->slices; j++)
			tree_data[1+obj_data->slices+j] =
				(short)((obj_data->ptr_table[j+1]-obj_data->ptr_table[j])/
					(binary? 2:3));
		for (this_ptr=obj_data->ptr_table[0]; this_ptr<
				obj_data->ptr_table[obj_data->rows*obj_data->slices];
				this_ptr+=(binary? 2:3))
		{	tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])] =
				this_ptr[0];
			tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])+1]=
					this_ptr[1];
			if (!binary)
				tree_data[num_of_NTSE+(this_ptr-obj_data->ptr_table[0])+2]=
					this_ptr[2];
		}
		error_code = VWriteData((char *)tree_data, 2, tree_size, fp, &items_written);
		if (error_code==0 && items_written!=tree_size)
			error_code = 3;
		if (error_code)
		{
			free(tree_data);
			fclose(fp);
			unlink(shell_file->file_header.gen.filename);
			return (error_code);
		}
		free(tree_data);
	}
	error_code = VCloseData(fp);
	if (error_code)
		unlink(shell_file->file_header.gen.filename);
	return (error_code);
}
