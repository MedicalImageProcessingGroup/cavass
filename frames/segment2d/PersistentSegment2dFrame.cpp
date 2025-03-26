#include "cavass.h"
#include "Segment2dAuxControls.h"
#include "Segment2dCanvas.h"
#include "Segment2dSlider.h"
#include "SetSegment2dOutputControls.h"
#include "PersistentSegment2dFrame.h"

#include "Segment2dIntDLControls.h"

#ifndef WIN32
#define VERBOSE        cout << __PRETTY_FUNCTION__  << endl
#else
#define VERBOSE
#endif
//------------------------------------------------------------------------
//const string PersistentSegment2dFrame::group = "Persistent_Options/Segment2dFrame/frame";

//general window items
const string PersistentSegment2dFrame::w_isIconized  = "w_isIconized";
const string PersistentSegment2dFrame::w_isMaximized = "w_isMaximized";
const string PersistentSegment2dFrame::w_pos_x       = "w_pos_x";
const string PersistentSegment2dFrame::w_pos_y       = "w_pos_y";
const string PersistentSegment2dFrame::w_sashPos     = "w_sashPos";
const string PersistentSegment2dFrame::w_size_h      = "w_size_h";
const string PersistentSegment2dFrame::w_size_w      = "w_size_w";
//canvas items
const string PersistentSegment2dFrame::c_defaultFillOn = "c_defaultFillOn";
const string PersistentSegment2dFrame::c_layout        = "c_layout";
const string PersistentSegment2dFrame::c_object        = "c_object";
const string PersistentSegment2dFrame::c_overlayOn     = "c_overlayOn";
const string PersistentSegment2dFrame::c_t_x           = "c_t_x";
const string PersistentSegment2dFrame::c_t_y           = "c_t_y";
//graymap items
const string PersistentSegment2dFrame::gm_visible = "gm_visible";
const string PersistentSegment2dFrame::gm_invert  = "gm_invert";
const string PersistentSegment2dFrame::gm_level   = "gm_level";
const string PersistentSegment2dFrame::gm_width   = "gm_width";
//set index items
const string PersistentSegment2dFrame::si_visible = "si_visible";
const string PersistentSegment2dFrame::si_labels  = "si_labels";
const string PersistentSegment2dFrame::si_scale   = "si_scale";
const string PersistentSegment2dFrame::si_slice   = "si_slice";
//set output items
const string PersistentSegment2dFrame::so_visible   = "so_visible";
const string PersistentSegment2dFrame::so_outObject = "so_outObject";
const string PersistentSegment2dFrame::so_type      = "so_type";

//mode items
const string PersistentSegment2dFrame::m_lastMode = "m_lastMode";

//features mode items
const string PersistentSegment2dFrame::f_lastFeature = "f_lastFeature";  ///< one of these features: HigherDensity, LowerDensity, Gradient1, Gradient2, Gradient3, Gradient4
#ifdef _seg2d
    #error "warning: redefining _seg2d!"
#endif
#define _seg2d(F)  \
    const string  PersistentSegment2dFrame::f_##F##_on                    = "f_"#F"_on";                    \
    const string  PersistentSegment2dFrame::f_##F##_lastTransform         = "f_"#F"_lastTransform";         \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_linear_weight         = "f_"#F"_linear_weight";         \
    const string  PersistentSegment2dFrame::f_##F##_linear_min            = "f_"#F"_linear_min";            \
    const string  PersistentSegment2dFrame::f_##F##_linear_max            = "f_"#F"_linear_max";            \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_invLinear_weight      = "f_"#F"_invLinear_weight";      \
    const string  PersistentSegment2dFrame::f_##F##_invLinear_min         = "f_"#F"_invLinear_min";         \
    const string  PersistentSegment2dFrame::f_##F##_invLinear_max         = "f_"#F"_invLinear_max";         \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_gaussian_weight       = "f_"#F"_gaussian_weight";       \
    const string  PersistentSegment2dFrame::f_##F##_gaussian_mean         = "f_"#F"_gaussian_mean";         \
    const string  PersistentSegment2dFrame::f_##F##_gaussian_stddev       = "f_"#F"_gaussian_stddev";       \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_invGaussian_weight    = "f_"#F"_invGaussian_weight";    \
    const string  PersistentSegment2dFrame::f_##F##_invGaussian_mean      = "f_"#F"_invGaussian_mean";      \
    const string  PersistentSegment2dFrame::f_##F##_invGaussian_stddev    = "f_"#F"_invGaussian_stddev";    \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_hyperbolic_weight     = "f_"#F"_hyperbolic_weight";     \
    const string  PersistentSegment2dFrame::f_##F##_hyperbolic_mean       = "f_"#F"_hyperbolic_mean";       \
    const string  PersistentSegment2dFrame::f_##F##_hyperbolic_stddev     = "f_"#F"_hyperbolic_stddev";     \
                                                                                                            \
    const string  PersistentSegment2dFrame::f_##F##_invHyperbolic_weight  = "f_"#F"_invHyperbolic_weight";  \
    const string  PersistentSegment2dFrame::f_##F##_invHyperbolic_mean    = "f_"#F"_invHyperbolic_mean";    \
    const string  PersistentSegment2dFrame::f_##F##_invHyperbolic_stddev  = "f_"#F"_invHyperbolic_stddev";
