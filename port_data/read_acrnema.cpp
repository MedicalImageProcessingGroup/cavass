/*
  Copyright 1993-2014, 2016-2020 Medical Image Processing Group
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
 








/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/


#include  <Viewnix.h>
#include  "cv3dv.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include  "port_data/from_dicom.h"



bool sequence_tag(unsigned tag);

/*-------------------------------------------------------------------------------------*/
/*
 *    Modified: 4/24/95 VReadData called by Dewey Odhner
 *    Modified: 8/25/95 byte swapping cases corrected by Dewey Odhner
 */
int read_BI(FILE *fp, unsigned short *bi, int rec_arch_type)
{
	int items_read;
	union {unsigned short s; char c[2];} t;
	char c;

	if( VReadData((char *)bi, sizeof(unsigned short), 1, fp, &items_read) != 0)
		return(-1);
	switch (rec_arch_type)
	{
		case TYPE2:
		case TYPE3:
		case TYPE6:
		case TYPE7:
			t.s = *bi;
			c = t.c[0];
			t.c[0] = t.c[1];
			t.c[1] = c;
			*bi = t.s;
	}
	return(1);
}

/*-------------------------------------------------------------------------------------*/
/*
 *    Modified: 4/24/95 VReadData called by Dewey Odhner
 */
int read_BD(FILE *fp, unsigned int *bd, int rec_arch_type)
{
	int items_read;
	union {int i; char c[4];} t, u;

	if( VReadData((char *)bd, sizeof(int), 1, fp, &items_read) != 0)
		return(-1);
	switch (rec_arch_type)
	{
		case TYPE1:
		case TYPE5:
			break;
		case TYPE2:
		case TYPE6:
			t.i = *bd;
			u.c[0] = t.c[1];
			u.c[1] = t.c[0];
			u.c[2] = t.c[3];
			u.c[3] = t.c[2];
			*bd = u.i;
			break;
		case TYPE3:
		case TYPE7:
			t.i = *bd;
			u.c[0] = t.c[3];
			u.c[1] = t.c[2];
			u.c[2] = t.c[1];
			u.c[3] = t.c[0];
			*bd = u.i;
			break;
		case TYPE4:
		case TYPE8:
			t.i = *bd;
			u.c[0] = t.c[2];
			u.c[1] = t.c[3];
			u.c[2] = t.c[0];
			u.c[3] = t.c[1];
			*bd = u.i;
			break;
	}
	return(1);
}


/*-------------------------------------------------------------------------------------*/
int read_AN(FILE *fp, char *an, int n, int rec_arch_type)
{
	char temp;
	int j;

	if( fread(an, n, 1, fp) == 0)
	{	an[0] = 0;
		return(-1);
	}
	if (rec_arch_type >= TYPE5)
		for (j=0; j<n; j+=2)
		{	temp = an[j];
			an[j] = an[j+1];
			an[j+1] = temp;
		}
	an[n] = 0;
	return(1);
}



/*-------------------------------------------------------------------------------------*/
int extract_floats(char *text, int n, float floats[])
{
	int i, j;
	int begin, end;
	char cfloat[20];

	strcpy(cfloat, "");

	begin = end = 0;
	for(i=0; i<n; i++)
	{
		while(text[end] != '\\' &&  text[end] != '\0')
			end++;

		j = end - begin;
		strncpy(cfloat, &text[begin], j);
		cfloat[j] = 0;
		sscanf(cfloat, "%f", &(floats[i]));
		begin = end+1;
		end = begin;
	}
	return 0;
}

/*-------------------------------------------------------------------------------------*/
/*
 * RETURN VALUE:
 *    -1: fp is NULL.
 *    0: success
 *    2: read error
 *    5: seek error
 *    100: group or element not found.
 *    235: element length greater than maxlen.
 * HISTORY:
 *    Modified: 4/26/95 element length limited by Dewey Odhner
 *    Modified: 9/5/96 to work without correct group length by George Grevera
 *    Modified: 12/9/98 to work without group length by Dewey Odhner
 *    Modified: 12/11/98 to work with explicit VR encoding by Dewey Odhner
 *    Modified: 12/15/98 to use correct transfer syntax by Dewey Odhner
 *    Modified: 2/13/03 undefined element length/SQ items skipped
 *       by Dewey Odhner.
 *    Modified: 6/16/03 corrected in case transfer syntax UID not given
 *       (no file meta information group) by Dewey Odhner.
 *    Modified: 11/29/05 *items_read initialized to 0 by Dewey Odhner.
 *    Modified: 1/26/10 corrected in case transfer syntax 1.2.840.10008.1.2.2
 *       by Dewey Odhner.
 */
int get_element(FILE *fp, unsigned short group, unsigned short element,
	int type /* type of element being read (BI=16bits, BD=32bits,
	AN=ASCIInumeric, AT=ASCIItext)*/, void *result,
	unsigned int maxlen /* bytes at result */, int *items_read)
{
	int return_val=get_element(fp, group, element, type, result, maxlen,
		items_read, true, 2);
	if (return_val && return_val!=235)
	{
		int return_val2 = get_element(fp, group, element, type, result, maxlen,
			items_read, false, 2);
		if (return_val2==0 || return_val2>return_val)
			return_val = return_val2;
	}
	return return_val;
}


