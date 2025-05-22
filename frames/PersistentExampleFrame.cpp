#include "cavass.h"
#include "ExampleCanvas.h"
#include "ExampleFrame.h"
#include "PersistentExampleFrame.h"
//------------------------------------------------------------------------
//const string PersistentExampleFrame::group = "Persistent_Options/ExampleFrame/frame";

//general window items
const string PersistentExampleFrame::w_isIconized  = "w_isIconized";
const string PersistentExampleFrame::w_isMaximized = "w_isMaximized";
const string PersistentExampleFrame::w_pos_x       = "w_pos_x";
const string PersistentExampleFrame::w_pos_y       = "w_pos_y";
const string PersistentExampleFrame::w_sashPos     = "w_sashPos";
const string PersistentExampleFrame::w_size_h      = "w_size_h";
const string PersistentExampleFrame::w_size_w      = "w_size_w";
//canvas items
const string PersistentExampleFrame::c_slice = "c_slice";
const string PersistentExampleFrame::c_t_x   = "c_t_x";
const string PersistentExampleFrame::c_t_y   = "c_t_y";
//graymap items
const string PersistentExampleFrame::gm_visible = "gm_visible";
const string PersistentExampleFrame::gm_invert  = "gm_invert";
const string PersistentExampleFrame::gm_level   = "gm_level";
const string PersistentExampleFrame::gm_width   = "gm_width";
//------------------------------------------------------------------------
/**
 * save the widget's properties (in ~/.cavass.ini).
 * note that multiple values are saved below.
 * this function should mimic Restore().
 */
void PersistentExampleFrame::Save ( ) const {
    //save the slider's properties
    if (verbose) cout << "PersistentExampleFrame::Save()" << endl;
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

    if (w->mSplitter != nullptr) {
        int sashPosVal = w->mSplitter->GetSashPosition();
        SaveValue(w_sashPos, sashPosVal);
    }

    //save canvas specific items
    auto canvas = dynamic_cast<ExampleCanvas *>( w->mCanvas );
    assert(canvas != nullptr);

    //save index of slice currently displayed
    int sliceVal = canvas->getSliceNo(0);
    SaveValue(c_slice, sliceVal);

    //save image translation tx, ty
    SaveValue(c_t_x, canvas->mTx);
    SaveValue(c_t_y, canvas->mTy);

    //save graymap settings

    if (w->mGrayMapControls == nullptr) {  //are graymap controls currently visible?
        SaveValue(gm_visible, 0);
    } else {
        SaveValue(gm_visible, 1);
    }

    int invertVal = canvas->getInvert(0);
    SaveValue(gm_invert, invertVal);

    int levelVal = canvas->getCenter(0);
    SaveValue(gm_level, levelVal);

    int widthVal = canvas->getWidth(0);
    SaveValue(gm_width, widthVal);

    //SaveValue( "fred", string("this is a test string") );  //example of saving a string

    output("Save(): ", "returning.");
}
//------------------------------------------------------------------------
/**
 * restore the widget's properties (from ~/.cavass.ini).
 * note that multiple values are restored below.
 * this function should mimic Save().
 */
bool PersistentExampleFrame::Restore ( ) {
    if (verbose) cout << "PersistentExampleFrame::Restore() \n";
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
    if (!ok)    cerr << "restore w_isMaximized failed." << endl;
    else        w->Maximize(isMaximizedVal == 1);

    //position (two values: x and y)
    wxPoint pos;
    ok = RestoreValue(w_pos_x, &pos.x);
    if (!ok) {
        cerr << "restore w_pos_x failed." << endl;
    } else {
        ok = RestoreValue(w_pos_y, &pos.y);
        if (!ok)    cerr << "restore w_pos_y failed." << endl;
        else        w->SetPosition(pos);
    }

    //size (two values: h and w)
    wxSize size;
    ok = RestoreValue(w_size_h, &size.y);
    if (!ok)    cerr << "restore w_size_h failed." << endl;
    else {
        ok = RestoreValue(w_size_w, &size.x);
        if (!ok)    cerr << "restore w_size_w failed." << endl;
        w->SetSize(size);
        cout << "setting size to (w,h)=" << size.GetWidth() << "," << size.GetHeight() << endl;
        cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
    }

    //sash position
    int sashPosVal;
    ok = RestoreValue(w_sashPos, &sashPosVal);
    if (!ok)    cerr << "restore w_sashPos failed." << endl;
    else        w->mSplitter->SetSashPosition(sashPosVal);

    output("Restore(): ", "returning true.");
    cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
    return true;
}
//------------------------------------------------------------------------
void PersistentExampleFrame::restoreControlSettings ( ) {
    if (verbose) {
        cout << "PersistentExampleFrame::restoreControlSettings()" << endl;
    }
    if (!Preferences::getDejaVuMode())    return;
    auto w = Get();
    assert( w != nullptr );
    if (w == nullptr)    return;

    if (verbose) {
        cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
    }

    //restore canvas specific items
    auto canvas = dynamic_cast<ExampleCanvas *>( w->mCanvas );
    if (canvas == nullptr && canvas->mCavassData == nullptr)    return;

    //restore graymap settings
    bool updateContrast = false;
    //invert
    int invertVal;
    bool ok = RestoreValue(gm_invert, &invertVal);
    if (!ok)    cerr << "restore gm_invert failed." << endl;
    else {
        canvas->setInvert( 0, invertVal!=0 );
        updateContrast = true;
    }
    //level
    int levelVal;
    ok = RestoreValue(gm_level, &levelVal);
    if (!ok)    cerr << "restore gm_level failed." << endl;
    else {
        canvas->setCenter(0, levelVal);
        updateContrast = true;
    }
    //width
    int widthVal;
    ok = RestoreValue(gm_width, &widthVal);
    if (!ok)    cerr << "restore gm_width failed." << endl;
    else {
        canvas->setWidth(0, widthVal);
        updateContrast = true;
    }

    //restore index of slice currently displayed
    int sliceVal;
    ok = RestoreValue(c_slice, &sliceVal);
    if (!ok)    cerr << "restore c_slice failed." << endl;
    else        canvas->setSliceNo(0, sliceVal);

    //get image display position tx, ty
    int txVal;
    bool txOK = RestoreValue( c_t_x, &txVal );
    if (!txOK)    cerr << "restore c_t_x failed." << endl;
    int tyVal;
    bool tyOK = RestoreValue( c_t_y, &tyVal );
    if (txOK && tyOK) {
        canvas->mTx = txVal;
        canvas->mTy = tyVal;
        //wxObject unused;
        //canvas->OnPaint( (wxPaintEvent&) unused );  //n.g. on win
        canvas->Refresh();
    }

    //graymap controls visible?
    int graymapVal;
    ok = RestoreValue(gm_visible, &graymapVal);
    if (!ok)    cerr << "restore gm_visible controls failed." << endl;
    else {
        if (graymapVal && w->mGrayMapControls == nullptr) {
            wxCommandEvent unused;
            w->OnGrayMap(unused);
        } else if (!graymapVal && w->mGrayMapControls != nullptr) {
            wxCommandEvent unused;
            w->OnGrayMap(unused);
        }
    }

    if (updateContrast) {
        canvas->initLUT( 0 );
        canvas->reload();
    }
}
//------------------------------------------------------------------------
