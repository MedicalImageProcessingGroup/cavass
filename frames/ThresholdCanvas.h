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
 * \file   ThresholdCanvas.h
 * \brief  ThresholdCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ThresholdCanvas_h
#define __ThresholdCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"

#define MAX_INTERVAL 4
#define MAX_OPTIMA 16
#define Left  0
#define Right 1
#define MAX_TRACK_POINTS 50


int compute_histogram(double hist[65536], CavassData *cd,
		int scope /* 0= slice, 1= volume, 2= scene */,
		int volume, int slice /* from 0 */);
void cvUpdateThresholdTablets(MainFrame *mf);

//class ThresholdCanvas : public wxScrolledWindow {
/** \brief ThresholdCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class ThresholdCanvas : public MainCanvas {
    int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
	bool           m_bThresholdDone;
public:
	enum ThresholdColor {WHITE, RED, GREEN, BLUE};

    wxImage*          m_images[2];           ///< images for displayed slices
    wxBitmap*         m_bitmaps[2];          ///< bitmaps for displayed slices
    int               m_tx, m_ty;         ///< translation for all images
protected:
	bool              fuzzy_flag;
  //static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
	bool             m_overlay;          ///< toggle display overlay
    int              m_rows, m_cols;
    int              lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction

	double hist[65536]; /* used for histogram calculation */
	int hist_size, min_hist, max_hist;
	double total_count;
	short histx, histy, histw, histh; /* location and dimension of histogram */
	short resx, resy; /* location of RESULTing image */
	float hist_zoom_factor;
	float opt_step;
	int hist_scope; /* 0= slice, 1= volume, 2= scene */
	int thr_state;
	int substate;
	unsigned char lookup_table[65536];
public:
	/* In the following 9 tables, elements 1 to MAX_INTERVAL refer to the
	   threshold intervals, MAX_INTERVAL+1 to the optimization interval. */
	int min_table[MAX_INTERVAL+2], max_table[MAX_INTERVAL+2];
	int tbar1x[MAX_INTERVAL+2],tbar2x[MAX_INTERVAL+2];
	int inter1_table[MAX_INTERVAL+2], inter2_table[MAX_INTERVAL+2];
	int tbar3x[MAX_INTERVAL+2], tbar4x[MAX_INTERVAL+2], tbar5x[MAX_INTERVAL+2];

	int num_optima, optimum[MAX_OPTIMA];
	int interval; /* The interval number for multiple thresholds, 1 to 
		MAX_INTERVAL; MAX_INTERVAL+1 = All, MAX_INTERVAL+2 = optimization
		interval. */
	bool BLINK;
	short track_pt[MAX_TRACK_POINTS][3];
	int num_track_points;
	int track_volume;
	int BOUNDARY_TYPE; /* 0- All-faces 1-Connected */

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ThresholdCanvas ( void );
    ThresholdCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                      const wxPoint &pos, const wxSize &size, bool fuzzy );

    ~ThresholdCanvas ( void );
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
	void draw_bar_end(wxDC &dc, int x, int mode, int value);
	void erase_bar_end(wxDC &dc, int x, int mode);
	void draw_bar(wxDC &dc, int x1, int x2);
	void erase_bar(wxDC &dc, int x1, int x2);
	void display_percentile(wxDC &dc, int eventx);
	void initialize_lookup_table();
	void create_new_lookup_table(ThresholdColor);
	void make_lookup_table(int min, int max, ThresholdColor, int inter1=-1,
		int inter2=-1);
	void   setOptimum  ( int rank, int value );
	void   setNumOptima( int );
	void SetStatusText(const wxString& text, int number)
	{
		m_parent_frame->SetStatusText(text, number);
	}
	int DisplayTrackPoints(int which, wxDC *dc);
	void RedrawHistogram( wxDC* dc );
	void draw_thresh_fn( wxDC* dc, int x1, int y1, int x2, int y2 );
	void UpdateThresholdTablets( void )
	{
		cvUpdateThresholdTablets(m_parent_frame);
	}

  public:
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh=NULL,
        const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void loadFile ( const char* const fn );
	void resetThresholds ( void );
    void initLUT ( const int which );
	void Redraw_Threshold_Values(wxDC & dc);
    void reload ( void );
    void mapWindowToData( int wx, int wy, int& x, int& y, int& z, int& which );
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
	bool   getThresholdDone(void) const;
	double getHistCount( unsigned short value );
	int    getHistMin  ( void );
	int    getHistMax  ( void );
	int    getHistScope( void ) { return hist_scope; }
	int    getInterval ( void );
	int    getIntervalMin( int intrvl );
	int    getIntervalMax( int intrvl );
	int    getThrState ( void );
	float  getHistZoomFactor( void ) { return hist_zoom_factor; }
	float  getOptStep ( void ) { return opt_step; }

    void   setOverlay  ( const bool overlay );
    void   setCenter   ( const int which, const int    center  );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, const int    sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setScale    ( const double scale );
	void   setThresholdDone(bool done);
	void   setHistCount( double count, unsigned short value );
	void   setHistMin  ( int min );
	void   setHistMax  ( int max );
	void   incrementHistCount( unsigned short value )
	       {
		      hist[value]++;
		   }
	void   setHistScope( int );
	void   setHistLayout(short x, short y, short w, short h );
	void   setResultLoc( short x, short y );
	void   setHistZoomFactor( float );
	void   setOptStep ( float new_opt_step ) { opt_step = new_opt_step; }
	void   findOptima ( void );
	void   setInterval ( int );
	void   setIntervalMin(int intrvl, int value );
	void   setIntervalMax(int intrvl, int value );
	void   setThrState ( int );
	void   moveThrBar1 ( double );
	void   moveThrBar2 ( double );
	void   moveThrBar3 ( double );
	void   moveThrBar4 ( double );

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void RunThreshold();
	void CreateDisplayImage(int which);

	DECLARE_DYNAMIC_CLASS(ThresholdCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
