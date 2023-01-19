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
 * \file   DensityCanvas.h
 * \brief  DensityCanvas definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __DensityCanvas_h
#define __DensityCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"
//#include  "ScaleComputation2D.h"
 
/* Structures for building interpolation tables  */
typedef struct TABLE_INFO {
  short  oldd[2],     /* old dimensions  */
         newd[2];     /* new dimensions  */
  short  offset[2],  /* Top left pos of image to be displayed */
         dim[2];     /* dimensions of the part that is displayed */
  short *index_table[2];  /* for each pix in new index into old (lower value)*/
  unsigned int   *dist_table[2];   /* units = old_pix_w/h *old_w/h */
} Table_Info;
/* structures used in layout and roi */
typedef struct MAG_IMG_STRUCT 
{
	int scene_num;
	short *slice_index;
	int x,y,			/* screen loc of zoomed roi */
		width,height;		/* size of roi */
	int img_offset[2],		/* offset of roi in img. */
		img_dim[2];		/* diemnsions of roi in img. */
	int dx,dy,			/* upper left corner of displayed area */
		dx1,dy1;			/* lower right corner of displayed ares */
	int zoom_width ;		/* full zoomed_width of the whole image */
	Table_Info tbl;		/* table for interpolation */
	unsigned char *data;		/* data pointer */
	char locked;			/* flag if the slice is changing */
	// Window win;			/* image window */
	struct MAG_IMG_STRUCT *next;
} MagImg;

/* temp structure used for drawing bounding boxes */
typedef struct MAG_BOX 
{
	//  XImage *TOP,*BOT,*LEFT,*RIGHT;
	short x,y,x1,y1;
	GC gc;
	MagImg *img;
	unsigned top:1,bot:1,left:1,right:1;
} MagBox;

typedef	struct Z_POINT{
	int      num;
	short *index;
	int      x,y;
	struct Z_POINT *next;
} zpoint;

typedef struct G_POINT{
	int     num;
	int	    x,y;
	int	    value;
	int     distance;
	struct  G_POINT *next;
}gpoint;
//class DensityCanvas : public wxScrolledWindow {
/** \brief DensityCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class DensityCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z  
  bool           m_bDensityDone;
  int            mFileOrDataCount;
  int            m_DensityGraphLeft;
  int            m_DensityGraphTop;
  int            m_DensityGraphRight;
  int            m_DensityGraphBottom;

public:
    /////////////// for Density Algorithm	 /////////
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
	void    UpdateIconPoint(struct Z_POINT *cur_point);
	void    UpdateIconLine(struct Z_POINT *cur_point, struct Z_POINT *cur_point1);
	int     CheckWindowPointIndex(struct Z_POINT *c_point, MagImg * c_win);

	double GetScaleUnits(char *gsc, short g_int);
	int VComputeLine ( int x1, int y1, int x2, int y2, wxPoint** points, int* npoints );
	int GetValue(struct Z_POINT *gvpoint);
	int DisplayProfile(struct G_POINT *dp);
	double FindDistance(struct G_POINT *gp1, struct G_POINT *gp2,float z_diff);
	int CheckPointsIndex(struct Z_POINT *c_point, struct Z_POINT *c_point1);
	void SortGraphPoints(struct G_POINT *gp);
	int FindDensityValues();
	void RemoveProfileInfo();
	int  MeasureWidth(const wxPoint  pos);
	void DrawDensity();
	int  DrawAllLine();
	int  ChangeDensity();
	int  InitDensity();
	int  SaveProfile ( unsigned char* cFilename );


	/////////////// for Density Algorithm	 /////////


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
    DensityCanvas ( void );
    DensityCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~DensityCanvas ( void );
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
  bool   getDensityDone(void) const; 
  
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
  void   setDensityDone(bool done)
  {
    m_bDensityDone = done;
  }; 
  
  void setLayout(const bool overlay)
  {
	  m_bLayout = overlay;
  }
  

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunDensity();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

    DECLARE_DYNAMIC_CLASS(DensityCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
