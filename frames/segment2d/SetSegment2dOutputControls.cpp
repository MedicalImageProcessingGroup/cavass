#include "cavass.h"
#include <wx/wx.h>
#include "Segment2dFrame.h"
#include "SetSegment2dOutputControls.h"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SetSegment2dOutputControls::SetOutType ( int newOutType ) {
    switch (newOutType) {
        case BINARY:
            m_outputType->SetLabel("Type: BIM");
            break;
        case GREY:
            m_outputType->SetLabel("Type: IM0");
            break;
        case GREY+1:
            m_outputType->SetLabel("Type: PAR");
            break;
        default:
            cerr << "SetSegment2dOutputControls::SetOutType( newOutType ): unrecognized newOutType." << endl;
            break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** ctor for Set Output controls */
SetSegment2dOutputControls::SetSegment2dOutputControls( wxPanel* cp,
    wxSizer* bottomSizer, const char* const title, int saveID,
    int outputObjectID, int currentOutObject, int currentOutType,
    int outputTypeID )
{
#if 1
    mBottomSizer = bottomSizer;
    //box for paint or train controls
    mSetOutputBox = new wxStaticBox( cp, wxID_ANY, title );
    ::setBoxColor( mSetOutputBox );
    mSetOutputSizer = new wxStaticBoxSizer( mSetOutputBox, wxHORIZONTAL );
    mSetOutputSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on left

    mFgs = new wxFlexGridSizer( 0, 2, 0, 0 );  //rows, cols, vgap, hgap
    mFgs->SetFlexibleDirection( wxBOTH );
    mFgs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    //row 0, col 0
    st = new wxStaticText( mSetOutputBox, wxID_ANY, "Out Object:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    ::setBoxColor( st );
    mFgs->Add( st, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    //row 0, col 1
    wxString objNam = "All";
    wxArrayString as(1, &objNam);
    FILE *objnamfp=fopen("object_names.spec", "rb");
    for (int j=1; j<=8; j++)
    {
        char buf[100];
        objNam = "";
        if (objnamfp && !feof(objnamfp) && fgets(buf, sizeof(buf), objnamfp))
            objNam = wxString(buf);
        objNam.Replace("\n", "");
        if (objNam == "")
            objNam = wxString::Format("%d", j);
        as.Add(objNam);
    }
    if (objnamfp)
        fclose(objnamfp);
    m_outputObject = new wxComboBox(mSetOutputBox, Segment2dFrame::ID_OUT_OBJECT,
                                    as[currentOutObject],
                                    wxDefaultPosition, wxDefaultSize,
                                    as, wxCB_READONLY );
    ::setColor( m_outputObject );
    mFgs->Add( m_outputObject, 0, wxALL, 5 );

    //row 1, col 0
    m_outputType = new wxButton( mSetOutputBox, outputTypeID,
                                 currentOutType==BINARY? "Type: BIM": "Type: IM0", wxDefaultPosition );
    ::setColor( m_outputType );
    mFgs->Add( m_outputType, 0, wxALL|wxEXPAND, 5 );

    //row 1, col 1
    m_saveBut = new wxButton( mSetOutputBox, Segment2dFrame::ID_SAVE, "Save" );
    ::setColor( m_saveBut );
    mFgs->Add( m_saveBut, 0, wxALL|wxEXPAND, 5 );

    mSetOutputSizer->Add( mFgs, 0, 0, 0 );
    mSetOutputSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on right
    mBottomSizer->Prepend( mSetOutputSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
#else  //original
    mBottomSizer = bottomSizer;
    mSetOutputBox = new wxStaticBox( cp, wxID_ANY, title );
    ::setBoxColor( mSetOutputBox );
    mSetOutputSizer = new wxStaticBoxSizer( mSetOutputBox, wxHORIZONTAL );

    mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
    mFgs->SetMinSize( controlsWidth, 0 );
    mFgs->AddGrowableCol( 0 );
    mFgsButton = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

    st = new wxStaticText( cp, wxID_ANY, "Out Object:" );
    ::setBoxColor( st );
    mFgsButton->Add( st, 0,
                     wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    wxString objNam = "All";
    wxArrayString as(1, &objNam);
    FILE *objnamfp=fopen("object_names.spec", "rb");
    for (int j=1; j<=8; j++)
    {
        char buf[100];
        objNam = "";
        if (objnamfp && !feof(objnamfp) && fgets(buf, sizeof(buf), objnamfp))
            objNam = wxString(buf);
        objNam.Replace("\n", "");
        if (objNam == "")
            objNam = wxString::Format("%d", j);
        as.Add(objNam);
    }
    if (objnamfp)
        fclose(objnamfp);
    m_outputObject = new wxComboBox(cp, Segment2dFrame::ID_OUT_OBJECT,
                                    as[currentOutObject],
                                    wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
                                    as, wxCB_READONLY );
    ::setColor( m_outputObject );
    mFgsButton->Add( m_outputObject, 0,
                     wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_outputType = new wxButton( cp, outputTypeID,
                                 currentOutType==BINARY? "Type: BIM": "Type: IM0", wxDefaultPosition,
                                 wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_outputType );
    mFgsButton->Add( m_outputType, 0,
                     wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_saveBut = new wxButton( cp, Segment2dFrame::ID_SAVE, "Save",
                              wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_saveBut );
    mFgsButton->Add( m_saveBut, 0,
                     wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
    mSetOutputSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Prepend( mSetOutputSizer, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Layout();
    cp->Refresh();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Modified: 7/18/08 mSetOutputSizer detached instead of removed
//    by Dewey Odhner.
SetSegment2dOutputControls::~SetSegment2dOutputControls ( ) {
#if 0
    mFgsButton->Detach(m_saveBut);
    delete m_saveBut;
    mFgsButton->Detach(m_outputType);
    delete m_outputType;
    mFgsButton->Detach(m_outputObject);
    delete m_outputObject;
    mFgsButton->Detach(st);
    delete st;

    mFgs->Detach(mFgsButton);
    delete mFgsButton;
    mSetOutputSizer->Detach(mFgs);
    delete mFgs;
#endif
    mBottomSizer->Detach(mSetOutputSizer);
    delete mSetOutputBox;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
