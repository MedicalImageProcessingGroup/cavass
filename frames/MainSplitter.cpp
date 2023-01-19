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
/**
 * \file   MainSplitter.cpp
 * \brief  MainSplitter class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( MainSplitter, wxSplitterWindow )
BEGIN_EVENT_TABLE(        MainSplitter, wxSplitterWindow )
    EVT_SPLITTER_SASH_POS_CHANGED(  wxID_ANY, MainSplitter::OnPositionChanged  )
    EVT_SPLITTER_SASH_POS_CHANGING( wxID_ANY, MainSplitter::OnPositionChanging )
    EVT_SPLITTER_DCLICK(            wxID_ANY, MainSplitter::OnDClick           )
    EVT_SPLITTER_UNSPLIT(           wxID_ANY, MainSplitter::OnUnsplitEvent     )
END_EVENT_TABLE()
//======================================================================

