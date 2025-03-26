#pragma once

#include "Segment2dCanvas.h"
#include "Segment2dFrame.h"
#include "PersistentSegment2dFrame.h"

class  SetSegment2dOutputControls {
    friend PersistentSegment2dFrame;

    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
	wxFlexGridSizer*  mFgsButton;
    wxStaticBox*      mSetOutputBox;
	wxButton*         m_saveBut;
	wxComboBox *      m_outputObject;
    wxStaticBoxSizer* mSetOutputSizer;
	wxStaticText     *st;
	wxButton         *m_outputType;

public:
	SetSegment2dOutputControls ( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int saveID, int outputObjectID,
		int currentOutObject, int currentOutType=BINARY,
		int outputTypeID=Segment2dFrame::ID_OUT_TYPE );

	void SetOutType( int newOutType );

	~SetSegment2dOutputControls();
};