int get_contour_struct_set(FILE *fp, contour_struct_set *ss)
{
	int items_read;
	unsigned short g, e, slen;
	unsigned int len, tag, cbuf_len=0;
	int explicit_vr=0, rec_arch_type=2;
	char *cp, vr[4]={0,0,0,0}, uid[65], *cbuf;

	ss->num_rois = 0;
	if (fseek(fp, 128, 0)==0 && fread(vr, 1, 4, fp)==4 &&
			strncmp(vr, "DICM", 4)==0)
	{
		explicit_vr = 1;
		if (get_element(fp, 2, 0x10, AT, uid, 64, &items_read, false, 2))
		/* transfer syntax UID not given */
		{
			explicit_vr = 0;
			fseek(fp, 132, 0);
		}
		else
		{
			if (strcmp(uid, "1.2.840.10008.1.2 ")==0 ||
					strcmp(uid, "1.2.840.10008.1.2")==0)
				explicit_vr = 0;
			else if (strcmp(uid, "1.2.840.10008.1.2.2 ")==0 ||
					strcmp(uid, "1.2.840.10008.1.2.2")==0)
				rec_arch_type = 0;
		}
	}

	// find Structure Set ROI Sequence
	int return_val=get_element(fp, 0x3006, 0x20, BI, 0, 0, &items_read,true,0);
	if (return_val != 235)
		return return_val;
	int sequence_depth=1;
	long sequence_end[100], item_end[100];
	sequence_end[0] = item_end[0] = item_end[1] = 0;
	if (fseek(fp, -4, 1))
		return (5);
	if (read_BD(fp, &len, rec_arch_type) < 0)
		return (2);
	sequence_end[1] = len==0xffffffff? 0:ftell(fp)+len;
	int num_contours=0;
	for (;;)
	{
		/* Check Group */
		if (read_BI(fp, &g, rec_arch_type) < 0)
			return (2);
		/* Check Element */
		if (read_BI(fp, &e, rec_arch_type) < 0)
			return (2);
		if ((g==0xfffe && e==0xe0dd) || (sequence_end[sequence_depth] &&
				ftell(fp)>=sequence_end[sequence_depth])) // end of sequence
		{
			if (g==0xfffe && e==0xe0dd)
			{
				if (read_BD(fp, &len, rec_arch_type) < 0)
					return (2);
				if (len)
					return (100);
			}
			if (sequence_depth == 1)
				break;
			sequence_depth--;
			continue;
		}
		if (g!=0xfffe || e!=0xe000) // beginning of item
			return (100);
		/* Check value representation */
		if (fread(vr, 1, 2, fp) != 2)
			return (2);
		tag = g;
		tag = (tag<<16)|e;
		if (g==0xfffe ||
				(!explicit_vr && (!sequence_tag(tag)||strncmp(vr, "SQ", 2))))
		// Explicit VR not found. (Sometimes sequence VR is given explicitly
		// even when transfer syntax with implicit VR is specified.)
		{
			if (fseek(fp, -2, 1))
				return (5);
			vr[0] = 0;
		}
		if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OD", 2)==0 ||
				strncmp(vr, "OF", 2)==0 || strncmp(vr, "OL", 2)==0 ||
				strncmp(vr, "OW", 2)==0 || strncmp(vr, "SQ", 2)==0 ||
				strncmp(vr, "UN", 2)==0 || strncmp(vr, "UT", 2)==0 ||
				(explicit_vr&&e==0))
		{
			if (fseek(fp, 2, 1))
				return (5);
		}
		/* Check length */
		if (e==0 && vr[0])
			len = 4;
		else if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OW", 2)==0 ||
				strncmp(vr, "SQ", 2)==0 || strncmp(vr, "UN", 2)==0 ||
				strncmp(vr, "UT", 2)==0 || vr[0]==0)
		{
			if (read_BD(fp, &len, rec_arch_type) < 0)
				return (2);
		}
		else
		{
			if (read_BI(fp, &slen, rec_arch_type) < 0)
				return (2);
			len = slen;
		}
		item_end[sequence_depth] = len==0xffffffff? 0:ftell(fp)+len;
		while ((g!=0xfffe || e!=0xe00d) && (item_end[sequence_depth]==0 ||
				ftell(fp)<item_end[sequence_depth])) // end of item
		{
			/* Check Group */
			if (read_BI(fp, &g, rec_arch_type) < 0)
			{
				if (feof(fp))
					goto find_contour_sequence;
				return (2);
			}
			/* Check Element */
			if (read_BI(fp, &e, rec_arch_type) < 0)
				return (2);
			/* Check value representation */
			if (fread(vr, 1, 2, fp) != 2)
				return (2);
			tag = g;
			tag = (tag<<16)|e;
			if (g==0xfffe || (!explicit_vr &&
					(!sequence_tag(tag)||strncmp(vr, "SQ", 2))))
			// Explicit VR not found. (Sometimes SQ VR is given explicitly
			// even when transfer syntax with implicit VR is specified.)
			{
				if (fseek(fp, -2, 1))
					return (5);
				vr[0] = 0;
			}
			if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OD", 2)==0 ||
					strncmp(vr, "OF", 2)==0 || strncmp(vr, "OL", 2)==0 ||
					strncmp(vr, "OW", 2)==0 || strncmp(vr, "SQ", 2)==0 ||
					strncmp(vr, "UN", 2)==0 || strncmp(vr, "UT", 2)==0 ||
					(explicit_vr&&e==0))
			{
				if (fseek(fp, 2, 1))
					return (5);
			}
			/* Check length */
			if (e==0 && vr[0])
				len = 4;
			else if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OW", 2)==0 ||
					strncmp(vr, "SQ", 2)==0 || strncmp(vr, "UN", 2)==0 ||
					strncmp(vr, "UT", 2)==0 || vr[0]==0)
			{
				if (read_BD(fp, &len, rec_arch_type) < 0)
					return (2);
			}
			else
			{
				if (read_BI(fp, &slen, rec_arch_type) < 0)
					return (2);
				len = slen;
			}
			if ((g==0xfffe && e==0xe0dd) || (sequence_end[sequence_depth] &&
				   ftell(fp)>=sequence_end[sequence_depth])) // end of sequence
			{
				if (sequence_depth == 1)
					break;
				sequence_depth--;
				continue;
			}
			if (sequence_tag(tag))
			{
				if (sequence_depth >= 99)
				{
					fprintf(stderr, "Too many nested sequences.\n");
					return (2);
				}
				sequence_depth++;
				if (len == 0xffffffff)
					sequence_end[sequence_depth] = 0;
				else
					sequence_end[sequence_depth] = ftell(fp)+len;
				item_end[sequence_depth] = 0;
				continue;
			}
			if (g==0x3006 && e==0x22) // ROI number
			{
				if (ss->num_rois == 0)
					ss->roi = (contour_roi *)malloc(sizeof(contour_roi));
				else
					ss->roi = (contour_roi *)realloc(ss->roi,
						(ss->num_rois+1)*sizeof(contour_roi));
				ss->roi[ss->num_rois].roi_name = NULL;
				ss->roi[ss->num_rois].num_contours = 0;
				if (cbuf_len <= len)
				{
					if (cbuf_len)
						free(cbuf);
					cbuf = (char *)malloc(len+1);
					cbuf_len = len+1;
				}
				if (read_AN(fp, cbuf, len, rec_arch_type) < 0)
					return (2);
				ss->roi[ss->num_rois].roi_number = atoi(cbuf);
				ss->num_rois++;
			}
			else if (g==0x3006 && e==0x26) // ROI name
			{
				if (ss->num_rois <= 0)
					return (100);
				if (cbuf_len < len+1)
				{
					if (cbuf_len)
						free(cbuf);
					cbuf = (char *)malloc(len+1);
					cbuf_len = len+1;
				}
				if (read_AN(fp, cbuf, len, rec_arch_type) < 0)
					return (2);
				if (cbuf[len-1] == ' ')
					cbuf[len-1] = 0;
				for (cp=cbuf+1; *cp; cp++)
					if (*cp==' ' || *cp=='/' || *cp=='\\')
						*cp = '_';
				cp = cbuf;
				if (*cp == ' ')
					cp++;
				ss->roi[ss->num_rois-1].roi_name= (char *)malloc(strlen(cp)+1);
				strcpy(ss->roi[ss->num_rois-1].roi_name, cp);
			}
			else
				if (fseek(fp, len, 1))
					return (5);
		}
	}
	find_contour_sequence: ;

	// find ROI Contour Sequence
	return_val = get_element(fp, 0x3006, 0x39, BI, 0, 0, &items_read, true, 0);
	if (return_val != 235)
		return return_val;
	sequence_depth = 1;
	if (fseek(fp, -4, 1))
		return (5);
	if (read_BD(fp, &len, rec_arch_type) < 0)
		return (2);
	sequence_end[1] = len==0xffffffff? 0:ftell(fp)+len;
	int roi_number=0;
	unsigned int *contour_num_points;
	float3p *contour_coord;
	item_end[sequence_depth] = 0;
	for (;;)
	{
		/* Check Group */
		if (read_BI(fp, &g, rec_arch_type) < 0)
		{
			if (feof(fp))
				break;
			return (2);
		}
		/* Check Element */
		if (read_BI(fp, &e, rec_arch_type) < 0)
			return (2);
		/* Check value representation */
		if (fread(vr, 1, 2, fp) != 2)
			return (2);
		tag = g;
		tag = (tag<<16)|e;
		if (g==0xfffe ||
				(!explicit_vr && (!sequence_tag(tag)||strncmp(vr, "SQ", 2))))
		// Explicit VR not found. (Sometimes sequence VR is given explicitly
		// even when transfer syntax with implicit VR is specified.)
		{
			if (fseek(fp, -2, 1))
				return (5);
			vr[0] = 0;
		}
		if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OD", 2)==0 ||
				strncmp(vr, "OF", 2)==0 || strncmp(vr, "OL", 2)==0 ||
				strncmp(vr, "OW", 2)==0 || strncmp(vr, "SQ", 2)==0 ||
				strncmp(vr, "UN", 2)==0 || strncmp(vr, "UT", 2)==0 ||
				(explicit_vr&&e==0))
		{
			if (fseek(fp, 2, 1))
				return (5);
		}
		/* Check length */
		if (e==0 && vr[0])
			len = 4;
		else if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OW", 2)==0 ||
				strncmp(vr, "SQ", 2)==0 || strncmp(vr, "UN", 2)==0 ||
				strncmp(vr, "UT", 2)==0 || vr[0]==0)
		{
			if (read_BD(fp, &len, rec_arch_type) < 0)
				return (2);
		}
		else
		{
			if (read_BI(fp, &slen, rec_arch_type) < 0)
				return (2);
			len = slen;
		}
		if ((g==0xfffe && e==0xe0dd) || (sequence_end[sequence_depth] &&
				ftell(fp)>=sequence_end[sequence_depth])) // end of sequence
		{
			if (sequence_depth == 1)
				break;
			sequence_depth--;
			if (g==0xfffe && e==0xe0dd)
				continue;
		}
		if (sequence_tag(tag))
		{
			if (sequence_depth >= 99)
			{
				fprintf(stderr, "Too many nested sequences.\n");
				return (2);
			}
			sequence_depth++;
			if (len == 0xffffffff)
				sequence_end[sequence_depth] = 0;
			else
				sequence_end[sequence_depth] = ftell(fp)+len;
			item_end[sequence_depth] = 0;
			continue;
		}
		if (g==0xfffe && e==0xe000) // beginning of item
		{
			item_end[sequence_depth] = len==0xffffffff? 0:ftell(fp)+len;
			continue;
		}
		if (g==0x3006 && e==0x46) // Number of Contour Points
		{
			if (num_contours == 0)
			{
				contour_num_points = (unsigned int *)malloc(sizeof(int));
				contour_coord = (float3p *)malloc(sizeof(float (*)[3]));
			}
			else
			{
				contour_num_points= (unsigned int *)realloc(contour_num_points,
					(num_contours+1)*sizeof(int));
				contour_coord = (float3p *)realloc(contour_coord,
					(num_contours+1)*sizeof(float (*)[3]));
			}
			if (cbuf_len <= len)
			{
				if (cbuf_len)
					free(cbuf);
				cbuf = (char *)malloc(len+1);
				cbuf_len = len+1;
			}
			if (read_AN(fp, cbuf, len, rec_arch_type) < 0)
				return (2);
			contour_num_points[num_contours] = atoi(cbuf);
			contour_coord[num_contours] = (float (*)[3])
				malloc(contour_num_points[num_contours]*sizeof(float [3]));
			if (contour_coord[num_contours] == NULL)
				return (1);
			num_contours++;
			continue;
		}
		if (g==0x3006 && e==0x50) // Contour Data
		{
			items_read = len+1>27*contour_num_points[num_contours-1]? len+1:
				27*contour_num_points[num_contours-1];
			if ((int)cbuf_len < items_read)
			{
				free(cbuf);
				cbuf = (char *)malloc(items_read);
				cbuf_len = items_read;
				if (cbuf == NULL)
					return (1);
			}
			if (read_AN(fp, cbuf, len, rec_arch_type) < 0)
				return (2);
			extract_floats(cbuf, 3*contour_num_points[num_contours-1],
				contour_coord[num_contours-1][0]);
			continue;
		}
		if (g==0x3006 && e==0x84) // Referenced ROI Number
		{
			if (read_AN(fp, cbuf, len, rec_arch_type) < 0)
				return (2);
			roi_number = atoi(cbuf);
			if (num_contours && roi_number)
				for (int j=0; j<ss->num_rois; j++)
					if (roi_number == ss->roi[j].roi_number)
					{
						ss->roi[j].num_contours = num_contours;
						ss->roi[j].contour_num_points = contour_num_points;
						ss->roi[j].contour_coord = contour_coord;
						break;
					}
			roi_number = 0;
			num_contours = 0;
			continue;
		}
		/* Skip to next Element */
		if (len!=0xffffffff && fseek(fp, len, 1))
			return (5);
	}
	if (cbuf_len)
		free(cbuf);
	return 0;
}

