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
 * \file   MontagePrint.h
 * \brief  MontagePrint definition and implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MontagePrint_h
#define __MontagePrint_h

#include  "wx/print.h"
#include  "wx/printdlg.h"
#include  "MontageCanvas.h"
/** \brief MontagePrint definition and implementation.
 */
class MontagePrint : public wxPrintout {
  protected:
    MontageCanvas*  mCanvas;

  public:
    MontagePrint ( const wxChar* const title = _T("Overlay"), MontageCanvas* mc = NULL )
        : wxPrintout(title), mCanvas(mc)
    {
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool OnPrintPage ( int page ) {
        wxDC*  dc = GetDC();
        if (!dc)    return false;

        if (mCanvas!=NULL) {
            //get the size of the canvas in pixels
            int  cw, ch;
            mCanvas->GetSize( &cw, &ch );
            //You might use THIS code if you were scaling graphics of 
            // known size to fit on the page.
            
            //50 device units margin
            const int  marginX=50, marginY=50;
            // Add the margin to the graphic size
            const double  maxX = cw + 2 * marginX;
            const double  maxY = ch + 2 * marginY;
            
            //get the size of the DC in pixels
            int  w, h;
            dc->GetSize( &w, &h );
            // Calculate a suitable scaling factor
            const double  scaleX = w / maxX;
            const double  scaleY = h / maxY;
            
            // Use x or y scaling factor, whichever fits on the DC
            const double  actualScale = wxMin( scaleX, scaleY );
            
            // Calculate the position on the DC for centering the graphic
            double  posX = (w - (cw*actualScale)) / 2.0;
            double  posY = (h - (ch*actualScale)) / 2.0;
            
            // Set the scale and origin
            dc->SetUserScale( actualScale, actualScale );
            dc->SetDeviceOrigin( (long)posX, (long)posY );
            mCanvas->paint( dc );
        }

        dc->SetDeviceOrigin( 0, 0 );
        dc->SetUserScale( 1.0, 1.0 );

        wxChar  buf[200];
        wxSprintf( buf, wxT("Page %d"), page );
        dc->SetTextBackground( *wxWHITE );
        dc->SetTextForeground( *wxBLACK );
        dc->DrawText( buf, 10, 10 );
        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool HasPage ( int pageNum ) {
        return (pageNum == 1);
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool OnBeginDocument ( int startPage, int endPage ) {
        if (!wxPrintout::OnBeginDocument(startPage, endPage))
            return false;
        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void GetPageInfo ( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
    {
        *minPage     = 1;
        *maxPage     = 1;
        *selPageFrom = 1;
        *selPageTo   = 1;
    }

};

#endif
//======================================================================
