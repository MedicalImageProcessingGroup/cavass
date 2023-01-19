/*
  Copyright 1993-2009, 2016 Medical Image Processing Group
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
// SnakesDialog
//----------------------------------------------------------------------
#include  "cavass.h"

#if 0
#ifdef WIN32
    #include  <iostream.h>
    #include  <iomanip.h>
#else
    #include  <fstream>
    #include  <iostream>
    #include  <iomanip>
#endif
#include  <assert.h>
#include  <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif
#include  <stdlib.h>
#include  <time.h>
#include  <Viewnix.h>
#include  "cv3dv.h"
#include  "Dicom.h"

#include  "wx/wx.h"
#include  "wx/file.h"
#include  "wx/gdicmn.h"
#include  "wx/progdlg.h"
#include  "MontageFrame.h"
#include  "MainCanvas.h"
#include  "Globals.h"
#endif

#include  "SnakesDialog.h"
using namespace std;
//----------------------------------------------------------------------
static const int  sSliderDivisor      =    1000;
static const int  sWidth              =    100;
static const int  sHeight             =    100;
static const int  sMaxWeightValue     =  10000;  //0 .. 100.00
//static const int  sWeightDefault      =     10;    //0.01
static const int  sTensileDefault     =     10;    //0.010
static const int  sFlexuralDefault    =     15;    //0.015
static const int  sExternalDefault    =     20;    //0.020
static const int  sInflationDefault   =      0;    //0.0
static const int  sMaxDiscretization  =    100;
static const int  sDefaultDiscretization = sMaxDiscretization / 10;
static const int  sDefaultThreshold   =    500;
static const int  sMaxThreshold       =  32000;  //stupid windoze
static const int  sDefaultIterations  =    100;
static const int  sMaxIterations      =  32000;  //stupid windoze
static const int  sDefaultRadius      =      2;
static const int  sMaxRadius          =    100;
//----------------------------------------------------------------------
void SnakesDialog::init ( void ) {
    srand( time(NULL) );
    //srand(0);
    mMode = 0;
    for (int i=0; i<MAXPOINTS; i++) {
        mData[i][0] = mData[i][1] = mData[i][2] = -1;
    }
    mPoints.clear();

    mExtendToEdges          = false;
    mCompleteContour        = true;
    m_update                = NULL;
    m_clear                 = NULL;
    m_previous              = NULL;
    m_next                  = NULL;
    m_close                 = NULL;
    m_process_all           = NULL;
    m_processed_slice       = NULL;
    m_processed_data        = NULL;
    if ( m_parent!=NULL && m_parent->mCanvas!=NULL &&
         m_parent->mCanvas->mCavassData->m_zSize>0 )
    {
        m_processed_points = new vector< p3d<double>* >[m_parent->mCanvas->mCavassData->m_zSize];
        m_processed_slice = new bool[m_parent->mCanvas->mCavassData->m_zSize];
        for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
            m_processed_slice[z] = false;
        }
        const long  size = m_parent->mCanvas->mCavassData->m_zSize *
                           m_parent->mCanvas->mCavassData->m_ySize *
                           m_parent->mCanvas->mCavassData->m_xSize;
        m_processed_data = new int[ size ];
        for (long i=0; i<size; i++)    m_processed_data[i] = 0;
    }

    m_userpoints            = MAXPOINTS/10;
    m_userpoints_st         = NULL;
    m_userpoints_text       = NULL;
    m_userpoints_slider     = NULL;

    m_discretization        = sDefaultDiscretization;
    m_discretization_st     = NULL;
    m_discretization_text   = NULL;
    m_discretization_slider = NULL;

    m_tensile               = (double)sTensileDefault / sSliderDivisor;
    m_tensile_st            = NULL;
    m_tensile_text          = NULL;
    m_tensile_slider        = NULL;

    m_flexural              = (double)sFlexuralDefault / sSliderDivisor;
    m_flexural_st           = NULL;
    m_flexural_text         = NULL;
    m_flexural_slider       = NULL;

    m_external              = (double)sExternalDefault / sSliderDivisor;
    m_external_st           = NULL;
    m_external_text         = NULL;
    m_external_slider       = NULL;

    m_inflation             = (double)sInflationDefault / sSliderDivisor;
    m_inflation_st          = NULL;
    m_inflation_text        = NULL;
    m_inflation_slider      = NULL;

    m_threshold             = sDefaultThreshold;
    m_threshold_st          = NULL;
    m_threshold_text        = NULL;
    m_threshold_slider      = NULL;

    m_iterations            = sDefaultIterations;
    m_iterations_st         = NULL;
    m_iterations_text       = NULL;
    m_iterations_slider     = NULL;

    m_radius                = sDefaultRadius;
    m_radius_st             = NULL;
    m_radius_text           = NULL;
    m_radius_slider         = NULL;

    m_show_normals          = true;
    m_show_normals_checkbox = NULL;

    if ( m_parent!=NULL && m_parent->mCanvas!=NULL &&
         m_parent->mCanvas->mCavassData->m_fname!=NULL )
    {
        char  tmp[255];
        sprintf( tmp, "%s.snakesdialog.log", m_parent->mCanvas->mCavassData->m_fname );
        mLogfile.open( tmp, fstream::out | fstream::trunc );
        timestamp();
        mLogfile << " input file=" << m_parent->mCanvas->mCavassData->m_fname << endl
                 << flush;
        cout << "log file is: " << tmp << endl;
    } else {
        mLogfile.open( "snakesdialog.log", fstream::out | fstream::trunc );
        timestamp();    mLogfile << endl << flush;
    }
}
//----------------------------------------------------------------------
void SnakesDialog::reset ( void ) {
    cout << "SnakesDialog::reset" << endl;
    //clear points indicated by user (and reset state to indicate that the
    // user must once again indicate points)
    mMode = 0;
    for (int i=0; i<MAXPOINTS; i++) {
        mData[i][0] = mData[i][1] = mData[i][2] = -1;
    }
    mPoints.clear();
    if (m_update!=NULL)         m_update->Enable( false );

    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );
    const int  z = canvas->getSliceNo();
    timestamp();    mLogfile << " reset " << z << endl << flush;
    if (m_processed_slice!=NULL) {
        m_processed_slice[z] = false;
        m_processed_points[z].clear();
        //for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
        //    m_processed_slice[z] = false;
        //}
    }
    if (m_process_all!=NULL) {
        //only disable this button if there is not even one processed slice
        bool  foundOne = false;
        for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
            if (m_processed_slice[z]) {
                foundOne = true;
                break;
            }
        }
        if (!foundOne)    m_process_all->Enable( false );
    }
    if (m_processed_data!=NULL) {
        //const long  size = m_parent->mCanvas->mCavassData->m_zSize *
        //                   m_parent->mCanvas->mCavassData->m_ySize *
        //                   m_parent->mCanvas->mCavassData->m_xSize;
        //for (long i=0; i<size; i++)    m_processed_data[i] = 0;
        for (int y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
            for (int x=0; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 0;
            }
        }
    }
}
//----------------------------------------------------------------------
SnakesDialog::SnakesDialog ( MontageFrame* parent )
  : wxDialog ( parent, -1, "Snakes", wxDefaultPosition, wxDefaultSize )
{
//    const int  sliderWidth = 200;
    m_parent = parent;
    init();
    m_parent->mCanvas->SetCursor( wxCursor(wxCURSOR_CROSS) );
    wxPanel*  mainPanel = new wxPanel( this, -1 );
    ::setColor( this );
    ::setColor( mainPanel );

    wxFlexGridSizer*  mainSizer = new wxFlexGridSizer(1, 5, 5);
    // - - - - - - - - - -
    mainSizer->Add(20, 20);
    // - - - - - - - - - -
    wxSizer*  gs1 = new wxFlexGridSizer(3, 10, 5);  //3 cols,vgap,hgap
    mainSizer->Add( gs1, 1, wxALIGN_CENTER );

    m_userpoints_st = new wxStaticText( mainPanel, -1, "user points:" );
    gs1->Add( m_userpoints_st, 1, wxALIGN_RIGHT );
    m_userpoints_text = new wxTextCtrl( mainPanel, ID_USERPOINTS_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_userpoints_text );
    gs1->Add( m_userpoints_text, 1, wxALIGN_RIGHT );
    m_userpoints_slider = new wxSlider( mainPanel, ID_USERPOINTS,
        m_userpoints, 1, MAXPOINTS,
        wxDefaultPosition, wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "userpoints" );
    m_userpoints_slider->SetPageSize(5);
    ::setColor( m_userpoints_slider );
    gs1->Add( m_userpoints_slider, 0, wxALIGN_CENTER );

    m_discretization_st = new wxStaticText( mainPanel, -1, "discretization:" );
    gs1->Add( m_discretization_st, 1, wxALIGN_RIGHT );
    m_discretization_text = new wxTextCtrl( mainPanel, ID_DISCRETIZATION_TEXT,
        "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_discretization_text );
    gs1->Add( m_discretization_text, 1, wxALIGN_RIGHT );
    m_discretization_slider = new wxSlider( mainPanel, ID_DISCRETIZATION,
        sDefaultDiscretization, 1, sMaxDiscretization, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "discretization" );
    m_discretization_slider->SetPageSize(5);
    ::setColor( m_discretization_slider );
    gs1->Add( m_discretization_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    //blank row
    gs1->Add( 20, 20 );    gs1->Add( 20, 20 );    gs1->Add( 20, 20 );
    // - - - - - - - - - -
    m_tensile_st = new wxStaticText( mainPanel, -1, "tensile:" );
    gs1->Add( m_tensile_st, 1, wxALIGN_RIGHT );
    m_tensile_text = new wxTextCtrl( mainPanel, ID_TENSILE_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_tensile_text );
    gs1->Add( m_tensile_text, 1, wxALIGN_RIGHT );
    m_tensile_slider = new wxSlider( mainPanel, ID_TENSILE,
        sTensileDefault, -sMaxWeightValue, sMaxWeightValue, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
        "tensile" );
    m_tensile_slider->SetPageSize(5);
    ::setColor( m_tensile_slider );
    gs1->Add( m_tensile_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    m_flexural_st = new wxStaticText( mainPanel, -1, "flexural:" );
    gs1->Add( m_flexural_st, 1, wxALIGN_RIGHT );
    m_flexural_text = new wxTextCtrl( mainPanel, ID_FLEXURAL_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_flexural_text );
    gs1->Add( m_flexural_text, 1, wxALIGN_RIGHT );
    m_flexural_slider = new wxSlider( mainPanel, ID_FLEXURAL, sFlexuralDefault,
        -sMaxWeightValue, sMaxWeightValue, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "flexural" );
    m_flexural_slider->SetPageSize(5);
    ::setColor( m_flexural_slider );
    gs1->Add( m_flexural_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    m_external_st = new wxStaticText( mainPanel, -1, "external (image):" );
    gs1->Add( m_external_st, 1, wxALIGN_RIGHT );
    m_external_text = new wxTextCtrl( mainPanel, ID_EXTERNAL_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_external_text );
    gs1->Add( m_external_text, 1, wxALIGN_RIGHT );
    m_external_slider = new wxSlider( mainPanel, ID_EXTERNAL, sExternalDefault,
        -sMaxWeightValue, sMaxWeightValue, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "external" );
    m_external_slider->SetPageSize(5);
    ::setColor( m_external_slider );
    gs1->Add( m_external_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
#if 0
    m_inflation_st = new wxStaticText( mainPanel, -1,
        " inflation (image, contour):" );
    gs1->Add( m_inflation_st, 1, wxALIGN_RIGHT );
    m_inflation_text = new wxTextCtrl( mainPanel, ID_INFLATION_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_inflation_text );
    gs1->Add( m_inflation_text, 1, wxALIGN_RIGHT );
    m_inflation_slider = new wxSlider( mainPanel, ID_INFLATION,
        sInflationDefault, -sMaxWeightValue, sMaxWeightValue,
        wxDefaultPosition, wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "inflation" );
    m_inflation_slider->SetPageSize(5);
    ::setColor( m_inflation_slider );
    gs1->Add( m_inflation_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    m_threshold_st = new wxStaticText( mainPanel, -1, " inflation threshold:" );
    gs1->Add( m_threshold_st, 1, wxALIGN_RIGHT );
    m_threshold_text = new wxTextCtrl( mainPanel, ID_THRESHOLD_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_threshold_text );
    gs1->Add( m_threshold_text, 1, wxALIGN_RIGHT );
    m_threshold_slider = new wxSlider( mainPanel, ID_THRESHOLD,
        sDefaultThreshold, 0, sMaxThreshold, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "threshold" );
    m_threshold_slider->SetPageSize(5);
    ::setColor( m_threshold_slider );
    gs1->Add( m_threshold_slider, 0, wxALIGN_CENTER );
#endif
    // - - - - - - - - - -
    m_radius_st = new wxStaticText( mainPanel, -1, "radius:" );
    gs1->Add( m_radius_st, 1, wxALIGN_RIGHT );
    m_radius_text = new wxTextCtrl( mainPanel, ID_RADIUS_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_radius_text );
    gs1->Add( m_radius_text, 1, wxALIGN_RIGHT );
    m_radius_slider = new wxSlider( mainPanel, ID_RADIUS,
        sDefaultRadius, 1, sMaxRadius, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "radius" );
    m_radius_slider->SetPageSize(5);
    ::setColor( m_radius_slider );
    gs1->Add( m_radius_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    //blank row
    gs1->Add( 20, 20 );    gs1->Add( 20, 20 );    gs1->Add( 20, 20 );
    // - - - - - - - - - -
    m_iterations_st = new wxStaticText( mainPanel, -1, "iterations:" );
    gs1->Add( m_iterations_st, 1, wxALIGN_RIGHT );
    m_iterations_text = new wxTextCtrl( mainPanel, ID_ITERATIONS_TEXT, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_iterations_text );
    gs1->Add( m_iterations_text, 1, wxALIGN_RIGHT );
    m_iterations_slider = new wxSlider( mainPanel, ID_ITERATIONS,
        sDefaultIterations, 1, sMaxIterations, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "iterations" );
    m_iterations_slider->SetPageSize(5);
    ::setColor( m_iterations_slider );
    gs1->Add( m_iterations_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    m_show_normals_checkbox = new wxCheckBox( mainPanel, ID_SHOW_NORMALS,
        "show normals" );
    m_show_normals_checkbox->SetValue( m_show_normals );
    gs1->Add( m_show_normals_checkbox, 1, wxALIGN_RIGHT );
    ::setColor( m_show_normals_checkbox );
    // - - - - - - - - - -
    //mainSizer->Add(20, 20);
    //mainSizer->Add(20, 20);
    // - - - - - - - - - -
    wxSizer*  gs3 = new wxGridSizer(4, 1, 1);  //cols,vgap,hgap
    gs3->SetMinSize( (int)(2.8*sliderWidth), 50 );
    mainSizer->Add( gs3, 1, wxALIGN_CENTER );

    m_update = new wxButton( mainPanel, ID_UPDATE, "Update" );
    m_update->Enable( false );
    gs3->Add( m_update, 0, wxALIGN_CENTER );
    ::setColor( m_update );

    m_process_all = new wxButton( mainPanel, ID_PROCESS_ALL, "Process All" );
    m_process_all->Enable( false );
    gs3->Add( m_process_all, 0, wxALIGN_CENTER );
    ::setColor( m_process_all );

    m_clear = new wxButton( mainPanel, ID_CLEAR, "Clear" );
    gs3->Add( m_clear, 0, wxALIGN_CENTER );
    ::setColor( m_clear );
#if 1
    m_previous = new wxButton( mainPanel, ID_PREVIOUS, "Previous" );
    gs3->Add( m_previous, 0, wxALIGN_CENTER );
    ::setColor( m_previous );

    m_next = new wxButton( mainPanel, ID_NEXT, "Next" );
    gs3->Add( m_next, 0, wxALIGN_CENTER );
    m_next->Enable( false );
    ::setColor( m_next );

    wxButton*  load = new wxButton( mainPanel, ID_LOAD, "Load" );
    gs3->Add( load, 0, wxALIGN_CENTER );
    ::setColor( load );

    m_save = new wxButton( mainPanel, ID_SAVE, "Save" );
    m_save->Enable( false );
    gs3->Add( m_save, 0, wxALIGN_CENTER );
    ::setColor( m_save );

    m_save_all = new wxButton( mainPanel, ID_SAVE_ALL, "Save All" );
    gs3->Add( m_save_all, 0, wxALIGN_CENTER );
    ::setColor( m_save_all );
#endif
    m_close = new wxButton( mainPanel, wxID_OK, "Close" );
    gs3->Add( m_close, 0, wxALIGN_CENTER );
    ::setColor( m_close );
#if 1
    m_debug = new wxButton( mainPanel, ID_DEBUG, "Debug" );
    gs3->Add( m_debug, 0, wxALIGN_CENTER );
    ::setColor( m_debug );
#endif
    // - - - - - - - - - -
    //mainSizer->Add(20, 20);
    //mainSizer->Add(20, 20);
    // - - - - - - - - - -
    mainPanel->SetAutoLayout( true );
    mainPanel->SetSizer( mainSizer );
    mainSizer->SetSizeHints( this );
    setupSliders();
    #ifdef wxUSE_TOOLTIPS
        #ifndef __WXX11__
            m_tensile_st->SetToolTip    ( "internal, contour continuity" );
            m_tensile_text->SetToolTip  ( "internal, contour continuity" );
            m_tensile_slider->SetToolTip( "internal, contour continuity" );

            m_flexural_st->SetToolTip    ( "internal, contour curvature" );
            m_flexural_text->SetToolTip  ( "internal, contour curvature" );
            m_flexural_slider->SetToolTip( "internal, contour curvature" );

            m_radius_st->SetToolTip    ( "neighborhood radius" );
            m_radius_text->SetToolTip  ( "neighborhood radius" );
            m_radius_slider->SetToolTip( "neighborhood radius" );
        #endif
    #endif
    //the dialog box keeps falling off of the bottom of my laptop's screen (win xp)
    wxPoint p = GetPosition();    p.x+=80;    p.y-=80;    Move( p );

    Show();
#if 0
    //determine the mag gradient (for the entire data set)
    wxLogMessage( "getting max gradient" );
    cout << "getting max gradient" << endl;
    double  max = mag( gradient2D(0, 0, 0) );
    int  maxX=0, maxY=0, maxZ=0;
    for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
        for (int y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
            for (int x=0; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                const double  magGradient = mag( gradient2D(x, y, z) );
                if (magGradient>max) {
                    max=magGradient;
                    maxX=x;  maxY=y;  maxZ=z;
                }
            }
        }
    }
    wxLogMessage( "done.  max gradient=%f at (%d,%d,%d)", max, maxX, maxY, maxZ );
#endif

    OnCornerSliceChange();
}
//----------------------------------------------------------------------
void SnakesDialog::setupSliders ( void ) {
    char  buff[255];

    m_userpoints = m_userpoints_slider->GetValue();
    sprintf( buff, "%d", m_userpoints );
    m_userpoints_text->SetValue( buff );

    m_discretization = m_discretization_slider->GetValue();
    sprintf( buff, "%d", m_discretization );
    m_discretization_text->SetValue( buff );

    m_tensile = ((double)m_tensile_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_tensile );
    m_tensile_text->SetValue( buff );

    m_flexural = ((double)m_flexural_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_flexural );
    m_flexural_text->SetValue( buff );

    m_external = ((double)m_external_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_external );
    m_external_text->SetValue( buff );

    if (m_inflation_slider!=NULL) {
        m_inflation = ((double)m_inflation_slider->GetValue()) / sSliderDivisor;
        sprintf( buff, "%f", m_inflation );
        m_inflation_text->SetValue( buff );
    }
    if (m_threshold_slider!=NULL) {
        m_threshold = m_threshold_slider->GetValue();
        sprintf( buff, "%d", m_threshold );
        m_threshold_text->SetValue( buff );
    }

    m_iterations = m_iterations_slider->GetValue();
    sprintf( buff, "%d", m_iterations );
    m_iterations_text->SetValue( buff );

    m_radius = m_radius_slider->GetValue();
    sprintf( buff, "%d", m_radius );
    m_radius_text->SetValue( buff );
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void SnakesDialog::OnUserpointsSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_userpoints = m_userpoints_slider->GetValue();
    sprintf( buff, "%d", m_userpoints );
    m_userpoints_text->SetValue( buff );
    reset();
    m_parent->mCanvas->Refresh();
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnUserpointsSliderUpdateUI ( wxUpdateUIEvent& e ) {
    if (m_userpoints == m_userpoints_slider->GetValue())    return;  //no change
    char  buff[255];
    m_userpoints = m_userpoints_slider->GetValue();
    sprintf( buff, "%d", m_userpoints );
    m_userpoints_text->SetValue( buff );
    reset();
    m_parent->mCanvas->mCavassData->Refresh();
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnDiscretizationSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_discretization = m_discretization_slider->GetValue();
    sprintf( buff, "%d", m_discretization );
    m_discretization_text->SetValue( buff );
    m_processed_points[ (int)(mData[0][2]+0.5) ].clear();
    if (mMode>m_userpoints-1) {
        mMode = m_userpoints-1;
        wxMouseEvent  unused;
        HandleLeftUp( unused );
    }
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnDiscretizationSliderUpdateUI ( wxUpdateUIEvent& e ) {
    //any change?
    if (m_discretization == m_discretization_slider->GetValue())    return; //no
    char  buff[255];
    m_discretization = m_discretization_slider->GetValue();
    sprintf( buff, "%d", m_discretization );
    m_discretization_text->SetValue( buff );
    m_processed_points[ (int)(mData[0][2]+0.5) ].clear();
    if (mMode>m_userpoints-1) {
        mMode = m_userpoints-1;
        wxMouseEvent  unused;
        HandleLeftUp( unused );
    }
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnUserpointsTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_userpoints_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const int          t = strtol( ptr, &endptr, 10 );
    if (endptr == ptr)    return;  //ignore bad values

    m_userpoints = t;
    const int  v   = m_userpoints;
    m_userpoints_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnDiscretizationTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_discretization_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const int          t = strtol( ptr, &endptr, 10 );
    if (endptr == ptr)    return;  //ignore bad values

    m_discretization = t;
    const int  v = m_discretization;
    m_discretization_slider->SetValue( v );
    m_processed_points[ (int)(mData[0][2]+0.5) ].clear();
    if (mMode>m_userpoints-1) {
        mMode = m_userpoints-1;
        wxMouseEvent  unused;
        HandleLeftUp( unused );
    }
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void SnakesDialog::OnTensileSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_tensile = ((double)m_tensile_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_tensile );
    m_tensile_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnTensileSliderUpdateUI ( wxUpdateUIEvent& e ) {
    //any change?
    if (m_tensile == ((double)m_tensile_slider->GetValue()) / sSliderDivisor)
        return;  //no change
    char  buff[255];
    m_tensile = ((double)m_tensile_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_tensile );
    m_tensile_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnFlexuralSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_flexural = ((double)m_flexural_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_flexural );
    m_flexural_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnFlexuralSliderUpdateUI ( wxUpdateUIEvent& e ) {
    //any change?
    if (m_flexural == ((double)m_flexural_slider->GetValue()) / sSliderDivisor)
        return;  //no change
    char  buff[255];
    m_flexural = ((double)m_flexural_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_flexural );
    m_flexural_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnExternalSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_external = ((double)m_external_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_external );
    m_external_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnExternalSliderUpdateUI ( wxUpdateUIEvent& e ) {
    //any change?
    if (m_external == ((double)m_external_slider->GetValue()) / sSliderDivisor)
        return;  //no change
    char  buff[255];
    m_external  = ((double)m_external_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_external );
    m_external_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnInflationSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_inflation = ((double)m_inflation_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_inflation );
    m_inflation_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnInflationSliderUpdateUI ( wxUpdateUIEvent& e ) {
    if (m_inflation
          == ((double)m_inflation_slider->GetValue()) / sSliderDivisor)
        return;
    char  buff[255];
    m_inflation = ((double)m_inflation_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_inflation );
    m_inflation_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnThresholdSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_threshold = m_threshold_slider->GetValue();
    sprintf( buff, "%d", m_threshold );
    m_threshold_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnThresholdSliderUpdateUI ( wxUpdateUIEvent& e ) {
    if (m_threshold == m_threshold_slider->GetValue())    return;
    char  buff[255];
    m_threshold  = m_threshold_slider->GetValue();
    sprintf( buff, "%d", m_threshold );
    m_threshold_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnIterationsSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_iterations = m_iterations_slider->GetValue();
    sprintf( buff, "%d", m_iterations );
    m_iterations_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnIterationsSliderUpdateUI ( wxUpdateUIEvent& e ) {
    if (m_iterations == m_iterations_slider->GetValue())    return;
    char  buff[255];
    m_iterations = m_iterations_slider->GetValue();
    sprintf( buff, "%d", m_iterations );
    m_iterations_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnRadiusSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_radius = m_radius_slider->GetValue();
    sprintf( buff, "%d", m_radius );
    m_radius_text->SetValue( buff );
}
//......................................................................
//especially (only) need on X11 (w/out GTK) to get slider events.
//it appears to be continually called so one must only respond when
//something actually changes.
#ifdef __WXX11__
void SnakesDialog::OnRadiusSliderUpdateUI ( wxUpdateUIEvent& e ) {
    if (m_radius == m_radius_slider->GetValue())    return;
    char  buff[255];
    m_radius = m_radius_slider->GetValue();
    sprintf( buff, "%d", m_radius );
    m_radius_text->SetValue( buff );
}
#endif
//----------------------------------------------------------------------
void SnakesDialog::OnTensileTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_tensile_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr==ptr)    return;  //ignore bad values

    m_tensile = d;
    const int  v   = (int)(m_tensile*sSliderDivisor + 0.5);
    m_tensile_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnFlexuralTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_flexural_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr==ptr)    return;  //ignore bad values

    m_flexural = d;
    const int  v = (int)(m_flexural*sSliderDivisor + 0.5);
    m_flexural_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnExternalTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_external_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr==ptr)    return;  //ignore bad values

    m_external = d;
    const int  v = (int)(m_external*sSliderDivisor + 0.5);
    m_external_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnInflationTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_inflation_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr==ptr)    return;  //ignore bad values

    m_inflation = d;
    const int  v   = (int)(m_inflation*sSliderDivisor + 0.5);
    m_inflation_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnThresholdTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_threshold_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const int          d = strtol( ptr, &endptr, 10 );
    if (endptr==ptr)    return;  //ignore bad values

    m_threshold = d;
    const int  v   = m_threshold;
    m_threshold_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnIterationsTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_iterations_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const int          d = strtol( ptr, &endptr, 10 );
    if (endptr==ptr)    return;  //ignore bad values

    m_iterations = d;
    const int  v   = m_iterations;
    m_iterations_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnRadiusTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_radius_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const int          d = strtol( ptr, &endptr, 10 );
    if (endptr==ptr)    return;  //ignore bad values

    m_radius = d;
    const int  v   = m_radius;
    m_radius_slider->SetValue( v );
}
//----------------------------------------------------------------------
void SnakesDialog::OnShowNormals ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        m_show_normals = true;
    } else {
        m_show_normals = false;
    }
    m_parent->mCanvas->Refresh();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void SnakesDialog::HandleChar ( wxKeyEvent& e ) {
}
//----------------------------------------------------------------------
void SnakesDialog::HandleMouseMove ( wxMouseEvent& e ) {
    //cout << "SnakesDialog::HandleMouseMove" << endl;
    //wxLogMessage( "SnakesDialog::HandleMouseMove" );
    if (mMode>=m_userpoints)    return;

    wxClientDC  dc(m_parent->mCanvas);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //determine the pixel value under the pointer
    int  x, y, z;
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );
    canvas->mapWindowToData( dc.DeviceToLogicalX( pos.x ),
        dc.DeviceToLogicalY( pos.y ), x, y, z );

    //don't allow snake to span different slices
    //if (mMode>0)    z=mData[0][2];

    mData[mMode][0] = x;  mData[mMode][1] = y;  mData[mMode][2] = z;
    m_parent->mCanvas->Refresh();
}
//----------------------------------------------------------------------
void SnakesDialog::HandleRightDown  ( wxMouseEvent& e ) {
    cout << "SnakesDialog::HandleRightDown"  << endl;
    wxLogMessage( "SnakesDialog::HandleRightDown" );
    if (mMode<=3)   return;  //we need at least 3 points before bailing
    m_userpoints = mMode;
    //update the slider
    m_userpoints_slider->SetValue( m_userpoints );
    //update the text box
    char  buff[255];
    sprintf( buff, "%d", m_userpoints );
    m_userpoints_text->SetValue( buff );
    --mMode;  //because it will be pre-incremented below
    HandleLeftUp( e );
}
//----------------------------------------------------------------------
void SnakesDialog::HandleLeftDown  ( wxMouseEvent& e ) {
    //cout << "SnakesDialog::HandleLeftDown"  << endl;
    //wxLogMessage( "SnakesDialog::HandleLeftDown" );
}
//----------------------------------------------------------------------
extern "C" int compare ( const void* A, const void* B );
int compare ( const void* A, const void* B ) {
    const int*  a = (const int*)A;
    const int*  b = (const int*)B;
    if (a[1]<b[1])    return -1;
    if (a[1]>b[1])    return  1;
    return 0;
}

void SnakesDialog::HandleLeftUp ( wxMouseEvent& e ) {
    cout << "SnakesDialog::HandleLeftUp"    << endl;
    wxLogMessage( "SnakesDialog::HandleLeftUp" );

    const double  delta=1.0/(m_discretization+1.0);
    ++mMode;
    if (mMode==m_userpoints) {    //ready to discretize the line & snake away!
        mPoints.clear();
        //just in case the points were specified in an order different
        // than expected (i.e., least y to greatest y)
        //qsort( mData, m_userpoints, 3*sizeof(mData[0][0]), compare );
        timestamp();    mLogfile << " HandleLeftUp" << endl;
        int  i;
        for (i=0; i<m_userpoints; i++) {
            mLogfile << " mData[" << i << "]=(" << mData[i][0] << ","
                     << mData[i][1] << "," << mData[i][2] << ")";
        }
        mLogfile << endl << flush;
        p3d<double>*  previous=NULL;
        int  start=0;
        if (mExtendToEdges) {  //from the top to the first user point
            //add a point at the top (i.e., at y=0) if necessary
            previous = new p3d<double>( mData[0][0], 0, mData[0][2] );
            mPoints.push_back( previous );
            //was the first user specified point in the first row (y==0)?
            if (mData[0][1]==0)    start=1;
        } else {
            previous = new p3d<double>( mData[0][0], mData[0][1], mData[0][2], User|Moveable );
            mPoints.push_back( previous );
            start=1;
        }
        for (i=start; i<m_userpoints; i++) {
            p3d<double>*  next = new p3d<double>( mData[i][0], mData[i][1],
                                                  mData[i][2], User|Moveable );
            //add points between the previous point and the next
            for (double d=delta; d<1.0; d+=delta) {
                const double  e = 1.0-d;
                const int  newX = (int)((d*next->x + e*previous->x)+0.5);
                const int  newY = (int)((d*next->y + e*previous->y)+0.5);
                const int  newZ = (int)((d*next->z + e*previous->z)+0.5);
                if (newY!=next->y && newY!=mPoints[mPoints.size()-1]->y) {
                    p3d<double>*  temp = new p3d<double>( newX, newY, newZ );
                    mPoints.push_back( temp );
                }
            }
            //now add the next user-specified point
            mPoints.push_back( next );
            previous = next;
        }

        if (mExtendToEdges) {  //from the bottom user point to the bottom of the image
            //did the user specify a point in the last row?
            if (mData[m_userpoints-1][1]!=m_parent->mCanvas->mCavassData->m_ySize-1) {
                //no, the user did NOT specify a point in the last row.
                // so we will specify the new last point below and also specify
                // the points in-between.
                p3d<double>*  next = new p3d<double>( mData[m_userpoints-1][0],
                    m_parent->mCanvas->mCavassData->m_ySize-1, mData[m_userpoints-1][2] );
                //add points between the last point (specified by oldX,oldY,oldZ
                //  and 'next'
                for (double d=delta; d<=1.0-delta; d+=delta) {
                    const double  e = 1.0-d;
                    const int  newX = (int)((d*next->x + e*previous->x)+0.5);
                    const int  newY = (int)((d*next->y + e*previous->y)+0.5);
                    const int  newZ = (int)((d*next->z + e*previous->z)+0.5);
                    if (newY!=next->y && newY!=mPoints[mPoints.size()-1]->y) {
                        p3d<double>*  temp = new p3d<double>( newX, newY, newZ );
                        mPoints.push_back( temp );
                    }
                }
                //now add 'next'
                mPoints.push_back( next );
                previous = next;
            }
        }

        //check for problems
        for (i=1; i<(int)mPoints.size(); i++) {
            if (mPoints[i]->y == mPoints[i-1]->y) {
                cout << "problemo at (" << mPoints[i-1]->x << ","
                     << mPoints[i-1]->y << ") and ("
                     << mPoints[i]->x << "," << mPoints[i]->y << ")!"
                     << endl;
                wxLogMessage( "problemo at (%f,%f) and (%f,%f)!",
                    mPoints[i-1]->x, mPoints[i-1]->y,
                    mPoints[i]->x,   mPoints[i]->y );
            }
        }

        if (mCompleteContour) {
            if (mExtendToEdges) {
                const double  dbar = getDBar();
                cout << "dbar = " << dbar << endl;
                //add points along the bottom edge
                int        ix;
                int        iy = (int)mPoints[mPoints.size()-1]->y;
                const int  iz = (int)mPoints[mPoints.size()-1]->z;
                double     x;
                for (x=mPoints[mPoints.size()-1]->x + dbar; ; x+=dbar) {
                    ix = (int)(x+0.5);
                    if (ix>=m_parent->mCanvas->mCavassData->m_xSize)    break;
#if 1
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz );
#else
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz, Fixed|Contour );
#endif
                    mPoints.push_back( temp );
                }
                //add the lower right corner point if necessary
                if (mPoints[mPoints.size()-1]->x < m_parent->mCanvas->mCavassData->m_xSize-1) {
                    const int  ix = m_parent->mCanvas->mCavassData->m_xSize-1;
#if 1
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz );
#else
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz, Fixed|Contour );
#endif
                    mPoints.push_back( temp );
                }
                //move upward from the lower right corner to the top right corner
                ix = m_parent->mCanvas->mCavassData->m_xSize-1;
                for (double y=m_parent->mCanvas->mCavassData->m_ySize-1-dbar; ; y-=dbar) {
                    iy = (int)(y+0.5);
                    if (iy<0)    break;
#if 1
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz );
#else
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz, Fixed|Contour );
#endif
                    mPoints.push_back( temp );
                }
                //add the upper right corner point if necessary
                if (mPoints[mPoints.size()-1]->y != 0) {
#if 1
                    p3d<double>*  temp = new p3d<double>( ix, 0, iz );
#else
                    p3d<double>*  temp = new p3d<double>( ix, 0, iz, Fixed|Contour );
#endif
                    mPoints.push_back( temp );
                }
                
                //move left from the top right corner to the start of the contour
                iy = 0;
                for (x=m_parent->mCanvas->mCavassData->m_xSize-1-dbar; ; x-=dbar) {
                    ix = (int)(x+0.5);
                    if (ix <= mPoints[0]->x)    break;
#if 1
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz );
#else
                    p3d<double>*  temp = new p3d<double>( ix, iy, iz, Fixed|Contour );
#endif
                    mPoints.push_back( temp );
                }
            } else {
                //todo: complete contour but don't extend to edges
                p3d<double>*  prev = new p3d<double>( mData[m_userpoints-1][0],
                    mData[m_userpoints-1][1], mData[m_userpoints-1][2] );
                p3d<double>*  next = new p3d<double>( mData[0][0], mData[0][1], mData[0][2] );
                //add points between the last point (prev) and 'next'
                for (double d=delta; d<=1.0-delta; d+=delta) {
                    const double  e = 1.0-d;
                    const int  newX = (int)((d*next->x + e*prev->x)+0.5);
                    const int  newY = (int)((d*next->y + e*prev->y)+0.5);
                    const int  newZ = (int)((d*next->z + e*prev->z)+0.5);
                    if ( newX==prev->x && newY==prev->y && newZ==prev->z )    continue;
                    if ( newX==next->x && newY==next->y && newZ==next->z )    continue;
                    p3d<double>*  temp = new p3d<double>( newX, newY, newZ );
                    mPoints.push_back( temp );
                }
            }
        }

        ++mMode;
        m_update->Enable( true );
        m_process_all->Enable( true );
        SetTitle( "SnakesDialog" );
    } else {
        char  buff[ 256 ];
        sprintf( buff, "SnakesDialog %d of %d points specified.", mMode, m_userpoints );
        SetTitle( buff );
    }
    m_parent->mCanvas->Refresh();
}
//----------------------------------------------------------------------
double SnakesDialog::calculateEnergy ( void ) {
    if (mPoints.size()==0)    return 0.0;
    double  tensile=0.0;
    int     i;
    for (i=1; i<(int)mPoints.size(); i++) {
        tensile += mag( *mPoints[i] - *mPoints[i-1] );
    }

    double  flexural=0.0;
    for (i=1; i<(int)mPoints.size()-1; i++) {
        flexural += mag( *mPoints[i+1] - 2.0 * *mPoints[i] + *mPoints[i-1] );
    }

    const double  magGradient = mag( gradient2D(mPoints[i]->x, mPoints[i]->y,
                                                mPoints[i]->z) );
    wxLogMessage( "e=( %f, %f, %f )", m_tensile*tensile,
        m_flexural*flexural, m_external*magGradient );
    return (m_tensile*tensile + m_flexural*flexural
            - m_external*magGradient);
}
//----------------------------------------------------------------------
p3d<double> SnakesDialog::gradient2D ( const int x, const int y,
    const int z ) const
{
    /*
    calculated using Sobel:
        1  0  -1         1  2  1
        2  0  -2         0  0  0
        1  0  -1        -1 -2 -1
    */
    if (m_parent->mCanvas->mCavassData->m_size==1) {
        unsigned char*  p = (unsigned char*)m_parent->mCanvas->mCavassData->m_data;
        p3d<double>  t;
        t.x =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x-1, y,   z)
            +  -2 * m_parent->get2DSmoothedData(p, x+1, y,   z)
            +   1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        t.y =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x,   y-1, z)
            +   1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -2 * m_parent->get2DSmoothedData(p, x,   y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        return t;
    } else if (m_parent->mCanvas->mCavassData->m_size==2) {
        unsigned short*  p = (unsigned short*)m_parent->mCanvas->mCavassData->m_data;
        p3d<double>  t;
        t.x =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x-1, y,   z)
            +  -2 * m_parent->get2DSmoothedData(p, x+1, y,   z)
            +   1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        t.y =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x,   y-1, z)
            +   1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -2 * m_parent->get2DSmoothedData(p, x,   y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        return t;
    } else if (m_parent->mCanvas->mCavassData->m_size==4) {
        int*  p = (int*)m_parent->mCanvas->mCavassData->m_data;
        p3d<double>  t;
        t.x =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x-1, y,   z)
            +  -2 * m_parent->get2DSmoothedData(p, x+1, y,   z)
            +   1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        t.y =   1 * m_parent->get2DSmoothedData(p, x-1, y-1, z)
            +   2 * m_parent->get2DSmoothedData(p, x,   y-1, z)
            +   1 * m_parent->get2DSmoothedData(p, x+1, y-1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x-1, y+1, z)
            +  -2 * m_parent->get2DSmoothedData(p, x,   y+1, z)
            +  -1 * m_parent->get2DSmoothedData(p, x+1, y+1, z);
        return t;
    } else {
        assert(0);  //todo
    }
    return p3d<double>();
}
//----------------------------------------------------------------------
p3d<double> SnakesDialog::gradient2D ( const double x, const double y,
    const double z ) const
{
    return gradient2D( (int)(x+0.5), (int)(y+0.5), (int)(z+0.5) );
}
//----------------------------------------------------------------------
void SnakesDialog::HandlePaint ( wxPaintEvent& e, wxMemoryDC& mdc ) {
    //cout << "in SnakesDialog::HandlePaint" << endl;
    //wxLogMessage( "in SnakesDialog::HandlePaint" );
    int  i;
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );

    //draw the discretized set of points (w/ lines) for a single slice (in mPoints)
