/*
  Copyright 1993-2013, 2021 Medical Image Processing Group
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
 * \file   MainFileDropTarget.h
 * \brief  MainFileDropTarget class definition and implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MainFileDropTarget_h
#define __MainFileDropTarget_h

#include  "wx/dnd.h"
#include  "MontageFrame.h"

#if wxUSE_DRAG_AND_DROP
/** \brief helper class to serve as the drop target when files are
 *  dragged-n-dropped nto the application.
 *  \todo  Currently, all files are dropped into montage.  It might
 *  be better to consider the file name extension and/or number of
 *  files (for example, 2 IM0s might be dropped into overlay instead
 *  of montage).
 */
class MainFileDropTarget : public wxFileDropTarget {
  public:
    virtual bool OnDropFiles ( wxCoord x, wxCoord y,
                               const wxArrayString& filenames ) {
        cout << "MainFileDropTarget::OnDropFiles" << endl;
        for (int i=0; i<(int)filenames.GetCount(); i++) {
            cout << "    " << filenames[i] << endl;
            MontageFrame*  f = new MontageFrame();
            f->loadFile( filenames[i].c_str() );
        }
        return true;
    }
};
#endif

#endif
//----------------------------------------------------------------------
