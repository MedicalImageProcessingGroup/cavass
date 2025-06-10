#include "cavass.h"
#include "SetIndexControls.h"

/** \brief SetIndexControls ctor.
 *  \param cp is the parent panel for these controls.
 *  \param bottomSizer is the existing sizer to which this the sizer associated
 *         with these controls will be added (and deleted upon destruction).
 *  \param title          is the title for the control area/box.
 *  \param currentSlice   is the current slice number.
 *  \param numberOfSlices is the number of slices.
 *  \param sliceSliderID  is the ID of the slice slider.
 *  \param scaleSliderID  is the ID of the scale slider.
 *  \param currentScale   is the current scale value.
 *  \param matchIndexID   is the match index button.
 */
SetIndexControls::SetIndexControls( wxPanel* cp, wxSizer* bottomSizer,
    const char* title, int currentSlice, int numberOfSlices, int sliceSliderID,
    int scaleSliderID, double currentScale, int overlayID, int matchIndexID )
{
    mSliceCount  = numberOfSlices;
    mBottomSizer = bottomSizer;
    mSetIndexBox = new wxStaticBox(cp, -1, title);
    //mSetIndexBox->SetSize( cp->GetSize().x/3, 0 );
    //::setSliderBoxColor(mSetIndexBox);
    ::setBoxColor( mSetIndexBox );
    mSetIndexSizer = new wxStaticBoxSizer(mSetIndexBox, wxHORIZONTAL);
    //mSetIndexSizer->SetMinSize(cp->GetSize().x/3, 0);
    mFgs = new wxFlexGridSizer(2, 0, 5);  //2 cols,vgap,hgap
    //mFgs->SetMinSize(controlsWidth * 2, 0);
    mFgs->SetMinSize(cp->GetSize().x/5, 0);
    mFgs->AddGrowableCol(1);

    //slice number
    if (numberOfSlices > 1) {
        mSliceST = new wxStaticText(cp, -1, "dimension 3:");
        ::setColor(mSliceST);
        mFgs->Add(mSliceST, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        mSliceNumber = new wxSlider(cp, sliceSliderID, currentSlice + 1, 1,
                                    numberOfSlices, wxDefaultPosition, wxDefaultSize,
                                    wxSL_HORIZONTAL | wxSL_MIN_MAX_LABELS | wxSL_VALUE_LABEL);
        mSliceNumber->SetName("setindex.slice");
        ::setColor(mSliceNumber);
        mSliceNumber->SetPageSize(5);
        mFgs->Add(mSliceNumber, 0, wxALL | wxEXPAND);  //wxGROW|wxLEFT|wxRIGHT );
    } else {
        mSliceST = nullptr;
        mSliceNumber = nullptr;
    }

    //scale (note: slider w/ a real number needs to be implemented below)
    mScaleText0 = new wxStaticText(cp, -1, "scale:");
    ::setColor(mScaleText0);
    mFgs->Add( mScaleText0, 0, wxALIGN_RIGHT );

    mFgsSlider = new wxFlexGridSizer(3, 0, 0);  //3 cols,vgap,hgap
    mFgsSlider->AddGrowableCol(1);

#ifdef  SetIndexControlsZoom
    mZoomOut = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomOut );
            mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
#else
    mZoomOut = nullptr;
    mFgsSlider->AddStretchSpacer();
#endif
    m_old_scale_value = 100;
    mScaleZoom = 1.0;
    mScale = new wxSlider(cp, scaleSliderID,
                          (int) (currentScale * 100 + 0.5), scaleIMin, scaleIMax,
                          wxDefaultPosition, wxDefaultSize,
            //wxSL_HORIZONTAL|wxSL_AUTOTICKS );
            //wxSL_HORIZONTAL|wxSL_MIN_MAX_LABELS|wxSL_VALUE_LABEL );  //bad (wrong value on top)
                          wxSL_HORIZONTAL | wxSL_TOP);
    mScale->SetName("setindex.scale");
    ::setColor(mScale);
    mScale->SetPageSize(5);
    mFgsSlider->Add(mScale, 0, wxALL | wxEXPAND);  //wxGROW|wxLEFT|wxRIGHT );
#ifdef  SetIndexControlsZoom
    mZoomIn = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomIn );
            mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
#else
    mZoomIn = nullptr;
    mFgsSlider->AddStretchSpacer();
#endif
    wxString s = wxString::Format("%5.2f", scaleIMin / 100.0);
    mScaleText1 = new wxStaticText(cp, -1, s);
    ::setColor(mScaleText1);
    mFgsSlider->Add(mScaleText1, 0, wxALIGN_RIGHT);

    s = wxString::Format("%5.2f", currentScale);
    mScaleText2 = new wxStaticText(cp, -1, s);
    ::setColor(mScaleText2);
    mFgsSlider->Add(mScaleText2, 0, wxALIGN_CENTER);

    s = wxString::Format("%5.2f", scaleIMax / 100.0);
    mScaleText3 = new wxStaticText(cp, -1, s);
    ::setColor(mScaleText3);
    mFgsSlider->Add(mScaleText3, 0, wxALIGN_LEFT);

    mFgs->Add(mFgsSlider, 0, wxGROW | wxLEFT | wxRIGHT);

    mSetIndexSizer->Add(mFgs, 0, wxGROW | wxALL, 5);
    // - - - - - - - - - -
    mOverlayST = nullptr;
    mCB = nullptr;
    mMatchIndex = nullptr;
    if (overlayID != wxID_ANY || matchIndexID != wxID_ANY) {
        mFgs->AddSpacer(ButtonOffset);
        mFgs->AddSpacer(ButtonOffset);
        wxSizer *gsLastRow = new wxGridSizer(2, 0, 0);  //2 cols,vgap,hgap
        if (overlayID != wxID_ANY) {
            mOverlayST = new wxStaticText(cp, -1, "labels:");
            ::setColor(mOverlayST);
            mFgs->Add(mOverlayST, 0, wxALIGN_RIGHT);

            mCB = new wxCheckBox(cp, overlayID, "on/off");
            ::setColor(mCB);
            mCB->SetValue(1);
            gsLastRow->Add(mCB, 0, wxALIGN_LEFT);
        }
        if (matchIndexID != wxID_ANY) {
            mMatchIndex = new wxButton(cp, matchIndexID, "MatchIndex");
            ::setColor(mMatchIndex);
            gsLastRow->Add(mMatchIndex, 0, wxALIGN_RIGHT);
        }
        mFgs->Add(gsLastRow);
    }

#ifdef wxUSE_TOOLTIPS
#ifndef __WXX11__
    if (mCB != nullptr)
        mCB->SetToolTip("toggle image overlay on & off");
    if (mOverlayST != nullptr)
        mOverlayST->SetToolTip("toggle image overlay on & off");
    if (mSliceNumber != nullptr)
        mSliceNumber->SetToolTip("slice number");
    mScale->SetToolTip("scale");
#endif
#endif

    mBottomSizer->Prepend(mSetIndexSizer, 0, wxGROW|wxALL, 5);  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
}
