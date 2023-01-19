/*
  Copyright 1993-2004, 2021 Medical Image Processing Group
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
 
 
 
 
/*
  Convert a single .img file from IMATRON C-150 to IM0 (MIPG image) format

  Arguments: the name of the input IMATRON .img file (with extension) and
     the name of the output IM0 file (with extension) 

  Created: 6/3/99 by Laszlo Nyul
*/

#include <stdio.h>
#include <stdlib.h>
#include <cv3dv.h>
#include <ctype.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

static char program_name[] = "imatron_to_IM0";
static char program_version[] = "1.0";
static char recognition_code[] = "VIEWNIX1.0";
static char temp_file_name[] = "imatron_to_IM0.tmp";

ViewnixHeader vh;
FILE *vf;
FILE *inf;

int ARGC;
char **ARGV;

int min_dens, max_dens;

#define KEY_STRUCT_SIZE 44

typedef struct {
  unsigned char key[22];
  unsigned int length;
  unsigned int offset;
  unsigned short type;
  unsigned char pad[10];
} key_struct;

unsigned int first_key_start;
unsigned int keys_length;
unsigned int last_key_start;
unsigned int first_value_start;
unsigned int values_end;

unsigned int header_size;
unsigned char *header;

int bpp;
unsigned int buflen;
int xdim, ydim, zdim, tdim;
char *buffer;

typedef union {
  unsigned char uc[120];
  unsigned short us[120];
  unsigned int ul[120];
  float fl[120];
  unsigned char buf[2048];
} value_struct;

value_struct value;

char *filename;

