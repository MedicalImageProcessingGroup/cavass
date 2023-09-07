/*
  Copyright 1993-2013, 2016 Medical Image Processing Group
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

#ifndef  __cavass_h
#define  __cavass_h

#if defined (WIN32) || defined (_WIN32)
    #pragma  warning(disable:4786)  //necessary because stl generates longer
                                    // names than bill's compiler can handle!
    #pragma  warning(disable:4996)  //necessary because bill's compiler deprecated stdio.h
#endif

#include  "wx/wx.h"
#include  "wx/clipbrd.h"
#include  "wx/dir.h"
#include  "wx/docview.h"
#include  "wx/file.h"
#include  "wx/fileconf.h"
#include  "wx/filename.h"
#include  "wx/grid.h"
#include  "wx/listctrl.h"
#include  <wx/msgdlg.h>
#include  "wx/notebook.h"
#include  "wx/printdlg.h"
#include  <wx/process.h>
#include  "wx/progdlg.h"
#include  "wx/splash.h"
#include  "wx/tooltip.h"
#include  "wx/txtstrm.h"

#include  <assert.h>
#include  <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif

#ifndef wxSAVE
	#define wxSAVE wxFD_SAVE
	#define wxOPEN wxFD_OPEN
	#define wxOVERWRITE_PROMPT wxFD_OVERWRITE_PROMPT
	#define wxFILE_MUST_EXIST wxFD_FILE_MUST_EXIST
	#define wxMULTIPLE wxFD_MULTIPLE
#endif

#include  <stdlib.h>
#include  <Viewnix.h>
#include  "cv3dv.h"
#include  "Globals.h"
#include  "CavassData.h"

//the following is for detecting memory leaks with vc++.
#ifdef  _DEBUG
    #include  <crtdbg.h>
    #define   DEBUG_NEW  new(_NORMAL_BLOCK ,__FILE__, __LINE__)
    #define   new        DEBUG_NEW
#endif

#define DETACH_CONTROLS_FROM_SIZERS

#include  "ColorControls.h"
#include  "ExportFrame.h"
#include  "FileHistory.h"
#include  "EasyHeaderFrame.h"
#include  "FromDicomFrame.h"
#include  "FuzzCompFrame.h"
#include  "GrayMapControls.h"
#include  "InformationDialog.h"
#include  "InputHistoryDialog.h"
#include  "IRFCFrame.h"
#include  "ITKFilterFrame.h"
#include  "MainCanvas.h"
#include  "MainFileDropTarget.h"
#include  "MainFrame.h"
#include  "MainPrint.h"
#include  "OverlayFrame.h"
#include  "OverlayPrint.h"
#include  "Owen2dFrame.h"
#include  "Preferences.h"
#include  "ProcessManager.h"
#include  "SaveScreenControls.h"
#include  "Segment2dFrame.h"
#include  "SetIndexControls.h"
#include  "MagnifyControls.h"
#include  "OpacityControls.h"
#include  "RegisterFrame.h"
#include  "ShowScreenFrame.h"
#include  "SurfViewFrame.h"
#include  "SurfViewPrint.h"
#include  "ThresholdControls.h"
#include  "ThresholdFrame.h"
#include  "ThresholdPrint.h"
#include  "AlgebraFrame.h"
#include  "../analyze/DensityFrame.h"
#include  "../analyze/ROIStatisticalFrame.h"
#include  "../analyze/KinematicsInterFrame.h"
#include  "../analyze/KinematicsIntraFrame.h"
#include  "../analyze/AnalyzeStaticFrame.h"

#endif
