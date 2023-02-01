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
// TrapezoidTransform
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
#include  "Dicom.h"

#include  "math.h"
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif
#include  <stdlib.h>

#include  <Viewnix.h>
#include  "cv3dv.h"

#include  "wx/wx.h"
#include  "wx/file.h"
#include  "wx/progdlg.h"
#include  "MainCanvas.h"
#include  "MontageFrame.h"
#include  "Globals.h"
#endif

#include  "TrapezoidTransform.h"
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
void TrapezoidTransform::init ( void ) {
    m_parent        = NULL;

    mTransform      = NULL;

    m_slope              = 0;
    m_slope_st           = NULL;
    m_slope_text         = NULL;
    m_slope_slider       = NULL;

    m_yintercept         = 0;
    m_yintercept_st      = NULL;
    m_yintercept_text    = NULL;
    m_yintercept_slider  = NULL;

    m_slope2             = 0;
    m_slope_st2          = NULL;
    m_slope_text2        = NULL;
    m_slope_slider2      = NULL;

    m_yintercept2        = 0;
    m_yintercept_st2     = NULL;
    m_yintercept_text2   = NULL;
    m_yintercept_slider2 = NULL;

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
TrapezoidTransform::TrapezoidTransform ( MontageFrame* parent )
  : GrayScaleTransform ( parent, -1, "Trapezoid grayscale transform options",
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
    m_slope_slider = new wxSlider( mainPanel, ID_SLOPE, 300,
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
    m_slope_st2 = new wxStaticText( mainPanel, -1, "slope2:" );
    gs1->Add( m_slope_st2, 1, wxALIGN_RIGHT );
    m_slope_text2 = new wxTextCtrl( mainPanel, ID_SLOPE_TEXT2, "5000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_slope_text2 );
    gs1->Add( m_slope_text2, 1, wxALIGN_RIGHT );
    m_slope_slider2 = new wxSlider( mainPanel, ID_SLOPE2, -300,
        -sMaxSlopeValue, sMaxSlopeValue,
        wxDefaultPosition, wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "slope2" );
    m_slope_slider2->SetPageSize(5);
    ::setColor( m_slope_slider2 );
    gs1->Add( m_slope_slider2, 0, wxALIGN_CENTER );

    m_yintercept_st2 = new wxStaticText( mainPanel, -1, "y intercept2:" );
    gs1->Add( m_yintercept_st2, 1, wxALIGN_RIGHT );
    m_yintercept_text2 = new wxTextCtrl( mainPanel, ID_YINTERCEPT_TEXT2, "1000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_yintercept_text2 );
    gs1->Add( m_yintercept_text2, 1, wxALIGN_RIGHT );
    m_yintercept_slider2 = new wxSlider( mainPanel, ID_YINTERCEPT2, 0,
        -sMaxYInterceptValue, sMaxYInterceptValue, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "yintercept2" );
    m_yintercept_slider2->SetPageSize(5);
    ::setColor( m_yintercept_slider2 );
    gs1->Add( m_yintercept_slider2, 0, wxALIGN_CENTER );
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
void TrapezoidTransform::doTransform ( void ) {
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
        mTransform[i-min] = (int)(trapezoid(i,max,min)*(max-min)+min+0.5);
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
void TrapezoidTransform::setupSliders ( void ) {
    char  buff[255];

    m_slope = ((double)m_slope_slider->GetValue()) / sSliderDivisor;
    int ret = snprintf( buff, sizeof buff, "%f", m_slope );
    assert( ret < sizeof buff );
    m_slope_text->SetValue( buff );

    m_yintercept = ((double)m_yintercept_slider->GetValue()) / sSliderDivisor;
    ret = snprintf( buff, sizeof buff, "%f", m_yintercept );
    assert( ret < sizeof buff );
    m_yintercept_text->SetValue( buff );

    m_slope2 = ((double)m_slope_slider2->GetValue()) / sSliderDivisor;
    ret = snprintf( buff, sizeof buff, "%f", m_slope2 );
    assert( ret < sizeof buff );
    m_slope_text2->SetValue( buff );

    m_yintercept2 = ((double)m_yintercept_slider2->GetValue()) / sSliderDivisor;
    ret = snprintf( buff, sizeof buff, "%f", m_yintercept2 );
    assert( ret < sizeof buff );
    m_yintercept_text2->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_slope = ((double)m_slope_slider->GetValue()) / sSliderDivisor;
    int ret = snprintf( buff, sizeof buff, "%f", m_slope );
    assert( ret < sizeof buff );
    m_slope_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptSlider ( wxScrollEvent& e ) {
    char  buff[255];
    m_yintercept = ((double)m_yintercept_slider->GetValue()) / sSliderDivisor;
    int ret = snprintf( buff, sizeof buff, "%f", m_yintercept );
    assert( ret < sizeof buff );
    m_yintercept_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "TrapezoidTransform::OnSlopeTextChanged" );
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "TrapezoidTransform::OnYInterceptTextChanged" );
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_slope_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_slope = d;
    const int  v   = (int)(m_slope*sSliderDivisor + 0.5);
    m_slope_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_yintercept_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_yintercept = d;
    const int  v = (int)(m_yintercept*sSliderDivisor + 0.5);
    m_yintercept_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeSlider2 ( wxScrollEvent& e ) {
    char  buff[255];
    m_slope2 = ((double)m_slope_slider2->GetValue()) / sSliderDivisor;
    int ret = snprintf( buff, sizeof buff, "%f", m_slope2 );
    assert( ret < sizeof buff );
    m_slope_text2->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptSlider2 ( wxScrollEvent& e ) {
    char  buff[255];
    m_yintercept2 = ((double)m_yintercept_slider2->GetValue()) / sSliderDivisor;
    int ret = snprintf( buff, sizeof buff, "%f", m_yintercept2 );
    assert( ret < sizeof buff );
    m_yintercept_text2->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeTextChanged2 ( wxCommandEvent& e ) {
    wxLogMessage( "TrapezoidTransform::OnSlopeTextChanged2" );
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptTextChanged2 ( wxCommandEvent& e ) {
    wxLogMessage( "TrapezoidTransform::OnYInterceptTextChanged2" );
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnSlopeTextEnter2 ( wxCommandEvent& e ) {
    const wxString     wxs = m_slope_text2->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_slope2 = d;
    const int  v   = (int)(m_slope2*sSliderDivisor + 0.5);
    m_slope_slider2->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void TrapezoidTransform::OnYInterceptTextEnter2 ( wxCommandEvent& e ) {
    const wxString     wxs = m_yintercept_text2->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    m_yintercept2 = d;
    const int  v = (int)(m_yintercept2*sSliderDivisor + 0.5);
    m_yintercept_slider2->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( TrapezoidTransform, GrayScaleTransform )
BEGIN_EVENT_TABLE       ( TrapezoidTransform, GrayScaleTransform )

    EVT_COMMAND_SCROLL( ID_SLOPE,      TrapezoidTransform::OnSlopeSlider )
    EVT_TEXT(           ID_SLOPE_TEXT, TrapezoidTransform::OnSlopeTextChanged )
    EVT_TEXT_ENTER(     ID_SLOPE_TEXT, TrapezoidTransform::OnSlopeTextEnter )

    EVT_COMMAND_SCROLL( ID_YINTERCEPT,      TrapezoidTransform::OnYInterceptSlider )
    EVT_TEXT(           ID_YINTERCEPT_TEXT, TrapezoidTransform::OnYInterceptTextChanged )
    EVT_TEXT_ENTER(     ID_YINTERCEPT_TEXT, TrapezoidTransform::OnYInterceptTextEnter )

    EVT_COMMAND_SCROLL( ID_SLOPE2,      TrapezoidTransform::OnSlopeSlider2 )
    EVT_TEXT(           ID_SLOPE_TEXT2, TrapezoidTransform::OnSlopeTextChanged2 )
    EVT_TEXT_ENTER(     ID_SLOPE_TEXT2, TrapezoidTransform::OnSlopeTextEnter2 )

    EVT_COMMAND_SCROLL( ID_YINTERCEPT2,      TrapezoidTransform::OnYInterceptSlider2 )
    EVT_TEXT(           ID_YINTERCEPT_TEXT2, TrapezoidTransform::OnYInterceptTextChanged2 )
    EVT_TEXT_ENTER(     ID_YINTERCEPT_TEXT2, TrapezoidTransform::OnYInterceptTextEnter2 )

END_EVENT_TABLE()
//======================================================================