#if wxCHECK_VERSION(2, 9, 0)
    mdc.SetBrush( wxBrush(wxColour(Green), wxBRUSHSTYLE_SOLID) );
    mdc.SetPen( wxPen(wxColour(Green), 1, wxPENSTYLE_SOLID) );
#else
    mdc.SetBrush( wxBrush(wxColour(Green), wxSOLID) );
    mdc.SetPen( wxPen(wxColour(Green), 1, wxSOLID) );
#endif
    p3d<double>*  last=NULL;
    int  lastWx=0, lastWy=0;
    for (i=0; i<(int)mPoints.size(); i++) {
        p3d<double>*  p = mPoints[i];
        int  wx, wy;
        canvas->mapDataToWindow( (int)(p->x+0.5), (int)(p->y+0.5),
            (int)(p->z+0.5), wx, wy );
        //cout << "    " << i << ": plotting " << wx << "," << wy << endl;
        //wxLogMessage( "    %d: plotting %d,%d", i, wx, wy );
        mdc.DrawRectangle( wx-2, wy-2, 4, 4 );
        if (last!=NULL) {
            mdc.DrawLine( lastWx, lastWy, wx, wy );
        }

        //draw a vector indicating the gradient
        p3d<double>  g = gradient2D(p->x, p->y, p->z);
        const double  m = mag( g );
        if (m>0.01 && m_show_normals) {
            g /= m;
            //if (m_parent->mCanvas->getScale()<2.0)  g *= 100.0;
            //else                                     g *= 10.0;
            g *= 10.0;
            g += *p;
            int  wgx, wgy;
            canvas->mapDataToWindow( (int)(g.x+0.5), (int)(g.y+0.5),
                (int)(g.z+0.5), wgx, wgy );
#if wxCHECK_VERSION(2, 9, 0)
            mdc.SetPen( wxPen(wxColour(Yellow), 1, wxPENSTYLE_SOLID) );
            mdc.DrawLine( wx, wy, wgx, wgy );
            mdc.SetPen( wxPen(wxColour(Green), 1, wxPENSTYLE_SOLID) );
#else
            mdc.SetPen( wxPen(wxColour(Yellow), 1, wxSOLID) );
            mdc.DrawLine( wx, wy, wgx, wgy );
            mdc.SetPen( wxPen(wxColour(Green), 1, wxSOLID) );
#endif
        }

        last   = p;
        lastWx = wx;
        lastWy = wy;
    }

    if (mCompleteContour && mPoints.size()>2) {
        //draw a line between the last point and the first to close the contour
        p3d<double>*  p = mPoints[0];
        int  wx, wy;
        canvas->mapDataToWindow( (int)(p->x+0.5), (int)(p->y+0.5),
            (int)(p->z+0.5), wx, wy );
        mdc.DrawLine( lastWx, lastWy, wx, wy );
    }

    wxLogMessage( "SnakesDialog::HandlePaint: energy=%f (t=%f,f=%f,e=%f)",
        calculateEnergy(), m_tensile, m_flexural, m_external );

    //draw the discretized set of points (w/ lines) for all slices
