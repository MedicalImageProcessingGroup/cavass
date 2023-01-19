/*
  Copyright 1993-2016 Medical Image Processing Group
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

//======================================================================
/**
 * \file:  ROIStatisticalCanvas.cpp
 * \brief  ROIStatisticalCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "ROIStatisticalCanvas.h"
//#include  "ROIStatisticalAlgorithm.h"

using namespace std;
//----------------------------------------------------------------------
ROIStatisticalCanvas::ROIStatisticalCanvas ( void )  {  puts("ROIStatisticalCanvas()");  }
//----------------------------------------------------------------------
ROIStatisticalCanvas::ROIStatisticalCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
    m_scale          = 1.0;
    m_overlay        = true;
	m_bLayout        = false;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 40;
	m_nROIType = 0;
    m_nGradType = 0;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
//	m_ROIStatisticalType = ROIStatistical_GRADIENT2D;
	m_bROIStatisticalDone = false;	

	FIRST_POINT = 1;	
	SCENE_NUM = 1;
	num_of_scenes = 1;	
	two_lines = 0;

	tpoints = NULL;
	points = NULL;    
	vertices = NULL;
	RoiStats = 0;
	TotStats = 0;
	m_bMouseMove = false;
	m_bTypeFreeStart = true;

	DONE_DISPLAY = 0;
    DONE_SELECT = 0;
    DISPLAY_MODE = 0;
}
//----------------------------------------------------------------------
ROIStatisticalCanvas::~ROIStatisticalCanvas ( void ) {
    cout << "ROIStatisticalCanvas::~ROIStatisticalCanvas" << endl;
    wxLogMessage( "ROIStatisticalCanvas::~ROIStatisticalCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
	
	if( m_sliceIn != NULL )
	{
		delete m_sliceIn;
		m_sliceIn = NULL;
	}
	if( m_sliceOut != NULL )
	{
		delete m_sliceOut;
		m_sliceOut = NULL;
	}
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::release ( void ) {
#if 0
    m_scale   = 1.0;
    freeImagesAndBitmaps();
    m_center = m_width = 0;
    if (m_lut!=NULL)    {  free(m_lut);      m_lut   = NULL;  }
    m_sliceNo = 0;
    if (m_fname!=NULL)  {  free(m_fname);    m_fname = NULL;  }
    if (m_data!=NULL)   {  free(m_data);     m_data  = NULL;  }
    m_min = m_max = 0;
    m_size = 0;
    m_bytesPerSlice = 0;
    m_tx = m_ty = 0;
    mInvert = false;
    lastX = lastY = -1;
#endif
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
/*
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    assert( mFileOrDataCount==0 || mFileOrDataCount==1 );
    CavassData*  cd = new CavassData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    if (mFileOrDataCount==0) {
        assert( mCavassData==NULL );
        mCavassData = cd;
    } else {
        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
        mCavassData->mNext = cd;
    }
    ++mFileOrDataCount;

    if (mFileOrDataCount==2) {
        CavassData&  A = *mCavassData;
        CavassData&  B = *(mCavassData->mNext);

        A.mR = 1.0;  A.mG = 0.5;  A.mB = 0.0;
        B.mR = 0.0;  B.mG = 0.5;  B.mB = 1.0;

        mOverallXSize = A.m_xSize;
        if (A.m_xSize*A.m_xSpacing/B.m_xSpacing > mOverallXSize)
            mOverallXSize = (int)(A.m_xSize*A.m_xSpacing/B.m_xSpacing + 0.5);
        if (B.m_xSize*B.m_xSpacing/A.m_xSpacing > mOverallXSize)
            mOverallXSize = (int)(B.m_xSize*B.m_xSpacing/A.m_xSpacing + 0.5);
        mOverallYSize = A.m_ySize;
        if (A.m_ySize*A.m_ySpacing/B.m_ySpacing > mOverallYSize)
            mOverallYSize = (int)(A.m_ySize*A.m_ySpacing/B.m_ySpacing + 0.5);
        if (B.m_ySize*B.m_ySpacing/A.m_ySpacing > mOverallYSize)
            mOverallYSize = (int)(B.m_ySize*B.m_ySpacing/A.m_ySpacing + 0.5);
        mOverallZSize = A.m_zSize;
        if (B.m_zSize>mOverallZSize)    mOverallZSize = B.m_zSize;

        int  w, h;
        GetSize( &w, &h );
        m_cols = (int)(w / (mOverallXSize * m_scale));
        m_rows = (int)(h / (mOverallYSize * m_scale));
        if (m_cols<1)  m_cols=1;
        if (m_rows<1)  m_rows=1;
        reload();
    }
    SetCursor( *wxSTANDARD_CURSOR );
*/
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::loadFile ( const char* const fn ) 
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }    
	