/*****************************************************************************
 * FUNCTION: bigendian
 * DESCRIPTION: Checks the byte order of the architecture the program
 *    is running on.
 * PARAMETERS: none
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: none
 * RETURN VALUE: returns 1 if the machine is BigEndian (most significant
 *    byte first), 0 otherwise.
 * EXIT CONDITIONS: none
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
int
bigendian(void)
{
  union
  {
    int l;
    char c[sizeof (int)];
  } u;
  u.l = 1;
  return (u.c[sizeof (int) - 1] == 1);
}

/*****************************************************************************
 * FUNCTION: swapbyte
 * DESCRIPTION: Changes the byte order of the elements of an array from
 *    MSB to LSB or from LSB to MSB. Each element in the array is
 *    reordered individually.
 * PARAMETERS:
 *    ptr: pointer to the array
 *    size: size of an element in the array (in bytes)
 *    n: number of elements in the array
 * SIDE EFFECTS: the bytes in the array pointed by ptr will be rearranged
 * ENTRY CONDITIONS: ptr must be allocated of a size at least size*n
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
swapbyte(ptr, size, n)
  void *ptr;
  size_t size;
  size_t n;
{
  unsigned char *p;
  size_t i, j, k;
  unsigned char c;

  for (i = 0, p = ptr; i < n; i++, p += size)
  {
    for (j = 0, k = size - 1; j < k; j++, k--)
    {
      c = p[j];
      p[j] = p[k];
      p[k] = c;
    }
  }
}

/*****************************************************************************
 * FUNCTION: makenativeendian
 * DESCRIPTION: Rearranges the bytes of the elements of an array if they
 *    are on in the native order of the architecture. Assumes that the
 *    input data is LittleEndian (LSB).
 * PARAMETERS:
 *    ptr: pointer to the array
 *    size: size of an element in the array (in bytes)
 *    n: number of elements in the array
 * SIDE EFFECTS: the bytes in the array pointed by ptr might be rearranged
 * ENTRY CONDITIONS: ptr must be allocated of a size at least size*n
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
makenativeendian(ptr, size, n)
  void *ptr;
  size_t size;
  size_t n;
{
  if (bigendian())
    swapbyte(ptr, size, n);
}

/*****************************************************************************
 * FUNCTION: read_imatron_header
 * DESCRIPTION: Reads the header of the IMATRON .img file and stores
 *    it for further processing.
 * PARAMETERS: none
 * SIDE EFFECTS: first_key_start, first_value_start, keys_length,
 *    values_end, last_key_start, header_size will be set,
 *    header will be allocated and filled with the header data read
 *    from the file.
 * ENTRY CONDITIONS: The input file (inf) must be open for reading and
 *   the reading pointer should be set to the beginning of the file.
 * RETURN VALUE: none
 *    Exits with 1 if memory allocation fails.
 * EXIT CONDITIONS: undetermined if input conditions are not met.
 *    The reading pointer of inf will be moved (by fseek and fread calls).
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
read_imatron_header()
{
  unsigned int ul;

  fseek(inf, 23, SEEK_SET);
  fread(&ul, 4, 1, inf);
  makenativeendian(&ul, 4, 1);
  first_key_start = ul;
  fseek(inf, 31, SEEK_SET);
  fread(&ul, 4, 1, inf);
  makenativeendian(&ul, 4, 1);
  first_value_start = ul;
  fread(&ul, 4, 1, inf);
  makenativeendian(&ul, 4, 1);
  keys_length = ul;
  fread(&ul, 4, 1, inf);
  makenativeendian(&ul, 4, 1);
  values_end = ul;
  fseek(inf, 82, SEEK_SET);
  fread(&ul, 4, 1, inf);
  makenativeendian(&ul, 4, 1);
  last_key_start = ul;
  header_size = values_end;
  header = (unsigned char*)malloc(header_size);
  if (!header)
  {
    printf("Cannot allocate memory for header data\n");
    exit(1);
  }
  fseek(inf, 0, SEEK_SET);
  fread(header, header_size, 1, inf);
}

/*****************************************************************************
 * FUNCTION: read_imatron_value
 * DESCRIPTION: Searches for the value of a key within the IMATRON header.
 *    If found, returns the value.
 * PARAMETERS:
 *    keylabel: a 0-terminated character string containing the key label
 *    keyvalue: if the key label is found in the header, the corresponging
 *       value is copied to this structure (with conversions, if necessary)
 *       Values are stored in keyvalue in the field corresponding to their
 *       type. Multiple values are stored in arrays, single values are
 *       stored in the 0-index element of the array.
 * SIDE EFFECTS: the structure pointed by keyvalue will be set
 * ENTRY CONDITIONS: first_key_start, first_value_start must be set,
 *    header must be allocated and filled with the header data read from
 *    the file by read_imatron_header. keyvalue must point to a valid,
 *    allocated structure of type value_struct.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met.
 *    keyvalue undetermined if keylabel not found.
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
read_imatron_value(keylabel, keyvalue)
  char *keylabel;
  value_struct *keyvalue;
{
  unsigned int pos;
  key_struct key;
  unsigned char *key_buf = (unsigned char *)&key;
  char buffer[KEY_STRUCT_SIZE];

  pos = first_key_start;
  while (pos < first_value_start)
  {
    memcpy(&buffer, header + pos, KEY_STRUCT_SIZE);
    memcpy(&key.key, buffer, 22);
    if (strcmp(keylabel, (const char *)key.key))
    {
      pos += KEY_STRUCT_SIZE;
      continue;
    }
    memcpy(&key.length, buffer+22, 4);
    memcpy(&key.offset, buffer+26, 4);
    memcpy(&key.type, buffer+30, 2);
    memcpy(&key.pad, buffer+34, 8);
    makenativeendian(&key.length, 4, 1);
    makenativeendian(&key.offset, 4, 1);
    makenativeendian(&key.type, 2, 1);
    switch (key.type)
    {
      case 0: /* string */
        memcpy(keyvalue->buf, header + key.offset, key.length);
        break;

      case 1: /* unsigned short */
        memcpy(keyvalue->buf, header + key.offset, key.length);
        makenativeendian(&keyvalue->buf, 2, key.length / 2);
        break;

      case 2: /* unsigned int */
        memcpy(keyvalue->buf, header + key.offset, key.length);
        makenativeendian(&keyvalue->buf, 4, key.length / 4);
        break;

      case 3: /* float */
        memcpy(keyvalue->buf, header + key.offset, key.length);
        makenativeendian(&keyvalue->buf, 4, key.length / 4);
        break;

      case 4: /* date */
        memcpy(keyvalue->buf, header + key.offset, key.length);
        break;
    }
    break;
  }
  if (pos >= first_value_start)
  {
/* not found */
  }
}

