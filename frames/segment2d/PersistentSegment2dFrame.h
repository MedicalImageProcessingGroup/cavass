#pragma once

#include <wx/persist.h>
#include <wx/persist/toplevel.h>

class PersistentSegment2dFrame : public wxPersistentWindow< Segment2dFrame > {
public:
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
    //static const string c_slice;  //see si_slice in set index items instead
    static const string c_defaultFillOn;
    static const string c_layout;
    static const string c_object;
    static const string c_overlayOn;
    static const string c_t_x;
    static const string c_t_y;

    //two types of controls: non-mode and mode.

    //non-mode type of controls:
    // graymap, set index, and set output.
    // none or at most one may be selected at any time.
    //graymap items (independent of mode; may come and go)
    static const string gm_visible;
    static const string gm_invert;
    static const string gm_level;
    static const string gm_width;
    //set index items (independent of mode; may come and go)
    static const string si_visible;
    static const string si_labels;
    static const string si_scale;
    static const string si_slice;
    //set output items (independent of mode; may come and go)
    static const string so_visible;
    static const string so_outObject;
    static const string so_type;

    //mode type of controls:
    // feature, ilw, livesnake, paint, peek, report, review, and train.
    // exactly one _must_ be selected at any time.
    static const string m_lastMode;  ///< current mode (which one is selected)
    //features mode items
    static const string f_lastFeature;  ///< one of these features: HigherDensity, LowerDensity, Gradient1, Gradient2, Gradient3, Gradient4
    /*
    static const string f_higherDensity_on,
                        f_higherDensity_transform,  ///< transforms: Linear, Gaussian, InvLinear,  InvGaussian, Hyperbolic, InvHyperbolic
                        f_higherDensity_linear_weight,         f_higherDensity_linear_min,          f_higherDensity_linear_max,
                        f_higherDensity_invLinear_weight,      f_higherDensity_invLinear_min,       f_higherDensity_invLinear_max,
                        f_higherDensity_gaussian_weight,       f_higherDensity_gaussian_mean,       f_higherDensity_gaussian_stddev,
                        f_higherDensity_invGaussian_weight,    f_higherDensity_invGaussian_mean,    f_higherDensity_invGaussian_stddev,
                        f_higherDensity_hyperbolic_weight,     f_higherDensity_hyperbolic_mean,     f_higherDensity_hyperbolic_stddev,
                        f_higherDensity_invHyperbolic_weight,  f_higherDensity_invHyperbolic_mean,  f_higherDensity_invHyperbolic_stddev;
    */
#ifdef _seg2d
    #error "warning: redefining _seg2d macro!"
#endif
    //repeat the above definitions with a macro for all of the features
#define _seg2d( F )  \
    static const string  f_##F##_on,  \
                         f_##F##_lastTransform,  \
                         f_##F##_linear_weight,         f_##F##_linear_min,          f_##F##_linear_max,          \
                         f_##F##_invLinear_weight,      f_##F##_invLinear_min,       f_##F##_invLinear_max,       \
                         f_##F##_gaussian_weight,       f_##F##_gaussian_mean,       f_##F##_gaussian_stddev,     \
                         f_##F##_invGaussian_weight,    f_##F##_invGaussian_mean,    f_##F##_invGaussian_stddev,  \
                         f_##F##_hyperbolic_weight,     f_##F##_hyperbolic_mean,     f_##F##_hyperbolic_stddev,   \
                         f_##F##_invHyperbolic_weight,  f_##F##_invHyperbolic_mean,  f_##F##_invHyperbolic_stddev
    _seg2d( higherDensity );
    _seg2d( lowerDensity );
    _seg2d( gradient1 );
    _seg2d( gradient2 );
    _seg2d( gradient3 );
    _seg2d( gradient4 );
#undef _seg2d
    static const string ilw_iterates, ilw_minCtrlPts;  ///< ilw mode items
    static const string ls_iterates, ls_alpha, ls_beta, ls_gamma;  ///< livesnake mode items
    static const string p_brushSize;  ///< paint mode item
    //peek mode items:   none
    //report mode items: none
    //review mode items: none
    static const string t_brushSize;  ///< train mode item
    static const string idl_model;  ///< interactive deep learning item
    //------------------------------------------------------------------------
    explicit PersistentSegment2dFrame ( Segment2dFrame* w )
            : wxPersistentWindow<Segment2dFrame>( w )
    {
        output( "PersistentSegment2dFrame(Segment2dFrame): " );
    }
    //------------------------------------------------------------------------
    [[nodiscard]] wxString GetKind ( ) const override {
        output( "GetKind(): ", "returning \"Segment2dFrame\"" );
        return "Segment2dFrame";
    }
    //------------------------------------------------------------------------
    void Save ( ) const override;
    void saveGeneralSettings ( ) const;
    void saveInteractive2dSettings ( ) const;
    void saveGrayMapSettings ( ) const;
    void saveSetIndexSettings ( ) const;
    void saveSetOutputSettings ( ) const;
    void saveIntDLSettings ( ) const;
    void saveAuxControls ( ) const;
    //specific aux controls:
    void saveFeatureControls ( ) const;
    void saveILWControls ( ) const;
    void saveLiveSnakeControls ( ) const;
    void savePaintControls ( ) const;
    void saveTrainControls ( ) const;

    bool Restore ( ) override;
    void restoreControlSettings ( );
    void restoreIntDLControls ( );
    //specific aux controls:
    void restoreFeatureControls ( );
    void restoreILWControls ( );
    void restoreLiveSnakeControls ( );
    void restorePaintControls ( );
    void restoreTrainControls ( );

    //set output controls are a special case (different from aux controls)
    void restoreSetOutputControls ( );
    //------------------------------------------------------------------------
private:
    /** debugging output.
     *  \todo this should be a pretty-print friend function for Segment2dFrame.
     */
    void output ( const string& prefix="", const string& postfix="" ) const {
#if 0  //only for debugging
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
        auto canvas = dynamic_cast<Segment2dCanvas*>( w->mCanvas );
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
#endif
    }

};
//----------------------------------------------------------------------------
inline wxPersistentObject* wxCreatePersistentObject ( Segment2dFrame* w ) {
#if 0  //only for debugging
    cout << "wxCreatePersistentObject( Segment2dFrame=0x" << hex << w << dec
         << " (" << w->GetName() << ") )" << endl;
#endif
    return new PersistentSegment2dFrame( w );
}
//------------------------------------------------------------------------