#if wxCHECK_VERSION(2, 9, 0)
    mdc.SetBrush( wxBrush(wxColour(Red), wxBRUSHSTYLE_SOLID) );
    mdc.SetPen( wxPen(wxColour(MenuBlue), 1, wxPENSTYLE_SOLID) );
#else
    mdc.SetBrush( wxBrush(wxColour(Red), wxSOLID) );
    mdc.SetPen( wxPen(wxColour(MenuBlue), 1, wxSOLID) );
#endif
    for (int j=0; j<m_parent->mCanvas->mCavassData->m_zSize; j++) {
        last=NULL;  lastWx=0;  lastWy=0;
        for (i=0; i<(int)m_processed_points[j].size(); i++) {
            p3d<double>*  p = m_processed_points[j][i];
            int  wx, wy;
            canvas->mapDataToWindow( (int)(p->x+0.5),
                (int)(p->y+0.5), (int)(p->z+0.5), wx, wy );
            if (wx==-1 && wy==-1)    continue;
            if (p->flags & User) {
#if wxCHECK_VERSION(2, 9, 0)
#if defined (WIN32) || defined (_WIN32)
                mdc.SetBrush( wxBrush( *(wxTheColourDatabase->FindColour("GOLDENROD")), wxBRUSHSTYLE_SOLID ) );
#else
                mdc.SetBrush(
                  wxBrush( wxTheColourDatabase->Find("GOLDENROD"), wxBRUSHSTYLE_SOLID ) );
#endif
                mdc.DrawRectangle( wx-4, wy-4, 8, 8 );
                mdc.SetBrush( wxBrush(wxColour(Red),    wxBRUSHSTYLE_SOLID) );
#else
#if defined (WIN32) || defined (_WIN32)
                mdc.SetBrush( wxBrush( *(wxTheColourDatabase->FindColour("GOLDENROD")), wxSOLID ) );
#else
                mdc.SetBrush(
                  wxBrush( wxTheColourDatabase->Find("GOLDENROD"), wxSOLID ) );
#endif
                mdc.DrawRectangle( wx-4, wy-4, 8, 8 );
                mdc.SetBrush( wxBrush(wxColour(Red),    wxSOLID) );
#endif
            }
            mdc.DrawRectangle( wx-2, wy-2, 4, 4 );
            if (last!=NULL) {
                mdc.DrawLine( lastWx, lastWy, wx, wy );
            }

            //draw a vector indicating the gradient
            p3d<double>  g = gradient2D(p->x, p->y, p->z);
            const double  m = mag( g );
            if (m>0.01 && m_show_normals) {
                g /= m;
                //if (m_parent->mCanvas->getScale()<2.0)  g *= 100.0;
                //else                                     g *= 10.0;
                g *= 10.0;
                g += *p;
                int  wgx, wgy;
                canvas->mapDataToWindow( (int)(g.x+0.5), (int)(g.y+0.5),
                    (int)(g.z+0.5), wgx, wgy );
#if wxCHECK_VERSION(2, 9, 0)
                mdc.SetPen( wxPen(wxColour(Yellow), 1, wxPENSTYLE_SOLID) );
                mdc.DrawLine( wx, wy, wgx, wgy );
                mdc.SetPen( wxPen(wxColour(MenuBlue), 1, wxPENSTYLE_SOLID) );
#else
                mdc.SetPen( wxPen(wxColour(Yellow), 1, wxSOLID) );
                mdc.DrawLine( wx, wy, wgx, wgy );
                mdc.SetPen( wxPen(wxColour(MenuBlue), 1, wxSOLID) );
#endif
            }

            last   = p;
            lastWx = wx;
            lastWy = wy;
        }
        if (mCompleteContour && m_processed_points[j].size()>2) {
            //draw a line between the last point and the first to close
            // the contour
            p3d<double>*  p = m_processed_points[j][0];
            int  wx, wy;
            canvas->mapDataToWindow( (int)(p->x+0.5),
                (int)(p->y+0.5), (int)(p->z+0.5), wx, wy );
            if (wx==-1 && wy==-1)    continue;
            mdc.DrawLine( lastWx, lastWy, wx, wy );
        }
    }

    //draw the points indicated by the user
