/*
  Copyright 1993-2011, 2016 Medical Image Processing Group
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
#include <ctype.h>

#if ! defined (WIN32) && ! defined (_WIN32)
#define PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
#endif

#define switch_acquisition(i1, i2) switch_index((short *)(i1), (short *)(i2))

static int skip; /* number of images to skip */


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

int main(int argc, char *argv[])
{
	float location;

	int i;
	FILE *fpin;
	int items_read;

	/* Elements */
	char at[500];
	char an[500];
	float floats[20];
	int slice_location_valid;
	char *suid;
	char new_filename[500];



	if (argc < 2)
	{
		fprintf(stderr, "Usage: ser_loc_tim <input_file>\n");
		exit(1);
	}

	skip = 0;

	suid = get_series_uid(NULL, argv[1]);

	if( (fpin=fopen(argv[1], "rb")) == NULL)
	{
		fprintf(stderr, "ERROR: Can't open [%s] !\n", argv[1]);
		exit(3);
	}
	i = get_element(fpin, 0x0020, 0x0011, AT,
		at, sizeof(at), &items_read) == 0;
	if (i)
	{
		if (!isalnum(at[strlen(at)-1]))
			at[strlen(at)-1] = 0;
		strcpy(new_filename, at);
	}
	else if (suid)
		strcpy(new_filename, strrchr(suid, '.'));
	else
	{
		if (get_element(fpin, 0x0008, 0x0032, AT, at, sizeof(at), &items_read))
			get_element(fpin, 0x0008, 0x0031, AT, at, sizeof(at), &items_read);
		sprintf(new_filename, "%f", hms_to_s(at));
	}
	strcat(new_filename, "_");
	slice_location_valid =
		get_element(fpin, 0x0020, 0x0050, AN, an, sizeof(an), &items_read)
		== 0 ||
		get_element(fpin, 0x0020, 0x1041, AN, an, sizeof(an), &items_read)== 0;
	if (slice_location_valid)
		extract_floats(an, 1, &location);
    else if (get_element(fpin, 0x0020, 0x0030, AN,
			an, sizeof(an), &items_read) == 0)
	{
		extract_floats(an, 3, floats);
		location = floats[2];
	}
	else
		location = 0.;
	sprintf(new_filename+strlen(new_filename), "%f", location);
	printf("%s\n", new_filename);

	exit(0);
}
