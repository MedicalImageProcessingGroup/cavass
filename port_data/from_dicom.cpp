/*
  Copyright 1993-2019, 2021, 2023-2024 Medical Image Processing Group
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

#include <stdio.h>
#include  "wx/wx.h"
#include  "wx/filename.h"
#include <Viewnix.h>
#include  "cv3dv.h"
#include <assert.h>
#include  "port_data/from_dicom.h"
#include <stdlib.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif


#if ! defined (WIN32) && ! defined (_WIN32)
#define PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
#endif

#define TEXT_SIZE 2048

#define switch_acquisition(i1, i2) switch_index((short *)(i1), (short *)(i2))

static int rec_arch_type; /* 0, 1, 2 or 3, depending on the architecture of the ACR-NEMA receiver */
				/* see "read_acrnema.c" for definitions */

static int skip; /* number of images to skip */


/*-------------------------------------------------------------------------------------*/
double hms_to_s(const char hms[])
{
	char h[3], m[3], *s;

	for (s=(char *)hms; *s&&*s!='.'; s++)
		;
	if (s < hms+6)
		return -1.;
	s -= 2;
	h[0] = s[-4];
	h[1] = s[-3];
	m[0] = s[-2];
	m[1] = s[-1];
	h[2] = m[2] = 0;
	return 60*(60*atoi(h)+atoi(m))+atof(s);
}

/*-------------------------------------------------------------------------------------*/
void switch_location(float *l1, float *l2)
{
	float t;

	t = *l1;
	*l1 = *l2;
	*l2 = t;
}


/*-------------------------------------------------------------------------------------*/
void switch_index(short *i1, short *i2)
{
	short t;

	t = *i1;
	*i1 = *i2;
	*i2 = t;
}

/*-------------------------------------------------------------------------------------*/
/*    Modified: 12/11/98 rewritten by Dewey Odhner */
void swap_bytes(void *in, void *out, int size)
{
	int i;
	unsigned char *tin, *tout, temp;

	tin = (unsigned char *)in;
	tout = (unsigned char *)out;

	for(i=0; i<size; i+=2)
	{
		temp = tin[i];
		tout[i] = tin[i+1];
		tout[i+1] = temp;
	}
}

void swap_4_bytes(void *in, void *out, int size)
{
	int i;
	unsigned char *tin, *tout, temp;

	tin = (unsigned char *)in;
	tout = (unsigned char *)out;

	for(i=0; i<size; i+=4)
	{
		temp = tin[i];
		tout[i] = tin[i+3];
		tout[i+3] = temp;
		temp = tin[i+1];
		tout[i+1] = tin[i+2];
		tout[i+2] = temp;
	}
}

/*-------------------------------------------------------------------------------------*/
/*    Modified: 4/20/95 Image Position used rather than Slice Location
 *       for location of subscenes by Dewey Odhner
 *    Modified: 4/21/95 axis labels corrected by Dewey Odhner
 *    Modified: 4/28/95 Image Position or Slice Location used
 *       for location of subscenes by Dewey Odhner
 *    Modified: 8/28/95 byte swapping cases corrected by Dewey Odhner
 *    Modified: 5/9/97 error corrected handling case where
 *       patient orientation is not given by Dewey Odhner
 *    Modified: 12/8/98 to work with other file naming schemes by Dewey Odhner
 *    Modified: 12/15/98 to use DICOM transfer syntax by Dewey Odhner
 *    Modified: 2/18/99 to work with other file naming schemes by Dewey Odhner
 *    Modified: 5/12/00 fclose call moved by Dewey Odhner
 *    Modified: 7/20/00 correct_min_max called by Dewey Odhner
 *    Modified: 8/4/00 bits boosted to 8 or 16 by Dewey Odhner
 *    Modified: 2/27/03 series UID used by Dewey Odhner
 *    Modified: 10/14/03 bug fixed-- wrong file was used to get header info
 *       by Dewey Odhner
 *    Modified: 11/29/05 at array bounds read error corrected by Dewey Odhner.
 *    Modified: 12/5/05 vh initialized to zero by Dewey Odhner.
 *    Modified: 10/15/07 bytes not swapped in 8-bit images by Dewey Odhner.
 *    Modified: 10/23/07 display data converted by Dewey Odhner.
 */