#if wxCHECK_VERSION(2, 9, 0)
    mdc.SetBrush( wxBrush(wxColour(Yellow), wxBRUSHSTYLE_SOLID) );
#else
    mdc.SetBrush( wxBrush(wxColour(Yellow), wxSOLID) );
#endif
    for (i=0; i<mMode && i<m_userpoints; i++) {  //points indicated by user
        int  wx, wy;
        canvas->mapDataToWindow( mData[i][0], mData[i][1],
                                             mData[i][2], wx, wy);
        if (wx>=0 && wy>=0)    mdc.DrawRectangle( wx-4, wy-4, 8, 8 );
    }

    //finally, draw the gradient as moves mouse and picks points
    if (mMode>=0 && mMode<m_userpoints) {
        p3d<double>  p( mData[mMode][0], mData[mMode][1], mData[mMode][2] );
        if (p.x!=-1) {
            int  wx, wy;
            canvas->mapDataToWindow( (int)(p.x+0.5),
                (int)(p.y+0.5), (int)(p.z+0.5), wx, wy);
            //draw a vector indicating the gradient
            p3d<double>  g = gradient2D( p.x, p.y, p.z );
            const double  m = mag( g );
            wxLogMessage( "HandlePaint: p=(%f,%f,%f), g=(%f,%f,%f), mag=%f", p.x, p.y, p.z, g.x, g.y, g.z, m );
            if (m>0.0001) {
                g /= m;
                wxLogMessage( "    HandlePaint: unit g=(%f,%f,%f)", g.x, g.y, g.z );
                //if (m_parent->mCanvas->getScale()<2.0)  g *= 20.0;
                //else                                     g *= 10.0;
                g *= 10.0;
                wxLogMessage( "        HandlePaint: 10*g=(%.1f,%.1f,%.1f)", g.x, g.y, g.z );
                g += p;
                wxLogMessage( "            HandlePaint: tg=(%.1f,%.1f,%.1f)", g.x, g.y, g.z );
                int  wgx, wgy;
                canvas->mapDataToWindow( (int)(g.x+0.5),
                    (int)(g.y+0.5), (int)(g.z+0.5), wgx, wgy );
#if wxCHECK_VERSION(2, 9, 0)
                mdc.SetPen( wxPen(wxColour(Yellow), 1, wxPENSTYLE_SOLID) );
#else
                mdc.SetPen( wxPen(wxColour(Yellow), 1, wxSOLID) );
#endif
                mdc.DrawLine( wx, wy, wgx, wgy );
            }
        }
    }
    //cout << "out SnakesDialog::HandlePaint" << endl;
    //wxLogMessage( "out SnakesDialog::HandlePaint" );
}
//----------------------------------------------------------------------
void SnakesDialog::OnClear ( wxCommandEvent& e ) {
    timestamp();    mLogfile << " OnClear" << endl << flush;
    reset();
    m_parent->mCanvas->Refresh();
}
//----------------------------------------------------------------------
void SnakesDialog::OnCornerSliceChange ( void ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );
    wxString  s = wxString::Format(
        "Clear (%d)", canvas->getSliceNo() );
    //m_clear->SetTitle( s );
    m_clear->SetLabel( s );
    //starting with the first slice that is displayed, find the next slice
    // that has already been processed.
    int  z;
    for (z=canvas->getSliceNo(); z<canvas->mCavassData->m_zSize; z++) {
        if (m_processed_slice[z])    break;
    }
    const int  zAlreadyProcessed = z;  //need a copy for later

    //update "Next" button
    //starting with the processed slice found above, find the next slice
    // that needs to be processed
    int  found=-1;
    for ( ; z<canvas->mCavassData->m_zSize; z++) {
        if (!m_processed_slice[z]) {
            found = z;
            break;
        }
    }
    if (found>=0) {
        wxString  s = wxString::Format( "Next (%d)", found );
        //m_next->SetTitle( s );
        m_next->SetLabel( s );
        m_next->Enable( true );
    } else {
        //m_next->SetTitle( "Next" );
        m_next->SetLabel( "Next" );
        m_next->Enable( false );
    }

    //update "Previous" button
    //starting with the processed slice found above, find the (first) previous slice
    // that needs to be processed
    z = zAlreadyProcessed;
    found=-1;
    for ( ; z<canvas->mCavassData->m_zSize; z--) {
        if (!m_processed_slice[z]) {
            found = z;
            break;
        }
    }
    if (found>=0) {
        wxString  s = wxString::Format( "Previous (%d)", found );
        //m_previous->SetTitle( s );
        m_previous->SetLabel( s );
        m_previous->Enable( true );
    } else {
        //m_previous->SetTitle( "Previous" );
        m_previous->SetLabel( "Previous" );
        m_previous->Enable( false );
    }

    //update "Save" button
    if (mPoints.size()>0) {
        z = (int)(mPoints[0]->z+0.5);
        wxString  s = wxString::Format( "Save (%d)", z );
        //m_save->SetTitle( s );
        m_save->SetLabel( s );
        m_save->Enable( true );
    } else {
        //m_save->SetTitle( "Save" );
        m_save->SetLabel( "Save" );
        m_save->Enable( false );
    }

    //update "Update" button
    if (mPoints.size()>0) {
        z = (int)(mPoints[0]->z+0.5);
        wxString  s = wxString::Format( "Update (%d)", z );
        //m_update->SetTitle( s );
        m_update->SetLabel( s );
        m_update->Enable( true );
    } else {
        //m_update->SetTitle( "Update" );
        m_update->SetLabel( "Update" );
        m_update->Enable( false );
    }
}
//----------------------------------------------------------------------
static wxCriticalSection  sLock;
//----------------------------------------------------------------------
void SnakesDialog::OnNext ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );
    wxCriticalSectionLocker  locker( sLock );