//	assert( mFileOrDataCount>=0 && mFileOrDataCount<=1 );
    CavassData*  cd = new CavassData( fn, true );
	if( cd == NULL ) //|| cd != NULL && cd->m_data == NULL )
		return; // ERR_CLASSCONSTRUCTION;
   
	SliceData*  sd = new SliceData( fn );	
	if( sd == NULL ) //|| cd != NULL && cd->m_data == NULL )
		return; // ERR_CLASSCONSTRUCTION;
    if (!sd->mIsCavassFile || !sd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return; // ERR_LOADCAVASSFILE;
	}

    if (mFileOrDataCount==0) 
	{
        assert( mCavassData==NULL );
        mCavassData = cd;

        CavassData&  A = *mCavassData;
        A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;        
	    m_cols  = m_rows = 1;
		
		m_sliceIn = sd;
		m_sliceIn->m_sliceNo = -1;
	} 
	else 
	{
		if( mFileOrDataCount >= 2 )
		{
			if (mCavassData->mNext!=NULL)
			{
				CavassData*  tmp = mCavassData->mNext;				
				delete tmp;
				tmp = NULL;
				mCavassData->mNext = NULL;
			}
		}

        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
        mCavassData->mNext = cd;
		freeImagesAndBitmaps();
		m_cols = 2;

		m_sliceOut = sd;
		m_sliceOut->m_sliceNo = -1;		
    }
    ++mFileOrDataCount;   

	if (isLoaded(0)) 
	{
		int  w, h;
		mOverallXSize = mCavassData->m_xSize;
		GetSize( &w, &h );
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 2));     

		m_ROIStatisticalGraphLeft = w/2+60;
		m_ROIStatisticalGraphTop = 80;
		m_ROIStatisticalGraphRight = w - 60;
		m_ROIStatisticalGraphBottom = h/2+80;
	}       

    reload();    
    SetCursor( *wxSTANDARD_CURSOR );

	InitROIStatistical();
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	const CavassData&  A = *mCavassData;
	const CavassData&  B = *(mCavassData->mNext);

	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}

	if (A.m_size==1) 
	{
		unsigned char*   cData = NULL;
		if (which==0) 
		{		  
			cData = (unsigned char*)m_sliceIn->getSlice(A.m_sliceNo); //A.m_data;
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )   // It don't need to initial LUT when Adjust the GreyMap
			{
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}
		}
		else if (which==1)     
		{
			cData = (unsigned char*)m_sliceOut->getSlice(B.m_sliceNo); //B.m_data;
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}
		}

		if( cData == NULL )
			return;

		for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
		{
			if (which==0)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[cData[j] - m_sliceIn->m_min];
			else if (which==1)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[cData[j] - m_sliceOut->m_min];
		}

		cData = NULL;		
	} 
	else if (A.m_size==2) 
	{
		unsigned short* sData = NULL;
		if(which==0)
		{
			sData = (unsigned short*)m_sliceIn->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )
			{
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}
		}
		else if(which==1)
		{
			sData = (unsigned short*)m_sliceOut->getSlice(B.m_sliceNo); //(B.m_data);	  
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{	   
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}
		}

		if( sData == NULL )
			return;

		for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
		{
			if(which==0)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[sData[j] - m_sliceIn->m_min];
			else if(which==1)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[sData[j] - m_sliceOut->m_min];		  
		}
			
		sData = NULL;		
	} 
	else if (mCavassData->m_size==4) 
	{
		int* iData = NULL;
		if(which==0)
		{
			iData = (int*)m_sliceIn->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )
			{
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}
		}
		else if(which==1)
		{	 
			iData = (int*)m_sliceOut->getSlice(B.m_sliceNo); //(B.m_data);	  
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}
		}

		if( iData == NULL )
			return;

		for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
		{
			if(which==0)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[iData[j] - m_sliceIn->m_min];
			else if(which==1)
				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[iData[j] - m_sliceOut->m_min];
		}

		iData = NULL;
	}

	if(which==0)
	{
		if(m_images[0]!=NULL)
			m_images[0]->Destroy();
		m_images[0] = new wxImage( A.m_xSize, A.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[0]->Rescale( (int)(ceil(m_scale*A.m_xSize)),
							  (int)(ceil(m_scale*A.m_ySize)) );
		}
		m_bitmaps[0] = new wxBitmap( *m_images[0] );
	}
	else if(which==1)
	{
		if(m_images[1]!=NULL)
			m_images[1]->Destroy();

		m_images[1] = new wxImage( B.m_xSize, B.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[1]->Rescale( (int)(ceil(m_scale*B.m_xSize)),
							  (int)(ceil(m_scale*B.m_ySize)) );
		}
		m_bitmaps[1] = new wxBitmap( *m_images[1] );
	}
	//delete [] slice;
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::reload ( void ) 
{
  //if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

	//if(m_bROIStatisticalDone) /* update ROIStatisticaling */		
	//	CreateDisplayImage(1);

    Refresh();
}