_seg2d( higherDensity );
_seg2d( lowerDensity );
_seg2d( gradient1 );
_seg2d( gradient2 );
_seg2d( gradient3 );
_seg2d( gradient4 );
#undef _seg2d

//ilw mode items
const string PersistentSegment2dFrame::ilw_iterates   = "ilw_iterates",
             PersistentSegment2dFrame::ilw_minCtrlPts = "ilw_minCtrlPts";
//livesnake mode items
const string PersistentSegment2dFrame::ls_iterates = "ls_iterates",
             PersistentSegment2dFrame::ls_alpha    = "ls_alpha",
             PersistentSegment2dFrame::ls_beta     = "ls_beta",
             PersistentSegment2dFrame::ls_gamma    = "ls_gamma";
//paint mode item
const string PersistentSegment2dFrame::p_brushSize = "p_brushSize";
//peek   mode items: none
//report mode items: none
//review mode items: none
//train  mode item
const string PersistentSegment2dFrame::t_brushSize = "t_brushSize";
//interactive deep learning item
const string PersistentSegment2dFrame::idl_model = "idl_model";
//------------------------------------------------------------------------
// save
//------------------------------------------------------------------------
/**
 * save the frame's properties (in ~/.cavass.ini).
 * note that multiple values are saved below.
 * this function should "mimic" Restore().
 */
void PersistentSegment2dFrame::Save ( ) const {
    VERBOSE;
    saveGeneralSettings();
    saveGrayMapSettings();
    saveIntDLSettings();
    saveInteractive2dSettings();
    saveSetIndexSettings();
    saveSetOutputSettings();
    saveAuxControls();

    //SaveValue( "fred", string("this is a test string") );  //ex. of saving a string
    output("Save(): ", "returning.");
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveGeneralSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    //save general window settings
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
    auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
    assert(canvas != nullptr);
#if 0
    /** \todo disabled for now; needs work. to demonstrate problem:
     * 1. set scale to a large value.
     * 2. move image around.
     * 3. repeatedly go in and out of cavass
     * 4. note that c_t_x and c_t_y keep changing
     */
    //save image translation tx, ty
    //SaveValue( c_t_x, canvas->mTx );
    //SaveValue( c_t_y, canvas->mTy );
    SaveValue( c_t_x, wxString::Format("%.2f", canvas->mTx/canvas->getScale()) );
    SaveValue( c_t_y, wxString::Format("%.2f", canvas->mTy/canvas->getScale()) );
#endif
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveInteractive2dSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;
    //save canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas *>( w->mCanvas );
    if (canvas == nullptr)    return;

    //save main controls (Interactive2D) settings: mode, default fill, layout, and overlay
    //SaveValue( m_lastMode, w->mMode->GetStringSelection() );
    SaveValue( m_lastMode, w->modeName[canvas->detection_mode] );
    if ( w->mMode->GetStringSelection() != w->modeName[canvas->detection_mode] ) {
        cerr << "mMode = " << w->mMode->GetStringSelection() << ", modeName = " << w->modeName[canvas->detection_mode] << endl;
        assert( w->mMode->GetStringSelection() == w->modeName[canvas->detection_mode] );
    }

    SaveValue( c_defaultFillOn, w->m_defaultFill->IsChecked() );
    SaveValue( c_layout, w->m_layout->IsChecked() );
    SaveValue( c_object, w->m_object->GetStringSelection() );
    SaveValue( c_overlayOn, w->m_overlay->IsChecked() );
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveGrayMapSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;
    //save canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
    if (canvas == nullptr)    return;

    //save graymap settings
    if (w->mGrayMapControls == nullptr) {  //are graymap controls currently visible?
        SaveValue(gm_visible, 0);
    } else {
        SaveValue(gm_visible, 1);
    }
    int invertVal = canvas->getInvert(0);  //invert
    SaveValue(gm_invert, invertVal);
    int levelVal = canvas->getCenter(0);  //level
    SaveValue(gm_level, levelVal);
    int widthVal = canvas->getWidth(0);  //width
    SaveValue(gm_width, widthVal);
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveSetIndexSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;
    //save canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
    if (canvas == nullptr)    return;

    //save set index settings
    if (w->mSetIndexControls == nullptr) {  //are set index controls currently visible?
        SaveValue(si_visible, 0);
    } else {
        SaveValue(si_visible, 1);
    }
    //save index of slice currently displayed
    SaveValue( si_slice, canvas->getSliceNo(0) );
    //save state of labels (on/off)
    SaveValue( si_labels, canvas->getLabels() );
    //save image scale (double as string)
    SaveValue( si_scale, wxString::Format("%.2f", canvas->getScale()) );
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveSetOutputSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;
    //save canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas *>( w->mCanvas );
    if (canvas == nullptr)    return;

    //save set output settings
    if (w->mSetOutputControls == nullptr) {  //are set output controls currently visible?
        SaveValue( so_visible, 0 );
    } else {
        SaveValue( so_visible, 1 );
        SaveValue( so_outObject, w->mSetOutputControls->m_outputObject->GetStringSelection() );
        SaveValue( so_type, w->mSetOutputControls->m_outputType->GetLabel() );
    }
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveIntDLSettings ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;
    //save canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas *>( w->mCanvas );
    if (canvas == nullptr)    return;
    if (w->mIntDLControls == nullptr) {
        //SaveValue( idl_model, "" );
    } else {
        SaveValue( idl_model, w->mIntDLControls->mFullFileNameStr );
    }
}
//------------------------------------------------------------------------
/** save current state of current aux/mode controls */
void PersistentSegment2dFrame::saveAuxControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    cout << "\t" << which << endl;
    if (which == "Feature Controls")           saveFeatureControls();
    else if (which == "ILW Controls")          saveILWControls();
    else if (which == "LiveSnake Controls")    saveLiveSnakeControls();
    else if (which == "Paint Controls")        savePaintControls();
    else if (which == "Train Controls")        saveTrainControls();
    else                                       cerr << "PersistentSegment2dFrame::saveAuxControls: unsupported mode controls, " << which << endl;
}
//------------------------------------------------------------------------
/** save feature control settings. complicated (yet simpler than restoring
 *  feature control settings)!
 *  note: slider control values are "pulled" directly from the slider and
 *  may/will contain leading spaces (which can be removed with .trim(false).
 */
