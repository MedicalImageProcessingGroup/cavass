#include <wx/wx.h>
#include "CButton.h"

/**
 * ctor for custom button for cavass.
 * has no effect on linux gtk!
 * haven't tested on mac yet.
 */
#ifndef __MACH__
CButton::CButton ( wxWindow* parent, wxWindowID id,
              const wxString& label,
              const wxPoint& pos,
              const wxSize& size, long style,
              const wxValidator& validator,
              const wxString& name )
    : wxButton(parent, id, label, pos, size, style, validator, name)
{
    extern void setColor( wxWindow* w );
    setColor( this );
#ifndef USING_GTK
    //save for later
    oldFore = GetForegroundColour();
    oldBack = GetBackgroundColour();
    //be able to handle events
    Bind( wxEVT_ENTER_WINDOW, &CButton::OnMouseEnter, this );
    Bind( wxEVT_LEAVE_WINDOW, &CButton::OnMouseLeave, this );
    //Bind( wxEVT_PAINT, &CButton::paintEvent, this );
#endif
}

void CButton::paintEvent ( wxPaintEvent& evt ) {
//    wxPaintDC dc(this);
//    render(dc);
    std::cout << "here" << std::endl;
}

void CButton::OnMouseEnter ( wxMouseEvent& unused ) {
    // change foreground to black on hover
    SetBackgroundColour( wxColour(255, 255, 0) );  //doesn't work on win
    SetForegroundColour( wxColour(0, 0, 0) );  //black
    Refresh();
}

void CButton::OnMouseLeave ( wxMouseEvent& unused ) {
    // revert to default colors
    SetBackgroundColour( oldBack );
    SetForegroundColour( oldFore );
    Refresh();
}

#if 1
BEGIN_EVENT_TABLE( CButton, wxButton )
    EVT_PAINT( CButton::paintEvent )
END_EVENT_TABLE()
#endif

#endif //ndef __MACH__

