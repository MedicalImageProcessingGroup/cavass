#pragma once
/**
 * custom check box class definition for cavass. it simply disables background
 * color changes on mouseenter (which requires that one also handle left
 * clicks and propagate them to the event handler).
 * \todo test on mac and win
 * (getting this to work has been a pain!)
 */
class CCheckBox : public wxCheckBox {

public:
     CCheckBox ( wxWindow* parent, wxWindowID id, const wxString& label,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize, long style = 0,
                 const wxValidator& validator = wxDefaultValidator,
                 const wxString& name = wxCheckBoxNameStr );

private:
    void OnLeft       ( wxMouseEvent& unused );
    void OnMouseEnter ( wxMouseEvent& unused );

};

