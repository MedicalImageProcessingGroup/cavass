#include <wx/wx.h>
#include "CCheckBox.h"

/**
 * ctor for custom check box for cavass.
 * \todo test on mac and win
 */
CCheckBox::CCheckBox ( wxWindow* parent, wxWindowID id, const wxString& label,
            const wxPoint& pos, const wxSize& size, long style,
            const wxValidator& validator, const wxString& name )
    : wxCheckBox( parent, id, label, pos, size, style, validator, name )
{
    //handle events
    Bind( wxEVT_ENTER_WINDOW, &CCheckBox::OnMouseEnter, this );
    Bind( wxEVT_LEFT_DOWN,    &CCheckBox::OnLeft,       this );
}

/**
 * this function handles checkbox clicks. it changes the state of the checkbox.
 * as a result, the event handler should NOT flip the state of the checkbox, but
 * should check the state of the checkbox instead.
 *     //doLayout( e.IsChecked() );       //don't do this
 *     doLayout( m_layout->GetValue() );  //do this instead
 *
 * if we handle mouse enter events, it turns out that we must also handle left
 * click events as well. otherwise, left clicks are ignored!
 * @param unused is not used
 */
void CCheckBox::OnLeft ( wxMouseEvent& unused ) {
    SetValue( !GetValue() );
    //we need to call event handler (ex., OnLayout).
    wxCommandEvent event( wxEVT_COMMAND_CHECKBOX_CLICKED, GetId() );
    //wxCommandEvent event( wxEVT_CHECKBOX, GetId() );  //works as well
    wxEvtHandler* handler = GetEventHandler();
    handler->ProcessEvent( event );
    Refresh();
}

/**
 * do nothing. (otherwise, the default handler will change the background
 * color to light gray and the lighter text will become unreadable.)
 */
void CCheckBox::OnMouseEnter ( wxMouseEvent& unused ) { }

