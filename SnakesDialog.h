//======================================================================
// SnakesDialog
//----------------------------------------------------------------------
#ifndef __SnakesDialog_h
#define __SnakesDialog_h

#include  <fstream>
#include  <vector>

class MainFrame;
class MontageFrame;

//note: the following may be 'or'ed together
enum {
    User     =  1,  //original (anchor) point specified by user
    Moveable =  2,  //can move in any direction
    Fixed    =  4,  //can't move at all in any direction
    FixedX   =  8,  //can't move in X
    FixedY   = 16,  //can't move in Y
    FixedZ   = 32,  //can't move in Z
    Contour  = 64,  //added during contour completion
    External =128   //loaded from external file
};
//----------------------------------------------------------------------
template< typename T > class p3d {
  public:
    T  x, y, z;
    unsigned long  flags;

    p3d ( const T x=0.0, const T y=0.0, const T z=0.0,
          const unsigned long flags=Moveable )
    {
        this->x=x;  this->y=y;  this->z=z;
        this->flags = flags;
    }
    //binary operator(s)
    inline p3d operator+ ( const p3d& rhs ) const {
        return p3d(this->x+rhs.x, this->y+rhs.y, this->z+rhs.z, this->flags);
    }
    inline p3d operator- ( const p3d& rhs ) const {
        return p3d(this->x-rhs.x, this->y-rhs.y, this->z-rhs.z, this->flags);
    }
    inline p3d& operator+= ( const p3d& rhs ) {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        return *this;
    }
    inline p3d operator* ( const T rhs ) const {
        return p3d(this->x*rhs, this->y*rhs, this->z*rhs, this->flags);
    }
    inline friend p3d operator* ( const T lhs, const p3d& rhs ) {
        return p3d(lhs*rhs.x, lhs*rhs.y, lhs*rhs.z, rhs.flags);
    }
    inline p3d operator/ ( const T rhs ) const {
        return p3d(this->x/rhs, this->y/rhs, this->z/rhs, this->flags);
    }
    inline p3d& operator*= ( const T rhs ) {
        this->x *= rhs;
        this->y *= rhs;
        this->z *= rhs;
        return *this;
    }
    inline p3d& operator/= ( const T rhs ) {
        this->x /= rhs;
        this->y /= rhs;
        this->z /= rhs;
        return *this;
    }

    inline friend double mag ( const p3d& p ) {
        return sqrt( p.x*p.x + p.y*p.y + p.z*p.z );
    }
    inline friend std::ostream& operator<< ( std::ostream &out, const p3d& p ) {
        out << "(" << p.x << "," << p.y << "," << p.z << ",f=";
        if (p.flags==0)            out << " none";
        if (p.flags & User)        out << " user";
        if (p.flags & Moveable)    out << " moveable";
        if (p.flags & Fixed)       out << " fixed";
        if (p.flags & FixedX)      out << " fixedX";
        if (p.flags & FixedY)      out << " fixedY";
        if (p.flags & FixedZ)      out << " fixedZ";
        if (p.flags & Contour)     out << " contour";
        out << ")";
        return out;
    }
    inline friend std::ostream& operator<< ( std::ostream &out,
        const p3d* const p )
    {
        out << *p;
        //out << "(" << p->x << "," << p->y << "," << p->z << ")";
        return out;
    }
};
//----------------------------------------------------------------------
class SnakesDialog : public wxDialog {
  private:
    int               mMode;
    #define MAXPOINTS 100
    int               mData[MAXPOINTS][3];  //user indicated/specified points
    bool              mExtendToEdges;
    bool              mCompleteContour;
    fstream           mLogfile;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    vector< p3d<double>* >    mPoints;  //current/active contour

    MontageFrame*   m_parent;

    wxButton*         m_update;
    wxButton*         m_clear;
    wxButton*         m_previous;
    wxButton*         m_next;
    wxButton*         m_save;
    wxButton*         m_save_all;
    wxButton*         m_close;
    wxButton*         m_debug;
    wxButton*         m_process_all;

