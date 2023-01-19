/*
  Copyright 1993-2013 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/

//======================================================================
// GrayScaleTransform
//----------------------------------------------------------------------
#include  "cavass.h"

#if 0
#ifdef WIN32
    #include  <iostream.h>
    #include  <iomanip.h>
#else
    #include  <iostream>
    #include  <iomanip>
#endif
#include  <assert.h>
#include  <math.h>
#include  <Viewnix.h>
#include  "cv3dv.h"
#include  "Dicom.h"

#include  "wx/wx.h"
#include  "wx/progdlg.h"
#include  "wx/file.h"
#include  "MainCanvas.h"
#include  "MontageFrame.h"
#include  "Globals.h"
#endif

#include  "GrayScaleTransform.h"
using namespace std;
//----------------------------------------------------------------------
const int GrayScaleTransform::sWidth  = 100;
const int GrayScaleTransform::sHeight = 100;
//----------------------------------------------------------------------
void GrayScaleTransform::OnDoSave ( wxCommandEvent& e ) {
    assert( m_parent->mCanvas->mCavassData->m_vh_initialized );

    if (false) {
        //gray min and max
        const int  min = m_parent->mCanvas->mCavassData->m_min;
        const int  max = m_parent->mCanvas->mCavassData->m_max;
        const int  numberOfPixels = m_parent->mCanvas->mCavassData->m_xSize
            * m_parent->mCanvas->mCavassData->m_ySize * m_parent->mCanvas->mCavassData->m_zSize;

        if        (m_parent->mCanvas->mCavassData->m_size == 1) {
            unsigned char* p = (unsigned char*)m_parent->mCanvas->mCavassData->m_data;
            histogramWithTransform( p, numberOfPixels, gHistogramBucketFactor,
                min, max, mTransform, "histogram after transform" );
        } else if (m_parent->mCanvas->mCavassData->m_size == 2) {
            unsigned short* p = (unsigned short*)m_parent->mCanvas->mCavassData->m_data;
            histogramWithTransform( p, numberOfPixels, gHistogramBucketFactor,
                min, max, mTransform, "histogram after transform" );

        } else if (m_parent->mCanvas->mCavassData->m_size == 4) {
            int* p = (int*)m_parent->mCanvas->mCavassData->m_data;
            histogramWithTransform( p, numberOfPixels, gHistogramBucketFactor,
                min, max, mTransform, "histogram after transform" );
        }
    }

    wxFileDialog*  d = new wxFileDialog( this, "Save transformed image data",
        "", "", "IM0 files(*.IM0)|*.IM0", wxSAVE );

    char  fn[255];
    for ( ; ; ) {
        if (d->ShowModal() != wxID_OK) {
            d->Destroy();
            return;
        }
        //check 'are you sure?' if file already exists
        const char* const  fileName = d->GetPath().c_str();
        if (wxFile::Exists(fileName)) {
            wxMessageDialog*  m = new wxMessageDialog( this,
                "File exists! \n Do you wish to overwrite it?", "Uh oh!",
                wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION );
            if (m->ShowModal() == wxID_YES) {
                cout << "you said yes" << endl;
                m->Destroy();    m=NULL;
                strcpy( fn, fileName );
                break;
            } else {
                cout << "you said no" << endl;
                m->Destroy();    m=NULL;
            }
        } else {    //save file doesn't already exist
            strcpy( fn, fileName );
            break;
        }
    }

    d->Destroy();    d=NULL;
    cout << "saving to " << fn << endl;
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    FILE*  fp = fopen(fn, "wb+");
    if (fp==NULL) {
        cerr << "Can't open " << fn << "." << endl;
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    char  group[5], element[5];
    int  err = VWriteHeader(fp, &m_parent->mCanvas->mCavassData->m_vh, group, element);
    if (err && err<106) {
        cerr << "Can't write " << fn << "'s header." << endl;
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }

    unsigned char*  data = (unsigned char*)malloc(
        m_parent->mCanvas->mCavassData->m_bytesPerSlice * m_parent->mCanvas->mCavassData->m_zSize );
    if (data==NULL) {
        cerr << "Out of memory while writing " << fn << endl;
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }

    //gray min and max
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    //const int  max = m_parent->mCanvas->mCavassData->m_max;
    const int  pixels = m_parent->mCanvas->mCavassData->m_xSize
        * m_parent->mCanvas->mCavassData->m_ySize * m_parent->mCanvas->mCavassData->m_zSize;
    if (m_parent->mCanvas->mCavassData->m_size==1) {
        unsigned char*  inData  = (unsigned char*)m_parent->mCanvas->mCavassData->m_data;
        unsigned char*  outData = (unsigned char*)data;
        for (int i=0; i<pixels; i++) {
            outData[i] = mTransform[ inData[i]-min ];
        }
    } else if (m_parent->mCanvas->mCavassData->m_size==2) {
        unsigned short*  inData  = (unsigned short*)m_parent->mCanvas->mCavassData->m_data;
        unsigned short*  outData = (unsigned short*)data;
        for (int i=0; i<pixels; i++) {
            outData[i] = mTransform[ inData[i]-min ];

        }
    } else if (m_parent->mCanvas->mCavassData->m_size==4) {
        unsigned int*  inData  = (unsigned int*)m_parent->mCanvas->mCavassData->m_data;
        unsigned int*  outData = (unsigned int*)data;
        for (int i=0; i<pixels; i++) {
            outData[i] = mTransform[ inData[i]-min ];
        }
    }

    //write all of the slices
    //err = VSeekData(fp, m_parent->mCanvas->m_sliceNo * m_parent->mCanvas->mCavassData->m_bytesPerSlice);
    err = VSeekData(fp, 0);
    int  num;
    err = VWriteData( (char*)data, m_parent->mCanvas->mCavassData->m_size, m_parent->mCanvas->mCavassData->m_zSize*m_parent->mCanvas->mCavassData->m_bytesPerSlice/m_parent->mCanvas->mCavassData->m_size,
        fp, &num );
    VCloseData(fp);  fp=NULL;
    free(data);      data=NULL;

    //back to normal
    m_parent->mCanvas->mCavassData->initLUT();
    m_parent->mCanvas->reload();
    Close();
    Destroy();
}
//----------------------------------------------------------------------
void GrayScaleTransform::OnDoInvert ( wxCommandEvent& e ) {
    mInvert = !mInvert;
    doTransform();
}
//----------------------------------------------------------------------
void GrayScaleTransform::OnDoFlash ( wxCommandEvent& e ) {
    static bool  flipFlop = false;

    flipFlop = !flipFlop;
    if (flipFlop) {
        //back to normal
        m_parent->mCanvas->mCavassData->initLUT();
        m_parent->mCanvas->reload();
        m_flash->SetLabel("Transformed");
        return;
    }

    //gray min and max
    const int  min = m_parent->mCanvas->mCavassData->m_min;
    const int  max = m_parent->mCanvas->mCavassData->m_max;
    //back to abnormal
    unsigned char*  newLUT = (unsigned char*)malloc(
        (max-min+1)*sizeof(unsigned char) );
    assert( newLUT!= NULL);
    for (int i=min; i<=max; i++) {
        const int  newValue = mTransform[i-min];
        assert( newValue>=min && newValue<=max );
        newLUT[i-min] = mLUTCopy[newValue-min];
    }
    m_parent->mCanvas->mCavassData->initLUT( newLUT, max-min+1 );
    m_parent->mCanvas->reload();
    free( newLUT );       newLUT    = NULL;
    m_flash->SetLabel("Original");
}
//----------------------------------------------------------------------
void GrayScaleTransform::OnDoDismiss ( wxCommandEvent& e ) {
    //back to normal
    m_parent->mCanvas->mCavassData->initLUT();
    m_parent->mCanvas->reload();
    Close();
    Destroy();
}
//----------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS ( GrayScaleTransform, wxDialog )
BEGIN_EVENT_TABLE       ( GrayScaleTransform, wxDialog )
    EVT_BUTTON( wxID_OK,     GrayScaleTransform::OnDoSave     )
    EVT_BUTTON( ID_FLASH,    GrayScaleTransform::OnDoFlash    )
    EVT_BUTTON( ID_INVERT,   GrayScaleTransform::OnDoInvert   )
    EVT_BUTTON( wxID_CANCEL, GrayScaleTransform::OnDoDismiss  )
END_EVENT_TABLE()
//======================================================================
