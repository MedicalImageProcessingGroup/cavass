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
// GrayScaleTransform
//----------------------------------------------------------------------
#ifndef __GrayScaleTransform_h
#define __GrayScaleTransform_h

class MainFrame;
class MontageFrame;

class GrayScaleTransform : public wxDialog {
  protected:
    MontageFrame*          m_parent;

//    static const int  sMaxSliderValue;
    static const int  sWidth;
    static const int  sHeight;
    unsigned char*    mGraph;  //rgb data
    wxImage*          mImage;
    wxBitmap*         mBitmap;
    wxStaticBitmap*   mSbm;

    int*              mTransform;

    wxButton*         m_ok;
    wxButton*         m_flash;
    wxButton*         m_invert;
    wxButton*         m_cancel;

    bool              mInvert;
    unsigned char*    mLUTCopy;

    enum { ID_FLASH, ID_INVERT, ID_NEXT };

    virtual void doTransform ( void ) = 0;

  public:
    GrayScaleTransform ( wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize,
        long style=wxDEFAULT_DIALOG_STYLE, const wxString& name="dialogBox" )
      : wxDialog ( parent, id, title, pos, size, style, name )
    {
    }
    void OnDoSave            ( wxCommandEvent& e );
    void OnDoFlash           ( wxCommandEvent& e );
    void OnDoInvert          ( wxCommandEvent& e );
    void OnDoDismiss         ( wxCommandEvent& e );

  private:
    DECLARE_DYNAMIC_CLASS(GrayScaleTransform)
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