int get_element(FILE *fp, unsigned short group, unsigned short element,
	int type /* type of element being read (BI=16bits, BD=32bits,
	AN=ASCIInumeric, AT=ASCIItext)*/, void *result,
	unsigned int maxlen /* bytes at result */, int *items_read,
	bool skip_sequences, int rec_arch_type)
{
	unsigned short g, e, slen;
	unsigned int len, tag;
	int items, explicit_vr=0, orig_arch;
	char *cp, vr[3]={0,0,0}, uid[65];
	int sequence_depth=0;

	*items_read = 0;
	if(fp == NULL) return (-1);
	orig_arch = rec_arch_type;
	if (fseek(fp, 128, 0)==0 && fread(uid, 1, 4, fp)==4 &&
			strncmp(uid, "DICM", 4)==0)
	{
		explicit_vr = 1;
		rec_arch_type = 2;
		if (group>2 &&
				get_element(fp, 2, 0x10, AT, uid, 64, &items,skip_sequences,2))
		/* transfer syntax UID not given */
		{
			fseek(fp, 132, 0);
			/* Check Group */
			if (read_BI(fp, &g, rec_arch_type) < 0)
				return (2);
			/* Check Element */
			if (read_BI(fp, &e, rec_arch_type) < 0)
				return (2);
			if (g==2 && e==0)
			{
				if (fread(vr, 1, 2, fp) != 2)
					return (2);
				if (strncmp(vr, "UL", 2))
					if (fseek(fp, -2, 1))
						return (5);
				if (read_BD(fp, &len, rec_arch_type) < 0)
					return (2);
				if (fseek(fp, len, 1))
					return (5);
			}
			else
				if (fseek(fp, -4, 1))
					return (5);
			explicit_vr = 0;
			rec_arch_type = orig_arch;
		}
	}
	else
		fseek(fp, 0, 0);

	/* Find Group and Element */
	int ii;
	long sequence_end[100];
	sequence_end[0] = 0;
	for (ii=0; ii>=0; ii++)
	{
		/* Check Group */
		if (read_BI(fp, &g, rec_arch_type) < 0)
			return (2);
		/* Check transfer syntax */
		if (explicit_vr && g>2)
		{
			if (strcmp(uid, "1.2.840.10008.1.2 ")==0 ||
					strcmp(uid, "1.2.840.10008.1.2")==0)
				explicit_vr = 0;
			else if (strcmp(uid, "1.2.840.10008.1.2.2 ")==0 ||
					strcmp(uid, "1.2.840.10008.1.2.2")==0)
			{
				rec_arch_type = orig_arch = 0;
				if (fseek(fp, -2, 1))
					return (5);
				if (read_BI(fp, &g, rec_arch_type) < 0)
					return (2);
			}
		}
		/* Check Element */
		if (read_BI(fp, &e, rec_arch_type) < 0)
			return (2);
		/* Check value representation */
		if (fread(vr, 1, 2, fp) != 2)
			return (2);
		tag = g;
		tag = (tag<<16)|e;
		if (g==0xfffe ||
				(!explicit_vr && (!sequence_tag(tag)||strncmp(vr, "SQ", 2))))
		// Explicit VR not found. (Sometimes sequence VR is given explicitly
		// even when transfer syntax with implicit VR is specified.)
		{
			if (fseek(fp, -2, 1))
				return (5);
			vr[0] = 0;
		}
		if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OD", 2)==0 ||
				strncmp(vr, "OF", 2)==0 || strncmp(vr, "OL", 2)==0 ||
				strncmp(vr, "OW", 2)==0 || strncmp(vr, "SQ", 2)==0 ||
				strncmp(vr, "UN", 2)==0 || strncmp(vr, "UT", 2)==0 ||
				(explicit_vr&&e==0))
		{
			if (fseek(fp, 2, 1))
				return (5);
		}
		/* Check length */
		if (e==0 && vr[0])
			len = 4;
		else if (strncmp(vr, "OB", 2)==0 || strncmp(vr, "OW", 2)==0 ||
				strncmp(vr, "SQ", 2)==0 || strncmp(vr, "UN", 2)==0 ||
				strncmp(vr, "UT", 2)==0 || vr[0]==0)
		{
			if (read_BD(fp, &len, rec_arch_type) < 0)
				return (2);
		}
		else
		{
			if (read_BI(fp, &slen, rec_arch_type) < 0)
				return (2);
			len = slen;
		}
		if (sequence_depth==0 && skip_sequences && g>group)
			return (100);
		if (g==group && e==element)
			break;
		if ((g==0xfffe && e==0xe0dd) || (sequence_end[sequence_depth] &&
				ftell(fp)>=sequence_end[sequence_depth])) // end of sequence
			sequence_depth--;
		else if (sequence_tag(tag) || ((g&1) && len==-1))
		{
			if (sequence_depth >= 99)
			{
				fprintf(stderr, "Too many nested sequences.\n");
				return (2);
			}
			sequence_depth++;
			if (len == 0xffffffff)
				sequence_end[sequence_depth] = 0;
			else
			{
				sequence_end[sequence_depth] = ftell(fp)+len;
				if (skip_sequences)
					sequence_depth--;
			}
		}
		if (len && len!=0xffffffff && fseek(fp, len, 1))
			return (5);
	}

	/* Read Element */
	switch (type)
	{
		case BI:
			if (len == 0xffffffff)
				return (235);
			items = (len>maxlen? maxlen:len)/sizeof(short);
			for (*items_read=0; *items_read<items; (*items_read)++)
				if (read_BI(fp, (unsigned short *)result+ *items_read,
						rec_arch_type) < 0)
					return (2);
			if (len > maxlen)
				return (235);
			break;
		case BD:
			items = (len>maxlen? maxlen:len)/sizeof(int);
			for (*items_read=0; *items_read<items; (*items_read)++)
				if (read_BD(fp, (unsigned int *)result+ *items_read,
						rec_arch_type) < 0)
					return (2);
			if (len > maxlen)
				return (235);
			break;
		case AN:
		case AT:
			*items_read = 0;
			if (read_AN(fp, (char *)result, len<maxlen? len: maxlen-1,
					rec_arch_type) < 0)
				return (2);
			for (cp=(char *)result, items=FALSE; ; cp++)
				if (*cp == '\\' || *cp == 0)
				{	if (items)
						(*items_read)++;
					if (*cp == 0)
						break;
					items = FALSE;
				}
				else if (*cp != ' ')
					items = TRUE;
			if (len >= maxlen)
				return (235);
			break;
	}

	return (0);
}


