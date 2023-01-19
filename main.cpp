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
 * \file   main.cpp
 * \brief  CAVASS program globals functions and CavassMain definition and
 * implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "MontageCanvas.h"

Vector            demonsInputList;
wxPrintData*      g_printData = NULL;      ///< global print data - to remember settings during the session
wxPageSetupData*  g_pageSetupData = NULL;  ///< global page setup data
int               gWhere = 20;             ///< neext new window position
FileHistory       gInputFileHistory;       ///< global input file history
wxFont            gDefaultFont;            ///< default font
wxLogWindow*      gLogWindow = NULL;       ///< log window (if any)
//use 1/gHistogramBucketFactor buckets for the histogram, i.e., divide each
// gray value by this factor before counting it
int  gHistogramBucketFactor = 1;
int  gNextWindowID = MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU;

#include  "cavass-splash.xpm"
#include  "button-zoomin15.xpm"
#include  "button-zoomout15.xpm"
//----------------------------------------------------------------------
/** \brief 
 *  \param f1 is the name of an input file.
 *  \param f2 is the name of an input file.
 *  \returns true if f1 equals f2; false otherwise.
 */
bool equals ( wxFileName f1, wxFileName f2 ) {
    f1.Normalize();
    f2.Normalize();
    return (f1==f2);
}
//----------------------------------------------------------------------
void setColor ( wxWindow* w ) {
    if (Preferences::getCustomAppearance()) {
        int  r, g, b;

        r = Preferences::getBgRed();
        g = Preferences::getBgGreen();
        b = Preferences::getBgBlue();
        //w->SetBackgroundColour( wxColour(LtBlue) );
        w->SetBackgroundColour( wxColour(r,g,b) );
        
        r = Preferences::getFgRed();
        g = Preferences::getFgGreen();
        b = Preferences::getFgBlue();
        //w->SetForegroundColour( wxColour(Yellow) );
        w->SetForegroundColour( wxColour(r,g,b) );
    }
}
//----------------------------------------------------------------------
void setBackgroundColor ( wxWindow* w ) {
    if (Preferences::getCustomAppearance()) {
        int  r, g, b;

        r = Preferences::getBgRed();
        g = Preferences::getBgGreen();
        b = Preferences::getBgBlue();
        //w->SetBackgroundColour( wxColour(LtBlue) );
        w->SetBackgroundColour( wxColour(r,g,b) );
    }
}
//----------------------------------------------------------------------
void setZoomInColor ( wxWindow* w ) {
    if (Preferences::getCustomAppearance()) {
        w->SetBackgroundColour( wxColour(Yellow) );
        w->SetForegroundColour( wxColour(LtBlue) );
    }
}
//----------------------------------------------------------------------
void setDisabledColor ( wxWindow* w ) {
    if (Preferences::getCustomAppearance()) {
        w->SetBackgroundColour( wxColour(DkBlue) );
        w->SetForegroundColour( wxColour(*wxLIGHT_GREY) );
    }
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
extern Vector  gFrameList;

void addToAllWindowMenus ( wxString s ) {
    for (int i=0; i<(int)::gFrameList.size(); i++) {
        if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
            temp->mWindowMenu->Append( ::gNextWindowID, s );
        } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
            temp->mWindowMenu->Append( ::gNextWindowID, s );
        } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
            temp->mWindowMenu->Append( ::gNextWindowID, s );
        }
    }
    ::gNextWindowID++;
    assert( ::gNextWindowID <= MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU );
}
//----------------------------------------------------------------------
void removeFromAllWindowMenus ( wxString s ) {
    for (int i=0; i<(int)::gFrameList.size(); i++) {
        if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
            for (int i=0; i<(int)temp->mWindowMenu->GetMenuItemCount(); /*unused*/) {
                wxMenuItem*  mi = temp->mWindowMenu->FindItemByPosition( i );
                if (mi->GetItemLabelText().Cmp(s) == 0)    temp->mWindowMenu->Remove( mi );
                else                               ++i;
            }
        } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
            for (int i=0; i<(int)temp->mWindowMenu->GetMenuItemCount(); /*unused*/) {
                wxMenuItem*  mi = temp->mWindowMenu->FindItemByPosition( i );
                if (mi->GetItemLabelText().Cmp(s) == 0)    temp->mWindowMenu->Remove( mi );
                else                               ++i;
            }
        } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
            for (int i=0; i<(int)temp->mWindowMenu->GetMenuItemCount(); /*unused*/) {
                wxMenuItem*  mi = temp->mWindowMenu->FindItemByPosition( i );
                if (mi->GetItemLabelText().Cmp(s) == 0)    temp->mWindowMenu->Remove( mi );
                else                               ++i;
            }
        }
    }
}
//----------------------------------------------------------------------
bool searchWindowTitles ( wxString s ) {
    for (int i=0; i<(int)::gFrameList.size(); i++) {
        if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0)    return true;  //found it
        } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0)    return true;  //found it
        } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0)    return true;  //found it
        }
    }
    return false;
}
//----------------------------------------------------------------------
void raiseWIndow ( wxString s ) {
    for (int i=0; i<(int)::gFrameList.size(); i++) {
        if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0) {
                temp->Raise();
                return;  //found it
            }
        } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0) {
                temp->Raise();
                return;  //found it
            }
        } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
            if (s.Cmp(temp->mWindowTitle)==0) {
                temp->Raise();  //found it
                return;
            }
        }
    }
}
//----------------------------------------------------------------------
void copyWindowTitles ( wxFrame* whichFrame ) {
    if (MontageFrame*  dst = dynamic_cast<MontageFrame*>(whichFrame)) {
        for (int i=0; i<(int)::gFrameList.size(); i++) {
            if (::gFrameList[i] == whichFrame)    continue;
            if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            }
            ::gNextWindowID++;
            assert( ::gNextWindowID <= MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU );
        }
    } else if (OverlayFrame*  dst = dynamic_cast<OverlayFrame*>(whichFrame)) {
        for (int i=0; i<(int)::gFrameList.size(); i++) {
            if (::gFrameList[i] == whichFrame)    continue;
            if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            }
            ::gNextWindowID++;
            assert( ::gNextWindowID <= MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU );
        }
    } else if (MainFrame*  dst = dynamic_cast<MainFrame*>(whichFrame)) {
        for (int i=0; i<(int)::gFrameList.size(); i++) {
            if (::gFrameList[i] == whichFrame)    continue;
            if (MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (OverlayFrame*  temp = dynamic_cast<OverlayFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            } else if (MainFrame*  temp = dynamic_cast<MainFrame*>(::gFrameList[i])) {
                dst->mWindowMenu->Append( ::gNextWindowID, temp->mWindowTitle );
            }
            ::gNextWindowID++;
            assert( ::gNextWindowID <= MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU );
        }
    }
}
//----------------------------------------------------------------------
void changeAllWindowMenus ( wxString from, wxString to ) {
    //doesn't work.  i think there is a bug in wx.  use remove and then add instead.
    assert( 0 );

    for (int i=0; i<(int)::gFrameList.size(); i++) {
        MainFrame*  main = dynamic_cast<MainFrame*>(::gFrameList[i]);
        if (main) {
            for (int j=0; j<(int)main->mWindowMenu->GetMenuItemCount(); j++) {
                wxMenuItem*  mi = main->mWindowMenu->FindItemByPosition( j );
                wxString     tmp = mi->GetItemLabelText();
                if (mi->GetItemLabelText().Cmp(from) == 0) {
                    mi->SetItemLabel( to );
                }
            }
        } else {
            MontageFrame*  temp = dynamic_cast<MontageFrame*>(::gFrameList[i]);
            if (temp) {
                for (int j=0; j<(int)temp->mWindowMenu->GetMenuItemCount(); j++) {
                    wxMenuItem*  mi = temp->mWindowMenu->FindItemByPosition( j );
                    wxString     tmp = mi->GetItemLabelText();
                    if (mi->GetItemLabelText().Cmp(from) == 0) {
                        mi->SetItemLabel( to );
                    }
                }
            }
        }
    }
}
//----------------------------------------------------------------------
template< typename T >
static unsigned char* lookup ( CavassData& cd, T dummy ) {
    assert( cd.mDisplay );
    assert( 0<=cd.m_sliceNo && cd.m_sliceNo<cd.m_zSize );

    unsigned char*  slice = (unsigned char*)malloc( cd.m_xSize * cd.m_ySize * 3 );  //3 for rgb data
    assert( slice!=NULL );

    int  dst = 0;  //offset into result rgb data
    int  offset = 0;  //offset from beginning of (original, gray or color) slice data
    const T* const  p = (const T* const)cd.getSlice( cd.m_sliceNo );
    if (cd.mSamplesPerPixel==1) {
        for (int y=0; y<cd.m_ySize; y++) {
            for (int x=0; x<cd.m_xSize; x++) {
                int  index = (int)(p[offset++] - cd.m_min);
                assert( 0 <= index && index <= cd.m_max-cd.m_min );
                double  red   = cd.mR * cd.m_lut[ index ];
                double  green = cd.mG * cd.m_lut[ index ];
                double  blue  = cd.mB * cd.m_lut[ index ];
                int  ir = (int)(red  +0.5);
                int  ig = (int)(green+0.5);
                int  ib = (int)(blue +0.5);
                if (ir<0)    ir=0;
                if (ir>255)  ir=255;
                if (ig<0)    ig=0;
                if (ig>255)  ig=255;
                if (ib<0)    ib=0;
                if (ib>255)  ig=255;
                slice[dst]   = (unsigned char)ir;
                slice[dst+1] = (unsigned char)ig;
                slice[dst+2] = (unsigned char)ib;
                dst += 3;
            }
        }
    } else if (cd.mSamplesPerPixel==3) {
        for (int y=0; y<cd.m_ySize; y++) {
            for (int x=0; x<cd.m_xSize; x++) {
                const double  red   = cd.mR * cd.m_lut[ (int)(p[offset++]-cd.m_min) ];
                const double  green = cd.mG * cd.m_lut[ (int)(p[offset++]-cd.m_min) ];
                const double  blue  = cd.mB * cd.m_lut[ (int)(p[offset++]-cd.m_min) ];
                int  ir = (int)(red  +0.5);
                int  ig = (int)(green+0.5);
                int  ib = (int)(blue +0.5);
                if (ir<0)    ir=0;
                if (ir>255)  ir=255;
                if (ig<0)    ig=0;
                if (ig>255)  ig=255;
                if (ib<0)    ib=0;
                if (ib>255)  ig=255;
                slice[dst]   = (unsigned char)ir;
                slice[dst+1] = (unsigned char)ig;
                slice[dst+2] = (unsigned char)ib;
                dst += 3;
            }
        }
    } else {
        assert( 0 );
    }
    return slice;
}
//----------------------------------------------------------------------
/** \brief   create the displayed data from the image data.
*   \param   cd is the source image data
*   \returns one composited slice of rgb data (malloc'd by this
*            function so the caller must free it)
*/
unsigned char* toRGB ( CavassData& cd ) {
    if (cd.m_size==1 || cd.m_size/cd.mSamplesPerPixel==1) {
        if (cd.m_min>=0) {
            unsigned char  dummy=0;
            return lookup( cd, dummy );
        } else {
            signed char  dummy=0;
            return lookup( cd, dummy );
        }
    } else if (cd.m_size==2) {
        if (cd.m_min>=0) {
            unsigned short  dummy=0;
            return lookup( cd, dummy );
        } else {
            signed short  dummy=0;
            return lookup( cd, dummy );
        }
    } else if (cd.m_size==4) {
        int  dummy=0;
        return lookup( cd, dummy );
    } else if (cd.m_size==8) {
        double  dummy=0;
        return lookup( cd, dummy );
    }
    return NULL;
}
//----------------------------------------------------------------------
inline static int interpolate ( int a,  int b,  int c,  int d,
                                int x1, int x2, int y1, int y2,
                                double xPos, double yPos )
{
    /*
        (x1,y1)          (x2,y1)
           a----------------b
           |   |            |
           |   |(xPos,yPos) |
           |---x------------|
           |   |            |
           |   |            |
           |   |            |
           c----------------d
        (x1,y2)          (x2,y2)

        The '*' point at (xPos,yPos) is the location in the input
        data that we wish to interpolate.
    */
    assert( (x2-x1)==1 && (y2-y1)==1 );
    //interpolate the gray value along the line segment between a-b
    double  dx = xPos - x1;
    double  ab = (int)((1.0-dx)*a + dx*b + 0.5);
    //interpolate the gray value along the line segment between c-d
    double  cd = (int)((1.0-dx)*c + dx*d + 0.5);
    //interpolate the gray value along the ling segment between ab-cd
    double  dy = yPos - y1;
    int  result = (int)((1.0-dy)*ab + dy*cd + 0.5);

    return result;
}
//----------------------------------------------------------------------
static unsigned char* lookup ( CavassData& cd, double sx, double sy ) {
    assert( cd.mDisplay );
    assert( 0<=cd.m_sliceNo && cd.m_sliceNo<cd.m_zSize );

    const int  ow = (int)(sx * cd.m_xSize);
    const int  oh = (int)(sy * cd.m_ySize);
    unsigned char*  slice = (unsigned char*)malloc( ow * oh * 3 );  //3 for rgb data
    assert( slice!=NULL );

    const double  dx = 1.0 / sx;
    const double  dy = 1.0 / sy;
    double  yPos = 0.0;
    int  dst = 0;  //offset into result rgb data
    for (int ro=0; ro<oh; ro++) {  //output row
        double  xPos = 0.0;
        for (int oc=0; oc<ow; oc++) {  //output col
            /*
                (x1,y1)          (x2,y1)
                   a----------------b
                   |                |
                   |    (xPos,yPos) |
                   |   *            |
                   |                |
                   |                |
                   |                |
                   c----------------d
                (x1,y2)          (x2,y2)

                All (xi,yk) above are integer coordinates into input data.

                The '*' point at (xPos,yPos) is the location in the input
                data that we wish to interpolate.
            */
            int  x1 = (int)xPos;
            int  y1 = (int)yPos;
            int  x2 = x1 + 1;
            int  y2 = y1 + 1;
            //get the gray values at each of the 4 corners: a, b, c, d
            int  a = cd.getData( x1, y1, cd.m_sliceNo );
            int  b = cd.getData( x2, y1, cd.m_sliceNo );
            int  c = cd.getData( x1, y2, cd.m_sliceNo );
            int  d = cd.getData( x2, y2, cd.m_sliceNo );

            // \todo interpolate the gray value at the desired position
            int  v = interpolate( a, b, c, d, x1, x2, y1, y2, xPos, yPos );
            v -= cd.m_min;
            if (v<0)    v = 0;
            const double  red   = cd.mR * cd.m_lut[ v ];
            const double  green = cd.mG * cd.m_lut[ v ];
            const double  blue  = cd.mB * cd.m_lut[ v ];
            int  ir = (int)(red  +0.5);
            int  ig = (int)(green+0.5);
            int  ib = (int)(blue +0.5);
            if (ir<0)    ir=0;
            if (ir>255)  ir=255;
            if (ig<0)    ig=0;
            if (ig>255)  ig=255;
            if (ib<0)    ib=0;
            if (ib>255)  ig=255;
            slice[dst]   = (unsigned char)ir;
            slice[dst+1] = (unsigned char)ig;
            slice[dst+2] = (unsigned char)ib;
            dst += 3;

            xPos += dx;
        }
        yPos += dy;
    }

    return slice;
}
//----------------------------------------------------------------------
/** \brief   create the displayed data from the image data.
*   \param   cd is the source image data
*   \param   sx is the scale in the x direction
*   \param   sy is the scale in the y direction
*   \returns one composited slice of rgb data (malloc'd by this
*            function so the caller must free it)
*/
unsigned char* toRGBInterpolated ( CavassData& cd, double sx, double sy ) {
    return lookup( cd, sx, sy );
}
//----------------------------------------------------------------------
//======================================================================
static void calledAtExit ( void ) {
	//this is very important because it causes the write to the configuration file
    delete wxConfigBase::Set( (wxConfigBase*)NULL );
}
//======================================================================
/** \brief CavassMain definition and implementation.
 */
