/*
  Copyright 1993-2013, 2017 Medical Image Processing Group
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

#include <ctype.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <Viewnix.h>

int GetNextToken(FILE *fp, char str[2048]);
void Copy_File(FILE *in, FILE *out);

int main(int argc, char *argv[])
{
  char BS0_file[2048];
  wxString ext, command;
  FILE *infp,*outfp;

  if (argc!=4) {
    fprintf(stderr, "Usage: PLN_TO_BS1 <BS0_PLAN> <BS1_PLAN> bg_flag\n");
    exit(-1);
  }
  if (strcmp(argv[1],argv[2])==0) {
    fprintf(stderr, "Error: Same plan file specified as the input and output\n");
    exit(-1);
  }

  infp=fopen(argv[1],"r");
  if (infp==NULL) {
    fprintf(stderr, "Error: Could not open input file\n");
    exit(-1);
  }
  outfp=fopen(argv[2],"w");
  if (outfp==NULL) {
    fprintf(stderr, "Error: Could not open input file\n");
    exit(-1);
  }
  
  if (GetNextToken(infp,BS0_file)!=0) {
    fprintf(stderr, "Error: Input file format error\n");
    fclose(infp);
    fclose(outfp);
    exit(-1);
  }


  wxFileName in_file_name = wxString(BS0_file);
  ext = in_file_name.GetExt();
  if (ext.CmpNoCase("BS0")) {
    fprintf(stderr, "Error: This is not a BS0 plan file\n");
    exit(-1);
  }

  wxFileName BS1_file = wxString(argv[2]);
  ext = BS1_file.GetExt();
  if (ext.CmpNoCase("PLN")) {
    fprintf(stderr, "Error: %s does not have the PLN extension\n", argv[2]);
    exit(-1);
  }
  BS1_file.SetExt("BS1");
  fprintf(outfp,"%s", (const char *)BS1_file.GetFullName().c_str());
  
  Copy_File(infp,outfp);
  fclose(infp);
  fclose(outfp);

  wxFileName in_plan_name= wxString(argv[1]);
  in_file_name.SetPath(in_plan_name.GetPath());
  wxString in_file_string=in_file_name.GetFullPath();
  if (!wxFile::Exists(in_file_string))
  {
    in_file_name = wxString(BS0_file);
    in_file_string = in_file_name.GetFullPath();
  }
  if (!wxFile::Exists(in_file_string))
  {
    fprintf(stderr, "%s does not exist\n", BS0_file);
	exit(-1);
  }
  command = wxString("BS0_TO_BS1 \"")+in_file_string+"\" \""+
    BS1_file.GetFullPath()+"\" 0";
  if (system((const char *)command.c_str())!=0) {
    fprintf(stderr, "system function returns implementation-defined value.\n");
  }

  return(0);

}


int GetNextToken(FILE *fp, char str[2048])
{
  int c;
  
  
  while ((c=getc(fp))!=EOF && isspace((int)c)) ;
  if (c==EOF) return(-1);
  ungetc(c,fp);
  if (fscanf(fp,"%s",str)!=1)
    return(-1);
  printf("%s\n",str);
 return(0);
  
}

/* Modified: 4/24/98 int value not clobbered by casting to char by Dewey Odhner
 */
void Copy_File(FILE *in, FILE *out)
{
  int c;

  while ((c=getc(in))!=EOF)
    putc(c,out);
  
}







