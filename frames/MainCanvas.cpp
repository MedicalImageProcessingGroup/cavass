/*
  Copyright 1993-2010, 2016-2017 Medical Image Processing Group
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
 * \file   MainCanvas.cpp
 * \brief  MainCanvas class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "cavass.xpm"
#include  "tiffio.h"

using namespace std;
//----------------------------------------------------------------------
MainCanvas::MainCanvas ( void ) {
}
//----------------------------------------------------------------------
MainCanvas::MainCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : wxPanel ( parent, id, pos, size )
{
    m_parent_frame = parent_frame;
    mCavassData = NULL;
    if (Preferences::getCustomAppearance())
        SetBackgroundColour( wxColour(DkBlue) );
    else
        SetBackgroundColour( *wxBLACK );
    SetForegroundColour(*wxWHITE);
    m_backgroundBitmap = wxBitmap( cavass_xpm );
    m_backgroundImage  = m_backgroundBitmap.ConvertToImage();
    m_backgroundLoaded = true;
}
//----------------------------------------------------------------------
MainCanvas::~MainCanvas ( void ) {
    cout << "MainCanvas::~MainCanvas" << endl;
    wxLogMessage( "MainCanvas::~MainCanvas" );
    while (mCavassData!=NULL) {
        CavassData*  tmp = mCavassData;
        mCavassData = mCavassData->mNext;
        delete tmp;
    }
}
//----------------------------------------------------------------------
void MainCanvas::OnPaint ( wxPaintEvent& e ) {
    wxMemoryDC  m;
    int  w, h;
    GetSize( &w, &h );
    wxBitmap  bitmap( w, h );
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
    
    wxPaintDC  dc( this );
    PrepareDC( dc );
    //dc.BeginDrawing();
    dc.Blit( 0, 0, w, h, &m, 0, 0 );  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
    //dc.EndDrawing();
}
//----------------------------------------------------------------------
void MainCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    if (m_backgroundLoaded) {
        int  w, h;
        dc->GetSize( &w, &h );
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
void MainCanvas::saveContents ( char* fname ) {
	if (CavassData::endsWith(fname,".tif") || CavassData::endsWith(fname,".tiff")) {
        appendContents( fname, true );
        return;
    }
    wxMemoryDC  m;
	//instead of using the size of the canvas (which may be split),
	// we'll use the size of the frame/window.
	int  w, h;
	m_parent_frame->GetSize( &w, &h );    //GetSize( &w, &h );
    wxBitmap  bitmap( w, h );
    m.SelectObject( bitmap );
    //clear background
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
    wxImage  image = bitmap.ConvertToImage();
    /*
    valid types:
        wxBITMAP_TYPE_BMP
        wxBITMAP_TYPE_JPEG
        wxBITMAP_TYPE_PNG
        wxBITMAP_TYPE_PCX
        wxBITMAP_TYPE_PNM
        wxBITMAP_TYPE_TIF
        wxBITMAP_TYPE_XPM
    */
    bool  result = false;
    if (CavassData::endsWith(fname,".bmp"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_BMP );
    else if (CavassData::endsWith(fname,".jpg") || CavassData::endsWith(fname,".jpeg"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_JPEG );
    else if (CavassData::endsWith(fname,".png"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_PNG );
    else if (CavassData::endsWith(fname,".pcx"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_PCX );
    else if (CavassData::endsWith(fname,".pnm"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_PNM );
    else if (CavassData::endsWith(fname,".tif") || CavassData::endsWith(fname,".tiff"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_TIF );
    else if (CavassData::endsWith(fname,".xpm"))
        result = image.SaveFile( fname, wxBITMAP_TYPE_XPM );
	else {
		wxMessageBox( "Invalid save type.\nValid types are:\nbmp, jpg, png, pcx, pnm, tif, and xpm.",
            "Sorry...", wxOK | wxICON_ERROR );
        return;
    }
    if (!result) {
        wxMessageBox( "Error saving image data file.", "Sorry...", wxOK | wxICON_ERROR );
    }
}
//----------------------------------------------------------------------
void MainCanvas::appendContents ( char* fname, bool overwrite ) {
    if (!CavassData::endsWith(fname,".tif") && !CavassData::endsWith(fname,".tiff")) {
        wxMessageBox( "Invalid save type (output file type).\n\nOnly TIFF (.tif or .tiff) supports append (multiple images in one file).",
            "Sorry...", wxOK | wxICON_ERROR );
        return;
    }
	if (overwrite && wxFile::Exists(fname) &&
		    wxMessageBox("Overwrite?", "Confirm", wxOK|wxCANCEL)!=wxOK)
		return;

    wxMemoryDC  m;
	//instead of using the size of the canvas (which may be split),
	// we'll use the size of the frame/window.
	int  w, h;
	m_parent_frame->GetSize( &w, &h );    //GetSize( &w, &h );
    wxBitmap  bitmap( w, h );
    m.SelectObject( bitmap );
    //clear background
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
    wxImage  image = bitmap.ConvertToImage();
    //note:  only tiff supports multiple images
    TIFF*  tif = NULL;
    if (overwrite)    tif = TIFFOpen( fname, "wb" );
    else              tif = TIFFOpen( fname, "ab" );
    if (tif!=NULL) {
        TIFFSetField( tif, TIFFTAG_IMAGEWIDTH,      image.GetWidth()    );
        TIFFSetField( tif, TIFFTAG_IMAGELENGTH,     image.GetHeight()   );
        TIFFSetField( tif, TIFFTAG_BITSPERSAMPLE,   8                   );
        TIFFSetField( tif, TIFFTAG_SAMPLESPERPIXEL, 3                   );
        TIFFSetField( tif, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT );
        TIFFSetField( tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG );
        TIFFSetField( tif, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB     );
        //TIFFSetField( tif, TIFFTAG_COMPRESSION,     COMPRESSION_LZW     );

        unsigned char*  data = image.GetData();
        for (int row=0; row<image.GetHeight(); row++) {
            TIFFWriteScanline( tif, &data[row*image.GetWidth()*3], row );
        }

        TIFFWriteDirectory( tif );
        TIFFClose( tif );
    } else {
        wxMessageBox( "Error saving image data file.", "Sorry...",
                      wxOK | wxICON_ERROR );
    }
}
//----------------------------------------------------------------------
/** \brief This function is called when the size of the window/viewing 
 *  area changes.
 */
void MainCanvas::OnSize ( wxSizeEvent& e ) {
    reload();
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( MainCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( MainCanvas, wxPanel )
    EVT_PAINT(            MainCanvas::OnPaint           )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_SIZE(             MainCanvas::OnSize            )
END_EVENT_TABLE()
//======================================================================