class CavassMain : public wxApp {
  public:
    /** \brief init and start the Cavass program.
     */
    virtual bool OnInit ( void ) {
        puts( "in CavassMain::OnInit()" );
        //cout << "free memory  = " << hex << ::wxGetFreeMemory() << dec << endl;
        cout << "aspect ratio = " << getAspectRatio() << endl;

        SetVendorName( _T("MIPG")   );
        SetAppName(    _T("CAVASS") );
        wxFileConfig*  wxfc = new wxFileConfig( "CAVASS" );
		assert( wxfc != NULL );
		wxFileConfig::Set( wxfc );
		wxfc->SetRecordDefaults();
		atexit( calledAtExit );

        loadConfig();
		::modifyEnvironment( (char *)(const char *)argv[0].c_str() );


        wxInitAllImageHandlers();
        gLogWindow = new wxLogWindow( NULL, "log", Preferences::getShowLog(), false );
        wxLogMessage( "VIEWNIX_ENV=%s", getenv("VIEWNIX_ENV") );
        wxLogMessage( "PATH=%s",        getenv("PATH") );
        wxLogMessage( "ARGV[0]=%s",     argv[0] );
		wxLogMessage( "ARGC=%d",        argc );
        wxLogMessage( "CAVASS home=%s", (const char *)Preferences::getHome().c_str() );

        g_printData = new wxPrintData;
        g_pageSetupData = new wxPageSetupDialogData;
        gDefaultFont = wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

        if (argc<2) {
            MainFrame*  f = new MainFrame();
            wxBitmap  bitmap( cavass_splash_xpm );
            //if (bitmap.LoadFile( "cavass-splash.jpg", wxBITMAP_TYPE_JPEG )) {
                new wxSplashScreen( bitmap,
                        wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT,
                        6000, f, wxID_ANY, wxDefaultPosition,
                        wxDefaultSize );
                wxYield();
            //}
//        } else if (argc==4 && strcmp(argv[1],"-mask")==0) {
//            doMask( argc, argv );
        }
        else if (argc==2 && strlen((char *)(const char *)argv[1].c_str())>4 &&
				strcmp((char *)(const char *)argv[1].c_str()+
				strlen((char *)(const char *)argv[1].c_str())-4, ".IM0")==0) {
			wxFileName fn(argv[1]);
			wxFileName gw(fn.GetPath(), "greymap.TMP");
			wxSetWorkingDirectory(fn.GetPath());
			if (!gw.FileExists())
			{
				FILE *fp=fopen((char *)
					(const char *)gw.GetFullPath().c_str(), "wb");
				if (fp)
				{
					fprintf(fp, "750 1250\n0 0\n2\n");
					fclose(fp);
				}
			}
			Segment2dFrame *frame = new Segment2dFrame(true);
			frame->loadFile((const char *)fn.GetFullPath().c_str());
        }
        else {
            for (int i=1; i<argc; i++) {
                cout << "argv[" << i << "]=" << argv[i] << endl;
                MontageFrame*  frame = new MontageFrame();
                frame->loadFile( argv[i].c_str() );
                MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( frame->mCanvas );
                assert( canvas != NULL );
                canvas->setScale( 1.0 );
            }
        }

        //loadRamp();  //for testing the display
        return TRUE;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \todo this never gets called because exit(0) is directly called
     *  in the frames.  this needs to be fixed so that this gets called.
     */
    virtual int OnExit ( void ) {
        puts( "in CavassMain::OnExit()" );
		//delete wxConfigBase::Set( (wxConfigBase*) NULL );
        //return wxApp::OnExit();
        //delete wxConfigBase::Set( (wxConfigBase*) NULL );
		return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  private:
    /** \brief create a frame with a 256x256 gray ramp image.
     */
    void loadRamp ( void ) {
        int*  slice = (int*)malloc( 256*256*sizeof(int) );
        assert( slice != NULL );
        int  i=0;
        for (int y=0; y<256; y++) {
            for (int x=0; x<256; x++) {
                slice[i++] = x;
            }
        }

        MontageFrame*  f = new MontageFrame();
        f->loadData( (char *)"ramp", 256, 256, 1, 1.0, 1.0, 1.0, slice );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief (unused) load the saved configuration.
     */
    void loadConfig ( void ) {
        //wxConfigBase*  pConfig = wxConfigBase::Get();
        //pConfig->SetRecordDefaults();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Determine the physical aspect ratio (width/height) of the
     *  display.
     *  \returns the aspect ratio
     */
    static double getAspectRatio ( void ) {
        int  widthPixels, heightPixels;
        ::wxDisplaySize( &widthPixels, &heightPixels );
        int  mmWidth, mmHeight;
        ::wxDisplaySize( &mmWidth, &mmHeight );
        const double  widthPixelSize  = ((double)mmWidth)  / widthPixels;
        const double  heightPixelSize = ((double)mmHeight) / heightPixels;
        return widthPixelSize / heightPixelSize;
    }

};

// main program
IMPLEMENT_APP( CavassMain )
//----------------------------------------------------------------------