#if 0
    for (int i=0; i<(int)mPoints.size(); i++) {
        if (mPoints[i]->z < canvas->mCavassData->m_zSize-1) {
            mPoints[i]->z += 1.0;
        }
    }
    canvas->Refresh();
#endif
    //starting with the first slice that is displayed, find the first slice
    // that has already been processed.
    int  z;
    for (z=canvas->getSliceNo(); z<canvas->mCavassData->m_zSize; z++) {
        if (m_processed_slice[z])    break;
    }
    //starting with the processed slice found above, find the next slice
    // that needs to be processed
    int  found=-1;
    for ( ; z<canvas->mCavassData->m_zSize; z++) {
        if (!m_processed_slice[z]) {
            found = z;
            break;
        }
    }
    if (found<0 || !m_processed_slice[found-1] || m_processed_points[found-1].size()<1) {
        return;
    }
    //reload mPoints with contents of previous slice but for this slice
    int  i;
    for (i=0; i<MAXPOINTS; i++) {
        mData[i][0] = mData[i][1] = mData[i][2] = -1;
    }
    int  j;
    for (i=0,j=0; i<(int)m_processed_points[found-1].size(); i++) {
        if (m_processed_points[found-1][i]->flags & User) {
            mData[j][0] = (int)m_processed_points[found-1][i]->x;
            mData[j][1] = (int)m_processed_points[found-1][i]->y;
            mData[j][2] = found;
            ++j;
        }
    }
    //for (int j=0; j<MAXPOINTS; j++) {
    //    if (mData[j][2]!=-1)    mData[j][2] = found;
    //}
    mPoints.clear();
    for (i=0; i<(int)m_processed_points[found-1].size(); i++) {
        p3d<double>*  p = new p3d<double>(
            m_processed_points[found-1][i]->x,
            m_processed_points[found-1][i]->y,
            found,
            m_processed_points[found-1][i]->flags );
        mPoints.push_back( p );
    }
    canvas->Refresh();
    OnUpdate( e );
    canvas->Refresh();
    checkAllProcessed();
}
//----------------------------------------------------------------------
void SnakesDialog::OnPrevious ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(m_parent->mCanvas);
    assert( canvas != NULL );
    wxCriticalSectionLocker  locker( sLock );
#if 0
    for (int i=0; i<(int)mPoints.size(); i++) {
        if (mPoints[i]->z >= 1.0) {
            mPoints[i]->z -= 1.0;
        }
    }
    canvas->Refresh();
#endif
    //starting with the first slice that is displayed, find the first slice
    // that has already been processed.
    int  z;
    for (z=canvas->getSliceNo(); z<canvas->mCavassData->m_zSize; z++) {
        if (m_processed_slice[z])    break;
    }
    //starting with the processed slice found above, find the (first) previous slice
    // that needs to be processed
    int  found=-1;
    for ( ; z<canvas->mCavassData->m_zSize; z--) {
        if (!m_processed_slice[z]) {
            found = z;
            break;
        }
    }
    if (found<0 || !m_processed_slice[found+1] || m_processed_points[found+1].size()<1) {
        return;
    }
    //reload mPoints with contents of previous slice but for this slice
    int  i;
    for (i=0; i<MAXPOINTS; i++) {
        mData[i][0] = mData[i][1] = mData[i][2] = -1;
    }
    int  j;
    for (i=0,j=0; i<(int)m_processed_points[found+1].size(); i++) {
        if (m_processed_points[found+1][i]->flags & User) {
            mData[j][0] = (int)m_processed_points[found+1][i]->x;
            mData[j][1] = (int)m_processed_points[found+1][i]->y;
            mData[j][2] = found;
            ++j;
        }
    }
    //for (int j=0; j<MAXPOINTS; j++) {
    //    if (mData[j][2]!=-1)    mData[j][2] = found;
    //}
    mPoints.clear();
    for (i=0; i<(int)m_processed_points[found+1].size(); i++) {
        p3d<double>*  p = new p3d<double>(
            m_processed_points[found+1][i]->x,
            m_processed_points[found+1][i]->y,
            found,
            m_processed_points[found+1][i]->flags );
        mPoints.push_back( p );
    }
    canvas->Refresh();
    OnUpdate( e );
    canvas->Refresh();
    checkAllProcessed();
}
//----------------------------------------------------------------------
void SnakesDialog::checkAllProcessed ( void ) {
    wxLogMessage( "in SnakesDialog::checkAllProcessed" );

    for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
        if (!m_processed_slice[z])    return;
    }
    //all slices have now been processed (segmented).
    MontageFrame*  f = new MontageFrame();
    f->loadData( "segmented via snakes", m_parent->mCanvas->mCavassData->m_xSize,
                 m_parent->mCanvas->mCavassData->m_ySize, m_parent->mCanvas->mCavassData->m_zSize,
                 m_parent->mCanvas->mCavassData->m_xSpacing, m_parent->mCanvas->mCavassData->m_ySpacing, 
                 m_parent->mCanvas->mCavassData->m_zSpacing, m_processed_data,
                 &m_parent->mCanvas->mCavassData->m_vh, m_parent->mCanvas->mCavassData->m_vh_initialized );

    m_update->Enable( true );
    m_process_all->Enable( true );
    SetCursor( *wxSTANDARD_CURSOR );
    m_parent->mCanvas->SetCursor( *wxSTANDARD_CURSOR );

    wxLogMessage( "out SnakesDialog::checkAllProcessed" );
}
//----------------------------------------------------------------------
void SnakesDialog::OnSave ( wxCommandEvent& e ) {
    const int  z = (int)(mPoints[0]->z+0.5);
    if (z==-1)    return;
    char  buff[256];
    if (m_parent->mCanvas->mCavassData->m_fname==NULL)
        sprintf( buff, "slice%03d.txt", z );
    else
        sprintf( buff, "%s-slice%03d.txt", m_parent->mCanvas->mCavassData->m_fname, z );
    wxLogMessage( "logging slice %d to file %s", z, buff );
    FILE*  fp = fopen( buff, "wb" );
    if (fp==NULL) {
        return;
    }
    for (int i=0; i<(int)m_processed_points[z].size(); i++) {
        fprintf( fp, "%d %d %d \n",
            (int)(m_processed_points[z][i]->x+0.5),
            (int)(m_processed_points[z][i]->y+0.5),
            (int)(m_processed_points[z][i]->z+0.5) );
    }
    fclose( fp );    fp=NULL;
}
//----------------------------------------------------------------------
void SnakesDialog::OnSaveAll ( wxCommandEvent& e ) {
    for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
        if (m_processed_points[z].size()<1)    continue;
        char  buff[256];
        if (m_parent->mCanvas->mCavassData->m_fname==NULL)
            sprintf( buff, "slice%03d.txt", z );
        else
            sprintf( buff, "%s-slice%03d.txt", m_parent->mCanvas->mCavassData->m_fname, z );
        wxLogMessage( "saving slice %d to file %s", z, buff );
        FILE*  fp = fopen( buff, "wb" );
        if (fp==NULL)    continue;
        for (int i=0; i<(int)m_processed_points[z].size(); i++) {
            fprintf( fp, "%d %d %d \n",
                (int)(m_processed_points[z][i]->x+0.5),
                (int)(m_processed_points[z][i]->y+0.5),
                (int)(m_processed_points[z][i]->z+0.5) );
        }
        fclose( fp );    fp=NULL;
    }
}
//----------------------------------------------------------------------
void SnakesDialog::OnLoad ( wxCommandEvent& e ) {
    wxString  filename = wxFileSelector( "Select contour points file" );
    if (filename && filename.Length()>0) {
        FILE*  fp = fopen( filename, "rb" );
        if (fp==NULL)    return;
        int  lastZ = -1;  //to detect changes in the slice number
        for ( ; ; ) {
            int  x, y, z;
            int  n = fscanf( fp, "%d %d %d", &x, &y, &z );
            if (n!=3)    break;  //done?
            if (z!=lastZ) {  //if anything already exists, clear it
                lastZ = z;
                m_processed_points[z].clear();
            }
            p3d<double>*  p = new p3d<double>( x, y, z, External );
            m_processed_points[z].push_back( p );
        }
        fclose( fp );
        mMode = 0;
        int  i;
        for (i=0; i<MAXPOINTS; i++) {
            mData[i][0] = mData[i][1] = mData[i][2] = -1;
        }
        mPoints.clear();
        mMode = m_processed_points[lastZ].size();
        for (i=0; i<(int)m_processed_points[lastZ].size(); i++) {
            mData[i][0] = (int)m_processed_points[lastZ][i]->x;
            mData[i][1] = (int)m_processed_points[lastZ][i]->y;
            mData[i][2] = (int)m_processed_points[lastZ][i]->z;
            p3d<double>*  p = new p3d<double>( mData[i][0], mData[i][1], mData[i][2], External );
            mPoints.push_back( p );
        }
        OnCornerSliceChange();
        m_parent->mCanvas->Refresh();
    }
}
//----------------------------------------------------------------------
void SnakesDialog::OnUpdate ( wxCommandEvent& e ) {
#if 0
    for (int k=0; k<(int)mPoints.size(); k++) {
        if (mPoints[k]->flags & User)
            wxLogMessage( "user" );
    }
#endif
    m_update->Enable( false );
    m_process_all->Enable( false );
    SetCursor( wxCursor(wxCURSOR_WAIT) );
    m_parent->mCanvas->SetCursor( wxCursor(wxCURSOR_WAIT) );

    const int  z = (int)(mPoints[0]->z+0.5);
    m_processed_points[z].clear();
    timestamp();    mLogfile << " OnUpdate: z=" << z << endl << flush;

    for (int i=0; i<m_iterations; i++) {
        cout << "SnakesDialog::OnUpdate: iteration # " << i << endl;
        wxLogMessage( "SnakesDialog::OnUpdate: iteration # %d", i );
        updateContour();
//        #ifndef WIN32
            m_parent->mCanvas->Refresh();    wxYield();
//        #endif
    }
//    #ifdef WIN32
//        m_parent->mCanvas->Refresh();     wxYield();
//    #endif

    segment();

    for (int j=0; j<(int)mPoints.size(); j++) {
        if (mPoints[j]->flags & User)
            wxLogMessage( "user" );
        p3d<double>*  p = new p3d<double>( mPoints[j]->x, mPoints[j]->y,
                                           mPoints[j]->z, mPoints[j]->flags );
        m_processed_points[z].push_back( p );
    }

    m_update->Enable( true );
    m_process_all->Enable( true );
    SetCursor( *wxSTANDARD_CURSOR );
    m_parent->mCanvas->SetCursor( *wxSTANDARD_CURSOR );
    m_parent->mCanvas->Refresh();

    OnCornerSliceChange();
}
//----------------------------------------------------------------------
void SnakesDialog::OnDebug ( wxCommandEvent& e ) {
#if 0
    const int  z = m_parent->mCanvas->getSliceNo();
    for (int i=0; i<(int)m_processed_points[z].size(); i++) {
        p3d<double>*  p = m_processed_points[z][i];
        cout << i << ":" << p << " ";
    }
    cout << endl << flush;
#endif
#if 0
    mPoints.clear();
    p3d<double>*  p;
    p = new p3d<double>( 311, 10, 24 );    mPoints.push_back( p );
    p = new p3d<double>( 301, 13, 24 );    mPoints.push_back( p );
    p = new p3d<double>( 308,  0, 24 );    mPoints.push_back( p );
    segment();
#endif
#if 0
    for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
        for (int i=0; i<(int)m_processed_points[z].size(); i++) {
            assert( m_parent->inBounds( (int)(m_processed_points[z][i]->x+0.5),
                (int)(m_processed_points[z][i]->y+0.5),
                (int)(m_processed_points[z][i]->z+0.5) ) );
            assert( m_processed_points[z][i]->z == m_processed_points[z][0]->z );
        }
    }
