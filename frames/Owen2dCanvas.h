/*
  Copyright 1993-2014, 2016 Medical Image Processing Group
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
 * \file   Owen2dCanvas.h
 * \brief  Owen2dCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __Owen2dCanvas_h
#define __Owen2dCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "misc.h"

#include  "MainCanvas.h"



#define NUM_SEG2D_MODES 10
#define MAX_NUM_FEATURES 7 
#define MAX_NUM_TRANSFORMS 6 
#define DEFAULT_FEATURE 5
#define MAX_Q_SZ 67600
#define OPEN  0
#define CLOSE 1
#define OFF 0
#define ON 1

/* directions of vertices. On SGI, chars are unsigned.
   So don't use -ve numbers for directions.    */
#define EAST  2
#define NORTH 4
#define WEST  6
#define SOUTH 8

#define BINARY 0
#define GREY   1

#define NEG1 255
#define OUTSIDE_MASK 0x7FFFFFFF

#define MAX_POINTS  40000

#define Circular_qsize (circular.qsize==0? 65536:circular.qsize)


#if defined (WIN32) || defined (_WIN32)
typedef unsigned short ushort;
typedef unsigned long ulong;
#endif


typedef struct {
        unsigned char *original_data;   /* pointer to original image from where this derived */
        unsigned char *data;            /* pointer to the actual data representing the image */
        unsigned char bits;             /* depth of the image in bits */
        unsigned short width, height;   /* size of original image in pixels */
        unsigned short offx, offy;      /* offset on the original image in pixels */
        float   scale;                  /* factor by which image is being scaled */
        unsigned short w, h;            /* size of output image in pixels on the screen */
        unsigned short wp, hp;          /* size of output image in pixels on original image */
        unsigned short framew, frameh;  /* size of the output frame of the window (intended image size) */
        unsigned short locx, locy;      /* location of output image within window */
        unsigned char type;             /* 0 = nearest neightbour, 1 = linear interpolation */
        unsigned short *tblx, *tbly;    /* NN tables for X and Y (screen => image) */
        short *tbl2x, *tbl2y;  /* NN tables for X and Y (image => screen) */
	 	unsigned short *tblofx,*tblofy;	/* Offsets for Linear interp. The values */
										/* on the table are 'shorts' multiplied by 1000 */
										/* The factor is multiplied by the NN pixel to */
										/* yield the actual pixel value. */
        float *mult_tbl;                /* Multiplication Table */
		short output_size_change_flag;	/* indicates if output size was changed */
		unsigned short index;			/* index of the image within the scene */
        } IMAGE;

typedef struct {
		X_Point *vertex;
		int last;	/* index of the last defined vertex in the contour */
		int slice_index;	/* index of slice in which contour is defined */
		IMAGE *img;	/* Image in which contour is specified */
} OPEN_CONTOUR;

struct dlisttype{
	dlisttype *next,*previous;
};

typedef struct {
	dlisttype *begin;
	int numberelements;
} dliststructuretype;

typedef struct {
  int nvertices;
  int w;
  int source;
  char *processed; /* verifies if the minimum-cost path to the
                          vertex has been computed */
  dliststructuretype *ccost;  /* circular vector */
  int initial;
  unsigned long initial_ccost;
  unsigned short qsize;  /* circular vector size  */ 
  dlisttype *ptr_vtx;
  X_Point pt; /* beginning of the current path */ 
} circulartype;

#define dlistnext(p) (((p)->next))

typedef struct {double x; double y;} CPoint;

struct FeatureList {
	
	int status;
	int transform;
	float weight;
	float rmin;
	float rmax;
	float rmean;
	float rstddev;
	  /* r-prefix ==> ratio & not actual value */

};

