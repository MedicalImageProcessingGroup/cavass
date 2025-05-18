#pragma once

#include "Segment2dFrame.h"
#include "PersistentSegment2dFrame.h"
#include "CCheckBox.h"

/**
 * \brief new aux-like control for interactive deep learning.
 * rather than adding another variation of an aux control, this is made
 * entirely separate for simplicity.
 */
class Segment2dIntDLControls {
    friend PersistentSegment2dFrame;
    friend Segment2dFrame;

    bool mModelLoaded = false;            ///< waiting to load model file
    bool mFirstCornerSpecified = false;   ///< waiting for left mouse down indicating first corner of rect
    bool mSecondCornerSpecified = false;  ///< left dragging & waiting for mouse up indicating second corner

    Segment2dFrame*   mFr;
    wxPanel*          mCp;
    wxSizer*          mBottomSizer;            ///< DO NOT DELETE in dtor!
    wxStaticBox*      mAuxBox;
    wxStaticBoxSizer* mAuxSizer;
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgs_left;
    wxFlexGridSizer*  mFgs_right;

    wxStaticText*     mSt1;
    wxButton*         mAdd;
    wxButton*         mBlink;
    wxButton*         mClear;
    wxButton*         mChoose;
    wxButton*         mRun;
    wxTextCtrl*       mFileNameCtrl;         ///< model file name (w/o _and_ w/ path)
    CCheckBox*        mUseGPU;               ///< use gpu (or cpu)

    wxString          mFullFileNameStr;      ///< model file name including path (and ext)
    wxString          mPath;                 ///< path to model file (w/o file name)
    wxString          mFileName;             ///< model file name (w/o path)

    int               m_sliceNo = -1;        ///< slice number (in z dir)
    wxString          mInSlice;              ///< name of temp input (to dl seg) slice file
    wxString          mOutSlice;             ///< name of temp output slice file (result of dl seg)

    //for selecting a rectangle:
    int               mdx1 = -1, mdy1 = -1;  ///< location of first click (in data subscript coords)
    int               mdx2 = -1, mdy2 = -1;  ///< location of second point while dragging (in data subscript coords)

    int               mWidth  = -1;          ///< width  (cols) of result in pixels; cols in slice
    int               mHeight = -1;          ///< height (rows) of result in pixels; rows in slice
    unsigned char*    mResult = nullptr;     ///< result of seg; rgb; blue value indicates segmentation
    unsigned char*    mAlpha  = nullptr;     ///< alpha transparency for mResult; binary; semi-opaque (not transparent) where the segmentation is present.
    wxBitmap*         mBitmap = nullptr;     ///< result of seg as a bitmap
    bool              mShow   = true;        ///< paint() should show bitmap
    double            mScale  = -1;          ///< image scale factor (to keep track of changes)

    void saveCurrentSlice ( bool ascii = true );
    void loadModel ( wxString& fn );
    void loadResult ( );
    void reset ( );
    void setButtonState ( );
public:
    /** based on paint controls */
    Segment2dIntDLControls ( Segment2dFrame* frame, wxPanel* cp, wxSizer* bottomSizer );
    ~Segment2dIntDLControls ( );

    void restoreIntDLControls ( );

    // button callbacks
    void doAdd ( );
    void doBlink ( );
    void doChoose ( wxString str );
    void doClear ( );
    void doRun ( );

    void doLeftDown ( int x, int y );
    void doMouseMove ( wxMouseEvent& e, int x, int y );
    void doLeftUp ( );
    void doRightDown ( );
    void doMiddleDown ( );
    void doMiddleUp ( );
    void doPaint ( wxDC* dc, int x, int y );
};