/*****************************************************************************
 * FUNCTION: check_acrnema_file
 * DESCRIPTION: Checks an ACR-NEMA file for image size.
 * PARAMETERS:
 *    path: The directory where the file is
 *    file: The file name
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable rec_arch_type must be properly set.
 * RETURN VALUE:
 *    0: success
 *    4: Cannot open file
 *    104: Cannot get image size
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/16/99 by Dewey Odhner
 *    Modified: 10/18/07 null path allowed by Dewey Odhner.
 *
 *****************************************************************************/
int check_acrnema_file(const char *path, const char *file)
{
    FILE *fp;
    char filename[500];
	int items_read, err;
	unsigned short bi;
 
    if (path)
	{
		strcpy(filename, path);
	    strcat(filename, "/");
	    strcat(filename, file);
	}
	else
		strcpy(filename, file);

    if( (fp = fopen(filename, "rb")) == NULL)
        return (4);
 
	/* IMAGE DIMENSIONS */
	err = get_element(fp, 0x0028, 0x0011, BI,
		&bi, sizeof(bi), &items_read) || bi <= 0 ||
		 get_element(fp, 0x0028, 0x0010, BI,
			&bi, sizeof(bi), &items_read) || bi <= 0;
	fclose(fp);
	return (err? 104: 0);
}

/*****************************************************************************
 * FUNCTION: names_are_similar
 * DESCRIPTION: Checks whether two names differ only by three
 *    consecutive digits or less.
 * PARAMETERS:
 *    name1, name2: the names to compare
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: TRUE or FALSE
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/1/98 by Dewey Odhner
 *    Modified: 1/18/99 difference not restricted to last three digits
 *       by Dewey Odhner
 *
 *****************************************************************************/
int names_are_similar(const char *name1, const char *name2)
{
	char *e1, *e2;
	char *namep1, *namep2;

	namep1 = (char *)name1;
	namep2 = (char *)name2;
	while (*namep1 && *namep2==*namep1)
		namep1++, namep2++;
	for (e1=namep1+strlen(namep1), e2=namep2+strlen(namep2);
			e1>namep1 && e2>namep2 && e1[-1]==e2[-1]; e1--, e2--)
		;
	if (e1-namep1>3 || e2-namep2>3)
		return (FALSE);
	for (; namep1<e1; namep1++)
		if (!isdigit(*namep1))
			return (FALSE);
	for (; namep2<e2; namep2++)
		if (!isdigit(*namep2))
			return (FALSE);
	return (TRUE);
}

/*****************************************************************************
 * FUNCTION: get_series_uid
 * DESCRIPTION: Returns the series instance UID from a DICOM file.
 * PARAMETERS:
 *    dir: the directory in which the file is
 *    filename: the file name
 * SIDE EFFECTS: An error message may be printed.
 * ENTRY CONDITIONS: rec_arch_type should be set.
 * RETURN VALUE: A string containing the series instance UID, if found, or
 *    NULL.  The memory should be freed by the caller after use.
 * EXIT CONDITIONS: Returns NULL on error without notice.
 * HISTORY:
 *    Created: 2/25/03 by Dewey Odhner
 *    Modified: 4/10/03 dir parameter added by Dewey Odhner.
 *    Modified: 5/19/03 buf_size updated by Dewey Odhner.
 *    Modified: 10/16/07 buf_size updated sooner by Dewey Odhner.
 *    Modified: 10/18/07 null dir allowed by Dewey Odhner.
 *
 *****************************************************************************/
char *get_series_uid(const char dir[], const char filename[])
{
	int items_read, valid, dir_len;
	char *suid;
	static int buf_size;
	static char *buf;
	FILE *fp;

	dir_len = dir? (int)strlen(dir): 0;
	if (buf_size == 0)
	{
		buf = (char *)malloc(dir_len+strlen(filename)+2);
		if (buf == NULL)
			return NULL;
		buf_size = dir_len+(int)strlen(filename)+2;
		buf[buf_size-1] = 0;
	}
	if ((unsigned)buf_size < dir_len+strlen(filename)+2)
	{
		suid = (char *)realloc(buf, dir_len+strlen(filename)+2);
		if (suid == NULL)
			return NULL;
		buf = suid;
		buf_size = dir_len+(int)strlen(filename)+2;
		buf[buf_size-1] = 0;
	}
	if (dir)
		sprintf(buf, "%s/%s", dir, filename);
	else
		strcpy(buf, filename);
	fp = fopen(buf, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", filename);
		return NULL;
	}
	valid = get_element(fp, 0x0020, 0x000e, AT, buf, buf_size-1, &items_read);
	while (valid && strlen(buf)+1>=(unsigned)buf_size-1)
	{
		suid = (char *)realloc(buf, buf_size+1000);
		if (suid == NULL)
		{
			fclose(fp);
			return NULL;
		}
		buf = suid;
		buf_size += 1000;
		buf[buf_size-1] = 0;
		valid =
			get_element(fp, 0x0020, 0x000e, AT, buf, buf_size-1, &items_read);
	}
	suid = (char *)malloc(strlen(buf)+1);
	if (suid)
		strcpy(suid, buf);
	fclose(fp);
	return (suid);
}