void PersistentSegment2dFrame::saveFeatureControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    if (which != "Feature Controls") {
        cerr << "PersistentSegment2dFrame::restoreFeatureControls: " << which
             << "('Feature Controls' expected.)" << endl;
        return;
    }

    auto f = c->mFeature->GetStringSelection();  //last (most recent) feature selected
    SaveValue( f_lastFeature, f );

    auto status = c->mFeatureStatus->GetValue();  //feature status (on or off)
    auto trans = c->mTransform->GetStringSelection();  //transform selected
    auto weight = c->mWeight->mSt2->GetLabel();  //weight amount

    //features: HigherDensity, LowerDensity, Gradient1, Gradient2, Gradient3, Gradient4
    //  transforms:
    //    Linear or Inv. Linear transforms: weight, min, and max
    //    all others transforms (Gaussian, Inv. Gaussian, Hyperbolic, Inv. Hyperbolic): weight, mean, and std dev

    //the following macro is for features w/ linear or inv. linear transforms.
    // min and max values will be saved in this case (along with other values).
#ifdef _seg2d
    #error "warning: redefining _seg2d!"
#endif
#define _seg2d(F1, F2, T1, T2)                        \
    if (f == F1 && trans == T1) {                     \
        auto min = c->mFeatureMin->mSt2->GetLabel();  \
        auto max = c->mFeatureMin->mSt2->GetLabel();  \
        SaveValue( f_##F2##_on,            status );  \
        SaveValue( f_##F2##_lastTransform, trans  );  \
        SaveValue( f_##F2##_##T2##_weight, weight );  \
        SaveValue( f_##F2##_##T2##_min,    min    );  \
        SaveValue( f_##F2##_##T2##_max,    max    );  \
        return;                                       \
    }
    //      F1                F2             T1             T2
    _seg2d( "Higher Density", higherDensity, "Linear",      linear );
    _seg2d( "Lower Density",  lowerDensity,  "Linear",      linear );
    _seg2d( "Gradient 1",     gradient1,     "Linear",      linear );
    _seg2d( "Gradient 2",     gradient2,     "Linear",      linear );
    _seg2d( "Gradient 3",     gradient3,     "Linear",      linear );
    _seg2d( "Gradient 4",     gradient4,     "Linear",      linear );
    _seg2d( "Higher Density", higherDensity, "Inv. Linear", invLinear );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Linear", invLinear );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Linear", invLinear );
#undef _seg2d

    //the following macro is for features w/ gaussian or inv. gaussian transforms.
    // mean and stddev values will be saved in this case (along with other values).
#ifdef _seg2d
    #error "warning: redefining _seg2d!"
#endif
#define _seg2d(F1, F2, T1, T2)                        \
    if (f == F1 && trans == T1) {                     \
        auto mean = c->mMean->mSt2->GetLabel();       \
        auto stddev = c->mStdDev->mSt2->GetLabel();   \
        SaveValue( f_##F2##_on, status );             \
        SaveValue( f_##F2##_lastTransform, trans  );  \
        SaveValue( f_##F2##_##T2##_weight, weight );  \
        SaveValue( f_##F2##_##T2##_mean,   mean   );  \
        SaveValue( f_##F2##_##T2##_stddev, stddev );  \
        return;                                       \
    }
    //      F1                F2             T1                 T2
    _seg2d( "Higher Density", higherDensity, "Gaussian",        gaussian );
    _seg2d( "Lower Density",  lowerDensity,  "Gaussian",        gaussian );
    _seg2d( "Gradient 1",     gradient1,     "Gaussian",        gaussian );
    _seg2d( "Gradient 2",     gradient2,     "Gaussian",        gaussian );
    _seg2d( "Gradient 3",     gradient3,     "Gaussian",        gaussian );
    _seg2d( "Gradient 4",     gradient4,     "Gaussian",        gaussian );
    _seg2d( "Higher Density", higherDensity, "Inv. Gaussian",   invGaussian );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Gaussian",   invGaussian );

    _seg2d( "Higher Density", higherDensity, "Hyperbolic",      hyperbolic );
    _seg2d( "Lower Density",  lowerDensity,  "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 1",     gradient1,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 2",     gradient2,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 3",     gradient3,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 4",     gradient4,     "Hyperbolic",      hyperbolic );
    _seg2d( "Higher Density", higherDensity, "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Hyperbolic", invHyperbolic );
#undef _seg2d

    cerr << "unsupported feature/transform: " << f << "/" << trans << endl;
    assert( 0 );  //should never get here!
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveILWControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    assert(which == "ILW Controls");

    SaveValue( ilw_iterates, c->mIterates->GetStringSelection() );
    SaveValue( ilw_minCtrlPts, c->mMinPointsCtrl->GetValue() );
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveLiveSnakeControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    assert(which == "LiveSnake Controls");

    SaveValue( ls_iterates, c->mIterates->GetStringSelection() );
    SaveValue( ls_alpha, c->mAlphaCtrl->GetValue() );
    SaveValue( ls_beta, c->mBetaCtrl->GetValue() );
    SaveValue( ls_gamma, c->mGammaCtrl->GetValue() );
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::savePaintControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    assert(which == "Paint Controls");

    SaveValue( p_brushSize, c->mBrushSize->GetStringSelection() );
}
//------------------------------------------------------------------------
void PersistentSegment2dFrame::saveTrainControls ( ) const {
    VERBOSE;
    auto w = Get();
    if (w == nullptr)    return;

    if (w->mAuxControls == nullptr)    return;
    auto c = w->mAuxControls;
    if (c == nullptr)    return;

    auto which = c->mAuxBox->GetLabel();
    assert(which == "Train Controls");

    SaveValue( t_brushSize, c->mBrushSize->GetStringSelection() );
}
//------------------------------------------------------------------------
// restore
//------------------------------------------------------------------------
/**
 * restore the frame's properties (from ~/.cavass.ini).
 * note that multiple values are restored below.
 * in general, this function should mimic Save().
 */
bool PersistentSegment2dFrame::Restore ( ) {
    VERBOSE;
    //restore the frame's properties
    auto w = Get();
    assert( w != nullptr );
    bool ok;

    //restore general window values

    //w_isIconized
    int w_isIconizedVal;
    ok = RestoreValue(w_isIconized, &w_isIconizedVal);
    if (!ok) {
        cerr << "the error messages below normally appear the very first time that this frame is instantiated." << endl;
        cerr << "restore w_isIconized failed." << endl;
    } else {
#ifndef WIN32
        w->SetIconizeState(w_isIconizedVal == 1);
#endif
    }

    //w_isMaximized
    int w_isMaximizedVal;
    ok = RestoreValue(w_isMaximized, &w_isMaximizedVal);
    if (!ok)    cerr << "restore w_isMaximized failed." << endl;
    else        w->Maximize(w_isMaximizedVal == 1);

    //position (two values: x and y)
    wxPoint w_posVal;
    ok = RestoreValue(w_pos_x, &w_posVal.x);
    if (!ok) {
        cerr << "restore w_pos_x failed." << endl;
    } else {
        ok = RestoreValue(w_pos_y, &w_posVal.y);
        if (!ok)    cerr << "restore w_pos_y failed." << endl;
        else        w->SetPosition(w_posVal);
    }

    //size (two values: h and w)
    wxSize w_sizeVal;
    ok = RestoreValue(PersistentSegment2dFrame::w_size_h, &w_sizeVal.y);
    if (!ok) {
        cerr << "restore w_size_h failed." << endl;
        w->Maximize();  //use default
    } else {
        ok = RestoreValue(PersistentSegment2dFrame::w_size_w, &w_sizeVal.x);
        if (!ok) {
            cerr << "restore size.w failed." << endl;
            w->Maximize();  //use default
        } else {
            w->SetSize(w_sizeVal);
            cout << "setting size to (w,h)=" << w_sizeVal.GetWidth() << "," << w_sizeVal.GetHeight() << endl;
            cout << "size is now (w,h)=" << w->GetSize().GetWidth() << "," << w->GetSize().GetHeight() << endl;
        }
    }

    //sash position
    int w_sashPosVal;
    ok = RestoreValue(w_sashPos, &w_sashPosVal);
    if (!ok)    cerr << "restore w_sashPos failed." << endl;
    else        w->mSplitter->SetSashPosition(w_sashPosVal);

#if 0
    //restore canvas specific items
    /** \todo determine is this is ever used and remove if possible */
    auto canvas = dynamic_cast<Segment2dCanvas *>( w->mCanvas );
    if (canvas != nullptr && canvas->mCavassData != nullptr) {
        //restore contrast settings
        //invert
        int invertVal;
        ok = RestoreValue(gm_invert, &invertVal);
        if (!ok) cerr << "restore invert failed." << endl;
        else canvas->setInvert(0, invertVal == 1);
        //level
        int levelVal;
        ok = RestoreValue(gm_level, &levelVal);
        if (!ok) cerr << "restore level failed." << endl;
        else canvas->setCenter(0, levelVal);
        //width
        int widthVal;
        ok = RestoreValue(gm_width, &widthVal);
        if (!ok) cerr << "restore width failed." << endl;
        else canvas->setWidth(0, widthVal);

        //restore index of slice currently displayed
        int sliceVal;
        ok = RestoreValue(si_slice, &sliceVal);
        if (!ok) cerr << "restore slice failed." << endl;
        else canvas->setSliceNo(0, sliceVal);

        //graymap controls visible?
        int graymapVal;
        ok = RestoreValue(gm_visible, &graymapVal);
        if (!ok) cerr << "restore graymap controls failed." << endl;
        else {
            wxCommandEvent unused;
            if (graymapVal && w->mGrayMapControls == nullptr) {  //should be on but is off
                w->OnGrayMap(unused);
            } else if (!graymapVal && w->mGrayMapControls != nullptr) {  //should be off but is on?
                w->OnGrayMap(unused);
            }
        }

        /** \todo restore tx, ty */
    } else {
        cerr << "no canvas (or cavass data) available (yet)" << endl;
    }
#endif

    output("Restore(): ", "returning true.");
    return true;
}
//------------------------------------------------------------------------
/**
 * this restores the settings of the Interactive2D controls that appear on the
 * lower right. it also restores one of the additional control sets (graymap,
 * set index, or set output) that may or may not be visible on the left. it
 * also restores the auxiliary control (mode) panel on the left as well.
 */
void PersistentSegment2dFrame::restoreControlSettings ( ) {
    VERBOSE;
    if (!Preferences::getDejaVuMode())    return;
    auto w = Get();
    assert( w != nullptr );
    if (w == nullptr)    return;

    //restore canvas specific items
    auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
    if (canvas == nullptr || canvas->mCavassData == nullptr)    return;
    bool reloadCanvas = false, ok;

    //restore image translation tx, ty on canvas
#if 0
    /** \todo disabled for now; needs work. to demonstrate problem:
     * 1. set scale to a large value.
     * 2. move image around.
     * 3. repeatedly go in and out of cavass
     * 4. note that c_t_x and c_t_y keep changing
     */
    int tx;
    ok = RestoreValue( c_t_x, &tx );
    if (!ok)    cerr << "restore c_t_x failed." << endl;
    else {
        int ty;
        ok = RestoreValue( c_t_y, &ty );
        if (!ok)    cerr << "restore c_t_y failed." << endl;
        else {
            canvas->mTx = tx;
            canvas->mTy = ty;
            reloadCanvas = true;
        }
    }
#endif
    /** \todo restore set index settings: slice no. (necessary?) */

    //restore set index settings: scale
    wxString tmp;
    ok = RestoreValue( si_scale, &tmp );
    if (!ok)    cerr << "restore si_scale failed." << endl;
    else {
        double sc;
        ok = tmp.ToDouble( &sc );
        if (!ok)    cerr << "bad scale value." << endl;
        else {
            canvas->setScale( sc );
            reloadCanvas = true;
        }
    }
    //restore set index settings: labels on/off
    bool labels;
    ok = RestoreValue( si_labels, &labels );
    if (!ok)    cerr << "restore si_labels failed." << endl;
    else {
        canvas->setLabels( labels );
        reloadCanvas = true;
    }

    //restore graymap settings
    bool updateContrast = false;
    //invert
    int invertVal;
    ok = RestoreValue(gm_invert, &invertVal);
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
        canvas->setWidth( 0, widthVal );
        updateContrast = true;
    }
    //any contrast changes (from defaults)?
    if (updateContrast) {
        canvas->initLUT( 0 );
        reloadCanvas = true;
    }

    if (reloadCanvas)    canvas->reload();

    //restore main controls (Interactive2D) settings

    /* restore aux controls/mode selection */
    wxString lastMode;
    ok = RestoreValue( m_lastMode, &lastMode );
    if (!ok) {
        cerr << "restore m_lastMode failed." << endl;
    } else {
        if (w->mMode->GetStringSelection() != lastMode) {  //need to change modes (from default)?
            //out with the old; in with the new
            w->doMode( lastMode );
        }
        if (lastMode == "Feature")           restoreFeatureControls();
        else if (lastMode == "ILW")          restoreILWControls();
        else if (lastMode == "IntDL")        restoreIntDLControls();
        else if (lastMode == "LiveSnake")    restoreLiveSnakeControls();
        else if (lastMode == "Paint")        restorePaintControls();
        else if (lastMode == "Train")        restoreTrainControls();
        else                                 cerr << "PersistentSegment2dFrame::restoreControlSettings: unsupported mode controls, " << lastMode << endl;
    }

    //restore default fill (c_defaultFillOn)
    int checked;
    ok = RestoreValue( c_defaultFillOn, &checked );
    if (!ok)    cerr << "restore c_defaultFillOn failed." << endl;
    else        w->doDefaultFill( checked );
    //restore layout (c_layout)
    ok = RestoreValue( c_layout, &checked );
    if (!ok)    cerr << "restore c_layout failed." << endl;
    else        w->doLayout( checked );
    //restore object (c_object)
    wxString object;
    ok = RestoreValue( c_object, &object );
    if (!ok)    cerr << "restore c_object failed." << endl;
    else        w->doObject( object );
    //restore overlay (c_overlayOn)
    ok = RestoreValue( c_overlayOn, &checked );
    if (!ok)    cerr << "restore c_overlayOn failed." << endl;
    else        w->doOverlay( checked );

    //restore graymap, set index, or set output controls _visibility_
    // (at most, only one should be visible at a time)

    //graymap controls visible?
    int vis;
    ok = RestoreValue( gm_visible, &vis );
    if (!ok)    cerr << "restore gm_visible controls failed." << endl;
    else {
        if (vis && w->mGrayMapControls == nullptr) {
            wxCommandEvent unused;
            w->OnGrayMap( unused );
        } else if (!vis && w->mGrayMapControls != nullptr) {
            wxCommandEvent unused;
            w->OnGrayMap( unused );
        }
    }

    //set index controls visible?
    ok = RestoreValue(si_visible, &vis);
    if (!ok)    cerr << "restore si_visible controls failed." << endl;
    else {
        if (vis) {  //should be visible?
            if (w->mSetIndexControls == nullptr) {  //but it's not; make visible
                wxCommandEvent unused;
                w->OnSetIndex( unused );
            }
            bool labelsOn;
            ok = RestoreValue( si_labels, &labelsOn );
            if (!ok)    cerr << "restore si_labels failed." << endl;
            else        w->doLabels( labelsOn );
        } else if (w->mSetIndexControls != nullptr) {  //visible but shouldn't be?
            wxCommandEvent unused;
            w->OnSetIndex( unused );
        }
    }

    //set output visible?
    ok = RestoreValue(so_visible, &vis);
    if (!ok)    cerr << "restore so_visible controls failed." << endl;
    else {
        if (vis && w->mSetOutputControls == nullptr) {  //should be visible but isn't?
            wxCommandEvent unused;
            w->OnSetOutput( unused );
        } else if (!vis && w->mSetOutputControls != nullptr) {  //shouldn't be visible but is?
            wxCommandEvent unused;
            w->OnSetOutput( unused );
        }
    }
}
//------------------------------------------------------------------------
/**
 * varies with feature and transform selected. analogous to
 * saveFeatureControls() with the additional complications of not only u.i.
 * changes, but data changes as well!
 */
void PersistentSegment2dFrame::restoreFeatureControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;
    if (w->mAuxControls == nullptr)    return;

    auto c = w->mAuxControls;
    if (c == nullptr)    return;
    if (c->mAuxBox == nullptr)    return;

    auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
    if (canvas == nullptr)    return;

    //make sure that we are restoring feature controls
    auto which = c->mAuxBox->GetLabel();
    if (which != "Feature Controls") {
        cerr << "PersistentSegment2dFrame::restoreFeatureControls: " << which
             << "('Feature Controls' expected.)" << endl;
        return;
    }

    //restore previously selected feature
    wxString feat;
    bool ok = RestoreValue( f_lastFeature, &feat );
    if (!ok) {
        cerr << "PersistentSegment2dFrame::restoreFeatureControls: can't get last feature." << endl;
        return;
    }
    //valid feature?
    if (std::find( Segment2dFrame::featureNames.begin(),
                   Segment2dFrame::featureNames.end(),
                   feat) == Segment2dFrame::featureNames.end())
    {
        cerr << "PersistentSegment2dFrame::restoreFeatureControls: unrecognized last feature -> " << feat << endl;
        return;
    }
    w->doFeature( feat );

    //restore the transform that was used with this feature
    wxString trans;
#ifdef _seg2d
    #error "warning: redefining _seg2d!"
#endif
#define _seg2d(F1, F2 )                                       \
    if (feat == F1) {                                         \
        ok = RestoreValue( f_##F2##_lastTransform, &trans );  \
        if (!ok) {                                            \
            cerr << "PersistentSegment2dFrame::restoreFeatureControls: unrecognized last transform -> "  \
                 << trans << endl;                            \
            return;                                           \
        }                                                     \
    }
    //      F1                F2
    _seg2d( "Higher Density", higherDensity );
    _seg2d( "Lower Density",  higherDensity );
    _seg2d( "Gradient 1",     gradient1     );
    _seg2d( "Gradient 2",     gradient2     );
    _seg2d( "Gradient 3",     gradient3     );
    _seg2d( "Gradient 4",     gradient4     );
#undef _seg2d
    //valid transform?
    if (std::find( Segment2dFrame::transformNames.begin(),
                   Segment2dFrame::transformNames.end(),
                   trans) == Segment2dFrame::transformNames.end())
    {
        cerr << "PersistentSegment2dFrame::restoreFeatureControls: unrecognized last transform -> "
             << trans << endl;
        return;
    }
    w->doTransform( trans );

    //handle features w/ linear or inv. linear transforms
#ifdef _seg2d
    #error "warning: redefining _seg2d!"
#endif
    //weight f_##F2##_##T2##_weight, min, max and status (on/off)
#define _seg2d(F1, F2, T1, T2)                              \
    if (feat == F1 && trans == T1) {                        \
        wxString tmp;                                       \
        /* weight */                                        \
        ok = RestoreValue( f_##F2##_##T2##_weight, &tmp );  \
        if (ok)    w->doWeight( tmp );                      \
        /* min */                                           \
        ok = RestoreValue( f_##F2##_##T2##_min, &tmp );     \
        if (ok)    w->doFeatureMin( tmp );                  \
        /* max */                                           \
        ok = RestoreValue( f_##F2##_##T2##_max, &tmp );     \
        if (ok)    w->doFeatureMax( tmp );                  \
        /* status (on/off) */                               \
        ok = RestoreValue( f_##F2##_on, &tmp );             \
        w->doFeatureStatus( tmp != "0" );                   \
    }
    //      F1                F2             T1             T2
    _seg2d( "Higher Density", higherDensity, "Linear",      linear );
    _seg2d( "Lower Density",  lowerDensity,  "Linear",      linear );
    _seg2d( "Gradient 1",     gradient1,     "Linear",      linear );
    _seg2d( "Gradient 2",     gradient2,     "Linear",      linear );
    _seg2d( "Gradient 3",     gradient3,     "Linear",      linear );
    _seg2d( "Gradient 4",     gradient4,     "Linear",      linear );
    _seg2d( "Higher Density", higherDensity, "Inv. Linear", invLinear );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Linear", invLinear );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Linear", invLinear );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Linear", invLinear );
#undef _seg2d

    //handle features w/ gaussian or inv. gaussian transforms
#ifdef _seg2d
#error "warning: redefining _seg2d!"
#endif
    //weight f_##F2##_##T2##_weight, min, max and status (on/off)
#define _seg2d(F1, F2, T1, T2)                              \
    if (feat == F1 && trans == T1) {                        \
        wxString tmp;                                       \
        /* weight */                                        \
        ok = RestoreValue( f_##F2##_##T2##_weight, &tmp );  \
        if (ok)    w->doWeight( tmp );                      \
        /* mean */                                          \
        ok = RestoreValue( f_##F2##_##T2##_mean, &tmp );    \
        if (ok)    w->doMean( tmp );                        \
        /* std dev */                                       \
        ok = RestoreValue( f_##F2##_##T2##_stddev, &tmp );  \
        if (ok)    w->doStdDev( tmp );                      \
        /* status (on/off) */                               \
        ok = RestoreValue( f_##F2##_on, &tmp );             \
        w->doFeatureStatus( tmp != "0" );                   \
    }
    //      F1                F2             T1                 T2
    _seg2d( "Higher Density", higherDensity, "Gaussian",        gaussian );
    _seg2d( "Lower Density",  lowerDensity,  "Gaussian",        gaussian );
    _seg2d( "Gradient 1",     gradient1,     "Gaussian",        gaussian );
    _seg2d( "Gradient 2",     gradient2,     "Gaussian",        gaussian );
    _seg2d( "Gradient 3",     gradient3,     "Gaussian",        gaussian );
    _seg2d( "Gradient 4",     gradient4,     "Gaussian",        gaussian );
    _seg2d( "Higher Density", higherDensity, "Inv. Gaussian",   invGaussian );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Gaussian",   invGaussian );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Gaussian",   invGaussian );

    _seg2d( "Higher Density", higherDensity, "Hyperbolic",      hyperbolic );
    _seg2d( "Lower Density",  lowerDensity,  "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 1",     gradient1,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 2",     gradient2,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 3",     gradient3,     "Hyperbolic",      hyperbolic );
    _seg2d( "Gradient 4",     gradient4,     "Hyperbolic",      hyperbolic );
    _seg2d( "Higher Density", higherDensity, "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Lower Density",  lowerDensity,  "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 1",     gradient1,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 2",     gradient2,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 3",     gradient3,     "Inv. Hyperbolic", invHyperbolic );
    _seg2d( "Gradient 4",     gradient4,     "Inv. Hyperbolic", invHyperbolic );
#undef _seg2d
}

void PersistentSegment2dFrame::restoreILWControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;

    //iterates
    wxString tmp;
    bool ok = RestoreValue( ilw_iterates, &tmp );
    if (!ok)    cerr << "restore ilw_iterates failed." << endl;
    else        w->doIterates( tmp );

    //min ctrl pts
    ok = RestoreValue( ilw_minCtrlPts, &tmp );
    if (!ok)    cerr << "restore ilw_minCtrlPts failed." << endl;
    else        w->doMinPoints( tmp );
}

void PersistentSegment2dFrame::restoreIntDLControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;

    wxString tmp;
    bool ok = RestoreValue( idl_model, &tmp );
    if (!ok)    cerr << "restore idl_model failed." << endl;
    /** \todo complete this */
    else {
        if (w->mIntDLControls != nullptr)
            w->mIntDLControls->doChoose( tmp );
    }
}

void PersistentSegment2dFrame::restoreLiveSnakeControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;

    //iterates
    wxString tmp;
    bool ok = RestoreValue( ls_iterates, &tmp );
    if (!ok)    cerr << "restore ls_iterates failed." << endl;
    else        w->doIterates( tmp );

    //alpha
    ok = RestoreValue( ls_alpha, &tmp );
    if (!ok)    cerr << "restore ls_alpha failed." << endl;
    else        w->doAlpha( tmp );

    //beta
    ok = RestoreValue( ls_beta, &tmp );
    if (!ok)    cerr << "restore ls_beta failed." << endl;
    else        w->doBeta( tmp );

    //gamma
    ok = RestoreValue( ls_gamma, &tmp );
    if (!ok)    cerr << "restore ls_gamma failed." << endl;
    else        w->doGamma( tmp );
}

void PersistentSegment2dFrame::restorePaintControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;

    wxString tmp;
    bool ok = RestoreValue( p_brushSize, &tmp );
    if (!ok)    cerr << "restore p_brushSize failed." << endl;
    else        w->doBrushSize( tmp.c_str() );
}

void PersistentSegment2dFrame::restoreTrainControls ( ) {
    auto w = Get();
    if (w == nullptr)    return;

    wxString tmp;
    bool ok = RestoreValue( t_brushSize, &tmp );
    if (!ok)    cerr << "restore t_brushSize failed." << endl;
    else        w->doBrushSize( tmp.c_str() );
}
//------------------------------------------------------------------------
/** set output controls are a special case (different from aux controls) */
void PersistentSegment2dFrame::restoreSetOutputControls ( ) {
    VERBOSE;
    if (!Preferences::getDejaVuMode())    return;
    auto w = Get();
    assert( w != nullptr );
    if (w == nullptr)    return;

    //restore output type
    wxString type;
    bool ok = RestoreValue( so_type, &type );
    if (ok) {
        w->doOutputType(type);
    } else {
        cerr << "can't restore Set Output type" << endl;
    }

    // restore out object no. (1..8 or All)
    wxString no;
    ok = RestoreValue( so_outObject, &no );
    if (ok) {
        //no callback for this one
        int which = w->mSetOutputControls->m_outputObject->FindString(no);
        if (which != wxNOT_FOUND) w->mSetOutputControls->m_outputObject->SetSelection(which);
    } else {
        cerr << "can't restore Set Output object number." << endl;
    }
}
//------------------------------------------------------------------------
