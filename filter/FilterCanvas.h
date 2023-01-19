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
 * \file   FilterCanvas.h
 * \brief  FilterCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FilterCanvas_h
#define __FilterCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"
//#include  "ScaleComputation2D.h"

#define MAX_INTERVAL 4 /* maximum number of intensity intervals allowed for correction */

enum CavassFilterType /* Type used in both frame and canvas */
{
	FILTER_GRADIENT2D=0,
	FILTER_GRADIENT3D,
	FILTER_GAUSSIAN2D,
	FILTER_GAUSSIAN3D,
	FILTER_DIST2Dxy,
	FILTER_DIST3D,
	FILTER_LTDT3D,
	FILTER_SCALE2D,
	FILTER_SCALE3D,
	FILTER_SBAv2D,
	FILTER_SBAv3D,
	FILTER_BScale3D,
	FILTER_SBAD2D,
	FILTER_SBAD3D,

	FILTER_SOBEL,
	FILTER_TOBOG,
	FILTER_DILATE,
	FILTER_ERODE,
	FILTER_MEAN2D,
	FILTER_MEAN3D,
	FILTER_INHOMO1,
	FILTER_INHOMO2,
	FILTER_INHOMO3,
	FILTER_BALL_ENH,
	FILTER_MAXIMA
};


