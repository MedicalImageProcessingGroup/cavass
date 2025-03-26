#pragma once
//======================================================================
class Segment2dSlider {
    friend class PersistentSegment2dFrame;
    friend class Segment2dFrame;

	wxFlexGridSizer  *mFgs;
	wxFlexGridSizer  *mFgsSlider;
	wxStaticText     *mSt;
	wxStaticText     *mSt1;  ///< min value
	wxStaticText     *mSt2;  ///< current value
	wxStaticText     *mSt3;  ///< max value
	wxSlider         *mSlider;

    /** monospaced font (so everything lines up) and a little bit smaller than usual */
    static void initText ( wxStaticText* st ) {
        ::setColor( st );
        //use a monospaced font so things line up nicely
        wxFont font = st->GetFont();
        font.SetFamily( wxFONTFAMILY_TELETYPE );
        font.SetPointSize( ::gDefaultFont.GetPointSize()-2 );
        st->SetFont( font );
    }

public:
    Segment2dSlider ( wxPanel* cp, wxFlexGridSizer *fgs, const wxString& title,
                      int actionId, double min_val, double max_val, double current_val )
    {
        mFgs = fgs;
        mSt = new wxStaticText( cp, wxID_ANY, title+":" );
        ::setColor( mSt );
        mFgs->Add( mSt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );

        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );

        mFgsSlider->AddStretchSpacer();

        mSlider = new wxSlider( cp, actionId, (int)(current_val*100+0.5),
                                (int)(min_val*100+0.5), (int)(max_val*100+0.5),
                                wxDefaultPosition, wxSize(sliderWidth, -1),
                                wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, title );
        ::setColor( mSlider );
        mSlider->SetPageSize( 5 );
        mFgsSlider->Add( mSlider, 0, wxGROW|wxLEFT|wxRIGHT );

        mFgsSlider->AddStretchSpacer();
        //min
        wxString  s = wxString::Format( "%8.2f", min_val );
        mSt1 = new wxStaticText( cp, wxID_ANY, s );
        ::setColor( mSt1 );
        mFgsSlider->Add( mSt1, 0, wxALIGN_RIGHT );
        //current
        s = wxString::Format( "%8.2f", current_val );
        mSt2 = new wxStaticText( cp, wxID_ANY, s );
        ::setColor( mSt2 );
        mFgsSlider->Add( mSt2, 0, wxALIGN_CENTER );
        //max
        s = wxString::Format( "%8.2f", max_val );
        mSt3 = new wxStaticText( cp, wxID_ANY, s );
        ::setColor( mSt3 );
        mFgsSlider->Add( mSt3, 0, wxALIGN_LEFT );

        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Segment2dSlider ( wxStaticBox* sb, wxFlexGridSizer* fgs, const wxString& title,
		int actionId, double min_val, double max_val, double current_val )
	{
		mFgs = fgs;
		mSt = new wxStaticText( sb, wxID_ANY, title+":" );
		::setColor( mSt );
		mFgs->Add( mSt, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 0 );

		mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
		mFgsSlider->AddGrowableCol( 0 );

		mFgsSlider->AddStretchSpacer();

		mSlider = new wxSlider( sb, actionId, (int)(current_val*100+0.5),
			(int)(min_val*100+0.5), (int)(max_val*100+0.5),
			wxDefaultPosition, wxSize(2*sliderWidth,-1),
			wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, title );
		::setColor( mSlider );
		mSlider->SetPageSize( 5 );
		mFgsSlider->Add( mSlider, 0, wxGROW|wxLEFT|wxRIGHT );

		mFgsSlider->AddStretchSpacer();
        //min
		wxString  s = wxString::Format( "%8.2f", min_val );
		mSt1 = new wxStaticText( sb, wxID_ANY, s );
        initText( mSt1 );
        mFgsSlider->Add( mSt1, 0, wxALIGN_LEFT);
        //current
		s = wxString::Format( "%8.2f", current_val );
		mSt2 = new wxStaticText( sb, wxID_ANY, s );
        initText( mSt2 );
        mFgsSlider->Add( mSt2, 0, wxALIGN_CENTER );
        //max
		s = wxString::Format( "%8.2f", max_val );
		mSt3 = new wxStaticText( sb, wxID_ANY, s );
        initText( mSt3 );
        mFgsSlider->Add( mSt3, 0, wxALIGN_RIGHT );

		mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
	}

	~Segment2dSlider ( ) {
		mFgsSlider->Detach(mSt3);
		delete mSt3;
		mFgsSlider->Detach(mSt2);
		delete mSt2;
		mFgsSlider->Detach(mSt1);
		delete mSt1;
		mFgsSlider->Detach(mSlider);
		delete mSlider;
		mFgs->Detach(mSt);
		delete mSt;
		mFgs->Remove( mFgsSlider );
	}

	void UpdateValue ( double new_val ) {
		mSt2->SetLabel( wxString::Format( "%8.2f", new_val ) );
	}
};
//======================================================================