typedef struct {
    int sd;              /* scene dimension */
        int width;                       /* imager width */
        int height;                      /* image height */
        int bits;                        /* image depth in bits (1, 8 or 16) */
    int total_slices;    /* total number of slices in the scene */
    int volumes;         /* number of volumes in the scene */
    int *slices;         /* number of slices in each volume in the scene */
	int max_slices;		 /* maximum #of slices within all volumes */
    float *location4;    /* location along X4 of each volume in the scene */
    float **location3;   /* location along X3 of each slice in the scene */
	int	**slice_index;	 /* index of the slice assuming a flat tree structure */
    float *min_location3,/* smallest location along X3 for each volume */
          *max_location3;/* largest location along X3 for each volume */
    float Min_location3, /* smallest slice location within the entire scene */
          Max_location3; /* largest slice location within the entire scene */
    float Min_spacing3,  /* smallest slice spacing within the entire scene */
          Max_spacing3;  /* largest slice spacing within the entire scene */
    float *min_spacing3,  /* smallest slice spacing within each volume */
          *max_spacing3;  /* largest slice spacing within each volume*/
    float min_spacing4,  /* smallest volume spacing within the entire scene */
          max_spacing4;  /* largest volume spacing within the entire scene */
    float *fov3;         /* field of view for each volume in the scene */
    float Fov3;          /* enclosing field of view (along X3) for the scene */
    int *variable_spacing;/* for each volume -  0=constant slice spacing, 1=variable s.s. */
    int Variable_spacing; /* entire scene  - 0=constant slice spacing, 1=variable s.s. */
 
                                        } SLICES;




//extern const unsigned char onbit[9];
//extern const unsigned char offbit[9];


/** \brief the canvas on which images and other things are drawn (i.e., 
 *  the drawing area of the window).
 *
 *  This class is responsible for drawing images (typically) in the upper
 *  part of the window or frame.  The main method are the constuctors,
 *  loadFile, and OnPaint.  The set* methods (mutators) change the 
 *  appearance of the drawn images.  The get* methods (inspectors) return
 *  the values of the current settings.
 */
class Owen2dCanvas : public MainCanvas {
    int  mXSize, mYSize, mZSize;         ///< max count of pixels of displayed images in x,y,z
public:
	enum {
		LWOF = 2   /* Live Wire On The Fly */ ,
		TRAINING,
		PAINT,
		MANUAL,
		ILW        /* Iterative Live Wire */ ,
		LSNAKE     /* Live Snake */ ,
		SEL_FEATURES,
		REVIEW,
		REPORT,
		PEEK,
		ROI
	};

