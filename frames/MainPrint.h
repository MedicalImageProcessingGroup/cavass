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
/**
 * \file   MainPrint.h
 * \brief  MainPrint definition and implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MainPrint_h
#define __MainPrint_h

#include  "wx/print.h"
#include  "wx/printdlg.h"
#include  "MainCanvas.h"

/** \brief MainPrint definition and implementation.  This class is used
 *         to print the canvas contents.
 */
class MainPrint: public wxPrintout {
  protected:
    MainCanvas*  mCanvas;

  public:
    /** \brief MainPrint ctor.
     *  \param title is the desired title.
     *  \param mc is the canvas which will be used to draw onto the printed page.
     */
    MainPrint ( const wxChar* const title = _T("CAVASS"), MainCanvas* mc = NULL )
        : wxPrintout(title), mCanvas(mc)
    {
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Print the specified page by using the canvas to draw it's contents.
     *  \param page is the page number to draw.
     *  \returns true if the page was drawn; false otherwise.
     */
    bool OnPrintPage ( int page ) {
        wxDC*  dc = GetDC();
        if (!dc)    return false;
        dc->SetBackground( *wxWHITE_BRUSH );
        dc->Clear();
        dc->SetFont( gDefaultFont );
        dc->SetBackgroundMode( wxTRANSPARENT );

        if (mCanvas!=NULL) {
            //determine scaling factor
            int  canvasW, canvasH;
            mCanvas->GetSize( &canvasW, &canvasH );
            int  dcW, dcH;
            dc->GetSize( &dcW, &dcH );
            double  scaleX = ((double)dcW) / canvasW;
            double  scaleY = ((double)dcH) / canvasH;
            double  actualScale = wxMin( scaleX, scaleY );
            dc->SetUserScale( actualScale, actualScale );
            //center
            long  posX = (long) ((dcW - (canvasW*actualScale)) / 2);
            long  posY = (long) ((dcH - (canvasH*actualScale)) / 2);
            dc->SetDeviceOrigin( posX, posY );
            //dc->SetDeviceOrigin( 0, 0 );
            mCanvas->paint( dc );
        }

        dc->SetDeviceOrigin( 0, 0 );
        dc->SetUserScale( 1.0, 1.0 );
        dc->SetTextBackground( *wxWHITE );
        dc->SetTextForeground( *wxBLACK );
        assert( mCanvas!=NULL );
        MainFrame*  main = mCanvas->getParentFrame();
        assert( main!=NULL );
        wxChar  buf[200];
        wxSprintf( buf, "%s: Page %d", (const char *)main->mModuleName.c_str(), page );
        dc->DrawText( buf, 25, 25 );

        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Determines if the specified page can be printed.
     *  \param pageNum is the possible page number to be printed.
     *  \returns true if the page can be printed; false otherwise.
     */
    bool HasPage ( int pageNum ) {
        return (pageNum==1);
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Called when starting to print.
     *  \param startPage is the first page to be printed.
     *  \param endPage is the last page to be printed.
     *  \returns true if the specified page range can be printed; false otherwise.
     */
    bool OnBeginDocument ( int startPage, int endPage ) {
        if (!wxPrintout::OnBeginDocument(startPage, endPage))
            return false;
        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief This function indicates page information.
     *  \param minPage is a ptr to where the min page should be stored.
     *  \param maxPage is a ptr to where the max page should be stored.
     *  \param selPageFrom is a ptr to where the first selected page should be stored.
     *  \param selPageTo is a ptr to where the last selected page should be stored.
     */
    void GetPageInfo ( int* minPage, int* maxPage, int* selPageFrom,
                       int* selPageTo )
    {
        *minPage     = 1;
        *maxPage     = 1;
        *selPageFrom = 1;
        *selPageTo   = 1;
    }

};

#endif
//======================================================================

