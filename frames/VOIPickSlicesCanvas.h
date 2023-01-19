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
 * \file   VOIPickSlicesCanvas.h
 * \brief  VOIPickSlicesCanvas definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __VOIPickSlicesCanvas_h
#define __VOIPickSlicesCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"


//class VOIPickSlicesCanvas : public wxScrolledWindow {
/** \brief VOIPickSlicesCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class VOIPickSlicesCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z  
  bool           m_bVOIPickSlicesDone;
  int            mFileOrDataCount;
  wxListCtrl     *mListCtrl;
  float          m_fHistZoomFactor;
  int            m_nMIPMinThre;
  int            m_nMIPMaxThre;
  int            m_nSH0Type;

public:
 
	int  SaveProfile ( unsigned char* cFilename );
	bool m_bMouseMove;
	wxPoint m_MousePos;
	enum ThresholdColor {WHITE, RED, GREEN, BLUE};
	bool BLINK;	
	
	SliceData*        m_sliceIn;
	SliceData*        m_sliceOut;

    wxImage*          m_images[2];           ///< images for displayed slices
    wxBitmap*         m_bitmaps[2];          ///< bitmaps for displayed slices
    int               m_tx, m_ty;         ///< translation for all images

	int hist_size, min_hist, max_hist, total_count;
	short histx, histy, histw, histh; /* location and dimension of histogram */
	short resx, resy; /* location of RESULTing image */
//	float hist_zoom_factor;	
	int hist_scope; /* 0= slice, 1= volume, 2= scene */
	int thr_state;
	int substate;
	

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
    VOIPickSlicesCanvas ( void );
    VOIPickSlicesCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~VOIPickSlicesCanvas ( void );
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
  bool   getVOIPickSlicesDone(void) const;
  void SetStatusText(const wxString& text, int number)
	{
		m_parent_frame->SetStatusText(text, number);
	}
 
  bool getLayout() const
  {
	  return m_bLayout;
  }

  int getMIPMinThre()
  {
	  return m_nMIPMinThre;
  }
  int getMIPMaxThre()
  {
	  return m_nMIPMaxThre;
  }
     
  void setSH0Type(int sh0Type)
  {
	  m_nSH0Type = sh0Type;
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
  void   setVOIPickSlicesDone(bool done)
  {
    m_bVOIPickSlicesDone = done;
  };
 
  void setLayout(const bool overlay)
  {
	  m_bLayout = overlay;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //void RunVOIPickSlices();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

    DECLARE_DYNAMIC_CLASS(VOIPickSlicesCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