    int               mFileOrDataCount;  ///< count of files and/or data (1 needed for this example)
    wxImage**         mImages;           ///< images for displayed slices
    wxBitmap**        mBitmaps;          ///< bitmaps for displayed slices
    int               mTx, mTy;          ///< translation for single images
	int               mTx_v, mTy_v;      ///< translation for review images
	int               mTx_f, mTy_f;      ///< translation for feature selection
	int               mTx_p, mTy_p;      ///< translation for report
	int               FixedPx, FixedPy;  ///< fixed point for single images
	int object_number;
	int empty_mask_flag[9]; /* used to indicate if a MASK has been specified or not */
	OPEN_CONTOUR o_contour;         /*  vertices of open contour
                                           in orig data coods       */
	OPEN_CONTOUR temp_contours;  /* temporary contour (used until
                                           a CUT op. is performed)         */
	unsigned char **V_edges; /*  maintains obj num bits of vert edges of whole slice */
	unsigned char **object_vertex; /* maintains obj num bits of vertices of whole slice */
	unsigned char *mask;
	unsigned char *object_mask;
	int object_mask_slice_index; // => to which slice the object_mask belongs
	unsigned char **hzl_edge_mask, **vert_edge_mask;
	unsigned char **hzl_edge_cont, **vert_edge_cont;
	struct FeatureList accepted_list[8][MAX_NUM_FEATURES];
	struct FeatureList temp_list[MAX_NUM_FEATURES];
	circulartype circular; /* Circular data structure and functions used
	    for Dijktra's algorithm in LWOF */
	unsigned char **hzl_sign_buffer,**vert_sign_buffer;//buffer for sign cost
	unsigned short **hzl_grad, **vert_grad; // hzl & vertical features arrays
	unsigned short **hzl_cost, **vert_cost;
	/* temporary storage of intermediate features and costs ... **/
	unsigned short **hzl_tempf, **vert_tempf, **hzl_tempc, **vert_tempc;
	double **totlhc, **totlvc;
	unsigned long **cc_vtx; /* cumulative costs of vertices */
	SLICES sl;
	int (*dp_anchor_point)[2], dp_anchor_points;
	int anchor_point_array_size;
	unsigned short (*ilw_control_point[2])[2];
	int ilw_control_points[2], ilw_control_slice[2];
	int ilw_iterations;
	float Imax, Imin;
	int Range;
	int pRow, pCol;
	IMAGE orig;
	int LAST_DP_slice_index; /* slice on which Calc_Edge_Cost last done */
	int detection_mode;
	int detection_modes[NUM_SEG2D_MODES];
	int num_detection_modes;
	X_Point Points[MAX_POINTS];  /* vertices in scene - used in LiveWire */
	int NumPoints; /* # of pts in Cont Seg, including both end pts */
	double SumCost;  /* sum of Edge Costs in Slice */
	char **dir_vtx; /* directions of vertices - values they can take are :
                       2 - east
                       4 - north
                       6 - west
                       8 - south
                   On SGI, chars are unsigned. So don't use -ve numbers
                   for directions.    */
	int *tblcc;  /* table built to speed up DP = row*(width+1) */
	int *tblr;  /* table built to speed up edge calc = row*width */
	unsigned char *region;
	int *hist;
	int curr_feature;
	unsigned area[8];   /* area for each of the objects (in pixels) */
	int default_mask_value;
	short roix, roiy, roiw, roih; /* location and size of ROI */
	int phase;
	int output_type;
	char mother_filename[101];
	int overlay_intensity;
	int ilw_min_pts;
	double lsnake_alpha, lsnake_beta, lsnake_gamma;
	double **Gradient;
	CPoint *ControlPoint;
	int train_brush_size, paint_brush_size;
    int               mRows, mCols;      ///< rows/cols of displayed images
	int training_phase;
	bool painting;
	int review_slice;
	bool layout_flag, overlay_flag;
	bool straight_path;


protected:
    static const int  sSpacing;          ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    int               mLastX, mLastY;    ///< last (x,y) of mouse as it's dragged
    double            mScale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
	double            reviewScale;
    bool              mLabels;          ///< toggle display overlay
	bool              switch_images_flag;
	int               num_peek_points;
	int               (*peek_point)[2];
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    Owen2dCanvas ( void );
    Owen2dCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                    const wxPoint &pos, const wxSize &size );

    ~Owen2dCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    void init ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void release ( void );
	int allocControlPoints();
	void paint_mask(int cx, int cy);
	void erase_mask(int cx, int cy);
	void paint_region(int cx, int cy);
	void Erase_PxRegion(int cx, int cy);
	int get_closest_contour_point(int x, int y);
	void delete_open_contour_to_the_end(IMAGE *image, int overlay, int n,
		OPEN_CONTOUR *cont);
	void Erase_Segment(int scrx, int scry) { delete_open_contour_to_the_end(
		&orig, 1, get_closest_contour_point(orig.tblx[scrx - orig.locx],
		orig.tbly[scry - orig.locy]), &o_contour); reload(); }
	void calculate_vert_features(double sum[6], double sumsq[6], float fmin[6],
		float fmax[6], int j, int i);
	void calculate_hzl_features(double sum[6], double sumsq[6], float fmin[6],
		float fmax[6], int j, int i);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    void freeImagesAndBitmaps ( void );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh=NULL,
        const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void loadFile ( const char* const fn );
    void initLUT ( const int which );
    void reload ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar       ( wxKeyEvent&   e );
	//owen funcs
    void OnUndoRedo   ( wxKeyEvent&   e );
	void undo         ( int ind         );
	void redo         ( int ind         );
	void ResetStates  (                 );
	void urhandler    ( int kc          );
	void afterUndo    (                 );
	int clump         ( int index       );
	void handleX      (int index, int act, int subindex);
	void handleU      (int index, int act, int subindex);
	void redraw       (int index        );
	void wipeCanvas   (int index        );
	//end owen funcs
    void OnLeftDown   ( wxMouseEvent& e );
    void OnLeftUp     ( wxMouseEvent& e );
    void OnMiddleDown ( wxMouseEvent& e );
    void OnMiddleUp   ( wxMouseEvent& e );
    void OnMouseMove  ( wxMouseEvent& e );
	void OnMouseWheel ( wxMouseEvent& e );
    void OnPaint      ( wxPaintEvent& e );
    void OnRightDown  ( wxMouseEvent& e );
    void OnRightUp    ( wxMouseEvent& e );
	void OnLeftDClick ( wxMouseEvent& e );

    void paint        ( wxDC* dc );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool   isLoaded    ( const int which ) const;

    double getB        ( const int which ) const;
    int    getCenter   ( const int which ) const;
    double getG        ( const int which ) const;
    bool   getInvert   ( const int which ) const;
    int    getMax      ( const int which ) const;
    int    getMin      ( const int which ) const;
    int    getNoSlices ( const int which ) const;
    double getR        ( const int which ) const;
    int    getSliceNo  ( const int which ) const;    //first displayed slice
    int    getWidth    ( const int which ) const;
    bool   getOverlay  ( void ) const;
	bool   getLabels   ( void ) const;
    double getScale    ( void ) const;

    void   setB        ( const int which, const double b       );
    void   setCenter   ( const int which, const int    center  );
    void   setG        ( const int which, const double g       );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, const int    sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setOverlay  ( const bool   overlay );
	void   setLabels   ( const bool   labels );
	void   setLayout   ( const bool   layout );
    void   setR        ( const int which, const double r       );
    void   setScale    ( const double scale );

	void SetStatusText(const wxString& text, int number)
	{
		m_parent_frame->SetStatusText(text, number);
	}
	int Initialize_Edge_Masks();
	void Reset_training_proc(int accept);
	void clear_temporary_contours_array();
	int InitCircular(X_Point pt0, IMAGE *timg);
	int compute_slices(ViewnixHeader *vh, SLICES *sl);
	void get_anchor_points(int *npoints, int (**point)[2], char filename[]);
	void draw_anchor_points(wxDC & dc, int img_x, int img_y);
	void erase_anchor_points(wxDC & dc);
	void add_anchor_point(int x, int y, OPEN_CONTOUR *cont);
	int iterate_live_snake(IMAGE *timg, int ilw_iterations,
		double alpha, double beta, double gamma);
	int iterate_live_wire(IMAGE *timg, int ilw_iterations, int min_pts);
	int is_vertex_of_contour(int x, int y, OPEN_CONTOUR *cont);
	int find_contour_midpoint(int *x, int *y, int x1, int y1, int x2, int y2,
		OPEN_CONTOUR *cont);
	int contour_length(int x1, int y1, int x2, int y2, OPEN_CONTOUR *cont);
	void initdliststructure(dliststructuretype *L);
	void vertexcoordinate(int *x, int *y, int vertex);
	int vertexposition(X_Point *vertex);
	void UpdateCircular(X_Point pt0, IMAGE *timg);
	void FreeCircular();
	int FindShortestPath(X_Point pt0, X_Point pt1, int flag);
	int FindStraightPath(IMAGE *timg, X_Point pt0, X_Point pt1, int flag);
	int circular_remove_first(X_Point *vertex);
	int circular_update();
	int circular_decrement(X_Point *vertex, unsigned long novocusto);
	int circular_insert(X_Point *vertex, unsigned long cost);
	int InitialPoint_Selected(IMAGE *timg, int eventx, int eventy);
	int Point_Selected(IMAGE *timg, int eventx, int eventy);
	int Is_Point_Selected_Valid(IMAGE *timg, int eventx, int eventy);
	int Live_Wire(IMAGE *timg, int scrx, int scry, int flag);
	int Store_Contour_Seg(IMAGE *timg, int scrx, int scry, int flag);
	int Back_Trace(IMAGE *timg);
	int Erase_Contour_Seg(IMAGE *timg, int scrx, int scry);
	int Close_Phase(IMAGE *timg);
	int Close_Contour(IMAGE *timg);
	int Add_Contours();
	int Cut_Contours();
	int Convert_Contours_to_Vedges();
	int SaveContour2ObjectMask();
	int DeleteContour2ObjectMask();
	int clear_Vedges_array();
	int Alloc_CostArrays();
	void Dealloc_CostArrays();
	int Alloc_Edge_Features();
	void Dealloc_Edge_Features();
	void Alloc_Temp_Arrays();
	void Dealloc_Temp_Arrays();
	int Calc_Edge_Features(int num, int flag, struct FeatureList *list);
	int Calc_Edge_Costs_from_Features(int num, int flag,
		struct FeatureList *list);
	void Calc_Combined_Edge_Costs(struct FeatureList *list, int flag);
	void Calc_Edge_Costs();
	void ReCalc_Edge_Costs();
	int Initialize_Costs(IMAGE *timg, int flag);
	int create_sign_buffer();
	int AbsGrad1(int flag);
	int AbsGrad2(int flag);
	int AbsGrad3(int flag);
	int AbsGrad4(int flag);
	int Density1(int flag);
	int Density2(int flag);
	void Reset_ObjectVertex();
	void reset_object_vertex_of_temparrays();
	int InvLinearTransform(int featr, int flag);
	int InvGaussianTransform(int featr, int flag);
	int LinearTransform(int featr, int flag);
	int GaussianTransform(int featr, int flag);
	int HyperTransform(int featr, int flag);
	int InvHyperTransform(int featr, int flag);
	void Calc_SumCost();
	void LoadFeatureList();
	void LoadFeatureList(const char *feature_def_file);
	void WriteFeatureList();
	double ComputeEnergy(CPoint,CPoint,CPoint,
		double,double,double,double,double);
	double ComputeContinuityEnergy(CPoint, CPoint);
	double ComputeCurvatureEnergy(CPoint, CPoint, CPoint);
	CPoint get_new_point(CPoint, int, int);
	void SnakeDeformation(double **Gradient, int NumberOfControlPoints, CPoint *ControlPoint, double alpha, double beta, double gamma, int nrows, int ncolumns, int niterations);
	void copy_ocontour_into_temp_array();
	void Reset_object_proc();
	void save_mask_proc(int fill_remainder);
	void set_object_vertex_mask_bit(int bit, int value);
	bool mask_index_different();
	int check_and_add_point(int n, /* index of point being filled */
		int x, int y /* coord. in image  coord. system */ );
	void cut_contour(IMAGE *src);
	void build_contour(IMAGE *img);
	void erase_edge_array();
	int load_object_mask(int n);
	int draw_and_save_vertices(int x1, int y1, int x2, int y2
		/* pixels in screen co-ordinates */);
	void add_vertex_to_o_contour(int x, int y);
	int save_vertices(int x1, int y1, int x2, int y2
		/* pixels in screen co-ordinates */ );
	void set_object_mask_bit(int bit /* 0,1,2,3,4,5,6,7, -1 or 8 = all */,
		int  /* 0=OFF, 1=ON */);
	void check_objects(unsigned char *result);
	void check_area(unsigned area[8]);
	int check_mask_file(int *slice, int *volume=NULL);
	int ask_user_what_to_do();
	void check_overlay_color(int overlay_intensity);
	void set_image_location(IMAGE *img, int px, int py);
	void set_image_output_size(IMAGE *img, int w, int h);
	void set_image_table(IMAGE *img);
	void set_image_table2(IMAGE *img);
	void free_image(IMAGE *img);
	void generate_masked_scene(const char *output_file, int out_object);
	static void bin_to_grey(unsigned char *bin_buffer, int length_grey,
		unsigned char *grey_buffer, int min_value, int max_value);
	void set_ilw_control_points();
	void load_object_proc();
	void load_mask_from_BIM();
	void Run_Statistics();
	bool ResetPeekPoints(){if (peek_point)
		{free(peek_point); peek_point=NULL; num_peek_points=0; return true;}
		else return false;}

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS(Owen2dCanvas)
    DECLARE_EVENT_TABLE()
};


extern "C" {
	int VComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
		int* npoints );
}

#endif