/*****************************************************************************
 * FUNCTION: data_to_DateFormat
 * DESCRIPTION: Converts a date value from the IMATRON representation
 *    into the 3DViewnix representation.
 * PARAMETERS:
 *    from: pointer to the 0-terminated string containing the IMATRON date
 *    to: pointer to the buffer where the 3DViewnix data will be stored
 * SIDE EFFECTS: 'to' buffer will be set
 * ENTRY CONDITIONS: from must be a pointer of a 0-terminated string
 *    containing a date value in the IMATRON representation. to must be
 *    a pointer to a valid buffer to store at least 11 characters.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified: 8/31/18 month first allowed by Dewey Odhner
 *
 *****************************************************************************/
void
date_to_DateFormat(char *from, char *to)
{
  static char mon[13][4] = { "",
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
  char *from2;
  char *brk1, *brk2, *brk3;
  int yy, mm, dd;

  from2 = strdup(from);
  brk1 = strtok(from2, "-");
  brk2 = strtok(0, "-");
  if (brk1 && sscanf(brk1, "%d", &dd)==1)
  {
    for (mm = 1; mm <= 12; mm++)
      if (toupper(brk2[0])==mon[mm][0] &&
          toupper(brk2[1])==mon[mm][1] &&
          toupper(brk2[2])==mon[mm][2])
        break;
  }
  else if (brk2 && sscanf(brk2, "%d", &dd)==1)
  {
    for (mm = 1; mm <= 12; mm++)
      if (toupper(brk1[0])==mon[mm][0] &&
          toupper(brk1[1])==mon[mm][1] &&
          toupper(brk1[2])==mon[mm][2])
        break;
  }
  else
  {
    fprintf(stderr, "date_to_DateFormat failed.\n");
    strcpy(to, from);
    return;
  }
  brk3 = strtok(0, "-");
  sscanf(brk3, "%d", &yy);
  if (yy < 100)
    yy += 1900;
  free(from2);
  sprintf(to, "%04d.%02d.%02d", yy, mm, dd);
}
 
/*****************************************************************************
 * FUNCTION: time_to_TimeFormat
 * DESCRIPTION: Converts a time value from the IMATRON representation
 *    into the 3DViewnix representation.
 * PARAMETERS:
 *    from: contains the IMATRON time (in seconds)
 *    to: pointer to the buffer where the 3DViewnix time will be stored
 * SIDE EFFECTS: 'to' buffer will be set
 * ENTRY CONDITIONS: to must be a pointer to a valid buffer to store at
 *    least 9 characters.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
time_to_TimeFormat(unsigned int from, char *to)
{
  int hh, mm, ss;

  ss = from;
  mm = ss / 60;
  ss -= mm * 60;
  hh = mm / 60;
  mm -= hh * 60;
  sprintf(to, "%02d:%02d:%02d", hh, mm, ss);
}

/*****************************************************************************
 * FUNCTION: setup_viewnix_header
 * DESCRIPTION: Sets the necessary fields of the ViewnixHeader structure
 *    and those having corresponding values in the imatron header.
 * PARAMETERS: none
 * SIDE EFFECTS: the bytes in the array pointed by ptr might be rearranged
 * ENTRY CONDITIONS: read_imatron_header should be called before to
 *    setup variables for the subsequent read_imatron_value calls.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met.
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
setup_viewnix_header()
{
  char buffer[10241];
  unsigned short us, us2;
  int i, j, k;
  float slice_spacing;

  memset(&vh, 0, sizeof(vh));
  
  {
    strncpy(vh.gen.recognition_code, recognition_code, 79);
    vh.gen.recognition_code_valid = 1;
  }

/* study date */
  read_imatron_value("study date", &value);
  {
    date_to_DateFormat((char *)value.buf, vh.gen.study_date);
    vh.gen.study_date_valid = 1;
  }

/* study time */
  read_imatron_value("study time", &value);
  {
    time_to_TimeFormat(value.ul[0], vh.gen.study_time);
    vh.gen.study_time_valid = 1;
  }
 
  {
    vh.gen.data_type = IMAGE0;
    vh.gen.data_type_valid = 1;
  }

/* "CT" */
  {
    strncpy(vh.gen.modality, "CT", 2);
    vh.gen.modality_valid = 1;
  }

/* hospital */
  read_imatron_value("hospital", &value);
  {
    strncpy(vh.gen.institution, (const char *)value.buf, 79);
    vh.gen.institution_valid = 1;
  }

/* referring phys */
  read_imatron_value("referring phys", &value);
  {
    strncpy(vh.gen.physician, (const char *)value.buf, 79);
    vh.gen.physician_valid = 1;
  }

/* department */
  read_imatron_value("department", &value);
  {
    strncpy(vh.gen.department, (const char *)value.buf, 79);
    vh.gen.department_valid = 1;
  }

/* radiologist */
  read_imatron_value("radiologist", &value);
  {
    strncpy(vh.gen.radiologist, (const char *)value.buf, 79);
    vh.gen.radiologist_valid = 1;
  }

/* scanner */
  read_imatron_value("scanner", &value);
  {
    strncpy(vh.gen.model, (const char *)value.buf, 79);
    vh.gen.model_valid = 1;
  }

  {
    strncpy(vh.gen.filename, filename, 79);
    vh.gen.filename_valid = 1;
  }

  {
/*    vh.gen.filename1 */
  }

/* stress description */
  read_imatron_value("stress description", &value);
  {
    vh.gen.description = strdup((const char *)value.buf);
    vh.gen.description_valid = 1;
  }

/* comment */
  read_imatron_value("comment", &value);
  {
    strcpy(buffer, ARGV[0]);
    for (i = 1; i < ARGC; i++)
    {
      strcat(buffer, " ");
      strcat(buffer, ARGV[i]);
    }
    vh.gen.comment = strdup(buffer);
    vh.gen.comment_valid = 1;
  }

/* patient name */
  read_imatron_value("patient name", &value);
  {
    strncpy(vh.gen.patient_name, (const char *)value.buf, 79);
    vh.gen.patient_name_valid = 1;
  }

/* patient ID */
  read_imatron_value("patient ID", &value);
  {
    strncpy(vh.gen.patient_id, (const char *)value.buf, 79);
    vh.gen.patient_id_valid = 1;
  }

/* slice thickness */
  read_imatron_value("slice thickness", &value);
  {
    vh.gen.slice_thickness = value.fl[0];
    vh.gen.slice_thickness_valid = 1;
  }

/*
  {
    vh.gen.kvp[0] = dbl;
    vh.gen.kvp_valid = 1;
  }
*/

/*
  {
    vh.gen.repetition_time = dbl;
    vh.gen.repetition_time_valid = 1;
  }
*/

/*
  {
    vh.gen.echo_time = dbl;
    vh.gen.echo_time_valid = 1;
  }
*/

  {
    buffer[0] = '\0';
    vh.gen.imaged_nucleus = strdup(buffer);
    vh.gen.imaged_nucleus_valid = 1;
  }

/*
  {
    vh.gen.gantry_tilt = dbl;
    vh.gen.gantry_tilt_valid = 1;
  }
*/

/* study */
  read_imatron_value("study", &value);
  {
    strncpy(vh.gen.study, (const char *)value.buf, 9);
    vh.gen.study_valid = 1;
  }

/* series */
  read_imatron_value("series", &value);
  {
    strncpy(vh.gen.series, (const char *)value.buf, 9);
    vh.gen.series_valid = 1;
  }

/*  vh.gen.gray_descriptor[] */
/*  vh.gen.red_descriptor[] */
/*  vh.gen.gree_descriptor[] */
/*  vh.gen.blue_descriptor[] */
/*  vh.gen.gray_lookup_data */
/*  vh.gen.red_lookup_data */
/*  vh.gen.gree_lookup_data */
/*  vh.gen.blue_lookup_data */

/* tdim */
  read_imatron_value("tdim", &value);
  tdim = value.us[0];
  {
    vh.scn.dimension = tdim > 1 ? 4 : 3;
    vh.scn.dimension_valid = 1;
  }

  {
    vh.scn.domain = (float *)malloc((vh.scn.dimension + 1) * vh.scn.dimension * sizeof(float));
    for (i = 0; i < vh.scn.dimension; i++)
      vh.scn.domain[i] = 0.0;
    for (j = 0; j < vh.scn.dimension; j++)
      for (i = 0; i < vh.scn.dimension; i++)
        if (i == j)
          vh.scn.domain[vh.scn.dimension + j * vh.scn.dimension + i] = 1.0;
        else
          vh.scn.domain[vh.scn.dimension + j * vh.scn.dimension + i] = 0.0;
    vh.scn.domain_valid = 1;
  }

  {
/*    vh.scn.axis_label */
  }

  {
    vh.scn.measurement_unit = (short *)malloc(vh.scn.dimension * sizeof(short));
    for (i = 0; i < vh.scn.dimension; i++)
      vh.scn.measurement_unit[i] = 3;
    vh.scn.measurement_unit_valid = 1;
  }

  {
    us = 1;
    vh.scn.num_of_density_values = us;
    vh.scn.num_of_density_values_valid = 1;
  }

  {
/*    vh.scn.density_measurement_unit */
  }

/* setup these values later */ 

/*
  {
    vh.scn.smallest_density_value = (float *)malloc(vh.scn.num_of_density_values * sizeof(float));
    for (i = 0; i < vh.scn.num_of_density_values; i++)
      vh.scn.smallest_density_value[i] = min_dens;
    vh.scn.smallest_density_value_valid = 1;
  }

  {
    vh.scn.largest_density_value = (float *)malloc(vh.scn.num_of_density_values * sizeof(float));
    for (i = 0; i < vh.scn.num_of_density_values; i++)
      vh.scn.largest_density_value[i] = max_dens;
    vh.scn.largest_density_value_valid = 1;
  }
*/

  {
    vh.scn.num_of_integers = vh.scn.num_of_density_values;
    vh.scn.num_of_integers_valid = 1;
  }

  {
    us = 0;
    vh.scn.signed_bits = (short *)malloc(vh.scn.num_of_density_values * sizeof(short));
    for (i = 0; i < vh.scn.num_of_density_values; i++)
      vh.scn.signed_bits[i] = us;
    vh.scn.signed_bits_valid = 1;
  }

/* pack */
  read_imatron_value("pack", &value);
  bpp = value.us[0] >> 3;
  {
    vh.scn.num_of_bits = value.us[0];
    vh.scn.num_of_bits_valid = 1;
  }

/* pack */
  read_imatron_value("pack", &value);
  {
    us = value.us[0];
    {
      us2 = value.us[0] - 1;
      vh.scn.bit_fields = (short *)malloc(2 * vh.scn.num_of_density_values * sizeof(short));
      for (i = 0; i < 2 * vh.scn.num_of_density_values; i+=2)
      {
        vh.scn.bit_fields[i] = us2 - us + 1;
        vh.scn.bit_fields[i+1] = us2;
      }
      vh.scn.bit_fields_valid = 1;
    }
  }

  {
    vh.scn.dimension_in_alignment = 2;
    vh.scn.dimension_in_alignment_valid = 1;
  }

  {
    vh.scn.bytes_in_alignment = 1;
    vh.scn.bytes_in_alignment_valid = 1;
  }

/* xdim */
  read_imatron_value("xdim", &value);
  xdim = value.us[0];
/* ydim */
  read_imatron_value("ydim", &value);
  ydim = value.us[0];
  {
    vh.scn.xysize[0] = xdim;
    vh.scn.xysize[1] = ydim;
    vh.scn.xysize_valid = 1;
  }

/* zdim */
  read_imatron_value("zdim", &value);
  zdim = value.us[0];
  {
    if (tdim > 1)
    {
      vh.scn.num_of_subscenes = (short *)malloc((1 + tdim) * sizeof(short));
      vh.scn.num_of_subscenes[0] = tdim;
      for (i = 1; i <= tdim; i++)
        vh.scn.num_of_subscenes[i] = zdim;
    }
    else
    {
      vh.scn.num_of_subscenes = (short *)malloc(1 * sizeof(short));
      vh.scn.num_of_subscenes[0] = zdim;
    }
    vh.scn.num_of_subscenes_valid = 1;
  }

  buflen = (int)xdim * /*SamplesPerPixel * */bpp;

/* pixel size */
  read_imatron_value("pixel size", &value);
  {
    vh.scn.xypixsz[0] = value.fl[0];
    vh.scn.xypixsz[1] = value.fl[0];
    vh.scn.xypixsz_valid = 1;
  }

  read_imatron_value("slice space", &value);
  slice_spacing = value.fl[0];

  {
    if (tdim > 1)
    {
      vh.scn.loc_of_subscenes = (float *)malloc((1 + zdim) * vh.scn.num_of_subscenes[0] * sizeof(float));
/* time space */
      read_imatron_value("time space", &value);
      for (i = 0; i < vh.scn.num_of_subscenes[0]; i++)
        vh.scn.loc_of_subscenes[i] = i*value.fl[0];
/* slice location */
      read_imatron_value("slice location", &value);
      for (j = 0; j < vh.scn.num_of_subscenes[0]; j++)
        for (k = 0; k < vh.scn.num_of_subscenes[1 + j]; k++, i++)
          vh.scn.loc_of_subscenes[i] = value.fl[0]+k*slice_spacing;
    }
    else
    {
      vh.scn.loc_of_subscenes = (float *)malloc(vh.scn.num_of_subscenes[0] * sizeof(float));
/* slice location */
      read_imatron_value("slice location", &value);
      for (i = 0; i < vh.scn.num_of_subscenes[0]; i++)
        vh.scn.loc_of_subscenes[i] = value.fl[0]+i*slice_spacing;
    }
    vh.scn.loc_of_subscenes_valid = 1;
  }

/* */
  {
    buffer[0] = '\0';
    vh.scn.description = strdup(buffer);
    vh.scn.description_valid = 1;
  }
}

