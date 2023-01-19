/*
  Copyright 1993-2008 Medical Image Processing Group
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

#include <fstream>
#include <iostream>
#include <string>
#include <math.h>

extern "C"{
#include "Viewnix.h"
}

extern "C" int VReadHeader(FILE *fp ,ViewnixHeader *vh ,char group[5], char element[5]);
extern "C" int VSeekData(FILE *fp,long offset);
extern "C" int VGetHeaderLength(FILE *fp,int *hdrlen);

int main(int argc, char ** argv)
{
  if(argc<3)
    {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputFile<IM0>  outputFile<mha> " << std::endl;
    return -1;
    }

  static ViewnixHeader vh_in;
  char group[6],element[6];
  FILE* fp_in;
  int error_code,hdrlen;

  fp_in = fopen(argv[1], "rb");
  error_code = VReadHeader(fp_in, &vh_in, group, element);
  if(error_code !=0 && error_code !=106 && error_code !=107)
  {
      std::cout<<"Open IM0 file error!"<<std::endl;
      fclose(fp_in);
      return -1;
  }
  VSeekData(fp_in,0);
  error_code = VGetHeaderLength(fp_in,&hdrlen);
  fclose(fp_in);

  std::ofstream fp;
  fp.open(argv[2]); /* output file */

  fp << "ObjectType = Image" << std::endl;  

  int nDims = vh_in.scn.dimension;
  if (vh_in.scn.num_of_subscenes[0]==1)
      nDims = 2;
  fp << "NDims = " << nDims << std::endl;  


  fp << "DimSize =";
  fp << " " <<vh_in.scn.xysize[0]<< " "<<vh_in.scn.xysize[1];
  if(nDims>2)
      fp << " " <<vh_in.scn.num_of_subscenes[0];
  fp << std::endl;

  fp << "BinaryData = True"<< std::endl;
  fp << "ElementSpacing =";
  fp << " " <<vh_in.scn.xypixsz[0]<< " "<<vh_in.scn.xypixsz[1];
  if(nDims>2)
      fp << " " <<fabs(vh_in.scn.loc_of_subscenes[1] - vh_in.scn.loc_of_subscenes[0]);
  fp << std::endl;

  fp << "ElementByteOrderMSB = True"<< std::endl;

  if(vh_in.scn.num_of_bits==8)
      fp << "ElementType = MET_UCHAR" << std::endl;
  else if(vh_in.scn.num_of_bits==16)
      fp << "ElementType = MET_USHORT" << std::endl;

  fp << "HeaderSize = " << hdrlen << std::endl;

  fp << "ElementDataFile = " << argv[1] << std::endl;

  fp.close();

  return 0;
}

