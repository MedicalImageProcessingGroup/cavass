/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   IntenMapCanvas.h
 * \brief  IntenMapCanvas definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __IntenMapCanvas_h
#define __IntenMapCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"


//class IntenMapCanvas : public wxScrolledWindow {
/** \brief IntenMapCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class IntenMapCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z  
  bool           m_bIntenMapDone;
  int            m_nROIType;
  int            m_nGradType;
  int            mFileOrDataCount;
  int            m_IntenMapGraphLeft;
  int            m_IntenMapGraphTop;
  int            m_IntenMapGraphRight;
  int            m_IntenMapGraphBottom;
  wxListCtrl     *mListCtrl;
  bool           m_bThresholdDone;	
  float          m_fHistZoomFactor;
  int            m_nMIPMinThre;
  int            m_nMIPMaxThre;
  int m_nTfmType; /* 0= gaussian, 1= lower gaussian, 2= upper gaussian,
        3= ramp, 4= inv. gaussian, 5= inv. lower gaussian,
		6= inv. upper gaussian, 7= inv. ramp */

public:
 
	int  SaveProfile ( const char* cFilename );
	bool m_bMouseMove;
	wxPoint m_MousePos;
	enum ThresholdColor {WHITE, RED, GREEN, BLUE};
	bool BLINK;

	int m_nROIL;
	int m_nROIR;
	int m_nROIT;
	int m_nROIB;
	int m_nStartSlice;
	int m_nEndSlice;
	int m_nROIWidth;
	int m_nROIHeight;

	SliceData*        m_sliceIn;
	SliceData*        m_sliceOut;

    wxImage*          m_images[2];           ///< images for displayed slices
    wxBitmap*         m_bitmaps[2];          ///< bitmaps for displayed slices
    int               m_tx, m_ty;         ///< translation for all images

	long hist[65536]; /* used for histogram calculation */
	int hist_size, min_hist, max_hist, total_count;
	short histx, histy, histw, histh; /* location and dimension of histogram */
	short resx, resy; /* location of RESULTing image */
	float gauss_mean, gauss_sd;
	int hist_scope; /* 0= slice, 1= volume, 2= scene */
	int thr_state;
	int substate;
	bool              fuzzy_flag;
	int num_optima, optimum[MAX_OPTIMA];
	int interval; /* The interval number for multiple thresholds, 1 to MAX_INTERVAL; MAX_INTERVAL+1 = All, MAX_INTERVAL+2 = optimization interval. */
	unsigned char lookup_table[65536];

	/* In the following 9 tables, elements 1 to MAX_INTERVAL refer to the
	   threshold intervals, MAX_INTERVAL+1 to the optimization interval. */
	int min_table[MAX_INTERVAL+2], max_table[MAX_INTERVAL+2];
	int tbar1x[MAX_INTERVAL+2],tbar2x[MAX_INTERVAL+2];
	int inter1_table[MAX_INTERVAL+2], inter2_table[MAX_INTERVAL+2];
	int tbar3x[MAX_INTERVAL+2], tbar4x[MAX_INTERVAL+2], tbar5x[MAX_INTERVAL+2];

	void draw_thresh_fn(wxDC* dc, int x1, int y1, int x2, int y2);
	void RedrawHistogram( wxDC* dc );
	void Redraw_Threshold_Values(wxDC & dc);
	void draw_bar_end(wxDC &dc, int x, int mode, int value);
	void erase_bar_end(wxDC &dc, int x, int mode);
	void draw_bar(wxDC &dc, int x1, int x2);
	void erase_bar(wxDC &dc, int x1, int x2);
	void resetThresholds ( void );
	void build_table(Table_Info *tbl);
	void make_lookup_table(int min, int max, ThresholdColor, int inter1=-1, int inter2=-1);
	void create_new_lookup_table(ThresholdColor c);
	void initialize_lookup_table();
	void RunThreshold();

