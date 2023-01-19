/*
  Copyright 1993-2011 Medical Image Processing Group
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
// RampTransform
//----------------------------------------------------------------------
#include  "cavass.h"

#if 0
#ifdef WIN32
    #include  <iostream.h>
    #include  <iomanip.h>
#else
    #include  <iostream>
    #include  <iomanip>
#endif

#include  <assert.h>
#include  <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif
#include  <stdlib.h>
#include  <Viewnix.h>
#include  "cv3dv.h"
#include  "Dicom.h"

#include  "wx/wx.h"
#include  "wx/file.h"
#include  "wx/progdlg.h"
#include  "MainCanvas.h"
#include  "MontageFrame.h"
#include  "Globals.h"
#endif

#include  "RampTransform.h"
using namespace std;
//----------------------------------------------------------------------
static const int  sSliderDivisor      =    100;
static const int  sWidth              =    100;
static const int  sHeight             =    100;
static const int  sMaxSlopeValue      =  10000;
#if defined (WIN32) || defined (_WIN32)
    static const int  sMaxYInterceptValue = 32000;  //stupid windoze!
#else
    static const int  sMaxYInterceptValue = 100000;
#endif
//----------------------------------------------------------------------
void RampTransform::init ( void ) {
    m_parent        = NULL;

    mTransform      = NULL;

    m_slope          = 0;
    m_slope_st       = NULL;
    m_slope_text     = NULL;
    m_slope_slider   = NULL;

    m_yintercept        = 0;
    m_yintercept_st     = NULL;
    m_yintercept_text   = NULL;
    m_yintercept_slider = NULL;

    m_ok            = NULL;
    m_flash         = NULL;
    m_invert        = NULL;
    m_cancel        = NULL;

    mGraph          = NULL;
    mImage          = NULL;
    mBitmap         = NULL;
    mSbm            = NULL;

    mInvert         = false;
    mLUTCopy        = NULL;
}
//----------------------------------------------------------------------
RampTransform::RampTransform ( MontageFrame* parent )
  : GrayScaleTransform ( parent, -1, "Ramp grayscale transform options",
                         wxDefaultPosition, wxDefaultSize )
{
    init();
//    const int  sliderWidth = 200;
    m_parent = parent;
    if (m_parent!=NULL && m_parent->mCanvas!=NULL) {
        const int  min = m_parent->mCanvas->mCavassData->m_min;
        const int  max = m_parent->mCanvas->mCavassData->m_max;
        mLUTCopy = (unsigned char*)malloc( (max-min+1)*sizeof(unsigned char) );
        assert( mLUTCopy != NULL );
        for (int i=0; i<(max-min+1); i++) {
            mLUTCopy[i] = m_parent->mCanvas->mCavassData->m_lut[i];
        }
    }

    wxPanel*  mainPanel = new wxPanel( this, -1 );
    ::setColor( this );
    ::setColor( mainPanel );

    wxFlexGridSizer*  mainSizer = new wxFlexGridSizer(1, 5, 5);
    // - - - - - - - - - -
    mainSizer->Add(20, 20);
    // - - - - - - - - - -
    wxSizer*  gs1 = new wxFlexGridSizer(3, 10, 5);  //3 cols,vgap,hgap
    mainSizer->Add( gs1, 1, wxALIGN_CENTER );

    m_slope_st = new wxStaticText( mainPanel, -1, "slope:" );
    gs1->Add( m_slope_st, 1, wxALIGN_RIGHT );
    m_slope_text = new wxTextCtrl( mainPanel, ID_SLOPE_TEXT, "5000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_slope_text );
    gs1->Add( m_slope_text, 1, wxALIGN_RIGHT );
    m_slope_slider = new wxSlider( mainPanel, ID_SLOPE, 100,
        -sMaxSlopeValue, sMaxSlopeValue,
        wxDefaultPosition, wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "slope" );
    m_slope_slider->SetPageSize(5);
    ::setColor( m_slope_slider );
    gs1->Add( m_slope_slider, 0, wxALIGN_CENTER );

    m_yintercept_st = new wxStaticText( mainPanel, -1, "y intercept:" );
    gs1->Add( m_yintercept_st, 1, wxALIGN_RIGHT );
    m_yintercept_text = new wxTextCtrl( mainPanel, ID_YINTERCEPT_TEXT, "1000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_yintercept_text );
    gs1->Add( m_yintercept_text, 1, wxALIGN_RIGHT );
    m_yintercept_slider = new wxSlider( mainPanel, ID_YINTERCEPT, 0,
        -sMaxYInterceptValue, sMaxYInterceptValue, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "yintercept" );
    m_yintercept_slider->SetPageSize(5);
    ::setColor( m_yintercept_slider );
    gs1->Add( m_yintercept_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    mainSizer->Add(20, 20);
    // - - - - - - - - - -
    mGraph = (unsigned char*)malloc( sWidth*sHeight*3 );    //never free this!
    assert( mGraph!=NULL );
    for (int i=0; i<sWidth*sHeight*3; i+=3) {
        mGraph[i]   = 100;
        mGraph[i+1] = mGraph[i+2] = 0;
    }
    mImage  = new wxImage( sWidth, sHeight, mGraph );
    mBitmap = new wxBitmap( (const wxImage&) *mImage );
    mSbm    = new wxStaticBitmap( mainPanel, -1, *mBitmap, wxDefaultPosition,
                                  wxSize(sWidth,sHeight) );
    mainSizer->Add( mSbm, 1, wxALIGN_CENTER );
    // - - - - - - - - - -
    mainSizer->Add(20, 20);
    // - - - - - - - - - -
    wxSizer*  gs3 = new wxGridSizer(4, 20, 20);  //cols,vgap,hgap
    gs3->SetMinSize( 2*sliderWidth, 50 );
    mainSizer->Add( gs3, 1, wxALIGN_CENTER );

    m_ok = new wxButton( mainPanel, wxID_OK, "Save" );
    gs3->Add( m_ok, 0, wxALIGN_RIGHT );
    ::setColor( m_ok );

    m_flash = new wxButton( mainPanel, ID_FLASH, "Original" );
    gs3->Add( m_flash, 0, wxALIGN_LEFT );
    ::setColor( m_flash );

    m_invert = new wxButton( mainPanel, ID_INVERT, "Invert" );
    gs3->Add( m_invert, 0, wxALIGN_LEFT );
    ::setColor( m_invert );

    m_cancel = new wxButton( mainPanel, wxID_CANCEL, "Dismiss" );
    gs3->Add( m_cancel, 0, wxALIGN_LEFT );
    ::setColor( m_cancel );
    // - - - - - - - - - -
    mainPanel->SetAutoLayout( true );
    mainPanel->SetSizer( mainSizer );
    mainSizer->SetSizeHints( this );
    setupSliders();
    Show();
}
//----------------------------------------------------------------------
void RampTransform::doTransform ( void ) {
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    if (mTransform==NULL) {
        mTransform = (int*)malloc( (max-min+1)*sizeof(int) );
        assert( mTransform!=NULL );
    }
    //FILE*  fp = fopen( "gst.txt", "wb" );    assert(fp!=NULL);
    //fprintf( fp, "# min = %d, max=%d\n", min, max );
    //cout << "min=" << min << ", max=" << max << ", slope=" << m_slope
    //     << ", y intercept=" << m_yintercept << endl;
    int  i;
    for (i=min; i<=max; i++) {
        mTransform[i-min] = (int)(ramp(i,max,min)*(max-min)+min+0.5);
        //fprintf( fp, "%d %d\n", i, mTransform[i-min] );
        //printf( "%d %d\n", i, mTransform[i-min] );
    }
    //fclose(fp);    fp=NULL;
    
    mGraph = (unsigned char*)malloc( sWidth*sHeight*3 );    //never free this!
    assert( mGraph!=NULL );
    for (i=0; i<sWidth*sHeight*3; i++)    mGraph[i] = 0;
    for (int x=min; x<=max; x++) {
        const int  xg = (int)((x-min)*99.0/(max-min)+0.5);
        const int  y  = mTransform[x-min];
        const int  yg = (sHeight-1) - (int)((y-min)*(sHeight-1.0)/(max-min)+0.5);
        assert( xg>=0 && xg<sWidth  );
        assert( yg>=0 && yg<sHeight );
        mGraph[3*(yg*sWidth+xg)] = 255;
    }
    if (mImage!=NULL)    delete mImage;
    mImage = new wxImage( sWidth, sHeight, mGraph );
    if (mBitmap!=NULL)    delete mBitmap;
    mBitmap = new wxBitmap( (const wxImage&) *mImage );
    mSbm->SetBitmap( *mBitmap );
    
    unsigned char*  newLUT = (unsigned char*)malloc(
        (max-min+1)*sizeof(unsigned char) );
    assert( newLUT!= NULL);
    for (i=min; i<=max; i++) {
        const int  newValue = mTransform[i-min];
        assert( newValue>=min && newValue<=max );
        newLUT[i-min] = mLUTCopy[newValue-min];
    }
    m_parent->mCanvas->mCavassData->initLUT( newLUT, max-min+1 );
    m_parent->mCanvas->reload();
    free( newLUT );       newLUT    = NULL;
}
//----------------------------------------------------------------------
void RampTransform::setupSliders ( void ) {
    char  buff[255];

    m_slope = ((double)m_slope_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_slope );
    m_slope_text->SetValue( buff );

    m_yintercept = ((double)m_yintercept_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_yintercept );
    m_yintercept_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void RampTransform::OnSlopeSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_slope = ((double)m_slope_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_slope );
    m_slope_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void RampTransform::OnYInterceptSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_yintercept = ((double)m_yintercept_slider->GetValue()) / sSliderDivisor;
    sprintf( buff, "%f", m_yintercept );
    m_yintercept_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void RampTransform::OnSlopeTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "RampTransform::OnSlopeTextChanged" );
}
//----------------------------------------------------------------------
void RampTransform::OnYInterceptTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "RampTransform::OnYInterceptTextChanged" );
}
//----------------------------------------------------------------------
void RampTransform::OnSlopeTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_slope_text->GetValue();
    const char* const  ptr = (const char *)wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_slope = d;
    const int  v   = (int)(m_slope*sSliderDivisor + 0.5);
    m_slope_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void RampTransform::OnYInterceptTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_yintercept_text->GetValue();
    const char* const  ptr = (const char *)wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_yintercept = d;
    const int  v = (int)(m_yintercept*sSliderDivisor + 0.5);
    m_yintercept_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( RampTransform, GrayScaleTransform )
BEGIN_EVENT_TABLE       ( RampTransform, GrayScaleTransform )
    EVT_COMMAND_SCROLL( ID_SLOPE,      RampTransform::OnSlopeSlider )
    EVT_TEXT(           ID_SLOPE_TEXT, RampTransform::OnSlopeTextChanged )
    EVT_TEXT_ENTER(     ID_SLOPE_TEXT, RampTransform::OnSlopeTextEnter )

    EVT_COMMAND_SCROLL( ID_YINTERCEPT,      RampTransform::OnYInterceptSlider )
    EVT_TEXT(           ID_YINTERCEPT_TEXT, RampTransform::OnYInterceptTextChanged )
    EVT_TEXT_ENTER(     ID_YINTERCEPT_TEXT, RampTransform::OnYInterceptTextEnter )
END_EVENT_TABLE()
//======================================================================