#endif
#if 0
    void test_lfit ( void );
    test_lfit();
#endif
}
//----------------------------------------------------------------------
void SnakesDialog::segment ( void ) {
    //cout << "in segment" << endl;    wxLogMessage( "in segment" );
    assert( m_processed_slice!=NULL && mPoints.size()>0 );
    assert( mPoints[0]->z>=0 && mPoints[0]->z<m_parent->mCanvas->mCavassData->m_zSize );
    const int  z = (int)(mPoints[0]->z + 0.5);
    m_processed_slice[z] = true;

    int  y;
    //clear whataver is currently in the processed slice data for this slice
    for (y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
        for (int x=0; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
            m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 0;
        }
    }

    //segment using current set of points
#if 0
    for (int i=1; i<(int)mPoints.size(); i++) {
        const int  x1 = (int)(mPoints[i-1]->x+0.5);
        const int  y1 = (int)(mPoints[i-1]->y+0.5);
        const int  z1 = (int)(mPoints[i-1]->z+0.5);

        const int  x2 = (int)(mPoints[i]->x+0.5);
        const int  y2 = (int)(mPoints[i]->y+0.5);
        const int  z2 = (int)(mPoints[i]->z+0.5);

        assert( z1==z2 );
        if (y1==y2) {  //in the same row?
            int  minX = x1;
            if (x2<minX)    minX=x2;
            for (int x=minX; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                m_processed_data[ m_parent->index(x,y1,z1) ] = 1;
            }
        } else if (x1==x2) {  //in the same col?
            int  minY = y1;
            if (y2<minY)    minY=y2;
            int  maxY = y1;
            if (y2>maxY)    maxY=y2;
            for (int y=minY; y<=maxY; y++) {
                for (int x=x1; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                    m_processed_data[ m_parent->index(x,y,z1) ] = 1;
                }
            }
        } else {
            const int  dx = abs(x2-x1);
            const int  dy = abs(y2-y1);
            double     ss = 1.0/dy;
            if (dx>dy)    ss = 1.0/dx;
            for (double p=0; p<=1.0; p+=ss) {
                const int  xp = (int)( (1.0-p)*x1 + p*x2 + 0.5 );
                const int  yp = (int)( (1.0-p)*y1 + p*y2 + 0.5 );
                for (int x=xp; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                    m_processed_data[ m_parent->index(x,yp,z) ] = 1;
                }
            }
        }
    }
    if (mCompleteContour && mPoints.size()>2) {
        const int  x1 = (int)(mPoints[mPoints.size()-1]->x+0.5);
        const int  y1 = (int)(mPoints[mPoints.size()-1]->y+0.5);
        const int  z1 = (int)(mPoints[mPoints.size()-1]->z+0.5);

        const int  x2 = (int)(mPoints[0]->x+0.5);
        const int  y2 = (int)(mPoints[0]->y+0.5);
        const int  z2 = (int)(mPoints[0]->z+0.5);

        assert( z1==z2 );
        if (y1==y2) {  //in the same row?
            int  minX = x1;
            if (x2<minX)    minX=x2;
            for (int x=minX; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                m_processed_data[ m_parent->index(x,y1,z1) ] = 1;
            }
        } else if (x1==x2) {  //in the same col?
            int  minY = y1;
            if (y2<minY)    minY=y2;
            int  maxY = y1;
            if (y2>maxY)    maxY=y2;
            for (int y=minY; y<=maxY; y++) {
                for (int x=x1; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                    m_processed_data[ m_parent->index(x,y,z1) ] = 1;
                }
            }
        } else {
            const int  dx = abs(x2-x1);
            const int  dy = abs(y2-y1);
            double     ss = 1.0/dy;
            if (dx>dy)    ss = 1.0/dx;
            for (double p=0; p<=1.0; p+=ss) {
                const int  xp = (int)( (1.0-p)*x1 + p*x2 + 0.5 );
                const int  yp = (int)( (1.0-p)*y1 + p*y2 + 0.5 );
                for (int x=xp; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
                    m_processed_data[ m_parent->index(x,yp,z) ] = 1;
                }
            }
        }
    }
#endif
#if 0
    assert( mCompleteContour );
    //for each row in the image...
    for (y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
        vector< double >  xValues;
        //for each pair of points...
        for (int i=0; i<(int)mPoints.size(); i++) {
            const double  x1 = mPoints[i]->x;
            const double  y1 = mPoints[i]->y;
            const double  z1 = mPoints[i]->z;
            double  x2, y2, z2;
            if (i+1<(int)mPoints.size()) {
                x2 = mPoints[i+1]->x;
                y2 = mPoints[i+1]->y;
                z2 = mPoints[i+1]->z;
            } else {
                x2 = mPoints[0]->x;
                y2 = mPoints[0]->y;
                z2 = mPoints[0]->z;
            }
            assert( z1==z2 );
            //is y on the line segment between (x1,y1) and (x2,y2)?
            if ( (floor(y1)<=y && y<=ceil(y2)) || (floor(y2)<=y && y<=ceil(y1)) )
            {
                //solve for m (slope), and then the x value along the 
                // line segment between (x1,y1) and (x2,y2)
                double  x;
                //check for a vertical line
                if ( (x2-x1)!= 0.0 ) {
                    //not a vertical line
                    const double  m = (y2-y1) / (x2-x1);
                    //check for a horizontal line
                    if (m!=0) {
                        //not a horizontal line
                        const double  b = y1 - m * x1;
                        x = (y-b) / m;
                    } else {
                        //special case of a horizontal line
                        if (x1<=x2)    x = x1;
                        else           x = x2;
                    }
                } else {
                    //special case of a vertical line
                    x = x1;
                }
                //before we add it to the list, make sure that it is not
                // already there.
                bool  foundIt = false;
                for (int j=0; j<xValues.size(); j++) {
                    const int  a = (int)(x+0.5);
                    const int  b = (int)(xValues[j]+0.5);
                    if (a==b) {
                        foundIt = true;
                        break;
                    }
                }
                if (!foundIt)    xValues.push_back( x );
            }
        }  //end for each pair
        sort( xValues.begin(), xValues.end() );
if (z==5) {
  cout << "segment(): y=" << y << " length=" << xValues.size();
  for (int i=0; i<xValues.size(); i++)
    cout << " " << xValues[i];
  cout << endl;
}
        //for each pair of x values...
        for (int j=0; j<(int)xValues.size()-1; j+=2) {
            for ( int x=(int)(xValues[j]+0.5);
                  x<(int)(xValues[j+1]+0.5) && x<m_parent->mCanvas->mCavassData->m_xSize;
                  x++ )
            {
                m_processed_data[ m_parent->index(x,y,z) ] = 1;
            }
        }
    }
#endif
#if 1
    assert( mCompleteContour );
    //for each pair of points...
    for (int i=0; i<(int)mPoints.size(); i++) {
        const double  x1 = mPoints[i]->x;
        const double  y1 = mPoints[i]->y;
        const double  z1 = mPoints[i]->z;
        double        x2, y2, z2;
        if (i+1<(int)mPoints.size()) {
            x2 = mPoints[i+1]->x;
            y2 = mPoints[i+1]->y;
            z2 = mPoints[i+1]->z;
        } else {
            x2 = mPoints[0]->x;
            y2 = mPoints[0]->y;
            z2 = mPoints[0]->z;
        }
        assert( z1==z2 );
        //check for a vertical line
        if ( (x2-x1)!=0.0 ) {
            //not a vertical line
            const double  m = (y2-y1) / (x2-x1);
            //check for a horizontal line
            if (m!=0) {
                //not a horizontal line
                const double  b = y1 - m * x1;
                int  minX = (int)(x1+0.5);
                int  maxX = (int)(x2+0.5);
                if (minX>maxX) {
                    minX = (int)(x2+0.5);
                    maxX = (int)(x1+0.5);
                }
                for (int x=minX; x<=maxX; x++) {
                    const int  y = (int)( m*x + b + 0.5 );
                    m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 1;
                }
                int  minY = (int)(y1+0.5);
                int  maxY = (int)(y2+0.5);
                if (minY>maxY) {
                    minY = (int)(y2+0.5);
                    maxY = (int)(y1+0.5);
                }
                for (int y=minY; y<=maxY; y++) {
                    const int  x = (int)( (y-b) / m + 0.5 );
                    m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 1;
                }
            } else {
                //special case of a horizontal line
                const int  y = (int)(y1+0.5);
                int  minX = (int)(x1+0.5);
                int  maxX = (int)(x2+0.5);
                if (minX>maxX) {
                    minX = (int)(x2+0.5);
                    maxX = (int)(x1+0.5);
                }
                for (int x=minX; x<=maxX; x++) {
                    m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 1;
                }
            }
        } else {
            //special case of a vertical line
            const int  x = (int)(x1+0.5);
            int  minY = (int)(y1+0.5);
            int  maxY = (int)(y2+0.5);
            if (minY>maxY) {
                minY = (int)(y2+0.5);
                maxY = (int)(y1+0.5);
            }
            for (int y=minY; y<=maxY; y++) {
                m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 1;
            }
        }
    }  //end for each pair of points
    //now, flood the background (to determine the foreground) by checking
    // around the periphery.
    //top & bottom rows rows
    int  x;
    for (x=0; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
        y = 0;
        if (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] == 0)
            flood( x, y, z );
        y = m_parent->mCanvas->mCavassData->m_ySize-1;
        if (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] == 0)
            flood( x, y, z );
    }
    //first & last columns
    for (y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
        x = 0;
        if (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] == 0)
            flood( x, y, z );
        x = m_parent->mCanvas->mCavassData->m_xSize-1;
        if (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] == 0)
            flood( x, y, z );
    }
    //finally, indicate the foreground by converting values of 0 or 1 to 1
    // and values of -1 (background) to 0.
    for (y=0; y<m_parent->mCanvas->mCavassData->m_ySize; y++) {
        for (int x=0; x<m_parent->mCanvas->mCavassData->m_xSize; x++) {
            switch (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ]) {
                case 0:
                case 1:   m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 1;
                          break;
                case -1:  m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = 0;
                          break;
                default:  assert( 0 );
                          break;
            }
        }
    }