/*****************************************************************************
 * FUNCTION: get_time
 * DESCRIPTION: Returns the time stamp from a DICOM file.
 * PARAMETERS:
 *    filename: the file name
 * SIDE EFFECTS: An error message may be printed.
 * ENTRY CONDITIONS: rec_arch_type should be set.
 * RETURN VALUE: the time stamp
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/3/10 by Dewey Odhner
 *
 *****************************************************************************/
float get_time(const char filename[])
{
	int items_read;
	float ft;
	FILE *fp;
	char an[500];

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", filename);
		return 0.0;
	}
	if (get_element(fp, 0x0008, 0x0033, AN, an, sizeof(an), &items_read) &&
		get_element(fp, 0x0008, 0x0032, AN, an, sizeof(an), &items_read) &&
		get_element(fp, 0x0008, 0x0031, AN, an, sizeof(an), &items_read))
		ft = 0;
	else
		extract_floats(an, 1, &ft);
	fclose(fp);
	return (ft);
}

/*****************************************************************************
 * FUNCTION: get_num_frames
 * DESCRIPTION: Returns the number of frames from a DICOM file.
 * PARAMETERS:
 *    filename: the file name
 * SIDE EFFECTS: An error message may be printed.
 * ENTRY CONDITIONS: rec_arch_type should be set.
 * RETURN VALUE: the number of frames
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 4/4/11 by Dewey Odhner
 *
 *****************************************************************************/
int get_num_frames(const char filename[])
{
	int items_read;
	float ft;
	FILE *fp;
	char an[500];

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", filename);
		return 0;
	}
	if (get_element(fp, 0x0028, 0x0008, AN, an, sizeof(an), &items_read))
		ft = 1;
	else
		extract_floats(an, 1, &ft);
	fclose(fp);
	return (int)ft;
}

/*****************************************************************************
 * FUNCTION: get_OffsetVector
 * DESCRIPTION: Returns the grid frame offset vector from a DICOM file.
 * PARAMETERS:
 *    filename: the file name
 * SIDE EFFECTS: An error message may be printed. Memory may be allocated at
 *    the return address; the caller should free it after use.
 * ENTRY CONDITIONS: rec_arch_type should be set.
 * RETURN VALUE: the grid frame offset vector
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 6/4/19 by Dewey Odhner
 *
 *****************************************************************************/
float *get_OffsetVector(const char filename[], int num_frames)
{
	int items_read;
	float *ft;
	FILE *fp;
	char *an;

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", filename);
		return 0;
	}
	ft = (float *)malloc(num_frames*sizeof(float));
	an = (char *)malloc(num_frames*25);
	if (get_element(fp, 0x3004, 0x000c, AN, an, num_frames*25, &items_read))
	{
		free(ft);
		free(an);
		fclose(fp);
		return NULL;
	}
	else
		extract_floats(an, num_frames, ft);
	free(an);
	fclose(fp);
	return ft;
}