protected:
  //static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
	bool             m_bROI;
	bool             m_bLocOrSize;       // false: location; true:size
    bool             m_overlay;          ///< toggle display overlay
	bool             m_bLayout; 
    int              m_rows, m_cols;
    int              lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    IntenMapCanvas ( void );
    IntenMapCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~IntenMapCanvas ( void );
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
  bool   getIntenMapDone(void) const;
  void SetStatusText(const wxString& text, int number)
	{
		m_parent_frame->SetStatusText(text, number);
	}
 
  //int    getHeight    ( const int which ) const;

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

  int getStartSlice()
  {
	  return m_nStartSlice;
  }
  int getEndSlice()
  {
	  return m_nEndSlice;
  }
  int getLocX()
  {
	  return m_nROIL;
  }
  int getLocY()
  {
	  return m_nROIT;
  }
  int getWinWidth()
  {
	  return m_nROIWidth;
  }
  int getWinHeight()
  {
	  return m_nROIHeight;
  }
  float getHistZoomFactor()
  {
	  return m_fHistZoomFactor;
  }

  int getMIPMinThre()
  {
	  return m_nMIPMinThre;
  }
  int getMIPMaxThre()
  {
	  return m_nMIPMaxThre;
  }
     
  void setTfmType(int tfmType)
  {
	  m_nTfmType = tfmType;
  }
  
  void setMIPMinThre(int minThre)
  {
	  m_nMIPMinThre = minThre;
  }
  void setMIPMaxThre(int maxThre)
  {
	  m_nMIPMaxThre = maxThre;
  }
  void   setOverlay  ( const bool overlay );
  void   setCenter   ( const int which, const int    center  );
  void   setInvert   ( const int which, const bool   invert  );
  void   setSliceNo  ( const int which, const int    sliceNo );
  void   setWidth    ( const int which, const int    width   );
  void   setScale    ( const double scale );
  void   setIntenMapDone(bool done)
  {
    m_bIntenMapDone = done;
  };
  void setHistZoomFactor(float fHistZoomFactor)
  {
	  m_fHistZoomFactor = fHistZoomFactor;
  }
 
  void setLayout(const bool overlay)
  {
	  m_bLayout = overlay;
  }
  
  void setStartSlice(int nStartSlice)
  {
	  m_nStartSlice = nStartSlice;
  }
  void setEndSlice(int nEndSlice)
  {
	  m_nEndSlice = nEndSlice;
  }
  void setLocX(int nLocX)
  {
	  m_nROIL = nLocX;
	  if( m_nROIL +m_nROIWidth-1 < mCavassData->m_xSize )
		  m_nROIR = m_nROIL +m_nROIWidth-1;
	  else
	  {
		  m_nROIWidth = mCavassData->m_xSize - m_nROIL;
		  m_nROIR = mCavassData->m_xSize-1;
	  }
  }
  void setLocY(int nLocY)
  {
	  m_nROIT = nLocY;
	  if( m_nROIT +m_nROIHeight-1 < mCavassData->m_ySize )
		  m_nROIB = m_nROIT +m_nROIHeight-1;
	  else
	  {
		  m_nROIHeight = mCavassData->m_ySize - m_nROIT;
		  m_nROIB = mCavassData->m_ySize-1;
	  }
  }
  void setWinWidth(int nWinWidth)
  {
	  m_nROIWidth = nWinWidth;
	  if( m_nROIL +m_nROIWidth-1 < mCavassData->m_xSize )
		  m_nROIR = m_nROIL +m_nROIWidth-1;
	  else if( m_nROIR -m_nROIWidth+1 >= 0 )
		  m_nROIL = m_nROIR -m_nROIWidth+1;
	  else
	  {
		  m_nROIL = 0;
		  m_nROIR = m_nROIL +m_nROIWidth-1;
	  }
  }
  void setWinHeight(int nWinHeight)
  {
	  m_nROIHeight = nWinHeight;
	  if( m_nROIT +m_nROIHeight-1 < mCavassData->m_ySize )
		  m_nROIB = m_nROIT +m_nROIHeight-1;
	  else if( m_nROIB -m_nROIHeight+1 >= 0 )	  
		  m_nROIT = m_nROIB -m_nROIHeight+1;
	  else
	  {
		  m_nROIT = 0;
		  m_nROIB = m_nROIT + m_nROIHeight-1;
	  }
  }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunIntenMap();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

    DECLARE_DYNAMIC_CLASS(IntenMapCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
