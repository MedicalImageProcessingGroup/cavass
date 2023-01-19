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

#include "GradientImageComputation.h"

GradientImageComputation:: GradientImageComputation()  //Constructor
  :m_vh(NULL),
   m_GradientData(NULL)
{}

GradientImageComputation::~GradientImageComputation()   // Deconstructor
{
  if(m_GradientData)
    {
    delete [] m_GradientData;
    m_GradientData = NULL;
    }
}


void GradientImageComputation:: ComputeGradientImage()
{

  int pcol = m_vh->scn.xysize[0];
  int prow = m_vh->scn.xysize[1];
  int pslice = m_vh->scn.num_of_subscenes[0];

  int volume_size = pcol*prow*pslice;
  m_GradientData = new unsigned short[volume_size];
  if(m_GradientData==NULL)
    {
    std::cerr<<"Memory allocation of affinity image X is wrong "<<std::endl;
    std::abort();
    }

  /* with initialization 0*/
  for(int i = 0;i<volume_size;i++)
    m_GradientData[i] = (unsigned short) m_vh->scn.largest_density_value[0];
  
  int xx,yy,zz;
  int diff1,diff2,dx,dy,dz;
  for (int z=0;z<pslice;z++)
    for ( int y=0;y<prow;y++)
      for ( int x=0;x<pcol;x++)
	{
	xx = x-1;yy = y;zz = z;
	if(xx<0) xx = x;
	diff1 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	xx = x+1;yy = y;zz = z;
	if(xx>=pcol) xx = x;
	diff2 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	dx = (diff1>diff2?diff1:diff2);

	xx = x;yy = y-1;zz = z;
	if(yy<0) yy = y;
	diff1 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	xx = x;yy = y+1;zz = z;
	if(yy>=prow) yy = y;
	diff2 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	dy = (diff1>diff2?diff1:diff2);

	xx = x;yy = y;zz = z-1;
	if(zz<0) zz = z;
	diff1 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	xx = x;yy = y;zz = z+1;
	if(zz>=pslice) zz = z;
	diff2 = abs(m_InData[z*prow*pcol+y*pcol+x] - m_InData[zz*prow*pcol+yy*pcol+xx]);
	dz = (diff1>diff2?diff1:diff2);

	m_GradientData[z*prow*pcol+y*pcol+x] = 4096 - static_cast<unsigned short>(sqrt(dx*dx+dy*dy+dz*dz)+0.5);
	}
}

void GradientImageComputation::Execute()
{
  ComputeGradientImage();
}