    bool*             m_processed_slice;  //flag indicating whether or not a
                                          // slice has been processed yet
    int*              m_processed_data;   //actual binary result of segmentation
    vector< p3d<double>* >*  m_processed_points;  //a vector of points (contours) for each slice

    int               m_userpoints;
    wxStaticText*     m_userpoints_st;
    wxTextCtrl*       m_userpoints_text;
    wxSlider*         m_userpoints_slider;

    int               m_discretization;
    wxStaticText*     m_discretization_st;
    wxTextCtrl*       m_discretization_text;
    wxSlider*         m_discretization_slider;

    double            m_tensile;
    wxStaticText*     m_tensile_st;
    wxTextCtrl*       m_tensile_text;
    wxSlider*         m_tensile_slider;

    double            m_flexural;
    wxStaticText*     m_flexural_st;
    wxTextCtrl*       m_flexural_text;
    wxSlider*         m_flexural_slider;

    double            m_external;
    wxStaticText*     m_external_st;
    wxTextCtrl*       m_external_text;
    wxSlider*         m_external_slider;

    double            m_inflation;
    wxStaticText*     m_inflation_st;
    wxTextCtrl*       m_inflation_text;
    wxSlider*         m_inflation_slider;

    int               m_threshold;
    wxStaticText*     m_threshold_st;
    wxTextCtrl*       m_threshold_text;
    wxSlider*         m_threshold_slider;

    int               m_iterations;
    wxStaticText*     m_iterations_st;
    wxTextCtrl*       m_iterations_text;
    wxSlider*         m_iterations_slider;

    int               m_radius;  //of neighborhoods
    wxStaticText*     m_radius_st;
    wxTextCtrl*       m_radius_text;
    wxSlider*         m_radius_slider;

    bool              m_show_normals;
    wxCheckBox*       m_show_normals_checkbox;

    enum {
        ID_UPDATE, ID_PROCESS_ALL, ID_CLEAR, ID_PREVIOUS, ID_NEXT, ID_DEBUG,
        ID_SAVE, ID_SAVE_ALL, ID_LOAD,

        ID_USERPOINTS,     ID_USERPOINTS_TEXT,
        ID_DISCRETIZATION, ID_DISCRETIZATION_TEXT,

        ID_TENSILE,        ID_TENSILE_TEXT,
        ID_FLEXURAL,       ID_FLEXURAL_TEXT,
        ID_EXTERNAL,       ID_EXTERNAL_TEXT,
        ID_INFLATION,      ID_INFLATION_TEXT,
        ID_THRESHOLD,      ID_THRESHOLD_TEXT,
        ID_ITERATIONS,     ID_ITERATIONS_TEXT,
        ID_RADIUS,         ID_RADIUS_TEXT,

        ID_SHOW_NORMALS
    };

  public:
    SnakesDialog ( MontageFrame* parent=NULL );
    ~SnakesDialog ( void ) {
        cout << "~SnakesDialog" << endl;
        wxLogMessage( "~SnakesDialog" );
        mLogfile.close();
    }
    void init  ( void );
    void reset ( void );
    void setupSliders ( void );

    void OnUserpointsSlider             ( wxScrollEvent&   e );
    void OnUserpointsTextEnter          ( wxCommandEvent&  e );

    void OnDiscretizationSlider         ( wxScrollEvent&   e );
    void OnDiscretizationTextEnter      ( wxCommandEvent&  e );

    void OnTensileSlider                ( wxScrollEvent&   e );
    void OnTensileTextEnter             ( wxCommandEvent&  e );

    void OnFlexuralSlider               ( wxScrollEvent&   e );
    void OnFlexuralTextEnter            ( wxCommandEvent&  e );

    void OnExternalSlider               ( wxScrollEvent&   e );
    void OnExternalTextEnter            ( wxCommandEvent&  e );