//----------------------------------------------------------------------
void ROIStatisticalCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
#if 0
    //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;
    const int  col = (int)((double)wx / (ceil(m_xSize*m_scale)+sSpacing));
    const int  row = (int)((double)wy / (ceil(m_ySize*m_scale)+sSpacing));
    x = (int)( wx % ((int)(ceil(m_xSize*m_scale)+sSpacing)) / m_scale );
    y = (int)( wy % ((int)(ceil(m_ySize*m_scale)+sSpacing)) / m_scale );
    z = m_sliceNo + row*m_cols + col;
    
    //clamp the values to acceptable values
    if (x<0)  x=0;
    else if (x>=m_xSize)  x=m_xSize-1;
    if (y<0)  y=0;
    else if (y>=m_ySize)  y=m_ySize-1;
    if (z<0)  z=0;
    else if (z>=m_zSize)  z=m_zSize-1;
#endif
}

int ROIStatisticalCanvas::ChangeROIStatistical()
{	
	MagImg   *ptr;
	int   i,j;	

	int dimension = mCavassData->m_vh.scn.dimension;

	//INTERPOLATE_FLAG=FALSE;	
	num_mag_images=0;

	/* ---------------- get sizes ------------------------------------*/
	int SAME_UNIT=0; //check_unit_types(scene,num_of_scenes);	
	int  w, h;		
	GetSize( &w, &h );

	float SF = 1/m_scale; //mCavassData->m_xSize/IS; //max_width/IS;

	for(i=0,ptr=icon;i<num_of_scenes;ptr=ptr->next,i++) 
	{
		ptr->scene_num=i;		
		//ptr->slice_index=(short *)calloc(max_dim,sizeof(short));
		for(j=2;j<dimension;j++) 
			ptr->slice_index[j]=mCavassData->m_sliceNo; //m_zSize;
		ptr->slice_index[0]=0; /* point to the row table */
		ptr->slice_index[1]=1; /* Give motion direction */
		ptr->tbl.oldd[0]=mCavassData->m_xSize;
		ptr->tbl.oldd[1]=mCavassData->m_ySize;
		if (SAME_UNIT) 
		{
			ptr->tbl.newd[0]= (short)floor((mCavassData->m_xSize*mCavassData->m_size)/SF);  //*get_factor(&scene[i])
			ptr->tbl.newd[1]= (short)floor((mCavassData->m_ySize*mCavassData->m_size)/SF);  //*get_factor(&scene[i])
		}
		else 
		{
			ptr->tbl.newd[0]=(short)floor((mCavassData->m_xSize/SF));
			ptr->tbl.newd[1]=(short)floor((mCavassData->m_ySize/SF));
		}
		ptr->img_offset[0]=ptr->img_offset[1]=0;
		ptr->zoom_width=ptr->width=ptr->tbl.dim[0]=ptr->tbl.newd[0];  //ptr->tbl.newd[0];
		ptr->height=ptr->tbl.dim[1]=ptr->tbl.newd[1];  //ptr->tbl.newd[1];
		ptr->img_dim[0]=ptr->tbl.oldd[0];
		ptr->img_dim[1]=ptr->tbl.oldd[1];
		ptr->tbl.offset[0]=ptr->tbl.offset[1]=0;		

		int xloc= m_tx; //(i%NCOL) * (IS+2) ;		
		int yloc= m_ty; //rint(k * (ht+1) +1);		
		ptr->x=ptr->dx=xloc;
		ptr->y=ptr->dy=yloc;
		ptr->dx1=xloc + ptr->width; // -1
		ptr->dy1=yloc + ptr->height; // -1
		ptr->locked=FALSE;
	}

	return 1;
}