bool sequence_tag(unsigned tag)
{
    switch (tag)
    {
        case 0x00080006:
        case 0x00080051:
        case 0x00080063:
        case 0x00080082:
        case 0x00080096:
        case 0x0008009D:
        case 0x00080109:
        case 0x00080110:
        case 0x00080121:
        case 0x00080123:
        case 0x00080124:
        case 0x00080220:
        case 0x00080300:
        case 0x00080305:
        case 0x00080310:
        case 0x00081032:
        case 0x0008103F:
        case 0x00081041:
        case 0x00081049:
        case 0x00081052:
        case 0x00081062:
        case 0x00081072:
        case 0x00081084:
        case 0x00081110:
        case 0x00081111:
        case 0x00081115:
        case 0x00081120:
        case 0x00081125:
        case 0x00081134:
        case 0x0008113A:
        case 0x00081140:
        case 0x0008114A:
        case 0x0008114B:
        case 0x00081156:
        case 0x00081164:
        case 0x00081198:
        case 0x00081199:
        case 0x0008119A:
        case 0x00081200:
        case 0x00081250:
        case 0x00082112:
        case 0x00082133:
        case 0x00082135:
        case 0x00082218:
        case 0x00082220:
        case 0x00082228:
        case 0x00082230:
        case 0x00083001:
        case 0x00083011:
        case 0x00089092:
        case 0x00089121:
        case 0x00089124:
        case 0x00089154:
        case 0x00089215:
        case 0x00089237:
        case 0x00089410:
        case 0x00089458:
        case 0x00100024:
        case 0x00100026:
        case 0x00100027:
        case 0x00100050:
        case 0x00100101:
        case 0x00100102:
        case 0x00100201:
        case 0x00100215:
        case 0x00100216:
        case 0x00100219:
        case 0x00100221:
        case 0x00100229:
        case 0x00101002:
        case 0x00101021:
        case 0x00101100:
        case 0x00102202:
        case 0x00102293:
        case 0x00102294:
        case 0x00102296:
        case 0x00120064:
        case 0x00120083:
        case 0x00140106:
        case 0x00142002:
        case 0x00142012:
        case 0x0014201E:
        case 0x00142030:
        case 0x00142204:
        case 0x00142220:
        case 0x00143020:
        case 0x00143040:
        case 0x00143060:
        case 0x00144002:
        case 0x00144008:
        case 0x0014400E:
        case 0x00144010:
        case 0x00144011:
        case 0x00144020:
        case 0x00144030:
        case 0x00144035:
        case 0x00144040:
        case 0x00144050:
        case 0x00144051:
        case 0x00144060:
        case 0x00144070:
        case 0x00144080:
        case 0x00144083:
        case 0x00144086:
        case 0x00144087:
        case 0x00144091:
        case 0x0014409A:
        case 0x00180012:
        case 0x00180014:
        case 0x00180026:
        case 0x00180029:
        case 0x0018002A:
        case 0x00180036:
        case 0x0018100A:
        case 0x00181272:
        case 0x00182041:
        case 0x00185104:
        case 0x00186011:
        case 0x00189006:
        case 0x00189042:
        case 0x00189045:
        case 0x00189049:
        case 0x00189076:
        case 0x00189083:
        case 0x00189084:
        case 0x00189092:
        case 0x00189103:
        case 0x00189107:
        case 0x00189112:
        case 0x00189114:
        case 0x00189115:
        case 0x00189117:
        case 0x00189118:
        case 0x00189119:
        case 0x00189125:
        case 0x00189126:
        case 0x00189152:
        case 0x00189176:
        case 0x00189197:
        case 0x00189226:
        case 0x00189227:
        case 0x00189239:
        case 0x00189251:
        case 0x0018925D:
        case 0x00189260:
        case 0x00189301:
        case 0x00189304:
        case 0x00189308:
        case 0x00189312:
        case 0x00189314:
        case 0x00189321:
        case 0x00189325:
        case 0x00189326:
        case 0x00189329:
        case 0x00189338:
        case 0x00189340:
        case 0x00189341:
        case 0x00189346:
        case 0x00189360:
        case 0x00189362:
        case 0x00189363:
        case 0x00189364:
        case 0x00189365:
        case 0x0018936F:
        case 0x00189379:
        case 0x0018937D:
        case 0x00189380:
        case 0x00189381:
        case 0x00189382:
        case 0x00189401:
        case 0x00189405:
        case 0x00189406:
        case 0x00189407:
        case 0x00189412:
        case 0x00189417:
        case 0x00189432:
        case 0x00189434:
        case 0x00189451:
        case 0x00189455:
        case 0x00189456:
        case 0x00189462:
        case 0x00189472:
        case 0x00189476:
        case 0x00189477:
        case 0x00189504:
        case 0x00189506:
        case 0x00189507:
        case 0x00189530:
        case 0x00189538:
        case 0x00189541:
        case 0x00189542:
        case 0x00189555:
        case 0x00189556:
        case 0x00189601:
        case 0x00189621:
        case 0x00189732:
        case 0x00189733:
        case 0x00189734:
        case 0x00189735:
        case 0x00189736:
        case 0x00189737:
        case 0x00189749:
        case 0x00189751:
        case 0x00189771:
        case 0x00189772:
        case 0x00189803:
        case 0x00189806:
        case 0x00189807:
        case 0x00189809:
        case 0x0018980D:
        case 0x0018980E:
        case 0x0018980F:
        case 0x00189902:
        case 0x00189903:
        case 0x00189906:
        case 0x00189907:
        case 0x00189909:
        case 0x0018990B:
        case 0x0018990C:
        case 0x0018990D:
        case 0x0018990E:
        case 0x00189911:
        case 0x00189912:
        case 0x00189913:
        case 0x00189914:
        case 0x0018991B:
        case 0x0018991C:
        case 0x0018991D:
        case 0x0018991F:
        case 0x00189920:
        case 0x00189931:
        case 0x00189932:
        case 0x00189933:
        case 0x00189934:
        case 0x00189935:
        case 0x00189936:
        case 0x0018993B:
        case 0x0018993C:
        case 0x0018993D:
        case 0x0018993E:
        case 0x0018A001:
        case 0x00209071:
        case 0x00209111:
        case 0x00209113:
        case 0x00209116:
        case 0x00209170:
        case 0x00209171:
        case 0x00209172:
        case 0x00209221:
        case 0x00209222:
        case 0x00209253:
        case 0x0020930E:
        case 0x0020930F:
        case 0x00209310:
        case 0x00209450:
        case 0x00209529:
        case 0x00220006:
        case 0x00220015:
        case 0x00220016:
        case 0x00220017:
        case 0x00220018:
        case 0x00220019:
        case 0x0022001A:
        case 0x0022001B:
        case 0x0022001C:
        case 0x0022001D:
        case 0x00220020:
        case 0x00220021:
        case 0x00220022:
        case 0x00220031:
        case 0x00220042:
        case 0x00220058:
        case 0x00221007:
        case 0x00221008:
        case 0x00221012:
        case 0x00221024:
        case 0x00221025:
        case 0x00221028:
        case 0x00221035:
        case 0x00221036:
        case 0x00221040:
        case 0x00221044:
        case 0x00221045:
        case 0x00221047:
        case 0x00221048:
        case 0x0022104A:
        case 0x0022104B:
        case 0x00221050:
        case 0x00221090:
        case 0x00221092:
        case 0x00221096:
        case 0x00221100:
        case 0x00221101:
        case 0x00221103:
        case 0x00221125:
        case 0x00221127:
        case 0x00221128:
        case 0x0022112A:
        case 0x00221132:
        case 0x00221133:
        case 0x00221134:
        case 0x00221135:
        case 0x00221150:
        case 0x00221210:
        case 0x00221211:
        case 0x00221212:
        case 0x00221220:
        case 0x00221225:
        case 0x00221230:
        case 0x00221250:
        case 0x00221255:
        case 0x00221257:
        case 0x00221260:
        case 0x00221262:
        case 0x00221300:
        case 0x00221310:
        case 0x00221330:
        case 0x00221420:
        case 0x00221423:
        case 0x00221436:
        case 0x00221443:
        case 0x00221445:
        case 0x00221450:
        case 0x00221458:
        case 0x00221465:
        case 0x00221470:
        case 0x00221472:
        case 0x00221512:
        case 0x00221513:
        case 0x00221518:
        case 0x00221525:
        case 0x00221526:
        case 0x00221612:
        case 0x00221615:
        case 0x00221618:
        case 0x00221620:
        case 0x00221628:
        case 0x00221640:
        case 0x00240016:
        case 0x00240021:
        case 0x00240024:
        case 0x00240032:
        case 0x00240033:
        case 0x00240034:
        case 0x00240058:
        case 0x00240064:
        case 0x00240065:
        case 0x00240067:
        case 0x00240083:
        case 0x00240085:
        case 0x00240089:
        case 0x00240097:
        case 0x00240110:
        case 0x00240112:
        case 0x00240114:
        case 0x00240115:
        case 0x00240122:
        case 0x00240317:
        case 0x00240320:
        case 0x00240325:
        case 0x00240344:
        case 0x00281230:
        case 0x00281352:
        case 0x00281401:
        case 0x00281404:
        case 0x0028140B:
        case 0x0028140C:
        case 0x00283000:
        case 0x00283010:
        case 0x00283110:
        case 0x00286100:
        case 0x00287000:
        case 0x00287008:
        case 0x0028700A:
        case 0x0028700F:
        case 0x00287010:
        case 0x00287011:
        case 0x00287012:
        case 0x00287015:
        case 0x00287016:
        case 0x0028701C:
        case 0x00287022:
        case 0x00287023:
        case 0x00287024:
        case 0x00287027:
        case 0x00287028:
        case 0x0028702C:
        case 0x0028702D:
        case 0x0028702E:
        case 0x00289110:
        case 0x00289132:
        case 0x00289145:
        case 0x00289415:
        case 0x00289422:
        case 0x00289443:
        case 0x00289501:
        case 0x00289502:
        case 0x00289505:
        case 0x00321031:
        case 0x00321034:
        case 0x00321064:
        case 0x00321067:
        case 0x00380004:
        case 0x00380014:
        case 0x00380064:
        case 0x00380100:
        case 0x00380101:
        case 0x00380502:
        case 0x003A0200:
        case 0x003A0208:
        case 0x003A0209:
        case 0x003A020A:
        case 0x003A0211:
        case 0x003A0240:
        case 0x003A0242:
        case 0x003A0300:
        case 0x00400008:
        case 0x0040000A:
        case 0x0040000B:
        case 0x00400026:
        case 0x00400027:
        case 0x00400036:
        case 0x00400039:
        case 0x0040003A:
        case 0x00400100:
        case 0x00400220:
        case 0x00400260:
        case 0x00400270:
        case 0x00400275:
        case 0x00400281:
        case 0x00400293:
        case 0x00400295:
        case 0x00400296:
        case 0x00400320:
        case 0x00400321:
        case 0x00400324:
        case 0x00400340:
        case 0x00400440:
        case 0x00400441:
        case 0x00400500:
        case 0x00400513:
        case 0x00400515:
        case 0x00400518:
        case 0x00400520:
        case 0x00400555:
        case 0x0040059A:
        case 0x00400560:
        case 0x00400562:
        case 0x00400610:
        case 0x00400612:
        case 0x00400620:
        case 0x00400710:
        case 0x0040071A:
        case 0x004008EA:
        case 0x0040100A:
        case 0x00401011:
        case 0x00401012:
        case 0x00401101:
        case 0x00404009:
        case 0x00404018:
        case 0x00404019:
        case 0x00404021:
        case 0x00404025:
        case 0x00404026:
        case 0x00404027:
        case 0x00404028:
        case 0x00404029:
        case 0x00404030:
        case 0x00404033:
        case 0x00404034:
        case 0x00404035:
        case 0x00404070:
        case 0x00404071:
        case 0x00404072:
        case 0x00404074:
        case 0x00409092:
        case 0x00409094:
        case 0x00409096:
        case 0x00409098:
        case 0x00409220:
        case 0x0040A043:
        case 0x0040A073:
        case 0x0040A078:
        case 0x0040A07A:
        case 0x0040A07C:
        case 0x0040A088:
        case 0x0040A168:
        case 0x0040A170:
        case 0x0040A195:
        case 0x0040A300:
        case 0x0040A301:
        case 0x0040A360:
        case 0x0040A370:
        case 0x0040A372:
        case 0x0040A375:
        case 0x0040A385:
        case 0x0040A390:
        case 0x0040A504:
        case 0x0040A525:
        case 0x0040A730:
        case 0x0040B020:
        case 0x0040E006:
        case 0x0040E008:
        case 0x0040E021:
        case 0x0040E022:
        case 0x0040E023:
        case 0x0040E024:
        case 0x0040E025:
        case 0x00420013:
        case 0x00440007:
        case 0x00440013:
        case 0x00440019:
        case 0x00440100:
        case 0x00440101:
        case 0x00440103:
        case 0x00440107:
        case 0x00440109:
        case 0x0044010A:
        case 0x00460014:
        case 0x00460015:
        case 0x00460016:
        case 0x00460018:
        case 0x00460028:
        case 0x00460047:
        case 0x00460050:
        case 0x00460052:
        case 0x00460070:
        case 0x00460071:
        case 0x00460074:
        case 0x00460080:
        case 0x00460097:
        case 0x00460098:
        case 0x00460100:
        case 0x00460101:
        case 0x00460102:
        case 0x00460110:
        case 0x00460111:
        case 0x00460112:
        case 0x00460113:
        case 0x00460116:
        case 0x00460121:
        case 0x00460122:
        case 0x00460123:
        case 0x00460124:
        case 0x00460145:
        case 0x00460207:
        case 0x00460210:
        case 0x00460211:
        case 0x00460215:
        case 0x00460218:
        case 0x00460244:
        case 0x00480008:
        case 0x00480100:
        case 0x00480105:
        case 0x00480108:
        case 0x00480110:
        case 0x00480120:
        case 0x00480200:
        case 0x00480207:
        case 0x0048021A:
        case 0x00500010:
        case 0x00500012:
        case 0x00520016:
        case 0x00520025:
        case 0x00520027:
        case 0x00520029:
        case 0x00540012:
        case 0x00540013:
        case 0x00540016:
        case 0x00540022:
        case 0x00540032:
        case 0x00540052:
        case 0x00540062:
        case 0x00540063:
        case 0x00540072:
        case 0x00540220:
        case 0x00540222:
        case 0x00540300:
        case 0x00540302:
        case 0x00540304:
        case 0x00540306:
        case 0x00540410:
        case 0x00540412:
        case 0x00540414:
        case 0x00603000:
        case 0x00620002:
        case 0x00620003:
        case 0x00620007:
        case 0x0062000A:
        case 0x0062000F:
        case 0x00620011:
        case 0x00620012:
        case 0x00640002:
        case 0x00640005:
        case 0x0064000F:
        case 0x00640010:
        case 0x00660002:
        case 0x00660011:
        case 0x00660012:
        case 0x00660013:
        case 0x00660026:
        case 0x00660027:
        case 0x00660028:
        case 0x0066002B:
        case 0x0066002D:
        case 0x0066002E:
        case 0x0066002F:
        case 0x00660030:
        case 0x00660034:
        case 0x00660035:
        case 0x00660101:
        case 0x00660102:
        case 0x00660104:
        case 0x00660108:
        case 0x00660121:
        case 0x00660124:
        case 0x00660130:
        case 0x00660132:
        case 0x00660133:
        case 0x00660134:
        case 0x00686222:
        case 0x00686224:
        case 0x00686225:
        case 0x00686230:
        case 0x00686260:
        case 0x00686265:
        case 0x006862A0:
        case 0x006862C0:
        case 0x006862E0:
        case 0x006862F0:
        case 0x00686320:
        case 0x00686360:
        case 0x006863A0:
        case 0x006863A4:
        case 0x006863A8:
        case 0x006863AC:
        case 0x006863B0:
        case 0x006863E0:
        case 0x00686400:
        case 0x00686430:
        case 0x00686470:
        case 0x00686500:
        case 0x00686510:
        case 0x00686520:
        case 0x00686545:
        case 0x00686550:
        case 0x006865A0:
        case 0x006865E0:
        case 0x00687003:
        case 0x00700001:
        case 0x00700008:
        case 0x00700009:
        case 0x0070005A:
        case 0x00700060:
        case 0x00700086:
        case 0x00700087:
        case 0x00700209:
        case 0x00700231:
        case 0x00700232:
        case 0x00700233:
        case 0x00700234:
        case 0x00700287:
        case 0x00700308:
        case 0x00700309:
        case 0x0070030A:
        case 0x0070030D:
        case 0x00700311:
        case 0x00700314:
        case 0x00700318:
        case 0x0070031C:
        case 0x0070031E:
        case 0x0070031F:
        case 0x00700402:
        case 0x00700404:
        case 0x00701104:
        case 0x00701201:
        case 0x0070120A:
        case 0x00701301:
        case 0x00701304:
        case 0x00701801:
        case 0x00701803:
        case 0x00701805:
        case 0x00701806:
        case 0x00701901:
        case 0x00701903:
        case 0x00701905:
        case 0x00701A04:
        case 0x00701A08:
        case 0x00701B01:
        case 0x00701B03:
        case 0x00701B04:
        case 0x00701B11:
        case 0x00701B12:
        case 0x0072000C:
        case 0x0072000E:
        case 0x00720012:
        case 0x00720020:
        case 0x00720022:
        case 0x00720030:
        case 0x0072003E:
        case 0x00720080:
        case 0x00720102:
        case 0x00720200:
        case 0x00720210:
        case 0x00720214:
        case 0x00720300:
        case 0x00720400:
        case 0x00720422:
        case 0x00720424:
        case 0x00720427:
        case 0x00720430:
        case 0x00720600:
        case 0x00720705:
        case 0x00741002:
        case 0x00741007:
        case 0x00741008:
        case 0x0074100E:
        case 0x00741020:
        case 0x00741030:
        case 0x00741040:
        case 0x00741042:
        case 0x00741044:
        case 0x00741046:
        case 0x00741048:
        case 0x0074104A:
        case 0x0074104C:
        case 0x0074104E:
        case 0x00741050:
        case 0x00741210:
        case 0x00741212:
        case 0x00741216:
        case 0x00741224:
        case 0x00741401:
        case 0x00741405:
        case 0x00741409:
        case 0x0074140D:
        case 0x0074140E:
        case 0x00760008:
        case 0x0076000C:
        case 0x0076000E:
        case 0x00760010:
        case 0x00760020:
        case 0x00760032:
        case 0x00760034:
        case 0x00760040:
        case 0x00760060:
        case 0x00780026:
        case 0x00780028:
        case 0x0078002A:
        case 0x00780070:
        case 0x007800B0:
        case 0x007800B4:
        case 0x00800001:
        case 0x00800002:
        case 0x00800003:
        case 0x00800008:
        case 0x00800012:
        case 0x00800013:
        case 0x00820004:
        case 0x00820005:
        case 0x00820007:
        case 0x0082000C:
        case 0x00820010:
        case 0x00820017:
        case 0x00820021:
        case 0x00820022:
        case 0x00820034:
        case 0x00820035:
        case 0x00880200:
        case 0x04000401:
        case 0x04000402:
        case 0x04000403:
        case 0x04000500:
        case 0x04000550:
        case 0x04000551:
        case 0x04000561:
        case 0x2000001E:
        case 0x200000A2:
        case 0x200000A4:
        case 0x200000A8:
        case 0x20000500:
        case 0x20100500:
        case 0x20100510:
        case 0x20100520:
        case 0x20200110:
        case 0x20200111:
        case 0x20500010:
        case 0x20500500:
        case 0x2200000D:
        case 0x30020030:
        case 0x30020040:
        case 0x30020050:
        case 0x30040010:
        case 0x30040050:
        case 0x30040060:
        case 0x30060010:
        case 0x30060012:
        case 0x30060014:
        case 0x30060016:
        case 0x30060018:
        case 0x30060020:
        case 0x30060030:
        case 0x30060037:
        case 0x30060039:
        case 0x30060040:
        case 0x30060080:
        case 0x30060086:
        case 0x300600A0:
        case 0x300600B0:
        case 0x300600B6:
        case 0x30080010:
        case 0x30080020:
        case 0x30080021:
        case 0x30080030:
        case 0x30080040:
        case 0x30080041:
        case 0x30080050:
        case 0x30080060:
        case 0x30080068:
        case 0x30080070:
        case 0x30080080:
        case 0x30080090:
        case 0x300800A0:
        case 0x300800B0:
        case 0x300800C0:
        case 0x300800D0:
        case 0x300800E0:
        case 0x300800F0:
        case 0x300800F2:
        case 0x300800F4:
        case 0x300800F6:
        case 0x30080100:
        case 0x30080110:
        case 0x30080120:
        case 0x30080130:
        case 0x30080140:
        case 0x30080150:
        case 0x30080160:
        case 0x30080171:
        case 0x30080173:
        case 0x30080220:
        case 0x30080240:
        case 0x300A0010:
        case 0x300A0040:
        case 0x300A0048:
        case 0x300A0070:
        case 0x300A008C:
        case 0x300A00B0:
        case 0x300A00B6:
        case 0x300A00CA:
        case 0x300A00D1:
        case 0x300A00E3:
        case 0x300A00F4:
        case 0x300A0107:
        case 0x300A0111:
        case 0x300A0116:
        case 0x300A011A:
        case 0x300A0180:
        case 0x300A0190:
        case 0x300A01A0:
        case 0x300A01B4:
        case 0x300A0206:
        case 0x300A0210:
        case 0x300A0230:
        case 0x300A0260:
        case 0x300A0280:
        case 0x300A02B0:
        case 0x300A02D0:
        case 0x300A02EA:
        case 0x300A030C:
        case 0x300A0314:
        case 0x300A0332:
        case 0x300A0342:
        case 0x300A0360:
        case 0x300A0370:
        case 0x300A0380:
        case 0x300A03A0:
        case 0x300A03A2:
        case 0x300A03A4:
        case 0x300A03A6:
        case 0x300A03A8:
        case 0x300A03AA:
        case 0x300A03AC:
        case 0x300A0401:
        case 0x300A0410:
        case 0x300A0420:
        case 0x300A0431:
        case 0x300A0441:
        case 0x300A0450:
        case 0x300A0453:
        case 0x300A0505:
        case 0x300A0506:
        case 0x300C0002:
        case 0x300C0004:
        case 0x300C000A:
        case 0x300C0020:
        case 0x300C0040:
        case 0x300C0042:
        case 0x300C0050:
        case 0x300C0055:
        case 0x300C0060:
        case 0x300C0080:
        case 0x300C00B0:
        case 0x300C00F2:
        case 0x300C0111:
        case 0x30100001:
        case 0x30100003:
        case 0x30100004:
        case 0x30100007:
        case 0x30100008:
        case 0x30100009:
        case 0x3010000A:
        case 0x30100011:
        case 0x30100012:
        case 0x30100014:
        case 0x30100016:
        case 0x30100018:
        case 0x30100019:
        case 0x30100021:
        case 0x30100023:
        case 0x30100024:
        case 0x30100025:
        case 0x30100026:
        case 0x30100027:
        case 0x30100028:
        case 0x3010002A:
        case 0x3010002B:
        case 0x3010002C:
        case 0x3010002E:
        case 0x30100030:
        case 0x30100032:
        case 0x30100044:
        case 0x30100049:
        case 0x3010004A:
        case 0x3010004B:
        case 0x3010004E:
        case 0x30100055:
        case 0x30100057:
        case 0x3010005B:
        case 0x3010005D:
        case 0x3010005F:
        case 0x30100060:
        case 0x30100062:
        case 0x30100064:
        case 0x30100065:
        case 0x30100067:
        case 0x30100069:
        case 0x3010006A:
        case 0x3010006B:
        case 0x3010006C:
        case 0x3010006D:
        case 0x30100070:
        case 0x30100071:
        case 0x30100076:
        case 0x30100078:
        case 0x30100079:
        case 0x30100080:
        case 0x30100081:
        case 0x30100082:
        case 0x30100087:
        case 0x30100088:
        case 0x40100004:
        case 0x40101001:
        case 0x4010100A:
        case 0x40101011:
        case 0x40101037:
        case 0x40101038:
        case 0x40101045:
        case 0x40101047:
        case 0x40101064:
        case 0x4010106F:
        case 0x40101071:
        case 0x40101072:
        case 0x40101076:
        case 0x40101077:
        case 0x40101079:
        case 0x4010107B:
        case 0x4010107D:
        case 0x4FFE0001:
        case 0x52009229:
        case 0x52009230:
        case 0x54000100:
        case 0xFFFAFFFA:
        case 0x00081100:
        case 0x00081130:
        case 0x00081145:
        case 0x00082229:
        case 0x00082240:
        case 0x00082242:
        case 0x00082244:
        case 0x00082246:
        case 0x00082251:
        case 0x00082253:
        case 0x00082255:
        case 0x00082257:
        case 0x00082259:
        case 0x0008225A:
        case 0x0008225C:
        case 0x00221153:
        case 0x00221265:
        case 0x00285000:
        case 0x00380044:
        case 0x0040030E:
        case 0x00400330:
        case 0x00400550:
        case 0x00400552:
        case 0x004008D8:
        case 0x004008DA:
        case 0x004009F8:
        case 0x00404004:
        case 0x00404007:
        case 0x00404015:
        case 0x00404016:
        case 0x00404022:
        case 0x00404031:
        case 0x00404032:
        case 0x0040A020:
        case 0x0040A026:
        case 0x0040A028:
        case 0x0040A066:
        case 0x0040A068:
        case 0x0040A070:
        case 0x0040A076:
        case 0x0040A085:
        case 0x0040A090:
        case 0x0040A167:
        case 0x0040A296:
        case 0x0040A313:
        case 0x0040A340:
        case 0x0040A358:
        case 0x0040A380:
        case 0x0040A404:
        case 0x0040A731:
        case 0x0040A732:
        case 0x0040A744:
        case 0x00741220:
        case 0x20000510:
        case 0x20200130:
        case 0x20200140:
        case 0x20400010:
        case 0x20400020:
        case 0x20400500:
        case 0x21000500:
        case 0x21200050:
        case 0x21200070:
        case 0x21300010:
        case 0x21300015:
        case 0x21300030:
        case 0x21300040:
        case 0x21300050:
        case 0x21300060:
        case 0x21300080:
        case 0x213000A0:
        case 0x213000C0:
        case 0x300600B9:
        case 0x300600C0:
        case 0x40080050:
        case 0x40080111:
        case 0x40080117:
        case 0x40080118:
            return true;
        default:
        //case 0x50XX2600:
			if (tag>=0x50002600 && tag<=0x50ff2600 && (tag&0xffff)==0x2600)
				return true;
            break;
    }
	return false;
}