int main(int argc, char *argv[])
{
	ViewnixHeader vh;
	int nslices[MAX_SERIES];	/* #of slices in each series */
	int totslices=0;	/* total #of slices */
	int nframes[MAX_SERIES];
	int totframes=0;
	short *index[MAX_SERIES];	/* table with the indices of the slices (in monotonic order) */
	float *location[MAX_SERIES];/* table with the location of each slice */
	float *slope[MAX_SERIES];  /* table with the rescale slope of each slice */
	unsigned short *acquisition[MAX_SERIES];/* table with the acquisition
		number of each slice */
	float delta;	/* distance between consecutive slices */
	char *output_file, *temp_output_file;
	char *command_line, scratch[TEXT_SIZE];
	char **input_list_array=NULL;
	int input_list_array_size=100;
	char uid[65];
	float bounds[2];

	char group[5], element[5];

	int i, j, k, dir_entry;
	FILE *fpin, *fpout;
	int flag;
	int items_read;
	int bigendian;
	union {unsigned short s; unsigned char c[2];} testbytes;
	int use_acquisition=0;

	/* Elements */
	char at[500];
	char an[500];
	unsigned short bi;
	unsigned int bd;
	float floats[20];

	/* Image data */
	unsigned char *data=NULL;

	int image_size;
	float *x3_axis=NULL; /* direction vector of X_3 axis in device coord. system*/
	int R_axis, C_axis;
	char *chrp;
	Char30 scn_axis_label[3];
	int slice_location_valid;
	char **unrecognized_filename[MAX_SERIES];
	char *last_filename, *cur_filename;
	int max_unrecognized_names[MAX_SERIES];
	int new_series=TRUE,
		new_exam;
	int a, b=0, nseries;
	void *tmpptr;
	char *(series_uid[MAX_SERIES]), *suid;
	float vol_time[MAX_SERIES], slice_spacing=1;
	float cine_rate = -1; // to read from 0018,0040
	int input_list_from_file, retain_patient, order_by_time, wrt_patient=0;
	int add_value=0;
	int rename_dicom=0;
	int slope_flag=0;
	float max_slope=0, slope_unit, DoseGridScaling=0, *GridFrameOffsetVector;
	int OffsetVectorSize=0;
	int move_dicom_subset=0;
	float DoseGridRescaling=1;


	testbytes.s = 0x100;
	bigendian = testbytes.c[0];
	if (argc>3 &&
	      sscanf(argv[argc-1], "-DoseGridRescaling=%f", &DoseGridRescaling)==1)
		argc--;
	if (argc>4 && strcmp(argv[argc-2], "-move_dicom_subset")==0)
	{
		move_dicom_subset = 1;
		fpin = fopen(argv[argc-1], "rb");
		if (fpin == NULL)
		{
			fprintf(stderr, "Cannot open %s\n", argv[argc-1]);
			exit(1);
		}
		j = VReadHeader(fpin, &vh, group,element);
		if (j && j<106)
		{
			fprintf(stderr, "Cannot read scene header.\n");
			exit(1);
		}
		if (vh.scn.dimension != 3)
		{
			fprintf(stderr, "Cannot use %d-dimensional scene.\n",
				vh.scn.dimension);
			exit(1);
		}
		fclose(fpin);
		if (vh.scn.domain_valid && vh.scn.domain[11]<.99)
		{
			fprintf(stderr, "Only axial scans handled\n");
			exit(1);
		}
		bounds[0] = vh.scn.loc_of_subscenes[0]-.0078125;
		bounds[1] =
			vh.scn.loc_of_subscenes[vh.scn.num_of_subscenes[0]-1]+.0078125;
		if (vh.scn.domain_valid)
		{
			bounds[0] += vh.scn.domain[2];
			bounds[1] += vh.scn.domain[2];
		}
		argc -= 2;
	}
	if (argc>3 && strcmp(argv[argc-1], "-use-acquisition")==0)
	{
		use_acquisition = 1;
		argc--;
	}
	if (argc>3 && sscanf(argv[argc-1], "-slopex%f", &slope_unit)==1)
	{
		slope_flag = 1;
		argc--;
	}
	if (argc>3 && strcmp(argv[argc-1], "-rename")==0)
	{
		rename_dicom = 1;
		argc--;
	}
	if (argc>3 && sscanf(argv[argc-1], "+%d", &add_value)==1)
		argc--;
	if(argc < 3)
	{
		fprintf(stderr, "Usage: from_dicom [ -l <input_list_file> | <input_file> ... ] <output_file> [-p] [-t] [+<add_value>] [-rename] [-slopex<unit>] [-use-acquisition] [-move_dicom_subset <subset_scene>] [-DoseGridRescaling=<new_scaling>]\n");
		exit(1);
	}
	input_list_from_file = strcmp(argv[1], "-l")==0;
	order_by_time = strcmp(argv[argc-1], "-t")==0;
	if (order_by_time)
		argc--;
	retain_patient = strcmp(argv[argc-1], "-p")==0;
	if (retain_patient)
		argc--;

	/* 'rec_arch_type' is checked to perform SWAP of image bytes */
	rec_arch_type = 0;
	skip = 0;

	/* Build command line */
	strcpy(scratch, "");

	for(i=0; i<argc &&
			strlen(scratch)+strlen(argv[i])+1<sizeof(scratch); i++)
	{
		strcat(scratch, argv[i]);
		strcat(scratch, " ");
	}
	command_line = (char *)malloc(strlen(scratch)+1);
	strcpy(command_line, scratch);

	/* Get Output filename */
	output_file = argv[argc-1];
	if (rename_dicom && strlen(output_file)+14 >= TEXT_SIZE)
	{
		fprintf(stderr, "File name is too long.\n");
		exit(1);
	}
	if (move_dicom_subset)
	{
		wxFileName dn(output_file, wxEmptyString);
		if (!dn.DirExists())
			dn.Mkdir(0755);
		if (!dn.IsDirWritable())
		{
			fprintf(stderr, "%s not writable\n", output_file);
			exit(1);
		}
	}


	/* Gather information about each series */
	{
		if (input_list_from_file)
		{
			FILE *input_list_file;

			input_list_file = fopen(argv[2], "rb");
			if (input_list_file == NULL)
			{
				fprintf(stderr, "Could not open file list.\n");
				exit(-1);
			}
			while (!feof(input_list_file))
				if (fgets(scratch, sizeof(scratch), input_list_file))
				{
					while (scratch[0] && (scratch[strlen(scratch)-1]=='\n' ||
							scratch[strlen(scratch)-1]=='\r'))
						scratch[strlen(scratch)-1] = 0;
					if (scratch[0] == 0)
						continue;
					if (input_list_array == NULL)
						input_list_array = (char **)
							malloc(input_list_array_size*sizeof(char *));
					else if (totslices == input_list_array_size)
					{
						input_list_array_size += 100;
						input_list_array = (char **)realloc(input_list_array,
							input_list_array_size*sizeof(char *));
					}
					input_list_array[totslices] =
						(char *)malloc(strlen(scratch)+1);
					strcpy(input_list_array[totslices], scratch);
					totslices++;
				}
			fclose(input_list_file);
		}
		else
		{
			totslices = argc-2;
			input_list_array_size = totslices;
			input_list_array =
				(char **)malloc(input_list_array_size*sizeof(char *));
			for(i=1; i<argc-1; i++)
				input_list_array[i-1] = argv[i];
		}
		totframes = 0;

		memset(series_uid, 0, sizeof(series_uid));
		unrecognized_filename[0] = (char **)malloc(256*sizeof(char *));
		if (unrecognized_filename[0] == NULL)
		{
		    fprintf(stderr, "Out of memory.\n");
		    exit(-1);
	    }
		max_unrecognized_names[0] = 256;

		/* Check all the different series files in the directory specified */
		last_filename = (char *)"*";
		k = 0;
		nseries = 0;
		new_exam = TRUE;
		/* For each file on the directory */
		for (dir_entry=0; dir_entry<totslices; dir_entry++)
		{
			cur_filename = input_list_array[dir_entry];

			/* Get information about ACRNEMA file */
			a = wxFileName::FileExists(cur_filename) &&
				!wxFileName(cur_filename).IsDir();
			if(a>0)
				b= check_acrnema_file(NULL, cur_filename);

			/* regular File and also ACR-NEMA */
			if( a > 0 &&  b == 0)
			{
				k++;

				suid = get_series_uid(NULL, cur_filename);
				if (suid == NULL)
				{
					suid = (char *)malloc(sizeof(DUMMY_SUID));
					strcpy(suid, DUMMY_SUID);
				}
				for (j=0; j<nseries; j++)
					if (series_uid[j] && strcmp(suid, series_uid[j])==0)
						break;
				if (j < nseries)
				{
					new_series = FALSE;
					free(suid);
				}
				else
				{
					new_series = TRUE;
					series_uid[j] = suid;
					vol_time[j] = get_time(cur_filename);
				}

				/* NEW SERIES */
				if(new_series == TRUE)
				{
					nseries++;
					nslices[j] = 0;
					nframes[j] = 0;
					unrecognized_filename[j] =
						(char **)malloc(256*sizeof(char *));
					if (unrecognized_filename[j] == NULL)
					{
						fprintf(stderr, "ERROR: Out of memory");
						exit(-1);
					}
					max_unrecognized_names[j] = 256;
				}
				if (nslices[j] >= max_unrecognized_names[j])
				{
					tmpptr = realloc(unrecognized_filename[j],
						(max_unrecognized_names[j]+256)*sizeof(char*));
					if (tmpptr == NULL)
					{
						fprintf(stderr, "ERROR: Out of memory");
						exit(-1);
					}
					max_unrecognized_names[j] += 256;
					unrecognized_filename[j] = (char **)tmpptr;
				}
				unrecognized_filename[j][nslices[j]] = cur_filename;
				last_filename = unrecognized_filename[j][nslices[j]];
				nslices[j]++;
				int cur_num_frames = get_num_frames(cur_filename);
				nframes[j] += cur_num_frames;
				if (cur_num_frames > OffsetVectorSize)
				{
					float *cur_OffsetVector =
					    get_OffsetVector(cur_filename, cur_num_frames);
					if (cur_OffsetVector)
					{
						if (OffsetVectorSize)
							free(GridFrameOffsetVector);
						GridFrameOffsetVector = cur_OffsetVector;
						OffsetVectorSize = cur_num_frames;
					}
				}
			}
		}
		for(i=0; i<nseries; i++)
		{
			printf("File [%s] contains %d slices.\n",
				unrecognized_filename[i][skip], nslices[i]);
			location[i] = (float *)calloc(nslices[i]*sizeof(float), 1);
			if (slope_flag)
				slope[i] = (float *)calloc(nslices[i]*sizeof(float), 1);
			acquisition[i]=(unsigned short*)calloc(nslices[i]*sizeof(short),1);
			index[i] = (short *) calloc(nslices[i]*sizeof(short), 1);
			if (location[i]==NULL || acquisition[i]==NULL || index[i]==NULL)
			{
				fprintf(stderr, "ERROR: Out of memory");
				exit(-1);
			}
			totframes += nframes[i];
		}
		cur_filename = unrecognized_filename[0][skip];
	}
	// sort series by time
	{
		float t, *l, *slp;
		char *s, **u;
		int f, m, n, p, q, mun;
		short *indx;
		unsigned short *ac;
		for (m=1; m<nseries; m++)
		{
			t = vol_time[m];
			s = series_uid[m];
			indx = index[m];
			n = nslices[m];
			f = nframes[m];
			l = location[m];
			if (slope_flag)
				slp = slope[m];
			ac = acquisition[m];
			u = unrecognized_filename[m];
			mun = max_unrecognized_names[m];
			for (p=0; p<m; p++)
				if (vol_time[p] > t)
					break;
			for (q=m-1; q>p; q--)
			{
				vol_time[q+1] = vol_time[q];
				series_uid[q+1] = series_uid[q];
				index[q+1] = index[q];
				nslices[q+1] = nslices[q];
				nframes[q+1] = nframes[q];
				location[q+1] = location[q];
				if (slope_flag)
					slope[q+1] = slope[q];
				acquisition[q+1] = acquisition[q];
				unrecognized_filename[q+1] = unrecognized_filename[q];
				max_unrecognized_names[q+1] = max_unrecognized_names[q];
			}
			vol_time[p] = t;
			series_uid[p] = s;
			index[p] = indx;
			nslices[p] = n;
			nframes[q] = f;
			location[p] = l;
			if (slope_flag)
				slope[p] = slp;
			acquisition[p] = ac;
			unrecognized_filename[p] = u;
			max_unrecognized_names[p] = mun;
		}
	}

	/* BUILD Viewnix.hEADER (based on first slice) */
	if( (fpin=fopen(cur_filename, "rb")) == NULL)
	{
		fprintf(stderr, "ERROR: Can't open [%s] !\n", cur_filename);
		fflush(stderr);
		exit(3);
	}


	memset(&vh, 0, sizeof(vh));
	/*******************************************************************************/
	/* <<< General >>> */
	/*******************************************************************************/
	if (get_element(fpin, 2, 0x10, AT, uid, 64, &items_read) ||
			strcmp(uid, "1.2.840.10008.1.2 ")==0 ||
			strcmp(uid, "1.2.840.10008.1.2")==0 ||
			strcmp(uid, "1.2.840.10008.1.2.1")==0 ||
			strcmp(uid, "1.2.840.10008.1.2.1 ")==0)
		rec_arch_type = 2;

	vh.gen.recognition_code_valid = 1;
	strcpy(vh.gen.recognition_code, "VIEWNIX1.0");
	
	vh.gen.study_date_valid = get_element(fpin, 0x0008, 0x0020, AT,
		vh.gen.study_date, sizeof(vh.gen.study_date), &items_read) == 0;

	vh.gen.study_time_valid = get_element(fpin, 0x0008, 0x0030, AT,
		vh.gen.study_time, sizeof(vh.gen.study_time), &items_read) == 0;

	vh.gen.data_type_valid = 1;
	vh.gen.data_type = strcmp(output_file+strlen(output_file)-4, ".MV0")==0?
		MOVIE0: IMAGE0;

	vh.gen.modality_valid = get_element(fpin, 0x0008, 0x0060, AT,
		vh.gen.modality, sizeof(vh.gen.modality), &items_read) == 0;

	vh.gen.institution_valid = get_element(fpin, 0x0008, 0x0080, AT,
		vh.gen.institution, sizeof(vh.gen.institution), &items_read) == 0;

	vh.gen.physician_valid = get_element(fpin, 0x0008, 0x0090, AT,
		vh.gen.physician, sizeof(vh.gen.physician), &items_read) == 0;

	vh.gen.department_valid = get_element(fpin, 0x0008, 0x1040, AT,
		vh.gen.department, sizeof(vh.gen.department), &items_read) == 0;

	vh.gen.radiologist_valid = get_element(fpin, 0x0008, 0x1060, AT,
		vh.gen.radiologist, sizeof(vh.gen.radiologist), &items_read) == 0;

	vh.gen.model_valid = get_element(fpin, 0x0008, 0x1090, AT, vh.gen.model,
		sizeof(vh.gen.model), &items_read) == 0;

	vh.gen.filename_valid = 1;
	strcpy(vh.gen.filename, output_file);

	vh.gen.filename1_valid = 1;
	strcpy(vh.gen.filename1, "UPenn PACS");

	vh.gen.description_valid = 1;
	vh.gen.description = (char *) calloc(20, 1);
	strcpy(vh.gen.description, "");

	vh.gen.comment_valid = 1;
	vh.gen.comment = command_line;

	if (retain_patient)
	{
		vh.gen.patient_name_valid = get_element(fpin, 0x0010, 0x0010, AT,
			vh.gen.patient_name, sizeof(vh.gen.patient_name), &items_read)== 0;

		vh.gen.patient_id_valid = get_element(fpin, 0x0010, 0x0020, AT,
			vh.gen.patient_id, sizeof(vh.gen.patient_id), &items_read) == 0;
	}

	vh.gen.slice_thickness_valid = get_element(fpin, 0x0018, 0x0050, AN,
		an, sizeof(an), &items_read) == 0;
	if (vh.gen.slice_thickness_valid)
		extract_floats(an, 1, &vh.gen.slice_thickness);

	vh.gen.kvp_valid =
		get_element(fpin, 0x0018, 0x0060, AN, an, sizeof(an), &items_read) ==0;
	if (items_read > 2)
		items_read = 2;
	vh.gen.kvp[1] = 0;
	if (vh.gen.kvp_valid)
		extract_floats(an, items_read, vh.gen.kvp);

	vh.gen.repetition_time_valid = get_element(fpin, 0x0018, 0x0080, AN,
		an, sizeof(an), &items_read) == 0;
	if (vh.gen.repetition_time_valid)
		extract_floats(an, 1, &vh.gen.repetition_time);

	vh.gen.echo_time_valid = get_element(fpin, 0x0018, 0x0081, AN,
		an, sizeof(an), &items_read) == 0;
	if (vh.gen.echo_time_valid)
		extract_floats(an, 1, &vh.gen.echo_time);

	vh.gen.imaged_nucleus_valid = 1;
	get_element(fpin, 0x0018, 0x0085, AT, at, sizeof(at), &items_read);
	if (items_read==0 || at[0]==0)
		strcpy(at, " ");
	vh.gen.imaged_nucleus = (char *) calloc( strlen(at)+1, 1);
	strcpy(vh.gen.imaged_nucleus, at);

	vh.gen.gantry_tilt_valid = get_element(fpin, 0x0018, 0x1120, AN,
		an, sizeof(an), &items_read) == 0;
	if (vh.gen.gantry_tilt_valid)
		extract_floats(an, 1, &vh.gen.gantry_tilt);

	vh.gen.study_valid = get_element(fpin, 0x0020, 0x0010, AT,
		vh.gen.study, sizeof(vh.gen.study), &items_read) == 0;

	vh.gen.series_valid = get_element(fpin, 0x0020, 0x0011, AT,
		vh.gen.series, sizeof(vh.gen.series), &items_read) == 0;

	vh.gen.gray_descriptor_valid = 0;
	vh.gen.red_descriptor_valid = 0;
	vh.gen.green_descriptor_valid = 0;
	vh.gen.blue_descriptor_valid = 0;
	vh.gen.gray_lookup_data_valid = 0;
	vh.gen.red_lookup_data_valid = 0;
	vh.gen.green_lookup_data_valid = 0;
	vh.gen.blue_lookup_data_valid = 0;
/*@ Ignore lookup data for now.
	vh.gen.gray_descriptor[0] = ;
	vh.gen.gray_descriptor[1] = ;
	vh.gen.gray_descriptor[2] = ;
	vh.gen.red_descriptor[0] = ;
	vh.gen.red_descriptor[1] = ;
	vh.gen.red_descriptor[2] = ;
	vh.gen.green_descriptor[0] = ;
	vh.gen.green_descriptor[1] = ;
	vh.gen.green_descriptor[2] = ;
	vh.gen.blue_descriptor[0] = ;
	vh.gen.blue_descriptor[1] = ;
	vh.gen.blue_descriptor[2] = ;
*/

	if (vh.gen.data_type == IMAGE0)
	{
		/* <<< Scene >>> */

		vh.scn.dimension_valid = 1;
		if(nseries > 1)
		{
			vh.scn.dimension = 4;
			vh.scn.domain = (float *) calloc((4+4*4)*sizeof(float), 1);
		}
		else
		{
			vh.scn.dimension = 3;
			vh.scn.domain = (float *) calloc((3+3*3)*sizeof(float), 1);
		}

		/* origin */
		vh.scn.domain_valid = !order_by_time && get_element(
			fpin, 0x0020, 0x0030, AN, an, sizeof(an), &items_read) ==0;
		if (!order_by_time && get_element(fpin, 0x0020, 0x0032, AN, an,
				sizeof(an), &items_read) ==0)
		{
			wrt_patient = 1;
			vh.scn.domain_valid = 1;
		}
		if (vh.scn.domain_valid)
			extract_floats(an, 3, vh.scn.domain);
		else
		{
			vh.scn.domain[0] = 0;
			vh.scn.domain[1] = 0;
			vh.scn.domain[2] = 0;
		}
		if(nseries > 1)
			vh.scn.domain[3] = 0.0;

		if (vh.scn.domain_valid)
			vh.scn.domain_valid = get_element(fpin, 0x0020, wrt_patient?
				0x0037:0x0035, AN, an, sizeof(an), &items_read) == 0;
		if (vh.scn.domain_valid)
			extract_floats(an, 6, floats);
		if(nseries == 1)
		{
			if (vh.scn.domain_valid == 0)
			{
			/* X1 unit vector */
			vh.scn.domain[3] = 1;
			vh.scn.domain[4] = 0;
			vh.scn.domain[5] = 0;
			/* X2 unit vector */
			vh.scn.domain[6] = 0;
			vh.scn.domain[7] = 1;
			vh.scn.domain[8] = 0;
			/* X3 unit vector */
			vh.scn.domain[9] = 0;
			vh.scn.domain[10] = 0;
			vh.scn.domain[11] = 1;
			}
			else
			{
			/* X1 unit vector */
			vh.scn.domain[3] = floats[0];
			vh.scn.domain[4] = floats[1];
			vh.scn.domain[5] = floats[2];
			/* X2 unit vector */
			vh.scn.domain[6] = floats[3];
			vh.scn.domain[7] = floats[4];
			vh.scn.domain[8] = floats[5];
			/* X3 unit vector */
			vh.scn.domain[9] = floats[1]*floats[5] - floats[4]*floats[2];
			vh.scn.domain[10] = floats[3]*floats[2] - floats[0]*floats[5];
			vh.scn.domain[11] = floats[0]*floats[4] - floats[3]*floats[1];
			}
			x3_axis = vh.scn.domain+9;
		}
		else
		{
			if (vh.scn.domain_valid == 0)
			{
			/* X1 unit vector */
			vh.scn.domain[4] = 1;
			vh.scn.domain[5] = 0;
			vh.scn.domain[6] = 0;
			vh.scn.domain[7] = 0.0;
			/* X2 unit vector */
			vh.scn.domain[8] = 0;
			vh.scn.domain[9] = 1;
			vh.scn.domain[10] = 0;
			vh.scn.domain[11] = 0.0;
			/* X3 unit vector */
			vh.scn.domain[12] = 0;
			vh.scn.domain[13] = 0;
			vh.scn.domain[14] = 1;
			vh.scn.domain[15] = 0.0;
			/* X4 unit vector */
			vh.scn.domain[16] = 0.0;
			vh.scn.domain[17] = 0.0;
			vh.scn.domain[18] = 0.0;
			vh.scn.domain[19] = 1.0;
			}
			else
			{
			/* X1 unit vector */
			vh.scn.domain[4] = floats[0];
			vh.scn.domain[5] = floats[1];
			vh.scn.domain[6] = floats[2];
			vh.scn.domain[7] = 0.0;
			/* X2 unit vector */
			vh.scn.domain[8] = floats[3];
			vh.scn.domain[9] = floats[4];
			vh.scn.domain[10] = floats[5];
			vh.scn.domain[11] = 0.0;
			/* X3 unit vector */
			vh.scn.domain[12] = floats[1]*floats[5] - floats[4]*floats[2];
			vh.scn.domain[13] = floats[3]*floats[2] - floats[0]*floats[5];
			vh.scn.domain[14] = floats[0]*floats[4] - floats[3]*floats[1];
			vh.scn.domain[15] = 0.0;
			/* X4 unit vector */
			vh.scn.domain[16] = 0.0;
			vh.scn.domain[17] = 0.0;
			vh.scn.domain[18] = 0.0;
			vh.scn.domain[19] = 1.0;
			}
			x3_axis = vh.scn.domain+12;
		
		}
		slice_location_valid =
			get_element(fpin, 0x0020, 0x0050, AN, an, sizeof(an), &items_read)
			== 0 ||
			get_element(fpin, 0x0020, 0x1041, AN, an, sizeof(an), &items_read)
			== 0;
		if (slice_location_valid)
		{	extract_floats(an, 1, floats);
			/* Offset location of scene domain. */
			if(nseries == 1)
			{	vh.scn.domain[0] += floats[0]*vh.scn.domain[9];
				vh.scn.domain[1] += floats[0]*vh.scn.domain[10];
				vh.scn.domain[2] += floats[0]*vh.scn.domain[11];
			}
			else
			{	vh.scn.domain[0] += floats[0]*vh.scn.domain[12];
				vh.scn.domain[1] += floats[0]*vh.scn.domain[13];
				vh.scn.domain[2] += floats[0]*vh.scn.domain[14];
			}
		}
		vh.scn.domain_valid = 1;

		if (get_element(fpin, 0x0018, 0x0088, AN, an, sizeof(an), &items_read)
				== 0)
			extract_floats(an, 1, &slice_spacing);

		/* to  check the delta_t and fps= cine_rate , and then write it into the header, by yubing */		
		if (get_element(fpin, 0x0018, 0x0040, AN, an, sizeof(an), &items_read)
				== 0)
			extract_floats(an, 1, &cine_rate);	
			if (cine_rate > 0)
			{
				slice_spacing = 1.0 / cine_rate;	
			}


		vh.scn.axis_label_valid =
			get_element(fpin, 0x0020,0x0020, AT, at,sizeof(at),&items_read)==0;
		if (items_read==0 || at[0]==0)
			strcpy(at, " ");
		if (vh.scn.axis_label_valid)
		{
			for (chrp=at; *chrp==' '; chrp++)
				;
			R_axis = *chrp;
			while (*chrp != '\\')
				chrp++;
			for (chrp++; *chrp==' '; chrp++)
				;
			C_axis = *chrp;
		}
		else
		{
			R_axis = 'L';
			C_axis = 'P';
		}
		switch (R_axis)
		{	case 'A':
				strcpy(scn_axis_label[0], "Anterior");
				switch (C_axis)
				{	case 'F':
						strcpy(scn_axis_label[1], "Feet");
						strcpy(scn_axis_label[2], "Left");
						break;
					case 'H':
						strcpy(scn_axis_label[1], "Head");
						strcpy(scn_axis_label[2], "Right");
						break;
					case 'L':
						strcpy(scn_axis_label[1], "Left");
						strcpy(scn_axis_label[2], "Head");
						break;
					case 'R':
						strcpy(scn_axis_label[1], "Right");
						strcpy(scn_axis_label[2], "Feet");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			case 'P':
				strcpy(scn_axis_label[0], "Posterior");
				switch (C_axis)
				{	case 'F':
						strcpy(scn_axis_label[1], "Feet");
						strcpy(scn_axis_label[2], "Right");
						break;
					case 'H':
						strcpy(scn_axis_label[1], "Head");
						strcpy(scn_axis_label[2], "Left");
						break;
					case 'L':
						strcpy(scn_axis_label[1], "Left");
						strcpy(scn_axis_label[2], "Feet");
						break;
					case 'R':
						strcpy(scn_axis_label[1], "Right");
						strcpy(scn_axis_label[2], "Head");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			case 'F':
				strcpy(scn_axis_label[0], "Feet");
				switch (C_axis)
				{	case 'A':
						strcpy(scn_axis_label[1], "Anterior");
						strcpy(scn_axis_label[2], "Right");
						break;
					case 'P':
						strcpy(scn_axis_label[1], "Posterior");
						strcpy(scn_axis_label[2], "Left");
						break;
					case 'L':
						strcpy(scn_axis_label[1], "Left");
						strcpy(scn_axis_label[2], "Anterior");
						break;
					case 'R':
						strcpy(scn_axis_label[1], "Right");
						strcpy(scn_axis_label[2], "Posterior");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			case 'H':
				strcpy(scn_axis_label[0], "Head");
				switch (C_axis)
				{	case 'A':
						strcpy(scn_axis_label[1], "Anterior");
						strcpy(scn_axis_label[2], "Left");
						break;
					case 'P':
						strcpy(scn_axis_label[1], "Posterior");
						strcpy(scn_axis_label[2], "Right");
						break;
					case 'L':
						strcpy(scn_axis_label[1], "Left");
						strcpy(scn_axis_label[2], "Posterior");
						break;
					case 'R':
						strcpy(scn_axis_label[1], "Right");
						strcpy(scn_axis_label[2], "Anterior");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			case 'L':
				strcpy(scn_axis_label[0], "Left");
				switch (C_axis)
				{	case 'A':
						strcpy(scn_axis_label[1], "Anterior");
						strcpy(scn_axis_label[2], "Feet");
						break;
					case 'P':
						strcpy(scn_axis_label[1], "Posterior");
						strcpy(scn_axis_label[2], "Head");
						break;
					case 'F':
						strcpy(scn_axis_label[1], "Feet");
						strcpy(scn_axis_label[2], "Posterior");
						break;
					case 'H':
						strcpy(scn_axis_label[1], "Head");
						strcpy(scn_axis_label[2], "Anterior");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			case 'R':
				strcpy(scn_axis_label[0], "Right");
				switch (C_axis)
				{	case 'A':
						strcpy(scn_axis_label[1], "Anterior");
						strcpy(scn_axis_label[2], "Head");
						break;
					case 'P':
						strcpy(scn_axis_label[1], "Posterior");
						strcpy(scn_axis_label[2], "Feet");
						break;
					case 'F':
						strcpy(scn_axis_label[1], "Feet");
						strcpy(scn_axis_label[2], "Anterior");
						break;
					case 'H':
						strcpy(scn_axis_label[1], "Head");
						strcpy(scn_axis_label[2], "Posterior");
						break;
					default:
						vh.scn.axis_label_valid = 0;
						break;
				}
				break;
			default:
				vh.scn.axis_label_valid = 0;
				break;
		}

		vh.scn.axis_label = scn_axis_label;

		/* Measurement unit along each dimension */
		vh.scn.measurement_unit_valid = 1;
		vh.scn.measurement_unit = (short *) calloc(4, sizeof(short));
		vh.scn.measurement_unit[0] = 3;
		vh.scn.measurement_unit[1] = 3;
		vh.scn.measurement_unit[2] = order_by_time || cine_rate>0? 5: 3;
		vh.scn.measurement_unit[3] = 5;

		vh.scn.num_of_density_values_valid = 1;
		vh.scn.num_of_density_values = 1;

		vh.scn.smallest_density_value_valid = get_element(
			fpin, 0x0028, 0x0108, BI, &bi, sizeof(bi), &items_read) == 0;
		vh.scn.smallest_density_value = (float *) calloc(sizeof(float), 1);
		vh.scn.smallest_density_value[0] =
			vh.scn.smallest_density_value_valid? bi: 0;

		vh.scn.largest_density_value_valid = get_element(
			fpin, 0x0028, 0x0109, BI, &bi, sizeof(bi), &items_read) == 0;
		vh.scn.largest_density_value = (float *) calloc(sizeof(float), 1);
		vh.scn.largest_density_value[0] =
			vh.scn.largest_density_value_valid? bi: 4096;

		vh.scn.num_of_integers_valid = 1;
		vh.scn.num_of_integers = 1;

		vh.scn.signed_bits = (short *) calloc(sizeof(short), 1);

        if (get_element(fpin, 0x3004, 0x000e, AN, an,
				sizeof(an), &items_read) == 0)
		{
			assert(items_read == 1);
			extract_floats(an, 1, &DoseGridScaling);
			assert(DoseGridScaling > 0);
		}

		if (slope_flag || DoseGridScaling>0)
		{
			vh.scn.signed_bits_valid = 1;
			vh.scn.num_of_bits = 16;
			vh.scn.num_of_bits_valid = 1;
		}
		else
		{
			vh.scn.signed_bits_valid = get_element(fpin, 0x0028, 0x0103, BI,
				&bi, sizeof(bi), &items_read) == 0;
			vh.scn.signed_bits[0] = vh.scn.signed_bits_valid? bi: 0;

			vh.scn.num_of_bits_valid = get_element(fpin, 0x0028, 0x0100, BI,
				&bi, sizeof(bi), &items_read) == 0;
			assert(vh.scn.num_of_bits_valid == items_read);
			if (vh.scn.num_of_bits_valid == 0)
			{
				bi = 16;
				fprintf(stderr,
					"Warning: bits per pixel not specified. Assuming 16.\n");
			}
			if (bi < 8)
				bi = 8;
			if (bi>8 && bi<16)
				bi = 16;
			vh.scn.num_of_bits = bi;
		}

		vh.scn.bit_fields_valid = 1;
		vh.scn.bit_fields = (short *) calloc(2*sizeof(short), 1);
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = vh.scn.num_of_bits-1;

		vh.scn.dimension_in_alignment_valid = 1;
		vh.scn.dimension_in_alignment = 2;

		vh.scn.bytes_in_alignment_valid = 1;
		vh.scn.bytes_in_alignment = 1;

		/* IMAGE DIMENSIONS */
		vh.scn.xysize_valid = get_element(fpin, 0x0028, 0x0011, BI,
			&bi, sizeof(bi), &items_read) == 0;
		if (vh.scn.xysize_valid)
			vh.scn.xysize[0] = bi;
		if (vh.scn.xysize_valid)
			vh.scn.xysize_valid = get_element(fpin, 0x0028, 0x0010, BI,
				&bi, sizeof(bi), &items_read) == 0;
		if (vh.scn.xysize_valid)
			vh.scn.xysize[1] = bi;
		if (vh.scn.xysize_valid == 0)
		{
			fprintf(stderr, "ERROR: Can't open image dimensions.\n");
			exit(104);
		}

		image_size= (vh.scn.xysize[0]*vh.scn.xysize[1]*vh.scn.num_of_bits+7)/8;

		/* NUMBER OF IMAGES */
		vh.scn.num_of_subscenes_valid = 1;
		if(nseries > 1)
		{
			vh.scn.num_of_subscenes = (short *) calloc( 1+nseries, sizeof(short));
			vh.scn.num_of_subscenes[0] = nseries;
			for(i=0; i<nseries; i++)
				vh.scn.num_of_subscenes[i+1] = nframes[i]-skip;
		}
		else
		{
			vh.scn.num_of_subscenes = (short *) calloc( 1*sizeof(short), 1);
			vh.scn.num_of_subscenes[0] = nframes[0]-skip;
		}

		/* PIXEL SIZE */
		floats[1] = -1;
		vh.scn.xypixsz_valid = get_element(fpin, 0x0028, 0x0030, AN,
			an, sizeof(an), &items_read) == 0;
		if (vh.scn.xypixsz_valid == 0)
			vh.scn.xypixsz_valid = get_element(fpin, 0x0018, 0x1164, AN,
				an, sizeof(an), &items_read) == 0;
		if (vh.scn.xypixsz_valid)
			extract_floats(an, 2, floats);
		else
		{
			floats[0] = floats[1] = 1;
			vh.scn.xypixsz_valid = 1;
		}
		vh.scn.xypixsz[0] = floats[0];
		vh.scn.xypixsz[1] = floats[1]>0? floats[1]: floats[0];
	}
	else
	{
	    /* <<< Movie >>> */

		vh.dsp.dimension = 3;
		vh.dsp.dimension_valid = 1;
		vh.dsp.measurement_unit[0] = 3;
		vh.dsp.measurement_unit[1] = 3;
		vh.dsp.measurement_unit_valid = 1;
		vh.dsp.num_of_elems_valid = get_element(fpin, 0x0028, 0x0002, BI,
			&bi, sizeof(bi), &items_read) == 0;
		vh.dsp.num_of_elems = vh.dsp.num_of_elems_valid? bi: 1;
		vh.dsp.num_of_elems_valid = 1;
		vh.dsp.num_of_integers = vh.dsp.num_of_elems;
		vh.dsp.num_of_integers_valid = 1;

		vh.dsp.num_of_bits_valid = get_element(fpin, 0x0028, 0x0100, BI,
			&bi, sizeof(bi), &items_read) == 0;
		if (vh.dsp.num_of_bits_valid == 0)
		{
			bi = 16;
			fprintf(stderr,
				"Warning: bits per pixel not specified. Assuming 16.\n");
		}
		if (bi < 8)
			bi = 8;
		if (bi>8 && bi<16)
			bi = 16;
		vh.dsp.num_of_bits = bi*vh.dsp.num_of_elems;
		vh.dsp.num_of_bits_valid = 1;

		vh.dsp.smallest_value_valid = get_element(
			fpin, 0x0028, 0x0108, BI, &bi, sizeof(bi), &items_read) == 0;
		vh.dsp.smallest_value =
			(float *)malloc(sizeof(float)*vh.dsp.num_of_elems);
		for (j=0; j<vh.dsp.num_of_elems; j++)
			vh.dsp.smallest_value[j] = vh.dsp.smallest_value_valid? bi: 0;
		vh.dsp.smallest_value_valid = 1;

		vh.dsp.largest_value_valid = get_element(
			fpin, 0x0028, 0x0109, BI, &bi, sizeof(bi), &items_read) == 0;
		vh.dsp.largest_value =
			(float *)malloc(sizeof(float)*vh.dsp.num_of_elems);
		for (j=0; j<vh.dsp.num_of_elems; j++)
			vh.dsp.largest_value[j] =
				vh.dsp.largest_value_valid? bi:
				(1<<vh.dsp.num_of_bits/vh.dsp.num_of_elems)-1;
		vh.dsp.largest_value_valid = 1;

		vh.dsp.bit_fields =
			(short *)malloc(2*sizeof(short)*vh.dsp.num_of_elems);
		for (j=0; j<vh.dsp.num_of_elems; j++)
		{
			vh.dsp.bit_fields[2*j] = bi*j;
			vh.dsp.bit_fields[2*j+1] = bi*(j+1)-1;
		}
		vh.dsp.bit_fields_valid = 1;

		vh.dsp.num_of_images = totslices;
		vh.dsp.num_of_images_valid = 1;
		vh.dsp.xysize_valid = get_element(fpin, 0x0028, 0x0011, BI,
			&bi, sizeof(bi), &items_read) == 0;
		if (vh.dsp.xysize_valid)
			vh.dsp.xysize[0] = bi;
		if (vh.dsp.xysize_valid)
			vh.dsp.xysize_valid = get_element(fpin, 0x0028, 0x0010, BI,
				&bi, sizeof(bi), &items_read) == 0;
		if (vh.dsp.xysize_valid)
			vh.dsp.xysize[1] = bi;
		if (vh.dsp.xysize_valid == 0)
		{
			fprintf(stderr, "ERROR: Can't open image dimensions.\n");
			exit(104);
		}
		image_size = vh.dsp.xysize[0]*vh.dsp.xysize[1]*vh.dsp.num_of_bits/8;

		/* PIXEL SIZE */
		vh.dsp.xypixsz_valid = get_element(fpin, 0x0028, 0x0030, AN,
			an, sizeof(an), &items_read) == 0;
		if (vh.dsp.xypixsz_valid == 0)
			vh.dsp.xypixsz_valid = get_element(fpin, 0x0018, 0x1164, AN,
				an, sizeof(an), &items_read) == 0;
		if (vh.dsp.xypixsz_valid)
			extract_floats(an, 1, floats);
		else
		{
			floats[0] = 1;
			vh.dsp.xypixsz_valid = 1;
		}
		vh.dsp.xypixsz[0] = floats[0];
		vh.dsp.xypixsz[1] = floats[0];
	}

	/* CLOSE INPUT FILE */
	fclose(fpin);


   if (vh.gen.data_type == IMAGE0)
   {
	/* For each series */
	for(i=0; i<nseries; i++)
	{
		/* Read the location of every slice, and build a table containing */
		/* their indices in correct order (increasing) */

		for(j=skip; j<nslices[i]; j++)
		{
			if( (fpin=fopen(unrecognized_filename[i][j], "rb")) == NULL)
			{
				fprintf(stderr, "ERROR: Can't open [%s] !\n", cur_filename);
				fflush(stderr);
				exit(3);
			}
			vh.scn.loc_of_subscenes_valid = 1;
			if (order_by_time)
			{
				if (get_element(fpin, 0x0008, 0x0032, AT, at, sizeof(at),
					&items_read) || (location[i][j]=(float)hms_to_s(at))<0)
				{
					fprintf(stderr, "ERROR: Can't parse image time.\n");
					exit(-1);
				}
			}
			else if (get_element(fpin, 0x0020, wrt_patient? 0x0032:0x0030, AN,
					an, sizeof(an), &items_read) == 0)
			{
				extract_floats(an, 3, floats);
				location[i][j]= (floats[0]-vh.scn.domain[0])*x3_axis[0]+
								(floats[1]-vh.scn.domain[1])*x3_axis[1]+
								(floats[2]-vh.scn.domain[2])*x3_axis[2];
			}
			else if ((k = get_element(fpin, 0x0020, 0x0050, AN,
					an, sizeof(an), &items_read)) == 0 ||
					(k = get_element(fpin, 0x0020, 0x1041, AN,
					an, sizeof(an), &items_read)) == 0)
				extract_floats(an, 1, location[i]+j);
			else
			{
				fprintf(stderr, "ERROR: Can't get [%s] slice location\n",
					cur_filename);
				if (get_element(fpin, 0x0020, 0x0013, AN, an, sizeof(an),
						&items_read) == 0)
					extract_floats(an, 1, location[i]+j);
				else
					location[i][j] = j*slice_spacing;
				fprintf(stderr, "Using default value %f\n", location[i][j]);
				vh.scn.loc_of_subscenes_valid = 0;
			}
			if (slope_flag)
			{
				if (get_element(fpin, 0x0028, 0x1053, AN, an, sizeof(an),
						&items_read))
				{
					fprintf(stderr, "Rescale Slope not found.\n");
					exit(1);
				}
				extract_floats(an, 1, slope[i]+j);
				if (slope[i][j] > max_slope)
				{
					if (slope[i][j]*slope_unit > 65535)
					{
						fprintf(stderr, "Slope unit is too large.\n");
						exit(1);
					}
					max_slope = slope[i][j];
				}
			}
			if (!use_acquisition)
				acquisition[i][j] = 0;
			else if (get_element(fpin, 0x0020, 0x0012, AN, an, sizeof(an),
					&items_read) == 0)
			{
				extract_floats(an, 1, floats);
				acquisition[i][j] = (unsigned short)floats[0];
			}
			index[i][j] = j;

			/* check orientation */
			if (get_element(fpin, 0x0020, 0x0035, AN,
					an, sizeof(an), &items_read) == 0)
			{	extract_floats(an, 6, floats);
				if(nseries == 1?
						/* X1 unit vector */
						vh.scn.domain[3] > floats[0]+.01 ||
						vh.scn.domain[3] < floats[0]-.01 ||
						vh.scn.domain[4] > floats[1]+.01 ||
						vh.scn.domain[4] < floats[1]-.01 ||
						vh.scn.domain[5] > floats[2]+.01 ||
						vh.scn.domain[5] < floats[2]-.01 ||
						/* X2 unit vector */
						vh.scn.domain[6] > floats[3]+.01 ||
						vh.scn.domain[6] < floats[3]-.01 ||
						vh.scn.domain[7] > floats[4]+.01 ||
						vh.scn.domain[7] < floats[4]-.01 ||
						vh.scn.domain[8] > floats[5]+.01 ||
						vh.scn.domain[8] < floats[5]-.01 :
						/* X1 unit vector */
						vh.scn.domain[4] > floats[0]+.01 ||
						vh.scn.domain[4] < floats[0]-.01 ||
						vh.scn.domain[5] > floats[1]+.01 ||
						vh.scn.domain[5] < floats[1]-.01 ||
						vh.scn.domain[6] > floats[2]+.01 ||
						vh.scn.domain[6] < floats[2]-.01 ||
						/* X2 unit vector */
						vh.scn.domain[8] > floats[3]+.01 ||
						vh.scn.domain[8] < floats[3]-.01 ||
						vh.scn.domain[9] > floats[4]+.01 ||
						vh.scn.domain[9] < floats[4]-.01 ||
						vh.scn.domain[10] > floats[5]+.01 ||
						vh.scn.domain[10] < floats[5]-.01)
				{  fprintf(stderr,"ERROR: Image Orientations do not match.\n");
					exit(-1);
				}
			}

			fclose(fpin);
		}

		/* Check if order is correct */
		flag = 0;
		for(j=skip+1; flag == 0 && j<nslices[i]; j++)
		{
			delta = acquisition[i][j]==acquisition[i][j-1]?
				location[i][j] - location[i][j-1]:
				acquisition[i][j] - acquisition[i][j-1];
	
			if(delta < 0)
			{
				printf("WARNING: images not in correct order (location) !!\n");
				flag = 1;
			}
		}


		/* If not correct order, reorder in correct order */
		if( flag == 1)
		{
			/* BUBLE SORT based on the locations */
			for(j=skip; j<nslices[i]; j++)
			{
				for(k=j+1; k<nslices[i]; k++)
				{
					if( acquisition[i][j]==acquisition[i][k]?
							location[i][j] > location[i][k]:
							acquisition[i][j] > acquisition[i][k] )
					{
						switch_location( &location[i][j], &location[i][k] );
						if (slope_flag)
							switch_location( &slope[i][j], &slope[i][k] );
						switch_index( &index[i][j], &index[i][k] );
						switch_acquisition( &acquisition[i][j],
							&acquisition[i][k] );
					}
				}
			}
		}

	}

	if (slope_flag && max_slope*slope_unit > 65535)
	{
		fprintf(stderr, "Overflow!  Reduce unit. max_slope=%f\n", max_slope);
		exit(1);
	}


	/* IMAGE LOCATIONS */
	if(nseries > 1)
	{
		/* 4D */
		vh.scn.loc_of_subscenes = (float *)calloc(nseries+totframes, sizeof(float));
		for(i=0; i<nseries; i++)
			vh.scn.loc_of_subscenes[i] = vol_time[i];

		k = nseries;
		for(i=0; i<nseries; i++)
			for(j=skip; j<nframes[i]; j++)
			{
				vh.scn.loc_of_subscenes[k] =
					location[i][j*nslices[i]/nframes[i]];
				k++;
			}
	}
	else
	{
		/* 3D */
		vh.scn.loc_of_subscenes = (float *) calloc( totframes, sizeof(float));
		if (vh.scn.loc_of_subscenes_valid)
			for(i=skip; i<nframes[0]; i++)
			{
				vh.scn.loc_of_subscenes[i-skip] =
					location[0][i*nslices[0]/nframes[0]];
				if (i < OffsetVectorSize)
					vh.scn.loc_of_subscenes[i-skip]+= GridFrameOffsetVector[i];
			}
		else
		{
			for(i=0; i<totframes; i++)
				vh.scn.loc_of_subscenes[i] = i*slice_spacing;
			for(i=0; i<OffsetVectorSize; i++)
				vh.scn.loc_of_subscenes[i] += GridFrameOffsetVector[i];
		}
	}
	vh.scn.loc_of_subscenes_valid = 1;

	/*******************************************************************************/
	/*******************************************************************************/

#ifdef VERBOSE

	printf("study            : %s\n", vh.gen.study);
	printf("series           : %s\n", vh.gen.series);
	printf("dimensions : %d\n", vh.scn.dimension);
	printf("num_of_density_values  : %d\n", vh.scn.num_of_density_values);
	for(i=0; i<vh.scn.num_of_density_values; i++)
	{
   		printf(" density #%d :\n", i);
   		printf("    range      : ( %f - %f )\n",
        	vh.scn.smallest_density_value[i], vh.scn.largest_density_value[i]);
	 
   		printf("    signed_bit : %d\n", vh.scn.signed_bits[i]);
   		printf("    bit_field  : (%d,%d)\n", vh.scn.bit_fields[i*2], vh.scn.bit_fields[i*2+1] );
	 
	}
	printf("num_of_integers        : %d\n", vh.scn.num_of_integers);
	printf("dimension_in_alignment : %d\n", vh.scn.dimension_in_alignment);
	printf("bytes_in_alignment     : %d\n", vh.scn.bytes_in_alignment);
	printf("width                  : %d\n", vh.scn.xysize[0]);
	printf("height                 : %d\n", vh.scn.xysize[1]);
	printf("pixel size             : %f\n", vh.scn.xypixsz[0]);
 
	i = vh.scn.dimension - 1;
	k = 0;
	printf("number of %dD scenes    : %d\n", i, vh.scn.num_of_subscenes[k]);
	j = vh.scn.num_of_subscenes[k];
	k++;
	while(i > 2)
	{
   		for(l=0; l<j; l++)
   		{
    		printf("    %dD scene #%d:\n", i, l);
    		printf("         location      : %f\n", vh.scn.loc_of_subscenes[k-1]);
    		printf("         #of %dD scenes : %d\n", i-1, vh.scn.num_of_subscenes[k]);
    		k++;
   		}
   		i--;
	}

#endif

	/*******************************************************************************/
	/* WRITE OUTPUT FILE */
	/*******************************************************************************/

	temp_output_file = (char *)malloc(strlen(output_file)+2);
	strcpy(temp_output_file, output_file);
	strcpy(temp_output_file+strlen(temp_output_file)-4, "_.IM0");

   }
   else
	temp_output_file = output_file;

	/* Write 3DViewnix Header */
   if (!rename_dicom && !move_dicom_subset)
   {
	if( (fpout = fopen(temp_output_file, "wb+")) == NULL)
	{
		fprintf(stderr, "ERROR: Can't open output file !\n");
		exit(3);
	}
	if( (i=VWriteHeader(fpout, &vh, group, element)) && i < 106)
	{
		fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", i,group, element);
        exit(i);
	}
	fflush(fpout);
	i = VSeekData(fpout, 0);
	if (i)
	{
	    fprintf(stderr, "VSeekData failure code %d\n", i);
		exit(i);
	}
   }


	/* WRITE IMAGES */
	for(i=0; i<nseries; i++)
	{
		for(j=skip; j<nslices[i]; j++)
		{

#ifdef VERBOSE
			printf("Converting image %d/%d series %d/%d\n", j+1,
				nslices[i], i+1, nseries);
			fflush(stdout);
#endif

			cur_filename = unrecognized_filename[i][vh.gen.data_type==IMAGE0?
				index[i][j]: j];
			int fframes=get_num_frames(cur_filename);
			if( (fpin=fopen(cur_filename, "rb")) == NULL)
			{
				fprintf(stderr, "ERROR: Can't open [%s] !\n", cur_filename);
				fflush(stderr);
				exit(3);
			}

			if (move_dicom_subset)
			{
				if (vh.scn.loc_of_subscenes[j]+vh.scn.domain[2]>bounds[0] &&
						vh.scn.loc_of_subscenes[j]+vh.scn.domain[2]<bounds[1])
				{
					if (rename(cur_filename,
							(const char *)wxFileName(output_file,
							wxFileName(cur_filename).GetFullName()).
					    	GetFullPath().c_str()))
					{
						fprintf(stderr, "ERROR: Can't move [%s] !\n",
							cur_filename);
						exit(3);
					}
				}
			}
			else if (rename_dicom)
			{
				sprintf(scratch, "%s_%04d_%04d.dcm", output_file, i+1, j+1);
				if (rename(cur_filename, scratch))
				{
					fprintf(stderr, "ERROR: Can't rename [%s] !\n",
						cur_filename);
					exit(3);
				}
			}
			else
            {
                if (slope_flag)
                {
					if (data == NULL)
						data = (unsigned char *) calloc(image_size*fframes, 1);
					if (data == NULL)
					{
						fprintf(stderr,
						    "ERROR: Can't allocate memory for images !\n");
						exit(1);
					}
					for (k=0; k<vh.scn.xysize[0]*vh.scn.xysize[1]; k++)
						((unsigned short *)data)[k] =
						    (unsigned short)rint(slope[i][j]*slope_unit);
					if (!bigendian)
						swap_bytes(data, data, image_size*fframes);
				}
                else if (DoseGridScaling > 0)
                {
                    k = get_element(fpin, 0x7fe0, 0x0010, BD, &bd, 0,
					    &items_read);
                    if (k != 235)
                        fprintf(stderr,
						    "ERROR: Can't find image data in file %s\n",
                            cur_filename);
                    if (data == NULL)
                        data = (unsigned char *)calloc(
							vh.scn.xysize[0]*vh.scn.xysize[1]*fframes, 4);
                    if (data == NULL)
                    {
                        fprintf(stderr,
						    "ERROR: Can't allocate memory for images !\n");
                        exit(3);
                    }

                    /* READ IMAGE */
                    if (fread(data,vh.scn.xysize[0]*vh.scn.xysize[1]*4*fframes,
						    1, fpin) != 1)
                    {
                        fprintf(stderr,
						   "ERROR: Can't read image#%d/series#%d !\n", j+1, i);
                        exit(3);
                    }

                    /****** SWAP IMAGE BYTES (if applicable) ******/
                    switch(rec_arch_type)
                    {
                        case TYPE2:
                        case TYPE3:
                        case TYPE6:
                        case TYPE7:
							if (bigendian)
							    swap_4_bytes(data, data, image_size*fframes);
							break;
						default:
                            if (!bigendian)
								swap_4_bytes(data, data, image_size*fframes);
							break;
                    }
					int *i_data=(int *)data;
					float gy;
                    unsigned short *p=(unsigned short *)data;
                    for (int i=0; i<vh.scn.xysize[0]*vh.scn.xysize[1]*fframes;
							i++)
                    {
					    gy = i_data[i]*(DoseGridScaling/DoseGridRescaling);
						if (gy < 0)
							gy = 0;
						if (gy > 65535)
							gy = 65535;
						p[i] = (unsigned short)rint(gy);
                    }

					if (!bigendian)
						swap_bytes(data, data, image_size*fframes);
                }
                else
                {
                    k = get_element(fpin, 0x7fe0, 0x0010, BD, &bd, 0,
					    &items_read);
                    if (k != 235)
                        fprintf(stderr,
						    "ERROR: Can't find image data in file %s\n",
                            cur_filename);
                    if (data == NULL)
                        data = (unsigned char *) calloc(image_size*fframes, 1);
                    if (data == NULL)
                    {
                        fprintf(stderr,
						    "ERROR: Can't allocate memory for images !\n");
                        exit(3);
                    }
 

                    /* READ IMAGE */
                    if( fread(data, image_size*fframes, 1, fpin) != 1)
                    {
                        fprintf(stderr,
						   "ERROR: Can't read image#%d/series#%d !\n", j+1, i);
                        exit(3);
                    }

                    /****** SWAP IMAGE BYTES (if applicable) ******/
                    if (vh.gen.data_type==IMAGE0?
                            vh.scn.num_of_bits%16==0: vh.dsp.num_of_bits%16==0)
                    {
                        switch(rec_arch_type)
                        {
                            case TYPE2:
                            case TYPE3:
                            case TYPE6:
                            case TYPE7:
								if (bigendian)
								    swap_bytes(data, data, image_size*fframes);
								break;
							default:
                                if (!bigendian)
									swap_bytes(data, data, image_size*fframes);
								break;
                        }
                        if (add_value)
                        {
                            short *p=(short *)data;
                            for (int i=0; i<vh.scn.xysize[0]*vh.scn.xysize[1]*
						    		fframes; i++)
                                p[i] = p[i]> -add_value? p[i]+add_value: 0;
                        }
						if (!bigendian)
							swap_bytes(data, data, image_size*fframes);
                    }
                }

				/* WRITE IMAGE */
				if( fwrite(data, image_size*fframes, 1, fpout) != 1)
				{
					fprintf(stderr, "ERROR: Can't write image#%d/series#%d onto output file !\n", j+1, i);
					exit(3);
				}
			}

			fclose(fpin);

		}
	}
	if (rename_dicom || move_dicom_subset)
		exit(0);
	VCloseData(fpout);

	if (vh.gen.data_type == IMAGE0)
	{
		sprintf(scratch, "correct_min_max \"%s\" \"%s\"",
			 temp_output_file, output_file);
		if (system(scratch))
		{
			fprintf(stderr,
			 "The function system() returns implementation-defined value.\n");
		}
		if (vh.gen.data_type==IMAGE0 && vh.scn.num_of_bits<=16)
			unlink(temp_output_file);
		free(temp_output_file);
	}

	exit(0);
}
