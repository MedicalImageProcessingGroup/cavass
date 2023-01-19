/*
  Copyright 1993-2014 Medical Image Processing Group
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
 * \file   AlgebraCanvas.h
 * \brief  AlgebraCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __AlgebraCanvas_h
#define __AlgebraCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"
//#include  "ScaleComputation2D.h"

enum CavassAlgebraType /* Type used in both frame and canvas */
{
	ALGEBRA_APLUSB=0,
	ALGEBRA_AMINUSB,
	ALGEBRA_BMINUSA,
	ALGEBRA_ABSAMINUSB,
	ALGEBRA_AXB,
    ALGEBRA_ADIVIDEB,
	ALGEBRA_BDIVIDEA,
	ALGEBRA_AGREATB,
    ALGEBRA_BGREATA,
    ALGEBRA_MEAN,        
};


//class AlgebraCanvas : public wxScrolledWindow {
/** \brief AlgebraCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class AlgebraCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
  CavassAlgebraType	  m_AlgebraType;
  bool				  m_bAlgebraDone;
  double			  m_Offset;
  double              m_Coefficient;
  int				  m_DivThre;
  int				  m_FileOrDataCount;

public:

	SliceData*        m_sliceData1;
	SliceData*        m_sliceData2;
	SliceData*        m_sliceOut;
	unsigned short   *m_sliceOutData;

    wxImage*          m_images[3];           ///< images for displayed slices 2
    wxBitmap*         m_bitmaps[3];          ///< bitmaps for displayed slices 2
    int               m_tx, m_ty;         ///< translation for all images
protected:
  //static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    bool             m_overlay;          ///< toggle display overlay
    int              m_rows, m_cols;
    int              lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    AlgebraCanvas ( void );
    AlgebraCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~AlgebraCanvas ( void );
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
		if (m_images[2]!=NULL)
		{
			m_images[2]->Destroy();		
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
		if (m_bitmaps[2]!=NULL)
		{
		delete m_bitmaps[2];
		m_bitmaps[2]=NULL;
		}

    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void release ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
	bool IsImgSizeEqual();
  void loadData ( char* name,
		  const int xSize, const int ySize, const int zSize,
		  const double xSpacing, const double ySpacing, const double zSpacing,
		  const int* const data, const ViewnixHeader* const vh=NULL,
		  const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  int  loadFile ( const char* const fn );
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
  bool   getAlgebraDone(void) const;
  
  CavassAlgebraType   getAlgebraType(void) const 
  {
    return m_AlgebraType;
  };
  double getOffset(void) const
  {
    return m_Offset;
  }
  double getCoefficient(void) const
  {
    return m_Coefficient;
  }
  int getDivThre(void) const
  {
    return m_DivThre;
  }
  void   setOverlay  ( const bool overlay );
  void   setCenter   ( const int which, const int    center  );
  void   setInvert   ( const int which, const bool   invert  );
  void   setSliceNo  ( const int which, const int    sliceNo );
  void   setWidth    ( const int which, const int    width   );
  void   setScale    ( const double scale );
  void   setAlgebraDone(bool done)
  {
    m_bAlgebraDone = done;
  };
  void   setAlgebraType(CavassAlgebraType ftype)
  {
    m_AlgebraType = ftype;
  };
  void setOffset(int  offset)
  {
    m_Offset = offset;
  }
  void setCoefficient(double coefficient)
  {
    m_Coefficient = coefficient;
  }
  void setDivThre(int threshold)
  {
    m_DivThre = threshold;
  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunAlgebra();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

  int  AlgebraCompute_Sw( unsigned short *inA, unsigned short *inB, int width, int height, unsigned short *pOut);  
 
    DECLARE_DYNAMIC_CLASS(AlgebraCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
