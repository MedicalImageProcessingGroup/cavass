/*
  Copyright 1993-2017, 2019-2020 Medical Image Processing Group
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

//=====================================================================
/**
 * \file:  SurfViewCanvas.cpp
 * \brief  SurfViewCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "limits.h"

#if LONG_MAX < 0xffffffff
#define __uint64 size_t
#else
#define __uint64 unsigned long
#endif

using namespace std;

extern "C" {
    int v_close_curve ( PointInfo** tpoints, int tnpoints, int max_points,
        X_Point** points, int* npoints, X_Point** vertices, int* nvertices );
    int v_draw_line ( int curx, int cury, PointInfo** points, int* npoints,
        int* max_points );
}

double getObjectDiffuseFraction ( cvRenderer *r, int n )
    { return 1-r->getObjectSpecularFraction( n ); }
void   setObjectDiffuseFraction ( cvRenderer *r, double v, int n )
    { r->setObjectSpecularFraction( 1-v, n );     }
void   setAllDiffuseFraction ( cvRenderer *r, double v )
	{ r->setAllSpecularFraction( 1-v );           }
int is_in_polygon(int x, int y, X_Point vertices[], int nvertices);
double angle(double A[3], double C[3], double B[3]);

extern int  gTimerInterval;

//======================================================================
/** \brief RenderEvent class definition.
 *
 *  each instance will be an entry in the event queue.
 */
class RenderEvent {
public:
    enum EventType {
        UNKNOWN,
        AA_ON,       AA_OFF,
        BOX_ON,      BOX_OFF,
        LEFT_DOWN,   LEFT_UP,
        MIDDLE_DOWN, MIDDLE_UP,
        MOTION,
        RERENDER,
        RIGHT_DOWN,  RIGHT_UP,
        SCALE,
        SET_AMBIENT, SET_BACKGROUND,
        SET_OBJECT_COLOR, SET_OBJECT_DIFFUSE, SET_OBJECT_OPACITY,
        SET_OBJECT_SPECULAR, SET_ALL_DIFFUSE, SET_ALL_SPECULAR,
		SET_OBJECT_ON, SET_OBJECT_OFF, SET_OBJECT,
        SET_X, SET_Y, SET_Z, SET_C, TRANSLATE,
        SET_MODE,
        SELECT_OBJECT,
        RESET_ERROR,
        SET_MOBILE, SET_IMMOBILE,
        REMOVE_OBJECT,
        CUT_CURVED, SEPARATE, CUT_PLANE,
        SELECT_LINE_POINT, UNSELECT_LINE_POINT,
        SET_LINE_X, SET_LINE_Y, SET_LINE_Z, SET_LINE_XY,
        SET_PLANE_X, SET_PLANE_Y, SET_PLANE_Z, SET_PLANE_T,
        SET_PLANE_AXIAL, SET_PLANE_SAGITTAL, SET_PLANE_CORONAL,
        SET_FIRST_LOC, SET_LAST_LOC,
        SELECT_MEASUREMENT_POINT, UNSELECT_MEASUREMENT_POINT,
        SET_MARK, ERASE_MARK,
        SET_VIEW,
        TURN_REFLECTIONS_ON, TURN_REFLECTIONS_OFF,
        GET_SLICE_ROI_STATS,
        REMOVE_CUT_OBJECTS,
        RESET_MOVE,
        SET_MATERIAL_OPACITY, SET_MATERIAL_THRESHOLDS,
        SET_MATERIAL_COLOR, SET_SURFACE_STRENGTH, MIP_ON, MIP_OFF,
		SET_GRAY_WINDOW, NEXT_SLICE, PREVIOUS_SLICE
    };
    
    cvRenderer      *mRenderer;
    const EventType  mType;
    const long       mTimestamp;
    const int        mIValue1, mIValue2, mIValue3, mIValue4;
    const double     mDValue1, mDValue2, mDValue3;
    const bool       mInside, mPrimary;
    X_Point         *mVertices;
private:
    //made private so that it can't be generally used
    RenderEvent ( cvRenderer *rr )
        : mRenderer(rr), mType(UNKNOWN), mTimestamp(-1),
          mIValue1(-1), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(-1), mDValue2(-1), mDValue3(-1),
          mInside(false), mPrimary(true), mVertices(0)
    {  }
public:
    RenderEvent ( cvRenderer *rr, EventType type, long timestamp=-1 )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(-1), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(-1), mDValue2(-1), mDValue3(-1),
          mInside(false), mPrimary(true), mVertices(0)
    {  }

    RenderEvent ( cvRenderer *rr, EventType type, long timestamp, int v1, int v2=-1, int v3=-1, int v4=-1 )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(v1), mIValue2(v2), mIValue3(v3), mIValue4(v4),
          mDValue1(-1), mDValue2(-1), mDValue3(-1),
          mInside(false), mPrimary(true), mVertices(0)
    {  }

    RenderEvent ( cvRenderer *rr, EventType type, long timestamp, double v1 )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(-1), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(v1), mDValue2(-1), mDValue3(-1),
          mInside(false), mPrimary(true), mVertices(0)
    {  }

    RenderEvent ( cvRenderer *rr, EventType type, long timestamp, double v1, double v2, double v3, int v4 )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(v4), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(v1), mDValue2(v2), mDValue3(v3),
          mInside(false), mPrimary(true), mVertices(0)
    {  }

    RenderEvent ( cvRenderer *rr, EventType type, long timestamp, double v1, int v2 )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(v2), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(v1), mDValue2(-1), mDValue3(-1),
          mInside(false), mPrimary(true), mVertices(0)
    {  }

    RenderEvent ( cvRenderer *rr, EventType type, long timestamp, X_Point *vertices, int num_vertices, bool inside, double cut_depth, bool primary=true )
        : mRenderer(rr), mType(type), mTimestamp(timestamp),
          mIValue1(num_vertices), mIValue2(-1), mIValue3(-1), mIValue4(-1),
          mDValue1(cut_depth), mDValue2(-1), mDValue3(-1),
		  mInside(inside), mPrimary(primary)
    {
        assert(type==CUT_CURVED || type==SEPARATE|| type==GET_SLICE_ROI_STATS);
        mVertices = (X_Point *)malloc(mIValue1*sizeof(X_Point));
        memcpy(mVertices, vertices, mIValue1*sizeof(X_Point));
    }

    ~RenderEvent ()
    {
        if (mType==CUT_CURVED || mType==SEPARATE) {
            assert( mVertices != 0 );
            free(mVertices);
            mVertices = 0;
        }
    }
};
//----------------------------------------------------------------------
/** \brief EventQueue class definition.
 *
 *  the event queue of render events (see RenderEvent).
 */
class EventQueue {

private:
    deque< RenderEvent* >  mEventQueue;
    wxCriticalSection      mCriticalSection;

    //assumed to be in a critical section w/ mCriticalSection above locked
    inline void removeEvents ( RenderEvent::EventType which ) {
        //remove any and all events of this type
        for ( deque<RenderEvent*>::iterator it = mEventQueue.begin();
            it != mEventQueue.end(); /*intentionally blank*/ )
        {
            if ( (*it)->mType == which ) {
                mEventQueue.erase( it );
                it = mEventQueue.begin();  //must restart at beginning after erase
            } else {
                it++;
            }
        }
    }

public:
    void removeEvents ( cvRenderer *which ) {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        //remove any and all events of this type
        for ( deque<RenderEvent*>::iterator it = mEventQueue.begin();
            it != mEventQueue.end(); /*intentionally blank*/ )
        {
            if ( (*it)->mRenderer == which ) {
                mEventQueue.erase( it );
                it = mEventQueue.begin();  //must restart at beginning after erase
            } else {
                it++;
            }
        }
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp=-1 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::AA_ON  :
            case RenderEvent::AA_OFF :
                //remove any and all antialias changes
                removeEvents( RenderEvent::AA_ON  );
                removeEvents( RenderEvent::AA_OFF );
                //no reason to turn it on if it's already on;
                // no reason to turn it off if it's already off.
                if (what == RenderEvent::AA_OFF) {
                    if (!rr->getAntialias()) {
                        if (rr->mAux!=NULL) { assert( rr->getAntialias() == rr->mAux->getAntialias() ); }
                        return;
                    }
                } else {
                    if (rr->getAntialias()) {
                        if (rr->mAux!=NULL) { assert( rr->getAntialias() == rr->mAux->getAntialias() ); }
                        return;
                    }
                }
                break;

            case RenderEvent::LEFT_UP :
                removeEvents( RenderEvent::LEFT_UP );
                break;

            case RenderEvent::RERENDER :
                //remove any and all rerenders
                removeEvents( RenderEvent::RERENDER );
                break;

            case RenderEvent::SET_OBJECT_ON:
            case RenderEvent::SET_OBJECT_OFF:
            case RenderEvent::SET_MOBILE:
            case RenderEvent::SET_IMMOBILE:
            case RenderEvent::SELECT_OBJECT:
            case RenderEvent::REMOVE_OBJECT:
            case RenderEvent::UNSELECT_LINE_POINT:
            case RenderEvent::SET_PLANE_AXIAL:
            case RenderEvent::SET_PLANE_SAGITTAL:
            case RenderEvent::SET_PLANE_CORONAL:
            case RenderEvent::SET_FIRST_LOC:
            case RenderEvent::SET_LAST_LOC:
            case RenderEvent::UNSELECT_MEASUREMENT_POINT:
            case RenderEvent::TURN_REFLECTIONS_OFF:
            case RenderEvent::REMOVE_CUT_OBJECTS:
            case RenderEvent::RESET_MOVE:
            case RenderEvent::BOX_ON:
            case RenderEvent::BOX_OFF:
			case RenderEvent::MIP_ON:
			case RenderEvent::MIP_OFF:
			case RenderEvent::NEXT_SLICE:
			case RenderEvent::PREVIOUS_SLICE:
                break;

            case RenderEvent::RESET_ERROR:
                removeEvents( RenderEvent::RESET_ERROR );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp, int i1, int i2 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::LEFT_DOWN  :
                //remove any and all antialias changes
                removeEvents( RenderEvent::LEFT_DOWN );
                break;

            case RenderEvent::SELECT_LINE_POINT :
                //remove any and all point selection changes
                removeEvents( RenderEvent::SELECT_LINE_POINT );
                break;

            case RenderEvent::SET_MARK :
                //remove any and all point selection changes
                removeEvents( RenderEvent::SET_MARK );
                break;

            case RenderEvent::ERASE_MARK :
                //remove any and all point selection changes
                removeEvents( RenderEvent::ERASE_MARK );
                break;

            case RenderEvent::TURN_REFLECTIONS_ON :
                //remove any and all point selection changes
                removeEvents( RenderEvent::TURN_REFLECTIONS_ON );
                break;

            case RenderEvent::CUT_PLANE :
                //remove any and all point selection changes
                removeEvents( RenderEvent::CUT_PLANE );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, i1, i2 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp, int i1, int i2, int i3 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_AMBIENT :
                //remove any and all ambient changes
                removeEvents( RenderEvent::SET_AMBIENT );
                break;

            case RenderEvent::SET_BACKGROUND :
                //remove any and all background changes
                removeEvents( RenderEvent::SET_BACKGROUND );
                break;

            case RenderEvent::SELECT_OBJECT :
                //remove any and all object selection changes
                removeEvents( RenderEvent::SELECT_OBJECT );
                break;

            case RenderEvent::SELECT_MEASUREMENT_POINT :
                //remove any and all point selection changes
                removeEvents( RenderEvent::SELECT_MEASUREMENT_POINT );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, i1, i2, i3 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp,
                   int i1, int i2, int i3, int i4 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_OBJECT_COLOR :
                //remove any and all object color changes
                removeEvents( RenderEvent::SET_OBJECT_COLOR );
                break;

            case RenderEvent::SET_MATERIAL_COLOR :
                //remove any and all material color changes
                removeEvents( RenderEvent::SET_MATERIAL_COLOR );
                break;

			case RenderEvent::SET_GRAY_WINDOW:
				removeEvents( RenderEvent::SET_GRAY_WINDOW );
				break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, i1, i2, i3, i4 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp, double d1 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_X :
            case RenderEvent::SET_Y :
            case RenderEvent::SET_Z :
                //remove any and all viewpoint changes
                removeEvents( RenderEvent::SET_X );
                removeEvents( RenderEvent::SET_Y );
                removeEvents( RenderEvent::SET_Z );
                //no change in viewpoint?
                if (d1 == 0)    return;
                break;

            case RenderEvent::SET_C :
            case RenderEvent::TRANSLATE :
                //remove any and all object motion
                removeEvents( RenderEvent::SET_C );
                removeEvents( RenderEvent::TRANSLATE );
                //no change in position?
                if (d1 == 0)    return;
                break;

            case RenderEvent::SCALE :
                //remove any and all scale changes
                removeEvents( RenderEvent::SCALE );
                //no reason to change the scale to its current value.
                if (d1 == rr->getScale()) {
                    if (rr->mAux!=NULL) { assert( rr->getScale() == rr->mAux->getScale() ); }
                    return;
                }
                break;

            case RenderEvent::SET_OBJECT:
                removeEvents( RenderEvent::SET_OBJECT );
                break;

            case RenderEvent::SET_MODE:
                removeEvents( RenderEvent::SET_MODE );
                break;

            case RenderEvent::SET_LINE_X :
            case RenderEvent::SET_LINE_Y :
            case RenderEvent::SET_LINE_Z :
            case RenderEvent::SET_PLANE_X :
            case RenderEvent::SET_PLANE_Y :
            case RenderEvent::SET_PLANE_Z :
            case RenderEvent::SET_PLANE_T :
                //remove any and all line orientation changes
                removeEvents( RenderEvent::SET_LINE_X );
                removeEvents( RenderEvent::SET_LINE_Y );
                removeEvents( RenderEvent::SET_LINE_Z );
                removeEvents( RenderEvent::SET_PLANE_X );
                removeEvents( RenderEvent::SET_PLANE_Y );
                removeEvents( RenderEvent::SET_PLANE_Z );
                removeEvents( RenderEvent::SET_PLANE_T );
                //no change in line orientation?
                if (d1 == 0)    return;
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, d1 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp,
                   double d1, double d2 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_LINE_XY :
                //remove any and all line location changes
                removeEvents( RenderEvent::SET_LINE_XY );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, d1, d2, -1.0, -1 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp,
                   double d1, double d2, double d3, int d4 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_OBJECT_DIFFUSE :
                //remove any and all diffuse changes
                removeEvents( RenderEvent::SET_OBJECT_DIFFUSE );
                break;

            case RenderEvent::SET_OBJECT_SPECULAR :
                //remove any and all specular changes
                removeEvents( RenderEvent::SET_OBJECT_SPECULAR );
                break;

            case RenderEvent::SET_ALL_DIFFUSE :
                //remove any and all diffuse changes
                removeEvents( RenderEvent::SET_ALL_DIFFUSE );
                break;

            case RenderEvent::SET_ALL_SPECULAR :
                //remove any and all specular changes
                removeEvents( RenderEvent::SET_ALL_SPECULAR );
                break;

            case RenderEvent::SET_VIEW :
                //remove any and all view changes
                removeEvents( RenderEvent::SET_VIEW );
                break;

            case RenderEvent::SET_MATERIAL_THRESHOLDS :
                //remove any and all threshold changes
                removeEvents( RenderEvent::SET_MATERIAL_THRESHOLDS );
                break;

            case RenderEvent::SET_SURFACE_STRENGTH :
                //remove any and all surface strength changes
                removeEvents( RenderEvent::SET_SURFACE_STRENGTH );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, d1, d2, d3, d4 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp, double d1, int i1 )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::SET_OBJECT_OPACITY :
                //remove any and all opacity changes
                removeEvents( RenderEvent::SET_OBJECT_OPACITY );
                break;

            case RenderEvent::SET_MATERIAL_OPACITY :
                //remove any and all opacity changes
                removeEvents( RenderEvent::SET_MATERIAL_OPACITY );
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, d1, i1 );
        mEventQueue.push_back( re );
    }

    void enqueue ( cvRenderer *rr, RenderEvent::EventType what, long timestamp, X_Point *vertices, int num_vertices, bool inside, double cut_depth, bool primary=true )
    {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        switch (what) {
            case RenderEvent::CUT_CURVED:
            case RenderEvent::SEPARATE:
            case RenderEvent::GET_SLICE_ROI_STATS:
                break;

            default :
                assert( 0 );  //shouldn't enqueue this event w/out proper value(s)!
                return;
                break;
        }
        RenderEvent*  re = new RenderEvent( rr, what, timestamp, vertices,
            num_vertices, inside, cut_depth, primary );
        mEventQueue.push_back( re );
    }

    RenderEvent* dequeue ( void ) {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        if (mEventQueue.empty())    return NULL;
        RenderEvent*  re = mEventQueue.front();
        mEventQueue.pop_front();
        return re;
    }

    RenderEvent* dequeue ( cvRenderer *rr ) {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
        if (mEventQueue.empty())    return NULL;
        RenderEvent*  re = mEventQueue.front();
        if (re->mRenderer == rr)
        {
            mEventQueue.pop_front();
            return re;
        }
        else
            return NULL;
    }

};

static EventQueue  eventQueue;
//======================================================================
const int  SurfViewCanvas::sSpacing=1;  ///< space, in pixels, between each slice (on the screen)
//----------------------------------------------------------------------
SurfViewCanvas::SurfViewCanvas ( void ) : manip_flag(false) {
    puts("SurfViewCanvas()");
}
//----------------------------------------------------------------------
SurfViewCanvas::SurfViewCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size, bool manipulate )
  : MainCanvas ( parent, parent_frame, id, pos, size ), manip_flag(manipulate)
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
    mFileOrDataCount = 0;
    m_scale          = 1.0;
    m_overlay        = false;
    m_rows           = 0;
    m_cols           = 0;
    m_images         = (wxImage**)NULL;
    m_bitmaps        = (wxBitmap**)NULL;
    m_tx = m_ty      = 0;
    mRotateOn        = !manip_flag;
    mRotateMode      = 0;
    mLastRotateMode  = 0;
    mRotateAngle     = 0;
    mLineDX          = 0;
    mLineDY          = 0;

    mRenderer        = NULL;
    mRenderThread    = NULL;
    mInterruptRenderingFlag = 0;
    mOverallXSize    = mOverallYSize    = mOverallZSize    = 0;
    mRotatingFrom[0] = mRotatingFrom[1] = mRotatingFrom[2] = 0;
    mWhichMode       = VIEW;
    mControlState    = 0;
    vertices         = NULL;
    nvertices        = 0;
    inside           = false;
    cut_depth        = 0;
    mLinePixelCount  = 0;
    mLinePixels      = NULL;
    first_loc        = 0;
    last_loc         = 0;
    out_pixel_size   = 0;
    key_poses        = 0;
    key_pose         = NULL;
    key_pose_array_size= 0;
    preview_all_poses= true;
    preview_icon_mode= false;
    preview_key_pose = -1;
    preview_view     = -1;
    preview_frame_displayed= -1;
    total_density    = 0;
    mean_density     = 0;
    standard_deviation= 0;
    min_density      = 0;
    max_density      = 0;
    stats_valid      = false;
	m_MIP_Invert     = false;

    lastX = lastY = -1;
    mCavassData = new CavassData();
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );
}
//----------------------------------------------------------------------
SurfViewCanvas::~SurfViewCanvas ( void ) {
    cout << "SurfViewCanvas::~SurfViewCanvas" << endl;
    wxLogMessage( "SurfViewCanvas::~SurfViewCanvas" );
    release();
}
//----------------------------------------------------------------------
void SurfViewCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
static wxMutex  s_mutexProtectingTheGlobalData;

void SurfViewCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxSafeYield(NULL, true);
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount==0 );

    //begin critical section -------------------------------------------
    s_mutexProtectingTheGlobalData.Lock();
    assert( s_mutexProtectingTheGlobalData.IsOk() );

    char *file_list=(char *)malloc(strlen(fn)+1);
    strcpy(file_list, fn);
    mRenderer = new cvRenderer(&file_list, 1);
    bool  stereoOn = false;
    if (Preferences::getStereoMode() != Preferences::StereoModeOff) {
        mRenderer->mAux = new cvRenderer( &file_list, 1 );
        assert( mRenderer->mAux != NULL );
        stereoOn = true;
    }
    free(file_list);
    mRenderer->setAntialias( false );
    if (stereoOn)    mRenderer->mAux->setAntialias( false );
    if (manip_flag) {
        mWhichMode = mRenderer->image_mode = WITH_ICON;
        if (stereoOn)    mRenderer->mAux->image_mode = WITH_ICON;
    }

    char*  data = mRenderer->render2( mOverallXSize, mOverallYSize, mInterruptRenderingFlag );
    assert( data != NULL );
    handleStereo( data );

    mOverallZSize    = 1;
    mFileOrDataCount = 1;
    mCavassData      = NULL;
    m_rows = m_cols = 1;
    char *data2 = NULL;
    if (mRenderer->image_mode==SEPARATE || mRenderer->image_mode==SLICE)
    {
        m_cols = 2;
        data2 = mRenderer->get_recolored_image2( mOverallXSize, mOverallYSize );
        /** \todo do same for aux and create stereo pair */
    }

    mRotatingFrom[0] = mRenderer->glob_angle[0];
    mRotatingFrom[1] = mRenderer->glob_angle[1];
    mRotatingFrom[2] = mRenderer->glob_angle[2];

    s_mutexProtectingTheGlobalData.Unlock();
    //end critical section ---------------------------------------------

    reload( (unsigned char*)data, (unsigned char*)data2,
            mOverallXSize, mOverallYSize );
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void SurfViewCanvas::loadFiles ( wxArrayString &filenames ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxSafeYield(NULL, true);
    release();
    if (filenames.Count() == 0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount==0 );

    //begin critical section -------------------------------------------
    s_mutexProtectingTheGlobalData.Lock();
    assert( s_mutexProtectingTheGlobalData.IsOk() );
    char **file_list=(char **)malloc(filenames.Count()*sizeof(char *));
    for (unsigned int j=0; j<filenames.Count(); j++)
    {
        file_list[j] = (char *)malloc(strlen((const char *)filenames[j].c_str())+1);
        strcpy(file_list[j], (const char *)filenames[j].c_str());
    }
    mRenderer = new cvRenderer(file_list, filenames.Count());
    bool  stereoOn = false;
    if (Preferences::getStereoMode() != Preferences::StereoModeOff) {
        mRenderer->mAux = new cvRenderer( file_list, filenames.Count() );
        assert( mRenderer->mAux != NULL );
        stereoOn = true;
    }
    for (unsigned int j=0; j<filenames.Count(); j++)
        free(file_list[j]);
    free(file_list);
    mRenderer->setAntialias( false );
    if (stereoOn)    mRenderer->mAux->setAntialias( false );
    if (manip_flag) {
        mRenderer->image_mode = WITH_ICON;
        if (stereoOn)    mRenderer->mAux->image_mode = WITH_ICON;
    }

    char*  data = mRenderer->render2( mOverallXSize, mOverallYSize, mInterruptRenderingFlag );
    assert( data != NULL );
    handleStereo( data );

    mOverallZSize    = 1;
    mFileOrDataCount = 1;
    mCavassData      = NULL;
    m_rows = m_cols = 1;
    char *data2 = NULL;
    if (mRenderer->image_mode==SEPARATE || mRenderer->image_mode==SLICE)
    {
        m_cols = 2;
        data2= mRenderer->get_recolored_image2( mOverallXSize, mOverallYSize );
        /** \todo do same for aux and create stereo pair */
    }

    mRotatingFrom[0] = mRenderer->glob_angle[0];
    mRotatingFrom[1] = mRenderer->glob_angle[1];
    mRotatingFrom[2] = mRenderer->glob_angle[2];

    s_mutexProtectingTheGlobalData.Unlock();
    //end critical section ---------------------------------------------

    reload( (unsigned char*)data, (unsigned char*)data2,
            mOverallXSize, mOverallYSize );
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void SurfViewCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
class RenderingEventData {
public:
    unsigned char*  mSlice1;
    unsigned char*  mSlice2;

    int             mCurrent_m_rows;
    int             mCurrent_m_cols;
    double          mCurrent_m_scale;
    int             mCurrent_mOverallXSize;
    int             mCurrent_mOverallYSize;

    RenderingEventData ( ) {
        mSlice1 = mSlice2 = 0;
    }
};
//----------------------------------------------------------------------
void SurfViewCanvas::OnDoRender ( wxCommandEvent& e ) {
    wxMutexLocker  lock( s_mutexProtectingTheGlobalData );  //critical section
                                                            //vvvvvvvvvvvvvvvv
    assert( s_mutexProtectingTheGlobalData.IsOk() );

    RenderingEventData*  re = (RenderingEventData*) e.GetClientData();
    assert( re != NULL );
    reloadNow( re );
}
//----------------------------------------------------------------------
BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE( wxEVT_MY_CUSTOM_COMMAND, 7777 )
END_DECLARE_EVENT_TYPES()
//----------------------------------------------------------------------
void SurfViewCanvas::reload ( unsigned char* slice1, unsigned char* slice2,
                              int overallXSize, int overallYSize )
{
#ifdef  WIN32
    //wxLogMessage( "in SurfViewCanvas::reload tid=%d", GetCurrentThreadId() );
#else
    //calls to wxLogMessage from other than the main thread under Linux,
    //  causes cavass to crash
    //cout << "in SurfViewCanvas::reload tid=" << gettid() << endl;
#endif
    //post a custom event that will cause the reload to occur
    // (typically in the main thread)
    /** \todo free this in the future */
    RenderingEventData*  re = new RenderingEventData();
#if 0
    re->mSlice1                = slice1;
    re->mSlice2                = slice2;
#else
    re->mSlice1 = re->mSlice2 = NULL;
    if (slice1) {
        //we need to make a copy because the copy will be freed when the
        // wxImage is freed.
        unsigned char*  sliceCopy = (unsigned char*)malloc( 3 * overallXSize
            * overallYSize );
        assert( sliceCopy != NULL );
		if (m_MIP_Invert)
			for (int j=0; j<3*overallXSize*overallYSize; j++)
				sliceCopy[j] = 255-slice1[j];
		else
	        memcpy( sliceCopy, slice1, 3 * overallXSize * overallYSize );

        re->mSlice1 = sliceCopy;
    }
    if (slice2) {
        unsigned char*  sliceCopy = (unsigned char*)malloc( 3 * overallXSize
            * overallYSize );
        assert( sliceCopy != NULL );
        assert( slice2 != 0 );
		if (m_MIP_Invert)
			for (int j=0; j<3*overallXSize*overallYSize; j++)
				sliceCopy[j] = 255-slice2[j];
		else
        	memcpy( sliceCopy, slice2, 3 * overallXSize * overallYSize );

        re->mSlice2 = sliceCopy;
    }
#endif
    re->mCurrent_m_rows        = m_rows;
    re->mCurrent_m_cols        = m_cols;
    re->mCurrent_m_scale       = m_scale;
    re->mCurrent_mOverallXSize = overallXSize;
    re->mCurrent_mOverallYSize = overallYSize;

    wxCommandEvent  customEvent( wxEVT_MY_CUSTOM_COMMAND );
    customEvent.SetClientData( re );
    ::wxPostEvent( this, customEvent );
#ifdef  WIN32
    //wxLogMessage( "out SurfViewCanvas::reload tid=%d", GetCurrentThreadId() );
#else
    //calls to wxLogMessage from other than the main thread under Linux,
    //  causes cavass to crash
    //cout << "out SurfViewCanvas::reload tid=" << gettid() << endl;
#endif
}
//----------------------------------------------------------------------
void SurfViewCanvas::reloadNow ( RenderingEventData* re )
{
    assert( m_rows  == re->mCurrent_m_rows );
    assert( m_cols  == re->mCurrent_m_cols );
    assert( m_scale == re->mCurrent_m_scale );
    //assert( mOverallXSize == re->mCurrent_mOverallXSize );
    //assert( mOverallYSize == re->mCurrent_mOverallYSize );
    mOverallXSize = re->mCurrent_mOverallXSize;
    mOverallYSize = re->mCurrent_mOverallYSize;

    freeImagesAndBitmaps();
    int  k;
    m_images  = (wxImage**)malloc(  m_cols * m_rows * sizeof(wxImage*) );
    assert( m_images != NULL );
    for (k=0; k<m_cols*m_rows; k++)    m_images[k]=NULL;
    
    m_bitmaps = (wxBitmap**)malloc( m_cols * m_rows * sizeof(wxBitmap*) );
    assert( m_bitmaps != NULL );
    for (k=0; k<m_cols*m_rows; k++)    m_bitmaps[k]=NULL;
    unsigned char*  sliceCopy = re->mSlice1;
    assert( sliceCopy != NULL );

    k=0;
    m_images[k] = new wxImage( mOverallXSize, mOverallYSize, sliceCopy );
    if (m_scale!=1.0) {
        m_images[k]->Rescale( (int)(ceil(m_scale*mOverallXSize)),
                              (int)(ceil(m_scale*mOverallYSize)) );
    }
    m_bitmaps[k] = new wxBitmap( (const wxImage&)*m_images[k] );
    if (m_cols * m_rows > 1)
    {
#if 0
        sliceCopy= (unsigned char*)malloc( 3 * mOverallXSize * mOverallYSize );
        assert( sliceCopy != NULL );
        assert( slice2 != 0 );
        memcpy( sliceCopy, slice2, 3 * mOverallXSize * mOverallYSize );
#else
        sliceCopy = re->mSlice2;
        assert( sliceCopy != NULL );
#endif
        k=1;
        m_images[k] = new wxImage( mOverallXSize, mOverallYSize, sliceCopy );
        if (m_scale!=1.0) {
            m_images[k]->Rescale( (int)(ceil(m_scale*mOverallXSize)),
                                  (int)(ceil(m_scale*mOverallYSize)) );
        }
        m_bitmaps[k] = new wxBitmap( (const wxImage&)*m_images[k] );
    }
    Refresh();
}
//----------------------------------------------------------------------
void SurfViewCanvas::mapWindowToData ( int wx, int wy,
                                       int& x, int& y, int& z )
{
}
//======================================================================
class RenderThread : public wxThread {
public:
    EventQueue*      mQ;
    SurfViewCanvas*  mCanvas;
    bool             mTimeToStop;
    double           xAngle, yAngle, zAngle;

    RenderThread ( SurfViewCanvas* canvas, EventQueue* q )
        : wxThread(wxTHREAD_DETACHED), mQ(q), mCanvas(canvas),
          mTimeToStop(false)
    {
        assert( q != NULL );
        xAngle = yAngle = zAngle=0;
        Create();
    }

    RenderThread ( SurfViewCanvas* canvas, EventQueue* q, int key, double scale )
        : wxThread(wxTHREAD_DETACHED), mQ(q), mCanvas(canvas),
          mTimeToStop(false)
    {

        assert( q != NULL );
        canvas->mRenderer->setScale( scale );
        xAngle = yAngle = zAngle=0;
        Create();

    }

    ExitCode Entry ( void ) {

        //do thread work
        wxMutex mutex;
        double  from_matrix[3][3], rot_matrix[3][3];
        int     rot_axis;
        double  whichAngleValue;
        char*   data = NULL, *data2 = NULL;
        #define  incr  0.1

        for ( ; !mTimeToStop; ) {
            Yield();
            RenderEvent*  re = mQ->dequeue(mCanvas->mRenderer);
            if (!re) {
                Yield();  Yield();  Yield();  Yield();  Yield();
                continue;
            }
            for (wxMutexError err=wxMUTEX_UNLOCKED; err!=wxMUTEX_NO_ERROR;
                    err=mutex.TryLock())
            {
                Yield();  Yield();  Yield();  Yield();  Yield();
            }

            assert(re->mRenderer == mCanvas->mRenderer);
            int  overallXSize=0, overallYSize=0;
            switch (re->mType) {

                case RenderEvent::BOX_OFF :
                case RenderEvent::BOX_ON :
                    re->mRenderer->box = re->mType==RenderEvent::BOX_ON;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::AA_OFF :
                    re->mRenderer->setAntialias( false );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::AA_ON :
                    re->mRenderer->setAntialias( true );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::MIP_OFF :
                    re->mRenderer->setMIP( false );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::MIP_ON :
                    re->mRenderer->setMIP( true );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::MOTION :
                    break;

                case RenderEvent::RESET_ERROR :
                    re->mRenderer->error_flag = false;
                    break;

                case RenderEvent::RERENDER :
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SCALE :
                    re->mRenderer->setScale( re->mDValue1 );
                    if (re->mRenderer->image_mode == SLICE)
                        re->mRenderer->do_plane_slice();
                    data = re->mRenderer->render2( overallXSize, overallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            overallXSize, overallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2, overallXSize, overallYSize );
                    break;

                case RenderEvent::SET_AMBIENT :
                    re->mRenderer->setAmbientLight( re->mIValue1, re->mIValue2, re->mIValue3 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_BACKGROUND :
                    re->mRenderer->setBackgroundColor( re->mIValue1, re->mIValue2, re->mIValue3 );
                    data = re->mRenderer->get_recolored_image( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::UNSELECT_LINE_POINT :
                    assert(re->mRenderer->points_selected > 0);
                    re->mRenderer->points_selected--;
                    re->mRenderer->line = FALSE;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SELECT_LINE_POINT :
                    {
                        assert(re->mRenderer->points_selected < 2);
                        double rotation[3][3];
                        double *plan_coords=re->mRenderer->selected_points[
                            re->mRenderer->points_selected];
                        int z = re->mRenderer->closest_z( re->mIValue1,
                            re->mIValue2, re->mRenderer->main_image );
                        if (z == 0)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        plan_coords[0] = re->mIValue1-
                            re->mRenderer->main_image->width/2;
                        plan_coords[1] = re->mIValue2-
                            re->mRenderer->main_image->height/2;
                        plan_coords[2] = z;
                        /* convert to physical units */
                        plan_coords[0] /= re->mRenderer->scale;
                        plan_coords[1] /= re->mRenderer->scale;
                        plan_coords[2] = (MIDDLE_DEPTH-plan_coords[2])/
                            re->mRenderer->depth_scale;
                        // convert from image coordinates to plan coordinates
                        ::AtoM(rotation, re->mRenderer->glob_angle[0],
                            re->mRenderer->glob_angle[1],
                            re->mRenderer->glob_angle[2]);
                        ::vector_matrix_multiply(plan_coords, plan_coords,
                            rotation);
                        re->mRenderer->points_selected++;
                        if (re->mRenderer->points_selected < 2)
                            break;
                        if (plan_coords[0]==plan_coords[-3] &&
                                plan_coords[1]==plan_coords[-2] &&
                                plan_coords[2]==plan_coords[-1])
                        {
                            re->mRenderer->points_selected = 1;
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        double line_direction[3], point_displacement;
                        ::vector_subtract(line_direction, plan_coords,
                            plan_coords-3);
                        ::VtoA(re->mRenderer->line_angle,
                            re->mRenderer->line_angle+1, line_direction);
                        ::AtoV(line_direction, re->mRenderer->line_angle[0],
                            re->mRenderer->line_angle[1]);
                        point_displacement = re->mRenderer->
                            selected_points[0][0]*line_direction[0]+
                            re->mRenderer->selected_points[0][1]*
                            line_direction[1]+re->mRenderer->
                            selected_points[0][2]*line_direction[2];
                        re->mRenderer->line_displacement[0] =
                            re->mRenderer->selected_points[0][0]-
                            point_displacement*line_direction[0];
                        re->mRenderer->line_displacement[1] =
                            re->mRenderer->selected_points[0][1]-
                            point_displacement*line_direction[1];
                        re->mRenderer->line_displacement[2] =
                            re->mRenderer->selected_points[0][2]-
                            point_displacement*line_direction[2];
                    }
                    re->mRenderer->line = TRUE;
                    if (mCanvas->mLinePixelCount)
                        free(mCanvas->mLinePixels);
                    re->mRenderer->get_line(mCanvas->mLinePixels,
                        mCanvas->mLinePixelCount);
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::UNSELECT_MEASUREMENT_POINT :
                    assert(re->mRenderer->nmeasurement_points > 0);
                    re->mRenderer->nmeasurement_points--;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    data2 = re->mRenderer->get_recolored_image2(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SELECT_MEASUREMENT_POINT :
                    if (re->mRenderer->nmeasurement_points >=
                            re->mRenderer->measurement_point_array_size)
                    {
                        double (*mp)[3]=(double (*)[3])(
                            re->mRenderer->nmeasurement_points?
                            realloc(re->mRenderer->measurement_point,
                            sizeof(*mp)*(re->mRenderer->nmeasurement_points
                            +1)): malloc(sizeof(*mp)));
                        if (mp == NULL)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        re->mRenderer->measurement_point = mp;
                        re->mRenderer->measurement_point_array_size =
                            re->mRenderer->nmeasurement_points+1;
                    }
                    if (re->mIValue3)
                    {
                        double rotation[3][3];
                        double *plan_coords = re->mRenderer->measurement_point[
                            re->mRenderer->nmeasurement_points];
                        int z = re->mRenderer->closest_z( re->mIValue1,
                            re->mIValue2, re->mRenderer->main_image );
                        if (z == 0)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        plan_coords[0] = re->mIValue1-
                            re->mRenderer->main_image->width/2;
                        plan_coords[1] = re->mIValue2-
                            re->mRenderer->main_image->height/2;
                        plan_coords[2] = z;
                        /* convert to physical units */
                        plan_coords[0] /= re->mRenderer->scale;
                        plan_coords[1] /= re->mRenderer->scale;
                        plan_coords[2] = (MIDDLE_DEPTH-plan_coords[2])/
                            re->mRenderer->depth_scale;
                        // convert from image coordinates to plan coordinates
                        ::AtoM(rotation, re->mRenderer->glob_angle[0],
                            re->mRenderer->glob_angle[1],
                            re->mRenderer->glob_angle[2]);
                        ::vector_matrix_multiply(
                            re->mRenderer->measurement_point[
                            re->mRenderer->nmeasurement_points],
                            plan_coords, rotation);
                    }
                    else
                    {
                        assert(re->mRenderer->slice_list);
                        Slice_image *sl=re->mRenderer->slice_list;
                        re->mRenderer->measurement_point[
                            re->mRenderer->nmeasurement_points][0] =
                            sl->plane_normal[0]*sl->plane_displacement+
                            (re->mIValue1-sl->image->width/2)*sl->x_axis[0]+
                            (re->mIValue2-sl->image->width/2)*sl->y_axis[0];
                        re->mRenderer->measurement_point[
                            re->mRenderer->nmeasurement_points][1] =
                            sl->plane_normal[1]*sl->plane_displacement+
                            (re->mIValue1-sl->image->width/2)*sl->x_axis[1]+
                            (re->mIValue2-sl->image->width/2)*sl->y_axis[1];
                        re->mRenderer->measurement_point[
                            re->mRenderer->nmeasurement_points][2] =
                            sl->plane_normal[2]*sl->plane_displacement+
                            (re->mIValue1-sl->image->width/2)*sl->x_axis[2]+
                            (re->mIValue2-sl->image->width/2)*sl->y_axis[2];
                    }
                    re->mRenderer->nmeasurement_points++;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    data2 = re->mRenderer->get_recolored_image2(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_MARK :
                {
                    double (*temp_mark)[3], rotation[3][3];
                    Shell *object;
                    Virtual_object *vobj;
                    int obj_n;
                    obj_n = re->mRenderer->closest_object(
                        re->mIValue1-re->mRenderer->main_image->width/2,
                        re->mIValue2-re->mRenderer->main_image->height/2, 1);
                    if (obj_n == 0)
                    {
                        re->mRenderer->error_flag = true;
                        break;
                    }
                    vobj = re->mRenderer->object_from_number(obj_n);
                    object = re->mRenderer->actual_object(vobj);
                    if (vobj == object->reflection)
                    {
                        re->mRenderer->error_flag = true;
                        break;
                    }
                    if (object->mark_array_size == object->marks)
                    {	temp_mark = (triple *)
                            (	object->mark_array_size==0
                                ?	malloc((object->marks+1)*3*sizeof(double))
                                :	realloc(object->mark,
                                        (object->marks+1)*3*sizeof(double))
                            );
                        if (temp_mark == NULL)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        object->mark = temp_mark;
                        object->mark_array_size = object->marks+1;
                    }
                    double *plan_coords = object->mark[object->marks];
                    int z = re->mRenderer->closest_z( re->mIValue1,
                        re->mIValue2, re->mRenderer->main_image );
                    if (z == 0)
                    {
                        re->mRenderer->error_flag = true;
                        break;
                    }
                    plan_coords[0] =
                        re->mIValue1-re->mRenderer->main_image->width/2;
                    plan_coords[1] =
                        re->mIValue2-re->mRenderer->main_image->height/2;
                    plan_coords[2] = z;
                    /* convert to physical units */
                    plan_coords[0] /= re->mRenderer->scale;
                    plan_coords[1] /= re->mRenderer->scale;
                    plan_coords[2] = (MIDDLE_DEPTH-plan_coords[2])/
                        re->mRenderer->depth_scale;
                    // convert from image coordinates to plan coordinates
                    ::AtoM(rotation, re->mRenderer->glob_angle[0],
                        re->mRenderer->glob_angle[1],
                        re->mRenderer->glob_angle[2]);
                    ::vector_matrix_multiply(plan_coords,plan_coords,rotation);

                    /* convert from plan coordinates to object coordinates */
                    object->mark[object->marks][0] -= object->displacement[0];
                    object->mark[object->marks][1] -= object->displacement[1];
                    object->mark[object->marks][2] -= object->displacement[2];
                    ::AtoM(rotation, object->angle[0], object->angle[1],
                        object->angle[2]);
                    ::vector_matrix_multiply(object->mark[object->marks],
                        object->mark[object->marks], rotation);
                    object->marks++;
                    re->mRenderer->marked_object = object;
                }
                    re->mRenderer->marks = TRUE;
                    re->mRenderer->new_mark_is_set = TRUE;
                    re->mRenderer->mark_is_erased = FALSE;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    data2 = re->mRenderer->get_recolored_image2(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::ERASE_MARK :
                    if (!re->mRenderer->marks)
                    {
                        re->mRenderer->error_flag = true;
                        break;
                    }
                {
                    int this_mark=0, this_x, this_y;
                    __uint64 dist_sqr=0;
                    Shell *object=NULL, *this_object;
                    bool mark_found = false;
                    for (this_object=re->mRenderer->object_list; this_object;
                            this_object=this_object->next)
                        for (int t_mark=0; t_mark<this_object->marks; t_mark++)
                            if (re->mRenderer->get_mark_x_y(&this_x, &this_y,
                                    this_object, this_object->mark[t_mark],
                                    re->mRenderer->main_image))
                            {
                                __uint64 dx, dy;
                                dx = abs(this_x-re->mIValue1);
                                dy = abs(this_y-re->mIValue2);
                                __uint64 this_dist_sqr = dx*dx+dy*dy;
                                if (!mark_found || this_dist_sqr<dist_sqr)
                                {
                                    object = this_object;
                                    this_mark = t_mark;
                                    dist_sqr = this_dist_sqr;
                                    mark_found = true;
                                }
                            }
                    if (!mark_found || dist_sqr>400)
                    {
                        re->mRenderer->error_flag = true;
                        break;
                    }
                    re->mRenderer->marked_object = object;
                    re->mRenderer->erased_mark[0] = object->mark[this_mark][0];
                    re->mRenderer->erased_mark[1] = object->mark[this_mark][1];
                    re->mRenderer->erased_mark[2] = object->mark[this_mark][2];
                    for (; this_mark<object->marks-1; this_mark++)
                    {
                        object->mark[this_mark][0] =
                            object->mark[this_mark+1][0];
                        object->mark[this_mark][1] =
                            object->mark[this_mark+1][1];
                        object->mark[this_mark][2] =
                            object->mark[this_mark+1][2];
                    }
                    object->marks--;
                }
                    re->mRenderer->new_mark_is_set = FALSE;
                    re->mRenderer->mark_is_erased = TRUE;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    data2 = re->mRenderer->get_recolored_image2(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SELECT_OBJECT :
                    {
                        int n = re->mRenderer->closest_object( re->mIValue1,
                            re->mIValue2, re->mIValue3 );
                        if (n == 0)
                        {
                            //@ wxMessageBox("No object at that location.");
                            if (re->mRenderer->selected_object == 0)
                                re->mRenderer->selected_object =
                                    re->mRenderer->old_selected_object;
                        }
                        else
                        {
                            re->mRenderer->selected_object =
                                re->mRenderer->object_label(
                                    re->mRenderer->object_from_number(n));
                            mCanvas->stats_valid = false;
                            if (mCanvas->mWhichMode==
                                    SurfViewCanvas::ROI_STATISTICS &&
                                    re->mRenderer->get_object_roi_stats(
                                    &mCanvas->total_density,
                                    &mCanvas->mean_density,
                                    &mCanvas->standard_deviation,
                                    &mCanvas->min_density,
                                    &mCanvas->max_density,
                                    cvRenderer::cvCheckInterrupt)==DONE)
                                mCanvas->stats_valid = true;
                            data = re->mRenderer->get_recolored_image(
                                mCanvas->mOverallXSize,
                                mCanvas->mOverallYSize );
                            if (re->mRenderer->image_mode==SEPARATE ||
                                    re->mRenderer->image_mode==SLICE)
                                data2 = re->mRenderer->get_recolored_image2(
                                    mCanvas->mOverallXSize,
                                    mCanvas->mOverallYSize );
                            else
                                data2 = NULL;
                            mCanvas->reload( (unsigned char*)data,
                                (unsigned char*)data2,
                                mCanvas->mOverallXSize,
                                mCanvas->mOverallYSize );
                        }
                    }
                    break;

                case RenderEvent::TURN_REFLECTIONS_ON :
                    {
                        double rotation[3][3];
                        double plan_coords[3];
                        int z = re->mRenderer->closest_z( re->mIValue1,
                            re->mIValue2, re->mRenderer->main_image );
                        if (z == 0)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        plan_coords[0] = re->mIValue1-
                            re->mRenderer->main_image->width/2;
                        plan_coords[1] = re->mIValue2-
                            re->mRenderer->main_image->height/2;
                        plan_coords[2] = z;
                        /* convert to physical units */
                        plan_coords[0] /= re->mRenderer->scale;
                        plan_coords[1] /= re->mRenderer->scale;
                        plan_coords[2] = (MIDDLE_DEPTH-plan_coords[2])/
                            re->mRenderer->depth_scale;
                        // convert from image coordinates to plan coordinates
                        ::AtoM(rotation, re->mRenderer->glob_angle[0],
                            re->mRenderer->glob_angle[1],
                            re->mRenderer->glob_angle[2]);
                        ::vector_matrix_multiply(plan_coords,
                            plan_coords, rotation);
                        if (plan_coords[0]*re->mRenderer->plane_normal[0]+
                                plan_coords[1]*re->mRenderer->plane_normal[1]+
                                plan_coords[2]*re->mRenderer->plane_normal[2] >
                                re->mRenderer->plane_displacement)
                        {
                            re->mRenderer->plane_normal[0] =
                                -re->mRenderer->plane_normal[0];
                            re->mRenderer->plane_normal[1] =
                                -re->mRenderer->plane_normal[1];
                            re->mRenderer->plane_normal[2] =
                                -re->mRenderer->plane_normal[2];
                            re->mRenderer->plane_displacement =
                                -re->mRenderer->plane_displacement;
                        }
                        for (Shell *object=re->mRenderer->object_list; object;
                                object=object->next)
                            if (object->O.on &&
                                    (st_cl(&object->main_data)==BINARY_B||
                                    st_cl(&object->main_data)==BINARY_A))
                            {
                                if (object->reflection == NULL)
                                {	object->reflection = (Virtual_object *)
                                        calloc(1, sizeof(Virtual_object));
                                    if (object->reflection == NULL)
                                    {
                                        fprintf(stderr, "Out of memory.\n");
                                        continue;
                                    }
                                    object->reflection->specular_fraction =
                                        object->O.specular_fraction;
                                    object->reflection->specular_exponent =
                                        object->O.specular_exponent;
                                    object->reflection->diffuse_exponent =
                                        object->O.diffuse_exponent;
                                    object->reflection->specular_n =
                                        object->O.specular_n;
                                    object->reflection->diffuse_n =
                                        object->O.diffuse_n;
                                    object->reflection->rgb.red = 65535;
                                    object->reflection->rgb.green = 65535;
                                    object->reflection->rgb.blue = 65535;
                                    re->mRenderer->set_color_number(
                                        object->reflection);
                                    object->reflection->opacity = .5625;
                                }
                                object->reflection->on = TRUE;
                            }
                    }
                    re->mRenderer->resize_image();
                    re->mRenderer->plane = FALSE;
                    break;

                case RenderEvent::TURN_REFLECTIONS_OFF :
                    if (re->mRenderer->selected_object < 0)
                        re->mRenderer->selected_object =
                            -re->mRenderer->selected_object;
                    for (Shell *object=re->mRenderer->object_list;
                            object; object=object->next)
                        if (object->reflection && object->reflection->on)
                            object->reflection->on = FALSE;
                    re->mRenderer->plane = TRUE;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::CUT_PLANE :
                    if (re->mRenderer->objects_need_be_turned_off())
                        break;
                    if (!re->mRenderer->object_from_label(re->mRenderer->
                            selected_object)->on)
                        for (Shell *obj=re->mRenderer->object_list; obj;
                                obj=obj->next)
                            if (obj->O.on)
                            {
                                re->mRenderer->selected_object =
                                    re->mRenderer->object_label(&obj->O);
                                break;
                            }
                    {
                        double rotation[3][3];
                        double plan_coords[3];
                        int z = re->mRenderer->closest_z( re->mIValue1,
                            re->mIValue2, re->mRenderer->main_image );
                        if (z == 0)
                        {
                            re->mRenderer->error_flag = true;
                            break;
                        }
                        plan_coords[0] = re->mIValue1-
                            re->mRenderer->main_image->width/2;
                        plan_coords[1] = re->mIValue2-
                            re->mRenderer->main_image->height/2;
                        plan_coords[2] = z;
                        /* convert to physical units */
                        plan_coords[0] /= re->mRenderer->scale;
                        plan_coords[1] /= re->mRenderer->scale;
                        plan_coords[2] = (MIDDLE_DEPTH-plan_coords[2])/
                            re->mRenderer->depth_scale;
                        // convert from image coordinates to plan coordinates
                        ::AtoM(rotation, re->mRenderer->glob_angle[0],
                            re->mRenderer->glob_angle[1],
                            re->mRenderer->glob_angle[2]);
                        ::vector_matrix_multiply(plan_coords,
                            plan_coords, rotation);
                        if (plan_coords[0]*re->mRenderer->plane_normal[0]+
                                plan_coords[1]*re->mRenderer->plane_normal[1]+
                                plan_coords[2]*re->mRenderer->plane_normal[2] >
                                re->mRenderer->plane_displacement)
                        {
                            re->mRenderer->plane_normal[0] =
                                -re->mRenderer->plane_normal[0];
                            re->mRenderer->plane_normal[1] =
                                -re->mRenderer->plane_normal[1];
                            re->mRenderer->plane_normal[2] =
                                -re->mRenderer->plane_normal[2];
                            re->mRenderer->plane_displacement =
                                -re->mRenderer->plane_displacement;
                        }
                        re->mRenderer->do_plane_cut();
                    }
                    re->mRenderer->resize_image();
                    re->mRenderer->plane = FALSE;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_OBJECT_ON:
                    if (re->mRenderer->selected_object < 0)
                        re->mRenderer->object_from_label(
                            -re->mRenderer->selected_object)->on = TRUE;
                    // fall through
                case RenderEvent::SET_OBJECT_OFF:
                    if (re->mType==RenderEvent::SET_OBJECT_OFF&&re->mRenderer->
                            actual_object(re->mRenderer->object_from_label(
                            re->mRenderer->selected_object))->reflection)
                        re->mRenderer->actual_object(re->mRenderer->
                            object_from_label(re->mRenderer->selected_object))
                            ->reflection->on = FALSE;
                    re->mRenderer->object_from_label(
                        re->mRenderer->selected_object)->on =
                            re->mType == RenderEvent::SET_OBJECT_ON;
                    if (re->mRenderer->image_mode == SLICE)
                    {
                        re->mRenderer->destroy_slice_list();
                        re->mRenderer->do_plane_slice();
                    }
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_PLANE_AXIAL:
                case RenderEvent::SET_PLANE_SAGITTAL:
                case RenderEvent::SET_PLANE_CORONAL:
                    Principal_plane plane_type;
                    switch (re->mType) {
                        case RenderEvent::SET_PLANE_AXIAL:
                            plane_type = AXIAL;
                            break;
                        case RenderEvent::SET_PLANE_SAGITTAL:
                            plane_type = SAGITTAL;
                            break;
                        case RenderEvent::SET_PLANE_CORONAL:
                            plane_type = CORONAL;
                            break;
						default:
							assert(false);
                    }
                    re->mRenderer->set_principal_plane(plane_type);
                    re->mRenderer->resize_image();
//@					re->mRenderer->resize_icon();

                    if (re->mRenderer->image_mode == SLICE)
                        re->mRenderer->do_plane_slice();
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::REMOVE_OBJECT:
                    re->mRenderer->remove_object(re->mRenderer->actual_object(
                        re->mRenderer->object_from_label(
                            re->mRenderer->selected_object)));
                    re->mRenderer->resize_image();
                    if (re->mRenderer->image_mode == SLICE)
                        re->mRenderer->do_plane_slice();
                    data = re->mRenderer->render2( overallXSize, overallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::REMOVE_CUT_OBJECTS:
                    for (;;)
                    {
                        Shell *obj;
                        for (obj=re->mRenderer->object_list; obj->next;
                                obj=obj->next)
                            if (obj->original)
                                obj->O.on = TRUE;
                            else
                                break;
                        if (obj->original)
                            break;
						assert(obj!=re->mRenderer->object_list || obj->next);
                        re->mRenderer->remove_object(obj);
                    }
					re->mRenderer->cut_count = 0;
					re->mRenderer->original_object = NULL;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode == SEPARATE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::RESET_MOVE:
                    for (Shell *obj=re->mRenderer->object_list; obj;
                            obj=obj->next)
                    {
                        memcpy(obj->angle, obj->plan_angle, sizeof(triple));
                        memcpy(obj->displacement, obj->plan_displacement,
                            sizeof(triple));
                    }
                    re->mRenderer->line_angle[0] = 0;
                    re->mRenderer->line_displacement[0] = 0;
                    re->mRenderer->line_displacement[1] = 0;
                    re->mRenderer->line_displacement[2] = 0;
                    re->mRenderer->resize_image();
//@					re->mRenderer->resize_icon();
                    mCanvas->mOverallXSize = re->mRenderer->main_image->width;
                    mCanvas->mOverallYSize = re->mRenderer->main_image->height;
                    break;

                case RenderEvent::SET_MOBILE:
                    re->mRenderer->actual_object(
                        re->mRenderer->object_from_label(
                            re->mRenderer->selected_object))->mobile = TRUE;
                    break;

                case RenderEvent::SET_IMMOBILE:
                    re->mRenderer->actual_object(
                        re->mRenderer->object_from_label(
                            re->mRenderer->selected_object))->mobile = FALSE;
                    break;

                case RenderEvent::SET_FIRST_LOC:
                    mCanvas->first_loc = re->mRenderer->plane_displacement;
                    break;

                case RenderEvent::SET_LAST_LOC:
                    mCanvas->last_loc = re->mRenderer->plane_displacement;
                    break;

                case RenderEvent::SET_OBJECT:
                    if (re->mDValue1==0 && re->mRenderer->selected_object)
                        re->mRenderer->old_selected_object =
                            re->mRenderer->selected_object;
                    re->mRenderer->selected_object = (int)re->mDValue1;
                    mCanvas->stats_valid = false;
                    if (re->mDValue1 && mCanvas->mWhichMode==
                            SurfViewCanvas::ROI_STATISTICS &&
                            re->mRenderer->get_object_roi_stats(
                            &mCanvas->total_density, &mCanvas->mean_density,
                            &mCanvas->standard_deviation,
                            &mCanvas->min_density, &mCanvas->max_density,
                            cvRenderer::cvCheckInterrupt)==DONE)
                    {
                        mCanvas->stats_valid = true;
                        data = re->mRenderer->get_recolored_image(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                        mCanvas->reload( (unsigned char*)data,
                            (unsigned char*)data2,
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    }
                    break;

                case RenderEvent::SET_MODE:
                    re->mRenderer->plane = FALSE;
                    re->mRenderer->nmeasurement_points = 0;
                    switch ((int)re->mDValue1)
                    {
                        case SurfViewCanvas::CUT_PLANE:
                        case SurfViewCanvas::VIEW:
                        case SurfViewCanvas::CUT_CURVED:
                        case SurfViewCanvas::CREATE_MOVIE_TUMBLE:
                        case SurfViewCanvas::PREVIOUS_SEQUENCE:
                        case SurfViewCanvas::MOVE:
                        case SurfViewCanvas::REFLECT:
                            re->mRenderer->image_mode = WITH_ICON;
                            break;
                        case SurfViewCanvas::SELECT_SLICE:
                            re->mRenderer->plane = TRUE;
                            // fall through
                        case SurfViewCanvas::MEASURE:
                        case SurfViewCanvas::ROI_STATISTICS:
                            re->mRenderer->image_mode = SLICE;
                            re->mRenderer->do_plane_slice();
                            mCanvas->stats_valid = false;
                            break;
                        case SEPARATE:
                            re->mRenderer->image_mode = SEPARATE;
                            break;
                    }
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_OBJECT_COLOR :
                    re->mRenderer->setObjectColor( re->mIValue1, re->mIValue2,
                        re->mIValue3, re->mIValue4 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_MATERIAL_COLOR :
                    re->mRenderer->tissue_red[re->mIValue4] = re->mIValue1;
                    re->mRenderer->tissue_green[re->mIValue4] = re->mIValue2;
                    re->mRenderer->tissue_blue[re->mIValue4] = re->mIValue3;
					if (re->mIValue4 == 0)
					{
						re->mRenderer->surface_red_factor = (float)
							MAX_SURFACE_FACTOR*re->mIValue1/
							(V_OBJECT_IMAGE_BACKGROUND-1);
						re->mRenderer->surface_green_factor = (float)
							MAX_SURFACE_FACTOR*re->mIValue2/
							(V_OBJECT_IMAGE_BACKGROUND-1);
						re->mRenderer->surface_blue_factor = (float)
							MAX_SURFACE_FACTOR*re->mIValue3/
							(V_OBJECT_IMAGE_BACKGROUND-1);
					}
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_OBJECT_DIFFUSE :
                    ::setObjectDiffuseFraction(re->mRenderer, re->mDValue1, re->mIValue1 );
                    re->mRenderer->setObjectDiffuseExponent( re->mDValue2, re->mIValue1 );
                    re->mRenderer->setObjectDiffuseDivisor(  re->mDValue3, re->mIValue1 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_OBJECT_OPACITY :
                    re->mRenderer->setObjectOpacity( re->mDValue1, re->mIValue1 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_OBJECT_SPECULAR :
                    re->mRenderer->setObjectSpecularFraction( re->mDValue1, re->mIValue1 );
                    re->mRenderer->setObjectSpecularExponent( re->mDValue2, re->mIValue1 );
                    re->mRenderer->setObjectSpecularDivisor(  re->mDValue3, re->mIValue1 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_ALL_DIFFUSE :
                    ::setAllDiffuseFraction(re->mRenderer, re->mDValue1 );
                    re->mRenderer->setAllDiffuseExponent( re->mDValue2 );
                    re->mRenderer->setAllDiffuseDivisor(  re->mDValue3 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_ALL_SPECULAR :
                    re->mRenderer->setAllSpecularFraction( re->mDValue1 );
                    re->mRenderer->setAllSpecularExponent( re->mDValue2 );
                    re->mRenderer->setAllSpecularDivisor(  re->mDValue3 );
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_SURFACE_STRENGTH :
                    re->mRenderer->surface_strength = re->mDValue1;
                    re->mRenderer->emission_power = re->mDValue2;
                    re->mRenderer->surf_pct_power = re->mDValue3;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_MATERIAL_OPACITY :
                    re->mRenderer->tissue_opacity[re->mIValue1] = re->mDValue1;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_MATERIAL_THRESHOLDS :
                    re->mRenderer->object_list->main_data.threshold[
                        2*re->mIValue1-2] = (int)re->mDValue1;
                    re->mRenderer->object_list->main_data.threshold[
                        2*re->mIValue1-1] = (int)re->mDValue2;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_VIEW :
                    re->mRenderer->glob_angle[0] = re->mDValue1;
                    re->mRenderer->glob_angle[1] = re->mDValue2;
                    re->mRenderer->glob_angle[2] = re->mDValue3;
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE ||
                            re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->preview_frame_displayed = re->mIValue1;
                    break;

                case RenderEvent::SET_X :
                    rot_axis = 0;  //x is 0, y is 1, and z is 2
                    if (mCanvas->mLastRotateMode != SurfViewCanvas::rxMode)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode = SurfViewCanvas::rxMode;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in viewing angle
                    ::AtoM( from_matrix,
                        re->mRenderer->glob_angle[0], re->mRenderer->glob_angle[1], re->mRenderer->glob_angle[2] );
                    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
                    ::matrix_multiply( rot_matrix, rot_matrix, from_matrix );
                    ::MtoA( re->mRenderer->glob_angle, re->mRenderer->glob_angle+1, re->mRenderer->glob_angle+2, rot_matrix );

                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_Y :
                    rot_axis = 1;  //x is 0, y is 1, and z is 2
                    if (mCanvas->mLastRotateMode != SurfViewCanvas::ryMode)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode = SurfViewCanvas::ryMode;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in viewing angle
                    ::AtoM( from_matrix,
                        re->mRenderer->glob_angle[0], re->mRenderer->glob_angle[1], re->mRenderer->glob_angle[2] );
                    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
                    ::matrix_multiply( rot_matrix, rot_matrix, from_matrix );
                    ::MtoA( re->mRenderer->glob_angle, re->mRenderer->glob_angle+1, re->mRenderer->glob_angle+2, rot_matrix );

                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_Z :
                    rot_axis = 2;  //x is 0, y is 1, and z is 2
                    if (mCanvas->mLastRotateMode != SurfViewCanvas::rzMode)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode = SurfViewCanvas::rzMode;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in viewing angle
                    ::AtoM( from_matrix,
                        re->mRenderer->glob_angle[0], re->mRenderer->glob_angle[1], re->mRenderer->glob_angle[2] );
                    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
                    ::matrix_multiply( rot_matrix, rot_matrix, from_matrix );
                    ::MtoA( re->mRenderer->glob_angle, re->mRenderer->glob_angle+1, re->mRenderer->glob_angle+2, rot_matrix );

                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_C : // rotate mobile objects
                    if (mCanvas->mLastRotateMode != SurfViewCanvas::roMode)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode = SurfViewCanvas::roMode;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in position
                    ::AtoM(rot_matrix, re->mRenderer->line_angle[0],
                        re->mRenderer->line_angle[1], whichAngleValue);
                    for (Shell *obj=re->mRenderer->object_list; obj;
                            obj=obj->next)
                        if (obj->mobile)
                        {
                            double obj_matrix[3][3], start_displacement[3];
                            ::AtoM(obj_matrix, obj->angle[0], obj->angle[1],
                                obj->angle[2]);
                            ::vector_subtract(start_displacement,
                                obj->displacement,
                                re->mRenderer->line_displacement);
                            ::matrix_vector_multiply(obj->displacement,
                                rot_matrix, start_displacement);
                            ::vector_add(obj->displacement, obj->displacement,
                                re->mRenderer->line_displacement);
                            ::matrix_multiply(obj_matrix, rot_matrix,
                                obj_matrix);
                            ::MtoA(obj->angle, obj->angle+1, obj->angle+2,
                                obj_matrix);
                            obj->O.main_image.projected =
                                obj->O.icon.projected = BAD;
                            if (obj->reflection)
                                obj->reflection->main_image.projected =
                                    obj->reflection->icon.projected = BAD;
                        }
                    re->mRenderer->resize_image();
//@					re->mRenderer->resize_icon();

                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::TRANSLATE :
                    if (mCanvas->mLastRotateMode != SurfViewCanvas::toMode)
                    {
                        mCanvas->mRotateAngle =
                            re->mDValue1/re->mRenderer->scale;
                        mCanvas->mLastRotateMode = SurfViewCanvas::toMode;
                    }
                    else
                        mCanvas->mRotateAngle +=
                            re->mDValue1/re->mRenderer->scale;
                    whichAngleValue = re->mDValue1/re->mRenderer->scale;
                    //handle change in position
                    double line_direction[3];
                    ::AtoV(line_direction, re->mRenderer->line_angle[0],
                        re->mRenderer->line_angle[1]);
                    for (Shell *obj=re->mRenderer->object_list; obj;
                            obj=obj->next)
                        if (obj->mobile)
                        {
                            obj->displacement[0] +=
                                whichAngleValue*line_direction[0];
                            obj->displacement[1] +=
                                whichAngleValue*line_direction[1];
                            obj->displacement[2] +=
                                whichAngleValue*line_direction[2];
                            obj->O.main_image.projected =
                                obj->O.icon.projected = BAD;
                            if (obj->reflection)
                                obj->reflection->main_image.projected =
                                    obj->reflection->icon.projected = BAD;
                        }
                    re->mRenderer->resize_image();
//@					re->mRenderer->resize_icon();

                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_PLANE_X :
                case RenderEvent::SET_PLANE_Y :
                case RenderEvent::SET_PLANE_Z :
                    switch (re->mType) {
                        case RenderEvent::SET_PLANE_X :
                            rot_axis = 0;  //x is 0, y is 1, and z is 2
                            break;
                        case RenderEvent::SET_PLANE_Y :
                            rot_axis = 1;  //x is 0, y is 1, and z is 2
                            break;
                        case RenderEvent::SET_PLANE_Z :
                            rot_axis = 2;  //x is 0, y is 1, and z is 2
							break;
						default:
							assert(false);
                    }
                    if (mCanvas->mLastRotateMode !=
                            SurfViewCanvas::rxMode+rot_axis)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode =
                            SurfViewCanvas::rxMode+rot_axis;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in line angle
                    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
                    double glob_matrix[3][3], start_normal[3];
                    ::AtoM(glob_matrix, re->mRenderer->glob_angle[0],
                        re->mRenderer->glob_angle[1],
                        re->mRenderer->glob_angle[2]);
                    ::matrix_vector_multiply(start_normal, glob_matrix,
                        re->mRenderer->plane_normal);
                    ::matrix_vector_multiply(re->mRenderer->plane_normal,
                        rot_matrix, start_normal);
                    ::vector_matrix_multiply(re->mRenderer->plane_normal,
                        re->mRenderer->plane_normal, glob_matrix);
                    re->mRenderer->resize_image();
//@					re->mRenderer->resize_icon();
                    mCanvas->stats_valid = false;

                    if (re->mRenderer->image_mode == SLICE)
                        re->mRenderer->do_plane_slice();
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_PLANE_T :
				case RenderEvent::NEXT_SLICE :
				case RenderEvent::PREVIOUS_SLICE :
                    switch (re->mType) {
                        case RenderEvent::SET_PLANE_T :
		                    if (mCanvas->mLastRotateMode)
		                    {
		                        mCanvas->mRotateAngle =
		                            re->mDValue1/re->mRenderer->scale;
		                        mCanvas->mLastRotateMode = 0;
		                    }
		                    else
		                        mCanvas->mRotateAngle +=
		                            re->mDValue1/re->mRenderer->scale;
		                    //handle change in position
		                    re->mRenderer->plane_displacement +=
		                        re->mDValue1/re->mRenderer->scale;
                            break;
                        case RenderEvent::NEXT_SLICE :
                            re->mRenderer->plane_displacement +=
								re->mRenderer->out_slice_spacing;
                            break;
                        case RenderEvent::PREVIOUS_SLICE :
                            re->mRenderer->plane_displacement -=
								re->mRenderer->out_slice_spacing;
							break;
						default:
							assert(false);
                    }
                    re->mRenderer->resize_image();

                    if (re->mRenderer->image_mode == SLICE)
                        re->mRenderer->do_plane_slice();
                    data = re->mRenderer->render2( mCanvas->mOverallXSize,
                        mCanvas->mOverallYSize,
                        mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    if (re->mRenderer->image_mode==SEPARATE || re->mRenderer->image_mode==SLICE)
                        data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    else
                        data2 = NULL;
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_LINE_X :
                case RenderEvent::SET_LINE_Y :
                case RenderEvent::SET_LINE_Z :
                    switch (re->mType) {
                        case RenderEvent::SET_LINE_X :
                            rot_axis = 0;  //x is 0, y is 1, and z is 2
                            break;
                        case RenderEvent::SET_LINE_Y :
                            rot_axis = 1;  //x is 0, y is 1, and z is 2
                            break;
                        case RenderEvent::SET_LINE_Z :
                            rot_axis = 2;  //x is 0, y is 1, and z is 2
							break;
						default:
							assert(false);
                    }
                    if (mCanvas->mLastRotateMode !=
                            SurfViewCanvas::rxMode+rot_axis)
                    {
                        mCanvas->mRotateAngle = re->mDValue1*180/M_PI;
                        mCanvas->mLastRotateMode =
                            SurfViewCanvas::rxMode+rot_axis;
                    }
                    else
                        mCanvas->mRotateAngle += re->mDValue1*180/M_PI;
                    whichAngleValue = re->mDValue1;
                    //handle change in line angle
                    ::axis_rot( rot_matrix, rot_axis, whichAngleValue );
                    double start_direction[3];
                    double start_displacement[3], new_direction[3];
                    ::AtoM(glob_matrix, re->mRenderer->glob_angle[0],
                        re->mRenderer->glob_angle[1],
                        re->mRenderer->glob_angle[2]);
                    ::AtoV(start_direction, re->mRenderer->line_angle[0],
                        re->mRenderer->line_angle[1]);
                    ::matrix_vector_multiply(start_direction, glob_matrix,
                        start_direction);
                    ::matrix_vector_multiply(start_displacement, glob_matrix,
                        re->mRenderer->line_displacement);
                    ::transpose(glob_matrix, glob_matrix);
                    ::matrix_vector_multiply(re->mRenderer->line_displacement,
                        rot_matrix, start_displacement);
                    ::matrix_vector_multiply(re->mRenderer->line_displacement,
                        glob_matrix, re->mRenderer->line_displacement);
                    ::matrix_vector_multiply(new_direction, rot_matrix,
                        start_direction);
                    ::matrix_vector_multiply(new_direction, glob_matrix,
                        new_direction);
                    ::VtoA(re->mRenderer->line_angle,
                        re->mRenderer->line_angle+1, new_direction);
                    data = re->mRenderer->get_recolored_image( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SET_LINE_XY :
                    if (mCanvas->mLastRotateMode)
                    {
                        mCanvas->mLineDX = re->mDValue1/re->mRenderer->scale;
                        mCanvas->mLineDY = re->mDValue2/re->mRenderer->scale;
                        mCanvas->mLastRotateMode = 0;
                    }
                    else
                    {
                        mCanvas->mLineDX += re->mDValue1/re->mRenderer->scale;
                        mCanvas->mLineDY += re->mDValue2/re->mRenderer->scale;
                    }
                    //handle change in line location
                    double p[3], q[3];
                    double dx, dy, norm;
                    ::AtoM(rot_matrix, re->mRenderer->glob_angle[0],
                        re->mRenderer->glob_angle[1],
                        re->mRenderer->glob_angle[2]);
                    ::AtoV(line_direction, re->mRenderer->line_angle[0],
                        re->mRenderer->line_angle[1]);
                    ::matrix_vector_multiply(line_direction, rot_matrix,
                        line_direction);
                    p[0] = -line_direction[2];
                    p[1] = 0;
                    p[2] = line_direction[0];
                    norm = p[0]*p[0]+p[2]*p[2];
                    if (norm <= 0)
                    {	p[0] = 1;
                        p[2] = 0;
                    }
                    else
                    {	norm = 1/sqrt(norm);
                        p[0] *= norm;
                        p[2] *= norm;
                    }
                    q[0] = line_direction[1]*p[2];
                    q[1] = line_direction[2]*p[0]-line_direction[0]*p[2];
                    q[2] = -line_direction[1]*p[0];
                    /* p & q are orthogonal vectors perpendicular to the line.
                       p is horizontal & q points up. */
                    ::vector_matrix_multiply(p, p, rot_matrix);
                    ::vector_matrix_multiply(q, q, rot_matrix);
                    dx = re->mDValue1/re->mRenderer->scale;
                    if (p[0] < 0)
                        dx = -dx;
                    dy = re->mDValue2/re->mRenderer->scale;
                    re->mRenderer->line_displacement[0] += p[0]*dx-q[0]*dy;
                    re->mRenderer->line_displacement[1] += p[1]*dx-q[1]*dy;
                    re->mRenderer->line_displacement[2] += p[2]*dx-q[2]*dy;
                    data = re->mRenderer->get_recolored_image(
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    if (re->mRenderer->line)
                    {
                        if (mCanvas->mLinePixelCount)
                            free(mCanvas->mLinePixels);
                        re->mRenderer->get_line(mCanvas->mLinePixels,
                            mCanvas->mLinePixelCount);
                    }
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::CUT_CURVED:
                    if (re->mRenderer->objects_need_be_turned_off())
                        break;
                    if (!re->mRenderer->object_from_label(re->mRenderer->
                            selected_object)->on)
                        for (Shell *obj=re->mRenderer->object_list; obj;
                                obj=obj->next)
                            if (obj->O.on)
                            {
                                re->mRenderer->selected_object =
                                    re->mRenderer->object_label(&obj->O);
                                break;
                            }
                    re->mRenderer->set_original_objects();
                    re->mRenderer->do_curved_cut(re->mVertices, re->mIValue1,
                        re->mInside, re->mDValue1);
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)NULL,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::SEPARATE:
                    if (re->mRenderer->objects_need_be_turned_off())
                        break;
                    re->mRenderer->set_original_objects();
                    re->mRenderer->do_separation(re->mVertices, re->mIValue1,
                        re->mInside, re->mPrimary? 1:2, re->mDValue1);
                    data = re->mRenderer->render2( mCanvas->mOverallXSize, mCanvas->mOverallYSize, mCanvas->mInterruptRenderingFlag );
                    mCanvas->handleStereo( data );
                    data2 = re->mRenderer->get_recolored_image2( mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                case RenderEvent::GET_SLICE_ROI_STATS:
                    if (re->mIValue1 <= 0)
                        break;
                    int min_x, max_x, min_y, max_y, this_x, this_y;
                    min_x = max_x = re->mVertices[0].x-mCanvas->mOverallXSize;
                    min_y = max_y = re->mVertices[0].y;
                    for (int j=1; j<re->mIValue1; j++)
                    {
                        if (re->mVertices[j].x-mCanvas->mOverallXSize < min_x)
                            min_x = re->mVertices[j].x-mCanvas->mOverallXSize;
                        if (re->mVertices[j].x-mCanvas->mOverallXSize > max_x)
                            max_x = re->mVertices[j].x-mCanvas->mOverallXSize;
                        if (re->mVertices[j].y < min_y)
                            min_y = re->mVertices[j].y;
                        if (re->mVertices[j].y > max_y)
                            max_y = re->mVertices[j].y;
                    }
                    if (min_x < 0)
                        min_x = 0;
                    if (max_x >= mCanvas->mOverallXSize)
                        max_x = mCanvas->mOverallXSize-1;
                    if (min_y < 0)
                        min_y = 0;
                    if (max_y >= mCanvas->mOverallYSize)
                        max_y = mCanvas->mOverallYSize-1;
                    __uint64 N;
                    N = 0;
                    mCanvas->total_density = 0;
                    mCanvas->min_density = 999999999;
                    mCanvas->max_density = 0;
                    int sl_scene, w;
                    sl_scene = re->mRenderer->slice_list->on[0]? 0: 1;
                    w = re->mRenderer->slice_list->image->width;
                    double sum_squares;
                    sum_squares = 0;
                    for (this_y=min_y; this_y<=max_y; this_y++)
                        for (this_x=min_x; this_x<=max_x; this_x++)
                            if (is_in_polygon(this_x+mCanvas->mOverallXSize,
                                    this_y, re->mVertices, re->mIValue1))
                            {
                                N++;
                                double z = re->mRenderer->slice_list->gray_max[
                                    sl_scene]>255? re->mRenderer->slice_list->
                                    image_data[sl_scene].s[this_y*w+this_x]:
                                    re->mRenderer->slice_list->image_data[
                                    sl_scene].c[this_y*w+this_x];
                                mCanvas->total_density += z;
                                sum_squares += z*z;
                                if (z < mCanvas->min_density)
                                    mCanvas->min_density = z;
                                if (z > mCanvas->max_density)
                                    mCanvas->max_density = z;
                            }
                    mCanvas->mean_density = mCanvas->total_density/N;
                    mCanvas->standard_deviation = sqrt((sum_squares-mCanvas->
                        total_density*mCanvas->mean_density)/N);
                    mCanvas->stats_valid = true;
                    break;

                case RenderEvent::SET_GRAY_WINDOW:
					if (re->mIValue2 >= 0)
						re->mRenderer->slice_list->gray_level[re->mIValue1] =
							re->mIValue2;
                    re->mRenderer->null_slice.gray_level[re->mIValue1] =
						re->mRenderer->slice_list->gray_level[re->mIValue1];
					if (re->mIValue3 >= 0)
						re->mRenderer->slice_list->gray_width[re->mIValue1] =
							re->mIValue3;
					re->mRenderer->null_slice.gray_width[re->mIValue1] =
						re->mRenderer->slice_list->gray_width[re->mIValue1];
					re->mRenderer->null_slice.gray_max[re->mIValue1] =
						re->mRenderer->slice_list->gray_max[re->mIValue1];
					re->mRenderer->null_slice.gray_window_valid[re->mIValue1] =
						re->mRenderer->slice_list->gray_window_valid[
						re->mIValue1];
					if (re->mIValue4 >= 0)
						re->mRenderer->slice_list->inverted[re->mIValue1] =
							re->mIValue4;
                    re->mRenderer->slice_list->valid = FALSE;
					if (re->mRenderer->image_mode == SLICE)
                        data2 = re->mRenderer->get_recolored_image2(
                            mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    mCanvas->reload( (unsigned char*)data,
                        (unsigned char*)data2,
                        mCanvas->mOverallXSize, mCanvas->mOverallYSize );
                    break;

                default :
                    assert( 0 );
                    break;
            }  //end switch
            mutex.Unlock();

        }  //end for

        return 0;  //ok
    }
};
//======================================================================
void SurfViewCanvas::displayGrayMapControls()
{
	if (mWhichMode==SELECT_SLICE || mWhichMode==MEASURE ||
			mWhichMode==ROI_STATISTICS)
	{
		mRenderer->error_flag = true;
		eventQueue.enqueue(mRenderer, RenderEvent::RESET_ERROR);
		ensureRenderThreadRunning();
		bool synched=0;
		while (!synched)
		{
			wxSafeYield(NULL, true);
			wxCriticalSectionLocker  lock( mCriticalSection );
			//critical section
			//vvvvvvvvvvvvvvvv

			synched = !mRenderer->error_flag;
		}
		SurfViewFrame *mf= dynamic_cast<SurfViewFrame *>( m_parent_frame );
	    mf->displayGrayMapControls();
	}
}
//======================================================================
void SurfViewCanvas::release ( void ) {
    wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                        //vvvvvvvvvvvvvvvv
    if (mRenderThread)
    {
        mRenderThread->mTimeToStop = true;
        mRenderThread = NULL;
    }
    if (mRenderer)
        ::eventQueue.removeEvents(mRenderer);
    delete mRenderer;
    mRenderer = NULL;
    if (nvertices)
        free(vertices);
    nvertices = 0;
    if (mLinePixelCount)
        free(mLinePixels);
    mLinePixelCount = 0;
    if (key_pose_array_size)
        free(key_pose);
    key_pose_array_size = key_poses = 0;
}
//----------------------------------------------------------------------
void SurfViewCanvas::ensureRenderThreadRunning ( void ) {
    if (!mRenderThread) {
        //create new thread to render a frame
        mRenderThread = new RenderThread( this, &eventQueue );
        assert( mRenderThread );
        mRenderThread->SetPriority( WXTHREAD_MIN_PRIORITY );
        mRenderThread->Run();
    }
    wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                        //vvvvvvvvvvvvvvvv
    if (!mRenderThread->mTimeToStop)
        assert( mRenderThread && mRenderThread->IsRunning() );
}
//----------------------------------------------------------------------
void SurfViewCanvas::setStatus ( bool state ) {
    if (state)
        eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_ON );
    else
        eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_OFF );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::removeObject ( void ) {
    if (mRenderer->object_list->next == NULL)
    {
        wxMessageBox("Cannot remove only remaining object.");
        return;
    }
    if (!mRenderer->object_from_label(mRenderer->selected_object)->on &&
            wxMessageBox("Remove hidden object?", "Are you sure?",
            wxYES_NO|wxICON_QUESTION)!=wxYES)
        return;
    int old_n = mRenderer->number_of_objects();
    eventQueue.enqueue( mRenderer, RenderEvent::REMOVE_OBJECT );
    ensureRenderThreadRunning();
    for (int new_n=old_n; new_n==old_n; )
    {
        wxSafeYield(NULL, true);
        wxCriticalSectionLocker  lock( mCriticalSection ); //critical section
                                                           //vvvvvvvvvvvvvvvv
        new_n = mRenderer->number_of_objects();
    }
    SurfViewFrame*  mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    mf->updateObjectSwitch();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMobile ( bool state ) {
    if (state)
        eventQueue.enqueue( mRenderer, RenderEvent::SET_MOBILE );
    else
        eventQueue.enqueue( mRenderer, RenderEvent::SET_IMMOBILE );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setObject ( int which )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT, -1, which );
    ensureRenderThreadRunning();
	bool synched=false;
	while (!synched)
	{
		wxSafeYield(NULL, true);
		wxCriticalSectionLocker  lock( mCriticalSection ); //critical section
		                                                   //vvvvvvvvvvvvvvvv
		synched = mRenderer->selected_object==which;
	}
}
//----------------------------------------------------------------------
void SurfViewCanvas::reset ( void )
{
    SurfViewFrame *mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    if (mRotateOn)
    {
        if (mRotateMode == 0)
        {
            m_tx = m_ty = 0;
            eventQueue.enqueue(mRenderer, RenderEvent::SET_VIEW, -1,
                0.0, 0.0, 0.0, -1);
            ensureRenderThreadRunning();
        }
        return;
    }
	int old_n, cut_count=1;
    switch (mWhichMode)
    {
        case VIEW:
            eventQueue.enqueue(mRenderer, RenderEvent::SET_VIEW, -1,
                0.0, 0.0, 0.0, -1);
            ensureRenderThreadRunning();
            break;
        case SELECT_SLICE:
            mControlState = 14;
            eventQueue.enqueue(mRenderer, RenderEvent::SET_PLANE_AXIAL, -1);
            ensureRenderThreadRunning();
            setActionText();
            break;
        case MEASURE:
        case ROI_STATISTICS:
            eventQueue.enqueue(mRenderer, RenderEvent::SET_MODE,-1,mWhichMode);
            ensureRenderThreadRunning();
            break;
        case REFLECT:
            eventQueue.enqueue(mRenderer, RenderEvent::SET_PLANE_AXIAL, -1);
            ensureRenderThreadRunning();
            break;
        case CUT_CURVED:
        case CUT_PLANE:
            old_n = 0;
            for (Shell *obj=mRenderer->object_list; obj; obj=obj->next)
                if (obj->original)
                    old_n++;
            if (old_n == 0)
			{
				wxMessageBox("Cannot undo.");
				break;
			}
			eventQueue.enqueue(mRenderer, RenderEvent::REMOVE_CUT_OBJECTS, -1);
			ensureRenderThreadRunning();
			while (cut_count)
			{
				wxSafeYield(NULL, true);
				wxCriticalSectionLocker lock( mCriticalSection );
				//critical section
				//vvvvvvvvvvvvvvvv

				cut_count = mRenderer->cut_count;
			}
			mf->updateObjectSwitch();
			break;
        case SEPARATE:
            old_n = 0;
            for (Shell *obj=mRenderer->object_list; obj; obj=obj->next)
                if (obj->original)
                    old_n++;
            if (old_n == 0)
			{
				wxMessageBox("Cannot undo.");
				break;
			}
			eventQueue.enqueue(mRenderer, RenderEvent::REMOVE_CUT_OBJECTS, -1);
            ensureRenderThreadRunning();
            for (int new_n=0; new_n!=old_n; )
            {
                wxSafeYield(NULL, true);
                wxCriticalSectionLocker lock( mCriticalSection );
                //critical section
                //vvvvvvvvvvvvvvvv

                new_n = 0;
                for (Shell *obj=mRenderer->object_list; obj; obj=obj->next)
                    new_n++;
            }
            mf->updateObjectSwitch();
            break;
        case MOVE:
            m_tx += mOverallXSize/2;
            m_ty += mOverallYSize/2;
            freeImagesAndBitmaps();
            eventQueue.enqueue(mRenderer, RenderEvent::RESET_MOVE, -1);
            ensureRenderThreadRunning();
            bool synched;
            synched = false;
            while (!synched)
            {
                wxSafeYield(NULL, true);
                wxCriticalSectionLocker lock( mCriticalSection );
                //critical section
                //vvvvvvvvvvvvvvvv

                synched = true;
                for (Shell *obj=mRenderer->object_list; obj; obj=obj->next)
                    for (int j=0; j<3; j++)
                        if (obj->angle[j]!=obj->plan_angle[j] ||
                               obj->displacement[j]!=obj->plan_displacement[j])
                            synched = false;
            }
            m_tx -= mOverallXSize/2;
            m_ty -= mOverallYSize/2;
            rerender();
            break;
        case CREATE_MOVIE_TUMBLE:
        case PREVIOUS_SEQUENCE:
            if (mControlState < 12)
                key_poses = 0;
            preview_key_pose = -1;
            preview_view = -1;
            m_parent_frame->SetStatusText("", 0);
            break;
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMode ( int which )
{
    if (nvertices)
        free(vertices);
    nvertices = 0;
    if (mRotateMode==toMode || mRotateMode==roMode || ((mWhichMode==CUT_PLANE||
            mWhichMode==SELECT_SLICE||mWhichMode==REFLECT) &&
            ((mControlState>=1 && mControlState<=6) || mControlState==12)))
    {
        m_tx -= mOverallXSize/2;
        m_ty -= mOverallYSize/2;
        mLastRotateMode = 0;
    }
    mRotateMode = 0;
    mControlState = 0;
    m_cine_timer->Stop();
    preview_key_pose = -1;
    preview_view = -1;
    freeImagesAndBitmaps();
    m_parent_frame->SetStatusText("", 0);
    mWhichMode = which;
    switch (which)
    {
        case SELECT_SLICE:
        case SEPARATE:
        case MEASURE:
        case ROI_STATISTICS:
            m_cols = 2;
            break;
        case PREVIOUS_SEQUENCE:
            m_cols = 1;
            mControlState = 12;

            // load previous sequence
            FILE *sequence_file;
            int pose;
            Key_pose *temp_key_pose;
            sequence_file = fopen("manip_sequence", "rb");
            if (sequence_file == NULL)
            {
                wxMessageBox("Could not load previous sequence.");
                break;
            }
            if (fscanf(sequence_file, "%d\n", &key_poses) != 1)
            {
                wxMessageBox("Could not load previous sequence.");
                fclose(sequence_file);
                break;
            }
            if (key_poses == 0)
            {	fclose(sequence_file);
                break;
            }
            if (key_poses > key_pose_array_size)
            {	temp_key_pose = (Key_pose *)
                (	key_pose_array_size
                    ?	realloc(key_pose, key_poses*sizeof(Key_pose))
                    :	malloc(key_poses*sizeof(Key_pose))
                );
                if (temp_key_pose == NULL)
                {
                    wxMessageBox("Out of memory.");
                    key_poses = 0;
                    fclose(sequence_file);
                    break;
                }
                key_pose = temp_key_pose;
                key_pose_array_size = key_poses;
            }
            for (pose=0; pose<key_poses; pose++)
                if (fscanf(sequence_file, "{%lf, %lf, %lf} %d\n",
                        key_pose[pose].angle, key_pose[pose].angle+1,
                        key_pose[pose].angle+2, &key_pose[pose].views) != 4)
                {
                    wxMessageBox("Could not load previous sequence.");
                    key_poses = 0;
                    fclose(sequence_file);
                    setActionText();
                    return;
                }
            fclose(sequence_file);
            break;
        default:
            m_cols = 1;
            break;
    }
    eventQueue.enqueue( mRenderer, RenderEvent::SET_MODE, -1, which );
    ensureRenderThreadRunning();
    setActionText();
	displayGrayMapControls();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setAmbientLight ( int r, int g, int b ) {
    eventQueue.enqueue( mRenderer, RenderEvent::SET_AMBIENT, -1, r, g, b );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setAntialias ( bool state ) {
    if (state)    eventQueue.enqueue( mRenderer, RenderEvent::AA_ON  );
    else          eventQueue.enqueue( mRenderer, RenderEvent::AA_OFF );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMIP ( bool state ) {
    if (state)    eventQueue.enqueue( mRenderer, RenderEvent::MIP_ON  );
    else          eventQueue.enqueue( mRenderer, RenderEvent::MIP_OFF );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMIP_Invert ( bool state ) {
	m_MIP_Invert = state;
    eventQueue.enqueue( mRenderer, RenderEvent::MIP_ON  );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setBox ( bool on ) {
    if (on)       eventQueue.enqueue( mRenderer, RenderEvent::BOX_ON  );
    else          eventQueue.enqueue( mRenderer, RenderEvent::BOX_OFF );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setBackgroundColor ( int r, int g, int b ) {
    eventQueue.enqueue( mRenderer, RenderEvent::SET_BACKGROUND, -1, r, g, b );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setObjectColor ( int r, int g, int b, int whichObject ) {
    eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_COLOR, -1, r, g, b, whichObject );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setObjectDiffuse ( double percent, double exponent,
                                        double divisor, int whichObject )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_DIFFUSE, -1, percent, exponent,
        divisor, whichObject );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setObjectOpacity ( double opacity, int whichObject ) {
    eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_OPACITY, -1, opacity, whichObject );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setObjectSpecular ( double percent, double exponent,
                                         double divisor, int whichObject )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT_SPECULAR, -1, percent, exponent,
        divisor, whichObject );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setAllDiffuse ( double percent, double exponent,
                                        double divisor )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_ALL_DIFFUSE, -1, percent, exponent,
        divisor, 1 );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setAllSpecular ( double percent, double exponent,
                                         double divisor )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_ALL_SPECULAR, -1, percent, exponent,
        divisor, 1 );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------

void SurfViewCanvas::setMaterialOpacity( double opacity, int whichMaterial )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_MATERIAL_OPACITY, -1,
        opacity, whichMaterial );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMaterialThresholds( double th1, double th2,
        int whichMaterial )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_MATERIAL_THRESHOLDS, -1,
        th1, th2, -1.0, whichMaterial );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setMaterialColor( int r, int g, int b, int whichMaterial )
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_MATERIAL_COLOR, -1,
        r, g, b, whichMaterial );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setSurfStrength( double surfStr, double emisPwr, double surfPctPwr)
{
    eventQueue.enqueue( mRenderer, RenderEvent::SET_SURFACE_STRENGTH, -1,
        surfStr, emisPwr, surfPctPwr, -1 );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setGrayMap ( int which, int center, int width, int invert ) {
    eventQueue.enqueue( mRenderer, RenderEvent::SET_GRAY_WINDOW, -1, which,
		center, width, invert );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::rerender ( void ) {
    eventQueue.enqueue( mRenderer, RenderEvent::RERENDER );
    ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnChar ( wxKeyEvent& e ) {
    switch (e.m_keyCode) {
        case 'a':
            eventQueue.enqueue( mRenderer, RenderEvent::AA_OFF, e.GetTimestamp() );
            break;
        case 'A':
            eventQueue.enqueue( mRenderer, RenderEvent::AA_ON, e.GetTimestamp() );
            break;
        default:
            return;
            break;
    }
    if (!mRenderThread) {
        //create new thread to render a frame
        mRenderThread = new RenderThread( this, &eventQueue );
        assert( mRenderThread );
        mRenderThread->SetPriority( WXTHREAD_MIN_PRIORITY );
        mRenderThread->Run();
    }
}

void SurfViewCanvas::OnMagnify ( double scale ) {
    #ifdef  WIN32
        //wxLogMessage( "in SurfViewCanvas::OnMagnify scale = %.6f", scale );
    #else
        //cout << "in SurfViewCanvas::OnMagnify scale=" << scale
        //     << " tid=" << gettid() << endl;
    #endif

    eventQueue.enqueue( mRenderer, RenderEvent::SCALE, -1, scale );
    ensureRenderThreadRunning();

    #ifdef  WIN32
        //wxLogMessage( "out SurfViewCanvas::OnMagnify scale = %.6f", scale );
    #else
        //cout << "out SurfViewCanvas::OnMagnify scale=" << scale
        //     << " tid=" << gettid() << endl;
    #endif
}
//----------------------------------------------------------------------
int visible_object_type(Shell *object_list)
{
    Shell *ptr;
    for (ptr=object_list; ptr; ptr=ptr->next)
        if (ptr->O.on)
            return ptr->main_data.file->file_header.gen.data_type;
    return -1;
}
//----------------------------------------------------------------------
Shell *visible_object(Shell *object_list)
{
    Shell *ptr;
    for (ptr=object_list; ptr; ptr=ptr->next)
        if (ptr->O.on)
            return ptr;
    return NULL;
}
//----------------------------------------------------------------------
void SurfViewCanvas::nextSlice()
{
	eventQueue.enqueue(mRenderer, RenderEvent::NEXT_SLICE);
	ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::previousSlice()
{
	eventQueue.enqueue(mRenderer, RenderEvent::PREVIOUS_SLICE);
	ensureRenderThreadRunning();
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnMouseMove ( wxMouseEvent& e ) {
    
    wxClientDC  dc( this );
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    long  wx  = dc.DeviceToLogicalX( pos.x );
    long  wy  = dc.DeviceToLogicalY( pos.y );
    SurfViewFrame*  mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    wxRect   r = GetRect();

    //move (translate) the image (if a mouse button is down)
    if (e.LeftIsDown() && mRotateOn && mRotateMode==0) {
        SetCursor( wxCursor(wxCURSOR_CROSS) );
        if (lastX==-1 || lastY==-1) {
            SetCursor( wxCursor(wxCURSOR_CROSS) );
            lastX = wx;
            lastY = wy;
            //wxLogMessage( "out SurfViewCanvas::OnMouseMove" );
            return;
        }
        
        bool  changed = false;
        if ((wx-lastX)>1) {
            m_tx += (wx-lastX);  // / 4 * 3;
            changed = true;
        } else if ((lastX-wx)>1) {
            m_tx -= (lastX-wx);  // / 4 * 3;
            changed = true;
        }
        if ((wy-lastY)>1) {
            m_ty += (wy-lastY);  // / 4 * 3;
            changed = true;
        } else if ((lastY-wy)>1) {
            m_ty -= (lastY-wy);  // / 4 * 3;
            changed = true;
        }
        
        if (changed)    Refresh();
        lastX = wx;
        lastY = wy;
        //wxLogMessage( "out SurfViewCanvas::OnMouseMove" );
        return;
    }

    const int  border = 40;

    //rotate mode
    if (mRotateOn) {
        if (lastX==-1 || lastY==-1) {
            lastX = wx;
            lastY = wy;
            return;
        }

        int  dx = wx - lastX;
        int  dy = wy - lastY;
        RenderEvent::EventType what;
        double angle;
        switch (mRotateMode) {
            case rxMode :
                what = RenderEvent::SET_X;
                angle = dy/mf->mSpeed;
                break;
            case ryMode :
                what = RenderEvent::SET_Y;
                angle = -dx/mf->mSpeed;
                break;
            case rzMode :
                what = RenderEvent::SET_Z;
                angle = dx/mf->mSpeed;
                break;
            default:
                return;
        }
        eventQueue.enqueue( mRenderer, what, e.GetTimestamp(), angle );

        bool     warp = false;
        int      tempX = wx;
        int      tempY = wy;
        if      (tempY > r.height-border) { tempY = border;           warp = true; }
        else if (tempY < border)          { tempY = r.height-border;  warp = true; }
        if      (tempX > r.width-border)  { tempX = border;           warp = true; }
        else if (tempX < border)          { tempX = r.width-border;   warp = true; }
        if (warp)    WarpPointer( tempX, tempY );

        ensureRenderThreadRunning();

        lastX = wx;
        lastY = wy;
        return;
    }

    switch (mWhichMode)
    {
        case MOVE:
            if (lastX==-1 || lastY==-1) {
                lastX = wx;
                lastY = wy;
                return;
            }
            RenderEvent::EventType what;
            double d1, d2;
            switch (mControlState)
            {
                case 3:
                    what = RenderEvent::SET_LINE_X;
                    d1 = (wy-lastY)/mf->mSpeed;
                    break;
                case 4:
                    what = RenderEvent::SET_LINE_Y;
                    d1 = -(wx-lastX)/mf->mSpeed;
                    break;
                case 5:
                    what = RenderEvent::SET_LINE_Z;
                    d1 = (wx-lastX)/mf->mSpeed;
                    break;
                case 6:
                case 7:
                case 8:
                    what = RenderEvent::SET_LINE_XY;
                    d1 = (wx-lastX)*32/mf->mSpeed;
                    d2 = (wy-lastY)*32/mf->mSpeed;
                    break;
                case 10:
                    what = RenderEvent::TRANSLATE;
                    d1 = (wx-lastX)*32/mf->mSpeed;
                    break;
                case 11:
                    what = RenderEvent::SET_C;
                    d1 = (wx-lastX)/mf->mSpeed;
                    break;
                default:
                    return;
            }
            if (what == RenderEvent::SET_LINE_XY)
                eventQueue.enqueue(mRenderer, what, e.GetTimestamp(), d1, d2);
            else
                eventQueue.enqueue(mRenderer, what, e.GetTimestamp(), d1);
            r = GetRect();
            bool     warp; warp = false;
            int      tempX; tempX = wx;
            int      tempY; tempY = wy;
            if      (tempY > r.height-border) { tempY = border;           warp = true; }
            else if (tempY < border)          { tempY = r.height-border;  warp = true; }
            if      (tempX > r.width-border)  { tempX = border;           warp = true; }
            else if (tempX < border)          { tempX = r.width-border;   warp = true; }
            if (warp)    WarpPointer( tempX, tempY );

            ensureRenderThreadRunning();

            lastX = wx;
            lastY = wy;
            return;

        case REFLECT:
        case CUT_PLANE:
            if (mControlState == 12)
                return;
            // fall through
        case SELECT_SLICE:
            if (lastX==-1 || lastY==-1) {
                lastX = wx;
                lastY = wy;
                return;
            }
            switch (mControlState)
            {
                case 1:
                    what = RenderEvent::SET_PLANE_X;
                    d1 = (wy-lastY)/mf->mSpeed;
                    break;
                case 2:
                    what = RenderEvent::SET_PLANE_Y;
                    d1 = -(wx-lastX)/mf->mSpeed;
                    break;
                case 3:
                    what = RenderEvent::SET_PLANE_Z;
                    d1 = (wx-lastX)/mf->mSpeed;
                    break;
                case 4:
                case 5:
                case 6:
                case 12:
                    what = RenderEvent::SET_PLANE_T;
                    d1 = (wx-lastX)*32/mf->mSpeed;
                    break;
                default:
                    return;
            }
            eventQueue.enqueue(mRenderer, what, e.GetTimestamp(), d1);
            r = GetRect();
            warp = false;
            tempX = wx;
            tempY = wy;
            if      (tempY > r.height-border) { tempY = border;           warp = true; }
            else if (tempY < border)          { tempY = r.height-border;  warp = true; }
            if      (tempX > r.width-border)  { tempX = border;           warp = true; }
            else if (tempX < border)          { tempX = r.width-border;   warp = true; }
            if (warp)    WarpPointer( tempX, tempY );

            ensureRenderThreadRunning();

            lastX = wx;
            lastY = wy;
            return;
        case CREATE_MOVIE_TUMBLE:
            if (lastX==-1 || lastY==-1) {
                lastX = wx;
                lastY = wy;
                return;
            }

            int  dx = wx - lastX;
            int  dy = wy - lastY;
            double angle;
            switch (mControlState) {
                case 1:
                    what = RenderEvent::SET_X;
                    angle = dy/mf->mSpeed;
                    break;
                case 2:
                    what = RenderEvent::SET_Y;
                    angle = -dx/mf->mSpeed;
                    break;
                case 3:
                    what = RenderEvent::SET_Z;
                    angle = dx/mf->mSpeed;
                    break;
                default:
                    return;
            }
            eventQueue.enqueue( mRenderer, what, e.GetTimestamp(), angle );

            warp = false;
            tempX = wx;
            tempY = wy;
            if      (tempY > r.height-border) { tempY = border;           warp = true; }
            else if (tempY < border)          { tempY = r.height-border;  warp = true; }
            if      (tempX > r.width-border)  { tempX = border;           warp = true; }
            else if (tempX < border)          { tempX = r.width-border;   warp = true; }
            if (warp)    WarpPointer( tempX, tempY );

            ensureRenderThreadRunning();

            lastX = wx;
            lastY = wy;
            return;
        }

    //no mode
    if (lastX!=-1 || lastY!=-1) {
        SetCursor( *wxSTANDARD_CURSOR );
        lastX = lastY = -1;
    }

}
//----------------------------------------------------------------------
void SurfViewCanvas::OnRightDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    wxClientDC  dc( this );
    PrepareDC( dc );
    const long  wx = dc.DeviceToLogicalX( pos.x );
    const long  wy = dc.DeviceToLogicalY( pos.y );
    lastX = wx;
    lastY = wy;
    if (mRotateOn)
    {
        switch (mRotateMode) {
            case rxMode :
            case ryMode :
            case rzMode :
                mRotateMode = 0;
                break;
            default:
                switch (mLastRotateMode) {
                    case rxMode:
                    case ryMode:
                    case rzMode:
                        mRotateMode = mLastRotateMode;
                        break;
                    default:
                        mRotateMode = rxMode;
                        break;
                }
                break;
        }
        switch (mRotateMode) {
            case rxMode :
                SetCursor( wxCursor(wxCURSOR_SIZENS) );
                m_parent_frame->SetStatusText( "X/", 2 );
                m_parent_frame->SetStatusText( "Release", 4 );
                break;
            case ryMode :
                SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                m_parent_frame->SetStatusText( "Y/", 2 );
                m_parent_frame->SetStatusText( "Release", 4 );
                break;
            case rzMode :
                SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                m_parent_frame->SetStatusText( "Z/", 2 );
                m_parent_frame->SetStatusText( "Release", 4 );
                break;
            default:
                SetCursor( wxCursor(wxCURSOR_HAND) );
                m_parent_frame->SetStatusText( "Scroll", 2 );
                m_parent_frame->SetStatusText( "Rotate Object", 4 );
                break;
        }
        return;
    }
    switch (mWhichMode) {
        case SEPARATE:
        case CUT_CURVED:
        case ROI_STATISTICS:
            switch (mControlState) {
                case 0:
                    break;
                case 1:
                    if (nvertices < 3)
                    {
                        wxMessageBox("Not enough vertices to close.");
                        return;
                    }
                    PointInfo *tpoints;
                    int tnpoints, max_points, npoints;
                    max_points = 99;
                    tpoints= (PointInfo *)malloc(max_points*sizeof(PointInfo));
                    tnpoints = 0;
                    for (int p=0; p<nvertices; p++)
                    {
                        v_draw_line(vertices[p].x, vertices[p].y, &tpoints,
                            &tnpoints, &max_points);
                    }
                    free(vertices);
                    X_Point *points;
                    v_close_curve(&tpoints, tnpoints, max_points, &points,
                        &npoints, &vertices, &nvertices);
                    free(tpoints);
                    free(points);
                    vertices = (X_Point *)
                        realloc(vertices, (nvertices+1)*sizeof(X_Point));
                    vertices[nvertices++] = vertices[0];
                    if (mWhichMode == ROI_STATISTICS)
                    {
                        mControlState = 0;
                        {
                            wxCriticalSectionLocker  lock( mCriticalSection );
                            //critical section
                            //vvvvvvvvvvvvvvvv
    
                            eventQueue.enqueue(mRenderer,
                                RenderEvent::GET_SLICE_ROI_STATS,
                                e.GetTimestamp(), vertices, nvertices,
                                true, 0, false);
                        }
                        ensureRenderThreadRunning();
                        bool synched = false;
                        while (!synched)
                        {
                            wxSafeYield(NULL, true);
                            wxCriticalSectionLocker  lock( mCriticalSection );
                            //critical section
                            //vvvvvvvvvvvvvvvv

                            synched = stats_valid;
                        }
                        free(vertices);
                        nvertices = 0;
                    }
                    else
                        mControlState = 2;
                    Refresh();
                    setActionText();
                    break;
                case 2:
                    free(vertices);
                    nvertices = 0;
                    mControlState = 0;
                    Refresh();
                    m_parent_frame->SetStatusText("", 0);
                    m_parent_frame->SetStatusText("Draw curve", 2);
                    m_parent_frame->SetStatusText("Select object", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
            }
            break;
        case VIEW:
            switch (mControlState) {
                case 0:
                    SetCursor( wxCursor(wxCURSOR_HAND) );
                    return;
                case 1:
                    break;
            }
            break;
        case MOVE:
            switch (mControlState) {
                case 1:
                case 9:
                case 10:
                case 11:
                    mControlState = 0;
                    mRenderer->line = FALSE;
                    if (mRotateMode==toMode || mRotateMode==roMode)
                    {
                        mRotateMode = 0;
                        m_tx -= mOverallXSize/2;
                        m_ty -= mOverallYSize/2;
                        Refresh();
                    }
                    m_parent_frame->SetStatusText("Move Objects", 2);
                    m_parent_frame->SetStatusText("Select object", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
                case 2:
                    switch (mRenderer->points_selected) {
                        case 0:
                            wxMessageBox("Select two points.");
                            break;
                        case 1:
                            wxMessageBox("Select one more point.");
                            break;
                        case 2:
                            mControlState = 9;
                            m_parent_frame->SetStatusText("Rotate", 2);
                            m_parent_frame->SetStatusText("Translate", 3);
                            m_parent_frame->SetStatusText("Done", 4);
                    }
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    mControlState = 9;
                    m_parent_frame->SetStatusText("Rotate", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    m_parent_frame->SetStatusText("Done", 4);
            }
            break;
        case SELECT_SLICE:
            switch (mControlState) {
                case 0:
                    mControlState = 12;
                    m_tx += mOverallXSize/2; 
                    m_ty += mOverallYSize/2;
                    Refresh();
                    setActionText();
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 12:
                    SetCursor( *wxSTANDARD_CURSOR );
                    m_tx -= mOverallXSize/2;
                    m_ty -= mOverallYSize/2;
                    Refresh();
                    // fall through
                case 7:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                    mControlState = 0;
                    setActionText();
                    break;
            }
            break;
        case MEASURE:
            mControlState = !mControlState;
            setActionText();
            break;
        case CREATE_MOVIE_TUMBLE:
            switch (mControlState) {
                case 1:
                case 2:
                case 3:
                case 5:
                    mControlState = 0;
                    setActionText();
                    SetCursor( *wxSTANDARD_CURSOR );
                    break;
            }
            break;
        case REFLECT:
        case CUT_PLANE:
            switch (mControlState) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    SetCursor( *wxSTANDARD_CURSOR );
                    mControlState = 12;
                    setActionText();
                    break;
                case 12:
                    m_tx -= mOverallXSize/2;
                    m_ty -= mOverallYSize/2;
                    mControlState = 0;
                    setActionText();
                    eventQueue.enqueue( mRenderer, RenderEvent::SET_MODE,
                        e.GetTimestamp(), mWhichMode );
                    ensureRenderThreadRunning();
                    break;
            }
            break;
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnRightUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
void SurfViewCanvas::selectObject(long wx, long wy, int xpos2, long timestamp)
{
    SurfViewFrame *mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    {
        wxCriticalSectionLocker lock( mCriticalSection ); //critical section
                                                          //vvvvvvvvvvvvvvvv

        eventQueue.enqueue( mRenderer, RenderEvent::SET_OBJECT, -1, 0 );
    }
    ensureRenderThreadRunning();
    bool synched = false;
    while (!synched)
    {
         wxSafeYield(NULL, true);
         wxCriticalSectionLocker  lock( mCriticalSection ); //critical section
                                                            //vvvvvvvvvvvvvvvv

         synched = mRenderer->selected_object==0;
         // Wait until object is deselected.
    }
    {
        int x = wx-m_tx;
        if (x >= xpos2)
            x -= xpos2;
        int y = wy-m_ty;
        if (x<0 || x>=mOverallXSize ||
                y<0 || y>=mOverallYSize)
            return;
        x -= mOverallXSize/2;
        y -= mOverallYSize/2;

        wxCriticalSectionLocker lock( mCriticalSection ); //critical section
                                                          //vvvvvvvvvvvvvvvv

        int image_number = wx-m_tx<xpos2? 1:2;
        eventQueue.enqueue( mRenderer, RenderEvent::SELECT_OBJECT,
            timestamp, x, y, image_number );
    }
    ensureRenderThreadRunning();
    synched = false;
    while (!synched)
    {
         wxSafeYield(NULL, true);
         wxCriticalSectionLocker  lock( mCriticalSection ); //critical section
                                                            //vvvvvvvvvvvvvvvv

         synched = mRenderer->selected_object!=0;
         // Wait until object, then we may need to update object switch.
    }
    mf->updateObjectSwitch();
}

void SurfViewCanvas::OnMiddleDown ( wxMouseEvent& e ) {

    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    wxClientDC  dc( this );
    PrepareDC( dc );
    const long  wx = dc.DeviceToLogicalX( pos.x );
    const long  wy = dc.DeviceToLogicalY( pos.y );
    lastX = wx;
    lastY = wy;
    const int xpos2 = (int)ceil(mOverallXSize*m_scale+sSpacing);
    SurfViewFrame*  mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    if (mRotateOn) {
        return;
    }
    if (mWhichMode == VIEW)
    {
        if (wx-m_tx<xpos2)
            selectObject(wx, wy, xpos2, e.GetTimestamp());
        return;
    }
    switch (mWhichMode) {
        bool synched;
		int cut_count, old_cut_count;
        case SEPARATE:
        case CUT_CURVED:
        case ROI_STATISTICS:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx<xpos2 || mWhichMode==SEPARATE)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                    X_Point *newvertices;
                    if (nvertices == 0)
                        newvertices = (X_Point *)malloc(sizeof(X_Point));
                    else
                    {
                        if (wx-m_tx==vertices[nvertices-1].x &&
                                wy-m_ty==vertices[nvertices-1].y)
                            return;
                        newvertices = (X_Point *)realloc(vertices,
                            (nvertices+1)*sizeof(X_Point));
                    }
                    if (newvertices == NULL)
                        return;
                    vertices = newvertices;
                    vertices[nvertices].x = wx-m_tx;
                    vertices[nvertices].y = wy-m_ty;
                    nvertices++;
                    Refresh();
                    break;
                case 2:
                    inside = is_in_polygon(wx-m_tx, wy-m_ty, vertices,
                        nvertices)!=0;
                    mControlState = 3;
                    m_parent_frame->SetStatusText("Specify depth.", 0);
                    m_parent_frame->SetStatusText("Entire", 2);
                    m_parent_frame->SetStatusText("Use slider", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
                case 3:
                    const bool do_cut_flag = visible_object_type(
                        mRenderer->object_list)==SHELL0;
                    if (do_cut_flag && mWhichMode==SEPARATE)
                    {
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        bool primary = vertices[0].x<xpos2;
                        if (!primary)
                            for (int j=0; j<nvertices; j++)
                                vertices[j].x -= xpos2;
                        eventQueue.enqueue(mRenderer,RenderEvent::SEPARATE,
                            e.GetTimestamp(), vertices, nvertices,
                            (inside!=0), cut_depth, primary);
                    }
                    else if (do_cut_flag)
                    {
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

						old_cut_count = mRenderer->cut_count;
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::CUT_CURVED,
                            e.GetTimestamp(), vertices, nvertices,
                            (inside!=0), cut_depth, true);
                    }
                    ensureRenderThreadRunning();
                    synched = !do_cut_flag;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        if (mWhichMode == SEPARATE)
                        {
                            synched = mRenderer->separate_piece2!=NULL ||
                                mRenderer->error_flag;
                            // Wait until separate_piece2 is created, then
                            // we may need to update object switch.
                        }
                        else
						{
							cut_count = mRenderer->cut_count;
                            synched = cut_count>old_cut_count ||
                                mRenderer->error_flag;
                    	}
                    }
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox("Operation failed.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                    }
                    mf->updateObjectSwitch();
                    free(vertices);
                    nvertices = 0;
                    mControlState = 0;
                    m_parent_frame->SetStatusText("", 0);
                    m_parent_frame->SetStatusText("Draw curve", 2);
                    m_parent_frame->SetStatusText("Select object", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
            }
            break;
        case MOVE:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx < xpos2)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                    mControlState = 3;
                    mRenderer->line = TRUE;
                    m_parent_frame->SetStatusText("X/", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    m_parent_frame->SetStatusText("Accept", 4);
                    break;
                case 2:
                    // Undo
                    if (mRenderer->points_selected)
                    {
                        if (mRenderer->points_selected == 1)
                            m_parent_frame->SetStatusText("", 3);
                        else
                        {
                            m_parent_frame->SetStatusText("Select point", 2);
                            m_parent_frame->SetStatusText("", 4);
                        }
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::UNSELECT_LINE_POINT,
                            e.GetTimestamp());
                        ensureRenderThreadRunning();
                        break;
                    }
                    // else
                    wxMessageBox("No points are already selected.");
                    break;
                case 3:
                case 4:
                case 5:
                    mControlState += 3;
                    m_parent_frame->SetStatusText("Rotate", 2);
                    m_parent_frame->SetStatusText("", 3);
                    break;
                case 9:
                    m_tx += mOverallXSize/2;
                    m_ty += mOverallYSize/2;
                    // fall through
                case 11:
                    mControlState = 10;
                    mRotateMode = toMode;
                    m_parent_frame->SetStatusText("Rotate", 2);
                    m_parent_frame->SetStatusText("", 3);
                    m_parent_frame->SetStatusText("Done", 4);
                    break;
            }
            break;
        case SELECT_SLICE:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx < xpos2)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                case 2:
                case 3:
                    SetCursor( *wxSTANDARD_CURSOR );
                    mControlState += 3;
                    m_parent_frame->SetStatusText("Rotate", 2);
                    m_parent_frame->SetStatusText("", 3);
                    break;
                case 12:
                    eventQueue.enqueue(mRenderer,
                        RenderEvent::SET_LAST_LOC, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    Refresh();
                    break;
                case 13:
                    mControlState = 14;
                    eventQueue.enqueue(mRenderer,
                            RenderEvent::SET_PLANE_AXIAL, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    m_parent_frame->SetStatusText("Axial/", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    break;
                case 14:
                case 15:
                case 16:
                    m_tx += mOverallXSize/2;
                    m_ty += mOverallYSize/2;
                    mControlState = 4;
                    m_parent_frame->SetStatusText("Rotate", 2);
                    m_parent_frame->SetStatusText("", 3);
                    break;
            }
            break;
        case MEASURE:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx < xpos2)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                    // erase mark
                    int x; x = wx-m_tx;
                    int y; y = wy-m_ty;
                    if (x<0 || x>=mOverallXSize || y<0 || y>=mOverallYSize)
                        break;
                    int old_nmarks;
                    Shell *obj;
                    for (old_nmarks=0, obj=mRenderer->object_list; obj;
                            obj=obj->next)
                        old_nmarks += obj->marks;
                    if (old_nmarks == 0)
                        break;
                    eventQueue.enqueue(mRenderer,
                        RenderEvent::ERASE_MARK, e.GetTimestamp(), x, y);
                    ensureRenderThreadRunning();
                    synched = false;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker lock(mCriticalSection);
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        int nmarks=0;
                        for (obj=mRenderer->object_list; obj; obj=obj->next)
                            nmarks += obj->marks;
                        synched = nmarks<old_nmarks || mRenderer->error_flag;
                    }
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox("No mark showing at that location.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                    }
                    break;
            }
            break;
        case CREATE_MOVIE_TUMBLE:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx < xpos2)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                case 2:
                case 3:
                    {
                        wxCriticalSectionLocker lock(mCriticalSection);
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        if (key_pose_array_size <= key_poses)
                        {
                            if (key_pose_array_size)
                                key_pose = (Key_pose *)realloc(key_pose,
                                    (key_poses+1)*sizeof(Key_pose));
                            else
                                key_pose= (Key_pose *)malloc(sizeof(Key_pose));
                            key_pose_array_size = key_poses+1;
                        }
                        memcpy(&key_pose[key_poses].angle,
                            mRenderer->glob_angle, sizeof(key_pose[0].angle));
                        key_pose[key_poses].views = mf->getIntermediateViews();
                        key_poses++;
                    }
                    mControlState = 5;
                    setActionText();
                    m_parent_frame->SetStatusText( wxString::Format(
                        "Key pose %d defined.", key_poses ), 0 );
                    SetCursor( *wxSTANDARD_CURSOR );
                    break;
                case 5:
                    // close
                    if (key_poses < 2)
                        break;
                    if (key_pose_array_size <= key_poses)
                    {
                        if (key_pose_array_size)
                            key_pose = (Key_pose *)realloc(key_pose,
                                (key_poses+1)*sizeof(Key_pose));
                        else
                            key_pose = (Key_pose *)malloc(sizeof(Key_pose));
                        key_pose_array_size = key_poses+1;
                    }
                    memcpy(&key_pose[key_poses].angle, &key_pose[0].angle,
                        sizeof(key_pose[0].angle));
                    key_pose[key_poses].views = mf->getIntermediateViews();
                    key_poses++;
                    eventQueue.enqueue(mRenderer, RenderEvent::SET_VIEW,
                        e.GetTimestamp(), key_pose[0].angle[0],
                        key_pose[0].angle[1], key_pose[0].angle[2], -1);
                    ensureRenderThreadRunning();
                    mControlState = 0;
                    setActionText();
                    m_parent_frame->SetStatusText( wxString::Format(
                        "Key pose %d defined.", key_poses ), 0 );
                    break;
                case 12:
                case 13:
                    // go continuous
                    mControlState = 13;
                    mf->updateIntermediateViews();
                    m_cine_timer->Start(::gTimerInterval, true);
                    break;
            }
            break;
        case PREVIOUS_SEQUENCE:
            switch (mControlState) {
                case 12:
                case 13:
                    // go continuous
                    mControlState = 13;
                    mf->updateIntermediateViews();
                    m_cine_timer->Start(::gTimerInterval, true);
                    break;
            }
            break;
        case REFLECT:
        case CUT_PLANE:
            switch (mControlState) {
                case 0:
                    if (wx-m_tx < xpos2)
                        selectObject(wx, wy, xpos2, e.GetTimestamp());
                    break;
                case 1:
                case 2:
                case 3:
                    SetCursor( *wxSTANDARD_CURSOR );
                    mControlState += 3;
                    setActionText();
                    break;
                case 12:
                    OnLeftDown(e);
                    break;
            }
            break;
    }

}
//----------------------------------------------------------------------
void SurfViewCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnLeftDown ( wxMouseEvent& e ) {

    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    wxClientDC  dc( this );
    PrepareDC( dc );
    const long  wx = dc.DeviceToLogicalX( pos.x );
    const long  wy = dc.DeviceToLogicalY( pos.y );
    lastX = wx;
    lastY = wy;
    const int xpos2 = (int)ceil(mOverallXSize*m_scale+sSpacing);
    SurfViewFrame*  mf = dynamic_cast<SurfViewFrame*>( m_parent_frame );
    if (mRotateOn)
    {
        if (mRotateMode)
        {
            assert(mRotateMode <= rzMode);
            ++mRotateMode;
            if (mRotateMode>rzMode)    mRotateMode = rxMode;
            switch (mRotateMode) {
                case rxMode :
                    SetCursor( wxCursor(wxCURSOR_SIZENS) );
                    m_parent_frame->SetStatusText( "X/", 2 );
                    break;
                case ryMode :
                    SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                    m_parent_frame->SetStatusText( "Y/", 2 );
                    break;
                case rzMode :
                    SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                    m_parent_frame->SetStatusText( "Z/", 2 );
                    break;
            }
            m_parent_frame->SetStatusText( "Quit", 4 );
        }
        return;
    }
    switch (mWhichMode) {
        bool synched;
		int cut_count, old_cut_count;
        case SEPARATE:
        case CUT_CURVED:
        case ROI_STATISTICS:
            switch (mControlState) {
                case 0:
                    mControlState = 1;
                    m_parent_frame->SetStatusText("Specify point on curve."
                        , 0);
                    m_parent_frame->SetStatusText("Select point", 2);
                    m_parent_frame->SetStatusText("Select point", 3);
                    m_parent_frame->SetStatusText("Close", 4 );
                    if (mWhichMode == ROI_STATISTICS)
                    {
                        eventQueue.enqueue( mRenderer, RenderEvent::SET_MODE,
                            e.GetTimestamp(), ROI_STATISTICS );
                        ensureRenderThreadRunning();
                    }
                    break;
                case 1:
                    X_Point *newvertices;
                    if (nvertices == 0)
                        newvertices = (X_Point *)malloc(sizeof(X_Point));
                    else
                    {
                        if (wx-m_tx==vertices[nvertices-1].x &&
                                wy-m_ty==vertices[nvertices-1].y)
                            return;
                        newvertices = (X_Point *)realloc(vertices,
                            (nvertices+1)*sizeof(X_Point));
                    }
                    if (newvertices == NULL)
                        return;
                    vertices = newvertices;
                    vertices[nvertices].x = wx-m_tx;
                    vertices[nvertices].y = wy-m_ty;
                    nvertices++;
                    Refresh();
                    break;
                case 2:
                    inside = is_in_polygon(wx-m_tx, wy-m_ty, vertices,
                        nvertices)!=0;
                    mControlState = 3;
                    m_parent_frame->SetStatusText("Specify depth.", 0);
                    m_parent_frame->SetStatusText("Entire", 2);
                    m_parent_frame->SetStatusText("Use slider", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
                case 3:
                    const bool do_cut_flag = visible_object_type(
                        mRenderer->object_list)==SHELL0;
                    if (do_cut_flag && mWhichMode==SEPARATE)
                    {
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        float cut_depth =
                            Z_BUFFER_LEVELS/mRenderer->depth_scale;
                        bool primary = vertices[0].x<xpos2;
                        if (!primary)
                            for (int j=0; j<nvertices; j++)
                                vertices[j].x -= xpos2;
                        eventQueue.enqueue(mRenderer,RenderEvent::SEPARATE,
                            e.GetTimestamp(), vertices, nvertices,
                            (inside!=0), cut_depth, primary);
                    }
                    else if (do_cut_flag)
                    {
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

						old_cut_count = mRenderer->cut_count;
                        float cut_depth =
                            Z_BUFFER_LEVELS/mRenderer->depth_scale;
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::CUT_CURVED,
                            e.GetTimestamp(), vertices, nvertices,
                            (inside!=0), cut_depth, true);
                    }
                    ensureRenderThreadRunning();
                    synched = !do_cut_flag;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker  lock( mCriticalSection );
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        if (mWhichMode == SEPARATE)
                        {
                            synched = mRenderer->separate_piece2!=NULL ||
                                mRenderer->error_flag;
                            // Wait until separate_piece2 is created, then
                            // we may need to update object switch.
                        }
                        else
						{
							cut_count = mRenderer->cut_count;
                            synched = cut_count>old_cut_count ||
                                mRenderer->error_flag;
                    	}
					}
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox("Operation failed.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                    }
                    mf->updateObjectSwitch();
                    free(vertices);
                    nvertices = 0;
                    mControlState = 0;
                    m_parent_frame->SetStatusText("", 0);
                    m_parent_frame->SetStatusText("Draw curve", 2);
                    m_parent_frame->SetStatusText("Select object", 3);
                    m_parent_frame->SetStatusText("", 4 );
                    break;
            }
            break;
        case VIEW:
            break;
        case MOVE:
            switch (mControlState) {
                case 0:
                    mControlState = 1;
                    m_parent_frame->SetStatusText("Via points", 2);
                    m_parent_frame->SetStatusText("Interactive", 3);
                    m_parent_frame->SetStatusText("Cancel", 4);
                    break;
                case 1:
                    mControlState = 2;
                    mRenderer->points_selected = 0;
                    m_parent_frame->SetStatusText("Select point", 2);
                    m_parent_frame->SetStatusText("", 3);
                    m_parent_frame->SetStatusText("", 4);
                    break;
                case 2:
                    int old_points_selected;
                    old_points_selected = mRenderer->points_selected;
                    if (old_points_selected < 2)
                    {
                        int x = wx-m_tx;
                        int y = wy-m_ty;
                        if (x<0 || x>=mOverallXSize || y<0 ||
                                y>=mOverallYSize)
                            break;
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::SELECT_LINE_POINT,
                            e.GetTimestamp(), x, y);
                        ensureRenderThreadRunning();
                        synched = false;
                        while (!synched)
                        {
                            wxSafeYield(NULL, true);
                            wxCriticalSectionLocker lock(mCriticalSection);
                            //critical section
                            //vvvvvvvvvvvvvvvv

                            synched = mRenderer->points_selected>
                                old_points_selected ||
                                mRenderer->error_flag;
                        }
                        if (mRenderer->error_flag)
                        {
                            wxMessageBox("No object at that location or points are identical.");
                            eventQueue.enqueue(mRenderer,
                                RenderEvent::RESET_ERROR);
                        }
                        else if (mRenderer->points_selected == 2)
                        {
                            m_parent_frame->SetStatusText("", 2);
                            m_parent_frame->SetStatusText("Undo", 3);
                            m_parent_frame->SetStatusText("Accept", 4);
                        }
                        else
                        {
                            m_parent_frame->SetStatusText("Select point",
                                2);
                            m_parent_frame->SetStatusText("Undo", 3);
                            m_parent_frame->SetStatusText("", 4);
                        }
                        break;
                    }
                    // else
                    wxMessageBox("Two points are already selected.");
                    break;
                case 3:
                case 7:
                    mControlState = 4;
                    m_parent_frame->SetStatusText("Y/", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    break;
                case 4:
                case 8:
                    mControlState = 5;
                    m_parent_frame->SetStatusText("Z/", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    break;
                case 5:
                case 6:
                    mControlState = 3;
                    m_parent_frame->SetStatusText("X/", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    break;
                case 9:
                    m_tx += mOverallXSize/2;
                    m_ty += mOverallYSize/2;
                    // fall through
                case 10:
                    mControlState = 11;
                    mRotateMode = roMode;
                    m_parent_frame->SetStatusText("", 2);
                    m_parent_frame->SetStatusText("Translate", 3);
                    m_parent_frame->SetStatusText("Done", 4);
                    break;
            }
            break;
        case SELECT_SLICE:
            switch (mControlState) {
                case 0:
                    mControlState = 13;
                    setActionText();
                    break;
                case 1:
                case 5:
                    SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                    mControlState = 2;
                    setActionText();
                    break;
                case 2:
                case 6:
                    SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                    mControlState = 3;
                    setActionText();
                    break;
                case 3:
                case 4:
                    SetCursor( wxCursor(wxCURSOR_SIZENS) );
                    mControlState = 1;
                    setActionText();
                    break;
                case 12:
                    eventQueue.enqueue(mRenderer,
                        RenderEvent::SET_FIRST_LOC, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    Refresh();
                    break;
                case 13:
                    m_tx += mOverallXSize/2;
                    m_ty += mOverallYSize/2;
                    SetCursor( wxCursor(wxCURSOR_SIZENS) );
                    mControlState = 1;
                    setActionText();
                    break;
                case 14:
                    mControlState = 15;
                    eventQueue.enqueue(mRenderer,
                            RenderEvent::SET_PLANE_SAGITTAL, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    m_parent_frame->SetStatusText("Sagittal/", 2);
                    break;
                case 15:
                    mControlState = 16;
                    eventQueue.enqueue(mRenderer,
                            RenderEvent::SET_PLANE_CORONAL, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    m_parent_frame->SetStatusText("Coronal/", 2);
                    break;
                case 16:
                    mControlState = 14;
                    eventQueue.enqueue(mRenderer,
                            RenderEvent::SET_PLANE_AXIAL, e.GetTimestamp());
                    ensureRenderThreadRunning();
                    m_parent_frame->SetStatusText("Axial/", 2);
                    break;
            }
            break;
        case MEASURE:
            switch (mControlState) {
                case 0:
                    int old_nmeasurement_points;
                    old_nmeasurement_points = mRenderer->nmeasurement_points;

                    int x; x = wx-m_tx;
                    int y; y = wy-m_ty;
                    if (x<0 || x>=xpos2+mOverallXSize || y<0 ||
                            y>=mOverallYSize)
                        break;
                    if (x < xpos2)
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::SELECT_MEASUREMENT_POINT,
                            e.GetTimestamp(), x, y, TRUE);
                    else
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::SELECT_MEASUREMENT_POINT,
                            e.GetTimestamp(), x-xpos2, y, FALSE);
                    ensureRenderThreadRunning();
                    synched = false;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker lock(mCriticalSection);
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        synched = mRenderer->nmeasurement_points>
                            old_nmeasurement_points ||
                            mRenderer->error_flag;
                    }
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox("No object at that location.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                    }
                    break;
                case 1:
                    // Mark
                    x = wx-m_tx;
                    y = wy-m_ty;
                    if (x<0 || x>=mOverallXSize || y<0 || y>=mOverallYSize)
                        break;
                    int old_nmarks;
                    Shell *obj;
                    for (old_nmarks=0, obj=mRenderer->object_list; obj;
                            obj=obj->next)
                        old_nmarks += obj->marks;
                    eventQueue.enqueue(mRenderer,
                        RenderEvent::SET_MARK, e.GetTimestamp(), x, y);
                    ensureRenderThreadRunning();
                    synched = false;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker lock(mCriticalSection);
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        int nmarks=0;
                        for (obj=mRenderer->object_list; obj; obj=obj->next)
                            nmarks += obj->marks;
                        synched = nmarks>old_nmarks || mRenderer->error_flag;
                    }
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox(
                            "No markable object at that location.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                    }
                    break;
            }
            break;
        case PREVIOUS_SEQUENCE:
        case CREATE_MOVIE_TUMBLE:
            switch (mControlState) {
                case 0:
                    switch (mLastRotateMode) {
                        default:
                            mControlState = 1;
                            SetCursor( wxCursor(wxCURSOR_SIZENS) );
                            setActionText();
                            break;
                        case ryMode:
                            mControlState = 2;
                            SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                            setActionText();
                            break;
                        case rzMode:
                            mControlState = 3;
                            SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                            setActionText();
                            break;
                    }
                    break;
                case 3:
                case 5:
                    mControlState = 1;
                    SetCursor( wxCursor(wxCURSOR_SIZENS) );
                    setActionText();
                    break;
                case 1:
                    mControlState = 2;
                    SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                    setActionText();
                    break;
                case 2:
                    mControlState = 3;
                    SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                    setActionText();
                    break;
                case 12:
                    // next frame
                    if (preview_key_pose == key_poses-1)
                        break;
                    double this_angle[3];
                    if (!preview_all_poses || preview_key_pose<0 ||
                            preview_view>=key_pose[preview_key_pose+1].views)
                    {
                        preview_key_pose++;
                        preview_view = 0;
                        memcpy(this_angle, &key_pose[preview_key_pose].angle,
                            sizeof(this_angle));
                        mf->updateIntermediateViews();
                    }
                    else
                    {
                        preview_view++;
                        ::view_interpolate(this_angle,
                            key_pose[preview_key_pose].angle,
                            key_pose[preview_key_pose+1].angle, preview_view/
                            (key_pose[preview_key_pose+1].views+1.0));
                    }
                    eventQueue.enqueue(mRenderer, RenderEvent::SET_VIEW,
                        e.GetTimestamp(), this_angle[0], this_angle[1],
                        this_angle[2], -1);
                    ensureRenderThreadRunning();
                    break;
                case 13:
                    mControlState = 12;
                    mf->updateIntermediateViews();
                    m_cine_timer->Stop();
                    break;
            }
            break;
        case REFLECT:
        case CUT_PLANE:
            switch (mControlState) {
                case 0:
                    m_tx += mOverallXSize/2;
                    m_ty += mOverallYSize/2;
                    eventQueue.enqueue(mRenderer,
                        RenderEvent::TURN_REFLECTIONS_OFF);
                    ensureRenderThreadRunning();
                    // fall through
                case 3:
                case 4:
                    SetCursor( wxCursor(wxCURSOR_SIZENS) );
                    mControlState = 1;
                    setActionText();
                    break;
                case 1:
                case 5:
                    SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                    mControlState = 2;
                    setActionText();
                    break;
                case 2:
                case 6:
                    SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                    mControlState = 3;
                    setActionText();
                    break;
                case 12:
                    int x, y;
                    x = wx-m_tx+mOverallXSize/2;
                    y = wy-m_ty+mOverallYSize/2;
                    if (x<0 || x>=mOverallXSize || y<0 || y>=mOverallYSize)
                        break;
                    eventQueue.enqueue(mRenderer,
                        mWhichMode==CUT_PLANE? RenderEvent::CUT_PLANE:
                        RenderEvent::TURN_REFLECTIONS_ON, e.GetTimestamp(),
                        x, y);
                    ensureRenderThreadRunning();
                    synched = false;
                    while (!synched)
                    {
                        wxSafeYield(NULL, true);
                        wxCriticalSectionLocker lock(mCriticalSection);
                        //critical section
                        //vvvvvvvvvvvvvvvv

                        synched = !mRenderer->plane || mRenderer->error_flag;
                    }
                    if (mRenderer->error_flag)
                    {
                        wxMessageBox("No object at that location, or too many objects on.");
                        eventQueue.enqueue(mRenderer,
                            RenderEvent::RESET_ERROR);
                        break;
                    }
                    m_tx -= mRenderer->main_image->width/2;
                    m_ty -= mRenderer->main_image->height/2;
                    mControlState = 0;
                    rerender();
                    setActionText();
                    mf->updateObjectSwitch();
                    break;
            }
            break;
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnCineTimer ( wxTimerEvent& e ) {
    // next frame
    if ((mWhichMode!=CREATE_MOVIE_TUMBLE && mWhichMode!=PREVIOUS_SEQUENCE) ||
            mControlState!=13 || preview_key_pose==key_poses-1)
        return;
    double this_angle[3];
    if (!preview_all_poses || preview_key_pose<0 ||
            preview_view>=key_pose[preview_key_pose+1].views)
    {
        preview_key_pose++;
        preview_view = 0;
        memcpy(this_angle, &key_pose[preview_key_pose].angle,
            sizeof(this_angle));
    }
    else
    {
        preview_view++;
        ::view_interpolate(this_angle,
            key_pose[preview_key_pose].angle,
            key_pose[preview_key_pose+1].angle, preview_view/
            (key_pose[preview_key_pose+1].views+1.0));
    }
    int this_frame=0;
    for (int kp=0; kp<preview_key_pose; kp++)
        this_frame += 1+key_pose[kp+1].views;
    this_frame += preview_view;
    eventQueue.enqueue(mRenderer, RenderEvent::SET_VIEW,
        e.GetTimestamp(), this_angle[0], this_angle[1],
        this_angle[2], this_frame);
    ensureRenderThreadRunning();
    bool synched = false;
    while (!synched)
    {
        wxSafeYield(NULL, true);
        wxCriticalSectionLocker lock(mCriticalSection);  //critical section
                                                         //vvvvvvvvvvvvvvvv
        synched = preview_frame_displayed==this_frame;
    }
    m_cine_timer->Start( ::gTimerInterval, true );
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnLeftUp ( wxMouseEvent& e ) {
    //wxMouseEvent*  me = new wxMouseEvent(e);
    //eventQueue.push_back( me );
    //eventQueue.enqueue( mRenderer, RenderEvent::LEFT_UP, e.GetTimestamp() );

    lastX = lastY = -1;
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnPaint ( wxPaintEvent& e ) {
    wxMutexLocker  lock( s_mutexProtectingTheGlobalData );
    assert( s_mutexProtectingTheGlobalData.IsOk() );

    wxMemoryDC  m;
    int         w, h;
    GetSize( &w, &h );
    wxBitmap    bitmap(w, h);
    m.SelectObject( bitmap );
    if (Preferences::getCustomAppearance())
#if wxCHECK_VERSION(2, 9, 0)
        m.SetBrush( wxBrush(wxColour(DkBlue), wxBRUSHSTYLE_SOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxBRUSHSTYLE_SOLID) );
#else
        m.SetBrush( wxBrush(wxColour(DkBlue), wxSOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxSOLID) );
#endif
    m.DrawRectangle( 0, 0, w, h );
    
    paint( &m );

    wxPaintDC  dc(this);
    PrepareDC(dc);
    //dc.BeginDrawing();
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
    //dc.EndDrawing();
}

/*****************************************************************************
 * FUNCTION: distance
 * DESCRIPTION: Returns the distance between two points.
 * PARAMETERS:
 *    point1, point2: The coordinates of two points
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The distance between the two points.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
double distance(double point1[3], double point2[3])
{
    return (sqrt((point2[0]-point1[0])*(point2[0]-point1[0])+
        (point2[1]-point1[1])*(point2[1]-point1[1])+
        (point2[2]-point1[2])*(point2[2]-point1[2])));
}

//----------------------------------------------------------------------
void SurfViewCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );

    if ((mRotateOn && mRotateMode) || mRotateMode==roMode
            || (mWhichMode==MOVE && mControlState>=3 && mControlState<=5) ||
            ((mWhichMode==SELECT_SLICE || mWhichMode==CREATE_MOVIE_TUMBLE ||
            mWhichMode==REFLECT || mWhichMode==CUT_PLANE) &&
            mControlState>=1 && mControlState<=3))
        m_parent_frame->SetStatusText( wxString::Format("%.1f degrees",
            mRotateAngle), 0 );
    else if (mRotateMode==toMode || ((mWhichMode==SELECT_SLICE||
            mWhichMode==REFLECT || mWhichMode==CUT_PLANE) &&
            mControlState>=4 && mControlState<=6))
        m_parent_frame->SetStatusText( wxString::Format("%.1f",
            mRotateAngle), 0 );
    else if (mWhichMode==SELECT_SLICE && mControlState==12)
    {
        if (mRenderer->out_slice_spacing <= 0)
            mRenderer->out_slice_spacing = 1/mRenderer->scale;
        int slices = (int)fabs((last_loc-first_loc)/
            mRenderer->out_slice_spacing)+1;
        if (slices > 1)
            m_parent_frame->SetStatusText(
                wxString::Format("%d slices.", slices), 0 );
        else
            m_parent_frame->SetStatusText("1 slice.", 0 );
    }
    else if (mWhichMode==MOVE && mControlState>=6 && mControlState<=8)
        m_parent_frame->SetStatusText( wxString::Format("(%.1f, %.1f)",
            mLineDX, mLineDY), 0 );
    else if (mWhichMode==ROI_STATISTICS && stats_valid)
        m_parent_frame->SetStatusText( wxString::Format(
            "Total density %.0f; Mean %.1f; St.dev. %.1f; Min %.0f; Max %.0f",
            total_density, mean_density, standard_deviation, min_density,
            max_density) );
    else if (mWhichMode==MEASURE && mControlState==0)
    {
        double total_length=0, this_length=0;
        for (int j=1; j<mRenderer->nmeasurement_points; j++)
        {
            this_length = distance(mRenderer->measurement_point[j],
                mRenderer->measurement_point[j-1]);
            total_length += this_length;
        }
        switch (mRenderer->nmeasurement_points)
        {
            case 0:
                break;
            case 1:
                m_parent_frame->SetStatusText( wxString::Format(
                    "Location: (%.1f, %.1f, %.1f)",
                    mRenderer->measurement_point[0][0]-
                    mRenderer->glob_displacement[0],
                    mRenderer->measurement_point[0][1]-
                    mRenderer->glob_displacement[1],
                    mRenderer->measurement_point[0][2]-
                    mRenderer->glob_displacement[2]), 0 );
                break;
            case 2:
                m_parent_frame->SetStatusText( wxString::Format(
                    "Total length %.1f; this segment %.1f",
                    total_length, this_length), 0 );
                break;
            default:
                m_parent_frame->SetStatusText( wxString::Format(
                    "Total length %.1f; this segment %.1f; angle %.1f degrees",
                    total_length, this_length, angle(mRenderer->
                    measurement_point[mRenderer->nmeasurement_points-3],
                    mRenderer->measurement_point[mRenderer->nmeasurement_points
                    -2], mRenderer->measurement_point[
                    mRenderer->nmeasurement_points-1])), 0 );
                break;
        }
    }
    if (m_bitmaps!=NULL) {
        int  i=0;
        int t_tx=m_tx, t_ty=m_ty;
        if (mRotateMode==toMode || mRotateMode==roMode ||
				((mWhichMode==CUT_PLANE
                ||mWhichMode==SELECT_SLICE||mWhichMode==REFLECT) &&
                ((mControlState>=1 && mControlState<=6) || mControlState==12)))
        {
            t_tx -= mOverallXSize/2;
            t_ty -= mOverallYSize/2;
        }
        for (int r=0; r<m_rows; r++) {
            //const int y = (int)(r*(m_ySize*m_scale+1)+0.5);
            const int y = (int)(r*(ceil(mOverallYSize*m_scale)+sSpacing));
            for (int c=0; c<m_cols; c++) {
                if (m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
                    //const int x = (int)(c*(m_xSize*m_scale+1)+0.5);
                    const int x = (int)(c*(ceil(mOverallXSize*m_scale)+sSpacing));
                    dc->DrawBitmap( *m_bitmaps[i], x+t_tx, y+t_ty );
                    //show the overlay?  (the overlay consists of numbers that indicate the slice)
                    if (m_overlay) {
                        const int sliceA = mCavassData->m_sliceNo+i;
                        const int sliceB = mCavassData->mNext->m_sliceNo+i;
                        //both in bounds?
                        if (mCavassData->inBounds(0,0,sliceA) && mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //both in bounds
                            const wxString  s = wxString::Format( "(%d/%d)", sliceA, sliceB );
                            dc->DrawText( s, x+t_tx, y+t_ty );
                        } else if (mCavassData->inBounds(0,0,sliceA) && !mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //only first in bounds
                            const wxString  s = wxString::Format( "(%d/-)", sliceA );
                            dc->DrawText( s, x+t_tx, y+t_ty );
                        } else if (!mCavassData->inBounds(0,0,sliceA) && mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //only second in bounds
                            const wxString  s = wxString::Format( "(-/%d)", sliceB );
                            dc->DrawText( s, x+t_tx, y+t_ty );
                        } else {
                            //both out of bounds
                            const wxString  s = wxString::Format( "(-/-)" );
                            dc->DrawText( s, x+t_tx, y+t_ty );
                        }
                    }
                }
                i++;
            }
        }
        dc->SetPen( wxPen(wxColour(Yellow)) );
        if (mRenderer->line)
            for (int j=0; j<mLinePixelCount; j++)
                dc->DrawPoint(mLinePixels[j].x+t_tx, mLinePixels[j].y+t_ty);
        for (int j=1; j<nvertices; j++)
            dc->DrawLine(vertices[j-1].x+t_tx, vertices[j-1].y+t_ty,
                vertices[j].x+t_tx, vertices[j].y+t_ty);
        for (int j=0; j<3; j++)
            if (mRenderer->box && mRenderer->box_axis_label_loc[j].x>=0 &&
                    mRenderer->
                    object_list->main_data.file->file_header.str.axis_label)
                dc->DrawText(mRenderer->
                    object_list->main_data.file->file_header.str.axis_label[j],
                    mRenderer->box_axis_label_loc[j].x+t_tx,
                    mRenderer->box_axis_label_loc[j].y+t_ty-GetCharHeight());

#ifdef SLICE_LABELING_IMPLEMENTED
@
            if (label_slice)
            {	int junk;
                long ljunk;
                static font_height, left_width, head_width, feet_width,
                    anterior_width, posterior_width;

                if (font_height == 0)
                {	VGetWindowInformation(image_window, &junk, &junk,
                        &junk, &junk, &junk, &font_height, &ljunk, &ljunk);
                    VGetTextWidth(image_window, "Left", 4, &left_width);
                    VGetTextWidth(image_window, "Head", 4, &head_width);
                    VGetTextWidth(image_window, "Feet", 4, &feet_width);
                    VGetTextWidth(image_window, "Anterior", 8,
                        &anterior_width);
                    VGetTextWidth(image_window, "Posterior", 9,
                        &posterior_width);
                }
                switch (this_slice->plane_type)
                {	case AXIAL:
                        VDisplayImageMessage(display_area2, "Anterior", 1,
                            win_xloc+(width-anterior_width)/2, win_yloc);
                        VDisplayImageMessage(display_area2, "Right", 1,
                            win_xloc, win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Left", 1,
                            win_xloc+width-left_width,
                            win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Posterior", 1,
                            win_xloc+(width-posterior_width)/2,
                            win_yloc+height-font_height);
                        break;
                    case CORONAL:
                        VDisplayImageMessage(display_area2, "Head", 1,
                            win_xloc+(width-head_width)/2, win_yloc);
                        VDisplayImageMessage(display_area2, "Right", 1,
                            win_xloc, win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Left", 1,
                            win_xloc+width-left_width,
                            win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Feet", 1,
                            win_xloc+(width-feet_width)/2,
                            win_yloc+height-font_height);
                        break;
                    case SAGITTAL:
                        VDisplayImageMessage(display_area2, "Head", 1,
                            win_xloc+(width-head_width)/2, win_yloc);
                        VDisplayImageMessage(display_area2, "Anterior", 1,
                            win_xloc, win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Posterior", 1,
                            win_xloc+width-posterior_width,
                            win_yloc+(height-font_height)/2);
                        VDisplayImageMessage(display_area2, "Feet", 1,
                            win_xloc+(width-feet_width)/2,
                            win_yloc+height-font_height);
                        break;
                }
            }
            else
            {	sl_scale = scale*DisplayWidthMM(display, screen)/
                    DisplayWidth(display, screen)*
                    this_slice->image->width/main_image->width;
                if (sl_scale > 1)
                    sprintf(scale_msg, "scale = 1:%.3f", sl_scale);
                else
                    sprintf(scale_msg, "scale = %.3f:1", 1/sl_scale);
                VDisplayImageMessage(display_area2, scale_msg, 1, win_xloc,
                    win_yloc);
            }
#endif
    } else if (m_backgroundLoaded) {
        int  w, h;
        dc->GetSize( &w, &h );
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setRotate ( bool on ) {
    wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                        //vvvvvvvvvvvvvvvv
    if (mRotateMode==roMode || mRotateMode==toMode ||
            ((mWhichMode==SELECT_SLICE||mWhichMode==REFLECT) &&
            ((mControlState>=1 && mControlState<=6) || mControlState==12)))
    {
        m_tx -= mOverallXSize/2;
        m_ty -= mOverallYSize/2;
        if (mWhichMode==SELECT_SLICE || mWhichMode==REFLECT)
            mControlState = 0;
    }
    mRotateMode = 0;
    mRotateOn = on;
    setActionText();
}
//----------------------------------------------------------------------
void SurfViewCanvas::setActionText()
{
    if (mRotateOn)
    {
        m_parent_frame->SetStatusText("Rotate mode", 0);
        m_parent_frame->SetStatusText( "", 3 );
        switch (mRotateMode) {
            case 0:
                m_parent_frame->SetStatusText( "Scroll", 2 );
                m_parent_frame->SetStatusText( "Rotate Object", 4 );
                return;
            case rxMode :
                SetCursor( wxCursor(wxCURSOR_SIZENS) );
                m_parent_frame->SetStatusText( "X/", 2 );
                break;
            case ryMode :
                SetCursor( wxCursor(wxCURSOR_SIZEWE) );
                m_parent_frame->SetStatusText( "Y/", 2 );
                break;
            case rzMode :
                SetCursor( wxCursor(wxCURSOR_SIZENESW) );
                m_parent_frame->SetStatusText( "Z/", 2 );
                break;
        }
        m_parent_frame->SetStatusText( "Release", 4 );
    }
    else
        switch (mWhichMode)
        {
            case SEPARATE:
            case CUT_CURVED:
            case ROI_STATISTICS:
                switch (mControlState)
                {
                    case 0:
                        m_parent_frame->SetStatusText("", 0);
                        m_parent_frame->SetStatusText("Draw Curve", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("", 4 );
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("Specify point on curve."
                            , 0);
                        m_parent_frame->SetStatusText("Select Point", 2);
                        m_parent_frame->SetStatusText("Select Point", 3);
                        m_parent_frame->SetStatusText("Close", 4 );
                        break;
                    case 2:
                        m_parent_frame->SetStatusText("Select side to retain.",
                            0);
                        m_parent_frame->SetStatusText("Select Side", 2);
                        m_parent_frame->SetStatusText("Select Side", 3);
                        m_parent_frame->SetStatusText("Quit", 4 );
                        break;
                    case 3:
                        m_parent_frame->SetStatusText("Specify Depth.", 0);
                        m_parent_frame->SetStatusText("Entire", 2);
                        m_parent_frame->SetStatusText("Use Slider", 3);
                        m_parent_frame->SetStatusText("", 4 );
                        break;
                }
                break;
            case MOVE:
                switch (mControlState) {
                    case 0:
                        m_parent_frame->SetStatusText("Move Objects", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("", 4 );
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("Via Points", 2);
                        m_parent_frame->SetStatusText("Interactive", 3);
                        m_parent_frame->SetStatusText("Cancel", 4);
                        break;
                    case 2:
                        m_parent_frame->SetStatusText("Select Point", 2);
                        if (mRenderer->points_selected)
                            m_parent_frame->SetStatusText("Undo", 3);
                        else
                            m_parent_frame->SetStatusText("", 3);
                        if (mRenderer->points_selected == 2)
                            m_parent_frame->SetStatusText("Accept", 4);
                        else
                            m_parent_frame->SetStatusText("", 4);
                        break;
                    case 3:
                        m_parent_frame->SetStatusText("X/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Accept", 4);
                        break;
                    case 4:
                        m_parent_frame->SetStatusText("Y/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Accept", 4);
                        break;
                    case 5:
                        m_parent_frame->SetStatusText("Z/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Accept", 4);
                        break;
                    case 6:
                    case 7:
                    case 8:
                        m_parent_frame->SetStatusText("Rotate", 2);
                        m_parent_frame->SetStatusText("", 3);
                        m_parent_frame->SetStatusText("Accept", 4);
                        break;
                    case 9:
                        m_parent_frame->SetStatusText("Rotate", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                    case 10:
                        m_parent_frame->SetStatusText("Rotate", 2);
                        m_parent_frame->SetStatusText("", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 11:
                        m_parent_frame->SetStatusText("", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                }
                break;
            case SELECT_SLICE:
                switch (mControlState) {
                    case 0:
                        m_parent_frame->SetStatusText("Select Plane", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("Select Range", 4 );
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("X/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 2:
                        m_parent_frame->SetStatusText("Y/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 3:
                        m_parent_frame->SetStatusText("Z/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 4:
                    case 5:
                    case 6:
                        m_parent_frame->SetStatusText("Rotate", 2);
                        m_parent_frame->SetStatusText("", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 12:
                        m_parent_frame->SetStatusText("First Slice", 2);
                        m_parent_frame->SetStatusText("Last Slice", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 13:
                        m_parent_frame->SetStatusText("Rotate Plane", 2);
                        m_parent_frame->SetStatusText("Principal Planes", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 14:
                        m_parent_frame->SetStatusText("Axial/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 15:
                        m_parent_frame->SetStatusText("Sagittal/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 16:
                        m_parent_frame->SetStatusText("Coronal/", 2);
                        m_parent_frame->SetStatusText("Translate", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                }
                break;
            case MEASURE:
                if (mRenderer->nmeasurement_points == 0)
                    m_parent_frame->SetStatusText("", 0);
                switch (mControlState) {
                    case 0:
                        m_parent_frame->SetStatusText("Select Point", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("Set Marks", 4);
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("Mark Object", 2);
                        m_parent_frame->SetStatusText("Erase Mark", 3);
                        m_parent_frame->SetStatusText("Measure", 4);
                        break;
                }
                break;
            case CREATE_MOVIE_TUMBLE:
            case PREVIOUS_SEQUENCE:
                switch (mControlState) {
                    case 0:
                        m_parent_frame->SetStatusText("Select Pose", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("", 4);
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("X/", 2);
                        m_parent_frame->SetStatusText("Select Key Pose", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 2:
                        m_parent_frame->SetStatusText("Y/", 2);
                        m_parent_frame->SetStatusText("Select Key Pose", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 3:
                        m_parent_frame->SetStatusText("Z/", 2);
                        m_parent_frame->SetStatusText("Select Key Pose", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 5:
                        m_parent_frame->SetStatusText("New Pose", 2);
                        m_parent_frame->SetStatusText("Close", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 12:
                    case 13:
                        m_parent_frame->SetStatusText("Single Frame", 2);
                        m_parent_frame->SetStatusText("Continuous", 3);
                        m_parent_frame->SetStatusText("", 4);
                        break;
                }
                break;
            case REFLECT:
            case CUT_PLANE:
                switch (mControlState) {
                    case 0:
                        m_parent_frame->SetStatusText("Select Plane", 2);
                        m_parent_frame->SetStatusText("Select Object", 3);
                        m_parent_frame->SetStatusText("", 4);
                        break;
                    case 1:
                        m_parent_frame->SetStatusText("X/", 2);
                        m_parent_frame->SetStatusText("Translate Plane", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 2:
                        m_parent_frame->SetStatusText("Y/", 2);
                        m_parent_frame->SetStatusText("Translate Plane", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 3:
                        m_parent_frame->SetStatusText("Z/", 2);
                        m_parent_frame->SetStatusText("Translate Plane", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 4:
                    case 5:
                    case 6:
                        m_parent_frame->SetStatusText("Rotate Plane", 2);
                        m_parent_frame->SetStatusText("", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                    case 12:
                        m_parent_frame->SetStatusText("Select Side", 2);
                        m_parent_frame->SetStatusText("Select Side", 3);
                        m_parent_frame->SetStatusText("Done", 4);
                        break;
                }
                break;
        }
}
//----------------------------------------------------------------------
void SurfViewCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool SurfViewCanvas::isLoaded ( const int which ) const {
	if (mRenderer == NULL)
		return false;
	Shell *object_list;
	int j;
	for (j=0,object_list=mRenderer->object_list; object_list;
			j++,object_list=object_list->next)
		if (j == which)
			return true;
    return false;
}
//----------------------------------------------------------------------
int SurfViewCanvas::getCenter ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_center;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int SurfViewCanvas::getMax ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_max;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_max;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int SurfViewCanvas::getMin ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_min;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_min;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
//number of slices in entire data set
int SurfViewCanvas::getNoSlices ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_zSize;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_zSize;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
bool SurfViewCanvas::getSurfView ( void ) const {
    return m_overlay;
}
//----------------------------------------------------------------------
double SurfViewCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int SurfViewCanvas::getSliceNo ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_sliceNo;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int SurfViewCanvas::getWidth ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_width;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
bool SurfViewCanvas::getInvert ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.mInvert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        return cd.mInvert;
    } else {
        assert( 0 );
    }
    return false;  //should never get here
}
//----------------------------------------------------------------------
double SurfViewCanvas::getB ( const int which ) {
    if (which==0) {
        if (mCavassData==NULL)
            mCavassData = new CavassData();
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getB();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        return cd.getB();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
double SurfViewCanvas::getG ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getG();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        return cd.getG();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
double SurfViewCanvas::getR ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getR();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        return cd.getR();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void SurfViewCanvas::setB ( const int which, const double b ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setB( b );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setB( b );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setCenter ( const int which, const int center ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_center = center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_center = center;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setG ( const int which, const double g ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setG( g );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setG( g );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setInvert ( const int which, const bool invert ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.mInvert = invert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.mInvert = invert;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setSurfView ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void SurfViewCanvas::setR ( const int which, const double r ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setR( r );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setR( r );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::setWidth ( const int which, const int width ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_width = width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_width = width;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void SurfViewCanvas::save_movie( const char path[] )
{
    if (key_poses == 0)
    {
        wxMessageBox("Sequence not defined.");
        return;
    }

    int frames_to_go = key_poses;
    for (int j=0; j<key_poses-1; j++)
        frames_to_go += key_pose[j+1].views;
    if (key_poses>2 && key_pose[key_poses-1].angle[0]==key_pose[0].angle[0] &&
            key_pose[key_poses-1].angle[1]==key_pose[0].angle[1] &&
            key_pose[key_poses-1].angle[2]==key_pose[0].angle[2])
        frames_to_go--;

  if (strlen(path)>4 && strcmp(path+strlen(path)-4, ".MV0")==0)
  {
    int error_code, obytes;
    FILE *movie_file;
    ViewnixHeader *header;
    float smallest_value[3], largest_value[3];
    short signed_bits[3], bit_fields[6];
    char *data, bad_group[5], bad_element[5];
    const int frame_bytes=mRenderer->main_image->width*
        mRenderer->main_image->height*3;
    Shell_data *obj_data=&mRenderer->object_list->main_data;

    /* Create new file, write header, copy frames from temp file. */
    header = (ViewnixHeader *)calloc(1, sizeof(ViewnixHeader));
    if (header == NULL)
    {
        wxMessageBox("Out of memory.");
        return;
    }
    strcpy(header->gen.filename1,
        obj_data->file->file_header.gen.filename);
    strncpy(header->gen.filename, path, sizeof(header->gen.filename)-1);
    header->gen.filename1_valid = header->gen.filename_valid = 1;
    movie_file = fopen(path, "wb+");
    if (movie_file == NULL)
    {
        wxMessageBox(wxString("Unable to create file ")+path);
        free(header);
        return;
    }
    /*@ Fill in type 1 now, deal with the rest later. */
    strcpy(header->gen.recognition_code, "VIEWNIX1.0");
    header->gen.data_type = MOVIE0;
    header->gen.data_type_valid = header->gen.recognition_code_valid = 1;
    if (obj_data->file->file_header.gen.study_date_valid)
    {	strcpy(header->gen.study_date,
            obj_data->file->file_header.gen.study_date);
        header->gen.study_date_valid = 1;
    }
    if (obj_data->file->file_header.gen.study_time_valid)
    {	strcpy(header->gen.study_time,
            obj_data->file->file_header.gen.study_time);
        header->gen.study_time_valid = 1;
    }
    if (obj_data->file->file_header.gen.modality_valid)
    {	strcpy(header->gen.modality,
            obj_data->file->file_header.gen.modality);
        header->gen.modality_valid = 1;
    }
    if (obj_data->file->file_header.gen.institution_valid)
    {	strcpy(header->gen.institution,
            obj_data->file->file_header.gen.institution);
        header->gen.institution_valid = 1;
    }
    if (obj_data->file->file_header.gen.physician_valid)
    {	strcpy(header->gen.physician,
            obj_data->file->file_header.gen.physician);
        header->gen.physician_valid = 1;
    }
    if (obj_data->file->file_header.gen.department_valid)
    {	strcpy(header->gen.department,
            obj_data->file->file_header.gen.department);
        header->gen.department_valid = 1;
    }
    if (obj_data->file->file_header.gen.radiologist_valid)
    {	strcpy(header->gen.radiologist,
            obj_data->file->file_header.gen.radiologist);
        header->gen.radiologist_valid = 1;
    }
    if (obj_data->file->file_header.gen.model_valid)
    {	strcpy(header->gen.model,
            obj_data->file->file_header.gen.model);
        header->gen.model_valid = 1;
    }
    if (obj_data->file->file_header.gen.patient_name_valid)
    {	strcpy(header->gen.patient_name,
            obj_data->file->file_header.gen.patient_name);
        header->gen.patient_name_valid = 1;
    }
    if (obj_data->file->file_header.gen.patient_id_valid)
    {	strcpy(header->gen.patient_id,
            obj_data->file->file_header.gen.patient_id);
        header->gen.patient_id_valid = 1;
    }
    if (obj_data->file->file_header.gen.study_valid)
    {	strcpy(header->gen.study,
            obj_data->file->file_header.gen.study);
        header->gen.study_valid = 1;
    }
    if (obj_data->file->file_header.gen.series_valid)
    {	strcpy(header->gen.series,
            obj_data->file->file_header.gen.series);
        header->gen.series_valid = 1;
    }
    header->dsp.num_of_elems = 3;
    header->dsp.measurement_unit[0] = header->dsp.measurement_unit[1] =
        obj_data->file->file_header.str.measurement_unit[0];
    header->dsp.measurement_unit_valid = 1;
    header->dsp.dimension = 5;
    smallest_value[0] = smallest_value[1] = smallest_value[2] = 0;
    header->dsp.smallest_value = smallest_value;
    largest_value[0] = largest_value[1] = largest_value[2] = 255;
    header->dsp.largest_value = largest_value;
    header->dsp.num_of_integers = header->dsp.num_of_elems;
    signed_bits[0] = signed_bits[1] = signed_bits[2] = 0;
    header->dsp.signed_bits = signed_bits;
    bit_fields[0] = 0;
    bit_fields[1] = 7;
    bit_fields[2] = 8;
    bit_fields[3] = 15;
    bit_fields[4] = 16;
    bit_fields[5] = 23;
    header->dsp.num_of_bits = 24;
    header->dsp.bit_fields = bit_fields;
    header->dsp.num_of_images = frames_to_go;
    header->dsp.xysize[0] = mRenderer->main_image->width;
    header->dsp.xysize[1] = mRenderer->main_image->height;
    header->dsp.xypixsz[0] = header->dsp.xypixsz[1] =
        1/(mRenderer->scale*unit_mm(obj_data));
    header->dsp.dimension_valid = header->dsp.num_of_elems_valid =
        header->dsp.smallest_value_valid = header->dsp.largest_value_valid =
        header->dsp.num_of_integers_valid = header->dsp.signed_bits_valid =
        header->dsp.num_of_bits_valid = header->dsp.bit_fields_valid =
        header->dsp.num_of_images_valid = header->dsp.xysize_valid =
        header->dsp.xypixsz_valid = 1;
    error_code = VWriteHeader(movie_file, header, bad_group, bad_element);
    switch (error_code)
    {	case 0:
        case 107:
        case 106:
            break;
        default:
            wxMessageBox(wxString::Format(
                "Group %s element %s undefined in VWriteHeader",
                bad_group, bad_element));
            free(header);
            fclose(movie_file);
            return;
    }
    error_code = VSeekData(movie_file, 0);
    if (error_code)
    {
        wxMessageBox("File seek failed.");
        free(header);
        fclose(movie_file);
        return;
    }
    for (preview_key_pose= -1; frames_to_go; frames_to_go--)
    {
        m_parent_frame->SetStatusText(wxString::Format("%d frames to go.",
            frames_to_go), 0);
        if (preview_key_pose<0 ||
            preview_view>=key_pose[preview_key_pose+1].views)
        {
            preview_key_pose++;
            preview_view = 0;
            memcpy(mRenderer->glob_angle, &key_pose[preview_key_pose].angle,
                sizeof(mRenderer->glob_angle));
        }
        else
        {
            preview_view++;
            ::view_interpolate(mRenderer->glob_angle,
                key_pose[preview_key_pose].angle,
                key_pose[preview_key_pose+1].angle, preview_view/
                (key_pose[preview_key_pose+1].views+1.0));
        }
        data = mRenderer->render().data;
        error_code = VWriteData(data, 1, frame_bytes, movie_file, &obytes);
        if (error_code==0 && obytes!=frame_bytes)
            error_code = 3;
        if (error_code)
        {
            wxMessageBox("File write failed.");
            free(header);
            fclose(movie_file);
        }
    }
    VCloseData(movie_file);
    free(header);
  }
  else
  {
    TIFF *tif = TIFFOpen( path, "wb" );
    if (tif == NULL)
    {
        wxMessageBox( "Error saving image data file.", "Sorry...",
                      wxOK | wxICON_ERROR );
        return;
    }

    for (preview_key_pose= -1; frames_to_go; frames_to_go--)
    {
        m_parent_frame->SetStatusText(wxString::Format("%d frames to go.",
            frames_to_go), 0);
        if (preview_key_pose<0 ||
            preview_view>=key_pose[preview_key_pose+1].views)
        {
            preview_key_pose++;
            preview_view = 0;
            memcpy(mRenderer->glob_angle, &key_pose[preview_key_pose].angle,
                sizeof(mRenderer->glob_angle));
        }
        else
        {
            preview_view++;
            ::view_interpolate(mRenderer->glob_angle,
                key_pose[preview_key_pose].angle,
                key_pose[preview_key_pose+1].angle, preview_view/
                (key_pose[preview_key_pose+1].views+1.0));
        }
        char *data = mRenderer->render().data;

        TIFFSetField( tif, TIFFTAG_IMAGEWIDTH,      mOverallXSize       );
        TIFFSetField( tif, TIFFTAG_IMAGELENGTH,     mOverallYSize       );
        TIFFSetField( tif, TIFFTAG_BITSPERSAMPLE,   8                   );
        TIFFSetField( tif, TIFFTAG_SAMPLESPERPIXEL, 3                   );
        TIFFSetField( tif, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT );
        TIFFSetField( tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG );
        TIFFSetField( tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB     );
#ifdef ALLOW_LZW
        TIFFSetField( tif, TIFFTAG_COMPRESSION,     COMPRESSION_LZW     );
#endif

        for (int row=0; row<mOverallYSize; row++) {
            TIFFWriteScanline( tif, &data[row*mOverallXSize*3], row );
        }

        TIFFWriteDirectory( tif );
    }
    TIFFClose( tif );
  }
    preview_key_pose = -1;
    preview_view = -1;
    m_parent_frame->SetStatusText("Done.", 0);
    if (mWhichMode == PREVIOUS_SEQUENCE)
        return;

    FILE *sequence_file;
    int pose;

    sequence_file = fopen("manip_sequence", "wb");
    if (sequence_file == NULL)
    {
        wxMessageBox("Could not create manip_sequence file.");
        return;
    }
    if (fprintf(sequence_file, "%d\n", key_poses) < 2)
    {
        wxMessageBox("File write failed.");
        fclose(sequence_file);
        return;
    }
    for (pose=0; pose<key_poses; pose++)
        if (fprintf(sequence_file, "{%f, %f, %f} %d\n",
                key_pose[pose].angle[0], key_pose[pose].angle[1],
                key_pose[pose].angle[2], key_pose[pose].views) < 18)
        {
            wxMessageBox("File write failed.");
            break;
        }
    fclose(sequence_file);
}
//----------------------------------------------------------------------
void SurfViewCanvas::handleStereo ( char* rendered ) {
    assert( rendered != NULL );
    if (Preferences::getStereoMode() == Preferences::StereoModeOff)    return;
    mRenderer->setStereoTransform();

    int  auxOverallXSize, auxOverallYSize;
    char*  dataAux = mRenderer->mAux->render2( auxOverallXSize, auxOverallYSize, mInterruptRenderingFlag );
    assert( dataAux != NULL );
    assert( auxOverallXSize == mOverallXSize && auxOverallYSize == mOverallYSize );
    /** \todo creat the stereo pair */
    if (Preferences::getStereoMode() == Preferences::StereoModeAnaglyph) {
        //data is the left image; dataAux is the right
        double  lr = Preferences::getStereoLeftRed()    / 255.0;
        double  lg = Preferences::getStereoLeftGreen()  / 255.0;
        double  lb = Preferences::getStereoLeftBlue()   / 255.0;

        double  rr = Preferences::getStereoRightRed()   / 255.0;
        double  rg = Preferences::getStereoRightGreen() / 255.0;
        double  rb = Preferences::getStereoRightBlue()  / 255.0;

        if (lr < 0.0)    lr = 0.0;
        if (lr > 1.0)    lr = 1.0;
        if (lg < 0.0)    lg = 0.0;
        if (lg > 1.0)    lg = 1.0;
        if (lb < 0.0)    lb = 0.0;
        if (lb > 1.0)    lb = 1.0;

        if (rr < 0.0)    rr = 0.0;
        if (rr > 1.0)    rr = 1.0;
        if (rg < 0.0)    rg = 0.0;
        if (rg > 1.0)    rg = 1.0;
        if (rb < 0.0)    rb = 0.0;
        if (rb > 1.0)    rb = 1.0;

		unsigned char*  uRendered = (unsigned char*) rendered;
		unsigned char*  uDataAux  = (unsigned char*) dataAux;
        for (int i=0; i<mOverallXSize*mOverallYSize*3; /*unused*/) {
            int  v;

            v = (int)(uRendered[i] * lr + uDataAux[i] * rr + 0.5);  // + 0.5 to round
            if (v<0)    v = 0;
            if (v>255)  v = 255;
            uRendered[i] = v;
            ++i;

            v = (int)(uRendered[i] * lg + uDataAux[i] * rg + 0.5);  // + 0.5 to round
            if (v<0)    v = 0;
            if (v>255)  v = 255;
            uRendered[i] = v;
            ++i;

            v = (int)(uRendered[i] * lb + uDataAux[i] * rb + 0.5);  // + 0.5 to round
            if (v<0)    v = 0;
            if (v>255)  v = 255;
            uRendered[i] = v;
            ++i;
        }
    } else if (Preferences::getStereoMode() == Preferences::StereoModeInterlaced) {
        if (Preferences::getStereoLeftOdd()) {
            //left comes from the odd numbered rows; right comes from the even.
            // so move right (dataAux) to even.
            for (int y=0; y<mOverallYSize; y+=2) {
                int  offset = y * mOverallXSize * 3;
                for (int x=0; x<mOverallXSize*3; x++) {
                    rendered[offset] = dataAux[offset];
                    ++offset;
                }
            }
        } else {
            //left comes from the even numbered rows; right comes from the odd.
            // so move right (dataAux) to odd.
            for (int y=1; y<mOverallYSize; y+=2) {
                int  offset = y * mOverallXSize * 3;
                for (int x=0; x<mOverallXSize*3; x++) {
                    rendered[offset] = dataAux[offset];
                    ++offset;
                }
            }
        }
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------

void run_command(char cmnd[], bool fg)
{
    //begin critical section -------------------------------------------
    s_mutexProtectingTheGlobalData.Lock();
    if ( !s_mutexProtectingTheGlobalData.IsOk() )
		wxMessageBox("Mutex failed.");

	ProcessManager  q( "Running command...", cmnd, fg );
	if (q.getCancel())
		fprintf(stderr, "Command \"%s\" canceled.\n", cmnd);
	free(cmnd);

    s_mutexProtectingTheGlobalData.Unlock();
    //end critical section ---------------------------------------------
}

void display_message(const char msg[])
{
//@	wxMessageBox(msg);
    fprintf(stderr, "%s\n", msg);
}
//----------------------------------------------------------------------
DEFINE_EVENT_TYPE( wxEVT_MY_CUSTOM_COMMAND )

//define an event table macro for this event type
#define  EVT_MY_CUSTOM_COMMAND( id, fn )                       \
    DECLARE_EVENT_TABLE_ENTRY(                                 \
        wxEVT_MY_CUSTOM_COMMAND, id, wxID_ANY,                 \
        (wxObjectEventFunction)(wxEventFunction)               \
            wxStaticCastEvent( wxCommandEventFunction, &fn ),  \
        (wxObject *) NULL ),

IMPLEMENT_DYNAMIC_CLASS(  SurfViewCanvas, wxPanel       )
BEGIN_EVENT_TABLE(        SurfViewCanvas, wxPanel       )
    EVT_PAINT(            SurfViewCanvas::OnPaint       )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           SurfViewCanvas::OnMouseMove   )
    EVT_SIZE(             MainCanvas::OnSize        )
    EVT_LEFT_DOWN(        SurfViewCanvas::OnLeftDown    )
    EVT_LEFT_UP(          SurfViewCanvas::OnLeftUp      )
    EVT_MIDDLE_DOWN(      SurfViewCanvas::OnMiddleDown  )
    EVT_MIDDLE_UP(        SurfViewCanvas::OnMiddleUp    )
    EVT_RIGHT_DOWN(       SurfViewCanvas::OnRightDown   )
    EVT_RIGHT_UP(         SurfViewCanvas::OnRightUp     )
    EVT_CHAR(             SurfViewCanvas::OnChar        )
    EVT_MY_CUSTOM_COMMAND(wxID_ANY, SurfViewCanvas::OnDoRender )
    EVT_TIMER( ID_CINE_TIMER, SurfViewCanvas::OnCineTimer )
END_EVENT_TABLE()
//======================================================================

