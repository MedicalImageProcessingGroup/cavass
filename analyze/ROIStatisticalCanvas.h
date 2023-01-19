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

//======================================================================
/**
 * \file   ROIStatisticalCanvas.h
 * \brief  ROIStatisticalCanvas definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __ROIStatisticalCanvas_h
#define __ROIStatisticalCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"

typedef struct 
{
        short x, y ;   /* x, y coordinates of curve */
        char vertex ;  /* flag to indicate if this point is vertex or not */
} PointInfo ;

typedef struct ROIStatistical
{
   int      roi_num,slice,volume;
   char     roi_type[30];
   double   mean_density,
            min_density,
            max_density,
            stdev_density,
            min_gradient,
            max_gradient,
            mean_gradient,
            stdev_gradient,
            area;
   int      AorV;

   wxPoint  *pVertices;
   int      nVertices;
   struct   ROIStatistical *next;
} Stat;

typedef struct total{
   double   mean_density,
            min_density,
            max_density,
            stdev_density,
            min_gradient,
            max_gradient,
            mean_gradient,
            stdev_gradient,
            volume;
 } Total;
//class ROIStatisticalCanvas : public wxScrolledWindow {
/** \brief ROIStatisticalCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class ROIStatisticalCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z  
  bool           m_bROIStatisticalDone;
  int            m_nROIType;
  int            m_nGradType;
  int            mFileOrDataCount;
  int            m_ROIStatisticalGraphLeft;
  int            m_ROIStatisticalGraphTop;
  int            m_ROIStatisticalGraphRight;
  int            m_ROIStatisticalGraphBottom;
  wxListCtrl     *mListCtrl;

public:
    /////////////// for ROIStatistical Algorithm	 /////////
	MagImg *icon;
	MagImg *magimg;
	struct Z_POINT *icon_zpoint;
	struct Z_POINT *prev_point;
	struct Z_POINT *temp_point;
	struct Z_POINT *point;
	struct G_POINT *profile;
	int    *first_point;
	int    cur_vol;
	int    very_first_point;
	unsigned short no_of_icon_points;
	unsigned int plot_points;
	char   process[20];
	int    FIRST_POINT;	
	short   *graph_data;
	MagBox  graph_box;
	MagBox  measure_dist;
	MagBox  first;
	MagBox  second;
	int     SCENE_NUM;
	int     VOLUME_NUM[2];
	int     SLICE_NUM[2];
	int     num_of_scenes;
	int     num_mag_images;	
	double  tot_dist;
	int     maxValue;
	int     minValue;
	double  min_dist;
	int     two_lines;
	int     firstMeasureX;
	int     secondMeasureX;	
	unsigned short DONE_DISPLAY;
	unsigned short DONE_SELECT;
	unsigned short DISPLAY_MODE;

	int     DrawImgLine(const wxPoint  pos);	
	void    UpdateIconLine(struct Z_POINT *cur_point, struct Z_POINT *cur_point1);
	int     CheckWindowPointIndex(struct Z_POINT *c_point, MagImg * c_win);
	double GetScaleUnits(char *gsc, short g_int);
	int VComputeLine ( int x1, int y1, int x2, int y2, wxPoint** points, int* npoints );
	int GetValue(struct Z_POINT *gvpoint);	
	double FindDistance(struct G_POINT *gp1, struct G_POINT *gp2,float z_diff);
	int CheckPointsIndex(struct Z_POINT *c_point, struct Z_POINT *c_point1);	
	
	void RemoveProfileInfo();
	int  MeasureWidth(const wxPoint  pos);
	void DrawROIStatistical();
	int  DrawAllLine();
	int  ChangeROIStatistical();
	int  InitROIStatistical();
	int  SaveProfile ( unsigned char* cFilename );

	PointInfo *tpoints ;
	int tnpoints;
	int max_points;
	wxPoint* points;
    int npoints; 
	wxPoint* vertices;
	int nvertices;
	Stat *RoiStats;
	Total *TotStats;
	int  RoiSize;
	int  RoiDimension;
	int  num_of_roistats;
	int  start;
	bool m_bMouseMove;
	wxPoint m_MousePos;
	bool m_bTypeFreeStart;
	int   m_nROIDownL;
	int m_nROIDownR;
	int m_nROIDownT;
	int m_nROIDownB;

	int m_nROIUpL;
	int m_nROIUpR;
	int m_nROIUpT;
	int m_nROIUpB;


	int v_close_curve (PointInfo** tpoints, int tnpoints, int max_points, wxPoint** points,
                       int* npoints, wxPoint** vertices, int* nvertices);
	int V_segment_intersection ( PointInfo* line1, PointInfo* line2, PointInfo** where1,  PointInfo** where2 );
	int v_draw_line ( int curx, int cury, PointInfo** points, int* npoints, int* max_points );
	int ComputeStats();
	int GetPrevRoistats();
	void GetStandardRoi(const wxPoint  pos);
	void SaveStats(unsigned char *OutStats );
	void DisplayStats(int x, int y);
	void DisplayRoiStatistics();
	int  DrawRoiVertices();


	/////////////// for ROIStatistical Algorithm	 /////////


	SliceData*        m_sliceIn;
	SliceData*        m_sliceOut;

    wxImage*          m_images[2];           ///< images for displayed slices
    wxBitmap*         m_bitmaps[2];          ///< bitmaps for displayed slices
    int               m_tx, m_ty;         ///< translation for all images
protected:
  //static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    bool             m_overlay;          ///< toggle display overlay
	bool             m_bLayout; 
    int              m_rows, m_cols;
    int              lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    ROIStatisticalCanvas ( void );
    ROIStatisticalCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~ROIStatisticalCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    /** \brief free any allocated images (of type wxImage) and/or bitmaps (of type wxBitmap) */
    void freeImagesAndBitmaps ( void ) 
	{
		  /* delete m_images */

		if (m_images[0]!=NULL)
		{
			m_images[0]->Destroy();
		}
		if (m_images[1]!=NULL)
		{
			m_images[1]->Destroy();		
		}
		// delete m_bitmaps 
		if (m_bitmaps[0]!=NULL)
		{
		delete m_bitmaps[0];
		m_bitmaps[0]=NULL;
		}
		if (m_bitmaps[1]!=NULL)
		{
		delete m_bitmaps[1];
		m_bitmaps[1]=NULL;
		}

    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void release ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
  void loadData ( char* name,
		  const int xSize, const int ySize, const int zSize,
		  const double xSpacing, const double ySpacing, const double zSpacing,
		  const int* const data, const ViewnixHeader* const vh=NULL,
		  const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void loadFile ( const char* const fn );
  void initLUT ( const int which );
  void reload ( void );
  void mapWindowToData ( int wx, int wy, int& x, int& y, int& z );
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void OnChar       ( wxKeyEvent&   e );
  void OnMouseMove  ( wxMouseEvent& e );
  void OnLeftDown   ( wxMouseEvent& e );
  void OnLeftUp     ( wxMouseEvent& e );
  void OnMiddleDown ( wxMouseEvent& e );
  void OnMiddleUp   ( wxMouseEvent& e );
  void OnRightDown  ( wxMouseEvent& e );
  void OnRightUp    ( wxMouseEvent& e );
  void OnPaint      ( wxPaintEvent& e );
  void paint        ( wxDC* dc );
  void OnSize       ( wxSizeEvent&  e );
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool   isLoaded    ( const int which ) const;
  
  bool   getOverlay  (void) const;
  int    getCenter   ( const int which ) const;
  bool   getInvert   ( const int which ) const;
  int    getMax      ( const int which ) const;
  int    getMin      ( const int which ) const;
  int    getNoSlices ( const int which ) const;    //number of slices in entire data set
  int    getSliceNo  ( const int which ) const;    //first displayed slice
  int    getWidth    ( const int which ) const;
  double getScale    ( void ) const;
  bool   getROIStatisticalDone(void) const;
 
  int betROIType() const
  {
    return m_nROIType;
  }
  int betGradType() const
  {
    return m_nGradType;
  }
  bool getLayout() const
  {
	  return m_bLayout;
  }

  void   setOverlay  ( const bool overlay );
  void   setCenter   ( const int which, const int    center  );
  void   setInvert   ( const int which, const bool   invert  );
  void   setSliceNo  ( const int which, const int    sliceNo );
  void   setWidth    ( const int which, const int    width   );
  void   setScale    ( const double scale );
  void   setROIStatisticalDone(bool done)
  {
    m_bROIStatisticalDone = done;
  };
 
  
  void setROIType(int nROIType)
  {
    m_nROIType = nROIType;

	switch(m_nROIType)
	{
		 case 1 : RoiSize=5;
			 RoiDimension=2;
			 break;
		 case 2 : RoiSize=5;
			 RoiDimension=3;
			 break;
		 case 3 : RoiSize=7;
			 RoiDimension=2;
			 break;
		 case 4 : RoiSize=7;
			 RoiDimension=3;
			 break;
		 case 5 : RoiSize=9;
			 RoiDimension=2;
			 break;
		 case 6 : RoiSize=9;
			 RoiDimension=3;
			 break;
	}
  }
  void setGradType(int nGradType)
  {
    m_nGradType = nGradType;
  }
  void setLayout(const bool overlay)
  {
	  m_bLayout = overlay;
  }
  

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunROIStatistical();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

    DECLARE_DYNAMIC_CLASS(ROIStatisticalCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
