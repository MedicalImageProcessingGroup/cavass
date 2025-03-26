#pragma once
/**
 * custom button class definition for cavass.
 * has no effect on linux gtk!
 * haven't tested on mac yet.
 * probably need something based on this instead:
 * https://forums.wxwidgets.org/viewtopic.php?t=42202
 */
class CButton : public wxButton {

public:
    CButton ( wxWindow* parent, wxWindowID id,
              const wxString& label = wxEmptyString,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = 0,
              const wxValidator& validator = wxDefaultValidator,
              const wxString& name = wxButtonNameStr );

private:
    wxColour oldFore;   ///< save old foreground color
    wxColour oldBack;   ///< save old background color

    void OnMouseEnter ( wxMouseEvent& unused );
    void OnMouseLeave ( wxMouseEvent& unused );
    void paintEvent ( wxPaintEvent& evt );

    DECLARE_EVENT_TABLE()
};

