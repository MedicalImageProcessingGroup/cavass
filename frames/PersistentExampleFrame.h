#pragma once

#include <wx/persist.h>
#include <wx/persist/toplevel.h>

class PersistentExampleFrame : public wxPersistentWindow< ExampleFrame > {
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
    //canvas items
    static const string c_slice;
    static const string c_t_x;
    static const string c_t_y;
    //graymap items
    static const string gm_visible;
    static const string gm_invert;
    static const string gm_level;
    static const string gm_width;
    //------------------------------------------------------------------------
    explicit PersistentExampleFrame ( ExampleFrame* w )
        : wxPersistentWindow<ExampleFrame>( w )
    {
        output( "PersistentExampleFrame(ExampleFrame): " );
    }
    //------------------------------------------------------------------------
    [[nodiscard]] wxString GetKind ( ) const override {
        output( "GetKind(): ", "returning \"ExampleFrame\"" );
        return "ExampleFrame";
    }
    //------------------------------------------------------------------------
    void Save ( ) const override;
    bool Restore ( ) override;
    void restoreControlSettings ( );
    //------------------------------------------------------------------------
private:
    /** debugging output.
     *  \todo this should be a pretty-print friend function for ExampleFrame.
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

        //canvas specific items
        auto canvas = dynamic_cast<ExampleCanvas*>( w->mCanvas );
        if (canvas != nullptr && canvas->mCavassData != nullptr) {
            cout << " invert=" << canvas->getInvert( 0 )
                 << " level="  << canvas->getCenter( 0 )
                 << " width="  << canvas->getWidth( 0 )
                 << " slice="  << canvas->getSliceNo( 0 );
        }
        if (canvas == nullptr)
            cout << " canvas=null";
        else if (canvas->mCavassData == nullptr)
            cout << " canvas cavass data=null";

        cout << " " << postfix << endl;
    }
};
//----------------------------------------------------------------------------
inline wxPersistentObject* wxCreatePersistentObject ( ExampleFrame* w ) {
    if (PersistentExampleFrame::verbose)
        cout << "wxCreatePersistentObject( ExampleFrame=0x" << hex << w << dec
             << " (" << w->GetName() << ") )" << endl;
    return new PersistentExampleFrame( w );
}
//------------------------------------------------------------------------