int ROIStatisticalCanvas::InitROIStatistical()
{
	/****************************************************************************/
	/*Init_Cycle would initialize the icon list and initialize the default vals */
	/****************************************************************************/
//	InitCycle();
	MagImg   *ptr;
	int   i,j;	

	int dimension = mCavassData->m_vh.scn.dimension;
	int   max_dim = dimension;

	//CURRENT_SCENE_NUM=0; //DEFAULT_SCENE_NUM;	
	//INTERPOLATE_FLAG=FALSE;	
	num_mag_images=0;

	/* ---------------- get sizes ------------------------------------*/
	int SAME_UNIT=0; //check_unit_types(scene,num_of_scenes);	
	int  w, h;		
	GetSize( &w, &h );

	float SF = 1/m_scale; //mCavassData->m_xSize/IS; //max_width/IS;

	icon=(MagImg *)calloc(1,sizeof(MagImg));
	if (icon==NULL) 
	{
		printf("Could not allocate space for cylce structures\n");
		return ERR_OUTOFMEMORY;
	}
	icon->next=NULL;
	for(ptr=icon,i=1; i<num_of_scenes;i++,ptr=ptr->next) 
	{
		ptr->next=(MagImg *)calloc(1,sizeof(MagImg));
		if (ptr->next==NULL) 
		{
			printf("Could not allocate space for cylce structures\n");
			return ERR_OUTOFMEMORY;
		}
		ptr->next->next=NULL;
	}

	//row_table=(short ***)calloc(num_of_scenes,sizeof(short **));
	//num_row_table=(short *)calloc(num_of_scenes,sizeof(short));

	for(i=0,ptr=icon;i<num_of_scenes;ptr=ptr->next,i++) 
	{
		ptr->scene_num=i;

		/*slices= scene[i].tree->size/scene[i].bytes_per_slice;
		row_table[i]=(short **)calloc(slices,sizeof(short *));
		for(j=0;j<slices;j++) 
			row_table[i][j]=(short *)calloc(scene[i].tree->dimension,sizeof(short));
		num_row_table[i]=BuildCycleRowTable(&scene[i],row_table[i],slices);
		if (num_row_table[i]!=slices) {
			printf("Could not build Row table for cycle\n");
			printf("%d out of %d was initialized\n",i,slices);
			if (num_row_table[i]==0) exit(-1);
		}*/

		ptr->slice_index=(short *)calloc(max_dim,sizeof(short));
		for(j=2;j<dimension;j++) 
			ptr->slice_index[j]=mCavassData->m_sliceNo; //m_zSize;
		ptr->slice_index[0]=0; /* point to the row table */
		ptr->slice_index[1]=1; /* Give motion direction */
		ptr->tbl.oldd[0]=mCavassData->m_xSize;
		ptr->tbl.oldd[1]=mCavassData->m_ySize;
		if (SAME_UNIT) 
		{
			ptr->tbl.newd[0]= (short)floor((mCavassData->m_xSize*mCavassData->m_size)/SF);  //*get_factor(&scene[i])
			ptr->tbl.newd[1]= (short)floor((mCavassData->m_ySize*mCavassData->m_size)/SF);  //*get_factor(&scene[i])
		}
		else 
		{
			ptr->tbl.newd[0]=(short)floor((mCavassData->m_xSize/SF));
			ptr->tbl.newd[1]=(short)floor((mCavassData->m_ySize/SF));
		}
		ptr->img_offset[0]=ptr->img_offset[1]=0;
		ptr->zoom_width=ptr->width=ptr->tbl.dim[0]=ptr->tbl.newd[0];  //ptr->tbl.newd[0];
		ptr->height=ptr->tbl.dim[1]=ptr->tbl.newd[1];  //ptr->tbl.newd[1];
		ptr->img_dim[0]=ptr->tbl.oldd[0];
		ptr->img_dim[1]=ptr->tbl.oldd[1];
		ptr->tbl.offset[0]=ptr->tbl.offset[1]=0;
		
		//int xloc= m_tx; //(i%NCOL) * (IS+2) ;
		//int k=i/NCOL;
		//int yloc= m_ty; //rint(k * (ht+1) +1);
		//xloc+= (IS+2 - ptr->tbl.newd[0])/2; /* center the image  */
		//yloc+= (ht+1 - ptr->tbl.newd[1])/2;
		int xloc= m_tx; //(i%NCOL) * (IS+2) ;		
		int yloc= m_ty; //rint(k * (ht+1) +1);		
		ptr->x=ptr->dx=xloc;
		ptr->y=ptr->dy=yloc;
		ptr->dx1=xloc + ptr->width; // -1
		ptr->dy1=yloc + ptr->height; // -1
		ptr->locked=FALSE;
	}

	/* adding the struct for storing points */

	if((icon_zpoint =(struct Z_POINT *) calloc(1, num_of_scenes*sizeof(struct Z_POINT)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
	else
	{
		for(i=0;i<num_of_scenes;i++)
		{
			icon_zpoint[i].next=NULL;
			if((icon_zpoint[i].index =(short *) calloc(1,dimension * sizeof(short)))==NULL)
				return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
		}
	}


	if((prev_point=(struct Z_POINT *) calloc(num_of_scenes,sizeof(struct Z_POINT)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
	else
	{
		for(i=0;i<num_of_scenes;i++)
		{
			prev_point[i].next=NULL;
			if((prev_point[i].index =(short *)calloc(dimension , sizeof(short)))==NULL)
				return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
		}
	}

	if(dimension > 3)
		i=mCavassData->m_vh.scn.num_of_subscenes[0];
	else
		i=1;

	if((first_point = (int *) calloc(1,i*sizeof(int)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
	else
	{
		for(j=0;j<i;j++)
			first_point[j]=0;
	}
	very_first_point = 0;

	no_of_icon_points=0;

	tpoints = (PointInfo *)malloc(1024*sizeof(PointInfo));
	if (tpoints == NULL)
		return (1);	

	max_points = 1024;	
	tnpoints = npoints = nvertices = 0 ;
	GetPrevRoistats();

    RoiSize=0;
    RoiDimension=2;
	num_of_roistats = 0;
	start = 1;
	mListCtrl = NULL;
		
	return 1;

}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void ROIStatisticalCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "ROIStatisticalCanvas::OnChar" << endl;
    wxLogMessage( "ROIStatisticalCanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
    //pass the event up to the parent frame
    //m_parent_frame->ProcessEvent( e );
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnMouseMove ( wxMouseEvent& e ) {
    //if (m_data==NULL)    return;
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
#if 0    
    //show the pixel value under the pointer
    int  x, y, z;
    mapWindowToData( dc.DeviceToLogicalX( pos.x ),
                     dc.DeviceToLogicalY( pos.y ), x, y, z );
    wxString  s;
    s.Printf( wxT("(%d,%d,%d)=%d -> %d"), x, y, z, getData(x,y,z),
              m_lut[getData(x,y,z)-m_min] );
    m_parent_frame->SetStatusText( s, 1 );
#endif

	if( m_bLayout ) //&& !m_bROIStatisticalDone )
	{
		//if we are NOT in any mode, then allow the user to move (translate)
		// the image (if a mouse button is down)
		if (e.LeftIsDown() || e.RightIsDown()) 
		{
			if (lastX==-1 || lastY==-1) {
				SetCursor( wxCursor(wxCURSOR_HAND) );
				lastX = wx;
				lastY = wy;
				return;
			}

			bool  changed=false;
			if (abs(wx-lastX)>2) {
				m_tx += (wx-lastX)/2;  // / 4 * 3;
				changed = true;
			} else if (abs(wx-lastX)>2) {
				m_tx -= (lastX-wx)/2;  // / 4 * 3;
				changed = true;
			}
			if (abs(wy-lastY)>2) {
				m_ty += (wy-lastY)/2;  // / 4 * 3;
				changed = true;
			} else if (abs(wy-lastY)>2) {
				m_ty -= (lastY-wy)/2;  // / 4 * 3;
				changed = true;
			}

			if (changed)  Refresh();
			lastX=wx;
			lastY=wy;

			ChangeROIStatistical();

			return;
		} 
		else 
		{
			if (lastX!=-1 || lastY!=-1) {
				SetCursor( *wxSTANDARD_CURSOR );
				lastX = lastY = -1;
			}
		}
	}
	else
	{
		if( !m_bROIStatisticalDone )
		{
			switch(m_nROIType)
			{			
			case 1:   // 5x5
			case 2:   // 5x5
			case 3:   // 5x5
			case 4:   // 5x5
			case 5:   // 5x5
			case 6:   // 5x5
				{
					m_bMouseMove = true;
					m_MousePos = pos;
					Refresh();					
				}
				break;
			default:
				break;
			}
		}
	}
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events

	if( m_nROIType == 0 && m_bTypeFreeStart == true )
	{
		v_close_curve ( &tpoints, tnpoints, max_points, &points, &npoints, &vertices, &nvertices);
		tnpoints = 0;

	    RoiDimension=2;      
		ComputeStats();
		m_bTypeFreeStart = false;
	}
	else
	{		
		if( m_bROIStatisticalDone )
		{
			if (mListCtrl)
			delete mListCtrl;
			mListCtrl = NULL;
		}
		m_bROIStatisticalDone = false;

		//if (isLoaded(0)) 
		//{
		//	mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
		//	reload();
		//}

		m_parent_frame->SetStatusText("Done.", 4);	

		Refresh();
	}
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnRightUp ( wxMouseEvent& e ) 
{
    //if (isLoaded(0)) {
    //    mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
    //    reload();
    //}
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
 //   SetFocus();  //to regain/recapture keypress events
  /*  if (isLoaded(1)) 
	{
        mCavassData->mNext->mDisplay = false;  //!mCavassData->mNext->mDisplay;
        reload();
    }*/

	if( !m_bROIStatisticalDone )
		DisplayRoiStatistics();
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
 /*   if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    } */
}

//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnLeftDown ( wxMouseEvent& e ) 
{
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
	wxClientDC  dc(this);
	PrepareDC(dc);
 
	if( m_bLayout )  //
	{
		const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
		const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
		lastX = wx;
		lastY = wy;
		SetCursor( wxCursor(wxCURSOR_HAND) );
	}
	else
	{

		if( !m_bROIStatisticalDone )
		{
			switch(m_nROIType)
			{
			case 0:   // free
				DrawImgLine( pos );
				m_bTypeFreeStart = true;
				m_parent_frame->SetStatusText("Close", 4);		
				break;
			case 1:   // 5x5
			case 2:   // 5x5
			case 3:   // 5x5
			case 4:   // 5x5
			case 5:   // 5x5
			case 6:   // 5x5
				GetStandardRoi(pos);
				break;
			default:
				break;
			}
		}
		else
		{	
		}	
	}

}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnPaint ( wxPaintEvent& e ) 
{
    wxMemoryDC  m;
    int         w, h;
    GetSize( &w, &h );
    wxBitmap    bitmap(w, h);
    m.SelectObject( bitmap );
    if (Preferences::getCustomAppearance())
#if wxCHECK_VERSION(2, 9, 0)
        m.SetBrush( wxBrush(wxColour(DkBlue), wxBRUSHSTYLE_SOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxBRUSHSTYLE_SOLID) );
#else
        m.SetBrush( wxBrush(wxColour(DkBlue), wxSOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxSOLID) );
#endif
    m.DrawRectangle( 0, 0, w, h );
    
    paint( &m );

    wxPaintDC  dc(this);
    PrepareDC(dc);
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	if(m_bitmaps[0]!=NULL)
	{
		if (m_bitmaps[0]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[0], m_tx, m_ty );
			if(m_overlay)
			{
				const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
				dc->DrawText( s, m_tx, m_ty );
			}
		}		

//		DrawRoiVertices();
		Stat *tmpstat = NULL;
		int index1;
		tmpstat = RoiStats;	
		while(tmpstat!= NULL) 
		{
			if( tmpstat->nVertices == 0 )
				break;

			for(index1=0; index1 < tmpstat->nVertices-1; index1++)
			{
				dc->DrawLine(tmpstat->pVertices[index1].x, tmpstat->pVertices[index1].y, tmpstat->pVertices[index1+1].x, tmpstat->pVertices[index1+1].y );
			}

			dc->DrawLine(tmpstat->pVertices[index1].x, tmpstat->pVertices[index1].y, tmpstat->pVertices[0].x, tmpstat->pVertices[0].y );


			tmpstat=tmpstat->next;
		}


	/*	if( m_bROIStatisticalDone )
		{
			DisplayRoiStatistics();
		} */

		if( m_bMouseMove )
		{
			int nRoiWin = (int)(RoiSize*(icon->width*1.0/icon->img_dim[0]));
			if((m_MousePos.x<=icon->dx1)&&(m_MousePos.y<=icon->dy1) && (m_MousePos.x>icon->x)&&(m_MousePos.y>icon->y)  )
			{
				dc->DrawRectangle(m_MousePos.x-nRoiWin/2, m_MousePos.y-nRoiWin/2, nRoiWin, nRoiWin);
			}
			m_bMouseMove = false;
		}
	
	}
	else if (m_backgroundLoaded) 
	{		
		const int  bmW = m_backgroundBitmap.GetWidth();
		const int  bmH = m_backgroundBitmap.GetHeight();
		dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
	}
}


//----------------------------------------------------------------------
void ROIStatisticalCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool ROIStatisticalCanvas::isLoaded ( const int which ) const {
    if (which==0) {
        if (mCavassData==NULL)    return false;
        const CavassData&  cd = *mCavassData;
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else if (which==1) {
        if (mCavassData==NULL || mCavassData->mNext==NULL)    return false;
        const CavassData&  cd = *(mCavassData->mNext);
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else {
        assert( 0 );
    }
    return false;
}

int ROIStatisticalCanvas::SaveProfile ( unsigned char* cFilename )
{
	SaveStats(cFilename );

	return 1;
}

bool ROIStatisticalCanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int ROIStatisticalCanvas::getCenter ( const int which ) const 
{
    if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_center;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_center;
    } 
	else 
	{
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int ROIStatisticalCanvas::getMax ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_max;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_max;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }  
}
//----------------------------------------------------------------------
int ROIStatisticalCanvas::getMin ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_min;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_min;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }    
}
//----------------------------------------------------------------------
//number of slices in entire data set
int ROIStatisticalCanvas::getNoSlices ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_zSize;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_zSize;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}

bool ROIStatisticalCanvas::getROIStatisticalDone (void) const
{
	return m_bROIStatisticalDone;
}
//----------------------------------------------------------------------
double ROIStatisticalCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int ROIStatisticalCanvas::getSliceNo ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_sliceNo;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int ROIStatisticalCanvas::getWidth ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_width;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_width;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }       
}
//----------------------------------------------------------------------
bool ROIStatisticalCanvas::getInvert ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->mInvert;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->mInvert;
    } 
	else 
	{
        assert( 0 );
		return 0;
    }          
}
void ROIStatisticalCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::setCenter ( const int which, const int center ) 
{
    if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->m_center = center;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->m_center = center;
    } 
	else 
	{
        assert( 0 );
    }
}

//----------------------------------------------------------------------
void ROIStatisticalCanvas::setInvert ( const int which, const bool invert ) 
{
	if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->mInvert = invert;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->mInvert = invert;
    } 
	else 
	{
        assert( 0 );
    }  
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::setScale   ( const double scale )  {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    m_scale = scale;
	//mCavassData->m_scale = scale;
    int  w, h;
    GetSize( &w, &h );
    m_cols  = (int)(w / (mOverallXSize * m_scale));
    m_rows  = (int)(h / (mOverallYSize * m_scale));
    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();

	ChangeROIStatistical();
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;
		for (int j=2; j<mCavassData->m_vh.scn.dimension; j++)
			icon->slice_index[j]=mCavassData->m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
		for (int j=2; j<mCavassData->mNext->m_vh.scn.dimension; j++)
			icon->next->slice_index[j]=mCavassData->mNext->m_sliceNo;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void ROIStatisticalCanvas::setWidth ( const int which, const int width ) 
{
	if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->m_width = width;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->m_width = width;
    } 
	else 
	{
        assert( 0 );
    }     
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ROIStatisticalCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ROIStatisticalCanvas, wxPanel )
    EVT_PAINT(            ROIStatisticalCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           ROIStatisticalCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        ROIStatisticalCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ROIStatisticalCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ROIStatisticalCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ROIStatisticalCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       ROIStatisticalCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ROIStatisticalCanvas::OnRightUp      )
    EVT_CHAR(             ROIStatisticalCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