    void OnInflationSlider              ( wxScrollEvent&   e );
    void OnInflationTextEnter           ( wxCommandEvent&  e );

    void OnThresholdSlider              ( wxScrollEvent&   e );
    void OnThresholdTextEnter           ( wxCommandEvent&  e );

    void OnIterationsSlider             ( wxScrollEvent&   e );
    void OnIterationsTextEnter          ( wxCommandEvent&  e );

    void OnRadiusSlider                 ( wxScrollEvent&   e );
    void OnRadiusTextEnter              ( wxCommandEvent&  e );
#ifdef __WXX11__
    void OnUserpointsSliderUpdateUI     ( wxUpdateUIEvent& e );
    void OnDiscretizationSliderUpdateUI ( wxUpdateUIEvent& e );
    void OnTensileSliderUpdateUI        ( wxUpdateUIEvent& e );
    void OnFlexuralSliderUpdateUI       ( wxUpdateUIEvent& e );
    void OnExternalSliderUpdateUI       ( wxUpdateUIEvent& e );
    void OnInflationSliderUpdateUI      ( wxUpdateUIEvent& e );
    void OnThresholdSliderUpdateUI      ( wxUpdateUIEvent& e );
    void OnIterationsSliderUpdateUI     ( wxUpdateUIEvent& e );
    void OnRadiusSliderUpdateUI         ( wxUpdateUIEvent& e );
#endif
    void OnShowNormals ( wxCommandEvent& e );

    void OnUpdate     ( wxCommandEvent& e );
    void OnProcessAll ( wxCommandEvent& e );
    void OnClear      ( wxCommandEvent& e );
    void OnPrevious   ( wxCommandEvent& e );
    void OnNext       ( wxCommandEvent& e );
    void OnClose      ( wxCommandEvent& e );
    void OnDebug      ( wxCommandEvent& e );
    void OnSave       ( wxCommandEvent& e );
    void OnSaveAll    ( wxCommandEvent& e );
    void OnLoad       ( wxCommandEvent& e );

    void HandleChar      ( wxKeyEvent& e );
    void HandleMouseMove ( wxMouseEvent& e );
    void HandleRightDown ( wxMouseEvent& e );
    void HandleLeftDown  ( wxMouseEvent& e );
    void HandleLeftUp    ( wxMouseEvent& e );
    void HandlePaint     ( wxPaintEvent& e, wxMemoryDC& m );

    void OnCornerSliceChange ( void );

  private:
    void checkAllProcessed ( void );
    double calculateEnergy ( void );
    p3d<double> gradient2D ( const int x, const int y, const int z ) const;
    p3d<double> gradient2D ( const double x, const double y, const double z )
        const;

    void updateContour ( void );

    double getLocalMaxContinuity ( const int i, const double dBar ) const;
    double getLocalMaxCurvature2 ( const int i ) const;
    void getLocalMinMaxGradientMag( const int i, double& min, double& max )
        const;
    double getDBar ( void ) const;

    //the following versions are called when we are summing up the individual
    // costs along a path.
    double localE ( const int i, const double dBar ) const;
    double localEContinuity ( const int i, const double dBar ) const;
    double localECurvature ( const int i ) const;
    double localEImage ( const int i ) const;

    //the following versions are called when we are evaluating possible changes
    // along a path.
    double localE ( const int i, const double dBar,
        const double localMaxContinuity, const double localMaxCurvature,
        const double minGradMag, const double maxGradMag ) const;
    double localEContinuity ( const int i, const double dBar,
        const double localMaxContinuity ) const;
    double localECurvature ( const int i,
        const double localMaxCurvature2 ) const;
    double localEImage ( const int i, const double min,
                         const double max ) const;

    void segment ( void );
    void flood ( const int x, const int y, const int z );
    void timestamp ( void );

  private:
    DECLARE_DYNAMIC_CLASS(SnakesDialog)
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================

