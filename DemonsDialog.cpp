/*
  Copyright 1993-2008 Medical Image Processing Group
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
// DemonsDialog
//----------------------------------------------------------------------
#include  "cavass.h"

#include  "demons2d.h"
#include  "demons3d.h"
#include  "DemonsDialog.h"

using namespace std;
//----------------------------------------------------------------------
void DemonsDialog::init ( void ) {
    m_parent          = NULL;
    m_it_value        = 0;
    m_it_slider       = NULL;
    m_mp_value        = 0;
    m_mp_slider       = NULL;
    m_hl_value        = 0;
    m_hl_slider       = NULL;
    m_sd_value        = 0;
    m_sd_text         = NULL;
    m_sd_slider       = NULL;
    m_dimension       = NULL;
    m_dim_value       = 0;
    m_thresholdAtMean = NULL;
    m_tam_value       = false;
    m_normalize       = NULL;
    m_norm_value      = false;
    m_ok              = NULL;
    m_cancel          = NULL;
    m_seconds         = 0;
}
//----------------------------------------------------------------------
DemonsDialog::DemonsDialog ( wxFrame* parent )
    : wxDialog( parent, -1, "Thirion's Demons algorithm options",
                wxDefaultPosition, wxDefaultSize )
{
    init();
//    const int  sliderWidth = 200;
    m_parent = parent;
    wxStaticText*  st;

    wxPanel*  mainPanel = new wxPanel( this, -1 );
    ::setColor( this );
    ::setColor( mainPanel );
    //SetAutoLayout( true );
    //mainPanel->SetAutoLayout( true );

    //wxSizer*  mainSizer = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer*  mainSizer = new wxFlexGridSizer(1, 5, 5);
    // - - - - - - - - - -
    mainSizer->Add(25, 25);
    // - - - - - - - - - -
    wxSizer*  gs1 = new wxFlexGridSizer(2, 25, 25);  //2 cols,vgap,hgap
    mainSizer->Add( gs1, 1, wxALIGN_CENTER );

    st = new wxStaticText( mainPanel, -1, "iterations:" );
    gs1->Add( st, 1, wxALIGN_RIGHT );
    m_it_slider = new wxSlider( mainPanel, ID_ITERATIONS, DemonsITDefault,
        DemonsITMin, DemonsITMax, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
        wxDefaultValidator, "iterations" );
    m_it_slider->SetPageSize(5);
    ::setColor( m_it_slider );
    gs1->Add( m_it_slider, 0, wxALIGN_CENTER );

    st = new wxStaticText( mainPanel, -1, "match points:" );
    gs1->Add( st, 1, wxALIGN_RIGHT );
    m_mp_slider = new wxSlider( mainPanel, ID_MATCH_POINTS, DemonsMPDefault,
        DemonsMPMin, DemonsMPMax, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
        wxDefaultValidator, "match points" );
    m_mp_slider->SetPageSize(5);
    ::setColor( m_mp_slider );
    gs1->Add( m_mp_slider, 0, wxALIGN_CENTER );

    st = new wxStaticText( mainPanel, -1, "histogram levels:" );
    gs1->Add( st, 1, wxALIGN_RIGHT );
    m_hl_slider = new wxSlider( mainPanel, ID_HISTOGRAM_LEVELS,
        DemonsHLDefault, DemonsHLMin, DemonsHLMax, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
        wxDefaultValidator, "histogram levels" );
    m_hl_slider->SetPageSize(5);
    ::setColor( m_hl_slider );
    gs1->Add( m_hl_slider, 0, wxALIGN_CENTER );

    m_sd_value = DemonsSDDefault;
    wxString  s = wxString::Format( "standard deviation: %4.1f",
                                    m_sd_value/10.0 );
    m_sd_text = st = new wxStaticText( mainPanel, -1, s );
    gs1->Add( st, 1, wxALIGN_RIGHT );
    m_sd_slider = new wxSlider( mainPanel, ID_STANDARD_DEVIATION,
        DemonsSDDefault, DemonsSDMin, DemonsSDMax, wxDefaultPosition,
        wxSize(sliderWidth,-1), wxSL_HORIZONTAL|wxSL_TOP,
        wxDefaultValidator, "standard deviation" );
    m_sd_slider->SetPageSize(5);
    ::setColor( m_sd_slider );
    gs1->Add( m_sd_slider, 0, wxALIGN_CENTER );
    // - - - - - - - - - -
    mainSizer->Add(25, 25);
    // - - - - - - - - - -
    wxSizer*  gs2 = new wxGridSizer(3, 1, 1);  //3 cols,vgap,hgap
    gs2->SetMinSize( 2*sliderWidth, -1 );
    mainSizer->Add( gs2, 1, wxALIGN_CENTER );

    wxString  dimension[] = { "2D", "3D" };
    m_dimension = new wxRadioBox( mainPanel, ID_DIMENSION, "dimension",
          wxDefaultPosition, wxSize((int)(0.6*sliderWidth),-1), 2,
          dimension, 1, wxRA_SPECIFY_COLS );
    gs2->Add( m_dimension, 0, wxALIGN_CENTER );
    m_dimension->SetSelection(0);
    ::setColor( m_dimension );

    wxString  threshold[] = { "yes", "no" };
    m_thresholdAtMean = new wxRadioBox( mainPanel, ID_THRESHOLD_AT_MEAN,
        "threshold at mean", wxDefaultPosition,
        wxSize((int)(0.6*sliderWidth),-1),
        2, threshold, 1, wxRA_SPECIFY_COLS );
    gs2->Add( m_thresholdAtMean, 0, wxALIGN_CENTER );
    m_thresholdAtMean->SetSelection(0);
    ::setColor( m_thresholdAtMean );

    wxString  normalize[] = { "yes", "no" };
    m_normalize = new wxRadioBox( mainPanel, ID_NORMALIZE, "normalize",
        wxDefaultPosition, wxSize((int)(0.6*sliderWidth),-1), 2,
        normalize, 1, wxRA_SPECIFY_COLS );
    gs2->Add( m_normalize, 0, wxALIGN_CENTER );
    m_normalize->SetSelection(1);
    ::setColor( m_normalize );
    // - - - - - - - - - -
    mainSizer->Add(25, 25);
    // - - - - - - - - - -
    wxSizer*  gs3 = new wxGridSizer(2, 20, 20);  //2 cols,vgap,hgap
    gs3->SetMinSize( 2*sliderWidth, 50 );
    mainSizer->Add( gs3, 1, wxALIGN_CENTER );

    m_ok = new wxButton( mainPanel, wxID_OK, "OK" );
    gs3->Add( m_ok, 0, wxALIGN_RIGHT );
    ::setColor( m_ok );

    m_cancel = new wxButton( mainPanel, wxID_CANCEL, "Cancel" );
    gs3->Add( m_cancel, 0, wxALIGN_LEFT );
    ::setColor( m_cancel );

    mainPanel->SetAutoLayout( true );
    mainPanel->SetSizer( mainSizer );

    //mainSizer->Fit( this );
    mainSizer->SetSizeHints( this );
    //SetSizer( mainSizer );
    //Layout();

    ShowModal();
}
//----------------------------------------------------------------------
void DemonsDialog::OnITSlider ( wxScrollEvent& e ) {
    //m_it_value = m_it_slider->GetValue();
}

void DemonsDialog::OnMPSlider ( wxScrollEvent& e ) {
    //m_mp_value = m_mp_slider->GetValue();
}

void DemonsDialog::OnHLSlider ( wxScrollEvent& e ) {
    //m_hl_value = m_hl_slider->GetValue();
}

void DemonsDialog::OnSDSlider ( wxScrollEvent& e ) {
    m_sd_value = m_sd_slider->GetValue();
    wxString  s = wxString::Format( "standard deviation: %4.1f",
                                    m_sd_value/10.0 );
    m_sd_text->SetLabel(s);
}
//----------------------------------------------------------------------
void DemonsDialog::OnDimRadiobox ( wxCommandEvent& e ) {
    //m_dim_value = m_dimension->GetSelection();
}

void DemonsDialog::OnTAMRadiobox ( wxCommandEvent& e ) {
    // 0 indicates yes is selected
    //m_tam_value = (m_thresholdAtMean->GetSelection()==0);
}

void DemonsDialog::OnNormRadiobox ( wxCommandEvent& e ) {
    // 0 indicates yes is selected
    //m_norm_value = (m_normalize->GetSelection()==0);
}
//----------------------------------------------------------------------
void DemonsDialog::OnDoDemonsRegistration ( wxCommandEvent& e ) {
    m_it_value = m_it_slider->GetValue();
    m_mp_value = m_mp_slider->GetValue();
    m_hl_value = m_hl_slider->GetValue();
    m_sd_value = m_sd_slider->GetValue();
    // 0 indicates yes is selected
    m_tam_value = (m_thresholdAtMean->GetSelection()==0);
    // 0 indicates yes is selected
    m_norm_value = (m_normalize->GetSelection()==0);

    Close();
    if (m_dim_value==0) {  //2d?
        OnDoDemons2DRegistration();
    }
    Destroy();
}
//----------------------------------------------------------------------
void DemonsDialog::OnBusyTimer ( wxTimerEvent& e ) {
    cout << "OnBusyTimer:" << m_seconds++ << "s" << endl;
    //static bool  flipFlop = true;
    //if (flipFlop)  SetCursor( wxCursor(wxCURSOR_WAIT) );
    //else           SetCursor( *wxSTANDARD_CURSOR );
    //flipFlop = !flipFlop;
}
//----------------------------------------------------------------------
void DemonsDialog::OnDoDemons2DRegistration ( void ) {
        if (::demonsInputList.size() != 2)    return;
        SetCursor( wxCursor(wxCURSOR_WAIT) );
        m_parent->SetCursor( wxCursor(wxCURSOR_WAIT) );
        cout << "starting 2D Demons registration" << endl;
        wxLogMessage("starting 2D Demons registration");
        wxProgressDialog( "processing", "running 2D Demons registration" );

        MontageFrame*  mmf1 = dynamic_cast<MontageFrame*>(::demonsInputList[0]);
        MontageFrame*  mmf2 = dynamic_cast<MontageFrame*>(::demonsInputList[1]);
        const MontageCanvas* const  c1 = dynamic_cast<MontageCanvas*>(mmf1->mCanvas);
        const MontageCanvas* const  c2 = dynamic_cast<MontageCanvas*>(mmf2->mCanvas);
#if 1
        wxTimer  busyTimer(this, ID_BUSY_TIMER);
        busyTimer.Start(1000);  //1000 = 1 second interval
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        class Demons2DRegistration : public wxThread {
            private:
                const int                   xSize1, ySize1, zSize1;
                const double                pixelSizeX1, pixelSizeY1;
                const int                   dataSize1;
                const unsigned char* const  ucData1;
                const int                   sliceNo1;

                const int                   xSize2, ySize2, zSize2;
                const double                pixelSizeX2, pixelSizeY2;
                const int                   dataSize2;
                const unsigned char* const  ucData2;
                const int                   sliceNo2;

                const int                   iterations, matchPoints,
                                            histogramLevels;
                const double                standardDeviations;
                const bool                  thresholdAtMean;
                const bool                  normalize;

            public:
                int*  mResult;
                // - - - - - - - - - - - - - - - - - - - - - - - - - - -
                Demons2DRegistration (
                    const int xSize1, const int ySize1, const int zSize1,
                    const double pixelSizeX1, const double pixelSizeY1,
                    const int dataSize1,
                    const unsigned char* const ucData1, const int sliceNo1,

                    const int xSize2, const int ySize2, const int zSize2,
                    const double pixelSizeX2, const double pixelSizeY2,
                    const int dataSize2,
                    const unsigned char* const ucData2, const int sliceNo2,

                    const int iterations=10, const int matchPoints=10,
                    const int histogramLevels=200,
                    const double standardDeviations=1.0,
                    const bool thresholdAtMean=true,
                    const bool normalize=false )

                  : wxThread( wxTHREAD_JOINABLE ),
                    xSize1(xSize1), ySize1(ySize1), zSize1(zSize1),
                    pixelSizeX1(pixelSizeX1), pixelSizeY1(pixelSizeY1),
                    dataSize1(dataSize1), ucData1(ucData1), sliceNo1(sliceNo1),

                    xSize2(xSize2), ySize2(ySize2), zSize2(zSize2),
                    pixelSizeX2(pixelSizeX2), pixelSizeY2(pixelSizeY2),
                    dataSize2(dataSize2), ucData2(ucData2), sliceNo2(sliceNo2),

                    iterations(iterations), matchPoints(matchPoints),
                    histogramLevels(histogramLevels),
                    standardDeviations(standardDeviations),
                    thresholdAtMean(thresholdAtMean),
                    normalize(normalize),

                    mResult(NULL)
                {
                    Create();
                }
                // - - - - - - - - - - - - - - - - - - - - - - - - - - -
                ExitCode Entry ( void ) {  //do thread work
                    mResult = doDemons2DRegistration(
                        xSize1, ySize1, zSize1,
                        pixelSizeX1, pixelSizeY1,
                        dataSize1, ucData1, sliceNo1,

                        xSize2, ySize2, zSize2,
                        pixelSizeX2, pixelSizeY2,
                        dataSize2, ucData2, sliceNo2,

                        iterations, matchPoints, histogramLevels,
                        standardDeviations, thresholdAtMean, normalize
                    );

                    return 0;  //ok
                }
        };
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Demons2DRegistration*  d2d = new Demons2DRegistration(
            c1->mCavassData->m_xSize,     c1->mCavassData->m_ySize,  c1->mCavassData->m_zSize,
            c1->mCavassData->m_xSpacing,  c1->mCavassData->m_ySpacing,
            c1->mCavassData->m_size,      (unsigned char*)c1->mCavassData->m_data,   c1->getSliceNo(),

            c2->mCavassData->m_xSize,     c2->mCavassData->m_ySize,  c2->mCavassData->m_zSize,
            c2->mCavassData->m_xSpacing,  c2->mCavassData->m_ySpacing,
            c2->mCavassData->m_size,      (unsigned char*)c2->mCavassData->m_data,   c2->getSliceNo(),

            m_it_value, m_mp_value, m_hl_value,
            m_sd_value/10.0, m_tam_value, m_norm_value
        );
        d2d->SetPriority(WXTHREAD_MAX_PRIORITY);
        d2d->Run();
        for ( ; ; ) {
            wxYield();
            if (!d2d->IsRunning())  break;
        }
        d2d->Wait();
        busyTimer.Stop();
        const int* const  result = d2d->mResult;
#else
        const int* const  result = doDemons2DRegistration(
            c1->m_xSize,     c1->m_ySize,  c1->m_zSize,
            c1->m_xSpacing,  c1->m_ySpacing,
            c1->m_size,      c1->m_data,   c1->getSliceNo(),

            c2->m_xSize,     c2->m_ySize,  c2->m_zSize,
            c2->m_xSpacing,  c2->m_ySpacing,
            c2->m_size,      c2->m_data,   c2->getSliceNo(),

            m_it_value, m_mp_value, m_hl_value,
            m_sd_value/10.0, m_tam_value, m_norm_value );
#endif
        //show the result (just one slice)
        char  buff[255];
        sprintf( buff, "registered result (fixed slice %d from %s and %d from %s; iterations %d)",
                 c1->getSliceNo(), c1->mCavassData->m_fname, c2->getSliceNo(), c2->mCavassData->m_fname,
                 m_it_value );
        MontageFrame*  f = new MontageFrame();
                  //f->Show();
                  f->loadData( buff, c1->mCavassData->m_xSize, c1->mCavassData->m_ySize, 1,
                      c1->mCavassData->m_xSpacing, c1->mCavassData->m_ySpacing, c1->mCavassData->m_zSpacing, result,
                      &c1->mCavassData->m_vh, c1->mCavassData->m_vh_initialized );

        //create the difference slice (just one)
        int*  diff = (int*)malloc( c1->mCavassData->m_xSize*c1->mCavassData->m_ySize*sizeof(int) );
        assert( diff!=NULL    );
        assert( c1->mCavassData->m_size==2 );
        unsigned short*  us = (unsigned short*)c1->mCavassData->m_data;
        const int  offset = c1->getSliceNo() * c1->mCavassData->m_xSize * c1->mCavassData->m_ySize;
        int     max=0;
        double  mean=0.0;
        for (int i=0; i<c1->mCavassData->m_xSize*c1->mCavassData->m_ySize; i++) {
            int value = result[i] - us[i+offset];
            if (value<0)    value = -value;
            diff[i] = value;

            if (value>max)  max=value;
            mean += value;
        }
        mean /= c1->mCavassData->m_xSize*c1->mCavassData->m_ySize;
        sprintf( buff, "slice (max mag diff=%d,mean mag diff=%.2f)",
                 max, mean );
        puts( buff );    wxLogMessage( buff );
        f = new MontageFrame();
        sprintf( buff, "difference (fixed slice %d from %s and %d from %s; iterations %d)",
                 c1->getSliceNo(), c1->mCavassData->m_fname, c2->getSliceNo(), c2->mCavassData->m_fname,
                 m_it_value );
        f->loadData( buff, c1->mCavassData->m_xSize, c1->mCavassData->m_ySize, 1,
            c1->mCavassData->m_xSpacing, c1->mCavassData->m_ySpacing, c1->mCavassData->m_zSpacing, diff,
            &c1->mCavassData->m_vh, c1->mCavassData->m_vh_initialized );

        cout << "finished 2D Demons registration" << endl;
        wxLogMessage("finished 2D Demons registration \n max=%d, mean=%f \n", max, mean);
        SetCursor( *wxSTANDARD_CURSOR );
        m_parent->SetCursor( *wxSTANDARD_CURSOR );

        //create a frame containing two slices so one may compare the
        // results of registration.  this creates a frame WITHOUT registration.
        if (true) {
          assert( c1->mCavassData->m_xSize == c2->mCavassData->m_xSize );
          assert( c1->mCavassData->m_ySize == c2->mCavassData->m_ySize );
          //number of pixels in a single frame
          const int numberOfPixels = c1->mCavassData->m_xSize * c1->mCavassData->m_ySize;
          int*  twoFrames = (int*)malloc( 2 * numberOfPixels * sizeof(int) );
          assert( twoFrames!=NULL );
          //frame 1 (from original input data)
          int  offset = c1->getSliceNo() * c1->mCavassData->m_xSize * c1->mCavassData->m_ySize;
          if        (c1->mCavassData->m_size == 1) {
              unsigned char*  uc = (unsigned char*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = uc[i + offset];
          } else if (c1->mCavassData->m_size == 2) {
              unsigned short*  us = (unsigned short*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = us[i + offset];
          } else if (c1->mCavassData->m_size == 4) {
              int*  si = (int*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = si[i + offset];
          } else {
              assert(0);
          }
          //frame 2 (from original input data)
          offset = c2->getSliceNo() * c2->mCavassData->m_xSize * c2->mCavassData->m_ySize;
          if        (c2->mCavassData->m_size == 1) {
              unsigned char*  uc = (unsigned char*)c2->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i+numberOfPixels] = uc[i + offset];
          } else if (c2->mCavassData->m_size == 2) {
              unsigned short*  us = (unsigned short*)c2->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i+numberOfPixels] = us[i + offset];
          } else if (c2->mCavassData->m_size == 4) {
              int*  si = (int*)c2->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i+numberOfPixels] = si[i + offset];
          } else {
              assert(0);
          }

          f = new MontageFrame();
          //f->Show();
          f->loadData( "without registration", c1->mCavassData->m_xSize, c1->mCavassData->m_ySize, 2,
              c1->mCavassData->m_xSpacing, c1->mCavassData->m_ySpacing, c1->mCavassData->m_zSpacing, twoFrames,
              &c1->mCavassData->m_vh, c1->mCavassData->m_vh_initialized );
        }

        //create a frame containing two slices so one may compare the
        // results of registration.  this creates a frame WITH registration.
        if (true) {
          assert( c1->mCavassData->m_xSize == c2->mCavassData->m_xSize );
          assert( c1->mCavassData->m_ySize == c2->mCavassData->m_ySize );
          //number of pixels in a single frame
          const int numberOfPixels = c1->mCavassData->m_xSize * c1->mCavassData->m_ySize;
          int*  twoFrames = (int*)malloc( 2 * numberOfPixels * sizeof(int) );
          assert( twoFrames!=NULL );
          //frame 1 (from original input data)
          int  offset = c1->getSliceNo() * c1->mCavassData->m_xSize * c1->mCavassData->m_ySize;
          if        (c1->mCavassData->m_size == 1) {
              unsigned char*  uc = (unsigned char*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = uc[i + offset];
          } else if (c1->mCavassData->m_size == 2) {
              unsigned short*  us = (unsigned short*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = us[i + offset];
          } else if (c1->mCavassData->m_size == 4) {
              int*  si = (int*)c1->mCavassData->m_data;
              for (int i=0; i<numberOfPixels; i++)
                  twoFrames[i] = si[i + offset];
          } else {
              assert(0);
          }
          //frame 2 (from registration result)
          for (int i=0; i<numberOfPixels; i++)
              twoFrames[i+numberOfPixels] = result[i];

          f = new MontageFrame();
          //f->Show();
          sprintf( buff, "WITH registration; iterations %d", m_it_value );
          f->loadData( buff, c1->mCavassData->m_xSize, c1->mCavassData->m_ySize, 2,
              c1->mCavassData->m_xSpacing, c1->mCavassData->m_ySpacing, c1->mCavassData->m_zSpacing, twoFrames,
              &c1->mCavassData->m_vh, c1->mCavassData->m_vh_initialized );
        }

    }
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( DemonsDialog, wxDialog )
BEGIN_EVENT_TABLE       ( DemonsDialog, wxDialog )
    EVT_COMMAND_SCROLL( ID_ITERATIONS,         DemonsDialog::OnITSlider )
    EVT_COMMAND_SCROLL( ID_MATCH_POINTS,       DemonsDialog::OnMPSlider )
    EVT_COMMAND_SCROLL( ID_HISTOGRAM_LEVELS,   DemonsDialog::OnHLSlider )
    EVT_COMMAND_SCROLL( ID_STANDARD_DEVIATION, DemonsDialog::OnSDSlider )

    EVT_RADIOBOX( ID_DIMENSION,         DemonsDialog::OnDimRadiobox  )
    EVT_RADIOBOX( ID_THRESHOLD_AT_MEAN, DemonsDialog::OnTAMRadiobox  )
    EVT_RADIOBOX( ID_NORMALIZE,         DemonsDialog::OnNormRadiobox )

    EVT_BUTTON( wxID_OK, DemonsDialog::OnDoDemonsRegistration )

    EVT_TIMER( ID_BUSY_TIMER, DemonsDialog::OnBusyTimer )
END_EVENT_TABLE()
//======================================================================
