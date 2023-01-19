/*
  Copyright 1993-2009 Medical Image Processing Group
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
 * \file   FuzzCompCanvas.h
 * \brief  FuzzCompCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FuzzCompCanvas_h
#define __FuzzCompCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"
#define MAXPOINTS 100

#define NUM_FEATURES 5
#define NUM_FUNCTIONS 3

class FuzzCompFrame;
enum FuzzCompType /* Type used in both frame and canvas */
{
	HISTOGRAM=0,
	COVARIANCE,
	PARAMETER
};

typedef unsigned short conn_t;

typedef struct PointWithValue {
  short x, y;
  conn_t val;
} PointWithValue;

typedef struct 
{
  int xdim, ydim, zdim;
  long slice_size, volume_size;
  double voxelsize_x, voxelsize_y, voxelsize_z;
} S_dimensions;

typedef struct {
  long l; /* link to next */
  long i; /* index into array */
  PointWithValue c[1]; /* content */
} HheapElem;


typedef struct {
  char *q;
  long *h;
  long *hh;
  long allocsize;
  long size;
  long hashsize;
  int itemsize;
  int elem_size;
  //long (*hf)(); /* hash function */
  //int (*valcmp)(); /* returns relative ordering of two items by value of item; highest value item is popped first */
  //int (*idcmp)(); /* returns relative ordering of two items by identity (not value).  Must return positive if only the second pointer is NULL. */
} Hheap;