//class FilterCanvas : public wxScrolledWindow {
/** \brief FilterCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class FilterCanvas : public MainCanvas 
{
  int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
  CavassFilterType  m_filterType;
  bool           m_bFilterDone;
  double         m_Sigma;
  int            m_ScaleADIterations;
  int            m_MorphIterations;
  int            m_Homogen;
  int            m_MorphN;
  int            m_nLTDTDistType;  // 0: Bg --> Fg; 1: Fg --> Bg;  2: Both
  double         m_VolThresh;
  double         m_StopCond;
  double         m_LeftParam;
  double         m_RightParam;
  double         m_ZetaFactor;
  double         m_IncluFactor;
  double         m_StdDevia;
  int            m_NumOfRegion;
  int            mFileOrDataCount;
  bool           m_nLTDTPararell;
  bool           m_nLTDTFTSave;
  bool           m_nSBAv3DPararell;

public:

	SliceData*        m_sliceIn;
	SliceData*        m_sliceOut;

    wxImage*          m_images[2];           ///< images for displayed slices
    wxBitmap*         m_bitmaps[2];          ///< bitmaps for displayed slices
    int               m_tx, m_ty;         ///< translation for all images
	int               min_thresh[MAX_INTERVAL], max_thresh[MAX_INTERVAL];
	int               num_interval;
    int               m_MaxRadius;
    int               m_MinRadius;
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
    FilterCanvas ( void );
    FilterCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                   const wxPoint &pos, const wxSize &size );

    ~FilterCanvas ( void );
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
  bool   getFilterDone(void) const;
  CavassFilterType   getFilterType(void) const 
  {
    return m_filterType;
  };
  double getSigma(void) const
  {
    return m_Sigma;
  }
  int getHomogen(void) const
  {
	  return m_Homogen;
  }
  int getMorphN(void) const
  {
	  return m_MorphN;
  }
  int getLTDTDistType(void) const
  {
      return m_nLTDTDistType;
  }
  int getScaleADIterations(void) const
  {
	  return m_ScaleADIterations;
  }
  int getMorphIterations(void) const
  {
      return m_MorphIterations;
  }
  double getVolThresh(void) const
  {
	  return m_VolThresh;
  }
  double getStopCond(void) const
  {
	  return m_StopCond;
  }
  double getLeftParam(void) const
  {
	  return m_LeftParam;
  }
  double getRightParam(void) const
  {
	  return m_RightParam;
  }
  double getZetaFactor(void) const
  {
	  return m_ZetaFactor;
  }
  double getIncluFactor(void) const
  {
	  return m_IncluFactor;
  }
  double getStdDevia(void) const
  {
	  return m_StdDevia;
  }
  int getNumOfRegion(void) const
  {
	  return m_NumOfRegion;
  }
  bool getLTDTPararell()
  {
	  return m_nLTDTPararell;
  }
  bool getLTDTFTSave()
  {
	  return m_nLTDTFTSave;
  }
  bool getSBAv3DPararell()
  {
	  return m_nSBAv3DPararell;
  }  

  void   setOverlay  ( const bool overlay );
  void   setCenter   ( const int which, const int    center  );
  void   setInvert   ( const int which, const bool   invert  );
  void   setSliceNo  ( const int which, const int    sliceNo );
  void   setWidth    ( const int which, const int    width   );
  void   setScale    ( const double scale );
  void   setFilterDone(bool done)
  {
    m_bFilterDone = done;
  };
  void   setFilterType(CavassFilterType ftype)
  {
    m_filterType = ftype;
  };
  void setSigma(double sigma)
  {
    m_Sigma = sigma;
  }
  void setHomogen(int nHomogen)
  {
    m_Homogen = nHomogen;
  }
  void setMorphN(int nMorph)
  {
    m_MorphN = nMorph;
  }
  void setLTDTDistType(int nDistType)
  {
    m_nLTDTDistType = nDistType;
  }
  void setLTDTPararell(bool value)
  {
    m_nLTDTPararell = value;
  }
  void setLTDTFTSave(bool value)
  {
    m_nLTDTFTSave = value;
  }
  void setSBAv3DPararell(bool value)
  {
    m_nSBAv3DPararell = value;
  }
  
  
  void setScaleADIterations(int iterations)
  {
	  m_ScaleADIterations = iterations;
  }
  void setMorphIterations(int iterations)
  {
      m_MorphIterations = iterations;
  }
  void setVolThresh(double new_val)
  {
	  m_VolThresh = new_val;
  }
  void setStopCond(double new_val)
  {
	  m_StopCond = new_val;
  }
  void setLeftParam(double new_val)
  {
	  m_LeftParam = new_val;
  }
  void setRightParam(double new_val)
  {
	  m_RightParam = new_val;
  }
  void setZetaFactor(double zetaFactor)
  {
	  m_ZetaFactor = zetaFactor;
  }
  void setIncluFactor(double incluFactor)
  {
	  m_IncluFactor = incluFactor;
  }
  void setStdDevia(double stdDevia)
  {
	  m_StdDevia = stdDevia;
  }
  void setNumOfRegion(int numOfRegion)
  {
	  m_NumOfRegion = numOfRegion;
  }
  void initializeParameters(void);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RunFilter();
  void CreateDisplayImage(int which);
  float normal(float x, float sigma);

  void scale2d(char *fname, int sliceno,int width,int height, int num_of_bits, unsigned char *out, float sigma);

  void scaleAv2d(char *fname, int sliceno, int width,int height, int num_of_bits, unsigned char *out, float sigma);

  void scaleAD2d(char *fname, int sliceno, int width,int height, int num_of_bits, unsigned char *out, float sigma, int iteration);


  void dist2d(char *fname, int sliceno,int width,int height,unsigned char *out);
  void gradient8(unsigned char *input, int width, int height, unsigned char *output, float MIN, float MAX);
  int gradient_separated8(unsigned char *in1, unsigned char *in2,unsigned char *in3,
					int width, int height, float space,float pixel,unsigned char *out);

  void gradient16(unsigned short *input, int width, int height, unsigned short *output, float MIN, float MAX);
  int gradient_separated16(unsigned short *in1, unsigned short *in2,unsigned short *in3,
					int width, int height, float space,float pixel,unsigned short *out);

  int gaussian8(unsigned char *in2, int width,int height, unsigned char *out, float sigma);
  int gaussian16(unsigned short *in2, int width,int height, unsigned short *out, float sigma);
  int gaussian_contiguous(unsigned char *in1,unsigned char *in2,unsigned char *in3,int width,int height, unsigned char *out, float sigma);
  int gaussian_separated8(unsigned char *in1,unsigned char *in2,unsigned char *in3,
		  				    int width,int height,float space,float pixel,
							unsigned char *out, float sigma);
  int gaussian_separated16(unsigned short *in1,unsigned short *in2,unsigned short *in3,
		  				    int width,int height,float space,float pixel,
							unsigned short *out, float sigma);

    DECLARE_DYNAMIC_CLASS(FilterCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
