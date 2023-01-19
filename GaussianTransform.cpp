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
// GaussianTransform
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
#include  "wx/filename.h"
#include  "wx/progdlg.h"

#include  "Globals.h"
#include  "MainCanvas.h"
#include  "MontageFrame.h"
#endif

#include  "GaussianTransform.h"
using namespace std;
//----------------------------------------------------------------------
const int     GaussianTransform::sMaxSliderValue = 10000;
const double  GaussianTransform::sScaleMin       =     0.000001;
const double  GaussianTransform::sScaleMax       =    10.0;
//----------------------------------------------------------------------
void GaussianTransform::init ( void ) {
    m_parent        = NULL;

    mTransform      = NULL;

    mMean           = 150;
    m_mean_st       = NULL;
    m_mean_text     = NULL;
    m_mean_slider   = NULL;

    mStddev         = 100;
    m_stddev_st     = NULL;
    m_stddev_text   = NULL;
    m_stddev_slider = NULL;

    mScale          = 1.0;
    m_scale_st      = NULL;
    m_scale_text    = NULL;
    m_scale_slider  = NULL;

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
GaussianTransform::GaussianTransform ( MontageFrame* parent )
    : GrayScaleTransform ( parent, -1, "Gaussian grayscale transform options",
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
        mMean   = (int)((max-min)/2.0+min);
        mStddev = (int)((max-min)/10.0);
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

    m_mean_st = new wxStaticText( mainPanel, -1, "mean:" );
    gs1->Add( m_mean_st, 1, wxALIGN_RIGHT );
    m_mean_text = new wxTextCtrl( mainPanel, ID_MEAN_TEXT, "5000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB );
    ::setColor( m_mean_text );
    gs1->Add( m_mean_text, 1, wxALIGN_RIGHT );
    m_mean_slider = new wxSlider( mainPanel, ID_MEAN, 5000, 0, sMaxSliderValue,
        wxDefaultPosition, wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "mean" );
    m_mean_slider->SetPageSize(5);
    ::setColor( m_mean_slider );
    gs1->Add( m_mean_slider, 0, wxALIGN_CENTER );

    m_stddev_st = new wxStaticText( mainPanel, -1, "stddev:" );
    gs1->Add( m_stddev_st, 1, wxALIGN_RIGHT );
    m_stddev_text = new wxTextCtrl( mainPanel, ID_STDDEV_TEXT, "1000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_stddev_text );
    gs1->Add( m_stddev_text, 1, wxALIGN_RIGHT );
    m_stddev_slider = new wxSlider( mainPanel, ID_STDDEV, 1000, 0,
        sMaxSliderValue, wxDefaultPosition, wxSize(sliderWidth,-1),
        wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, "stddev" );
    m_stddev_slider->SetPageSize(5);
    ::setColor( m_stddev_slider );
    gs1->Add( m_stddev_slider, 0, wxALIGN_CENTER );

    m_scale_st = new wxStaticText( mainPanel, -1, "scale:" );
    gs1->Add( m_scale_st, 1, wxALIGN_RIGHT );
    m_scale_text = new wxTextCtrl( mainPanel, ID_SCALE_TEXT, "1000",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    ::setColor( m_scale_text );
    gs1->Add( m_scale_text, 1, wxALIGN_RIGHT );
    m_scale_slider = new wxSlider( mainPanel, ID_SCALE, 1000, 0,
        sMaxSliderValue, wxDefaultPosition, wxSize(sliderWidth,-1),
        wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, "scale" );
    m_scale_slider->SetPageSize(5);
    ::setColor( m_scale_slider );
    gs1->Add( m_scale_slider, 0, wxALIGN_CENTER );
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
    setup();
    Show();
}
//----------------------------------------------------------------------
void GaussianTransform::doTransform ( void ) {
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    if (mTransform==NULL) {
        mTransform = (int*)malloc( (max-min+1)*sizeof(int) );
        assert( mTransform!=NULL );
    }
    //FILE*  fp = fopen( "gst.txt", "wb" );    assert(fp!=NULL);
    //fprintf( fp, "# min = %d, max=%d\n", min, max );
    int  i;
    for (i=min; i<=max; i++) {
        mTransform[i-min] = (int)(gauss(i)*(max-min)+min+0.5);
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
void GaussianTransform::setup ( void ) {
    char  buff[255];
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    mMean = ((double)m_mean_slider->GetValue())*(max-min) / sMaxSliderValue
            + min;
    sprintf( buff, "%f", mMean );
    m_mean_text->SetValue( buff );

    static const double  stddevMin = 0.000001;
    const double         stddevMax = 2.0*(max-min);
    mStddev = ((double)m_stddev_slider->GetValue())*(stddevMax-stddevMin)
              / sMaxSliderValue + stddevMin;
    sprintf( buff, "%f", mStddev );
    m_stddev_text->SetValue( buff );

    mScale = ((double)m_scale_slider->GetValue())*(sScaleMax-sScaleMin)
              / sMaxSliderValue + sScaleMin;
    sprintf( buff, "%f", mScale );
    m_scale_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnMeanSlider ( wxScrollEvent& e ) {
    char  buff[255];
    //gray min and max
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    mMean = ((double)m_mean_slider->GetValue())*(max-min) / sMaxSliderValue
            + min;
    sprintf( buff, "%f", mMean );
    m_mean_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnStddevSlider ( wxScrollEvent& e ) {
    char  buff[255];
    //gray min and max
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    static const double  stddevMin = 0.000001;
    const double         stddevMax = 2.0*(max-min);
    mStddev = ((double)m_stddev_slider->GetValue())*(stddevMax-stddevMin)
              / sMaxSliderValue + stddevMin;
    sprintf( buff, "%f", mStddev );
    m_stddev_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnScaleSlider ( wxScrollEvent& e ) {
    mScale = ((double)m_scale_slider->GetValue())*(sScaleMax-sScaleMin)
              / sMaxSliderValue + sScaleMin;
    char  buff[255];
    sprintf( buff, "%f", mScale );
    m_scale_text->SetValue( buff );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnMeanTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "GaussianTransform::OnMeanTextChanged" );
}
//----------------------------------------------------------------------
void GaussianTransform::OnStddevTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "GaussianTransform::OnStddevTextChanged" );
}
//----------------------------------------------------------------------
void GaussianTransform::OnScaleTextChanged ( wxCommandEvent& e ) {
    wxLogMessage( "GaussianTransform::OnScaleTextChanged" );
}
//----------------------------------------------------------------------
void GaussianTransform::OnMeanTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_mean_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    mMean = d;
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    const int  v   = (int)((mMean-min)*sMaxSliderValue / (max-min) + 0.5);
    m_mean_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnStddevTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_stddev_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    mStddev = d;
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    static const double  stddevMin = 0.000001;
    const double         stddevMax = 2.0*(max-min);
    const int  v = (int)((mStddev-stddevMin)*sMaxSliderValue
                         / (stddevMax-stddevMin) + 0.5);
    m_stddev_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnScaleTextEnter ( wxCommandEvent& e ) {
    const wxString     wxs = m_scale_text->GetValue();
    const char* const  ptr = wxs.c_str();
    char*              endptr;
    const double       d = strtod( ptr, &endptr );
    if (endptr == ptr)    return;  //ignore bad values

    mScale = d;
    const int  v = (int)((mScale-sScaleMin)*sMaxSliderValue
                         / (sScaleMax-sScaleMin) + 0.5);
    m_scale_slider->SetValue( v );

    doTransform();
}
//----------------------------------------------------------------------
void GaussianTransform::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "GaussianTransform::OnChar" );
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( GaussianTransform, GrayScaleTransform )
BEGIN_EVENT_TABLE       ( GaussianTransform, GrayScaleTransform )
    EVT_COMMAND_SCROLL( ID_MEAN,   GaussianTransform::OnMeanSlider )
    EVT_TEXT(       ID_MEAN_TEXT,  GaussianTransform::OnMeanTextChanged )
    EVT_TEXT_ENTER( ID_MEAN_TEXT,  GaussianTransform::OnMeanTextEnter )
    EVT_CHAR( GaussianTransform::OnChar )

    EVT_COMMAND_SCROLL( ID_STDDEV,  GaussianTransform::OnStddevSlider )
    EVT_TEXT(       ID_STDDEV_TEXT, GaussianTransform::OnStddevTextChanged )
    EVT_TEXT_ENTER( ID_STDDEV_TEXT, GaussianTransform::OnStddevTextEnter )

    EVT_COMMAND_SCROLL( ID_SCALE,  GaussianTransform::OnScaleSlider )
    EVT_TEXT(       ID_SCALE_TEXT, GaussianTransform::OnScaleTextChanged )
    EVT_TEXT_ENTER( ID_SCALE_TEXT, GaussianTransform::OnScaleTextEnter )
END_EVENT_TABLE()
//======================================================================

