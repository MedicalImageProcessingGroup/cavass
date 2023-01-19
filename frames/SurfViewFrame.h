/*
  Copyright 1993-2013, 2015-2019 Medical Image Processing Group
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
 * \file   SurfViewFrame.h
 * \brief  SurfViewFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SurfViewFrame_h
#define __SurfViewFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "SurfViewCanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  ColorControls;
class  SurfOpacityControls;
class  SurfPropertiesControls;
class  SurfSpeedControls;
class  manipObjectSwitch;
class  CutDepthControls;
class  SliceOutputControls;
class  TumbleControls;
class  VolumeRenderingControls;
class  GrayMapControls;


/** \brief SurfViewFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class SurfViewFrame : public MainFrame {
  friend class SurfViewCanvas;
  public:
    enum {
        ID_AMBIENT = ID_LAST, ///< continue from where MainFrame.h left off
                       ID_AMB_BLUE_SLIDER, ID_AMB_GREEN_SLIDER,
                       ID_AMB_RED_SLIDER,
        ID_ANTIALIAS,
        ID_BACKGROUND, ID_BG_BLUE_SLIDER, ID_BG_GREEN_SLIDER,
                       ID_BG_RED_SLIDER,
        ID_BOX,
        ID_COLOR,      ID_COLOR_BLUE_SLIDER, ID_COLOR_GREEN_SLIDER,
                       ID_COLOR_RED_SLIDER,
        ID_DIFFUSE,    ID_DIFFUSE_PERCENT_SLIDER, ID_DIFFUSE_EXPONENT_SLIDER,
                       ID_DIFFUSE_DIVISOR_SLIDER,
        ID_LAYOUT,
        ID_LIGHT,
        ID_MAGNIFY,    ID_MAGNIFY_SLIDER,
        ID_MATCH_ROI,
        ID_OBJECT,
        ID_OPACITY,    ID_OPACITY_SLIDER,
        ID_RESET,
        ID_RESET_OBJ,
        ID_SPECULAR,   ID_SPECULAR_PERCENT_SLIDER, ID_SPECULAR_EXPONENT_SLIDER,
                       ID_SPECULAR_DIVISOR_SLIDER,
        ID_SPEED,      ID_SPEED_SLIDER,
        ID_STATUS,
        ID_STRUCT_SYS,
        ID_MODE,       ID_SAVE,
		ID_ROTATE,
		ID_CUT_DEPTH_SLIDER,
		ID_REMOVE, ID_MOBILE, ID_SCENE_STATUS,
		ID_VOL_REND_PARAMS,
		ID_SURF_STRENGTH, ID_SURF_STREN_SLIDER,
		ID_EMISSION_POWER_SLIDER, ID_SURF_PCT_POWER_SLIDER,
		ID_MATERIAL,
		ID_PIXEL_SIZE_SLIDER, ID_SLICE_SPACING_SLIDER,
		ID_INTR_VIEWS_SLIDER, ID_PREVIEW, ID_POSES, ID_PREVIEW_MODE,
		ID_DELETE_POSE,
		ID_MATERL_OPACITY, ID_MATERL_OPACITY_SLIDER,
		ID_MATERL_COLOR,
		ID_MATERL_RED_SLIDER, ID_MATERL_GREEN_SLIDER, ID_MATERL_BLUE_SLIDER,
		ID_MATERL_THRESHOLD1_SLIDER, ID_MATERL_THRESHOLD2_SLIDER,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
		ID_CENTER1_SLIDER,  ID_WIDTH1_SLIDER,
        ID_FORWARD_SLICE, ID_BACKWARD_SLICE,
		ID_MIP, ID_MIP_INVERT
    };
    int                      mFileOrDataCount;    ///< two needed for overlay
  protected:
    bool                     mSurface;            ///< true = surface; false = volume
    bool                     manip_flag;
    wxMenu*                  m_options_menu;      ///< options menu

    ColorControls*           mAmbientControls;
    ColorControls*           mBackgroundControls;
    ColorControls*           mColorControls;
    SurfPropertiesControls*  mDiffuseControls;
    MagnifyControls*         mMagnifyControls;
    SurfOpacityControls*     mOpacityControls;
    SurfPropertiesControls*  mSpecularControls;
    SurfSpeedControls*       mSpeedControls;
	CutDepthControls        *mCutDepthControls;
	SliceOutputControls     *mSliceOutputControls;
	TumbleControls          *mTumbleControls;
	VolumeRenderingControls *mVRControls;
	GrayMapControls         *mGrayMapControls;

    double            mScale;
    double            mSpeed;

    wxCheckBox       *mStatus;
	wxCheckBox       *mRotate;
	wxCheckBox       *mMobile;
	wxCheckBox       *mSceneStatus;

    wxButton*         mLayout;
    bool              mLayoutOn;

    wxButton*         mLight;
    #define           MAX_LIGHT  5
    int               mWhichLight;

    manipObjectSwitch *mObject;
    int               mWhichObject;
	bool              matched_output;

	int               intermediate_views;
	int               mWhichMaterial;
	bool              all_objects_selected;
	bool              mMIP, mMIP_Invert;

    wxFlexGridSizer  *bfgs;

    wxArrayString     mFilenames;

    void initializeMenu ( void );
    void addButtonBox ( void );
    void removeAll ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createSurfViewFrame ( wxFrame* parentFrame, bool useHistory=true, bool surface=true, bool manipulate=false );
    SurfViewFrame ( bool maximize=false, int w=800, int h=600, bool surface=true, bool manipulate=false );
    ~SurfViewFrame ( void );
    static bool match ( wxString filename, bool surface, bool manipulate );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename, mSurface, manip_flag );
    };

    void loadFile ( const char* const fname );
    void loadFiles ( wxArrayString &filenames );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );

    virtual void OnSaveScreen ( wxCommandEvent& );
	virtual void OnInformation ( wxCommandEvent& unused );

    void OnOpen            ( wxCommandEvent& e );
    void OnSurfView        ( wxCommandEvent& e );
    void OnHideControls    ( wxCommandEvent& e );
    void OnPrintPreview    ( wxCommandEvent& e );
    void OnPrint           ( wxCommandEvent& e );
    virtual void OnInput   ( wxCommandEvent& unused );
    void OnChar            ( wxKeyEvent& e );
    void OnMode            ( wxCommandEvent& e );
    void OnAntialias       ( wxCommandEvent& e );
    void OnStatus          ( wxCommandEvent& e );
    void OnBox             ( wxCommandEvent& e );
	void OnRotate          ( wxCommandEvent& e );
	void OnMobile          ( wxCommandEvent& e );
	void OnSceneStatus     ( wxCommandEvent& e );
	void OnMIP             ( wxCommandEvent& e );
	void OnMIP_Invert      ( wxCommandEvent& e );

    wxString modeName( int );

	virtual void OnNext     ( wxCommandEvent& unused );
	virtual void OnPrevious ( wxCommandEvent& unused );
    void OnAmbient    ( wxCommandEvent& unused );
    void OnBackground ( wxCommandEvent& unused );
    void OnColor      ( wxCommandEvent& unused );
    void OnDiffuse    ( wxCommandEvent& unused );
    void OnLayout     ( wxCommandEvent& unused );
    void OnLight      ( wxCommandEvent& unused );
    void OnMagnify    ( wxCommandEvent& unused );
    void OnObject     ( wxCommandEvent& e );
    void OnOpacity    ( wxCommandEvent& unused );
    void OnSpecular   ( wxCommandEvent& unused );
    void OnSpeed      ( wxCommandEvent& unused );
    void OnSave       ( wxCommandEvent& unused );
	void OnReset      ( wxCommandEvent& unused );
	void OnRemove     ( wxCommandEvent& unused );
	void OnPreview    ( wxCommandEvent& e );
	void OnPoses      ( wxCommandEvent& e );
	void OnPreviewMode( wxCommandEvent& e );
	void OnDeletePose ( wxCommandEvent& unused );
	void OnVRParams   ( wxCommandEvent& unused );
	void OnMaterial   ( wxCommandEvent& e );
	void OnSurfaceStrength(wxCommandEvent &unused);
	void OnMaterialOpacity(wxCommandEvent &unused);
	void OnMaterialColor  (wxCommandEvent &unused);
    void updateObjectSwitch ( void );
	void displaySliceOutputControls ( void );
	void updateIntermediateViews ( void );
	int getIntermediateViews ( void );
	void displayGrayMapControls ( void );

    virtual void  OnAnyAmbientSlider    ( wxScrollEvent& e );
    virtual void  OnAnyBackgroundSlider ( wxScrollEvent& e );
    virtual void  OnAnyColorSlider      ( wxScrollEvent& e );
    virtual void  OnAnyDiffuseSlider    ( wxScrollEvent& e );
    virtual void  OnAnySpecularSlider   ( wxScrollEvent& e );
    virtual void  OnMagnifySlider       ( wxScrollEvent& e );
    virtual void  OnOpacitySlider       ( wxScrollEvent& e );
    virtual void  OnSpeedSlider         ( wxScrollEvent& e );
	virtual void  OnCutDepthSlider      ( wxScrollEvent& e );
	virtual void  OnPixelSizeSlider     ( wxScrollEvent& e );
	virtual void  OnSliceSpacingSlider  ( wxScrollEvent& e );
	virtual void  OnIntermediateViews   ( wxScrollEvent& e );
	virtual void  OnSurfaceStrengthSlider ( wxScrollEvent& e );
	virtual void  OnMaterialOpacitySlider ( wxScrollEvent& e );
	virtual void  OnAnyMatlThresholdSlider( wxScrollEvent& e );
	virtual void  OnAnyMatlColorSlider  ( wxScrollEvent& e );
	virtual void  OnCenterSlider        ( wxScrollEvent& e );
	virtual void  OnWidthSlider         ( wxScrollEvent& e );
	virtual void OnCTLung       ( wxCommandEvent& unused );
	virtual void OnCTSoftTissue ( wxCommandEvent& unused );
	virtual void OnCTBone       ( wxCommandEvent& unused );
    virtual void OnPET          ( wxCommandEvent& unused );
#ifdef __WXX11__
    virtual void  OnUpdateUIMagnifySlider ( wxUpdateUIEvent& unused );
#endif
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( SurfViewFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
