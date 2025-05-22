#ifdef BUILD_WITH_TORCH
#include <torch/torch.h>
#include <torch/script.h>  //for loading TorchScript models
#endif

#include "cavass.h"
#include "Segment2dIntDLControls.h"
#include "PersistentSegment2dFrame.h"

#ifndef WIN32
#define VERBOSE        cout << __PRETTY_FUNCTION__  << endl
#else
#define VERBOSE        /*this space intentionally left blank*/
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * \brief ctor (obviously).
 * layout is similar to Feature controls (in aux controls).
 */
Segment2dIntDLControls::Segment2dIntDLControls ( Segment2dFrame* frame,
                                                 wxPanel* cp,
                                                 wxSizer* bottomSizer )
{

    /** \todo determine how to control the size of this! */

    VERBOSE;
    mFr = frame;
    mCp = cp;
    mBottomSizer = bottomSizer;
    //box for controls
    mAuxBox = new wxStaticBox( mCp, wxID_ANY, "Int DL Controls" );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
//    mAuxSizer->SetMinSize(cp->GetSize().x/6, 0);
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on left
    mFgs = new wxFlexGridSizer( 0, 3, 0, 0 );  //rows, cols, vgap, hgap
//    mFgs->SetMinSize(cp->GetSize().x/6, 0);
    mFgs->SetFlexibleDirection( wxBOTH );
    mFgs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    //left
	mFgs_left = new wxFlexGridSizer( 0, 2, 0, 0 );
	mFgs_left->SetFlexibleDirection( wxBOTH );
	mFgs_left->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	mRun = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_INTDL_RUN, _("Run"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_left->Add( mRun, 0, wxALL, 5 );

	mAdd = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_INTDL_ADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_left->Add( mAdd, 0, wxALL, 5 );

	mClear = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_INTDL_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_left->Add( mClear, 0, wxALL, 5 );

	mBlink = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_INTDL_BLINK, _("Blink"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_left->Add( mBlink, 0, wxALL, 5 );

	mFgs->Add( mFgs_left, 1, wxEXPAND, 5 );

    //middle
	mFgs->Add( 30, 0, 1, wxEXPAND, 5 );  //spacer

    //right
	mFgs_right = new wxFlexGridSizer( 0, 3, 0, 0 );
	mFgs_right->SetFlexibleDirection( wxBOTH );
	mFgs_right->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	mSt1 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("Model file:"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_right->Add( mSt1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	mFileNameCtrl = new wxTextCtrl( mAuxSizer->GetStaticBox(), wxID_ANY, "<none>", wxDefaultPosition, wxSize(400,-1), wxTE_READONLY|wxTE_MULTILINE );
	mFgs_right->Add( mFileNameCtrl, 1, wxALL, 5 );

	mChoose = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_INTDL_CHOOSE, _("Choose"), wxDefaultPosition, wxDefaultSize, 0 );
	mFgs_right->Add( mChoose, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	mFgs->Add( mFgs_right, 2, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    mUseGPU = new CCheckBox( mAuxSizer->GetStaticBox(), wxID_ANY, "use GPU" );
    mFgs->Add( mUseGPU, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 10 );

    mAuxSizer->Add( mFgs, 0, 0, 0 );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on right
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();

    setButtonState();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief dtor (obviously) */
Segment2dIntDLControls::~Segment2dIntDLControls ( ) {
    VERBOSE;
    if (mResult != nullptr) {
        delete mResult;
        mResult = nullptr;
    }
    if (mAlpha != nullptr) {
        delete mAlpha;
        mAlpha = nullptr;
    }
    if (mBitmap != nullptr) {
        delete mBitmap;
        mBitmap = nullptr;
    }
    if (mFr->mPersistentMe != nullptr) {
        auto p = dynamic_cast<PersistentSegment2dFrame*>( mFr->mPersistentMe );
        //update persisted values
        if (p != nullptr)    p->saveAuxControls();
    }

    assert( mFgs_left != nullptr );
    if (mAdd != nullptr) {
        mFgs_left->Detach( mAdd );
        delete mAdd;
        mAdd = nullptr;
    }
    if (mBlink != nullptr) {
        mFgs_left->Detach( mBlink );
        delete mBlink;
        mBlink = nullptr;
    }
    if (mClear != nullptr) {
        mFgs_left->Detach( mClear );
        delete mClear;
        mClear = nullptr;
    }
    if (mRun != nullptr) {
        mFgs_left->Detach( mRun );
        delete mRun;
        mRun = nullptr;
    }

    assert( mFgs_right != nullptr );
    if (mSt1 != nullptr) {
        mFgs_right->Detach( mSt1 );
        delete mSt1;
        mSt1 = nullptr;
    }
    if (mFileNameCtrl != nullptr) {
        mFgs_right->Detach( mFileNameCtrl );
        delete mFileNameCtrl;
        mFileNameCtrl = nullptr;
    }
    if (mChoose != nullptr) {
        mFgs_right->Detach( mChoose );
        delete mChoose;
        mChoose = nullptr;
    }

    assert( mFgs != nullptr );
    mFgs->Remove( mFgs_left  );
    mFgs_left = nullptr;
    mFgs->Remove( mFgs_right );
    mFgs_right = nullptr;
    if (mUseGPU != nullptr) {
        mFgs->Detach( mUseGPU );
        delete( mUseGPU );
        mUseGPU = nullptr;
    }

    mAuxSizer->Remove( mFgs );
    mFgs = nullptr;

    mBottomSizer->Remove( mAuxSizer );
    mAuxSizer = nullptr;

    mBottomSizer->Layout();  //keep the bottom sizer for reuse
    mCp->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dIntDLControls::restoreIntDLControls ( ) {
    VERBOSE;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * an error like the following:
 *     error loading model: PytorchStreamReader failed locating file 
 *     constants.pkl: file not found
 * indicates that the model file was (most likely) not saved properly (for 
 * input to c/c++ programs).
 *
 * "usual" python way to save a model:
 *     torch.save( model.state_dict(), "mnist_cnn.pt" )
 * (python) code to save a model for input to c/c++:
 *     #create scripted model from "ordinary" model
 *     sm = torch.jit.script( model )
 *     torch.jit.save( sm, "mnist_cnn.scripted.pt" )
 */
void Segment2dIntDLControls::loadModel ( wxString& fn ) {
    VERBOSE;
    if (fn.empty())    return;
    wxFileName tmp = fn;
    if (!tmp.FileExists())        return;    //does it exist?
    if (!tmp.IsFileReadable())    return;    //is it readable?
#if 1
    /**
     * send request to server to load model file.
     * if unsuccessful, then return (w/out changing current model, if any).
     * otherwise, update model info (as per below).
     */

    /** \todo send request to server to load model file. */

    //update model info after successful load
    mFullFileNameStr = fn;
    mPath = tmp.GetPath();
    mFileName = tmp.GetFullName();  //fn w/o path but w/ ext
    mFileNameCtrl->SetValue( mFileName + "\n(" + mFullFileNameStr + ")" );  //full fn incl path in parens on a separate line
#endif
#if 0
    saveCurrentSlice();
    // cd /home/jorge/mipg/MedSAM && ./.venv/bin/python gjg.py --data_path /tmp/cavass-seg2dintdl-fVNnAB.pgm --seg_path /tmp
    // cd /home/jorge/mipg/MedSAM && /home/jorge/mipg/MedSAM/.venv/bin/python /home/jorge/mipg/MedSAM/gjg.py  --data_path /tmp/cavass-seg2dintdl-lDTvjk.pgm --seg_path /tmp
    // cd /home/jorge/mipg/MedSAM && ./.venv/bin/python gjg.py  --data_path /tmp/cavass-seg2dintdl-lDTvjk.pgm --seg_path /tmp
    //const char* tmp = "cd /home/jorge/mipg/MedSAM && ./.venv/bin/python gjg.py  --data_path /tmp/cavass-seg2dintdl-lDTvjk.pgm --seg_path /tmp";
    wxString tmp = wxString::Format(
            "cd /home/jorge/mipg/MedSAM && ./.venv/bin/python gjg.py --data_path %s --seg_path /tmp",
            mInSlice.c_str() );
    cout << tmp << endl;
    mFr->SetStatusText( "loading ...", 0 );
    wxSetCursor( wxCursor(wxCURSOR_WAIT) );
    wxYield();
    auto result = system( tmp );
    cout << result << endl;
    wxSetCursor( *wxSTANDARD_CURSOR );
    if (!mFirstCornerSpecified || !mSecondCornerSpecified) {
        mFr->SetStatusText("Loaded. Select a rectangular region.", 0);
    } else {
        mFr->SetStatusText("Loaded. Run (or select a different region or model).", 0);
    }
#endif
#if 0
    //load the model directly (no separate server)
    torch::jit::script::Module module;
    try {
        const char* tmp = fn.mb_str();
        module = torch::jit::load( tmp );
    } catch (const c10::Error& e) {
        std::cerr << "error loading model: " << e.msg() << std::endl;
        return;
    } catch (...) {
        std::cerr << "unrecognized exception \n";
        return;
    }
#endif
    mModelLoaded = true;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * \brief save current slice to temp file in PGM P2 (grey, ASCII) or P5 (grey,
 * binary) format for input to dl seg. P2 ascii format allows for 16-bit data
 * and is generally supported by PGM readers. (however, i determined that the
 * python PGM reader will NOT read 16-bit P5 (binary) data.)
 *
 * pgm format ref: https://netpbm.sourceforge.net/doc/pgm.html
 *
 * @param ascii is true (default) for ascii output; otherwise, 16-bit binary.
 */
void Segment2dIntDLControls::saveCurrentSlice ( bool ascii ) {
    VERBOSE;
    //step 1: determine max value in slice
    int rows = mFr->mCanvas->mCavassData->m_ySize;
    int cols = mFr->mCanvas->mCavassData->m_xSize;
    int z    = mFr->mCanvas->mCavassData->m_sliceNo;
    //determine max gray value in slice
    int max  = mFr->mCanvas->mCavassData->getData( 0, 0, z );
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int v = mFr->mCanvas->mCavassData->getData(x, y, z);
            if (v > max)    max = v;
        }
    }
    //step 2: output pgm file header
    mInSlice = wxFileName::CreateTempFileName( "cavass-seg2dintdl-" );
    mInSlice += ".pgm";    //add .pgm extension
    cout << "output file: " << mInSlice << endl;
    mOutSlice = wxFileName( mInSlice ).GetPath() + wxFileName::GetPathSeparator() + "seg_" + wxFileName( mInSlice ).GetFullName();

    FILE* fp = fopen( mInSlice.c_str(), "wb" );
    assert( fp != nullptr );
    if (ascii) {
        if (max < 256)    cout << "output in 8-bit ascii" << endl;
        else              cout << "output in 16-bit ascii" << endl;
        // P2 ASCII supports 16-bit data
        fprintf( fp, "P2\n" );
        fprintf( fp, "%d %d\n", cols, rows );
        fprintf( fp, "%d\n", max );
        //step 3: output slice values
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int v = mFr->mCanvas->mCavassData->getData(x, y, z);
                fprintf( fp, "%d ", v );
            }
            fprintf( fp, "\n" );
        }
    } else {
        /** \todo may want to support 8-bit binary as well (when max < 256) */
        cout << "output in 16-bit binary" << endl;
        // P5 16-bit binary
        fprintf( fp, "P5\n" );
        fprintf( fp, "%d %d\n", cols, rows );
        fprintf( fp, "%d\n", max );
        if (max < 256)    max = 256;
        auto tmp = new unsigned short[rows*cols];
        assert( tmp != nullptr );
        for (int y=0,i=0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                tmp[i++] = mFr->mCanvas->mCavassData->getData(x, y, z);
            }
        }
        fwrite( tmp, sizeof(unsigned short), rows*cols, fp );
        delete[] tmp;
        tmp = nullptr;
    }
    fclose( fp );
    fp = nullptr;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** called when Add button is pressed */
void Segment2dIntDLControls::doAdd ( ) {
    VERBOSE;
    if (mAlpha == nullptr)    return;  //seg not performed yet
    mShow = true;
    auto c = dynamic_cast<Segment2dCanvas*>( mFr->mCanvas );
    cout << "object_mask_slice_index = " << c->object_mask_slice_index << endl;
    cout << "object_number = " << c->object_number << endl;

    //onbit is defined in Segment2dCanvas.[h,cpp]. it defines individual
    // object_mask values in positions [0..7] and onbit[8] = 255 (all 8 bits
    // on). this corresponds with the object selection in the Interactive2D
    // control panel values 1..8,All.

    int which = mFr->get_object_selection_number();
    which = onbit[ which ];

    //mAlpha contains the result of segmentation (0=no object; otherwise, object)
    int rows = mFr->mCanvas->mCavassData->m_ySize;
    int cols = mFr->mCanvas->mCavassData->m_xSize;
    int z    = mFr->mCanvas->mCavassData->m_sliceNo;
    for (int y=0,i=0; y<rows; y++) {
        for (int x=0; x<cols; x++) {
            if (mAlpha[i] != 0) {
                c->object_mask[i] |= which;
            }
            if (c->object_mask[i] != 0) {
                cout << "(x,y)=(" << x << "," << y << ")=" << (int)c->object_mask[i] << endl;
            }
            ++i;
        }
    }

    //the following is no longer needed (so reset to initial state)
    reset();
#if 0
    delete mResult;    mResult = nullptr;
    delete mAlpha;     mAlpha  = nullptr;
    delete mBitmap;    mBitmap = nullptr;
    mFirstCornerSpecified = mSecondCornerSpecified = false;
    mdx1 = mdy1 = mdx2 = mdy2 = -1;
#endif
    mFr->refresh_object_selection();
    //mFr->mCanvas->Refresh();  //cause repaint
}

/** called when Blink button is pressed */
void Segment2dIntDLControls::doBlink ( ) {
    // VERBOSE;
    mShow = !mShow;
    mFr->mCanvas->Refresh();  //cause repaint
}

/** called when Choose button is pressed */
void Segment2dIntDLControls::doChoose ( wxString str ) {
    VERBOSE;
    if (str.empty())    return;
    loadModel( str );
    if (!mModelLoaded) {
        mFr->SetStatusText("Please choose a model.", 0);
    } else if (!mFirstCornerSpecified || !mSecondCornerSpecified) {
        mFr->SetStatusText("Please select a rectangle.", 0);
    } else {
        mFr->SetStatusText( "Run.", 0 );
    }
}

/** called when Clear button is pressed */
void Segment2dIntDLControls::doClear ( ) {
    VERBOSE;
    reset();
    mFr->mCanvas->Refresh();  //cause repaint
    setButtonState();
}

void Segment2dIntDLControls::reset ( ) {
    VERBOSE;
    if (mResult != nullptr) {  delete mResult;    mResult = nullptr;  }
    if (mAlpha  != nullptr) {  delete mAlpha;     mAlpha  = nullptr;  }
    if (mBitmap != nullptr) {  delete mBitmap;    mBitmap = nullptr;  }
    mFirstCornerSpecified = mSecondCornerSpecified = false;
    mdx1 = mdy1 = mdx2 = mdy2 = -1;
    mdx1 = mdy1 = mdx2 = mdy2 = -1;
    //m_sliceNo = c->mCavassData->m_sliceNo;
    mShow = true;
}


/**
 * run can be initiated via a button press in the controls, or by a right
 * click in the canvas.
 */
void Segment2dIntDLControls::doRun ( ) {
    VERBOSE;
    //rectangle specified?
    if (!mFirstCornerSpecified || !mSecondCornerSpecified) {
        mFr->SetStatusText("Please select a rectangular region.", 0);
        return;
    }
    //model specified?
    if (!mModelLoaded) {
        //display a dialog that allows the user to choose an input file
        auto str = wxFileSelector( _T("Select model/checkpoint file"), _T(""), _T(""),
                                   _T(""),
                                   /** \todo check if upper/lower case mix is necessary on win and mac */
                                   "model files (*.pt;*.pth;*.PT;*.PTH)|*.pt;*.pth;*.PT;*.PTH",
                                   wxFILE_MUST_EXIST );
        loadModel( str );
        if (!mModelLoaded) {
            mFr->SetStatusText("Please choose a model.", 0);
            return;
        }
    }

    //save current slice to temp file in PGM P2 (grey, ASCII) format
    /** \todo avoid doing this more than once per a specific slice */
    saveCurrentSlice();

    //determine top-left of rect
    int top  = (mdy1 <= mdy2) ? mdy1 : mdy2;
    int left = (mdx1 <= mdx2) ? mdx1 : mdx2;
    //determine width and height of rect
    int w = mdx2 - mdx1;
    int h = mdy2 - mdy1;
    if (w < 0)    w = -w;
    if (h < 0)    h = -h;
    //calc bottom-right of rect
    int bottom = top + h;
    int right = left + w;

    //inference.py is assumed to be in the cavass build directory (which is
    // assumed to be in path). if it isn't in path, something like the
    // following may be used instead:
    // "cd /home/jorge/mipg/MedSAM && ./.venv/bin/python gjg.py --box [%d,%d,%d,%d] --data_path %s --device cpu --seg_path /tmp --checkpoint %s",
    // python inference.py --box [84,309,114,343] --data_path C:\Users\george\AppData\Local\Temp\cav3FB9.tmp.pgm --device cpu --seg_path C:\Users\george\AppData\Local\Temp --checkpoint C:\Users\george\Desktop\medsam_vit_b.pth
    wxString sep = wxFileName::GetPathSeparator();
    wxString device = (mUseGPU->IsChecked()) ? "gpu" : "cpu";
    wxString path = Preferences::getHome() + sep + "dist" + sep + "inference" + sep;
    wxString tmp = wxString::Format(
                // "python inference.py --box [%d,%d,%d,%d] --data_path %s --device %s --seg_path %s --checkpoint %s",
                "%sinference --box [%d,%d,%d,%d] --data_path %s --device %s --seg_path %s --checkpoint %s",
                path.c_str(),
                left, top, right, bottom,
                mInSlice.c_str(),
                device.c_str(),
                wxFileName( mOutSlice ).GetPath().c_str(),
                mFullFileNameStr.c_str() );
    wxLogMessage( "cmd: " + tmp );
    cout << "cmd: " << tmp << endl;
    mFr->SetStatusText( "running ...", 0 );
    wxSetCursor( wxCursor(wxCURSOR_WAIT) );
    wxYield();
    auto result = system( tmp );
    cout << "result: " << result << endl;
    wxLogMessage( wxString::Format("result: %d", result) );
    if (result != 0) {
        wxMessageBox( "Inference failed!", "Error", wxICON_ERROR );
    }
    mFr->SetStatusText( "loading result ...", 0 );
    loadResult();
    mFr->SetStatusText( "done", 0 );
    wxSetCursor( *wxSTANDARD_CURSOR );
    mShow = true;
    mFr->mCanvas->Refresh();  //cause repaint
    setButtonState();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @param x is in terms of data subscript (col)
 * @param y is in terms of data subscript (row)
 */
void Segment2dIntDLControls::doLeftDown ( int x, int y ) {
    VERBOSE;
    mShow = true;
    int z = mFr->mCanvas->mCavassData->m_sliceNo;
    int v = mFr->mCanvas->mCavassData->getData( x, y, z );
    //show data value at this location
    mFr->SetStatusText( wxString::Format( "(%d,%d,%d)=%d", x, y, z, v ), 1 );
    //reset & save location of first click
    reset();
    mdx1 = x;
    mdy1 = y;
    mFirstCornerSpecified = true;
    //move to next state (waiting for 2nd corner while dragging until mouse button up)
//    mdx2 = mdy2 = -1;
//    mSecondCornerSpecified = false;
    //mFr->mCanvas->reload();  //cause repaint
    mFr->mCanvas->Refresh();  //cause repaint
    setButtonState();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 *
 * @param x coord is in terms of data subscript (col)
 * @param y coord is in terms of data subscript (row)
 */
void Segment2dIntDLControls::doMouseMove ( wxMouseEvent& e, int x, int y ) {
    // VERBOSE;
    int z = mFr->mCanvas->mCavassData->m_sliceNo;
    int v = mFr->mCanvas->mCavassData->getData( x, y, z );
    //report data value at current position
    mFr->SetStatusText( wxString::Format( "(%d,%d,%d)=%d", x, y, z, v ), 1 );
    if (!e.LeftIsDown())    return;  //not dragging
    //track 2nd position while dragging
    cout << "2nd pos" << endl;
    mdx2 = x;
    mdy2 = y;
    //mSecondCornerSpecified = true;
    mFr->mCanvas->Refresh();  //cause repaint
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * \todo should this record the location of the "up" (as doMouseMove does) by calling it with (x,y)?
 */
void Segment2dIntDLControls::doLeftUp ( ) {
    VERBOSE;
    if (mModelLoaded) {
        mFr->SetStatusText("", 0);
        mFr->SetStatusText("run", 4);
    } else {
        mFr->SetStatusText("choose a model, then run", 0);
    }
//    mFr->SetStatusText( "run", 4 );
    if (mFirstCornerSpecified)    mSecondCornerSpecified = true;
    mFr->mCanvas->Refresh();  //cause repaint

    setButtonState();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dIntDLControls::doRightDown ( ) {
    doRun();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * \brief load the result (from a P5 pgm file).
 * mAlpha will contain the transparency for the segmentation result. it's in
 * effect a binary 2D array. a transparent value occurs where the pixel is not
 * part of the segmentation; a semi-opaque value occurs where the pixel _is_
 * part of the segmentation.
 */
void Segment2dIntDLControls::loadResult ( ) {
    VERBOSE;
    if (mResult) { delete mResult;  mResult = nullptr; }
    if (mBitmap) { delete mBitmap;  mBitmap = nullptr; }
    if (mAlpha ) { delete mAlpha;   mAlpha  = nullptr; }
    cout << "loading " << mOutSlice << endl;
    FILE* fp = fopen( mOutSlice.c_str(), "rb" );
    if (fp == nullptr)    return;

    mHeight = mFr->mCanvas->mCavassData->m_ySize;
    mWidth  = mFr->mCanvas->mCavassData->m_xSize;
    mAlpha  = new unsigned char[ mWidth * mHeight ];  //_not_ automatically deleted; deleted in dtor
    char ln[ 1024 ];
    /** \todo add error checks/handling */
    fgets( ln, sizeof ln, fp );    //first line should be P5
    fgets( ln, sizeof ln, fp );    //second line should be cols rows
    fgets( ln, sizeof ln, fp );    //third line should be max value (typically 255 although only 0 or 1 appears)
    fread( mAlpha, mWidth*mHeight, 1, fp );    //rest of file is data as binary uchars
    fclose( fp );
    fp = nullptr;
    //convert the binary result of seg to rgb and alpha values
    mResult = new unsigned char[ mWidth * mHeight * 3 ];  //rgb data
    for (int i=0,j=0; i<mWidth*mHeight; i++) {
        if (mAlpha[i] == 0) {  //nothing?
            mAlpha[i] = wxALPHA_TRANSPARENT;
            mResult[j++] = 0;    //r
            mResult[j++] = 0;    //g
            mResult[j++] = 0;    //b
        } else {  //something
            mAlpha[i] = wxALPHA_OPAQUE / 2;
            mResult[j++] = 0;    //r
            mResult[j++] = 0;    //g
            mResult[j++] = 255;  //b
        }
    }

    //must be allocated via malloc because wxwidgets will free it when necessary (by calling free)
    auto tmpAlpha  = (unsigned char*)malloc( mWidth * mHeight );  //binary data (segmentation)
    auto tmpResult = (unsigned char*)malloc( mWidth * mHeight * 3 );  //rgb data (blue for segmented data)
    //make a copy (for auto free via wxwidgets)
    memcpy( tmpAlpha,  mAlpha,  mWidth*mHeight   );
    memcpy( tmpResult, mResult, mWidth*mHeight*3 );
    //create image from above
    auto image = new wxImage( mWidth, mHeight, tmpResult, tmpAlpha );
    auto c = dynamic_cast<Segment2dCanvas*>( mFr->mCanvas );
    assert( c != nullptr );
    mScale = c->mScale;  //get the current scale factor
    if (mScale != 1.0) {
        image->Rescale( (int)(ceil(mScale*mWidth)), (int)(ceil(mScale*mHeight)) );
    }
    //create bitmap from image
    mBitmap = new wxBitmap( *image );
    //image is no longer needed
    delete image;
    image = nullptr;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * 
 * @param dc
 * @param x (x,y) is the offset in the window to the left-top coord of the image
 * @param y 
 */
void Segment2dIntDLControls::doPaint ( wxDC* dc, int x, int y ) {
    VERBOSE;
    auto c = dynamic_cast<Segment2dCanvas*>( mFr->mCanvas );
    if (c == nullptr)    return;
    //has the slice changed (so the rect, etc. is no longer relevant)?
    if (m_sliceNo == -1) {  //not set yet?
        //set to current slice
        m_sliceNo = c->mCavassData->m_sliceNo;
        mShow = true;
    }
    if (m_sliceNo != c->mCavassData->m_sliceNo) {
        //reset because we moved to a different slice
        reset();
        m_sliceNo = c->mCavassData->m_sliceNo;
    }

    if (!mShow)                      return;
    if (mdx1 == -1 || mdy1 == -1)    return;    //nothing to draw
    if (mdx2 == -1 || mdy2 == -1)    return;    //2nd corner not specified yet

    //get ready to draw rect
    dc->SetPen( wxPen(*wxYELLOW, 2) );
    dc->SetBrush( *wxTRANSPARENT_BRUSH );
    //determine top-left corner of rect
    int top  = (mdy1 <= mdy2) ? mdy1 : mdy2;
    int left = (mdx1 <= mdx2) ? mdx1 : mdx2;
    //determine width and height of rect
    int w = mdx2 - mdx1;
    int h = mdy2 - mdy1;
    if (w < 0)    w = -w;
    if (h < 0)    h = -h;
    //scale rect according to current scale setting
    left = (int)(left * c->mScale + x);    //don't round,  but offset
    top  = (int)(top  * c->mScale + y);    //don't round,  but offset
    w    = (int)(w    * c->mScale + 0.5);  //don't offset, but round
    h    = (int)(h    * c->mScale + 0.5);  //don't offset, but round
    dc->DrawRectangle( left, top, w, h );

    //draw segmentation (if any)
    if (mResult == nullptr)    return;  //none
    int rows = mFr->mCanvas->mCavassData->m_ySize;
    int cols = mFr->mCanvas->mCavassData->m_xSize;
    //before drawing the result, check for a change in the scale factor,
    // and rescale (only) if necessary
    if (mScale != c->mScale) {  //changed?
        mScale = c->mScale;  //remember for next time
        //recreate image
        // (must be allocated via malloc because wxwidgets will free it
        // "automagically" when necessary (by calling free).)
        // *** ignore "allocated memory is leaked" message ***
        auto tmpAlpha  = (unsigned char*)malloc( mWidth * mHeight     );  //binary data (segmentation)
        auto tmpResult = (unsigned char*)malloc( mWidth * mHeight * 3 );  //rgb data (blue for segmented data)
        //make a copy (for free via wxwidgets)
        memcpy( tmpAlpha,  mAlpha,  mWidth*mHeight   );
        memcpy( tmpResult, mResult, mWidth*mHeight*3 );
        //create image from above
        auto image = new wxImage( mWidth, mHeight, tmpResult, tmpAlpha );
        if (mScale != 1.0) {
            image->Rescale( (int)(ceil(mScale*mWidth)), (int)(ceil(mScale*mHeight)) );
        }
        //create bitmap from image
        mBitmap = new wxBitmap( *image );
        //image is no longer needed
        delete image;
        image = nullptr;
    }

    dc->DrawBitmap( *mBitmap, x, y );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dIntDLControls::doMiddleDown ( ) {
    mShow = false;
    mFr->mCanvas->Refresh();  //cause repaint
}

void Segment2dIntDLControls::doMiddleUp ( ) {
    mShow = true;
    mFr->mCanvas->Refresh();  //cause repaint
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dIntDLControls::setButtonState ( ) {
    if (!mFirstCornerSpecified || !mSecondCornerSpecified) {
        //Add, Blink, Clear and Run should all be disabled
        mAdd->Enable(   false );
        mBlink->Enable( false );
        mClear->Enable( false );
        mRun->Enable(   false );
        return;
    }
    //here when both corners available
    //Blink and Clear should be enabled
    mBlink->Enable( true );
    mClear->Enable( true );

    mAdd->Enable( mResult != nullptr );  //only when result is available
    mRun->Enable( mModelLoaded );  //only when model (and corners) available
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
