/*
  Copyright 1993-2013, 2022 Medical Image Processing Group
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
/**
 * \file   InformationDialog.cpp
 * \brief  InformationDialog class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

InformationDialog::InformationDialog ( MainFrame* parent )
    : wxDialog ( parent, -1, "Information", wxDefaultPosition, wxDefaultSize ),
      mParent(parent)
{
    Prepare( wxArrayString() );
}

InformationDialog::InformationDialog ( wxArrayString& selected,
    MainFrame* parent )
    : wxDialog ( parent, -1, "Information", wxDefaultPosition, wxDefaultSize ),
      mParent(parent)
{
    Prepare( selected );
}

void InformationDialog::Prepare ( wxArrayString selected )
{
    wxPanel*  mainPanel = new wxPanel( this, -1 );
    ::setColor( this );
    ::setColor( mainPanel );

    mSizer   = new wxGridSizer( 2, 5, 5 );
    mListBox = new wxListBox( mainPanel, ID_INFO_LIST, wxDefaultPosition,
                              wxSize(400,400), 0, NULL, wxLB_HSCROLL );
    ::setColor( mListBox );
    mSizer->Add( mListBox, 0, wxGROW|wxALL, 5 );

    mNotebook = new wxNotebook( mainPanel, -1, wxDefaultPosition,
                                wxSize(400,400) );
    ::setColor( mNotebook );

    mGeneral = new wxTextCtrl( mNotebook, wxID_ANY, "", wxDefaultPosition,
                               wxSize(400,400), wxTE_MULTILINE );
    ::setColor( mGeneral );
    mNotebook->AddPage( mGeneral, "general" );

    mScene = new wxTextCtrl( mNotebook, wxID_ANY, "", wxDefaultPosition,
                             wxSize(400,400), wxTE_MULTILINE );
    ::setColor( mScene );
    mNotebook->AddPage( mScene, "scene" );

    mStructure = new wxTextCtrl( mNotebook, wxID_ANY, "", wxDefaultPosition,
                                 wxSize(400,400), wxTE_MULTILINE );
    ::setColor( mStructure );
    mNotebook->AddPage( mStructure, "structure" );

    mDisplay = new wxTextCtrl( mNotebook, wxID_ANY, "", wxDefaultPosition,
                               wxSize(400,400), wxTE_MULTILINE );
    ::setColor( mDisplay );
    mNotebook->AddPage( mDisplay, "display" );

    mSizer->Add( mNotebook, 0, wxGROW|wxALL, 10 );

    if (selected.GetCount() != 0) {
        for (unsigned int j=0; j<selected.Count(); j++)
            mListBox->Append( selected[j] );
        //select the first entry
        mListBox->Select( 0 );
        wxCommandEvent  unused;
        OnSelect( unused );
    } else if (mParent && mParent->mCanvas->mCavassData!=NULL) {
        CavassData*  temp = mParent->mCanvas->mCavassData;
        while (temp!=NULL) {
            if (temp->m_fname!=NULL) mListBox->Append( temp->m_fname );
            else                     mListBox->Append( "<no name available>" );
            temp = temp->mNext;
        }
        //select the first entry
        mListBox->Select( 0 );
        wxCommandEvent  unused;
        OnSelect( unused );
    } else {
        mListBox->Append( "<no files>" );
    }

    //mSizer->Layout();
    mainPanel->SetAutoLayout( true );
    mainPanel->SetSizer( mSizer );
    mSizer->SetSizeHints( this );
}
//----------------------------------------------------------------------
void free_scene_header(ViewnixHeader *vh);

void free_viewnix_header(ViewnixHeader *vh)
{
    if (vh->str.domain)
        free(vh->str.domain);
    if (vh->str.axis_label)
        free(vh->str.axis_label);
    if (vh->str.measurement_unit)
        free(vh->str.measurement_unit);
    if (vh->str.scene_file)
        free(vh->str.scene_file);
    if (vh->str.num_of_TSE)
        free(vh->str.num_of_TSE);
    if (vh->str.num_of_NTSE)
        free(vh->str.num_of_NTSE);
    if (vh->str.TSE_measurement_unit)
        free(vh->str.TSE_measurement_unit);
    if (vh->str.NTSE_measurement_unit)
        free(vh->str.NTSE_measurement_unit);
    if (vh->str.smallest_value)
        free(vh->str.smallest_value);
    if (vh->str.largest_value)
        free(vh->str.largest_value);
    if (vh->str.signed_bits_in_TSE)
        free(vh->str.signed_bits_in_TSE);
    if (vh->str.bit_fields_in_TSE)
        free(vh->str.bit_fields_in_TSE);
    if (vh->str.signed_bits_in_NTSE)
        free(vh->str.signed_bits_in_NTSE);
    if (vh->str.bit_fields_in_NTSE)
        free(vh->str.bit_fields_in_NTSE);
    if (vh->str.num_of_samples)
        free(vh->str.num_of_samples);
    if (vh->str.loc_of_samples)
        free(vh->str.loc_of_samples);
    if (vh->str.description_of_element)
        free(vh->str.description_of_element);
    if (vh->str.parameter_vectors)
        free(vh->str.parameter_vectors);
    if (vh->str.min_max_coordinates)
        free(vh->str.min_max_coordinates);
    if (vh->str.volume)
        free(vh->str.volume);
    if (vh->str.surface_area)
        free(vh->str.surface_area);
    if (vh->str.description)
        free(vh->str.description);
    if (vh->dsp.smallest_value)
        free(vh->dsp.smallest_value);
    if (vh->dsp.largest_value)
        free(vh->dsp.largest_value);
    if (vh->dsp.signed_bits)
        free(vh->dsp.signed_bits);
    if (vh->dsp.bit_fields)
        free(vh->dsp.bit_fields);
    if (vh->dsp.specification_pv)
        free(vh->dsp.specification_pv);
    if (vh->dsp.pv)
        free(vh->dsp.pv);
    if (vh->dsp.description)
        free(vh->dsp.description);
    free_scene_header(vh);
}
//----------------------------------------------------------------------
void InformationDialog::OnSelect ( wxCommandEvent& unused ) {
    if (mListBox == NULL)
        return;

    mDisplay->Clear();
    mGeneral->Clear();
    mScene->Clear();
    mStructure->Clear();

    //find the selected item in the list
    const int  which = mListBox->GetSelection();
    wxString filename=mListBox->GetString(which);
    CavassData*  temp = mParent==NULL? NULL: mParent->mCanvas->mCavassData;
    for (; temp!=NULL && filename!=temp->m_fname; temp=temp->mNext) {
        ;
    }
    ViewnixHeader *vh;
    if (temp)
        vh = &temp->m_vh;
    else
    {
        vh = (ViewnixHeader *)malloc(sizeof(*vh));
        memset(vh, 0, sizeof(*vh));
        FILE *fp=fopen(filename.c_str(), "rb");
        if (fp == NULL)
        {
            free(vh);
            return;
        }
        char grp[6], elem[6];
        int err=VReadHeader(fp, vh, grp,elem);
        fclose(fp);
        switch (err)
        {
            case 0:
            case 106:
            case 107:
                break;
            default:
                wxMessageBox(wxString::Format(
                    "VReadHeader returned error code %d\nGroup ", err)+grp+
                    " Element "+elem);
                free_viewnix_header(vh);
                return;
        }
    }

    char  buff[ 1000 ];

    mGeneral->AppendText( "General Information \n\n" );
    if (vh->gen.series_valid)
        sprintf( buff, "Data Set ID : %s\n", vh->gen.series );
    else
        sprintf( buff, "Data Set ID : Not Found\n" );
    mGeneral->AppendText( buff );
    sprintf( buff, "Description : " );
    if (vh->gen.description_valid)
    {
        strncpy(buff+strlen(buff), vh->gen.description, 983);
        buff[997] = 0;
        strcpy(buff+strlen(buff), "\n" );
    }
    else
        strcpy(buff+strlen(buff), "Not Found\n" );
    mGeneral->AppendText( buff );
    if (vh->gen.institution_valid)
        sprintf( buff, "Institution : %s\n", vh->gen.institution );
    else
        sprintf( buff, "Institution : Not Found\n" );
    mGeneral->AppendText( buff );
    if (vh->gen.data_type_valid)
        switch (vh->gen.data_type)
        {
            case 0:
                sprintf( buff, "Data Type   : IMAGE0\n" );
                break;
            case 1:
                sprintf( buff, "Data Type   : IMAGE1\n" );
                break;
            case 100:
                sprintf( buff, "Data Type   : CURVE0\n" );
                break;
            case 110:
                sprintf( buff, "Data Type   : SURFACE0\n" );
                break;
            case 111:
                sprintf( buff, "Data Type   : SURFACE1\n" );
                break;
            case 120:
                sprintf( buff, "Data Type   : SHELL0\n" );
                break;
            case 121:
                sprintf( buff, "Data Type   : SHELL1\n" );
                break;
            case 122:
                sprintf( buff, "Data Type   : SHELL2\n" );
                break;
            case 200:
                sprintf( buff, "Data Type   : MOVIE0\n" );
                break;
        }
    else
        sprintf( buff, "Data Type   : Not Found\n" );
    mGeneral->AppendText( buff );
    if (vh->gen.study_date_valid)
        sprintf( buff, "Study Date  : %s", vh->gen.study_date );
    else
        sprintf( buff, "Study Date  :  Not Found\n" );
    mGeneral->AppendText( buff );

    mScene->AppendText(   "Scene Information \n\n"   );
    if (vh->gen.data_type < CURVE0)
    {
        if (vh->scn.dimension == 3)
        {
            if (vh->scn.num_of_subscenes[0] == 1)
                sprintf( buff, "Cell Size: %.2fx%.2fx \n             : Only One Slice \n",
                    vh->scn.xypixsz[0], vh->scn.xypixsz[1] );
            else
            {
                bool nonuniform_slice_spacing=false;
                float first_slice_spacing=vh->scn.loc_of_subscenes[1]-
                                          vh->scn.loc_of_subscenes[0];
                for (int j=2; j<vh->scn.num_of_subscenes[0]; j++)
                {
                    float slice_spacing=vh->scn.loc_of_subscenes[j]-
                                        vh->scn.loc_of_subscenes[j-1];
                    if (slice_spacing<=.99*first_slice_spacing ||
                            .99*slice_spacing>=first_slice_spacing)
                    {
                        nonuniform_slice_spacing = true;
                        break;
                    }
                }
                if (nonuniform_slice_spacing)
                    sprintf( buff, "Cell Size: %.2fx%.2fx \nSlice Spg   : Not Uniform \n",
                        vh->scn.xypixsz[0], vh->scn.xypixsz[1] );
                else
                sprintf( buff, "Cell Size: %.2fx%.2fx%.2f \n",
                    vh->scn.xypixsz[0], vh->scn.xypixsz[1], first_slice_spacing );
            }
        }
        else if (vh->scn.dimension == 4)
        {
            //@@
        }
        mScene->AppendText( buff );
        if (vh->scn.measurement_unit_valid)
        {
            sprintf( buff, "Meas Unit   : " );
            for (int j=0; j<vh->scn.dimension; j++)
                switch (vh->scn.measurement_unit[j])
                {
                    case 0:
                        sprintf( buff+strlen(buff), "km X " );
                        break;
                    case 1:
                        sprintf( buff+strlen(buff), "m X " );
                        break;
                    case 2:
                        sprintf( buff+strlen(buff), "cm X " );
                        break;
                    case 3:
                        sprintf( buff+strlen(buff), "mm X " );
                        break;
                    case 4:
                        sprintf( buff+strlen(buff), "micron X " );
                        break;
                    case 5:
                        sprintf( buff+strlen(buff), "sec X " );
                        break;
                    case 6:
                        sprintf( buff+strlen(buff), "msec X " );
                        break;
                    case 7:
                        sprintf( buff+strlen(buff), "microsec X " );
                        break;
                }
            sprintf( buff+strlen(buff)-3, "\n" );
            mScene->AppendText( buff );
        }
        if (vh->scn.dimension == 3)
            sprintf( buff, "Scene Size: %dx%dx%d \n", vh->scn.xysize[0],
                vh->scn.xysize[1], vh->scn.num_of_subscenes[0] );
        else if (vh->scn.dimension == 4)
            sprintf( buff, "Scene Size: %dx%dx%dx%d \n", vh->scn.xysize[0],
                vh->scn.xysize[1], vh->scn.num_of_subscenes[1],
                vh->scn.num_of_subscenes[0] );
        mScene->AppendText( buff );

        if (vh->scn.smallest_density_value_valid)
            sprintf( buff, "Min Density: %.0f \n", vh->scn.smallest_density_value[0] );
        else
            sprintf( buff, "Min Density : Not Found" );
        mScene->AppendText( buff );

        if (vh->scn.largest_density_value_valid)
            sprintf( buff, "Max Density: %.0f \n", vh->scn.largest_density_value[0] );
        else
            sprintf( buff, "Max Density : Not Found" );
        mScene->AppendText( buff );

        sprintf( buff, "Bits/Cell: %d \n", vh->scn.num_of_bits );
        mScene->AppendText( buff );
    }
    if (vh->scn.domain_valid) {
        sprintf( buff, "Domain location: (%.2f,%.2f,%.2f) \n", vh->scn.domain[0], vh->scn.domain[1], vh->scn.domain[2] );
        mScene->AppendText( buff );
        sprintf( buff, "Domain orientation: (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f) \n",
            vh->scn.domain[3], vh->scn.domain[4], vh->scn.domain[5],
            vh->scn.domain[6], vh->scn.domain[7], vh->scn.domain[8],
            vh->scn.domain[9], vh->scn.domain[10], vh->scn.domain[11] );
        mScene->AppendText( buff );
    }

    mStructure->AppendText( "Structure Information \n\n" );
    if (vh->gen.data_type>=CURVE0 && vh->gen.data_type<MOVIE0)
    {
        sprintf( buff, "Cell Size   : %3.2f X %3.2f X\n",
            vh->str.xysize[0], vh->str.xysize[1] );
        mStructure->AppendText( buff );
        if (vh->str.dimension == 3)
        {
            if (vh->str.num_of_samples[0] == 1)
                sprintf( buff, "            : Only One Slice" );
            else
            {
                float slice_spacing=
                    vh->str.loc_of_samples[1] -
                    vh->str.loc_of_samples[0];
                for (int j=1; j<vh->str.num_of_samples[0]-1; j++)
                    if (slice_spacing<=.99*(
                            vh->str.loc_of_samples[j+1]-
                            vh->str.loc_of_samples[j]) ||
                            .99*slice_spacing>=(
                            vh->str.loc_of_samples[j+1]-
                            vh->str.loc_of_samples[j]))
                    {
                        slice_spacing = 0;
                        break;
                    }
                if (slice_spacing > 0)
                    sprintf( buff, "            : %3.2f\n", slice_spacing );
                else
                    sprintf( buff, "            : *\n" );
            mStructure->AppendText( buff );
            }
        }
        else
        {
            //@ to be implemented
        }
        if (vh->str.measurement_unit_valid)
        {
            sprintf( buff, "Meas Unit   : " );
            for (int j=0; j<vh->str.dimension; j++)
                switch (vh->str.measurement_unit[j])
                {
                    case 0:
                        sprintf( buff+strlen(buff), "km X " );
                        break;
                    case 1:
                        sprintf( buff+strlen(buff), "m X " );
                        break;
                    case 2:
                        sprintf( buff+strlen(buff), "cm X " );
                        break;
                    case 3:
                        sprintf( buff+strlen(buff), "mm X " );
                        break;
                    case 4:
                        sprintf( buff+strlen(buff), "micron X " );
                        break;
                    case 5:
                        sprintf( buff+strlen(buff), "sec X " );
                        break;
                    case 6:
                        sprintf( buff+strlen(buff), "msec X " );
                        break;
                    case 7:
                        sprintf( buff+strlen(buff), "microsec X " );
                        break;
                }
            sprintf( buff+strlen(buff)-3, "\n" );
            mStructure->AppendText( buff );
        }
        sprintf( buff, "Num Struc   : %d\n", vh->str.num_of_structures);
        mStructure->AppendText( buff );
        for (int j=0; j<vh->str.num_of_structures; j++)
        {
            sprintf( buff, "Struc Size  : %d\n", vh->str.num_of_TSE[j]);
            mStructure->AppendText( buff );
        }
        if (vh->str.volume_valid)
            for (int j=0; j<vh->str.num_of_structures; j++)
            {
                sprintf( buff, "Volume(%d)   : %3.2f\n", j+1,
                    vh->str.volume[j] );
                mStructure->AppendText( buff );
            }
        else
            mStructure->AppendText( (char *)"Volume      : Not Found\n" );
        if (vh->str.surface_area_valid)
            for (int j=0; j<vh->str.num_of_structures; j++)
            {
                sprintf( buff, "Surface Area(%d): %3.2f\n", j+1,
                    vh->str.surface_area[j] );
                mStructure->AppendText( buff );
            }
        else
            mStructure->AppendText( (char *)"Surface Area   : Not Found\n" );
    }

    mDisplay->AppendText( "Display Information \n\n" );
    if (vh->gen.data_type >= MOVIE0)
    {
        if (vh->dsp.xypixsz_valid)
        {
            sprintf( buff, "Cell Size   : %3.2f X %3.2f\n",
                vh->dsp.xypixsz[0], vh->dsp.xypixsz[1] );
            mDisplay->AppendText( buff );
        }
        else
            mDisplay->AppendText( (char *)"Cell Size   : Not Found\n" );
        if (vh->dsp.measurement_unit_valid)
        {
            sprintf( buff, "Meas Unit   : " );
            for (int j=0; j<vh->dsp.dimension; j++)
                switch (vh->dsp.measurement_unit[j])
                {
                    case 0:
                        sprintf( buff+strlen(buff), "km X " );
                        break;
                    case 1:
                        sprintf( buff+strlen(buff), "m X " );
                        break;
                    case 2:
                        sprintf( buff+strlen(buff), "cm X " );
                        break;
                    case 3:
                        sprintf( buff+strlen(buff), "mm X " );
                        break;
                    case 4:
                        sprintf( buff+strlen(buff), "micron X " );
                        break;
                    case 5:
                        sprintf( buff+strlen(buff), "sec X " );
                        break;
                    case 6:
                        sprintf( buff+strlen(buff), "msec X " );
                        break;
                    case 7:
                        sprintf( buff+strlen(buff), "microsec X " );
                        break;
                }
            sprintf( buff+strlen(buff)-3, "\n" );
            mDisplay->AppendText( buff );
        }
        sprintf( buff, "Image Size  : %d X %d\n", vh->dsp.xysize[0],
            vh->dsp.xysize[1] );
        mDisplay->AppendText( buff );
        sprintf( buff, "Num Images  : %d\n", vh->dsp.num_of_images );
        mDisplay->AppendText( buff );
        if (vh->dsp.smallest_value_valid)
            for (int j=0; j<vh->dsp.num_of_elems; j++)
            {
                sprintf( buff, "Min Density : %3.2f\n",
                    vh->dsp.smallest_value[j] );
                mDisplay->AppendText( buff );
            }
        else
            mDisplay->AppendText( (char *)"Min Density : Not Found\n" );
        if (vh->dsp.largest_value_valid)
            for (int j=0; j<vh->dsp.num_of_elems; j++)
            {
                sprintf( buff, "Max Density : %3.2ff\n",
                    vh->dsp.largest_value[j] );
                mDisplay->AppendText( buff );
            }
        else
            mDisplay->AppendText( (char *)"Max Density : Not Found\n" );
        sprintf( buff, "Bits/Cell   : %d\n", vh->dsp.num_of_bits );
        mDisplay->AppendText( buff );
    }
    if (temp == NULL)
        free_viewnix_header(vh);
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( InformationDialog, wxDialog )
BEGIN_EVENT_TABLE       ( InformationDialog, wxDialog )
    EVT_LISTBOX ( ID_INFO_LIST, InformationDialog::OnSelect )
END_EVENT_TABLE()
//----------------------------------------------------------------------

