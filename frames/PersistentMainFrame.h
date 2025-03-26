#pragma once

#include <wx/persist.h>
#include <wx/persist/toplevel.h>

class PersistentMainFrame : public wxPersistentWindow< MainFrame > {
public:
    static const bool verbose = true;  //true for debugging

    //static const string group;

    //general window items
    static const string w_isIconized;
    static const string w_isMaximized;
    static const string w_pos_x;
    static const string w_pos_y;
    static const string w_size_h;
    static const string w_size_w;
    static const string w_sashPos;
    //------------------------------------------------------------------------
    explicit PersistentMainFrame ( MainFrame* w )
    : wxPersistentWindow<MainFrame>( w )
    {
        output( "PersistentMainFrame(MainFrame): " );
    }
    //------------------------------------------------------------------------
    [[nodiscard]] wxString GetKind ( ) const override {
        output( "GetKind(): ", "returning \"MainFrame\"" );
        return "MainFrame";
    }
    //------------------------------------------------------------------------
    void Save ( ) const override;
    bool Restore ( ) override;
private:
    /** debugging output.
     *  \todo this should be a pretty-print friend function for MainFrame.
     */
    void output ( const string& prefix="", const string& postfix="" ) const {
        if (!verbose)    return;
        auto w = Get();
        assert( w != nullptr );
        cout << prefix << "0x" << hex << w << dec << " (" << w->GetName() << ")"
             << " isIconized="  << w->IsIconized()
             << " isMaximized=" << w->IsMaximized()
             << " position="    << w->GetPosition().x << "," << w->GetPosition().y;
        if (w->mSplitter != nullptr)
            cout << " sashPos=" << w->mSplitter->GetSashPosition();
        cout << " size="        << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight();
    }
};
//----------------------------------------------------------------------------
inline wxPersistentObject* wxCreatePersistentObject ( MainFrame* w ) {
    if (PersistentMainFrame::verbose)
        cout << "wxCreatePersistentObject( MainFrame=0x" << hex << w << dec
             << " (" << w->GetName() << ") )" << endl;
    return new PersistentMainFrame( w );
}
//------------------------------------------------------------------------