#endif
    //cout << "out segment" << endl;    wxLogMessage( "out segment" );
}
//----------------------------------------------------------------------
void SnakesDialog::flood ( const int x, const int y, const int z ) {
    //base cases:
    if (x<0 || x>=m_parent->mCanvas->mCavassData->m_xSize)    return;  //out of bounds
    if (y<0 || y>=m_parent->mCanvas->mCavassData->m_ySize)    return;  //out of bounds
    if (z<0 || z>=m_parent->mCanvas->mCavassData->m_zSize)    return;  //out of bounds
    if (m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] != 0)    return;  //visited

    //mark the background
    m_processed_data[ m_parent->mCanvas->mCavassData->index(x,y,z) ] = -1;
    //flood within the slice
    flood( x-1, y,   z );
    flood( x+1, y,   z );
    flood( x,   y-1, z );
    flood( x,   y+1, z );
}
//----------------------------------------------------------------------
void SnakesDialog::OnProcessAll ( wxCommandEvent& e ) {
    m_update->Enable( false );
    m_process_all->Enable( false );
    SetCursor( wxCursor(wxCURSOR_WAIT) );
    m_parent->mCanvas->SetCursor( wxCursor(wxCURSOR_WAIT) );

    wxLogMessage( "in SnakesDialog::OnProcessAll" );
    timestamp();    mLogfile << " OnProcessAll" << endl << flush;

    for (bool foundOne=true; foundOne; ) {
        foundOne = false;
        for (int z=0; z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
            //does this slice need to be processed?
            if (m_processed_slice[z])    continue;
            //this slice needs to be processed.
            // has the previous slice been processed?
            if (z>0 && m_processed_slice[z-1]) {
                //reload mPoints with contents of previous slice but
                // for this slice
                assert( m_processed_points[z-1].size()>0 );
                mPoints.clear();
                for (int i=0; i<(int)m_processed_points[z-1].size(); i++) {
                    p3d<double>*  p = new p3d<double>(
                        m_processed_points[z-1][i]->x,
                        m_processed_points[z-1][i]->y,
                        m_processed_points[z-1][i]->z+1,
                        m_processed_points[z-1][i]->flags );
                    mPoints.push_back( p );
                }
                OnUpdate( e );
                foundOne = true;
                ++z;
            } else if (z<m_parent->mCanvas->mCavassData->m_zSize-1 &&
                       m_processed_slice[z+1]) {
                //reload mPoints with contents of next slice but
                // for this slice
                assert( m_processed_points[z+1].size()>0 );
                mPoints.clear();
                for (int i=0; i<(int)m_processed_points[z+1].size(); i++) {
                    p3d<double>*  p = new p3d<double>(
                        m_processed_points[z+1][i]->x,
                        m_processed_points[z+1][i]->y,
                        m_processed_points[z+1][i]->z-1,
                        m_processed_points[z+1][i]->flags );
                    mPoints.push_back( p );
                }
                OnUpdate( e );
                foundOne = true;
            }
        }
    }

#if 0
    //anything currently loaded?
    if (mPoints.size()==0) {
        //no, so try and reload something
        bool  foundSomething = false;
        int  z=-1;
        for (z=m_parent->mCanvas->getSliceNo(); z<m_parent->mCanvas->mCavassData->m_zSize; z++) {
            if (m_processed_points[z].size()>0) {
                foundSomething = true;
                break;
            }
        }
        if (!foundSomething) {
            for (z=m_parent->mCanvas->getSliceNo()-1; z>=0; z--) {
                if (m_processed_points[z].size()>0) {
                    foundSomething = true;
                    break;
                }
            }
        }
        if (!foundSomething)    return;  //nothing we can do!
        //load the start
        for (int i=0; i<(int)m_processed_points[z].size(); i++) {
            p3d<double>*  p = new p3d<double>( m_processed_points[z][i]->x,
                m_processed_points[z][i]->y, m_processed_points[z][i]->z,
                m_processed_points[z][i]->flags );
            mPoints.push_back( p );
        }
    }
    //has the start been updated?
    if (!m_processed_slice[(int)(mPoints[0]->z+0.5)]) {
        wxLogMessage( "    processing %f", mPoints[0]->z );
        OnUpdate(e);
    }
    //remember where we started
    const int  startZ = (int)(mPoints[0]->z+0.5);
    //process from start to end
    while ((int)(mPoints[0]->z+0.5) < m_parent->mCanvas->mCavassData->m_zSize-1) {
        OnNext(e);
        if (!m_processed_slice[(int)(mPoints[0]->z+0.5)]) {
            wxLogMessage( "    processing %f", mPoints[0]->z );
            OnUpdate(e);
        } else {
            wxLogMessage( "    skipping %f.  already processed.", mPoints[0]->z );
        }
    }

    //get ready to process from start to the beginning (0) by reloading start.
    mPoints.clear();
    for (int i=0; i<(int)m_processed_points[startZ].size(); i++) {
        p3d<double>*  p = new p3d<double>( m_processed_points[startZ][i]->x,
            m_processed_points[startZ][i]->y, m_processed_points[startZ][i]->z,
            m_processed_points[startZ][i]->flags );
        mPoints.push_back( p );
    }
    //work backwards from the start to the beginning (0)
    while ((int)(mPoints[0]->z+0.5) > 0) {
        OnPrevious(e);
        if (!m_processed_slice[(int)(mPoints[0]->z+0.5)]) {
            wxLogMessage( "    processing %f", mPoints[0]->z );
            OnUpdate(e);
        } else {
            wxLogMessage( "    skipping %f.  already processed.", mPoints[0]->z );
        }
    }
#endif
    //all slices have now been processed (segmented).
    MontageFrame*  f = new MontageFrame();
    f->loadData( "segmented via snakes", m_parent->mCanvas->mCavassData->m_xSize,
                 m_parent->mCanvas->mCavassData->m_ySize, m_parent->mCanvas->mCavassData->m_zSize,
                 m_parent->mCanvas->mCavassData->m_xSpacing, m_parent->mCanvas->mCavassData->m_ySpacing, 
                 m_parent->mCanvas->mCavassData->m_zSpacing, m_processed_data,
                 &m_parent->mCanvas->mCavassData->m_vh, m_parent->mCanvas->mCavassData->m_vh_initialized );

    m_update->Enable( true );
    m_process_all->Enable( true );
    SetCursor( *wxSTANDARD_CURSOR );
    m_parent->mCanvas->SetCursor( *wxSTANDARD_CURSOR );

    wxLogMessage( "out SnakesDialog::OnProcessAll" );
}
//----------------------------------------------------------------------
double SnakesDialog::getLocalMaxContinuity ( const int i, const double dBar )
const {
    assert( i>=0 && i<=(int)mPoints.size()-1 );
    assert( mPoints.size()>=2 );

    //wrap around to the last one (if necessary)
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        //determine the max in the neighborhood
        double  max = dBar - mag( *mPoints[0] - *mPoints[mPoints.size()-1] );
        for (int dy=-m_radius; dy<=m_radius; dy++) {
            for (int dx=-m_radius; dx<=m_radius; dx++) {
                p3d<double>*  tmp = new p3d<double>( mPoints[0]->x+dx,
                    mPoints[0]->y+dy, mPoints[0]->z );
                const double  v = dBar - mag( *tmp - *mPoints[mPoints.size()-1] );
                delete tmp;    tmp=NULL;
                if (v>max)    max=v;
            }
        }
        return max;
    }

    //determine the max in the neighborhood
    double  max = dBar - mag( *mPoints[i] - *mPoints[i-1] );
    for (int dy=-m_radius; dy<=m_radius; dy++) {
        for (int dx=-m_radius; dx<=m_radius; dx++) {
            p3d<double>*  tmp = new p3d<double>( mPoints[i]->x+dx, mPoints[i]->y+dy,
                mPoints[i]->z );
            const double  v = dBar - mag( *tmp - *mPoints[i-1] );
            delete tmp;    tmp=NULL;
            if (v>max)    max=v;
        }
    }
    return max;
}
//----------------------------------------------------------------------
double SnakesDialog::getLocalMaxCurvature2 ( const int i ) const
{
    //note: this function returns the max curvature _squared_.
    assert( i>=0 && i<=(int)mPoints.size()-1 );
    assert( mPoints.size()>=3 );

    //wrap around if necessary
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        //determine the max in the neighborhood
        double  max = mag( *mPoints[mPoints.size()-1] - 2.0 * *mPoints[0] + *mPoints[1] );
        max *= max;    //square it
        for (int dy=-m_radius; dy<=m_radius; dy++) {
            for (int dx=-m_radius; dx<=m_radius; dx++) {
                p3d<double>*  tmp = new p3d<double>( mPoints[0]->x+dx, mPoints[0]->y+dy,
                    mPoints[0]->z );
                double  v = mag( *mPoints[mPoints.size()-1] - 2.0 * *tmp + *mPoints[1] );
                v *= v;  //square it
                delete tmp;    tmp=NULL;
                if (v>max)    max=v;
            }
        }
        return max;
    }

    if (i==(int)mPoints.size()-1) {
        if (!mCompleteContour)    return 0.0;
        //determine the max in the neighborhood
        double  max = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[0] );
        max *= max;    //square it
        for (int dy=-m_radius; dy<=m_radius; dy++) {
            for (int dx=-m_radius; dx<=m_radius; dx++) {
                p3d<double>*  tmp = new p3d<double>( mPoints[i]->x+dx, mPoints[i]->y+dy,
                    mPoints[i]->z );
                double  v = mag( *mPoints[i-1] - 2.0 * *tmp + *mPoints[0] );
                v *= v;  //square it
                delete tmp;    tmp=NULL;
                if (v>max)    max=v;
            }
        }
        return max;
    }

    //determine the max in the neighborhood
    double  max = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[i+1] );
    max *= max;    //square it
    for (int dy=-m_radius; dy<=m_radius; dy++) {
        for (int dx=-m_radius; dx<=m_radius; dx++) {
            p3d<double>*  tmp = new p3d<double>( mPoints[i]->x+dx, mPoints[i]->y+dy,
                mPoints[i]->z );
            double  v = mag( *mPoints[i-1] - 2.0 * *tmp + *mPoints[i+1] );
            v *= v;  //square it
            delete tmp;    tmp=NULL;
            if (v>max)    max=v;
        }
    }
    return max;
}
//----------------------------------------------------------------------
void SnakesDialog::getLocalMinMaxGradientMag ( const int i, double& min,
    double& max ) const
{
    const int  cx=(int)(mPoints[i]->x + 0.5);
    const int  cy=(int)(mPoints[i]->y + 0.5);
    const int  cz=(int)(mPoints[i]->z + 0.5);
    min = max = mag( gradient2D(cx, cy, cz) );
    for (int dy=-m_radius; dy<=m_radius; dy++) {
        for (int dx=-m_radius; dx<=m_radius; dx++) {
            const double  m = mag( gradient2D(cx+dx, cy+dy, cz) );
            if (m<min)    min=m;
            if (m>max)    max=m;
        }
    }
    //from p. 19 of Williams and Shah
    if ((max-min)<5)    min = max-5;
}
//----------------------------------------------------------------------
//the following versions are called when we are summing up the individual
// costs along a path.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localE ( const int i, const double dBar ) const
{
    return m_tensile  * localEContinuity( i, dBar )
         + m_flexural * localECurvature( i ) - m_external * localEImage( i );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localEContinuity ( const int i, const double dBar ) const
{
    //determine the max in the neighborhood
    assert( mPoints.size()>=2 );
    const double  max = getLocalMaxContinuity( i, dBar );
    if (max==0.0)    return 0.0;
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        return (dBar - mag( *mPoints[0] - *mPoints[mPoints.size()-1] )) / max;
    }
    return (dBar - mag( *mPoints[i] - *mPoints[i-1] )) / max;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localECurvature ( const int i ) const
{
    //determine the max in the neighborhood
    assert( mPoints.size()>=3 );
    const double  max2 = getLocalMaxCurvature2( i );
    if (max2==0.0)    return 0.0;
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        const double  curvature = mag( *mPoints[mPoints.size()-1] - 2.0 * *mPoints[i] + *mPoints[i+1] );
        return curvature*curvature/max2;
    }
    if (i==(int)mPoints.size()-1) {
        if (!mCompleteContour)    return 0.0;
        const double  curvature = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[0] );
        return curvature*curvature/max2;
    }
    const double  curvature = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[i+1] );
    return curvature*curvature/max2;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localEImage ( const int i ) const
{
    double  min=0.0, max=0.0;
    getLocalMinMaxGradientMag( i, min, max );
    if (min==max)    return 0.0;
    assert( min<max );
    const double  centerMag = mag( gradient2D( mPoints[i]->x, mPoints[i]->y,
                                               mPoints[i]->z ) );
    return (centerMag-min) / (max-min);
}
//----------------------------------------------------------------------
//the following versions are called when we are evaluating possible changes
// along a path.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localE ( const int i, const double dBar,
    const double maxContinuity, const double maxCurvature,
    const double minGradMag, const double maxGradMag ) const
{
    return m_tensile  * localEContinuity( i, dBar, maxContinuity )
        +  m_flexural * localECurvature ( i, maxCurvature  )
        -  m_external * localEImage     ( i, minGradMag, maxGradMag );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localEContinuity ( const int i, const double dBar,
    const double maxContinuity ) const
{
    assert( mPoints.size()>=2 );
    if (maxContinuity==0.0)    return 0.0;
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        return (dBar - mag( *mPoints[i] - *mPoints[mPoints.size()-1] )) / maxContinuity;
    }
    return (dBar - mag( *mPoints[i] - *mPoints[i-1] )) / maxContinuity;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localECurvature ( const int i, 
    const double maxCurvature2 ) const
{
    assert( mPoints.size()>=3 );
    if (maxCurvature2==0.0)    return 0.0;
    if (i==0) {
        if (!mCompleteContour)    return 0.0;
        const double  curvature = mag( *mPoints[mPoints.size()-1] - 2.0 * *mPoints[i] + *mPoints[i+1] );
        return curvature*curvature/maxCurvature2;
    }
    if (i==(int)mPoints.size()-1) {
        if (!mCompleteContour)    return 0.0;
        const double  curvature = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[0] );
        return curvature*curvature/maxCurvature2;
    }
    const double  curvature = mag( *mPoints[i-1] - 2.0 * *mPoints[i] + *mPoints[i+1] );
    return curvature*curvature/maxCurvature2;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SnakesDialog::localEImage ( const int i, const double min,
    const double max ) const
{
    if (min==max)    return 0.0;
    assert( min<max );
    const double  centerMag = mag( gradient2D( mPoints[i]->x, mPoints[i]->y, mPoints[i]->z ) );
    return (centerMag-min) / (max-min);
}
//----------------------------------------------------------------------
double SnakesDialog::getDBar ( void ) const {
    if (mPoints.size()==0)    return 0.0;
    double  sum=0.0;
    for (int i=1; i<(int)mPoints.size(); i++) {
        sum += mag( *mPoints[i] - *mPoints[i-1] );
    }
    //note:  it's size-1 because n points have n-1 line segments between them.
    return sum / (mPoints.size()-1.0);
}
//----------------------------------------------------------------------
void SnakesDialog::updateContour ( void ) {
    //based on p. 20 of D.J. Williams and M. Shah, "A fast algorithm for
    // active contours and curvature estimation," CVGIP:IU 55(1):14-26, 1992.
    const double  dBar = getDBar();
    int  i;

    double  before = 0.0;
    for (i=0; i<(int)mPoints.size(); i++) {
        const double  e = localE(i, dBar);
        before += e;
        // cout << "    i=" << i << " localE=" << e << endl;
    }
    wxLogMessage( "energy before=%.4f", before );
    cout << "energy before=" << before << endl;
    // for (i=0; i<(int)mPoints.size(); i++)    cout << "    " << mPoints[i];
    // cout << endl;

    int           pointsMoved = 0;
    for (i=0; i<(int)mPoints.size(); i++) {
        if (mPoints[i]->flags & Fixed)    continue;
        const double  localMaxContinuity = getLocalMaxContinuity( i, dBar );
        const double  localMaxCurvature2  = getLocalMaxCurvature2( i );
        double  minGradMag=0.0, maxGradMag=0.0;
        getLocalMinMaxGradientMag( i, minGradMag, maxGradMag );
        const p3d<double>  copy = *mPoints[i];
        p3d<double>  bestMove;
        double  bestE = localE(i, dBar, localMaxContinuity,
            localMaxCurvature2, minGradMag, maxGradMag);
        cout << "i=" << i << ": " << bestE << endl;
        bool  improved = false;
        for (int dy=-m_radius; dy<=m_radius; dy++) {
            for (int dx=-m_radius; dx<=m_radius; dx++) {
                if (dx==0 && dy==0)    continue;  //skip no change at all
                //todo???: allow corner points to change from FixedX to FixedY or vice versa
                if (dx!=0 && mPoints[i]->flags & FixedX)    continue;
                if (dy!=0 && mPoints[i]->flags & FixedY)    continue;
                const p3d<double>  d(dx,dy,0,copy.flags);
                *mPoints[i] = copy + d;
                //evaluate the change
                const double  tempE = localE(i, dBar,
                    localMaxContinuity, localMaxCurvature2, minGradMag, maxGradMag);
                if (tempE<bestE) {
                    bestE = tempE;
                    bestMove = d;
                    improved = true;
                    // cout << "    better: " << bestE << " " << &bestMove << endl;
                }
            }
        }

        if (improved) {
            *mPoints[i] = copy + bestMove;
            if (m_parent->mCanvas->mCavassData->inBounds( (int)(mPoints[i]->x), (int)(mPoints[i]->y),
                                    (int)(mPoints[i]->z) )) {
                ++pointsMoved;
            } else {
                *mPoints[i] = copy;
            }
        } else {
            *mPoints[i] = copy;
        }
    }

    double  after = 0.0;
    for (i=0; i<(int)mPoints.size(); i++) {
        const double  e = localE(i, dBar);
        after += e;    //after += localE(i, dBar);
        // cout << "    i=" << i << " localE=" << e << endl;
    }
    wxLogMessage( "energy after=%.4f", after );
    cout << "energy after=" << after << endl;
    // for (i=0; i<(int)mPoints.size(); i++)    cout << "    " << mPoints[i];
    // cout << endl;

    wxLogMessage( "updateContour: %d updated.", pointsMoved );
}
//----------------------------------------------------------------------
void SnakesDialog::timestamp ( void ) {
    //write state information and time to log file
    const time_t  t = time(NULL);
    mLogfile << "time=" << ctime( &t )
             << " userpoints="     << m_userpoints
             << " discretization=" << m_discretization
             << " tensile="        << m_tensile
             << " flexural="       << m_flexural
             << " external="       << m_external
             << " inflation="      << m_inflation
             << " threshold="      << m_threshold
             << " iterations="     << m_iterations
             << " radius="         << m_radius
             << flush;
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( SnakesDialog, wxDialog )
BEGIN_EVENT_TABLE       ( SnakesDialog, wxDialog )

  EVT_COMMAND_SCROLL( ID_USERPOINTS, SnakesDialog::OnUserpointsSlider )
  EVT_TEXT( ID_USERPOINTS_TEXT, SnakesDialog::OnUserpointsTextEnter )
  EVT_TEXT_ENTER( ID_USERPOINTS_TEXT, SnakesDialog::OnUserpointsTextEnter )

  EVT_COMMAND_SCROLL( ID_DISCRETIZATION, SnakesDialog::OnDiscretizationSlider )
  EVT_TEXT( ID_DISCRETIZATION_TEXT, SnakesDialog::OnDiscretizationTextEnter )
  EVT_TEXT_ENTER( ID_DISCRETIZATION_TEXT, SnakesDialog::OnDiscretizationTextEnter )

  EVT_COMMAND_SCROLL( ID_TENSILE, SnakesDialog::OnTensileSlider    )
  EVT_TEXT( ID_TENSILE_TEXT, SnakesDialog::OnTensileTextEnter )
  EVT_TEXT_ENTER( ID_TENSILE_TEXT, SnakesDialog::OnTensileTextEnter )

  EVT_COMMAND_SCROLL( ID_FLEXURAL, SnakesDialog::OnFlexuralSlider )
  EVT_TEXT( ID_FLEXURAL_TEXT, SnakesDialog::OnFlexuralTextEnter )
  EVT_TEXT_ENTER( ID_FLEXURAL_TEXT, SnakesDialog::OnFlexuralTextEnter )

  EVT_COMMAND_SCROLL( ID_EXTERNAL, SnakesDialog::OnExternalSlider )
  EVT_TEXT( ID_EXTERNAL_TEXT, SnakesDialog::OnExternalTextEnter )
  EVT_TEXT_ENTER( ID_EXTERNAL_TEXT, SnakesDialog::OnExternalTextEnter )

  EVT_COMMAND_SCROLL( ID_INFLATION, SnakesDialog::OnInflationSlider )
  EVT_TEXT( ID_INFLATION_TEXT, SnakesDialog::OnInflationTextEnter )
  EVT_TEXT_ENTER( ID_INFLATION_TEXT, SnakesDialog::OnInflationTextEnter )

  EVT_COMMAND_SCROLL( ID_THRESHOLD, SnakesDialog::OnThresholdSlider )
  EVT_TEXT( ID_THRESHOLD_TEXT, SnakesDialog::OnThresholdTextEnter )
  EVT_TEXT_ENTER( ID_THRESHOLD_TEXT, SnakesDialog::OnThresholdTextEnter )

  EVT_COMMAND_SCROLL( ID_ITERATIONS, SnakesDialog::OnIterationsSlider )
  EVT_TEXT( ID_ITERATIONS_TEXT, SnakesDialog::OnIterationsTextEnter )
  EVT_TEXT_ENTER( ID_ITERATIONS_TEXT, SnakesDialog::OnIterationsTextEnter )

  EVT_COMMAND_SCROLL( ID_RADIUS,      SnakesDialog::OnRadiusSlider )
  EVT_TEXT(           ID_RADIUS_TEXT, SnakesDialog::OnRadiusTextEnter )
  EVT_TEXT_ENTER(     ID_RADIUS_TEXT, SnakesDialog::OnRadiusTextEnter )
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_USERPOINTS,  SnakesDialog::OnUserpointsSliderUpdateUI )
  EVT_UPDATE_UI( ID_DISCRETIZATION, SnakesDialog::OnDiscretizationSliderUpdateUI )
  EVT_UPDATE_UI( ID_TENSILE,     SnakesDialog::OnTensileSliderUpdateUI )
  EVT_UPDATE_UI( ID_FLEXURAL,    SnakesDialog::OnFlexuralSliderUpdateUI )
  EVT_UPDATE_UI( ID_EXTERNAL,    SnakesDialog::OnExternalSliderUpdateUI )
  EVT_UPDATE_UI( ID_INFLATION,   SnakesDialog::OnInflationSliderUpdateUI )
  EVT_UPDATE_UI( ID_THRESHOLD,   SnakesDialog::OnThresholdSliderUpdateUI )
  EVT_UPDATE_UI( ID_ITERATIONS,  SnakesDialog::OnIterationsSliderUpdateUI )
  EVT_UPDATE_UI( ID_RADIUS,      SnakesDialog::OnRadiusSliderUpdateUI )
#endif
  EVT_CHECKBOX( ID_SHOW_NORMALS, SnakesDialog::OnShowNormals )

  EVT_BUTTON( ID_UPDATE,      SnakesDialog::OnUpdate     )
  EVT_BUTTON( ID_PROCESS_ALL, SnakesDialog::OnProcessAll )
  EVT_BUTTON( ID_CLEAR,       SnakesDialog::OnClear      )
  EVT_BUTTON( ID_NEXT,        SnakesDialog::OnNext       )
  EVT_BUTTON( ID_PREVIOUS,    SnakesDialog::OnPrevious   )
  EVT_BUTTON( ID_DEBUG,       SnakesDialog::OnDebug      )
  EVT_BUTTON( ID_SAVE,        SnakesDialog::OnSave       )
  EVT_BUTTON( ID_SAVE_ALL,    SnakesDialog::OnSaveAll    )
  EVT_BUTTON( ID_LOAD,        SnakesDialog::OnLoad       )

END_EVENT_TABLE()
//======================================================================

