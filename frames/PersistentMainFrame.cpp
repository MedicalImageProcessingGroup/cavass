#include "cavass.h"
#include "ExampleCanvas.h"
#include "ExampleFrame.h"
#include "PersistentMainFrame.h"
//------------------------------------------------------------------------
//const string PersistentExampleFrame::group = "Persistent_Options/ExampleFrame/frame";

//general window items
const string PersistentMainFrame::w_isIconized  = "w_isIconized";
const string PersistentMainFrame::w_isMaximized = "w_isMaximized";
const string PersistentMainFrame::w_pos_x       = "w_pos_x";
const string PersistentMainFrame::w_pos_y       = "w_pos_y";
const string PersistentMainFrame::w_sashPos     = "w_sashPos";
const string PersistentMainFrame::w_size_h      = "w_size_h";
const string PersistentMainFrame::w_size_w      = "w_size_w";
//------------------------------------------------------------------------
/**
 * save the widget's properties (in ~/.cavass.ini).
 * note that multiple values are saved below.
 * this function should mimic Restore().
 */
void PersistentMainFrame::Save ( ) const {
    //save the slider's properties
    if (verbose) cout << "PersistentMainFrame::Save()" << endl;
    auto w = Get();
    assert(w != nullptr);

    //save general window values
    bool isIconizedVal = w->IsIconized();
    SaveValue(w_isIconized, isIconizedVal ? 1 : 0);

    bool isMaximizedVal = w->IsMaximized();
    SaveValue(w_isMaximized, isMaximizedVal ? 1 : 0);

    wxPoint pos = w->GetPosition();
    SaveValue(w_pos_x, pos.x);
    SaveValue(w_pos_y, pos.y);

    wxSize size = w->GetSize();
    SaveValue(w_size_h, size.GetHeight());
    SaveValue(w_size_w, size.GetWidth());

    output("Save(): ", "returning.");
}
//------------------------------------------------------------------------
/**
 * restore the widget's properties (from ~/.cavass.ini).
 * note that multiple values are restored below.
 * this function should mimic Save().
 */
bool PersistentMainFrame::Restore ( ) {
    if (verbose) cout << "PersistentMainFrame::Restore() \n";
    //restore the widget's properties
    auto w = Get();
    bool ok;

    //restore general window values

    //isIconized
    int isIconizedVal;
    ok = RestoreValue(w_isIconized, &isIconizedVal);
    if (!ok) {
        cerr << "the error messages below normally appear the very first time that this frame is instantiated." << endl;
        cerr << "restore w_isIconized failed." << endl;
    } else {
#if defined(__APPLE__) || defined(__MACH__)
        w->Iconize(isIconizedVal == 1);  //ok for linux, but not mac
#elif !defined(WIN32)
        w->SetIconizeState(isIconizedVal == 1);  //ok for linux, but not mac
#endif
    }

    //isMaximized
    int isMaximizedVal;
    ok = RestoreValue(w_isMaximized, &isMaximizedVal);
    if (!ok) cerr << "restore w_isMaximized failed." << endl;
    else w->Maximize(isMaximizedVal == 1);

    //position (two values: x and y)
    wxPoint pos;
    ok = RestoreValue(w_pos_x, &pos.x);
    if (!ok) {
        cerr << "restore w_pos_x failed." << endl;
    } else {
        ok = RestoreValue(w_pos_y, &pos.y);
        if (!ok) cerr << "restore w_pos_y failed." << endl;
        else w->SetPosition(pos);
    }

    //size (two values: h and w)
    wxSize size;
    ok = RestoreValue(w_size_h, &size.y);
    if (!ok) cerr << "restore w_size_h failed." << endl;
    else {
        ok = RestoreValue(w_size_w, &size.x);
        if (!ok) cerr << "restore w_size_w failed." << endl;
        w->SetSize(size);
        cout << "setting size to (w,h)=" << size.GetWidth() << "," << size.GetHeight() << endl;
        cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
    }

    output("Restore(): ", "returning true.");
    cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
    return true;
}
//------------------------------------------------------------------------