//class FilterCanvas : public wxScrolledWindow {
/** \brief FilterCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class FuzzCompCanvas : public MainCanvas {
    int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
	FuzzCompType     m_FuzzCompType;
	bool             m_bFuzzCompDone;
	bool             m_bCostImgDone;
	bool             m_bPararell;
	
public:

    wxImage**        m_images;           ///< images for displayed slices
    wxBitmap**       m_bitmaps;          ///< bitmaps for displayed slices
	wxBitmap**       m_bitmaps1;
	wxBitmap**       m_bitmaps2;
	int              m_tx, m_ty, nSeed;         ///< translation for all images
	int              mMode;
	int              mData[MAXPOINTS][3];  //user indicated/specified points
protected:
    wxFrame*         myFrame;
   static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    bool             m_overlay;          ///< toggle display overlay
	bool             m_training, m_painting, m_erasing;
    int              m_rows, m_cols;
    int              lastX, lastY;
	int              m_sliceNo, m_BrushSize;
    double           m_scale;           ///< scale for both directions
    int              m_weight[5], m_level[5], m_widthLevel[5];
    int              mNoPaintingPts, mNoErasingPts;
	int              (*m_paintingPts)[3]; //mask points
	int              (*m_erasingPts)[3]; //mask points
	int*              m_brushSizePts;//brush size
	//wxMask         m_training_mask; //training mask
                                         
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    FuzzCompCanvas ( void );
    FuzzCompCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                      const wxPoint &pos, const wxSize &size );

    ~FuzzCompCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    /** \brief free any allocated images (of type wxImage) and/or bitmaps (of type wxBitmap) */
    void freeImagesAndBitmaps ( void ) 
	{
        if (m_images!=NULL) {
            for (int i=0; i<3; i++)   //m_rows*m_cols
			{
                if (m_images[i]!=NULL) {
                    delete m_images[i];
                    m_images[i]=NULL;
                }
            }
            free(m_images);
            m_images = NULL;
        }

        if (m_bitmaps!=NULL)
		{
            for (int i=0; i<3; i++)    //m_rows*m_cols
			{
                if (m_bitmaps[i]!=NULL) {
                    delete m_bitmaps[i];
                    m_bitmaps[i]=NULL;
                }
            }
            free(m_bitmaps);
            m_bitmaps = NULL;
        }
		
		if (m_bitmaps1!=NULL) 
		{
            for (int i=0; i<3; i++)   //m_rows*m_cols
			{
                if (m_bitmaps1[i]!=NULL) {
                    delete m_bitmaps1[i];
                    m_bitmaps1[i]=NULL;
                }
            }
            free(m_bitmaps1);
            m_bitmaps1 = NULL;
		}
		
		if (m_bitmaps2!=NULL) {
            for (int i=0; i<3; i++)    //m_rows*m_cols
			{
                if (m_bitmaps2[i]!=NULL) {
                    delete m_bitmaps2[i];
                    m_bitmaps2[i]=NULL;
                }
            }
            free(m_bitmaps2);
            m_bitmaps2 = NULL;
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
	//void OnEraseBackground ( wxEraseEvent& e );
	void paint       ( wxDC& dc        );
    void OnSize       ( wxSizeEvent&  e );
    void HandlePaint  ( wxDC& m         );
	
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	////////////Fuzzy component algorithm  //////////
	int affinity_type;
	int training_samples;
	int max_training_samples;
	int (*point_picked)[3], num_points_picked, max_points;
	int smallest_density_value, largest_density_value;
	unsigned short *training_sample;
	bool  affinity_data_valid;
	bool  connectivity_data_valid;
	bool  masked_original_valid;
	int fuzzy_adjacency_flag;	
    unsigned short *feature_data_across[NUM_FEATURES], *feature_data_down[NUM_FEATURES], *affinity_data_across, *affinity_data_down;
    conn_t *connectivity_data;
	unsigned short *affinityImg, *connectivityImg;
	int and_op;
	int training_image_flag;
	unsigned char *training_image;	
	void *slice_data, *masked_original;
	int computed_threshold;
	int threshold;
	
	S_dimensions dimensions;
	wxPoint nbor[4];

	int histogram_bins[2]; //={50, 50};
	int *histogram_counts;
	int feature_selected, function_selected[NUM_FEATURES], feature_status[NUM_FEATURES],
	feature_data_valid[NUM_FEATURES];
	float weight[NUM_FEATURES]; //={1, 1, 1, 1, 1};
	float function_level[NUM_FEATURES], function_width[NUM_FEATURES], function_domain[NUM_FEATURES][2];
	double training_sum[NUM_FEATURES], 	training_sum_sqr[NUM_FEATURES][NUM_FEATURES], 
		   training_min[NUM_FEATURES], training_max[NUM_FEATURES];
	double training_mean[NUM_FEATURES], inv_covariance[NUM_FEATURES*NUM_FEATURES], det_covariance;

	int  ApplyTraining();
	void function_update();
	void accumulate_training();
	int  VInvertMatrix(double Ainv[], double A[], int N);
	int  VComputeDeterminant(double *det, double A[], int N);
	void FuzzyComp_update();
	void compute_feature_image(int feature);
	void compute_affinity_image();
	void compute_connectivity_image();
	float transform(float x, int type, float level, float width);
	void  display_image(int which, unsigned short* pData);

	Hheap* hheap_create(long hashsize, int itemsize ); //, long (*hf)(), int (*valcmp)(), int (*idcmp)() );
	int  hheap_destroy(  Hheap *H );
	int hheap_isempty( Hheap *H );
	int hheap_enlarge( Hheap *H );
	int hheap_push( Hheap *H,  PointWithValue *v );
	int hheap_repush(Hheap *H, PointWithValue *v);
	int hheap_pop(  Hheap *H, PointWithValue *v );
	
	int push_xy_hheap(int x, int y, conn_t w);
	int repush_xy_hheap(int x, int y, conn_t w);
	long pop_xy_hheap(int *x, int *y);
	long hheap_hash_xy(  long hashsize, PointWithValue *v );	
	int point_value_cmp(PointWithValue *v, PointWithValue *vv);
	int point_cmp(PointWithValue *v, PointWithValue *vv);
	Hheap *H;

	////////////Fuzzy component algorithm  //////////

    bool   isLoaded    ( const int which ) const;

    bool   getOverlay  (void) const;
    int    getCenter   ( const int which ) const;
    bool   getInvert   ( const int which ) const;
    int    getMax      ( const int which ) const;
    int    getMin      ( const int which ) const;
    int    getNoSlices ( const int which ) const;    //number of slices in entire data set
    int    getSliceNo  ( const int which ) const;    //first displayed slice
    int    getWidth    ( const int which ) const;
    int    getWeight       ( const int which ) const;
    int    getLevel        ( const int which ) const;
    int    getWidthLevel   ( const int which ) const;
    double getScale    ( void ) const;

	FuzzCompType   getFuzzCompType(void) const 
	{
		return m_FuzzCompType;
	};

	bool    getFuzzCompDone(void) const 
	{
		return m_bFuzzCompDone;
	};
	bool    getCostImgDone(void) const 
	{
		return m_bCostImgDone;
	};	
	bool    getPararell()  
	{
		return m_bPararell;		
	};

    void   setOverlay  ( const bool overlay );
	void   setTraining ( const bool training );
	void   setPainting ( const bool painting );
	void   setErasing  ( const bool erasing );
    void   setCenter   ( const int which, const int    center  );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, const int    sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setScale    ( const double scale );
    void   setWeight   ( const int which, const int   weight  );
    void   setLevel    ( const int which, const int    level  );
    void   setWidthLevel  ( const int which, const int   widthLevel );	
	void   setFuzzCompType(FuzzCompType ftype)
	{
		m_FuzzCompType = ftype;
	} ;
	void    setFuzzCompDone(bool bFuzzCompDone)  
	{
		m_bFuzzCompDone = bFuzzCompDone;
		connectivity_data_valid = bFuzzCompDone;	

		for( int i=0; i<NUM_FEATURES; i++)
			feature_data_valid[i] = false;
	};
	void    setCostImgDone(bool bCostImgDone)  
	{
		m_bCostImgDone = bCostImgDone;
		affinity_data_valid = bCostImgDone;
		for( int i=0; i<NUM_FEATURES; i++)
			feature_data_valid[i] = false;
	};

	void    setPararell(bool value)  
	{
		m_bPararell = value;		
	};

    void setBrushSize ( const int size );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void RunFuzzComp();
    void CreateDisplayImage(int which);
	void ResetSeed ( void );
	void ResetTraining (void);
	bool IsSeed (void) const;
	void resetScale (void);
   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline void mapDataToWindow ( int x, int y, int z, int& wx, int& wy ) const
    {
        //map the point in the 3d data set back into a point in the window
        wx = wy = -1;
        //if (!inBounds(x, y, z)) {
        //    wxLogMessage( "MontageCanvas::mapDataToWindow: out of bounds at (%d,%d,%d).", x, y, z );
        //}
        if      (x<0)         x=0;
        else if (x>=mCavassData->m_xSize)  x=mCavassData->m_xSize-1;
        if      (y<0)         y=0;
        else if (y>=mCavassData->m_ySize)  y=mCavassData->m_ySize-1;
        if      (z<0)         z=0;
        else if (z>=mCavassData->m_zSize)  z=mCavassData->m_zSize-1;
        const int lastSlice = m_sliceNo + m_rows*m_cols - 1;
        if (z<m_sliceNo || z>lastSlice)    return;
        const int  col    = (z-m_sliceNo) % m_cols;
        const int  row    = (z-m_sliceNo) / m_cols;
        const int  startX = (int)(col*(ceil(mCavassData->m_xSize*mCavassData->m_scale)+1));
        const int  startY = (int)(row*(ceil(mCavassData->m_ySize*mCavassData->m_scale)+1));
        wx = startX + (int)(x*mCavassData->m_scale+0.5);
        wy = startY + (int)(y*mCavassData->m_scale+0.5);
        wx += m_tx;
        wy += m_ty;
    }

	DECLARE_DYNAMIC_CLASS(FuzzCompCanvas)
    DECLARE_EVENT_TABLE()
};

//#include "FilterAlgorithms.cpp"
#endif
