/*
  Copyright 1993-2013 Medical Image Processing Group
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
 * \file   ToStructureCanvas.h
 * \brief  ToStructureCanvas definition.
 * \author 
 *
 * Copyright 2013 University of Pennsylvania
 *
 */
//======================================================================
#ifndef __ToStructureCanvas_h
#define __ToStructureCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"

#define NUM_FEATURE_LINES 11
//#define MSG_WIDTH  200
//class ToStructureCanvas : public wxScrolledWindow {
/** \brief ToStructureCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class ToStructureCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z  
  bool           m_bToStructureDone;
  int            m_nROIType;
  int            m_nGradType;
  int            mFileOrDataCount;  
  wxListCtrl     *mListCtrl;
  int            m_timeInstances;
  int            m_nRefFrame;

public:
    /////////////// for ToStructure Algorithm	 /////////
	Surf_Info *surf;
	char *strDisplay;
	int  nInstancesNum;

	char m_DisplayStr[NUM_FEATURE_LINES][MSG_WIDTH];
	int  SaveProfile ( unsigned char* cFilename );
	

	/////////////// for ToStructure Algorithm	 /////////


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
    ToStructureCanvas ( void );
    ToStructureCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~ToStructureCanvas ( void );
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
 
  bool   getToStructureDone(void) const;
 
 
  int getInstances() const
  {
	  return m_timeInstances;
  }  

    int getRefFrame() const
  {
	  return m_nRefFrame;
  }  
 
  void   setToStructureDone(bool done)
  {
    m_bToStructureDone = done;
  }  
 
   void setInstances(int ntimeInstances)
  {
    m_timeInstances = ntimeInstances;
  }
  
    void setRefFrame(int nRefFrame)
  {
    m_nRefFrame = nRefFrame;
  }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunToStructure();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

    DECLARE_DYNAMIC_CLASS(ToStructureCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