/*****************************************************************************
 * FUNCTION: fix_min_max_values_in_header
 * DESCRIPTION: sets the fields of vh containing the min and max density
 *    values within the dataset
 * PARAMETERS: none
 * SIDE EFFECTS: the fields of vh containing the min and max values will
 *    be set
 * ENTRY CONDITIONS: vh must be prepared already by the call to
 *    setup_viewnix_header. min_dens and max_dens should contain the
 *    real min and max values within the dataset
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
fix_min_max_values_in_header()
{
  int i;

  {
    vh.scn.smallest_density_value = (float *)malloc(vh.scn.num_of_density_values * sizeof(float));
    for (i = 0; i < vh.scn.num_of_density_values; i++)
      vh.scn.smallest_density_value[i] = (float)min_dens;
    vh.scn.smallest_density_value_valid = 1;
  }

  {
    vh.scn.largest_density_value = (float *)malloc(vh.scn.num_of_density_values * sizeof(float));
    for (i = 0; i < vh.scn.num_of_density_values; i++)
      vh.scn.largest_density_value[i] = (float)max_dens;
    vh.scn.largest_density_value_valid = 1;
  }
}

/*****************************************************************************
 * FUNCTION: update_min_max
 * DESCRIPTION: Computes the real min and max values in the dataset.
 * PARAMETERS:
 *    buffer: pointer to the dataset
 *    buflen: size of dataset
 *    bpp: number of bytes per pixel
 *    rep: pixel representation (0=unsigned integer, 1=signed integer)
 * SIDE EFFECTS: min_dens, max_dens will be set
 * ENTRY CONDITIONS: buffer must be allocated of a size at least buflen.
 *    The elements containing the pixel values are assumed to be in
 *    the native byte order.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
update_min_max(buffer, buflen, bpp, rep)
char *buffer;
unsigned int buflen;
unsigned short bpp;
unsigned short rep;
{
  char *b;
  unsigned short j;
  unsigned char uc;
  signed char sc;
  unsigned short us;
  signed short ss;

  if (bpp > 1)
  {
    switch (rep)
    {
      case 0:
      {
        for (j = 0, b = buffer; j < buflen; j+=bpp, b+=bpp)
        {
          memcpy(&us, b, bpp);
          if (us < min_dens)
            min_dens = us;
          else if (us > max_dens)
            max_dens = us;
        }
        break;
      }
      case 1:
      {
        for (j = 0, b = buffer; j < buflen; j+=bpp, b+=bpp)
        {
          memcpy(&ss, b, bpp);
          if (ss < min_dens)
            min_dens = ss;
          else if (ss > max_dens)
            max_dens = ss;
        }
        break;
      }
    }
  }
  else
  {
    switch (rep)
    {
      case 0:
      {
        for (j = 0, b = buffer; j < buflen; j+=bpp, b+=bpp)
        {
          memcpy(&uc, b, bpp);
          if (uc < min_dens)
            min_dens = uc;
          else if (uc > max_dens)
            max_dens = uc;
        }
        break;
      }
      case 1:
      {
        for (j = 0, b = buffer; j < buflen; j+=bpp, b+=bpp)
        {
          memcpy(&sc, b, bpp);
          if (sc < min_dens)
            min_dens = sc;
          else if (sc > max_dens)
            max_dens = sc;
        }
        break;
      }
    }
  }
}

/*****************************************************************************
 * FUNCTION: get_min_max_values
 * DESCRIPTION: Determines the min and max density values within the
 *    dataset by reading through the file and examining all pixels.
 * PARAMETERS: none
 * SIDE EFFECTS: the content of buffer will be changed
 * ENTRY CONDITIONS: the input file (inf) must be opened for reading.
 *    values_end must be set (by call to read_imatron_header), ydim, zdim,
 *    bpp must be set (by call to setup_viewnix_header, buflen must be set
 *    and buffer allocated at least of size buflen.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
get_min_max_values()
{
  unsigned short i, k;

  fseek(inf, values_end, SEEK_SET);
  {
    for (k = 0; k < zdim; k++)
    {
      for (i = 0; i < ydim; i++)
      {
        fread(buffer, 1, buflen, inf);
        if (bpp > 1)
        {
          makenativeendian(buffer, bpp, buflen / bpp);
          update_min_max(buffer, buflen, bpp, 0);
        }
        else
        {
          update_min_max(buffer, buflen, bpp, 0);
        }
      }
    }
  }
}

/*****************************************************************************
 * FUNCTION: copy_pixel_data
 * DESCRIPTION: Copies the pixel data from the IMATRON file to the 3DViewnix
 *    file with necessary byte swapping performed.
 * PARAMETERS: none
 * SIDE EFFECTS: the read the write positions of the two files will be
 *    changed by the fread and fwrite calls. The content of buffer will be
 *    changed.
 * ENTRY CONDITIONS: the input file (inf) must be opened for reading.
 *    values_end must be set (by call to read_imatron_header), ydim, zdim,
 *    bpp must be set (by call to setup_viewnix_header, buflen must be set
 *    and buffer allocated at least of size buflen. The output file vf
 *    must be open for writing and positioned at where the pixel data
 *    should be stored.
 * RETURN VALUE: none
 * EXIT CONDITIONS: undetermined if input conditions are not met
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
void
copy_pixel_data()
{
  unsigned short i, k;

  fseek(inf, values_end, SEEK_SET);
  for (k = 0; k < zdim; k++)
  {
    for (i = 0; i < ydim; i++)
    {
      fread(buffer, 1, buflen, inf);
      if (bpp > 1)
      {
        swapbyte(buffer, bpp, buflen / bpp);
        fwrite(buffer, 1, buflen, vf);
      }
      else
      {
        fwrite(buffer, 1, buflen, vf);
      }
    }
  }
}

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Implements the conversion program.
 * PARAMETERS: argc, argv
 * SIDE EFFECTS: none
 * ENTRY CONDITIONS: The commandline must have two arguments: the first
 *    specifying a filename and that file must contain data according
 *    to the file format used by the IMATRON scanner, the second specifying
 *    a filename for the output (must contain the .IM0 extension).
 * RETURN VALUE: Exits with 0 on normal completion.
 * EXIT CONDITIONS: If too few arguments are given on the commandline or
 *    the files cannot be opened or the data in the input file is in wrong
 *    format, exits with nonzero value and might produce system error messages.
 * HISTORY:
 *    Created: 6/3/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/
int
main(argc,argv)
  int argc;
  char *argv[];
{
  char group[6], elem[6];

  if (argc < 2)
  {
    printf("Usage: %s <input img file> <output IM0 file> \n", program_name);
    return 0;
  }
  ARGC = argc;
  ARGV = argv;
  min_dens = 65535UL;
  max_dens = 0UL;
  filename = ARGV[2];
  inf = fopen(ARGV[1], "rb");
  if (!inf)
  {
    printf("Error in opening input file %s\n", argv[1]);
    return 1;
  }
  read_imatron_header();
  setup_viewnix_header();
  buffer = (char *)malloc(buflen);
  get_min_max_values();
  fix_min_max_values_in_header();
  vf = fopen(filename, "wb");
  if (!vf)
  {
    printf("Error in opening output file %s\n", filename);
    return 1;
  }
  VWriteHeader(vf, &vh, group, elem);
  copy_pixel_data();
  VCloseData(vf);
  free(buffer);
  fclose(inf);
  free(header);
  return 0;
}

