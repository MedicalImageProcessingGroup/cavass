#pragma once

#if 0

#include <wx/odcombo.h>

class CComboBox : public wxOwnerDrawnComboBox {
public:
    CComboBox ( wxWindow* parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxComboBoxNameStr  )
        : wxOwnerDrawnComboBox( parent, id, value, pos, size, choices, style, validator, name )
    {
        SetBackgroundColour( wxColour(255, 0, 0) );    //ignored on gtk
        SetForegroundColour( wxColour(255, 0, 255) );  //ignored on gtk
    }
#if 0
    void OnDrawBackground ( wxDC& dc, const wxRect& rect, int item, int flags )
    const override
    {
        // If item is selected or even, or we are painting the
        // combo control itself, use the default rendering.
        if ( (flags & (wxODCB_PAINTING_CONTROL|wxODCB_PAINTING_SELECTED)) ||
             (item & 1) == 0 )
        {
            wxOwnerDrawnComboBox::OnDrawBackground(dc,rect,item,flags);
            return;
        }

        // Otherwise, draw every other background with different colour.
        //wxColour bgCol(240,240,250);
        wxColour bgCol( 40,40,250 );
        dc.SetBrush( wxBrush(bgCol) );
        dc.SetPen( wxPen(bgCol) );
        dc.DrawRectangle( rect );
    }
#endif
    void OnDrawItem ( wxDC& dc, const wxRect& rect, int item, int flags )
    const override
    {
        cout << "item=" << item << " flags=" << flags << endl;

        if (item == wxNOT_FOUND)    return;
        if (flags & wxODCB_PAINTING_CONTROL) {
            cout << "wxODCB_PAINTING_CONTROL" << endl;
            return;
        }
        if (flags & wxODCB_PAINTING_SELECTED) {
            cout << "wxODCB_PAINTING_SELECTED" << endl;
            return;
        }

        //set background color
        dc.SetBrush( wxBrush(wxColour(200, 0, 0)) );
        dc.SetPen( *wxTRANSPARENT_PEN );
        dc.DrawRectangle( rect );

        //draw text
        wxString text = GetString( item );
        dc.SetTextForeground( GetForegroundColour() );
        dc.DrawText( text, rect.x + 2,
            rect.y
                + (rect.height - dc.GetTextExtent(text).GetHeight()) / 2 );
    }
    
    wxCoord OnMeasureItem ( int item ) const {
         return GetTextExtent( GetString(item) ).GetHeight() + 6;
    }

    wxString GetStringSelection ( ) const override {
        return wxItemContainerImmutable::GetStringSelection();
        //return wxTextEntryBase::GetStringSelection();  //doesn't work
    }

};

#else
#define CComboBox wxComboBox
#endif

