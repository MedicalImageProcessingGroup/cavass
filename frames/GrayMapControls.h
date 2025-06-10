/*
  Copyright 1993-2013, 2015 Medical Image Processing Group
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
 * \file   GrayMapControls.h
 * \brief  Definition and implementation of GrayMapControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#pragma once

#include <wx/persist.h>
#include <wx/persist/toplevel.h>
#include "CCheckBox.h"
/**
 * \brief Definition and implementation of GrayMapControls class.
 *
 * Gray map controls consist of a control box with window level and width
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class GrayMapControls {
    wxSizer*          mBottomSizer;  ///< DO NOT DELETE in dtor!
    wxSlider*         mLevel;        ///< window level slider
//    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
//    wxFlexGridSizer*  mContrastSizer;
//    wxStaticText*     mSt0;          ///< level slider title
//    wxStaticText*     mSt1;          ///< width slider title
//    wxStaticText*     mSt2;          ///< invert checkbox title
    wxSlider*         mWidth;        ///< window width slider
//    wxButton*         mLung;         ///< CT lung button
//    wxButton*         mSoftTissue;   ///< CT soft tissue button
//    wxButton*         mBone;         ///< CT bone button
//    wxButton*         mPET;          ///< PET button
public:
//    static const string levelGroupDefault, widthGroupDefault, invertGroupDefault;
//    static const string levelNameDefault,  widthNameDefault,  invertNameDefault;
    CCheckBox* m_cb_invert; ///< invert the gray map checkbox

    GrayMapControls ( wxPanel* cp, wxSizer* bottomSizer, const char* title,
                      int currentLevel, int currentWidth,int max, bool currentInvert,
                      int levelSliderID, int widthSliderID, int invertID,
                      int lungID=wxID_ANY, int softTissueID=wxID_ANY, int boneID=wxID_ANY,
                      int PETID=wxID_ANY, int min=0 );

    void update_sliders ( int newLevel, int newWidth, bool invert=false );

    ~GrayMapControls ( );
};

